cmake_minimum_required(VERSION 3.20)

# Прошивка hw18 для STM32F4-Discovery окремий CMake-проєкт,
#
#   cmake -S homework_18/firmware -B build-firmware \
#         --toolchain ../../cmake/toolchains/arm-none-eabi-stm32f4.cmake
#   cmake --build build-firmware
#

project(hw18_firmware LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(cmsis_core
  GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_core.git
  GIT_TAG        v5.9.0
  GIT_SHALLOW    TRUE
)
FetchContent_Declare(cmsis_device_f4
  GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_f4.git
  GIT_TAG        v2.6.11
  GIT_SHALLOW    TRUE
)
FetchContent_Declare(stm32f4xx_hal_driver
  GIT_REPOSITORY https://github.com/STMicroelectronics/stm32f4xx_hal_driver.git
  GIT_TAG        v1.8.5
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(cmsis_core cmsis_device_f4 stm32f4xx_hal_driver)

file(GLOB hal_sources ${stm32f4xx_hal_driver_SOURCE_DIR}/Src/*.c)
list(FILTER hal_sources EXCLUDE REGEX "_template\\.c$")

add_library(hw18_hal STATIC ${hal_sources})
target_include_directories(hw18_hal PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
  ${stm32f4xx_hal_driver_SOURCE_DIR}/Inc
  ${cmsis_device_f4_SOURCE_DIR}/Include
  ${cmsis_core_SOURCE_DIR}/Core/Include
)
target_compile_definitions(hw18_hal PUBLIC STM32F407xx USE_HAL_DRIVER)
target_compile_options(hw18_hal PRIVATE -Os -ffunction-sections -fdata-sections -Wno-unused-parameter)

add_library(hw18_cmsis_startup STATIC
  ${cmsis_device_f4_SOURCE_DIR}/Source/Templates/system_stm32f4xx.c
  ${cmsis_device_f4_SOURCE_DIR}/Source/Templates/gcc/startup_stm32f407xx.s
)
target_include_directories(hw18_cmsis_startup PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
  ${stm32f4xx_hal_driver_SOURCE_DIR}/Inc
  ${cmsis_device_f4_SOURCE_DIR}/Include
  ${cmsis_core_SOURCE_DIR}/Core/Include
)
target_compile_definitions(hw18_cmsis_startup PUBLIC STM32F407xx USE_HAL_DRIVER)
target_compile_options(hw18_cmsis_startup PRIVATE -Os -ffunction-sections -fdata-sections)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../core ${CMAKE_CURRENT_BINARY_DIR}/hw18_core)

add_executable(hw18_firmware.elf
  src/main.cpp
  src/app.cpp
  src/lis3dsh.cpp
  src/syscalls.c
)
target_link_libraries(hw18_firmware.elf PRIVATE hw18_core hw18_hal hw18_cmsis_startup)
target_compile_definitions(hw18_firmware.elf PRIVATE STM32F407xx USE_HAL_DRIVER)
target_compile_options(hw18_firmware.elf PRIVATE
  -Os -ffunction-sections -fdata-sections -Wall -Wextra
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>
)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F407VGTX_FLASH.ld)
target_link_options(hw18_firmware.elf PRIVATE
  -T${LINKER_SCRIPT}
  -nostartfiles
  --specs=nano.specs
  -Wl,--gc-sections
  -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/hw18_firmware.map
  -Wl,--print-memory-usage
)
set_target_properties(hw18_firmware.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

# .bin для прошивання + команда size, що в лекції
add_custom_command(TARGET hw18_firmware.elf POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary
          $<TARGET_FILE:hw18_firmware.elf>
          ${CMAKE_CURRENT_BINARY_DIR}/hw18_firmware.bin
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:hw18_firmware.elf>
  COMMENT "hw18_firmware.bin + розмір секцій"
)
