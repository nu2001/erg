
#ifndef _TEST_ON_PC
#include <libopencm3/stm32/adc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/cortex.h>
#endif

#include <math.h>
#include <string.h>

#include "erg.h"
#include "clock.h"

#ifndef _TEST_ON_PC
#include "led.h"
#endif

/**
   Erg flywheel has two holes that generate detectable signal.  We use
   ADC watchdog to throw an interrupt when analog signal rises above a
   threshold. Then we use the same ADC watchdog to throw an interrupt
   when analog value falls below a threshold. (Had to detect both
   rising and falling edges. If not, ADC watchdog kept calling
   interrupt all the time). So each full circle of the flywheel we get
   4 interrupts. Rising edge is only used to detect it and set a
   threshold for detecting falling edge. On falling edge we prepare
   for rising edge AND increment revolution counter and timestamp. The
   rest of the erg math is done before ui update (drag_factor,
   distance, elapsed time, 500m split, stroke rate, etc).
 */

/**
   Erg math and physics:
   http://www.atm.ox.ac.uk/rowing/physics/ergometer.html
 */


#define STROKE_STATE_STROKE 1
#define STROKE_STATE_RECOVERY 2

static const float INERTIA = 0.1001f; /** moment of inertia */
static const float DIST_CONST = 2.8f; /** concept2 const for dist conversion */

/* rolling average filter size for angular speed */
static const uint32_t ANG_SP_FILT_SZ = 30;

/* default and sanity checks for drag factor */
static const float DRAG_FACTOR_DEFAULT = 150.f;
static const float DRAG_FACTOR_MIN = 50.f;
static const float DRAG_FACTOR_MAX = 250.f;

#ifdef _TEST_ON_PC
volatile uint32_t revs_counter = 0;
volatile uint32_t rev_time_us = 0;
#else
static volatile uint32_t revs_counter = 0;
static volatile uint32_t rev_time_us = 0;
#endif

void erg_init(struct erg_status_s * status)
{
#ifndef _TEST_ON_PC
    nvic_enable_irq(NVIC_ADC_IRQ);
#endif

    memset(status, 0, sizeof(struct erg_status_s));
    status->drag_factor = DRAG_FACTOR_DEFAULT; //< initial guess in mid range
}

/** return delta time in seconds from microsecond inputs */
static float erg_delta_time_s(uint32_t start_us, uint32_t end_us)
{
    return (float)(get_delta_time(start_us, end_us)) * 1e-6f;
}

void erg_update_status(struct erg_status_s * status)
{
    /** do the erg math here */

    uint32_t revs;
    uint32_t time_us;
    static uint32_t last_time_us = 0;

#ifndef _TEST_ON_PC
    cm_disable_interrupts();
#endif
    {
        revs = revs_counter;
        time_us = rev_time_us;
        revs_counter = 0; /** reset global counter */
    }
#ifndef _TEST_ON_PC
    cm_enable_interrupts();
#endif

    status->elapsed_time = 0.001f * get_time_ms();
    status->distance += (float)pow(status->drag_factor/DIST_CONST/1e6f, 1.f/3) * revs * (float)(M_PI);
    status->revs = revs;
    status->revs_time = time_us;

    static float filtered_angular_speed = 0.f;
    static float last_filtered_angular_speed = 0.f;

    uint32_t d_time_us = get_delta_time(last_time_us, time_us);
    if (d_time_us == 0)
        d_time_us = 1;

    /* rad / us */
    float instant_angular_speed =
        (float)(revs) / d_time_us;

    last_time_us = time_us;

    last_filtered_angular_speed = filtered_angular_speed;
    if (ANG_SP_FILT_SZ > revs)
    {
        filtered_angular_speed =
            filtered_angular_speed * (ANG_SP_FILT_SZ - revs - 1) / ANG_SP_FILT_SZ
            + instant_angular_speed * (revs + 1) / ANG_SP_FILT_SZ;
    }
    else
    {
        filtered_angular_speed = instant_angular_speed;
    }
    status->aaa = filtered_angular_speed;

    static uint32_t last_stroke_state = STROKE_STATE_RECOVERY;
    uint32_t stroke_state;
    if (filtered_angular_speed > last_filtered_angular_speed)
    {
        led_set();
        stroke_state = STROKE_STATE_STROKE;
    }
    else
    {
        led_clear();
        stroke_state = STROKE_STATE_RECOVERY;
    }

    static float stroke_start_ang_speed = 0.f;
    static uint32_t stroke_start_time_us = 0;
    static float recovery_start_ang_speed = 0.f;
    static uint32_t recovery_start_time_us = 0;
    static float recovery_start_distance = 0.f;
    if (last_stroke_state != stroke_state)
    {
        last_stroke_state = stroke_state;
        if (stroke_state == STROKE_STATE_STROKE)
        {
            /** calculate drag from recovery */
            stroke_start_time_us = time_us;
            stroke_start_ang_speed = filtered_angular_speed;

            float d_speed = recovery_start_ang_speed - stroke_start_ang_speed;
            uint32_t rec_d_time_us = get_delta_time(recovery_start_time_us,
                                                    stroke_start_time_us);

            status->drag_factor = -INERTIA * (1.0f / d_speed) / rec_d_time_us;
            //status->aaa = status->drag_factor;

            if (!(status->drag_factor > DRAG_FACTOR_MIN &&
                  status->drag_factor < DRAG_FACTOR_MAX))
            {
                status->drag_factor = DRAG_FACTOR_DEFAULT;
            }
        }
        else
        {
            /** End of stroke updates */
            static float last_recovery_start_distance = 0.f;
            recovery_start_time_us = time_us;
            recovery_start_ang_speed = filtered_angular_speed;
            recovery_start_distance = status->distance;

            float d_dist = recovery_start_distance
                - last_recovery_start_distance;
            last_recovery_start_distance = recovery_start_distance;

            static uint32_t last_recovery_start_time_us = 0;
            float d_time_s = erg_delta_time_s(last_recovery_start_time_us,
                                              recovery_start_time_us);
            last_recovery_start_time_us = recovery_start_time_us;

            status->strokes_per_min = 60.f / d_time_s;
            float avg_stroke_speed = d_dist / d_time_s;
            status->split = 500.f / avg_stroke_speed;
            float avg_speed = status->distance / status->elapsed_time;
            status->average_split = 500.f / avg_speed;
        }
    }
}

#ifndef _TEST_ON_PC
void adc_isr(void)
{
    static uint32_t rising = 1;

    adc_disable_analog_watchdog_regular(ADC1);

    /* clear interrupt flags */
    if (ADC_SR(ADC1) & ADC_SR_AWD)
    {
        ADC_SR(ADC1) &= ~ADC_SR_AWD;
    }

    if (rising)
    {
        /* rising edge */
        //led_set();
        /* get ready to detect falling edge */
        adc_set_watchdog_high_threshold(ADC1, 0xffff);
        adc_set_watchdog_low_threshold(ADC1, ERG_ADC_LOW_THRES);
        rising = 0;
    }
    else
    {
        /* falling edge */
        //led_clear();
        /* get ready to detect rising edge */
        adc_set_watchdog_high_threshold(ADC1, ERG_ADC_HIGH_THRES);
        adc_set_watchdog_low_threshold(ADC1, 0);
        rising = 1;
        /* label new half revolution */
        revs_counter += 1;
        rev_time_us = get_time_us();
    }

    adc_enable_analog_watchdog_regular(ADC1);
}
#endif
