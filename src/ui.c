
#include "ui.h"
#include "sdram.h"
#include "lcd-dma.h"
#include "gfx.h"
#include "clock.h"
#include "erg.h"

#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#define TEXT_SIZE (5)
#define TEXT_HEIGHT (12)
#define TEXT_WIDTH (7)
#define TEXT_LINE_SIZE TEXT_SIZE*TEXT_HEIGHT

void ui_init(void)
{
    sdram_init();
    lcd_dma_init();

    gfx_init(lcd_dma_draw_pixel, LCD_WIDTH, LCD_HEIGHT);
    gfx_setRotation(2, LCD_WIDTH, LCD_HEIGHT);
    gfx_setTextSize(TEXT_SIZE);
    gfx_setTextColor(GFX_COLOR_WHITE, GFX_COLOR_BLACK);
}

void ui_update(void)
{
    char str[10];
    if (lcd_dma_buffer_ready())
    {
        gfx_setCursor(0, 0);
        sprintf(str, "%6"PRIu32, get_time_ms() % 1000000);
        gfx_puts(str);

        gfx_setCursor(0, TEXT_LINE_SIZE);
        sprintf(str, "%3.2f", erg_get_omega());
        gfx_puts(str);

        gfx_setCursor(0, 2*TEXT_LINE_SIZE);
        sprintf(str, "%3.2f", erg_get_b());
        gfx_puts(str);

        gfx_setCursor(0, 3*TEXT_LINE_SIZE);
        sprintf(str, "%3.2f", erg_get_power());
        gfx_puts(str);
        
        lcd_dma_swap_buffers();
    }
}
