
#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <cdcacm.h>

int main(void)
{
  usbd_device *usbd_dev;

  rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_120MHZ]);

  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_OTGHS);

  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE,
		  GPIO13 | GPIO14 | GPIO15);
  gpio_set_af(GPIOB, GPIO_AF12, GPIO13 | GPIO14 | GPIO15);

  usbd_dev = cdcacm_init();
  while (1) {
    usbd_poll(usbd_dev);
  }
}
