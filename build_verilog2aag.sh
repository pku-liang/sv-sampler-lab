#!/bin/bash

# 检查参数数量
if [ $# -lt 2 ]; then
    echo "用法: $0 <输入.v文件路径> <输出.aag文件路径>"
    echo "示例: $0 ./example.v ./out.aag"
    exit 1
fi

VERILOG_FILE="$1"
AAG_FILE="$2"

# 生成 Yosys 脚本
YOSYS_SCRIPT="read_verilog $VERILOG_FILE
hierarchy -check
opt
proc
techmap
opt
aigmap 
opt
write_aiger -symbols -ascii $AAG_FILE
exit"

# 执行 Yosys
echo "$YOSYS_SCRIPT" | ./yosys/yosys
