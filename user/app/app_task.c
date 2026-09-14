/**
 * @file    app_task.c
 * @brief   任务调度模块实现 — 基于定时器的简易任务管理（无需 RTOS）
 * @author  Reach
 * @version v1.1
 *
 * ============================================================
 * 【框架说明】
 * ============================================================
 * 本模块使用 PIT 定时器 + 主循环轮询的方式实现任务调度：
 *   1. PIT 每隔 TASK_PERIOD_MS 触发一次中断
 *   2. 中断中根据各任务的周期取模，置位对应的 g_task_flag
 *   3. main.c 主循环中检查标志位并执行对应的 task_xxx() 函数
 *
 * 这种方式的优势：
 *   - 无需 RTOS，代码轻量
 *   - 任务函数可以自由使用 printf / 延时
 *   - 新增任务只需 5 步，改动点明确
 *
 * ============================================================
 * 【任务执行顺序建议（数据依赖关系）】
 * ============================================================
 *   task_track()   → 采集传感器原始数据
 *   task_control() → 读编码器 + 用 track 数据计算 PID + 输出电机
 *   task_display() → 打印上面两步的结果（调试用）
 *   task_key()     → 独立，与控制流无依赖
 *   以上顺序在 main.c 的主循环中天然保证（按 flag 依次判断）。
 */

#include "app_task.h"
#include "bsp_led.h"
#include "bsp_motor.h"
#include "bsp_encoder.h"
#include "bsp_track.h"
#include "bsp_key.h"
#include "app_control.h"

/*-----------------------------------------------------------
 * 全局变量
 *-----------------------------------------------------------*/

/* 任务标志由 PIT 中断置位，在 main.c 主循环中清零并执行。 */
volatile task_flag_t g_task_flag = {0};

/* 任务计数器 — 每次 PIT 中断 +1，用于各任务的周期取模 */
static volatile uint16 s_task_tick = 0;

/* 系统状态 */
static sys_state_enum s_sys_state = SYS_STATE_IDLE;

/* 循迹数据 */
static track_data_t s_track_data;

/* 编码器数据 */
static int16 s_encoder_l = 0;
static int16 s_encoder_r = 0;

/* 丢线计数 */
static uint16 s_lost_count = 0;
static int16 s_last_error = 0;

/*-----------------------------------------------------------
 * 定时器回调 (PIT 中断中调用)
 *
 * ⚠ 注意事项：
 *   - 此函数在中断上下文中执行，只能置位标志，不能做耗时操作
 *   - 禁止使用 printf、system_delay_ms 等阻塞函数
 *   - 标志位由 main.c 主循环消费
 *-----------------------------------------------------------*/
void task_timer_callback(void)
{
    s_task_tick++;

    /* 循迹采样任务 */
    if (s_task_tick % TASK_TRACK_PERIOD == 0)
    {
        g_task_flag.track = 1;
    }

    /* 控制计算任务 */
    if (s_task_tick % TASK_CONTROL_PERIOD == 0)
    {
        g_task_flag.control = 1;
    }

    /* 显示刷新任务 */
    if (s_task_tick % TASK_DISPLAY_PERIOD == 0)
    {
        g_task_flag.display = 1;
    }

    /* 按键扫描任务 */
    if (s_task_tick % TASK_KEY_PERIOD == 0)
    {
        g_task_flag.key = 1;
    }

    /* ============================================================
     * 【新增任务示例】
     * 1. 在 board_config.h 添加 TASK_UART_PERIOD
     * 2. 在 app_task.h 的 task_flag_t 添加 uint8 uart
     * 3. 在这里按周期置位 g_task_flag.uart
     * 4. 在下方实现 task_uart() 函数
     * 5. 在 main.c 主循环中添加执行块
     *
     * 示例代码（取消注释即可使用）：
     *   if (s_task_tick % TASK_UART_PERIOD == 0)
     *   {
     *       g_task_flag.uart = 1;
     *   }
     * ============================================================ */

    /* 计数器溢出处理 */
    if (s_task_tick >= TASK_TICK_MAX)
    {
        s_task_tick = 0;
    }
}

/*-----------------------------------------------------------
 * PIT 中断回调包装 — 衔接逐飞库的回调机制
 *-----------------------------------------------------------*/
static void pit_callback(uint32 flag, void *param)
{
    (void)flag;
    (void)param;
    task_timer_callback();
}

/*-----------------------------------------------------------
 * 任务调度初始化
 *
 * 初始化 PIT 定时器，设置系统状态为就绪。
 * 调用后各任务标志将在中断中按周期自动置位。
 *
 * 【扩展说明】如果需要在任务调度启动前做额外的初始化
 * （如加载配置文件），在此函数中 task_init 之后添加。
 *-----------------------------------------------------------*/
void task_init(void)
{
    /* 初始化定时器，回调频率 = TASK_PERIOD_MS */
    pit_ms_init(TASK_PIT, TASK_PERIOD_MS, pit_callback, NULL);

    /* 初始化状态 */
    s_sys_state = SYS_STATE_READY;
}

/*-----------------------------------------------------------
 * 循迹采样任务 — 读取循迹传感器原始值
 *
 * 执行频率：TASK_TRACK_PERIOD × TASK_PERIOD_MS（默认 5ms）
 *
 * 【扩展说明】如需添加更多传感器采样（如超声波、TOF），
 *   可在此函数中添加 sensor_read(&data) 调用。
 *   注意：此函数执行时间应远小于调度周期。
 *-----------------------------------------------------------*/
void task_track(void)
{
    /* 读取循迹传感器 */
    track_read(&s_track_data);
}

/*-----------------------------------------------------------
 * 控制计算任务 — PID 控制 + 电机输出
 *
 * 执行频率：TASK_CONTROL_PERIOD × TASK_PERIOD_MS（默认 10ms）
 *
 * 流程：
 *   1. 读取编码器速度反馈
 *   2. 非运行状态 → 关闭电机，直接返回
 *   3. 检测丢线 → 超过阈值则停车
 *   4. 计算循迹偏差 → PID 转向 + 速度控制 → 输出电机
 *
 * 【扩展说明】如需添加新型控制算法（如 MPC、模糊控制），
 *   替换 control_run() 调用即可。
 *-----------------------------------------------------------*/
void task_control(void)
{
    /* 读取编码器 */
    s_encoder_l = encoder_get_and_clear(ENCODER_LEFT);
    s_encoder_r = encoder_get_and_clear(ENCODER_RIGHT);

    /* 非运行状态强制关电机，避免上电或暂停时误输出 PWM。 */
    if (s_sys_state != SYS_STATE_RUNNING)
    {
        motor_stop_all();
        return;
    }

    /* 检测丢线 */
    if (track_is_lost(&s_track_data))
    {
        s_lost_count++;
        if (s_lost_count > TRACK_LOST_MAX_COUNT)
        {
            /* 超时停车 */
            task_stop();
            return;
        }
    }
    else
    {
        s_lost_count = 0;
    }

    /* 计算循迹偏差 */
    int16 error;
    if (track_is_lost(&s_track_data))
    {
        /* 丢线时使用上次的偏差（保持方向） */
        error = s_last_error;
    }
    else
    {
        error = track_get_error(&s_track_data);
        s_last_error = error;
    }

    /* 控制计算 */
    int16 motor_l, motor_r;
    control_run(error, s_encoder_l, s_encoder_r, &motor_l, &motor_r);

    /* 输出到电机 */
    motor_set_speed_both(motor_l, motor_r);

    /* LED 指示运行状态 */
    static uint8 led_cnt = 0;
    if (++led_cnt >= LED_RUN_TOGGLE_COUNT)
    {
        led_cnt = 0;
        led_toggle();
    }
}

/*-----------------------------------------------------------
 * 显示刷新任务 — 串口打印调试信息
 *
 * 执行频率：TASK_DISPLAY_PERIOD × TASK_PERIOD_MS（默认 300ms）
 *
 * 输出格式：S:状态 E:偏差 L:左编码器 R:右编码器
 *
 * 【扩展说明】如需改为 OLED/LCD 显示：
 *   1. 在 bsp_init() 中初始化显示屏
 *   2. 在此函数中用显示函数替换 printf
 *   3. 例子（OLED）：oled_clear_buffer();
 *                      oled_draw_str(0,0, buf);
 *                      oled_send_buffer();
 *-----------------------------------------------------------*/
void task_display(void)
{
    /* 串口打印调试信息（可替换为 OLED/LCD 显示） */
    printf("S:%d E:%d L:%d R:%d\r\n",
           s_sys_state,
           s_last_error,
           s_encoder_l,
           s_encoder_r);
}

/*-----------------------------------------------------------
 * 按键处理任务 — 短按启停，长按预留
 *
 * 执行频率：TASK_KEY_PERIOD × TASK_PERIOD_MS（默认 20ms）
 *
 * 当前逻辑：
 *   - 短按 KEY1 → 切换启动/停止
 *
 * 【扩展说明】如需增加功能（如长按调速、双击切换模式）：
 *   参考 bsp_key.c 中的按键检测框架，在此函数中添加对应逻辑。
 *   示例：
 *     if (bsp_key_is_long_press(KEY1)) {
 *         // 长按 KEY1 调速
 *         target_speed += TARGET_SPEED_STEP;
 *     }
 *-----------------------------------------------------------*/
void task_key(void)
{
    /* 扫描按键（必须每周期调用一次，内部做消抖和状态机） */
    bsp_key_scan();

    /* 读取 KEY1 事件 */
    bsp_key_event_enum ev = bsp_key_get_event(BSP_KEY_1);
    bsp_key_clear_event(BSP_KEY_1);   /* 清除事件，防止下次重复触发 */

    if (ev == KEY_EVENT_PRESS)
    {
        /* 短按 KEY1：切换启动/停止 */
        if (s_sys_state == SYS_STATE_RUNNING)
        {
            task_stop();
        }
        else if (s_sys_state == SYS_STATE_READY || s_sys_state == SYS_STATE_STOP)
        {
            task_start();
        }
    }
    else if (ev == KEY_EVENT_LONG_PRESS)
    {
        /* 长按 KEY1：预留（可在此处扩展功能，如调整速度） */
    }
}

/*-----------------------------------------------------------
 * 设置系统状态
 *-----------------------------------------------------------*/
void task_set_state(sys_state_enum state)
{
    s_sys_state = state;
}

/*-----------------------------------------------------------
 * 获取系统状态
 *-----------------------------------------------------------*/
sys_state_enum task_get_state(void)
{
    return s_sys_state;
}

/*-----------------------------------------------------------
 * 系统启动 — 进入运行状态
 *-----------------------------------------------------------*/
void task_start(void)
{
    if (s_sys_state == SYS_STATE_READY || s_sys_state == SYS_STATE_STOP)
    {
        control_reset();
        control_enable(1);
        s_lost_count = 0;
        s_last_error = 0;
        s_sys_state = SYS_STATE_RUNNING;
        led_on();
        printf("System Started!\r\n");
    }
}

/*-----------------------------------------------------------
 * 系统停止 — 进入停止状态
 *-----------------------------------------------------------*/
void task_stop(void)
{
    control_enable(0);
    motor_stop_all();
    s_sys_state = SYS_STATE_STOP;
    led_off();
    printf("System Stopped!\r\n");
}
