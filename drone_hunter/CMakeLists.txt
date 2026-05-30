cmake_minimum_required(VERSION 3.20)

# Окремий CMake-проєкт для домашньої роботи 06. Його можна збирати як через root CMakeLists.txt, так
# і напряму з цієї директорії.
project(ballistic CXX)

# Код домашньої роботи збирається з тим самим стандартом, що й лекційні приклади.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Бібліотека json
add_library(json INTERFACE)
target_include_directories(json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Виконуваний файл
add_executable(mission
    src/main.cpp
    src/Factory.cpp
    src/AnalyticalSolver.cpp
    src/TargetProvider.cpp
    src/FileConfigLoader.cpp
    src/MissionProcessor.cpp
)

target_include_directories(mission PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(mission PRIVATE json)

target_compile_options(mission PRIVATE -Wall -Wextra -pedantic)

#if(NOT CMAKE_CROSSCOMPILING)
#    include(GoogleTest)
#    add_executable(mission_tests tests/ballistics_tests.cpp)
#    target_link_libraries(mission_tests PRIVATE json GTest::gtest_main)
#    gtest_discover_tests(mission_tests DISCOVERY_MODE PRE_TEST)
#endif()
