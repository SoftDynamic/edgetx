set(CPU_FAMILY GD32)
set(MCU cortex-m4)
set(FPU_FLAGS "-mfloat-abi=hard -mfpu=fpv4-sp-d16")

if(NOT NATIVE_BUILD)

  # Add MCU specific flags
  add_link_options(
    -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
  )

  # GD32 Standard Peripheral Library
  set(GD32LIB_DIR ${THIRDPARTY_DIR}/GD32F3x0_StdPeripheral_Driver)
  # include_directories(${GD32STDLIB_DIR}/Inc)

  add_library(gd32_stdperiph OBJECT EXCLUDE_FROM_ALL
    targets/common/arm/gd32/f3x0/system_gd32f3x0.c
    boards/generic_gd32/startup/startup_gd32f3x0.s
    ${GD32LIB_DIR}/Src/gd32f3x0_gpio.c
    ${GD32LIB_DIR}/Src/gd32f3x0_timer.c
    ${GD32LIB_DIR}/Src/gd32f3x0_dma.c
    ${GD32LIB_DIR}/Src/gd32f3x0_usart.c
    ${GD32LIB_DIR}/Src/gd32f3x0_rcu.c
    ${GD32LIB_DIR}/Src/gd32f3x0_exti.c
    ${GD32LIB_DIR}/Src/gd32f3x0_adc.c
    )

  # target_compile_options(gd32stdlib PRIVATE -DUSE_FULL_LL_DRIVER)
  target_include_directories(gd32_stdperiph PRIVATE
    ${GD32LIB_DIR}/Inc
    ${THIRDPARTY_DIR}/CMSIS/Device/GigaDevice/GD32F3x0/Include
    ${THIRDPARTY_DIR}/CMSIS/Include)

  # set(FIRMWARE_SRC ${FIRMWARE_SRC} $<TARGET_OBJECTS:gd32_stdperiph>)
  # set(BOOTLOADER_SRC ${BOOTLOADER_SRC} $<TARGET_OBJECTS:gd32_stdperiph>)
endif()

include_directories(${THIRDPARTY_DIR}/CMSIS/Device/GigaDevice/GD32F3x0/Include)
include_directories(${THIRDPARTY_DIR}/CMSIS/Include)
include_directories(${THIRDPARTY_DIR}/GD32F3x0_StdPeripheral_Driver/Inc)

add_definitions(-DGD32F3 -DGD32F3x0 -D__FPU_PRESENT=1)

set(FIRMWARE_SRC ${FIRMWARE_SRC} $<TARGET_OBJECTS:gd32_stdperiph>)
set(BOOTLOADER_SRC ${BOOTLOADER_SRC} $<TARGET_OBJECTS:gd32_stdperiph>)

if(NOT NATIVE_BUILD)
  include_directories(targets/common/arm/gd32/f3x0)

  #
  # FreeRTOS port specific include
  #
  add_library(freertos OBJECT EXCLUDE_FROM_ALL
    ${RTOS_DIR}/list.c
    ${RTOS_DIR}/queue.c
    ${RTOS_DIR}/tasks.c
    ${RTOS_DIR}/event_groups.c
    ${RTOS_DIR}/timers.c
    ${RTOS_DIR}/stream_buffer.c
    ${RTOS_DIR}/portable/GCC/ARM_CM4F/port.c
    )

  target_include_directories(freertos
    PUBLIC  ${RTOS_DIR}/portable/GCC/ARM_CM4F
    PRIVATE ${RTOS_DIR}/include
    )

  include_directories(${RTOS_DIR}/portable/GCC/ARM_CM4F)

  # EdgeTx Compat layer
  set(FIRMWARE_SRC ${FIRMWARE_SRC} rtos.c)

  #
  # System and FreeRTOS port files
  #
  set(FIRMWARE_SRC ${FIRMWARE_SRC} $<TARGET_OBJECTS:freertos>)
endif()

