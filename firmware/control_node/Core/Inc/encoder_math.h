#ifndef ENCODER_MATH_H
#define ENCODER_MATH_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    ENCODER_DIRECTION_STATIONARY = 0,
    ENCODER_DIRECTION_FORWARD,
    ENCODER_DIRECTION_REVERSE
} EncoderDirection_t;

typedef struct
{
    uint16_t previous_counter;
    int64_t accumulated_count;
    int32_t delta_count;
    float rpm;
    EncoderDirection_t direction;
    bool initialized;
} EncoderTracker_t;

/**
 * Initializes a tracker from the current value of a 16-bit timer counter.
 */
void EncoderMath_Init(
    EncoderTracker_t *tracker,
    uint16_t initial_counter);

/**
 * Returns the signed modular difference between two 16-bit counter samples.
 * The result is unambiguous when fewer than 32768 counts occur per sample.
 */
int32_t EncoderMath_CalculateDelta(
    uint16_t previous_counter,
    uint16_t current_counter);

/**
 * Updates accumulated position, signed delta, direction, and output-shaft RPM.
 */
bool EncoderMath_Update(
    EncoderTracker_t *tracker,
    uint16_t current_counter,
    uint32_t elapsed_ms,
    float counts_per_revolution,
    bool invert_direction);

#endif
