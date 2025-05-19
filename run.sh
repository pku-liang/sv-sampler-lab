#!/bin/bash
# filepath: /root/finalProject/COSMOS-BDD-solver/run.sh

set -e  # 遇错退出

# 检查参数
if [ "$#" -lt 3 ]; then
    echo "用法: $0 <约束文件.json> <解的数量> <输出目录> [<随机种子>]"
    exit 1
fi

# 参数
constraint_file="$1"
solution_num="$2"
run_dir="$3"
seed="${4:-42}"  # 默认种子为42

# 提取数据集和数据ID信息，便于日志显示
dataset_name=$(dirname "$constraint_file")
data_id=$(basename "$constraint_file" .json)

# 确保输出目录存在
mkdir -p "$run_dir"
basename=$(basename "$constraint_file" .json)

echo "===== 处理 $dataset_name/$data_id.json 到 $run_dir ====="

echo "===== Step 1: 编译 json2verilog ====="
g++ -std=c++11 -I./include -o "$run_dir/json2verilog" ./srcs/json2verilog.cpp
echo "✔ json2verilog 编译成功"

echo "===== Step 2: JSON → Verilog ====="
"$run_dir/json2verilog" "$constraint_file" "$run_dir"
# 确保 json2verilog.v 生成到正确目录
if [ ! -f "$run_dir/json2verilog.v" ]; then
    echo "❌ 错误: Verilog 文件未生成"
    exit 1
fi
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

# 检查 AAG 文件是否生成成功
if [ ! -f "$run_dir/verilog2aag.aag" ]; then
    echo "❌ 错误: AAG 文件未生成"
    echo "详情请查看: $run_dir/yosys.log"
    exit 1
fi
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
g++ $CXX_FLAGS $INCLUDE_FLAGS "$SRC_DIR/solution_gen.cpp" -o "$run_dir/$EXEC_NAME" $LINK_FLAGS

if [ $? -ne 0 ]; then
    echo "❌ 编译失败: 无法生成 $EXEC_NAME"
    exit 1
fi
echo "✔ 编译成功: $run_dir/$EXEC_NAME"

# 自动运行解生成器
AAG_FILE="$run_dir/verilog2aag.aag"
OUTPUT="$run_dir/result.json"       # 固定输出到 run_dir/result.json

echo "运行 $EXEC_NAME 生成解..."
echo "命令: $run_dir/$EXEC_NAME $AAG_FILE $seed $solution_num $OUTPUT"

# 记录开始时间
start_time=$(date +%s)

# 运行求解器，并将输出记录到日志文件
"$run_dir/$EXEC_NAME" "$AAG_FILE" "$seed" "$solution_num" "$OUTPUT" > "$run_dir/solver.log" 2>&1

if [ $? -ne 0 ]; then
    echo "❌ 解生成失败，请查看日志: $run_dir/solver.log"
    exit 1
fi

# 记录结束时间和运行时间
end_time=$(date +%s)
runtime=$((end_time - start_time))

# 将运行时间写入文件
echo "$runtime" > "$run_dir/runtime.txt"

# 检查结果文件是否存在
if [ ! -f "$OUTPUT" ]; then
    echo "❌ 错误: 结果文件未生成: $OUTPUT"
    exit 1
fi

echo "✔ 解已生成: $OUTPUT (用时: ${runtime}秒)"
echo "处理完成: $dataset_name/$data_id.json (种子: $seed)"

exit 0