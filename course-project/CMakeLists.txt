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

# Тести потребують GoogleTest, який підключає root CMakeLists.txt
if(NOT CMAKE_CROSSCOMPILING AND TARGET GTest::gtest_main)
    include(GoogleTest)
    add_executable(rth_tests
        tests/geo_tests.cpp
        tests/attitude_tests.cpp
        tests/nmea_tests.cpp
        tests/bmp388_tests.cpp
        tests/mpu9250_tests.cpp
        tests/failsafe_tests.cpp
        tests/nav_tests.cpp
        tests/replay_logger_tests.cpp
    )
    target_link_libraries(rth_tests PRIVATE rth_lib GTest::gtest_main)
    gtest_discover_tests(rth_tests DISCOVERY_MODE PRE_TEST)
else()
    # При збірці напряму з цієї директорії GoogleTest ніхто не підключив - тестів просто не буде
    message(STATUS "rth_tests пропущено: GoogleTest недоступний")
endif()
