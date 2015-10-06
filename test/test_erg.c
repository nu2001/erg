
#include <stdio.h>
#include <stdint.h>
#include "erg.h"
#include "clock.h"

/* these two variables are set in isr.
   will use them to inject tests */
extern volatile uint32_t revs_counter;
extern volatile uint32_t rev_time_us;

/* use this to fake timing */
static uint32_t global_time_us = 0;

/*
  During light pull by arm only I get revs to switch between 3 and
  4. UI updates at about 10 Hz. To simulate this behavior, I will run
  erg_update_status every 100 ms. Revs updates by 7 every 200 ms. That
  is 7/200 revs/ms
 */


int main()
{
    struct erg_status_s status;
    erg_init(&status);

    global_time_us = 1234;
    uint32_t last_global_time_us = 0;
    for (int i=0; i<10; i++)
    {
        global_time_us += 100000;
        /* use doubles so I don't have to worry about overflows */
        uint32_t new_counter = (uint32_t)((7.0 / 200000.0) * global_time_us + 0.5);
        uint32_t old_counter = (uint32_t)((7.0 / 200000.0) * last_global_time_us + 0.5);
        /* revs counter is reset every time erg_update is run */
        revs_counter = new_counter - old_counter;
        rev_time_us = global_time_us;
        last_global_time_us = global_time_us;

        printf("time us = %u, revs_counter = %u\n",
               global_time_us, revs_counter);

        erg_update_status(&status);

        printf("  elapsed time    = %f\n", status.elapsed_time);
        printf("  distance        = %f\n", status.distance);
        printf("  strokes per min = %f\n", status.strokes_per_min);
        printf("  drag factor     = %f\n", status.drag_factor);
        printf("  split           = %f\n", status.split);
        printf("  avg split       = %f\n", status.average_split);
        printf(" filt ang speed   = %f\n", status.aaa);
    }

    return 0;
}

uint32_t get_time_ms(void)
{
    return global_time_us / 1000;
}

uint32_t get_delta_time(uint32_t start, uint32_t end)
{
    if (start > end)
    {
        return 0xFFFFFFFF - start + end;
    }
    return end - start;    
}
