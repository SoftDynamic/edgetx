set(LUA "NO" CACHE STRING "Lua scripts (YES/NO/NO_MODEL_SCRIPTS)")
set_property(CACHE LUA PROPERTY STRINGS YES NO NO_MODEL_SCRIPTS)
set(LUA_SCRIPT_LOAD_MODE "" CACHE STRING "Script loading mode and compilation flags [btTxcd] (see loadScript() API docs). Blank for default ('bt' on radio, 'T' on SIMU/DEBUG builds)")
option(LUA_COMPILER "Pre-compile and save Lua scripts" OFF)
option(LUA_ALLOCATOR_TRACER "Trace Lua memory (de)allocations to debug port (also needs DEBUG=YES NANO=NO)" OFF)
option(USB "Enable USB" OFF)
option(USB_SERIAL "Enable USB serial (CDC)" OFF)

set(ARCH ARM)
# add_definitions(-DSTM32 -DLUA_INPUTS -DVARIO)
add_definitions(-DGD32)

set(LINKER_DIR ${RADIO_SRC_DIR}/boards/generic_gd32/linker)
add_link_options(-L${LINKER_DIR} -L${LINKER_DIR}/${TARGET_LINKER_DIR})

set(GD32_DRIVER_DIR targets/common/arm/gd32)
include_directories(${GD32_DRIVER_DIR})

if(NOT NATIVE_BUILD)

  # StdPeripheral driver library
  add_library(gd32_drivers OBJECT EXCLUDE_FROM_ALL
    # ${GD32_DRIVER_DIR}/gd32_hal.cpp
    ${GD32_DRIVER_DIR}/timers_driver.cpp
    # ${GD32_DRIVER_DIR}/diskio_spi.cpp
    ${GD32_DRIVER_DIR}/gd32_exti_driver.cpp
    ${GD32_DRIVER_DIR}/gd32_serial_driver.cpp
    ${GD32_DRIVER_DIR}/gd32_usart_driver.cpp
    ${GD32_DRIVER_DIR}/gd32_gpio_driver.cpp
    ${GD32_DRIVER_DIR}/gd32_timer.cpp
    ${GD32_DRIVER_DIR}/gd32_dma.cpp
    ${GD32_DRIVER_DIR}/gd32_gpio.cpp
    ${GD32_DRIVER_DIR}/gd32_adc.cpp
  )

  if(CPU_TYPE STREQUAL GD32F3x0)
    target_sources(gd32_drivers PUBLIC
      ${GD32_DRIVER_DIR}/gd32_spi.cpp
      ${GD32_DRIVER_DIR}/gd32_pulse_driver.cpp
      ${GD32_DRIVER_DIR}/gd32_softserial_driver.cpp
      ${GD32_DRIVER_DIR}/gd32_switch_driver.cpp
    )
    # GD32F3x0: flash page = 1KB → FatFS sector = 1024
    add_definitions(-DFF_MIN_SS=1024 -DFF_MAX_SS=1024 -DSECTOR_SIZE=1024)
  else()
    message(FATAL_ERROR "Not supported CPU_TYPE '${CPU_TYPE}'")
  endif()

  # # HAL/LL drivers using TRACE
  # add_library(gd32_drivers_w_dbg_fw OBJECT EXCLUDE_FROM_ALL
  #     ${GD32_DRIVER_DIR}/sdcard_spi.cpp
  #     ${GD32_DRIVER_DIR}/diskio_sdio.cpp
  # )
  # add_library(gd32_drivers_w_dbg_bl OBJECT EXCLUDE_FROM_ALL
  #     ${GD32_DRIVER_DIR}/sdcard_spi.cpp
  #     ${GD32_DRIVER_DIR}/diskio_sdio.cpp
  # )

  set(FIRMWARE_SRC ${FIRMWARE_SRC} $<TARGET_OBJECTS:gd32_drivers>)
  # set(FIRMWARE_SRC ${FIRMWARE_SRC} $<TARGET_OBJECTS:gd32_drivers_w_dbg_fw>)

  set(BOOTLOADER_SRC ${BOOTLOADER_SRC} $<TARGET_OBJECTS:gd32_drivers>)
  # set(BOOTLOADER_SRC ${BOOTLOADER_SRC} $<TARGET_OBJECTS:gd32_drivers_w_dbg_bl>)
endif()

if(DEBUG OR CLI OR USB_SERIAL)
  add_definitions(-DUSB_SERIAL)
  message("-- Adding support for USB serial")
endif()

set(FIRMWARE_SRC ${FIRMWARE_SRC} syscalls.c)
set(FIRMWARE_SRC ${FIRMWARE_SRC} ${GD32_DRIVER_DIR}/diskio_internal_flash.cpp)

set(TARGET_SRC ${TARGET_SRC} ../common/arm/gd32/cpu_id.cpp)
set(SRC ${SRC} io/bootloader_flash.cpp)
