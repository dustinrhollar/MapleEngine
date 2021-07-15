#ifndef _STR_POOL_H
#define _STR_POOL_H

// TODO(Dustin): Distingush between ASCII, UTF8, and UTF16 

//typedef u64 STR_POOL_ID;
struct STR_POOL_ID
{
    u64 v;
    static const STR_POOL_ID INVALID;
};
const STR_POOL_ID STR_POOL_ID::INVALID = { (u64) 1.8446744e+19 };

FORCE_INLINE
bool operator==(STR_POOL_ID left, STR_POOL_ID right)
{
    return left.v == right.v;
}

FORCE_INLINE
bool operator!=(STR_POOL_ID left, STR_POOL_ID right)
{
    return left.v != right.v;
}

struct StrPool
{
    void   *backing_memory;
    memory *str_memory;
    
    struct StrPair *string_table;
    u32 table_count;
    u32 table_cap;
    
    static StrPool Init(u64 size);
    void Shutdown();
    
    STR_POOL_ID Inject(const char *str, u32 len);
    void Eject(STR_POOL_ID *sid);
    
    // Returns the number of copied chars into the buffer
    // if the buffer was large enough
    // RETURN VALUE DOES NOT INCLUDE NULL TERMINATOR
    u32 StrIdToStr(STR_POOL_ID sid, char *buf, u32 size);
    
    static bool CompareStrId(STR_POOL_ID left, STR_POOL_ID right)
    {
        return CompareHash64(left.v, right.v);
    }
    
    static STR_POOL_ID GetStrId(const char *str, u32 len)
    {
        return { MummurHash64(str, len) };
    }
    
    static const STR_POOL_ID INVALID;
    
    private:
    
    STR_POOL_ID AddStrPairToTable(StrPair pair);
    StrPair RemoveStrPairFromTable(STR_POOL_ID sid);
    StrPair *GetStrPairFromTable(STR_POOL_ID sid);
    void ResizeStrPairTable(u64 new_cap);
    
    static const r32 LOAD_FACTOR;
    static const i32 INITIAL_CAP; 
};

#endif //_STR_POOL_H

#if defined(MAPLE_STR_POOL_IMPLEMENTATION)

const r32 StrPool::LOAD_FACTOR = 0.75;
const i32 StrPool::INITIAL_CAP = 500;
const STR_POOL_ID StrPool::INVALID = { (u64)1.8446744e+19 }; // 2^64-1

struct StrPair : Hasher
{
    char       *heap;
    u32         heap_size;
    STR_POOL_ID sid; // key
    
    u64 Hash64()
    {
        return MummurHash64(heap, heap_size);
    }
};

StrPool StrPool::Init(u64 size)
{
    StrPool result = {};
    
    size += sizeof(memory);
    result.backing_memory = MemAlloc(size);
    memory_init(&result.str_memory, size, result.backing_memory);
    
    result.string_table = 0;
    arrsetlen(result.string_table, INITIAL_CAP);
    for (u32 i = 0; i < INITIAL_CAP; ++i) 
        result.string_table[i].sid = INVALID; 
    
    result.table_count = 0;
    result.table_cap   = INITIAL_CAP;
    
    return result;
}

void StrPool::Shutdown()
{
    memory_free(&str_memory);
    MemFree(backing_memory);
    arrfree(string_table);
    table_count = 0;
    table_cap   = 0;
}

STR_POOL_ID StrPool::Inject(const char *str, u32 len)
{
    STR_POOL_ID result = {0};
    
    StrPair pair = {};
    pair.heap_size = len;
    
    if (str && len > 0)
    {
        pair.heap_size = len;
        pair.heap = (char*)memory_alloc(str_memory, len + 1);
        memcpy(pair.heap, str, len);
        pair.heap[pair.heap_size] = 0;
        
        result = AddStrPairToTable(pair);
    }
    else
    {
        result = INVALID;
    }
    
    return result;
}

void StrPool::Eject(STR_POOL_ID *sid)
{
    StrPair result = RemoveStrPairFromTable(*sid);
    
    if (result.sid != INVALID)
    {
        memory_release(str_memory, result.heap);
        result.heap = 0;
    }
#if 1
    else
    {
        LogError("Attempted to remove a string from the STRING_POOL with invalid id.\n");
    }
#endif
    
    *sid = INVALID;
}

u32 StrPool::StrIdToStr(STR_POOL_ID sid, char *buf, u32 size)
{
    const char *result = 0;
    u32 heap_size = 0;
    
    StrPair *str_pair = GetStrPairFromTable(sid);
    if (str_pair)
    {
        result = str_pair->heap;
        heap_size = str_pair->heap_size;
    }
    
    if (result && heap_size <= size)
    {
        memcpy(buf, result, heap_size);
    }
    
    return heap_size;
}


STR_POOL_ID StrPool::AddStrPairToTable(StrPair pair)
{
    STR_POOL_ID result = STR_POOL_ID::INVALID;
    
    if (pair.heap)
    {
        pair.sid = { pair.Hash64() };
        
        r32 current_load = (r32)(table_count + 1) / (r32)table_cap;
        if (current_load >= LOAD_FACTOR)
        {
            ResizeStrPairTable(table_cap * 2);
        }
        
        u32 start_idx = pair.sid.v % table_cap;
        for (u32 i = 0; i < table_cap; ++i)
        {
            u32 idx = (start_idx + i) % table_cap;
            
            // StrPair at this entry
            StrPair *to_cmp = string_table + idx;
            
            if (to_cmp->sid == STR_POOL_ID::INVALID)
            {
                // No entry at this index, go ahead and add
                // the string
                *to_cmp = pair;
                result = pair.sid;
                table_count++;
                break;
            }
#if 1
            else if (CompareHash64(to_cmp->sid.v, pair.sid.v))
            {
                // Posible string duplicate
                if (strncmp(pair.heap, to_cmp->heap, fast_min(pair.heap_size, to_cmp->heap_size)) == 0)
                {
                    // Attempting to add a duplicate string. Will I allow this?
                    // Log the duplicate and move on until i decide
                    LogInfo("Attempting to add a duplicate string. String will be added at next available index.\n");
                    LogInfo("\tExisting String: %s\n", to_cmp->heap);
                    LogInfo("\tNew String:      %s\n", pair.heap);
                }
            }
#endif
        }
    }
    
    return result;
}

StrPair StrPool::RemoveStrPairFromTable(STR_POOL_ID sid)
{
    // TODO(Dustin): Look into inserting tombstones
    // rather than immediately removing from the table
    
    StrPair *str_pair = GetStrPairFromTable(sid);
    if (str_pair)
    {
        StrPair cpy = *str_pair;
        
        str_pair->sid = INVALID;
        str_pair->heap = 0;
        str_pair->heap_size = 0;
        table_count--;
        return cpy;
    }
    else
    {
        StrPair invalid = {};
        invalid.sid = INVALID;
        invalid.heap = 0;
        return invalid;
    }
}

StrPair *StrPool::GetStrPairFromTable(STR_POOL_ID sid)
{
    StrPair *result = 0;
    
    if (sid != STR_POOL_ID::INVALID)
    {
        u32 start_idx = sid.v % table_cap;
        for (u32 i = 0; i < table_cap; ++i)
        {
            u32 idx = (start_idx + i) % table_cap;
            
            // StrPair at this entry
            StrPair *to_cmp = string_table + idx;
            
            if (to_cmp->sid != STR_POOL_ID::INVALID &&
                CompareHash64(to_cmp->sid.v, sid.v))
            {
                result = to_cmp;
                break;
            }
        }
    }
    
    return result;
}

void StrPool::ResizeStrPairTable(u64 new_cap)
{
    LogError("Resizing str pair table in StrPool\n");
    arrsetcap(string_table, new_cap);
    for (u32 i = table_cap; i < new_cap; ++i)
        string_table[i].sid = INVALID; 
    table_cap = (u32)new_cap;
}

#endif // MAPLE_STR_POOL_IMPLEMENTATION