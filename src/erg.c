
#include <libopencm3/stm32/adc.h>
#include <libopencm3/cm3/nvic.h>

/**
 * http://www.atm.ox.ac.uk/rowing/physics/ergometer.html
 */

#include "erg.h"
#include "led.h"
#include "clock.h"

#define STROKE 1
#define RECOVERY 2
uint32_t stroke_state = RECOVERY;

/** time of last peak */
static uint32_t last_peak_time_us = 0;
/** delta between peaks */
static volatile uint32_t time_since_last_peak_us = 0;

/** angular velocity in rad/s */
static float omega = 0.0f;
/** angular acceleration */
static float b = 0.0f;

/** moment of inertia */
static const float I = 1.0f;


/** drag factor */
static float k = 1.0;
static float recovery_start_omega = 0.0f;
static float recovery_end_omega = 0.1f;
static float recovery_start_time = 0.0f;
static float recovery_end_time = 0.1f;

void erg_init(void)
{
    nvic_enable_irq(NVIC_ADC_IRQ);
}

float erg_drag_factor(void);
float erg_drag_factor(void)
{
    return I \
        / (recovery_end_omega - recovery_start_omega) \
        / (recovery_end_time - recovery_start_time);
}

float erg_get_power(void)
{
    return k * omega * omega * omega;
}

float erg_get_omega(void)
{
    return omega;
}

float erg_get_b(void)
{
    return b;
}

float erg_get_rpm(void)
{
    if (time_since_last_peak_us)
    {
        return 1000000.0f / time_since_last_peak_us;
    }
    return 0.0f;
}

uint32_t erg_get_time_diff(void)
{
    return time_since_last_peak_us;
}

void erg_update(void)
{
    float last_omega = omega;
    if (time_since_last_peak_us)
    {
        omega = 1.0e6 / time_since_last_peak_us;
        b = (omega - last_omega) / time_since_last_peak_us;
    }
}

void adc_isr(void)
{
    //uint16_t value;
    uint32_t time;
    uint32_t temp_diff;
    static uint32_t rising = 1;

    adc_disable_analog_watchdog_regular(ADC1);
    //adc_disable_awd_interrupt(ADC1);
    //value = adc_read_regular(ADC1);

    if (ADC_SR(ADC1) & ADC_SR_AWD)
    {
        ADC_SR(ADC1) &= ~ADC_SR_AWD;
    }

    if (rising)
    {
        /* rising edge */
        led_set();
        time = get_time_us();
        temp_diff = time - last_peak_time_us;
        if (temp_diff > 15000)
        {
            time_since_last_peak_us = temp_diff;
            last_peak_time_us = time;
        }
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
    }

    //adc_enable_awd_interrupt(ADC1);
    adc_enable_analog_watchdog_regular(ADC1);
}
