# zero-infra

## 分支说明

| 分支 | 内容 |
|------|------|
| `main` | 同学公共文档资源 |
| `public` | ros2-doc 课件资料 |
| `wangmaosen` | wangmaosen 个人学习工程（Topic / Service / Action demos） |

**Agent / 推送规范见根目录 [`AGENTS.md`](./AGENTS.md)**（禁止把 build 产物和二进制推进仓库）。

## 本分支（wangmaosen）

ROS 2 通信学习工程在 `ws01_plumbing/`：

```bash
cd ws01_plumbing
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install
source install/setup.bash
```
