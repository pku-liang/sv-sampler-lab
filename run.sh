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

# 检查预编译的可执行文件是否存在，如果不存在则运行 build.sh
if [ ! -f "_run/json2verilog" ] || [ ! -f "_run/solution_gen" ]; then
    echo "===== 可执行文件不存在，运行 build.sh 进行编译 ====="
    ./build.sh
    if [ $? -ne 0 ]; then
        echo "❌ 编译失败: build.sh 执行出错"
        exit 1
    fi
    echo "✔ 编译成功: 可执行文件已生成"
else
    echo "✔ 使用已有的可执行文件"
fi

echo "===== Step 1: JSON → Verilog ====="
# 复制json2verilog到run_dir（可选，如果希望保留在运行目录中）
cp "_run/json2verilog" "$run_dir/json2verilog"

# 执行转换
"_run/json2verilog" "$constraint_file" "$run_dir"

# 确保 json2verilog.v 生成到正确目录
if [ ! -f "$run_dir/json2verilog.v" ]; then
    echo "❌ 错误: Verilog 文件未生成"
    exit 1
fi
echo "✔ Verilog 文件已生成: $run_dir/json2verilog.v"

echo "===== Step 2: Verilog → AAG ====="
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

echo "===== Step 3: 运行 BDD 求解器 ====="
# 复制solution_gen到run_dir（可选，如果希望保留在运行目录中）
cp "_run/solution_gen" "$run_dir/solution_gen"

# 自动运行解生成器
AAG_FILE="$run_dir/verilog2aag.aag"
OUTPUT="$run_dir/result.json"       # 固定输出到 run_dir/result.json

echo "运行 solution_gen 生成解..."
echo "命令: _run/solution_gen $AAG_FILE $seed $solution_num $OUTPUT"

# 记录开始时间
start_time=$(date +%s)

# 运行求解器，并将输出记录到日志文件
"_run/solution_gen" "$AAG_FILE" "$seed" "$solution_num" "$OUTPUT" > "$run_dir/solver.log" 2>&1

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