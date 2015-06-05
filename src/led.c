
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "led.h"

#define LED_DISCO_GREEN_PORT GPIOG
#define LED_DISCO_GREEN_PIN GPIO13

void led_init(void)
{
    /* Enable GPIOD clock for LED */
    rcc_periph_clock_enable(RCC_GPIOG);
    gpio_mode_setup(LED_DISCO_GREEN_PORT, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, LED_DISCO_GREEN_PIN);
    gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
}

void led_toggle(void)
{
    gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
}

void led_set(void)
{
    gpio_set(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
}

void led_clear(void)
{
    gpio_clear(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
}
