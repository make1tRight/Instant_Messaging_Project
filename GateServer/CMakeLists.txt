cmake_minimum_required(VERSION 3.12)
project(GateServer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

# 添加可执行文件和源文件
file(GLOB
    SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_executable(GateServer ${SOURCES})

# 包含头文件路径（包括其他目录）
target_include_directories(
    GateServer
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)