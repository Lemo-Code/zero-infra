# Topic 模块：API 与相关内容详解

> 配套文档：`Topic调用过程详解.md`（讲调用过程）  
> 本文件：讲 **本模块用到的全部 API、类型、依赖包、构建与命令行**

对应包：`cpp01_topic`  
接口包：`base_interfaces_demo`（`msg/Student.msg`）  
源码：`demo01_talker.cpp` / `demo02_listener.cpp`

---

## 1. 模块与依赖关系

```text
cpp01_topic
├── depend: rclcpp
├── depend: base_interfaces_demo   ← Student 消息从这里来
└── 间接依赖（随 ros/jazzy 带入）
      └── rmw / DDS 实现（本 demo 不直接调用）
```

`package.xml` 关键依赖：

| 依赖 | 作用 |
|------|------|
| `ament_cmake` | C++ 包构建 |
| `rclcpp` | ROS 2 C++ 客户端库（Node / Pub / Sub / Timer / spin） |
| `base_interfaces_demo` | 自定义 `Student` 消息 |

`CMakeLists.txt` 关键目标：

| 可执行文件 | 源文件 | 角色 |
|------------|--------|------|
| `demo01_talker` | `demo01_talker.cpp` | 发布端 |
| `demo02_listener` | `demo02_listener.cpp` | 订阅端 |

头文件：

```cpp
#include "rclcpp/rclcpp.hpp"
#include "base_interfaces_demo/msg/student.hpp"
```

生成规则：`Student.msg` → `base_interfaces_demo/msg/student.hpp`  
（文件名小写 + `.hpp`，命名空间 `base_interfaces_demo::msg::Student`）

---

## 2. 消息类型（相关模块：base_interfaces_demo）

### 2.1 接口定义 `Student.msg`

| 字段 | IDL 类型 | C++ 类型（大致） | 说明 |
|------|----------|------------------|------|
| `name` | `string` | `std::string` | 姓名 |
| `age` | `int32` | `int32_t` | 年龄 |
| `height` | `float64` | `double` | 身高 |

### 2.2 代码里怎么用

```cpp
using base_interfaces_demo::msg::Student;

Student msg;
msg.name = "张三";
msg.age = 18;
msg.height = 1.70;

// 订阅回调参数常用：
void topic_callback(const Student::SharedPtr msg);
// SharedPtr = std::shared_ptr<Student>
```

相关生成物（了解即可，一般不手改）：

- `student.hpp`：C++ 消息类
- `rosidl` 类型支持、序列化等（构建时自动生成）

---

## 3. 公共 API（两端都用）

### 3.1 `rclcpp::init`

```cpp
rclcpp::init(argc, argv);
```

| 项 | 说明 |
|----|------|
| 作用 | 初始化 ROS 2 上下文（解析 ROS 相关参数、启动底层通信） |
| 调用时机 | `main` 最前面，创建任何 Node 之前 |
| 注意 | 一个进程通常只 `init` 一次 |

### 3.2 `rclcpp::Node`

```cpp
class Talker : public rclcpp::Node {
  Talker() : Node("talker_node") { ... }
};
```

| 项 | 说明 |
|----|------|
| 作用 | 节点基类；Pub/Sub/Timer/Service 等都挂在 Node 上 |
| 构造参数 | 节点名字符串（图上显示的名字） |
| 本模块节点名 | 发布端 `talker_node`；订阅端 `listener_node` |

常用成员（本模块用到）：

| API | 说明 |
|-----|------|
| `this->get_logger()` | 取日志器，配合 `RCLCPP_*` 宏 |
| `this->create_publisher<Msg>(...)` | 创建发布者 |
| `this->create_subscription<Msg>(...)` | 创建订阅者 |
| `this->create_wall_timer(...)` | 创建墙上时钟定时器 |

### 3.3 日志宏

```cpp
RCLCPP_INFO(this->get_logger(), "格式串 %s %d %.2f", ...);
RCLCPP_WARN(...);
RCLCPP_ERROR(...);
```

| 宏 | 级别 |
|----|------|
| `RCLCPP_DEBUG` | 调试 |
| `RCLCPP_INFO` | 信息（本 demo 主要用） |
| `RCLCPP_WARN` | 警告 |
| `RCLCPP_ERROR` | 错误 |

### 3.4 `rclcpp::spin`

```cpp
rclcpp::spin(node);
```

| 项 | 说明 |
|----|------|
| 作用 | 阻塞处理该节点上的回调（定时器、订阅等） |
| 没有 spin | **定时器不触发、订阅收不到** |
| 退出 | `Ctrl+C` 或别处调用 `rclcpp::shutdown()` 后返回 |

### 3.5 `rclcpp::shutdown`

```cpp
rclcpp::shutdown();
```

关闭上下文，使 `spin` 有机会返回；本 Topic demo 通常靠 Ctrl+C 触发清理。

### 3.6 `std::bind` / `placeholders`

```cpp
using namespace std::placeholders;  // _1, _2, ...
std::bind(&Listener::topic_callback, this, _1);
```

把成员函数绑成 `std::function`，交给 `create_subscription` / `create_wall_timer`。

---

## 4. 发布端 API（demo01_talker）

### 4.1 `Node::create_publisher`

```cpp
publisher_ = this->create_publisher<Student>("chatter_stu", 10);
```

| 参数 | 本 demo | 含义 |
|------|---------|------|
| 模板参数 `Msg` | `Student` | 消息类型 |
| `topic_name` | `"chatter_stu"` | 话题名，必须与订阅端一致 |
| `qos` | `10` | 简化写法：队列深度 10（等价于部分默认 QoS + depth） |

返回类型：

```cpp
rclcpp::Publisher<Student>::SharedPtr
```

相关类型：

| 类型 | 说明 |
|------|------|
| `rclcpp::Publisher<Msg>` | 发布者类 |
| `Publisher::SharedPtr` | `shared_ptr` 别名，作成员保存，保证节点存活期间发布者有效 |

### 4.2 `Publisher::publish`

```cpp
publisher_->publish(msg);
```

| 项 | 说明 |
|----|------|
| 作用 | 把一条 `Student` 发到话题 `chatter_stu` |
| 阻塞性 | 一般很快返回；**不等待**有没有订阅者、也不等对方处理完 |
| 无人订阅 | 消息通常被丢弃（默认不持久化历史） |

### 4.3 `Node::create_wall_timer`

```cpp
timer_ = this->create_wall_timer(
  500ms, std::bind(&Talker::on_timer, this));
```

| 参数 | 本 demo | 含义 |
|------|---------|------|
| `period` | `500ms`（`std::chrono_literals`） | 触发周期 |
| `callback` | `on_timer` | 到期时调用的函数 |

返回：

```cpp
rclcpp::TimerBase::SharedPtr
```

要点：

- **wall timer** = 按墙上时钟（真实时间）计时
- 回调在 `spin` 线程中执行
- 本 demo 用定时器驱动 `publish`，而不是 `while` 里自己 sleep（更符合 ROS 风格）

### 4.4 发布端成员一览

| 成员 | 类型 | 作用 |
|------|------|------|
| `publisher_` | `Publisher<Student>::SharedPtr` | 发消息 |
| `timer_` | `TimerBase::SharedPtr` | 周期触发 |
| `count_` | `size_t` | 变化 age/height 用 |

---

## 5. 订阅端 API（demo02_listener）

### 5.1 `Node::create_subscription`

```cpp
subscription_ = this->create_subscription<Student>(
  "chatter_stu",
  10,
  std::bind(&Listener::topic_callback, this, _1));
```

| 参数 | 本 demo | 含义 |
|------|---------|------|
| 模板 `Msg` | `Student` | 必须与发布端相同 |
| `topic_name` | `"chatter_stu"` | 必须与发布端相同 |
| `qos` | `10` | 接收队列深度 |
| `callback` | `topic_callback` | 每收到一条调用一次 |

返回：

```cpp
rclcpp::Subscription<Student>::SharedPtr
```

### 5.2 订阅回调签名

```cpp
void topic_callback(const Student::SharedPtr msg);
```

| 项 | 说明 |
|----|------|
| 触发条件 | 话题上到来一条匹配类型的消息 |
| `msg` | 共享指针，读字段即可：`msg->name` 等 |
| 注意 | 回调里不要做很久的阻塞，否则影响同节点其它回调 |

### 5.3 订阅端成员

| 成员 | 类型 | 作用 |
|------|------|------|
| `subscription_` | `Subscription<Student>::SharedPtr` | 保持订阅存活 |

---

## 6. 相关概念补充（本模块会碰到）

### 6.1 话题名

- 逻辑名：`chatter_stu`
- 实际图上可能带命名空间前缀（若节点在 namespace 下）
- 两端**字符串必须一致**才能匹配

### 6.2 QoS（本 demo 只用了 depth）

传入整数 `10` 是简写。进阶会用：

```cpp
rclcpp::QoS(10).reliable();   // 示例，本 demo 未写
```

学习阶段记住：depth = 缓存条数；发太快订太慢可能丢。

### 6.3 一对多

- 1 个 Talker + N 个 Listener：每人都能收到
- N 个 Talker + 1 个 Listener：订阅端会收到所有发布端的消息

### 6.4 与 Service / Action 的模块边界

| 能力 | Topic 本模块有没有 |
|------|-------------------|
| 持续广播 | 有 |
| 请求应答 | 无（那是 `cpp02_service`） |
| 进度反馈 + 取消 | 无（那是 `cpp03_action`） |

---

## 7. 常用命令行（调试相关）

```bash
source ~/learning/ws01_plumbing/install/setup.bash

ros2 run cpp01_topic demo01_talker
ros2 run cpp01_topic demo02_listener

ros2 topic list
ros2 topic echo /chatter_stu
ros2 topic info /chatter_stu -v
ros2 interface show base_interfaces_demo/msg/Student
```

---

## 8. API 速查表

| API | 用在 | 一句话 |
|-----|------|--------|
| `rclcpp::init` | 两端 | 初始化 |
| `rclcpp::Node` | 两端 | 节点基类 |
| `create_publisher` | Talker | 创建发布者 |
| `publish` | Talker | 发一条消息 |
| `create_wall_timer` | Talker | 周期回调 |
| `create_subscription` | Listener | 创建订阅者 |
| `topic_callback` | Listener | 收消息处理 |
| `rclcpp::spin` | 两端 | 调度回调 |
| `rclcpp::shutdown` | 两端 | 关闭 |
| `RCLCPP_INFO` | 两端 | 打日志 |
| `Student` | 两端 | 自定义消息类型 |
