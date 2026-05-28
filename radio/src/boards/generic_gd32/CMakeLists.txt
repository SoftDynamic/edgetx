
# Generate hardware struct defs
AddHWGenTarget(${HW_DESC_JSON} gd32_keys gd32_keys.inc)
AddHWGenTarget(${HW_DESC_JSON} gd32_switches gd32_switches.inc)
AddHWGenTarget(${HW_DESC_JSON} gd32_adc_inputs gd32_adc_inputs.inc)
AddHWGenTarget(${HW_DESC_JSON} gd32_pwm_inputs gd32_pwm_inputs.inc)
AddHWGenTarget(${HW_DESC_JSON} hal_adc_inputs hal_adc_inputs.inc)

# Dependencies common to bootloader and firmware
set(MINIMAL_BOARD_LIB_SRC
  ${CMAKE_CURRENT_BINARY_DIR}/${HW_DESC_JSON}
  ${CMAKE_CURRENT_BINARY_DIR}/gd32_keys.inc

  boards/generic_gd32/inputs.cpp
  boards/generic_gd32/bl_keys.cpp
)

# Dependencies only used in firmware
set(BOARD_LIB_SRC
  ${CMAKE_CURRENT_BINARY_DIR}/gd32_switches.inc
  ${CMAKE_CURRENT_BINARY_DIR}/gd32_adc_inputs.inc
  ${CMAKE_CURRENT_BINARY_DIR}/gd32_pwm_inputs.inc
  ${CMAKE_CURRENT_BINARY_DIR}/hal_adc_inputs.inc

  boards/generic_gd32/module_ports.cpp
  boards/generic_gd32/analog_inputs.cpp
  boards/generic_gd32/switches.cpp
)

add_library(minimal_board_lib OBJECT EXCLUDE_FROM_ALL ${MINIMAL_BOARD_LIB_SRC})

add_library(board_lib OBJECT EXCLUDE_FROM_ALL ${BOARD_LIB_SRC})
add_dependencies(board_lib minimal_board_lib)

if(DEBUG)
  target_compile_definitions(board_lib PRIVATE DEBUG)
endif()

set(FIRMWARE_SRC ${FIRMWARE_SRC}
  $<TARGET_OBJECTS:minimal_board_lib>
  $<TARGET_OBJECTS:board_lib>
)

set(BOOTLOADER_SRC ${BOOTLOADER_SRC}
  $<TARGET_OBJECTS:minimal_board_lib>
)
