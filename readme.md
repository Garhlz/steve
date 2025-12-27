# Steve's Adventure (史蒂夫的冒险)

这是一个基于 OpenGL (Core Profile) 开发的计算机图形学课程大作业项目。
项目构建了一个包含昼夜循环、动态光影和双人交互的虚拟场景，展示了从底层渲染管线到上层游戏逻辑的完整实现。

## ✨ 核心特性 (Key Features)

### 🖥️ 图形渲染

- **光照系统**：
    - 基于 **Blinn-Phong** 的光照模型，支持环境光、漫反射、镜面高光计算。
    - 支持多光源类型：**方向光**（太阳/月亮）、**点光源**（路灯）和聚光灯。
    - **材质系统**：支持 Diffuse（漫反射）和 Specular（高光）贴图，模拟不同材质的质感。
- **阴影映射 (Shadow Mapping)**：
    - 实现基于 **深度纹理 (Depth Map)** 的阴影生成，消除“彼得潘悬浮”现象。
    - 阴影中心跟随当前操控角色动态移动，优化阴影分辨率利用率。
- **动态环境系统**：
    - **昼夜循环 (Day/Night Cycle)**：按键一键切换，动态插值天空盒 (Cubemap)、光照色调及环境光强度。
    - **静态天体配置**：太阳与月亮的位置、大小及自发光强度与昼夜状态完全解耦管理。
- **后处理与色彩**：
    - **Gamma 校正** (Gamma 2.2)：采用线性工作流，输出色彩更真实，暗部细节更丰富。
- **层级建模与动画**：
    - 角色采用分层结构（Torso -> Head/Limb -> Weapon），支持层级变换。
    - 实现基于时间函数的**过程式动画**（行走摆臂、挥剑攻击）。

### 🎮 游戏玩法 & 物理

- **双人交互**：
    - **角色切换**：支持在两角色之间无缝切换控制权 (T键)。
    - **跟随模式 (Follow Mode)**：非操控角色自动寻路并跟随玩家移动。
    - **实体碰撞**：基于 AABB 的动态碰撞检测，角色之间无法穿模。
- **物理模拟**：
    - 简易模拟重力加速度、跳跃冲量及地面检测。
    - 支持与静态场景物体（如树木、岩石、路灯）的碰撞阻挡。
- **摄像机系统**：
    - **双视角切换**：支持 **第一人称 (FPS)** 沉浸视角与 **第三人称 (TPS)** 观察视角。
    - **轨道相机**：第三人称下支持鼠标环绕观察 (Orbit) 及滚轮缩放距离，且包含防抖动平滑处理。
- **UI 交互系统**：
    - 集成 **Dear ImGui**，提供主菜单、暂停菜单及实时 HUD（帧率显示）。
    - 可视化的操作按键说明与游戏状态控制。

### 🛠️ 架构设计

- **轻量化资源管线**：
    - 移除臃肿的 Assimp，全面迁移至 **TinyObjLoader** (Header-only)，大幅减小编译依赖。
    - 实现 **Auto-Triangulation** (自动三角化) 与无贴图模型的**材质烘焙**。
- **资源管理系统**：
    - 实现 `ResourceManager` 单例，统一管理 Mesh、Texture 等资源的加载与缓存，避免重复 I/O。
- **高内聚低耦合**：
    - **LightManager**：作为“单一数据源”统一管理所有光照状态与天体配置。
    - **Input Decoupling**：抽象 `SteveInput` 结构体，统一处理玩家输入与 AI 指令，实现逻辑复用。

------

## 🛠️ 技术栈 (Tech Stack)

* **语言**: C++17
* **构建**: CMake
* **图形库**: OpenGL 3.3+ (Core Profile)
* **窗口/输入**: GLFW
* **加载器**: GLAD
* **数学库**: GLM
* **模型加载**: TinyObjLoader
* **GUI**: Dear ImGui
* **图像加载**: stb_image

## 🎮 操作说明 (Controls)

| 按键              | 功能描述                                  |
| :---------------- | :---------------------------------------- |
| **W / A / S / D** | 移动角色 (第三人称) / 移动相机 (第一人称) |
| **Mouse**         | 旋转视角 / 调整朝向                       |
| **Scroll**        | 缩放相机距离 (第三人称)                   |
| **TAB**           | 切换 **第一人称 / 第三人称** 视角         |
| **T**             | 切换控制角色 (**Steve / Alex**)           |
| **F**             | 开启 / 关闭 **跟随模式**                  |
| **B**             | 切换 **白天 / 黑夜**                      |
| **Space**         | 跳跃                                      |
| **L-Click**       | 攻击 (挥剑)                               |
| **ESC**           | 暂停 / 呼出菜单                           |

## 🚀 构建与运行 (Build)

本项目支持通过 `vcpkg` 管理依赖。

### 环境要求
* C++ 编译器 (GCC/Clang/MSVC)
* CMake 3.17+
* vcpkg (推荐)

### 编译步骤

```bash
# 1. 创建构建目录
mkdir build && cd build

# 2. 生成工程 (假设使用了 vcpkg)
# 请将 <path_to_vcpkg> 替换为你的 vcpkg 实际安装路径
cmake .. -DCMAKE_TOOLCHAIN_FILE=<path_to_vcpkg>/scripts/buildsystems/vcpkg.cmake

# 3. 编译
cmake --build . 

# 4. 运行
./steve  # (Linux/Mac)
.\Debug\steve.exe # (Windows)
```


## 📂 项目结构 (Project Structure)
```Plaintext
.
├── assets/                     # 🎨 资源目录：游戏资产
│   ├── models/                 #    .obj 模型文件 (Steve, Alex, 场景物体等)
│   ├── shaders/                #    GLSL 着色器代码
│   │   ├── lighting_*.glsl     #      核心光照着色器 (Blinn-Phong + Shadow)
│   │   ├── shadow_depth_*.glsl #      阴影深度贴图生成
│   │   └── skybox_*.glsl       #      天空盒着色器
│   ├── textures/               #    纹理贴图 (漫反射, 高光, 天空盒)
│   └── steve.rc                #    Windows 资源文件 (图标配置)
│
├── include/
│   ├── Core/                   # ⚙️ 核心引擎层 (通用图形/物理功能)
│   │   ├── AABB.h              #      AABB 碰撞盒类 (物理碰撞检测基础)
│   │   ├── Camera.h            #      基础摄像机类 (View Matrix 计算)
│   │   ├── Shader.h            #      GLSL 编译与 Uniform 管理工具
│   │   ├── TriMesh.h           #      网格数据类 (封装 TinyObjLoader, VBO/VAO 管理)
│   │   ├── ResourceManager.h   #      资源管理器单例 (模型/纹理缓存池)
│   │   └── Skybox.h            #      天空盒渲染组件
│   │
│   └── Game/                   # 🎮 游戏逻辑层 (具体玩法实现)
│       ├── Game.h              #      游戏主循环与状态机管理
│       ├── Steve.h             #      角色类 (动画状态机, 物理运动, 骨骼层级)
│       ├── Scene.h             #      场景图管理 (静态物体渲染, 地图加载)
│       ├── LightManager.h      #      光照管理器 (昼夜循环逻辑, 阴影配置)
│       ├── CameraController.h  #      相机控制器 (第一/第三人称切换, 跟随逻辑)
│       └── UIManager.h         #      UI 界面管理 (基于 ImGui)
│
├── src/                        # 📝 源代码实现 (对应 include 中的头文件)
│   ├── Core/                   #      核心层实现
│   ├── Game/                   #      逻辑层实现
│   └── main.cpp                #      程序入口 (GLFW 窗口创建, 回调函数注册)
│
└── CMakeLists.txt              # 🏗️ CMake 构建脚本 (依赖管理与编译配置)
```

### 💡 结构说明
项目采用 "Engine-Game" 分层架构：

- Core 层：负责底层的图形渲染封装（Shader, Mesh）和数学物理基础（AABB），与具体游戏逻辑解耦，具有复用性。

- Game 层：调用 Core 提供的接口实现具体的游戏玩法（如 Steve 的行走逻辑、昼夜切换的具体参数、UI 菜单交互）。

- ResourceManager：作为连接两层的桥梁，负责高效地加载和分发资源，防止内存泄漏和重复加载。



## 📦 依赖安装指南 (vcpkg)

本项目推荐使用 vcpkg 包管理器来安装 C++ 依赖库。它能自动处理头文件路径和链接库，极大简化构建流程。
1. 安装 vcpkg

如果你还没有安装 vcpkg，请先克隆并引导它：

```Bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh  # Linux/macOS
.\bootstrap-vcpkg.bat # Windows
```

2. 安装项目依赖

在 vcpkg 目录下运行以下命令，安装本项目所需的所有库：

Windows (x64)
```PowerShell
./vcpkg install glfw3:x64-windows glm:x64-windows tinyobjloader:x64-windows imgui[core,glfw-binding,opengl3-binding]:x64-windows stb:x64-windows
```
Linux / macOS
```Bash
./vcpkg install glfw3 glm tinyobjloader imgui[core,glfw-binding,opengl3-binding] stb
```

依赖说明：

    glfw3: 用于创建窗口和处理输入。
    
    glm: OpenGL 数学库（向量、矩阵运算）。
    
    tinyobjloader: 轻量级 OBJ 模型加载库。
    
    imgui: 即时模式 GUI 库（需启用 glfw-binding 和 opengl3-binding 特性）。
    
    stb: 图像加载库 (stb_image)。

3. 构建项目

安装完成后，在 CMake 构建时指定 vcpkg 的工具链文件即可：
```Bash
# 假设 vcpkg 安装在用户根目录
cmake .. -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake
```

4. 打包
```PowerShell
mkdir build
cd build

# 指定 vcpkg 工具链 (路径根据你实际安装位置修改)
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build . --config Release
```