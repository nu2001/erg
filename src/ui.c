
#include "ui.h"
#include "sdram.h"
#include "lcd-dma.h"
#include "gfx.h"
#include "clock.h"
#include "erg.h"

#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>

#define TEXT_SIZE (3)
#define TEXT_HEIGHT (12)
#define TEXT_WIDTH (7)
#define TEXT_LINE_SIZE TEXT_SIZE*TEXT_HEIGHT

static char str[200];

void ui_init(void)
{
    sdram_init();
    lcd_dma_init();

    gfx_init(lcd_dma_draw_pixel, LCD_WIDTH, LCD_HEIGHT);
    gfx_setRotation(2, LCD_WIDTH, LCD_HEIGHT);
    gfx_setTextSize(TEXT_SIZE);
    gfx_setTextColor(GFX_COLOR_WHITE, GFX_COLOR_BLACK);
}

void ui_update(struct erg_status_s * status)
{
    if (lcd_dma_buffer_ready())
    {
        erg_update_status(status);

        gfx_setCursor(0, 0);
        sprintf(str, "%10.1f", status->elapsed_time);
        str[10] = 0;
        gfx_puts(str);

        gfx_setCursor(0, TEXT_LINE_SIZE);
        sprintf(str, "%10.1f", status->distance);
        str[10] = 0;
        gfx_puts(str);

        gfx_setCursor(0, 2*TEXT_LINE_SIZE);
        sprintf(str, "%10.1f", status->strokes_per_min);
        str[10] = 0;
        gfx_puts(str);

        gfx_setCursor(0, 3*TEXT_LINE_SIZE);
        sprintf(str, "%10.1f", status->drag_factor);
        str[10] = 0;
        gfx_puts(str);
        
        gfx_setCursor(0, 4*TEXT_LINE_SIZE);
        int split_min = (int)floor(status->split / 60.f);
        float split_sec = status->split - split_min;
        sprintf(str, "%d:%3.1f    ", split_min, split_sec);
        str[10] = 0;
        gfx_puts(str);

        gfx_setCursor(0, 5*TEXT_LINE_SIZE);
        int avg_split_min = (int)floor(status->split / 60.f);
        float avg_split_sec = status->split - avg_split_min;
        sprintf(str, "%d:%3.1f    ", avg_split_min, avg_split_sec);
        str[10] = 0;
        gfx_puts(str);
        
        lcd_dma_swap_buffers();
    }
}
