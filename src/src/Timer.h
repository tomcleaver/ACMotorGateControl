#pragma once
#include <Arduino.h>

class CTimer
{
public:

    CTimer(String _TimerName);

    // Sets time to complete, does not start timer
    void SetTimer(float Seconds);

    void StartTimer();

    void Pause();

    void Resume();

    // Return true if timer has gone over the limit, if true is returned the timer should be reset or destroyed
    bool Update();

    void Reset();

    void SetDebugTimer(bool NewDebug);

private:

    // Amount of seconds this timer will run
    unsigned long RunTime{};
    unsigned long StartTime{};
    unsigned long ElapsedTimeSeconds{};
    bool bIsPaused = false;
    bool bDebugTimer = false;
    String TimerName{""};
    bool bHasCompleted = false;
};