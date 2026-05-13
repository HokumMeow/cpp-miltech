cmake_minimum_required(VERSION 3.20)

# Окремий CMake-проєкт для домашньої роботи 05. Його можна збирати як
# через root CMakeLists.txt, так і напряму з цієї директорії.
project(telemetry CXX)

# Код домашньої роботи збирається з тим самим стандартом, що й лекційні приклади.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Бібліотека
add_library(telemetry src/telemetry.cpp)
target_include_directories(telemetry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(telemetry PRIVATE -Wall -Wextra -pedantic)

# Виконуваний файл
add_executable(telemetry_check src/main.cpp)
target_link_libraries(telemetry_check PRIVATE telemetry)
target_compile_options(telemetry_check PRIVATE -Wall -Wextra -pedantic)
