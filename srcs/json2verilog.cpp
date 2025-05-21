#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/stat.h>

using json = nlohmann::json;

/**
 * Check if directory exists
 * @param path Path to check
 * @return True if directory exists, false otherwise
 */
bool directoryExists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
}

/**
 * Create directory if it doesn't exist
 * @param path Directory path to create
 * @return True if directory exists or was created, false otherwise
 */
bool createDirectory(const std::string& path) {
    if (directoryExists(path)) return true;
    
    #ifdef _WIN32
    int result = mkdir(path.c_str());
    #else
    int result = mkdir(path.c_str(), 0755);
    #endif
    
    return result == 0;
}

/**
 * Recursively generate Verilog expression string from JSON constraint
 * @param expression JSON expression object
 * @param variableList List of variables
 * @return Verilog expression string
 */
std::string generateExpression(const json& expression, const json& variableList, std::vector<std::string>& divisors) {
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
        return "(!(" + generateExpression(expression["lhs_expression"], variableList, divisors) + "))";
    } else if (op == "BIT_NEG") {
        return "(~(" + generateExpression(expression["lhs_expression"], variableList, divisors) + "))";
    } else if (op == "MINUS") {
        return "(-" + generateExpression(expression["lhs_expression"], variableList, divisors) + ")";
    }
    
    // Binary operators
    else if (op == "ADD") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " + " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "SUB") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " - " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "MUL") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " * " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "DIV") {
        std::string divisor = generateExpression(expression["rhs_expression"], variableList, divisors);
        divisors.push_back(divisor);
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " / " + divisor + ")";
    } else if (op == "LOG_AND") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " && " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "LOG_OR") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " || " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "EQ") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " == " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "NEQ") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " != " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "LT") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " < " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "LTE") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " <= " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "GT") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " > " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "GTE") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " >= " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "BIT_AND") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " & " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "BIT_OR") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " | " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "BIT_XOR") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " ^ " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "RSHIFT") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " >> " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "LSHIFT") {
        return "(" + generateExpression(expression["lhs_expression"], variableList, divisors) + " << " + generateExpression(expression["rhs_expression"], variableList, divisors) + ")";
    } else if (op == "IMPLY") {
        // For multi-bit, (lhs != 0) -> (rhs != 0)
        std::string lhs = generateExpression(expression["lhs_expression"], variableList, divisors);
        std::string rhs = generateExpression(expression["rhs_expression"], variableList, divisors);
        return "(!(" + lhs + " != 0) || (" + rhs + " != 0))";
    }

    // Handle unsupported operations
    std::cerr << "Warning: Unsupported operator '" << op << "'" << std::endl;
    return "";  
}

/**
 * Main function: Parse JSON constraint file and generate Verilog module
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit status
 */
int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <json_file> [output_dir]" << std::endl;
        return 1;
    }

    std::string inputFilePath = argv[1];
    std::string outputDir = "./run_dir";  // Default output directory
    
    // Use custom output directory if provided
    if (argc >= 3) {
        outputDir = argv[2];
    }
    
    // Ensure output directory exists
    if (!createDirectory(outputDir)) {
        std::cerr << "Error: Failed to create output directory: " << outputDir << std::endl;
        return 1;
    }

    // Open and parse JSON input file
    std::ifstream inputFile(inputFilePath);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Unable to open file: " << inputFilePath << std::endl;
        return 1;
    }

    json inputJson;
    inputFile >> inputJson;

    auto& variableList = inputJson["variable_list"];
    const auto& constraintList = inputJson["constraint_list"];

    // Create output file
    std::string outputFilePath = outputDir + "/json2verilog.v";
    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
        std::cerr << "Error: Unable to create output file: " << outputFilePath << std::endl;
        return 1;
    }

    // Generate Verilog module header
    outputFile << "module generated_module(";

    // Remove quotes from variable names
    for (size_t i = 0; i < variableList.size(); ++i) {
        std::string name = variableList[i]["name"].get<std::string>();
        name.erase(std::remove(name.begin(), name.end(), '"'), name.end());
        variableList[i]["name"] = name;
    }

    // Generate port list
    for (size_t i = 0; i < variableList.size(); ++i) {
        outputFile << variableList[i]["name"].get<std::string>();
        if (i != variableList.size() - 1) {
            outputFile << ", ";
        }
    }
    outputFile << ", x";
    outputFile << ");" << std::endl;

    // Generate input declarations
    for (size_t i = 0; i < variableList.size(); ++i) {
        outputFile << "    input "
                   << (variableList[i]["signed"] ? "signed " : "")
                   << "[" << (static_cast<int>(variableList[i]["bit_width"]) - 1) << ":0] "
                   << variableList[i]["name"].get<std::string>() << ";" << std::endl;
    }

    // Generate internal constraint wires
    outputFile << "    wire constraint_0";
    for (size_t i = 1; i < constraintList.size(); ++i) {
        outputFile << ", constraint_" << i;
    }
    outputFile << ";" << std::endl;
    
    // Generate output declaration
    outputFile << "    output wire x;" << std::endl;
    outputFile << std::endl;

    // for division
    std::vector<std::string> divisors;

    // Generate constraint assignments
    for (size_t i = 0; i < constraintList.size(); ++i) {
        std::string expr = generateExpression(constraintList[i], variableList, divisors);
        outputFile << "    assign constraint_" << i << " = |(" << expr << ");" << std::endl;
    }

    // Generate divisor assignments
    outputFile << "    wire constraint_" << constraintList.size();
    for (size_t i = constraintList.size() + 1; i < constraintList.size() + divisors.size(); ++i) {
        outputFile << ", constraint_" << i;
    }
    outputFile << ";" << std::endl;
    for (size_t i = 0; i < divisors.size(); ++i) {
        outputFile << "    assign constraint_" << constraintList.size() + i << " = |(" << divisors[i] << ");" << std::endl;
    }
    
    // Generate overall constraint (AND of all individual constraints)
    outputFile << "    assign x = constraint_0";
    for (size_t i = 1; i < constraintList.size() + divisors.size(); ++i) {
        outputFile << " & constraint_" << i;
    }
    outputFile << ";" << std::endl;
    
    // End module
    outputFile << "endmodule" << std::endl;

    std::cout << "Verilog file generated: " << outputFilePath << std::endl;
    return 0;
}