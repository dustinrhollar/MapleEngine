#ifndef _STR_POOL_H
#define _STR_POOL_H

// TODO(Dustin):
// - Add UTF-16 support

//
// Arena allocations are done in 2MB pages.
// Maximum possible offset into the arena is ~2097152, which fits nicely into
// 21 bits. 8 bits are reserved for selecting the arena the string is from.
// This allows for ~256 arenas - or about 2GB of string memory. If I ever fill
// that up, we got other problems to deal with ;)
//
// If the upper bound in memory becomes a problem, this can be solved by simply
// using 64bit storage value, which would allow for as many arena as you could
// possible want.
//
union STR_ID
{
    struct { u32 offset:24; u32 index:8; };
    u32 mask;
};

#define INVALID_STR_ID 0xFFFFFF

FORCE_INLINE bool
IsStrIdValid(STR_ID sid)
{
    return sid.mask != INVALID_STR_ID;
}

FORCE_INLINE bool
operator==(STR_ID left, STR_ID right)
{
    return left.mask == right.mask;
}

FORCE_INLINE bool
operator!=(STR_ID left, STR_ID right)
{
    return left.mask != right.mask;
}

//
// In order to avoid storing strings throughout global memory, the string pool
// seeks to maintain a single storage container for all strings within an application.
// This is done by allocating a list of pages of size 2MB, where each page contains
// a free-list type allocator. The allocator manages string allocations, while the
// pool will manage intializing string memory. The underlying memory can be acquired
// by calling "Str{16}ToString" - be aware: you are recieving the actual storage pointer, not
// a copy of the string so be careful if you choose to edit the string memory!
//
// A few notes on the String Allocations:
// - A String will have the following memory footprint:
//
// | ---- String Memory ---- | - Footer (32 bits) - |
//
// The 32bit Footer has the layout:
//
// |--------|------------|
// | Bit    | Purpose    |
// |--------|------------|
// | 0 - 7  | NULL Bit   |
// |--------|------------|
// | 8 - 31 | Str Length |
// |--------|------------|
//
// The full size of any given allocation will be:
//
// Total Size = String Length + sizeof(uint32_t)
//
// - Strings are considered immutable, and will require reallocations if the size is updated.
//   This is imporant to be aware of because strings are not optimized for concatenation,
//   even though this functionality is supported.
//
// - Strings are NOT NULL terminated. Instead, the null bit is embedded into the first bit of
//   the length in the footer.
//
// - The StrPool does not distinguish between ASCII, UTF-8, and UTF-16 strings. It is up to the
//   caller to know which strings are what format. The user can then call the correct API call.
//   Generally, API calls will be the format: "Str{,16}_".
//
// - UTF-16 support is very limited - only supports storing UTF16 and converting a UTF-8 to UTF-16.
//
// - Strings are not garbage collected - either internally or through RAII semantics. The user much
//   free strings manually by using "StrFree(STR_ID)". If the user does not care about freeing strings,
//   all string memory is cleaned up when the global string pool is freed (probably at application shutdown).
//

#if defined(MAPLE_STR_POOL_NO_SYNC_PRIMITIVES)

#define STR_POOL_LOCK(name)
#define STR_POOL_LOCK_INIT(lock)
#define STR_POOL_LOCK_FREE(lock)
#define STR_POOL_LOCK_LOCK(lock)
#define STR_POOL_LOCK_UNLOCK(lock)

#elif defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Windows.h"

#define STR_POOL_LOCK(name)        CRITICAL_SECTION name
#define STR_POOL_LOCK_INIT(lock)   InitializeCriticalSectionAndSpinCount(&(lock), 1024)
#define STR_POOL_LOCK_FREE(lock)   DeleteCriticalSection(&(lock))
#define STR_POOL_LOCK_LOCK(lock)   EnterCriticalSection(&(lock))
#define STR_POOL_LOCK_UNLOCK(lock) LeaveCriticalSection(&(lock))

#else

#pragma message (__FILE__ "[" STRING(__LINE__) "]: StrPool only supports Win32 for synchronization. Sync primitives will be turned off.")

#define STR_POOL_LOCK(name)
#define STR_POOL_LOCK_INIT(lock)
#define STR_POOL_LOCK_FREE(lock)
#define STR_POOL_LOCK_LOCK(lock)
#define STR_POOL_LOCK_UNLOCK(lock)

#endif

// As mentioned in the @brief{STR_ID}, the maximum size of a page within the pool
// is 2MB, or 2^21. This imposes a maximum length on any string a user wishes to create.
// Strings cannot be greater than 2MB, which allows for the assumption that the length
// of a string will only require 21 bits at most! Set the first 8 bits to be the NULL
// terminator for the string, and the remaining bits record the size.
union StrFooter
{
    struct {
        u64 null_bit:8;
        u64 len:24;
        u64 hash:32;
    };
    u64 mask;
};

#define STR_POOL_NULL_BIT 0x00

struct StrPool
{
    struct Page
    {
        void    *backing;
        memory_t allocator;
    };

    Page         *pages = 0;
    STR_POOL_LOCK(cs_lock); // define synchronization lock w/ name "cs_lock"
};

static void     StrPoolInit();
static void     StrPoolFree();
static void     StrPoolAlloc(void **mem, STR_ID *sid, u32 size);
static void     StrPoolRelease(STR_ID sid);

static STR_ID   StrInit(const char *str, u32 len);
static STR_ID   StrInit(u32 capactiy);
static void     StrFree(STR_ID sid);

static bool     StrCmp(STR_ID left, STR_ID right);

static STR_ID   StrAdd(STR_ID left, STR_ID right);
static STR_ID   StrAdd(STR_ID left, char  right);
static STR_ID   StrAdd(STR_ID left, const char* right, u32 len);
static STR_ID   StrAdd(const char *left, u32 left_len, const char* right, u32 right_len);
static STR_ID   StrPrepend(STR_ID left, const char* cstr_to_prepend, u32 len);

static char*    StrToString(STR_ID sid);
static wchar_t* Str16ToString(STR_ID sid);

static u32      StrLen(STR_ID sid);
static u32      Str16Len(STR_ID sid);

// Calculate a string hash value. This is automatically called when a string
// is initialized with data, or when a String OP occurs. If a user manually
// sets the storage data, then it is the user's responsibility to update
// a string's hash value.
static void     StrHash(STR_ID sid);
static u32      StrGetHash(STR_ID sid);

#define StrLogTrace(...) StrLog(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define StrLogDebug(...) StrLog(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define StrLogInfo(...)  StrLog(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define StrLogWarn(...)  StrLog(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define StrLogError(...) StrLog(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define StrLogFatal(...) StrLog(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
static void StrLog(int level, const char* file, int line, STR_ID sid);

#endif

#if defined(MAPLE_STR_POOL_IMPLEMENTATION)

static StrPool g_str_pool = {};

static void
StrPoolInit()
{
    // Create a single page and add it to the page list
    StrPool::Page page = {};
    page.backing = PlatformVirtualAlloc(_2MB);
    memory_init(&page.allocator, _2MB, page.backing);
    arrput(g_str_pool.pages, page);

    // Initialize the sync primitves
    STR_POOL_LOCK_INIT(g_str_pool.cs_lock);
}

static void
StrPoolFree()
{

    STR_POOL_LOCK_LOCK(g_str_pool.cs_lock);

    for (u32 i = 0; i < (u32)arrlen(g_str_pool.pages); ++i)
    {
        StrPool::Page *page = g_str_pool.pages + i;
        memory_free(&page->allocator);
        PlatformVirtualFree(page->backing);
    }

    STR_POOL_LOCK_UNLOCK(g_str_pool.cs_lock);

    arrfree(g_str_pool.pages);
    STR_POOL_LOCK_FREE(g_str_pool.cs_lock);
}


static void
StrPoolAlloc(void **mem, STR_ID *sid, u32 size)
{
    assert(size <= _2MB);

    u32 page_idx = 0;
    u32 page_offset = 0;
    void *result = NULL;

    STR_POOL_LOCK_LOCK(g_str_pool.cs_lock);

    for (u32 i = 0; i < (u32)arrlen(g_str_pool.pages); ++i)
    {
        StrPool::Page *page = g_str_pool.pages + i;
        result = memory_alloc(page->allocator, size);
        if (result)
        {
            page_idx = i;
            page_offset = (u32)((char*)result - (char*)page->backing);
            break;
        }
    }

    if (!result)
    {
        StrPool::Page page = {};
        page.backing = PlatformVirtualAlloc(_2MB);
        memory_init(&page.allocator, _2MB, page.backing);
        arrput(g_str_pool.pages, page);

        page_idx = (u32)arrlen(g_str_pool.pages)-1;
        result = memory_alloc(g_str_pool.pages[page_idx].allocator, size);
        page_offset = (u32)((char*)result - (char*)g_str_pool.pages[page_idx].allocator);
    }

    STR_POOL_LOCK_UNLOCK(g_str_pool.cs_lock);

    if (result)
    {
        *mem = result;
        sid->offset = page_offset;
        sid->index  = page_idx;
    }
    else
    {
        *mem = NULL;
        sid->mask = INVALID_STR_ID;
    }
}

static void
StrPoolRelease(STR_ID sid)
{
    if (!IsStrIdValid(sid)) return;

    u32 page_idx = sid.index;
    u32 page_offset = sid.offset;

    STR_POOL_LOCK_LOCK(g_str_pool.cs_lock);

    StrPool::Page *page = g_str_pool.pages + page_idx;
    void *mem = (char*)page->backing + page_offset;
    memory_release(page->allocator, mem);

    STR_POOL_LOCK_UNLOCK(g_str_pool.cs_lock);
}

static void*
StrPoolGetPtr(STR_ID sid)
{
    void *result = 0;
    if (IsStrIdValid(sid)) result = (char*)g_str_pool.pages[sid.index].backing + sid.offset;
    return result;
}

static STR_ID
StrInit(const char *cstr, u32 len)
{
    STR_ID result;
    char*  str;

    StrPoolAlloc((void**)&str, &result, len + sizeof(u32));

    if (str)
    {
        // Instead of having the offset be from the start of
        // the string, the offset should become the start of the
        // footer. This allows for the string's length to quickly
        // be determined.
        result.offset += len;

        StrFooter *footer = (StrFooter*)(str + len);
        footer->null_bit = STR_POOL_NULL_BIT;
        footer->len = len;
        memcpy(str, cstr, len);
    }

    StrHash(result);
    return result;
}

static STR_ID
StrInit(u32 capacity)
{
    STR_ID result;
    char*  str;

    StrPoolAlloc((void**)&str, &result, capacity + sizeof(u32));

    if (str)
    {
        // Instead of having the offset be from the start of
        // the string, the offset should become the start of the
        // footer. This allows for the string's length to quickly
        // be determined.
        result.offset += capacity;

        StrFooter *footer = (StrFooter*)(str + capacity);
        footer->null_bit = STR_POOL_NULL_BIT;
        footer->len = capacity;
    }

    return result;
}

static void
StrFree(STR_ID sid)
{
    // Adjust the offset so that it points to the start of
    // the allocation, instead of the footer.
    StrFooter *footer = (StrFooter*)StrPoolGetPtr(sid);
    sid.offset -= (u32)footer->len;

    StrPoolRelease(sid);
}

static bool
StrCmp(STR_ID left, STR_ID right)
{
    StrFooter *left_footer  = (StrFooter*)StrPoolGetPtr(left);
    StrFooter *right_footer = (StrFooter*)StrPoolGetPtr(right);
    return left_footer->hash == right_footer->hash;
}

static STR_ID
StrAdd(STR_ID left, STR_ID right)
{
    StrFooter *left_footer  = (StrFooter*)StrPoolGetPtr(left);
    StrFooter *right_footer = (StrFooter*)StrPoolGetPtr(right);

    u32 left_len  = (left_footer)  ? (u32)left_footer->len  : 0;
    u32 right_len = (right_footer) ? (u32)right_footer->len : 0;
    u32 new_len   = left_len + right_len;

    STR_ID result = StrInit(new_len);

    StrFooter *result_footer = (StrFooter*)StrPoolGetPtr(result);
    assert(result_footer);
    void *result_ptr = (char*)result_footer - result_footer->len;

    u32 offset = 0;
    if (left_footer)
    {
        void *left_ptr  = (char*)left_footer  - left_footer->len;
        memcpy(result_ptr, left_ptr, left_footer->len);
        offset += (u32)left_footer->len;
    }

    if (right_footer)
    {
        void *right_ptr = (char*)right_footer - right_footer->len;
        memcpy((char*)result_ptr + offset, right_ptr, right_footer->len);
    }

    StrHash(result);
    return result;
}

static STR_ID
StrAdd(STR_ID left, char right)
{
    StrFooter *left_footer  = (StrFooter*)StrPoolGetPtr(left);

    u32 left_len  = (left_footer)  ? (u32)left_footer->len  : 0;
    u32 right_len = 1;
    u32 new_len   = left_len + right_len;

    STR_ID result = StrInit(new_len);

    StrFooter *result_footer = (StrFooter*)StrPoolGetPtr(result);
    assert(result_footer);
    void *result_ptr = (char*)result_footer - result_footer->len;

    u32 offset = 0;
    if (left_footer)
    {
        void *left_ptr  = (char*)left_footer  - left_footer->len;
        memcpy(result_ptr, left_ptr, left_footer->len);
        offset += (u32)left_footer->len;
    }

    // Copy the character over
    ((char*)result_ptr)[offset] = right;

    StrHash(result);
    return result;
}

static STR_ID
StrAdd(STR_ID left, const char* right, u32 len)
{
    StrFooter *left_footer  = (StrFooter*)StrPoolGetPtr(left);

    u32 left_len  = (left_footer)  ? (u32)left_footer->len  : 0;
    u32 right_len = len;
    u32 new_len   = left_len + right_len;

    STR_ID result = StrInit(new_len);

    StrFooter *result_footer = (StrFooter*)StrPoolGetPtr(result);
    assert(result_footer);
    void *result_ptr = (char*)result_footer - result_footer->len;

    u32 offset = 0;
    if (left_footer)
    {
        void *left_ptr  = (char*)left_footer  - left_footer->len;
        memcpy(result_ptr, left_ptr, left_footer->len);
        offset += (u32)left_footer->len;
    }

    if (right)
    {
        memcpy((char*)result_ptr + offset, right, len);
    }

    StrHash(result);
    return result;
}

static STR_ID
StrPrepend(STR_ID left, const char* cstr_to_prepend, u32 len)
{
    StrFooter *left_footer  = (StrFooter*)StrPoolGetPtr(left);

    u32 left_len  = (left_footer)  ? (u32)left_footer->len  : 0;
    u32 right_len = len;
    u32 new_len   = left_len + right_len;

    STR_ID result = StrInit(new_len);

    StrFooter *result_footer = (StrFooter*)StrPoolGetPtr(result);
    assert(result_footer);
    void *result_ptr = (char*)result_footer - result_footer->len;

    u32 offset = 0;
    if (cstr_to_prepend)
    {
        memcpy((char*)result_ptr + offset, cstr_to_prepend, len);
        offset += len;
    }

    if (left_footer)
    {
        void *left_ptr  = (char*)left_footer  - left_footer->len;
        memcpy((char*)result_ptr + offset, left_ptr, left_footer->len);
    }

    StrHash(result);
    return result;
}

static STR_ID
StrAdd(const char *left, u32 left_len, const char *right, u32 right_len)
{
    u32 new_len   = left_len + right_len;
    STR_ID result = StrInit(new_len);

    StrFooter *result_footer = (StrFooter*)StrPoolGetPtr(result);
    assert(result_footer);
    void *result_ptr = (char*)result_footer - result_footer->len;

    u32 offset = 0;
    if (left)
    {
        memcpy(result_ptr, left, left_len);
        offset += left_len;
    }

    if (right)
    {
        memcpy((char*)result_ptr + offset, right, right_len);
    }

    StrHash(result);
    return result;
}

static char*
StrToString(STR_ID sid)
{
    StrFooter *footer = (StrFooter*)StrPoolGetPtr(sid);
    return (char*)footer - footer->len;
}

static wchar_t*
Str16ToString(STR_ID sid)
{}

static u32
StrLen(STR_ID sid)
{
    StrFooter *footer = (StrFooter*)StrPoolGetPtr(sid);
    return footer->len;
}

static u32
Str16Len(STR_ID sid)
{}

static void
StrLog(int level, const char *file, int line, STR_ID sid)
{
    const char *ptr = StrToString(sid);
    PlatformLog(level, file, line, ptr);
}

#if 0
FORCE_INLINE uint32_t
getblock32(const uint32_t * p, int i)
{
  return p[i];
}

FORCE_INLINE uint32_t
fmix32(uint32_t h)
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}
#endif

static void
StrHash(STR_ID sid)
{
#define ROTL32(x,y) _rotl(x,y)
#define getblock32(p, i) ((const uint32_t*)(p))[(int)(i)]
#define fmix32(h)        \
  h ^= h >> 16;          \
  h *= 0x85ebca6b;       \
  h ^= h >> 13;          \
  h *= 0xc2b2ae35;       \
  h ^= h >> 16;

    static uint32_t seed = (uint32_t)23216551321;
    const char *key = StrToString(sid);
    StrFooter *footer = (StrFooter*)StrPoolGetPtr(sid);
    int len = (int)footer->len;

    const uint8_t * data = (const uint8_t*)key;
    const int nblocks = len / 4;

    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    //----------
    // body

    const uint32_t * blocks = (const uint32_t *)(data + nblocks*4);

    for(int i = -nblocks; i; i++)
    {
        uint32_t k1 = getblock32(blocks,i);

        k1 *= c1;
        k1 = ROTL32(k1,15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1,13);
        h1 = h1*5+0xe6546b64;
    }

    //----------
    // tail

    const uint8_t * tail = (const uint8_t*)(data + nblocks*4);

    uint32_t k1 = 0;

    switch(len & 3)
    {
    case 3: k1 ^= tail[2] << 16;
    case 2: k1 ^= tail[1] << 8;
    case 1: k1 ^= tail[0];
          k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len;

    h1 = fmix32(h1);

    footer->hash = h1;
}

static u32
StrGetHash(STR_ID sid)
{
    u32 result = INVALID_STR_ID;

    StrFooter *footer = (StrFooter*)StrPoolGetPtr(sid);
    if (footer) result = (u32)footer->hash;
    return result;
}

#endif

