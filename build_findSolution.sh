#!/bin/bash

# Directories
SRC_DIR="./srcs"
INCLUDE_DIR="./include"
RUN_DIR="./run_dir"
CUDD_DIR="./cudd"

# Executable name
EXEC_NAME="findSolution"

# Make sure run directory exists
echo "Creating run directory if it doesn't exist..."
mkdir -p $RUN_DIR

# 检查 CUDD 库是否已编译
if [ ! -f "$CUDD_DIR/cudd/.libs/libcudd.a" ] && [ ! -f "$CUDD_DIR/cudd/.libs/libcudd.so" ]; then
    echo "CUDD library not found. Please compile it first:"
    echo "cd $CUDD_DIR && ./configure && make && cd .."
    exit 1
fi

# 更新 CUDD 库的位置
CUDD_LIB="$CUDD_DIR/cudd/.libs"
CUDD_INCLUDE="$CUDD_DIR/cudd"

# Compilation flags
CXX_FLAGS="-std=c++17 -O2 -Wall"
INCLUDE_FLAGS="-I$INCLUDE_DIR -I$SRC_DIR -I$CUDD_INCLUDE"  
LINK_FLAGS="-L$CUDD_LIB -lcudd -lm"  

# Compile command
COMPILE_CMD="g++ $CXX_FLAGS $INCLUDE_FLAGS"

# Log compilation steps
echo "Compiling aag2BDD.cpp..."
$COMPILE_CMD -c $SRC_DIR/aag2BDD.cpp -o $RUN_DIR/aag2BDD.o

if [ $? -ne 0 ]; then
    echo "Failed to compile aag2BDD.cpp"
    exit 1
fi

echo "Compiling random_solutions.cpp and linking..."
$COMPILE_CMD $SRC_DIR/random_solutions.cpp $RUN_DIR/aag2BDD.o -o $RUN_DIR/$EXEC_NAME $LINK_FLAGS

if [ $? -eq 0 ]; then
    echo "Compilation successful! Executable created at $RUN_DIR/$EXEC_NAME"
    echo "Usage: $RUN_DIR/$EXEC_NAME <aag_file> <random_seed> <num_solutions> [output_file]"
else
    echo "Failed to compile random_solutions.cpp"
    exit 1
fi