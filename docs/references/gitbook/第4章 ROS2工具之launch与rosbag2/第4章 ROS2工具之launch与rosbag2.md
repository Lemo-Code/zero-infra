# 第4章 ROS2工具之launch与rosbag2

本章开始，将正式进入ROS2工具部分内容的介绍。之前我们已经多次使用到launch文件了，本章将系统性的介绍ROS2中的launch实现，除此之外还将介绍，ROS2中极其实用的工具——rosbag2，通过该工具可以实现话题消息的录制与回放。

#### 本章概览

| **章节** | **学习内容** | **学习收获** |
| --- | --- | --- |
| 4.1 启动文件 launch 简介 | launch文件的概念、作用、应用场景，以及不同类型的launch文件的基本实用流程。 | 能够明确什么情况下使用launch文件，以及如何编写不同格式的launch文件。 |
| 4.2 launch之Python实现 | Python格式的launch文件实现语法。 | 可以使用Python编写复杂的launch文件。 |
| 4.3 launch之xml、yaml实现 | xml、yaml格式的launch文件实现语法。 | 可以使用xml、yaml编写复杂的launch文件。 |
| 4.4 录制回放工具——rosbag2 | 主要介绍rosbag2的概念、作用、应用场景，以及如何以命令或编码的方式使用rosbag2。 | 能够了解rosbag2的理论知识，并且可以通过命令或编码实现话题消息的录制与回放。 |
| 4.5 本章小结 | 知识点汇总。 | 知识点回顾。 |
