# ROS2 动作通信学习 Demo

对应教程：**2.4.3 动作通信（C++）**

## 功能说明

客户端提交整数 `num`，服务端计算 `1 + 2 + ... + num`：

- **Goal**：目标整数 `num`
- **Feedback**：连续进度百分比
- **Result**：最终累加和

## 包结构

```
ws01_plumbing/src/
├── base_interfaces_demo/     # 自定义接口
│   └── action/Progress.action
└── cpp03_action/             # 动作通信示例
    ├── src/demo01_action_server.cpp
    └── src/demo02_action_client.cpp
```

## 编译

```bash
cd ~/learning/ws01_plumbing
source /opt/ros/jazzy/setup.bash
colcon build --packages-select base_interfaces_demo cpp03_action
source install/setup.bash
```

## 运行

终端 1 —— 启动动作服务端：

```bash
ros2 run cpp03_action demo01_action_server
```

终端 2 —— 启动动作客户端（参数为累加上限，默认 10）：

```bash
ros2 run cpp03_action demo02_action_client 10
```
