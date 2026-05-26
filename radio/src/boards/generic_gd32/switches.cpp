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

#include "hal/switch_driver.h"
#include "gd32_switch_driver.h"
#include "gd32_gpio_driver.h"

#include "definitions.h"
#include "myeeprom.h"

#include "gd32_switches.inc"

#include <stdlib.h>

__weak void boardInitSwitches()
{
  _init_switches();
}

__weak SwitchHwPos boardSwitchGetPosition(SwitchCategory cat, uint8_t idx)
{
  const gd32_switch_t* sw = &_switch_offsets[cat][idx];
  return gd32_switch_get_position(sw);
}

__weak const char* boardSwitchGetName(SwitchCategory cat, uint8_t idx)
{
  return _switch_name_offsets[cat][idx];
}

__weak SwitchHwType boardSwitchGetType(SwitchCategory cat, uint8_t idx)
{
  return _switch_offsets[cat][idx].type;
}

uint8_t boardGetMaxSwitches() { return n_switches; }
uint8_t boardGetMaxFctSwitches() { return n_fct_switches; }

swconfig_t boardSwitchGetDefaultConfig() { return _switch_default_config; }

switch_display_pos_t switchGetDisplayPosition(uint8_t idx)
{
  if (idx >= DIM(_switch_display)) return {0, 0};
  return _switch_display[idx];
}
