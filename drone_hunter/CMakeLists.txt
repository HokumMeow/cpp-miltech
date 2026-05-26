cmake_minimum_required(VERSION 3.20)

# Окремий CMake-проєкт для домашньої роботи 06. Його можна збирати як через root CMakeLists.txt, так
# і напряму з цієї директорії.
project(ballistic CXX)

# Код домашньої роботи збирається з тим самим стандартом, що й лекційні приклади.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Бібліотека
add_library(json include/json.hpp)
target_include_directories(json PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(json PRIVATE -Wall -Wextra -pedantic)

# Виконуваний файл
add_executable(drone_hunter_cli src/main.cpp)
target_link_libraries(drone_hunter_cli PRIVATE json)
target_compile_options(drone_hunter_cli PRIVATE -Wall -Wextra -pedantic)

if(NOT CMAKE_CROSSCOMPILING)
    include(GoogleTest)
    add_executable(drone_hunter_tests tests/ballistics_tests.cpp)
    target_link_libraries(drone_hunter_tests PRIVATE json GTest::gtest_main)
    gtest_discover_tests(drone_hunter_tests DISCOVERY_MODE PRE_TEST)
endif()
