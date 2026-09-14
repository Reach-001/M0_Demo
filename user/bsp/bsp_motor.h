/**
 * @file    bsp_motor.h
 * @brief   电机驱动模块 — 双路直流电机（兼容 TB6612 / DRV8833）
 * @author  Reach
 * @version v1.0
 *
 * 引脚配置：board_config.h 中的 MOTOR_L_* / MOTOR_R_*
 * 速度范围：[-MOTOR_PWM_MAX, +MOTOR_PWM_MAX]，正数前进，负数后退，0 停止
 *
 * 使用示例：
 * @code
 *   motor_init();                          // 初始化，bsp_init() 中调用
 *   motor_set_speed(MOTOR_LEFT,  500);     // 左电机正转，占空比 5%
 *   motor_set_speed(MOTOR_RIGHT, -500);    // 右电机反转
 *   motor_set_speed_both(800, 800);        // 双电机同速前进
 *   motor_stop_all();                      // 急停（刹车模式）
 *
 *   // 差速转向示例（配合 PID 输出）：
 *   int16 steer = 200;                     // 转向量（正=右转）
 *   int16 base  = 600;                     // 基础速度
 *   motor_set_speed_both(base - steer, base + steer);
 * @endcode
 *
 * @note 若小车上电后方向反：在 bsp_motor.c 的 motor_set_speed() 中
 *       交换对应电机的 DIR1 和 DIR2 的高低电平
 */

#ifndef _BSP_MOTOR_H_
#define _BSP_MOTOR_H_

#include "board_config.h"

/* 电机编号枚举 */
typedef enum
{
    MOTOR_LEFT = 0,     /* 左电机 */
    MOTOR_RIGHT,        /* 右电机 */
    MOTOR_NUM           /* 电机数量 */
} motor_id_enum;

/**
 * @brief   电机初始化
 */
void motor_init(void);

/**
 * @brief   设置单个电机速度
 * @param   id      电机编号
 * @param   speed   速度值 [-MOTOR_PWM_MAX, +MOTOR_PWM_MAX]
 *                  正数前进，负数后退
 */
void motor_set_speed(motor_id_enum id, int16 speed);

/**
 * @brief   同时设置双电机速度
 * @param   left    左电机速度
 * @param   right   右电机速度
 */
void motor_set_speed_both(int16 left, int16 right);

/**
 * @brief   电机停止
 * @param   id      电机编号
 */
void motor_stop(motor_id_enum id);

/**
 * @brief   所有电机停止
 */
void motor_stop_all(void);

/**
 * @brief   电机使能/失能 (可选，需硬件支持)
 * @param   enable  1=使能, 0=失能
 */
void motor_enable(uint8 enable);

#endif /* _BSP_MOTOR_H_ */
