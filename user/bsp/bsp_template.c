/**
 * @file    bsp_template.c
 * @brief   BSP 模块开发模板实现 — 复制后把"xxx"全文替换成你的模块名
 *
 * ============================================================
 * 【快速上手 — 跟着做就行】
 * ============================================================
 *
 * 1. 全文搜索替换：
 *      xxx      → 你的模块名（如 oled、imu）
 *      XXX      → 大写模块名（如 OLED、IMU）
 *
 * 2. 按照下面每个 TODO 标记，填入实际代码
 *
 * 3. 完成后把这段注释删掉（或保留作为记录）
 *
 * ============================================================
 * 【代码规范（统一风格）】
 * ============================================================
 *   - 模块私有的变量/函数加 static 前缀（只能在本 .c 文件内访问）
 *   - 公开的函数不加 static（在 .h 声明，外部可以调用）
 *   - 打印日志用 printf("[XXX] ...\r\n") 格式
 *   - 错误返回值用 bsp_status_t（定义在 bsp_common.h）
 *   - 参数检查用 BSP_ASSERT_RETURN(xxx, BSP_ERR_PARAM)
 */

#include "bsp_template.h"  /* TODO: 改成 #include "bsp_xxx.h" */

/*-----------------------------------------------------------
 * 模块私有变量 — 只有本文件能访问
 *
 * static 是 C 语言的"私有"关键字，
 * 加上后别的 .c 文件看不到这些变量，避免名字冲突。
 *-----------------------------------------------------------*/
static uint8 s_xxx_ready = 0;   /* 就绪标志：0=未初始化, 1=已就绪 */

/*-----------------------------------------------------------
 * 初始化实现
 *
 * 调用时机：main.c 的 bsp_init() 中，在所有模块初始化阶段调用。
 * 调用顺序：通常放在电机、编码器初始化之后，显示/通信模块之前。
 *
 * 典型初始化步骤：
 *   1. 如果定义了 XXX_USE 使能宏，先判断是否为 0
 *   2. 初始化 GPIO 引脚（从 board_config.h 取引脚宏）
 *   3. 初始化通信外设（SPI / I2C / UART / ADC / PWM 等）
 *   4. 发送硬件初始化命令（传感器/显示屏需要，LED/按键不需要）
 *   5. 设置默认参数和状态
 *   6. 打印日志 + 标记就绪
 *-----------------------------------------------------------*/
void xxx_init(void)
{
    /* TODO: 如果 board_config.h 中有 #define XXX_USE 1
     *       写成 #if XXX_USE，效果：宏为 0 时整段跳过编译 */
#if 1  /* TODO: 改为 #if XXX_USE（如果定义了使能宏的话） */

    /* ---------- 第1步：初始化 GPIO ---------- */
    /* TODO: 把 XXX_PIN_1 换成 board_config.h 中定义的引脚宏
     *       根据实际硬件选输出还是输入：
     *         输出示例：gpio_init(XXX_PIN, GPO, GPIO_LOW, GPO_PUSH_PULL);
     *         输入示例：gpio_init(XXX_PIN, GPI, GPIO_HIGH, GPI_PULL_UP); */

    /* ---------- 第2步：初始化通信外设 ---------- */
    /* TODO: 根据硬件通信方式选一个，其他的删掉：
     *
     *   SPI 设备（Flash/DAC/传感器）：
     *     spi_init(XXX_SPI_INDEX, XXX_SPI_MODE, XXX_SPI_BAUD,
     *              XXX_SCK_PIN, XXX_MOSI_PIN, XXX_MISO_PIN, XXX_CS_PIN);
     *
     *   软件 I2C 设备（OLED/IMU/EEPROM）：
     *     soft_iic_init(&s_iic_dev, XXX_I2C_ADDR, 100, XXX_SCL_PIN, XXX_SDA_PIN);
     *
     *   UART 设备（蓝牙/GPS）：
     *     uart_init(XXX_UART, XXX_UART_BAUD, XXX_UART_TX, XXX_UART_RX);
     *
     *   ADC 传感器：
     *     adc_init(XXX_ADC_PIN, ADC_12BIT);
     *
     *   纯 GPIO 设备（LED/按键/蜂鸣器）：
     *     已在第1步搞定，这里不需要额外初始化 */

    /* ---------- 第3步：发硬件初始化序列 ---------- */
    /* TODO: 显示屏、传感器等设备上电后需要发命令序列，
     *       不是所有模块都需要这一步。
     *
     *   示例（OLED）：
     *     xxx_write_cmd(0xAE);  // 关闭显示
     *     xxx_write_cmd(0x8D);  // 电荷泵设置
     *     xxx_write_cmd(0xAF);  // 开启显示 */

    /* ---------- 第4步：设默认状态 ---------- */
    /* TODO: 把模块的默认状态设好
     *   示例：电机初始速度为 0、LED 初始熄灭、传感器初始阈值 */

    /* ---------- 第5步：标记就绪 ---------- */
    s_xxx_ready = 1;
    printf("[XXX] Init OK.\r\n");

#else
    /* 模块禁用时，静默跳过 */
    (void)s_xxx_ready;
#endif
}

/*-----------------------------------------------------------
 * 其他接口函数实现
 *
 * 下面给出几种常见函数类型的实现模板，按需要选择。
 * 记得在 .h 文件中声明这些函数。
 *-----------------------------------------------------------*/

/*
 * 模板1：写操作（设置/控制类函数）
 *
 * 使用场景：LED 亮灭、电机调速、OLED 显示、串口发送
 *
 * bsp_status_t xxx_write(uint8 channel, int16 value)
 * {
 *     // 检查模块是否已初始化
 *     BSP_ASSERT_RETURN(s_xxx_ready, BSP_ERR_STATE);
 *
 *     // 参数范围检查
 *     // BSP_ASSERT_RETURN(channel < XXX_CHANNEL_NUM, BSP_ERR_PARAM);
 *
 *     // 实际写操作
 *     // gpio_set_level(XXX_PIN, value);
 *     // spi_write_8bit(XXX_SPI, value);
 *
 *     return BSP_OK;
 * }
 */

/*
 * 模板2：读操作（获取/采样类函数）
 *
 * 使用场景：读按键状态、读传感器值、读编码器计数
 *
 * bsp_status_t xxx_read(xxx_data_t *data)
 * {
 *     // 检查空指针
 *     BSP_ASSERT_RETURN(data != NULL, BSP_ERR_PARAM);
 *
 *     // 检查模块是否已初始化
 *     BSP_ASSERT_RETURN(s_xxx_ready, BSP_ERR_STATE);
 *
 *     // 实际读操作
 *     // data->raw = adc_convert(XXX_ADC_PIN);
 *     // data->filtered = xxx_filter(data->raw);
 *
 *     return BSP_OK;
 * }
 */

/*
 * 模板3：检查状态
 *
 * 使用场景：其他模块想知道这个模块是否正常
 *
 * uint8 xxx_is_ready(void)
 * {
 *     return s_xxx_ready;
 * }
 */
