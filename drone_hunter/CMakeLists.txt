cmake_minimum_required(VERSION 3.20)

# Окремий CMake-проєкт для домашньої роботи 07. Його можна збирати як через root CMakeLists.txt, так
# і напряму з цієї директорії.
project(drone_hunter CXX)

# Код домашньої роботи збирається з тим самим стандартом, що й лекційні приклади.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Бібліотека json
add_library(json INTERFACE)
target_include_directories(json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(Threads REQUIRED)

# ДЗ11: libgpiod (керування START/DROP через GpioLink) — та сама бібліотека
# і той самий код на платі й у симуляції, шукаємо через pkg-config.
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)

# Виконуваний файл
add_executable(drone_hunter
    src/main.cpp
    src/Factory.cpp
    src/solvers/AnalyticalSolver.cpp
    src/solvers/TableSolver.cpp
    src/providers/ThreadSafeTargetProvider.cpp
    src/providers/UartTargetProvider.cpp
    src/config/FileConfigLoader.cpp
    src/config/UartConfigLoader.cpp
    src/MissionProcessor.cpp
    src/DronePhysics.cpp
    src/engine/RemoteDronePhysics.cpp
    src/link/UartLink.cpp
    src/link/GpioLink.cpp
    src/states/StateStopped.cpp
    src/states/StateAccelerating.cpp
    src/states/StateDecelerating.cpp
    src/states/StateMoving.cpp
    src/states/StateTurning.cpp
)

target_include_directories(drone_hunter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(drone_hunter PRIVATE json Threads::Threads PkgConfig::GPIOD)

target_compile_options(drone_hunter PRIVATE -Wall -Wextra -pedantic)
