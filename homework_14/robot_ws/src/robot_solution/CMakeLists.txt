cmake_minimum_required(VERSION 3.20)
project(robot_solution)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()

ament_auto_add_executable(mission_explorer_node src/mission_explorer_node.cpp)
ament_auto_add_executable(payload_action_node src/payload_action_node.cpp)

ament_auto_package()
