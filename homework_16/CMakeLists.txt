cmake_minimum_required(VERSION 3.20)

# Окремий CMake-проєкт для домашньої роботи 16. Його можна збирати як
# через root CMakeLists.txt, так і напряму з цієї директорії.
project(mpu9250_sensor CXX)

# Код домашньої роботи збирається з тим самим стандартом, що й лекційні приклади.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# i2c-dev + MPU-9250
add_library(mpu9250_sensor src/I2cBus.cpp src/Mpu9250.cpp)
target_include_directories(mpu9250_sensor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(mpu9250_sensor PRIVATE -Wall -Wextra -pedantic)

# Виконуваний файл
add_executable(mpu9250_reader src/main.cpp)
target_link_libraries(mpu9250_reader PRIVATE mpu9250_sensor)
target_compile_options(mpu9250_reader PRIVATE -Wall -Wextra -pedantic)
