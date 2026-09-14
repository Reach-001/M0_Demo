/**
 * @file    bsp_lcd147.h
 * @brief   1.47 inch 172x320 SPI LCD driver
 */

#ifndef _BSP_LCD147_H_
#define _BSP_LCD147_H_

#include "board_config.h"

#define LCD147_FRAMEBUFFER_SIZE ((((LCD147_WIDTH) + 7U) / 8U) * (LCD147_HEIGHT))

void bsp_lcd147_init(void);
void bsp_lcd147_set_backlight(uint8 on);
void bsp_lcd147_clear(void);
void bsp_lcd147_fill(uint8 color);
void bsp_lcd147_fill_direct(uint16 color);
void bsp_lcd147_draw_point(uint16 x, uint16 y, uint8 color);
uint8 bsp_lcd147_get_point(uint16 x, uint16 y);
uint8 bsp_lcd147_get_ascii_column(char ch, uint8 column);
void bsp_lcd147_refresh(void);
void bsp_lcd147_show_string(uint16 x, uint16 line, const char *str);
void bsp_lcd147_show_string_scaled(uint16 x, uint16 y, const char *str, uint8 scale);
void bsp_lcd147_show_int(uint16 x, uint16 line, int32 val, uint8 len);
void bsp_lcd147_show_uint(uint16 x, uint16 line, uint32 val, uint8 len);
void bsp_lcd147_show_float(uint16 x, uint16 line, double val, uint8 total, uint8 decimal);

extern uint8 lcd147_framebuffer[LCD147_FRAMEBUFFER_SIZE];

#endif /* _BSP_LCD147_H_ */
