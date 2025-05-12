#!/bin/bash

# check parameters
if [ $# -lt 1 ]; then
    echo "fromat: $0 <.json file>"
    echo "example: $0 ./basic/0.json"
    exit 1
fi

# add include path and compile
g++ -std=c++11 -I./include -o json2verilog ./srcs/json2verilog.cpp

# check if compilation was successful
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

# execute
./json2verilog "$1"