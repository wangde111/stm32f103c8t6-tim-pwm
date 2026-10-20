/**
 ***************************************************************************************************
 * 实验名称: 实验四  STM32F103 定时器 (TIM3) 计数方式 + PWM 模式驱动呼吸灯
 * 实验平台: STM32F103C8T6 最小系统核心板 + 面板板 (LED1=PA1 红, LED2=PB0 绿, LED3=PB1 蓝, PC13 板载)
 * 开发环境: Keil MDK 5.36 (ARMCC / AC5) + STM32F1xx HAL 库, 定时器部分寄存器级配置
 *
 * 实验目标: 1) 掌握通用定时器 TIM2~TIM5 的计数方式, 用定时器计数控制 LED 周期亮灭;
 *           2) 掌握 PWM 模式的配置方法(PSC/ARR/CCR/OCM/CCER), 产生可调占空比的 PWM;
 *           3) 用占空比渐变实现 LED 呼吸灯, 并用 Keil 逻辑分析仪观察 PWM 波形。
 *
 * 一、实验要求与实现结果
 *
 *   要求: (1) 用 TIM2~TIM5 某通道引脚接 LED, 定时器计数方式, LED 以 2s 周期亮-灭;
 *         (2) 定时器 PWM 模式, LED 呼吸灯渐亮渐灭, 周期 1~2 秒, Keil 虚拟示波器观察波形。
 *   实现: (1) TIM3 每 1ms 计数一次(ARR 溢出), 更新中断里累计 2000 次 = 2s, 翻转 LED1(PA1);
 *         (2) TIM3_CH3(PB0, LED2 绿) 输出 1kHz PWM, 占空比每 1ms 变 0.1%,
 *             1s 内 0->100%, 再 1s 内 100%->0, 呼吸周期 2s;
 *         (3) 仿真时 Logic Analyzer 观察 TIM3_CH3 / P1.0 / P0.1 三个信号。
 *
 * 二、硬件连接
 *
 *   PB0  --- 220欧 --- |>| --- GND    绿色 LED (PWM 输出, 高电平点亮, TIM3_CH3 复用引脚)
 *   PA1  --- 220欧 --- |>| --- GND    红色 LED (定时器计数方式, 高电平点亮)
 *   GND  --------------- 核心板 GND (必须共地)
 *
 * 三、关键参数与计算
 *
 *   系统时钟: HSE 8MHz x 9 = 72MHz, APB1 = HCLK/2 = 36MHz
 *   定时器时钟: APB1 预分频系数 != 1  =>  TIM3CLK = 2 x PCLK1 = 72MHz
 *   PSC = 71   -> 计数频率 = 72MHz / 72 = 1MHz
 *   ARR = 999  -> 溢出周期 = 1000 / 1MHz = 1ms  =>  PWM 载波 1kHz, 更新中断 1kHz
 *   占空比 = CCR3 / (ARR + 1) = CCR3 / 1000
 *
 * 四、STM32F1 寄存器位域提醒 (最容易写错的地方)
 *
 *   CCMR1: 低字节=通道1, 高字节=通道2        OC1M=bit6~4, OC1PE=bit3
 *   CCMR2: 低字节=通道3, 高字节=通道4        OC3M=bit6~4, OC3PE=bit3 (不是 bit12~14!)
 *   CCER : 每通道 4 位(E/P/NE/NP)            CC3E=bit8, CC3P=bit9, CC4E=bit12
 *   DIER : UIE=0, CC1IE=1, CC2IE=2, CC3IE=3, CC4IE=4
 *   SR   : UIF=0, CC1IF=1, CC2IF=2, CC3IF=3, CC4IF=4   (标志位写 0 清除)
 *   可对照 ST 标准外设库: TIM_OCMode_PWM1=0x0060, TIM_CCER_CC3E=0x0100
 *
 * 五、工程目录结构
 *
 *   TIM_PWM_HAL\
 *   |- Core\
 *   |  |- Inc\  main.h  stm32f1xx_hal_conf.h  stm32f1xx_it.h  tim_pwm.h
 *   |  `- Src\  main.c  tim_pwm.c  stm32f1xx_it.c  stm32f1xx_hal_msp.c  system_stm32f1xx.c
 *   |- Drivers\        HAL 驱动 + CMSIS
 *   |- Startup\        startup_stm32f103xb.s
 *   |- Project\MDK-ARM\  TIM_PWM.uvprojx   <- 双击用 Keil 打开
 *   |- Output\         编译产物 (TIM_PWM.hex)
 *   |- sim.ini         仿真逻辑分析仪配置
 *   `- readme.txt
 *
 * 六、Keil 仿真观察 PWM
 *
 *   Debug 页选 Use Simulator; F5 进入调试; View -> Logic Analyzer;
 *   点 Signal 按钮添加: TIM3_CH3 (定时器通道3的PWM输出) / P1.0 (PB0引脚) / P0.1 (PA1引脚)。
 *   若 PB0 引脚上看不到 PWM, 把 tim_pwm.h 里的 SIM_PWM_MIRROR 改成 1 重新编译,
 *   程序会把 PWM 波形镜像到 PC13 (P2.13) 上供观察。
 *
 * 七、常见问题
 *
 *   Q: PB0 输出恒为低?
 *   A: CCMR2 的 OC3M 应写 bit6~4, CCER 的 CC3E 应为 bit8。写错位会配置到通道4上。
 *   Q: LED 亮度不变化?
 *   A: 检查更新中断是否进来: TIM3->DIER 的 UIE + HAL_NVIC_EnableIRQ(TIM3_IRQn)。
 *   Q: 中文注释乱码?
 *   A: 本工程源文件为 GB2312 编码。Keil: Edit -> Configuration -> Editor -> Encoding,
 *      选 "Chinese GB2312"。
 ***************************************************************************************************
 */
