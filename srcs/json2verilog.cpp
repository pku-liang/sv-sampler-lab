#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

using json = nlohmann::json;

// recursively generate the expression string
std::string generateExpression(const json& expression, const json& variableList) {
    std::string op = expression["op"];
    
    // Basic operands
    if (op == "VAR") {
        int id = expression["id"];
        return variableList[id]["name"];
    } else if (op == "CONST") {
        return expression["value"];
    }
    
    // Unary operators
    else if (op == "LOG_NEG") {
        return "!(" + generateExpression(expression["lhs_expression"], variableList) + ")";
    } else if (op == "BIT_NEG") {
        return "~(" + generateExpression(expression["lhs_expression"], variableList) + ")";
    } else if (op == "MINUS") {
        return "(-" + generateExpression(expression["lhs_expression"], variableList) + ")";
    }
    
    // Binary operators
    else if (op == "ADD") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " + " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "SUB") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " - " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "MUL") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " * " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "DIV") {
        std::string divisor = generateExpression(expression["rhs_expression"], variableList);
        return "(" + divisor + " == 0 ? 0 : " + generateExpression(expression["lhs_expression"], variableList) + " / " + divisor + ")";
    } else if (op == "LOG_AND") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " && " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "LOG_OR") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " || " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "EQ") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " == " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "NEQ") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " != " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "LT") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " < " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "LTE") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " <= " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "GT") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " > " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "GTE") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " >= " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "BIT_AND") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " & " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "BIT_OR") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " | " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "BIT_XOR") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " ^ " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "RSHIFT") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " >> " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "LSHIFT") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " << " + generateExpression(expression["rhs_expression"], variableList) + ")";
    } else if (op == "IMPLY") {
        return "(" + generateExpression(expression["lhs_expression"], variableList) + " -> " + generateExpression(expression["rhs_expression"], variableList) + ")";
    }

    // Other unsupported operations
    std::cerr << "Warning: Unsupported operator '" << op << "'" << std::endl;
    return "";  
}

// get json_file from cmd and generate verilog file
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "syntax error. Try " << argv[0] << " <json_file>" << std:: endl;
        return 1;
    }

    std::string inputFilePath = argv[1];

    // open json file
    std::ifstream inputFile(inputFilePath);
    if (!inputFile.is_open()) {
        std::cerr << "Unable to open:" << inputFilePath << std::endl;
        return 1;
    }

    json inputJson;
    inputFile >> inputJson;

    auto& variableList = inputJson["variable_list"];
    const auto& constraintList = inputJson["constraint_list"];

    // create output directory
    std::ofstream outputFile("./run_dir/json2verilog.v");
    if (!outputFile.is_open()) {
        std::cerr << "Unable to create output file" << std::endl;
        return 1;
    }

    outputFile << "module generated_module(";

    //remove the quotes from the variable names
    for (size_t i = 0; i < variableList.size(); ++i) {
        std::string name = variableList[i]["name"].get<std::string>();
        name.erase(std::remove(name.begin(), name.end(), '"'), name.end());
        variableList[i]["name"] = name;
    }

    // write variavle names
    for (size_t i = 0; i < variableList.size(); ++i) {
        outputFile << variableList[i]["name"].get<std::string>();
        if (i != variableList.size() - 1) {
            outputFile << ", ";
        }
    }

    outputFile << ");" << std::endl;

    // write variable declarations
    for (size_t i = 0; i < variableList.size(); ++i) {
        outputFile << "    input "
                   << (variableList[i]["signed"] ? "signed " : "")
                   << "[" << (static_cast<int>(variableList[i]["bit_width"]) - 1) << ":0] "
                   << variableList[i]["name"].get<std::string>() << ";" << std::endl;
    }

    outputFile << std::endl;

    // write constraints
    for (size_t i = 0; i < constraintList.size(); ++i) {
        std::string expr = generateExpression(constraintList[i], variableList);
        outputFile << "    wire constraint_" << i << ';' << std::endl;
        outputFile << "    assign constraint_" << i << " = |(" << expr << ");" << std::endl;
    }

    outputFile << "endmodule" << std::endl;

    std::cout << "Verilog file has been created : ./run_dir/json2verilog.v" << std::endl;

    return 0;
}