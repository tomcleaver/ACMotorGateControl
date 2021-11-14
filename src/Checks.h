#include "Pins.h"
#include <Arduino.h>
#include "Enums.h"

class CChecks
{
public:
    CChecks();
    // Queries the state of the Open limit switch - true if high
    bool CheckOpenLimitSwitch();

    // Queries the state of the Closed limit switch - true if high
    bool CheckClosedLimitSwitch();

    // Queries the state of the Command Control Pin - returns true if high
    bool CheckCommandSignalSwitch();

    // Controls when we can return the command signal state to ready to receive
    // when the command signal is received we say that the state is Active until the command signal goes low again
    // this is because the signal sent by the receiver module is HIGH for ~1 second
    bool ProcessControlSignal();

    // returns true if we've reached the closed limit switch if we're EMoveDirection::Closing
    bool CheckClosingLimit();

    // returns true if we've reached the open limit switch if we're opening
    bool CheckOpeningLimit();

    void SetMovementState(EMoveDirection NewState) { MovementState = NewState; };

    EPosition GetGatePosition() { return GatePosition; };

    EMoveDirection GetMoveDirection() { return MovementState; };

    // Every frame we set the location based on the limit switches states
    void CheckAndSetCurrentPosition();

    // Our last idle position
    EPosition LastGatePosition = EPosition::None;

private:
    bool CommandSignalState = false;

    // Enum when we know our current position
    EPosition GatePosition = EPosition::None;

    // Our Movement Direction
    EMoveDirection MovementState = EMoveDirection::None;
};