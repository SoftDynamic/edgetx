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

#pragma once

#include <stdint.h>

#if defined(STORAGE_RAW_FLASH)

#include "dataconstants.h"

#define FLASH_PAGE_SIZE         1024
#define FLASH_RADIO_OFFSET      0
#define FLASH_MODELS_OFFSET     (FLASH_PAGE_SIZE)
#define FLASH_MODEL_PAGES       3
#define FLASH_MODEL_SLOT_SIZE   (FLASH_PAGE_SIZE * FLASH_MODEL_PAGES)
#define FLASH_STORAGE_SIZE      (FLASH_MODELS_OFFSET + FLASH_MODEL_SLOT_SIZE * MAX_MODELS)

void  flashStorageInit();

bool  flashReadRadioSettings(void* buf, uint32_t size);
bool  flashWriteRadioSettings(const void* buf, uint32_t size);
bool  flashReadModel(uint8_t idx, void* buf, uint32_t size);
bool  flashWriteModel(uint8_t idx, const void* buf, uint32_t size);
bool  flashModelExists(uint8_t idx);
void  flashModelDelete(uint8_t idx);
void  flashStorageEraseAll();

const char* flashLoadRadioSettings();
const char* flashWriteGeneralSettings();
const char* flashLoadModel(uint8_t idx, bool alarms = true);
const char* flashWriteModelData(uint8_t idx);
void  flashLoadModelHeader(uint8_t id, void* header, uint32_t size);

void storageReadAll();
void storageCheck(bool immediately);

bool storageReadRadioSettings(bool checks);
void loadModelHeaders();
void loadModelHeader(uint8_t id, void* header, uint32_t size);
const char* loadModel(uint8_t idx, bool alarms);
bool modelExists(uint8_t idx);
const char* createModel();
bool copyModel(uint8_t dst, uint8_t src);
void swapModels(uint8_t id1, uint8_t id2);
int8_t deleteModel(uint8_t idx);
void selectModel(uint8_t idx);

#endif
