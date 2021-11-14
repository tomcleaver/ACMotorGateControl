#pragma once

#define openLEDPin 7 // LED on when gate is EMoveDirection::Opening

#define closeLEDPin 6 // LED on when gate is EMoveDirection::Closing

#define idleLEDPin 5 // LED on when gate is idle

#define controlSignalPin 3 // 5V is passed to this pin when the relay on the receiver is triggered by a controller, HIGH is received for ~1 second

#define relayControlOpenPin 11 // Pin that controls the SSR, gate will open when HIGH (pulled LOW);

#define relayControlClosePin 13 // Pin that controls the SSR, gate will close when HIGH (pulled LOW);

#define reedSwitchClosedPin 10 // Pin that when high indicates the closed position has been reached

#define reedSwitchOpenPin 9 // Pin that when high indicates the open position has been reached

#define setSoftwareLimitSwitch 4 // pin that when LOW sets the software limit mode, where a new software motor shutoff will be calculated