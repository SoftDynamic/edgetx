#include "hal/serial_port.h"
#include "board.h"

// TODO: Populate with real serial port definitions for trainer port
//       and external module port when hardware is available.
//       Reference: boards/generic_stm32/aux_ports.cpp

const etx_serial_port_t* auxSerialGetPort(int port_nr)
{
  (void)port_nr;
  return nullptr;
}
