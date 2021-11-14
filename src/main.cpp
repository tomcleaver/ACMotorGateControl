#include <Arduino.h>
#include "Pins.h"
#include "Checks.h"
#include "Enums.h"
#include "EEPROM.h"

CChecks *StateCheck = nullptr;
EMoveDirection LastMovementDirection{EMoveDirection::Idle};
ECommandState CommandState = ECommandState::Ready;
bool bTesting = true;
bool bWantsNewTimeoutRecording = false;
bool bOpenButtonPressAllowed = true;

int EEPROMTimeoutMemLoc = 0;
// value stored in EEProm which updates when a full open or close cycle completes
// ensures the motor doesn't stay on in the case of a failure with one of the
// limit switches... prevents motor overheat
int ActiveTimeout = 0;

// save any attempts here, if we complete a full cycle we update ActiveTimeout
int TemporaryTimeoutRecording = 0;

// will become true whenever button is pressed and then the gate opens or closes from an open or closed position
// will then become false if a cycle completes (in which case we update) or fails (in which case we disregard)
bool bIsRecordingNewTimeout = false;

// this stops the motor after it reaches ActiveTimeout
int TimeoutCounter = 0;

// if button pressed 2 times before timer max, set the active timeout recorder if only pressed once, open/close gate
int setTimeoutButtonPressCounter = 0;
int setTimeoutButtonPressTimer = -1;
int setTimeoutButtonPressTimerMax = 100;

//////////////// Master direction control ///////////////
void SetOpening()
{
  TimeoutCounter = 0;
  Serial.println("Opening State Set");
  StateCheck->SetMovementState(EMoveDirection::Opening);
  LastMovementDirection = EMoveDirection::Opening;
  digitalWrite(openLEDPin, HIGH);
  digitalWrite(closeLEDPin, LOW);
  digitalWrite(idleLEDPin, LOW);
  digitalWrite(relayControlClosePin, LOW);
  digitalWrite(relayControlOpenPin, HIGH);
}

void SetClosing()
{
  TimeoutCounter = 0;
  Serial.println("Closing State Set");
  StateCheck->SetMovementState(EMoveDirection::Closing);
  LastMovementDirection = EMoveDirection::Closing;
  digitalWrite(openLEDPin, LOW);
  digitalWrite(closeLEDPin, HIGH);
  digitalWrite(idleLEDPin, LOW);
  digitalWrite(relayControlOpenPin, LOW);
  digitalWrite(relayControlClosePin, HIGH);
}

void SetIdle()
{
  TimeoutCounter = 0;
  Serial.println("Idle State Set");
  StateCheck->SetMovementState(EMoveDirection::Idle);

  digitalWrite(openLEDPin, LOW);
  digitalWrite(closeLEDPin, LOW);
  digitalWrite(idleLEDPin, HIGH);
  digitalWrite(relayControlClosePin, LOW);
  digitalWrite(relayControlOpenPin, LOW);
}

///////////////////////////////////////////////////////////

void CommandAction()
{
  switch (StateCheck->GetMoveDirection())
  {
  case EMoveDirection::None:
    SetIdle();
    break;

  // if we're idle, check our position and move
  case EMoveDirection::Idle:

    switch (StateCheck->GetGatePosition())
    {
      // if we're open, close and process command
    case EPosition::Open:

      SetClosing();
      if (bWantsNewTimeoutRecording)
      {
        bIsRecordingNewTimeout = true;
      }
      StateCheck->LastGatePosition = EPosition::Open;
      break;

    // if we're closed, open and process command
    case EPosition::Closed:

      SetOpening();
      if (bWantsNewTimeoutRecording)
      {
        bIsRecordingNewTimeout = true;
      }
      StateCheck->LastGatePosition = EPosition::Closed;
      break;

    // if we don't know our position, we check first our last known direction and go opposite to that
    // this gives us the opposite movement if we stop and issue a new command
    case EPosition::Unknown:

      switch (LastMovementDirection)
      {
      case EMoveDirection::Opening:
        SetClosing();
        break;

      case EMoveDirection::Closing:
        SetOpening();
        break;

      default:
        SetOpening();
        break;
      }

      break;

    default:
      break;
    }
    break;

  // When opening and a command is triggered, set idle
  case EMoveDirection::Opening:
    SetIdle();
    break;

  // When closing and a command is triggered, set idle
  case EMoveDirection::Closing:
    SetIdle();
    break;
  }
}

void RecordNewActiveTimeout()
{
  Serial.print("TemporaryTimeoutRecording is ");
  Serial.println(TemporaryTimeoutRecording + (TemporaryTimeoutRecording / 10));
  Serial.print(" and active is ");
  Serial.print(ActiveTimeout);

  // if the new timeout is more than 10% different from the previous we record a new timeout
  if (TemporaryTimeoutRecording + (TemporaryTimeoutRecording / 10) != ActiveTimeout + (ActiveTimeout / 10))
  {
    // Add ten percent to make sure we don't cut off too early
    ActiveTimeout = abs(TemporaryTimeoutRecording + (TemporaryTimeoutRecording / 10));

    TemporaryTimeoutRecording = 0;
    EEPROM.put(EEPROMTimeoutMemLoc, static_cast<float>(ActiveTimeout));
    Serial.println("Saving new value");
  }
  else
  {
    Serial.println("Skipping Update");
    TemporaryTimeoutRecording = 0;
  }
  bWantsNewTimeoutRecording = false;
}

bool InitializeProgram()
{
  if (!StateCheck)
  {
    Serial.println("Initializing Program");
    StateCheck = new CChecks();
    float getEEPROM = 0.f;
    ActiveTimeout = static_cast<int>(EEPROM.get(EEPROMTimeoutMemLoc, getEEPROM));

    digitalWrite(openLEDPin, HIGH);
    digitalWrite(closeLEDPin, HIGH);
    delay(100);
    digitalWrite(openLEDPin, LOW);
    digitalWrite(closeLEDPin, LOW);
    delay(100);
    digitalWrite(openLEDPin, HIGH);
    digitalWrite(closeLEDPin, HIGH);
    delay(100);
    digitalWrite(openLEDPin, LOW);
    digitalWrite(closeLEDPin, LOW);
    Serial.print("Initialization Complete - Timeout is ");
    Serial.println(ActiveTimeout);
    return false;
  }
  else
  {
    return true;
  }
}

void setup()
{
  pinMode(closeLEDPin, OUTPUT);
  pinMode(openLEDPin, OUTPUT);
  pinMode(idleLEDPin, OUTPUT);
  pinMode(setSoftwareLimitSwitch, INPUT);
  pinMode(reedSwitchClosedPin, INPUT);
  pinMode(reedSwitchOpenPin, INPUT);
  Serial.begin(9600);
}

void loop()
{

  if (!InitializeProgram())
  {
    return;
  }

  // Set the gate position every frame, used to process movement directions and what to do if we're not at a limit switch
  StateCheck->CheckAndSetCurrentPosition();

  /////////////////// Button presses for opening gate or setting limit setup mode
  bool setLimitButtonState = digitalRead(setSoftwareLimitSwitch);

  // only allow one button press to be added per button release
  if (!bOpenButtonPressAllowed)
  {
    if (!setLimitButtonState)
    {
      bOpenButtonPressAllowed = true;
    }
  }
  else
  {
    if (setLimitButtonState && setTimeoutButtonPressTimer == -1)
    {
      setTimeoutButtonPressTimer = 0;
      Serial.println("setSoftwareLimitSwitch Pressed! Timer Started");
    }
  }

  bool commandSignal = false;
  bool setLimits = false;

  if (setTimeoutButtonPressTimer != -1)
  {
    if (setTimeoutButtonPressTimer < setTimeoutButtonPressTimerMax)
    {
      setTimeoutButtonPressTimer++;

      Serial.println(setTimeoutButtonPressTimer);
      if (setLimitButtonState)
      {
        if (bOpenButtonPressAllowed)
        {
          setTimeoutButtonPressCounter++;
          bOpenButtonPressAllowed = false;
        }
      }
    }
    else
    {

     /* if (setTimeoutButtonPressTimer == 3)
      {
        float memReset = 500;
        EEPROM.put(EEPROMTimeoutMemLoc, memReset);
        setTimeoutButtonPressCounter = 0;
        Serial.println("EEPROM reset!");
      }
      else */
      if (setTimeoutButtonPressCounter == 2)
      {
        setLimits = true;
        setTimeoutButtonPressCounter = 0;
      }
      else if (setTimeoutButtonPressCounter == 1)
      {
        commandSignal = true;
        setTimeoutButtonPressCounter = 0;
      }
      setTimeoutButtonPressTimer = -1;
    }
  }

  if (setLimits && !bWantsNewTimeoutRecording)
  {
    Serial.println("setSoftwareLimitSwitch Pressed!");
    bWantsNewTimeoutRecording = true;
    digitalWrite(idleLEDPin, LOW);
    digitalWrite(openLEDPin, HIGH);
    digitalWrite(closeLEDPin, HIGH);
    delay(500);
    digitalWrite(openLEDPin, LOW);
    digitalWrite(closeLEDPin, LOW);
    delay(500);
    digitalWrite(openLEDPin, HIGH);
    digitalWrite(closeLEDPin, HIGH);
    delay(500);
    digitalWrite(openLEDPin, LOW);
    digitalWrite(closeLEDPin, LOW);
  }

  // If our command state from the last loop was acknowledged, process the command once.
  if (CommandState == ECommandState::Acknowledged)
  {
    CommandAction();
    CommandState = ECommandState::Processing;
    Serial.println(" Command Acknowledged!");
    delay(1000);
    Serial.println(" Ready for new command!");
  }

  // Query our command signal state
  if ((StateCheck->ProcessControlSignal() && CommandState == ECommandState::Ready) || commandSignal == true)
  {
    CommandState = ECommandState::Acknowledged;
    commandSignal = false;
  }

  // to stop the gate while operating
  else if (StateCheck->ProcessControlSignal() && CommandState == ECommandState::Processing)
  {
    SetIdle();
    Serial.println("Process Interrupted! Set IDLE");
    bIsRecordingNewTimeout = false;
    TemporaryTimeoutRecording = 0;
    if (bTesting)
      delay(2000);
    CommandState = ECommandState::Ready;
  }

  if (CommandState == ECommandState::Processing)
  {
    // Run the timeout while processing
    if (!bIsRecordingNewTimeout || !bWantsNewTimeoutRecording)
    {
      TimeoutCounter++;
      Serial.println(TimeoutCounter);
      if (TimeoutCounter >= ActiveTimeout)
      {
        SetIdle();
        Serial.println("Opening Timed Out");
        TimeoutCounter = 0;
        return;
      }
    }

    // check to see if we've reached the open position
    switch (StateCheck->GetMoveDirection())
    {
    case EMoveDirection::Opening:

      if (bIsRecordingNewTimeout)
      {
        TemporaryTimeoutRecording++;
        Serial.print("Opening time = ");
        Serial.println(TemporaryTimeoutRecording);
      }

      if (StateCheck->GetGatePosition() == EPosition::Open)
      {
        // Reached open position while opening
        Serial.println("Open position reached! Set IDLE");

        // we have completed a full opening cycle, record the new value if significantly different
        // from previous recordings
        if (bIsRecordingNewTimeout)
        {
          bIsRecordingNewTimeout = false;
          RecordNewActiveTimeout();
        }

        SetIdle();
        CommandState = ECommandState::Ready;
        if (bTesting)
          delay(2000);
      }
      break;

    case EMoveDirection::Closing:
      if (bIsRecordingNewTimeout)
      {
        TemporaryTimeoutRecording++;
        Serial.print("Closing time = ");
        Serial.println(TemporaryTimeoutRecording);
      }

      if (StateCheck->GetGatePosition() == EPosition::Closed)
      {
        // Reached closed position while closing
        Serial.println("Closed position reached! Set IDLE");

        // we have completed a full closing cycle, record the new value if significantly different
        // from previous recordings
        if (bIsRecordingNewTimeout)
        {
          bIsRecordingNewTimeout = false;
          RecordNewActiveTimeout();
        }

        SetIdle();
        CommandState = ECommandState::Ready;
        if (bTesting)
          delay(2000);
      }
      break;

    case EMoveDirection::Idle:

    default:

      Serial.println("Error: Default case. Set IDLE");
      SetIdle();
      CommandState = ECommandState::Ready;
      if (bTesting)
        delay(2000);
    }
  }
}