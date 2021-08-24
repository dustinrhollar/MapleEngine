
//-----------------------------------------------------------------------------

struct Timer 
{
    u64 start;
};

static void GlobalTimerSetup();
static void TimerBegin(Timer *timer);
static r32  TimerSecondsElapsed(Timer *timer);
static r32  TimerMiliSecondsElapsed(Timer *timer);
static r32  TimerNanoSecondsElapsed(Timer *timer);

FORCE_INLINE r32 SecondsToNanoSeconds(r32 Seconds)
{
    return Seconds * 1000000000.0f;
}

FORCE_INLINE r32 NanoSecondsToSeconds(r32 NanoSeconds)
{
    return NanoSeconds / 1000000000.0f;
}

FORCE_INLINE r32 SecondsToMiliSeconds(r32 Seconds)
{
    return Seconds * 1000.0f;
}

FORCE_INLINE r32 MiliSecondsToSeconds(r32 MiliSeconds)
{
    return MiliSeconds / 1000.0f;
}

//-----------------------------------------------------------------------------

file_global i64 g_performance_frequency;

file_internal void 
GlobalTimerSetup()
{
    LARGE_INTEGER perf_count_frequency_result;
    QueryPerformanceFrequency(&perf_count_frequency_result);
    g_performance_frequency = perf_count_frequency_result.QuadPart;
    UINT desired_scheduler_ms = 1;
    // Check the time result?
    (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
}

file_internal void 
TimerBegin(Timer *timer)
{
    LARGE_INTEGER local_start;
    QueryPerformanceCounter(&local_start);
    timer->start = local_start.QuadPart;
}

file_internal r32 
TimerSecondsElapsed(Timer *timer)
{
    LARGE_INTEGER local_counter;
    QueryPerformanceCounter(&local_counter);
    r32 Result = ((r32)(local_counter.QuadPart - timer->start) / (r32)g_performance_frequency);
    return(Result);
}

file_internal r32 
TimerMiliSecondsElapsed(Timer *timer)
{
    LARGE_INTEGER local_counter;
    QueryPerformanceCounter(&local_counter);
    r32 Result = ((r32)(local_counter.QuadPart - timer->start) * 1000 / (r32)g_performance_frequency);
    return(Result);
}

file_internal r32 
TimerNanoSecondsElapsed(Timer *timer)
{
    LARGE_INTEGER local_counter;
    QueryPerformanceCounter(&local_counter);
    r32 Result = ((r32)(local_counter.QuadPart - timer->start) * 1000000000.0f / (r32)g_performance_frequency);
    return(Result);
}
