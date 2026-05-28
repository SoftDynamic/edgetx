#include "hal/gpio.h"
#include "hal/usb_driver.h"

#include "hal.h"
#include "debug.h"

#if defined(USB_GPIO_DP)

// TODO: Implement GD32 USB peripheral driver

#else // no USB, stub functions

// TODO: Replace with real USB driver when hardware is available

int usbPlugged()
{
  return false;
}

void usbInit()
{
}

void usbStart()
{
}

void usbStop()
{
}

bool usbStarted()
{
  return false;
}

int getSelectedUsbMode()
{
  return USB_UNSELECTED_MODE;
}

void setSelectedUsbMode(int mode)
{
  (void)mode;
}

uint32_t usbSerialFreeSpace()
{
  return 0;
}

void usbJoystickUpdate()
{
}

const etx_serial_port_t UsbSerialPort = {
  "VCOM",
  nullptr,
  nullptr,
  nullptr,
};

#endif
