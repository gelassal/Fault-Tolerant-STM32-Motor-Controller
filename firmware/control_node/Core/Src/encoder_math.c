#include "encoder_math.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>


void EncoderMath_Init(
    EncoderTracker_t *tracker,
    uint16_t initial_counter)
{
    if (tracker == NULL)
    {
        return;
    }

    tracker->previous_counter = initial_counter;
    tracker->accumulated_count = 0;
    tracker->delta_count = 0;
    tracker->rpm = 0.0f;
    tracker->direction = ENCODER_DIRECTION_STATIONARY;
    tracker->initialized = true;
}


int32_t EncoderMath_CalculateDelta(
    uint16_t previous_counter,
    uint16_t current_counter)
{
    uint16_t modular_difference =
        (uint16_t)(current_counter - previous_counter);

    if (modular_difference <= (uint16_t)INT16_MAX)
    {
        return (int32_t)modular_difference;
    }

    return -((int32_t)UINT16_MAX -
             (int32_t)modular_difference + 1);
}


bool EncoderMath_Update(
    EncoderTracker_t *tracker,
    uint16_t current_counter,
    uint32_t elapsed_ms,
    float counts_per_revolution,
    bool invert_direction)
{
    if ((tracker == NULL) ||
        (!tracker->initialized) ||
        (elapsed_ms == 0U) ||
        (!isfinite(counts_per_revolution)) ||
        (counts_per_revolution <= 0.0f))
    {
        return false;
    }

    int32_t delta = EncoderMath_CalculateDelta(
        tracker->previous_counter,
        current_counter
    );

    if (invert_direction)
    {
        delta = -delta;
    }

    tracker->previous_counter = current_counter;
    tracker->delta_count = delta;
    tracker->accumulated_count += (int64_t)delta;
    tracker->rpm =
        ((float)delta * 60000.0f) /
        (counts_per_revolution * (float)elapsed_ms);

    if (delta > 0)
    {
        tracker->direction = ENCODER_DIRECTION_FORWARD;
    }
    else if (delta < 0)
    {
        tracker->direction = ENCODER_DIRECTION_REVERSE;
    }
    else
    {
        tracker->direction = ENCODER_DIRECTION_STATIONARY;
    }

    return true;
}
