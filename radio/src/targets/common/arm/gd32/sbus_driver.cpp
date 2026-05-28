#include "sbus.h"

#if !defined(SBUS)

// TODO: Enable SBUS support (set SBUS=ON in CMakeLists.txt when
//       serial port hardware is configured) to replace these stubs
//       with the real implementations in sbus.cpp

void sbusSetReceiveCtx(void* ctx, const etx_serial_driver_t* drv)
{
  (void)ctx;
  (void)drv;
}

void sbusAuxFrameReceived(void* param)
{
  (void)param;
}

void sbusAuxSetEnabled(bool enabled)
{
  (void)enabled;
}

void sbusFrameReceived(void* param)
{
  (void)param;
}

#endif
