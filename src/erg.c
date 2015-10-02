#include <libopencm3/stm32/adc.h>
#include <libopencm3/cm3/nvic.h>

#include <math.h>
#include <string.h>

/**
 * http://www.atm.ox.ac.uk/rowing/physics/ergometer.html
 */

#include "erg.h"
#include "led.h"
#include "clock.h"

#define STROKE 1
#define RECOVERY 2

static const float INERTIA = 0.1001f; /** moment of inertia */
static const float DIST_CONST = 2.8f; /** concept2 const for dist conversion */

static volatile uint32_t revs_counter = 0;

void erg_init(struct erg_status_s * status)
{
    nvic_enable_irq(NVIC_ADC_IRQ);
    memset(status, 0, sizeof(struct erg_status_s));
    status->drag_factor = 150.f; //< initial guess in mid range
}

void erg_update_status(struct erg_status_s * status)
{
    /** do the erg math here */
    uint32_t time_ms = get_time_ms();
    status->elapsed_time = 1000.0f * time_ms;

    uint32_t revs_time = get_time_us();
    uint32_t revs = revs_counter;
    revs_counter = 0; /** reset revs counter. Will be incremented in adc_isr */

    static uint32_t last_revs_time = 0;
    uint32_t d_revs_time;
    if (revs_time >= last_revs_time)
    {
        d_revs_time = revs_time - last_revs_time;
    }
    else
    {
        /** timer wrapped around (see get_time_us comment) */
        d_revs_time = 0xffffffff - last_revs_time + revs_time;
    }
    last_revs_time = revs_time;

    static float last_angular_speed = 0.f;
    float angular_speed = (float)revs / d_revs_time;
    float angular_accel = (angular_speed - last_angular_speed) / (d_revs_time);
    last_angular_speed = angular_speed;

    status->distance += pow(status->drag_factor / DIST_CONST, 1.f / 3) *
        M_PI * 2 * revs;

    static uint32_t last_stroke_state = RECOVERY;
    uint32_t stroke_state = angular_accel > 0.f ? STROKE : RECOVERY;
    if (last_stroke_state != stroke_state)
    {
        static float stroke_start_speed = 0.f;
        static uint32_t stroke_start_time_ms = 0;
        static float recovery_start_speed = 0.f;
        static uint32_t recovery_start_time_ms = 0;
        static float recovery_start_distance = 0.f;

        if (stroke_state == STROKE)
        {
            stroke_start_time_ms = time_ms;
            stroke_start_speed = angular_speed;
            /** calculate drag from recovery */
            float d_speed = recovery_start_speed - stroke_start_speed;
            uint32_t d_time = recovery_start_time_ms - stroke_start_time_ms;
            status->drag_factor = INERTIA * (1.0f / d_speed) / d_time;
        }
        else
        {
            /** End of stroke updates */
            float d_dist = status->distance - recovery_start_distance;
            uint32_t d_time = time_ms - recovery_start_time_ms;
            status->strokes_per_min = 60.f / d_time;

            float avg_stroke_speed = d_dist / d_time;
            status->split = 500.f / avg_stroke_speed;
            float avg_speed = status->distance / status->elapsed_time;
            status->average_split = 500.f / avg_speed;

            recovery_start_time_ms = time_ms;
            recovery_start_speed = angular_speed;
            recovery_start_distance = status->distance;
        }
    }
    last_stroke_state = stroke_state;
}


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
        led_set();
        /* get ready to detect falling edge */
        adc_set_watchdog_high_threshold(ADC1, 0xffff);
        adc_set_watchdog_low_threshold(ADC1, ERG_ADC_LOW_THRES);
        rising = 0;
    }
    else
    {
        /* falling edge */
        led_clear();
        /* get ready to detect rising edge */
        adc_set_watchdog_high_threshold(ADC1, ERG_ADC_HIGH_THRES);
        adc_set_watchdog_low_threshold(ADC1, 0);
        rising = 1;
        revs_counter += 1;
    }

    adc_enable_analog_watchdog_regular(ADC1);
}
