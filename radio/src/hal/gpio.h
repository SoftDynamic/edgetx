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

typedef uint32_t gpio_t;

typedef uint8_t  gpio_mode_t;
typedef uint8_t  gpio_af_t;

#if defined(GD32F3x0)
#include "gd32f3x0.h"
typedef uint32_t  gpio_speed_t; // GD32F3x0 has OSPD0 & OSPD1
#else
typedef uint8_t  gpio_speed_t;
#endif

#if defined(EXTI_TRIGGER_RISING)
typedef enum {
    GPIO_RISING = EXTI_TRIGGER_RISING,
    GPIO_FALLING = EXTI_TRIGGER_FALLING,
    GPIO_BOTH = EXTI_TRIGGER_RISING_FALLING
} gpio_flank_t;
#else
typedef enum {
    GPIO_RISING = EXTI_TRIG_RISING,
    GPIO_FALLING = EXTI_TRIG_FALLING,
    GPIO_BOTH = EXTI_TRIG_BOTH
} gpio_flank_t;
#endif

typedef void (*gpio_cb_t)();

void gpio_init(gpio_t pin, gpio_mode_t mode, gpio_speed_t speed);
void gpio_init_af(gpio_t pin, gpio_af_t af, gpio_speed_t speed);
void gpio_init_int(gpio_t pin, gpio_mode_t mode, gpio_flank_t flank, gpio_cb_t cb);
void gpio_init_analog(gpio_t pin);

void gpio_int_disable(gpio_t pin);
void gpio_set_af(gpio_t pin, gpio_af_t af);

gpio_mode_t gpio_get_mode(gpio_t pin);

int gpio_read(gpio_t pin);
void gpio_set(gpio_t pin);
void gpio_clear(gpio_t pin);
void gpio_write(gpio_t pin, int value);
void gpio_toggle(gpio_t pin);
