#!/bin/bash
# filepath: /root/finalProject/COSMOS-BDD-solver/build.sh

set -e  # 遇错退出

# 创建_run目录（如果不存在）
mkdir -p _run


echo "===== Step 1: 编译 CUDD 库 ====="
if [ ! -f "./cudd/cudd/.libs/libcudd.a" ] && [ ! -f "./cudd/cudd/.libs/libcudd.so" ]; then
    echo "编译 CUDD 库..."
    cd ./cudd
    ./configure
    make
    cd ..
    echo "✔ CUDD 库编译成功"
else
    echo "✔ CUDD 库已存在，跳过编译"
fi

echo "===== Step 2: 编译 Yosys ====="
if [ ! -f "./yosys/yosys" ]; then
    echo "编译 Yosys..."
    cd ./yosys
    make config-gcc
    make
    cd ..
    echo "✔ Yosys 编译成功"
else
    echo "✔ Yosys 已存在，跳过编译"
fi

echo "===== Step 3: 编译 json2verilog ====="
# 设置编译参数
JSON2VERILOG_SRC="./json2verilog.cpp"
JSON2VERILOG_EXEC="_run/json2verilog"
JSON2VERILOG_FLAGS="-std=c++11 -I./json/include"

# 编译json2verilog
echo "编译 json2verilog.cpp..."
g++ ${JSON2VERILOG_FLAGS} -o "${JSON2VERILOG_EXEC}" ${JSON2VERILOG_SRC}

if [ $? -ne 0 ]; then
    echo "❌ 编译失败: 无法生成 json2verilog"
    exit 1
fi
echo "✔ 编译成功: ${JSON2VERILOG_EXEC}"

echo "===== Step 4: 编译 BDD 求解器 ====="
# C++编译相关设置
SRC_DIR="./"
INCLUDE_DIR="./json/include"
CUDD_DIR="./cudd"
CUDD_LIB="$CUDD_DIR/cudd/.libs"
CUDD_INCLUDE="$CUDD_DIR/cudd"

EXEC_NAME="solution_gen"
CXX_FLAGS="-std=c++17 -O2 -Wall"
INCLUDE_FLAGS="-I$INCLUDE_DIR -I./cudd -I./cudd/epd -I./cudd/st -I./cudd/mtr -I$SRC_DIR -I$CUDD_INCLUDE"
LINK_FLAGS="-L$CUDD_LIB -lcudd -lm -lquadmath"

# 检查 CUDD 库是否存在
if [ ! -f "$CUDD_LIB/libcudd.a" ] && [ ! -f "$CUDD_LIB/libcudd.so" ]; then
    echo "❌ 错误: 未找到已编译的 CUDD 库"
    exit 1
fi

# 编译 BDD 求解器
echo "编译 solution_gen.cpp..."
g++ ${CXX_FLAGS} ${INCLUDE_FLAGS} "${SRC_DIR}/solution_gen.cpp" -o "_run/${EXEC_NAME}" ${LINK_FLAGS}

if [ $? -ne 0 ]; then
    echo "❌ 编译失败: 无法生成 ${EXEC_NAME}"
    exit 1
fi
echo "✔ 编译成功: _run/${EXEC_NAME}"

echo "===== 所有工具编译完成 ====="
echo "可执行文件位置:"
echo "- json2verilog: _run/json2verilog"
echo "- solution_gen: _run/solution_gen"

exit 0