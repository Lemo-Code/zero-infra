/*
  需求：编写话题订阅端（Listener），接收并打印学生信息。

  消息接口 Student.msg：
    string name
    int32 age
    float64 height

  流程：
    1. 包含头文件
    2. 初始化 ROS2 客户端
    3. 定义节点类
       3-1. 创建订阅者
       3-2. 在回调中解析并打印收到的消息
    4. 调用 spin，持续等待并处理回调
    5. 资源释放

  运行（先开 talker，再开 listener）：
    ros2 run cpp01_topic demo01_talker
    ros2 run cpp01_topic demo02_listener
*/

#include "rclcpp/rclcpp.hpp"
#include "base_interfaces_demo/msg/student.hpp"

using base_interfaces_demo::msg::Student;
using namespace std::placeholders;

class Listener : public rclcpp::Node
{
public:
  Listener()
  : Node("listener_node")
  {
    RCLCPP_INFO(this->get_logger(), "订阅端节点已创建，等待消息...");

    // 话题名必须和发布端一致："chatter_stu"
    subscription_ = this->create_subscription<Student>(
      "chatter_stu",
      10,
      std::bind(&Listener::topic_callback, this, _1));
  }

private:
  // 每收到一条消息，就会触发一次本回调
  void topic_callback(const Student::SharedPtr msg)
  {
    RCLCPP_INFO(
      this->get_logger(),
      "收到学生信息: name=%s, age=%d, height=%.2f",
      msg->name.c_str(), msg->age, msg->height);
  }

  rclcpp::Subscription<Student>::SharedPtr subscription_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<Listener>();
  // 没有 spin，订阅回调永远不会被调用
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
