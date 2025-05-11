#!/bin/bash

# 检查参数
if [ $# -lt 1 ]; then
    echo "fromat: $0 <.json file>"
    echo "example: $0 ./basic/0.json"
    exit 1
fi

# 1. 编译，添加头文件搜索路径
g++ -std=c++11 -I./include -o json2verilog ./srcs/json2verilog.cpp

# 2. 检查编译是否成功
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

# 3. 运行，使用用户指定的json文件路径
./json2verilog "$1"