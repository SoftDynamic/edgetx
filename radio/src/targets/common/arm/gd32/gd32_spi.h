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

#pragma once

#include "gd32_stdlib.h"
#include "hal/gpio.h"
#include <stdint.h>

// GD32F3x0 CMSIS doesn't define SPI_TypeDef
// SPL uses uint32_t peripheral base address
// Forward-declare for struct compatibility
struct spi_reg_map { uint32_t reserved[4]; };
typedef struct spi_reg_map SPI_TypeDef;

struct gd32_spi_t {
  SPI_TypeDef*   SPIx;
  gpio_t         SCK;
  gpio_t         MISO;
  gpio_t         MOSI;
  gpio_t         CS;

  DMA_TypeDef*   DMA_Handle;
  uint32_t       DMA_Channel;
  dma_channel_enum txDMA_Stream;
  dma_channel_enum rxDMA_Stream;
  uint32_t       DMA_FIFOMode;
  uint32_t       DMA_FIFOThreshold;
  uint32_t       DMA_MemoryOrM2MDstDataSize;
  uint32_t       DMA_MemBurst;
};

void gd32_spi_enable_clock(SPI_TypeDef *SPIx);

void gd32_spi_init(const gd32_spi_t* spi, uint32_t data_width);
void gd32_spi_deinit(const gd32_spi_t* spi);

void gd32_spi_select(const gd32_spi_t* spi);
void gd32_spi_unselect(const gd32_spi_t* spi);

void gd32_spi_set_max_baudrate(const gd32_spi_t* spi, uint32_t baudrate);
void gd32_spi_set_data_width(const gd32_spi_t* spi, uint32_t data_width);

uint8_t gd32_spi_transfer_byte(const gd32_spi_t* spi, uint8_t out);
uint16_t gd32_spi_transfer_word(const gd32_spi_t* spi, uint16_t out);

uint32_t gd32_spi_transfer_bytes(const gd32_spi_t* spi, const uint8_t* out,
                                  uint8_t* in, uint32_t length);

uint32_t gd32_spi_dma_receive_bytes(const gd32_spi_t* spi, uint8_t* data,
                                     uint32_t length);

uint32_t gd32_spi_dma_transmit_bytes(const gd32_spi_t* spi,
                                      const uint8_t* data, uint32_t length);
