
// A very straightforward threadpool. Nothing clever or complex...

struct Win32ThreadPoolTask
{
    void (*fn)(void*);
    void *arg;
};

struct Win32ThreadPool
{
    CRITICAL_SECTION     cs_lock;
    CONDITION_VARIABLE   notify;
    HANDLE              *threads;
    Win32ThreadPoolTask *tasks;
    i32                  thread_count;
    i32                  queue_size;
    i32                  head;
    i32                  tail;
    i32                  count;
    i32                  started; // number of started threads
    b8                   shutdown;
};

file_internal void Win32ThreadPoolInit(Win32ThreadPool **result, i32 thread_count, i32 queue_max);
file_internal void Win32ThreadPoolFree(Win32ThreadPool **pool);
file_internal void Win32ThreadQueueTask(Win32ThreadPool *pool, void (*fn)(void*), void *arg);
file_internal DWORD WINAPI Win32ThreadPoolThread(LPVOID lp_param); 

file_internal void Win32ThreadPoolInit(Win32ThreadPool **result, i32 thread_count, i32 queue_max)
{
    Win32ThreadPool *pool = (Win32ThreadPool*)SysAlloc(sizeof(Win32ThreadPool));
    pool->thread_count = thread_count;
    pool->queue_size = queue_max;
    pool->head = pool->tail = pool->count = 0;
    pool->started = pool->shutdown = 0;
    pool->threads = (HANDLE*)SysAlloc(sizeof(HANDLE) * pool->thread_count);
    pool->tasks = (Win32ThreadPoolTask*)SysAlloc(sizeof(Win32ThreadPoolTask) * pool->queue_size);
    
    InitializeConditionVariable(&pool->notify);
    InitializeCriticalSectionAndSpinCount(&pool->cs_lock, 0x00000400);
    
    // Clear all thread handles to reset memory
    for (i32 i = 0; i < pool->thread_count; ++i)
    {
        pool->threads[i] = NULL;
    }
    
    for (i32 i = 0; i < pool->thread_count; ++i)
    {
        pool->threads[i] = CreateThread(NULL, 0, Win32ThreadPoolThread, (void*)pool, 0, NULL);
        if (pool->threads[i] == NULL)
        {
            LogError("Win32ThreadPool::Init::Failed to create all threads!");
            goto GOTO_ERR;
        }
        
        pool->started++;
    }
    
    *result = pool;
    return;
    
    // @GOTO
    GOTO_ERR:
    Win32ThreadPoolFree(&pool);
    *result = 0;
}

file_internal void Win32ThreadPoolFree(Win32ThreadPool **pool)
{
    Win32ThreadPool *local = *pool;
    if (!local) return;
    
    EnterCriticalSection(&local->cs_lock);
    if(!local->shutdown) 
    {
        local->shutdown = true;
        WakeAllConditionVariable(&local->notify);
        LeaveCriticalSection(&local->cs_lock);
        
        // Now, just wait for all threads to die
        
        // NOTE(Dustin): WinApi places a limit on the number of handles that can be
        // waited on at a single. If the thread count is greater than MAXIMUM_WAIT_OBJECTS
        // then wait for the threads to die in batches.
        i32 thread_count = local->thread_count;
        i32 start_wait = 0;
        while (thread_count > 0)
        {
            i32 wait_count = (thread_count > MAXIMUM_WAIT_OBJECTS) ? MAXIMUM_WAIT_OBJECTS : thread_count;
            DWORD wait_res = WaitForMultipleObjects(wait_count, local->threads, TRUE, INFINITE);
            DWORD abd = WAIT_ABANDONED_0;
            if (wait_res >= WAIT_ABANDONED_0 && wait_res < WAIT_ABANDONED_0 + wait_count)
            {
                LogError("Win32ThreadPool::Free::Thread_Wait::Wait_Abandoned");
                return;
            }
            else if (wait_res == WAIT_TIMEOUT)
            {
                LogError("Win32ThreadPool::Free::Thread_Wait::Wait_Timeout");
                return;
            }
            else if (wait_res == WAIT_FAILED)
            {
                DWORD wait_res = GetLastError();
                LogError("Win32ThreadPool::Free::Thread_Wait::Wait_Failed %d", wait_res);
                
                return;
            }
            
            start_wait += wait_count;
            thread_count -= wait_count;
        }
        
        SysFree(local->tasks);
        if (local->threads)
        {
            for (i32 i = 0; i < local->thread_count; ++i)
            {
                if (local->threads[i]) CloseHandle(local->threads[i]);
                local->threads[i] = NULL;
            }
            SysFree(local->threads);
        }
        
        DeleteCriticalSection(&local->cs_lock);
        
        *pool = 0;
        SysFree(local);
    }
    else
    {
        LogError("Win32ThreadPool::Free::Already shutting down...");
    }
}

file_internal void Win32ThreadQueueTask(Win32ThreadPool *pool, void (*fn)(void*), void *arg)
{
    if (!pool || !fn) 
    {
        return;
    }
    
    i32 next;
    
    EnterCriticalSection(&pool->cs_lock);
    
    next = (pool->tail + 1) % pool->queue_size;
    
    do {
        if (pool->count == pool->queue_size) 
        {
            LogWarn("Win32ThreadPool::AddTask::Attempting to add tasks to Thread Pool work list, but pool is full!");
            break;
        }
        
        if (pool->shutdown)
        {
            LogWarn("Win32ThreadPool::AddTask::Cannot add task after Pool has been shutdown");
            break;
        }
        
        pool->tasks[pool->tail].fn = fn;
        pool->tasks[pool->tail].arg = arg;
        pool->tail = next;
        pool->count++;
        
        WakeAllConditionVariable(&pool->notify);
        
    } while(0);
    
    LeaveCriticalSection(&pool->cs_lock);
}

file_internal DWORD WINAPI Win32ThreadPoolThread(LPVOID lp_param)
{
    Win32ThreadPool *pool = (Win32ThreadPool*)lp_param;
    Win32ThreadPoolTask task;
    
    for (;;)
    {
        EnterCriticalSection(&pool->cs_lock);
        
        while (pool->count == 0 && !pool->shutdown)
        {
            // NOTE(Dustin): Should I worry about the return value?
            SleepConditionVariableCS(&pool->notify, &pool->cs_lock, INFINITE);
        }
        
        // Will finish all tasks in the queue.
        // Might want to introduce a way for immediate shutdowns
        if (pool->shutdown && pool->count == 0) break;
        
        // Get the task
        task.fn = pool->tasks[pool->head].fn;
        task.arg = pool->tasks[pool->head].arg;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count--;
        
        LeaveCriticalSection(&pool->cs_lock);
        
        (*(task.fn))(task.arg);
    }
    
    pool->started--;
    LeaveCriticalSection(&pool->cs_lock);
    
    return (0);
}