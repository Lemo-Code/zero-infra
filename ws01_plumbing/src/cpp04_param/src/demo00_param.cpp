/*
  ============================================================
  demo00_param —— 参数「增删改查」入门（单节点，跑完即退出）
  ============================================================
  对应课程：2.5.3 参数服务 (C++) / 第 04 节入门

  学习目标：
    搞清四个动作分别用哪个 API，不涉及远端客户端。

  流程：
    1. 包含头文件
    2. 初始化 ROS2
    3. 自定义节点，构造函数里依次调用：
       3-1. declare_param()  —— 增
       3-2. get_param()      —— 查
       3-3. update_param()   —— 改
       3-4. del_param()      —— 删
    4. 演示结束，shutdown（本文件不长期 spin）

  运行：
    source install/setup.bash
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
      // 允许操作尚未 declare 的参数（set 时会自动声明）
      // 旧课件若写 NodeOptions().all()，Jazzy 请改用下面这一句
      rclcpp::NodeOptions().allow_undeclared_parameters(true))
  {
    RCLCPP_INFO(this->get_logger(), "======== 参数增删改查演示开始 ========");
    declare_param();  // 3-1. 增
    get_param();      // 3-2. 查
    update_param();   // 3-3. 改
    del_param();      // 3-4. 删
    RCLCPP_INFO(this->get_logger(), "======== 参数增删改查演示结束 ========");
  }

private:
  // ---------- 3-1. 增 ----------
  void declare_param()
  {
    RCLCPP_INFO(this->get_logger(), "-------- 增 --------");
    this->declare_parameter<std::string>("car_name", "tiger");
    this->declare_parameter<double>("width", 0.15);
    this->declare_parameter<double>("length", 0.40);
    this->declare_parameter<bool>("tmp_flag", true);
    RCLCPP_INFO(this->get_logger(), "已 declare: car_name / width / length / tmp_flag");
  }

  // ---------- 3-2. 查 ----------
  void get_param()
  {
    RCLCPP_INFO(this->get_logger(), "-------- 查 --------");

    // 写法 A：先拿到 Parameter，再 as_*
    auto car = this->get_parameter("car_name");
    RCLCPP_INFO(this->get_logger(), "car_name = %s", car.as_string().c_str());

    // 写法 B：直接灌进变量
    double width = 0.0;
    double length = 0.0;
    this->get_parameter("width", width);
    this->get_parameter("length", length);
    RCLCPP_INFO(this->get_logger(), "width=%.3f, length=%.3f", width, length);

    RCLCPP_INFO(
      this->get_logger(),
      "has car_name? %s | has tmp_flag? %s | has no_such? %s",
      this->has_parameter("car_name") ? "yes" : "no",
      this->has_parameter("tmp_flag") ? "yes" : "no",
      this->has_parameter("no_such") ? "yes" : "no");
  }

  // ---------- 3-3. 改 ----------
  void update_param()
  {
    RCLCPP_INFO(this->get_logger(), "-------- 改 --------");
    this->set_parameter(rclcpp::Parameter("car_name", "lion"));
    this->set_parameter(rclcpp::Parameter("width", 0.20));
    this->set_parameter(rclcpp::Parameter("length", 0.45));

    // allow_undeclared_parameters(true) 时：没 declare 过也能 set（自动声明）
    this->set_parameter(rclcpp::Parameter("height", 0.10));

    RCLCPP_INFO(
      this->get_logger(),
      "改后: car_name=%s, width=%.3f, length=%.3f, height=%.3f",
      this->get_parameter("car_name").as_string().c_str(),
      this->get_parameter("width").as_double(),
      this->get_parameter("length").as_double(),
      this->get_parameter("height").as_double());
  }

  // ---------- 3-4. 删 ----------
  void del_param()
  {
    RCLCPP_INFO(this->get_logger(), "-------- 删 --------");
    this->undeclare_parameter("tmp_flag");
    this->undeclare_parameter("height");

    RCLCPP_INFO(
      this->get_logger(),
      "删后: has tmp_flag? %s | has height? %s",
      this->has_parameter("tmp_flag") ? "yes" : "no",
      this->has_parameter("height") ? "yes" : "no");
  }
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ParamDemo>();
  // 逻辑全在构造函数里完成，无需 spin
  rclcpp::shutdown();
  return 0;
}
