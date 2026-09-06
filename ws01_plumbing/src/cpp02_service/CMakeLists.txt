cmake_minimum_required(VERSION 3.8)
project(cpp02_service)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(base_interfaces_demo REQUIRED)

add_executable(demo01_server src/demo01_server.cpp)
ament_target_dependencies(demo01_server rclcpp base_interfaces_demo)

add_executable(demo02_client src/demo02_client.cpp)
ament_target_dependencies(demo02_client rclcpp base_interfaces_demo)

install(TARGETS
  demo01_server
  demo02_client
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
