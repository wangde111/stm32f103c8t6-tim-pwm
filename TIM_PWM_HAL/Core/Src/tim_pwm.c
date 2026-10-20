/**
  ******************************************************************************
  * @file    tim_pwm.c
  * @brief   TIM3 (寄存器级) 定时器驱动: 计数方式 + PWM 模式
  *
  *          1) 计数方式: TIM3 每 1 ms 溢出一次, 在更新中断里累计 2000 次(2 s),
  *                       翻转 LED1 (PA1), 实现"定时器计数方式控制 LED 周期亮灭"
  *          2) PWM 模式 : TIM3_CH3 (PB0) 输出 1 kHz PWM, 占空比每 1 ms 加/减 1 个
  *                       计数点, 1 s 内 0 -> 100%, 再 1 s 内 100% -> 0,
  *                       形成周期 2 s 的呼吸灯
  ******************************************************************************
  */

#include "tim_pwm.h"

/* 呼吸灯状态 */
static volatile uint16_t s_duty      = 0U;   /* 当前占空比 0 ~ TIM3_DUTY_MAX */
static volatile uint8_t  s_breath_up = 1U;   /* 1 = 渐亮, 0 = 渐灭 */

/* 计数方式: 2 s 闪烁状态 */
static volatile uint16_t s_blink_tick  = 0U; /* 1 ms 计数 */
static volatile uint8_t  s_blink_state = 0U; /* 0 = 灭, 1 = 亮 */

/* 仿真观察用: 把 PWM 波形用软件镜像到 PC13 (开关在 tim_pwm.h 里) */

/**
  * @brief  TIM3 初始化: CH3 (PB0) 输出 PWM, 并打开 1 ms 更新中断
  */
void TIM3_PWM_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* 1) 外设时钟: GPIOB + TIM3 */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_TIM3_CLK_ENABLE();

  /* 2) PB0 复用成 TIM3_CH3: AF 推挽输出, 50 MHz */
  GPIO_InitStruct.Pin   = GPIO_PIN_0;
  GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* 3) 计数器基本配置
   *    TIM3CLK = 2 x PCLK1 = 72 MHz (因为 APB1 预分频系数 != 1)
   *    PSC = 71  -> 计数频率 = 72MHz / 72 = 1 MHz
   *    ARR = 999 -> 计数到 999 溢出, 周期 = 1000 / 1MHz = 1 ms
   *               (这个 1 ms 同时就是 PWM 的载波周期, 即 PWM 频率 1 kHz)
   */
  TIM3->PSC = TIM3_PSC;
  TIM3->ARR = TIM3_ARR;
  TIM3->CR1 = TIM3_CR1_ARPE;      /* 预装载: 改 ARR/CCR 不影响当前这一周期 */

  /* 4) 通道 3 输出比较: PWM 模式 1
   *    PWM1 规则: CNT < CCR3 时输出有效电平(高), CNT >= CCR3 时输出低
   *    所以占空比 = CCR3 / (ARR + 1) = CCR3 / 1000
   */
  TIM3->CCMR2 &= ~TIM3_CCMR2_OC3M;    /* 先把模式位清零 */
  TIM3->CCMR2 |=  TIM3_CCMR2_PWM1;    /* 模式 1 */
  TIM3->CCMR2 |=  TIM3_CCMR2_OC3PE;   /* CCR3 预装载, 改占空比时输出无毛刺 */

  /* 5) 通道 3 输出使能, 初始占空比 0 */
  TIM3->CCR3  = 0U;
  TIM3->CCER |=  TIM3_CCER_CC3E;

  /* 6) 更新中断: 每 1 ms 一次, 作为"呼吸"和"2 s 闪烁"的软件时基 */
  TIM3->DIER |= TIM3_DIER_UIE;
  HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);

#if (SIM_PWM_MIRROR == 1)
  /* 仿真镜像用: 通道 3 比较中断 + PC13 普通推挽输出 */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitStruct.Pin   = GPIO_PIN_13;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  TIM3->DIER |= TIM3_DIER_CC3IE;
#endif

  /* 7) 启动计数器 */
  TIM3->CR1 |= TIM3_CR1_CEN;
}

/**
  * @brief  设置 PWM 占空比 (0 ~ TIM3_DUTY_MAX)
  * @note   直接改 CCR3 即可, 因为开了预装载, 新值下一个周期生效
  */
void TIM3_PWM_SetDuty(uint16_t duty)
{
  if (duty > TIM3_DUTY_MAX)
  {
    duty = TIM3_DUTY_MAX;
  }
  s_duty = duty;
  TIM3->CCR3 = duty;
}

/**
  * @brief  读取当前占空比
  */
uint16_t TIM3_PWM_GetDuty(void)
{
  return s_duty;
}

/**
  * @brief  TIM3 更新中断里调用的 1 ms 任务
  */
void TIM3_App_Tick(void)
{
  /* ---------- 1) 呼吸灯: 占空比 0 -> 100% (1 s), 再 100% -> 0 (1 s) ---------- */
  if (s_breath_up != 0U)
  {
    s_duty += BREATH_STEP;
    if (s_duty >= TIM3_DUTY_MAX)
    {
      s_duty = TIM3_DUTY_MAX;
      s_breath_up = 0U;          /* 到顶了, 开始渐灭 */
    }
  }
  else
  {
    if (s_duty <= BREATH_STEP)
    {
      s_duty = 0U;
      s_breath_up = 1U;          /* 到底了, 开始渐亮 */
    }
    else
    {
      s_duty -= BREATH_STEP;
    }
  }
  TIM3->CCR3 = s_duty;           /* 预装载, 下一个 PWM 周期生效 */

  /* ---------- 2) 计数方式: 每 2 s 翻转一次 LED1 (PA1) ---------- */
  s_blink_tick++;
  if (s_blink_tick >= BLINK_TICKS)          /* 2000 x 1 ms = 2 s */
  {
    s_blink_tick  = 0U;
    s_blink_state ^= 1U;
    if (s_blink_state != 0U)
    {
      HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);   /* 亮 2 s */
    }
    else
    {
      HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET); /* 灭 2 s */
    }
  }

#if (SIM_PWM_MIRROR == 1)
  /* 仿真镜像: 更新事件(周期结束)时 PWM 变高, 比较匹配时 PWM 变低 */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
#endif
}

/**
  * @brief  TIM3 通道 3 比较中断 (仅仿真镜像时使用)
  */
#if (SIM_PWM_MIRROR == 1)
void TIM3_CC3_Conv(void)
{
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}
#endif
