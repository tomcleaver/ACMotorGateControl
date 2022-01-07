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
    TimerState = ETimerState::Running;
    StartTime = millis();
}

void CTimer::Pause()
{
    TimerState = ETimerState::Paused;
}

void CTimer::Resume()
{
    TimerState = ETimerState::Running;
    StartTime = millis() + ElapsedTimeSeconds;
    Update();
}

void CTimer::Reset()
{
    TimerState = ETimerState::None;
    ElapsedTimeSeconds = 0;
}

void CTimer::Update()
{
    if (TimerState == ETimerState::Running)
    {
        // If the timer has not reached the set time, update the elapsed time
        if (!EvaluateRuntime())
        {
            ElapsedTimeSeconds = (millis() - StartTime) / 1000;

            if (bDebugTimer)
            {
                Serial.print(TimerName);
                Serial.print(" - Time Elapsed = ");
                Serial.println(ElapsedTimeSeconds);
            }
        }
        else
        {
            TimerState = ETimerState::Complete;
        }
    }
}

ETimerState CTimer::GetTimerState()
{
    return TimerState;
}

void CTimer::SetDebugTimer(bool NewDebug)
{
    bDebugTimer = NewDebug;
}