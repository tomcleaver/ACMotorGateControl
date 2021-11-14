#include "Timer.h"

CTimer::CTimer()
{

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
    StartTimer();
    Update();
}

bool CTimer::Update()
{ 
    if(!bIsPaused)
    {
        ElapsedTimeSeconds = (millis() - StartTime) / 1000;
    }

    if(bDebugTimer)
    {
        Serial.print("Time Elapsed = ");
        Serial.println(ElapsedTimeSeconds);
    }

    if(ElapsedTimeSeconds < RunTime)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void CTimer::SetDebugTimer(bool NewDebug)
{
    bDebugTimer = NewDebug;
}