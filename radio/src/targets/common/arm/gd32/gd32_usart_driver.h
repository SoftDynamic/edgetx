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
#include "hal/serial_driver.h"
#include "hal/gpio.h"

#include <stdint.h>
#include <stdbool.h>

#define GPIO_UNDEF ((gpio_t)0xFFFFFFFF)

#define USART_OPTION_HALF_DUPLEX  (1 << 0)

struct gd32_usart_t {
  USART_TypeDef*  USARTx;
  gpio_t          txGPIO;
  gpio_t          rxGPIO;
  gpio_t          set_input;
  DMA_TypeDef*    txDMA;
  dma_channel_enum txDMA_Stream;
  DMA_TypeDef*    rxDMA;
  dma_channel_enum rxDMA_Stream;
};

// GD32 USART DMA control bits in CTL2
#define USD_CTL2_DENT (1 << 1)  // DMA enable for transmission
#define USD_CTL2_DENR (1 << 0)  // DMA enable for reception

void gd32_usart_enable_clock(USART_TypeDef* USARTx);
void gd32_usart_deinit(USART_TypeDef* USARTx);
bool gd32_usart_init(const gd32_usart_t* usart, const etx_serial_init* params);

bool gd32_usart_init_rx_dma(const gd32_usart_t* usart,
                              uint8_t* buffer, uint32_t length);

void gd32_usart_send_byte(USART_TypeDef* USARTx, uint8_t data);
void gd32_usart_send_buffer(USART_TypeDef* USARTx,
                              const uint8_t* data, uint32_t length);

bool gd32_usart_tx_completed(USART_TypeDef* USARTx);
void gd32_usart_enable_tx_irq(USART_TypeDef* USARTx);
void gd32_usart_enable_rx(USART_TypeDef* USARTx);

void gd32_usart_set_baudrate(USART_TypeDef* USARTx, uint32_t baudrate);
uint32_t gd32_usart_get_baudrate(USART_TypeDef* USARTx);
void gd32_usart_set_hw_option(USART_TypeDef* USARTx, uint32_t option);
void gd32_usart_set_idle_irq(USART_TypeDef* USARTx, bool enabled);

void gd32_usart_isr(USART_TypeDef* USARTx, etx_serial_callbacks_t* cb);
