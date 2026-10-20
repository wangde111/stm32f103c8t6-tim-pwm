/**
  ******************************************************************************
  * @file    stm32f1xx_hal_msp.c
  * @brief   HAL 底层初始化 (MSP: MCU Support Package)
  *          这里只做最基础的外设时钟使能, 其余外设的 MSP 由各自的驱动回调完成。
  ******************************************************************************
  */

#include "main.h"

/**
  * @brief  全局 HAL 初始化回调, 由 HAL_Init() 自动调用
  * @note   使能 AFIO(外部中断需要) 与 PWR 时钟
  */
void HAL_MspInit(void)
{
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}
