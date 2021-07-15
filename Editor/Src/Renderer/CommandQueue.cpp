
static DWORD WINAPI ProcessInFlightCommandListsThreadProc(LPVOID lp_param);

RenderError
CommandQueue::Init(D3D12_COMMAND_LIST_TYPE queue_type)
{
    RenderError result = RenderError::Success;
    ID3D12Device *d3d_device = device::GetDevice();
    
    type = queue_type;
    
    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type  = type;
    AssertHr(d3d_device->CreateCommandQueue(&queue_desc, IIDE(&handle)));
    
    fence_value = 0;
    AssertHr(d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IIDE(&fence)));
    fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fence_event && "Failed to create fence event!\n");
    
    // Shouldn't need *that* many command lists duruing runtime, 
    // so just use 10 for now...
    _cmd_list_allocator.Init(10);
    _in_flight_cmd_lists.Init();
    _available_cmd_lists.Init();
    
    InitializeCriticalSectionAndSpinCount(&_cs_lock_in_flight, 1024);
    _InterlockedIncrement(&_is_active);
    _process_thread = CreateThread(NULL, 0, ProcessInFlightCommandListsThreadProc, (void*)this, 0, NULL);
    assert(_process_thread != NULL);
    
    return result;
}

RenderError 
CommandQueue::Free()
{
    RenderError result = RenderError::Success;
    _InterlockedDecrement(&_is_active);
    _in_flight_cmd_lists.Free();
    _available_cmd_lists.Free();
    _cmd_list_allocator.Free();
    DeleteCriticalSection(&_cs_lock_in_flight);
    D3D_RELEASE(handle);
    return result;
}

void 
CommandQueue::Flush()
{
    EnterCriticalSection(&_cs_lock_in_flight);
    if (arrlen(_in_flight_cmd_lists._list) > 0)
        SleepConditionVariableCS(&_cv_in_flight, &_cs_lock_in_flight, INFINITE);
    
    WaitForFenceValue(fence_value);
    
    LeaveCriticalSection(&_cs_lock_in_flight);
}

CommandList*
CommandQueue::GetCommandList()
{
    CommandList* result = 0;
    if (arrlen(_available_cmd_lists._list) > 0)
    {
        result = _available_cmd_lists.Pop();
    }
    else
    {
        result = _cmd_list_allocator.Allocate();
        result->Init(type);
    }
    return result;
}

u64
CommandQueue::ExecuteCommandLists(CommandList **cmd_lists, i32 count)
{
    ResourceStateTracker::Lock();
    
    // Command Lists that need to be put back on the command list qeueu
    // 2x since each command list will have a pending list
    // The pending command list resolves the initial state of resources that
    // are required for the actual command list
    CommandList **to_be_queued = 0;
    arrsetcap(to_be_queued, count * 2);
    
    // MIPS command list
    CommandList **mips_list = 0;
    arrsetcap(mips_list, count);
    
    // Lists that need to be executed
    ID3D12CommandList **to_be_executed = 0;
    arrsetcap(to_be_executed, count * 2);
    
    for (i32 i = 0; i < count; ++i)
    {
        CommandList *list = cmd_lists[i];
        CommandList *pending_list = GetCommandList();
        bool has_pending_barriers = list->ClosePending(pending_list);
        pending_list->Close();
        // If there are no pending barriers, then do not need to
        // execute the command list
        if (has_pending_barriers)
            arrput(to_be_executed, pending_list->_handle);
        arrput(to_be_executed, list->_handle);
        
        arrput(to_be_queued, pending_list);
        arrput(to_be_queued, list);
        
        // Accumulate MIPS lists
        CommandList *mips_cmd_list = list->_compute_command_list;
        if (mips_cmd_list)
        {
            arrput(mips_list, mips_cmd_list);
        }
    }
    
    UINT list_to_execute_count = (UINT)arrlen(to_be_executed);
    handle->ExecuteCommandLists(list_to_execute_count, to_be_executed);
    u64 fence_val = Signal();
    
    ResourceStateTracker::Unlock();
    
    for (u32 i = 0; i < (u32)arrlen(to_be_queued); ++i)
        _in_flight_cmd_lists.Push({ fence_val, to_be_queued[i] });
    
    // Execute MIPS lists on the compute command queue
    if (arrlen(mips_list) > 0)
    {
        CommandQueue *compute_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
        compute_queue->Wait(this);
        compute_queue->ExecuteCommandLists(mips_list, (i32)arrlen(mips_list));
    }
    
    return fence_val;
}

u64
CommandQueue::Signal()
{
    u64 result = ++fence_value;
    AssertHr(handle->Signal(fence, result));
    return result;
}

RenderError
CommandQueue::WaitForFenceValue(u64 fencevalue)
{
    RenderError result = RenderError::Success;
    
    if (fence->GetCompletedValue() < fencevalue)
    {
        //AssertHr(fence->SetEventOnCompletion(fence_value, fence_event));
        //WaitForSingleObject(fence_event, INFINITE);
        
        HANDLE event = ::CreateEvent( NULL, FALSE, FALSE, NULL );
        AssertHr(fence->SetEventOnCompletion(fencevalue, event));
        WaitForSingleObject(event, INFINITE);
        ::CloseHandle(event);
    }
    return result;
}

void 
CommandQueue::Wait(CommandQueue *other)
{
    handle->Wait(other->fence, other->fence_value);
}

static DWORD WINAPI ProcessInFlightCommandListsThreadProc(LPVOID lp_param)
{
    CommandQueue *queue = (CommandQueue*)lp_param;
    queue->ProcessInFlightCommandLists();
    return 0;
}

void 
CommandQueue::ProcessInFlightCommandLists()
{
    while (_is_active > 0)
    {
        CommandListEntry entry = {};
        
        EnterCriticalSection(&_cs_lock_in_flight);
        
        while (_in_flight_cmd_lists.Pop(&entry))
        {
            u64 val = entry.fence_value;
            CommandList *list = entry.list;
            
            WaitForFenceValue(val);
            
            list->Reset();
            _available_cmd_lists.Push(list);
        }
        
        LeaveCriticalSection(&_cs_lock_in_flight);
        
        WakeConditionVariable(&_cv_in_flight);
        Sleep(0); // Yield remainder of time slice
    }
}

//-----------------------------------------------------------------------------------
// Command List Allocator implementation 

void 
CommandListAllocator::Init(u32 max)
{
    _max_allocations_per_page = max;
    _current_page = 0;
    _page_list = 0;
    
    Page page = {};
    page.Init(_max_allocations_per_page);
    arrput(_page_list, page);
}

void 
CommandListAllocator::Free()
{
    for (u32 i = 0; i < (u32)arrlen(_page_list); ++i)
    {
        _page_list[i].Free();
    }
    arrfree(_page_list);
}

CommandList* 
CommandListAllocator::Allocate(u32 count)
{
    CommandList *result = 0;
    if (!(result = _page_list[_current_page].Allocate(count)))
    {
        _max_allocations_per_page = fast_max(_max_allocations_per_page, count);
        _current_page = (u32)arrlen(_page_list);
        
        Page page = {};
        page.Init(_max_allocations_per_page);
        arrput(_page_list, page);
        result = _page_list[_current_page].Allocate(count);
    }
    return result;
}

void 
CommandListAllocator::Page::Init(u32 max)
{
    _list = 0;
    arrsetcap(_list, max);
    _max   = max;
    _count = 0;
}

void 
CommandListAllocator::Page::Free()
{
    arrfree(_list);
}

CommandList* 
CommandListAllocator::Page::Allocate(u32 count)
{
    CommandList *result = 0;
    if (_count + count <= _max)
    {
        // Place empty command lists into the array
        // NOTE(Dustin): Is this unnecessary? arrsetcap
        // should auto reserve space, so we could just return
        // a pointer w/out zero initializing the structure
        CommandList list = {};
        for (u32 i = 0; i < count; ++i)
        {
            arrput(_list, list);
        }
        
        // save the pointer to the first list. the caller can then iterate over
        // the list as needed
        result = _list + _count;
        _count += count;
    }
    return result;
}
