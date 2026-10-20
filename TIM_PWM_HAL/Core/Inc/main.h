/**
  ******************************************************************************
  * @file    main.h
  * @brief   实验�?头文�? 引脚定义与公共声�?  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

/* ---------------------------------------------------------------------------
 * LED 引脚定义 (4 �?LED, 覆盖 GPIOA / GPIOB / GPIOC 三个端口)
 *   PA1  红色 LED  高电平点�? *   PB0  绿色 LED  高电平点�? *   PB1  蓝色 LED  高电平点�? *   PC13 板载 LED  低电平点�?(核心板出厂已焊好)
 * --------------------------------------------------------------------------- */
#define LED1_Pin        GPIO_PIN_1
#define LED1_GPIO_Port  GPIOA
#define LED2_Pin        GPIO_PIN_0
#define LED2_GPIO_Port  GPIOB
#define LED3_Pin        GPIO_PIN_1
#define LED3_GPIO_Port  GPIOB
#define LED4_Pin        GPIO_PIN_13
#define LED4_GPIO_Port  GPIOC

/* ---------------------------------------------------------------------------
 * 按键定义
 *
 *   ★★�?极性开�? 只改下面这一个数�? 其它代码都不用动 ★★�? *
 *     KEY_ACTIVE_HIGH = 0   低电平有�? 按下时引脚被拉到 GND, 内部上拉
 *                           适用: 外接按键(一脚接 PA0, 一脚接 GND)
 *                                 以及多数板载 KEY(按下接地)
 *
 *     KEY_ACTIVE_HIGH = 1   高电平有�? 按下时引脚被拉到 3.3V, 内部下拉
 *                           适用: 部分板载 KEY / WK_UP(按下�?3.3V)
 *
 *   怎么判断用哪�? 用万用表�?PA0 �?GND 的电�?
 *       不按�?3.3V、按下变 0V  -> �?0 (低电平有�?
 *       不按�?0V�? 按下�?3.3V -> �?1 (高电平有�?
 * --------------------------------------------------------------------------- */
#define KEY_ACTIVE_HIGH 0

/* 按键所用引�? PA0 (对应 EXTI0 外部中断�? */
#define KEY_Pin         GPIO_PIN_0
#define KEY_GPIO_Port   GPIOA
#define KEY_EXTI_IRQn   EXTI0_IRQn

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
