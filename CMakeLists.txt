# CMakeLists.txt fixed version
# CMake最低版本要求
cmake_minimum_required(VERSION 3.17)

# 项目名称
project(steve)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================
# 1. 依赖查找 (适配 vcpkg 和 System)
# ==========================================

find_package(glfw3 CONFIG REQUIRED)      # 使用 CONFIG 模式查找 vcpkg 提供的配置文件
find_package(glm CONFIG REQUIRED)        # 同上
find_package(OpenGL REQUIRED)            # OpenGL 通常仍然由系统提供
find_package(assimp CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
#find_package(stb CONFIG REQUIRED)
# ==========================================
# 2. 源代码管理 & 生成可执行文件
# ==========================================

# [修改] 定义资源文件变量
if(WIN32)
	set(app_icon_resource "${CMAKE_CURRENT_SOURCE_DIR}/steve.rc")
else()
	set(app_icon_resource "")
endif()

# A. 先扫描源文件
# 自动扫描 src 下所有的 .cpp 和 .c 文件
file(GLOB_RECURSE SOURCES
		"src/Core/*.cpp"
		"src/Game/*.cpp"
		"src/Vendor/*.c"
		"src/Vendor/*.cpp"
		"src/*.cpp"
		${app_icon_resource}
)



# B. 必须先创建可执行文件目标，后面才能对它进行设置
add_executable(${PROJECT_NAME} ${SOURCES})

# ==========================================
# 3. 配置目标属性 (Include & Link)
# ==========================================

# target 已经存在了，可以设置包含目录了
target_include_directories(${PROJECT_NAME} PRIVATE
		include/Core
		include/Game
		include/Vendor
		include
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
		glfw
		glm::glm
		OpenGL::GL
		assimp::assimp
		imgui::imgui
#		stb::stb
		${CMAKE_DL_LIBS} # Linux 下需要的动态加载库
)

# ==========================================
# 4. 资源文件拷贝
# ==========================================
# 这样你直接点 CLion 的运行按钮就能读到模型，不用手动 cd
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
)

# ==========================================
# 5. 解决中文乱码 (Windows 特供)
# ==========================================
if(MSVC)
	add_compile_options("/utf-8") # 强制 MSVC 使用 UTF-8，避免 GBK 乱码
endif()