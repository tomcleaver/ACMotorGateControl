#pragma once
#include <Arduino.h>

enum class ETimerState
{
    None,
    Paused,
    Running,
    Complete,
};

class CTimer
{
public:


    CTimer(String _TimerName);

    // Sets time to complete, does not start timer
    void SetTimer(float Seconds);

    void StartTimer();

    void Pause();

    void Resume();

    // Updates the TimerState enum to reflect its current state
    void Update();

    void Reset();
    
    ETimerState GetTimerState();

    void SetDebugTimer(bool NewDebug);

private:

    // Amount of seconds this timer will run
    unsigned long RunTime{};
    unsigned long StartTime{};
    unsigned long ElapsedTimeSeconds{};
    bool bDebugTimer = false;
    String TimerName{""};
    ETimerState TimerState = ETimerState::None;

    bool EvaluateRuntime() {return ElapsedTimeSeconds > RunTime; };
};