# COSMOS-BDD-solver 项目开发计划

## 项目概述

实现一个面向SystemVerilog约束的二元决策图(BDD)求解器，流程如下：
1. 将JSON约束描述转换为Verilog电路
2. 使用Yosys将Verilog综合为AIG网表
3. 构建BDD并从中采样生成随机解

## 目录结构设计

COSMOS-BDD-solver/
├── bin/                   # 编译后的可执行文件目录
├── build/                 # 构建目录
├── cudd/                  # CUDD库子模块
├── include/               # 头文件目录
│   └── nlohmann/           # JSON库
│       └── json.hpp
├── src/                   # 源代码目录
│   ├── json2verilog.cpp   # JSON到Verilog转换器
│   ├── verilog2aig.cpp    # 调用Yosys转换为AIG
│   ├── aig2bdd.cpp        # AIG到BDD转换器
│   ├── bdd_sampler.cpp    # BDD随机采样器
│   └── main.cpp           # 主程序
├── test/                  # 测试代码和测试用例
├── basic/                 # 基础测试数据
├── opt1/                  # 优化测试数据1
├── opt2/                  # 优化测试数据2
├── opt3/                  # 优化测试数据3
├── opt4/                  # 优化测试数据4
├── opt5/                  # 优化测试数据5
├── CMakeLists.txt         # CMake构建脚本
├── build.sh               # 构建脚本
└── run.sh                 # 运行脚本

## 开发任务列表

### 1. 环境准备与依赖安装

- [ ] 安装C++编译器(支持C++11或更高版本)
- [ ] 安装CMake(3.10或更高版本)
- [ ] 安装Yosys综合工具
- [ ] 下载并安装CUDD库
- [ ] 下载nlohmann/json库

### 2. 项目初始化

- [ ] 创建基本目录结构
- [ ] 初始化Git仓库
- [ ] 添加CUDD库作为子模块
- [ ] 下载json.hpp到include目录
- [ ] 创建基本的CMakeLists.txt

### 3. 实现JSON到Verilog转换器

- [ ] 解析JSON文件结构
- [ ] 实现变量声明生成
- [ ] 实现操作符映射(ADD, SUB, MUL等到Verilog操作符)
- [ ] 递归生成表达式树
- [ ] 生成完整的Verilog模块
- [ ] 测试basic目录下的JSON文件转换

### 4. 实现Verilog到AIG的转换

- [ ] 研究Yosys API或命令行参数
- [ ] 编写脚本或程序调用Yosys
- [ ] 处理中间文件和错误情况
- [ ] 验证生成的AIG文件格式正确

### 5. 实现AIG到BDD转换

- [ ] 学习CUDD库API
- [ ] 解析AIG文件格式
- [ ] 建立AIG与BDD节点的映射
- [ ] 实现变量排序优化
- [ ] 处理大型BDD的内存限制问题

### 6. 实现BDD采样器

- [ ] 设计随机路径采样算法
- [ ] 实现采样函数接收随机种子参数
- [ ] 确保样本的均匀分布
- [ ] 将样本转换为指定的JSON格式
- [ ] 优化采样性能

### 7. 实现主程序

- [ ] 解析命令行参数
- [ ] 实现完整流程控制
- [ ] 处理错误情况和边界条件
- [ ] 确保符合README要求的接口

### 8. 编写构建与运行脚本

- [ ] 编写build.sh脚本
- [ ] 编写run.sh脚本
- [ ] 确保脚本在不同环境中的兼容性
- [ ] 添加适当的错误处理和提示

### 9. 测试与优化

- [ ] 对basic测试集运行基准测试
- [ ] 识别性能瓶颈
- [ ] 针对opt1-opt5优化性能
- [ ] 确保所有测试用例都能在规定时间内完成

### 10. 文档与提交准备

- [ ] 编写使用说明文档
- [ ] 确保所有脚本具有执行权限
- [ ] 检查文件格式(特别是行尾格式)
- [ ] 准备最终提交

