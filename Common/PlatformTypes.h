
#ifndef _PLATFORM_TYPES_H
#define _PLATFORM_TYPES_H

#if defined(__linux__) || defined(__APPLE__) 

#define FORCE_INLINE inline __attribute__((always_inline))

#elif defined(_WIN32)

#include <guiddef.h>
typedef GUID MAPLE_GUID;
#define INVALID_FILE_ID INVALID_HANDLE_VALUE
#define FORCE_INLINE    __forceinline

#else
#error Platform not supported!
#endif

#endif //_PLATFORM_TYPES_H
