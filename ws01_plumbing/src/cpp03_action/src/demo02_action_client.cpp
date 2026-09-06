/*
  需求：编写动作客户端，向服务端提交整型目标 num，
        接收连续进度反馈，并打印最终累加结果。

  Action 接口 Progress.action 回顾：
    Goal（目标）    : int32 num      —— 要累加到几
    Result（结果）  : int32 sum      —— 最终累加和
    Feedback（反馈）: float64 progress —— 进度 0.0 ~ 1.0

  流程：
    1. 包含头文件
    2. 初始化 ROS2 客户端
    3. 定义节点类
       3-1. 创建动作客户端
       3-2. 发送目标请求（async_send_goal，非阻塞）
       3-3. 处理目标响应（是否被服务端接受，通常 1 次）
       3-4. 处理连续反馈（服务端每次 publish_feedback 触发，可多次 = 实时进度）
       3-5. 处理最终结果（成功/取消/中止，通常 1 次）
    4. 调用 spin 函数，并传入节点对象指针（没有 spin，上述回调不会执行）
    5. 资源释放

  回调时序（一次完整任务）：
    async_send_goal
      → goal_response_callback（接受/拒绝）
      → feedback_callback × N（进度 0%→100%）
      → result_callback（最终 sum）

  运行示例：
    ros2 run cpp03_action demo02_action_client 100
    （参数 100 表示计算 1~100 的累加和；不传参数则默认 10）
*/

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "base_interfaces_demo/action/progress.hpp"

#include <memory>
#include <chrono>

using base_interfaces_demo::action::Progress;
// 客户端侧的 GoalHandle（和服务器侧的 ServerGoalHandle 不同！）
using GoalHandleProgress = rclcpp_action::ClientGoalHandle<Progress>;
using namespace std::placeholders;

class ProgressActionClient : public rclcpp::Node
{
public:
  // 构造函数：传入要累加到的数字 num
  explicit ProgressActionClient(int32_t num)
  : Node("progress_action_client"), num_(num)
  {
    RCLCPP_INFO(this->get_logger(), "动作客户端节点已创建，目标 num = %d", num_);

    // 创建动作客户端，动作名必须和服务端一致："get_sum"
    action_client_ = rclcpp_action::create_client<Progress>(this, "get_sum");
  }

  // 对外提供的「发送目标」接口（在 main 里调用）
  void send_goal()
  {
    // ---------- ① 等待服务端上线 ----------
    // 服务端没启动时，客户端会一直等；这里最多等 10 秒
    RCLCPP_INFO(this->get_logger(), "正在等待动作服务端上线...");
    if (!action_client_->wait_for_action_server(std::chrono::seconds(10))) {
      RCLCPP_ERROR(this->get_logger(), "等待服务端超时！请先启动 demo01_action_server");
      rclcpp::shutdown();
      return;
    }
    RCLCPP_INFO(this->get_logger(), "服务端已上线，准备发送目标...");

    // ---------- ② 填写目标数据 ----------
    auto goal_msg = Progress::Goal();
    goal_msg.num = num_;

    // ---------- ③ 配置三个回调 ----------
    // SendGoalOptions：发目标时一次性「注册」回调函数指针，本身不会循环更新。
    // 真正的「实时感」来自服务端反复 publish_feedback，从而反复触发 feedback_callback。
    //
    // 三个回调的触发次数不同：
    //   goal_response_callback —— 通常只调 1 次（接受 / 拒绝）
    //   feedback_callback      —— 服务端每发一次反馈就调 1 次（可多次，进度实时刷新）
    //   result_callback        —— 任务结束时调 1 次（成功 / 取消 / 中止）
    //
    // 注意：回调不会自己跑；main 里必须 rclcpp::spin()，否则永远收不到。
    auto send_goal_options = rclcpp_action::Client<Progress>::SendGoalOptions();

    // ① 目标响应：服务端 handle_goal 返回 ACCEPT/REJECT 后触发（一次性）
    //    参数是 ClientGoalHandle；为空表示被拒绝
    send_goal_options.goal_response_callback =
      std::bind(&ProgressActionClient::goal_response_callback, this, _1);

    // ② 连续反馈：对应服务端 goal_handle->publish_feedback(...)
    //    本例服务端循环累加时约每 100ms 发一次，所以这里会持续被调用
    //    参数：goal_handle + Feedback（含 progress 0.0~1.0）
    send_goal_options.feedback_callback =
      std::bind(&ProgressActionClient::feedback_callback, this, _1, _2);

    // ③ 最终结果：服务端 succeed / canceled / abort 后触发（一次性）
    //    参数 WrappedResult：含 result.code 与 result.result->sum
    send_goal_options.result_callback =
      std::bind(&ProgressActionClient::result_callback, this, _1);

    // ---------- ④ 异步发送目标 ----------
    // async_send_goal：立刻返回，不阻塞等待算完。
    // 后续进度/结果全部通过上面注册的回调异步到达（前提是节点在 spin）。
    RCLCPP_INFO(this->get_logger(), "发送目标: num = %d", goal_msg.num);
    action_client_->async_send_goal(goal_msg, send_goal_options);
  }

private:
  // ======================== 回调：目标是否被接受 ========================
  // 时机：服务端对本次 Goal 做出 ACCEPT / REJECT 之后（通常只触发一次）。
  // 参数 goal_handle：
  //   - 为空 (nullptr) → 服务端拒绝（例如服务端要求 num > 1）
  //   - 非空           → 已接受；可用该句柄后续 cancel / 查状态
  // 注意：此时任务可能才刚开始，还没有最终 sum。
  void goal_response_callback(GoalHandleProgress::SharedPtr goal_handle)
  {
    if (!goal_handle) {
      RCLCPP_ERROR(this->get_logger(), "目标被服务端拒绝！");
      rclcpp::shutdown();
      return;
    }
    RCLCPP_INFO(this->get_logger(), "目标已被服务端接受，等待执行结果...");
  }

  // ======================== 回调：连续反馈（实时进度） ========================
  // 时机：服务端每调用一次 publish_feedback，本函数就执行一次。
  // 这是 Action 相对「普通 Service」的核心差别：执行过程中可持续推送中间状态。
  // 参数：
  //   goal_handle —— 当前目标句柄（演示里未用，可忽略）
  //   feedback    —— Feedback 消息；本例只有 progress（0.0~1.0）
  void feedback_callback(
    GoalHandleProgress::SharedPtr /*goal_handle*/,
    const std::shared_ptr<const Progress::Feedback> feedback)
  {
    // progress 是比例，乘 100 打印成百分比更直观
    RCLCPP_INFO(
      this->get_logger(),
      "收到反馈，当前进度: %.1f%%",
      feedback->progress * 100.0);
  }

  // ======================== 回调：最终结果 ========================
  // 时机：服务端调用 succeed / canceled / abort 之后（任务生命周期结束，通常一次）。
  // WrappedResult：
  //   result.code   —— 结束原因（SUCCEEDED / ABORTED / CANCELED ...）
  //   result.result —— Result 消息指针（本例含最终或取消时的部分 sum）
  void result_callback(const GoalHandleProgress::WrappedResult & result)
  {
    switch (result.code) {
      case rclcpp_action::ResultCode::SUCCEEDED:
        RCLCPP_INFO(this->get_logger(), "任务成功！累加和 sum = %d", result.result->sum);
        break;
      case rclcpp_action::ResultCode::ABORTED:
        RCLCPP_ERROR(this->get_logger(), "任务被中止");
        break;
      case rclcpp_action::ResultCode::CANCELED:
        // 本例服务端取消时仍会带回当前已算出的部分和
        RCLCPP_WARN(this->get_logger(), "任务被取消，部分结果 sum = %d", result.result->sum);
        break;
      default:
        RCLCPP_ERROR(this->get_logger(), "未知结果码");
        break;
    }

    // 演示程序：拿到终态后退出；真实项目通常继续跑节点、可再发新 Goal
    rclcpp::shutdown();
  }

  int32_t num_;  // 要发送给服务端的目标数字
  rclcpp_action::Client<Progress>::SharedPtr action_client_;
};

int main(int argc, char ** argv)
{
  // 1. 初始化 ROS2
  rclcpp::init(argc, argv);

  // 2. 解析命令行参数：ros2 run ... demo02_action_client 100
  //    argv[0] 是程序名，argv[1] 才是用户传的数字
  int32_t num = 10;  // 默认值
  if (argc >= 2) {
    num = static_cast<int32_t>(std::atoi(argv[1]));
  }

  // 3. 创建客户端节点，并发送目标
  auto client = std::make_shared<ProgressActionClient>(num);
  client->send_goal();

  // 4. spin：让回调有机会被执行
  //    SendGoalOptions 里注册的三个回调都依赖 spin 调度；
  //    没有 spin，goal_response / feedback / result 回调都不会跑。
  rclcpp::spin(client);

  // 5. 释放资源（result_callback 里也可能已经 shutdown 过，重复调用是安全的）
  rclcpp::shutdown();
  return 0;
}
