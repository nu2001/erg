
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>

#include "cdcacm.h"
#include "clock.h"
#include "led.h"
#include "ui.h"
#include "erg.h"

int _write(int file, char *ptr, int len);

static void adc_init(void);
static uint16_t read_last_adc(void);
static void capture_data(void);

#define ADC_BUF_SIZE (90*1024)
static uint16_t adc_data[ADC_BUF_SIZE];
uint32_t saved_bytes = 0;

int main(void)
{
    clock_setup();
    led_init();
    adc_init();
    cdcacm_init();
    erg_init();
    ui_init();

    adc_enable_awd_interrupt(ADC1);

    uint16_t adc0_value;
    while (1)
    {
        cdcacm_poll();
        if (read_buf_len && read_buf[0] == 'p')
        {
            adc0_value = read_last_adc();
            printf("adc0 = %"PRIu16"\r\n", adc0_value);
            printf("  time ms = %"PRIu32", us = %"PRIu32"\r\n",
                   get_time_ms(), get_time_us());
            led_toggle();
        }

        if (read_buf_len && read_buf[0] == 'c')
        {
            led_toggle();
            capture_data();
            led_toggle();
        }

        erg_update();
        ui_update();

        read_buf_len = 0;
    }
}


/**
 * Use USART_CONSOLE as a console.
 * This is a syscall for newlib
 * @param file
 * @param ptr
 * @param len
 * @return
 */
int _write(int file, char *ptr, int len)
{
    if (file == STDOUT_FILENO || file == STDERR_FILENO)
    {
        return cdcacm_write(ptr, len);
    }
    errno = EIO;
    return -1;
}


static void adc_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_ADC1);

    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0);
    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);

    adc_off(ADC1);
    adc_disable_scan_mode(ADC1);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_3CYC);

    adc_disable_scan_mode(ADC1);
    uint8_t channel_array[16];
    channel_array[0] = ADC_CHANNEL0;
    adc_set_regular_sequence(ADC1, 1, channel_array);
    adc_set_continuous_conversion_mode(ADC1);

    /* get ready to detect rising edge*/
    adc_set_watchdog_high_threshold(ADC1, ERG_ADC_HIGH_THRES);
    adc_set_watchdog_low_threshold(ADC1, 0);

    //adc_set_sample_time(ADC1, ADC_CR1_AWDCH_CHANNEL0,
    //                    ADC_SMPR_SMP_144CYC);
    adc_set_sample_time(ADC1, ADC_CR1_AWDCH_CHANNEL0,
                        ADC_SMPR_SMP_480CYC);
    adc_enable_analog_watchdog_on_selected_channel(ADC1, ADC_CR1_AWDCH_CHANNEL0);
    adc_enable_analog_watchdog_regular(ADC1);

    //adc_set_resolution(ADC1, ADC_CR1_RES_12BIT); // or 10, 8, 6 - faster

    adc_power_on(ADC1);
    adc_start_conversion_regular(ADC1);
}


static uint16_t read_last_adc(void)
{
    while (!adc_eoc(ADC1));
    return adc_read_regular(ADC1);
}

static void capture_data(void)
{
    int i, j;
    uint32_t start_time, end_time;
    adc_disable_awd_interrupt(ADC1);
    start_time = get_time_us();
    for (i=0; i<ADC_BUF_SIZE; i++)
    {
        adc_data[i] = read_last_adc();
        for (j=0; j<50; j++)
        {
            asm("nop");
        }
    }
    end_time = get_time_us();

    printf("c %d %"PRIu32" %"PRIu32"\r\n",
           ADC_BUF_SIZE, start_time, end_time);
    for (i=0; i<ADC_BUF_SIZE; i++)
    {
        printf("s %"PRIu16"\r\n", adc_data[i]);
    }
    printf("d\r\n");
    adc_enable_awd_interrupt(ADC1);
}
