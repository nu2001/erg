
#pragma once

#define LCD_WIDTH  240
#define LCD_HEIGHT 320

void lcd_dma_init(void);
bool lcd_dma_buffer_ready(void);
void lcd_dma_swap_buffers(void);
void lcd_dma_draw_pixel(int, int, uint16_t);

