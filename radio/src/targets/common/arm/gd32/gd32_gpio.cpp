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
#include "gd32_exti_driver.h"
#include "gd32_gpio_driver.h"
#include "gd32_stdlib.h"

// [ ] TODO: 用直接寄存器访问代替GD32 SPL, 里面的for循环太SB了
static inline void _enable_clock(GPIO_TypeDef* port)
{
  uint32_t reg_idx = (((uint32_t)port) - GPIOA_BASE) / 0x0400UL;
#if defined(RCU_AHBEN_PAEN)
  rcu_periph_clock_enable((rcu_periph_enum)(RCU_GPIOA + reg_idx));
#else
  #error "Unsupported GPIO clock"
#endif
}

void gpio_init(gpio_t pin, gpio_mode_t mode, gpio_speed_t speed)
{
  GPIO_TypeDef* port = _GPIO_GET_PORT(pin);
  uint32_t pin_bit = _GPIO_GET_PINNUM(pin);

  _enable_clock(port);

  gpio_mode_set((uint32_t)port, _GPIO_GET_IOMODE(mode), _GPIO_GET_PUPD(mode), pin_bit);

  gpio_output_options_set((uint32_t)port, _GPIO_GET_OTYPE(mode), speed, pin_bit);
}

void gpio_init_int(gpio_t pin, gpio_mode_t mode, gpio_flank_t flank, gpio_cb_t cb)
{
  int port_num = _GPIO_GET_PORTNUM(pin);
  int pin_num = _GPIO_GET_PINNUM(pin);

  gpio_init(pin, mode, GPIO_PIN_SPEED_LOW);

// EXTI clock moved to gd32_exti_enable
//   // enable specific pin as exti sources
// #if defined(LL_APB4_GRP1_PERIPH_SYSCFG)
//   LL_APB4_GRP1_EnableClock(LL_APB4_GRP1_PERIPH_SYSCFG);
// #elif defined(LL_APB2_GRP1_PERIPH_SYSCFG)
//   LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
// #elif defined(LL_APB4_GRP1_PERIPH_SBS)
// # define SYSCFG SBS
//   LL_APB4_GRP1_EnableClock(LL_APB4_GRP1_PERIPH_SBS);
// #else
//   #error "Unsupported SYSCFG clock"
// #endif

  // SYSCFG->EXTICR[pin_num >> 2] &= ~(0xf << ((pin_num & 0x03) * 4));
  // SYSCFG->EXTICR[pin_num >> 2] |= (port_num << ((pin_num & 0x03) * 4));
  syscfg_exti_line_config(port_num, pin_num);

  gd32_exti_enable((exti_line_enum)(1 << pin_num), (exti_trig_type_enum)flank, cb);
}

void gpio_int_disable(gpio_t pin)
{
  int pin_num = _GPIO_GET_PINNUM(pin);
  gd32_exti_disable((exti_line_enum)BIT(pin_num));
}

void gpio_init_af(gpio_t pin, gpio_af_t af, gpio_speed_t speed)
{
  GPIO_TypeDef* port = _GPIO_GET_PORT(pin);
  uint32_t pin_bit = _GPIO_GET_PIN(pin);

  _enable_clock(port);

  // // Clear PUPDR
  // port->PUPDR &= ~(0x3 << (2 * pin_num));

  // // set selected function
  // port->AFR[(pin_num > 7) ? 1 : 0] &= ~(0xf << ((pin_num & 0x07) * 4));
  // port->AFR[(pin_num > 7) ? 1 : 0] |= (af << ((pin_num & 0x07) * 4));
  // // pin speed to max
  // port->OSPEEDR |= (speed << (2 * pin_num));

  // _set_mode(port, pin_num, 2);
  gpio_mode_set((uint32_t)port, GPIO_MODE_AF, GPIO_PUPD_NONE, pin_bit);
  gpio_output_options_set((uint32_t)port, GPIO_OTYPE_PP, speed, pin_bit); // default PP mode. OD mode use gpio_set_af
  gpio_af_set((uint32_t)port, af, pin_bit);
}

void gpio_set_af(gpio_t pin, gpio_af_t af)
{
  GPIO_TypeDef* port = _GPIO_GET_PORT(pin);
  uint32_t pin_bit = _GPIO_GET_PIN(pin);
  int pin_num = _GPIO_GET_PINNUM(pin);

  GPIO_CTL(port) &= ~GPIO_MODE_MASK(pin_num);
  GPIO_CTL(port) |= GPIO_MODE_SET(pin_num, GPIO_MODE_AF);

  gpio_af_set((uint32_t)port, af, pin_bit);
}

void gpio_init_analog(gpio_t pin)
{
  GPIO_TypeDef* port = _GPIO_GET_PORT(pin);
  uint32_t pin_bit = _GPIO_GET_PIN(pin);

  _enable_clock(port);
  // GPIO_CTL(port) &= ~GPIO_MODE_MASK(pin_num);
  // GPIO_CTL(port) |= GPIO_MODE_SET(pin_num, GPIO_MODE_AF);
  // // PUPD has to be 0b00
  // port->PUPDR &= ~(0x3 << (2 * pin_num));
  gpio_mode_set((uint32_t)port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, pin_bit);
}

gpio_mode_t gpio_get_mode(gpio_t pin)
{
  GPIO_TypeDef* port = _GPIO_GET_PORT(pin);
  int pin_num = _GPIO_GET_PINNUM(pin);
  return (port->CTL >> (2U * pin_num)) & 0x03;
}

int gpio_read(gpio_t pin)
{
  return (_GPIO_GET_PORT(pin)->ISTAT & _GPIO_GET_PIN(pin));
}

void gpio_set(gpio_t pin)
{
  _GPIO_GET_PORT(pin)->BOP = _GPIO_GET_PIN(pin);
}

void gpio_clear(gpio_t pin)
{
  _GPIO_GET_PORT(pin)->BOP = (_GPIO_GET_PIN(pin) << 16U);
}

void gpio_toggle(gpio_t pin)
{
  _GPIO_GET_PORT(pin)->TG = _GPIO_GET_PIN(pin);
}

void gpio_write(gpio_t pin, int value)
{
  if (value)
    gpio_set(pin);
  else
    gpio_clear(pin);
}

GPIO_TypeDef* gpio_get_port(gpio_t pin)
{
  return _GPIO_GET_PORT(pin);
}

uint32_t gpio_get_pin(gpio_t pin)
{
  return _GPIO_GET_PINNUM(pin);
}
