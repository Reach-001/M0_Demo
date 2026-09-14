/**
 * @file    isr.h
 * @brief   中断服务函数头文件 — 声明 isr.c 中所有 IRQ Handler
 * @author  Reach
 * @version v1.0
 *
 * @note    本文件仅供编译器链接使用，用户一般不需要直接 #include
 *          若需要在其他文件中手动使能/禁用某个 IRQ，可引用本头文件获取函数原型
 */

#ifndef _ISR_H_
#define _ISR_H_

#include "zf_common_headfile.h"

/* ==================== 定时器中断 ==================== */
void TIMA0_IRQHandler(void);
void TIMA1_IRQHandler(void);
void TIMG0_IRQHandler(void);
void TIMG6_IRQHandler(void);
void TIMG7_IRQHandler(void);
void TIMG8_IRQHandler(void);
void TIMG12_IRQHandler(void);

/* ==================== 串口中断 ==================== */
void UART0_IRQHandler(void);
void UART1_IRQHandler(void);
void UART2_IRQHandler(void);
void UART3_IRQHandler(void);

/* ==================== GPIO 外部中断 ==================== */
void GROUP1_IRQHandler(void);

#endif /* _ISR_H_ */
