
#include <libopencm3/stm32/adc.h>
#include <libopencm3/cm3/nvic.h>

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
    static const int ST_RISING = 1;
    static const int ST_FALLING = 2;
    static int state = ST_RISING;

    adc_disable_awd_interrupt(ADC1);

    if (ADC_SR(ADC1) & ADC_SR_AWD)
    {
        ADC_SR(ADC1) &= ~ADC_SR_AWD;
    }

    if (state == ST_RISING)
    {
        /* Get ready to detect falling edge */
        adc_set_watchdog_high_threshold(ADC1, 0xffff);
        adc_set_watchdog_low_threshold(ADC1, 100);
        led_set();
        state = ST_FALLING;
    }
    else
    {
        /* Get ready to detect rising edge */
        adc_set_watchdog_high_threshold(ADC1, 100);
        adc_set_watchdog_low_threshold(ADC1, 0);
        led_clear();
        uint32_t t = get_time_us();
        time_since_last_peak_us = t - last_peak_time_us;
        last_peak_time_us = t;
        state = ST_RISING;
    }

    adc_enable_awd_interrupt(ADC1);
}
