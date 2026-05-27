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
 * FatFS disk I/O driver for GD32 internal flash.
 * Uses 1024-byte sectors (1 sector = 1 flash page).
 */

#include "diskio_internal_flash.h"
#include "hal/flash_driver.h"

extern const uint32_t __data_flash_end;

#define SECTOR_SIZE 1024

static uint32_t flash_base_addr()
{
  return (uint32_t)&__data_flash_end - INTERNAL_FLASH_STORAGE_SIZE;
}

static const etx_flash_driver_t* get_drv()
{
  return flashFindDriver(flash_base_addr());
}

static DSTATUS internal_flash_initialize(BYTE lun)
{
  if (!get_drv()) return STA_NOINIT;
  return 0;
}

static DSTATUS internal_flash_status(BYTE lun)
{
  return 0;
}

static DRESULT internal_flash_read(BYTE lun, BYTE* buff, DWORD sector, UINT count)
{
  auto drv = get_drv();
  if (!drv) return RES_ERROR;
  uint32_t addr = flash_base_addr() + (uint32_t)sector * SECTOR_SIZE;
  if (drv->read(addr, buff, (uint32_t)count * SECTOR_SIZE) != 0)
    return RES_ERROR;
  return RES_OK;
}

static DRESULT internal_flash_write(BYTE lun, const BYTE* buff, DWORD sector, UINT count)
{
  auto drv = get_drv();
  if (!drv) return RES_ERROR;
  uint32_t addr = flash_base_addr() + (uint32_t)sector * SECTOR_SIZE;
  for (UINT i = 0; i < count; i++) {
    if (drv->erase_sector(addr + i * SECTOR_SIZE) != 0)
      return RES_ERROR;
    if (drv->program(addr + i * SECTOR_SIZE, (void*)(buff + i * SECTOR_SIZE), SECTOR_SIZE) != 0)
      return RES_ERROR;
  }
  return RES_OK;
}

static DRESULT internal_flash_ioctl(BYTE lun, BYTE ctrl, void* buff)
{
  switch (ctrl) {
  case GET_SECTOR_COUNT:
    *(DWORD*)buff = INTERNAL_FLASH_STORAGE_SIZE / SECTOR_SIZE;
    break;
  case GET_SECTOR_SIZE:
    *(WORD*)buff = SECTOR_SIZE;
    break;
  case GET_BLOCK_SIZE:
    *(DWORD*)buff = 1;
    break;
  case CTRL_SYNC:
    break;
  default:
    return RES_PARERR;
  }
  return RES_OK;
}

void internalFlashDiskEraseAll()
{
  auto drv = get_drv();
  if (!drv) return;
  uint32_t base = flash_base_addr();
  uint32_t sector_size = drv->get_sector_size(drv->get_sector(base));
  uint32_t end = base + INTERNAL_FLASH_STORAGE_SIZE;
  for (uint32_t addr = base; addr < end; addr += sector_size) {
    drv->erase_sector(addr);
  }
}

const diskio_driver_t internal_flash_diskio_driver = {
  .initialize = internal_flash_initialize,
  .deinit = nullptr,
  .status = internal_flash_status,
  .read = internal_flash_read,
  .write = internal_flash_write,
  .ioctl = internal_flash_ioctl,
};
