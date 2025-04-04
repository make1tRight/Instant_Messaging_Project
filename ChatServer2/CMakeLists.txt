cmake_minimum_required(VERSION 3.12)
project(ChatServer2)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)
set(PROJECT_NAME ChatServer2)

find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(utf8_range REQUIRED)

# 添加可执行文件和源文件
file(GLOB
    SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
)

add_executable(${PROJECT_NAME} ${SOURCES})

# 包含头文件路径（包括其他目录）
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
)

# 链接相关库
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    jsoncpp
    ${Boost_LIBRARIES}
    ${Protobuf_LIBRARIES}
    gRPC::grpc++
    hiredis
    mysqlcppconn
    utf8_range::utf8_range
    utf8_range::utf8_validity
)
