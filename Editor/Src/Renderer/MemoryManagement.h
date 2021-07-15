#ifndef _MEMORY_MANAGEMENT_H
#define _MEMORY_MANAGEMENT_H

static const u64 DEFAULT_PAGE_SIZE = _2MB;
static const u32 INVALID_PAGE_ID = U32_MAX - 1;

struct UploadBuffer
{
    struct Allocation
    {
        void                     *cpu = 0;
        D3D12_GPU_VIRTUAL_ADDRESS gpu = 0;
    };
    
    struct Page
    {
        ID3D12Resource           *_rsrc;
        void                     *_base; // cpu ptr
        D3D12_GPU_VIRTUAL_ADDRESS _gpu_ptr;
        u64                       _page_size;
        u64                       _offset;
        
        void Init(u64 page_size);
        void Free();
        
        FORCE_INLINE bool HasSpace(u64 size, u64 alignment);
        Allocation Allocate(u64 size, u64 alignment);
        void Reset();
    };
    
    using PAGE_ID = u32;
    
    Page    *_page_pool   = 0;
    PAGE_ID *_avail_pages = 0; // stack of pages - last in, first out
    PAGE_ID  _active_page = INVALID_PAGE_ID;
    u64      _page_size;
    
    void Init(u64 page_size = DEFAULT_PAGE_SIZE);
    void Free();
    
    Allocation Allocate(u64 size, u64 alignment);
    PAGE_ID RequestPage();
    void Reset();
};

struct DescriptorAllocation
{
    void Init(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, 
              u32                         num_handles, 
              u32                         descriptor_size, 
              struct DescriptorAllocator *allocator,
              u32                         page_id);
    void Free();
    // Get a descriptor at a particular offset in the allocation.
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(u32 offset = 0);
    bool IsNull();
    
    struct DescriptorAllocatorPage* GetDescriptorAllocatorPage();
    
    // @Internal
    
    struct DescriptorAllocator *_allocator = 0;
    // NOTE(Dustin): Can we delete Descriptor Pages?
    // If so, need to be careful about storing the index here
    u32                         _page_id;
    D3D12_CPU_DESCRIPTOR_HANDLE _descriptor = {};
    u32                         _num_handles = 0;
    u32                         _descriptor_size = 0;
};

struct DescriptorAllocatorPage
{
    void Init(D3D12_DESCRIPTOR_HEAP_TYPE type, u32 count_per_heap, struct DescriptorAllocator *allocator, u32 id);
    void Free();
    
    DescriptorAllocation Allocate(u32 num_descriptors);
    
    u32 NumFreeHandles();
    bool HasSpace(u32 num_descriptors);
    
    // @brief Return a descripor back to the heap.
    // Release does not immediately free descriptors, but adds them to a stale
    // descriptor queue. Stale allocations are returned all at once with the
    // call to ReleaseStaleAllocations.
    // //NOTE(Dustin): Will need to change the descriptor handle to be more appropriate
    void Release(DescriptorAllocation *descriptor_handle);
    // Return stale descriptors back to the heap
    void ReleaseStaleDescriptors();
    
    // @Internal
    
    u32 ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle);
    // Add a new block to the free list
    void AddNewBlock(u32 offset, u32 num_descriptors);
    // Free a block a descriptors. This will merge free blocks in the list 
    // to form larger blocks that can be reused.
    void FreeBlock(u32 offset, u32 num_descriptors);
    
    // TODO(Dustin): Try to find a clean Bimap implementation for
    // optimized searching for size and offsets
    
    // NOTE(Dustin): Possble Alternative - Needs two lists:
    // 1. A (sorted) list/map for offset lookups  (merge operations)
    // 2. A SORTED list for size lookups (alloc operations)
    // TODO(Dustin): Use a map for offset lookups
    // TODO(Dustin): Use a binary tree (allowing dups) for size lookups 
    
    // For now, keep it simple: one list sorted by offset
    // TODO(Dustin): Maybe try embedding BLock info into the allocation
    // so that there is not an extra list to track?
    struct FreeBlockInfo
    {
        u32 size; // number of descriptors for this allocation
        u32 offset;
    };
    
    struct StaleDescriptorInfo
    {
        u32 offset;
        u32 size;
    };
    
    FreeBlockInfo              *_free_list = 0;
    // TODO(Dustin): Use a resizable ringbuffer instead...
    StaleDescriptorInfo        *_stale_descriptor_queue = 0;
    ID3D12DescriptorHeap       *_descriptor_heap = 0;
    D3D12_DESCRIPTOR_HEAP_TYPE  _heap_type;
    D3D12_CPU_DESCRIPTOR_HANDLE _base_descriptor;
    u32                         _descriptor_increment_size;
    u32                         _num_descriptors_in_heap;
    u32                         _num_free_handles;
    CRITICAL_SECTION            _cs_lock;
    
    // NOTE(Dustin): 
    // Backpointer to the allocator that allocated this page.
    // Page are allocated (by value) inside of a Page Heap in the
    // allocator, but a DescriptorAllocation needs a backpointer to this
    // Page. Since the Page Heap can resize, any pointer to a page will 
    // become stale during a resize. To avoid this problem an allocation
    // gets a pointer to the Allocator & PAGE_ID.
    //
    // TODO(Dustin): Find a way to remove this backpointer.
    struct DescriptorAllocator *_allocator;
    u32                         _id; // id of this page
};

struct DescriptorAllocator
{
    // Acceptable Descriptor Types:
    // - CBV_SRV_UAV
    // - SAMPLER
    // - RTV
    // - DSV
    void Init(D3D12_DESCRIPTOR_HEAP_TYPE type, u32 count_per_heap = 256);
    void Free();
    
    // Allocates a number of contiguous descriptors from a CPU
    // visible heap. Cannot be more than the number of descriptors
    // per heap.
    DescriptorAllocation Allocate(u32 num_descriptors);
    
    // When a frame completes, release stale descriptors
    void ReleaseStaleDescriptors();
    
    //@Internal
    using PAGE_ID = u32;
    static const PAGE_ID INVALID_PAGE_ID = U32_MAX - 1;
    
    PAGE_ID CreateAllocatorPage();
    // Inserts a page into the available page heap list
    // Searches the list first so that duplicates are
    // not added to the list.
    void AddAvailableHeap(PAGE_ID avail_page);
    
    D3D12_DESCRIPTOR_HEAP_TYPE _heap_type;
    u32                        _descriptors_per_heap;
    DescriptorAllocatorPage   *_heap_pool = 0;
    u64                       *_avail_heaps; // @NOTE: Check for dups?
    CRITICAL_SECTION           _cs_lock;
};

struct DynamicDescriptorHeap
{
    void Init(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, u32 num_descriptors_per_heap = 1024);
    void Free();
    
    // Stages a contiguous range of CPU descriptors. Descriptors are not copied to the
    // GPU visible desccriptor heap until the CommitStagedDescriptors func is called
    void StageDescriptors(u32 root_param_idx, u32 offset, u32 num_descriptors, D3D12_CPU_DESCRIPTOR_HANDLE src);
    
    /*
* Copy all of the descriptors to the GPU visible descriptor heap
* and bind the descriptor heap and the descriptor tables to the
 * command list. The passed-in function object is used to set the
* GPU visible descriptors on the command list. Two posible functions:
* - Before a draw:     ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable
* - Before a dispatch: ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
*/
    void CommitDescriptorTables(struct CommandList *cmd_list, void (*set_root)(ID3D12GraphicsCommandList *list, UINT root_index, D3D12_GPU_DESCRIPTOR_HANDLE handle));
    void CommitInlineDescriptors(struct CommandList *cmd_list,
                                 D3D12_GPU_VIRTUAL_ADDRESS *buffer_locs,
                                 u32 *bitmask, 
                                 void (*set_root)(ID3D12GraphicsCommandList *list, UINT root_index, D3D12_GPU_VIRTUAL_ADDRESS handle));
    
    void CommitStagedDescriptorsForDraw(struct CommandList *cmd_list);
    void CommitStagedDescriptorsForDispatch(struct CommandList *cmd_list);
    
    /*
* Copies a single CPU visible descriptor to a GPU visible descriptor heap.
* Useful for the
* - ID3D12GraphicsCommandList::ClearUnorderedAccessViewFloat
* - ID3D12GraphicsCommandList::ClearUnorderedAccessViewUint
* methods which require both a CPU and GPU visible descriptors for a UAV
* resource. 
*
*/
    D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptor(struct CommandList *cmd_list, D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor);
    
    // Parse root signature to determine which root parameters
    // contain descriptor tables and determine the number of 
    // descriptors needed for each table
    void ParseRootSignature(struct RootSignature *root_sig);
    
    // Reset used descriptors. This should only be done if any
    // descriptors that are being referenced by a command list 
    // has finished executing on the command queue
    void Reset();
    
    // @INTERNAL
    
    // Maximum number of descriptor tables oer root signature;
    // A 32-bit mask is used to keep track of the root 
    // parameter indices that are descriptor tables
    static const u32 MAX_DESCRIPTOR_TABLES = 32;
    
    struct DescriptorTableCache
    {
        u32                          _num_descriptors = 0;
        // The ptr to the dsecriptor in the descriptor handle cache
        D3D12_CPU_DESCRIPTOR_HANDLE* _base_descriptor = 0;
        
        void Reset() { _num_descriptors = 0; _base_descriptor = 0; }
    };
    
    // Request a heap - if one is available
    ID3D12DescriptorHeap* RequestDescriptorHeap();
    // Create new descriptor heap if no heap is available
    ID3D12DescriptorHeap* CreateDescriptorHeap();
    // Compute the number of stale descriptors that need to be
    // copied to the GPU visible descriptor heap
    u32 ComputeStaleDescriptorCount();
    
    // Stage inline descriptors
    void StageInlineCBV(u32 root_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_loc);
    void StageInlineSRV(u32 root_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_loc);
    void StageInlineUAV(u32 root_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_loc);
    
    // Describes the type of descriptors that can be staged using this
    // dynamic descriptor heap. Valid values are:
    // - D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    // - D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
    D3D12_DESCRIPTOR_HEAP_TYPE    _heap_type;
    u32                           _num_descriptors_per_heap;
    u32                           _descriptor_increment_size;
    D3D12_CPU_DESCRIPTOR_HANDLE  *_descriptor_handle_cache;
    
    DescriptorTableCache          _descriptor_table_cache[MAX_DESCRIPTOR_TABLES];
    D3D12_GPU_VIRTUAL_ADDRESS _inline_cbv[MAX_DESCRIPTOR_TABLES];
    D3D12_GPU_VIRTUAL_ADDRESS _inline_srv[MAX_DESCRIPTOR_TABLES];
    D3D12_GPU_VIRTUAL_ADDRESS _inline_uav[MAX_DESCRIPTOR_TABLES];
    
    // Each bit in the mask contains the index in the root signature
    // that contains a descriptor table
    u32                           _descriptor_table_bitmask;
    // Each bit in the mask contains the index in the root signature
    // that contains a descriptor table that has changed since the last
    // time the descriptors were copied
    u32                           _stale_descriptor_table_bitmask;
    u32                           _stale_cbv_bitmask;
    u32                           _stale_srv_bitmask;
    u32                           _stale_uav_bitmask;
    
    // List of allocated descriptor heap pool. Acts as a backing storage
    // for Reset calls. On Reset, the heap pool is copied over to
    // avail_descriptors
    ID3D12DescriptorHeap        **_descriptor_heap_pool; // 
    // List of indices of available descriptor heaps in the
    // descriptor heap pool
    ID3D12DescriptorHeap        **_avail_descriptors;
    // Current descriptor heap bound to the command list
    ID3D12DescriptorHeap         *_current_descriptor_heap;
    D3D12_GPU_DESCRIPTOR_HANDLE   _current_gpu_handle;
    D3D12_CPU_DESCRIPTOR_HANDLE   _current_cpu_handle;
    u32                           _num_free_handles = 0;
};

#endif //_MEMORY_MANAGEMENT_H
