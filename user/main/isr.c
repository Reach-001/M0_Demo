/*
 * @file    isr.c
 * @brief   中断服务函数 — 将硬件 IRQ 向量路由到逐飞库回调机制
 * @author  Reach
 * @version v1.0
 *
 * ============================================================
 * 【本文件说明】
 * ============================================================
 * MSPM0G3507 的中断向量表由编译器链接到固定符号名（如 TIMA0_IRQHandler）。
 * 逐飞库内部维护了回调函数数组，本文件只负责在 IRQ 中转发调用，
 * 用户不需要直接修改中断函数内部逻辑。
 *
 * 【如何使用定时器中断（PIT）】
 *   1. 调用 pit_ms_init(PIT_TIM_A1, 5, my_callback, NULL)
 *      → 库内部将 my_callback 存入 pit_callback_list[1]
 *   2. TIMA1_IRQHandler 触发时自动调用 my_callback
 *   ⚠ 回调函数在中断上下文中执行，只能做置位标志等轻量操作
 *
 * 【如何使用 GPIO 外部中断（EXTI）】
 *   1. 调用 exti_init(pin, EXTI_TRIGGER_RISING, my_exti_cb, NULL)
 *   2. GROUP1_IRQHandler 会自动分发到对应引脚的回调
 *
 * 【如何使用串口接收中断】
 *   1. 调用 uart_rx_interrupt_init(UART_1, my_uart_rx_cb, NULL)
 *   2. UART1_IRQHandler 收到数据时调用 my_uart_rx_cb
 *
 * ============================================================
 * 【新增中断（以 SPI0 为例）】
 * ============================================================
 *   1. 在本文件末尾添加：
 *      void SPI0_IRQHandler(void)
 *      {
 *          spi_callback_list[0](DL_SPI_getPendingInterrupt(SPI0),
 *                               spi_callback_ptr_list[0]);
 *      }
 *   2. 在逐飞库 zf_driver_spi.c 中注册回调即可
 * ============================================================
 */

#include "isr.h"

/* ==================== 定时器中断（PIT 周期中断） ==================== */

/** @brief TIMA0 中断 - 对应 pit_index 0 */
void TIMA0_IRQHandler(void)
{
    pit_callback_list[0](0, pit_callback_ptr_list[0]);
}

/** @brief TIMA1 中断 - 对应 pit_index 1 */
void TIMA1_IRQHandler(void)
{
    pit_callback_list[1](0, pit_callback_ptr_list[1]);
}

/** @brief TIMG0 中断 - 对应 pit_index 2 */
void TIMG0_IRQHandler(void)
{
    pit_callback_list[2](0, pit_callback_ptr_list[2]);
}

/** @brief TIMG6 中断 - 对应 pit_index 3 */
void TIMG6_IRQHandler(void)
{
    pit_callback_list[3](0, pit_callback_ptr_list[3]);
}

/** @brief TIMG7 中断 - 对应 pit_index 4 */
void TIMG7_IRQHandler(void)
{
    pit_callback_list[4](0, pit_callback_ptr_list[4]);
}

/** @brief TIMG8 中断 - 对应 pit_index 5 */
void TIMG8_IRQHandler(void)
{
    pit_callback_list[5](0, pit_callback_ptr_list[5]);
}

/** @brief TIMG12 中断 - 对应 pit_index 6 */
void TIMG12_IRQHandler(void)
{
    pit_callback_list[6](0, pit_callback_ptr_list[6]);
}

/* ==================== 串口中断（UART） ==================== */

/** @brief UART0 中断 - 发送/接收事件分发 */
void UART0_IRQHandler(void)
{
    switch (DL_UART_getPendingInterrupt(UART0))
    {
        case DL_UART_IIDX_TX:
            uart_callback_list[0](UART_INTERRUPT_STATE_TX, uart_callback_ptr_list[0]);
            break;
        case DL_UART_IIDX_RX:
            uart_callback_list[0](UART_INTERRUPT_STATE_RX, uart_callback_ptr_list[0]);
#if DEBUG_UART_USE_INTERRUPT
            debug_interrupr_handler();
#endif
            break;
        default: break;
    }
    DL_UART_clearInterruptStatus(UART0, UART0->CPU_INT.RIS);
}

/** @brief UART1 中断 - 发送/接收事件分发 */
void UART1_IRQHandler(void)
{
    switch (DL_UART_getPendingInterrupt(UART1))
    {
        case DL_UART_IIDX_TX:
            uart_callback_list[1](UART_INTERRUPT_STATE_TX, uart_callback_ptr_list[1]);
            break;
        case DL_UART_IIDX_RX:
            uart_callback_list[1](UART_INTERRUPT_STATE_RX, uart_callback_ptr_list[1]);
            wireless_module_uart_handler(); /* 无线串口模块回调 */
            break;
        default: break;
    }
    DL_UART_clearInterruptStatus(UART1, UART1->CPU_INT.RIS);
}

/** @brief UART2 中断 - 发送/接收事件分发 */
void UART2_IRQHandler(void)
{
    switch (DL_UART_getPendingInterrupt(UART2))
    {
        case DL_UART_IIDX_TX:
            uart_callback_list[2](UART_INTERRUPT_STATE_TX, uart_callback_ptr_list[2]);
            break;
        case DL_UART_IIDX_RX:
            uart_callback_list[2](UART_INTERRUPT_STATE_RX, uart_callback_ptr_list[2]);
            break;
        default: break;
    }
    DL_UART_clearInterruptStatus(UART2, UART2->CPU_INT.RIS);
}

/** @brief UART3 中断 - 发送/接收事件分发 */
void UART3_IRQHandler(void)
{
    switch (DL_UART_getPendingInterrupt(UART3))
    {
        case DL_UART_IIDX_TX:
            uart_callback_list[3](UART_INTERRUPT_STATE_TX, uart_callback_ptr_list[3]);
            break;
        case DL_UART_IIDX_RX:
            uart_callback_list[3](UART_INTERRUPT_STATE_RX, uart_callback_ptr_list[3]);
            break;
        default: break;
    }
    DL_UART_clearInterruptStatus(UART3, UART3->CPU_INT.RIS);
}

/* ==================== GPIO 外部中断 ==================== */

/**
 * @brief GROUP1 中断 - GPIO EXTI 事件分发
 *
 * MSPM0G3507 的所有 GPIO 外部中断共用 GROUP1 向量，
 * 驱动层通过 CPU_INT.IIDX 寄存器区分具体引脚，
 * 再查询 POLARITY 寄存器获取边沿方向后调用用户回调。
 */
void GROUP1_IRQHandler(void)
{
    uint8  exti_index = 0;
    uint8  exti_event = 0;
    uint32 register_temp;

    /* 检查 GPIOA */
    register_temp = gpio_group[0]->CPU_INT.IIDX;
    if (register_temp)
    {
        exti_index = register_temp - 1;
        exti_event = (exti_index <= 15)
            ? (gpio_group[0]->POLARITY15_0  >> ((exti_index % 16) * 2)) & 0x03
            : (gpio_group[0]->POLARITY31_16 >> ((exti_index % 16) * 2)) & 0x03;
        exti_callback_list[exti_index](exti_event, exti_callback_ptr_list[exti_index]);
    }
    else
    {
        /* 检查 GPIOB */
        register_temp = gpio_group[1]->CPU_INT.IIDX;
        if (register_temp)
        {
            exti_index = register_temp - 1;
            exti_event = (exti_index <= 15)
                ? (gpio_group[1]->POLARITY15_0  >> ((exti_index % 16) * 2)) & 0x03
                : (gpio_group[1]->POLARITY31_16 >> ((exti_index % 16) * 2)) & 0x03;
            exti_callback_list[exti_index](exti_event, exti_callback_ptr_list[exti_index]);
        }
    }
}
