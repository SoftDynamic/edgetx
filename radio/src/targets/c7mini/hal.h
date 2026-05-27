/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

#if defined(GD32F330)
  #define CPU_FREQ            84000000
  #define PERI1_FREQUENCY     42000000
  #define PERI2_FREQUENCY     42000000
  #define TIMER_MULT_APB1     2
  #define TIMER_MULT_APB2     2
#else
  #define CPU_FREQ            120000000
  #define PERI1_FREQUENCY     30000000
  #define PERI2_FREQUENCY     60000000
  #define TIMER_MULT_APB1     2
  #define TIMER_MULT_APB2     2
#endif

#define TELEMETRY_EXTI_PRIO             0 // required for soft serial

// Keys
#if defined(PCBC7MINI)
  #define KEYS_GPIO_REG_MENU            GPIOC
  #define KEYS_GPIO_PIN_MENU            GPIO_PIN_0
  #define KEYS_GPIO_REG_EXIT            GPIOC
  #define KEYS_GPIO_PIN_EXIT            GPIO_PIN_1
  #define KEYS_GPIO_REG_TELE            GPIOA
  #define KEYS_GPIO_PIN_TELE            GPIO_PIN_11
  #define KEYS_GPIO_REG_PAGEUP          GPIOC
  #define KEYS_GPIO_PIN_PAGEUP          GPIO_PIN_2
  #define KEYS_GPIO_REG_PAGEDN          GPIOC
  #define KEYS_GPIO_PIN_PAGEDN          GPIO_PIN_3
  #define KEYS_GPIO_REG_ENTER           GPIOA
  #define KEYS_GPIO_PIN_ENTER           GPIO_PIN_12 // Use LOCK button as ENTER
#else
  #error "Unknown PCB"
#endif

// Trims
#if defined(PCBC7MINI)
  #define TRIMS_GPIO_REG_LHL            GPIOB
  #define TRIMS_GPIO_PIN_LHL            GPIO_PIN_9
  #define TRIMS_GPIO_REG_LHR            GPIOB
  #define TRIMS_GPIO_PIN_LHR            GPIO_PIN_8
  #define TRIMS_GPIO_REG_LVD            GPIOB
  #define TRIMS_GPIO_PIN_LVD            GPIO_PIN_7
  #define TRIMS_GPIO_REG_LVU            GPIOB
  #define TRIMS_GPIO_PIN_LVU            GPIO_PIN_6
  #define TRIMS_GPIO_REG_RVD            GPIOB
  #define TRIMS_GPIO_PIN_RVD            GPIO_PIN_4
  #define TRIMS_GPIO_REG_RVU            GPIOB
  #define TRIMS_GPIO_PIN_RVU            GPIO_PIN_5
  #define TRIMS_GPIO_REG_RHL            GPIOB
  #define TRIMS_GPIO_PIN_RHL            GPIO_PIN_2
  #define TRIMS_GPIO_REG_RHR            GPIOB
  #define TRIMS_GPIO_PIN_RHR            GPIO_PIN_3
#else
  #error "Unknown PCB"
#endif

// Switches
#if defined(PCBC7MINI)
  #define STORAGE_SWITCH_A
  #define HARDWARE_SWITCH_A

  #define STORAGE_SWITCH_B
  #define HARDWARE_SWITCH_B


  #define STORAGE_SWITCH_C
  #define HARDWARE_SWITCH_C
  #define SWITCHES_GPIO_REG_C           GPIOD
  #define SWITCHES_GPIO_PIN_C           GPIO_PIN_2

  #define STORAGE_SWITCH_D
  #define HARDWARE_SWITCH_D
  #define SWITCHES_GPIO_REG_D           GPIOB
  #define SWITCHES_GPIO_PIN_D           GPIO_PIN_10
#else
  #error "Unknown PCB"
#endif

// ADC
// [x]: 移植到gd32
#define ADC_MAIN                      ADC
#define ADC_DMA                       DMA
#define ADC_DMA_CHANNEL               DMA_CH0
#ifdef ADC_DMA_STREAM
    #undef ADC_DMA_STREAM
#endif
#define ADC_DMA_STREAM                  0
#define ADC_DMA_STREAM_IRQ              DMA_Channel0_IRQn
#define ADC_DMA_STREAM_IRQHandler       DMA_Channel0_IRQHandler
#define ADC_IRQ_HANDLER                 ADC_CMP_IRQHandler
#define ADC_SAMPTIME                    ADC_SAMPLETIME_28POINT5

#if defined(PCBC7MINI)
  #define ADC_GPIO_PIN_STICK_RV         GPIO_PIN_2    // PA2
  #define ADC_GPIO_PIN_STICK_RH         GPIO_PIN_1    // PA1
  #define ADC_GPIO_PIN_STICK_LH         GPIO_PIN_4    // PA4
  #define ADC_GPIO_PIN_STICK_LV         GPIO_PIN_3    // PA3
  #define ADC_CHANNEL_STICK_RV          ADC_CHANNEL_2
  #define ADC_CHANNEL_STICK_RH          ADC_CHANNEL_1
  #define ADC_CHANNEL_STICK_LH          ADC_CHANNEL_4
  #define ADC_CHANNEL_STICK_LV          ADC_CHANNEL_3

  #define HARDWARE_POT1
  #define HARDWARE_POT2
  #define ADC_GPIO_PIN_POT1             GPIO_PIN_7  // PA7
  #define ADC_GPIO_PIN_POT2             GPIO_PIN_1  // PB1
  #define ADC_CHANNEL_POT1              ADC_CHANNEL_7
  #define ADC_CHANNEL_POT2              ADC_CHANNEL_9

  #define ADC_GPIO_PIN_BATT             GPIO_PIN_0  // PA0
  #define ADC_CHANNEL_BATT              ADC_CHANNEL_0

  #define ADC_GPIO_PIN_SWA              GPIO_PIN_6  // PA6
  #define ADC_CHANNEL_SWA               ADC_CHANNEL_6
  #define ADC_GPIO_PIN_SWB              GPIO_PIN_5  // PA5
  #define ADC_CHANNEL_SWB               ADC_CHANNEL_5

  #define ADC_GPIOA_PINS                (ADC_GPIO_PIN_BATT | ADC_GPIO_PIN_STICK_RH | ADC_GPIO_PIN_STICK_RV | ADC_GPIO_PIN_STICK_LV | ADC_GPIO_PIN_STICK_LH | ADC_GPIO_PIN_SWB | ADC_GPIO_PIN_SWA | ADC_GPIO_PIN_POT1)
  #define ADC_GPIOB_PINS                (ADC_GPIO_PIN_POT2)

  #define ADC_VREF_PREC2                300
#else
  #error "Unknown PCB"
#endif

#if defined(PCBC7MINI)
  #define ADC_DIRECTION               {1,1,1,1, 1,1, 1, 1,1} // [ ]: 测量实际的方向
#else
  #error "Unknown PCB"
#endif

// PWR and LED driver

#if defined(PCBC7MINI)
  #define PWR_SWITCH_GPIO               GPIO_PIN(GPIOC, 10)  // PC10
  #define PWR_ON_GPIO                   GPIO_PIN(GPIOF, 7)  // PF7
#else
  #error "Unknown PCB"
#endif

#if defined(PCBC7MINI)
  #define STATUS_LEDS
  #define GPIO_LED_GPIO_ON              gpio_clear
  #define GPIO_LED_GPIO_OFF             gpio_set
  #define POWER_LED_STANDALONG
  #define LED_PWR_GPIO                  GPIO_PIN(GPIOC, 9)
  #define RF_LEDS
  #define LED_RF_GPIO                   GPIO_PIN(GPIOC, 8)
#else
  #error "Unknown PCB"
#endif

// Internal Module
// [ ]：添加A7105支持

// External Module
// [ ]: TODO

// Trainer Port
// [ ]: TODO

// Serial Port
// [ ]: TODO

// Telemetry

// PCBREV

// USB Charger

// S.Port update connector

// Heartbeat for iXJT / ISRM synchro

// Trainer / Trainee from the module bay
#if defined(PCBX9LITE) || defined(PCBXLITE) || defined(RADIO_X9DP2019) || \
  defined(PCBX7ACCESS) || defined(RADIO_ZORRO) || defined(RADIO_POCKET) || \
  defined(RADIO_TX12MK2) || defined(RADIO_BOXER) || defined(RADIO_MT12) ||\
  defined(RADIO_T14) || defined(RADIO_T12MAX) || defined(RADIO_V14) || defined(RADIO_V12) \
  || defined(RADIO_GX12)
  #define TRAINER_MODULE_CPPM_TIMER            TIM3
  #define TRAINER_MODULE_CPPM_FREQ             (PERI1_FREQUENCY * TIMER_MULT_APB1)
  #define TRAINER_MODULE_CPPM_GPIO             EXTMODULE_RX_GPIO
  #define TRAINER_MODULE_CPPM_TIMER_Channel    LL_TIM_CHANNEL_CH2
  #define TRAINER_MODULE_CPPM_TIMER_IRQn       TIM3_IRQn
  #define TRAINER_MODULE_CPPM_GPIO_AF          LL_GPIO_AF_2
#if defined(PCBX9LITE) ||  defined(PCBXLITE)
  #define TRAINER_MODULE_CPPM_TIMER_IRQHandler TIM3_IRQHandler
#endif
#elif defined(INTMODULE_HEARTBEAT_GPIO) && defined(HARDWARE_EXTERNAL_MODULE)
  // Trainer CPPM input on heartbeat pin
  #define TRAINER_MODULE_CPPM_TIMER               TRAINER_TIMER
  #define TRAINER_MODULE_CPPM_FREQ                (PERI1_FREQUENCY * TIMER_MULT_APB1)
  #define TRAINER_MODULE_CPPM_GPIO                INTMODULE_HEARTBEAT_GPIO
  #define TRAINER_MODULE_CPPM_TIMER_Channel       LL_TIM_CHANNEL_CH2
  #define TRAINER_MODULE_CPPM_TIMER_IRQn          TRAINER_TIMER_IRQn
  #define TRAINER_MODULE_CPPM_GPIO_AF             GPIO_AF2
  // Trainer SBUS input on heartbeat pin
  #define TRAINER_MODULE_SBUS_USART               USART6
  #define TRAINER_MODULE_SBUS_GPIO                INTMODULE_HEARTBEAT_GPIO
  #define TRAINER_MODULE_SBUS_DMA                 DMA2
  #define TRAINER_MODULE_SBUS_DMA_STREAM          DMA2_Stream1
  #define TRAINER_MODULE_SBUS_DMA_STREAM_LL       LL_DMA_STREAM_1
  #define TRAINER_MODULE_SBUS_DMA_CHANNEL         LL_DMA_CHANNEL_5
#else
  // TODO: replace SBUS trainer with S.PORT pin
#endif

// USB

// BackLight
#if defined(PCBC7MINI)
  #define BACKLIGHT_GPIO                  GPIO_PIN(GPIOA, 10)
#else
  #error "Unknown PCB"
#endif

// LCD driver
#if defined(PCBC7MINI)
  #define LCD_VERTICAL_INVERT
  // #define LCD_HORIZONTAL_INVERT

  // Soft SPI: these pins are not connected to SPI periph on STM32F205
  #define LCD_MOSI_GPIO                 GPIO_PIN(GPIOC, 5)
  #define LCD_CLK_GPIO                  GPIO_PIN(GPIOC, 4)
  #define LCD_A0_GPIO                   GPIO_PIN(GPIOB, 0)
  #define LCD_NCS_GPIO                  GPIO_PIN(GPIOF, 5)
  #define LCD_RST_GPIO                  GPIO_PIN(GPIOF, 4)
#endif
#if defined(SSD1309_LCD)
  #define LCD_SPI_PRESCALER             SPI_CR1_BR_1
#else
  #define LCD_SPI_PRESCALER             0
#endif

// Storage
#define STORAGE_USE_INTERNAL_FLASH

// Audio
// [ ]: TODO 蜂鸣器
#define AUDIO_OUTPUT_GPIO               GPIO_PIN(GPIOC, 6)
// #define AUDIO_DMA                       DMA1
// #define AUDIO_DMA_Stream                DMA1_Stream5
// #define AUDIO_DMA_Stream_IRQn           DMA1_Stream5_IRQn
// #define AUDIO_DMA_Stream_IRQHandler     DMA1_Stream5_IRQHandler
#define AUDIO_TIMER                     TIMER2

// Millisecond timer
#define MS_TIMER                        TIMER13
#define MS_TIMER_IRQn                   TIMER13_IRQn
#define MS_TIMER_IRQHandler             TIMER13_IRQHandler

// Mixer scheduler timer
// [ ]: TODO
// #define MIXER_SCHEDULER_TIMER                TIM12
// #define MIXER_SCHEDULER_TIMER_FREQ           (PERI1_FREQUENCY * TIMER_MULT_APB1)
// #define MIXER_SCHEDULER_TIMER_IRQn           TIM8_BRK_TIM12_IRQn
// #define MIXER_SCHEDULER_TIMER_IRQHandler     TIM8_BRK_TIM12_IRQHandler
