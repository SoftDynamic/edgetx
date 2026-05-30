/*
 * Copyright (C) EdgeTX
 *
 * Raw flash storage implementation for radios without external storage (C7MINI etc.)
 */

#include "edgetx.h"
#include "hal/storage_flash.h"

#include "crc.h"
#include "hal/flash_driver.h"

uint8_t logDelay100ms;

extern const uint32_t __data_flash_start;

static uint32_t flashStorageBase()
{
  return (uint32_t)&__data_flash_start;
}

static const etx_flash_driver_t* getDriver()
{
  return flashFindDriver(flashStorageBase());
}

static int flashWrite(uint32_t addr, const uint8_t* data, uint32_t len)
{
  auto drv = getDriver();
  if (!drv) return -1;
  return drv->program(addr, (void*)data, len);
}

static int flashRead(uint32_t addr, uint8_t* data, uint32_t len)
{
  auto drv = getDriver();
  if (!drv) return -1;
  return drv->read(addr, data, len);
}

static int flashErase(uint32_t addr)
{
  auto drv = getDriver();
  if (!drv) return -1;
  return drv->erase_sector(addr);
}

static uint16_t crcData(const uint8_t* data, uint32_t size)
{
  return crc16(0, data, size, 0xFFFF);
}

void flashStorageInit()
{
  auto drv = getDriver();
  (void)drv;
}

static bool verifySlot(uint32_t addr, uint32_t size)
{
  const uint16_t MAGIC = 0xED6E;
  uint16_t magic;
  if (flashRead(addr, (uint8_t*)&magic, 2) != 0) return false;
  if (magic != MAGIC) return false;

  uint16_t storedCrc;
  if (flashRead(addr + 2 + size, (uint8_t*)&storedCrc, 2) != 0) return false;

  uint8_t* buf = (uint8_t*)alloca(size);
  if (!buf) return false;
  if (flashRead(addr + 2, buf, size) != 0) return false;

  uint16_t calcCrc = crcData(buf, size);
  return calcCrc == storedCrc;
}

static bool writeSlot(uint32_t addr, const void* data, uint32_t size, uint32_t pageCount)
{
  const uint16_t MAGIC = 0xED6E;

  for (uint32_t i = 0; i < pageCount; i++) {
    if (flashErase(addr + i * FLASH_PAGE_SIZE) != 0)
      return false;
  }

  if (flashWrite(addr, (const uint8_t*)&MAGIC, 2) != 0) return false;
  if (flashWrite(addr + 2, (const uint8_t*)data, size) != 0) return false;

  uint16_t calcCrc = crcData((const uint8_t*)data, size);
  if (flashWrite(addr + 2 + size, (const uint8_t*)&calcCrc, 2) != 0) return false;

  return true;
}

bool flashReadRadioSettings(void* buf, uint32_t size)
{
  uint32_t addr = flashStorageBase() + FLASH_RADIO_OFFSET;
  if (!verifySlot(addr, size)) return false;
  return flashRead(addr + 2, (uint8_t*)buf, size) == 0;
}

bool flashWriteRadioSettings(const void* buf, uint32_t size)
{
  uint32_t addr = flashStorageBase() + FLASH_RADIO_OFFSET;
  return writeSlot(addr, buf, size, 1);
}

bool flashReadModel(uint8_t idx, void* buf, uint32_t size)
{
  uint32_t addr = flashStorageBase() + FLASH_MODELS_OFFSET + idx * FLASH_MODEL_SLOT_SIZE;
  if (!flashModelExists(idx)) return false;
  return flashRead(addr + 2, (uint8_t*)buf, size) == 0;
}

bool flashWriteModel(uint8_t idx, const void* buf, uint32_t size)
{
  uint32_t addr = flashStorageBase() + FLASH_MODELS_OFFSET + idx * FLASH_MODEL_SLOT_SIZE;
  return writeSlot(addr, buf, size, FLASH_MODEL_PAGES);
}

bool flashModelExists(uint8_t idx)
{
  uint32_t addr = flashStorageBase() + FLASH_MODELS_OFFSET + idx * FLASH_MODEL_SLOT_SIZE;
  const uint16_t MAGIC = 0xED6E;
  uint16_t magic;
  if (flashRead(addr, (uint8_t*)&magic, 2) != 0) return false;
  return magic == MAGIC;
}

void flashModelDelete(uint8_t idx)
{
  uint32_t addr = flashStorageBase() + FLASH_MODELS_OFFSET + idx * FLASH_MODEL_SLOT_SIZE;
  for (int i = 0; i < FLASH_MODEL_PAGES; i++) {
    flashErase(addr + i * FLASH_PAGE_SIZE);
  }
}

void flashStorageEraseAll()
{
  uint32_t base = flashStorageBase();
  for (uint32_t addr = 0; addr < FLASH_STORAGE_SIZE; addr += FLASH_PAGE_SIZE) {
    flashErase(base + addr);
  }
}

const char* flashLoadRadioSettings()
{
  memset(&g_eeGeneral, 0, sizeof(g_eeGeneral));
  if (!flashReadRadioSettings(&g_eeGeneral, sizeof(g_eeGeneral))) {
    return "no radio settings";
  }
  g_eeGeneral.chkSum = evalChkSum();
  postRadioSettingsLoad();
  return nullptr;
}

const char* flashWriteGeneralSettings()
{
  g_eeGeneral.manuallyEdited = false;
  g_eeGeneral.chkSum = evalChkSum();
  if (!flashWriteRadioSettings(&g_eeGeneral, sizeof(g_eeGeneral))) {
    return "write error";
  }
  return nullptr;
}

const char* flashLoadModel(uint8_t idx, bool alarms)
{
  preModelLoad();

  if (!flashReadModel(idx, &g_model, sizeof(g_model))) {
    postModelLoad(alarms);
    return "no model";
  }

  postModelLoad(alarms);
  return nullptr;
}

const char* flashWriteModelData(uint8_t idx)
{
  if (!flashWriteModel(idx, &g_model, sizeof(g_model))) {
    return "write error";
  }
  return nullptr;
}

void flashLoadModelHeader(uint8_t id, void* header, uint32_t size)
{
  auto h = (ModelHeader*)header;
  memset(h, 0, size);

  if (!flashModelExists(id)) {
    h->modelId[0] = 0;
    return;
  }

  // Read directly from flash — headers stored at start of model slot
  uint32_t addr = flashStorageBase() + FLASH_MODELS_OFFSET + id * FLASH_MODEL_SLOT_SIZE;
  const uint16_t MAGIC = 0xED6E;
  uint16_t magic;
  if (flashRead(addr, (uint8_t*)&magic, 2) != 0) return;
  if (magic != MAGIC) return;

  ModelData tmp;
  if (flashRead(addr + 2, (uint8_t*)&tmp, sizeof(tmp)) != 0) return;

  memcpy(h->name, tmp.header.name, sizeof(h->name));
#if LEN_BITMAP_NAME > 0
  memcpy(h->bitmap, tmp.header.bitmap, sizeof(h->bitmap));
#endif
  h->modelId[0] = tmp.header.modelId[0];
}

ModelHeader modelHeaders[MAX_MODELS];

void loadModelHeaders()
{
  for (uint32_t i = 0; i < MAX_MODELS; i++) {
    flashLoadModelHeader(i, &modelHeaders[i], sizeof(ModelHeader));
  }
}

void loadModelHeader(uint8_t id, ModelHeader* header)
{
  flashLoadModelHeader(id, header, sizeof(ModelHeader));
}

const char* loadModel(uint8_t idx, bool alarms)
{
  return flashLoadModel(idx, alarms);
}

bool modelExists(uint8_t idx)
{
  return flashModelExists(idx);
}

const char* loadRadioSettings()
{
  return flashLoadRadioSettings();
}

const char* writeGeneralSettings()
{
  return flashWriteGeneralSettings();
}

bool copyModel(uint8_t dst, uint8_t src)
{
  if (!flashModelExists(src)) return false;
  ModelData tmp;
  if (!flashReadModel(src, &tmp, sizeof(tmp))) return false;
  return flashWriteModel(dst, &tmp, sizeof(tmp));
}

void swapModels(uint8_t id1, uint8_t id2)
{
  if (id1 == id2) return;
  bool exists1 = flashModelExists(id1);
  bool exists2 = flashModelExists(id2);
  ModelData tmp1, tmp2;
  if (exists1) flashReadModel(id1, &tmp1, sizeof(tmp1));
  if (exists2) flashReadModel(id2, &tmp2, sizeof(tmp2));
  if (exists2) flashWriteModel(id1, &tmp2, sizeof(tmp2));
  else flashModelDelete(id1);
  if (exists1) flashWriteModel(id2, &tmp1, sizeof(tmp1));
  else flashModelDelete(id2);
}

int8_t deleteModel(uint8_t idx)
{
  if (!flashModelExists(idx)) return -1;
  flashModelDelete(idx);
  return 0;
}

const char* createModel()
{
  return "not supported";
}

const char* writeModel()
{
  return flashWriteModelData(g_eeGeneral.currModel);
}

void storageReadAll()
{
  memset(&g_eeGeneral, 0, sizeof(g_eeGeneral));

  if (flashLoadRadioSettings()) {
    flashStorageEraseAll();
    storageDirty(EE_GENERAL);
    flashLoadRadioSettings();
  }

  loadModelHeaders();

  if (!flashModelExists(g_eeGeneral.currModel)) {
    for (uint8_t i = 0; i < MAX_MODELS; i++) {
      if (flashModelExists(i)) {
        g_eeGeneral.currModel = i;
        break;
      }
    }
  }

  flashLoadModel(g_eeGeneral.currModel, false);
}

void storageCheck(bool immediately)
{
  if (immediately) {
    if (storageDirtyMsk & EE_GENERAL) {
      flashWriteGeneralSettings();
    }
    if (storageDirtyMsk & EE_MODEL) {
      storageFlushCurrentModel();
      flashWriteModelData(g_eeGeneral.currModel);
    }
    storageDirtyMsk = 0;
    storageDirtyTime10ms = 0;
    return;
  }

  if (storageDirtyMsk && (tmr10ms_t)(get_tmr10ms() - storageDirtyTime10ms) >= (tmr10ms_t)WRITE_DELAY_10MS) {
    storageCheck(true);
  }
}

void storageEraseAll(bool warn)
{
  flashStorageEraseAll();
  storageDirty(EE_GENERAL | EE_MODEL);
  storageCheck(true);
}

void storageFormat()
{
  flashStorageEraseAll();
}

bool storageReadRadioSettings(bool checks)
{
  const char* error = flashLoadRadioSettings();
  if (error && checks) {
    ALERT(STR_STORAGE_WARNING, TR_RADIO_DATA_RECOVERED, AU_BAD_RADIODATA);
    return false;
  }
  return error == nullptr;
}

void selectModel(uint8_t idx)
{
  storageFlushCurrentModel();
  storageCheck(true);
  g_eeGeneral.currModel = idx;
  storageDirty(EE_GENERAL);
  loadModel(idx);
}

uint8_t findEmptyModel(uint8_t id, bool down)
{
  uint8_t i = id;
  for (;;) {
    i = (MAX_MODELS + (down ? i + 1 : i - 1)) % MAX_MODELS;
    if (!modelExists(i)) break;
    if (i == id) return 0xff;
  }
  return i;
}

// Stub SD card functions for raw flash builds
bool isFileAvailable(const char*, bool) { return false; }
bool sdMounted() { return true; }
bool sdIsFull() { return false; }
bool sdListFiles(const char*, const char*, const uint8_t, const char*, uint8_t) { return false; }
const char* sdCheckAndCreateDirectory(const char*) { return nullptr; }
const char* sdCopyFile(const char*, const char*) { return "not supported"; }
void logsClose() {}
const char* logsOpen() { return nullptr; }
const char* backupModel(uint8_t) { return "not supported"; }
const char* restoreModel(uint8_t, char*) { return "not supported"; }
void pushModelNotes() {}
void menuRadioSdManager(event_t) {}
void readModelNotes() {}
void initLoggingTimer() {}
void loggingTimerStart() {}
void loggingTimerStop() {}
void logsInit() {}
const char* writeScreenshot() { return nullptr; }
void menuChannelsView(event_t) {}
void menuChannelsViewCommon(event_t) {}
void menuModelDisplay(event_t) {}
void menuAboutView(event_t) {}
void menuRadioDiagAnalogs(event_t) {}
void menuRadioDiagKeys(event_t) {}
void displayTelemetryScreen() {}
void displayRssiLine() {}
uint8_t selectedTelemView = 0;
