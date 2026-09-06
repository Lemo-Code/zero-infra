cmake_minimum_required(VERSION 3.8)
project(cpp01_topic)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(base_interfaces_demo REQUIRED)

add_executable(demo01_talker src/demo01_talker.cpp)
ament_target_dependencies(demo01_talker rclcpp base_interfaces_demo)

add_executable(demo02_listener src/demo02_listener.cpp)
ament_target_dependencies(demo02_listener rclcpp base_interfaces_demo)

install(TARGETS
  demo01_talker
  demo02_listener
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
