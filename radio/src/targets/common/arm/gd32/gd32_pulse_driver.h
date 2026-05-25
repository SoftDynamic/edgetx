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

#include <stdint.h>
#include <stdbool.h>

#include "gd32_stdlib.h"
#include "hal/gpio.h"

struct gd32_pulse_timer_t;
typedef bool (*gd32_pulse_dma_tc_fct)(void*);

struct gd32_pulse_dma_tc_cb_t {
  gd32_pulse_dma_tc_fct cb;
  void* ctx;
};

struct gd32_pulse_timer_t {
  gpio_t                     GPIO;
  uint32_t                   GPIO_Alternate;

  TIMER_TypeDef*             TIMx;
  uint32_t                   TIM_Freq;
  uint16_t                   TIM_Channel;
  IRQn_Type                  TIM_IRQn;

  void*                      DMAx;
  uint32_t                   DMA_Stream;
  uint32_t                   DMA_Channel;
  IRQn_Type                  DMA_IRQn;
  gd32_pulse_dma_tc_cb_t*   DMA_TC_CallbackPtr;
};

int gd32_pulse_init(const gd32_pulse_timer_t* tim, uint32_t freq);
void gd32_pulse_deinit(const gd32_pulse_timer_t* tim);

void gd32_pulse_config_input(const gd32_pulse_timer_t* tim);
void gd32_pulse_config_output(const gd32_pulse_timer_t* tim, bool polarity,
                               uint32_t ocmode, uint32_t cmp_val);
void gd32_pulse_set_polarity(const gd32_pulse_timer_t* tim, bool polarity);
bool gd32_pulse_get_polarity(const gd32_pulse_timer_t* tim);
void gd32_pulse_set_period(const gd32_pulse_timer_t* tim, uint32_t period);
void gd32_pulse_set_cmp_val(const gd32_pulse_timer_t* tim, uint32_t cmp_val);
void gd32_pulse_start(const gd32_pulse_timer_t* tim);
void gd32_pulse_stop(const gd32_pulse_timer_t* tim);
void gd32_pulse_wait_for_completed(const gd32_pulse_timer_t* tim);
bool gd32_pulse_if_not_running_disable(const gd32_pulse_timer_t* tim);
void gd32_pulse_start_dma_req(const gd32_pulse_timer_t* tim,
                               const void* pulses, uint16_t length,
                               uint32_t ocmode, uint32_t cmp_val);
void gd32_pulse_dma_tc_isr(const gd32_pulse_timer_t* tim);
void gd32_pulse_tim_update_isr(const gd32_pulse_timer_t* tim);
