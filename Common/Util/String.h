
#ifndef _UTIL_STRING_H
#define _UTIL_STRING_H

#define STRING_STACK_SIZE 12

#ifndef MAPLE_STRING_REPLACEMENT_CHAR
#define MAPLE_STRING_REPLACEMENT_CHAR 0xfffd
#endif

typedef union 
{
    struct {
        char     sptr[STRING_STACK_SIZE];
        unsigned len:31;
        unsigned heap:1; // 0 - heap is not used. 1 - heap is used
    };
    
    struct {
        char*    hptr;
        unsigned heap_size;
    };
} Str;

// If a user only wants to init the string with a CAP, then
// pass NULL as ptr. 
void str_init(Str *str, const char *ptr, u32 len);
void str_free(Str *str);
void str_set_cap(Str *str, u32 len);
char* str_to_string(Str *str);
void str_add(Str *result, Str *left, Str *right);
void str_concat(Str *left, Str *right);
void str_log(Str *str);

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

#if defined(MAPLE_STRING_IMPLEMENTATION)

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

void str_init(Str *str, const char *ptr, u32 len)
{
    str->len = 0;
    str->heap = 0;
    
    if (len < STRING_STACK_SIZE)
    {
        if (ptr)
        {
            memcpy(str->sptr, ptr, len);
            str->sptr[len] = 0;
            str->len = len;
        }
    }
    else
    {
        str->heap = 1;
        str->heap_size = len+1;
        str->hptr = (char*)SysAlloc(len+1);
        
        if (ptr)
        {
            str->len = len;
            memcpy(str->hptr, ptr, len);
            str->hptr[len] = 0;
        }
    }
}

void str_free(Str *str)
{
    if (str->len > 0 && str->heap == 1) SysFree(str->hptr);
    str->len  = 0;
    str->heap = 0;
    str->hptr = 0;
}

void str_set_cap(Str *str, u32 len)
{
    // Only think about adjusting the cap if the length
    // is greater than the stack size
    if (len >= STRING_STACK_SIZE)
    {
        // Don't bother allocating new space if the heap
        // is already large enough
        if (str->heap == 1 && len > str->heap_size)
        {
            u32 new_heap = len + 1;
            char *tmp = (char*)SysAlloc(new_heap);
            memcpy(tmp, str->hptr, str->len);
            tmp[str->len] = 0;
            SysFree(str->hptr);
            str->hptr = tmp;
            str->heap_size = new_heap;
        }
        else
        {
            u32 new_heap = len + 1;
            char *tmp = (char*)SysAlloc(new_heap);
            memcpy(tmp, str->sptr, str->len);
            tmp[str->len] = 0;
            SysFree(str->hptr);
            str->hptr = tmp;
            str->heap_size = new_heap;
        }
        str->heap = 1;
    }
}

char* str_to_string(Str *str)
{
    if (str->heap) return str->hptr;
    else           return str->sptr;
}

void str_log(Str *str)
{
    LogInfo("%s", str_to_string(str));
}

void str_add(Str *result, Str *left, Str *right)
{
    str_init(result, NULL, left->len + right->len);
    
    if (result->heap)
    {
        LogDebug("Adding the two string: %s : %s. The result should allocate on the heap. %d", 
                 str_to_string(left),
                 str_to_string(right),
                 result->len);
        
        memcpy(result->hptr, str_to_string(left), left->len);
        result->len += left->len;
        memcpy(result->hptr+result->len, str_to_string(right), right->len);   
        result->len += right->len;
        result->hptr[result->len] = 0;
    }
    else
    {
        memcpy(result->sptr, str_to_string(left), left->len);
        result->len += left->len;
        memcpy(result->sptr+result->len, str_to_string(right), right->len);   
        result->len += right->len;
        result->sptr[result->len] = 0;
    }
}

void str_add_string(Str *result, Str *left, const char *right, u32 right_len)
{
    str_init(result, NULL, left->len + right_len);
    
    if (result->heap)
    {
        memcpy(result->hptr, str_to_string(left), left->len);
        result->len += left->len;
        memcpy(result->hptr+result->len, right, right_len);   
        result->len += right_len;
        result->hptr[result->len] = 0;
    }
    else
    {
        memcpy(result->sptr, str_to_string(left), left->len);
        result->len += left->len;
        memcpy(result->sptr+result->len, right, right_len);   
        result->len += right_len;
        result->sptr[result->len] = 0;
    }
}

void str_concat(Str *left, Str *right)
{
    str_set_cap(left, left->len + right->len);
    if (left->heap)
    {
        memcpy(left->hptr+left->len, str_to_string(right), right->len);
        left->len += right->len;
        left->hptr[left->len] = 0;
    }
    else
    {
        memcpy(left->sptr+left->len, str_to_string(right), right->len);
        left->len += right->len;
        left->sptr[left->len] = 0;
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

#endif //
