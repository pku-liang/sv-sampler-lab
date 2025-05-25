#!/bin/bash

set -e  # 遇到错误时停止执行

# 默认参数
AAG_FILE="./run_dir/test.aag"
SEED=42
NUM_SOLUTIONS=5
OUTPUT_FILE="./run_dir/solutions.json"

# 解析命令行参数
if [ $# -ge 1 ]; then
    AAG_FILE="$1"
fi

if [ $# -ge 2 ]; then
    SEED="$2"
fi

if [ $# -ge 3 ]; then
    NUM_SOLUTIONS="$3"
fi

if [ $# -ge 4 ]; then
    OUTPUT_FILE="$4"
fi

# 确保运行目录存在
mkdir -p ./run_dir

# 输出参数信息
echo "BDD Solver Parameters:"
echo "---------------------"
echo "AAG File:       $AAG_FILE"
echo "Random Seed:    $SEED"
echo "Num Solutions:  $NUM_SOLUTIONS"
echo "Output File:    $OUTPUT_FILE"
echo "---------------------"

# 检查源代码文件是否存在
if [ ! -f "srcs/random_solutions.cpp" ]; then
    echo "错误：源文件 srcs/random_solutions.cpp 不存在"
    exit 1
fi

if [ ! -f "srcs/aag2BDD.cpp" ]; then
    echo "错误：源文件 srcs/aag2BDD.cpp 不存在"
    exit 1
fi

# 确保必要的头文件已包含在random_solutions.cpp中
if ! grep -q "#include <set>" srcs/random_solutions.cpp; then
    echo "添加缺失的 #include <set> 到 random_solutions.cpp"
    sed -i '1s/^/#include <set>\n/' srcs/random_solutions.cpp
fi

if ! grep -q "#include <functional>" srcs/random_solutions.cpp; then
    echo "添加缺失的 #include <functional> 到 random_solutions.cpp"
    sed -i '1s/^/#include <functional>\n/' srcs/random_solutions.cpp
fi

if ! grep -q "#include <algorithm>" srcs/random_solutions.cpp; then
    echo "添加缺失的 #include <algorithm> 到 random_solutions.cpp"
    sed -i '1s/^/#include <algorithm>\n/' srcs/random_solutions.cpp
fi

# 编译程序
echo "编译 BDD 求解器..."

# 编译 random_solutions.cpp
g++ -std=c++17 -O3 -Wall -I./include -I./srcs -I./cudd/cudd -L./cudd/cudd/.libs \
    srcs/random_solutions.cpp \
    -o run_dir/find_solution \
    -lcudd -lm

# 检查编译是否成功
if [ $? -ne 0 ]; then
    echo "编译失败！"
    exit 1
fi

echo "编译成功！"

# 显示使用说明
echo "使用方法: ./run_dir/find_solution <aag_file> <random_seed> <num_solutions> [output_file]"
echo "现在开始执行程序..."

# 执行程序
echo "执行 BDD 求解器..."
./run_dir/find_solution "$AAG_FILE" "$SEED" "$NUM_SOLUTIONS" "$OUTPUT_FILE"

# 检查执行是否成功
if [ $? -ne 0 ]; then
    echo "执行失败！"
    exit 1
fi

echo "执行成功！结果已保存到 $OUTPUT_FILE"

# 显示找到的解的数量
if [ -f "$OUTPUT_FILE" ]; then
    NUM_FOUND=$(grep -c "\[" "$OUTPUT_FILE" | awk '{print ($1-1)/2}')
    echo "找到 $NUM_FOUND 个解决方案"
    
    # 显示结果文件前几行
    echo "结果示例（前10行）："
    head -n 10 "$OUTPUT_FILE"
else
    echo "警告：找不到输出文件 $OUTPUT_FILE"
fi

exit 0