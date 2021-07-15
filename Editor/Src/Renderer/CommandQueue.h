#ifndef _COMMAND_QUEUE_H
#define _COMMAND_QUEUE_H

// A simple allocator for  command lists
// from. The intent of this structure is to solve stale 
// pointers that could arise by passing around command
// lists. Do not need to worry about resize
// since command lists are allocated in a set of pages,
// where each page is treated as a linear allocator.
// It is assumed that any CommandList allocated will persist
// through the lifetime of the CommandQueue that allocates it.
struct CommandListAllocator
{
    struct Page
    {
        struct CommandList *_list  = 0;
        u32                 _max   = 0;
        u32                 _count = 0;
        
        void Init(u32 max);
        void Free();
        struct CommandList* Allocate(u32 count = 1);
    };
    
    void Init(u32 max);
    void Free();
    struct CommandList* Allocate(u32 count = 1);
    
    Page *_page_list = 0;
    u32   _current_page;
    u32   _max_allocations_per_page;
};

struct CommandQueue
{
    RenderError Init(D3D12_COMMAND_LIST_TYPE queue_type);
    RenderError Free();
    
    struct CommandList* GetCommandList();
    u64 ExecuteCommandLists(struct CommandList **cmd_lists, i32 count);
    u64 Signal();
    
    bool IsFenceComplete(u64 fence_value);
    RenderError WaitForFenceValue(u64 fence_value);
    
    void Flush();
    
    /// Wait for another command queue to finish
    void Wait(CommandQueue *wait_queue);
    
    // @INTERNAL
    
    void ProcessInFlightCommandLists();
    
    ID3D12CommandQueue     *handle;
    D3D12_COMMAND_LIST_TYPE type;
    
    ID3D12Fence            *fence;
    u64                     fence_value;
    HANDLE                  fence_event;
    
    CommandListAllocator    _cmd_list_allocator;
    
    // Check in-flight command lists async
    // The flush command needs to wait until there are no
    // in flight command lists. This requires a CRIT SECTION
    // and a CRIT variable
    volatile u32       _is_active = 0;
    CRITICAL_SECTION   _cs_lock_in_flight;
    CONDITION_VARIABLE _cv_in_flight;
    HANDLE             _process_thread;
    
    // TODO(Dustin): Use a ringbuffer instead of 
    // of a simple array
    
    struct CommandListEntry
    {
        u64                 fence_value = 0;
        struct CommandList *list = 0;
    };
    
    struct InFlightQueue
    {
        CommandListEntry *_list = 0;
        CRITICAL_SECTION  _cs_lock;
        
        void Init()
        {
            _list = 0;
            InitializeCriticalSectionAndSpinCount(&_cs_lock, 1024);
        }
        
        void Free()
        {
            for (u32 i = 0; i < (u32)arrlen(_list); ++i)
                _list[i].list->Free();
            arrfree(_list);
            DeleteCriticalSection(&_cs_lock);
        }
        
        void Push(CommandListEntry entry)
        {
            EnterCriticalSection(&_cs_lock);
            arrput(_list, entry);
            LeaveCriticalSection(&_cs_lock);
        }
        
        bool Pop(CommandListEntry *entry)
        {
            bool result = false;
            if (arrlen(_list) > 0)
            {
                EnterCriticalSection(&_cs_lock);
                *entry = _list[0];
                arrdel(_list, 0);
                LeaveCriticalSection(&_cs_lock);
                result = true;
            }
            return result;
        }
    } _in_flight_cmd_lists;
    
    struct AvailableQueue
    {
        CommandList     **_list = 0;
        CRITICAL_SECTION  _cs_lock;
        
        void Init()
        {
            _list = 0;
            InitializeCriticalSectionAndSpinCount(&_cs_lock, 1024);
        }
        
        void Free()
        {
            for (u32 i = 0; i < (u32)arrlen(_list); ++i)
                _list[i]->Free();
            arrfree(_list);
            DeleteCriticalSection(&_cs_lock);
        }
        
        void Push(struct CommandList* entry)
        {
            EnterCriticalSection(&_cs_lock);
            arrput(_list, entry);
            LeaveCriticalSection(&_cs_lock);
        }
        
        struct CommandList* Pop()
        {
            CommandList* result = 0;
            if (arrlen(_list) > 0)
            {
                EnterCriticalSection(&_cs_lock);
                result = _list[0];
                arrdel(_list, 0);
                LeaveCriticalSection(&_cs_lock);
            }
            return result;
        }
        
    } _available_cmd_lists;
    
};

#endif //_COMMAND_QUEUE_H
