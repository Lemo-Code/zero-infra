/*
  需求：编写参数服务端（持有参数的一方），完整演示「增、查、改、删」，
        并常驻 spin，供客户端 / ros2 param 远程操作。

  对应课程：2.5.3 参数服务 (C++)

  流程：
    1. 包含头文件
    2. 初始化 ROS2 客户端
    3. 定义节点类（可开启 allow_undeclared_parameters）
       3-1. 增：declare_parameter
       3-2. 查：get_parameter / has_parameter
       3-3. 改：set_parameter（本节点自改）+ on_set_parameters（远端改时的闸门）
       3-4. 删：undeclare_parameter（演示用临时参数）
    4. 调用 spin，持续提供参数服务
    5. 资源释放

  本节点长期保留的参数：
    car_name   string   默认 "turtle"
    width      double   默认 0.25   （0.1 ~ 1.0）
    length     double   默认 0.45   （0.1 ~ 2.0）

  运行：
    ros2 run cpp04_param demo01_param_server
    ros2 param list
    ros2 param get /param_server_node car_name
    ros2 param set /param_server_node width 0.30
*/

#include "rclcpp/rclcpp.hpp"
#include "rcl_interfaces/msg/set_parameters_result.hpp"

#include <string>
#include <vector>

using namespace std::chrono_literals;

class ParamServer : public rclcpp::Node
{
public:
  ParamServer()
  : Node(
      "param_server_node",
      // 课件常见：允许未 declare 的参数被 set（会自动声明）
      // 正式项目更推荐：只用 declare 过的参数，便于类型与范围管理
      rclcpp::NodeOptions().allow_undeclared_parameters(true))
  {
    RCLCPP_INFO(this->get_logger(), "参数服务端创建");

    // ---------- 3-1. 增 ----------
    this->declare_parameter<std::string>("car_name", "turtle");
    this->declare_parameter<double>("width", 0.25);
    this->declare_parameter<double>("length", 0.45);
    // 临时参数：后面专门用来演示「删」
    this->declare_parameter<bool>("tmp_flag", true);
    RCLCPP_INFO(this->get_logger(), "[增] declare car_name / width / length / tmp_flag");

    // ---------- 3-2. 查 ----------
    refresh_from_node();
    RCLCPP_INFO(
      this->get_logger(),
      "[查] car_name=%s, width=%.3f, length=%.3f, tmp_flag=%s",
      car_name_.c_str(), width_, length_,
      this->get_parameter("tmp_flag").as_bool() ? "true" : "false");

    // ---------- 3-3. 改（本节点自己改一遍，证明 set_parameter 可用）----------
    this->set_parameter(rclcpp::Parameter("car_name", "turtle1"));
    car_name_ = this->get_parameter("car_name").as_string();
    RCLCPP_INFO(this->get_logger(), "[改] 本节点自改 car_name -> %s", car_name_.c_str());

    // 远端 set 时的校验回调（客户端 / ros2 param set 都会进这里）
    param_cb_handle_ = this->add_on_set_parameters_callback(
      std::bind(&ParamServer::on_set_parameters, this, std::placeholders::_1));

    // ---------- 3-4. 删 ----------
    this->undeclare_parameter("tmp_flag");
    RCLCPP_INFO(
      this->get_logger(),
      "[删] undeclare tmp_flag, has_parameter=%s",
      this->has_parameter("tmp_flag") ? "true" : "false");

    // 定时打印，方便观察远端改参是否生效
    timer_ = this->create_wall_timer(
      2s, std::bind(&ParamServer::on_timer, this));
  }

private:
  void refresh_from_node()
  {
    car_name_ = this->get_parameter("car_name").as_string();
    width_ = this->get_parameter("width").as_double();
    length_ = this->get_parameter("length").as_double();
  }

  rcl_interfaces::msg::SetParametersResult on_set_parameters(
    const std::vector<rclcpp::Parameter> & parameters)
  {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;

    for (const auto & p : parameters) {
      RCLCPP_INFO(this->get_logger(), "[远端改] 请求设置: %s", p.get_name().c_str());

      if (p.get_name() == "car_name") {
        if (p.as_string().empty()) {
          result.successful = false;
          result.reason = "car_name 不能为空";
          return result;
        }
        car_name_ = p.as_string();
      } else if (p.get_name() == "width") {
        const double v = p.as_double();
        if (v < 0.1 || v > 1.0) {
          result.successful = false;
          result.reason = "width 必须在 0.1 ~ 1.0";
          return result;
        }
        width_ = v;
      } else if (p.get_name() == "length") {
        const double v = p.as_double();
        if (v < 0.1 || v > 2.0) {
          result.successful = false;
          result.reason = "length 必须在 0.1 ~ 2.0";
          return result;
        }
        length_ = v;
      }
      // 其它名字：因 allow_undeclared_parameters，可能是新参数，默认放行
    }
    return result;
  }

  void on_timer()
  {
    RCLCPP_INFO(
      this->get_logger(),
      "当前参数: car_name=%s, width=%.3f, length=%.3f",
      car_name_.c_str(), width_, length_);
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr param_cb_handle_;

  std::string car_name_;
  double width_{0.0};
  double length_{0.0};
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ParamServer>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
