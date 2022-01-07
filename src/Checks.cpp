#include "Checks.h"

CChecks::CChecks()
{
    // on init, query the limit switches for position
    // if gate is open
    if(CheckOpenLimitSwitch() && !CheckClosedLimitSwitch())
    {
        GatePosition = EPosition::Open;
        LastGatePosition = EPosition::Unknown;
        MovementState = EMoveDirection::Idle;
    }
    else if(!CheckOpenLimitSwitch() && CheckClosedLimitSwitch())
    {
        GatePosition = EPosition::Closed;
        LastGatePosition = EPosition::Unknown;
        MovementState = EMoveDirection::Idle;
    }
    else
    {
        GatePosition =  EPosition::Unknown;
        MovementState = EMoveDirection::Idle;
    }
}

bool CChecks::CheckOpenLimitSwitch()
{
    return digitalRead(reedSwitchOpenPin);
}

bool CChecks::CheckClosedLimitSwitch()
{
    return digitalRead(reedSwitchClosedPin);
}

bool CChecks::CheckCommandSignalSwitch()
{
    return digitalRead(controlSignalPin);
}


bool CChecks::ProcessControlSignal()
{
   /* // if we have already received a command signal, don't allow new command until we've gone low first
    if(CheckCommandSignalSwitch() && !CommandSignalState)
    {
        // set true to establish we're processing a command signal
        CommandSignalState = true;
        Serial.println("Command Received");
    }
    // If the comman signal has gone low and our CommandSignalState is true means we have finished with this command
    else if(!CheckCommandSignalSwitch())
    {
        CommandSignalState = false;
    }

    return CommandSignalState;*/

    // return the state of the input pin, true means the radio has been triggered, a timer needs to disallow additional checks in main so we don't do this multiple times
    return CheckCommandSignalSwitch();
}

bool CChecks::CheckClosingLimit()
{
    bool bHasReachedLimit = false;
    // While we're EMoveDirection::Closing check whether we've reached the closed limit switch
    if(MovementState == EMoveDirection::Closing)
    {
        if(CheckClosedLimitSwitch())
        {
            bHasReachedLimit = true;
            GatePosition =  EPosition::Closed;
            LastGatePosition = GatePosition;
            Serial.println("Reached Closed Position!");
        }
    }

    return bHasReachedLimit;
}

bool CChecks::CheckOpeningLimit()
{
    bool bHasReachedLimit = false;

    // While we're EMoveDirection::Opening check whether we've reached the Open limit switch
    if(MovementState == EMoveDirection::Opening)
    {
        if(CheckOpenLimitSwitch())
        {
            bHasReachedLimit = true;   
            GatePosition = EPosition::Open;
            LastGatePosition = GatePosition;
            Serial.println("Reached Open Position!");
        }
    }

    return bHasReachedLimit;
}

void CChecks::CheckAndSetCurrentPosition()
{
    bool bIsOpen = CheckOpenLimitSwitch();
    bool bIsClosed = CheckClosedLimitSwitch();

    if(!bIsOpen && !bIsClosed)
    {
        GatePosition =  EPosition::Unknown;
       if(GatePosition != LastPositionDebug)
        {
            LastPositionDebug = GatePosition;
            Serial.println("Gate position unknown!");
        }
        return;
    }
    else if(bIsOpen && !bIsClosed)
    {
        GatePosition = EPosition::Open;      
        if(GatePosition != LastPositionDebug)
        {
            LastPositionDebug = GatePosition;
            Serial.println("Gate position open!");
        }
        return;
    }
    else if(!bIsOpen && bIsClosed)
    {
        GatePosition =  EPosition::Closed;       
        if(GatePosition != LastPositionDebug)
        {
            LastPositionDebug = GatePosition;
            Serial.println("Gate position closed!");
        }
        return;
    }    
    else if(bIsOpen && bIsClosed)
    {
        GatePosition =  EPosition::Unknown;       
        if(GatePosition != LastPositionDebug)
        {
            LastPositionDebug = GatePosition;
            Serial.println("Gate position unknown!");
        }
        return;
    }
}