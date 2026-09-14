/**
 * @file    bsp_oled.h
 * @brief   I2C OLED 显示驱动 — SSD1306 0.96寸 128x64
 * @author  Reach
 * @version v1.0
 *
 * 硬件连接：SCL=PB9, SDA=PB8（I2C 7位地址 0x3C）
 * 引脚修改：board_config.h → OLED_SCL_PIN / OLED_SDA_PIN
 *
 * ⚠ 函数名加 bsp_ 前缀，与逐飞库 SPI OLED 的 oled_xxx 函数区分
 *
 * 使用示例：
 * @code
 *   bsp_oled_init();                          // bsp_init() 中调用一次
 *   bsp_oled_clear();                         // 清屏
 *   bsp_oled_show_string(0, 0, "Hello!");    // 在 (0,0) 显示字符串
 *   bsp_oled_show_int(0, 2, 12345, 5);       // 在 (0,2行) 显示5位数字
 *   bsp_oled_show_float(0, 4, 3.14, 2, 2);   // 在 (0,4行) 显示浮点数
 *
 *   // 在 task_display() 中更新显示（替换串口打印）：
 *   bsp_oled_clear();
 *   bsp_oled_show_string(0, 0, "S:");
 *   bsp_oled_show_int(24, 0, s_sys_state, 1);
 *   bsp_oled_show_string(0, 2, "L:");
 *   bsp_oled_show_int(24, 2, s_encoder_l, 4);
 *   bsp_oled_show_string(64, 2, "R:");
 *   bsp_oled_show_int(88, 2, s_encoder_r, 4);
 *   bsp_oled_refresh();
 * @endcode
 */

#ifndef _BSP_OLED_H_
#define _BSP_OLED_H_

#include "board_config.h"

/*-----------------------------------------------------------
 * OLED 基本操作
 *-----------------------------------------------------------*/

/** @brief OLED 初始化（配置 I2C + SSD1306 寄存器） */
void bsp_oled_init(void);

/** @brief 清屏（全黑） */
void bsp_oled_clear(void);

/** @brief 全屏填充 */
void bsp_oled_fill(uint8 color);

/** @brief 画点 (坐标原点在左上角) */
void bsp_oled_draw_point(uint8 x, uint8 y, uint8 color);

/** @brief 刷新显存到屏幕（内部调用，一般不需要手动调） */
void bsp_oled_refresh(void);

/*-----------------------------------------------------------
 * 文字显示
 *-----------------------------------------------------------*/

void bsp_oled_show_string(uint8 x, uint8 line, const char *str);
void bsp_oled_show_int(uint8 x, uint8 line, int32 val, uint8 len);
void bsp_oled_show_uint(uint8 x, uint8 line, uint32 val, uint8 len);
void bsp_oled_show_float(uint8 x, uint8 line, double val, uint8 total, uint8 decimal);

/*-----------------------------------------------------------
 * 扩展功能
 *-----------------------------------------------------------*/

/** @brief 显示中文字符（16x16 点阵，需预先包含字模数组） */
void bsp_oled_show_chinese(uint8 x, uint8 line, const uint8 *bitmap);

/** @brief 填充矩形区域 */
void bsp_oled_fill_rect(uint8 x, uint8 y, uint8 w, uint8 h, uint8 color);

/** @brief 画水平线 */
void bsp_oled_draw_hline(uint8 x, uint8 y, uint8 len, uint8 color);

/** @brief 画垂直线 */
void bsp_oled_draw_vline(uint8 x, uint8 y, uint8 len, uint8 color);

/** @brief OLED 帧缓冲区（供 Astra UI 等上层框架读取像素值做 XOR 绘制） */
extern uint8 oled_framebuffer[8 * 128];  /* 1024 字节 */

#endif /* _BSP_OLED_H_ */
