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

#include "gd32_timer.h"
#include "gd32_stdlib.h"

void gd32_timer_enable_clock(TIMER_TypeDef *TIMx)
{
#if defined(GD32F3x0)
  uint32_t base = (uint32_t)TIMx;
  // APB1 timers: TIMER1=0x40000000, TIMER2=0x40000400, TIMER13=0x40002000
  if (base == TIMER1)
    rcu_periph_clock_enable(RCU_TIMER1);
  else if (base == TIMER2)
    rcu_periph_clock_enable(RCU_TIMER2);
  else if (base == TIMER13)
    rcu_periph_clock_enable(RCU_TIMER13);
  // APB2 timers: TIMER0=0x40012C00, TIMER14=0x40014000, TIMER15=0x40014400, TIMER16=0x40014800
  else if (base == TIMER0)
    rcu_periph_clock_enable(RCU_TIMER0);
  else if (base == TIMER14)
    rcu_periph_clock_enable(RCU_TIMER14);
  else if (base == TIMER15)
    rcu_periph_clock_enable(RCU_TIMER15);
  else if (base == TIMER16)
    rcu_periph_clock_enable(RCU_TIMER16);
#endif
}

void gd32_timer_disable_clock(TIMER_TypeDef *TIMx)
{
#if defined(GD32F3x0)
  uint32_t base = (uint32_t)TIMx;
  if (base == TIMER1)
    rcu_periph_clock_disable(RCU_TIMER1);
  else if (base == TIMER2)
    rcu_periph_clock_disable(RCU_TIMER2);
  else if (base == TIMER13)
    rcu_periph_clock_disable(RCU_TIMER13);
  else if (base == TIMER0)
    rcu_periph_clock_disable(RCU_TIMER0);
  else if (base == TIMER14)
    rcu_periph_clock_disable(RCU_TIMER14);
  else if (base == TIMER15)
    rcu_periph_clock_disable(RCU_TIMER15);
  else if (base == TIMER16)
    rcu_periph_clock_disable(RCU_TIMER16);
#endif
}

bool gd32_timer_is_clock_enabled(TIMER_TypeDef *TIMx)
{
#if defined(GD32F3x0)
  uint32_t base = (uint32_t)TIMx;
  rcu_periph_clock_enable(RCU_TIMER1); // dummy just to compile
  (void)base;
  return true;
#else
  return false;
#endif
}
