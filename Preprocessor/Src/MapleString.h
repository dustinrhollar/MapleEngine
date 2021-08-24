
#ifndef _UTIL_STRING_H
#define _UTIL_STRING_H

#ifndef MAPLE_STRING_REPLACEMENT_CHAR
#define MAPLE_STRING_REPLACEMENT_CHAR 0xfffd
#endif

struct Str
{
    union
    {
        char sptr[16];
        struct 
        {
            char *hptr;
            //
            // If stack allocation:
            // - Field is overwritten and unused
            //
            // If heap allocation:
            // - Field is treated as the length
            //
            u64   storage;
        };
    };
    
    //
    // If stack allocation:
    // - Up to first 7 bytes is extra storage for sptr
    // - Length is stored as 2 * (MaxShortLen - Len)
    // ---- Bottom bit will always be zero in this case
    // ---- At max length (23 bytes), length acts as the NULL byte
    //
    // If heap allocation:
    // - This field is treated as the heap size for the allocation
    // - Assume all allocations are 8 byte aligned, therefore bottom
    //   bottom three bits can be used as flag bits
    // -- Bit 0 unused
    // -- Bit 1 unused
    // -- Bit 2 heap flag
    //
    u64 len;
};

static Str   StrInit(u64 len, const char *str = NULL);
static void  StrFree(Str *str);
static char* StrGetString(Str *str);
static void  StrSetLen(Str *str, u64 len);
static u64   StrLen(Str *str);
static Str   StrAdd(Str *left, Str *right);
static Str   StrAdd(Str *left, const char *right_ptr, u64 right_len);
static int   StrCmp(Str *left, Str *right);
static int   StrCmp(Str *left, const char *right);

void char8_to_char16(char16_t *out, char *in);

FORCE_INLINE
i64 StrLen16(char16_t *strarg)
{
    if(!strarg)
        return -1; //strarg is NULL pointer
    char16_t* str = strarg;
    for(;*str;++str)
        ; // empty body
    return str-strarg;
}

#endif // _STRING_H
