# Service 模块：API 与相关内容详解

> 配套文档：`Service调用过程详解.md`（讲调用过程）  
> 本文件：讲 **本模块用到的全部 API、类型、依赖包、构建与命令行**

对应包：`cpp02_service`  
接口包：`base_interfaces_demo`（`srv/AddInts.srv`）  
源码：`demo01_server.cpp` / `demo02_client.cpp`

---

## 1. 模块与依赖关系

```text
cpp02_service
├── depend: rclcpp
├── depend: base_interfaces_demo   ← AddInts 服务定义从这里来
└── 底层仍走 rmw/DDS（不直接调用）
```

`package.xml` 关键依赖：

| 依赖 | 作用 |
|------|------|
| `ament_cmake` | 构建 |
| `rclcpp` | Node / Service / Client / spin |
| `base_interfaces_demo` | `AddInts` 服务接口 |

可执行文件：

| 目标 | 源文件 | 角色 |
|------|--------|------|
| `demo01_server` | `demo01_server.cpp` | 服务端 |
| `demo02_client` | `demo02_client.cpp` | 客户端 |

头文件：

```cpp
#include "rclcpp/rclcpp.hpp"
#include "base_interfaces_demo/srv/add_ints.hpp"
```

生成规则：`AddInts.srv` → `base_interfaces_demo/srv/add_ints.hpp`  
类型名：`base_interfaces_demo::srv::AddInts`

---

## 2. 服务接口（相关模块：base_interfaces_demo）

### 2.1 `AddInts.srv` 结构

```text
int32 num1      # Request
int32 num2
---
int32 sum       # Response
```

| 段 | 字段 | C++ 访问 | 谁填 |
|----|------|----------|------|
| Request | `num1`, `num2` | `request->num1` | 客户端 |
| Response | `sum` | `response->sum` | 服务端 |

### 2.2 生成类型怎么用

```cpp
using base_interfaces_demo::srv::AddInts;

AddInts::Request::SharedPtr  request;
AddInts::Response::SharedPtr response;

// 或
auto request = std::make_shared<AddInts::Request>();
request->num1 = 3;
request->num2 = 5;
```

嵌套类型：

| 类型 | 含义 |
|------|------|
| `AddInts` | 整个服务接口标签类型 |
| `AddInts::Request` | 请求消息 |
| `AddInts::Response` | 响应消息 |
| `AddInts::Request::SharedPtr` | `shared_ptr<Request>` |

---

## 3. 公共 API（两端都用）

与 Topic 相同的基础件：

| API | 作用 |
|-----|------|
| `rclcpp::init(argc, argv)` | 初始化 |
| `rclcpp::Node` | 节点基类 |
| `this->get_logger()` + `RCLCPP_*` | 日志 |
| `rclcpp::spin(node)` | 调度回调（**服务回调 / 响应回调都靠它**） |
| `rclcpp::shutdown()` | 关闭 |
| `rclcpp::ok()` | 上下文是否仍在运行（客户端等待循环里用到） |
| `std::bind` / `_1` `_2` | 绑定成员回调 |

本模块节点名：

| 端 | 节点名 |
|----|--------|
| 服务端 | `add_ints_server` |
| 客户端 | `add_ints_client` |

服务名（两端必须一致）：`"add_ints"`

---

## 4. 服务端 API（demo01_server）

### 4.1 `Node::create_service`

```cpp
server_ = this->create_service<AddInts>(
  "add_ints",
  std::bind(&AddIntsServer::handle_add, this, _1, _2));
```

| 参数 | 本 demo | 含义 |
|------|---------|------|
| 模板 `ServiceT` | `AddInts` | 服务接口类型 |
| `service_name` | `"add_ints"` | 服务名 |
| `callback` | `handle_add` | 收到请求时调用 |

返回：

```cpp
rclcpp::Service<AddInts>::SharedPtr
```

相关类型：

| 类型 | 说明 |
|------|------|
| `rclcpp::Service<ServiceT>` | 服务端对象 |
| `Service::SharedPtr` | 成员保存，防止被析构 |

### 4.2 服务回调签名

```cpp
void handle_add(
  const AddInts::Request::SharedPtr request,
  AddInts::Response::SharedPtr response);
```

| 参数 | 方向 | 说明 |
|------|------|------|
| `request` | 入 | 客户端发来的数据，只读使用 |
| `response` | 出 | **你在回调里填好**；函数返回后框架发给客户端 |

本 demo 核心逻辑：

```cpp
response->sum = request->num1 + request->num2;
```

注意：

- 回调在 `spin` 线程执行
- 本例计算极快，直接在回调里算完即可
- 若逻辑很重，长期阻塞会卡住同节点其它回调（进阶再考虑多线程执行器）

### 4.3 服务端成员

| 成员 | 类型 | 作用 |
|------|------|------|
| `server_` | `Service<AddInts>::SharedPtr` | 保持服务存活 |

---

## 5. 客户端 API（demo02_client）

### 5.1 `Node::create_client`

```cpp
client_ = this->create_client<AddInts>("add_ints");
```

| 参数 | 含义 |
|------|------|
| 模板 `AddInts` | 服务类型，必须与服务端一致 |
| `"add_ints"` | 服务名，必须与服务端一致 |

返回：

```cpp
rclcpp::Client<AddInts>::SharedPtr
```

### 5.2 `Client::wait_for_service`

```cpp
while (!client_->wait_for_service(1s)) {
  if (!rclcpp::ok()) { ...; return; }
  RCLCPP_WARN(...);
}
```

| 项 | 说明 |
|----|------|
| 作用 | 等待图上出现该服务 |
| 参数 | 超时时间（本例每次等 1 秒） |
| 返回 | `true`=服务已可用；`false`=超时仍没有 |
| 本 demo 策略 | 超时就警告并继续循环等（无总超时上限） |

`1s` 来自：

```cpp
using namespace std::chrono_literals;
```

### 5.3 `Client::async_send_request`

```cpp
client_->async_send_request(
  request,
  std::bind(&AddIntsClient::response_callback, this, _1));
```

| 参数 | 含义 |
|------|------|
| `request` | `shared_ptr<AddInts::Request>` |
| `callback` | 响应到达时调用 |

| 项 | 说明 |
|----|------|
| 返回时机 | **立刻返回**，不在这里等加法结果 |
| 结果如何拿 | 在 `response_callback` 里通过 `SharedFuture` |
| 前提 | 之后必须 `spin`，否则回调不跑 |

相关类型：

```cpp
rclcpp::Client<AddInts>::SharedFuture
```

本质是带 `shared_ptr` 语义的 future，里面装着 `Response`。

### 5.4 响应回调

```cpp
void response_callback(rclcpp::Client<AddInts>::SharedFuture future)
{
  auto response = future.get();
  // response->sum
  rclcpp::shutdown();
}
```

| API | 说明 |
|-----|------|
| `future.get()` | 取出 `Response`（在回调里调用时，结果通常已就绪） |
| 异常 | 本 demo 用 `try/catch` 包住 |

本 demo 在回调末尾 `rclcpp::shutdown()`：演示「请求一次就退出」。

### 5.5 同步写法（本 demo 未用，对照了解）

也存在「发完再阻塞等」的用法（示意）：

```cpp
auto future = client_->async_send_request(request);
// 再配合 executor / spin_until_future_complete 等待
```

本学习代码统一用 **异步 + 回调**，风格更接近 Action 客户端。

### 5.6 客户端成员 / 命令行参数

| 成员 | 类型 | 作用 |
|------|------|------|
| `client_` | `Client<AddInts>::SharedPtr` | 客户端对象 |
| `num1_`, `num2_` | `int32_t` | 要发送的加数 |

```bash
ros2 run cpp02_service demo02_client 3 5
# argv[1]=num1, argv[2]=num2；缺省则 1 和 2
```

使用了 `<cstdlib>` 的 `std::atoi`。

---

## 6. 相关概念补充

### 6.1 服务名 vs 话题名

- Service 用**服务名**匹配（本例 `add_ints`）
- Topic 用**话题名**匹配
- 都要求类型一致

### 6.2 与 Topic / Action 边界

| | Topic | Service（本模块） | Action |
|--|-------|-------------------|--------|
| API 核心 | `publish` / subscription 回调 | `create_service` / `async_send_request` | `create_server` / `async_send_goal` |
| 中间过程 | 持续消息流 | **无** | Feedback |
| 取消 | 无 | **无** | `cancel` |
| 本仓库包 | `cpp01_topic` | `cpp02_service` | `cpp03_action` |

### 6.3 为何 Service 回调里通常不开线程

- 一问一答、本例计算极短
- Action 才需要长时间执行 + 反馈 + 取消，所以要工作线程

---

## 7. 常用命令行

```bash
source ~/learning/ws01_plumbing/install/setup.bash

ros2 run cpp02_service demo01_server
ros2 run cpp02_service demo02_client 3 5

ros2 service list
ros2 service type /add_ints
ros2 interface show base_interfaces_demo/srv/AddInts
ros2 service call /add_ints base_interfaces_demo/srv/AddInts "{num1: 3, num2: 5}"
```

---

## 8. API 速查表

| API | 用在 | 一句话 |
|-----|------|--------|
| `rclcpp::init` / `spin` / `shutdown` / `ok` | 两端 | 生命周期 |
| `Node` / `get_logger` / `RCLCPP_*` | 两端 | 节点与日志 |
| `create_service` | Server | 创建服务端并注册回调 |
| `handle_add(request, response)` | Server | 填 `response->sum` |
| `create_client` | Client | 创建客户端 |
| `wait_for_service` | Client | 等服务上线 |
| `async_send_request` | Client | 异步发请求 |
| `SharedFuture::get` | Client | 取响应 |
| `AddInts::Request/Response` | 两端 | 接口字段 |
| `std::atoi` | Client | 解析命令行数字 |
