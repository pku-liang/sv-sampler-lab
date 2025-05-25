#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
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

//define operator weights
std::map<std::string, int> operator_weights = {
    {"VAR", 0},
    {"CONST", 0},
    {"LOG_NEG", 1},
    {"BIT_NEG", 1},
    {"MINUS", 2},
    {"ADD", 2},
    {"SUB", 2},
    {"MUL", 6},
    {"DIV", 6},
    {"LOG_AND", 1},
    {"LOG_OR", 1},
    {"EQ", 2},
    {"NEQ", 2},
    {"LT", 2},
    {"LTE", 2},
    {"GT", 2},
    {"GTE", 2},
    {"BIT_AND", 2},
    {"BIT_OR", 2},
    {"BIT_XOR", 4},
    {"RSHIFT", 2},
    {"LSHIFT", 2},
    {"IMPLY", 3}
};

struct CostComponents{
    int bit_width_sum = 0;
    int operator_weight_sum = 0;
    int num_variables = 0;
    bool has_division = false;
};

//Recursive helper to get cost components of an expression
CostComponents get_expression_cost_components(const json& expression, const json& variableList) {
    CostComponents cost;
    std::string op = expression["op"];

    // Use a default weight of 1 if operator is not in the map
    cost.operator_weight_sum += operator_weights.count(op) ? operator_weights[op] : 1; 
    if(op == "DIV"){
        cost.has_division = true;
    }

    if (op == "VAR"){
        int id = expression["id"];
        // Corrected typo: "bit_width"
        cost.bit_width_sum += static_cast<int>(variableList[id]["bit_width"]); 
        cost.num_variables = 1;
    }
    else if (op == "CONST") {
        // Constants typically have low cost, num_variables remains 0
    }
    else if(expression.contains("lhs_expression") && !expression.contains("rhs_expression")) { // Unary
        CostComponents lhs_cost = get_expression_cost_components(expression["lhs_expression"], variableList);
        cost.bit_width_sum += lhs_cost.bit_width_sum;
        cost.operator_weight_sum += lhs_cost.operator_weight_sum;
        cost.num_variables += lhs_cost.num_variables;
        cost.has_division = cost.has_division || lhs_cost.has_division;
    }
    // Corrected condition for binary: check for both lhs_expression and rhs_expression
    else if (expression.contains("lhs_expression") && expression.contains("rhs_expression")) { // Binary
            CostComponents lhs_cost = get_expression_cost_components(expression["lhs_expression"], variableList);
            CostComponents rhs_cost = get_expression_cost_components(expression["rhs_expression"], variableList);
            cost.bit_width_sum += rhs_cost.bit_width_sum + lhs_cost.bit_width_sum;
            // Corrected logic: add operator_weight_sum from rhs_cost
            cost.operator_weight_sum += rhs_cost.operator_weight_sum + lhs_cost.operator_weight_sum; 
            cost.num_variables += rhs_cost.num_variables + lhs_cost.num_variables;
            cost.has_division = cost.has_division || lhs_cost.has_division || rhs_cost.has_division;
    }
    // Consider other cases if your JSON can have different structures for expressions
    return cost;
}
// Calculate the cost of a constraint expression based on its components
double calculate_constraint_cost(const json& constraint_expression, const json& variableList) {
    CostComponents components = get_expression_cost_components(constraint_expression, variableList);
    double cost = 0.0;

    // Weighted sum of components. Tune weights as needed.
    // Example: average bit width per variable + operator weights.
    if (components.num_variables > 0) {
        cost += (double)components.bit_width_sum / components.num_variables; 
    } else { // Constant expression, very cheap
        cost += 0.1;
    }
    cost += (double)components.operator_weight_sum;

    // Add a large penalty if the constraint involves division, as it implies a divisor != 0 check.
    if (components.has_division) {
        cost += 50.0; // Arbitrary large penalty
    }
    return cost;
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

    // Generate output declaration
    outputFile << "    output wire x;" << std::endl;

    // Data structures to hold information for Verilog generation
    std::vector<std::string> all_wires_to_declare;
    std::vector<std::pair<std::string, std::string>> assignments_to_print; // pair: {wire_name, rhs_expression_for_assign}
    
    struct ConstraintInfoForSorting { // Renamed from ConstraintCostInfo for clarity
        std::string wire_name;
        double cost;
    };
    std::vector<ConstraintInfoForSorting> original_constraints_for_sorting;
    std::vector<std::string> divisor_check_wires_for_final_AND;
    
    std::set<std::string> unique_divisor_expressions_set; // Use a set for automatic uniqueness of divisor expressions
    int global_constraint_idx = 0;

    // --- Phase 1: Process original constraints (collect info) ---
    for (size_t i = 0; i < constraintList.size(); ++i) {
        std::string current_wire_name = "constraint_" + std::to_string(global_constraint_idx++);
        all_wires_to_declare.push_back(current_wire_name);
        
        std::vector<std::string> current_constraint_divisors; // To capture divisors from this specific constraint
        std::string expr_str = generateExpression(constraintList[i], variableList, current_constraint_divisors);
        assignments_to_print.push_back({current_wire_name, "|(" + expr_str + ")" });

        double cost = calculate_constraint_cost(constraintList[i], variableList);
        original_constraints_for_sorting.push_back({current_wire_name, cost});

        for (const auto& div_expr : current_constraint_divisors) {
            unique_divisor_expressions_set.insert(div_expr);
        }
    }

    // --- Phase 2: Process unique divisor checks (collect info) ---
    for (const std::string& div_expr_str : unique_divisor_expressions_set) {
        std::string current_wire_name = "constraint_" + std::to_string(global_constraint_idx++);
        all_wires_to_declare.push_back(current_wire_name);
        assignments_to_print.push_back({current_wire_name, "|(" + div_expr_str + ")" });
        divisor_check_wires_for_final_AND.push_back(current_wire_name);
    }
    
    outputFile << std::endl; // Blank line before wire declarations

    // --- Phase 3: Print all wire declarations ---
    if (!all_wires_to_declare.empty()) {
        outputFile << "    wire ";
        for (size_t k = 0; k < all_wires_to_declare.size(); ++k) {
            outputFile << all_wires_to_declare[k] << (k == all_wires_to_declare.size() - 1 ? "" : ", ");
        }
        outputFile << ";" << std::endl;
    }
    outputFile << std::endl; // Blank line after wire declarations

    // --- Phase 4: Print all assignments ---
    for (const auto& assignment_pair : assignments_to_print) {
        outputFile << "    assign " << assignment_pair.first << " = " << assignment_pair.second << ";" << std::endl;
    }
    outputFile << std::endl; // Blank line after assignments


    // --- Phase 5: Sort the original constraint wires by cost for the final AND ---
    std::sort(original_constraints_for_sorting.begin(), original_constraints_for_sorting.end(),
              [](const ConstraintInfoForSorting& a, const ConstraintInfoForSorting& b) {
        return a.cost < b.cost; // Ascending order by cost
    });

    // --- Phase 6: Generate the final 'x' assignment ---
    outputFile << "    assign x = ";
    bool first_term_in_and_chain = true;
    auto append_to_and_chain = [&](const std::string& term) {
        if (!first_term_in_and_chain) {
            outputFile << " & ";
        }
        outputFile << term;
        first_term_in_and_chain = false;
    };

    bool has_any_terms = !original_constraints_for_sorting.empty() || !divisor_check_wires_for_final_AND.empty();

    if (!has_any_terms) {
        outputFile << "1'b1"; // No constraints or divisor checks, x is true
    } else {
        // Add sorted original constraint wires
        for (const auto& sorted_constraint_info : original_constraints_for_sorting) {
            append_to_and_chain(sorted_constraint_info.wire_name);
        }
        // Add divisor check wires
        // Their order among themselves doesn't typically need sorting by cost,
        // but they are ANDed after the sorted original constraints.
        for (const auto& div_check_wire : divisor_check_wires_for_final_AND) {
            append_to_and_chain(div_check_wire);
        }
    }
    outputFile << ";" << std::endl;
    
    // End module
    outputFile << "endmodule" << std::endl;

    std::cout << "Verilog file generated: " << outputFilePath << std::endl;
    return 0;
}