/*
  需求：编写动作服务端，接收客户端提交的整型数据 num，
        计算 1 到 num 的累加和，连续反馈进度，最终返回结果。

  Action 接口 Progress.action 回顾：
    Goal（目标）    : int32 num      —— 客户端要累加到几
    Result（结果）  : int32 sum      —— 最终累加和
    Feedback（反馈）: float64 progress —— 进度 0.0 ~ 1.0

  流程：
    1. 包含头文件
    2. 初始化 ROS2 客户端
    3. 定义节点类
       3-1. 创建动作服务端
       3-2. 处理请求（解析提交的数据、判断合法性）
       3-3. 处理取消请求
       3-4. 请求被接受后，在新线程中执行任务
       3-5. 生成连续反馈并返回最终响应
    4. 调用 spin 函数，并传入节点对象指针
    5. 资源释放
*/

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "base_interfaces_demo/action/progress.hpp"

#include <memory>
#include <thread>
#include <chrono>

// 动作类型别名，后面写起来更短
using base_interfaces_demo::action::Progress;
// GoalHandle：服务端用来管理「某一个具体目标」的句柄
// 通过它可以：发反馈、检查是否取消、返回最终结果
using GoalHandleProgress = rclcpp_action::ServerGoalHandle<Progress>;

// 使用占位符 _1, _2，方便 std::bind 绑定回调参数
using namespace std::placeholders;

class ProgressActionServer : public rclcpp::Node
{
public:
  ProgressActionServer()
  : Node("progress_action_server")
  {
    RCLCPP_INFO(this->get_logger(), "动作服务端节点已创建，等待客户端请求...");

    /*
      create_server 需要绑定三个回调函数：

      ① handle_goal
         —— 客户端刚发来目标时调用
         —— 在这里决定：接受 / 拒绝

      ② handle_cancel
         —— 客户端请求取消时调用
         —— 在这里决定：允许取消 / 拒绝取消

      ③ handle_accepted
         —— 目标已被接受后调用
         —— 通常在这里启动一个新线程去真正执行任务
           （避免阻塞主线程，主线程要继续 spin）

      动作名 "get_sum" 必须和服务端、客户端保持一致！
    */
    action_server_ = rclcpp_action::create_server<Progress>(
      this,                                          // 所属节点
      "get_sum",                                     // 动作名称（话题前缀）
      std::bind(&ProgressActionServer::handle_goal, this, _1, _2),
      std::bind(&ProgressActionServer::handle_cancel, this, _1),
      std::bind(&ProgressActionServer::handle_accepted, this, _1));
  }

private:
  // ======================== 回调 ①：处理新目标 ========================
  // 参数：
  //   uuid —— 这个目标的唯一 ID（一般用不到，可以忽略）
  //   goal —— 客户端提交的目标数据（这里面有 num）
  // 返回：
  //   ACCEPT_AND_EXECUTE —— 接受并立刻开始执行
  //   ACCEPT_AND_DEFER   —— 接受但稍后执行
  //   REJECT             —— 拒绝
  rclcpp_action::GoalResponse handle_goal(
    const rclcpp_action::GoalUUID & /*uuid*/,
    std::shared_ptr<const Progress::Goal> goal)
  {
    RCLCPP_INFO(this->get_logger(), "收到目标请求，num = %d", goal->num);

    // 合法性检查：num 太小没有意义，拒绝请求
    if (goal->num <= 1) {
      RCLCPP_WARN(this->get_logger(), "num 必须大于 1，拒绝该请求");
      return rclcpp_action::GoalResponse::REJECT;
    }

    // 接受请求，并马上执行
    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  // ======================== 回调 ②：处理取消请求 ========================
  // 客户端可以在任务执行过程中请求取消
  // 返回 ACCEPT 表示允许取消；REJECT 表示不允许
  rclcpp_action::CancelResponse handle_cancel(
    const std::shared_ptr<GoalHandleProgress> /*goal_handle*/)
  {
    RCLCPP_INFO(this->get_logger(), "收到取消请求，允许取消");
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  // ======================== 回调 ③：目标被接受后 ========================
  // 注意：真正耗时的计算不要写在这里！
  // 否则会卡住主线程，导致无法处理取消、反馈等。
  // 正确做法：开一个新线程，把执行逻辑放到 execute() 里。
  void handle_accepted(const std::shared_ptr<GoalHandleProgress> goal_handle)
  {
    RCLCPP_INFO(this->get_logger(), "目标已接受，启动执行线程...");

    // detach：让线程自己跑完就结束，不阻塞当前函数
    std::thread{
      std::bind(&ProgressActionServer::execute, this, goal_handle)
    }.detach();
  }

  // ======================== 真正执行任务的函数 ========================
  // 在这里：循环累加、发布反馈、检查取消、返回最终结果
  void execute(const std::shared_ptr<GoalHandleProgress> goal_handle)
  {
    RCLCPP_INFO(this->get_logger(), "开始执行任务...");

    // 取出客户端提交的目标
    const auto goal = goal_handle->get_goal();
    const int32_t num = goal->num;

    // 准备反馈消息和结果消息
    auto feedback = std::make_shared<Progress::Feedback>();
    auto result = std::make_shared<Progress::Result>();

    int32_t sum = 0;  // 当前累加和

    // 从 1 累加到 num，每加一个数就反馈一次进度
    for (int32_t i = 1; i <= num; ++i) {
      // ---------- 检查客户端是否请求了取消 ----------
      if (goal_handle->is_canceling()) {
        // 即使取消，也可以把当前已经算出来的部分结果返回
        result->sum = sum;
        goal_handle->canceled(result);
        RCLCPP_INFO(this->get_logger(), "任务已取消，当前部分和 = %d", sum);
        return;
      }

      // ---------- 做一点点「工作」：累加 ----------
      sum += i;

      // ---------- 发布连续反馈 ----------
      // publish_feedback 每调用一次，客户端的 feedback_callback 就会被触发一次。
      // 这就是客户端能「实时」看到进度的原因；不是客户端自己轮询，而是服务端主动推送。
      // progress 是 0.0 ~ 1.0 的进度比例
      feedback->progress = static_cast<double>(i) / static_cast<double>(num);
      goal_handle->publish_feedback(feedback);

      RCLCPP_INFO(
        this->get_logger(),
        "反馈进度: %.2f%%  (当前 i=%d, sum=%d)",
        feedback->progress * 100.0, i, sum);

      // 故意 sleep 一下，让进度变化肉眼可见（学习演示用）
      // 真实项目中，这里通常是真正耗时的计算/运动控制等
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ---------- 任务正常完成，返回最终结果 ----------
    // 注意：执行过程中节点可能已经关闭，所以先判断是否还在运行
    if (rclcpp::ok()) {
      result->sum = sum;
      goal_handle->succeed(result);  // 标记成功，并把结果发给客户端
      RCLCPP_INFO(this->get_logger(), "任务完成！1 到 %d 的累加和 = %d", num, sum);
    }
  }

  // 动作服务端智能指针（成员变量，保证节点存活期间服务端一直存在）
  rclcpp_action::Server<Progress>::SharedPtr action_server_;
};

int main(int argc, char ** argv)
{
  // 1. 初始化 ROS2
  rclcpp::init(argc, argv);

  // 2. 创建服务端节点
  auto server = std::make_shared<ProgressActionServer>();

  // 3. 进入循环，持续处理回调（目标、取消、反馈相关事件）
  //    没有 spin，回调函数永远不会被触发！
  rclcpp::spin(server);

  // 4. 退出前释放资源
  rclcpp::shutdown();
  return 0;
}
