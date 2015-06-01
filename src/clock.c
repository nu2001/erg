/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Chuck McManis <cmcmanis@mcmanis.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Now this is just the clock setup code from systick-blink as it is the
 * transferrable part.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>


/* Common function descriptions */
#include "clock.h"

/* milliseconds since boot */
static volatile uint32_t system_millis = 0;

/* Called when systick fires */
void sys_tick_handler(void)
{
    system_millis++;
}

/* simple sleep for delay milliseconds */
void milli_sleep(uint32_t delay)
{
    uint32_t wake = system_millis + delay;
    while (wake > system_millis) {
        continue;
    }
}

/* Getter function for the current time */
uint32_t get_time_ms(void)
{
    return system_millis;
}

/** Note: this will overflow after about 71 minutes!!! */
uint32_t get_time_us(void)
{
    return timer_get_counter(TIM2);
}

/*
 * clock_setup(void)
 *
 * This function sets up both the base board clock rate
 * and a 1khz "system tick" count. The SYSTICK counter is
 * a standard feature of the Cortex-M series.
 *
 * It also sets up a microsecond counter (tied two counters together)
 */
void clock_setup(void)
{
    /* Base board frequency, set to 168Mhz */
    rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);

    /* clock rate / 168000 to get 1mS interrupt rate */
    systick_set_reload(168000);
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_counter_enable();
    systick_interrupt_enable();

    rcc_periph_clock_enable(RCC_TIM1);
    timer_reset(TIM1);
    timer_set_period(TIM1, 167);
    timer_set_master_mode(TIM1, TIM_CR2_MMS_UPDATE);

    rcc_periph_clock_enable(RCC_TIM2);
    timer_reset(TIM2);
    timer_slave_set_trigger(TIM2, TIM_SMCR_TS_ITR0);
    timer_slave_set_mode(TIM2, TIM_SMCR_SMS_ECM1);

    timer_enable_counter(TIM1);
    timer_enable_counter(TIM2);
}
