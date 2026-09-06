/*
  ============================================================
  demo02_param_client —— 参数客户端（远程查 / 改）
  ============================================================
  对应课程：2.5.3 参数服务 (C++) / 第 04 节客户端

  学习目标：
    1. 用 SyncParametersClient 连接远端 param_server_node
    2. 远程：list（查列表）/ get（查值）/ set（改）
    3. 观察合法 set 成功、非法 set 被服务端拒绝
    4. 了解「删」通常在服务端 undeclare（见 demo00 / demo01）

  流程：
    1. 包含头文件
    2. 初始化 ROS2
    3. 创建客户端节点 + SyncParametersClient
    4. wait_for_service → list → get → set → 再 get → 非法 set
    5. shutdown 退出

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
    RCLCPP_INFO(this->get_logger(), "参数客户端创建");
    // 第二个参数 = 要操作的远端节点名（必须和服务端一致）
    param_client_ = std::make_shared<rclcpp::SyncParametersClient>(
      this, "param_server_node");
  }

  bool run_demo()
  {
    RCLCPP_INFO(this->get_logger(), "等待参数服务端...");
    if (!param_client_->wait_for_service(10s)) {
      RCLCPP_ERROR(this->get_logger(), "超时！请先启动: ros2 run cpp04_param demo01_param_server");
      return false;
    }
    RCLCPP_INFO(this->get_logger(), "服务端已就绪");

    // ---------- 查：列出 ----------
    if (!do_list()) {
      return false;
    }

    // ---------- 查：取值 ----------
    if (!do_get("改之前")) {
      return false;
    }

    // ---------- 改：合法 ----------
    if (!do_set_ok()) {
      return false;
    }

    // ---------- 查：确认 ----------
    if (!do_get("改之后")) {
      return false;
    }

    // ---------- 改：非法（应被拒绝）----------
    do_set_bad();

    // ---------- 删：说明 ----------
    RCLCPP_INFO(
      this->get_logger(),
      "【删】客户端一般不直接删远端参数；请看服务端 undeclare_parameter（demo00/demo01）");

    return true;
  }

private:
  bool do_list()
  {
    RCLCPP_INFO(this->get_logger(), "======== 查 list ========");
    try {
      auto listed = param_client_->list_parameters({}, 0);
      RCLCPP_INFO(this->get_logger(), "远端参数共 %zu 个:", listed.names.size());
      for (const auto & name : listed.names) {
        RCLCPP_INFO(this->get_logger(), "  - %s", name.c_str());
      }
      return true;
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "list_parameters 失败: %s", e.what());
      return false;
    }
  }

  bool do_get(const char * tag)
  {
    RCLCPP_INFO(this->get_logger(), "======== 查 get（%s）========", tag);
    try {
      auto params = param_client_->get_parameters({"car_name", "width", "length"});
      for (const auto & p : params) {
        RCLCPP_INFO(
          this->get_logger(),
          "  %s = %s",
          p.get_name().c_str(),
          p.value_to_string().c_str());
      }
      return true;
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "get_parameters 失败: %s", e.what());
      return false;
    }
  }

  bool do_set_ok()
  {
    RCLCPP_INFO(this->get_logger(), "======== 改 set（合法）========");
    try {
      std::vector<rclcpp::Parameter> to_set = {
        rclcpp::Parameter("car_name", std::string("turtle2")),
        rclcpp::Parameter("width", 0.30),
        rclcpp::Parameter("length", 0.50),
      };
      auto results = param_client_->set_parameters(to_set);
      for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].successful) {
          RCLCPP_INFO(this->get_logger(), "  OK  %s", to_set[i].get_name().c_str());
        } else {
          RCLCPP_WARN(
            this->get_logger(), "  FAIL %s: %s",
            to_set[i].get_name().c_str(), results[i].reason.c_str());
        }
      }
      return true;
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "set_parameters 失败: %s", e.what());
      return false;
    }
  }

  void do_set_bad()
  {
    RCLCPP_INFO(this->get_logger(), "======== 改 set（非法 width=9.9，应拒绝）========");
    try {
      auto results = param_client_->set_parameters(
        {rclcpp::Parameter("width", 9.9)});
      if (!results.empty()) {
        if (!results[0].successful) {
          RCLCPP_WARN(
            this->get_logger(),
            "  预期拒绝: %s", results[0].reason.c_str());
        } else {
          RCLCPP_ERROR(this->get_logger(), "  意外：非法值居然成功了");
        }
      }
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "非法 set 异常: %s", e.what());
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
