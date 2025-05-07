# 面向SystemVerilog约束的二元决策图（BDD）求解器大作业说明
## NOTE
有问题的话可以先去Issues查一下，如果没有找到可以提Issue。

## 输入格式
输入为一个JSON文件，分为 `variable_list` 和 `constraint_list` 组成。`variable_list` 为一个描述变量信息的JSON array，每一个变量包含如下信息
```json
{
  "id": 0,
  "name": "var_0",
  "signed": false,
  "bit_width": 13
}
```
在测试用例中所有的变量均为无符号数，即 `"signed"` 都是 `false`，不需要考虑 `"signed": true` 的情况。

`constraint_list` 为一个描述约束表达式的JSON array，每个约束表达式为如下形式
```json
{
  "op": "BIT_NEG",
  "lhs_expression": {
    //...表达式定义
  },
  "rhs_expression": {
    //...表达式定义
  }
}
```
其中 `lhs_expression` 和 `rhs_expression` 递归地描述了运算的左右两边的表达式。
当 `op` 是单元运算符（unary operator）时只有 `lhs_expression` ，没有 `rhs_expression`。
当 `op` 是 `VAR` 时，表达式为一个变量，形式为
```json
{
  "op": "VAR",
  "id": 1
}
```
当 `op` 是 `CONST`，表达式为一个常数，形式为
```json
{
  "op": "CONST",
  "value": "1'h1"
}
```

## 输出格式
你需要输出一个JSON文件，格式如下
```hjson
{
  "assignment_list": [
    // 第一组解
    [
      { "value": "2fd3d29"}, // id=0的变量赋值
      { "value": "a0a1"}, // id=1的变量赋值
      ...
    ],
    // 第二组解
    [
      ...
    ]
  ]
}
```
其中每一组解为一个数组，按照变量 `id` 顺序从小到大给出赋值，每个变量的赋值用16进制串表示，例如 `wire [10:0] var_0` 的赋值为 `5'b110_1110_1001`，对应的输出为 `6e9`。

## 提交形式
你提交的源代码需要额外提供两个脚本 `build.sh` 和 `run.sh`。
在评测时，会执行一次 `./build.sh`，用于编译你的代码。
`run.sh` 用于执行你的程序，应当实现如下调用方式：
```bash
./run.sh [constraint.json] [num_samples] [run_dir] [random_seed]
```
你的程序需要读入constraint.json，根据random_seed设置随机种子，生成num_samples个随机解到 `run_dir/result.json` 中。程序执行过程中只能使用一个线程，运行的中间文件统一输出到 `run_dir` 中，避免对不同测试点同时测试时文件互相覆盖。

## 评分
评测数据包含多个部分，每个部分的总分和要求如下

|测试文件夹|分值|要求|
|-------|----|----|
|basic|70|运行10次，每次均能在60s内完成|
|opt1|4|运行10次，每次均能在30s内完|
|opt2|4|运行10次，每次均能在120s内完|
|opt3|6|运行10次，每次都能在30s内完成|
|opt4|4|运行10次，每次都能在120s内完成|
|opt5|12|运行10次，至少8次能在20s内完成|

每个部分包含多个测试点，通过一个测试点即可拿到对应比例的分数（ $n$ 个测试点通过了 $k$ 个即可拿到分数的 $\frac{k}{n}$）。

## 性能优化Hint
- 根据电路结构求得一个变量初始顺序，调用 `Cudd_ShuffleHeap` 设置手动变量顺序
- 使用CUDD中动态变量重排策略，调用 `Cudd_AutodynEnable` 开启。
- 电路中需要使用AND将所有约束的结果合并为一个输出，约束的合并顺序会影响BDD的中间结果，对运行时间有较大影响。
- 有些变量可能相互没有任何关系，不相关的变量可以分开求解。
  
## 第三方库说明
这个大作业中需要用到很多第三方库，**配置和编译第三方库会比较耗费时间，请提前配好环境**。

由于第三方库不同版本可能存在bug，请从这个repo的submodule中下载，运行
```bash
git submodule update --init
```
当然你可以用其他第三方库，但是如果使用了不同版本，你需要自己解决第三方库的问题。

第三方库文档：
- [CUDD设计文档](http://web.mit.edu/sage/export/tmp/y/usr/share/doc/polybori/cudd/node4.html), [CUDD使用文档](http://web.mit.edu/sage/export/tmp/y/usr/share/doc/polybori/cudd/node3.html)，[CUDD接口函数](http://web.mit.edu/sage/export/tmp/y/usr/share/doc/polybori/cudd/cuddExtDet.html)
- [Yosys文档](https://yosyshq.readthedocs.io/projects/yosys/en/latest/index.html)，下面几个命令会比较有用
  - `read_verilog` 读入verilog文件
  - `synth` 综合verilog文件
  - `aigmap` 将电路映射为AIG网表
  - `write_aiger` 输出AIG网表
  - 运行 `yosys` 进入交互式命令行中，可以执行 `help [cmd]` 打印关于 `[cmd]` 的使用方法
  - `yosys -q -p "read_verilog cnstr.sv; synth; aigmap; writer_aiger cnstr.aig;"` 可以在非交互式情况下执行一串指令（`-q`会阻止额外信息被打印，如果想看到细节可以去掉 `-q`）
- [AIG格式描述](https://github.com/arminbiere/aiger/blob/master/FORMAT)
- [nlohmann::json文档](https://github.com/nlohmann/json?tab=readme-ov-file#read-json-from-a-file)
