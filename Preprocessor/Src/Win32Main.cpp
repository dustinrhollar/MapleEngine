#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include "Core.h"

static void* PlatformVirtualAlloc(u64 Size);
static void  PlatformVirtualFree(void *Ptr);

static void PlatformReadFileToBuffer(const char* file_path, u8** buffer, u32* size);
static void PlatformWriteBufferToFile(const char* file_path, u8* buffer, u64 size, bool append = false);

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "timeapi.h"

#include "Win32Timer.cpp"
#include "Win32Logger.cpp"

#define SysAlloc   malloc
#define SysRealloc realloc
#define SysFree    free

#include "Memory.cpp"
#include "MapleString.h"
#include "MapleString.cpp"

#include "PrettyBuffer.h"

#include "PrettyBuffer.c"

#define MAPLE_META_IMPLEMENTATION
#include "Meta.h"

#include "Parser.cpp"

static const char *CmdLineArg_LUT[] = {
    "/OUT:",
    "/IN:",
    "/Mt",
    "/Help"
};

struct MdfFile
{
    Str            orig_path;
    Str            fullpath;
    const char    *hfile; // points into char array in fullpath
    ParserRegistry parsed_file;
};

#include "CodeGen.cpp"

//
// Current TODO List:
// - Metadata generates nested (unnamed) structs
// - Unions & Unnamed unions
// - Metadata generates nested (unnamed) unions
// - Preserve includes within an MDF file
// ---- .mdf files should be converted to their Header counterpart
// - Command line arguments: /OUT (output directory), /IN (input file/directory), /Mt (multithread tag)
// - Multithread the application
// - Generate serialize/deserialize information
//

struct ThreadData
{
    // TODO(Dustin): Local string linear allocator
};

//
// When searching for include paths, we want to lookup if a 
// file has already been parsed.
// 
// Maps a mdf file to the MDF (parsed) metadata
//
struct MDF_LUT { char *key; MdfFile value; } static *g_file_registry = {};

FORCE_INLINE bool
PathEndsWith(const char *p, u64 len, char c)
{
    return (len > 0) ? p[len-1] == c : false;
}

static MdfFile 
MdfFilenameConvert(const char *outdir, const char *filename)
{
    // some_file.mdf -> some_file.h
    MdfFile mdf_file = {};
    
    u64 new_filename_len = strlen(filename) - 2;
    {
        // file the filename, and strop directories
        const char *iter_end = filename + new_filename_len;
        const char *iter = iter_end - 1;
        
        while (iter != filename && *iter != '/' && *iter != '\\')
            --iter;
        
        if (*iter != '/' || *iter != '\\')
            ++iter;
        
        filename = iter;
        new_filename_len = (u32)(iter_end - iter);
    }
    
    u64 outdir_len = strlen(outdir);
    bool ends_with_slash = PathEndsWith(outdir, outdir_len, '/') || PathEndsWith(outdir, outdir_len, '\\');
    
    u64 new_len = new_filename_len + outdir_len;
    if (!ends_with_slash) ++new_len;
    
    Str str = StrInit(new_len, outdir);
    char *pstr = StrGetString(&str);
    
    pstr += outdir_len;
    if (!ends_with_slash) 
    {
        *pstr = '/';
        ++pstr;
    }
    
    memcpy(pstr, filename, new_filename_len);
    pstr += new_filename_len - 1;
    
    *pstr = 'h';
    
    mdf_file.fullpath = str;
    mdf_file.hfile = StrGetString(&mdf_file.fullpath) + StrLen(&mdf_file.fullpath) - new_filename_len;
    
    return mdf_file;
}

static void
CollectMdfFiles(MdfFile **files, const char *input, const char *output)
{
    *files = 0;
    
    BY_HANDLE_FILE_INFORMATION file_info;
    BOOL err = GetFileAttributesEx(input,
                                   GetFileExInfoStandard,
                                   &file_info);
    Assert(err);
    AssertCustom(file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY, "Expected directory as input!");
    
    Str input_dir;
    u64 in_len = strlen(input);
    if (PathEndsWith(input, in_len, '/') || PathEndsWith(input, in_len, '\\'))
    {
        input_dir = StrInit(in_len, input);
    }
    else
    {
        input_dir = StrInit(in_len + 1, NULL);
        char *str = StrGetString(&input_dir);
        memcpy(str, input, in_len);
        str[in_len] = '/';
    }
    
    Str *directory_queue = 0;
    arrput(directory_queue, input_dir);
    
    while (arrlen(directory_queue) > 0)
    {
        Str iter = directory_queue[0];
        arrdel(directory_queue, 0);
        
        Str search_regex = StrAdd(&iter, "*", 1);
        
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
                
                u32 fil_len = (u32)strlen(find_file_data.cFileName);
                u32 new_len = fil_len + (u32)StrLen(&iter);
                Str new_fil = StrInit(new_len, NULL);
                
                char *str = StrGetString(&new_fil);
                memcpy(str, StrGetString(&iter), StrLen(&iter));
                memcpy(str + StrLen(&iter), find_file_data.cFileName, new_len);
                
                err = GetFileAttributesEx(StrGetString(&new_fil), GetFileExInfoStandard, &file_info);
                Assert(err);
                
                if (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    Str new_dir = StrInit(new_len + 1, NULL);
                    
                    char *str = StrGetString(&new_dir);
                    memcpy(str, StrGetString(&iter), StrLen(&iter));
                    memcpy(str + StrLen(&iter), find_file_data.cFileName, new_len);
                    str[new_len] = '/';
                    
                    arrput(directory_queue, new_dir);
                    StrFree(&new_fil);
                }
                else 
                {
                    MdfFile file = MdfFilenameConvert(output, str);
                    file.orig_path = new_fil;
                    arrput(*files, file);
                }
            }
        }
        while (FindNextFileA(handle, &find_file_data) != 0);
        
        FindClose(handle);
        StrFree(&search_regex);
        StrFree(&iter);
    }
}

static void 
ParseMdfFiles(MdfFile *files, u32 file_count)
{
    for (u32 i = 0; i < file_count; ++i)
    {
        ParseFile(&files[i].parsed_file, StrGetString(&files[i].orig_path));
    }
}

static void 
GenerateHeaderFiles(CodeGenerator *generator, MdfFile *files, u32 file_count)
{
    for (u32 i = 0; i < file_count; ++i)
    {
        CodeGeneratorGenCode(generator, &files[i]);
    }
}

struct Test
{
    struct N
    {
        int q;
    };
    
    int s;
    struct 
    {
        struct B
        {
            int b;
            int c;
            N   n;
        } b;
        
        int d;
        int q;
    } a;
};

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
    void *backing = PlatformVirtualAlloc(_2MB);
    
    GlobalTimerSetup(); 
    PlatformLoggerInit();
    
    size_t o = meta_offset(Test, a.q);
    LogWarn("%d", o);
    
    o = meta_offset(Test, a.b.b);
    LogWarn("%d", o);
    
    o = meta_offset(Test, a.b.c);
    LogWarn("%d", o);
    
    o = meta_offset(Test, a.d);
    LogWarn("%d", o);
    
    const char *t = (const char *)((uptr)5);
    int tc = (int)((uptr)t);
    
    g_file_registry = 0;
    sh_new_strdup(g_file_registry);
    
    char *outdir = "CodeGen/"; // let the outdir be the bin dir for now...
    char *indir = "Data";
    
    MdfFile *mdf_files = 0;
    
    CollectMdfFiles(&mdf_files, indir, outdir);
    ParseMdfFiles(mdf_files, (u32)arrlen(mdf_files));
    
    CodeGenerator generator = CodeGeneratorInit();
    {
        GenerateHeaderFiles(&generator, mdf_files, (u32)arrlen(mdf_files));
        CodeGeneratorGenMetaFile(&generator, outdir, mdf_files, (u32)arrlen(mdf_files));
    }
    CodeGeneratorFree(&generator);
    
    PlatformLoggerFree();
    PlatformVirtualFree(backing);
    return 0;
}

static void* 
PlatformVirtualAlloc(u64 Size)
{
    SYSTEM_INFO sSysInfo;
    DWORD       dwPageSize;
    LPVOID      lpvBase;
    u64         ActualSize;
    
    GetSystemInfo(&sSysInfo);
    dwPageSize = sSysInfo.dwPageSize;
    
    ActualSize = (Size + (u64)dwPageSize - 1) & ~((u64)dwPageSize - 1);
    lpvBase = VirtualAlloc(NULL,                    // System selects address
                           ActualSize,              // Size of allocation
                           MEM_COMMIT|MEM_RESERVE,  // Allocate reserved pages
                           PAGE_READWRITE);          // Protection = no access
    
    return lpvBase;
}

static void 
PlatformVirtualFree(void *Ptr)
{
    BOOL bSuccess = VirtualFree(Ptr,           // Base address of block
                                0,             // Bytes of committed pages
                                MEM_RELEASE);  // Decommit the pages
    if (!bSuccess)
    {
        DWORD error = GetLastError();
        PlatformFatalError("Unable to free a VirtualAlloc allocation!\n\tError: %ld\n", error);
    }
}