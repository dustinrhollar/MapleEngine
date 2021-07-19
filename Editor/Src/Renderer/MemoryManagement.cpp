
//-----------------------------------------------------------------------------------------------//
// Upload Buffer

void 
UploadBuffer::Init(u64 page_size)
{
    _page_size = page_size;
    _active_page = INVALID_PAGE_ID;
    
    for (u32 i = 0; i < (u32)arrlen(_page_pool); ++i)
    {
        _page_pool[i].Free();
    }
    arrfree(_page_pool);
}

void 
UploadBuffer::Free()
{
    _page_size = 0;
    arrfree(_avail_pages);
}

UploadBuffer::Allocation
UploadBuffer::Allocate(u64 size, u64 alignment)
{
    assert(size <= _page_size);
    if ((_active_page == INVALID_PAGE_ID) || !_page_pool[_active_page].HasSpace(size, alignment))
    {
        _active_page = RequestPage();
        assert(_active_page != INVALID_PAGE_ID);
    }
    return _page_pool[_active_page].Allocate(size, alignment);
}

UploadBuffer::PAGE_ID 
UploadBuffer::RequestPage()
{
    PAGE_ID result = INVALID_PAGE_ID;
    
    if (arrlen(_avail_pages) > 0)
    {
        result = _avail_pages[arrlen(_avail_pages) - 1];
        arrdel(_avail_pages, arrlen(_avail_pages) - 1);
    }
    else
    {
        result = (PAGE_ID)arrlen(_page_pool);
        Page page = {};
        page.Init(_page_size);
        arrput(_page_pool, page);
    }
    return result;
}

void UploadBuffer::Reset()
{
    _active_page = INVALID_PAGE_ID;
    arrsetlen(_avail_pages, 0);
    
    for (u32 i = 0; i < arrlen(_page_pool); ++i)
    {
        _page_pool[i].Reset();
        arrput(_avail_pages, i);
    }
}

//-----------------------------------------------------------------------------------------------//
// Upload Buffer Page

void 
UploadBuffer::Page::Init(u64 page_size)
{
    ID3D12Device *d3d_device = device::GetDevice();
    _page_size = page_size;
    _offset = 0;
    
    D3D12_HEAP_PROPERTIES dh_prop = d3d::GetHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC ds_desc = d3d::GetBufferResourceDesc(_page_size);
    AssertHr(d3d_device->CreateCommittedResource(&dh_prop,
                                                 D3D12_HEAP_FLAG_NONE,
                                                 &ds_desc,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ,
                                                 nullptr,
                                                 IIDE(&_rsrc)));
    
    _gpu_ptr = _rsrc->GetGPUVirtualAddress();
    _rsrc->Map(0, nullptr, &_base);
}

void 
UploadBuffer::Page::Free()
{
    _rsrc->Unmap(0, nullptr);
    _base = 0;
    _gpu_ptr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

FORCE_INLINE bool 
UploadBuffer::Page::HasSpace(u64 size, u64 alignment)
{
    u64 aligned_size   = memory_align(size, alignment);
    u64 aligned_offset = memory_align(_offset, alignment);
    return aligned_offset + aligned_size <= _page_size;
}

UploadBuffer::Allocation 
UploadBuffer::Page::Allocate(u64 size, u64 alignment)
{
    assert(HasSpace(size, alignment));
    
    u64 aligned_size = memory_align(size, alignment);
    _offset = memory_align(_offset, alignment);
    
    Allocation result = {};
    result.cpu = (u8*)_base + _offset;
    result.gpu = _gpu_ptr + _offset;
    
    _offset += aligned_size;
    
    return result;
}

void 
UploadBuffer::Page::Reset()
{
    _offset = 0;
}

//-----------------------------------------------------------------------------------------------//
// Descriptor Allocator

void 
DescriptorAllocator::Init(D3D12_DESCRIPTOR_HEAP_TYPE type, u32 count_per_heap)
{
    _heap_type = type;
    _descriptors_per_heap = count_per_heap;
    
    BOOL ret = InitializeCriticalSectionAndSpinCount(&_cs_lock, 1024);
    assert(ret > 0); // this shouldn't ever happen
}

void 
DescriptorAllocator::Free()
{
}

DescriptorAllocation 
DescriptorAllocator::Allocate(u32 num_descriptors)
{
    DescriptorAllocation result = {};
    
    EnterCriticalSection(&_cs_lock);
    {
        for (u32 i = 0; i < (u32)arrlen(_avail_heaps); ++i)
        {
            DescriptorAllocatorPage *page = _heap_pool + _avail_heaps[i];
            result = page->Allocate(num_descriptors);
            
            if (page->NumFreeHandles() == 0)
            {
                arrdelswap(_avail_heaps, i);
                --i;
            }
            
            if (!result.IsNull()) break; // Successfully allocated
        }
        
        if (result.IsNull())
        { // Could not find an available allocation, alloc new page
            _descriptors_per_heap = fast_max(_descriptors_per_heap, num_descriptors);
            PAGE_ID new_page = CreateAllocatorPage();
            result = _heap_pool[new_page].Allocate(num_descriptors);
        }
    }
    LeaveCriticalSection(&_cs_lock);
    
    return result;
}

void 
DescriptorAllocator::ReleaseStaleDescriptors()
{
    EnterCriticalSection(&_cs_lock);
    {
        for (u32 i = 0; i < (u32)arrlen(_heap_pool); ++i)
        {
            DescriptorAllocatorPage *page = &_heap_pool[i];
            page->ReleaseStaleDescriptors();
            
            if (page->NumFreeHandles() > 0) 
                AddAvailableHeap(i);
        }
    }
    LeaveCriticalSection(&_cs_lock);
}

void 
DescriptorAllocator::AddAvailableHeap(PAGE_ID avail_page)
{
    for (u32 i = 0; i < (u32)arrlen(_avail_heaps); ++i)
    {
        if (avail_page == _avail_heaps[i]) return;
    }
    arrput(_avail_heaps, avail_page);
}

DescriptorAllocator::PAGE_ID 
DescriptorAllocator::CreateAllocatorPage()
{
    PAGE_ID result = (PAGE_ID)arrlen(_heap_pool);
    
    DescriptorAllocatorPage new_page = {};
    new_page.Init(_heap_type, _descriptors_per_heap, this, result);
    arrput(_heap_pool, new_page);
    
    AddAvailableHeap(result);
    return result;
}

//-----------------------------------------------------------------------------------------------//
// Descriptor Allocator Page

void 
DescriptorAllocatorPage::Init(D3D12_DESCRIPTOR_HEAP_TYPE type, u32 count_per_heap, DescriptorAllocator *allocator, u32 id)
{
    ID3D12Device *d3d_device = device::GetDevice();
    
    _heap_type = type;
    _num_descriptors_in_heap = count_per_heap;
    
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = _heap_type;
    heap_desc.NumDescriptors = _num_descriptors_in_heap;
    AssertHr(d3d_device->CreateDescriptorHeap(&heap_desc, IIDE(&_descriptor_heap)));
    
    _base_descriptor = _descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    _descriptor_increment_size = d3d_device->GetDescriptorHandleIncrementSize(_heap_type);
    _num_free_handles = _num_descriptors_in_heap;
    
    _allocator = allocator;
    _id = id;
    
    InitializeCriticalSectionAndSpinCount(&_cs_lock, 1024);
    
    // Initializes the free lists
    AddNewBlock(0, _num_free_handles);
}

void 
DescriptorAllocatorPage::Free()
{
    arrfree(_free_list);
    arrfree(_stale_descriptor_queue);
    D3D_RELEASE(_descriptor_heap);
    _base_descriptor = {};
    _descriptor_increment_size = 0;
    _num_descriptors_in_heap = 0;
    _num_free_handles = 0;
    _allocator = 0;
    _id = DescriptorAllocator::INVALID_PAGE_ID;
    DeleteCriticalSection(&_cs_lock);
}

DescriptorAllocation 
DescriptorAllocatorPage::Allocate(u32 num_descriptors)
{
    u64 offset;
    
    EnterCriticalSection(&_cs_lock);
    {
        if (num_descriptors > _num_free_handles) return {};
        
        // First-Fit Search, not great, but works for now...
        // TODO(Dustin): Binary Search + Best Fit
        u32 block_idx = 0;
        for (block_idx = 0; block_idx < (u32)arrlen(_free_list); ++block_idx)
        {
            if (num_descriptors <= _free_list[block_idx].size)
                break;
        }
        
        // Couldn't find an allocation that could fullfill the request...
        if (block_idx >= (u32)arrlen(_free_list)) return {};
        
        FreeBlockInfo selected_block = _free_list[block_idx];
        arrdel(_free_list, block_idx);
        
        // Split the block, if possible
        u32 split_offset = selected_block.offset + num_descriptors;
        u32 split_size = selected_block.size - num_descriptors;
        
        if (split_size > 0)
            AddNewBlock(split_offset, split_size);
        
        _num_free_handles -= num_descriptors;
        
        offset = selected_block.offset;
    }
    LeaveCriticalSection(&_cs_lock);
    
    D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
    handle.ptr = _base_descriptor.ptr + offset * _descriptor_increment_size;
    
    DescriptorAllocation result = {};
    result.Init(handle, num_descriptors, _descriptor_increment_size, _allocator, _id);
    return result;
}

u32 
DescriptorAllocatorPage::NumFreeHandles()
{
    return _num_free_handles;
}

bool 
DescriptorAllocatorPage::HasSpace(u32 num_descriptors)
{
    // Looks for a single occurance within the free list that can
    // contain the number of descriptors
    bool result = false;
    for (u32 i = 0; i < arrlen(_free_list); ++i)
    {
        if (_free_list[i].size >= num_descriptors)
        {
            result = true;
            break;
        }
    }
    return result;
}

// @brief Return a descripor back to the heap.
// Release does not immediately free descriptors, but adds them to a stale
// descriptor queue. Stale allocations are returned all at once with the
// call to ReleaseStaleAllocations.
// //NOTE(Dustin): Will need to change the descriptor handle to be more appropriate
void 
DescriptorAllocatorPage::Release(DescriptorAllocation *descriptor_handle)
{
    u32 offset = ComputeOffset(descriptor_handle->_descriptor);
    
    StaleDescriptorInfo stale_info = {};
    stale_info.offset = offset;
    stale_info.size = descriptor_handle->_num_handles;
    
    EnterCriticalSection(&_cs_lock);
    {
        arrput(_stale_descriptor_queue, stale_info);
    }
    LeaveCriticalSection(&_cs_lock);
}

u32 
DescriptorAllocatorPage::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    return (u32)(handle.ptr - _base_descriptor.ptr) / _descriptor_increment_size;
}

// Add a new block to the free list
void 
DescriptorAllocatorPage::AddNewBlock(u32 offset, u32 num_descriptors)
{
    FreeBlockInfo block = {};
    block.offset = offset;
    block.size = num_descriptors;
    
    u32 ins_idx = 0;
    for (ins_idx = 0; ins_idx < (u32)arrlen(_free_list); ++ins_idx)
    {
        if (offset <= _free_list[ins_idx].offset)
            break;
    }
    arrins(_free_list, ins_idx, block);
}

// Free a block a descriptors. This will merge free blocks in the list 
// to form larger blocks that can be reused.
void 
DescriptorAllocatorPage::FreeBlock(u32 offset, u32 num_descriptors)
{
    _num_free_handles += num_descriptors;
    
    // Find the first element in the free list where the offset
    // is greater than the new block being added to the list
    i32 ins_index = (i32)arrlen(_free_list); // assume end of list
    for (i32 i = 0; i < (i32)arrlen(_free_list); ++i)
    {
        if (_free_list[i].offset > offset)
        {
            ins_index = i;
            break;
        }
    }
    
    i32 prev_block = (i32)arrlen(_free_list); // assume no block before
    i32 next_block = -1;                      // assume no block after
    
    if (ins_index > 0)
    {
        prev_block = ins_index - 1;
    }
    
    // Since the new block has not been inserted into the
    // list, the "next" block currently sits in the ins index
    if (ins_index < (i32)arrlen(_free_list))
    {
        next_block = ins_index;
    }
    
    // Insert into left block, if offsets coincide, and can merge blocks
    bool merge = false;       // did any merge happen?
    bool merge_left = false;  // did a left merge happen?
    if (prev_block != (i32)arrlen(_free_list) &&
        offset == _free_list[prev_block].offset + _free_list[prev_block].size)
    {
        // The previous block is exactly behind this block
        //
        // prevblock.offset                 offset
        // |                                |
        // | <------ prevblock.size ------> | <------ size ------> | 
        //
        //
        _free_list[prev_block].size += num_descriptors;
        merge_left = true;
        merge = true;
    }
    
    // Merge right block if offsets coincide
    if (next_block >= 0 &&
        offset + num_descriptors == _free_list[next_block].offset)
    {
        // The next block is exactly after this block
        //
        // offset                 nextblock.offset
        // |                      |
        // | <------ size ------> | <------ nextblock.size ------> | 
        //
        //
        if (merge_left)
        { // Merged left block, need to remove this block and merge left again
            _free_list[prev_block].size += _free_list[next_block].size;
            arrdel(_free_list, next_block);
        }
        else
        { // can just merge directly into this block
            _free_list[next_block].offset = offset;
            _free_list[next_block].size += num_descriptors;
        }
        merge = true;
    }
    
    if (!merge)
    { // no merge occurred, need to insert a new block into the list
        AddNewBlock(offset, num_descriptors);
    }
}

// Return stale descriptors back to the heap
void 
DescriptorAllocatorPage::ReleaseStaleDescriptors()
{
    EnterCriticalSection(&_cs_lock);
    {
        while ((arrlen(_stale_descriptor_queue) > 0))
        {
            StaleDescriptorInfo *stale_info = &_stale_descriptor_queue[0];
            
            u32 offset = stale_info->offset;
            u32 num_descriptors = stale_info->size;
            FreeBlock(offset, num_descriptors);
            
            arrdel(_stale_descriptor_queue, 0);
        }
    }
    LeaveCriticalSection(&_cs_lock);
}

//-----------------------------------------------------------------------------------------------//
// Descriptor Allocation

void 
DescriptorAllocation::Init(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, 
                           u32                         num_handles, 
                           u32                         descriptor_size, 
                           struct DescriptorAllocator *allocator,
                           u32                         page_id)
{
    _descriptor = descriptor;
    _num_handles = num_handles;
    _descriptor_size = descriptor_size;
    _allocator = allocator;
    _page_id = page_id;
}

void 
DescriptorAllocation::Free()
{
    if (!IsNull() && _allocator)
    {
        DescriptorAllocatorPage *page = &_allocator->_heap_pool[_page_id];
        page->Release(this);
        
        _descriptor = {};
        _num_handles = 0;
        _descriptor_size = 0;
        _allocator = 0;
        _page_id = DescriptorAllocator::INVALID_PAGE_ID;
    }
}

// Get a descriptor at a particular offset in the allocation.
D3D12_CPU_DESCRIPTOR_HANDLE 
DescriptorAllocation::GetDescriptorHandle(u32 offset)
{
    assert( offset < _num_handles );
    return { _descriptor.ptr + ( _descriptor_size * offset ) };
}

bool 
DescriptorAllocation::IsNull()
{
    return _descriptor.ptr == 0;
}

struct DescriptorAllocatorPage* 
DescriptorAllocation::GetDescriptorAllocatorPage()
{
    assert(_allocator && _page_id != DescriptorAllocator::INVALID_PAGE_ID);
    DescriptorAllocatorPage *page = &_allocator->_heap_pool[_page_id];
    return page;
}

//-----------------------------------------------------------------------------------------------//
// Dynamic Descriptor Heap

void 
DynamicDescriptorHeap::Init(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, u32 num_descriptors_per_heap)
{
    ID3D12Device *d3d_device = device::GetDevice();
    
    _heap_type = heap_type;
    _num_descriptors_per_heap = num_descriptors_per_heap;
    _descriptor_table_bitmask = 0;
    _stale_cbv_bitmask = 0;
    _stale_srv_bitmask = 0;
    _stale_uav_bitmask = 0;
    _stale_descriptor_table_bitmask = 0;
    _current_gpu_handle = {};
    _current_cpu_handle = {};
    _num_free_handles = 0;
    _descriptor_increment_size = d3d_device->GetDescriptorHandleIncrementSize(_heap_type);
    _descriptor_handle_cache = (D3D12_CPU_DESCRIPTOR_HANDLE*)SysAlloc(sizeof(D3D12_CPU_DESCRIPTOR_HANDLE) * _num_descriptors_per_heap);
    
    _avail_descriptors    = 0;
    _descriptor_heap_pool = 0;
}

void 
DynamicDescriptorHeap::Free()
{
    _num_descriptors_per_heap = 0;
    _descriptor_table_bitmask = 0;
    _stale_cbv_bitmask = 0;
    _stale_srv_bitmask = 0;
    _stale_uav_bitmask = 0;
    _stale_descriptor_table_bitmask = 0;
    _current_gpu_handle = {};
    _current_cpu_handle = {};
    _num_free_handles = 0;
    _descriptor_increment_size = 0;
    SysFree(_descriptor_handle_cache);
    
    for (u32 i = 0; i < (u32)arrlen(_descriptor_heap_pool); ++i)
    {
        if (_descriptor_heap_pool[i]) D3D_RELEASE(_descriptor_heap_pool[i]);
    }
    
    for (u32 i = 0; i < (u32)arrlen(_avail_descriptors); ++i)
    {
        if (_avail_descriptors[i]) D3D_RELEASE(_avail_descriptors[i]);
    }
}

// Stages a contiguous range of CPU descriptors. Descriptors are not copied to the
// GPU visible desccriptor heap until the CommitStagedDescriptors func is called
void 
DynamicDescriptorHeap::StageDescriptors(u32 root_param_idx, u32 offset, u32 num_descriptors, D3D12_CPU_DESCRIPTOR_HANDLE src_descriptor)
{
    // Cannot stage more than the maximum number od descriptors per heap
    // Cannot stage more than MaxDescriptorTables root params
    assert(num_descriptors <= _num_descriptors_per_heap || root_param_idx < MAX_DESCRIPTOR_TABLES);
    
    // Check number of descriptors to copy does not exceed the number of descriptors
    // expected in the descriptor table
    DescriptorTableCache *table_cache = &_descriptor_table_cache[root_param_idx];
    assert((offset + num_descriptors) <= table_cache->_num_descriptors);
    
    D3D12_CPU_DESCRIPTOR_HANDLE* dst_descriptor = (table_cache->_base_descriptor + offset);
    for (u32 i = 0; i < num_descriptors; ++i)
    {
        dst_descriptor[i] = d3d::GetCpuDescriptorHandle(src_descriptor, _descriptor_increment_size, i);
    }
    
    _stale_descriptor_table_bitmask |= (1 << root_param_idx);
}

void 
DynamicDescriptorHeap::StageInlineCBV(u32 root_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_loc)
{
    assert(root_index < MAX_DESCRIPTOR_TABLES);
    _inline_cbv[root_index] = buffer_loc;
    _stale_cbv_bitmask |= ( 1 << root_index );
}

void 
DynamicDescriptorHeap::StageInlineSRV(u32 root_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_loc)
{
    assert(root_index < MAX_DESCRIPTOR_TABLES);
    _inline_srv[root_index] = buffer_loc;
    _stale_srv_bitmask |= ( 1 << root_index );
}

void 
DynamicDescriptorHeap::StageInlineUAV(u32 root_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_loc)
{
    assert(root_index < MAX_DESCRIPTOR_TABLES);
    _inline_uav[root_index] = buffer_loc;
    _stale_uav_bitmask |= ( 1 << root_index );
}

FORCE_INLINE void 
SetRootDescriptorGraphicsWrapper(ID3D12GraphicsCommandList *list, UINT root_index, D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
    list->SetGraphicsRootDescriptorTable(root_index, handle);
}

FORCE_INLINE void 
SetRootDescriptorComputeWrapper(ID3D12GraphicsCommandList *list, UINT root_index, D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
    list->SetComputeRootDescriptorTable(root_index, handle);
}

FORCE_INLINE void
SetGraphicsRootCBVWrapper(ID3D12GraphicsCommandList *list,
                          UINT root_index,
                          D3D12_GPU_VIRTUAL_ADDRESS buffer_loc)
{
    list->SetGraphicsRootConstantBufferView(root_index, buffer_loc);
}

FORCE_INLINE void
SetGraphicsRootSRVWrapper(ID3D12GraphicsCommandList *list,
                          UINT root_index,
                          D3D12_GPU_VIRTUAL_ADDRESS buffer_loc)
{
    list->SetGraphicsRootShaderResourceView(root_index, buffer_loc);
}

FORCE_INLINE void
SetGraphicsRootUAVWrapper(ID3D12GraphicsCommandList *list,
                          UINT root_index,
                          D3D12_GPU_VIRTUAL_ADDRESS buffer_loc)
{
    list->SetGraphicsRootUnorderedAccessView(root_index, buffer_loc);
}

FORCE_INLINE void
SetComputeRootCBVWrapper(ID3D12GraphicsCommandList *list,
                         UINT root_index,
                         D3D12_GPU_VIRTUAL_ADDRESS buffer_loc)
{
    list->SetComputeRootConstantBufferView(root_index, buffer_loc);
}

FORCE_INLINE void
SetComputeRootSRVWrapper(ID3D12GraphicsCommandList *list,
                         UINT root_index,
                         D3D12_GPU_VIRTUAL_ADDRESS buffer_loc)
{
    list->SetComputeRootShaderResourceView(root_index, buffer_loc);
}

FORCE_INLINE void
SetComputeRootUAVWrapper(ID3D12GraphicsCommandList *list,
                         UINT root_index,
                         D3D12_GPU_VIRTUAL_ADDRESS buffer_loc)
{
    list->SetComputeRootUnorderedAccessView(root_index, buffer_loc);
}

/*
* Copy all of the descriptors to the GPU visible descriptor heap
* and bind the descriptor heap and the descriptor tables to the
* command list. The passed-in function object is used to set the
* GPU visible descriptors on the command list. Two posible functions:
* - Before a draw:     ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable
* - Before a dispatch: ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
*/
void 
DynamicDescriptorHeap::CommitDescriptorTables(CommandList *cmd_list, void (*set_root)(ID3D12GraphicsCommandList *list, UINT root_index, D3D12_GPU_DESCRIPTOR_HANDLE handle))
{
    ID3D12Device *d3d_device = device::GetDevice();
    
    // Compute the number of descriptors that need to be copied 
    u32 num_desc_to_commit  = ComputeStaleDescriptorCount();
    if (num_desc_to_commit > 0)
    {
        ID3D12GraphicsCommandList *list_handle = cmd_list->_handle;
        assert(list_handle);
        
        if (!_current_descriptor_heap || _num_free_handles < num_desc_to_commit)
        {
            _current_descriptor_heap = RequestDescriptorHeap();
            _current_cpu_handle = _current_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
            _current_gpu_handle = _current_descriptor_heap->GetGPUDescriptorHandleForHeapStart();
            _num_free_handles = _num_descriptors_per_heap;
            
            cmd_list->SetDescriptorHeap(_heap_type, _current_descriptor_heap);
            
            // When updating the descriptor heap on the command list, all descriptor
            // tables must be (re)recopied to the new descriptor heap (not just
            // the stale descriptor tables).
            _stale_descriptor_table_bitmask = _descriptor_table_bitmask;
        }
        
        DWORD root_idx;
        // Scan from LSB to MSB for a bit set in staleDescriptorsBitMask
        while (_BitScanForward( &root_idx, _stale_descriptor_table_bitmask))
        {
            UINT num_src_desc = _descriptor_table_cache[root_idx]._num_descriptors;
            D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorHandles = _descriptor_table_cache[root_idx]._base_descriptor;
            
            D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[] =
            {
                _current_cpu_handle
            };
            UINT pDestDescriptorRangeSizes[] =
            {
                num_src_desc
            };
            
            // Copy the staged CPU visible descriptors to the GPU visible descriptor heap.
            d3d_device->CopyDescriptors(1, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
                                        num_src_desc, pSrcDescriptorHandles, nullptr, _heap_type);
            
            // Set the descriptors on the command list using the passed-in setter function.
            set_root(list_handle, root_idx, _current_gpu_handle);
            
            // Offset current CPU and GPU descriptor handles.
            _current_cpu_handle = d3d::GetCpuDescriptorHandle(_current_cpu_handle, _descriptor_increment_size, num_src_desc);
            _current_gpu_handle = d3d::GetGpuDescriptorHandle(_current_gpu_handle, _descriptor_increment_size, num_src_desc);
            
            _num_free_handles -= num_src_desc;
            
            // Flip the stale bit so the descriptor table is not recopied again unless it is updated with a new descriptor.
            _stale_descriptor_table_bitmask ^= (1 << root_idx);
        }
    }
}

void
DynamicDescriptorHeap::CommitInlineDescriptors(CommandList *cmd_list,
                                               D3D12_GPU_VIRTUAL_ADDRESS *buffer_locs,
                                               u32 *bitmask, 
                                               void (*set_root)(ID3D12GraphicsCommandList *list, UINT root_index, D3D12_GPU_VIRTUAL_ADDRESS handle))
{
    if (bitmask != 0)
    {
        unsigned long root_index;
        while (_BitScanForward(&root_index, *bitmask))
        {
            set_root(cmd_list->_handle, root_index, buffer_locs[root_index]);
            
            // Flip the stale bit so the descriptor is not recopied again unless it is updated with a new descriptor.
            *bitmask ^= (1 << root_index);
        }
    }
}

void 
DynamicDescriptorHeap::CommitStagedDescriptorsForDraw(struct CommandList *cmd_list)
{
    CommitDescriptorTables(cmd_list, &SetRootDescriptorGraphicsWrapper);
    CommitInlineDescriptors(cmd_list, _inline_cbv, &_stale_cbv_bitmask, &SetGraphicsRootCBVWrapper);
    CommitInlineDescriptors(cmd_list, _inline_srv, &_stale_srv_bitmask, &SetGraphicsRootSRVWrapper);
    CommitInlineDescriptors(cmd_list, _inline_uav, &_stale_uav_bitmask, &SetGraphicsRootUAVWrapper);
}

void 
DynamicDescriptorHeap::CommitStagedDescriptorsForDispatch(struct CommandList *cmd_list)
{
    CommitDescriptorTables(cmd_list, &SetRootDescriptorComputeWrapper);
    CommitInlineDescriptors(cmd_list, _inline_cbv, &_stale_cbv_bitmask, &SetComputeRootCBVWrapper);
    CommitInlineDescriptors(cmd_list, _inline_srv, &_stale_srv_bitmask, &SetComputeRootSRVWrapper);
    CommitInlineDescriptors(cmd_list, _inline_uav, &_stale_uav_bitmask, &SetComputeRootUAVWrapper);
}

/*
* Copies a single CPU visible descriptor to a GPU visible descriptor heap.
* Useful for the
* - ID3D12GraphicsCommandList::ClearUnorderedAccessViewFloat
* - ID3D12GraphicsCommandList::ClearUnorderedAccessViewUint
* methods which require both a CPU and GPU visible descriptors for a UAV
* resource. 
*
*/
D3D12_GPU_DESCRIPTOR_HANDLE 
DynamicDescriptorHeap::CopyDescriptor(CommandList *cmd_list, D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor)
{
    ID3D12Device *d3d_device = device::GetDevice();
    
    D3D12_GPU_DESCRIPTOR_HANDLE result = {};
    
    if (!_current_descriptor_heap || _num_free_handles < 1)
    {
        _current_descriptor_heap = RequestDescriptorHeap();
        _current_cpu_handle = _current_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
        _current_gpu_handle = _current_descriptor_heap->GetGPUDescriptorHandleForHeapStart();
        _num_free_handles = _num_descriptors_per_heap;
        
        cmd_list->SetDescriptorHeap(_heap_type, _current_descriptor_heap);
        
        // When updating the descriptor heap on the command list, all descriptor
        // tables must be (re)recopied to the new descriptor heap (not just
        // the stale descriptor tables).
        _stale_descriptor_table_bitmask = _descriptor_table_bitmask;
    }
    
    result = _current_gpu_handle;
    d3d_device->CopyDescriptorsSimple(1, _current_cpu_handle, cpu_descriptor, _heap_type);
    
    _current_cpu_handle = d3d::GetCpuDescriptorHandle(_current_cpu_handle, _descriptor_increment_size, 1);
    _current_gpu_handle = d3d::GetGpuDescriptorHandle(_current_gpu_handle, _descriptor_increment_size, 1);
    _num_free_handles -= 1;
    
    return result;
}

// Parse root signature to determine which root parameters
// contain descriptor tables and determine the number of 
// descriptors needed for each table
void 
DynamicDescriptorHeap::ParseRootSignature(RootSignature *root_sig)
{
    // if the root sig changes, must rebind all descriptors to the command list
    _stale_descriptor_table_bitmask = 0;
    D3D12_ROOT_SIGNATURE_DESC1 root_sig_desc = root_sig->_desc;
    
    _descriptor_table_bitmask = root_sig->_descriptor_table_bitmask;
    u32 descriptor_table_bitmask = _descriptor_table_bitmask;
    
    u32 current_offset = 0;
    DWORD root_index;
    while (_BitScanForward(&root_index, descriptor_table_bitmask) && root_index < root_sig_desc.NumParameters)
    {
        u32 num_descriptors = root_sig->GetNumDescriptors(root_index);
        
        DescriptorTableCache *table_cache = &_descriptor_table_cache[root_index];
        table_cache->_num_descriptors = num_descriptors;
        table_cache->_base_descriptor = _descriptor_handle_cache + current_offset;
        
        current_offset += num_descriptors;
        
        // Flip the descirptor table so it's not scanned again
        descriptor_table_bitmask ^= (1 << root_index);
    }
    assert(current_offset <= _num_descriptors_per_heap && "Root signature requires more than the max number of descriptors per descriptor heap. Consider enlaring the maximum.");
}

// Reset used descriptors. This should only be done if any
// descriptors that are being referenced by a command list 
// has finished executing on the command queue
void 
DynamicDescriptorHeap::Reset()
{
    // Copy all descriptor heaps into the available descriptor heap pool
    arrsetlen(_avail_descriptors, 0);
    for (u32 i = 0; i < (u32)arrlen(_descriptor_heap_pool); ++i)
        arrput(_avail_descriptors, _descriptor_heap_pool[i]);
    
    _current_descriptor_heap = 0; // needs to Reset ComPtr?
    _current_cpu_handle = {};
    _current_gpu_handle = {};
    _num_free_handles = 0;
    _descriptor_table_bitmask = 0;
    _stale_cbv_bitmask = 0;
    _stale_srv_bitmask = 0;
    _stale_uav_bitmask = 0;
    _stale_descriptor_table_bitmask = 0;
    
    // Reset the table cache
    for (int i = 0; i < MAX_DESCRIPTOR_TABLES; ++i)
    {
        _descriptor_table_cache[i].Reset();
        _inline_cbv[i] = 0ull;
        _inline_srv[i] = 0ull;
        _inline_uav[i] = 0ull;
    }
}

// Request a heap - if one is available
ID3D12DescriptorHeap* 
DynamicDescriptorHeap::RequestDescriptorHeap()
{
    ID3D12DescriptorHeap *result = 0;
    
    if (arrlen(_avail_descriptors) > 0)
    { 
        result = arrpop(_avail_descriptors);
    }
    else
    {
        result = CreateDescriptorHeap();
        arrpush(_descriptor_heap_pool, result);
    }
    
    return result;
}

// Create new descriptor heap if no heap is available
ID3D12DescriptorHeap* 
DynamicDescriptorHeap::CreateDescriptorHeap()
{
    ID3D12Device *d3d_device = device::GetDevice();
    ID3D12DescriptorHeap *result = 0;
    
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = _heap_type;
    desc.NumDescriptors = _num_descriptors_per_heap;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    AssertHr(d3d_device->CreateDescriptorHeap(&desc, IIDE(&result)));
    
    return result;
}

// Compute the number of stale descriptors that need to be
// copied to the GPU visible descriptor heap
u32 DynamicDescriptorHeap::ComputeStaleDescriptorCount()
{
    u32 result = 0;
    DWORD i;
    DWORD bitmask = _stale_descriptor_table_bitmask;
    
    while (_BitScanForward(&i, bitmask))
    {
        result += _descriptor_table_cache[i]._num_descriptors;
        bitmask ^= (1 << i);
    }
    
    return result;
}