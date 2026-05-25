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

#include "timers_driver.h"
#include "gd32_timer.h"

#include "hal.h"
#include "hal/watchdog_driver.h"

static volatile uint32_t _ms_ticks;

static void _init_1ms_timer()
{
  gd32_timer_enable_clock((TIMER_TypeDef *)MS_TIMER);
  if ((((TIMER_TypeDef *)MS_TIMER)->CTL0 & TIMER_CTL0_CEN) == TIMER_CTL0_CEN) return;

  _ms_ticks = 0;
  ((TIMER_TypeDef *)MS_TIMER)->CAR = 999; // 1mS in uS
  ((TIMER_TypeDef *)MS_TIMER)->PSC = (PERI1_FREQUENCY * TIMER_MULT_APB1) / 1000000 - 1;  // 1uS
  ((TIMER_TypeDef *)MS_TIMER)->CHCTL2 = 0;
  ((TIMER_TypeDef *)MS_TIMER)->CHCTL0 = 0;
  ((TIMER_TypeDef *)MS_TIMER)->SWEVG = 0;
  ((TIMER_TypeDef *)MS_TIMER)->CTL0 = TIMER_CTL0_CEN | TIMER_CTL0_UPS;
  ((TIMER_TypeDef *)MS_TIMER)->DMAINTEN = TIMER_DMAINTEN_UPDEN;

  NVIC_EnableIRQ(MS_TIMER_IRQn);
  NVIC_SetPriority(MS_TIMER_IRQn, 0);
}

void timersInit()
{
  _init_1ms_timer();
}

uint32_t timersGetMsTick()
{
  return _ms_ticks;
}

uint32_t timersGetUsTick()
{
  uint32_t ms;
  uint32_t us;

  do {
    ms = _ms_ticks;
    us = ((TIMER_TypeDef *)MS_TIMER)->CNT;
    asm volatile("nop");
    asm volatile("nop");
  } while (ms != _ms_ticks);

  return ms * 1000 + us;
}

static volatile uint32_t watchdogTimeout = 0;

void watchdogSuspend(uint32_t timeout)
{
  watchdogTimeout = timeout;
}

static inline void _interrupt_1ms()
{
  static uint8_t pre_scale = 0;

  ++pre_scale;
  ++_ms_ticks;

  __DSB();
  __ISB();

  // 5ms loop
  if (pre_scale == 5 || pre_scale == 10) {
    per5ms();
  }

  // 10ms loop
  if (pre_scale == 10) {
    pre_scale = 0;

    if (watchdogTimeout) {
      watchdogTimeout -= 1;
      WDG_RESET();
    }
  }
}

extern "C" void MS_TIMER_IRQHandler()
{
  ((TIMER_TypeDef *)MS_TIMER)->INTF &= ~TIMER_INTF_UPIF;
  _interrupt_1ms();
}
