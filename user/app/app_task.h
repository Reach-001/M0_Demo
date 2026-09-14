/**
 * @file    app_task.h
 * @brief   任务调度模块 — 基于定时器的简易任务管理（无需 RTOS）
 * @author  Reach
 * @version v1.1
 *
 * ============================================================
 * 【框架核心概念】
 * ============================================================
 *
 * 本框架使用"中断置标志 + 主循环执行"的轻量级任务调度：
 *
 *   PIT 定时器中断（每 5ms）
 *       │
 *       ▼
 *   task_timer_callback()
 *       │  检查 s_task_tick % TASK_XXX_PERIOD 是否等于 0
 *       │  等于 0 则置位 g_task_flag.xxx = 1
 *       │
 *       ▼
 *   main.c 主循环
 *       │  轮询 g_task_flag 的每个字段
 *       │  flag=1 时：清零 → 调用 task_xxx()
 *       │
 *       ▼
 *   task_xxx() 执行实际业务逻辑
 *
 * ============================================================
 * 【新增任务 — 完整 5 步流程】
 * ============================================================
 * 以"串口上报任务（50ms）"为例：
 *
 * Step 1. board_config.h 添加周期宏：
 *           #define TASK_UART_PERIOD  10   // 10 × 5ms = 50ms
 *
 * Step 2. 本文件 task_flag_t 加标志位：
 *           uint8 uart;   // 串口上报任务
 *
 * Step 3. app_task.c 的 task_timer_callback() 中置位：
 *           if (s_task_tick % TASK_UART_PERIOD == 0)
 *               g_task_flag.uart = 1;
 *
 * Step 4. app_task.c 实现任务函数，本文件声明：
 *           void task_uart(void);
 *
 * Step 5. main.c 主循环中添加执行块：
 *           if (g_task_flag.uart) {
 *               g_task_flag.uart = 0;
 *               task_uart();
 *           }
 * ============================================================
 *
 * ============================================================
 * 【任务执行规则】
 * ============================================================
 *   - task_timer_callback() 在 PIT 中断中被调用，只能置位标志，
 *     不能执行 printf / 延时 / 复杂逻辑。
 *   - task_xxx() 在主循环中执行，可以使用所有功能。
 *   - 每个 task_xxx() 应在一个调度周期内完成，避免阻塞其他任务。
 *   - 任务执行顺序由 main.c 中 flag 检查的顺序决定。
 *
 * ============================================================
 * 【任务周期计算公式】
 * ============================================================
 *   实际周期 = TASK_PERIOD_MS × TASK_xxx_PERIOD（毫秒）
 *
 *   示例：
 *     TASK_PERIOD_MS = 5, TASK_TRACK_PERIOD = 1  → 每 5ms 执行一次
 *     TASK_PERIOD_MS = 5, TASK_KEY_PERIOD   = 4  → 每 20ms 执行一次
 *     TASK_PERIOD_MS = 5, TASK_DISPLAY_PERIOD = 60 → 每 300ms 执行一次
 */

#ifndef _APP_TASK_H_
#define _APP_TASK_H_

#include "board_config.h"

/**
 * @brief   任务标志位结构体
 * @note    新增任务时在此处添加 uint8 字段（字段名与任务名保持一致）
 *          中断只置 1，主循环读取后清 0 再执行，确保每次触发只执行一次
 *
 * @code
 *   // 新增示例：添加串口上报任务
 *   uint8 uart;   // 串口上报任务标志
 * @endcode
 */
typedef struct
{
    uint8 track;        ///< 循迹采样任务标志（5ms 置位一次）
    uint8 control;      ///< 控制计算任务标志（10ms 置位一次）
    uint8 display;      ///< 显示刷新任务标志（300ms 置位一次）
    uint8 key;          ///< 按键扫描任务标志（20ms 置位一次）
    /* 示例：uint8 uart; — 串口上报任务，参考本文件头部的5步流程 */
} task_flag_t;

/* 全局任务标志（PIT 中断置位，main.c 主循环清零） */
extern volatile task_flag_t g_task_flag;

/* 系统状态枚举 */
typedef enum
{
    SYS_STATE_IDLE = 0,     /* 待机状态 — 初始状态 */
    SYS_STATE_READY,        /* 就绪状态 — 等待启动 */
    SYS_STATE_RUNNING,      /* 运行状态 — PID 控制使能 */
    SYS_STATE_STOP,         /* 停止状态 — 可重新启动 */
    SYS_STATE_ERROR,        /* 错误状态 — 需复位恢复 */
} sys_state_enum;

/*============================================================
 * 初始化
 *============================================================*/

/**
 * @brief   任务调度初始化
 *
 * 初始化 PIT 定时器，设置系统状态为就绪。
 * 调用后各任务标志将在中断中按周期自动置位。
 *
 * 调用位置：main.c → app_init()
 */
void task_init(void);

/**
 * @brief   定时器回调 (在 PIT 中断中调用)
 *
 * ⚠ 这里只设置任务标志，不能执行耗时逻辑、printf 或长延时。
 *    每 TASK_PERIOD_MS 触发一次（默认 5ms）。
 */
void task_timer_callback(void);

/*============================================================
 * 任务函数（在 main.c 主循环中调用）
 *============================================================*/

/**
 * @brief   循迹采样任务 — 读取传感器原始值
 * 周期：TASK_TRACK_PERIOD × 5ms（默认 5ms）
 */
void task_track(void);

/**
 * @brief   控制计算任务 — 读编码器 → PID → 输出电机
 * 周期：TASK_CONTROL_PERIOD × 5ms（默认 10ms）
 */
void task_control(void);

/**
 * @brief   显示刷新任务 — 串口打印状态
 * 周期：TASK_DISPLAY_PERIOD × 5ms（默认 300ms）
 *
 * 可替换为 OLED/LCD 显示。
 */
void task_display(void);

/**
 * @brief   按键处理任务 — 短按启停
 * 周期：TASK_KEY_PERIOD × 5ms（默认 20ms）
 *
 * 可扩展长按调速、双击切换模式等功能。
 */
void task_key(void);

/*============================================================
 * 状态管理
 *============================================================*/

/**
 * @brief   设置系统状态
 * @param   state   目标状态
 */
void task_set_state(sys_state_enum state);

/**
 * @brief   获取系统状态
 * @return  当前状态枚举值
 */
sys_state_enum task_get_state(void);

/**
 * @brief   系统启动 — 进入运行状态，使能 PID 控制
 */
void task_start(void);

/**
 * @brief   系统停止 — 关闭电机，进入停止状态
 */
void task_stop(void);

#endif /* _APP_TASK_H_ */
