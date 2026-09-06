# Service 服务端/客户端 从 0 到 1：完整调用过程

> API / 依赖 / 类型详解见同目录：[`Service_API与模块详解.md`](./Service_API与模块详解.md)

对应代码：

- 接口：`base_interfaces_demo/srv/AddInts.srv`
- 服务端：`cpp02_service/src/demo01_server.cpp`
- 客户端：`cpp02_service/src/demo02_client.cpp`
- 服务名：`add_ints`（两端必须相同）

运行：

```bash
# 终端 1：服务端（常驻）
ros2 run cpp02_service demo01_server

# 终端 2：客户端（请求一次后退出）
ros2 run cpp02_service demo02_client 3 5
```

必须先有服务端；客户端会 `wait_for_service` 等待。

---

## 0. 接口长什么样

`AddInts.srv` 用 `---` 分成请求 / 响应两段：

| 段 | 字段 | 谁填 | 含义 |
|----|------|------|------|
| Request | `num1`, `num2` | 客户端 | 两个加数 |
| Response | `sum` | 服务端 | 两数之和 |

C++ 类型：`base_interfaces_demo::srv::AddInts`（本例别名 `AddInts`）。

Service 模型一句话：

> **客户端发一次 Request，服务端算完回一次 Response；没有中间进度，也没有连续广播。**

和 Topic / Action 对比：

| | Topic | Service（本例） | Action |
|--|-------|-----------------|--------|
| 方向 | 单向持续 | 一问一答 | 目标 + 反馈 + 结果 |
| 中间进度 | 无（本身就是流） | 无 | 有 Feedback |
| 取消 | 无 | 基本无 | 有 |
| 本仓库包 | `cpp01_topic` | `cpp02_service` | `cpp03_action` |

---

## 1. 完整时序（从进程启动到客户端退出）

参与者：

- `Srv`：服务端主线程（`spin`）
- `Cli`：客户端主线程（`spin`）

```mermaid
sequenceDiagram
    autonumber
    participant Srv as 服务端 Server
    participant Cli as 客户端 Client

    Note over Srv: 【阶段1】服务端启动
    Srv->>Srv: rclcpp::init
    Srv->>Srv: new AddIntsServer → Node("add_ints_server")
    Srv->>Srv: create_service&lt;AddInts&gt;("add_ints", handle_add)
    Srv->>Srv: rclcpp::spin(server) 阻塞等待请求

    Note over Cli: 【阶段2】客户端启动
    Cli->>Cli: rclcpp::init
    Cli->>Cli: 解析 argv 得到 num1, num2（默认 1, 2）
    Cli->>Cli: new AddIntsClient(num1, num2)
    Cli->>Cli: create_client&lt;AddInts&gt;("add_ints")
    Cli->>Cli: send_request()

    Note over Cli,Srv: 【阶段3】发现服务端
    loop 直到服务可用或被中断
        Cli->>Srv: wait_for_service(1s)
        alt 服务端未就绪
            Cli->>Cli: 打印警告，继续等
        else 服务端已上线
            Srv-->>Cli: 服务可用
        end
    end

    Note over Cli,Srv: 【阶段4】发请求
    Cli->>Cli: 填写 Request{num1, num2}
    Cli->>Srv: async_send_request(request, response_callback)
    Note right of Cli: 立刻返回，不在此处阻塞等结果
    Cli->>Cli: rclcpp::spin(client) 开始等响应回调

    Note over Cli,Srv: 【阶段5】服务端处理并应答
    Srv->>Srv: spin 触发 handle_add(request, response)
    Srv->>Srv: response-&gt;sum = num1 + num2
    Srv-->>Cli: 返回 Response{sum}

    Note over Cli: 【阶段6】客户端收结果并退出
    Cli->>Cli: response_callback(future)
    Cli->>Cli: future.get() → 打印 sum
    Cli->>Cli: rclcpp::shutdown()
    Cli->>Cli: spin 返回 / 进程结束

    Note over Srv: 服务端继续 spin，可接下一个客户端
```

---

## 2. 阶段拆解

### 阶段 1：服务端从 0 起来

`demo01_server.cpp` 的 `main`：

1. `rclcpp::init`
2. `make_shared<AddIntsServer>()`
3. 构造函数里：

```cpp
create_service<AddInts>("add_ints", handle_add);
```

只注册**一个**回调：收到请求时进 `handle_add`。

4. `rclcpp::spin(server)` —— **没有 spin，请求回调不会触发**

`handle_add` 职责很单纯：

```text
读 request->num1 / num2
→ 算 response->sum
→ 函数返回后框架把 response 送回客户端
```

本例计算极快，所以**不需要**像 Action 那样再开工作线程。若服务端逻辑很重、会堵很久，才考虑多线程 / 异步执行器（进阶话题）。

---

### 阶段 2：客户端从 0 起来

`demo02_client.cpp` 的 `main`：

1. `rclcpp::init`
2. 读命令行：`demo02_client 3 5` → `num1=3, num2=5`
3. `make_shared<AddIntsClient>(num1, num2)` → 内部 `create_client("add_ints")`
4. 立刻 `send_request()`（此时还没 `spin`）
5. `rclcpp::spin(client)` —— 之后 `response_callback` 靠它调度

---

### 阶段 3：发现服务端

`send_request()` 开头：

```cpp
while (!client_->wait_for_service(1s)) {
  // 服务端没开：打警告，继续等
}
```

- 服务端已开 → 跳出循环，继续发请求
- 服务端一直不开 → 客户端会一直等（本 demo 没有总超时退出，只会周期性警告）

这对应 Action 里的 `wait_for_action_server`，都是「先确认对端在，再发业务」。

---

### 阶段 4：异步发出 Request

```cpp
auto request = std::make_shared<AddInts::Request>();
request->num1 = num1_;
request->num2 = num2_;

client_->async_send_request(
  request,
  std::bind(&AddIntsClient::response_callback, this, _1));
```

要点：

- `async_send_request` **马上返回**，不在这里等加法算完
- 结果通过 `response_callback` 异步到达
- 因此后面必须 `spin`，否则回调永远不跑

（也存在同步写法，但本学习 demo 用的是异步 + 回调，和 Action 客户端风格一致。）

---

### 阶段 5：服务端处理

服务端 `spin` 收到请求后调用：

```cpp
void handle_add(Request::SharedPtr request, Response::SharedPtr response)
{
  response->sum = request->num1 + request->num2;
}
```

函数返回后，ROS 2 把填好的 `response` 发回客户端。  
业务上这就是完整的「一问一答」，没有 Feedback 阶段。

---

### 阶段 6：客户端收结果并退出

```cpp
void response_callback(SharedFuture future)
{
  auto response = future.get();
  // 打印 num1 + num2 = sum
  rclcpp::shutdown();  // 演示：拿一次结果就退出
}
```

| 进程 | 结束后行为 |
|------|------------|
| 客户端 | `shutdown` → `spin` 结束 → 进程退出 |
| 服务端 | **继续 spin**，可服务下一个 `demo02_client` |

---

## 3. 线程关系

```text
服务端进程
└── 主线程 spin
      └── handle_add（每次请求一次）

客户端进程
└── 主线程 spin
      └── response_callback（本 demo 一次请求对应一次）
```

本 demo **没有**额外工作线程。和 Action 的差别正在这里：Service 回调短、一次返回；Action 任务长，所以要 `detach` 执行 + 多次反馈。

---

## 4. 一次完整路径（对照记）

**正常（服务端已开，`3 5`）**

```text
服务端 init → create_service → spin
客户端 init → create_client → wait_for_service
         → async_send_request → spin
→ handle_add：sum = 8
→ response_callback 打印 3+5=8 → 客户端退出
服务端继续等下一次请求
```

**服务端未开**

```text
客户端 wait_for_service 循环警告……
（直到你把 demo01_server 拉起来，才会进入发请求）
```

---

## 5. 必须记住的 4 句话

1. **Service = 一次 Request + 一次 Response，没有连续反馈。**
2. **先 `create_service` / `create_client`，客户端再 `wait_for_service`，然后发请求。**
3. **`async_send_request` 只负责发出去；结果在 `response_callback`，必须靠 `spin`。**
4. **服务端通常常驻；本 demo 客户端拿一次结果就 `shutdown` 退出。**
