#!/bin/bash
BUILD_DIR="build"
# 检查并创建构建目录
if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi
mkdir "$BUILD_DIR"

cd "$BUILD_DIR"

cmake ..
make -j 8
cp ../config.ini .
