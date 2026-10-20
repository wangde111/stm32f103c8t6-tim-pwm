# 实验四：STM32 定时器（TIM3）计数方式 + PWM 呼吸灯

> 目标：深入理解 STM32 定时器原理，掌握脉宽调制（PWM）生成方法。
> 器件：STM32F103C8T6，HAL 库 + Keil MDK 5.36（ARMCC V5.06），定时器部分采用**寄存器级**配置。

---

## 一、实验内容

| 任务 | 要求 | 本工程实现 |
|------|------|-----------|
| 任务一 | 用 TIM2~TIM5 某一通道引脚接 LED，用**定时器计数方式**控制 LED 以 2 s 的周期亮-灭 | TIM3 每 1 ms 计数一次（ARR 溢出），在更新中断里累计 2000 次 = 2 s，翻转 **LED1（PA1）** |
| 任务二 | 用**定时器 PWM 模式**让 LED 以呼吸灯方式渐亮渐灭，周期 1~2 s | TIM3_CH3（**PB0，LED2 绿色**）输出 1 kHz PWM，占空比 1 s 内 0→100%、再 1 s 内 100%→0，呼吸周期 **2 s** |

---

## 二、硬件接线

| STM32 | 外接 | 说明 |
|-------|------|------|
| **PB0** | LED2（绿色，220 Ω 串 1 kΩ）→ GND | TIM3_CH3 的复用引脚，PWM 输出，高电平点亮 |
| **PA1** | LED1（红色）→ GND | 任务一：定时器计数方式控制的 2 s 亮-灭 |
| GND | 与 LED 共地 | 必须共地 |

> PB0 默认是 TIM3_CH3 的复用功能（AF），需要在 GPIO 里配置成 **AF 推挽输出**。
> 如果手边没有现成的 LED，PB0 / PA1 都是普通 IO，直接接 LED + 限流电阻即可。

---

## 三、工程结构

```
定时器和pwm/
├── TIM_PWM_HAL/
│   ├── Core/
│   │   ├── Inc/  main.h  stm32f1xx_hal_conf.h  stm32f1xx_it.h  tim_pwm.h
│   │   └── Src/  main.c  tim_pwm.c  stm32f1xx_it.c  stm32f1xx_hal_msp.c  system_stm32f1xx.c
│   ├── Drivers/      STM32F1xx HAL 驱动 + CMSIS
│   ├── Startup/      startup_stm32f103xb.s
│   ├── Project/MDK-ARM/  TIM_PWM.uvprojx   ← 双击用 Keil 打开
│   ├── Output/       编译产物 (TIM_PWM.hex)
│   ├── sim.ini       Keil 仿真逻辑分析仪配置
│   └── readme.txt
└── README.md         本文档
```

---

## 四、核心代码说明

### 1. 时钟与分频

```
HSE 8 MHz × 9 (PLL) = SYSCLK 72 MHz
AHB  = 72 MHz
APB1 = 36 MHz (HCLK/2)   → TIM2/3/4 挂在 APB1 上
APB2 = 72 MHz (HCLK/1)
```
关键点：**APB1 预分频系数 ≠ 1 时，定时器时钟 TIMxCLK = 2 × PCLK1 = 72 MHz**。

```c
TIM3->PSC = 71;    // 计数频率 = 72 MHz / (71+1) = 1 MHz
TIM3->ARR = 999;   // 溢出周期 = 1000 / 1 MHz = 1 ms
TIM3->CR1 = ARPE;  // 预装载：改 ARR/CCR 不影响当前周期
```
于是：**PWM 载波频率 = 1 kHz**（ARR 溢出一次就是一个 PWM 周期），同时**更新中断也是每 1 ms 一次**，
正好用它当软件时基去做"呼吸"和"2 s 闪烁"。

### 2. CH3 输出 PWM（寄存器级）

```c
TIM3->CCMR2 &= ~(7UL << 4);   // 清 OC3M（通道3 输出比较模式）
TIM3->CCMR2 |=  (6UL << 4);   // OC3M = 110b = PWM 模式 1
TIM3->CCMR2 |=  (1UL << 3);   // OC3PE：CCR3 预装载，改占空比无毛刺
TIM3->CCR3  = duty;           // 占空比 = CCR3 / (ARR+1)
TIM3->CCER |=  (1UL << 8);    // CC3E：通道3 输出使能
```

PWM 模式 1 的规则：**CNT < CCR3 时输出高电平，否则低电平**。
CCR3 = 0 → 0%；CCR3 = 1000 → 100%（CNT 只到 999，永远小于 1000）。

### 3. ⚠️ 位域陷阱（STM32F1 特有，最容易写错的地方）

| 寄存器 | STM32F1 的排布 | 注意事项 |
|--------|----------------|----------|
| `CCMR1` | 低字节 = 通道1，高字节 = 通道2 | OC1M 在 bit 6~4，OC1PE 在 bit 3 |
| `CCMR2` | 低字节 = **通道3**，高字节 = 通道4 | **OC3M 在 bit 6~4，OC3PE 在 bit 3**；OC4M 才是 bit 14~12 |
| `CCER`  | 每个通道占 4 位（E/P/NE/NP） | **CC3E 在 bit 8**，CC3P 在 bit 9，CC4E 在 bit 12 |
| `DIER`  | UIE=0, CC1IE=1, CC2IE=2, **CC3IE=3**, CC4IE=4 | |
| `SR`    | UIF=0, CC1IF=1, CC2IF=2, **CC3IF=3**, CC4IF=4 | 标志位"写 0 清除" |

> 可对照 ST 标准外设库验证：`TIM_OCMode_PWM1 = 0x0060`、`TIM_CCER_CC3E = 0x0100`
> （位于 `MDK5.36\Packs\Keil\STM32F1xx_DFP\2.3.0\Device\StdPeriph_Driver`）。
> 本工程没有使能 HAL_TIM 模块（机器上找不到 `stm32f1xx_hal_tim.c` 且无外网），
> 所以 HAL 的 `HAL_TIM_PWM_Start()` 用不了，采用寄存器级实现，正好也便于理解定时器原理。

### 4. 中断服务

```c
void TIM3_IRQHandler(void)
{
  if ((TIM3->SR & TIM_SR_UIF) != 0U)
  {
    TIM3->SR = (uint16_t)~TIM_SR_UIF;   // F1 的标志位是"写 0 清除"
    TIM3_App_Tick();                    // 1 ms 软件时基
  }
}
```
`TIM3_App_Tick()` 里做两件事：
1. **呼吸灯**：占空比每 1 ms ±1 个计数点（±0.1%），0→100% 用 1 s，100%→0 用 1 s，合计 2 s 一个呼吸周期；
2. **2 s 闪烁**：`s_blink_tick` 累加到 2000（=2 s）就翻转一次 LED1（PA1）。

---

## 五、Keil 仿真：用逻辑分析仪观察 PWM 波形

1. 打开 `Project\MDK-ARM\TIM_PWM.uvprojx`；
2. `Options for Target → Debug → Use Simulator`（本工程已配好 `sim.ini`）；
3. `F5` 进入调试 → `View → Logic Analyzer`；
4. 点 `Setup` 旁边的 **Signal** 按钮，添加信号：
   - `TIM3_CH3` —— TIM3 通道 3 的 PWM 输出（**首选**）
   - `P1.0` —— PB0 引脚的电平（P1.x = GPIOB，x 为引脚号）
   - `P0.1` —— PA1 引脚电平（2 s 闪烁）
5. `Run`（F5）运行，即可看到：
   - `TIM3_CH3`：1 kHz 的方波，高电平宽度随呼吸缓慢变化（0%→100%→0%）；
   - `P0.1`：2 s 翻转一次的方波。

> `sim.ini` 里已经写好了 `LA TIM3_CH3` / `LA P1.0` / `LA P0.1`，如果仿真窗口里没有信号，
> 就按上面第 4 步手动添加一次即可（信号名来自 Keil 的 STM32 外设模型）。

**若 PB0 上看不到 PWM 波形**：把 `Core/Inc/tim_pwm.h` 里的 `SIM_PWM_MIRROR` 改成 `1` 重新编译，
程序会用定时器中断把同样的波形"镜像"到 PC13（P2.13）上，供逻辑分析仪观察。

---

## 六、常见问题

| 现象 | 原因 / 排查 |
|------|-------------|
| PB0 输出恒为低 | CCMR2 的 OC3M 写成了 bit 12~14（F1 里那是通道4）；CCER 的 CC3E 应为 bit 8 |
| LED 亮度不变 | 占空比没变：检查更新中断是否进来（`TIM3->DIER` 的 UIE、`HAL_NVIC_EnableIRQ(TIM3_IRQn)`） |
| 完全不亮 | PB0 没配成 AF 推挽输出；或 LED 极性反了（低电平点亮） |
| 呼吸太快/太慢 | 由 `BREATH_STEP` 与 `TIM3_ARR` 决定：`0→100%` 用时 = 1000 × 1 ms ≈ 1 s |
| 编译报找不到 TIM 头文件 | 工程里没加 `tim_pwm.h` 所在目录，或没把 `tim_pwm.c` 加入工程（已在 uvprojx 里加好） |

---

## 七、源码编码说明

工程内 `Core` 目录下的 `.c/.h` 源文件为 **GB2312** 编码（与原工程保持一致）。
若 Keil 里中文注释乱码：`Edit → Configuration → Editor → Encoding → Chinese GB2312`。
