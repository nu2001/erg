
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "cdcacm.h"
#include "tick.h"

#define LED_DISCO_GREEN_PORT GPIOG
#define LED_DISCO_GREEN_PIN GPIO13

int _write(int file, char *ptr, int len);
static void clock_setup(void);
static void adc_setup(void);
static void dac_setup(void);
static uint16_t read_adc_naiive(uint8_t channel);
static uint16_t read_last_adc(void);

static void capture_data(void);

#define ADC_BUF_SIZE (90*1024)
static uint16_t adc_data[ADC_BUF_SIZE];
uint32_t saved_bytes = 0;

int main(void)
{
  clock_setup();
  adc_setup();
  dac_setup();
  systick_setup();

  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE,
		  GPIO13 | GPIO14 | GPIO15);
  gpio_set_af(GPIOB, GPIO_AF12, GPIO13 | GPIO14 | GPIO15);

  cdcacm_init();

  gpio_mode_setup(LED_DISCO_GREEN_PORT, GPIO_MODE_OUTPUT,
		  GPIO_PUPD_NONE, LED_DISCO_GREEN_PIN);

  uint16_t input_adc0 = read_adc_naiive(0);

  while (1)
  {
    if (read_buf_len && read_buf[0] == 'p')
    {
      input_adc0 = read_last_adc();
      printf("adc0 = %u\r\n", input_adc0);
      gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
    }

    if (read_buf_len && read_buf[0] == 'c')
    {
      gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
      capture_data();
      gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
    }

    read_buf_len = 0;
    cdcacm_poll();
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
  rcc_periph_clock_enable(RCC_GPIOA);
  
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
    return cdcacm_write(ptr, len);
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

static uint16_t read_last_adc(void)
{
  adc_start_conversion_regular(ADC1);
  while (!adc_eoc(ADC1));
  return adc_read_regular(ADC1);
}

static void capture_data(void)
{
  int i;
  uint32_t start_time, end_time;
  start_time = system_millis;
  for (i=0; i<ADC_BUF_SIZE; i++)
  {
    adc_data[i] = read_last_adc();
  }
  end_time = system_millis;

  printf("c %d %u %u\r\n", ADC_BUF_SIZE, start_time, end_time);
  for (i=0; i<ADC_BUF_SIZE; i++)
  {
    printf("s %d\r\n", adc_data[i]);
  }
  printf("d\r\n");
}
