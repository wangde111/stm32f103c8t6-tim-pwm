/**
  ******************************************************************************
  * @file    tim_pwm.h
  * @brief   STM32F103 定时器 TIM3 驱动 (寄存器级) —— 计数方式 + PWM 模式
  *
  *          TIM3_CH3 的复用引脚是 PB0 (AF 推挽输出), 该引脚上接有 LED2 (绿色)
  *          TIM3 挂在 APB1 上, 本工程 APB1 预分频为 2, 所以 TIM3CLK = 2 x PCLK1 = 72 MHz
  *
  *          时基安排:
  *            PSC = 71   -> 计数频率 72MHz / 72 = 1 MHz
  *            ARR = 999  -> 溢出周期 1000 / 1MHz = 1 ms
  *            PWM 载波 = 1 kHz (与更新中断同频), 每 1 ms 改一次占空比
  ******************************************************************************
  */

#ifndef __TIM_PWM_H
#define __TIM_PWM_H

#include "main.h"

/* ---------------------- 定时器参数 ---------------------- */
#define TIM3_PSC        71U      /* 预分频: 72MHz / (PSC+1)  = 1 MHz 计数频率 */
#define TIM3_ARR        999U     /* 自动重装: 1MHz / (ARR+1) = 1 kHz, 溢出周期 1 ms */
#define TIM3_DUTY_MAX   1000U    /* 占空比满量程: CCR3 = 1000 时输出 100% */

/* ---------------------- 应用参数 ---------------------- */
#define BREATH_STEP     1U       /* 每 1 ms 占空比变化的计数点 (1 ms 改 0.1%) */
#define BLINK_TICKS     2000U    /* 计数 2000 次 = 2 s */

/* ---------------------- 仿真观察开关 ----------------------
   Keil 仿真器里若看不到 TIM3_CH3 在 PB0 上的 PWM 波形, 把这个改成 1,
   程序会用定时器中断把同样的波形"镜像"到 PC13 上, 供逻辑分析仪观察. */
#define SIM_PWM_MIRROR  0

/* ---------------------- TIM3 寄存器位定义 ----------------------
   说明 1: CMSIS 器件头文件(stm32f103xb.h)里已经带了 TIM_CCMR2_OC3M 等宏,
           但它们是按 TIM1(高级定时器) 的排布定义的, 对 TIM2~TIM5 不完全通用,
           所以这里自己按 RM0008 的通用定时器定义一份, 避免看错位。
   说明 2: STM32F1 的 CCMR1 = 通道1(低字节) + 通道2(高字节),
           CCMR2 = 通道3(低字节) + 通道4(高字节);
           CCER 每个通道占 4 位 (E / P / NE / NP), 所以 CC3E = bit8。
           参考: ST 标准外设库里 TIM_OCMode_PWM1 = 0x60, TIM_CCER_CC3E = 0x0100。 */
#define TIM3_CR1_CEN     (1UL << 0)    /* CR1.0   计数器使能 */
#define TIM3_CR1_ARPE    (1UL << 7)    /* CR1.7   自动重装预装载 */
#define TIM3_CCMR2_OC3M  (7UL << 4)    /* CCMR2.6~4  通道3 输出比较模式 */
#define TIM3_CCMR2_PWM1  (6UL << 4)    /* 通道3 = PWM 模式 1 (110b) */
#define TIM3_CCMR2_OC3PE (1UL << 3)    /* CCMR2.3   通道3 预装载使能 */
#define TIM3_CCER_CC3E   (1UL << 8)    /* CCER.8    通道3 输出使能 */
#define TIM3_DIER_UIE    (1UL << 0)    /* DIER.0    更新中断使能 */
#define TIM3_DIER_CC3IE  (1UL << 3)    /* DIER.3    通道3 捕获/比较中断使能 */
#define TIM3_SR_UIF      (1UL << 0)    /* SR.0      更新标志 (写 0 清除) */
#define TIM3_SR_CC3IF    (1UL << 3)    /* SR.3      通道3 捕获/比较标志 (写 0 清除) */

void     TIM3_PWM_Init(void);
void     TIM3_PWM_SetDuty(uint16_t duty);
uint16_t TIM3_PWM_GetDuty(void);
void     TIM3_App_Tick(void);        /* 在 TIM3 更新中断里调用, 每 1 ms 一次 */
#if (SIM_PWM_MIRROR == 1)
void     TIM3_CC3_Conv(void);        /* 仿真镜像用: 比较匹配时把 PC13 拉低 */
#endif

#endif /* __TIM_PWM_H */
