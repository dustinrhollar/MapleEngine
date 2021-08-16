#ifndef _PLATFORM_H
#define _PLATFORM_H

void PlatformCloseApplication();

//------------------------------------------------------------------------------------
// Memory API

void* PlatformVirtualAlloc(u64 size);
void  PlatformVirtualFree(void *ptr);

//------------------------------------------------------------------------------------
// Windowing API 

void PlatformGetWindowDims(u32 *width, u32 *height);

//------------------------------------------------------------------------------------
// Threading API 

void PlatformAsyncTask(void (*fn)(void*), void *args);
void PlatformAtomicInc(volatile u32*);
void PlatformAtomicDec(volatile u32*);

//------------------------------------------------------------------------------------
// FILE API 

union FILE_ID
{
    struct { u32 offset:24; u32 index:8; };
    u32 mask;
};

// A file id is basically a 32bit number
#define INVALID_FID { 0xFFFFFFFF, 0xFFFFFFFF }

enum class FileType : u8
{
    Volume,
    Directory,
    Compressed,
    File,
    
    Count,
    Unknown = Count,
};

struct PlatformFile
{
    Str      physical_name; // abs path,    , ex. "C:\some_dir\maple-merchant\shaders\internal\file.hlsl"
    Str      relative_name; // rel path,    , ex. "internal\file.hlsl"
    FileType type;
    FILE_ID  fid;           // NOTE(Dustin): Does this need to be stored?
    FILE_ID  parent_fid;    // NOTE(Dustin): Does this need to be stored?
    FILE_ID *child_fids;    // stb_array
};

static void          PlatformMountFile(const char *virtual_name, const char *path);
static FILE_ID       PlatformGetMountFile(const char *virtual_name);
static PlatformFile* PlatformGetFile(FILE_ID fid);

FORCE_INLINE bool
PlatformIsValidFid(FILE_ID fid)
{
    return fid.mask != 0xFFFFFFFF;
}

typedef enum 
{
    // File Errors
    PlatformError_FileOpenFailure,
    PlatformError_FileCloseFailure,
    PlatformError_FileWriteFailure,
    PlatformError_FileReadFailure,
    PlatformError_FilePartialeWrite, // can occur if there is not enough disk space, or socket is blocked
    PlatformError_FilePartialeRead,
    PlatformError_DirectoryAlreadyExists,
    PlatformError_FileNotFound,
    PlatformError_PathNotFound,
    PlatformError_TooManyOpenFiles,
    PlatformError_AccessDenied,     
    PlatformError_InvalidHandle,
    PlatformError_MountNotFound,
    PlatformError_Success,
    PlatformError_Count,
    PlatformError_Unknown = PlatformError_Count,
} PlatformErrorType;

PlatformErrorType PlatformReadFileToBuffer(const char* file_path, u8** buffer, u32* size);
PlatformErrorType PlatformWriteBufferToFile(const char* file_path, u8* buffer, u64 size, bool append = false);

// TODO(Matt): Replace these params with enums.
// Defaults 0, -1
//Str PlatformShowBasicFileDialog(int type, int resource_type);

bool PlatformShowAssertDialog(const char* message, const char* file, u32 line);
void PlatformShowErrorDialog(const char* message);

//------------------------------------------------------------------------------------
// LOGGING API

enum LogLevels { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

#define LogTrace(...) PlatformLog(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__) 
#define LogDebug(...) PlatformLog(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__) 
#define LogInfo(...)  PlatformLog(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__) 
#define LogWarn(...)  PlatformLog(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__) 
#define LogError(...) PlatformLog(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__) 
#define LogFatal(...) PlatformLog(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__) 

void PlatformLog(int level, const char *file, int line, const char *fmt, ...);

//------------------------------------------------------------------------------------
// BIT SHIFTING SHENANIGANS API

u32 PlatformCtz(u32 v); 
u32 PlatformClz(u32 v); 
u32 PlatformCtzl(u64 v); 
u32 PlatformClzl(u64 v); 

//------------------------------------------------------------------------------------
// GUID API

static MAPLE_GUID PlatformGenerateGuid();
static bool       PlatformGuidCmp(MAPLE_GUID *left, MAPLE_GUID *right);
static Str        PlatformGuidToString(MAPLE_GUID guid);
static MAPLE_GUID PlatformStringToGuid(const char* guid_str);
// 
// Compares against a global runtime value for GUIDs. This is not guarenteed to 
// be the same across multiple instances of a program. The invalid GUID is initialized
// on first call to this function or the first call to PlatformGetInvalidGuid, depending
// on which is called first.
//
static bool       PlatformIsGuidValid(MAPLE_GUID guid);
//
// The invalid GUID is initialized on first call to this function or the first call to 
// PlatformIsGuidValid, depending on which is called first. If you plan on utilizing 
// the invalid guid , it is recommended that you call this function before intializing 
// any other GUIDs. The invalid guid is not synchronized across threads, so it must be 
// initialized before work using GUIDs can be executed async.
//
static MAPLE_GUID PlatformGetInvalidGuid();

#endif //_PLATFORM_H
