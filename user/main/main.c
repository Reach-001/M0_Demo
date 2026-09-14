/**
 * @file    main.c
 * @brief   主程序入口 — MSPM0G3507 通用框架
 * @author  Reach
 * @version v1.1
 *
 * ============================================================
 * 【工程架构说明】
 * ============================================================
 *
 *   ┌─────────────────────────────────────────┐
 *   │              main.c（主循环）           │
 *   │  轮询 g_task_flag，调用对应 task_xxx()  │
 *   └──────────────┬──────────────────────────┘
 *                  │ 调用
 *   ┌──────────────▼──────────────────────────┐
 *   │         app/（应用层）                  │
 *   │  app_task.c  — 任务调度、状态机         │
 *   │  app_control.c — PID 控制算法           │
 *   └──────────────┬──────────────────────────┘
 *                  │ 调用
 *   ┌──────────────▼──────────────────────────┐
 *   │         bsp/（驱动层）                  │
 *   │  bsp_motor   bsp_encoder   bsp_track    │
 *   │  bsp_led     bsp_key                    │
 *   └──────────────┬──────────────────────────┘
 *                  │ 依赖
 *   ┌──────────────▼──────────────────────────┐
 *   │         config/（配置层）               │
 *   │  board_config.h — 所有引脚和参数定义    │
 *   └─────────────────────────────────────────┘
 *
 * ============================================================
 * 【快速上手 — 如何用本框架做新项目】
 * ============================================================
 *
 *  第1步：修改 board_config.h 中的引脚和参数
 *  第2步：在 user/bsp/ 下添加新硬件驱动（参考 bsp_template.c）
 *  第3步：在 user/app/ 下添加新任务（参考 app_task.h 头部5步流程）
 *  第4步：在本文件 app_init() 中调用新模块的初始化函数
 *  第5步：在本文件主循环中添加新任务的执行块
 *
 *  ★ 不需要改动 core/ 目录下的任何代码，那是底层框架。
 *
 * ============================================================
 * 【如何添加新 BSP 模块（例如：OLED 显示屏）】
 * ============================================================
 *   1. 在 user/bsp/ 下新建 bsp_xxx.c / bsp_xxx.h
 *      → 复制 bsp_template.c/h 开始写
 *   2. 在 board_config.h 添加相关引脚/参数宏定义
 *   3. 在下方 #include "bsp_xxx.h"
 *   4. 在 bsp_init() 中调用 xxx_init()
 *   5. 在对应任务中调用驱动函数
 *
 * ============================================================
 * 【如何添加新任务（完整 5 步流程见 app_task.h）】
 * ============================================================
 *   简要：在 board_config.h 加周期 → app_task.h 加标志位 →
 *         app_task.c 定时器回调中置位 → 写 task_xxx() 函数 →
 *         在本文件主循环中调用
 */

#include "zf_common_headfile.h"

/* BSP 层 */
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_motor.h"
#include "bsp_encoder.h"
#include "bsp_track.h"
#include "bsp_w25q128.h"

/* APP 层 */
#include "app_control.h"
#include "app_task.h"

/*-----------------------------------------------------------
 * 系统初始化 — 时钟 + 调试串口
 *-----------------------------------------------------------*/
static void system_init(void)
{
    /* 时钟初始化（必须第一个调用） */
    clock_init(SYSTEM_CLOCK);

    /* 调试串口初始化（读取 board_config.h 中的 DEBUG_UART_* 宏配置） */
    debug_init();

    printf("\r\n");
    printf("========================================\r\n");
    printf("  %s  v%s\r\n", PROJECT_NAME, FW_VERSION_STR);
    printf("  MCU: MSPM0G3507 @ %dMHz\r\n", SYSTEM_CLOCK / 1000000);
    printf("  Build: %s %s\r\n", __DATE__, __TIME__);
    printf("========================================\r\n");
}

/*-----------------------------------------------------------
 * BSP 层初始化
 *
 * 【扩展说明】新增硬件模块时，在此处追加 xxx_init() 调用：
 *   static void bsp_init(void)
 *   {
 *       led_init();
 *       motor_init();
 *       xxx_init();   // ← 在这里添加新模块初始化
 *   }
 *-----------------------------------------------------------*/
static void bsp_init(void)
{
    printf("[BSP] Initializing...\r\n");

    led_init();         /* LED：上电状态指示 */
    bsp_key_init();     /* 按键：启动/停止/调速 */
    motor_init();       /* 电机：初始化 PWM 和方向引脚，默认输出0 */
    encoder_init();     /* 编码器：正交/方向模式初始化 */
    track_init();       /* 循迹传感器：ADC 初始化 */

#if W25Q128_USE
    w25q128_init();         /* W25Q128：SPI1 Flash，启动时只做总线初始化 */
#else
    /* Flash 禁用时仍固定拉高 CS，避免其误接收 LCD 的 SPI 数据。 */
    gpio_init(W25Q128_CS_PIN, GPO, GPIO_HIGH, GPO_PUSH_PULL);
#endif

    printf("[BSP] Done.\r\n");
}

/*-----------------------------------------------------------
 * APP 层初始化
 *
 * 【扩展说明】新增应用模块时，在此处追加 xxx_init() 调用
 *-----------------------------------------------------------*/
static void app_init(void)
{
    printf("[APP] Initializing...\r\n");

    control_init();     /* PID 控制参数初始化（含 Flash 恢复） */
    task_init();        /* 任务调度器初始化（PIT 定时器） */

    printf("[APP] Done.\r\n");
}

/*-----------------------------------------------------------
 * 开机自检
 *
 * 用于验证硬件连接是否正确，新项目可根据需要修改自检内容。
 * 建议检查项：
 *   - LED 闪烁确认程序已启动
 *   - 串口打印传感器原始值确认接线正确
 *   - 有显示屏的项目可在此处刷屏确认 SPI 正常
 *-----------------------------------------------------------*/
static void self_test(void)
{
    printf("[TEST] Self testing...\r\n");

    /* LED 按配置闪烁，提示程序已启动。 */
    led_blink(LED_SELF_TEST_TIMES, LED_SELF_TEST_MS);

    /* 打印循迹传感器原始值，确认接线正常 */
    track_data_t track;
    track_read(&track);
    printf("[TEST] Track:");
    for (uint8 i = 0; i < TRACK_SENSOR_NUM; i++)
    {
        printf(" %d", track.raw[i]);
    }
    printf("\r\n");

    printf("[TEST] Ready. Short press KEY1 to start/stop.\r\n");

    /* LED 常亮表示初始化完成 */
    led_on();
    system_delay_ms(200);
    led_off();
}

/*-----------------------------------------------------------
 * 主函数
 *
 * ============================================================
 * 【任务执行模型】
 * ============================================================
 *   中断（PIT）只置位标志，主循环负责执行。
 *   好处：任务函数可以使用 printf / 延时，不受中断限制。
 *   原则：每个 task_xxx() 内部不要做长时间阻塞（> 1个周期）。
 *
 * ============================================================
 * 【任务执行顺序 — 数据依赖关系】
 * ============================================================
 *   task_track()   → 采集传感器原始数据
 *   task_control() → 读编码器 + 用 track 数据计算 PID + 输出电机
 *   task_display() → 打印上面两步的结果（调试用）
 *   task_key()     → 独立，与控制流无依赖
 *   以上顺序在下方主循环中天然保证（按 flag 依次判断）。
 *
 * ============================================================
 * 【如何新增任务（以串口上报任务为例）】
 * ============================================================
 *   Step 1. board_config.h 加：#define TASK_UART_PERIOD 10
 *   Step 2. app_task.h 的 task_flag_t 加：uint8 uart;
 *   Step 3. app_task.c 的 task_timer_callback() 加：
 *             if (s_task_tick % TASK_UART_PERIOD == 0) g_task_flag.uart = 1;
 *   Step 4. app_task.c 实现：void task_uart(void) { ... }
 *           app_task.h 声明：void task_uart(void);
 *   Step 5. 在下面仿照其他 if 块加入：
 *             if (g_task_flag.uart) { g_task_flag.uart = 0; task_uart(); }
 * ============================================================
 *-----------------------------------------------------------*/
int main(void)
{
    /*==================== 初始化 ====================*/
    system_init();
    bsp_init();
    app_init();
    self_test();

    /*==================== 主循环 ====================*/
    while (1)
    {
        /* 循迹采样任务（5ms） */
        if (g_task_flag.track)
        {
            g_task_flag.track = 0;
            task_track();
        }

        /* 控制计算任务（10ms）：读编码器 → 循迹偏差 → PID → 输出电机 */
        if (g_task_flag.control)
        {
            g_task_flag.control = 0;
            task_control();
        }

        /* 显示刷新任务（300ms）：串口打印状态 */
        if (g_task_flag.display)
        {
            g_task_flag.display = 0;
            task_display();
        }

        /* 按键处理任务（20ms）：短按启停 */
        if (g_task_flag.key)
        {
            g_task_flag.key = 0;
            task_key();
        }
    }
}
