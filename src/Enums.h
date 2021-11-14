#pragma once
// Enum controlling the movement of the gate
enum class EMoveDirection
{
    None,
    Opening,
    Closing,
    Idle
};

enum class EPosition
{
    None,
    Open,
    Closed,
    Unknown
};

enum class ECommandState
{
    None,
    Ready,
    Acknowledged,
    Processing
};