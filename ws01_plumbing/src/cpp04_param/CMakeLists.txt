cmake_minimum_required(VERSION 3.8)
project(cpp04_param)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rcl_interfaces REQUIRED)

add_executable(demo00_param src/demo00_param.cpp)
ament_target_dependencies(demo00_param rclcpp rcl_interfaces)

add_executable(demo01_param_server src/demo01_param_server.cpp)
ament_target_dependencies(demo01_param_server rclcpp rcl_interfaces)

add_executable(demo02_param_client src/demo02_param_client.cpp)
ament_target_dependencies(demo02_param_client rclcpp rcl_interfaces)

install(TARGETS
  demo00_param
  demo01_param_server
  demo02_param_client
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
