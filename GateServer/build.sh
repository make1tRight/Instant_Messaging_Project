#!/bin/bash
mkdir build && cd build
cmake ..
make

# 2. 启动
./GateServer
