/**
  ******************************************************************************
  * @file    main.c
  * @brief   实验: STM32F103 定时器 (TIM3) 计数方式 + PWM 模式驱动呼吸灯
  *
  *          任务一 (计数方式):
  *            TIM3 每 1 ms 计数一次(ARR 溢出), 在更新中断里累计 2000 次 = 2 s,
  *            翻转 LED1 (PA1), LED 以 2 s 的周期亮-灭.
  *
  *          任务二 (PWM 模式):
  *            TIM3_CH3 (复用引脚 PB0, 接 LED2 绿色) 输出 1 kHz PWM,
  *            占空比每 1 ms 加/减 1 个计数点: 1 s 内 0 -> 100%, 再 1 s 内 100% -> 0,
  *            形成周期 2 s 的呼吸灯.
  *
  *          定时器相关代码全部在 Core/Src/tim_pwm.c, 采用寄存器级配置.
  ******************************************************************************
  */

#include "main.h"
#include "tim_pwm.h"

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

int main(void)
{
  /* 1) HAL 初始化: Flash 预取 + SysTick 1 ms 时基 + NVIC 分组 */
  HAL_Init();

  /* 2) 系统时钟: HSE 8 MHz x 9 = 72 MHz */
  SystemClock_Config();

  /* 3) GPIO 初始化 (LED1 用于计数方式闪烁) */
  MX_GPIO_Init();

  /* 4) TIM3 初始化: CH3(PB0) 输出 PWM + 1 ms 更新中断 */
  TIM3_PWM_Init();

  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* 呼吸灯和 2 s 闪烁都在 TIM3 更新中断里做, 主循环空转即可 */
  }
  /* USER CODE END WHILE */
}

/* USER CODE BEGIN 4 */

/**
  * @brief 72 MHz 系统时钟 (HSE 8 MHz x 9)
  * @note  APB1 = HCLK/2 = 36 MHz, APB2 = HCLK = 72 MHz
  *        因为 APB1 预分频系数 != 1, 所以 TIM3CLK = 2 x PCLK1 = 72 MHz
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  GPIO 初始化: LED1 (PA1) 推挽输出, 用于定时器计数方式的 2 s 亮-灭
  * @note   LED2 (PB0) 是 TIM3_CH3 的复用引脚, 在 TIM3_PWM_Init() 里配置成 AF_PP
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* LED1 = PA1 (红色), 高电平点亮 */
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin  = LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED1_GPIO_Port, &GPIO_InitStruct);
}

/**
  * @brief  错误处理
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
/* USER CODE END 4 */
