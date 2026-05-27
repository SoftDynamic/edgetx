#pragma once

#include "hal/fatfs_diskio.h"

void internalFlashDiskEraseAll();

extern const diskio_driver_t internal_flash_diskio_driver;
