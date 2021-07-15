
file_global memory_t g_app_memory = 0;

void SysMemoryInit(void *ptr, u64 size)
{
    memory_init(&g_app_memory, size, ptr);
}

void SysMemoryFree()
{
    memory_free(&g_app_memory);
}

void* SysMemoryAlloc(u64 size)
{
    return memory_alloc(g_app_memory, size);
}

void SysMemoryRelease(void *ptr)
{
    memory_release(g_app_memory, ptr);
}

void* SysMemoryRealloc(void *ptr, u64 size)
{
    return memory_realloc(g_app_memory, ptr, size);
}
