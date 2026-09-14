/**
 * @file    bsp_encoder.h
 * @brief   编码器驱动模块 — 双路正交/方向编码器
 * @author  Reach
 * @version v1.0
 *
 * 引脚配置：board_config.h 中的 ENCODER_L_* / ENCODER_R_*
 *
 * 使用示例：
 * @code
 *   encoder_init();   // 初始化，bsp_init() 中调用一次
 *
 *   // 在控制任务中（每 10ms 调用一次）：
 *   int16 cnt_l = encoder_get_and_clear(ENCODER_LEFT);   // 原子读取并清零
 *   int16 cnt_r = encoder_get_and_clear(ENCODER_RIGHT);
 *   // cnt 含义：10ms 内的脉冲计数，正负表示方向
 *   // 若值偏小 → 速度慢或编码器线数配置有误
 *   // 若方向反 → 在 bsp_encoder.c 的 encoder_get() 中调整取反
 *
 *   // 转换为速度（mm/s）：
 *   float speed_l = encoder_count_to_speed(cnt_l);
 * @endcode
 *
 * @note encoder_get_and_clear() 内部已做原子保护（关中断），
 *       直接在主循环中调用即可，不需要额外加锁
 */

#ifndef _BSP_ENCODER_H_
#define _BSP_ENCODER_H_

#include "board_config.h"

/* 编码器编号枚举 */
typedef enum
{
    ENCODER_LEFT = 0,   /* 左轮编码器 */
    ENCODER_RIGHT,      /* 右轮编码器 */
    ENCODER_NUM         /* 编码器数量 */
} encoder_id_enum;

/**
 * @brief   编码器初始化
 */
void encoder_init(void);

/**
 * @brief   获取编码器计数值（带方向）
 * @param   id      编码器编号
 * @return  int16   计数值，正负表示方向
 */
int16 encoder_get(encoder_id_enum id);

/**
 * @brief   获取并清零编码器计数值
 * @param   id      编码器编号
 * @return  int16   计数值
 */
int16 encoder_get_and_clear(encoder_id_enum id);

/**
 * @brief   清零编码器计数
 * @param   id      编码器编号
 */
void encoder_clear(encoder_id_enum id);

/**
 * @brief   清零所有编码器
 */
void encoder_clear_all(void);

/**
 * @brief   计数值转换为速度 (脉冲/采样周期 → mm/s)
 * @param   count   计数值
 * @return  float   速度 (mm/s)
 */
float encoder_count_to_speed(int16 count);

#endif /* _BSP_ENCODER_H_ */
