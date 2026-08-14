#ifndef ENCODER_DRIVER_H
#define ENCODER_DRIVER_H

#include "encoder_math.h"
#include "main.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    ENCODER_STATUS_OK = 0,
    ENCODER_STATUS_INVALID_ARGUMENT,
    ENCODER_STATUS_NOT_INITIALIZED,
    ENCODER_STATUS_HAL_ERROR
} EncoderStatus_t;

typedef struct
{
    TIM_HandleTypeDef *timer;
    float counts_per_output_revolution;
    uint32_t sample_period_ms;
    bool invert_direction;
} EncoderConfig_t;

/**
 * Starts both TIM encoder channels and captures the initial 16-bit count.
 */
EncoderStatus_t Encoder_Init(
    const EncoderConfig_t *config,
    uint32_t now_ms);

/**
 * Samples the counter when the configured nonblocking interval has elapsed.
 */
EncoderStatus_t Encoder_Process(uint32_t now_ms);

/** Continuous signed position relative to Encoder_Init(). */
int64_t Encoder_GetCount(void);

/** Most recent signed count change over one sample interval. */
int32_t Encoder_GetDelta(void);

/** Signed gearbox-output speed. */
float Encoder_GetRPM(void);

EncoderDirection_t Encoder_GetDirection(void);
bool Encoder_IsInitialized(void);

#endif
