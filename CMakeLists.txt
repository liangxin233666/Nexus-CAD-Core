cmake_minimum_required(VERSION 3.16) # 推荐 3.16+ 以获得更好的多平台支持
project(IndustrialCurveEditor LANGUAGES C CXX)

# --- 1. 全局标准设置 ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录（让生成的 exe 在根目录的 bin 文件夹里，方便找资源文件）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- 2. 外部依赖 (GLFW) ---
set(GLFW_LOCAL_PATH "${CMAKE_SOURCE_DIR}/thirdparty/glfw-src")
# 优化：禁止编译 GLFW 的测试程序、文档和例子，大幅提升初次编译速度
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

if(EXISTS ${GLFW_LOCAL_PATH}/CMakeLists.txt)
    add_subdirectory(${GLFW_LOCAL_PATH} "${CMAKE_BINARY_DIR}/glfw-build" EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "GLFW source not found at ${GLFW_LOCAL_PATH}")
endif()

# --- 3. 外部依赖 (ImGui) ---
set(IMGUI_LOCAL_PATH "${CMAKE_SOURCE_DIR}/thirdparty/imgui-src")
# 将 ImGui 定义为一个独立的静态库
add_library(imgui STATIC
    ${IMGUI_LOCAL_PATH}/imgui.cpp
    ${IMGUI_LOCAL_PATH}/imgui_draw.cpp
    ${IMGUI_LOCAL_PATH}/imgui_tables.cpp
    ${IMGUI_LOCAL_PATH}/imgui_widgets.cpp
    ${IMGUI_LOCAL_PATH}/backends/imgui_impl_glfw.cpp
    ${IMGUI_LOCAL_PATH}/backends/imgui_impl_opengl3.cpp
)
# ImGui 库的包含路径
target_include_directories(imgui PUBLIC 
    ${IMGUI_LOCAL_PATH} 
    ${IMGUI_LOCAL_PATH}/backends
)
# 寻找 OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# --- 4. 项目源码管理 ---
# 使用 CONFIGURE_DEPENDS，这样你增加文件后，IDE 通常会自动重载 CMake
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.h"
    "include/*.h"
)

# --- 5. 创建主程序 ---
add_executable(${PROJECT_NAME} ${SRC_FILES})

# 链接刚才定义好的 imgui 模块和 glfw
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)

# 设置包含路径
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

# --- 6. 编译器差异化配置 (最优配置) ---
if(MSVC)
    # Visual Studio 专用配置
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W4          # 高警告等级
        /MP          # 开启多核并行编译
        /utf-8       # 强制 UTF-8 编码
    )
    # 解决 VS 的执行路径问题：将工作目录设为项目根目录（方便读取图片/配置）
    set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    
elseif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # MinGW (g++) / Clang 专用配置
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall -Wextra -Wpedantic 
    )
    
    if(MINGW)
        # 静态链接标准库，这样生成的 exe 拷给别人也能直接跑，不需要带一堆 dll
        target_link_options(${PROJECT_NAME} PRIVATE 
            -static-libgcc 
            -static-libstdc++
            -static # 视情况开启：如果希望彻底摆脱所有 dll 依赖
        )
    endif()
endif()