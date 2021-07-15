#ifndef _CORE_H
#define _CORE_H

#define file_global   static
#define file_internal static
#define local_persist static

#define U8_MAX  UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX
#define I8_MAX  INT8_MAX
#define I16_MAX INT16_MAX
#define I32_MAX INT32_MAX
#define I64_MAX INT64_MAX
#define R32_MIN -FLT_MAX
#define R32_MAX FLT_MAX
#define R64_MIN -DBL_MAX
#define R64_MAX DBL_MAX
#define OUTPUT_BUFFER_SIZE 2048

#define ARRAYCOUNT(x) (sizeof(x) / sizeof(x[0]))

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef int8_t    b8;
typedef int16_t   b16;
typedef int32_t   b32;

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef float     r32;
typedef double    r64;

typedef uintptr_t uptr;
typedef intptr_t  iptr;

typedef union u128
{
    struct { i64 upper, lower; };
    i64 Bits[2];
} u128;

// Defines for calculating size
#define _KB(x) (x * 1024)
#define _MB(x) (_KB(x) * 1024)
#define _GB(x) (_MB(x) * 1024)

#define _64KB  _KB(64)
#define _1MB   _MB(1)
#define _2MB   _MB(2)
#define _4MB   _MB(4)
#define _8MB   _MB(8)
#define _16MB  _MB(16)
#define _32MB  _MB(32)
#define _64MB  _MB(64)
#define _128MB _MB(128)
#define _256MB _MB(256)
#define _1GB   _GB(1)

#define memory_align(val, alignment) (((alignment) + (val) - 1) & ~((alignment) - 1))
#define divide_align(val, align) (((val) + (align) - 1) / (align))

#define BIT(x) 1<<(x)

#define BIT_TOGGLE(n, b, v) ((n) = ((n) & ~(1 << (b))) | ((v) << (b)))
#define BIT32_TOGGLE_0(n ,b) BIT_TOGGLE(n, b, 0)
#define BIT32_TOGGLE_1(n, b) BIT_TOGGLE(n, b, 1)
#define BIT64_TOGGLE_0(n ,b) BIT_TOGGLE(n, b, 0)
#define BIT64_TOGGLE_1(n, b) BIT_TOGGLE(n, b, 1ULL)

#define BITMASK_CLEAR(x,m) ((x) &=(~(m)))

#define SINGLE_ARG(...) __VA_ARGS__

#ifndef NDEBUG
#define Assert(x)                                                                  \
{                                                                                  \
if (!(x))                                                                      \
{                                                                              \
if (PlatformShowAssertDialog(#x, __FILE__, (u32)__LINE__)) DebugBreak();   \
}                                                                              \
}
#else
#define Assert(x)
#endif // NDEBUG

#ifndef NDEBUG
#define AssertCustom(x, message)                                                      \
{                                                                                     \
if (!(x))                                                                         \
{                                                                                 \
if (PlatformShowAssertDialog(message, __FILE__, (u32)__LINE__)) DebugBreak(); \
}                                                                                 \
}
#else
#define AssertCustom(x, message)
#endif // NDEBUG

FORCE_INLINE void
fast_swap(i32 x, i32 y)
{
    i32 _x = x;            
    i32 _y = y;            
    x = _y;                      
    y = _x;                      
}

FORCE_INLINE void
fast_swapf(r32 &x, r32 &y)
{
    r32 _x = x;            
    r32 _y = y;            
    x = _y;                      
    y = _x;                      
}

FORCE_INLINE void
fast_swapd(r64 x, r64 y)
{
    r64 _x = x;            
    r64 _y = y;            
    x = _y;                      
    y = _x;                      
}

// source: https://hbfs.wordpress.com/2008/08/05/branchless-equivalents-of-simple-functions/
// short for sign extend :p
// forces the compile to use cbw family of instructions (ideally)
FORCE_INLINE
int sex(int x)
{
    union
    {
        i64 w;
        struct { i32 lo, hi; } _p;
    } z;
    z.w = x;
    return z._p.hi;
}

FORCE_INLINE
i32 fast_sign32(i32 v)
{
    return (((u64)-v >> 31) - ((u64)v >> 31));
}

FORCE_INLINE
i32 fast_sign64(i64 v)
{
    return (((u64)-v >> 63) - ((u64)v >> 63));
}

FORCE_INLINE
int fast_abs(int x)
{
    int result;
    result = (x ^ sex(x)) - sex(x);
    return result;
}

FORCE_INLINE
int fast_max(int min, int max)
{
    int result;
    result = min + ((max - min) & ~sex(max - min));
    return result; 
}

FORCE_INLINE
int fast_min(int min, int max)
{
    int result;
    result = max + ((min - max) & sex(min - max));
    return result;
}

FORCE_INLINE
int fast_clamp(int min, int max, int val)
{
    return fast_min(fast_max(val, min), max);
}

// TODO(Dustin): Should probably test this to make sure
// it is faster than ternary operators. Depends on how
// well optimized abs/absf is on the cl compiler

// the clamp_mm functions combines the min max 

FORCE_INLINE r64 fast_mind(r64 a, r64 b)
{
    return (a + b - fabs(a - b)) * 0.5;
}

FORCE_INLINE r32 fast_minf(r32 a, r32 b)
{
    return (a + b - fabsf(a - b)) * 0.5f;
}

FORCE_INLINE r64 fast_maxd(r64 a, r64 b)
{
    return (a + b + fabs(a - b)) * 0.5;
}

FORCE_INLINE r32 fast_maxf(r32 a, r32 b)
{
    return (a + b + fabsf(a - b)) * 0.5f;
}

FORCE_INLINE r64 fast_clampd(r64 min, r64 max, r64 value)
{
    return fast_mind(max, fast_maxd(min, value));
}

FORCE_INLINE r32 fast_clampf(r32 min, r32 max, r32 value)
{
    return fast_minf(max, fast_maxf(min, value));
}

/**
    * Round up to the next highest power of 2.
    * @source: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    */
FORCE_INLINE u32
NextHighestPow2(u32 v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

/**
* Round up to the next highest power of 2.
* @source: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
*/
FORCE_INLINE u64 
NextHighestPow2(u64 v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return v;
}

#endif //_CORE_H
