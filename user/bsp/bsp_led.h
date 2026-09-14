/**
 * @file    bsp_led.h
 * @brief   LED 驱动模块 — 板载指示灯控制
 * @author  Reach
 * @version v1.0
 *
 * 引脚配置：board_config.h 中的 LED_PIN / LED_ON_LEVEL
 *
 * 使用示例：
 * @code
 *   led_init();                      // 初始化，在 bsp_init() 中调用一次
 *   led_blink(3, 100);               // 上电闪3次，确认程序已启动
 *   led_on();                        // 系统运行时常亮
 *   led_toggle();                    // 在控制任务中定期翻转，表示"心跳"
 *   led_off();                       // 停车时熄灭
 * @endcode
 */

#ifndef _BSP_LED_H_
#define _BSP_LED_H_

#include "board_config.h"

/**
 * @brief   LED 初始化
 */
void led_init(void);

/**
 * @brief   LED 点亮
 */
void led_on(void);

/**
 * @brief   LED 熄灭
 */
void led_off(void);

/**
 * @brief   LED 翻转
 */
void led_toggle(void);

/**
 * @brief   LED 闪烁指定次数
 * @param   times   闪烁次数
 * @param   ms      单次亮灭时间(ms)
 */
void led_blink(uint8 times, uint16 ms);

#endif /* _BSP_LED_H_ */
