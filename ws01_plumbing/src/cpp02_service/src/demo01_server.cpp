/*
  需求：编写服务端，接收两个整数，返回它们的和。

  服务接口 AddInts.srv：
    Request : int32 num1, int32 num2
    Response: int32 sum

  流程：
    1. 包含头文件
    2. 初始化 ROS2 客户端
    3. 定义节点类
       3-1. 创建服务端
       3-2. 在回调中解析请求、计算结果、填写响应
    4. 调用 spin，等待客户端请求
    5. 资源释放

  与话题的区别：
    - 话题是「持续广播」，一对多，发布端不等待回复
    - 服务是「一问一答」，一对一，客户端发请求后等待响应

  运行：
    ros2 run cpp02_service demo01_server
*/

#include "rclcpp/rclcpp.hpp"
#include "base_interfaces_demo/srv/add_ints.hpp"

using base_interfaces_demo::srv::AddInts;
using namespace std::placeholders;

class AddIntsServer : public rclcpp::Node
{
public:
  AddIntsServer()
  : Node("add_ints_server")
  {
    RCLCPP_INFO(this->get_logger(), "服务端节点已创建，等待客户端请求...");

    // 服务名必须和客户端一致："add_ints"
    server_ = this->create_service<AddInts>(
      "add_ints",
      std::bind(&AddIntsServer::handle_add, this, _1, _2));
  }

private:
  // request  —— 客户端发来的请求（num1, num2）
  // response —— 要填好后返回给客户端的响应（sum）
  void handle_add(
    const AddInts::Request::SharedPtr request,
    AddInts::Response::SharedPtr response)
  {
    RCLCPP_INFO(
      this->get_logger(),
      "收到请求: num1=%d, num2=%d",
      request->num1, request->num2);

    response->sum = request->num1 + request->num2;

    RCLCPP_INFO(this->get_logger(), "返回结果: sum=%d", response->sum);
  }

  rclcpp::Service<AddInts>::SharedPtr server_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<AddIntsServer>();
  // 没有 spin，服务回调不会被触发
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
