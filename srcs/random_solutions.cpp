#include <cstdio>
#include <cstddef>
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <regex>
#include <algorithm>
#include <fstream>
#include <map>
#include <set>
#include "cudd.h"
#include "cuddInt.h"
#include "aag2BDD.cpp"

// Evaluate a BDD with a given assignment
int evaluateBDD(DdManager* mgr, DdNode* f, const std::vector<bool>& assignment) {
    DdNode* node = f;
    bool invert = false;
    // Traverse until reach a constant node
    while (!Cudd_IsConstant(node)) {
        int index = Cudd_NodeReadIndex(node);

        if (static_cast<size_t>(index) >= assignment.size()) {
            std::cerr << "Warning: BDD index " << index << " out of assignment range!" << std::endl;
            return 0; 
        }

        // enter child node according to the assignment
        if (assignment[index])
            node = Cudd_T(node);
        else
            node = Cudd_E(node);
        
        // Handle complemented edge
        if (Cudd_IsComplement(node)) {
            node = Cudd_Regular(node);
            invert = !invert;
        }
    }
    return ((node == Cudd_ReadOne(mgr)) != invert) ? 1 : 0;
}

// Validate if a solution satisfies all constraints
bool validateSolution(const std::vector<bool>& assignment, DdManager* mgr, const std::vector<DdNode*>& constraints) {
    for (DdNode* constraint : constraints) {
        if (evaluateBDD(mgr, constraint, assignment) != 1) {
            return false;
        }
    }
    return true;
}

// Generate a random satisfying assignment
std::vector<bool> generateRandomSolution(DdManager* mgr, DdNode* bdd, int input_num, std::mt19937& rng, int max_attempts = 10000) {
    // If BDD is constant 1 , any assignment works
    if (bdd == Cudd_ReadOne(mgr)) {
        std::vector<bool> solution(input_num);
        std::uniform_int_distribution<int> dist(0, 1);
        for (int i = 0; i < input_num; i++) {
            solution[i] = dist(rng) == 1;
        }
        return solution;
    }
    
    int attempt = 0;
    bool invert = false;

    for (; attempt < max_attempts; attempt++) {
        // Initialize a random assignment
        std::vector<bool> solution(input_num, false);
        // Some variables may not be assigned(don't cares)
        // Use a vector to track assigned variables
        std::vector<bool> is_assigned(input_num, false); 
        DdNode* current = bdd;
        
        // Traverse the BDD to generate a random assignment
        while (!Cudd_IsConstant(current)) {
            int var_index = Cudd_NodeReadIndex(current);
            
            if (var_index < input_num) {
                // Randomly assign the variable 0 or 1
                std::uniform_int_distribution<int> dist(0, 1);
                solution[var_index] = dist(rng) == 1;
                is_assigned[var_index] = true;
            }
            else{
                std::cerr << "Warning: BDD index " << var_index << " out of assignment range!" << std::endl;
                return std::vector<bool>();
            }
            
            // Follow the selected branch
            if (solution[var_index]) {
                current = Cudd_T(current);
            } else {
                current = Cudd_E(current);
            }
            
            // Complemented edge
            if (Cudd_IsComplement(current)) {
                current = Cudd_Regular(current);
                invert = !invert;
            }
        }
        
        // Validate the correctness of the assignment
        bool is_one = (current == Cudd_ReadOne(mgr));
        if (is_one != invert) {
            // randomly assign the unassigned variables
            std::uniform_int_distribution<int> dist(0, 1);
            for (int i = 0; i < input_num; i++) {
                if (!is_assigned[i]) {
                    solution[i] = dist(rng) == 1;
                }
            }
            return solution;
        }
    }
    
    if(attempt >= max_attempts) {
        std::cerr << "Warning: Max attempts reached while generating random solution." << std::endl;
    }
    else{
        std::cerr << "Warning: No satisfying assignment found." << std::endl;
    }
    return std::vector<bool>();
}

// 修改 generateSolutions 函数
std::vector<std::vector<bool>> generateSolutions(AAG2BDD& aag_bdd, int num_solutions, unsigned int seed) {
    std::mt19937 rng(seed);
    std::vector<std::vector<bool>> all_solutions;
    
    // 创建约束合取
    DdNode* constraint = Cudd_ReadOne(aag_bdd.manager);
    if (constraint == nullptr) {
        std::cerr << "Error: Failed to create constant node" << std::endl;
        return all_solutions;
    }
    Cudd_Ref(constraint);
    
    for (DdNode* output : *(aag_bdd.output_nodes)) {
        if (output == nullptr) continue;
        
        DdNode* temp = Cudd_bddAnd(aag_bdd.manager, constraint, output);
        if (temp == nullptr) {
            Cudd_RecursiveDeref(aag_bdd.manager, constraint);
            return all_solutions;
        }
        
        Cudd_Ref(temp);
        Cudd_RecursiveDeref(aag_bdd.manager, constraint);
        constraint = temp;
    }
    
    int target_solutions = num_solutions * 2; 
    std::cout << "Attempting to generate " << target_solutions << " unique solutions..." << std::endl;
    
    // 使用集合来确保唯一性
    std::set<std::vector<bool>> unique_solutions;
    int attempts = 0;
    const int max_attempts = 1000000;  // 设置一个合理的上限
    
    // 尝试生成足够的唯一解决方案
    while (unique_solutions.size() < target_solutions && attempts < max_attempts) {
        attempts++;
        
        // 生成一个随机解
        std::vector<bool> solution = generateRandomSolution(aag_bdd.manager, constraint, aag_bdd.input_num, rng);
        if (solution.empty()) continue;
        
        // 添加到唯一解集合
        if (unique_solutions.insert(solution).second) {
            // 插入成功，说明是一个新解
            if (unique_solutions.size() % 100 == 0 || unique_solutions.size() == target_solutions) {
                std::cout << "Generated " << unique_solutions.size() << " unique solutions so far" << std::endl;
            }
        }
        
        // 如果已经尝试了很多次但进展缓慢，则提前结束
        if (attempts % 10000 == 0 && unique_solutions.size() < attempts / 100) {
            std::cout << "Progress is slow, may be close to exhausting all solutions" << std::endl;
            break;
        }
    }
    
    // 清理约束
    Cudd_RecursiveDeref(aag_bdd.manager, constraint);
    
    // 将集合转换为向量
    all_solutions.assign(unique_solutions.begin(), unique_solutions.end());
    std::cout << "Successfully generated " << all_solutions.size() << " unique solutions" << std::endl;
    
    // 如果生成的解决方案少于请求的数量
    if (all_solutions.size() < num_solutions) {
        std::cerr << "Warning: Could only generate " << all_solutions.size() << " solutions, less than requested " << num_solutions << std::endl;
        return all_solutions;
    }
    
    // 如果生成的解决方案多于请求的数量，随机选择子集
    if (all_solutions.size() > num_solutions) {
        std::cout << "Selecting " << num_solutions << " solutions randomly from pool of " << all_solutions.size() << std::endl;
        
        // 打乱解决方案
        std::shuffle(all_solutions.begin(), all_solutions.end(), rng);
        
        // 只保留需要的数量
        all_solutions.resize(num_solutions);
    }
    
    return all_solutions;
}

// Extract variable name and bit index from a variable name
// Example: "var_2[3]" returns {var_2, 3}
std::pair<std::string, int> parseVarName(const std::string& name) {
    std::regex pattern("([^\\[]+)\\[(\\d+)\\]");
    std::smatch matches;
    
    if (std::regex_search(name, matches, pattern) && matches.size() >= 3) {
        std::string base_name = matches[1].str();
        int bit_index = std::stoi(matches[2].str());
        return {base_name, bit_index};
    }
    
    // No index found, treat as single bit variable
    return {name, 0};
}

// Organize variables into groups and sort them
std::map<std::string, std::vector<int>> organizeVariables(const std::map<int, std::string>& input_names) {
    std::map<std::string, std::vector<int>> var_groups;
    
    // Extract variable groups
    for (const auto& [idx, name] : input_names) {
        auto [base_name, bit_idx] = parseVarName(name);
        var_groups[base_name].push_back(idx);
    }
    
    // Sort each group
    for (auto& [name, idxs] : var_groups) {
        std::sort(idxs.begin(), idxs.end(), [&input_names](int a, int b) {
            auto [name_a, bit_a] = parseVarName(input_names.at(a));
            auto [name_b, bit_b] = parseVarName(input_names.at(b));
            return bit_a > bit_b; // Sort by bit index in descending order
        });
    }
    
    return var_groups;
}

// Convert a vector of boolean values to hexadecimal string
std::string bits_to_hex(const std::vector<bool>& bits) {
    std::stringstream ss;
    
    // 每4位转换为一个十六进制数字
    for (int i = 0; i < (int)bits.size(); i += 4) {
        int value = 0;
        // 从高位到低位处理每个4位组
        for (int j = 0; j < 4 && i + j < (int)bits.size(); j++) {
            if (bits[i + j]) {
                value |= (1 << (3 - j));
            }
        }
        ss << std::hex << value;
    }
    
    return ss.str();
}

// 辅助函数，从变量名中提取基础变量名（例如从"var_0[1]"提取"var_0"）
std::string extractBaseVarName(const std::string& name) {
    // 查找左方括号的位置
    size_t bracket_pos = name.find('[');
    if (bracket_pos != std::string::npos) {
        return name.substr(0, bracket_pos);
    }
    return name; // 如果没有方括号，返回整个名称
}

// 提取位索引（例如从"var_0[1]"提取1）
int extractBitIndex(const std::string& name) {
    size_t open_bracket = name.find('[');
    size_t close_bracket = name.find(']');
    
    if (open_bracket != std::string::npos && close_bracket != std::string::npos && open_bracket < close_bracket) {
        std::string index_str = name.substr(open_bracket + 1, close_bracket - open_bracket - 1);
        try {
            return std::stoi(index_str);
        } catch (...) {
            return -1; // 转换失败
        }
    }
    return -1; // 格式不匹配
}

// 修改后的 generateJson 函数，按照变量名分组
void generateJson(const std::vector<std::vector<bool>>& solutions, const AAG2BDD& aag_bdd, const std::string& output_file) {
    std::ofstream file;
    std::ostream* out;
    
    if (!output_file.empty()) {
        file.open(output_file);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open output file: " << output_file << std::endl;
            out = &std::cout;
        } else {
            out = &file;
        }
    } else {
        out = &std::cout;
    }
    
    // 首先，分析输入变量并按基础名称分组
    std::map<std::string, std::vector<std::pair<int, int>>> var_groups; // 映射：基础变量名 -> [(id, bit_index), ...]
    
    for (int i = 0; i < aag_bdd.input_num; i++) {
        std::string name = aag_bdd.get_input_name(i);
        std::string base_name = extractBaseVarName(name);
        int bit_index = extractBitIndex(name);
        
        var_groups[base_name].push_back(std::make_pair(i, bit_index == -1 ? 0 : bit_index));
    }
    
    // 对每个变量组内的位按索引排序
    for (auto& group : var_groups) {
        std::sort(group.second.begin(), group.second.end(),
                 [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                     return a.second < b.second;
                 });
    }
    
    // 开始生成JSON
    *out << "{\n";
    *out << "  \"assignment_list\": [\n";
    
    for (size_t sol_idx = 0; sol_idx < solutions.size(); sol_idx++) {
        const auto& solution = solutions[sol_idx];
        
        *out << "    [\n";
        
        // 对每个基础变量名生成一个条目
        size_t group_count = 0;
        for (const auto& group : var_groups) {
            const auto& base_name = group.first;
            const auto& bit_indices = group.second;
            
            // 构建该组变量的位向量
            std::vector<bool> group_bits;
            for (const auto& bit_info : bit_indices) {
                int id = bit_info.first;
                group_bits.push_back(solution[id]);
            }
            
            // 输出为十六进制
            *out << "      { \"value\": \"" << bits_to_hex(group_bits) << "\"";
            
            if (group_count < var_groups.size() - 1) {
                *out << "},";
            } else {
                *out << "}";
            }
            *out << "\n";
            
            group_count++;
        }
        
        *out << "    ]";
        if (sol_idx < solutions.size() - 1) {
            *out << ",";
        }
        *out << "\n";
    }
    
    *out << "  ]\n";
    *out << "}\n";
    
    if (file.is_open()) {
        file.close();
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Invalid format. Try: " << argv[0] << " [aag_file] <random_seed> <num_solutions> [output_file]" << std::endl;
        return 1;
    }
    
    std::string aag_file = argv[1];
    unsigned int seed = std::stoul(argv[2]);
    int num_solutions = std::stoi(argv[3]);
    std::string output_file = (argc > 4) ? argv[4] : "";
    
    // Load AAG file and build BDD
    AAG2BDD aag_bdd;
    if (!aag_bdd.Build_BDD(aag_file)) {
        std::cerr << "Failed to build BDD from AAG file: " << aag_file << std::endl;
        return 1;
    }
    
    std::cout << "Successfully built BDD from " << aag_file << std::endl;
    std::cout << "Number of inputs: " << aag_bdd.input_num << std::endl;
    std::cout << "Number of outputs: " << aag_bdd.output_nodes->size() << std::endl;
    
    // Generate solutions
    std::cout << "Generating " << num_solutions << " solutions with seed " << seed << "..." << std::endl;
    std::vector<std::vector<bool>> solutions = generateSolutions(aag_bdd, num_solutions, seed);
    
    if (solutions.empty()) {
        std::cerr << "No solutions were found." << std::endl;
        return 1;
    }
    
    std::cout << "Generated " << solutions.size() << " solutions." << std::endl;
    
    // Output solutions in JSON format
    generateJson(solutions, aag_bdd, output_file);
    
    std::cout << "Done." << std::endl;
    return 0;
}