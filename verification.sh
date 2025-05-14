#!/bin/bash

# 目录设置
SRC_DIR="./srcs"
INCLUDE_DIR="./include"
RUN_DIR="./run_dir"
CUDD_DIR="./cudd"

# 可执行文件名称
EXEC_NAME="verification"

# 确保运行目录存在
echo "创建运行目录(如果不存在)..."
mkdir -p $RUN_DIR

# 检查 CUDD 库是否已编译
if [ ! -f "$CUDD_DIR/cudd/.libs/libcudd.a" ] && [ ! -f "$CUDD_DIR/cudd/.libs/libcudd.so" ]; then
    echo "未找到 CUDD 库。请先编译它:"
    echo "cd $CUDD_DIR && ./configure && make && cd .."
    exit 1
fi

# 更新 CUDD 库的位置
CUDD_LIB="$CUDD_DIR/cudd/.libs"
CUDD_INCLUDE="$CUDD_DIR/cudd"

# 编译参数
CXX_FLAGS="-std=c++17 -O2 -Wall"
INCLUDE_FLAGS="-I$INCLUDE_DIR -I$SRC_DIR -I$CUDD_INCLUDE"  
LINK_FLAGS="-L$CUDD_LIB -lcudd -lm"  

# 编译命令
COMPILE_CMD="g++ $CXX_FLAGS $INCLUDE_FLAGS"

# 记录编译步骤
echo "编译 verification.cpp..."
$COMPILE_CMD $SRC_DIR/verification.cpp -o $RUN_DIR/$EXEC_NAME $LINK_FLAGS

if [ $? -eq 0 ]; then
    echo "编译成功! 可执行文件已创建: $RUN_DIR/$EXEC_NAME"
    echo "用法: $RUN_DIR/$EXEC_NAME <aag文件路径>"
    
    # 可选: 如果提供了命令行参数，则直接运行验证程序
    if [ "$1" != "" ]; then
        echo "正在运行验证程序..."
        $RUN_DIR/$EXEC_NAME "$1"
    fi
else
    echo "编译 verification.cpp 失败"
    exit 1
fi