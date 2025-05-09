#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using json = nlohmann::json;

// 生成 Verilog 表达式的递归函数
std::string generateExpression(const json& expression, const json& variableList) {
    std::string op = expression["op"];
    if (op == "VAR") {
        int id = expression["id"];
        return variableList[id]["name"];
    } else if (op == "CONST") {
        return expression["value"];
    } else if (op == "BIT_NEG") {
        return "~(" + generateExpression(expression["lhs_expression"], variableList) + ")";
    } else if (op == "LOG_NEG") {
        return "!(" + generateExpression(expression["lhs_expression"], variableList) + ")";
    } else if (op == "ADD") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " + " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "SUB") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " - " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "MUL") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " * " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "DIV") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " / " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "BIT_XOR") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " ^ " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "LOG_AND") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " && " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "RSHIFT") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " >> " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "LSHIFT") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " << " + generateExpression(expression["rhs_expression"], variableList) + ")";
    }
    return "";
}

// 主函数：解析 JSON 并生成 Verilog
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <输入文件路径>" << std::endl;
        return 1;
    }

    std::string inputFilePath = argv[1];

    // 打开 JSON 文件
    std::ifstream inputFile(inputFilePath);
    if (!inputFile.is_open()) {
        std::cerr << "无法打开输入文件：" << inputFilePath << std::endl;
        return 1;
    }

    // 解析 JSON 数据
    json inputJson;
    inputFile >> inputJson;

    // 提取变量列表和约束
    const auto& variableList = inputJson["variable_list"];
    const auto& constraintList = inputJson["constraint_list"];

    // 创建输出文件夹
    std::ofstream outputFile("verilogFile/output.v");
    if (!outputFile.is_open()) {
        std::cerr << "无法创建输出文件！" << std::endl;
        return 1;
    }

    outputFile << "module generated_module(";

    // 写出所有输入端口名
    for (size_t i = 0; i < variableList.size(); ++i) {
        outputFile << variableList[i]["name"];
        if (i != variableList.size() - 1) {
            outputFile << ", ";
        }
    }

    outputFile << ");" << std::endl;

    // 写变量定义
    for (size_t i = 0; i < variableList.size(); ++i) {
        outputFile << "    input "
                   << (variableList[i]["signed"] ? "signed " : "")
                   << "[" << (static_cast<int>(variableList[i]["bit_width"]) - 1) << ":0] "
                   << variableList[i]["name"] << ";" << std::endl;
    }

    outputFile << std::endl;

    // 写约束表达式逻辑为 wire
    for (size_t i = 0; i < constraintList.size(); ++i) {
        std::string expr = generateExpression(constraintList[i], variableList);
        outputFile << "    wire constraint_" << i << " = " << expr << ";" << std::endl;
    }

    outputFile << "endmodule" << std::endl;

    std::cout << "Verilog 文件已生成：verilogFile/output.v" << std::endl;

    return 0;
}