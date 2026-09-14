/**
 * @file    board_config.h
 * @brief   硬件配置文件 — 所有引脚、参数、阈值的唯一入口
 * @author  Reach
 * @version v1.0
 *
 * ============================================================
 * 【快速移植指南 — 用本工程做新项目时只需改这里】
 * ============================================================
 *
 * 第 1 步：修改引脚定义（★ 必改）
 *   - 电机 PWM/方向引脚：MOTOR_L_*, MOTOR_R_*
 *   - 编码器引脚：ENCODER_L_*, ENCODER_R_*
 *   - 循迹传感器：TRACK_SENSOR_1 ~ TRACK_SENSOR_N
 *   - LED / 按键引脚：LED_PIN, KEY1_PIN, KEY2_PIN
 *
 * 第 2 步：修改传感器数量（★ 必改）
 *   - 改 TRACK_SENSOR_NUM，同步修改 TRACK_WEIGHT_LIST 的元素个数
 *   - 权值从左到右排列，居中传感器权值为 0
 *   示例（5路）：{-100, -50, 0, 50, 100}
 *   示例（8路）：{-100, -70, -40, -10, 10, 40, 70, 100}
 *
 * 第 3 步：上电后调试（建议顺序）
 *   a. 接好串口，观察 self_test 打印的传感器原始值
 *   b. 标定 TRACK_THRESHOLD（黑线处约 2500+，白色约 1000，居中取值）
 *   c. 调试方向：跑一小段，若跑偏反向则交换左右电机或改编码器取反
 *   d. 整定 PID（先调 KP，再加 KD 抑制超调，速度环最后调）
 *
 * 可选修改（不影响编译）：
 *   - 调度周期 TASK_PERIOD_MS / TASK_XXX_PERIOD
 *   - PWM 频率 MOTOR_PWM_FREQ（一般10kHz即可）
 *   - 长按阈值 KEY_LONG_PRESS_COUNT（50 × 20ms = 1s）
 *
 * ⚠ 注意事项：
 *   1. TASK_PIT 不能与电机 PWM 使用同一个定时器（TIMA0 已被 PWM 占用）
 *   2. TRACK_WEIGHT_LIST 的元素个数必须等于 TRACK_SENSOR_NUM
 *   3. MOTOR_PWM_MAX + MOTOR_DEADZONE 不能超过 65535（uint16 上限）
 * ============================================================
 */

#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include "zf_common_headfile.h"

/*===========================================================================
 * 工程信息（★ 新建项目时修改此处）
 *===========================================================================*/
#define PROJECT_NAME            "LineFollower"       ///< 工程名称（自检和串口欢迎信息中显示）
#define FW_VERSION_MAJOR        1                   ///< 固件主版本号（大功能变更时+1）
#define FW_VERSION_MINOR        0                   ///< 固件次版本号（新增功能时+1）
#define FW_VERSION_PATCH        1                   ///< 固件修订号（Bug修复时+1）
#define FW_VERSION_STR          "1.0.1"             ///< 固件版本字符串（保持与上面三者同步）

/*===========================================================================
 * 系统配置（一般不需要修改）
 *===========================================================================*/
#define SYSTEM_CLOCK            SYSTEM_CLOCK_80M    ///< 系统主频 80MHz（PLL + 外部晶振）
#define DEBUG_UART_BAUD         115200              ///< 调试串口波特率，与上位机保持一致

/*===========================================================================
 * 调试串口配置（UART0）
 *
 * ★ UART0 引脚在 include/core/zf/zf_common_debug.h 中配置，不在此处
 *   默认：TX=A10, RX=A11, 115200
 *   修改方法：直接编辑 zf_common_debug.h 中的 DEBUG_UART_TX_PIN/RX_PIN 宏
 *   debug_init() 会自动读取这些宏的值，无需额外代码
 *===========================================================================*/

/*===========================================================================
 * LED 指示灯配置（★ 修改引脚 LED_PIN 即可）
 *===========================================================================*/
#define LED_PIN                 B22                 ///< 板载 LED 引脚（高电平亮）
#define LED_ON_LEVEL            1                   ///< LED 点亮电平：1=高电平亮，0=低电平亮
#define LED_SELF_TEST_TIMES     3                   ///< 上电自检闪烁次数（用于确认程序已启动）
#define LED_SELF_TEST_MS        100                 ///< 自检单次亮灭时长（ms）
#define LED_RUN_TOGGLE_COUNT    10                  ///< 运行时 LED 翻转间隔（单位：控制周期次数，10×10ms=100ms翻转一次）

/*===========================================================================
 * 按键配置（★ 修改引脚 KEY1_PIN / KEY2_PIN）
 *===========================================================================*/
#define KEY1_PIN                A0                  ///< 按键1 引脚
#define KEY2_PIN                A1                  ///< 按键2 引脚（预留，可扩展功能）
#define KEY_USER_PIN            B21                  ///< 按键 用户 引脚
#define KEY_ACTIVE_LEVEL        0                   ///< 按键触发电平：0=低电平触发（按下接地），1=高电平触发
#define KEY_DEBOUNCE_COUNT      2                   ///< 消抖次数（2 × 20ms = 40ms），增大可减少误触
#define KEY_LONG_PRESS_COUNT    30                  ///< 长按判定次数（30 × 20ms = 600ms），减小可缩短长按时间

/*===========================================================================
 * 电机配置（★ 必改：引脚需与驱动板接线一致）
 *
 * 接线说明（以 TB6612 为例）：
 *   MOTOR_L_PWM  → PWMA    MOTOR_L_DIR1 → AIN1    MOTOR_L_DIR2 → AIN2
 *   MOTOR_R_PWM  → PWMB    MOTOR_R_DIR1 → BIN1    MOTOR_R_DIR2 → BIN2
 *
 * 若小车跑反方向：在 bsp_motor.c 的 motor_set_speed() 中交换 DIR1/DIR2
 *===========================================================================*/
/* 电机1 (左电机) */
#define MOTOR_L_PWM             PWM_TIM_A0_CH0_A8   ///< 左电机 PWM（使用 TIMA0 CH0，避开 W25Q128 的 PB8）
#define MOTOR_L_DIR1            A12                 ///< 左电机方向引脚1（避开 W25Q128 的 PB9）
#define MOTOR_L_DIR2            A13                 ///< 左电机方向引脚2（避开 LCD_RES 的 PB10）

/* 电机2 (右电机) */
#define MOTOR_R_PWM             PWM_TIM_A0_CH1_B12  ///< 右电机 PWM（使用 TIMA0 CH1）
#define MOTOR_R_DIR1            B13                 ///< 右电机方向引脚1
#define MOTOR_R_DIR2            A14                 ///< 右电机方向引脚2（避开 LCD_CS 的 PB14）

/* 电机参数（一般不需要修改） */
#define MOTOR_PWM_FREQ          10000               ///< PWM 频率 10kHz（超出人耳听觉范围，消除噪音）
#define MOTOR_PWM_MAX           10000               ///< PWM 最大占空比计数值（对应100%）
#define MOTOR_DEADZONE          500                 ///< 死区补偿值（补偿电机启动静摩擦力，约5%占空比）

/*===========================================================================
 * 编码器配置（★ 必改：引脚需与实际接线一致）
 *
 * 注意：左轮使用正交模式（两路脉冲），右轮使用方向模式（脉冲+方向信号）
 * 若两路都是正交编码器，可在 bsp_encoder.c 中将右轮改为 encoder_quad_init()
 *===========================================================================*/
/* 编码器1 (左轮) - 正交模式
 * ⚠ A5/A6 被外部 32MHz 晶振 (HFXT) 占用，改为 A7(CH1) + A2(CH2) */
#define ENCODER_L_TIM           TIM_G8              ///< 左轮编码器定时器（TIMG8，支持正交解码）
#define ENCODER_L_CH1           TIMG8_ENCODER1_CH1_A7   ///< 左轮编码器 A 相（A7，AF4 — 避免 A5 与 HFXIN 冲突）
#define ENCODER_L_CH2           TIMG8_ENCODER1_CH2_A2   ///< 左轮编码器 B 相（A2，AF2 — 避免 A6 与 HFXOUT 冲突）

/* 编码器2 (右轮) - 方向模式 */
#define ENCODER_R_TIM           TIM_G7              ///< 右轮编码器定时器（TIMG7）
#define ENCODER_R_CH1           TIMG7_ENCODER1_CH1_B15  ///< 右轮编码器脉冲信号
#define ENCODER_R_DIR           B16                 ///< 右轮编码器方向信号（高电平正转）

/* 编码器参数 */
#define ENCODER_RESOLUTION      1024                ///< 编码器线数×4（正交4倍频），需与实物一致
#define WHEEL_DIAMETER          65                  ///< 轮子直径（mm），用于速度换算
#define ENCODER_SAMPLE_MS       10                  ///< 编码器采样周期（ms），需与 TASK_CONTROL_PERIOD 一致

/*===========================================================================
 * 循迹传感器配置（★ 必改：数量、引脚、权值）
 *
 * 权值设定原则：
 *   - 最左传感器权值最负（如 -100），最右最正（如 +100），居中为 0
 *   - 偏差 = Σ(有效传感器权值) / 有效传感器数量
 *   - 偏差为负 → 小车偏左 → 向右打舵；偏差为正 → 偏右 → 向左打舵
 *
 * 扩展传感器数量步骤：
 *   1. 修改 TRACK_SENSOR_NUM
 *   2. 修改 TRACK_WEIGHT_LIST（元素个数必须等于 TRACK_SENSOR_NUM）
 *   3. 在下方 TRACK_SENSOR_6~8 取消注释并填入实际引脚
 *===========================================================================*/
/* ★ 修改传感器数量（1~8 路可选） */
#define TRACK_SENSOR_NUM        5

/* ★ 传感器引脚（从左到右排列） */
#define TRACK_SENSOR_1          ADC0_CH0_A27        ///< 最左传感器
#define TRACK_SENSOR_2          ADC0_CH1_A26
#define TRACK_SENSOR_3          ADC0_CH2_A25        ///< 中间传感器
#define TRACK_SENSOR_4          ADC0_CH3_A24
#define TRACK_SENSOR_5          ADC0_CH4_B25        ///< 最右传感器
#define TRACK_SENSOR_6          ADC0_CH5_B24        ///< 扩展预留（TRACK_SENSOR_NUM >= 6 时生效）
#define TRACK_SENSOR_7          ADC0_CH6_B20        ///< 扩展预留
#define TRACK_SENSOR_8          ADC0_CH7_A22        ///< 扩展预留

/* ★ 传感器参数（需实际标定） */
#define TRACK_THRESHOLD         2000                ///< 黑白判定阈值（黑线 > 阈值）。标定方法：串口打印 raw 值，取黑/白中间值
#define TRACK_ADC_RESOLUTION    ADC_12BIT           ///< ADC 分辨率（12位 → 0~4095）
#define TRACK_ADC_FILTER_COUNT  3                   ///< 均值滤波采样次数，增大可降低噪声但增加采样时间
#define TRACK_LOST_MAX_COUNT    50                  ///< 连续丢线保护次数（50 × 5ms = 250ms 后停车）

/* ★ 权值列表（元素数必须等于 TRACK_SENSOR_NUM） */
#define TRACK_WEIGHT_LIST       {-100, -50, 0, 50, 100}

/*===========================================================================
 * 串口通信配置（可选，使用无线模块时修改）
 *===========================================================================*/
#define WIRELESS_UART           UART_1              ///< 无线串口/蓝牙模块使用的 UART 编号
#define WIRELESS_UART_BAUD      115200              ///< 无线模块波特率（需与模块设置一致）
#define WIRELESS_UART_TX        UART1_TX_B4         ///< 无线模块 TX 引脚
#define WIRELESS_UART_RX        UART1_RX_B5         ///< 无线模块 RX 引脚

/*===========================================================================
 * SPI Flash 配置（W25Q128）
 *
 * 原理图连接：
 *   W_CS   -> PB6
 *   W_MISO -> PB7
 *   W_MOSI -> PB8
 *   W_CLK  -> PB9
 *
 * 注意：PB6~PB9 已固定留给板载 Flash，其它外设不要再复用这组引脚。
 *===========================================================================*/
#define W25Q128_USE             0                   ///< 调试 UI 时关闭，PB6 仍保持高电平以释放 SPI1
#define W25Q128_SPI_INDEX       SPI_1               ///< W25Q128 使用 SPI1
#define W25Q128_SPI_MODE        SPI_MODE0           ///< W25Q128 支持 SPI Mode0/Mode3，默认使用 Mode0
#define W25Q128_SPI_BAUD        (10000000)          ///< SPI 时钟 10MHz，先保证连线可靠后再提高
#define W25Q128_CS_PIN          B6                  ///< Flash 片选，低电平有效
#define W25Q128_MISO_PIN        SPI1_MISO_B7        ///< Flash DO / MISO
#define W25Q128_MOSI_PIN        SPI1_MOSI_B8        ///< Flash DI / MOSI
#define W25Q128_SCK_PIN         SPI1_SCK_B9         ///< Flash CLK

/*===========================================================================
 * 1.47 inch 172x320 SPI LCD 配置（ST7789）
 *
 * 与板载 W25Q128 共用 SPI1 的 SCK/MOSI/MISO：
 *   LCD_SCL -> PB9   (SPI1_SCK)
 *   LCD_SDA -> PB8   (SPI1_MOSI)
 *   FLASH   -> PB7   (SPI1_MISO)
 *
 * 片选独立：
 *   FLASH_CS -> PB6
 *   LCD_CS   -> PB14
 *===========================================================================*/
#define LCD147_USE               0                     ///< 是否启用 1.47 寸 LCD（0=禁用, 1=启用，启用前需接入 LCD 驱动代码）
#define LCD147_SPI_INDEX         SPI_1
#define LCD147_SPI_BAUD          (10000000)
#define LCD147_SCK_PIN           SPI1_SCK_B9
#define LCD147_MOSI_PIN          SPI1_MOSI_B8
#define LCD147_MISO_PIN          SPI_MISO_NULL
#define LCD147_RST_PIN           B10
#define LCD147_DC_PIN            B11
#define LCD147_CS_PIN            B14
#define LCD147_BL_PIN            B26
#define LCD147_WIDTH             172                   ///< 原厂方向 1 的逻辑宽度
#define LCD147_HEIGHT            320                   ///< 原厂方向 1 的逻辑高度
#define LCD147_COLUMN_OFFSET     34
#define LCD147_LINE_OFFSET       0
#define LCD147_USE_HORIZONTAL    1                     ///< 先使用原厂示例验证过的 172x320 方向

#define CONTROL_PARAM_FLASH_USE  0                     ///< 是否启用 Flash 参数保存（0=每次使用默认值, 1=掉电保存 PID 参数）

/*===========================================================================
 * 定时任务配置
 *
 * 任务调度原理：
 *   PIT 每隔 TASK_PERIOD_MS 触发一次中断，中断中对计数器取模来控制各任务频率。
 *   实际周期 = TASK_PERIOD_MS × TASK_xxx_PERIOD（毫秒）
 *
 * ⚠ TASK_PIT 不能使用 PIT_TIM_A0（已被电机 PWM 占用）
 *    可选：PIT_TIM_A1 / PIT_TIM_G0 / PIT_TIM_G6 / PIT_TIM_G7 / PIT_TIM_G8 / PIT_TIM_G12
 *===========================================================================*/
/* BUG FIX 2（致命）: 原 PIT_TIM_A0 与电机 PWM (MOTOR_L_PWM/MOTOR_R_PWM 均使用 TIMA0) 冲突，
 * timer_funciton_check() 会触发 assert → while(1)，程序卡死，LED不亮。
 * 改用 PIT_TIM_A1，TIMA1 未被其他外设占用。 */
#define TASK_PIT                PIT_TIM_A1          /* 任务调度定时器 (TIMA0已被电机PWM占用) */
#define TASK_PERIOD_MS          5                   ///< 基础调度周期 5ms（所有任务周期的最大公约数）

/* 各任务执行周期（单位：调度次数） */
#define TASK_TRACK_PERIOD       1                   ///< 循迹采样：1×5ms = 5ms（20Hz，满足实时性）
#define TASK_CONTROL_PERIOD     2                   ///< 控制计算：2×5ms = 10ms（100Hz）
#define TASK_DISPLAY_PERIOD     60                  ///< 显示刷新：60×5ms = 300ms（全屏刷新~80ms，保留余量避免 SPI 冲突）
#define TASK_KEY_PERIOD         4                   ///< 按键扫描：4×5ms = 20ms（消抖周期基准）

/* 新增任务时，在这里添加 TASK_xxx_PERIOD，再到 app_task.h/c 和 main.c 接入。 */
#define TASK_TICK_MAX           10000               ///< 计数器回绕上限（防止极端情况下取模异常）

/*===========================================================================
 * PID 参数默认值（需根据实际小车整定）
 *
 * 整定方法（经验法，逐步调试）：
 * 【转向 PD】
 *   1. KI=0, KD=0，只用 KP，逐渐增大直到小车能跟线但有左右摆动
 *   2. 增大 KD，抑制超调和摆动；KD 过大会引起高频抖动
 *   3. 一般循迹不需要 KI（积分项），保持 0
 *
 * 【速度 PI】
 *   1. KD=0，先调 KP，让速度能跟上目标值
 *   2. 如果稳态有误差，缓慢增大 KI；KI 过大会引起积分饱和振荡
 *   3. INTEGRAL_MAX 限制积分累积，防止积分饱和
 *===========================================================================*/
/* 转向 PD 控制（★ 需调参） */
#define PID_STEER_KP            50.0f               ///< 转向比例系数（过大则摆动，过小则跟线迟缓）
#define PID_STEER_KI            0.0f                ///< 转向积分系数（循迹一般置0）
#define PID_STEER_KD            10.0f               ///< 转向微分系数（抑制超调，过大高频抖动）
#define PID_STEER_INTEGRAL_MAX  1000.0f             ///< 转向积分限幅（防止积分饱和）
#define PID_STEER_OUTPUT_MAX    5000.0f             ///< 转向输出限幅（最大差速量，不超过 MOTOR_PWM_MAX）

/* 速度 PI 控制（★ 需调参） */
#define PID_SPEED_KP            20.0f               ///< 速度比例系数
#define PID_SPEED_KI            5.0f                ///< 速度积分系数（消除稳态速度误差）
#define PID_SPEED_KD            0.0f                ///< 速度微分系数（一般置0）
#define PID_SPEED_INTEGRAL_MAX  5000.0f             ///< 速度积分限幅
#define PID_SPEED_OUTPUT_MAX    ((float)MOTOR_PWM_MAX)  ///< 速度输出上限（等于 PWM 最大值）

/* 目标速度（★ 需根据场地调整） */
#define TARGET_SPEED_DEFAULT    100                 ///< 默认目标速度（编码器计数/采样周期）
#define TARGET_SPEED_MAX        300                 ///< 最大速度限制（超过此值长按按键不再增速）
#define TARGET_SPEED_STEP       20                  ///< 长按 KEY1 每次增速步进量

#endif /* _BOARD_CONFIG_H_ */
