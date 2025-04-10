cmake_minimum_required(VERSION 3.16)

project(Feynman)

# 使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加子目录
add_subdirectory(ChatServer1)
add_subdirectory(ChatServer2)
add_subdirectory(GateServer)
add_subdirectory(StatusServer)