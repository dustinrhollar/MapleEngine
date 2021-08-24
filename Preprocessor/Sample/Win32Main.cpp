
#include <stdlib.h> // req for Core
#include <stdint.h> // req for Core
#include <math.h>   // req for Core
#include <stdio.h>  // req for Core

#include <Core.h> // req for the Logger

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

// req for the Windows Logger
static void* PlatformVirtualAlloc(u64 Size);
static void  PlatformVirtualFree(void *Ptr);

#include <Win32Logger.cpp> // Windows Logging

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define MAPLE_META_IMPLEMENTATION
#include <Meta.h>

#define MAPLE_REFLECTION_META_IMPLEMENTATION
#include <CodeGen/ReflectionMetadata.h>

#include <CodeGen/Test0.h>
#include <CodeGen/Test1.h>
#include <CodeGen/Test2.h>
#include <CodeGen/Test3.h>

static void 
MetaLogWrapper(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Win32SimpleVLog(1, fmt, args);
    va_end(args);
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
    Foo foo = {
        42069,
        3.145f,
        -20438964.6549,
    };
    
    Goo goo = {
        -12,
        34567
    };
    
    Zoo zoo = {
        56475868,
        foo,
        goo,
    };
    
    Woo woo = {
        2342341,
        foo,
        goo,
    };
    
    PlatformLoggerInit();
    ReflectionMetadataInit();
    
    MetadataCallbacks cb = {};
    cb.Log = MetaLogWrapper;
    ReflectionMetadataSetCallbacks(&cb);
    
    MetaPrint(&foo, Foo);
    MetaPrint(&goo, Goo);
    MetaPrint(&zoo, Zoo);
    MetaPrint(&woo, Woo);
    
    ReflectionMetadataFree();
    PlatformLoggerFree();
    
    return (0);
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