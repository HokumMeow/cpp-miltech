cmake_minimum_required(VERSION 3.20)

# Окремий CMake-проєкт для домашньої роботи 06. Його можна збирати як
# через root CMakeLists.txt, так і напряму з цієї директорії.
project(ballistic CXX)

# Код домашньої роботи збирається з тим самим стандартом, що й лекційні приклади.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Бібліотека
add_library(ballistics src/ballistics.cpp)
target_include_directories(ballistics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(ballistics PRIVATE -Wall -Wextra -pedantic)

# Виконуваний файл
add_executable(ballistics_cli src/main.cpp)
target_link_libraries(ballistics_cli PRIVATE ballistics)
target_compile_options(ballistics_cli PRIVATE -Wall -Wextra -pedantic)

if(NOT CMAKE_CROSSCOMPILING)
include(FetchContent)
include(GoogleTest)
add_executable(ballistics_tests tests/ballistics_tests.cpp)
target_link_libraries(ballistics_tests PRIVATE ballistics GTest::gtest_main)
gtest_discover_tests(ballistics_tests DISCOVERY_MODE PRE_TEST)
endif()
