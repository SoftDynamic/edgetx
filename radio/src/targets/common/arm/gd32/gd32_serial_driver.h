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

#include "gd32_usart_driver.h"
#include "hal/serial_port.h"

struct gd32_serial_buffer {
  uint8_t* buffer;
  uint32_t length;
};

struct gd32_serial_port {
  const gd32_usart_t* usart;
  gd32_serial_buffer  tx_buffer;
  gd32_serial_buffer  rx_buffer;
};

#define DEFINE_GD32_SERIAL_PORT(name, _usart, _tx_buf, _tx_len, _rx_buf, _rx_len) \
  gd32_serial_buffer name##_tx_buf = { .buffer = _tx_buf, .length = _tx_len };    \
  gd32_serial_buffer name##_rx_buf = { .buffer = _rx_buf, .length = _rx_len };    \
  const gd32_serial_port name = {                                                   \
    .usart = &_usart,                                                                \
    .tx_buffer = name##_tx_buf,                                                      \
    .rx_buffer = name##_rx_buf,                                                      \
  }

void gd32_serial_init_driver();

extern const etx_serial_driver_t GD32SerialDriver;
