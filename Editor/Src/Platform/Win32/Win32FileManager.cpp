
//-----------------------------------------------------------------------------------------------//
// External API

FORCE_INLINE bool
operator==(FILE_ID left, FILE_ID right)
{
    return left.mask == right.mask;
}

FORCE_INLINE bool
operator!=(FILE_ID left, FILE_ID right)
{
    return left.mask != right.mask;
}

//-----------------------------------------------------------------------------------------------//

#include <fileapi.h>

namespace file_manager
{
    // TODO(Dustin): Synchronization
    
    struct FileKeyValue
    {
        u128    key;   // Hash value of a string
        FILE_ID value; // FILE_ID for this particular file
    };
    
    struct PlatformFilePoolPage
    {
        void       *backing;
        PlatformFile **free_list;
    };
    
    struct PlatformFilePool
    {
        u32                count_per_page;
        PlatformFilePoolPage *page_list;
    };
    
    struct FileMount
    {
        Str           name;       // virtual name for the mount point
        FILE_ID       fid;        // root file for the mount
        FileKeyValue *file_table; // hashtable lookup for file ids
    };
    
    static PlatformFilePool  g_file_pool;
    static FileMount        *g_mounts = 0;
    
    // File Pool Page Interface
    static void PlatformFilePoolPageInit(PlatformFilePoolPage *page, u32 count_per_page);
    static void PlatformFilePoolPageFree(PlatformFilePoolPage *page);
    static PlatformFile* PlatformFilePoolPageAlloc(PlatformFilePoolPage *page);
    static void PlatformFilePoolPageRelease(PlatformFilePoolPage *page, PlatformFile *file);
    
    // File Pool Interface
    static void PlatformFilePoolInit(PlatformFilePool *pool, u32 count_per_page);
    static void PlatformFilePoolFree(PlatformFilePool *pool);
    FORCE_INLINE PlatformFile* OffsetToPlatformFile(PlatformFilePoolPage *page, u32 offset);
    FORCE_INLINE u32 PlatformFileToOffset(PlatformFilePoolPage *page, PlatformFile *file);
    static PlatformFile* PlatformFilePoolAlloc(PlatformFilePool *pool);
    static PlatformFile* PlatformFilePoolGetFile(PlatformFilePool *pool, FILE_ID fid);
    
    // File Mount interface
    static void MountFile(const char *virtual_name, const char *path);
    
    // File Manager Interface
    static void Init();
    
};

static void 
file_manager::PlatformFilePoolPageInit(PlatformFilePoolPage *page, u32 count_per_page)
{
    page->backing   = PlatformVirtualAlloc(count_per_page * sizeof(PlatformFile));
    page->free_list = (PlatformFile**)page->backing;
    
    // initialize the free list.
    PlatformFile **iter = page->free_list;
    for (u32 i = 0; i < count_per_page - 1; ++i)
    {
        *iter = (PlatformFile*)iter + 1;
        iter = (PlatformFile**)(*iter);
    }
    *iter = nullptr;
}

static void 
file_manager::PlatformFilePoolPageFree(PlatformFilePoolPage *page)
{
    PlatformVirtualFree(page->backing);
    page->backing = 0;
    page->free_list = 0;
}

static PlatformFile* 
file_manager::PlatformFilePoolPageAlloc(PlatformFilePoolPage *page)
{
    if (!page->free_list) return 0;
    
    PlatformFile *result = (PlatformFile*)page->free_list;
    page->free_list = (PlatformFile**)(*page->free_list);
    return result;
}

static void 
file_manager::PlatformFilePoolPageRelease(PlatformFilePoolPage *page, PlatformFile *file)
{
    *((PlatformFile**)file) = (PlatformFile*)page->free_list;
    page->free_list = (PlatformFile**)file;
}

static void 
file_manager::PlatformFilePoolInit(PlatformFilePool *pool, u32 count_per_page)
{
    pool->count_per_page = count_per_page;
    pool->page_list = 0;
}

static void 
file_manager::PlatformFilePoolFree(PlatformFilePool *pool)
{
    for (u32 i = 0; i < (u32)arrlen(pool->page_list); ++i)
    {
        PlatformFilePoolPageFree(pool->page_list + i);
    }
    
    arrfree(pool->page_list);
}

FORCE_INLINE PlatformFile*
file_manager::OffsetToPlatformFile(PlatformFilePoolPage *page, u32 offset)
{
    u64 real_offset = sizeof(PlatformFile) * offset;
    return (PlatformFile*)((char*)page->backing + real_offset);
}

FORCE_INLINE u32
file_manager::PlatformFileToOffset(PlatformFilePoolPage *page, PlatformFile *file)
{
    return (u32)(((char*)file - (char*)page->backing) / sizeof(PlatformFile));
}

static PlatformFile* 
file_manager::PlatformFilePoolAlloc(PlatformFilePool *pool)
{
    PlatformFile *result = 0;
    
    for (u32 i = 0; i < (u32)arrlen(pool->page_list); ++i)
    {
        PlatformFilePoolPage *page = pool->page_list + i;
        result = PlatformFilePoolPageAlloc(page);
        if (result) 
        {
            *result = {};
            result->fid.offset = PlatformFileToOffset(page, result);
            result->fid.index  = i;
            break;
        }
    }
    
    if (!result)
    {
        PlatformFilePoolPage page = {};
        PlatformFilePoolPageInit(&page, pool->count_per_page);
        result = PlatformFilePoolPageAlloc(&page);
        
        arrput(pool->page_list, page);
    }
    
    return result;
}

static PlatformFile*
file_manager::PlatformFilePoolGetFile(PlatformFilePool *pool, FILE_ID fid)
{
    PlatformFilePoolPage *page = pool->page_list + fid.index;
    PlatformFile *result = OffsetToPlatformFile(page, fid.offset);
    return result;
}

static void
file_manager::Init()
{
    g_mounts = 0;
    PlatformFilePoolInit(&g_file_pool, 255);
}

// TODO(Dustin): Free version

static void
file_manager::MountFile(const char *virtual_name, const char *path)
{
    FileKeyValue *file_table;
    FileMount mount = {};
    
    PlatformFile* mount_file = PlatformFilePoolAlloc(&g_file_pool);
    Assert(mount_file);
    
    // Root file for the mount
    mount_file->parent_fid = INVALID_FID;
    
    // For the mount file...
    // Physical Path: C:\some\path\project\
    // Relative Path: \

    // Normalize the path, and create search string
    Str physical_path = PlatformNormalizePath(path);
    Str relative_path = StrInit(0);
    
    mount_file->physical_name = physical_path;
    mount_file->relative_name = relative_path;
    
    PlatformFile **directory_queue = 0;
    arrput(directory_queue, mount_file);
    
    while (arrlen(directory_queue) > 0)
    {
        PlatformFile *iter = directory_queue[0];
        arrdel(directory_queue, 0);
        
        physical_path = iter->physical_name;
        relative_path = iter->relative_name;
        
        Str search_regex = StrAdd(&physical_path, "/*", 2);
        
        WIN32_FIND_DATA find_file_data;
        HANDLE handle = FindFirstFileExA(StrGetString(&search_regex), FindExInfoStandard, &find_file_data,
                                         FindExSearchNameMatch, NULL, 0);
        Assert(handle != INVALID_HANDLE_VALUE);
        
        do
        {
            if (strcmp(find_file_data.cFileName, ".") != 0 &&
                strcmp(find_file_data.cFileName, "..") != 0 &&
                find_file_data.cFileName[0] != '.' ) // don't allow hidden files or folders
            {
                // Build relative path
                Str child_relative_path;
                {
                    u64 offset = 0;
                    char *child_relative_path_ptr;
                    
                    if (StrLen(&relative_path) > 0)
                    {
                        child_relative_path = StrInit(1 + strlen(find_file_data.cFileName) + StrLen(&relative_path));
                        child_relative_path_ptr = StrGetString(&child_relative_path);
                        
                        memcpy(child_relative_path_ptr + offset, StrGetString(&relative_path), StrLen(&relative_path));
                        offset += StrLen(&relative_path);
                        
                        child_relative_path_ptr[offset++] = '/';
                    }
                    else
                    {
                        child_relative_path = StrInit(strlen(find_file_data.cFileName));
                        child_relative_path_ptr = StrGetString(&child_relative_path);
                    }
                    
                    memcpy(child_relative_path_ptr + offset, find_file_data.cFileName, strlen(find_file_data.cFileName));
                }
                
                // Build physical path
                Str child_physical_path = StrInit(1 + StrLen(&physical_path) + strlen(find_file_data.cFileName));
                {
                    u64 offset = 0;
                    char *child_physical_path_ptr = StrGetString(&child_physical_path);
                    
                    memcpy(child_physical_path_ptr + offset, StrGetString(&physical_path), StrLen(&physical_path));
                    offset += StrLen(&physical_path);
                    
                    child_physical_path_ptr[offset++] = '/';
                    
                    memcpy(child_physical_path_ptr + offset, find_file_data.cFileName, strlen(find_file_data.cFileName));
                }
                
                PlatformFile* file = PlatformFilePoolAlloc(&g_file_pool);
                file->physical_name = child_physical_path;
                file->relative_name = child_relative_path;
                file->parent_fid = iter->fid;
                
                BY_HANDLE_FILE_INFORMATION file_info;
                BOOL Err = GetFileAttributesEx(StrGetString(&child_physical_path),
                                               GetFileExInfoStandard,
                                               &file_info);
                Assert(Err);
                
                if (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    file->type = FileType::Directory;
                    arrput(directory_queue, file);
                }
                else 
                {
                    file->type = FileType::File;
                }
                
                arrput(iter->child_fids, file->fid);
                
                //LogInfo("Loading file...\n\tPhysical Path: %s\n\tRelative Path: %s\n", 
                //StrGetString(&file->physical_name), StrGetString(&file->relative_name));
            }
        }
        while (FindNextFileA(handle, &find_file_data) != 0);
        
        FindClose(handle);
        StrFree(&search_regex);
    }
    
    arrfree(directory_queue);
    
    // For funsies, let's walk the directories
    FILE_ID *walk_queue = 0;
    arrput(walk_queue, mount_file->fid);
    
#if 0
    while (arrlen(walk_queue) > 0)
    {
        FILE_ID fid = walk_queue[0];
        arrdel(walk_queue, 0);
        
        PlatformFile *file = PlatformFilePoolGetFile(&g_file_pool, fid);
        Assert(file);
        
        LogInfo("File Data:\n\tPhysical Path: %s\n\tRelative Path: %s\n",
                StrGetString(&file->physical_name), StrGetString(&file->relative_name));
        
        for (u32 i = 0; i < (u32)arrlen(file->child_fids); ++i)
            arrput(walk_queue, file->child_fids[i]);
    }
#endif
    
    mount.name = StrInit(strlen(virtual_name), virtual_name);
    mount.fid = mount_file->fid;
    
    arrput(g_mounts, mount);
}

static void 
PlatformMountFile(const char *virtual_name, const char *path)
{
    file_manager::MountFile(virtual_name, path);
}

static PlatformFile* 
PlatformGetFile(FILE_ID fid)
{
    Assert(PlatformIsValidFid(fid));
    return file_manager::PlatformFilePoolGetFile(&file_manager::g_file_pool, fid);
}

static FILE_ID
PlatformGetMountFile(const char *virtual_name)
{
    FILE_ID result = INVALID_FID;
    
    for (u32 i = 0; i < (u32)arrlen(file_manager::g_mounts); ++i)
    {
        if (strcmp(StrGetString(&file_manager::g_mounts[i].name), virtual_name) == 0)
        {
            result = file_manager::g_mounts[i].fid;
            break;
        }
    }
    
    return result;
}
