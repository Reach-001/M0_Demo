/**
 * @file    app_control.h
 * @brief   控制算法模块 — PID 转向控制 + PI 速度控制
 * @author  Reach
 * @version v1.1
 *
 * ============================================================
 * 【PID 控制器说明】
 * ============================================================
 *
 * 转向控制（PD）：
 *   输入：循迹偏差 track_error（负=偏左，正=偏右）
 *   输出：差速修正量 steer（正值 → 左轮减速、右轮加速 → 小车向右转）
 *   公式：steer = KP × error + KD × (error - last_error)
 *
 * 速度控制（PI）：
 *   输入：当前编码器均值速度
 *   输出：速度基准量（加上差速后分别输出到左右电机）
 *   公式：speed = KP × (target - actual) + KI × Σerror
 *
 * 电机输出：
 *   左电机 = speed_out - steer   （转向帮左轮减速）
 *   右电机 = speed_out + steer   （转向帮右轮加速）
 *
 * ============================================================
 * 【PID 参数整定指南】
 * ============================================================
 *
 * 转向 PD（先调 KP，后加 KD）：
 *   1. KI=0, KD=0，只用 KP
 *   2. 逐渐增大 KP，直到小车能跟线但有左右摆动
 *   3. 增大 KD 抑制超调和摆动；KD 过大 → 高频抖动
 *   4. 一般循迹不需要 KI（积分项），保持 0
 *
 * 速度 PI（先调 KP，后加 KI）：
 *   1. KI=0，先调 KP，让速度能跟上目标值
 *   2. 如果稳态有误差，缓慢增大 KI
 *   3. KI 过大 → 积分饱和振荡
 *   4. INTEGRAL_MAX 限制积分累积，防止积分饱和
 *
 * 参数调整入口：board_config.h 中的 PID_STEER_* 和 PID_SPEED_*
 * 在线调参：通过 control_set_steer_pid() / control_set_speed_pid() 运行时修改
 *
 * ============================================================
 * 【如何使用 — 示例代码】
 * ============================================================
 *
 * @code
 * // 初始化（在 app_init 中调用一次）
 * control_init();
 *
 * // 在控制任务中（每 10ms 调用一次）
 * int16 error  = track_get_error(&track_data);    // 获取循迹偏差
 * int16 enc_l  = encoder_get_and_clear(ENCODER_LEFT);  // 左轮速度
 * int16 enc_r  = encoder_get_and_clear(ENCODER_RIGHT);  // 右轮速度
 * int16 motor_l, motor_r;
 * control_run(error, enc_l, enc_r, &motor_l, &motor_r);
 * motor_set_speed_both(motor_l, motor_r);
 *
 * // 运行时调速
 * control_set_target_speed(150);  // 设置目标速度
 *
 * // 启停控制
 * control_enable(1);   // 使能，自动重置 PID 状态
 * control_enable(0);   // 禁用
 * @endcode
 */

#ifndef _APP_CONTROL_H_
#define _APP_CONTROL_H_

#include "board_config.h"

/*-----------------------------------------------------------
 * PID 参数结构体
 *
 * @note 每个 PID 实例独立维护积分和误差历史，
 *       支持同时存在多个控制器（如转向 + 速度）
 *-----------------------------------------------------------*/
typedef struct
{
    float kp;           ///< 比例系数（越大响应越快，过大会振荡）
    float ki;           ///< 积分系数（消除稳态误差，循迹一般置0）
    float kd;           ///< 微分系数（抑制超调，过大高频抖动）
    float integral;     ///< 积分累计（内部使用，reset 时清零）
    float last_error;   ///< 上次误差（内部使用，用于微分计算）
    float integral_max; ///< 积分限幅（防止积分饱和）
    float output_max;   ///< 输出限幅（防止输出超调）
} pid_t;

/*-----------------------------------------------------------
 * 控制参数结构体（便于保存/加载到 Flash）
 *-----------------------------------------------------------*/
typedef struct
{
    pid_t steer_pid;    ///< 转向 PID 参数
    pid_t speed_pid;    ///< 速度 PID 参数
    int16 target_speed; ///< 目标速度（编码器计数/采样周期）
    uint8 enable;       ///< 控制使能标志（1=使能, 0=禁用）
} control_param_t;

/*============================================================
 * 初始化
 *============================================================*/

/**
 * @brief   控制模块初始化
 *
 * 从 board_config.h 加载默认 PID 参数，设置初始状态。
 * 调用位置：main.c → app_init()
 */
void control_init(void);

/*============================================================
 * 控制计算（核心算法）
 *============================================================*/

/**
 * @brief   转向控制计算 — PD 控制器
 * @param   track_error     循迹偏差（负=偏左，正=偏右）
 * @return  int16           转向差速量（正值=右转，负值=左转）
 *
 * @note    track_error 类型为 int16，与 track_get_error() 返回值对齐
 */
int16 control_steer(int16 track_error);

/**
 * @brief   速度控制计算 — PI 控制器
 * @param   current_speed   当前速度（编码器读数）
 * @return  int16           速度基准输出
 */
int16 control_speed(int16 current_speed);

/**
 * @brief   综合控制输出 — 电机差速计算
 *
 * 一次调用完成：转向 PD + 速度 PI → 左右电机 PWM 值
 *
 * @param   track_error     循迹偏差
 * @param   speed_l         左轮速度反馈（编码器值）
 * @param   speed_r         右轮速度反馈（编码器值）
 * @param   out_l           [out] 左电机 PWM 输出
 * @param   out_r           [out] 右电机 PWM 输出
 *
 * @note    调用前检查 control_is_enabled()，禁用时直接输出 0
 */
void control_run(int16 track_error, int16 speed_l, int16 speed_r,
                 int16 *out_l, int16 *out_r);

/*============================================================
 * 参数设置（运行时调参）
 *============================================================*/

/**
 * @brief   设置目标速度
 * @param   speed   目标速度（自动限幅到 [0, TARGET_SPEED_MAX]）
 */
void control_set_target_speed(int16 speed);

/**
 * @brief   获取目标速度
 * @return  当前目标速度
 */
int16 control_get_target_speed(void);

/**
 * @brief   使能/禁用控制
 * @param   enable  1=使能（重置 PID 状态）, 0=禁用
 *
 * @note    从禁用变为使能时，自动调用 control_reset() 清零积分和误差
 */
void control_enable(uint8 enable);

/**
 * @brief   获取控制使能状态
 * @return  1=使能, 0=禁用
 */
uint8 control_is_enabled(void);

/**
 * @brief   在线设置转向 PID 参数
 * @param   kp/kd  比例/积分/微分系数
 */
void control_set_steer_pid(float kp, float ki, float kd);

/**
 * @brief   在线设置速度 PID 参数
 * @param   kp/kd  比例/积分/微分系数
 */
void control_set_speed_pid(float kp, float ki, float kd);

/**
 * @brief   获取控制参数指针（用于读取全部参数）
 * @return  control_param_t*  参数结构体指针
 */
control_param_t* control_get_param(void);

/**
 * @brief   重置 PID 状态 — 清零积分和误差历史
 *
 * 调用时机：启动前、切换模式时
 */
void control_reset(void);

/*============================================================
 * Flash 掉电保存（可选功能）
 *============================================================*/

/**
 * @brief   保存控制参数到 Flash
 * @return  void
 *
 * 保存内容：转向PID、速度PID、目标速度
 * 存储格式：魔数 + 版本号 + 参数 + 校验和
 *
 * @warning Flash 擦写寿命约 10 万次，不要在循环中频繁调用
 *          建议仅在用户触发保存时调用（如长按按键）
 */
void control_save_to_flash(void);

/**
 * @brief   从 Flash 加载控制参数（上电自动调用）
 * @return  void（无效数据时静默使用默认值）
 *
 * @note    在 control_init() 中自动调用，无需手动调用
 *          若 Flash 中无有效数据（首次烧录），自动使用默认值
 */
void control_load_from_flash(void);

#endif /* _APP_CONTROL_H_ */
