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

#include "gd32_usart_driver.h"
#include "hal/gpio.h"
#include "gd32_dma.h"
#include "gd32_gpio.h"
#include "gd32_stdlib.h"

#include <string.h>

// NVIC_GetEnableIRQ helper for older CMSIS
#if !defined(NVIC_GetEnableIRQ)
static inline uint32_t NVIC_GetEnableIRQ(IRQn_Type IRQn)
{
  if ((int32_t)(IRQn) >= 0) {
    return ((uint32_t)(((NVIC->ISER[(((uint32_t)(int32_t)IRQn) >> 5UL)] &
                         (1UL << (((uint32_t)(int32_t)IRQn) & 0x1FUL))) != 0UL)
                            ? 1UL
                            : 0UL));
  } else {
    return 0U;
  }
}
#endif

static inline uint32_t _usart_periph(USART_TypeDef* USARTx)
{
  return (uint32_t)USARTx;
}

static uint32_t _get_usart_clock(USART_TypeDef* USARTx)
{
  // GD32F3x0: USART0 on APB2, USART1 on APB1
  uint32_t periph = _usart_periph(USARTx);
  if (periph == USART0)
    return rcu_clock_freq_get(CK_APB2);
  else
    return rcu_clock_freq_get(CK_APB1);
}

void gd32_usart_enable_clock(USART_TypeDef* USARTx)
{
  uint32_t periph = _usart_periph(USARTx);
  if (periph == USART0)
    rcu_periph_clock_enable(RCU_USART0);
  else if (periph == USART1)
    rcu_periph_clock_enable(RCU_USART1);
}

void gd32_usart_deinit(USART_TypeDef* USARTx)
{
  uint32_t periph = _usart_periph(USARTx);
  usart_disable(periph);
  usart_deinit(periph);
}

static void _usart_init_gpio(const gd32_usart_t* usart)
{
  uint32_t af = GPIO_AF_1; // USART0/1 default AF1

  // TX pin
  if (usart->txGPIO != GPIO_UNDEF)
    gpio_init_af(usart->txGPIO, af, GPIO_OSPEED_50MHZ);
  // RX pin
  if (usart->rxGPIO != GPIO_UNDEF)
    gpio_init_af(usart->rxGPIO, af, GPIO_OSPEED_50MHZ);
}

bool gd32_usart_init(const gd32_usart_t* usart, const etx_serial_init* params)
{
  uint32_t periph = _usart_periph(usart->USARTx);

  gd32_usart_enable_clock(usart->USARTx);

  // Configure GPIOs
  _usart_init_gpio(usart);

  // Deinit
  usart_deinit(periph);

  // Compute baudrate from clock (8N1 default)
  uint32_t pclk = _get_usart_clock(usart->USARTx);
  uint32_t baud_div = (pclk + params->baudrate / 2) / params->baudrate;

  usart_baudrate_set(periph, baud_div);
  usart_word_length_set(periph, USART_WL_8BIT);
  usart_parity_config(periph, USART_PM_NONE);
  usart_stop_bit_set(periph, USART_STB_1BIT);

  // Enable USART
  usart_receive_config(periph, USART_RECEIVE_ENABLE);
  usart_transmit_config(periph, USART_TRANSMIT_ENABLE);
  usart_enable(periph);

  // Enable RXNE interrupt
  usart_interrupt_enable(periph, USART_INT_RBNE);

  return true;
}

bool gd32_usart_init_rx_dma(const gd32_usart_t* usart,
                              uint8_t* buffer, uint32_t length)
{
  if (!usart->rxDMA) return false;

  uint32_t periph = _usart_periph(usart->USARTx);
  gd32_dma_enable_clock(usart->rxDMA);

  dma_parameter_struct dmaInit;
  dma_struct_para_init(&dmaInit);

  dmaInit.periph_addr  = (uint32_t)(periph + 0x24U); // USART_RDATA
  dmaInit.memory_addr  = (uint32_t)buffer;
  dmaInit.number       = length;
  dmaInit.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
  dmaInit.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
  dmaInit.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
  dmaInit.memory_width = DMA_MEMORY_WIDTH_8BIT;
  dmaInit.direction    = DMA_PERIPHERAL_TO_MEMORY;
  dmaInit.priority     = DMA_PRIORITY_HIGH;
  dma_init(usart->rxDMA_Stream, &dmaInit);

  dma_circulation_enable(usart->rxDMA_Stream);
  dma_channel_enable(usart->rxDMA_Stream);

  usart_dma_receive_config(periph, 1);

  return true;
}

void gd32_usart_send_byte(USART_TypeDef* USARTx, uint8_t data)
{
  uint32_t periph = _usart_periph(USARTx);
  while (!usart_flag_get(periph, USART_FLAG_TBE));
  usart_data_transmit(periph, data);
}

void gd32_usart_send_buffer(USART_TypeDef* USARTx,
                              const uint8_t* data, uint32_t length)
{
  if (!length) return;

  uint32_t periph = _usart_periph(USARTx);

  if (periph != USART1) {
    // Use DMA TX for USART1 if available
    // For now, fall back to byte-by-byte
    for (uint32_t i = 0; i < length; i++) {
      gd32_usart_send_byte(USARTx, data[i]);
    }
    return;
  }

  // DMA TX
  if (periph == USART1) {
    // TODO: implement DMA TX properly
    for (uint32_t i = 0; i < length; i++) {
      gd32_usart_send_byte(USARTx, data[i]);
    }
  }
}

bool gd32_usart_tx_completed(USART_TypeDef* USARTx)
{
  uint32_t periph = _usart_periph(USARTx);
  return usart_flag_get(periph, USART_FLAG_TC) == SET;
}

void gd32_usart_enable_tx_irq(USART_TypeDef* USARTx)
{
  uint32_t periph = _usart_periph(USARTx);
  usart_interrupt_enable(periph, USART_INT_TBE);
}

void gd32_usart_enable_rx(USART_TypeDef* USARTx)
{
  uint32_t periph = _usart_periph(USARTx);
  usart_interrupt_enable(periph, USART_INT_RBNE);
}

void gd32_usart_set_baudrate(USART_TypeDef* USARTx, uint32_t baudrate)
{
  uint32_t periph = _usart_periph(USARTx);
  uint32_t pclk = _get_usart_clock(USARTx);
  uint32_t baud_div = (pclk + baudrate / 2) / baudrate;
  usart_baudrate_set(periph, baud_div);
}

uint32_t gd32_usart_get_baudrate(USART_TypeDef* USARTx)
{
  uint32_t periph = _usart_periph(USARTx);
  uint32_t pclk = _get_usart_clock(USARTx);
  uint32_t baud_div = USART_BAUD(periph);
  if (baud_div == 0) return 0;
  return pclk / baud_div;
}

void gd32_usart_set_hw_option(USART_TypeDef* USARTx, uint32_t option)
{
  uint32_t periph = _usart_periph(USARTx);
  if (option == USART_OPTION_HALF_DUPLEX) {
    usart_halfduplex_enable(periph);
  }
}

void gd32_usart_set_idle_irq(USART_TypeDef* USARTx, bool enabled)
{
  uint32_t periph = _usart_periph(USARTx);
  if (enabled)
    usart_interrupt_enable(periph, USART_INT_IDLE);
  else
    usart_interrupt_disable(periph, USART_INT_IDLE);
}

void gd32_usart_isr(USART_TypeDef* USARTx, etx_serial_callbacks_t* cb)
{
  uint32_t periph = _usart_periph(USARTx);

  // RXNE
  if (usart_interrupt_flag_get(periph, USART_INT_FLAG_RBNE) != RESET) {
    uint8_t data = (uint8_t)usart_data_receive(periph);
    if (cb && cb->on_receive)
      cb->on_receive(data);
    usart_interrupt_flag_clear(periph, USART_INT_FLAG_RBNE);
  }

  // TXE
  if (usart_interrupt_flag_get(periph, USART_INT_FLAG_TBE) != RESET) {
    uint8_t data;
    if (cb && cb->on_send && cb->on_send(&data)) {
      usart_data_transmit(periph, data);
    } else {
      usart_interrupt_disable(periph, USART_INT_TBE);
    }
    usart_interrupt_flag_clear(periph, USART_INT_FLAG_TBE);
  }

  // IDLE
  if (usart_interrupt_flag_get(periph, USART_INT_FLAG_IDLE) != RESET) {
    usart_data_receive(periph); // clear IDLE by reading data
    if (cb && cb->on_idle)
      cb->on_idle(cb->on_idle_ctx);
  }

  // TC
  if (usart_interrupt_flag_get(periph, USART_INT_FLAG_TC) != RESET) {
    usart_interrupt_flag_clear(periph, USART_INT_FLAG_TC);
  }
}
