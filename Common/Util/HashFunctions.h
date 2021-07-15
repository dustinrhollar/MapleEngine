#ifndef _HASH_FUNCTIONS_H
#define _HASH_FUNCTIONS_H

typedef u64  Hash64;
typedef u128 Hash128;

class Hasher
{
    virtual u64 Hash64()
    {
        assert(false && "Hasher64 not implemented!");
        return 0;
    }
    
    virtual u128 Hash128()
    {
        assert(false && "Hasher128 not implemented!");
        return {};
    }
};

u64  MummurHash64(const void *key, u32 len);
u128 MummurHash128(const void *key, u32 len);

FORCE_INLINE
bool CompareHash64(u64 left, u64 right)
{
    return left == right;
}

FORCE_INLINE
bool CompareHash128(u128 left, u128 right)
{
    return left.upper == right.upper && left.upper == right.upper;
}

// A little different from the above version as it is not
// guarenteed that the value is a u128
// This function will coerce the type into a u128 for easy
// comparison.
FORCE_INLINE
bool Compare128BitCoerce(void *left, void *right)
{
    u128 *left_c = (u128*)left;
    u128 *right_c = (u128*)right;
    return CompareHash128(*left_c, *right_c);
}

#endif //_HASH_FUNCTIONS_H

#if defined(MAPLE_HASH_FUNCTION_IMPLEMENTATION)

static const u32 g_mummur_hash_seed = 8026;

u64 MurmurHash2_x64_64A( const void * key, int len, unsigned int seed );
void MurmurHash3_x64_128( const void * key, int len, uint32_t seed, void * out );

u64 MummurHash64(const void *key, u32 len)
{
    u64 result = MurmurHash2_x64_64A(key, len, g_mummur_hash_seed);
    return result;
}

u128 MummurHash128(const void *key, u32 len)
{
    u128 result = {};
    MurmurHash3_x64_128(key, len, g_mummur_hash_seed, &result);
    return result;
}

//-----------------------------------------------------------------------------
// MurmurHash2, 64-bit versions, by Austin Appleby

// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment 
// and endian-ness issues if used across multiple platforms.

//typedef unsigned __int64 uint64_t;

// 64-bit hash for 64-bit platforms

u64 MurmurHash2_x64_64A( const void * key, int len, unsigned int seed )
{
	const u64 m = 0xc6a4a7935bd1e995;
	const int r = 47;
    
	u64 h = seed ^ (len * m);
    
	const u64 * data = (const u64 *)key;
	const u64 * end = data + (len/8);
    
	while(data != end)
	{
		u64 k = *data++;
        
		k *= m; 
		k ^= k >> r; 
		k *= m; 
		
		h ^= k;
		h *= m; 
	}
    
	const unsigned char * data2 = (const unsigned char*)data;
    
	switch(len & 7)
	{
        case 7: h ^= u64(data2[6]) << 48;
        case 6: h ^= u64(data2[5]) << 40;
        case 5: h ^= u64(data2[4]) << 32;
        case 4: h ^= u64(data2[3]) << 24;
        case 3: h ^= u64(data2[2]) << 16;
        case 2: h ^= u64(data2[1]) << 8;
        case 1: h ^= u64(data2[0]);
        h *= m;
	};
    
	h ^= h >> r;
	h *= m;
	h ^= h >> r;
    
	return h;
}

//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
//
// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
//-----------------------------------------------------------------------------

#if defined(_MSC_VER)

#include <stdlib.h>

#define ROTL32(x,y) _rotl(x,y)
#define ROTL64(x,y) _rotl64(x,y)

#define BIG_CONSTANT(x) (x)

// Other compilers

#else // defined(_MSC_VER)

//#define FORCE_INLINE inline __attribute__((always_inline))

inline uint32_t rotl32 ( uint32_t x, int8_t r )
{
    return (x << r) | (x >> (32 - r));
}

inline u64 rotl64 ( u64 x, int8_t r )
{
    return (x << r) | (x >> (64 - r));
}

#define ROTL32(x,y) rotl32(x,y)
#define ROTL64(x,y) rotl64(x,y)

#define BIG_CONSTANT(x) (x##LLU)

#endif // !defined(_MSC_VER)

//-----------------------------------------------------------------------------
// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here

FORCE_INLINE uint32_t getblock32 ( const uint32_t * p, int i )
{
    return p[i];
}

FORCE_INLINE u64 getblock64 ( const u64 * p, int i )
{
    return p[i];
}

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

FORCE_INLINE uint32_t fmix32 ( uint32_t h )
{
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    
    return h;
}

//----------

FORCE_INLINE u64 fmix64 ( u64 k )
{
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xff51afd7ed558ccd);
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
    k ^= k >> 33;
    
    return k;
}

void MurmurHash3_x64_128( const void * key, const int len,
                         const uint32_t seed, void * out )
{
    const uint8_t * data = (const uint8_t*)key;
    const int nblocks = len / 16;
    
    u64 h1 = seed;
    u64 h2 = seed;
    
    const u64 c1 = BIG_CONSTANT(0x87c37b91114253d5);
    const u64 c2 = BIG_CONSTANT(0x4cf5ad432745937f);
    
    //----------
    // body
    
    const u64 * blocks = (const u64 *)(data);
    
    for(int i = 0; i < nblocks; i++)
    {
        u64 k1 = getblock64(blocks,i*2+0);
        u64 k2 = getblock64(blocks,i*2+1);
        
        k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
        
        h1 = ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;
        
        k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;
        
        h2 = ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
    }
    
    //----------
    // tail
    
    const uint8_t * tail = (const uint8_t*)(data + nblocks*16);
    
    u64 k1 = 0;
    u64 k2 = 0;
    
    switch(len & 15)
    {
        case 15: k2 ^= ((u64)tail[14]) << 48;
        case 14: k2 ^= ((u64)tail[13]) << 40;
        case 13: k2 ^= ((u64)tail[12]) << 32;
        case 12: k2 ^= ((u64)tail[11]) << 24;
        case 11: k2 ^= ((u64)tail[10]) << 16;
        case 10: k2 ^= ((u64)tail[ 9]) << 8;
        case  9: k2 ^= ((u64)tail[ 8]) << 0;
        k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;
        
        case  8: k1 ^= ((u64)tail[ 7]) << 56;
        case  7: k1 ^= ((u64)tail[ 6]) << 48;
        case  6: k1 ^= ((u64)tail[ 5]) << 40;
        case  5: k1 ^= ((u64)tail[ 4]) << 32;
        case  4: k1 ^= ((u64)tail[ 3]) << 24;
        case  3: k1 ^= ((u64)tail[ 2]) << 16;
        case  2: k1 ^= ((u64)tail[ 1]) << 8;
        case  1: k1 ^= ((u64)tail[ 0]) << 0;
        k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
    };
    
    //----------
    // finalization
    
    h1 ^= len; h2 ^= len;
    
    h1 += h2;
    h2 += h1;
    
    h1 = fmix64(h1);
    h2 = fmix64(h2);
    
    h1 += h2;
    h2 += h1;
    
    ((u64*)out)[0] = h1;
    ((u64*)out)[1] = h2;
}

#endif