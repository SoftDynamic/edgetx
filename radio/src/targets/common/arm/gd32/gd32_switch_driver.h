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

#include "gd32_stdlib.h"
#include "hal/switch_driver.h"

#include <stdint.h>
#include <stdbool.h>

struct gd32_switch_t {
  GPIO_TypeDef* GPIOx_high;
  uint32_t      Pin_high;
  GPIO_TypeDef* GPIOx_low;
  uint32_t      Pin_low;
  uint8_t       type;
  uint8_t       flags;
  uint8_t       adc_input;
};

SwitchHwPos gd32_switch_get_position(const gd32_switch_t* sw);
bool gd32_switch_get_state(const gd32_switch_t* sw, SwitchHwPos pos);
