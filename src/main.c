
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
#include "sdram.h"
#include "lcd-dma.h"
#include "gfx.h"

#define LED_DISCO_GREEN_PORT GPIOG
#define LED_DISCO_GREEN_PIN GPIO13

int _write(int file, char *ptr, int len);
static void adc_setup(void);
static void dac_setup(void);
static uint16_t read_last_adc(void);
static void capture_data(void);

#define ADC_BUF_SIZE (90*1024)
static uint16_t adc_data[ADC_BUF_SIZE];
uint32_t saved_bytes = 0;

static volatile int adc_interrupt = 0;

int main(void)
{
    clock_setup();
    adc_setup();
    dac_setup();
    cdcacm_init();
    sdram_init();
    lcd_dma_init();
    gfx_init(lcd_dma_draw_pixel, LCD_WIDTH, LCD_HEIGHT);
    gfx_setRotation(2, LCD_WIDTH, LCD_HEIGHT);
    gfx_setTextSize(5);
    gfx_setTextColor(GFX_COLOR_WHITE, GFX_COLOR_BLACK);

    /* Enable GPIOD clock for LED */
    rcc_periph_clock_enable(RCC_GPIOG);
    gpio_mode_setup(LED_DISCO_GREEN_PORT, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, LED_DISCO_GREEN_PIN);
    gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);

    adc_enable_awd_interrupt(ADC1);

    uint16_t adc0_value;
    while (1)
    {
        if (read_buf_len && read_buf[0] == 'p')
        {
            adc0_value = read_last_adc();
            printf("adc0 = %"PRIu16"\r\n", adc0_value);
            printf("  time ms = %"PRIu32", us = %"PRIu32"\r\n",
                   get_time_ms(), get_time_us());
            gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
        }

        if (read_buf_len && read_buf[0] == 'c')
        {
            gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
            capture_data();
            gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
        }

        if (adc_interrupt)
        {
            adc0_value = read_last_adc();
            printf("ADC interrupt = %d, val = %"PRIu16"\r\n",
                   adc_interrupt, adc0_value);
            adc_interrupt = 0;
        }

        if (lcd_dma_buffer_ready())
        {
            char str[10];
            sprintf(str, "%6"PRIu32, get_time_ms() % 1000000);
            gfx_setCursor(0, 0);
            gfx_puts(str);
            lcd_dma_swap_buffers();
        }

        read_buf_len = 0;
        cdcacm_poll();
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


static void adc_setup(void)
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
    adc_set_watchdog_high_threshold(ADC1, 2000);
    adc_set_watchdog_low_threshold(ADC1, 0);

    adc_enable_analog_watchdog_on_all_channels(ADC1);
    adc_enable_analog_watchdog_regular(ADC1);

    nvic_enable_irq(NVIC_ADC_IRQ);

    //adc_set_resolution(ADC1, ADC_CR1_RES_12BIT); // or 10, 8, 6 - faster

    adc_power_on(ADC1);
    adc_start_conversion_regular(ADC1);
}



void adc_isr(void)
{
    static const int ST_RISING = 1;
    static const int ST_FALLING = 2;
    static int state = ST_RISING;

    adc_disable_awd_interrupt(ADC1);

    adc_interrupt += 1;

    if (ADC_SR(ADC1) & ADC_SR_AWD)
    {
        ADC_SR(ADC1) &= ~ADC_SR_AWD;
    }

    if (state == ST_RISING)
    {
        /* Get ready to detect falling edge */
        adc_set_watchdog_high_threshold(ADC1, 0xffff);
        adc_set_watchdog_low_threshold(ADC1, 2000);
        gpio_set(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
        state = ST_FALLING;
    }
    else
    {
        /* Get ready to detect rising edge */
        adc_set_watchdog_high_threshold(ADC1, 2000);
        adc_set_watchdog_low_threshold(ADC1, 0);
        gpio_clear(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
        state = ST_RISING;
    }

    adc_enable_awd_interrupt(ADC1);
}

static void dac_setup(void)
{
    rcc_periph_clock_enable(RCC_DAC);
    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO5);

    dac_disable(CHANNEL_2);
    dac_disable_waveform_generation(CHANNEL_2);
    dac_enable(CHANNEL_2);
    dac_set_trigger_source(DAC_CR_TSEL2_SW);
}

static uint16_t read_last_adc(void)
{
    while (!adc_eoc(ADC1));
    return adc_read_regular(ADC1);
}

static void capture_data(void)
{
    int i;
    uint32_t start_time, end_time;
    start_time = get_time_us();
    for (i=0; i<ADC_BUF_SIZE; i++)
    {
        adc_data[i] = read_last_adc();
    }
    end_time = get_time_us();

    printf("c %d %"PRIu32" %"PRIu32"\r\n",
           ADC_BUF_SIZE, start_time, end_time);
    for (i=0; i<ADC_BUF_SIZE; i++)
    {
        printf("s %"PRIu16"\r\n", adc_data[i]);
    }
    printf("d\r\n");
}
