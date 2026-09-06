# 第 04 节 · 参数服务 API 与模块详解

> 配套：[`Param调用过程详解.md`](./Param调用过程详解.md)  
> 包：`cpp04_param`  
> 面向初学者：按「增删改查 + 客户端」索引 API

---

## 1. 包结构

```text
cpp04_param/
├── package.xml / CMakeLists.txt
├── Param调用过程详解.md      ← 流程与概念（先读）
├── Param_API与模块详解.md    ← 本文
└── src/
    ├── demo00_param.cpp           入门：单节点增删改查
    ├── demo01_param_server.cpp    服务端：四函数 + spin
    └── demo02_param_client.cpp    客户端：远程 list/get/set
```

依赖：

| 依赖 | 用途 |
|------|------|
| `rclcpp` | Node、参数 API、SyncParametersClient |
| `rcl_interfaces` | `SetParametersResult` 等 |

```cpp
#include "rclcpp/rclcpp.hpp"
#include "rcl_interfaces/msg/set_parameters_result.hpp"
```

---

## 2. 增删改查 API

### 2.0 构造选项

```cpp
rclcpp::NodeOptions()
  .allow_undeclared_parameters(true);
```

| API | 含义 |
|-----|------|
| `allow_undeclared_parameters(true)` | 未 declare 也可 set（自动声明） |
| 默认 false | 必须先 declare |

旧课件 `NodeOptions().all()` → 请改成上面写法（Jazzy）。

### 2.1 增 —— `declare_parameter`

```cpp
this->declare_parameter<std::string>("car_name", "turtle");
this->declare_parameter<double>("width", 0.15);
this->declare_parameter<bool>("tmp_flag", true);
```

| 点 | 说明 |
|----|------|
| 作用 | 登记参数名、类型、默认值 |
| 模板 | 指定 C++ 类型 |
| 不声明 | `allow_undeclared=false` 时外部很难正确读写 |

### 2.2 查 —— `get_parameter` / `has_parameter` / `get_parameters`

```cpp
auto p = this->get_parameter("car_name");
std::string name = p.as_string();

double width = 0.0;
this->get_parameter("width", width);

bool ok = this->has_parameter("tmp_flag");
auto many = this->get_parameters({"car_name", "width", "length"});
```

| 方法 | 类型 |
|------|------|
| `as_bool()` | bool |
| `as_int()` | int64 |
| `as_double()` | double |
| `as_string()` | string |
| `value_to_string()` | 打印友好 |
| `get_type_name()` | 类型名字符串 |

远端查：

```cpp
client->list_parameters({}, 0);
client->get_parameters({"car_name", "width"});
```

### 2.3 改 —— `set_parameter` / `set_parameters`

```cpp
// 本节点
this->set_parameter(rclcpp::Parameter("car_name", "turtle1"));
this->set_parameters({
  rclcpp::Parameter("width", 0.20),
  rclcpp::Parameter("length", 0.45),
});

// 远端客户端
client->set_parameters({rclcpp::Parameter("width", 0.30)});
```

远端改会先走服务端：

```cpp
add_on_set_parameters_callback(...)
→ SetParametersResult { successful, reason }
```

| 字段 | 含义 |
|------|------|
| `successful=true` | 接受修改 |
| `successful=false` | 拒绝，值不变 |
| `reason` | 拒绝原因 |

### 2.4 删 —— `undeclare_parameter`

```cpp
this->undeclare_parameter("tmp_flag");
// 之后 has_parameter("tmp_flag") == false
```

本节客户端以查/改为主；删除在服务端演示即可。

---

## 3. 客户端 API

### 3.1 `SyncParametersClient`

```cpp
auto client = std::make_shared<rclcpp::SyncParametersClient>(
  this, "param_server_node");  // 远端节点名！
```

| 方法 | 作用 |
|------|------|
| `wait_for_service(timeout)` | 等参数服务就绪 |
| `list_parameters(prefixes, depth)` | 列名 |
| `get_parameters(names)` | 批量读 |
| `set_parameters(params)` | 批量写，返回每条结果 |

同步客户端：调用时阻塞等结果，适合学习。  
还有 `AsyncParametersClient`（本节不展开）。

### 3.2 构造要写入的参数

```cpp
rclcpp::Parameter("car_name", std::string("turtle2"));
rclcpp::Parameter("width", 0.30);
```

---

## 4. 命令行对照（一定要会）

```bash
ros2 param list
ros2 param describe /param_server_node width
ros2 param get /param_server_node width
ros2 param set /param_server_node width 0.30
ros2 param dump /param_server_node
```

CLI 节点名常带 `/`：`/param_server_node`。

---

## 5. 并发（初学者够用版）

| 场景 | 结论 |
|------|------|
| 单线程 `spin`（demo01） | 参数回调串行，类似 Redis，内存竞态压力小 |
| 多个 client 同时 set | last-write-wins，没有分布式锁 |
| 多线程 Executor | 自有缓存要同步，或读时再 `get_parameter` |

`get` → 本地算 → `set` **不是原子事务**，中间别人可能插队写入。

---

## 6. 相关模块

| 模块 | 关系 |
|------|------|
| `rclcpp::Node` | 参数挂在 Node 上 |
| `rcl_interfaces` | 结果消息、描述符、标准 param 服务 |
| `cpp02_service` | 自定义一问一答；Param 是配置面 |
| `ros2 param` | 命令行版参数客户端 |

---

## 7. API 速查表

| API | 角色 | 一句话 |
|-----|------|--------|
| `NodeOptions::allow_undeclared_parameters` | Server | 是否允许未声明就 set |
| `declare_parameter` | Server | **增** |
| `get_parameter` / `has_parameter` / `get_parameters` | Server | **查** |
| `set_parameter` / `set_parameters` | Server | **改**（本地） |
| `undeclare_parameter` | Server | **删** |
| `add_on_set_parameters_callback` | Server | 远端改的闸门 |
| `SyncParametersClient` | Client | 连接远端节点 |
| `wait_for_service` | Client | 等待 |
| `list_parameters` | Client | 远程查列表 |
| `get_parameters` | Client | 远程查值 |
| `set_parameters` | Client | 远程改 |
| `rclcpp::spin` | Server | 常驻提供服务 |

---

## 8. 推荐阅读顺序

1. 本文第 2 节（增删改查 API）  
2. 打开 `demo00_param.cpp` 对照跑一遍  
3. 打开 `demo01_param_server.cpp` 看四函数 + 回调  
4. 打开 `demo02_param_client.cpp` + 调用过程文档第 5 节  
5. 用 `ros2 param` 自己改服务端参数  
