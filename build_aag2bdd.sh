#!/bin/bash

# 设置默认输出路径
OUTPUT_DIR="./run_dir"  # 输出文件夹，路径相对于脚本位置

# 如果没有传递输入的 AAG 文件路径，提示用户并退出
if [ -z "$1" ]; then
  echo "错误: 未指定 AAG 文件路径!"
  echo "用法: ./build_aag2bdd.sh <AAG_FILE>"
  exit 1
fi

AAG_FILE="$1"  # 使用第一个命令行参数作为 AAG 文件路径

# 检查输出文件夹是否存在，如果不存在则创建
if [ ! -d "$OUTPUT_DIR" ]; then
  echo "创建输出目录 $OUTPUT_DIR"
  mkdir -p "$OUTPUT_DIR"
fi

# 检查 aag2BDD.exe 文件是否存在
if [ ! -f "./aag2BDD" ]; then
  echo "错误: 找不到 aag2BDD 文件!"
  exit 1
fi

# 运行 aag2BDD.exe 程序
echo "运行 aag2BDD 转换..."
./aag2BDD "$AAG_FILE" "$OUTPUT_DIR"

# 检查程序是否成功运行
if [ $? -eq 0 ]; then
  echo "转换成功! 输出保存在 $OUTPUT_DIR"
else
  echo "转换失败!"
  exit 1
fi
