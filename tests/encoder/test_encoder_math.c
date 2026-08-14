#include "encoder_math.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


#define OUTPUT_COUNTS_PER_REVOLUTION 3591.84f
#define FLOAT_TOLERANCE              0.01f


static unsigned int assertions_run = 0U;
static unsigned int assertions_failed = 0U;


static void ExpectTrue(
    bool condition,
    const char *expression,
    int line)
{
    assertions_run++;

    if (!condition)
    {
        assertions_failed++;
        (void)printf(
            "FAIL line %d: %s\n",
            line,
            expression
        );
    }
}


static void ExpectInt64Equal(
    int64_t expected,
    int64_t actual,
    const char *expression,
    int line)
{
    assertions_run++;

    if (expected != actual)
    {
        assertions_failed++;
        (void)printf(
            "FAIL line %d: %s expected %lld, got %lld\n",
            line,
            expression,
            (long long)expected,
            (long long)actual
        );
    }
}


static void ExpectFloatNear(
    float expected,
    float actual,
    float tolerance,
    const char *expression,
    int line)
{
    assertions_run++;

    if (fabsf(expected - actual) > tolerance)
    {
        assertions_failed++;
        (void)printf(
            "FAIL line %d: %s expected %.6f, got %.6f\n",
            line,
            expression,
            (double)expected,
            (double)actual
        );
    }
}


#define EXPECT_TRUE(expression) \
    ExpectTrue((expression), #expression, __LINE__)

#define EXPECT_EQ(expected, actual) \
    ExpectInt64Equal( \
        (int64_t)(expected), \
        (int64_t)(actual), \
        #actual, \
        __LINE__ \
    )

#define EXPECT_NEAR(expected, actual) \
    ExpectFloatNear( \
        (expected), \
        (actual), \
        FLOAT_TOLERANCE, \
        #actual, \
        __LINE__ \
    )


static float ExpectedRPM(
    int32_t delta,
    uint32_t elapsed_ms)
{
    return ((float)delta * 60000.0f) /
        (OUTPUT_COUNTS_PER_REVOLUTION * (float)elapsed_ms);
}


static void TestInitialization(void)
{
    EncoderTracker_t tracker = {0};

    EncoderMath_Init(&tracker, 1234U);

    EXPECT_TRUE(tracker.initialized);
    EXPECT_EQ(1234, tracker.previous_counter);
    EXPECT_EQ(0, tracker.accumulated_count);
    EXPECT_EQ(0, tracker.delta_count);
    EXPECT_NEAR(0.0f, tracker.rpm);
    EXPECT_EQ(
        ENCODER_DIRECTION_STATIONARY,
        tracker.direction
    );

    /* A null destination must be harmless. */
    EncoderMath_Init(NULL, 0U);
    EXPECT_TRUE(true);
}


static void TestSignedCounterDelta(void)
{
    EXPECT_EQ(100, EncoderMath_CalculateDelta(1000U, 1100U));
    EXPECT_EQ(-100, EncoderMath_CalculateDelta(1100U, 1000U));
    EXPECT_EQ(0, EncoderMath_CalculateDelta(500U, 500U));

    /* Forward and reverse crossings of the 16-bit rollover point. */
    EXPECT_EQ(11, EncoderMath_CalculateDelta(65530U, 5U));
    EXPECT_EQ(-11, EncoderMath_CalculateDelta(5U, 65530U));

    EXPECT_EQ(30000, EncoderMath_CalculateDelta(1000U, 31000U));
    EXPECT_EQ(-30000, EncoderMath_CalculateDelta(31000U, 1000U));
}


static void TestPositiveNegativeAndZeroSpeed(void)
{
    EncoderTracker_t tracker = {0};
    EncoderMath_Init(&tracker, 1000U);

    EXPECT_TRUE(EncoderMath_Update(
        &tracker,
        1060U,
        10U,
        OUTPUT_COUNTS_PER_REVOLUTION,
        false
    ));
    EXPECT_EQ(60, tracker.delta_count);
    EXPECT_EQ(60, tracker.accumulated_count);
    EXPECT_NEAR(ExpectedRPM(60, 10U), tracker.rpm);
    EXPECT_EQ(ENCODER_DIRECTION_FORWARD, tracker.direction);

    EXPECT_TRUE(EncoderMath_Update(
        &tracker,
        1030U,
        10U,
        OUTPUT_COUNTS_PER_REVOLUTION,
        false
    ));
    EXPECT_EQ(-30, tracker.delta_count);
    EXPECT_EQ(30, tracker.accumulated_count);
    EXPECT_NEAR(ExpectedRPM(-30, 10U), tracker.rpm);
    EXPECT_EQ(ENCODER_DIRECTION_REVERSE, tracker.direction);

    EXPECT_TRUE(EncoderMath_Update(
        &tracker,
        1030U,
        10U,
        OUTPUT_COUNTS_PER_REVOLUTION,
        false
    ));
    EXPECT_EQ(0, tracker.delta_count);
    EXPECT_EQ(30, tracker.accumulated_count);
    EXPECT_NEAR(0.0f, tracker.rpm);
    EXPECT_EQ(ENCODER_DIRECTION_STATIONARY, tracker.direction);
}


static void TestWraparoundUpdates(void)
{
    EncoderTracker_t tracker = {0};
    EncoderMath_Init(&tracker, 65530U);

    EXPECT_TRUE(EncoderMath_Update(
        &tracker,
        5U,
        10U,
        OUTPUT_COUNTS_PER_REVOLUTION,
        false
    ));
    EXPECT_EQ(11, tracker.delta_count);
    EXPECT_EQ(11, tracker.accumulated_count);
    EXPECT_NEAR(ExpectedRPM(11, 10U), tracker.rpm);

    EncoderMath_Init(&tracker, 5U);
    EXPECT_TRUE(EncoderMath_Update(
        &tracker,
        65530U,
        10U,
        OUTPUT_COUNTS_PER_REVOLUTION,
        false
    ));
    EXPECT_EQ(-11, tracker.delta_count);
    EXPECT_EQ(-11, tracker.accumulated_count);
    EXPECT_NEAR(ExpectedRPM(-11, 10U), tracker.rpm);
}


static void TestRPMConversionAndHighRate(void)
{
    EncoderTracker_t tracker = {0};
    EncoderMath_Init(&tracker, 0U);

    EXPECT_TRUE(EncoderMath_Update(
        &tracker,
        3592U,
        1000U,
        OUTPUT_COUNTS_PER_REVOLUTION,
        false
    ));
    EXPECT_NEAR(ExpectedRPM(3592, 1000U), tracker.rpm);
    EXPECT_NEAR(60.0027f, tracker.rpm);

    EncoderMath_Init(&tracker, 1000U);
    EXPECT_TRUE(EncoderMath_Update(
        &tracker,
        31000U,
        1U,
        OUTPUT_COUNTS_PER_REVOLUTION,
        false
    ));
    EXPECT_EQ(30000, tracker.delta_count);
    EXPECT_NEAR(ExpectedRPM(30000, 1U), tracker.rpm);
    EXPECT_TRUE(isfinite(tracker.rpm));
}


static void TestDirectionInversion(void)
{
    EncoderTracker_t tracker = {0};
    EncoderMath_Init(&tracker, 100U);

    EXPECT_TRUE(EncoderMath_Update(
        &tracker,
        125U,
        10U,
        OUTPUT_COUNTS_PER_REVOLUTION,
        true
    ));
    EXPECT_EQ(-25, tracker.delta_count);
    EXPECT_EQ(-25, tracker.accumulated_count);
    EXPECT_NEAR(ExpectedRPM(-25, 10U), tracker.rpm);
    EXPECT_EQ(ENCODER_DIRECTION_REVERSE, tracker.direction);
}


static void TestInvalidInputs(void)
{
    EncoderTracker_t tracker = {0};

    EXPECT_TRUE(!EncoderMath_Update(
        NULL,
        1U,
        10U,
        OUTPUT_COUNTS_PER_REVOLUTION,
        false
    ));

    EXPECT_TRUE(!EncoderMath_Update(
        &tracker,
        1U,
        10U,
        OUTPUT_COUNTS_PER_REVOLUTION,
        false
    ));

    EncoderMath_Init(&tracker, 10U);

    EXPECT_TRUE(!EncoderMath_Update(
        &tracker,
        20U,
        0U,
        OUTPUT_COUNTS_PER_REVOLUTION,
        false
    ));
    EXPECT_TRUE(!EncoderMath_Update(
        &tracker,
        20U,
        10U,
        0.0f,
        false
    ));
    EXPECT_TRUE(!EncoderMath_Update(
        &tracker,
        20U,
        10U,
        NAN,
        false
    ));
    EXPECT_TRUE(!EncoderMath_Update(
        &tracker,
        20U,
        10U,
        INFINITY,
        false
    ));

    /* Invalid samples leave the previous valid tracker state untouched. */
    EXPECT_EQ(10, tracker.previous_counter);
    EXPECT_EQ(0, tracker.accumulated_count);
}


int main(void)
{
    TestInitialization();
    TestSignedCounterDelta();
    TestPositiveNegativeAndZeroSpeed();
    TestWraparoundUpdates();
    TestRPMConversionAndHighRate();
    TestDirectionInversion();
    TestInvalidInputs();

    if (assertions_failed != 0U)
    {
        (void)printf(
            "%u of %u encoder assertions failed.\n",
            assertions_failed,
            assertions_run
        );
        return 1;
    }

    (void)printf(
        "All %u encoder assertions passed.\n",
        assertions_run
    );

    return 0;
}
