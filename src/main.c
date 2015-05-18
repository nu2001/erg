
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/usbd.h>

#include "cdcacm.h"
#include "tick.h"

#define LED_DISCO_GREEN_PORT GPIOG
#define LED_DISCO_GREEN_PIN GPIO13

static usbd_device *usbd_dev;

int _write(int file, char *ptr, int len);
static void clock_setup(void);
static void adc_setup(void);
static void dac_setup(void);
static uint16_t read_adc_naiive(uint8_t channel);

int main(void)
{
  clock_setup();
  adc_setup();
  dac_setup();
  systick_setup();

  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE,
		  GPIO13 | GPIO14 | GPIO15);
  gpio_set_af(GPIOB, GPIO_AF12, GPIO13 | GPIO14 | GPIO15);

  usbd_dev = cdcacm_init();

  gpio_mode_setup(LED_DISCO_GREEN_PORT, GPIO_MODE_OUTPUT,
  GPIO_PUPD_NONE, LED_DISCO_GREEN_PIN);

  int j = 0;
  uint32_t wake = system_millis + 1000;
  while (1) {
    if (wake < system_millis) {
      uint16_t input_adc0 = read_adc_naiive(0);
      uint16_t target = input_adc0 / 2;
      dac_load_data_buffer_single(target, RIGHT12, CHANNEL_2);
      dac_software_trigger(CHANNEL_2);
      uint16_t input_adc1 = read_adc_naiive(1);
      printf("tick: %d: adc0= %u, target adc1=%d, adc1=%d\r\n",
	     j++, input_adc0, target, input_adc1);
    
      /* LED on/off */
      gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
      wake = system_millis + 1000;
    }

    usbd_poll(usbd_dev);
  }

}


static void clock_setup(void)
{
  //rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_120MHZ]); // USB
  rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);

  /* USB */
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_OTGHS);

  /* Enable GPIOD clock for LED & USARTs. */
  rcc_periph_clock_enable(RCC_GPIOG);
  //rcc_periph_clock_enable(RCC_GPIOA);
  
  /* Enable clocks for USART2 and dac */
  //rcc_periph_clock_enable(RCC_USART1);
  rcc_periph_clock_enable(RCC_DAC);
  
  /* And ADC*/
  rcc_periph_clock_enable(RCC_ADC1);
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
  if (file == STDOUT_FILENO || file == STDERR_FILENO) {
    return cdcacm_write(usbd_dev, ptr, len);
  }
  errno = EIO;
  return -1;
}


static void adc_setup(void)
{
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0);
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);

  adc_off(ADC1);
  adc_disable_scan_mode(ADC1);
  adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_3CYC);

  adc_power_on(ADC1);
}


static void dac_setup(void)
{
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO5);
  dac_disable(CHANNEL_2);
  dac_disable_waveform_generation(CHANNEL_2);
  dac_enable(CHANNEL_2);
  dac_set_trigger_source(DAC_CR_TSEL2_SW);
}

static uint16_t read_adc_naiive(uint8_t channel)
{
  uint8_t channel_array[16];
  channel_array[0] = channel;
  adc_set_regular_sequence(ADC1, 1, channel_array);
  adc_start_conversion_regular(ADC1);
  while (!adc_eoc(ADC1));
  uint16_t reg16 = adc_read_regular(ADC1);
  return reg16;
}
