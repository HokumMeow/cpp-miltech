# Toolchain для крос-компіляції bare-metal прошивки STM32F407 (F4 Discovery)
# з Linux x86_64 devcontainer-а. Пакет: gcc-arm-none-eabi (Debian/Ubuntu).
# Немає операційної системи під прошивкою -> CMAKE_SYSTEM_NAME Generic:
# CMake не намагається лінкувати системні бібліотеки хоста чи запускати
# try_run на цільовому "процесорі", якого тут немає.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy CACHE FILEPATH "")
set(CMAKE_SIZE arm-none-eabi-size CACHE FILEPATH "")

# Прошивка не має main() у сенсі хостової ОС і не лінкується проти crt0 з
# libc хоста - тому CMake не може виконати стандартну перевірку компілятора
# лінкуванням виконуваного файлу. TRY_COMPILE зупиняється на етапі .o.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Ядро Cortex-M4F (STM32F407VGTx на платі STM32F4-Discovery):
# апаratний FPU одинарної точності, hard float ABI, Thumb-2.
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${CPU_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS}")

# Пошук headers/бібліотек - лише у sysroot тулчейна, програми - на хості
# (не намагатись запускати arm-none-eabi-* як "знайдені" host-програми).
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
