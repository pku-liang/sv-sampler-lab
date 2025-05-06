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

`constraint_list` 为一个描述约束表达式的JSON array，每个约束表达式为如下形式
```json
{
  "op": "BIT_NEG",
  "lhs_expression": {
    ...表达式定义
  },
  "rhs_expression": {
    ...表达式定义
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
./run.sh [constraint.json] [num_samples] [result.json] [random_seed]
```
你的程序需要读入constraint.json，根据random_seed设置随机种子，生成num_samples个随机解到result.json中。

## 三方库说明
由于三方库不同版本可能存在bug，请从这个repo的submodule中下载，运行
```bash
git submodule update --init
```
当然你可以用其他三方库，但是如果使用了不同版本，你需要自己解决三方库的问题。
