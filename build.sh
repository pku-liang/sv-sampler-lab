#!/bin/bash
# 文件路径: /root/finalProject/COSMOS-BDD-solver/build.sh

set -e  # 遇错退出
run_dir="./run_dir"
mkdir -p "$run_dir"

# 检查参数
if [ "$#" -lt 1 ]; then
    echo "用法: $0 <约束.json> [<seed>] [<num>]"
    exit 1
fi

# 参数
constraint_file="$1"
solution_num="$2"
run_dir="$3"
seed="${4:-42}"  # 默认种子为42

basename=$(basename "$constraint_file" .json)

echo "===== Step 1: 编译 json2verilog ====="
g++ -std=c++11 -I./include -o "$run_dir/json2verilog" ./srcs/json2verilog.cpp
echo "✔ json2verilog 编译成功"

echo "===== Step 2: JSON → Verilog ====="
"$run_dir/json2verilog" "$constraint_file" 
echo "✔ Verilog 文件已生成: $run_dir/json2verilog.v"

echo "===== Step 3: Verilog → AAG ====="
YOSYS_SCRIPT="read_verilog $run_dir/json2verilog.v
hierarchy -check
opt
proc
techmap
opt
aigmap
opt
write_aiger -symbols -ascii $run_dir/verilog2aag.aag
exit"
echo "$YOSYS_SCRIPT" | ./yosys/yosys -q > "$run_dir/yosys.log" 2>&1
echo "✔ AAG 文件已生成: $run_dir/verilog2aag.aag"

echo "===== Step 4: 编译并运行 BDD 求解器 ====="
# C++编译相关设置
SRC_DIR="./srcs"
INCLUDE_DIR="./include"
CUDD_DIR="./cudd"
CUDD_LIB="$CUDD_DIR/cudd/.libs"
CUDD_INCLUDE="$CUDD_DIR/cudd"

EXEC_NAME="solution_gen"
CXX_FLAGS="-std=c++17 -O2 -Wall"
INCLUDE_FLAGS="-I$INCLUDE_DIR -I$SRC_DIR -I$CUDD_INCLUDE"
LINK_FLAGS="-L$CUDD_LIB -lcudd -lm -lquadmath"

# 检查 CUDD 库是否存在
if [ ! -f "$CUDD_LIB/libcudd.a" ] && [ ! -f "$CUDD_LIB/libcudd.so" ]; then
    echo "❌ 错误: 未找到已编译的 CUDD 库，请先执行："
    echo "   cd $CUDD_DIR && ./configure && make && cd .."
    exit 1
fi

# 编译 BDD 求解器
echo "编译 solution_gen.cpp..."
g++ $CXX_FLAGS $INCLUDE_FLAGS "$SRC_DIR/solution_gen.cpp" -o "$RUN_DIR/$EXEC_NAME" $LINK_FLAGS

if [ $? -ne 0 ]; then
    echo "❌ 编译失败: 无法生成 $EXEC_NAME"
    exit 1
fi
echo "✔ 编译成功: $RUN_DIR/$EXEC_NAME"

# 自动运行解生成器
AAG_FILE="$run_dir/verilog2aag.aag"
OUTPUT="$run_dir/result.json"       # 固定输出到 run_dir/result.json

echo "运行 $EXEC_NAME 生成解..."
"$run_dir/$EXEC_NAME" "$AAG_FILE" "$seed" "$solution_num" "$OUTPUT"

if [ $? -ne 0 ]; then
    echo "❌ 解生成失败"
    exit 1
fi

echo "✔ 解已生成: $OUTPUT"

exit 0
