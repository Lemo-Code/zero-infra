/*
  需求：编写「参数服务端」节点（持有参数的一方）。
        声明若干参数，定时打印；外部修改参数时通过回调更新并校验。

  对应课程：2.5.3 参数服务 (C++)

  概念：
    - 每个 Node 自带参数接口（底层是一组标准服务，如 get/set/list/describe）
    - 本节点「声明 + 持有」参数；其它节点或 ros2 param 命令可远程读写
    - add_on_set_parameters_callback：参数被设置前触发，可接受或拒绝

  本 demo 参数：
    car_name   string   默认 "turtle"
    width      double   默认 0.25   （建议 0.1 ~ 1.0）
    length     double   默认 0.45   （建议 0.1 ~ 2.0）

  运行：
    ros2 run cpp04_param demo01_param_server
    # 另开终端：
    ros2 param list
    ros2 param get /param_server_node car_name
    ros2 param set /param_server_node width 0.30
*/

#include "rclcpp/rclcpp.hpp"
#include "rcl_interfaces/msg/set_parameters_result.hpp"

#include <vector>
#include <string>

using namespace std::chrono_literals;

class ParamServer : public rclcpp::Node
{
public:
  ParamServer()
  : Node("param_server_node")
  {
    RCLCPP_INFO(this->get_logger(), "参数服务端节点已创建");

    // ---------- ① 声明参数（必须先 declare，才能被 get / 远程 set）----------
    this->declare_parameter<std::string>("car_name", "turtle");
    this->declare_parameter<double>("width", 0.25);
    this->declare_parameter<double>("length", 0.45);

    // 读出当前值到成员，后面定时器直接用
    refresh_from_node();

    // ---------- ② 注册「设置参数」回调：外部 set 时会进这里 ----------
    // 返回 successful=false 可拒绝本次修改（例如越界）
    param_cb_handle_ = this->add_on_set_parameters_callback(
      std::bind(&ParamServer::on_set_parameters, this, std::placeholders::_1));

    // ---------- ③ 定时打印，观察参数是否被改掉 ----------
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

  // parameters：本次要设置的参数列表（可能一次改多个）
  rcl_interfaces::msg::SetParametersResult on_set_parameters(
    const std::vector<rclcpp::Parameter> & parameters)
  {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;

    for (const auto & p : parameters) {
      RCLCPP_INFO(this->get_logger(), "收到参数设置请求: %s", p.get_name().c_str());

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
  // spin：定时器 + 参数服务回调都靠它调度
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
