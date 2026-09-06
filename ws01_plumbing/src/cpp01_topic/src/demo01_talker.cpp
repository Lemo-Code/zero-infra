/*
  需求：编写话题发布端（Talker），以固定频率发布学生信息。

  消息接口 Student.msg：
    string name
    int32 age
    float64 height

  流程：
    1. 包含头文件
    2. 初始化 ROS2 客户端
    3. 定义节点类
       3-1. 创建发布者
       3-2. 创建定时器，在回调里组织并发布消息
    4. 调用 spin，持续触发定时器回调
    5. 资源释放

  运行：
    ros2 run cpp01_topic demo01_talker
*/

#include "rclcpp/rclcpp.hpp"
#include "base_interfaces_demo/msg/student.hpp"

using base_interfaces_demo::msg::Student;
using namespace std::chrono_literals;

class Talker : public rclcpp::Node
{
public:
  Talker()
  : Node("talker_node"), count_(0)
  {
    RCLCPP_INFO(this->get_logger(), "发布端节点已创建");

    // 话题名必须和订阅端一致："chatter_stu"
    // 队列深度 10：发布过快、订阅端处理不过来时，最多缓存 10 条
    publisher_ = this->create_publisher<Student>("chatter_stu", 10);

    // 每 500ms 触发一次 on_timer，在里面发布一条消息
    timer_ = this->create_wall_timer(
      500ms, std::bind(&Talker::on_timer, this));
  }

private:
  void on_timer()
  {
    auto msg = Student();
    msg.name = "张三";
    msg.age = static_cast<int32_t>(18 + (count_ % 10));
    msg.height = 1.70 + 0.01 * static_cast<double>(count_ % 10);

    publisher_->publish(msg);
    ++count_;

    RCLCPP_INFO(
      this->get_logger(),
      "发布学生信息: name=%s, age=%d, height=%.2f",
      msg.name.c_str(), msg.age, msg.height);
  }

  rclcpp::Publisher<Student>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  size_t count_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<Talker>();
  // 没有 spin，定时器回调不会执行，也就发不出消息
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
