/**
 * @file    bsp_template.h
 * @brief   BSP 模块开发模板 — 复制此文件快速创建新硬件驱动
 *
 * ============================================================
 * 【完整开发流程 — 6 步】
 * ============================================================
 *
 * Step 1. 复制文件
 *         把 bsp_template.h 和 bsp_template.c 复制一份，重命名为：
 *           bsp_xxx.h / bsp_xxx.c
 *         （比如你要写 OLED 驱动，就命名为 bsp_oled.h / bsp_oled.c）
 *
 * Step 2. 在 board_config.h 添加配置宏
 *         翻到 board_config.h 末尾（#endif 之前），粘贴这段模板：
 *
 *         // ==================== XXX 模块配置 ====================
 *         #define XXX_USE           1        // 0=禁用, 1=启用
 *         #define XXX_PIN_1         B22      // 第一个功能引脚
 *         #define XXX_PIN_2         B21      // 第二个功能引脚（没有就删掉）
 *         #define XXX_PARAM_DEFAULT 100      // 默认参数（按需定义）
 *         // =====================================================
 *
 *         注意：引脚不要碰 A3/A4/A5/A6（已被外部晶振占用）
 *
 * Step 3. 在本文件（bsp_xxx.h）中：
 *         - 搜索 "TODO" 替换每个占位符
 *         - 把 BSP_XXX_H 改成 BSP_OLED_H（两个下划线中间写模块名）
 *         - 声明初始化函数 xxx_init() 和所有对外接口
 *         - 定义模块专属的数据结构、枚举
 *
 * Step 4. 在 bsp_xxx.c 中：
 *         - 把头文件引用改成 #include "bsp_xxx.h"
 *         - 搜索 "TODO" 替换所有占位符
 *         - 实现初始化函数、读写函数、状态检查函数
 *
 * Step 5. 在每个用到此模块的 .c 文件顶部添加：
 *           #include "bsp_xxx.h"
 *         只包含当前文件实际使用的模块接口，避免无关依赖扩散。
 *
 * Step 6. 在 main.c 的 bsp_init() 中调用：
 *           static void bsp_init(void) {
 *               led_init();
 *               motor_init();
 *               xxx_init();   // ← 在这里添加
 *           }
 *         在合适的任务函数中调用驱动接口。
 *
 * ============================================================
 * 【参考现有模块】
 * ============================================================
 *   GPIO 输出类 → 参考 bsp_led.c/h（最简单的模板）
 *   GPIO 输入类 → 参考 bsp_key.c/h（按键状态机）
 *   PWM 驱动类 → 参考 bsp_motor.c/h（电机正反转）
 *   ADC 采集类 → 参考 bsp_track.c/h（多通道传感器）
 *   SPI 通信类 → 参考 bsp_w25q128.c/h（Flash 读写）
 *   编码器类   → 参考 bsp_encoder.c/h（正交/方向模式）
 */

#ifndef _BSP_TEMPLATE_H_
#define _BSP_TEMPLATE_H_

#include "board_config.h"
#include "bsp_common.h"    /* 提供 bsp_status_t 和 BSP_ASSERT_RETURN 宏 */

/*-----------------------------------------------------------
 * 模块数据结构（按需要定义）
 *
 * 所有的结构体、枚举都在这里声明，.c 文件只负责实现逻辑。
 *
 * 一个典型的传感器数据结构：
 * @code
 *   typedef struct {
 *       int16 raw;         // 原始读数
 *       int16 filtered;    // 滤波后读数
 *       uint8 status;      // 状态位
 *   } xxx_data_t;
 * @endcode
 *-----------------------------------------------------------*/

/*-----------------------------------------------------------
 * 初始化 — 必须在其他操作之前调用一次
 *
 * 调用位置：main.c → bsp_init()
 *-----------------------------------------------------------*/
void xxx_init(void);

/*-----------------------------------------------------------
 * 其他对外接口（根据模块功能添加）
 *
 * 命名规范：模块名_动作()
 *   写数据：xxx_write(...)、xxx_set_speed(...)、xxx_clear(...)
 *   读数据：xxx_read(...)、xxx_get_count(...)、xxx_get_state(...)
 *   检查状态：xxx_is_ready()、xxx_is_error()
 *
 * 返回值建议使用 bsp_status_t：
 *   BSP_OK         = 0   (成功)
 *   BSP_ERR_PARAM  = -1  (参数不对)
 *   BSP_ERR_HW     = -4  (硬件无响应)
 *-----------------------------------------------------------*/

#endif /* _BSP_TEMPLATE_H_ */
