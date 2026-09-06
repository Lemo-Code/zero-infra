# Topic 发布/订阅 从 0 到 1：完整调用过程

> API / 依赖 / 类型详解见同目录：[`Topic_API与模块详解.md`](./Topic_API与模块详解.md)

对应代码：

- 接口：`base_interfaces_demo/msg/Student.msg`
- 发布端：`cpp01_topic/src/demo01_talker.cpp`
- 订阅端：`cpp01_topic/src/demo02_listener.cpp`
- 话题名：`chatter_stu`（两端必须相同）

运行：

```bash
# 终端 1：发布端（常驻）
ros2 run cpp01_topic demo01_talker

# 终端 2：订阅端（常驻）
ros2 run cpp01_topic demo02_listener
```

先开谁都行；订阅端开晚了，只能收到之后发布的消息（默认不补历史）。

---

## 0. 接口长什么样

`Student.msg` 只有一段消息体：

| 字段 | 类型 | 含义 |
|------|------|------|
| `name` | `string` | 姓名 |
| `age` | `int32` | 年龄 |
| `height` | `float64` | 身高 |

C++ 类型：`base_interfaces_demo::msg::Student`（本例别名 `Student`）。

Topic 模型一句话：

> **发布端按节奏往话题里丢消息；订阅端只要连上同名话题，就能持续收到；没有「请求-应答」。**

---

## 1. 完整时序（从进程启动到持续通信）

参与者：

- `P`：发布端主线程（`spin`）
- `S`：订阅端主线程（`spin`）

```mermaid
sequenceDiagram
    autonumber
    participant P as 发布端 Talker
    participant S as 订阅端 Listener

    Note over P: 【阶段1】发布端启动
    P->>P: rclcpp::init
    P->>P: new Talker → Node("talker_node")
    P->>P: create_publisher&lt;Student&gt;("chatter_stu", 10)
    P->>P: create_wall_timer(500ms, on_timer)
    P->>P: rclcpp::spin(talker) 阻塞

    Note over S: 【阶段2】订阅端启动
    S->>S: rclcpp::init
    S->>S: new Listener → Node("listener_node")
    S->>S: create_subscription&lt;Student&gt;("chatter_stu", 10, topic_callback)
    S->>S: rclcpp::spin(listener) 阻塞

    Note over P,S: 【阶段3】发现 / 匹配话题（底层自动完成）
    P-->>S: 同名话题 chatter_stu + 同类型 Student 建立匹配

    Note over P,S: 【阶段4】持续发布与接收（循环，直到 Ctrl+C）
    loop 每 500ms
        P->>P: on_timer 被 spin 触发
        P->>P: 组装 Student(name/age/height)
        P->>S: publisher-&gt;publish(msg)
        S->>S: topic_callback(msg)
        S->>S: 打印收到的学生信息
    end

    Note over P,S: 【阶段5】退出（任一侧 Ctrl+C）
    P->>P: shutdown / 进程结束
    S->>S: shutdown / 进程结束
```

---

## 2. 阶段拆解

### 阶段 1：发布端从 0 起来

`demo01_talker.cpp` 的 `main`：

1. `rclcpp::init`
2. `make_shared<Talker>()`
3. 构造函数里：
   - `create_publisher<Student>("chatter_stu", 10)` —— 创建发布者
   - `create_wall_timer(500ms, on_timer)` —— 注册定时器回调
4. `rclcpp::spin(node)` —— **没有 spin，定时器不会响，消息发不出去**

定时器回调里做的事：

```text
组装 msg → publisher_->publish(msg) → 打印日志
```

本例每 500ms 发一条，`age` / `height` 随 `count_` 轻微变化，方便观察。

队列深度 `10`：发得太快、订得太慢时，中间最多攒 10 条，再多会丢旧的（具体策略与 QoS 有关；本 demo 用默认）。

---

### 阶段 2：订阅端从 0 起来

`demo02_listener.cpp` 的 `main`：

1. `rclcpp::init`
2. `make_shared<Listener>()`
3. 构造函数里：

```cpp
create_subscription<Student>(
  "chatter_stu",   // 必须和发布端同名
  10,              // 队列深度
  topic_callback   // 每收到一条调一次
);
```

4. `rclcpp::spin(node)` —— **没有 spin，订阅回调永远不执行**

---

### 阶段 3：话题如何对上

两端都起来后，ROS 2 中间件根据：

- 话题名：`chatter_stu`
- 消息类型：`Student`

自动完成匹配。业务代码里**没有**显式的 `connect()`；你只要名字、类型一致即可。

注意：

- 可以 1 个发布端 + 多个订阅端（广播）
- 也可以多个发布端往同一话题发（订阅端都会收到）
- 订阅端启动前已经发出的消息，**默认收不到**（Topic 不保证历史回放）

---

### 阶段 4：一条消息的路径

```text
P: spin 到点 → on_timer
     → 填 Student
     → publish
         ↓（DDS / 中间件）
S: spin 收到 → topic_callback(msg)
     → 打印 name / age / height
```

这是**单向、持续、无回复**的流。发布端不知道有没有人在听，也不等订阅端回什么。

---

### 阶段 5：退出

本 demo 两端都是常驻 `spin`，用 Ctrl+C 结束；没有「算完就自动退出」的逻辑。

---

## 3. 线程关系

```text
发布端进程
└── 主线程 spin
      ├── 定时器回调 on_timer（周期性）
      └── publish

订阅端进程
└── 主线程 spin
      └── topic_callback（每条消息一次）
```

本 demo **没有**额外 `std::thread`。所有回调都在各自进程的 `spin` 线程里跑。

---

## 4. 必须记住的 4 句话

1. **Topic = 同名话题上的持续广播，不是一问一答。**
2. **发布靠定时器（或你自己的循环）触发；订阅靠回调收。**
3. **两端都要 `spin`，回调才会执行。**
4. **名字 + 类型一致就能通；晚订阅默认丢历史。**
