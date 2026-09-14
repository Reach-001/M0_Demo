/**
 * @file    bsp_track.h
 * @brief   循迹传感器驱动模块 — 多路红外/灰度传感器（加权重心算法）
 * @author  Reach
 * @version v1.0
 *
 * 引脚配置：board_config.h 中的 TRACK_SENSOR_1~8 / TRACK_SENSOR_NUM
 * 偏差范围：由权值决定，默认 [-100, +100]
 *   负数 → 小车偏左 → 向右打舵（steer > 0）
 *   正数 → 小车偏右 → 向左打舵（steer < 0）
 *   0    → 居中
 *
 * 使用示例：
 * @code
 *   track_init();          // 初始化 ADC，bsp_init() 中调用
 *
 *   // 在循迹采样任务中（每 5ms）：
 *   track_data_t data;
 *   track_read(&data);     // 读取并数字化所有传感器
 *
 *   // 在控制任务中：
 *   if (track_is_lost(&data)) {
 *       // 丢线处理（使用上次偏差保持方向，或停车）
 *   } else {
 *       int16 error = track_get_error(&data);   // 计算偏差
 *   }
 *
 *   // 调试时打印原始值：
 *   for (uint8 i = 0; i < TRACK_SENSOR_NUM; i++)
 *       printf("%d ", data.raw[i]);   // 对照 TRACK_THRESHOLD 标定阈值
 * @endcode
 *
 * @note 阈值标定方法：
 *   1. 将小车放在轨道上，串口打印 raw 值
 *   2. 传感器在黑线上时记录最小值（如 1200），白色时记录最大值（如 3500）
 *   3. 将 TRACK_THRESHOLD 设为中间值（如 2000~2500）
 */

#ifndef _BSP_TRACK_H_
#define _BSP_TRACK_H_

#include "board_config.h"

/* 传感器原始数据结构 */
typedef struct
{
    uint16 raw[TRACK_SENSOR_NUM];   /* ADC 原始值 */
    uint8  digital[TRACK_SENSOR_NUM]; /* 数字化结果 (0=白, 1=黑) */
} track_data_t;

/**
 * @brief   循迹传感器初始化
 */
void track_init(void);

/**
 * @brief   读取所有传感器原始值
 * @param   data    数据结构指针
 */
void track_read_raw(track_data_t *data);

/**
 * @brief   读取并数字化传感器数据
 * @param   data    数据结构指针
 */
void track_read(track_data_t *data);

/**
 * @brief   计算循迹偏差 (加权算法)
 * @param   data    传感器数据
 * @return  int16   偏差值 [-100, +100]
 *                  负数=偏左, 正数=偏右, 0=居中
 * BUG FIX 6（潜在）: 返回类型由 int8 改为 int16，防止权值均值超出 ±127 时截断
 */
int16 track_get_error(track_data_t *data);

/**
 * @brief   检测是否全白 (丢线)
 * @param   data    传感器数据
 * @return  uint8   1=全白(丢线), 0=正常
 */
uint8 track_is_lost(track_data_t *data);

/**
 * @brief   检测是否全黑 (十字/起点线)
 * @param   data    传感器数据
 * @return  uint8   1=全黑, 0=正常
 */
uint8 track_is_cross(track_data_t *data);

/**
 * @brief   设置黑白阈值 (用于标定)
 * @param   threshold   阈值
 */
void track_set_threshold(uint16 threshold);

/**
 * @brief   获取当前阈值
 * @return  uint16  阈值
 */
uint16 track_get_threshold(void);

#endif /* _BSP_TRACK_H_ */
