cmake_minimum_required(VERSION 3.16) 
project(IndustrialCurveEditor LANGUAGES C CXX RC)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


set(GLFW_LOCAL_PATH "${CMAKE_SOURCE_DIR}/thirdparty/glfw-src")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

if(EXISTS ${GLFW_LOCAL_PATH}/CMakeLists.txt)
    add_subdirectory(${GLFW_LOCAL_PATH} "${CMAKE_BINARY_DIR}/glfw-build" EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "GLFW source not found at ${GLFW_LOCAL_PATH}")
endif()


set(IMGUI_LOCAL_PATH "${CMAKE_SOURCE_DIR}/thirdparty/imgui-src")

add_library(imgui STATIC
    ${IMGUI_LOCAL_PATH}/imgui.cpp
    ${IMGUI_LOCAL_PATH}/imgui_draw.cpp
    ${IMGUI_LOCAL_PATH}/imgui_tables.cpp
    ${IMGUI_LOCAL_PATH}/imgui_widgets.cpp
    ${IMGUI_LOCAL_PATH}/backends/imgui_impl_glfw.cpp
    ${IMGUI_LOCAL_PATH}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC 
    ${IMGUI_LOCAL_PATH} 
    ${IMGUI_LOCAL_PATH}/backends
)

find_package(OpenGL REQUIRED)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)


file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.h"
    "include/*.h"
)


add_executable(${PROJECT_NAME} ${SRC_FILES} src/resources.rc)


target_link_libraries(${PROJECT_NAME} PRIVATE imgui user32)


target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)


if(MSVC)

    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W4         
        /MP        
        /utf-8      
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    
elseif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # MinGW (g++) / Clang 专用配置
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall -Wextra -Wpedantic 
    )
    
    if(MINGW)
        target_link_options(${PROJECT_NAME} PRIVATE 
            -static-libgcc 
            -static-libstdc++
            -static 
        )
    endif()
endif()
