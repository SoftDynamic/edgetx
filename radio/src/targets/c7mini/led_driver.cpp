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

#include "hal/gpio.h"
#include "gd32_gpio.h"
#include "board.h"

void ledInit()
{
#if defined(POWER_LED_STANDALONG)
  gpio_init(LED_PWR_GPIO, GPIO_OUT, GPIO_PIN_SPEED_LOW);
#endif

#if defined(RF_LEDS)
  gpio_init(LED_RF_GPIO, GPIO_OUT, GPIO_PIN_SPEED_LOW);
#endif
}

void ledOff()
{
#if defined(POWER_LED_STANDALONG)
  GPIO_LED_GPIO_OFF(LED_PWR_GPIO);
#endif
#if defined(RF_LEDS)
  GPIO_LED_GPIO_OFF(LED_RF_GPIO);
#endif
}

void ledPwr()
{
  ledOff();
#if defined(LED_PWR_GPIO)
  GPIO_LED_GPIO_ON(LED_PWR_GPIO);
#endif
}

void ledRf()
{
  ledOff();
#if defined(LED_RF_GPIO)
  GPIO_LED_GPIO_ON(LED_RF_GPIO);
#endif
}
