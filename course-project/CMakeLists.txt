cmake_minimum_required(VERSION 3.20)

# Курсова робота: автономна система повернення додому (RTH) з реальною телеметрією
# Збирається через root CMakeLists.txt або напряму з цієї директорії
project(rth_system CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Уся логіка й драйвери в одній бібліотеці
add_library(rth_lib
    src/common/Geo.cpp
    src/common/Attitude.cpp
    src/sensors/I2cBus.cpp
    src/sensors/Mpu9250.cpp
    src/sensors/Ak8963.cpp
    src/sensors/Bmp388.cpp
    src/sensors/NmeaParser.cpp
    src/sensors/GpsReceiver.cpp
    src/source/ReplayTelemetrySource.cpp
    src/source/KeyboardLink.cpp
    src/source/HardwareTelemetrySource.cpp
    src/failsafe/FailsafeFsm.cpp
    src/nav/Navigator.cpp
    src/logging/RouteLogger.cpp
)
target_include_directories(rth_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(rth_lib PRIVATE -Wall -Wextra -pedantic)

add_executable(rth_app src/main.cpp)
target_link_libraries(rth_app PRIVATE rth_lib)
target_compile_options(rth_app PRIVATE -Wall -Wextra -pedantic)
