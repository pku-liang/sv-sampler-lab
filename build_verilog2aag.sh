#!/bin/bash

# check parameters
if [ $# -lt 2 ]; then
    echo "fromat: $0 <input_verilog_path.v> <output_aag_path.aag>"
    echo "example: $0 ./run_dir/json2verilog.v ./run_dir/verilog2aag.aag"
    exit 1
fi

VERILOG_FILE="$1"
AAG_FILE="$2"

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

# execute 
echo "$YOSYS_SCRIPT" | ./yosys/yosys
echo "----------------------------------------"
echo "convert $VERILOG_FILE to $AAG_FILE done"
echo "AAG file: $AAG_FILE"
