
#pragma once

/* monotonically increasing number of milliseconds from reset
 * overflows every 49 days if you're wondering */
extern volatile uint32_t system_millis;

/* sleep for delay milliseconds */
void msleep(uint32_t delay);

/*
 * systick_setup(void)
 *
 * This function sets up the 1khz "system tick" count. The SYSTICK counter is a
 * standard feature of the Cortex-M series.
 */
void systick_setup(void);
