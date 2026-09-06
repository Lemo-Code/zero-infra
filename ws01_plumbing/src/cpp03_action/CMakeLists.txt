cmake_minimum_required(VERSION 3.8)
project(cpp03_action)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(base_interfaces_demo REQUIRED)

add_executable(demo01_action_server src/demo01_action_server.cpp)
ament_target_dependencies(demo01_action_server
  rclcpp rclcpp_action base_interfaces_demo)

add_executable(demo02_action_client src/demo02_action_client.cpp)
ament_target_dependencies(demo02_action_client
  rclcpp rclcpp_action base_interfaces_demo)

install(TARGETS
  demo01_action_server
  demo02_action_client
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
