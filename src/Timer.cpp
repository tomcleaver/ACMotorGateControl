#include "Timer.h"

CTimer::CTimer(String _TimerName)
{
    TimerName = _TimerName;
};

void CTimer::SetTimer(float Seconds)
{
    RunTime = static_cast<unsigned long>(Seconds);
}

void CTimer::StartTimer()
{
    StartTime = millis();
}

void CTimer::Pause()
{
    bIsPaused = true;
}

void CTimer::Resume()
{
    bIsPaused = false;
    StartTime = millis() + ElapsedTimeSeconds;
    Update();
}

void CTimer::Reset()
{
    bIsPaused = false;
    ElapsedTimeSeconds = 0;
    bHasCompleted = false;
    StartTimer();
}

bool CTimer::Update()
{

    if (!bHasCompleted)
    {
        if (!bIsPaused)
        {
            ElapsedTimeSeconds = (millis() - StartTime) / 1000;
        }

        if (bDebugTimer)
        {
            Serial.print(TimerName);
            Serial.print(" - Time Elapsed = ");
            Serial.println(ElapsedTimeSeconds);
        }

        bHasCompleted = ElapsedTimeSeconds > RunTime;
    }

    return bHasCompleted;
}

void CTimer::SetDebugTimer(bool NewDebug)
{
    bDebugTimer = NewDebug;
}