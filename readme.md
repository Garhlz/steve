这是szu计算机图形学的课程实验。以下描述了我的实验环境和配置方法
实验环境：
```
OS: Arch Linux x86_64
Host: ThinkPad T480s
CPU: Intel(R) Core(TM) i5-8350U (8) @ 3.60 GHz 
GPU: Intel UHD Graphics 620 @ 1.10 GHz [Integrated]
包管理工具：pacman
依赖：OpenGL, GLFW, GLAD, GLM
gcc (GCC) 15.2.1 20250813
cmake 4.1.1-1 
glfw 1:3.4-1 
glm 1.0.1-1 
mesa 1:25.2.3-2
```
---

在官方的脚手架代码之上，对于我的个人项目环境，需要修改的配置如下：

1. 从include中把两个头文件的文件夹夹复制过来

2. 把glad.c切换为自己的新版本的
   
3. 把InitShader.cpp切换为新版本的文件

4. 将CMakeLists.txt换为自己的版本。如果有官方的新文件，需要添加到add_executable中
   
5. main.c的main()中在init（）函数之后加入代码重置窗口位置：
```cpp
int width, height;
glfwGetFramebufferSize(window, &width, &height);
glViewport(0, 0, width, height);	
```

---
## arch + pacman 版本
编译运行方法：
- 构建：
```bash
mkdir build; cd build; cmake ..; make
```

- 运行
```bash
cd..; ./build/main
```

- 修改之后运行：
```bash
cd build;make;cd ..;./build/main
```

---

## vcpkg 版本
- 使用vcpkg进行构建：
```bash
mkdir build; cd build;cmake .. -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake;make
```

- 运行
```bash
cd ..; ./build/main
```

- 修改之后运行：
```bash
cd build;make;cd ..;./build/main
```

## windows + vcpkg 版本
需要修改编码方式为GBK
- 使用vcpkg进行构建：
```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

- 运行
```bash
cd ..
.\build\Debug\main.exe
```

- 修改之后运行：
```bash
cd build
cmake --build .
cd ..
.\build\Debug\main.exe
```

- 发布
```bash
cmake --build . --config Release
```