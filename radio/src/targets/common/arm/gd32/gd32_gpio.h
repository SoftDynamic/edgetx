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

#include "gd32_cmsis.h"
#include "hal/gpio.h"

#define GPIO_UNDEF (0xffffffff)

#if defined(GD32F3x0)
  #define GPIO_BITS_MODE 2
  #define GPIO_BITS_SPEED 2
  #define GPIO_BITS_OTYPE 1
  #define GPIO_BITS_PUPD 2

  #define GPIO_BITS_PIN 10
  #define GPIO_BITS_PORT (32 - (GPIO_BITS_PIN))
#endif

#if defined(GPIO_BITS_PIN)
  typedef union {
    struct {
      uint32_t pin : GPIO_BITS_PIN;
      uint32_t port : GPIO_BITS_PORT;
    }__attribute__ ((__packed__));
    gpio_t to_gpio_t;
  } __gpio_t;

  #define GPIO_PIN(x, y) ((__gpio_t){.pin = (y), .port = ((uintptr_t)(x) >> GPIO_BITS_PIN)}).to_gpio_t
  #define _GPIO_GET_PINNUM(x) ((__gpio_t){.to_gpio_t = (x)}.pin)
  #define _GPIO_GET_PIN(x) BIT(__gpio_t{.to_gpio_t = (x)}.pin)
  #define _GPIO_GET_PORT(x) ((GPIO_TypeDef *)(__gpio_t{.to_gpio_t = pin}.port << GPIO_BITS_PIN))
  #define _GPIO_GET_PORTNUM(x) (__gpio_t{.to_gpio_t = pin}.port - (GPIOA_BASE >> GPIO_BITS_PIN))
#else
  #define GPIO_PIN(x, y) ((uintptr_t)x | y)
  #define _GPIO_GET_PINNUM(x) (x & 0x0fUL)
  #define _GPIO_GET_PIN(x) BIT(x & 0x0fUL)
  #define _GPIO_GET_PORT(x) (GPIO_TypeDef*)(x & ~(0x0fUL))
  #define _GPIO_GET_PORTNUM(x) (((x & ~(0x0fUL)) - GPIOA_BASE) / 0x0400UL)
#endif

#if defined(GPIO_BITS_MODE)
  typedef union {
    struct {
      uint8_t mode : GPIO_BITS_MODE;
      uint8_t otype : GPIO_BITS_OTYPE;
      uint8_t pupd : GPIO_BITS_PUPD;
    }__attribute__ ((__packed__));
    gpio_mode_t to_gpio_mode_t;
  } __gpio_mode_t;

  #if defined(__cplusplus)
  #define _GPIO_MODE(mode, pupd, otype) (((mode) & 0x03) | (((pupd) & 0x03) << 2) | (((otype) & 0x01) << 4))
  #else
  #define _GPIO_MODE(mode, pupd, otype) ((__gpio_mode_t){\
    .mode = (mode), .otype = (otype), .pupd = (pupd)}).to_gpio_mode_t
  #endif
  
  #define _GPIO_GET_IOMODE(x) (__gpio_mode_t){.to_gpio_mode_t = x}.mode
  #define _GPIO_GET_OTYPE(x) (__gpio_mode_t){.to_gpio_mode_t = x}.otype
  #define _GPIO_GET_PUPD(x) (__gpio_mode_t){.to_gpio_mode_t = x}.pupd
  
  enum {
    GPIO_IN = _GPIO_MODE(GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP),     // input w/o pull R
    GPIO_IN_PD = _GPIO_MODE(GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_OTYPE_PP),  // input with pull-down
    GPIO_IN_PU = _GPIO_MODE(GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_OTYPE_PP),  // input with pull-up
    GPIO_OUT = _GPIO_MODE(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP),    // push-pull output
    GPIO_OD = _GPIO_MODE(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_OD),     // open-drain w/o pull R
    GPIO_OD_PU = _GPIO_MODE(GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_OTYPE_OD)   // open-drain with pull-up
  };
#else

// Generate GPIO mode bitfields
//
// bit 0+1: pin mode (input / output)
// bit 2+3: pull resistor configuration
// bit   4: output type (0: push-pull, 1: open-drain)
//
#define _GPIO_MODE(io, pr, ot) ((io << 0) | (pr << 2) | (ot << 4))
#define _GPIO_GET_IOMODE(x) ((x >> 0) & 0x03)
#define _GPIO_GET_OTYPE(x) ((x >> 4) & 0x01)
#define _GPIO_GET_PUPD(x) ((x >> 2) & 0x03)

enum {
  GPIO_IN = _GPIO_MODE(0, 0, 0),     // input w/o pull R
  GPIO_IN_PD = _GPIO_MODE(0, 2, 0),  // input with pull-down
  GPIO_IN_PU = _GPIO_MODE(0, 1, 0),  // input with pull-up
  GPIO_OUT = _GPIO_MODE(1, 0, 0),    // push-pull output
  GPIO_OD = _GPIO_MODE(1, 0, 1),     // open-drain w/o pull R
  GPIO_OD_PU = _GPIO_MODE(1, 1, 1)   // open-drain with pull-up
};

#endif

enum {
  GPIO_AF0 = GPIO_AF_0,
  GPIO_AF1,
  GPIO_AF2,
  GPIO_AF3,
  GPIO_AF4,
  GPIO_AF5,
  GPIO_AF6,
  GPIO_AF7,
  // GPIO_AF8,
  // GPIO_AF9,
  // GPIO_AF10,
  // GPIO_AF11,
  // GPIO_AF12,
  // GPIO_AF13,
  // GPIO_AF14,
  // GPIO_AF15,
  GPIO_AF_UNDEF
};

enum {
  GPIO_PIN_SPEED_LOW = GPIO_OSPEED_2MHZ,
  GPIO_PIN_SPEED_MEDIUM = GPIO_OSPEED_10MHZ,
  GPIO_PIN_SPEED_HIGH = GPIO_OSPEED_50MHZ,
  GPIO_PIN_SPEED_VERY_HIGH = GPIO_OSPEED_MAX,
};

GPIO_TypeDef* gpio_get_port(gpio_t pin);
uint32_t gpio_get_pin(gpio_t pin); // PIN number! NOT bitmask
