#include <libopencm3/cm3/systick.h>
#include "tick.h"

/* monotonically increasing number of milliseconds from reset
 * overflows every 49 days if you're wondering */
volatile uint32_t system_millis;

void sys_tick_handler(void);

/* Called when systick fires */
void sys_tick_handler(void)
{
  system_millis++;
}

/* sleep for delay milliseconds */
void msleep(uint32_t delay)
{
  uint32_t wake = system_millis + delay;
  while (wake > system_millis);
}

/*
 * systick_setup(void)
 *
 * This function sets up the 1khz "system tick" count. The SYSTICK counter is a
 * standard feature of the Cortex-M series.
 */
void systick_setup(void)
{
  /* clock rate / 1000 to get 1mS interrupt rate */
  systick_set_reload(168000);
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_counter_enable();
  /* this done last */
  systick_interrupt_enable();
}
