
typedef u64 FILE_STR_ID; 
constexpr u16 MAX_WIN32_FILE_POOL_COUNT = 256;
constexpr u16 MAX_WIN32_FILE_PER_POOL_COUNT = 255;
constexpr u16 WIN32_BIT_MASK_COUNT = memory_align(MAX_WIN32_FILE_PER_POOL_COUNT, 64) / 64;

//
// A file id is a mask that can locate the file within the
// Win32FilePool data structure. An Win32FileManager instance will
// contain *at most* 256 Win32FilePools, each with the capacity
// of 256 files. This allows for up to 65,536 files for a single
// instance assetsys. 
//
// A file id is a mask for the major and minor pool index.
// A major pool index is the index in the file pool array
// whereas the minor pool index is the index into the file array
// inside a pool. The lower 8 bits of the file id are the major 
// pool index and the upper 8 bits are the minor pool index.
//
union FILE_ID
{
    struct { u8 major, minor; } _p;
    u16 mask;
};
constexpr FILE_ID INVALID_FID = {255,255};

enum class Win32FileType
{
    Volume = 0,
    Directory,
    Compressed,
    File,
    
    Count,
    Unknown = Count, // used for checking if file is initialized
};

enum class Win32FileMode
{
    Read,
    Write,
    ReadWrite,
    Append,
    
    Count,
    Unknown = Count, 
};

struct Win32File
{
    // Name information
    FILE_STR_ID physical_name; // abs path,    , ex. "C:\some_dir\maple-merchant\shaders\internal\file.vert"
    Win32FileType type;
    bool          is_meta;
    FILE_ID       fid;
    FILE_ID       parent;
    FILE_ID      *child_files;   //stb_array
};

struct Win32FilePair
{
    FILE_ID     file;
    FILE_ID     meta;
    FILE_STR_ID virtual_name;
    MAPLE_GUID  guid;
};

struct Win32Mount
{
    FILE_STR_ID    name; // virtual name for the mount point
    FILE_ID        fid;  // root file for the mount
    
    Win32FilePair *by_name_lookup;
    Win32FilePair *by_guid_lookup;
};

enum class FileInfoType : u8 
{
	AsFile, // file info as filepath
    AsGuid, // file info as GUID
    AsName, // file info as virtual name
    
    Count,
    Unknown = Count,
};

// Files are allocated from a File Pool.
struct Win32FilePool
{
    static const u16 MAX_POOL_COUNT = 256;
    static const u16 MAX_FILE_COUNT = 255;
    
    Win32File *handles;
    u32   allocated_files;
    // The number of required u64s is dependent on the number
    // of allowed files in the Pool. For example, if we have a
    // Pool with a max of 256 files should have 4 u64s to have a
    // mask for all files. Likewise, a Pool with 250 files should
    // also have 4 u64s.
    u64   allocation_mask[WIN32_BIT_MASK_COUNT];
};

struct Win32FileManager
{
    StrPool         str_pool;
    Win32FilePool  *file_pools;   // stb array
    Win32Mount     *mount_points; // stb array
    STR_POOL_ID     root_path;    // NOTE(Dustin): Is this needed anymore?
};

struct FileInfo
{
	FileInfoType type;
    
    u8 is_relative:1; // is the filename a relative path to mount? 1: yes, 0: no
    u8 is_meta:1;     // is this a meta file? 1: yes, 0: no
    u8 pad0:6;        // what to put here in the future?
    
	union {
		struct {
			const char *filename;
			const char *mount;
        } _as_file;
        
		struct {
			MAPLE_GUID  guid;
			const char *mount;
		} _as_guid;
        
		struct {
			const char *virtual_name;
			const char *mount;
		} _as_virtual;
	} _p;
};


FORCE_INLINE b8
IsFileIdValid(FILE_ID id)
{
    return (id.mask != INVALID_FID.mask);
}

FORCE_INLINE bool
operator==(FILE_ID left, FILE_ID right)
{
    return left.mask == right.mask;
}

file_global Win32FileManager g_file_manager;

file_internal void Win32FileManagerInit(const char *root);
file_internal void Win32FileManagerFree();

file_internal void Win32FilePoolInit(Win32FilePool *pool);
file_internal void Win32FilePoolFree(Win32FilePool *pool);
file_internal void Win32FilePoolAlloc(Win32FilePool *pool, Win32File **File);
file_internal void Win32FilePoolRelease(Win32FilePool *pool, FILE_ID fid);

file_internal void 
Win32FilePoolInit(Win32FilePool *pool)
{
    pool->handles = 0;
    arrsetcap(pool->handles, MAX_WIN32_FILE_PER_POOL_COUNT);
    pool->allocated_files = 0;
    
#if 0
    for (u32 i = 0; i < arrlen(pool->handles); ++i)
        pool->handles[i] = File::Invalid();
#endif
    
    u32 mask_ints = WIN32_BIT_MASK_COUNT;
    for (u32 i = 0; i < mask_ints; ++i)
        pool->allocation_mask[i] = 0;
}

file_internal void 
Win32FilePoolFree(Win32FilePool *pool)
{
    arrfree(pool->handles);
    pool->allocated_files = 0;
}

file_internal void 
Win32FilePoolAlloc(Win32FilePool *pool, Win32File **file)
{
    *file = 0;
    
    // Early out: don't bother allocating a file if the pool is full
    if (pool->allocated_files + 1 >= arrlen(pool->handles)) return;
    
    u32 mask_ints = WIN32_BIT_MASK_COUNT;
    u64 bitset;
    for (u32 i = 0; i < mask_ints; ++i)
    {
        // Negate the particular bitset. Ctz will find the first
        // 1 starting from the the least significant digit. However,
        // for the purposes of this bitlist, a 1 means an allocation,
        // but we want to find unallocated blocks. Negating the bitset will
        // set all allocated blocks to 0 and unallocated blocks to 1.
        // The found idx is the index of the block we want to retrieve
        // for the allocation.
        bitset = ~pool->allocation_mask[i];
        
        // Ctz() is undefined when the number == 0
        // If the negated bitset == 0, then there are
        // no available allocations in the block.
        // If there are no allocations, then go to the
        // next bitset.
        u32 idx = 0;
        u32 bit = 0;
        if (bitset)
        {
            bit = PlatformCtzl(bitset);
            idx = i * 64 + bit;
            BIT64_TOGGLE_1(pool->allocation_mask[i], bit);
            
            *file = pool->handles + idx;
            (*file)->fid._p.minor = idx;
            ++pool->allocated_files;
            
            break;
        }
    }
}

file_internal void 
Win32FilePoolRelease(Win32FilePool *pool, FILE_ID fid)
{
    
}

file_internal void 
Win32FileManagerInit(const char *root)
{
}


file_internal void 
Win32FileManagerFree()
{
}