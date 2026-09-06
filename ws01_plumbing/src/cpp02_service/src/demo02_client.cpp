/*
  需求：编写客户端，提交两个整数，接收服务端返回的和。

  服务接口 AddInts.srv：
    Request : int32 num1, int32 num2
    Response: int32 sum

  流程：
    1. 包含头文件
    2. 初始化 ROS2 客户端
    3. 定义节点类
       3-1. 创建客户端
       3-2. 等待服务端上线
       3-3. 组织请求并异步发送（async_send_request）
       3-4. 在回调中处理响应
    4. 调用 spin，等待响应回调
    5. 资源释放

  运行示例：
    ros2 run cpp02_service demo01_server
    ros2 run cpp02_service demo02_client 3 5
    （参数 3 和 5 表示 num1、num2；不传则默认 1 和 2）
*/

#include "rclcpp/rclcpp.hpp"
#include "base_interfaces_demo/srv/add_ints.hpp"

#include <chrono>
#include <cstdlib>

using base_interfaces_demo::srv::AddInts;
using namespace std::chrono_literals;
using namespace std::placeholders;

class AddIntsClient : public rclcpp::Node
{
public:
  AddIntsClient(int32_t num1, int32_t num2)
  : Node("add_ints_client"), num1_(num1), num2_(num2)
  {
    RCLCPP_INFO(
      this->get_logger(),
      "客户端节点已创建，准备请求: num1=%d, num2=%d",
      num1_, num2_);

    // 服务名必须和服务端一致："add_ints"
    client_ = this->create_client<AddInts>("add_ints");
  }

  void send_request()
  {
    // ---------- ① 等待服务端上线 ----------
    RCLCPP_INFO(this->get_logger(), "正在等待服务端上线...");
    while (!client_->wait_for_service(1s)) {
      if (!rclcpp::ok()) {
        RCLCPP_ERROR(this->get_logger(), "等待被中断");
        return;
      }
      RCLCPP_WARN(this->get_logger(), "服务端未就绪，继续等待...");
    }
    RCLCPP_INFO(this->get_logger(), "服务端已上线，发送请求...");

    // ---------- ② 填写请求 ----------
    auto request = std::make_shared<AddInts::Request>();
    request->num1 = num1_;
    request->num2 = num2_;

    // ---------- ③ 异步发送 ----------
    // async_send_request 立刻返回，真正的结果在回调里拿
    client_->async_send_request(
      request,
      std::bind(&AddIntsClient::response_callback, this, _1));
  }

private:
  // SharedFuture 里装着最终的 Response
  void response_callback(rclcpp::Client<AddInts>::SharedFuture future)
  {
    try {
      auto response = future.get();
      RCLCPP_INFO(
        this->get_logger(),
        "收到响应: %d + %d = %d",
        num1_, num2_, response->sum);
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "获取响应失败: %s", e.what());
    }

    // 演示程序：拿到一次结果后退出
    rclcpp::shutdown();
  }

  int32_t num1_;
  int32_t num2_;
  rclcpp::Client<AddInts>::SharedPtr client_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  int32_t num1 = 1;
  int32_t num2 = 2;
  if (argc >= 3) {
    num1 = static_cast<int32_t>(std::atoi(argv[1]));
    num2 = static_cast<int32_t>(std::atoi(argv[2]));
  }

  auto node = std::make_shared<AddIntsClient>(num1, num2);
  node->send_request();

  // spin：让响应回调有机会执行
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
