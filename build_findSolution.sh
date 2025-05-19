#!/bin/bash

# 目录定义
SRC_DIR="./srcs"
INCLUDE_DIR="./include"
RUN_DIR="./run_dir"
CUDD_DIR="./cudd"

# 可执行文件名
EXEC_NAME="solution_gen"

# 确保运行目录存在
mkdir -p "$RUN_DIR"

# 检查 CUDD 库是否已编译
if [ ! -f "$CUDD_DIR/cudd/.libs/libcudd.a" ] && [ ! -f "$CUDD_DIR/cudd/.libs/libcudd.so" ]; then
    echo "CUDD 库未找到。请先编译 CUDD 库:"
    echo "cd $CUDD_DIR && ./configure && make && cd .."
    exit 1
fi

# 更新 CUDD 库位置
CUDD_LIB="$CUDD_DIR/cudd/.libs"
CUDD_INCLUDE="$CUDD_DIR/cudd"

# 编译标志
CXX_FLAGS="-std=c++17 -O2 -Wall"
INCLUDE_FLAGS="-I$INCLUDE_DIR -I$SRC_DIR -I$CUDD_INCLUDE"
LINK_FLAGS="-L$CUDD_LIB -lcudd -lm -lquadmath"

# 编译命令
COMPILE_CMD="g++ $CXX_FLAGS $INCLUDE_FLAGS"

# 编译 solution_gen.cpp
echo "编译 solution_gen.cpp..."
$COMPILE_CMD "$SRC_DIR/solution_gen.cpp" -o "$RUN_DIR/$EXEC_NAME" $LINK_FLAGS

if [ $? -eq 0 ]; then
    echo "编译成功！可执行文件已创建: $RUN_DIR/$EXEC_NAME"
    echo "使用方法: $RUN_DIR/$EXEC_NAME <aag_file> <random_seed> <solution_num> <output_file> [<constraint_file>]"
    
    # 如果有命令行参数，则运行程序
    if [ $# -ge 4 ]; then
        AAG_FILE="$1"
        RANDOM_SEED="$2"
        SOLUTION_NUM="$3"
        OUTPUT_FILE="$4"
        # 第五个参数作为约束文件（如果提供）
        CONSTRAINT_FILE="${5:-constraint.json}"  # 如果未提供，默认为constraint.json
        
        echo "正在运行 $EXEC_NAME..."
        "$RUN_DIR/$EXEC_NAME" "$AAG_FILE" "$RANDOM_SEED" "$SOLUTION_NUM" "$OUTPUT_FILE"
        
        if [ $? -eq 0 ]; then
            echo "解决方案已保存到: $OUTPUT_FILE"
            
            # 添加验证解决方案的功能，无额外提示
            if [ -f "./evalcns" ] && [ -f "$CONSTRAINT_FILE" ]; then
                ./evalcns -p "$CONSTRAINT_FILE" -a "$OUTPUT_FILE"
            fi
        else
            echo "程序运行时出错。"
            exit 1
        fi
    else
        echo "提示：您可以通过运行 '$0 <aag_file> <random_seed> <solution_num> <output_file> [<constraint_file>]' 来直接编译并执行程序"
    fi
else
    echo "编译 solution_gen.cpp 失败"
    exit 1
fi