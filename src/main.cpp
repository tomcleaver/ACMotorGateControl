#include <Arduino.h>
#include "Pins.h"
#include "Checks.h"
#include "Enums.h"
#include "EEPROM.h"
#include "Timer.h"

CChecks *StateCheck = nullptr;
EMoveDirection LastMovementDirection{EMoveDirection::Idle};
ECommandState CommandState = ECommandState::Ready;
bool bTesting = true;
bool bWantsNewTimeoutRecording = false;
bool bOpenButtonPressAllowed = true;

unsigned long EEPROMTimeoutMemLoc = 0;
// value stored in EEProm which updates when a full open or close cycle completes
// ensures the motor doesn't stay on in the case of a failure with one of the
// limit switches... prevents motor overheat
unsigned long ActiveTimeout = 10;

bool bHasRecordedStartTime = false;
// save any attempts here, if we complete a full cycle we update ActiveTimeout
unsigned long TemporaryTimeoutRecording = 0;
// will become true whenever button is pressed and then the gate opens or closes from an open or closed position
// will then become false if a cycle completes (in which case we update) or fails (in which case we disregard)
bool bIsRecordingNewTimeout = false;

// if button pressed 2 times before timer max, set the active timeout recorder if only pressed once, open/close gate
int setTimeoutButtonPressCounter = 0;
int setTimeoutButtonPressTimer = -1;
int setTimeoutButtonPressTimerMax = 100;

// Blink the closed or open LED while closing or opening
CTimer *BlinkLEDTimer = nullptr;
CTimer *TimeoutTimer = nullptr;

// Timer to disallow triggering a command after the radio module goes high, as it has an on time of around one second we
// must block triggering function on more than one frame per button press
CTimer *InputCooldownTimer = nullptr;

//////////////// Master direction control ///////////////
void SetOpening()
{
  TimeoutTimer->Reset();
  TimeoutTimer->StartTimer();

  // Allows gate to run, is set to ready when Idle is triggered by a button press during processing
  // or when the gate reaches a limit or timeout is triggered
  CommandState = ECommandState::Processing;

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
  TimeoutTimer->Reset();
  TimeoutTimer->StartTimer();

  // Allows gate to run, is set to ready when Idle is triggered by a button press during processing
  // or when the gate reaches a limit or timeout is triggered
  CommandState = ECommandState::Processing;

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
  TimeoutTimer->Reset();
  Serial.println("Idle State Set");
  StateCheck->SetMovementState(EMoveDirection::Idle);
  CommandState = ECommandState::Ready;
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

    case EPosition::Unknown:

      // In all cases, if the gate is stopped and the position is unknown, open
      // the gate, this is for maximum safety
      // Note - set closing, somehow the positions got reversed - TODO FIX THIS
      //SetOpening();
      SetClosing();

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
  unsigned long CompletedRecordingTimeMillis = millis();

  unsigned long NewSoftwareLimitTime = CompletedRecordingTimeMillis - TemporaryTimeoutRecording;

  // Only save larger value to prevent short stops in operation
  if ((NewSoftwareLimitTime+ (NewSoftwareLimitTime / 10) > ActiveTimeout)
  {
    // Add ten percent to make sure we don't cut off too early
    ActiveTimeout = abs(NewSoftwareLimitTime + (NewSoftwareLimitTime / 10)) / 1000;

    TemporaryTimeoutRecording = 0;
    EEPROM.put(EEPROMTimeoutMemLoc, static_cast<float>(ActiveTimeout));

    Serial.print("Saving new timeout = ");
    Serial.println(static_cast<float>(ActiveTimeout));
  }
  else
  {
    Serial.print("New Timeout Shorter than Existing, discarding new timeout value...");
  }

  bHasRecordedStartTime = false;
  bWantsNewTimeoutRecording = false;
}

bool InitializeProgram()
{
  if (!StateCheck)
  {
    Serial.println("Initializing Program");
    StateCheck = new CChecks();

    BlinkLEDTimer = new CTimer("BlinkTimer");
    BlinkLEDTimer->SetTimer(.5);
    BlinkLEDTimer->StartTimer();

    TimeoutTimer = new CTimer("TimeoutTimer");
    float getEEPROM = EEPROM.get(EEPROMTimeoutMemLoc, getEEPROM);
    TimeoutTimer->SetTimer(getEEPROM);
    ActiveTimeout = getEEPROM;
    Serial.print("Timeout = ");
    Serial.print(getEEPROM);
    Serial.println(" seconds");

    TimeoutTimer->SetDebugTimer(false);
    InputCooldownTimer = new CTimer("CooldownTimer");
    InputCooldownTimer->SetTimer(1.5);

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
  pinMode(relayControlClosePin, OUTPUT);
  pinMode(relayControlOpenPin, OUTPUT);
  pinMode(openLEDPin, OUTPUT);
  pinMode(idleLEDPin, OUTPUT);
  pinMode(setSoftwareLimitSwitch, INPUT);
  pinMode(reedSwitchClosedPin, INPUT);
  pinMode(reedSwitchOpenPin, INPUT);
  Serial.begin(9600);

  if (!InitializeProgram())
  {
    return;
  }
}

void UpdateTimers()
{
  TimeoutTimer->Update();
  InputCooldownTimer->Update();
  BlinkLEDTimer->Update();
}

void loop()
{
  UpdateTimers();

  // Set the gate position every frame, used to process movement directions and what to do if we're not at a limit switch
  StateCheck->CheckAndSetCurrentPosition();

  // Blink LED if Idle and at a limit
  if (StateCheck->GetMoveDirection() == EMoveDirection::Idle)
  {
    // blink the open or close position LED's while awaiting command
    if (BlinkLEDTimer->GetTimerState() == ETimerState::Complete)
    {
      BlinkLEDTimer->Reset();

      if (StateCheck->GetGatePosition() == EPosition::Closed)
      {
        digitalWrite(openLEDPin, LOW);
        digitalWrite(idleLEDPin, LOW);
        digitalWrite(closeLEDPin, !digitalRead(closeLEDPin));
      }
      else if (StateCheck->GetGatePosition() == EPosition::Open)
      {
        digitalWrite(closeLEDPin, LOW);
        digitalWrite(idleLEDPin, LOW);
        digitalWrite(openLEDPin, !digitalRead(openLEDPin));
      }
      else if (StateCheck->GetGatePosition() == EPosition::Unknown)
      {
        digitalWrite(closeLEDPin, LOW);
        digitalWrite(openLEDPin, LOW);
        digitalWrite(idleLEDPin, !digitalRead(idleLEDPin));
      }
    }
  }

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

  // Manual button presses
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
      if (setTimeoutButtonPressCounter == 2)
      {
        setLimits = true;
        setTimeoutButtonPressCounter = 0;
        Serial.println("Selected Set Limit Mode");
      }
      else if (setTimeoutButtonPressCounter == 1)
      {
        commandSignal = true;
        setTimeoutButtonPressCounter = 0;
        Serial.println("Manual Command Selected");
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

  //Manual button press occurred, skip the pin check and run the command actions
  if (commandSignal)
  {
    Serial.println("Manual Override Button Pressed");
    commandSignal = false;
    goto ManualCommand;
  }

  // Check if we have a high signal on the radio input
  if (StateCheck->ProcessControlSignal())
  {

  ManualCommand:
    // block reading any additional high inputs until the timer runs out, this allows time for the remote relay to switch off
    switch (InputCooldownTimer->GetTimerState())
    {
    // A new timer or a reset timer has a None ETimerState, therefor we can start the timer and start an action
    case ETimerState::None:

      // Set Input timer state to Running
      InputCooldownTimer->StartTimer();
      CommandAction();
      break;

    case ETimerState::Running:
      // Do nothing, we cannot trigger this more than once per button press
      break;
    case ETimerState::Complete:
      // Setting back to None state will now allow a new command action to be triggered by a button press
      InputCooldownTimer->Reset();
      break;

    default:
      break;
    }
  }

  //////////////// The gate is actioning the last command here ////////////////////////

  // Motor protection timeout is handled here - shuts off motor if the timer reaches completion
  // Set Idle returns the timer state to None, allowing a new start timer to occur with each command received
  if (TimeoutTimer->GetTimerState() == ETimerState::Complete)
  {
    // Run the timeout while processing
    if (!bIsRecordingNewTimeout || !bWantsNewTimeoutRecording)
    {
      {
        if (StateCheck->GetMoveDirection() == EMoveDirection::Opening)
        {
          Serial.println("Opening Timed Out");
        }
        else if (StateCheck->GetMoveDirection() == EMoveDirection::Closing)
        {
          Serial.println("Closing Timed Out");
        }

        SetIdle();
        return;
      }
    }
  }

  // if the gate is currently operating
  if (CommandState == ECommandState::Processing)
  {
    // check to see if we've reached the open position
    switch (StateCheck->GetMoveDirection())
    {
    case EMoveDirection::Opening:

      // Runs if we're setting a new timeout
      if (bIsRecordingNewTimeout)
      {
        if (!bHasRecordedStartTime)
        {
          bHasRecordedStartTime = true;
          TemporaryTimeoutRecording = millis(); //get the current "time" (actually the number of milliseconds since the program started)
          Serial.print("Start Time = ");
          Serial.println(TemporaryTimeoutRecording);
        }

        Serial.print("Setting new timeout - elapsed time(seconds) = ");
        Serial.println((millis() - TemporaryTimeoutRecording) / 1000);
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
        return;
      }
      break;

    case EMoveDirection::Closing:
      if (bIsRecordingNewTimeout)
      {
        if (!bHasRecordedStartTime)
        {
          bHasRecordedStartTime = true;
          TemporaryTimeoutRecording = millis(); //get the current "time" (actually the number of milliseconds since the program started)
          Serial.print("Start Time = ");
          Serial.println(TemporaryTimeoutRecording);
        }
      }

      if (StateCheck->GetGatePosition() == EPosition::Closed)
      {
        // Reached closed position while closing
        Serial.println("Closed position reached! Set IDLE");

        // we have completed a full closing cycle, record the new value
        if (bIsRecordingNewTimeout)
        {
          bIsRecordingNewTimeout = false;
          RecordNewActiveTimeout();
        }

        SetIdle();
        return;
      }
      break;

    case EMoveDirection::Idle:
      // If we're already idle, stay idle.
      Serial.println("Already Idle, gate must have been stopped manually or timed out.");
      break;

    default:
      Serial.println("Error: Default case. Set IDLE");
      SetIdle();
    }
  }
}