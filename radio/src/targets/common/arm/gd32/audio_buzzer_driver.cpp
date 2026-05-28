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

#include "board.h"
#include "hal.h"
#include "gd32_pulse_driver.h"
#include "gd32_gpio.h"
#include "gd32_timer.h"

#include "dataconstants.h"

extern uint8_t g_beepCnt;

#define BUZZER_PWM_FREQ      2500
#define BUZZER_TIMER_FREQ    2000000

#define BUZZER_ARR_VAL       (BUZZER_TIMER_FREQ / BUZZER_PWM_FREQ - 1)
#define BUZZER_CMP_50        (BUZZER_ARR_VAL / 2)
#define BUZZER_CMP_OFF       0

#define MAX_BEEP_DURATION    250

static const gd32_pulse_timer_t _buzzer_timer = {
  .GPIO = AUDIO_OUTPUT_GPIO,
  .GPIO_Alternate = AUDIO_GPIO_AF,
  .TIMx = (TIMER_TypeDef *)AUDIO_TIMER,
  .TIM_Freq = AUDIO_TIMER_FREQ,
  .TIM_Channel = AUDIO_TIMER_CHANNEL,
  .TIM_IRQn = (IRQn_Type)-1,
  .DMAx = nullptr,
  .DMA_Stream = 0,
  .DMA_Channel = 0,
  .DMA_IRQn = (IRQn_Type)-1,
  .DMA_TC_CallbackPtr = nullptr,
};

void audioInit()
{
  gd32_pulse_init(&_buzzer_timer, BUZZER_TIMER_FREQ);
  gd32_pulse_config_output(&_buzzer_timer, true, TIMER_OC_MODE_PWM0, BUZZER_CMP_OFF);
  gd32_pulse_set_period(&_buzzer_timer, BUZZER_ARR_VAL + 1);
  gd32_pulse_start(&_buzzer_timer);
}

void audioEnd()
{
  gd32_pulse_stop(&_buzzer_timer);
  gd32_pulse_deinit(&_buzzer_timer);
}

void buzzerSound(uint8_t b)
{
  if (b > 0) {
    g_beepCnt = (b > MAX_BEEP_DURATION) ? MAX_BEEP_DURATION : b;
    gd32_pulse_set_cmp_val(&_buzzer_timer, BUZZER_CMP_50);
  } else {
    g_beepCnt = 0;
    gd32_pulse_set_cmp_val(&_buzzer_timer, BUZZER_CMP_OFF);
  }
}

void buzzerSound(uint8_t b, uint16_t freq)
{
  if (b > 0) {
    g_beepCnt = (b > MAX_BEEP_DURATION) ? MAX_BEEP_DURATION : b;
    gd32_pulse_set_period(&_buzzer_timer, (BUZZER_TIMER_FREQ / freq) - 1);
    gd32_pulse_set_cmp_val(&_buzzer_timer, ((BUZZER_TIMER_FREQ / freq) - 1) / 2);
  } else {
    g_beepCnt = 0;
    gd32_pulse_set_cmp_val(&_buzzer_timer, BUZZER_CMP_OFF);
  }
}

void per5ms()
{
  if (g_beepCnt == 0)
    return;
  if (--g_beepCnt == 0) {
    gd32_pulse_set_cmp_val(&_buzzer_timer, BUZZER_CMP_OFF);
  }
}
