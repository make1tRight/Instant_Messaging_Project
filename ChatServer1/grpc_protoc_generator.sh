#!/bin/bash
set -e  # 遇到错误立即退出

GRPC_CPP_PLUGIN="/usr/local/bin/grpc_cpp_plugin"
PROTO_FILE="message.proto"

# 检测 protoc 是否安装
if ! command -v protoc &> /dev/null; then
    echo "Error: protoc is not installed."
    exit 1
fi

# 检测 gRPC C++ 插件是否存在
if [ ! -f "$GRPC_CPP_PLUGIN" ]; then
    echo "Error: gRPC plugin not found at $GRPC_CPP_PLUGIN"
    exit 1
fi

echo Generating gRPC code.
protoc -I=. --grpc_out=. --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${PROTO_FILE}

echo Generating CXX code.
protoc --cpp_out=. ${PROTO_FILE}

echo Done.