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

#include "gd32_dma.h"
#include "gd32_stdlib.h"

void gd32_dma_enable_clock(DMA_TypeDef* DMAx)
{
  (void)DMAx;
#if defined(GD32F3x0)
  rcu_periph_clock_enable(RCU_DMA);
#endif
}

FlagStatus gd32_dma_check_tc_flag(DMA_TypeDef* DMAx, uint32_t stream)
{
  (void)DMAx;
#if defined(GD32F3x0)
  // GD32F3x0 uses channels, not streams
  return dma_flag_get((dma_channel_enum)stream, DMA_FLAG_FTF);
#else
  return RESET;
#endif
}

FlagStatus gd32_dma_check_ht_flag(DMA_TypeDef* DMAx, uint32_t stream)
{
  (void)DMAx;
#if defined(GD32F3x0)
  return dma_flag_get((dma_channel_enum)stream, DMA_FLAG_HTF);
#else
  return RESET;
#endif
}
