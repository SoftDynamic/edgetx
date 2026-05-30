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

#include "gd32_spi.h"
#include "memory_sections.h"
#include "gd32_dma.h"
#include "hal/gpio.h"
#include "gd32_gpio.h"
#include "gd32_stdlib.h"
#include "definitions.h"

#include <stdlib.h>
#include <string.h>

#if !defined(SPI_DISABLE_DMA)
#define USE_SPI_DMA
#endif

#define SPI_DUMMY_BYTE (0xFF)

static inline uint32_t _spi_periph(SPI_TypeDef *SPIx)
{
  return (uint32_t)SPIx;
}

void gd32_spi_enable_clock(SPI_TypeDef *SPIx)
{
  uint32_t periph = _spi_periph(SPIx);
  if (periph == SPI0)
    rcu_periph_clock_enable(RCU_SPI0);
  else if (periph == SPI1)
    rcu_periph_clock_enable(RCU_SPI1);
}

static inline uint32_t _get_spi_af(SPI_TypeDef *SPIx)
{
  (void)SPIx;
  return GPIO_AF_0;
}

static uint32_t _get_spi_prescaler(SPI_TypeDef *SPIx, uint32_t max_freq)
{
  uint32_t pclk;
  if (_spi_periph(SPIx) == SPI0)
    pclk = rcu_clock_freq_get(CK_APB2);
  else
    pclk = rcu_clock_freq_get(CK_APB1);

  uint32_t divider = (pclk + max_freq) / max_freq;
  uint32_t presc;
  if (divider > 128)
    presc = SPI_PSC_256;
  else if (divider > 64)
    presc = SPI_PSC_128;
  else if (divider > 32)
    presc = SPI_PSC_64;
  else if (divider > 16)
    presc = SPI_PSC_32;
  else if (divider > 8)
    presc = SPI_PSC_16;
  else if (divider > 4)
    presc = SPI_PSC_8;
  else if (divider > 2)
    presc = SPI_PSC_4;
  else
    presc = SPI_PSC_2;

  return presc;
}

static void _init_gpios(const gd32_spi_t* spi)
{
  gpio_init_af(spi->MISO, _get_spi_af(spi->SPIx), GPIO_PIN_SPEED_VERY_HIGH);
  gpio_init_af(spi->SCK,  _get_spi_af(spi->SPIx), GPIO_PIN_SPEED_VERY_HIGH);
  gpio_init_af(spi->MOSI, _get_spi_af(spi->SPIx), GPIO_PIN_SPEED_VERY_HIGH);
  gpio_init(spi->CS, GPIO_OUT, GPIO_PIN_SPEED_HIGH);
}

#if defined(USE_SPI_DMA)
static void _config_dma_channels(const gd32_spi_t* spi)
{
  gd32_dma_enable_clock(spi->DMA_Handle);

  dma_parameter_struct dmaInit;
  dma_struct_para_init(&dmaInit);

  dmaInit.periph_addr  = (uint32_t) & SPI_DATA(spi->SPIx);
  dmaInit.memory_addr  = 0;
  dmaInit.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
  dmaInit.periph_width = spi->DMA_MemoryOrM2MDstDataSize;
  dmaInit.memory_width = spi->DMA_MemoryOrM2MDstDataSize;
  dmaInit.priority     = DMA_PRIORITY_ULTRA_HIGH;
  dmaInit.direction    = DMA_PERIPHERAL_TO_MEMORY;
  dmaInit.number       = 0;
  dmaInit.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
  dma_init(spi->rxDMA_Stream, &dmaInit);

  dmaInit.direction    = DMA_MEMORY_TO_PERIPHERAL;
  dma_init(spi->txDMA_Stream, &dmaInit);
}
#endif

void gd32_spi_init(const gd32_spi_t* spi, uint32_t data_width)
{
  _init_gpios(spi);

  auto SPIx = spi->SPIx;
  uint32_t periph = _spi_periph(SPIx);
  gd32_spi_enable_clock(SPIx);

  spi_parameter_struct spiInit;
  spi_struct_para_init(&spiInit);

  spiInit.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
  spiInit.device_mode = SPI_MASTER;
  spiInit.nss = SPI_NSS_SOFT;
  spiInit.frame_size = data_width;
  spiInit.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
  spiInit.prescale = _get_spi_prescaler(SPIx, 1000000);

  spi_init(periph, &spiInit);
  spi_enable(periph);

#if defined(USE_SPI_DMA)
  if (spi->DMA_Handle) {
    _config_dma_channels(spi);
  }
#endif
}

void gd32_spi_select(const gd32_spi_t* spi)
{
  gpio_clear(spi->CS);
}

void gd32_spi_unselect(const gd32_spi_t* spi)
{
  gpio_set(spi->CS);
}

void gd32_spi_set_max_baudrate(const gd32_spi_t* spi, uint32_t baudrate)
{
  auto* SPIx = spi->SPIx;
  uint32_t periph = _spi_periph(SPIx);
  uint32_t presc = _get_spi_prescaler(SPIx, baudrate);
  SPI_CTL0(periph) = (SPI_CTL0(periph) & ~SPI_CTL0_PSC) | presc;
}

uint8_t gd32_spi_transfer_byte(const gd32_spi_t* spi, uint8_t out)
{
  auto* SPIx = spi->SPIx;
  uint32_t periph = _spi_periph(SPIx);

  while (!spi_i2s_flag_get(periph, SPI_FLAG_TBE));
  spi_i2s_data_transmit(periph, out);

  while (!spi_i2s_flag_get(periph, SPI_FLAG_RBNE));
  return (uint8_t)spi_i2s_data_receive(periph);
}

uint32_t gd32_spi_transfer_bytes(const gd32_spi_t* spi, const uint8_t* out,
                                  uint8_t* in, uint32_t length)
{
  unsigned trans_bytes = 0;
  uint8_t in_temp;

  for (trans_bytes = 0; trans_bytes < length; trans_bytes++) {
    if (out != nullptr) {
      in_temp = gd32_spi_transfer_byte(spi, out[trans_bytes]);
    } else {
      in_temp = gd32_spi_transfer_byte(spi, SPI_DUMMY_BYTE);
    }
    if (in != nullptr) {
      in[trans_bytes] = in_temp;
    }
  }

  return trans_bytes;
}

uint16_t gd32_spi_transfer_word(const gd32_spi_t* spi, uint16_t out)
{
  auto* SPIx = spi->SPIx;
  uint32_t periph = _spi_periph(SPIx);

  while (!spi_i2s_flag_get(periph, SPI_FLAG_TBE));
  spi_i2s_data_transmit(periph, out);

  while (!spi_i2s_flag_get(periph, SPI_FLAG_RBNE));
  return (uint16_t)spi_i2s_data_receive(periph);
}

#if defined(USE_SPI_DMA)
static uint16_t _scratch_word __DMA_NO_CACHE;
#if defined(PCBC7MINI)
static uint8_t _scratch_buffer[64] __DMA_NO_CACHE;
#else
static uint8_t _scratch_buffer[512] __DMA_NO_CACHE;
#endif

static void _dma_enable_channel(dma_channel_enum channel,
                                const void* data, uint32_t length)
{
  gd32_dma_check_tc_flag(NULL, channel);
  dma_memory_address_config(channel, (uint32_t)data);
  dma_transfer_number_config(channel, length);
  dma_channel_enable(channel);
}

uint32_t gd32_spi_dma_receive_bytes(const gd32_spi_t* spi, uint8_t* data,
                                     uint32_t length)
{
#if defined(USE_SPI_DMA)
  if (!spi->DMA_Handle) {
    return gd32_spi_transfer_bytes(spi, nullptr, data, length);
  }

  uint32_t periph = _spi_periph(spi->SPIx);
  uint32_t max_xfer_len = sizeof(_scratch_buffer);

  uint32_t xfer_len = length;
  while (xfer_len > 0) {
    uint32_t single_xfer_len = (xfer_len > max_xfer_len) ? max_xfer_len : xfer_len;

    _dma_enable_channel(spi->rxDMA_Stream, _scratch_buffer, single_xfer_len);
    spi_dma_enable(periph, SPI_DMA_RECEIVE);

    _scratch_word = 0xFFFF;
    dma_memory_increase_disable(spi->txDMA_Stream);
    _dma_enable_channel(spi->txDMA_Stream, &_scratch_word, single_xfer_len);
    spi_dma_enable(periph, SPI_DMA_TRANSMIT);

    while (dma_flag_get(spi->rxDMA_Stream, DMA_FLAG_FTF) == RESET);

    while (!spi_i2s_flag_get(periph, SPI_FLAG_TBE));
    while (spi_i2s_flag_get(periph, SPI_FLAG_TRANS));

    spi_dma_disable(periph, SPI_DMA_TRANSMIT);
    spi_dma_disable(periph, SPI_DMA_RECEIVE);

    memcpy(data, _scratch_buffer, single_xfer_len);

    xfer_len -= single_xfer_len;
    data += single_xfer_len;
  }

  return length;
#else
  return gd32_spi_transfer_bytes(spi, nullptr, data, length);
#endif
}

uint32_t gd32_spi_dma_transmit_bytes(const gd32_spi_t* spi,
                                      const uint8_t* data, uint32_t length)
{
#if defined(USE_SPI_DMA)
  if (!spi->DMA_Handle) {
    return gd32_spi_transfer_bytes(spi, data, nullptr, length);
  }

  uint32_t periph = _spi_periph(spi->SPIx);
  uint32_t max_xfer_len = sizeof(_scratch_buffer);

  uint32_t xfer_len = length;
  while (xfer_len > 0) {
    uint32_t single_xfer_len = (xfer_len > max_xfer_len) ? max_xfer_len : xfer_len;

    memcpy(_scratch_buffer, data, single_xfer_len);
    dma_memory_increase_enable(spi->txDMA_Stream);
    _dma_enable_channel(spi->txDMA_Stream, _scratch_buffer, single_xfer_len);
    spi_dma_enable(periph, SPI_DMA_TRANSMIT);

    while (dma_flag_get(spi->txDMA_Stream, DMA_FLAG_FTF) == RESET);

    while (!spi_i2s_flag_get(periph, SPI_FLAG_TBE));
    while (spi_i2s_flag_get(periph, SPI_FLAG_TRANS));

    if (spi_i2s_flag_get(periph, SPI_FLAG_RBNE))
      (void)spi_i2s_data_receive(periph);

    spi_dma_disable(periph, SPI_DMA_TRANSMIT);

    xfer_len -= single_xfer_len;
    data += single_xfer_len;
  }

  return length;
#else
  return gd32_spi_transfer_bytes(spi, data, 0, length);
#endif
}
#endif
