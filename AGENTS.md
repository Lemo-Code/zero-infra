# AGENTS.md — 本仓库 Agent / 协作者须知

> 适用范围：仓库 [Lemo-Code/zero-infra](https://github.com/Lemo-Code/zero-infra)  
> 个人学习工程默认在 **`wms`** 分支维护。  
> Cursor Agent、人工提交前都先读本文。

---

## 1. 分支约定

| 分支 | 谁维护 | 放什么 | 禁止 |
|------|--------|--------|------|
| `main` | 同学公共资源 | gitbook / docs 等 | 不要把个人工程、build 产物推这里 |
| `public` | 课件资料 | `ros2-doc` 类文档资源 | 不要和 `wms` 学习代码混推 |
| **`wms`** | **本人默认工作分支** | `ws01_plumbing` 源码与学习笔记 | 不要推二进制 / 编译产物 |

本地日常：

```bash
cd ~/learning/zero-infra
git checkout wms
# 改代码 → add → commit → push
git push origin wms
```

- 新建功能、修 bug、补文档：**只提交到 `wms`**
- 不要 `git push --force` 到 `main` / `public`
- 不要把同学的 `main` 内容 force 覆盖掉

---

## 2. 推送规范（必读）

### 2.1 推之前自检

```bash
git status
git diff --staged
git check-ignore -v <可疑路径>   # 确认已被忽略
du -sh $(git diff --staged --name-only) 2>/dev/null | sort -h
```

若 staged 里出现单个文件 **> 1MB**，先停下来问：这是不是二进制 / 编译产物？

### 2.2 绝对不要提交的内容

| 类别 | 示例 | 说明 |
|------|------|------|
| colcon 产物 | `build/` `install/` `log/` | 可本地重建 |
| 编译中间文件 | `*.o` `*.a` `*.so` `*.dylib` | 平台相关 |
| 可执行文件 | `install/.../lib/**` 下无后缀二进制 | 体积大且无意义 |
| IDE / LSP 缓存 | `.cache/` `compile_commands.json` | 本地生成 |
| 密钥 | `.env` `*credentials*` `id_rsa` token | **严禁** |
| 临时文件 | `*.swp` `*~` `.DS_Store` | 噪音 |

### 2.3 可以提交的内容

- 源码：`*.cpp` `*.hpp` `CMakeLists.txt` `package.xml`
- 接口：`*.msg` `*.srv` `*.action`
- 学习文档：`*.md`（调用过程、API 说明等）
- 仓库配置：`.gitignore` `AGENTS.md` `.clangd`（不含缓存）
- 有意纳入版本管理的小体积资源（需在 commit message 里说明）

### 2.4 Agent 提交检查清单

在执行 `git add` / `git commit` / `git push` 前，Agent **必须**：

1. 确认当前分支是 `wms`（除非用户明确指定其它分支）
2. 用 `git status` / `git diff` 确认没有 `build/`、`install/`、`log/`、`.so`、`.o` 等
3. 不使用 `git add .` / `git add -A` 除非刚检查过忽略规则有效；优先按路径精确 add
4. 不提交密钥、token、密码
5. push 目标为 `origin wms`，不默认推 `main`

若用户说「提交全部」，仍要先过滤二进制与构建目录，并在回复里说明跳过了哪些路径。

---

## 3. 工程布局

```text
zero-infra/                 ← git 根（wms 分支）
├── AGENTS.md               ← 本文
├── .gitignore
├── README.md
├── docs/                   ← 来自 main 的公共文档（勿随意大改）
└── ws01_plumbing/          ← ROS 2 学习工作空间
    ├── src/
    │   ├── base_interfaces_demo/
    │   ├── cpp01_topic/
    │   ├── cpp02_service/
    │   ├── cpp03_action/
    │   └── cpp04_param/
    ├── build/              ← 忽略，本地 colcon 生成
    ├── install/            ← 忽略
    └── log/                ← 忽略
```

构建（本地，不入库）：

```bash
cd ws01_plumbing
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install
source install/setup.bash
```

换机器 / 换路径后必须重新 `colcon build`，不要指望把 `install/` 推上去。

---

## 4. 文档维护约定

每个通信模块保持两类文档（已有则更新，没有则补）：

| 文档 | 作用 |
|------|------|
| `*调用过程详解.md` | 从 0 到 1 的完整时序（含取消等） |
| `*_API与模块详解.md` | 本模块用到的 API / 依赖 / 命令行 |

接口包：`base_interfaces_demo/接口模块详解.md`  
参数包：`cpp04_param/Param调用过程详解.md`、`Param_API与模块详解.md`

改 API 行为或回调流程时：**同步改对应 md**，避免文档和代码脱节。

---

## 5. `.gitignore` 与二进制防护

根目录 `.gitignore` 已忽略常见构建产物。若新增工具链产生新产物（例如 `*.pyc`、覆盖率报告、bag 包），**先改 `.gitignore`，再提交源码**。

可疑扩展名（默认不要 add）：

```text
*.o *.a *.so *.so.* *.dylib *.dll *.exe
*.bin *.out *.elf
*.bag *.mcap
*.tar *.tar.gz *.zip *.7z（除非用户明确要求且说明用途）
```

大文件优先用 Git LFS 或外链，不要直接塞进普通 git 提交。

---

## 6. 远程协作提醒

- 远程：`git@github.com:Lemo-Code/zero-infra.git`（SSH）
- 只读同学资源看 `main` / `public`，个人开发推 `wms`
- 误推了二进制：立刻从后续提交中删掉并更新 `.gitignore`；若已进历史且体积很大，再商量是否改写历史（需用户明确同意）

---

## 7. 一句话

> **只在 `wms` 上推进学习工程；只提交源码与文档；构建产物和二进制一律不进仓库。**
