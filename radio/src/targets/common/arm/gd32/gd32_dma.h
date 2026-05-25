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

#include <stdint.h>
#include <stdbool.h>

#if defined(GD32F3x0)

// GD32F3x0 has DMA channels, not streams
#define __GD32_DMA_IS_STREAM_SUPPORTED(sid)  (0)
#define DMA_STREAM_NOT_SUPPORTED_ON_GD32F3x0  0xFF

FlagStatus gd32_dma_check_tc_flag(DMA_TypeDef* DMAx, uint32_t stream);
FlagStatus gd32_dma_check_ht_flag(DMA_TypeDef* DMAx, uint32_t stream);

#endif

void gd32_dma_enable_clock(DMA_TypeDef* DMAx);
