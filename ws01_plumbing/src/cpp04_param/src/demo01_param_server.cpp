/*
  ============================================================
  demo01_param_server —— 参数服务端（课件标准结构）
  ============================================================
  对应课程：2.5.3 参数服务 (C++) / 第 04 节主演示

  学习目标：
    1. 用四个成员函数分别演示：增 / 查 / 改 / 删
    2. 节点常驻 spin，对外提供参数服务
    3. 配合 demo02 / ros2 param 做远程读写
    4. on_set_parameters 做范围校验（非法值可拒绝）

  流程：
    1. 包含头文件
    2. 初始化 ROS2 客户端
    3. 自定义节点类
       3-1. declare_param()  —— 增
       3-2. get_param()      —— 查
       3-3. update_param()   —— 改
       3-4. del_param()      —— 删
    4. 调用 spin
    5. 资源释放

  长期保留参数（供客户端使用）：
    car_name  string  默认 "turtle"
    width     double  默认 0.15   （合法 0.1 ~ 1.0）
    length    double  默认 0.40   （合法 0.1 ~ 2.0）

  运行：
    ros2 run cpp04_param demo01_param_server
*/

#include "rclcpp/rclcpp.hpp"
#include "rcl_interfaces/msg/parameter_descriptor.hpp"
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
      // 课件常见：允许未声明参数被 set（自动声明）
      // 正式项目更推荐只操作已 declare 的参数
      rclcpp::NodeOptions().allow_undeclared_parameters(true))
  {
    RCLCPP_INFO(this->get_logger(), "参数服务端创建");

    declare_param();  // 3-1. 增
    get_param();      // 3-2. 查
    update_param();   // 3-3. 改
    del_param();      // 3-4. 删

    // 远端 set 时的闸门（demo02 / ros2 param set 都会进这里）
    param_cb_handle_ = this->add_on_set_parameters_callback(
      std::bind(&ParamServer::on_set_parameters, this, std::placeholders::_1));

    // 定时打印，方便观察远端改参是否生效
    timer_ = this->create_wall_timer(
      3s, std::bind(&ParamServer::on_timer, this));
  }

private:
  // ======================== 3-1. 增 ========================
  void declare_param()
  {
    RCLCPP_INFO(this->get_logger(), "--------增--------");
    this->declare_parameter<std::string>("car_name", "turtle");
    this->declare_parameter<double>("width", 0.15);
    this->declare_parameter<double>("length", 0.40);
    // 临时参数（动态类型）：Jazzy 下只有动态类型才能 undeclare
    rcl_interfaces::msg::ParameterDescriptor desc;
    desc.dynamic_typing = true;
    this->declare_parameter("tmp_flag", rclcpp::ParameterValue(true), desc);
  }

  // ======================== 3-2. 查 ========================
  void get_param()
  {
    RCLCPP_INFO(this->get_logger(), "--------查--------");

    auto car = this->get_parameter("car_name");
    RCLCPP_INFO(this->get_logger(), "key = %s, value = %s",
      car.get_name().c_str(), car.as_string().c_str());

    double width = 0.0;
    double length = 0.0;
    this->get_parameter("width", width);
    this->get_parameter("length", length);
    RCLCPP_INFO(this->get_logger(), "width=%.3f, length=%.3f", width, length);

    // 批量查
    auto params = this->get_parameters({"car_name", "width", "length", "tmp_flag"});
    for (const auto & p : params) {
      RCLCPP_INFO(
        this->get_logger(),
        "[批量查] %s = %s (%s)",
        p.get_name().c_str(),
        p.value_to_string().c_str(),
        p.get_type_name().c_str());
    }

    RCLCPP_INFO(
      this->get_logger(),
      "has tmp_flag? %s",
      this->has_parameter("tmp_flag") ? "yes" : "no");
  }

  // ======================== 3-3. 改 ========================
  void update_param()
  {
    RCLCPP_INFO(this->get_logger(), "--------改--------");

    // 单个改
    this->set_parameter(rclcpp::Parameter("car_name", "turtle1"));

    // 批量改
    this->set_parameters({
      rclcpp::Parameter("width", 0.20),
      rclcpp::Parameter("length", 0.45),
    });

    // 未 declare 过的新名字（依赖 allow_undeclared_parameters）
    this->set_parameter(rclcpp::Parameter("height", 0.10));

    RCLCPP_INFO(
      this->get_logger(),
      "改后: car_name=%s, width=%.3f, length=%.3f, height=%.3f",
      this->get_parameter("car_name").as_string().c_str(),
      this->get_parameter("width").as_double(),
      this->get_parameter("length").as_double(),
      this->get_parameter("height").as_double());
  }

  // ======================== 3-4. 删 ========================
  void del_param()
  {
    RCLCPP_INFO(this->get_logger(), "--------删--------");
    this->undeclare_parameter("tmp_flag");
    this->undeclare_parameter("height");

    RCLCPP_INFO(
      this->get_logger(),
      "删后 has tmp_flag? %s | has height? %s | has car_name? %s",
      this->has_parameter("tmp_flag") ? "yes" : "no",
      this->has_parameter("height") ? "yes" : "no",
      this->has_parameter("car_name") ? "yes" : "no");
  }

  // ======================== 远端改参校验 ========================
  rcl_interfaces::msg::SetParametersResult on_set_parameters(
    const std::vector<rclcpp::Parameter> & parameters)
  {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;

    for (const auto & p : parameters) {
      RCLCPP_INFO(this->get_logger(), "[远端改] %s", p.get_name().c_str());

      if (p.get_name() == "car_name") {
        if (p.as_string().empty()) {
          result.successful = false;
          result.reason = "car_name 不能为空";
          return result;
        }
      } else if (p.get_name() == "width") {
        const double v = p.as_double();
        if (v < 0.1 || v > 1.0) {
          result.successful = false;
          result.reason = "width 必须在 0.1 ~ 1.0";
          return result;
        }
      } else if (p.get_name() == "length") {
        const double v = p.as_double();
        if (v < 0.1 || v > 2.0) {
          result.successful = false;
          result.reason = "length 必须在 0.1 ~ 2.0";
          return result;
        }
      }
    }
    return result;
  }

  void on_timer()
  {
    RCLCPP_INFO(
      this->get_logger(),
      "[定时] car_name=%s, width=%.3f, length=%.3f",
      this->get_parameter("car_name").as_string().c_str(),
      this->get_parameter("width").as_double(),
      this->get_parameter("length").as_double());
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr param_cb_handle_;
};

int main(int argc, char ** argv)
{
  // 2. 初始化 ROS2 客户端
  rclcpp::init(argc, argv);
  // 4. 调用 spin，并传入节点对象指针
  rclcpp::spin(std::make_shared<ParamServer>());
  // 5. 资源释放
  rclcpp::shutdown();
  return 0;
}
