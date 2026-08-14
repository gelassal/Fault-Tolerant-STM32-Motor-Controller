#include "encoder_driver.h"

#include <math.h>
#include <stddef.h>


static EncoderConfig_t encoder_config;
static EncoderTracker_t encoder_tracker;
static uint32_t last_sample_ms = 0U;
static bool encoder_initialized = false;


EncoderStatus_t Encoder_Init(
    const EncoderConfig_t *config,
    uint32_t now_ms)
{
    if ((config == NULL) ||
        (config->timer == NULL) ||
        (config->sample_period_ms == 0U) ||
        (!isfinite(config->counts_per_output_revolution)) ||
        (config->counts_per_output_revolution <= 0.0f))
    {
        return ENCODER_STATUS_INVALID_ARGUMENT;
    }

    encoder_initialized = false;
    encoder_config = *config;

    __HAL_TIM_SET_COUNTER(encoder_config.timer, 0U);

    if (HAL_TIM_Encoder_Start(
            encoder_config.timer,
            TIM_CHANNEL_ALL) != HAL_OK)
    {
        return ENCODER_STATUS_HAL_ERROR;
    }

    EncoderMath_Init(&encoder_tracker, 0U);
    last_sample_ms = now_ms;
    encoder_initialized = true;

    return ENCODER_STATUS_OK;
}


EncoderStatus_t Encoder_Process(uint32_t now_ms)
{
    if (!encoder_initialized)
    {
        return ENCODER_STATUS_NOT_INITIALIZED;
    }

    uint32_t elapsed_ms = now_ms - last_sample_ms;

    if (elapsed_ms < encoder_config.sample_period_ms)
    {
        return ENCODER_STATUS_OK;
    }

    uint16_t current_counter = (uint16_t)
        __HAL_TIM_GET_COUNTER(encoder_config.timer);

    if (!EncoderMath_Update(
            &encoder_tracker,
            current_counter,
            elapsed_ms,
            encoder_config.counts_per_output_revolution,
            encoder_config.invert_direction))
    {
        return ENCODER_STATUS_INVALID_ARGUMENT;
    }

    last_sample_ms = now_ms;

    return ENCODER_STATUS_OK;
}


int64_t Encoder_GetCount(void)
{
    return encoder_initialized
        ? encoder_tracker.accumulated_count
        : 0;
}


int32_t Encoder_GetDelta(void)
{
    return encoder_initialized
        ? encoder_tracker.delta_count
        : 0;
}


float Encoder_GetRPM(void)
{
    return encoder_initialized
        ? encoder_tracker.rpm
        : 0.0f;
}


EncoderDirection_t Encoder_GetDirection(void)
{
    return encoder_initialized
        ? encoder_tracker.direction
        : ENCODER_DIRECTION_STATIONARY;
}


bool Encoder_IsInitialized(void)
{
    return encoder_initialized;
}
