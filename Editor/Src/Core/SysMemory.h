#ifndef _SYS_MEMORY_H
#define _SYS_MEMORY_H

#if 1
#define SysAlloc(s)      SysMemoryAlloc((s))
#define SysFree(p)       (SysMemoryRelease((void*)(p)), p = NULL)
#define SysRealloc(p, s) SysReallocWrapperT((p), (s))
#else
#define SysAlloc(s)      malloc((s))
#define SysFree(p)       (free((void*)(p)), p = NULL)
#define SysRealloc(p, s) realloc((p), (s))
#endif

#ifdef __cplusplus

template<typename T> T* SysReallocWrapperT(T* ptr, u64 size)
{
    return (T*)SysMemoryRealloc((void*)ptr, size);
}

#else
#define SysReallocWrapperT(p, s) ((p) = SysMemoryRealloc((p), (s)))
#endif

void SysMemoryInit(void *ptr, u64 size);
void SysMemoryFree();

void* SysMemoryAlloc(u64 size);
void  SysMemoryRelease(void *ptr);
void* SysMemoryRealloc(void *ptr, u64 size);

#endif // _SYS_MEMORY_H
