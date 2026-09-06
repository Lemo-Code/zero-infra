/*
  需求：编写「参数客户端」节点，连接已启动的参数服务端，
        列出 / 读取 / 修改对方节点上的参数。

  对应课程：2.5.3 参数服务 (C++)

  目标节点名必须一致：param_server_node

  流程：
    1. 创建 SyncParametersClient(本节点, "param_server_node")
    2. wait_for_service 等待对端参数服务就绪
    3. list_parameters / get_parameters / set_parameters
    4. 演示结束 shutdown 退出（本 demo 不常驻 spin）

  运行（先开服务端）：
    ros2 run cpp04_param demo01_param_server
    ros2 run cpp04_param demo02_param_client
*/

#include "rclcpp/rclcpp.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

using namespace std::chrono_literals;

class ParamClient : public rclcpp::Node
{
public:
  ParamClient()
  : Node("param_client_node")
  {
    RCLCPP_INFO(this->get_logger(), "参数客户端节点已创建");

    // 第二个参数：要操作的「远端节点名」
    param_client_ = std::make_shared<rclcpp::SyncParametersClient>(
      this, "param_server_node");
  }

  // 在 main 里调用：完整演示一遍后返回
  bool run_demo()
  {
    RCLCPP_INFO(this->get_logger(), "等待参数服务端上线...");
    if (!param_client_->wait_for_service(10s)) {
      RCLCPP_ERROR(this->get_logger(), "等待超时，请先启动 demo01_param_server");
      return false;
    }
    RCLCPP_INFO(this->get_logger(), "服务端已就绪");

    // ---------- ① 列出参数 ----------
    try {
      auto listed = param_client_->list_parameters({}, 0);
      RCLCPP_INFO(this->get_logger(), "远端共有 %zu 个参数:", listed.names.size());
      for (const auto & name : listed.names) {
        RCLCPP_INFO(this->get_logger(), "  - %s", name.c_str());
      }
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "list_parameters 失败: %s", e.what());
      return false;
    }

    // ---------- ② 读取参数 ----------
    try {
      auto params = param_client_->get_parameters(
        {"car_name", "width", "length"});
      for (const auto & p : params) {
        RCLCPP_INFO(
          this->get_logger(),
          "读取: %s = %s",
          p.get_name().c_str(),
          p.value_to_string().c_str());
      }
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "get_parameters 失败: %s", e.what());
      return false;
    }

    // ---------- ③ 修改参数（会触发服务端 on_set_parameters）----------
    try {
      std::vector<rclcpp::Parameter> to_set = {
        rclcpp::Parameter("car_name", std::string("turtle2")),
        rclcpp::Parameter("width", 0.30),
        rclcpp::Parameter("length", 0.50),
      };
      auto results = param_client_->set_parameters(to_set);
      for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].successful) {
          RCLCPP_INFO(
            this->get_logger(),
            "设置成功: %s", to_set[i].get_name().c_str());
        } else {
          RCLCPP_WARN(
            this->get_logger(),
            "设置失败: %s, reason=%s",
            to_set[i].get_name().c_str(),
            results[i].reason.c_str());
        }
      }
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "set_parameters 失败: %s", e.what());
      return false;
    }

    // ---------- ④ 再读一遍确认 ----------
    try {
      auto params = param_client_->get_parameters(
        {"car_name", "width", "length"});
      RCLCPP_INFO(this->get_logger(), "修改后再次读取:");
      for (const auto & p : params) {
        RCLCPP_INFO(
          this->get_logger(),
          "  %s = %s",
          p.get_name().c_str(),
          p.value_to_string().c_str());
      }
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "二次 get 失败: %s", e.what());
      return false;
    }

    // ---------- ⑤ 故意设一个非法值，观察服务端拒绝 ----------
    try {
      auto results = param_client_->set_parameters(
        {rclcpp::Parameter("width", 9.9)});
      if (!results.empty() && !results[0].successful) {
        RCLCPP_WARN(
          this->get_logger(),
          "预期中的拒绝: width=9.9, reason=%s",
          results[0].reason.c_str());
      }
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "非法 set 异常: %s", e.what());
    }

    return true;
  }

private:
  std::shared_ptr<rclcpp::SyncParametersClient> param_client_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ParamClient>();

  // SyncParametersClient 内部会临时 spin 等待服务结果；
  // 本演示在 run_demo 里做完就退出，不必长期 rclcpp::spin。
  const bool ok = node->run_demo();

  rclcpp::shutdown();
  return ok ? 0 : 1;
}
