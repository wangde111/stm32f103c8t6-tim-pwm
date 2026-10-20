/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   中断服务函数
  ******************************************************************************
  */

#include "main.h"
#include "stm32f1xx_it.h"
#include "tim_pwm.h"

/* Cortex-M3 内核异常 --------------------------------------------------------*/

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
  while (1)
  {
  }
}

void MemManage_Handler(void)
{
  while (1)
  {
  }
}

void BusFault_Handler(void)
{
  while (1)
  {
  }
}

void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

/**
  * @brief  SysTick 中断：每 1 ms 一次，为 HAL_Delay()/HAL_GetTick() 提供时基
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/* 片内外设中断 --------------------------------------------------------------*/

/**
  * @brief  TIM3 全局中断 (每 1 ms 一次更新中断)
  * @note   寄存器级配置, 没有用 HAL_TIM, 所以这里自己清标志位
  *         STM32F1 的标志位是"写 0 清除": 写 ~UIF 即可只清掉 UIF
  */
void TIM3_IRQHandler(void)
{
  if ((TIM3->SR & TIM_SR_UIF) != 0U)
  {
    TIM3->SR = (uint16_t)~TIM_SR_UIF;      /* 写 0 清除更新标志 */
    TIM3_App_Tick();                       /* 1 ms 软件时基: 呼吸灯 + 2 s 闪烁 */
  }

#if (SIM_PWM_MIRROR == 1)
  if ((TIM3->SR & TIM_SR_CC3IF) != 0U)
  {
    TIM3->SR = (uint16_t)~TIM_SR_CC3IF;
    TIM3_CC3_Conv();
  }
#endif
}
