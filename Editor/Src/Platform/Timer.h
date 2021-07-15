#ifndef _TIMER_H
#define _TIMER_H

struct Timer 
{
    u64 start;
};

void GlobalTimerSetup();
void TimerBegin(Timer *timer);
r32  TimerSecondsElapsed(Timer *timer);
r32  TimerMiliSecondsElapsed(Timer *timer);
r32  TimerNanoSecondsElapsed(Timer *timer);

#if 0
void global_timer_setup();
void timer_begin(Timer *timer);
r32  timer_seconds_elapsed(Timer *timer);
r32  timer_mili_seconds_elapsed(Timer *timer);
r32  timer_nano_seconds_elapsed(Timer *timer);
#endif

//FORCE_INLINE r32 seconds_to_nano_seconds(r32 Seconds)
FORCE_INLINE r32 SecondsToNanoSeconds(r32 Seconds)
{
    return Seconds * 1000000000.0f;
}

//FORCE_INLINE r32 nano_seconds_to_seconds(r32 NanoSeconds)
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

#endif //_WIN32_TIMER_H
