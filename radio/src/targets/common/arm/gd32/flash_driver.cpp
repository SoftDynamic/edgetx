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

#include "flash_driver.h"

#include "gd32_cmsis.h"

#include <string.h>

uint32_t gd32_flash_get_size_kb()
{
  uint32_t flash_size_kb = (*(const uint32_t*)STORAGE_DENSITY_BASE) & 0x00FF;
  return flash_size_kb;
}

#if defined(GD32F3x0)
  #define SECTOR_ADDRESS(addr) ((uint32_t)(addr) & 0xFFFFFC00)
  #define FLASH_SECTOR_SIZE         1024 // 1KB
  #define FLASH_PROG_WORDS 1UL
  #define _FLASH_PROGRAM(address, p_data) \
    fmc_word_program((address), *((uint32_t *)(p_data)))

  uint32_t gd32_flash_get_sector_size(uint32_t sector)
  {
    return FLASH_SECTOR_SIZE;
  }
  uint32_t gd32_flash_get_sector(uint32_t address)
  {
    uint32_t sector_addr = SECTOR_ADDRESS(address);
    return sector_addr / FLASH_SECTOR_SIZE;
  }

#endif

inline void gd32_flash_unlock() { fmc_unlock(); }
inline void gd32_flash_lock() { fmc_lock(); }

int gd32_flash_erase_sector(uint32_t address)
{
  int ret = 0;

  gd32_flash_unlock();
  fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR);
  if (fmc_page_erase(SECTOR_ADDRESS(address)) != FMC_READY) {
    ret = -1;
  }

  gd32_flash_lock();
  return ret;
}

int gd32_flash_program(uint32_t address, void* data, uint32_t len)
{
  uint32_t* p_data = (uint32_t*)data;
  uint32_t end_addr = address + len;

  int ret = 0;
  gd32_flash_unlock();
  fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR);
  while (address < end_addr) {
    if (_FLASH_PROGRAM(address, p_data) != FMC_READY) {
      ret = -1;
      break;
    }
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR);
    address += sizeof(uint32_t) * FLASH_PROG_WORDS;
    p_data += FLASH_PROG_WORDS;
  }

  gd32_flash_lock();
  return ret;
}

int gd32_flash_read(uint32_t address, void* data, uint32_t len)
{
  memcpy(data, (void*)address, len);
  return 0;
}

const etx_flash_driver_t gd32_flash_driver = {
  .get_size_kb = gd32_flash_get_size_kb,
  .get_sector = gd32_flash_get_sector,
  .get_sector_size = gd32_flash_get_sector_size,
  .erase_sector = gd32_flash_erase_sector,
  .program = gd32_flash_program,
  .read = gd32_flash_read,
};

// Legacy API

#define FLASH_PAGESIZE 256 // TODO: legacy api should not be used

void unlockFlash() { gd32_flash_unlock(); }
void lockFlash() { gd32_flash_lock(); }

void flashWrite(uint32_t* address, const uint32_t* buffer)
{
  // check first if the address is on a sector boundary
  uint32_t sector = gd32_flash_get_sector((uintptr_t)address);

  if ((uintptr_t)address == SECTOR_ADDRESS(address)) {
    if (gd32_flash_erase_sector((uintptr_t)address) < 0) return;
  }

  gd32_flash_program((uintptr_t)address, (uint8_t*)buffer, FLASH_PAGESIZE);
}

// TODO: move this somewhere else, as it depends on firmware layout
uint32_t isFirmwareStart(const uint8_t * buffer)
{
  const uint32_t * block = (const uint32_t *)buffer;

  // Stack pointer in RAM
  if ((block[0] & 0xFFFC0000) != 0x20000000) {
    return 0;
  }
  // First ISR pointer in FLASH
  if ((block[1] & 0xFFF00000) != 0x08000000) {
    return 0;
  }
  // Second ISR pointer in FLASH
  if ((block[2] & 0xFFF00000) != 0x08000000) {
    return 0;
  }
  return 1;
}

uint32_t isBootloaderStart(const uint8_t * buffer)
{
  const uint32_t * block = (const uint32_t *)buffer;

  for (int i = 0; i < 256; i++) {
    if (block[i] == 0x544F4F42/*BOOT*/) {
      return 1;
    }
  }
  return 0;
}
