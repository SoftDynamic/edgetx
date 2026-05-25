/*
 * Copyright (C) EdgeTx
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

#include "gd32_pulse_driver.h"
#include "gd32_gpio.h"
#include "gd32_dma.h"
#include "gd32_timer.h"
#include "hal/gpio.h"

#include <string.h>

#define GD32_DEFAULT_TIMER_FREQ      2000000
#define GD32_DEFAULT_TIMER_AUTORELOAD 65535

static uint32_t _get_timer_periph(const gd32_pulse_timer_t* tim)
{
  return (uint32_t)tim->TIMx;
}

static void init_dma_arr_mode(const gd32_pulse_timer_t* tim)
{
  dma_channel_enum ch = (dma_channel_enum)tim->DMA_Stream;

  dma_deinit(ch);

  dma_parameter_struct dmaInit;
  dma_struct_para_init(&dmaInit);

  dmaInit.direction    = DMA_MEMORY_TO_PERIPHERAL;
  dmaInit.memory_addr  = 0;
  dmaInit.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
  dmaInit.periph_addr  = (uint32_t)&tim->TIMx->CAR;
  dmaInit.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
  dmaInit.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
  dmaInit.memory_width = DMA_MEMORY_WIDTH_16BIT;
  dmaInit.priority     = DMA_PRIORITY_ULTRA_HIGH;
  dmaInit.number       = 0;

  gd32_dma_enable_clock(nullptr);
  dma_init(ch, &dmaInit);
}

int gd32_pulse_init(const gd32_pulse_timer_t* tim, uint32_t freq)
{
  uint32_t timer_periph = _get_timer_periph(tim);

  if (tim->DMA_TC_CallbackPtr) {
    memset(tim->DMA_TC_CallbackPtr, 0, sizeof(gd32_pulse_dma_tc_cb_t));
  }

  gpio_init_af(tim->GPIO, (gpio_af_t)tim->GPIO_Alternate, GPIO_PIN_SPEED_MEDIUM);

  timer_parameter_struct timInit;
  timer_struct_para_init(&timInit);

  if (!freq) {
    freq = GD32_DEFAULT_TIMER_FREQ;
  }

  timInit.prescaler       = (tim->TIM_Freq / freq) - 1;
  timInit.period          = GD32_DEFAULT_TIMER_AUTORELOAD;
  timInit.alignedmode     = TIMER_COUNTER_EDGE;
  timInit.counterdirection = TIMER_COUNTER_UP;
  timInit.clockdivision   = TIMER_CKDIV_DIV1;
  timInit.repetitioncounter = 0;

  gd32_timer_enable_clock(tim->TIMx);
  timer_init(timer_periph, &timInit);

  if (tim->DMAx != nullptr && (int32_t)tim->DMA_IRQn >= 0) {
    init_dma_arr_mode(tim);
    NVIC_EnableIRQ(tim->DMA_IRQn);
    NVIC_SetPriority(tim->DMA_IRQn, 7);
  }

  if ((int32_t)tim->TIM_IRQn >= 0) {
    NVIC_EnableIRQ(tim->TIM_IRQn);
    NVIC_SetPriority(tim->TIM_IRQn, 7);
  }

  return 0;
}

void gd32_pulse_deinit(const gd32_pulse_timer_t* tim)
{
  uint32_t timer_periph = _get_timer_periph(tim);

  if ((int32_t)tim->DMA_IRQn >= 0) {
    NVIC_DisableIRQ(tim->DMA_IRQn);
  }

  if ((int32_t)tim->TIM_IRQn >= 0) {
    NVIC_DisableIRQ(tim->TIM_IRQn);
  }

  if (tim->DMAx != nullptr) {
    dma_deinit((dma_channel_enum)tim->DMA_Stream);
  }

  if (tim->DMA_TC_CallbackPtr) {
    memset(tim->DMA_TC_CallbackPtr, 0, sizeof(gd32_pulse_dma_tc_cb_t));
  }

  timer_deinit(timer_periph);
  gd32_timer_disable_clock(tim->TIMx);

  gpio_init(tim->GPIO, GPIO_IN, GPIO_PIN_SPEED_MEDIUM);
}

static bool _is_complementary_channel(uint16_t channel)
{
  (void)channel;
  return false;
}

static uint16_t _get_base_channel(uint16_t channel)
{
  return channel;
}

void gd32_pulse_config_output(const gd32_pulse_timer_t* tim, bool polarity,
                               uint32_t ocmode, uint32_t cmp_val)
{
  uint32_t timer_periph = _get_timer_periph(tim);
  uint16_t channel = _get_base_channel(tim->TIM_Channel);

  timer_oc_parameter_struct ocInit;
  timer_channel_output_struct_para_init(&ocInit);

  bool comp_ch = _is_complementary_channel(tim->TIM_Channel);
  if (!comp_ch) {
    ocInit.outputstate  = TIMER_CCX_ENABLE;
    ocInit.outputnstate = (uint16_t)DISABLE;
  } else {
    ocInit.outputstate  = (uint32_t)DISABLE;
    ocInit.outputnstate = TIMER_CCXN_ENABLE;
  }

  ocInit.ocpolarity  = polarity ? TIMER_OC_POLARITY_HIGH : TIMER_OC_POLARITY_LOW;
  ocInit.ocnpolarity = polarity ? TIMER_OC_POLARITY_HIGH : TIMER_OC_POLARITY_LOW;

  timer_channel_output_config(timer_periph, channel, &ocInit);

  timer_channel_output_mode_config(timer_periph, channel, (uint16_t)ocmode);
  timer_channel_output_pulse_value_config(timer_periph, channel, cmp_val);
  timer_channel_output_shadow_config(timer_periph, channel, TIMER_OC_SHADOW_ENABLE);

  timer_primary_output_config(timer_periph, ENABLE);
}

void gd32_pulse_set_polarity(const gd32_pulse_timer_t* tim, bool polarity)
{
  uint32_t timer_periph = _get_timer_periph(tim);
  uint16_t polarity_val = polarity ? TIMER_OC_POLARITY_HIGH : TIMER_OC_POLARITY_LOW;
  timer_channel_output_polarity_config(timer_periph, tim->TIM_Channel, polarity_val);
}

bool gd32_pulse_get_polarity(const gd32_pulse_timer_t* tim)
{
  uint32_t timer_periph = _get_timer_periph(tim);
  uint32_t shift = (uint32_t)tim->TIM_Channel * 4;
  return (TIMER_CHCTL2(timer_periph) & (TIMER_CHCTL2_CH0P << shift)) != 0;
}

void gd32_pulse_set_period(const gd32_pulse_timer_t* tim, uint32_t period)
{
  timer_autoreload_value_config(_get_timer_periph(tim), period - 1);
}

void gd32_pulse_set_cmp_val(const gd32_pulse_timer_t* tim, uint32_t cmp_val)
{
  timer_channel_output_pulse_value_config(_get_timer_periph(tim), tim->TIM_Channel, cmp_val);
}

void gd32_pulse_start(const gd32_pulse_timer_t* tim)
{
  timer_enable(_get_timer_periph(tim));
}

void gd32_pulse_stop(const gd32_pulse_timer_t* tim)
{
  timer_disable(_get_timer_periph(tim));
}

bool gd32_pulse_if_not_running_disable(const gd32_pulse_timer_t* tim)
{
  uint32_t timer_periph = _get_timer_periph(tim);

  if (DMA_CHCTL((dma_channel_enum)tim->DMA_Stream) & DMA_CHXCTL_CHEN)
    return false;

  timer_disable(timer_periph);
  timer_interrupt_disable(timer_periph, TIMER_INT_UP);

  return true;
}

static void set_compare_reg(const gd32_pulse_timer_t* tim, uint32_t val)
{
  timer_channel_output_pulse_value_config(_get_timer_periph(tim), tim->TIM_Channel, val);
}

void gd32_pulse_wait_for_completed(const gd32_pulse_timer_t* tim)
{
  uint32_t timer_periph = _get_timer_periph(tim);
  while (TIMER_CTL0(timer_periph) & TIMER_CTL0_CEN);
}

void gd32_pulse_start_dma_req(const gd32_pulse_timer_t* tim,
                               const void* pulses, uint16_t length,
                               uint32_t ocmode, uint32_t cmp_val)
{
  uint32_t timer_periph = _get_timer_periph(tim);
  dma_channel_enum ch = (dma_channel_enum)tim->DMA_Stream;

  set_compare_reg(tim, cmp_val);
  timer_channel_output_mode_config(timer_periph, tim->TIM_Channel, (uint16_t)ocmode);

  dma_memory_address_config(ch, (uint32_t)pulses);
  dma_transfer_number_config(ch, length);

  dma_interrupt_enable(ch, DMA_INT_FTF);

  timer_counter_value_config(timer_periph, 0xFFFF);
  timer_event_software_generate(timer_periph, TIMER_EVENT_SRC_UPG);

  timer_dma_enable(timer_periph, TIMER_DMA_UPD);
  dma_channel_enable(ch);

  timer_enable(timer_periph);
}

void gd32_pulse_dma_tc_isr(const gd32_pulse_timer_t* tim)
{
  dma_channel_enum ch = (dma_channel_enum)tim->DMA_Stream;

  if (dma_flag_get(ch, DMA_FLAG_FTF) != SET)
    return;

  dma_flag_clear(ch, DMA_FLAG_FTF);

  if (tim->DMA_TC_CallbackPtr) {
    auto closure = tim->DMA_TC_CallbackPtr;
    if (closure->cb && closure->cb(closure->ctx)) {
      return;
    }
  }

  uint32_t timer_periph = _get_timer_periph(tim);
  timer_flag_clear(timer_periph, TIMER_FLAG_UP);
  timer_interrupt_enable(timer_periph, TIMER_INT_UP);

  set_compare_reg(tim, 0);
  timer_channel_output_mode_config(timer_periph, tim->TIM_Channel, TIMER_OC_MODE_PWM1);
}

void gd32_pulse_tim_update_isr(const gd32_pulse_timer_t* tim)
{
  uint32_t timer_periph = _get_timer_periph(tim);

  if (timer_flag_get(timer_periph, TIMER_FLAG_UP) != SET)
    return;

  timer_flag_clear(timer_periph, TIMER_FLAG_UP);
  timer_interrupt_disable(timer_periph, TIMER_INT_UP);

  timer_channel_output_mode_config(timer_periph, tim->TIM_Channel, TIMER_OC_MODE_INACTIVE);
  timer_disable(timer_periph);
}

void gd32_pulse_config_input(const gd32_pulse_timer_t* tim)
{
  uint32_t timer_periph = _get_timer_periph(tim);

  timer_ic_parameter_struct icInit;
  timer_channel_input_struct_para_init(&icInit);
  icInit.icpolarity  = TIMER_IC_POLARITY_RISING;
  icInit.icselection = TIMER_IC_SELECTION_DIRECTTI;
  icInit.icprescaler = TIMER_IC_PSC_DIV1;
  icInit.icfilter    = 0;

  timer_input_capture_config(timer_periph, tim->TIM_Channel, &icInit);
}
