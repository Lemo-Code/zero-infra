/*
  需求：参数客户端 —— 对远端 param_server_node 做「查、改」，
        并演示 list；删除一般在服务端 undeclare（客户端侧较少直接删）。

  对应课程：2.5.3 参数服务 (C++)

  流程：
    1. 创建 SyncParametersClient(this, "param_server_node")
    2. wait_for_service
    3. 查：list_parameters / get_parameters
    4. 改：set_parameters（合法 + 非法拒绝）
    5. 再查确认；演示结束退出

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
    param_client_ = std::make_shared<rclcpp::SyncParametersClient>(
      this, "param_server_node");
  }

  bool run_demo()
  {
    RCLCPP_INFO(this->get_logger(), "等待参数服务端上线...");
    if (!param_client_->wait_for_service(10s)) {
      RCLCPP_ERROR(this->get_logger(), "等待超时，请先启动 demo01_param_server");
      return false;
    }
    RCLCPP_INFO(this->get_logger(), "服务端已就绪");

    // ---------- 查：列出 ----------
    try {
      auto listed = param_client_->list_parameters({}, 0);
      RCLCPP_INFO(this->get_logger(), "[查/list] 远端共 %zu 个参数:", listed.names.size());
      for (const auto & name : listed.names) {
        RCLCPP_INFO(this->get_logger(), "  - %s", name.c_str());
      }
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "list_parameters 失败: %s", e.what());
      return false;
    }

    // ---------- 查：取值 ----------
    if (!print_params("[查/get] ")) {
      return false;
    }

    // ---------- 改：合法 ----------
    try {
      std::vector<rclcpp::Parameter> to_set = {
        rclcpp::Parameter("car_name", std::string("turtle2")),
        rclcpp::Parameter("width", 0.30),
        rclcpp::Parameter("length", 0.50),
      };
      auto results = param_client_->set_parameters(to_set);
      for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].successful) {
          RCLCPP_INFO(this->get_logger(), "[改] 成功 %s", to_set[i].get_name().c_str());
        } else {
          RCLCPP_WARN(
            this->get_logger(), "[改] 失败 %s: %s",
            to_set[i].get_name().c_str(), results[i].reason.c_str());
        }
      }
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "set_parameters 失败: %s", e.what());
      return false;
    }

    if (!print_params("[查/改后] ")) {
      return false;
    }

    // ---------- 改：非法（观察服务端拒绝）----------
    try {
      auto results = param_client_->set_parameters(
        {rclcpp::Parameter("width", 9.9)});
      if (!results.empty() && !results[0].successful) {
        RCLCPP_WARN(
          this->get_logger(),
          "[改/拒绝] width=9.9, reason=%s",
          results[0].reason.c_str());
      }
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "非法 set 异常: %s", e.what());
    }

    // 说明：参数的「删」通常用服务端 undeclare_parameter；
    // 客户端也可尝试 delete_parameters（若发行版 API 支持），本课以服务端删除演示为准。
    RCLCPP_INFO(this->get_logger(), "[删] 见 demo00/demo01 服务端 undeclare_parameter");

    return true;
  }

private:
  bool print_params(const char * prefix)
  {
    try {
      auto params = param_client_->get_parameters({"car_name", "width", "length"});
      for (const auto & p : params) {
        RCLCPP_INFO(
          this->get_logger(), "%s%s = %s",
          prefix, p.get_name().c_str(), p.value_to_string().c_str());
      }
      return true;
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "get_parameters 失败: %s", e.what());
      return false;
    }
  }

  std::shared_ptr<rclcpp::SyncParametersClient> param_client_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ParamClient>();
  const bool ok = node->run_demo();
  rclcpp::shutdown();
  return ok ? 0 : 1;
}
