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

#include "gd32_serial_driver.h"
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct gd32_buffer_state {
  volatile uint32_t ridx;
  volatile uint32_t widx;
};

struct gd32_send_buffer {
  volatile const uint8_t* buf;
  volatile uint32_t len;
};

struct gd32_serial_state {
  const gd32_serial_port* sp;
  gd32_buffer_state rx_buf;
  union {
    gd32_buffer_state tx_fifo;
    gd32_send_buffer  tx_buf;
  } u;
  etx_serial_callbacks_t callbacks;
};

// GD32F3x0: USART0 and USART1
#define GD32_MAX_UART_PORTS 2

static gd32_serial_state _serial_states[GD32_MAX_UART_PORTS];

void gd32_serial_init_driver()
{
  memset(_serial_states, 0, sizeof(_serial_states));
}

static volatile gd32_serial_state* _isr_state;

static uint8_t _on_send_fifo(uint8_t* data)
{
  auto st = _isr_state;
  auto& buf_st = st->u.tx_fifo;

  if (buf_st.ridx == buf_st.widx)
    return 0;

  const auto& tx_buf = st->sp->tx_buffer;
  auto buf = tx_buf.buffer;
  *data = buf[buf_st.ridx];

  auto buf_len = tx_buf.length;
  buf_st.ridx = (buf_st.ridx + 1) & (buf_len - 1);

  return 1;
}

static uint8_t _on_send_single_buffer(uint8_t* data)
{
  auto sb = &_isr_state->u.tx_buf;
  if (!sb->len) return 0;

  *data = *(sb->buf++);
  sb->len--;

  return 1;
}

// USART0 IRQ handler
extern "C" void USART0_IRQHandler(void)
{
  auto st = &_serial_states[0];
  auto old_st = _isr_state;
  _isr_state = st;
  gd32_usart_isr(st->sp->usart->USARTx, &st->callbacks);
  _isr_state = old_st;
}

// USART1 IRQ handler
extern "C" void USART1_IRQHandler(void)
{
  auto st = &_serial_states[1];
  auto old_st = _isr_state;
  _isr_state = st;
  gd32_usart_isr(st->sp->usart->USARTx, &st->callbacks);
  _isr_state = old_st;
}

static gd32_serial_state* gd32_serial_find_state(const gd32_usart_t* usart)
{
  if ((uint32_t)(usart->USARTx) == USART1) return &_serial_states[1];
  // Default to USART0
  return &_serial_states[0];
}

static void gd32_serial_free_state(gd32_serial_state* st)
{
  memset(st, 0, sizeof(gd32_serial_state));
}

static inline uint32_t _dma_get_data_length(DMA_TypeDef* DMAx, dma_channel_enum channel)
{
  (void)DMAx;
  return dma_transfer_number_get(channel);
}

static inline void _dma_clear(gd32_buffer_state* buf_st, uint32_t length,
                              DMA_TypeDef* DMAx, dma_channel_enum channel)
{
  buf_st->ridx = length - _dma_get_data_length(DMAx, channel);
}

static inline void _fifo_clear(gd32_buffer_state* buf_st)
{
  buf_st->widx = buf_st->ridx = 0;
}

static inline bool _fifo_full(gd32_buffer_state* buf_st, uint32_t length)
{
  return ((buf_st->widx + 1) & (length - 1)) == buf_st->ridx;
}

static inline void _fifo_push(uint8_t c, gd32_buffer_state* buf_st,
                              uint32_t length, uint8_t* buf)
{
  buf[buf_st->widx] = c;
  buf_st->widx = (buf_st->widx + 1) & (length - 1);
}

static void _on_rx_fifo(uint8_t data)
{
  auto st = _isr_state;
  gd32_buffer_state* buf_st = (gd32_buffer_state*)&st->rx_buf;

  auto buf_len = st->sp->rx_buffer.length;
  if (_fifo_full(buf_st, buf_len)) return;

  auto buf = st->sp->rx_buffer.buffer;
  _fifo_push(data, buf_st, buf_len, buf);
}

static void* gd32_serial_init(void* hw_def, const etx_serial_init* params)
{
  auto sp = (const gd32_serial_port*)hw_def;
  if (!sp) return nullptr;

  auto usart = sp->usart;
  auto st = gd32_serial_find_state(usart);
  if (!st || st->sp) return nullptr;

  st->sp = sp;

  if (!gd32_usart_init(usart, params)) {
    st->sp = nullptr;
    return nullptr;
  }

  if (params->direction & ETX_Dir_TX) {
    if (sp->tx_buffer.length > 0) {
      st->callbacks.on_send = _on_send_fifo;
    }
  }

  if (params->direction & ETX_Dir_RX) {
    auto rx_buf = sp->rx_buffer.buffer;
    auto buf_len = sp->rx_buffer.length;

    if (usart->rxDMA) {
      gd32_usart_init_rx_dma(usart, rx_buf, buf_len);

      auto dma = usart->rxDMA;
      auto stream = usart->rxDMA_Stream;
      _dma_clear(&st->rx_buf, buf_len, dma, stream);
    } else {
      st->callbacks.on_receive = _on_rx_fifo;
    }
  }

  return (void*)st;
}

static void gd32_serial_deinit(void* ctx)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return;

  gd32_usart_deinit(st->sp->usart->USARTx);
  gd32_serial_free_state(st);
}

static void gd32_serial_send_byte(void* ctx, uint8_t c)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return;

  auto sp = st->sp;
  auto buf_len = sp->tx_buffer.length;
  if (buf_len > 0) {
    auto buf_st = &st->u.tx_fifo;
    auto buf = sp->tx_buffer.buffer;

    if (_fifo_full(buf_st, buf_len)) return;
    _fifo_push(c, buf_st, buf_len, buf);

    gd32_usart_enable_tx_irq(sp->usart->USARTx);
  } else {
    gd32_usart_send_byte(sp->usart->USARTx, c);
  }
}

static void gd32_serial_send_buffer(void* ctx, const uint8_t* data, uint32_t size)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return;

  auto sp = st->sp;
  auto usart = sp->usart;
  if (usart->txDMA) {
    gd32_usart_send_buffer(usart->USARTx, data, size);
    return;
  }

  if (!sp->tx_buffer.length) {
    st->u.tx_buf.buf = data;
    st->u.tx_buf.len = size;
    st->callbacks.on_send = _on_send_single_buffer;
    gd32_usart_enable_tx_irq(usart->USARTx);
    return;
  }

  while (size > 0) {
    gd32_serial_send_byte(ctx, *data++);
    size--;
  }
}

static uint8_t gd32_serial_tx_completed(void* ctx)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return 1;

  return gd32_usart_tx_completed(st->sp->usart->USARTx);
}

static void gd32_wait_tx_completed(void* ctx)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return;

  while (!gd32_usart_tx_completed(st->sp->usart->USARTx));
}

static void gd32_enable_rx(void* ctx)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return;

  gd32_usart_enable_rx(st->sp->usart->USARTx);
}

static int gd32_serial_get_byte(void* ctx, uint8_t* data)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return -1;

  auto sp = st->sp;
  const auto& rx_buf = sp->rx_buffer;
  auto buf_len = rx_buf.length;
  if (!buf_len) return -1;

  auto buf = rx_buf.buffer;
  auto& buf_st = st->rx_buf;

  uint32_t widx;
  auto usart = sp->usart;
  uint32_t periph = (uint32_t)usart->USARTx;
  if (USART_CTL2(periph) & USD_CTL2_DENR) {
    auto dma = usart->rxDMA;
    auto stream = usart->rxDMA_Stream;
    widx = buf_len - _dma_get_data_length(dma, stream);
  } else {
    widx = buf_st.widx;
  }

  if (buf_st.ridx == widx)
    return 0;

  *data = buf[buf_st.ridx];
  buf_st.ridx = (buf_st.ridx + 1) & (buf_len - 1);

  return 1;
}

static int gd32_serial_get_last_byte(void* ctx, uint32_t idx, uint8_t* data)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return -1;

  auto sp = st->sp;
  const auto& rx_buf = sp->rx_buffer;
  auto buf_len = rx_buf.length;
  if (!buf_len) return -1;

  auto buf = rx_buf.buffer;
  auto& buf_st = st->rx_buf;

  uint32_t widx;
  auto usart = sp->usart;
  uint32_t periph = (uint32_t)usart->USARTx;
  if (USART_CTL2(periph) & USD_CTL2_DENR) {
    auto dma = usart->rxDMA;
    auto stream = usart->rxDMA_Stream;
    widx = buf_len - _dma_get_data_length(dma, stream);
  } else {
    widx = buf_st.widx;
  }

  uint32_t ridx = (buf_len + widx - idx) & (buf_len - 1);
  *data = buf[ridx];

  return 1;
}

static int gd32_serial_get_buffered_bytes(void* ctx)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return -1;

  auto sp = st->sp;
  const auto& rx_buf = sp->rx_buffer;
  auto buf_len = rx_buf.length;
  if (!buf_len) return -1;

  uint32_t widx;
  auto usart = sp->usart;
  const auto& buf_st = st->rx_buf;
  uint32_t periph = (uint32_t)usart->USARTx;

  if (USART_CTL2(periph) & USD_CTL2_DENR) {
    auto dma = usart->rxDMA;
    auto stream = usart->rxDMA_Stream;
    widx = buf_len - _dma_get_data_length(dma, stream);
  } else {
    widx = buf_st.widx;
  }

  return (widx - buf_st.ridx) & (buf_len - 1);
}

static inline void _copy_buffer_chunk(const gd32_serial_buffer& rx_buf,
                                      gd32_buffer_state& buf_st,
                                      uint8_t* buf, uint32_t len)
{
  memcpy(buf, rx_buf.buffer + buf_st.ridx, len);
  buf_st.ridx = (buf_st.ridx + len) & (rx_buf.length - 1);
}

static int gd32_serial_copy_rx_buffer(void* ctx, uint8_t* buf, uint32_t len)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return -1;

  auto sp = st->sp;
  const auto& rx_buf = sp->rx_buffer;
  auto buf_len = rx_buf.length;
  if (!buf_len) return -1;

  uint32_t widx;
  auto usart = sp->usart;
  auto& buf_st = st->rx_buf;
  uint32_t periph = (uint32_t)usart->USARTx;

  if (USART_CTL2(periph) & USD_CTL2_DENR) {
    auto dma = usart->rxDMA;
    auto stream = usart->rxDMA_Stream;
    widx = buf_len - _dma_get_data_length(dma, stream);
  } else {
    widx = buf_st.widx;
  }

  if (buf_st.ridx == widx) return 0;

  int res = 0;
  if (buf_st.ridx > widx) {
    auto cp_len = MIN(len, buf_len - buf_st.ridx);
    _copy_buffer_chunk(rx_buf, buf_st, buf, cp_len);
    buf += cp_len;
    len -= cp_len;
    res += cp_len;
  }

  if (buf_st.ridx < widx) {
    auto cp_len = MIN(len, widx - buf_st.ridx);
    _copy_buffer_chunk(rx_buf, buf_st, buf, cp_len);
    len -= cp_len;
    res += cp_len;
  }

  return res;
}

static void gd32_serial_clear_rx_buffer(void* ctx)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return;

  auto sp = st->sp;
  auto buf_st = &st->rx_buf;
  auto usart = sp->usart;
  if (usart->rxDMA) {
    auto buf_len = sp->rx_buffer.length;
    auto dma = usart->rxDMA;
    auto stream = usart->rxDMA_Stream;
    _dma_clear(buf_st, buf_len, dma, stream);
  } else {
    _fifo_clear(buf_st);
  }
}

static uint32_t gd32_serial_get_baudrate(void* ctx)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return 0;

  auto sp = st->sp;
  auto usart = sp->usart;
  return gd32_usart_get_baudrate(usart->USARTx);
}

static void gd32_serial_set_baudrate(void* ctx, uint32_t baudrate)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return;

  auto sp = st->sp;
  auto usart = sp->usart;
  gd32_usart_set_baudrate(usart->USARTx, baudrate);
}

static void gd32_serial_hw_option(void* ctx, uint32_t option)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return;

  auto sp = st->sp;
  auto usart = sp->usart;
  gd32_usart_set_hw_option(usart->USARTx, option);
}

static void gd32_serial_set_idle_cb(void* ctx, void (*on_idle)(void*), void* param)
{
  auto st = (gd32_serial_state*)ctx;
  if (!st) return;

  st->callbacks.on_idle = on_idle;
  st->callbacks.on_idle_ctx = param;

  uint32_t enabled = (on_idle != nullptr);
  gd32_usart_set_idle_irq(st->sp->usart->USARTx, enabled);
}

const etx_serial_driver_t GD32SerialDriver = {
  .init = gd32_serial_init,
  .deinit = gd32_serial_deinit,
  .sendByte = gd32_serial_send_byte,
  .sendBuffer = gd32_serial_send_buffer,
  .txCompleted = gd32_serial_tx_completed,
  .waitForTxCompleted = gd32_wait_tx_completed,
  .enableRx = gd32_enable_rx,
  .getByte = gd32_serial_get_byte,
  .getLastByte = gd32_serial_get_last_byte,
  .getBufferedBytes = gd32_serial_get_buffered_bytes,
  .copyRxBuffer = gd32_serial_copy_rx_buffer,
  .clearRxBuffer = gd32_serial_clear_rx_buffer,
  .getBaudrate = gd32_serial_get_baudrate,
  .setBaudrate = gd32_serial_set_baudrate,
  .setPolarity = nullptr,
  .setHWOption = gd32_serial_hw_option,
  .setReceiveCb = nullptr,
  .setIdleCb = gd32_serial_set_idle_cb,
  .setBaudrateCb = nullptr,
};
