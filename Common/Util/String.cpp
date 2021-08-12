
#define STACK_STR_SIZE 23

#if (IS_LITTLE_ENDIAN==1)
#define SMALL_STRING_MASK  0xFF00000000000000
#define STRING_ZERO_MASK   0x00FFFFFFFFFFFFFF
#define SMALL_STRING_SHIFT (u64)56
#define HEAP_STRING_BIT    (u64)63
#else
#define SMALL_STRING_MASK  0x00000000000000FF
#define STRING_ZERO_MASK   0xFFFFFFFFFFFFFF00
#define SMALL_STRING_SHIFT 0
#define HEAP_STRING_BIT    (u64)0
#endif

#define STR_MEMORY_PAGE_SIZE _2MB
struct StrMemoryPage
{
    // NOTE(Dustin): No need to store the backing pointer. 
    // Structure is automatically allocated, so use this pointer
    // when calling VirtualFree
    //void          *backing;
    memory_t       allocator;
    
    StrMemoryPage *prev;
    StrMemoryPage *next;
};

// Pages are released to the operating system at application
// close, so do not need to explicitly free the page list.
// TODO(Dustin): Synchronization
static StrMemoryPage *g_str_page_list = 0;

static void* StrMemoryPageAlloc(u32 size);
static void StrMemoryPageRelease(void *ptr);

// UTF-16 API
int str_size_from_leading(char16_t c);
char32_t str_to_code_point(const char utf8[4], int* consumed);
int str_size_in_utf16(char32_t cp);

static void*
StrMemoryPageAlloc(u32 size)
{
    assert(size <= STR_MEMORY_PAGE_SIZE);
    
    void *result = NULL;
    
    StrMemoryPage *iter = g_str_page_list;
    while (iter)
    {
        result = memory_alloc(iter->allocator, size);
        if (result) break;
        iter = iter->next;
    }
    
    if (!result)
    {
        StrMemoryPage *page = 0;
        // NOTE(Dustin): Allocations are actually _2MB + 4096 bytes with a 
        // 4kb page size. This is due to the alignment requirements of windows
        // VirtualAlloc. When large pages are enabled, there will be a considerable
        // amount of wasted space (total alloc would be 4MB), so need a better 
        // solution than this in the long run.
        void *backing = PlatformVirtualAlloc(_2MB + sizeof(StrMemoryPage));
        
        page = (StrMemoryPage*)backing;
        page->prev = 0;
        page->next = 0;
        memory_init(&page->allocator, _2MB, (char*)backing + sizeof(StrMemoryPage));
        
        result = memory_alloc(page->allocator, size);
        assert(result);
        
        // Place the new page at the front of the page list
        page->next = g_str_page_list;
        if (page->next) 
            page->next->prev = page;
        g_str_page_list = page;
    }
    
    return result;
}

static void
StrMemoryPageRelease(void *ptr)
{
    if (!ptr) return;
    
    // Find the page the string was allocated from
    StrMemoryPage *page = g_str_page_list;
    while (page)
    {
        i64 diff  = (char*)ptr - (char*)page->allocator->Start;
        i64 diff2 = ((char*)page->allocator->Start + page->allocator->Size) - (char*)ptr;
        if ((diff | diff2) >= 0)
            break;
        
        page = page->next;
    }
    
    if (page)
    {
        memory_release(page->allocator, ptr);
        
        // TODO(Dustin): Hueristic, move the pool
        // closer to the front of the list based
        // on the remaining size. 
    }
}

static Str
StrInit(u64 len, const char *str)
{
    Str result = {};
    result.len  = 0;
    result.storage = 0;
    
    if (len <= STACK_STR_SIZE)
    { // Alloc string on the stack
        if (str)
            memcpy(result.sptr, str, len);
        
        // Reverse length
        len = STACK_STR_SIZE - len;
        // Zero the bottom bit
        len <<= 1;
        // Set length to bottom byte
        len <<= SMALL_STRING_SHIFT;
        result.len |= len;
    }
    else
    { // Alloc string on the heap
        result.hptr = (char*)StrMemoryPageAlloc((u32)len+1);
        if (str)
            memcpy(result.hptr, str, len);
        result.hptr[len] = 0;
        
        result.storage = len;
        result.len = memory_align(len+1, 8);
        // heap remains set to 0 to mark it as dirty
        // set bottom bit to 1, to mark the heap
        //result.len <<= 3;
        result.len |= BIT(HEAP_STRING_BIT);
    }
    
    return result;
}

static void
StrFree(Str *str)
{
    if (str->len & BIT(HEAP_STRING_BIT))
    {
        StrMemoryPageRelease(str->hptr);
    }
    str->len  = 0;
    str->storage = 0;
}

static char*
StrGetString(Str *str)
{
    char *result = 0;
    if (str->len & BIT(HEAP_STRING_BIT))
        result = str->hptr;
    else
        result = str->sptr;
    return result;
}

static u64
StrLen(Str *str)
{
    u64 result = 0;
    
    if (str->len & BIT(HEAP_STRING_BIT))
    {
        result = str->storage;
    }
    else
    {
        result = str->len;
        // Get the length byte
        result &= SMALL_STRING_MASK;
        result >>= SMALL_STRING_SHIFT;
        // Divide by two
        result >>= 1;
        // Reverse the length (gets stored as STACK_STR_SIZE - length)
        result = STACK_STR_SIZE - result;
    }
    
    return result;
}

static void
StrSetLen(Str *str, u64 len)
{
    if (str->len & BIT(HEAP_STRING_BIT))
    {
        // NOTE(Dustin): Is it worth checking if the string shrunk enough
        // to be place on the stack?
        
        str->hptr[len] = 0;
        str->storage = len;
    }
    else
    {
        str->sptr[len] = 0;
        // Zero the length byte
        str->len &= STRING_ZERO_MASK;
        // Reverse length
        len = STACK_STR_SIZE - len;
        // Zero the bottom bit
        len <<= 1;
        // Set length to bottom byte
        len <<= SMALL_STRING_SHIFT;
        str->len |= len;
    }
}

static Str   
StrAdd(Str *left, Str *right)
{
    const char *left_ptr  = StrGetString(left);
    const char *right_ptr = StrGetString(right);
    
    u64 left_len  = StrLen(left);
    u64 right_len = StrLen(right);
    u64 new_len   = left_len + right_len;
    
    Str result = StrInit(new_len);
    char *result_ptr = StrGetString(&result);
    
    memcpy(result_ptr, left_ptr, left_len);
    memcpy(result_ptr + left_len, right_ptr, right_len);
    
    return result;
}

static Str   
StrAdd(Str *left, const char *right_ptr, u64 right_len)
{
    const char *left_ptr  = StrGetString(left);
    
    u64 left_len  = StrLen(left);
    u64 new_len   = left_len + right_len;
    
    Str result = StrInit(new_len);
    char *result_ptr = StrGetString(&result);
    
    memcpy(result_ptr, left_ptr, left_len);
    memcpy(result_ptr + left_len, right_ptr, right_len);
    
    return result;
}

// UTF-16 IMPLEMENTATIONS...

int str_size_from_leading(char16_t c)
{
    if (c <= 0xd7ff) return 1; // Low range of single-word codes.
    if (c <= 0xdbff) return 2; // High surrogate of double-word codes.
    
    // If unpaired surrogates are disallowed, a trailing surrogate cannot
    // appear as the leading word. Otherwise just treat as a single word.
#ifdef MAPLE_STRING_NO_UNPAIRED_SURROGATES
    if (c <= 0xdfff) return 0;
#endif
    if (c <= 0xfffd) return 1; // High range of single-word surrogates.
    return 0; // Code points 0xfffe and 0xffff are invalid.
}

char32_t str_to_code_point(const char utf8[4], int* consumed)
{
    *consumed = str_size_from_leading(utf8[0]);
    switch(*consumed)
    {
        case 1: return utf8[0] & 0x7f;
        case 2:
        {
            if ((utf8[1] & 0xc0) != 0x80) break;
            return ((utf8[0] & 0x1f) << 6) | (utf8[1] & 0x3f);
        }
        case 3:
        {
            if ((utf8[1] & 0xc0) != 0x80 || (utf8[2] & 0xc0) != 0x80) break;
            char32_t out = ((utf8[0] & 0x0f) << 12) |
            ((utf8[1] & 0x3f) << 6) |
            (utf8[2] & 0x3f);
            if (utf8[0] == 0xe0 && out < 0x0800) break;
            if (utf8[0] == 0xe4 && out > 0xd800 && out < 0xdfff) break;
            return out;
        }
        case 4:
        {
            if ((utf8[1] & 0xc0) != 0x80 ||
                (utf8[2] & 0xc0) != 0x80 ||
                (utf8[3] & 0xc0) != 0x80) break;
            char32_t out = ((utf8[0] & 0x07) << 18) |
            ((utf8[1] & 0x3f) << 12) |
            ((utf8[2] & 0x3f) << 6) |
            (utf8[3] & 0x3f);
            if (utf8[0] == 0xf0 && out < 0x10000) break;
            if (utf8[0] == 0xf4 && out > 0x10ffff) break;
            return out;
        }
    }
    *consumed = 1;
    return MAPLE_STRING_REPLACEMENT_CHAR;
}

int str_size_in_utf16(char32_t cp)
{
    if (cp <= 0xd7ff) return 1; // Low range of single-word codes.
    
    // If unpaired surrogates are disallowed, the range 0xd800..0xdfff is
    // invalid. Otherwise, we treat as a single word code.
#ifdef MAPLE_STRING_NO_UNPAIRED_SURROGATES
    if (cp <= 0xdfff) return 0;
#endif
    
    if (cp <= 0xfffd) return 1; // High range of single-word codes.
    if (cp <= 0xffff) return 0; // 0xfffe and 0xffff are invalid.
    if (cp <= 0x10ffff) return 2; // Anything above 0xffff needs two words.
    return 0; // Anything above 0x10ffff is invalid.
}

bool str_to_utf16(char32_t cp, char16_t out[2], int* size)
{
    *size = str_size_in_utf16(cp);
    switch(*size)
    {
        case 1:
        {
            out[0] = (char16_t)(cp);
            return true;
        }
        case 2:
        {
            cp -= 0x10000;
            out[0] = (char16_t)((cp >> 10) + 0xd800);
            out[1] = (cp & 0x3ff) + 0xdc00;
            return true;
        }
        default:
        {
            *size = 1;
            out[0] = 0xfffd;
            return false;
        }
    }
}

void char8_to_char16(char16_t *out, char *str)
{
    if (!str || !str[0]) return;
    i32 size = 0;
    int i = 0;
    while (str[i])
    {
        int consumed;
        char32_t cp = str_to_code_point(&str[i], &consumed);
        i += consumed;
        size += str_size_in_utf16(cp);
    }
    u32 capacity = size + 1;
    out = (char16_t*)SysAlloc(capacity * 2);
    
    i = 0;
    int j = 0;
    while (str[i])
    {
        int consumed;
        char32_t cp = str_to_code_point(&str[i], &consumed);
        i += consumed;
        if (size < j) break;
        str_to_utf16(cp, &out[j], &consumed);
        j += consumed;
    }
    out[size] = 0;
}
