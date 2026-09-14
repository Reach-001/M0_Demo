/**
 * @file    app_control.c
 * @brief   控制算法模块实现
 */

#include "app_control.h"

/* 控制参数实例 */
static control_param_t s_control = {
    /* 转向 PD */
    .steer_pid = {
        .kp = PID_STEER_KP,
        .ki = PID_STEER_KI,
        .kd = PID_STEER_KD,
        .integral = 0,
        .last_error = 0,
        .integral_max = PID_STEER_INTEGRAL_MAX,
        .output_max = PID_STEER_OUTPUT_MAX,
    },
    /* 速度 PI */
    .speed_pid = {
        .kp = PID_SPEED_KP,
        .ki = PID_SPEED_KI,
        .kd = PID_SPEED_KD,
        .integral = 0,
        .last_error = 0,
        .integral_max = PID_SPEED_INTEGRAL_MAX,
        .output_max = PID_SPEED_OUTPUT_MAX,
    },
    .target_speed = TARGET_SPEED_DEFAULT,
    .enable = 0,
};

/* 限幅函数 */
static float limit_f(float value, float max)
{
    if (value > max)  return max;
    if (value < -max) return -max;
    return value;
}

static int16 limit_i(int16 value, int16 max)
{
    if (value > max)  return max;
    if (value < -max) return -max;
    return value;
}

/*-----------------------------------------------------------
 * 控制模块初始化
 *-----------------------------------------------------------*/
void control_init(void)
{
    control_reset();
#if CONTROL_PARAM_FLASH_USE
    control_load_from_flash();  /* 尝试从 Flash 恢复上次保存的参数 */
#endif
}

/*-----------------------------------------------------------
 * PID 计算通用函数
 *-----------------------------------------------------------*/
static float pid_calculate(pid_t *pid, float error)
{
    float output;

    /* 积分 */
    pid->integral += error;
    pid->integral = limit_f(pid->integral, pid->integral_max);

    /* PID 计算 */
    output = pid->kp * error +
             pid->ki * pid->integral +
             pid->kd * (error - pid->last_error);

    /* 保存误差 */
    pid->last_error = error;

    /* 输出限幅 */
    output = limit_f(output, pid->output_max);

    return output;
}

/*-----------------------------------------------------------
 * 转向控制计算
 *-----------------------------------------------------------*/
/* BUG FIX 9（类型）: track_error 由 int8 改为 int16，与 track_get_error 返回类型对齐，防止截断 */
int16 control_steer(int16 track_error)
{
    float error = (float)track_error;
    float output = pid_calculate(&s_control.steer_pid, error);
    return (int16)output;
}

/*-----------------------------------------------------------
 * 速度控制计算
 *-----------------------------------------------------------*/
int16 control_speed(int16 current_speed)
{
    float error = (float)(s_control.target_speed - current_speed);
    float output = pid_calculate(&s_control.speed_pid, error);
    return (int16)output;
}

/*-----------------------------------------------------------
 * 综合控制输出
 *-----------------------------------------------------------*/
/* BUG FIX 9（类型）: track_error 由 int8 改为 int16，防止隐式截断 */
void control_run(int16 track_error, int16 speed_l, int16 speed_r,
                 int16 *out_l, int16 *out_r)
{
    if (!s_control.enable)
    {
        *out_l = 0;
        *out_r = 0;
        return;
    }

    /* 转向控制 */
    int16 steer = control_steer(track_error);

    /* 速度控制 (取平均速度) */
    int16 avg_speed = (speed_l + speed_r) / 2;
    int16 speed_out = control_speed(avg_speed);

    /* 差速输出 */
    *out_l = limit_i(speed_out - steer, MOTOR_PWM_MAX);
    *out_r = limit_i(speed_out + steer, MOTOR_PWM_MAX);
}

/*-----------------------------------------------------------
 * 设置目标速度
 *-----------------------------------------------------------*/
void control_set_target_speed(int16 speed)
{
    s_control.target_speed = limit_i(speed, TARGET_SPEED_MAX);
}

/*-----------------------------------------------------------
 * 获取目标速度
 *-----------------------------------------------------------*/
int16 control_get_target_speed(void)
{
    return s_control.target_speed;
}

/*-----------------------------------------------------------
 * 使能/禁用控制
 *-----------------------------------------------------------*/
void control_enable(uint8 enable)
{
    if (enable && !s_control.enable)
    {
        /* 从禁用到使能，重置状态 */
        control_reset();
    }
    s_control.enable = enable;
}

/*-----------------------------------------------------------
 * 获取控制使能状态
 *-----------------------------------------------------------*/
uint8 control_is_enabled(void)
{
    return s_control.enable;
}

/*-----------------------------------------------------------
 * 设置转向 PID 参数
 *-----------------------------------------------------------*/
void control_set_steer_pid(float kp, float ki, float kd)
{
    s_control.steer_pid.kp = kp;
    s_control.steer_pid.ki = ki;
    s_control.steer_pid.kd = kd;
}

/*-----------------------------------------------------------
 * 设置速度 PID 参数
 *-----------------------------------------------------------*/
void control_set_speed_pid(float kp, float ki, float kd)
{
    s_control.speed_pid.kp = kp;
    s_control.speed_pid.ki = ki;
    s_control.speed_pid.kd = kd;
}

/*-----------------------------------------------------------
 * 获取控制参数指针
 *-----------------------------------------------------------*/
control_param_t* control_get_param(void)
{
    return &s_control;
}

/*-----------------------------------------------------------
 * 重置 PID 积分和状态
 *-----------------------------------------------------------*/
void control_reset(void)
{
    s_control.steer_pid.integral = 0;
    s_control.steer_pid.last_error = 0;
    s_control.speed_pid.integral = 0;
    s_control.speed_pid.last_error = 0;
}

/*===========================================================================
 * Flash 参数保存/加载（掉电保存骨架）
 *
 * 存储布局（Flash 扇区 0，页 0）：
 *   [0]     : 魔数 FLASH_MAGIC（用于判断数据有效性）
 *   [1]     : 版本号（参数结构变更时+1，可用于迁移）
 *   [2~4]   : 转向 KP, KI, KD （float → uint32 联合体）
 *   [5~7]   : 速度 KP, KI, KD
 *   [8]     : 目标速度
 *   [9]     : 校验和（所有字段异或）
 *
 * 使用方法：
 *   - 保存：长按 KEY2 时调用 control_save_to_flash()
 *   - 加载：control_init() 中自动调用 control_load_from_flash()
 *   - 恢复默认：调用 control_reset_to_default() 后再 save
 *
 * ⚠ Flash 擦写寿命约 10 万次，不要在循环中频繁保存
 *===========================================================================*/

/** Flash 存储扇区/页配置（可按需修改） */
#define PARAM_FLASH_SECTOR      0
#define PARAM_FLASH_PAGE        0
#define PARAM_FLASH_MAGIC       0x50494400   /* "PID\0" 的 ASCII 编码，用于标识有效数据 */
#define PARAM_FLASH_VERSION     1            /* 参数结构版本，变更结构时+1 */

/** float 与 uint32 互转联合体 */
typedef union
{
    float    f;
    uint32   u;
} float_uint32_t;

/*-----------------------------------------------------------
 * 保存控制参数到 Flash
 *-----------------------------------------------------------*/
void control_save_to_flash(void)
{
    float_uint32_t conv;
    uint32 buf[10] = {0};
    uint32 checksum = 0;

    /* 填充数据 */
    buf[0] = PARAM_FLASH_MAGIC;
    buf[1] = PARAM_FLASH_VERSION;

    /* 转向 PID */
    conv.f = s_control.steer_pid.kp;  buf[2] = conv.u;
    conv.f = s_control.steer_pid.ki;  buf[3] = conv.u;
    conv.f = s_control.steer_pid.kd;  buf[4] = conv.u;

    /* 速度 PID */
    conv.f = s_control.speed_pid.kp;  buf[5] = conv.u;
    conv.f = s_control.speed_pid.ki;  buf[6] = conv.u;
    conv.f = s_control.speed_pid.kd;  buf[7] = conv.u;

    /* 目标速度 */
    buf[8] = (uint32)s_control.target_speed;

    /* 计算校验和（简单异或） */
    for (uint8 i = 0; i < 9; i++)
    {
        checksum ^= buf[i];
    }
    buf[9] = checksum;

    /* 写入 Flash（先擦后写） */
    flash_erase_page(PARAM_FLASH_SECTOR, PARAM_FLASH_PAGE);
    flash_write_page(PARAM_FLASH_SECTOR, PARAM_FLASH_PAGE, buf, 10);

    printf("[FLASH] Params saved.\r\n");
}

/*-----------------------------------------------------------
 * 从 Flash 加载控制参数
 *-----------------------------------------------------------*/
void control_load_from_flash(void)
{
    float_uint32_t conv;
    uint32 buf[10] = {0};
    uint32 checksum = 0;

    /* 从 Flash 读取 */
    flash_read_page(PARAM_FLASH_SECTOR, PARAM_FLASH_PAGE, buf, 10);

    /* 验证魔数 */
    if (buf[0] != PARAM_FLASH_MAGIC)
    {
        printf("[FLASH] No saved params, using defaults.\r\n");
        return;
    }

    /* 验证版本（可扩展：旧版本迁移逻辑） */
    if (buf[1] != PARAM_FLASH_VERSION)
    {
        printf("[FLASH] Version mismatch (got %d, expect %d), using defaults.\r\n",
               (int)buf[1], PARAM_FLASH_VERSION);
        return;
    }

    /* 验证校验和 */
    for (uint8 i = 0; i < 9; i++)
    {
        checksum ^= buf[i];
    }
    if (checksum != buf[9])
    {
        printf("[FLASH] Checksum error, using defaults.\r\n");
        return;
    }

    /* 恢复参数 */
    conv.u = buf[2]; s_control.steer_pid.kp = conv.f;
    conv.u = buf[3]; s_control.steer_pid.ki = conv.f;
    conv.u = buf[4]; s_control.steer_pid.kd = conv.f;

    conv.u = buf[5]; s_control.speed_pid.kp = conv.f;
    conv.u = buf[6]; s_control.speed_pid.ki = conv.f;
    conv.u = buf[7]; s_control.speed_pid.kd = conv.f;

    s_control.target_speed = (int16)buf[8];

    printf("[FLASH] Params loaded: Steer(%.1f,%.2f,%.1f) Speed(%.1f,%.2f,%.1f) Target=%d\r\n",
           s_control.steer_pid.kp, s_control.steer_pid.ki, s_control.steer_pid.kd,
           s_control.speed_pid.kp, s_control.speed_pid.ki, s_control.speed_pid.kd,
           s_control.target_speed);
}
