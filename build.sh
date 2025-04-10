#!/bin/bash

BUILD_DIR="build"

# 检查并创建构建目录
if [ -d "$BUILD_DIR" ]; then
    echo "delete build directory"
    rm -rf "$BUILD_DIR"
fi
echo "make build directory"
mkdir "$BUILD_DIR"

# 进入构建目录
cd "$BUILD_DIR"

cmake ..
make -j 8

# 拷贝配置文件
cp ../ChatServer1/config.ini ChatServer1/
cp ../ChatServer2/config.ini ChatServer2/
cp ../GateServer/config.ini GateServer/
cp ../StatusServer/config.ini StatusServer/

