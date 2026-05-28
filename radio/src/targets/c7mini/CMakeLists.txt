
# ==功能开关==
option(SHUTDOWN_CONFIRMATION "Shutdown confirmation" OFF)
option(PXX1 "PXX1 protocol support" OFF)      # 关闭FrSky协议
option(PXX2 "PXX2 protocol support" OFF)
option(AFHDS3 "AFHDS3 TX Module" OFF)
option(GHOST "Ghost TX Module" OFF)
option(INTERNAL_MODULE_PPM "Support for PPM internal module" OFF)
option(AUTOUPDATE "Auto update internal chips from SD" OFF)
option(BIND_KEY "Enable bind button" OFF)
option(BLUETOOTH "FrSky BT module support" OFF)
option(RGBLEDS "WS2812 addressable LED support" OFF)
option(HAPTIC "Haptic support" OFF)
option(HELI "Heli menu" OFF)
option(FLIGHT_MODES "Flight Modes" OFF)
option(CURVES "Curves" OFF)
option(GVARS "Global variables" OFF)
option(LUA "Enable LUA support" OFF)
option(BOOTLOADER "Include Bootloader" OFF)
option(WATCHDOG "Enable hardware Watchdog" OFF)
option(AUDIO "Audio support" OFF)
option(VOICE "Voice support" OFF)
option(ENABLE_SERIAL_PASSTHROUGH "Enable serial passthrough" OFF)
option(XJT "XJT TX Module" OFF)
option(PPM "PPM TX Module" OFF)
option(MULTIMODULE "DIY Multiprotocol TX Module (https://github.com/pascallanger/DIY-Multiprotocol-TX-Module)" OFF)
option(DSM2 "DSM2 TX Module" OFF)
option(DSMP "DSMP LemonRX TX Module" OFF)
option(SBUS "SBUS TX Module" OFF)
option(CROSSFIRE "Crossfire TX Module" OFF)

# ==Target==
set(TARGET_DIR c7mini)
set(CPU_TYPE_FULL GD32F330RB)
set(ARCH ARM)
set(TARGET_FLASH_SIZE 128K)
set(TARGET_RAM_SIZE 16K)
set(MCU cortex-m4)
set(FPU_FLAGS "-mfpu=fpv4-sp-d16 -mfloat-abi=hard")

# ==时钟==
set(USE_RTC_CLOCK NO)
set(HSE_VALUE 16000000)
add_definitions(-DHXTAL_VALUE=${HSE_VALUE})

# ==编译器flag==
# add_definitions(-DPCBFRSKY -DPCBTARANIS -DAUDIO -DVOICE)
add_definitions(-DPCBC7MINI)

set(PWR_BUTTON "PRESS" CACHE STRING "Pwr button type (PRESS/SWITCH)")
set(HAPTIC NO)
set(GUI_DIR 128x64)
set(BITMAPS_DIR 128x64)
set(BITMAPS_TARGET 9x_bitmaps)
set(FONTS_TARGET 9x_fonts_1bit)
set(LCD_DRIVER lcd_driver_aspi.cpp)
set(STATUS_LEDS YES)
# add_definitions(-DPCBX7 -DSOFTWARE_VOLUME)
add_definitions(-DPWR_BUTTON_${PWR_BUTTON})

set(DEFAULT_INTERNAL_MODULE NONE CACHE STRING "Default internal module")
set(HARDWARE_EXTERNAL_MODULE NO)
set(INTERNAL_MODULE_SERIAL NO)
set(MODULE_SIZE_STD YES)
set(FLAVOUR c7mini)
set(ROTARY_ENCODER NO)
# set(PXX2 ON)
set(USB_CHARGE_LED NO)
add_definitions(-DRADIO_C7MINI)
add_definitions(-DMANUFACTURER_MICROZONE)

# Internal flash FatFS volume size (bytes)
set(INTERNAL_FLASH_STORAGE_SIZE 65536 CACHE STRING "Internal flash storage size")
add_definitions(-DINTERNAL_FLASH_STORAGE_SIZE=${INTERNAL_FLASH_STORAGE_SIZE})



message("CPU_TYPE_FULL = ${CPU_TYPE_FULL}")
if(CPU_TYPE_FULL STREQUAL GD32F330RB)
  set(CPU_TYPE GD32F3x0)
  set(CPU_FAMILY GD32)
  add_definitions(-DGD32F330 -DGD32F3x0 -DGD32F330xB -DGD32F3)
  set(TARGET_LINKER_DIR gd32f330xb)
else()
  message(FATAL_ERROR "Unknown CPU_TYPE_FULL" )
endif()

if(FUNCTION_SWITCHES)
  add_definitions(-DFUNCTION_SWITCHES)
endif()

if(FUNCTION_SWITCHES_WITH_RGB)
  set(FUNCTION_SWITCHES YES)
  add_definitions(-DFUNCTION_SWITCHES)
  add_definitions(-DFUNCTION_SWITCHES_RGB_LEDS)
endif()

if(ENABLE_SERIAL_PASSTHROUGH)
  set(CLI ON "Enable CLI")
endif()

if(INTERNAL_MODULE_SERIAL)

  # defines default internal modules
  if(DEFAULT_INTERNAL_MODULE STREQUAL XJT_PXX1)
    set(INTERNAL_MODULES PXX1 CACHE STRING "Internal modules")
  elseif(DEFAULT_INTERNAL_MODULE STREQUAL ISRM_PXX2)
    set(INTERNAL_MODULES PXX2 CACHE STRING "Internal modules")
  else()
    set(INTERNAL_MODULES MULTI;CRSF CACHE STRING "Internal modules")
  endif()

  add_definitions(-DINTERNAL_MODULE_SERIAL)
  if(PXX_FREQUENCY STREQUAL HIGH)
    add_definitions(-DPXX_FREQUENCY_HIGH)
  endif()

else()

  # defines existing internal modules
  set(INTERNAL_MODULES NONE CACHE STRING "Internal modules")

endif()

set(TARGET_SRC_DIR targets/${TARGET_DIR})

set(BOARD_COMMON_SRC
  ${TARGET_SRC_DIR}/board.cpp
  ${TARGET_SRC_DIR}/led_driver.cpp
  # ${TARGET_SRC_DIR}/haptic_driver.cpp
  ${TARGET_SRC_DIR}/backlight_driver.cpp
  ${TARGET_SRC_DIR}/${LCD_DRIVER}
  targets/common/arm/gd32/abnormal_reboot.cpp
  targets/common/arm/gd32/delays_driver.cpp
  targets/common/arm/gd32/flash_driver.cpp
  targets/common/arm/gd32/pwr_driver.cpp
  # targets/common/arm/gd32/rtc_driver.cpp
  # targets/common/arm/gd32/watchdog_driver.cpp
)

if(ROTARY_ENCODER)
  list(APPEND BOARD_COMMON_SRC
    targets/common/arm/gd32/rotary_encoder_driver.cpp
  )
endif()

if(BLUETOOTH)
  list(APPEND BOARD_COMMON_SRC
    targets/common/arm/gd32/bluetooth_driver.cpp
  )
endif()

if(BOOTLOADER)
  # Bootloader board library
  add_library(board_bl OBJECT EXCLUDE_FROM_ALL
    ${BOARD_COMMON_SRC}
    # ${RADIO_SRC_DIR}/gui/common/stdlcd/boot_menu.cpp
  )
  target_include_directories(board_bl PRIVATE
    ${THIRDPARTY_DIR}/CMSIS/Device/GigaDevice/GD32F3x0/Include
    ${THIRDPARTY_DIR}/CMSIS/Include
    ${THIRDPARTY_DIR}/GD32F3x0_StdPeripheral_Driver/Inc)
  # add_dependencies(board_bl ${BITMAPS_TARGET})
  set(BOOTLOADER_SRC ${BOOTLOADER_SRC} $<TARGET_OBJECTS:board_bl>)
endif()

# Firmware board library
add_library(board OBJECT EXCLUDE_FROM_ALL
  ${BOARD_COMMON_SRC}
  # ${TARGET_SRC_DIR}/volume_i2c.cpp
  targets/common/arm/gd32/audio_buzzer_driver.cpp
  targets/common/arm/gd32/delays_driver.cpp
  targets/common/arm/gd32/heartbeat_driver.cpp
  # targets/common/arm/gd32/mixer_scheduler_driver.cpp
  targets/common/arm/gd32/module_timer_driver.cpp
  # targets/common/arm/gd32/trainer_driver.cpp
)
set(FIRMWARE_SRC ${FIRMWARE_SRC} $<TARGET_OBJECTS:board>)

set(GD32LIB_DIR ${THIRDPARTY_DIR}/GD32F3x0_StdPeripheral_Driver)

target_include_directories(board PRIVATE
  ${THIRDPARTY_DIR}/CMSIS/Device/GigaDevice/GD32F3x0/Include
  ${THIRDPARTY_DIR}/CMSIS/Include
  ${THIRDPARTY_DIR}/GD32F3x0_StdPeripheral_Driver/Inc)

if(STATUS_LEDS)
  target_sources(board PRIVATE ${TARGET_SRC_DIR}/led_driver.cpp)
endif()

if(BIND_KEY)
  add_definitions(-DBIND_KEY)
  target_sources(board PRIVATE ${TARGET_SRC_DIR}/bind_button_driver.cpp)
endif()

if(USB_CHARGE_LED)
  add_definitions(-DUSB_CHARGE_LED)
endif()

if(USB_CHARGER)
  add_definitions(-DUSB_CHARGER)
  target_sources(board PRIVATE ${TARGET_SRC_DIR}/usb_charger_driver.cpp)
endif()

if(IMU)
  target_sources(board PRIVATE targets/common/arm/gd32/lsm6ds_driver.cpp)
endif()

if(AUTOUPDATE)
  add_definitions(-DAUTOUPDATE)
endif()

if(USE_RTC_CLOCK)
  add_definitions(-DRTCLOCK)
endif()

if(RGBLEDS)
  add_definitions(-DRGBLEDS)
endif()

set(SRC ${SRC}
  # io/frsky_firmware_update.cpp
  # io/multi_firmware_update.cpp
)

set(GUI_SRC ${GUI_SRC}
  # model_input_edit.cpp
  # model_mix_edit.cpp
  # model_display.cpp
  radio_diagkeys.cpp
  radio_diaganas.cpp
  view_channels.cpp
  # view_telemetry.cpp
  view_about.cpp
  # bmp.cpp
)

if(BOOTLOADER)
  set(FIRMWARE_SRC
    ${FIRMWARE_SRC}
    bootloader/loadboot.cpp
    )
endif()

set(RADIO_DEPENDENCIES ${RADIO_DEPENDENCIES} ${BITMAPS_TARGET})
