#!/bin/bash

# 启动各个服务器
echo "Starting ChatServer1."
./build/ChatServer1/ChatServer1 &

echo "Starting ChatServer2."
./build/ChatServer2/ChatServer2 &

echo "Starting GateServer."
./build/GateServer/GateServer &

echo "Starting StatusServer."
./build/StatusServer/StatusServer &

echo "Starting VarifyServer."
cd VarifyServer && npm run server

cd ..

# 等待所有后台程序
wait