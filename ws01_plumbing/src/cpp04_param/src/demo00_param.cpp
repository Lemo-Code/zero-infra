/*
  需求：单节点演示参数的「增、查、改、删」（不涉及远端客户端）。

  对应课程：2.5.3 参数服务 (C++) —— 入门

  流程：
    1. 包含头文件
    2. 初始化 ROS2
    3. 自定义节点，在构造函数里演示：
       3-1. 增：declare_parameter
       3-2. 查：get_parameter
       3-3. 改：set_parameter
       3-4. 删：undeclare_parameter
    4. 演示完即可退出（本文件不长期 spin）

  运行：
    ros2 run cpp04_param demo00_param
*/

#include "rclcpp/rclcpp.hpp"

#include <string>

class ParamDemo : public rclcpp::Node
{
public:
  ParamDemo()
  : Node(
      "param_demo_node",
      // 允许操作「尚未 declare」的参数（设置时会自动声明）
      // 课件里常见写法；正式项目更推荐先 declare 再使用
      rclcpp::NodeOptions().allow_undeclared_parameters(true))
  {
    RCLCPP_INFO(this->get_logger(), "===== 参数增删改查演示开始 =====");

    // ========== 3-1. 增：声明参数 ==========
    // 声明后，本节点 / 外部 ros2 param / 参数客户端才能稳定读写
    this->declare_parameter<std::string>("car_name", "tiger");
    this->declare_parameter<double>("width", 0.15);
    this->declare_parameter<double>("length", 0.40);
    RCLCPP_INFO(this->get_logger(), "[增] 已 declare: car_name / width / length");

    // ========== 3-2. 查：读取参数 ==========
    const std::string name = this->get_parameter("car_name").as_string();
    const double width = this->get_parameter("width").as_double();
    const double length = this->get_parameter("length").as_double();
    RCLCPP_INFO(
      this->get_logger(),
      "[查] car_name=%s, width=%.3f, length=%.3f",
      name.c_str(), width, length);

    // ========== 3-3. 改：设置参数 ==========
    this->set_parameter(rclcpp::Parameter("car_name", "lion"));
    this->set_parameter(rclcpp::Parameter("width", 0.20));
    RCLCPP_INFO(
      this->get_logger(),
      "[改] 之后 car_name=%s, width=%.3f",
      this->get_parameter("car_name").as_string().c_str(),
      this->get_parameter("width").as_double());

    // 因为 allow_undeclared_parameters(true)，也可以直接 set 一个没 declare 过的名
    this->set_parameter(rclcpp::Parameter("height", 0.10));
    RCLCPP_INFO(
      this->get_logger(),
      "[改/自动声明] height=%.3f",
      this->get_parameter("height").as_double());

    // ========== 3-4. 删：取消声明 ==========
    // undeclare 之后再 get 会抛异常（或失败），相当于从本节点参数表移除
    this->undeclare_parameter("height");
    RCLCPP_INFO(this->get_logger(), "[删] 已 undeclare height");

    if (!this->has_parameter("height")) {
      RCLCPP_INFO(this->get_logger(), "[删] 确认：height 已不存在");
    }

    RCLCPP_INFO(this->get_logger(), "===== 参数增删改查演示结束 =====");
  }
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ParamDemo>();
  // 本 demo 逻辑全在构造函数里跑完，无需 spin
  rclcpp::shutdown();
  return 0;
}
