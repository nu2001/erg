
#pragma once

#include <stdint.h>

#define LCD_WIDTH  240
#define LCD_HEIGHT 320

void lcd_dma_init(void);
int lcd_dma_buffer_ready(void);
void lcd_dma_swap_buffers(void);
void lcd_dma_draw_pixel(int, int, uint16_t);

