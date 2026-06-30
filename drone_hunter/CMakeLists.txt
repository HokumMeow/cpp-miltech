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

# Виконуваний файл
add_executable(drone_hunter
    src/main.cpp
    src/Factory.cpp
    src/solvers/AnalyticalSolver.cpp
    src/providers/TargetProvider.cpp
    src/config/FileConfigLoader.cpp
    src/MissionProcessor.cpp
)

target_include_directories(drone_hunter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(drone_hunter PRIVATE json)

target_compile_options(drone_hunter PRIVATE -Wall -Wextra -pedantic)
