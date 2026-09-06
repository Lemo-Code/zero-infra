# Action 模块：API 与相关内容详解

> 配套文档：`Action调用过程详解.md`（讲完整调用与取消过程）  
> 本文件：讲 **本模块用到的全部 API、类型、依赖包、构建与命令行**

对应包：`cpp03_action`  
接口包：`base_interfaces_demo`（`action/Progress.action`）  
源码：`demo01_action_server.cpp` / `demo02_action_client.cpp`  
额外库：`rclcpp_action`（Topic/Service 不需要）

---

## 1. 模块与依赖关系

```text
cpp03_action
├── depend: rclcpp
├── depend: rclcpp_action          ← Action 专用 API
├── depend: base_interfaces_demo   ← Progress.action
└── 间接：action_msgs 等（接口包已声明）
```

`package.xml`：

| 依赖 | 作用 |
|------|------|
| `rclcpp` | Node / spin / 日志 / ok |
| `rclcpp_action` | `create_server` / `create_client` / GoalHandle 等 |
| `base_interfaces_demo` | `Progress` 动作接口 |

可执行文件：

| 目标 | 源文件 | 角色 |
|------|--------|------|
| `demo01_action_server` | `demo01_action_server.cpp` | 动作服务端 |
| `demo02_action_client` | `demo02_action_client.cpp` | 动作客户端 |

头文件：

```cpp
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "base_interfaces_demo/action/progress.hpp"
```

标准库本模块额外用到：

| 头文件 | 用途 |
|--------|------|
| `<memory>` | `shared_ptr` / `make_shared` |
| `<thread>` | 服务端 `std::thread{...}.detach()` |
| `<chrono>` | `sleep_for` / `seconds(10)` |
| `<cstdlib>` | 客户端 `atoi`（解析 num） |

---

## 2. 动作接口（相关模块：base_interfaces_demo）

### 2.1 `Progress.action`

```text
int32 num          # Goal
---
int32 sum          # Result
---
float64 progress   # Feedback
```

| 段 | 字段 | 谁填 | 含义 |
|----|------|------|------|
| Goal | `num` | 客户端 | 累加上限 |
| Result | `sum` | 服务端 | 最终和或取消时部分和 |
| Feedback | `progress` | 服务端 | `0.0~1.0` 进度 |

### 2.2 生成 C++ 类型

```cpp
using base_interfaces_demo::action::Progress;

Progress::Goal
Progress::Result
Progress::Feedback
```

头文件路径：`base_interfaces_demo/action/progress.hpp`（小写文件名）。

### 2.3 句柄类型（两端不同！）

```cpp
// 服务端
using GoalHandleProgress = rclcpp_action::ServerGoalHandle<Progress>;

// 客户端
using GoalHandleProgress = rclcpp_action::ClientGoalHandle<Progress>;
```

| 类型 | 端 | 典型能力 |
|------|----|----------|
| `ServerGoalHandle<ActionT>` | 服务端 | `get_goal` / `publish_feedback` / `succeed` / `canceled` / `abort` / `is_canceling` |
| `ClientGoalHandle<ActionT>` | 客户端 | 标识本次目标；用于 `async_cancel_goal` 等 |

不要混用两个 Handle。

---

## 3. 公共 API（两端都用）

| API | 作用 |
|-----|------|
| `rclcpp::init` | 初始化 |
| `rclcpp::Node` | 节点基类（Action Server/Client 挂在其上） |
| `get_logger` + `RCLCPP_INFO/WARN/ERROR` | 日志 |
| `rclcpp::spin` | **必须**：否则服务端三回调、客户端三回调都不跑 |
| `rclcpp::shutdown` | 关闭（客户端结果回调里会调） |
| `rclcpp::ok` | 服务端 `succeed` 前检查节点是否还在 |
| `std::bind` / `placeholders` | 绑定回调 |

动作名（两端一致）：`"get_sum"`  
节点名：服务端 `progress_action_server`；客户端 `progress_action_client`

---

## 4. 服务端 API（demo01_action_server）

### 4.1 `rclcpp_action::create_server`

```cpp
action_server_ = rclcpp_action::create_server<Progress>(
  this,           // Node*
  "get_sum",      // 动作名
  handle_goal,    // 目标受理回调
  handle_cancel,  // 取消回调
  handle_accepted // 接受后回调
);
```

| 参数 | 含义 |
|------|------|
| 模板 `Progress` | 动作类型 |
| `node` | 所属节点（`this`） |
| `name` | 动作名；图上常显示为 `/get_sum` |
| 三个回调 | 见下表 |

返回：

```cpp
rclcpp_action::Server<Progress>::SharedPtr
```

成员保存为 `action_server_`，保证节点存活期间服务端存在。

---

### 4.2 回调① `handle_goal`

```cpp
rclcpp_action::GoalResponse handle_goal(
  const rclcpp_action::GoalUUID & uuid,
  std::shared_ptr<const Progress::Goal> goal);
```

| 参数 | 含义 |
|------|------|
| `uuid` | 本次 Goal 唯一 ID（本 demo 未用） |
| `goal` | 客户端 Goal；读 `goal->num` |

返回值 `rclcpp_action::GoalResponse`：

| 枚举 | 含义 | 本 demo |
|------|------|---------|
| `REJECT` | 拒绝 | `num <= 1` |
| `ACCEPT_AND_EXECUTE` | 接受并马上进入执行流程 | `num > 1` |
| `ACCEPT_AND_DEFER` | 接受但稍后执行 | 未用 |

---

### 4.3 回调② `handle_cancel`（主动终止入口）

```cpp
rclcpp_action::CancelResponse handle_cancel(
  const std::shared_ptr<GoalHandleProgress> goal_handle);
```

| 返回 | 含义 |
|------|------|
| `CancelResponse::ACCEPT` | 允许取消（本 demo） |
| `CancelResponse::REJECT` | 拒绝取消，任务继续 |

注意：这里返回 ACCEPT **只表示同意取消**；真正停下要等工作线程看到 `is_canceling()`。

---

### 4.4 回调③ `handle_accepted`

```cpp
void handle_accepted(const std::shared_ptr<GoalHandleProgress> goal_handle);
```

本 demo：

```cpp
std::thread{
  std::bind(&ProgressActionServer::execute, this, goal_handle)
}.detach();
```

| API | 含义 |
|-----|------|
| `std::thread{...}` | 创建工作线程跑 `execute` |
| `.detach()` | 分离线程，不 join；跑完自行结束 |

**不要**在 `handle_accepted` 里直接写长时间循环，否则堵死 `spin`，取消进不来。

---

### 4.5 `ServerGoalHandle` 上的执行期 API

均在 `execute()` 工作线程中使用：

#### `get_goal`

```cpp
const auto goal = goal_handle->get_goal();
int32_t num = goal->num;
```

取只读 Goal。

#### `is_canceling`

```cpp
if (goal_handle->is_canceling()) { ... }
```

| 项 | 说明 |
|----|------|
| 返回 | 客户端已取消且服务端 `handle_cancel` 已 ACCEPT 时为 true |
| 用法 | 协作式退出检查点 |

#### `publish_feedback`

```cpp
auto feedback = std::make_shared<Progress::Feedback>();
feedback->progress = ...;
goal_handle->publish_feedback(feedback);
```

每调用一次 → 客户端 `feedback_callback` 一次。

#### `succeed`

```cpp
auto result = std::make_shared<Progress::Result>();
result->sum = sum;
goal_handle->succeed(result);
```

正常结束 → 客户端 `ResultCode::SUCCEEDED`。

#### `canceled`

```cpp
result->sum = sum;  // 可带回部分结果
goal_handle->canceled(result);
```

取消收尾 → 客户端 `ResultCode::CANCELED`。

#### `abort`（本 demo 未调用，相关了解）

```cpp
goal_handle->abort(result);
```

服务端主动失败 → 客户端 `ResultCode::ABORTED`。

---

### 4.6 其它服务端相关

| API | 位置 | 说明 |
|-----|------|------|
| `std::this_thread::sleep_for(100ms)` | `execute` | 演示用延时，让进度可见 |
| `std::make_shared<Progress::Feedback/Result>` | `execute` | 构造反馈/结果消息 |
| `rclcpp::ok()` | `succeed` 前 | 避免节点已关仍发结果 |

---

## 5. 客户端 API（demo02_action_client）

### 5.1 `rclcpp_action::create_client`

```cpp
action_client_ = rclcpp_action::create_client<Progress>(this, "get_sum");
```

返回：`rclcpp_action::Client<Progress>::SharedPtr`

---

### 5.2 `Client::wait_for_action_server`

```cpp
if (!action_client_->wait_for_action_server(std::chrono::seconds(10))) {
  // 超时：shutdown
}
```

| 项 | 说明 |
|----|------|
| 作用 | 等待动作服务端上线 |
| 本 demo | 最多 10 秒；失败则报错退出 |
| 对照 | Service 的 `wait_for_service` |

---

### 5.3 `SendGoalOptions`（注册三个客户端回调）

```cpp
auto opts = rclcpp_action::Client<Progress>::SendGoalOptions();
opts.goal_response_callback = ...;
opts.feedback_callback = ...;
opts.result_callback = ...;
```

| 字段 | 触发 | 次数 |
|------|------|------|
| `goal_response_callback` | 服务端 ACCEPT/REJECT 后 | 通常 1 |
| `feedback_callback` | 每次 `publish_feedback` | N |
| `result_callback` | `succeed`/`canceled`/`abort` 后 | 通常 1 |

Options **本身不会循环**；只是把函数指针登记进去。

---

### 5.4 `Client::async_send_goal`

```cpp
action_client_->async_send_goal(goal_msg, send_goal_options);
```

| 项 | 说明 |
|----|------|
| 参数1 | `Progress::Goal` |
| 参数2 | `SendGoalOptions` |
| 返回 | 立刻返回（异步）；后续全靠回调 |
| 前提 | `spin` 正在跑 |

---

### 5.5 客户端三个回调签名

#### goal_response_callback

```cpp
void goal_response_callback(GoalHandleProgress::SharedPtr goal_handle);
```

| `goal_handle` | 含义 |
|---------------|------|
| `nullptr` | 被拒绝 |
| 非空 | 已接受；**保存它可以用于取消** |

#### feedback_callback

```cpp
void feedback_callback(
  GoalHandleProgress::SharedPtr goal_handle,
  const std::shared_ptr<const Progress::Feedback> feedback);
```

读 `feedback->progress`。

#### result_callback

```cpp
void result_callback(const GoalHandleProgress::WrappedResult & result);
```

| 字段 | 含义 |
|------|------|
| `result.code` | `ResultCode` 枚举 |
| `result.result` | `Progress::Result` 指针，含 `sum` |

`rclcpp_action::ResultCode`：

| 枚举 | 来源 |
|------|------|
| `SUCCEEDED` | 服务端 `succeed` |
| `CANCELED` | 服务端 `canceled` |
| `ABORTED` | 服务端 `abort` |
| 其它 | 未知/错误类 |

本 demo 在回调末尾 `rclcpp::shutdown()`。

---

### 5.6 主动取消相关 API（服务端已支持；客户端 demo 未调用，但属于本模块必会）

```cpp
// 需持有 goal_response_callback 里拿到的非空句柄
action_client_->async_cancel_goal(goal_handle);

// 也可取消该 client 跟踪的全部目标（API 名以发行版为准）
// action_client_->async_cancel_all_goals();
```

命令行等价验证：

```bash
ros2 action send_goal /get_sum base_interfaces_demo/action/Progress "{num: 200}"
ros2 action cancel_goal /get_sum          # 或带具体 goal id
```

取消链路对应服务端：

1. 客户端 `async_cancel_goal`
2. 服务端 `handle_cancel` → `ACCEPT`
3. 工作线程 `is_canceling()` → `canceled(result)`
4. 客户端 `result_callback` → `CANCELED`

---

## 6. 相关模块内容汇总

### 6.1 `rclcpp` vs `rclcpp_action`

| 库 | 提供什么 |
|----|----------|
| `rclcpp` | Node、spin、日志、通用客户端基础设施 |
| `rclcpp_action` | Action Server/Client、GoalHandle、Goal/Cancel/Result 枚举与选项 |

写 Action **两个头都要**。

### 6.2 `base_interfaces_demo` 在本模块的角色

| 文件 | 生成 |
|------|------|
| `action/Progress.action` | `Progress` Goal/Result/Feedback 类型 |

构建：`rosidl_generate_interfaces`（接口包 CMake）。  
运行前需 `source install/setup.bash`，否则找不到接口包。

### 6.3 标准库模块

| 模块 | 在本 demo 中的职责 |
|------|-------------------|
| `std::thread` | 服务端执行线程 |
| `std::chrono` | 等待超时、sleep |
| `std::placeholders` | `bind` 占位符 |
| `std::make_shared` | Feedback/Result/节点对象 |

### 6.4 与 Topic / Service 对照（API 层）

| 能力 | Topic | Service | Action |
|------|-------|---------|--------|
| 创建 | `create_publisher/subscription` | `create_service/client` | `create_server/client`（rclcpp_action） |
| 发送 | `publish` | `async_send_request` | `async_send_goal` |
| 等待对端 | 无（可先发） | `wait_for_service` | `wait_for_action_server` |
| 中间更新 | 消息流本身 | 无 | `publish_feedback` |
| 主动终止 | 无 | 无 | `async_cancel_goal` + `handle_cancel` + `canceled` |
| 终态 | 无「一次任务终态」 | Response 一次 | `succeed`/`canceled`/`abort` |

---

## 7. 常用命令行

```bash
source ~/learning/ws01_plumbing/install/setup.bash

ros2 run cpp03_action demo01_action_server
ros2 run cpp03_action demo02_action_client 100

ros2 action list
ros2 action info /get_sum
ros2 interface show base_interfaces_demo/action/Progress

# 发送并观察；另开终端取消
ros2 action send_goal -f /get_sum base_interfaces_demo/action/Progress "{num: 50}"
ros2 action cancel_goal /get_sum
```

`-f`：命令行客户端同时打印 feedback（便于对照本模块反馈 API）。

---

## 8. API 速查表

### 服务端

| API | 一句话 |
|-----|--------|
| `rclcpp_action::create_server` | 创建动作服务端，注册三回调 |
| `GoalResponse::ACCEPT_AND_EXECUTE / REJECT` | 接 / 拒 Goal |
| `CancelResponse::ACCEPT / REJECT` | 允 / 拒 取消 |
| `ServerGoalHandle::get_goal` | 读 Goal |
| `is_canceling` | 是否正在取消 |
| `publish_feedback` | 推进度 |
| `succeed` / `canceled` / `abort` | 三种终态 |
| `std::thread::detach` | 开执行线程 |

### 客户端

| API | 一句话 |
|-----|--------|
| `rclcpp_action::create_client` | 创建动作客户端 |
| `wait_for_action_server` | 等服务端 |
| `SendGoalOptions` 三字段 | 注册响应/反馈/结果回调 |
| `async_send_goal` | 异步发 Goal |
| `async_cancel_goal` | **主动终止**（demo 可用命令行代替） |
| `WrappedResult::code / result` | 读终态与 sum |
| `ResultCode::SUCCEEDED/CANCELED/ABORTED` | 终态枚举 |

### 公共

| API | 一句话 |
|-----|--------|
| `rclcpp::init/spin/shutdown/ok` | 生命周期 |
| `Progress::Goal/Result/Feedback` | 接口三段 |
| `RCLCPP_*` | 日志 |
