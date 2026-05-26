/*
 * Copyright (C) EdgeTx
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

#include "gd32_softserial_driver.h"
#include "gd32_exti_driver.h"
#include "gd32_timer.h"
#include "gd32_gpio.h"
#include "hal/gpio.h"

#include <string.h>

#define BITLEN 16

static uint8_t rxBitCount;
static uint8_t rxByte;

static volatile uint8_t rxRidx;
static volatile uint8_t rxWidx;
static uint8_t *rxBuffer;
static uint32_t rxBufLen;

static const gd32_softserial_rx_port* _softserialPort;

static void _softserial_exti()
{
  if (rxBitCount == 0) {
    auto port = _softserialPort;

    for (uint8_t i = 0; i < 16; ++i) {
      if (!gpio_read(port->GPIO)) return;
    }

    auto timer_periph = (uint32_t)port->TIMx;
    timer_autoreload_value_config(timer_periph, (BITLEN + BITLEN / 2) - 1);
    timer_enable(timer_periph);

    gd32_exti_disable((exti_line_enum)port->EXTI_Line);
  }
}

static inline void _rx_fifo_clear()
{
  rxWidx = 0;
  rxRidx = 0;
}

static bool _softserial_init_rx(const gd32_softserial_rx_port* port,
                                const etx_serial_init* params)
{
  if (gpio_get_mode(port->GPIO) != GPIO_IN) return false;

  rxBitCount = 0;
  rxBuffer = port->buffer.buffer;
  rxBufLen = port->buffer.length;
  _rx_fifo_clear();

  timer_parameter_struct timInit;
  timer_struct_para_init(&timInit);

  uint32_t freq = params->baudrate * 16;
  if (!freq) return false;

  timInit.prescaler = (port->TIM_Freq / freq) - 1;
  timInit.period = 0xFFFF;
  timInit.alignedmode = TIMER_COUNTER_EDGE;
  timInit.counterdirection = TIMER_COUNTER_UP;
  timInit.clockdivision = TIMER_CKDIV_DIV1;
  timInit.repetitioncounter = 0;

  gd32_timer_enable_clock(port->TIMx);
  timer_init((uint32_t)port->TIMx, &timInit);

  timer_flag_clear((uint32_t)port->TIMx, TIMER_FLAG_UP);
  timer_interrupt_enable((uint32_t)port->TIMx, TIMER_INT_UP);

  NVIC_SetPriority(port->TIM_IRQn, 0);
  NVIC_EnableIRQ(port->TIM_IRQn);

  if (port->dir_GPIO != GPIO_UNDEF) {
    gpio_init(port->dir_GPIO, GPIO_OUT, GPIO_PIN_SPEED_MEDIUM);
    if (!port->dir_Input) {
      gpio_clear(port->dir_GPIO);
    } else {
      gpio_set(port->dir_GPIO);
    }
  }

  _softserialPort = port;
  gpio_init_int(port->GPIO, GPIO_IN_PD, GPIO_RISING, _softserial_exti);
  return true;
}

static void _softserial_deinit_gpio(const gd32_softserial_rx_port* port)
{
  gpio_init(port->GPIO, GPIO_IN, GPIO_PIN_SPEED_LOW);
}

static void _softserial_deinit_rx(const gd32_softserial_rx_port* port)
{
  gd32_exti_disable((exti_line_enum)port->EXTI_Line);
  NVIC_DisableIRQ(port->TIM_IRQn);
  timer_disable((uint32_t)port->TIMx);
}

static void* gd32_softserial_rx_init(void* hw_def, const etx_serial_init* params)
{
  auto port = (const gd32_softserial_rx_port*)hw_def;
  if (!_softserial_init_rx(port, params)) return nullptr;
  return hw_def;
}

static void gd32_softserial_rx_deinit(void* ctx)
{
  auto port = (const gd32_softserial_rx_port*)ctx;

  if (port == _softserialPort) {
    _softserial_deinit_rx(port);
  }

  _softserial_deinit_gpio(port);
  _softserialPort = nullptr;
}

static inline bool _rx_fifo_full()
{
  return ((rxWidx + 1) & (rxBufLen - 1)) == rxRidx;
}

static inline void _rx_fifo_push(uint8_t c)
{
  rxBuffer[rxWidx] = c;
  rxWidx = (rxWidx + 1) & (rxBufLen - 1);
}

static int gd32_softserial_rx_get_byte(void* ctx, uint8_t* data)
{
  if (rxWidx == rxRidx) return 0;

  *data = rxBuffer[rxRidx];
  rxRidx = (rxRidx + 1) & (rxBufLen - 1);

  return 1;
}

void gd32_softserial_rx_timer_isr(const gd32_softserial_rx_port* port)
{
  auto timer_periph = (uint32_t)port->TIMx;
  timer_flag_clear(timer_periph, TIMER_FLAG_UP);

  if (rxBitCount < 8) {
    if (rxBitCount == 0) {
      timer_autoreload_value_config(timer_periph, BITLEN - 1);
      rxByte = 0;
    } else {
      rxByte >>= 1;
    }

    if (gpio_read(port->GPIO) == 0)
      rxByte |= 0x80;

    ++rxBitCount;
  } else if (rxBitCount == 8) {

    if (!_rx_fifo_full()) _rx_fifo_push(rxByte);
    rxBitCount = 0;

    timer_disable(timer_periph);

    exti_interrupt_enable((exti_line_enum)port->EXTI_Line);
  }
}

static void gd32_softserial_rx_clear_rx_buffer(void* ctx)
{
  (void)ctx;
  rxWidx = 0;
  rxRidx = 0;
}

const etx_serial_driver_t STM32SoftSerialRxDriver = {
  .init = gd32_softserial_rx_init,
  .deinit = gd32_softserial_rx_deinit,
  .sendByte = nullptr,
  .sendBuffer = nullptr,
  .waitForTxCompleted = nullptr,
  .enableRx = nullptr,
  .getByte = gd32_softserial_rx_get_byte,
  .getLastByte = nullptr,
  .clearRxBuffer = gd32_softserial_rx_clear_rx_buffer,
  .getBaudrate = nullptr,
  .setReceiveCb = nullptr,
  .setBaudrateCb = nullptr,
};


static inline void _set_level(gd32_softserial_tx_state* st, uint8_t v)
{
  *st->pulse_ptr = v - 1;
  ++st->pulse_ptr;
}

static void _conv_byte_8e2(gd32_softserial_tx_state* st, uint8_t b)
{
  bool lev = 0;
  uint8_t parity = 1;

  uint8_t len = BITLEN;
  for (uint8_t i = 0; i <= 9; i++) {

    bool nlev = b & 1;
    parity = parity ^ (uint8_t)nlev;

    if (lev == nlev) {
      len += BITLEN;
    } else {
      _set_level(st, len);
      len = BITLEN;
      lev = nlev;
    }

    b = (b >> 1) | 0x80;
    if (i == 7) b = b ^ parity;
  }

  _set_level(st, len + BITLEN);
  --st->serial_size;
}

static void _conv_byte_8n1(gd32_softserial_tx_state* st, uint8_t b)
{
  bool lev = 0;
  uint8_t len = BITLEN;
  for (uint8_t i = 0; i <= 8; i++) {

    bool nlev = b & 1;
    if (lev == nlev) {
      len += BITLEN;
    } else {
      _set_level(st, len);
      len = BITLEN;
      lev = nlev;
    }

    b = (b >> 1) | 0x80;
  }

  _set_level(st, len);
  --st->serial_size;
}

#define PXX1_FREQ      1000000
#define PXX1_PWM_ON    8
#define PXX1_BIT_ZERO  16
#define PXX1_BIT_ONE   24

__attribute__ ((weak)) uint32_t __pxx1_get_inverter_comp() { return 0; }

static void _conv_byte_pxx1(gd32_softserial_tx_state* st, uint8_t b)
{
  uint32_t bits = st->serial_size < 8 ? st->serial_size : 8;

  for (uint8_t i = 0; i < bits; i++) {
    if (b & 0x80)
      _set_level(st, PXX1_BIT_ONE);
    else
      _set_level(st, PXX1_BIT_ZERO);

    b <<= 1;
  }

  st->serial_size -= bits;
}

static void* gd32_softserial_tx_init(void* hw_def, const etx_serial_init* params)
{
  auto port = (const gd32_softserial_tx_port*)hw_def;

  auto st = port->st;
  memset(port->st, 0, sizeof(gd32_softserial_tx_state));

  bool polarity = params->polarity;
  uint32_t freq = params->baudrate * 16;
  uint32_t ocmode = TIMER_OC_MODE_TOGGLE;
  uint32_t cmp_val = 0;

  switch (params->encoding) {

  case ETX_Encoding_8N1:
    st->conv_byte = _conv_byte_8n1;
    break;

  case ETX_Encoding_8E2:
    st->conv_byte = _conv_byte_8e2;
    break;

  case ETX_Encoding_PXX1_PWM:
    st->conv_byte = _conv_byte_pxx1;
    freq = PXX1_FREQ;
    polarity = false;
    ocmode = TIMER_OC_MODE_INACTIVE;
    cmp_val = PXX1_PWM_ON + __pxx1_get_inverter_comp();
    break;

  default:
    return nullptr;
  }

  if (freq == 0) return nullptr;

  auto tim = port->tim;
  gd32_pulse_init(tim, freq);
  gd32_pulse_config_output(tim, polarity, ocmode, cmp_val);

  return hw_def;
}

static void gd32_softserial_tx_deinit(void* ctx)
{
  auto port = (const gd32_softserial_tx_port*)ctx;
  gd32_pulse_deinit(port->tim);
}

static void gd32_softserial_tx_wait(void* ctx)
{
  auto port = (const gd32_softserial_tx_port*)ctx;
  gd32_pulse_wait_for_completed(port->tim);
}

static void gd32_softserial_tx_send_buffer(void* ctx, const uint8_t* data, uint32_t size);

static void gd32_softserial_tx_send_byte(void* ctx, uint8_t byte)
{
  gd32_softserial_tx_send_buffer(ctx, &byte, 1);
  gd32_softserial_tx_wait(ctx);
}

static uint16_t _fill_pulses(gd32_softserial_tx_state* st)
{
  st->pulse_ptr = (uint16_t*)st->pulse_buffer;
  bool is_pxx1 = (st->conv_byte == _conv_byte_pxx1);

  uint32_t size = st->serial_size;

  if (is_pxx1) {
    size = (size + 7) / 8;
    if (size > STM32_SOFTSERIAL_MAX_PULSES_TRANSITIONS / 8)
      size = STM32_SOFTSERIAL_MAX_PULSES_TRANSITIONS / 8;
  } else if (size > STM32_SOFTSERIAL_BUFFERED_PULSES) {
    size = STM32_SOFTSERIAL_BUFFERED_PULSES;
  }

  for (uint8_t i = 0; i < size; i++) {
    st->conv_byte(st, *st->serial_data++);
  }

  uint16_t length = st->pulse_ptr - (uint16_t*)st->pulse_buffer;

  return length;
}

static bool gd32_softserial_tx_dma_tc_isr(void* ctx)
{
  auto port = (const gd32_softserial_tx_port*)ctx;
  auto tim = port->tim;
  auto st = port->st;

  if (!st->serial_size)
    return false;

  uint16_t length = _fill_pulses(st);
  dma_transfer_number_config((dma_channel_enum)tim->DMA_Stream, length);
  dma_channel_enable((dma_channel_enum)tim->DMA_Stream);

  return true;
}

static void gd32_softserial_tx_send_buffer(void* ctx, const uint8_t* data, uint32_t size)
{
  auto port = (const gd32_softserial_tx_port*)ctx;
  auto timer = port->tim;
  if (!gd32_pulse_if_not_running_disable(timer))
    return;

  auto st = port->st;
  st->serial_data = data;
  st->serial_size = size;

  auto length = _fill_pulses(st);

  if (st->serial_size > 0 && timer->DMA_TC_CallbackPtr) {
    auto closure = timer->DMA_TC_CallbackPtr;
    closure->cb = gd32_softserial_tx_dma_tc_isr;
    closure->ctx = ctx;
  }

  const void* pulses = st->pulse_buffer;

  uint32_t ocmode = TIMER_OC_MODE_TOGGLE;
  uint32_t cmp_val = 0;

  if (st->conv_byte == _conv_byte_pxx1) {
    ocmode = TIMER_OC_MODE_PWM1;
    cmp_val = PXX1_PWM_ON + __pxx1_get_inverter_comp();
  }

  gd32_pulse_start_dma_req(timer, pulses, length, ocmode, cmp_val);
}

void gd32_softserial_set_polarity(void* ctx, uint8_t polarity)
{
  auto port = (const gd32_softserial_tx_port*)ctx;
  gd32_pulse_set_polarity(port->tim, polarity);
}

const etx_serial_driver_t STM32SoftSerialTxDriver = {
  .init = gd32_softserial_tx_init,
  .deinit = gd32_softserial_tx_deinit,
  .sendByte = gd32_softserial_tx_send_byte,
  .sendBuffer = gd32_softserial_tx_send_buffer,
  .txCompleted = nullptr,
  .waitForTxCompleted = gd32_softserial_tx_wait,
  .enableRx = nullptr,
  .getByte = nullptr,
  .clearRxBuffer = nullptr,
  .getBaudrate = nullptr,
  .setBaudrate = nullptr,
  .setPolarity = gd32_softserial_set_polarity,
  .setHWOption = nullptr,
  .setReceiveCb = nullptr,
  .setIdleCb = nullptr,
  .setBaudrateCb = nullptr,
};
