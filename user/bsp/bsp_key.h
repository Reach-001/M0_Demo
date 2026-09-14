/**
 * @file    bsp_key.h
 * @brief   按键驱动模块 — 支持短按、长按检测（基于状态机，无延时阻塞）
 * @author  Reach
 * @version v1.0
 *
 * 引脚配置：board_config.h 中的 KEY1_PIN / KEY2_PIN / KEY_ACTIVE_LEVEL
 * 时序参数：KEY_DEBOUNCE_COUNT（消抖）/ KEY_LONG_PRESS_COUNT（长按阈值）
 *
 * 使用示例：
 * @code
 *   // 在任务函数中（每 20ms 调用一次）
 *   bsp_key_scan();    // 扫描按键状态（必须定期调用）
 *
 *   // 读取事件（推荐：读一次，用 if-else if 判断，避免重复触发）
 *   bsp_key_event_enum ev = bsp_key_get_event(BSP_KEY_1);
 *   bsp_key_clear_event(BSP_KEY_1);   // 清除事件，防止下次重复读到
 *   if (ev == KEY_EVENT_PRESS)       { ... }   // 短按：按下并释放
 *   else if (ev == KEY_EVENT_LONG_PRESS) { ... } // 长按：按住达到阈值时触发
 *
 *   // 查询实时状态（用于需要持续检测"是否按住"的场景）
 *   if (bsp_key_get_state(BSP_KEY_2)) { ... }  // 1=当前按住
 * @endcode
 *
 * @note 扩展按键：在 board_config.h 添加 KEY3_PIN，
 *       在 bsp_key.h 的枚举中添加 BSP_KEY_3，
 *       在 bsp_key.c 的 key_pins[] 数组末尾追加 KEY3_PIN
 */

#ifndef _BSP_KEY_H_
#define _BSP_KEY_H_

#include "board_config.h"

/* 按键编号枚举 */
typedef enum
{
    BSP_KEY_1 = 0,
    BSP_KEY_2,
    BSP_KEY_NUM
} bsp_key_id_enum;

/* 按键事件枚举 */
typedef enum
{
    KEY_EVENT_NONE = 0,     /* 无事件 */
    KEY_EVENT_PRESS,        /* 短按 */
    KEY_EVENT_LONG_PRESS,   /* 长按 */
    KEY_EVENT_RELEASE,      /* 释放 */
} bsp_key_event_enum;

/**
 * @brief   按键初始化
 */
void bsp_key_init(void);

/**
 * @brief   按键扫描，由 TASK_KEY_PERIOD 决定调用周期
 */
void bsp_key_scan(void);

/**
 * @brief   获取按键事件
 * @param   id      按键编号
 * @return  按键事件
 */
bsp_key_event_enum bsp_key_get_event(bsp_key_id_enum id);

/**
 * @brief   获取按键当前状态
 * @param   id      按键编号
 * @return  1=按下, 0=松开
 */
uint8 bsp_key_get_state(bsp_key_id_enum id);

/**
 * @brief   清除按键事件
 * @param   id      按键编号
 */
void bsp_key_clear_event(bsp_key_id_enum id);

#endif /* _BSP_KEY_H_ */
