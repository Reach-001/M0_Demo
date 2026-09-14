# MSPM0G3507 逐飞库快速入门模板

基于 TI MSPM0G3507 + 逐飞科技开源库的 GCC 工程模板，适用于智能车、机器人等嵌入式项目开发。

## 芯片规格

| 参数 | 数值 |
|------|------|
| 内核 | ARM Cortex-M0+ |
| 主频 | 80 MHz (SYSPLL) |
| Flash | 128 KB |
| SRAM | 32 KB |
| GPIO | GPIOA (A0-A31) + GPIOB (B0-B27) |

## 工程结构

```
├── user/
│   ├── config/
│   │   └── board_config.h      # 唯一参数入口，改引脚、周期、PID 都在这里
│   ├── bsp/                     # 板级驱动（LED、按键、电机、编码器、循迹等）
│   ├── app/                     # 应用层（任务调度、PID 控制）
│   └── main/
│       ├── main.c               # 初始化和主循环
│       └── isr.c                # 中断服务（一般不需要修改）
├── source/core/                 # 逐飞库源码（勿修改）
│   ├── zf_common/               # 公共模块
│   ├── zf_driver/               # 外设驱动
│   └── zf_device/               # 设备驱动
├── include/core/zf/             # 逐飞库头文件
├── linker/                      # 链接脚本 + SVD
├── ti_msp_dl_config.*           # SysConfig 生成的时钟配置
└── empty.syscfg                 # SysConfig 工程文件
```

推荐约定：

- 改参数只改 `user/config/board_config.h`
- 读写硬件放在 `user/bsp/`
- 业务流程放在 `user/app/`
- `main.c` 只做初始化和调用任务，不堆业务细节

## 接口命名与依赖规则

项目接口尽量保持与逐飞库相近的风格，但不要为了统一命名而修改
`source/core/zf_*` 或 `include/core/zf/` 中的逐飞库文件。

| 接口类型 | 固定格式 | 示例 |
|----------|----------|------|
| 初始化 | `模块_init` | `motor_init()` |
| 设置参数 | `模块_set_参数` | `motor_set_speed()` |
| 获取数据 | `模块_get_数据` | `encoder_get()` |
| 清除状态 | `模块_clear_对象` | `encoder_clear_all()` |
| 使能控制 | `模块_enable` | `control_enable()` |
| 判断状态 | `模块_is_状态` | `control_is_enabled()` |
| 枚举类型 | `模块_xxx_enum` | `motor_id_enum` |
| 结构体类型 | `模块_xxx_t` | `control_param_t` |

统一遵循以下规则：

1. 函数和变量使用小写下划线命名，例如 `motor_set_speed()`。
2. 每个公开函数都带模块名前缀，避免不同模块出现同名符号。
3. 配置宏使用全大写下划线命名，例如 `MOTOR_PWM_FREQ`。
4. 新硬件接口放在 `user/bsp/`，业务接口放在 `user/app/`。
5. 每个 `.c` 文件只包含自己实际使用的头文件，不使用统一聚合头文件。
6. 应用层通过 BSP 接口访问硬件，不直接调用 GPIO、PWM、ADC 等底层接口。
7. 逐飞库作为底层依赖保持原样；需要适配时，在 `user/bsp/` 中封装。

推荐依赖方向：

```text
user/main → user/app → user/bsp → 逐飞库 / TI DriverLib
```

不要产生反向依赖，例如不要让 `user/bsp/` 调用 `user/app/`。

## 快速开始

### 环境准备

1. **编译器**: ARM GCC (arm-none-eabi-gcc)
2. **IDE**: VS Code + EIDE 插件
3. **调试器**: CMSIS-DAP + PyOCD
   ```bash
   # 安装 PyOCD
   pip install pyocd
   
   # 安装 MSPM0 支持包
   pyocd pack install mspm0g3507
   ```
4. **配置工具**: TI SysConfig (可选，用于修改时钟)

### 编译 & 烧录

```bash
# VS Code 中
Ctrl+Shift+B → build        # 编译
Ctrl+Shift+B → flash        # 烧录（PyOCD）
F5                          # 调试（PyOCD）
```

EIDE 已配置为 PyOCD 烧录/调试，无需额外设置。烧录器选 CMSIS-DAP，目标芯片 `mspm0g3507`，频率 4MHz。

---

# 框架使用说明

## 程序启动流程

```
main()
├── system_init()    # 时钟、调试串口
├── bsp_init()       # LED、按键、电机、编码器、循迹传感器
├── app_init()       # 控制模块、任务定时器
├── self_test()      # 上电自检（LED闪烁 + 传感器打印）
└── while (1)        # 主循环轮询执行任务
```

`task_init()` 会启动一个 PIT 定时器。定时器中断只设置任务标志，真正的任务函数在 `main.c` 的主循环里执行。这样做的好处是：中断里不跑复杂逻辑，任务里可以安全调用普通函数和打印。

## 修改参数

所有常用参数集中在 `user/config/board_config.h`。

常见修改项：

```c
#define LED_PIN                 B22
#define KEY1_PIN                A0
#define MOTOR_L_PWM             PWM_TIM_A0_CH0_A8
#define TRACK_THRESHOLD         2000
#define TASK_PERIOD_MS          5
#define TASK_CONTROL_PERIOD     2
#define PID_STEER_KP            50.0f
```

任务周期的计算方式：

```
任务实际周期 = TASK_PERIOD_MS × TASK_xxx_PERIOD（毫秒）
```

例如 `TASK_PERIOD_MS=5, TASK_CONTROL_PERIOD=2` → 控制任务周期 = 5ms × 2 = 10ms。

## 当前内置任务说明

| 任务 | 函数 | 默认周期 | 作用 |
|------|------|----------|------|
| 循迹采样 | `task_track()` | 5ms | 读取循迹传感器 |
| 控制计算 | `task_control()` | 10ms | 编码器读取、PID、输出电机 |
| 显示打印 | `task_display()` | 300ms | 串口打印状态 |
| 按键处理 | `task_key()` | 20ms | 扫描按键、启动/停止 |

默认上电后不会直接运行电机。按 KEY1 启动，再按 KEY1 停止。

## 如何添加一个新任务

示例：添加一个 `uart` 串口处理任务，每 50ms 执行一次。

### Step 1. 在 board_config.h 添加周期

```c
#define TASK_UART_PERIOD        10      /* 10 × 5ms = 50ms */
```

### Step 2. 在 app_task.h 添加任务标志和函数声明

```c
typedef struct
{
    uint8 track;
    uint8 control;
    uint8 display;
    uint8 key;
    uint8 uart;        /* 新增 */
} task_flag_t;

void task_uart(void);
```

### Step 3. 在 app_task.c 的 task_timer_callback() 设置标志

```c
if (s_task_tick % TASK_UART_PERIOD == 0)
{
    g_task_flag.uart = 1;
}
```

### Step 4. 在 app_task.c 实现任务逻辑

```c
void task_uart(void)
{
    /* 这里写串口接收解析、状态上传等逻辑 */
}
```

### Step 5. 在 main.c 主循环执行任务

```c
if (g_task_flag.uart)
{
    g_task_flag.uart = 0;
    task_uart();
}
```

## 如何添加一个新 BSP 模块

1. 复制 `user/bsp/bsp_template.c` 和 `bsp_template.h`，重命名为 `bsp_xxx.c/h`
2. 在 `board_config.h` 添加相关引脚/参数宏定义
3. 在使用该模块的 `.c` 文件中添加 `#include "bsp_xxx.h"`
4. 在 `main.c` 的 `bsp_init()` 中调用 `xxx_init()`
5. 在对应任务中调用驱动函数

## 如何写一个新业务逻辑

推荐新建一个应用模块，例如 `user/app/app_user.c/h`。

典型接口：

```c
void app_user_init(void);
void app_user_task(void);
```

然后：

1. 在 `app_init()` 中调用 `app_user_init()`
2. 新增一个任务标志，例如 `user`
3. 在 `task_user()` 中调用 `app_user_task()`

业务模块可以调用 BSP 接口，例如：

```c
track_read(&data);
motor_set_speed_both(left, right);
bsp_key_get_event(BSP_KEY_1);
```

不要在业务模块里直接写一堆 GPIO/PWM 初始化。硬件细节尽量放在 BSP 层。

## 任务里应该写什么

适合写在任务里的内容：读取传感器、更新状态机、PID 计算、电机输出、按键事件处理、串口解析、周期性打印。

不建议写在任务里的内容：长时间 `while` 等待、很长的 `system_delay_ms()`、大量阻塞式打印、会卡住主循环的测试代码。如果必须等待外设响应，建议拆成"状态机"：本次任务发命令，下次任务检查结果。

## 常见问题

### 编译找不到函数或变量

检查对应 `.c` 文件是否在 `.eide/eide.yml` 的 `srcDirs` 覆盖目录下。用户代码建议放在 `user/main`、`user/bsp`、`user/app`。

### 新增任务不执行

依次检查：`board_config.h` 有没有定义 `TASK_xxx_PERIOD` → `task_flag_t` 有没有新增标志 → `task_timer_callback()` 有没有置位 → `main.c` 主循环有没有清标志并调用任务函数。

### 电机上电就动

检查 `motor_init()` 和 `task_control()`。框架默认初始化后停止电机，只有系统状态为 `SYS_STATE_RUNNING` 才输出控制量。

### 串口打印太多导致卡顿

降低 `TASK_DISPLAY_PERIOD` 的执行频率，或减少 `printf` 内容。串口打印属于慢操作，不适合高频执行。

### 串口输出乱码

常见原因：某个引脚初始化覆盖了外部晶振引脚（A3/A4/A5/A6）。MSPM0G3507 使用外部 32MHz 晶振作为 PLL 参考时钟，如果代码中把这些引脚重新配置为 GPIO 或编码器，时钟会崩溃导致波特率偏移。

## 推荐开发顺序

1. 先改 `board_config.h` 的 LED、按键、传感器、电机、编码器引脚
2. 编译并下载
3. 看 LED 自检和串口输出
4. 检查按键是否能启动/停止
5. 检查循迹 ADC 原始值
6. 检查编码器方向
7. 低速测试电机方向
8. 再调 PID 参数

---

# 逐飞库 API 详解

## 1. 系统初始化

每个程序必须首先调用：

```c
clock_init(SYSTEM_CLOCK_80M);  // 时钟初始化，必须第一个调用
debug_init();                   // 调试串口初始化（UART0, 115200）
```

**注意**：逐飞库在运行时动态初始化外设，无需在 SysConfig 中预配置（时钟除外）。

---

## 2. GPIO 通用输入输出

### 引脚命名

```c
A0, A1, A2 ... A31   // GPIOA 端口
B0, B1, B2 ... B27   // GPIOB 端口
```

### 初始化

```c
// gpio_init(引脚, 方向, 初始电平, 模式)

// 输出模式
gpio_init(B22, GPO, GPIO_LOW,  GPO_PUSH_PULL);   // 推挽输出，默认低
gpio_init(B22, GPO, GPIO_HIGH, GPO_OPEN_DRAIN);  // 开漏输出，默认高

// 输入模式
gpio_init(A0, GPI, GPIO_HIGH, GPI_PULL_UP);      // 上拉输入
gpio_init(A1, GPI, GPIO_LOW,  GPI_PULL_DOWN);    // 下拉输入
gpio_init(A2, GPI, GPIO_LOW,  GPI_FLOATING_IN);  // 浮空输入
gpio_init(A3, GPI, GPIO_LOW,  GPI_ANAOG_IN);     // 模拟输入（ADC用）
```

### 输出操作

```c
gpio_high(B22);              // 输出高电平
gpio_low(B22);               // 输出低电平
gpio_toggle_level(B22);      // 电平翻转
gpio_set_level(B22, 1);      // 设置电平（0 或 1）
```

### 输入操作

```c
uint8 level = gpio_get_level(A0);  // 读取引脚电平
```

### 完整引脚列表

| 端口 | 引脚范围 | 说明 |
|------|----------|------|
| GPIOA | A0 - A31 | 32 个引脚 |
| GPIOB | B0 - B27 | 28 个引脚 |

---

## 3. 延时函数

```c
system_delay_ms(100);   // 毫秒延时
system_delay_us(50);    // 微秒延时
```

---

## 4. UART 串口通信

### 资源概览

| 串口 | TX 引脚 | RX 引脚 |
|------|---------|---------|
| UART0 | A0, A10, A28, B0 | A1, A11, A31, B1 |
| UART1 | A8, A17, B4, B6 | A9, A18, B5, B7 |
| UART2 | A21, A23, B15, B17 | A22, A24, B16, B18 |
| UART3 | A14, A26, B2, B12 | A13, A25, B3, B13 |

### 初始化

```c
// uart_init(串口号, 波特率, TX引脚, RX引脚)
uart_init(UART_1, 115200, UART1_TX_A8, UART1_RX_A9);
uart_init(UART_2, 9600,   UART2_TX_B17, UART2_RX_B18);
```

### 发送数据

```c
uart_write_byte(UART_1, 0x55);                      // 发送单字节
uart_write_string(UART_1, "Hello World!\r\n");      // 发送字符串
uart_write_buffer(UART_1, data_buf, sizeof(buf));   // 发送数组
```

### 接收数据

```c
uint8 dat;

// 非阻塞查询（推荐）
if (uart_query_byte(UART_1, &dat))
{
    // 收到数据 dat
}

// 阻塞等待（有超时）
uart_read_byte(UART_1, &dat);
```

### 中断接收

```c
// 设置回调函数
void uart1_callback(uint32 state, void *param)
{
    if (state == UART_INTERRUPT_STATE_RX)
    {
        uint8 dat;
        uart_query_byte(UART_1, &dat);
        // 处理接收数据
    }
}

// 初始化时设置
uart_init(UART_1, 115200, UART1_TX_A8, UART1_RX_A9);
uart_set_callback(UART_1, uart1_callback, NULL);
uart_set_interrupt_config(UART_1, UART_INTERRUPT_CONFIG_RX_ENABLE);
```

---

## 5. PWM 脉冲宽度调制

### 资源概览

| 定时器 | 通道数 | 可用引脚 |
|--------|--------|----------|
| TIMA0 | CH0-CH3 | A0/A8/A21/B8/B14, A1/A3/A7/A9/A22/B9/B12/B20, A3/A7/A10/A15/B0/B4/B12/B17/B20, A4/A12/A17/A23/A25/A28/B2/B13/B24/B26 |
| TIMA1 | CH0-CH1 | A10/A15/A17/A28/B0/B2/B4/B17/B26, A11/A16/A18/A24/A31/B1/B3/B5/B18/B27 |
| TIMG0 | CH0-CH1 | A5/A12/A23/B10, A6/A13/A24/B11 |
| TIMG6 | CH0-CH1 | A5/A21/A29/B2/B6/B10/B26, A6/A22/A30/B3/B7/B11/B27 |
| TIMG7 | CH0-CH1 | A3/A17/A23/A26/A28/B15, A2/A4/A7/A18/A24/A27/A31/B16/B19 |
| TIMG8 | CH0-CH1 | A1/A3/A5/A7/A21/A23/A26/A29/B6/B10/B15/B21, A0/A2/A4/A6/A22/A27/A30/B7/B11/B16/B19/B22 |
| TIMG12 | CH0-CH1 | A10/A14/B13/B20, A25/A31/B14/B24 |

### 初始化与控制

```c
// pwm_init(通道, 频率Hz, 占空比‰)
pwm_init(PWM_TIM_A0_CH0_A8, 10000, 5000);   // 10kHz, 50%占空比
pwm_init(PWM_TIM_G8_CH0_B6, 20000, 2500);   // 20kHz, 25%占空比

// 动态调整占空比（0-10000 对应 0%-100%）
pwm_set_duty(PWM_TIM_A0_CH0_A8, 7500);      // 改为 75%
pwm_set_duty(PWM_TIM_A0_CH0_A8, 0);         // 停止输出
```

### 电机控制示例

```c
// 初始化 4 路 PWM（双 H 桥电机驱动）
pwm_init(PWM_TIM_A0_CH0_A8, 10000, 0);   // 电机1 正转
pwm_init(PWM_TIM_A0_CH1_A9, 10000, 0);   // 电机1 反转
pwm_init(PWM_TIM_A0_CH2_A10, 10000, 0);  // 电机2 正转
pwm_init(PWM_TIM_A0_CH3_A12, 10000, 0);  // 电机2 反转

// 电机1 正转 50%
pwm_set_duty(PWM_TIM_A0_CH0_A8, 5000);
pwm_set_duty(PWM_TIM_A0_CH1_A9, 0);
```

---

## 6. ADC 模数转换

### 资源概览

| ADC | 通道 | 引脚 |
|-----|------|------|
| ADC0 | CH0-CH7 | A27, A26, A25, A24, B25, B24, B20, A22 |
| ADC1 | CH0-CH7 | A15, A16, A17, A18, B17, B18, B19, A21 |

### 初始化与采集

```c
// adc_init(ADC引脚, 分辨率)
adc_init(ADC0_CH0_A27, ADC_12BIT);   // 12位分辨率
adc_init(ADC1_CH0_A15, ADC_10BIT);   // 10位分辨率

// 单次采集
uint16 value = adc_convert(ADC0_CH0_A27);

// 均值滤波采集（采 10 次取平均）
uint16 value = adc_mean_filter_convert(ADC0_CH0_A27, 10);
```

### 电压换算

```c
// 12位 ADC，参考电压 3.3V
float voltage = adc_convert(ADC0_CH0_A27) * 3.3f / 4096.0f;
```

---

## 7. PIT 定时中断

### 资源概览

| PIT 编号 | 对应定时器 |
|----------|-----------|
| PIT_TIM_A0 | TIMA0 |
| PIT_TIM_A1 | TIMA1 |
| PIT_TIM_G0 | TIMG0 |
| PIT_TIM_G6 | TIMG6 |
| PIT_TIM_G7 | TIMG7 |
| PIT_TIM_G8 | TIMG8 |
| PIT_TIM_G12 | TIMG12 |

### 初始化与使用

```c
// 回调函数
void pit_callback(uint32 flag, void *param)
{
    // 定时执行的代码
    gpio_toggle_level(B22);
}

// 毫秒级定时中断
pit_ms_init(PIT_TIM_A0, 10, pit_callback, NULL);   // 10ms 周期

// 微秒级定时中断
pit_us_init(PIT_TIM_G0, 500, pit_callback, NULL);  // 500us 周期

// 控制
pit_disable(PIT_TIM_A0);   // 禁止中断
pit_enable(PIT_TIM_A0);    // 使能中断
```

---

## 8. EXTI 外部中断

支持所有 GPIO 引脚作为外部中断源。

### 初始化与使用

```c
// 回调函数
void key_callback(uint32 event, void *param)
{
    // event: EXTI_TRIGGER_RISING / EXTI_TRIGGER_FALLING / EXTI_TRIGGER_BOTH
    if (event == EXTI_TRIGGER_FALLING)
    {
        // 下降沿触发
    }
}

// exti_init(引脚, 触发方式, 回调函数, 用户参数)
exti_init(A0, EXTI_TRIGGER_FALLING, key_callback, NULL);  // 下降沿
exti_init(A1, EXTI_TRIGGER_RISING, key_callback, NULL);   // 上升沿
exti_init(A2, EXTI_TRIGGER_BOTH, key_callback, NULL);     // 双边沿

// 控制
exti_disable(A0);   // 禁止中断
exti_enable(A0);    // 使能中断
```

---

## 9. 编码器接口

### 资源概览（正交编码器仅 TIMG8 支持）

| 定时器 | CH1 引脚 | CH2 引脚 |
|--------|----------|----------|
| TIMG8 | A1/A3/A5/A7/A21/A23/A26/A29/B6/B10/B15/B21 | A0/A2/A4/A6/A22/A27/A30/B7/B11/B16/B19/B22 |

> 注意：A5/A6 被外部 32MHz 晶振占用，选编码器引脚时需避开。

其他定时器(TIMA0/TIMA1/TIMG0/TIMG6/TIMG7/TIMG12)支持方向编码器模式。

### 正交编码器（AB 相）

```c
// encoder_quad_init(定时器, CH1引脚, CH2引脚)
encoder_quad_init(TIM_G8, TIMG8_ENCODER1_CH1_A7, TIMG8_ENCODER1_CH2_A2);

// 读取计数值（有符号，正负表示方向）
int16 count = encoder_get_count(TIM_G8);

// 清零
encoder_clear_count(TIM_G8);
```

### 方向编码器（脉冲 + 方向）

```c
// encoder_dir_init(定时器, 脉冲引脚, 方向引脚)
encoder_dir_init(TIM_A0, TIMA0_ENCODER1_CH1_A8, A9);

int16 count = encoder_get_count(TIM_A0);
encoder_clear_count(TIM_A0);
```

---

## 10. SPI 通信

### 资源概览

| SPI | SCK 引脚 | MOSI 引脚 | MISO 引脚 | CS 引脚 |
|-----|----------|-----------|-----------|---------|
| SPI0 | A6/A11/A12/B18 | A5/A9/A14/B17 | A4/A10/A13/B19 | A2/A8/B25 |
| SPI1 | A17/B9/B16/B23 | A18/B8/B15/B22 | A16/B7/B14/B21 | A2/A26/B6/B20 |

### 初始化

```c
// spi_init(SPI号, 模式, 波特率, SCK, MOSI, MISO, CS)
spi_init(SPI_0, SPI_MODE0, 1000000, 
         SPI0_SCK_A6, SPI0_MOSI_A5, SPI0_MISO_A4, SPI0_CS_A2);

// 不使用硬件 MISO 或 CS
spi_init(SPI_0, SPI_MODE0, 1000000, 
         SPI0_SCK_A6, SPI0_MOSI_A5, SPI_MISO_NULL, SPI_CS_NULL);
```

### SPI 模式

| 模式 | CPOL | CPHA | 说明 |
|------|------|------|------|
| SPI_MODE0 | 0 | 0 | 空闲低电平，第一边沿采样 |
| SPI_MODE1 | 0 | 1 | 空闲低电平，第二边沿采样 |
| SPI_MODE2 | 1 | 0 | 空闲高电平，第一边沿采样 |
| SPI_MODE3 | 1 | 1 | 空闲高电平，第二边沿采样 |

### 数据传输

```c
// 写
spi_write_8bit(SPI_0, 0xAA);
spi_write_8bit_array(SPI_0, data, len);
spi_write_16bit(SPI_0, 0x1234);

// 读
uint8 dat = spi_read_8bit(SPI_0);
spi_read_8bit_array(SPI_0, buffer, len);

// 寄存器操作
spi_write_8bit_register(SPI_0, 0x20, 0x01);
uint8 val = spi_read_8bit_register(SPI_0, 0x20);

// 全双工传输
spi_transfer_8bit(SPI_0, tx_buf, rx_buf, len);
```

---

## 11. 软件 IIC

软件模拟 IIC，可使用任意 GPIO 引脚。

### 初始化

```c
soft_iic_info_struct iic_dev;

// soft_iic_init(结构体, 7位设备地址, 延时, SCL引脚, SDA引脚)
soft_iic_init(&iic_dev, 0x68, 100, A6, A7);   // MPU6050 地址 0x68
soft_iic_init(&iic_dev, 0x50, 100, B10, B11); // EEPROM 地址 0x50
```

### 数据传输

```c
// 写寄存器
soft_iic_write_8bit_register(&iic_dev, 0x6B, 0x00);        // 单字节
soft_iic_write_8bit_registers(&iic_dev, 0x00, data, len);  // 多字节

// 读寄存器
uint8 val = soft_iic_read_8bit_register(&iic_dev, 0x75);   // 单字节
soft_iic_read_8bit_registers(&iic_dev, 0x3B, buffer, 14);  // 多字节

// 16位寄存器地址
soft_iic_write_16bit_register(&iic_dev, 0x0000, 0x1234);
uint16 val = soft_iic_read_16bit_register(&iic_dev, 0x0000);

// SCCB 协议（摄像头）
soft_iic_sccb_write_register(&iic_dev, 0x12, 0x80);
uint8 val = soft_iic_sccb_read_register(&iic_dev, 0x0A);
```

---

## 12. Flash 存储

用于保存参数、配置等掉电不丢失的数据。

### 存储规格

| 参数 | 数值 |
|------|------|
| 基地址 | 0x00016000 |
| 扇区数 | 6 个 |
| 每扇区页数 | 2 页 |
| 页大小 | 1024 字节 |
| 擦除单位 | 页 |

### 直接读写

```c
uint32 write_data[4] = {100, 200, 300, 400};
uint32 read_data[4];

// 擦除（写入前必须擦除）
flash_erase_page(0, 0);   // 扇区0，页0

// 写入
flash_write_page(0, 0, write_data, 4);

// 读取
flash_read_page(0, 0, read_data, 4);
```

### 使用缓冲区

```c
// 读取到缓冲区
flash_read_page_to_buffer(0, 0);

// 修改缓冲区数据
flash_union_buffer[0].float_type = 3.14f;
flash_union_buffer[1].int32_type = -100;
flash_union_buffer[2].uint16_type = 1000;

// 写回 Flash
flash_erase_page(0, 0);
flash_write_page_from_buffer(0, 0);

// 清空缓冲区
flash_buffer_clear();
```

---

## 13. 调试打印

`debug_init()` 初始化后，可使用标准 `printf`：

```c
printf("System Clock: %d Hz\r\n", system_clock);
printf("ADC Value: %d\r\n", adc_value);
printf("Float: %.2f\r\n", 3.14159f);
```

---

# 中断配置说明

所有中断服务函数已在 `isr.c` 中实现，采用回调机制：

| 中断源 | 中断向量 | 回调设置方式 |
|--------|----------|--------------|
| 定时器 | TIMx_IRQHandler | `pit_xx_init()` 时传入回调 |
| 串口 | UARTx_IRQHandler | `uart_set_callback()` |
| 外部中断 | GROUP1_IRQHandler | `exti_init()` 时传入回调 |

---

# SysConfig 与逐飞库的关系

| 配置项 | 配置方式 | 说明 |
|--------|----------|------|
| 时钟树 | SysConfig | HFXT/LFXT/SYSPLL 配置 |
| GPIO | 逐飞库 | `gpio_init()` 运行时配置 |
| UART | 逐飞库 | `uart_init()` 运行时配置 |
| PWM | 逐飞库 | `pwm_init()` 运行时配置 |
| ADC | 逐飞库 | `adc_init()` 运行时配置 |
| SPI | 逐飞库 | `spi_init()` 运行时配置 |
| 定时器 | 逐飞库 | `pit_xx_init()` 运行时配置 |

**核心原则**：SysConfig 只管时钟，逐飞库管所有外设。

---

# 许可证

- 逐飞库：GPL-3.0
- TI DriverLib：BSD-3-Clause

---

# 参考资料

- [MSPM0G3507 数据手册](https://www.ti.com/product/MSPM0G3507)
- [逐飞科技官方淘宝店](https://seekfree.taobao.com/)
- [TI MSPM0 SDK](https://www.ti.com/tool/MSPM0-SDK)
