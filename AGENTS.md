# EdgeTX Agent Guide

This is a project migrating edgetx to GD32F330 series MCUs.

## Build system

CMake superbuild with two sub-projects via `ExternalProject`:
- `arm-none-eabi` — cross-compiled firmware (requires ARM GNU toolchain)
- `native` — desktop simulator + Companion app

PCB target selected at configure time via `-DPCB=<TARGET>` with optional `-DPCBREV=<REV>`. All 50+ targets in `fw.json` and `tools/build-common.sh`.

This project is already configured for Microzone C7MINI radio.

## Quick start

```sh
# Clone with submodules
git clone --recurse-submodules <repo>
# or after clone:
git submodule update --init --recursive

# Configure and build firmware for a target
cd build
make arm-none-eabi-configure
make -j$(nproc) firmware
```

## Formatting

- **clang-format** (Google style, Linux braces, 120 col): `tools/clang-format.sh`
- **uncrustify** (K&R, 2-space indent) + copyright header + include-guard check: `tools/codeformat.sh` (excludes `thirdparty/`)
- Root `.clang-format` file defines the canonical style.

## Style conventions

- C++17, GPL v2 licensed
- Warnings-as-errors: `-DWARNINGS_AS_ERRORS=YES` (enabled in CI)
- Translation validation: `tools/check_translations.py`

## CI

Runs in `ghcr.io/edgetx/edgetx-dev:2.11` Docker image (includes ARM toolchain). Main workflow (`.github/workflows/actions.yml`): test matrix (30 targets) → build matrix (grouped targets) → package.

## Project layout

| Directory | Purpose |
|---|---|---|
| `radio/src/` | Radio firmware (entry: `edgetx.cpp`, `main.cpp`) |
| `companion/src/` | Desktop Companion app (entry: `companion.cpp`) |
| `cmake/` | CMake modules & toolchains |
| `tools/` | Build scripts, formatting, QA utilities |

## GD32 → C7MINI Port Status

### Overview

EdgeTX 移植到 GD32F330 (Microzone C7MINI 遥控器)，项目进行中，目前正在进行外设接口的移植

### Directory Structure

```
radio/src/
├── targets/c7mini/                          ← C7MINI 板级代码
│   ├── CMakeLists.txt                       (配置：GD32F330RB, 128K flash, 16K RAM, 所有功能OFF)
│   ├── board.h                              (板级定义：Flash/电池/LCD/驱动声明)
│   ├── board.cpp                            (初始化流程：GPIO→定时器→按键→开关→ADC→LCD→背光)
│   ├── hal.h                                (引脚映射：按键/微调/开关/ADC/背光/LCD/音频/定时器)
│   ├── bsp_io.h                             (开关BSP存根)
│   ├── lcd_driver_spi.cpp                   (128x64 OLED, 软SPI驱动, SSD1309)
│   ├── top_lcd_driver.cpp                   (HT1621段码LCD, 时间/RSSI/电池显示)
│   ├── backlight_driver.cpp                 (GPIO背光, 简单开关)
│   └── led_driver.cpp                       (电源/RF LED指示灯)
│
├── targets/common/arm/gd32/                 ← GD32 通用驱动层
│   ├── CMakeLists.txt                       (构建驱动库 gd32_drivers)
│   ├── board_common.h                       (公共头文件包含)
│   ├── gd32_gpio.h / .cpp                   (GPIO HAL)
│   ├── gd32_gpio_driver.h / .cpp            (GPIO时钟使能)
│   ├── gd32_adc.h / .cpp                    (ADC驱动, 过采样+DMA, 16通道)
│   ├── gd32_dma.h / .cpp                    (DMA时钟控制)
│   ├── gd32_spi.h / .cpp                    (SPI驱动, 轮询+DMA)
│   ├── gd32_timer.h / .cpp                  (定时器时钟使能)
│   ├── gd32_exti_driver.h / .cpp            (EXTI中断, 灵活IRQ分发)
│   ├── gd32_serial_driver.h / .cpp          (串口驱动)
│   ├── gd32_usart_driver.h / .cpp           (USART HAL, 含DMA TX/RX)
│   ├── gd32_softserial_driver.h / .cpp      (软串口, 定时器+EXTI, 不完整)
│   ├── gd32_switch_driver.h / .cpp          (开关读取)
│   ├── gd32_cmsis.h                         (CMSIS包含包装)
│   ├── gd32_stdlib.h                        (StdPeriph包含包装)
│   ├── timers_driver.h / .cpp               (系统定时器, TIMER13 + SysTick)
│   ├── heartbeat_driver.h / .cpp            (内部模块心跳)
│   ├── module_timer_driver.h / .cpp         (模块脉冲定时器)
│   ├── pwr_driver.cpp                       (电源管理)
│   ├── flash_driver.h / .cpp                (Flash FMC编程/擦除)
│   ├── delays_driver.h / .cpp               (DWT周期延时)
│   ├── abnormal_reboot.cpp                  (异常重启检测)
│   ├── cpu_id.h / .cpp                      (唯一ID读取)
│   └── f3x0/                                ← F3x0 系列特定
│       ├── CMakeLists.txt                   (StdPeriph库 + FreeRTOS CM4F)
│       ├── system_gd32f3x0.c                (SystemInit, 84MHz PLL配置)
│       └── memory_sections.h                (内存段属性)
│
├── boards/generic_gd32/                     ← GD32 板级通用
│   ├── CMakeLists.txt                       (复用 generic_stm32 代码)
│   ├── module_ports.h / .cpp                (内外模块串口/定时器定义)
│   ├── linker/
│   │   ├── firmware.ld                      (主链接脚本)
│   │   ├── common_sections.ld               (标准段)
│   │   ├── common_text.ld                   (代码段)
│   │   └── gd32f330xb/
│   │       ├── layout.ld                    (64K CODE + 64K DATA + 16K RAM)
│   │       └── extra_sections.ld            (栈/堆)
│   └── startup/startup_gd32f3x0.s           (Cortex-M4启动)
│
└── thirdparty/
    ├── CMSIS/Device/GigaDevice/GD32F3x0/    (CMSIS设备头文件)
    └── GD32F3x0_StdPeripheral_Driver/       (GigaDevice标准外设库, 46文件)
```

### Known Issues

- **构建失败**: `gd32_i2c_driver.cpp` 在 CMakeLists 中被引用但文件不存在
- **软串口不完整**: `enableRx`/`clearRxBuffer` 设为 nullptr
- **HT1621 电池边框**始终显示（`top_lcd_driver.cpp:336`）
- **ADC 方向**未校准（`hal.h:149`）
- **电池分压值**存疑（`board.h:325`）
- GD32 SPL 代码含低效循环，计划替换为直接寄存器访问（`gd32_gpio.cpp:28`）

## TODO List

### P0 — 构建修复

- [x] 外设接口移植 (`radio/src/targets/common/arm/gd32/`) i2c暂时不加
- [ ] 板级接口移植 (`radio/src/boards/generic_gd32`)
- [ ] 验证编译通过，生成 `.elf`/`.bin`

### P1 — 核心缺失功能
- [ ] 实现 **外置模块端口** (hal.h:180)
- [ ] 实现 **教练端口** (hal.h:183, 替换 SBUS 为 S.PORT)
- [ ] 实现 **串口** (hal.h:186)
- [ ] 配置 **混频器调度定时器** (hal.h:277)
- [ ] 实现 **蜂鸣器/音频** (hal.h:263, 目前只有 PWM 定时器定义)
- [ ] 完成 **软串口 RX** (gd32_softserial_driver.cpp)
- [ ] 实现 **RTC驱动** (board.h:297 引用 rtcInit())

### P2 — 功能完善
- [ ] 添加 **A7105 内部模块**支持 (hal.h:177)
- [ ] 校准 **ADC 方向** (hal.h:149)
- [ ] 确认 **电池分压电路值** (board.h:325)
- [ ] 修复 HT1621 电池边框始终显示 (top_lcd_driver.cpp:336)
- [ ] 添加 FreeRTOS 支持（如果需要）
- [ ] 用直接寄存器访问替换 GD32 SPL (gd32_gpio.cpp:28)

### P3 — 工程优化
- [ ] 创建 C7MINI 硬件定义 JSON 文件
- [ ] 将 C7MINI 加入 fw.json 发布矩阵
- [ ] 添加 CI 构建支持
- [ ] 清理代码中的所有 TODO 注释
- [ ] 实现 USB 充电检测
- [ ] SD 卡支持（如果需要）
- [ ] 测试并校准所有开关/摇杆/旋钮
