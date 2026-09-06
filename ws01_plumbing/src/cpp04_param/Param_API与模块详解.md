# 参数服务模块：API 与相关内容详解

> 配套文档：`Param调用过程详解.md`  
> 课程：**2.5.3 参数服务 (C++)**  
> 包：`cpp04_param`

---

## 1. 模块与依赖

```text
cpp04_param
├── depend: rclcpp
└── depend: rcl_interfaces   ← SetParametersResult 等
```

| 可执行文件 | 源文件 | 角色 |
|------------|--------|------|
| `demo01_param_server` | `demo01_param_server.cpp` | 声明并持有参数 |
| `demo02_param_client` | `demo02_param_client.cpp` | 远程 list/get/set |

头文件：

```cpp
#include "rclcpp/rclcpp.hpp"
#include "rcl_interfaces/msg/set_parameters_result.hpp"  // 服务端回调返回值
```

---

## 2. 服务端 API

### 2.1 `declare_parameter`

```cpp
this->declare_parameter<std::string>("car_name", "turtle");
this->declare_parameter<double>("width", 0.25);
this->declare_parameter<double>("length", 0.45);
```

| 项 | 说明 |
|----|------|
| 作用 | 在本节点登记参数名、类型、默认值 |
| 不声明 | 远程 get/set 通常不可用或不符合预期 |
| 模板 | 指定 C++ 类型；也可用非模板重载 + `ParameterValue` |

可选进阶：带 `ParameterDescriptor`（描述、范围），本基础 demo 未展开。

### 2.2 `get_parameter` / `as_*`

```cpp
car_name_ = this->get_parameter("car_name").as_string();
width_    = this->get_parameter("width").as_double();
```

| 方法 | 类型 |
|------|------|
| `as_bool()` | bool |
| `as_int()` | int64 |
| `as_double()` | double |
| `as_string()` | string |
| `as_double_array()` 等 | 数组 |

也可用 `this->get_parameter("width", width_);` 等形式（重载较多，见官方文档）。

### 2.3 `add_on_set_parameters_callback`

```cpp
param_cb_handle_ = this->add_on_set_parameters_callback(
  std::bind(&ParamServer::on_set_parameters, this, _1));
```

| 项 | 说明 |
|----|------|
| 何时触发 | 有人对本节点执行 set（含 `ros2 param set`、Param Client） |
| 返回 | `rcl_interfaces::msg::SetParametersResult` |
| `successful` | `true` 接受；`false` 拒绝 |
| `reason` | 拒绝原因字符串 |
| 句柄 | 必须用成员保存，否则回调可能被注销 |

回调签名：

```cpp
SetParametersResult on_set_parameters(
  const std::vector<rclcpp::Parameter> & parameters);
```

一次 set 可能带多个 `Parameter`，需全部检查。

### 2.4 定时器 / spin / 日志

与 Topic 模块相同：`create_wall_timer`、`rclcpp::spin`、`RCLCPP_INFO`。

---

## 3. 客户端 API

### 3.1 `rclcpp::SyncParametersClient`

```cpp
param_client_ = std::make_shared<rclcpp::SyncParametersClient>(
  this, "param_server_node");
```

| 参数 | 含义 |
|------|------|
| `this` | 本客户端节点 |
| `"param_server_node"` | **要操作的远端节点名** |

同步客户端：调用阻塞到结果返回，适合教学 / 工具脚本。  
异步版：`rclcpp::AsyncParametersClient`（本 demo 未用）。

### 3.2 `wait_for_service`

```cpp
param_client_->wait_for_service(10s);
```

等待远端参数相关服务就绪（对端节点已启动并 declare）。

### 3.3 `list_parameters`

```cpp
auto listed = param_client_->list_parameters({}, 0);
// listed.names → vector<string>
```

| 参数 | 本 demo | 含义 |
|------|---------|------|
| prefixes | `{}` | 空 = 不按前缀过滤 |
| depth | `0` | 深度限制（0 表示不限制，按实现约定） |

### 3.4 `get_parameters`

```cpp
auto params = param_client_->get_parameters({"car_name", "width", "length"});
p.get_name();
p.value_to_string();
p.as_double();  // 等
```

返回 `std::vector<rclcpp::Parameter>`。

### 3.5 `set_parameters`

```cpp
std::vector<rclcpp::Parameter> to_set = {
  rclcpp::Parameter("car_name", std::string("turtle2")),
  rclcpp::Parameter("width", 0.30),
};
auto results = param_client_->set_parameters(to_set);
// results[i].successful / results[i].reason
```

| 类型 | 说明 |
|------|------|
| 入参 | `vector<rclcpp::Parameter>` |
| 返回 | 每个参数一条 `SetParametersResult` |

构造 `rclcpp::Parameter`：

```cpp
rclcpp::Parameter("name", value);  // value 可以是 bool/int/double/string/...
```

### 3.6 本 demo 为何 main 里不 `spin`

`SyncParametersClient` 在 wait/get/set 时会自行处理与参数服务的交互；  
演示做完即 `shutdown`。若客户端要长期订阅参数事件，再另开 `spin`。

---

## 4. 相关模块

| 模块 | 关系 |
|------|------|
| `rclcpp::Node` | 参数接口挂在 Node 上 |
| `rcl_interfaces` | `SetParametersResult`、`ParameterDescriptor`、标准 param srv |
| `ros2 param` CLI | 与 Client 等价的命令行入口 |
| Topic/Service/Action | 通信数据通道；Param 是**配置面** |

命令行对照：

```bash
ros2 param list
ros2 param describe /param_server_node width
ros2 param get /param_server_node width
ros2 param set /param_server_node width 0.30
ros2 param dump /param_server_node
```

注意 CLI 里节点名常带 `/` 前缀：`/param_server_node`。

---

## 5. API 速查表

| API | 端 | 一句话 |
|-----|----|--------|
| `declare_parameter` | Server | 登记参数 |
| `get_parameter` / `as_*` | Server | 本地读值 |
| `add_on_set_parameters_callback` | Server | 设置闸门 |
| `SetParametersResult` | Server | 接受/拒绝 |
| `SyncParametersClient` | Client | 连远端节点 |
| `wait_for_service` | Client | 等参数服务 |
| `list_parameters` | Client | 列名 |
| `get_parameters` | Client | 批量读 |
| `set_parameters` | Client | 批量写 |
| `rclcpp::Parameter` | Client | 构造要写的项 |
| `create_wall_timer` / `spin` | Server | 打印与调度 |
