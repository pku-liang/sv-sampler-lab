#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <regex>
#include <algorithm>
#include "aag2BDD.cpp"

// 辅助函数：利用 BDD 结构对给定赋值进行求值
int evaluateBDD(DdManager* mgr, DdNode* f, const std::vector<bool>& assignment) {
    DdNode* node = f;
    // 当节点无子节点时即为终值节点
    while (!Cudd_IsConstant(node)) {
        int index = Cudd_NodeReadIndex(node);
        // 确保索引在assignment范围内
        if (index >= assignment.size()) {
            std::cerr << "警告: BDD索引 " << index << " 超出赋值范围!" << std::endl;
            return 0; // 返回假作为默认值
        }
        // 按照变量 index 取值
        if (assignment[index])
            node = Cudd_T(node);
        else
            node = Cudd_E(node);
    }
    return (node == Cudd_ReadOne(mgr)) ? 1 : 0;
}

// 将一组布尔值（代表一个多位变量）转换为16进制字符串
// 例如输入 {1,1,0,1,1,1,0,1,0,0,1} 返回 "6e9"
std::string bits_to_hex(const std::vector<bool>& bits) {
    std::stringstream ss;
    
    // 处理每4位为一组
    int bitCount = bits.size();
    int padBits = (4 - (bitCount % 4)) % 4; // 需要补0的位数，使总位数为4的倍数
    
    // 创建一个临时向量存储所有位（包括前导0）
    std::vector<bool> paddedBits(padBits, false);
    paddedBits.insert(paddedBits.end(), bits.begin(), bits.end());
    
    // 每4位转换为一个16进制数字
    for (size_t i = 0; i < paddedBits.size(); i += 4) {
        int value = 0;
        for (size_t j = 0; j < 4 && (i + j) < paddedBits.size(); j++) {
            if (paddedBits[i + j]) {
                value |= (1 << (3 - j));
            }
        }
        ss << std::hex << value;
    }
    
    return ss.str();
}

// 从变量名中提取基本名称和位索引
// 例如: "var_2[2]" 返回 {var_2, 2}
std::pair<std::string, int> parseNameAndIndex(const std::string& name) {
    // 提取基本变量名和索引
    std::regex index_pattern("([^\\[]+)\\[(\\d+)\\]");
    std::smatch matches;
    
    if (std::regex_search(name, matches, index_pattern) && matches.size() >= 3) {
        std::string base_name = matches[1].str();
        int index = std::stoi(matches[2].str());
        return {base_name, index};
    }
    
    // 没有索引，默认为单比特变量
    return {name, 0};
}

// 验证解是否满足所有约束条件
bool validateSolution(const std::vector<bool>& assignment, DdManager* mgr, const std::vector<DdNode*>& outputs) {
    // 检查所有输出 BDD 都为 1
    for (DdNode* f : outputs) {
        if (evaluateBDD(mgr, f, assignment) != 1) {
            return false;
        }
    }
    return true;
}

// 使用更加强大的搜索策略寻找解
std::vector<std::vector<bool>> findSolutions(AAGBDD& aagbdd, unsigned int seed, int num_solutions, int max_attempts) {
    auto& outputs = aagbdd.getOutputBdds();
    int input_num = aagbdd.getVarNames().size();
    DdManager* mgr = aagbdd.getManager();
    
    // 设置随机数生成器
    std::mt19937 rng(seed);
    std::vector<std::vector<bool>> solutions;
    
    // 阶段1：纯随机搜索
    int random_attempts = std::min(max_attempts / 5, 100000); // 尝试更多纯随机搜索
    
    std::cerr << "尝试纯随机生成解...（约束数：" << outputs.size() << "）" << std::endl;
    for (int attempt = 0; attempt < random_attempts && solutions.size() < num_solutions; attempt++) {
        std::vector<bool> assignment(input_num, false);
        for (int i = 0; i < input_num; i++) {
            assignment[i] = (rng() % 2 == 1);
        }
        
        if (validateSolution(assignment, mgr, outputs)) {
            if (std::find(solutions.begin(), solutions.end(), assignment) == solutions.end()) {
                solutions.push_back(assignment);
                std::cerr << "找到解 #" << solutions.size() << " (纯随机)" << std::endl;
            }
        }
        
        // 每10000次尝试输出一次进度
        if (attempt % 10000 == 0 && attempt > 0) {
            std::cerr << "完成 " << attempt << " 次纯随机尝试，找到 " << solutions.size() << " 个解" << std::endl;
        }
    }
    
    // 阶段2：增量式搜索
    if (solutions.size() < num_solutions) {
        std::cerr << "纯随机搜索仅找到 " << solutions.size() << " 个解，尝试增量式搜索..." << std::endl;
        
        int remaining_attempts = max_attempts - random_attempts;
        int iterations_per_solution = std::max(1, remaining_attempts / 10); // 增加迭代次数

        // 多种起点策略
        std::vector<std::vector<bool>> starting_points;
        
        // 已有解作为起点
        if (!solutions.empty()) {
            for (const auto& sol : solutions) {
                starting_points.push_back(sol);
            }
        }
        
        // 增加一些新的随机起点
        for (int i = 0; i < 5; i++) {
            std::vector<bool> random_point(input_num, false);
            for (int j = 0; j < input_num; j++) {
                random_point[j] = (rng() % 2 == 1);
            }
            starting_points.push_back(random_point);
        }
        
        // 从每个起点开始局部搜索
        for (const auto& start_point : starting_points) {
            if (solutions.size() >= num_solutions) break;
            
            std::vector<bool> current = start_point;
            
            // 做多轮变异，每轮变异率不同
            for (int round = 0; round < 3 && solutions.size() < num_solutions; round++) {
                double base_mutation_rate;
                switch (round) {
                    case 0: base_mutation_rate = 0.05; break; // 小变异
                    case 1: base_mutation_rate = 0.2; break;  // 中等变异
                    case 2: base_mutation_rate = 0.4; break;  // 大变异
                }
                
                for (int iter = 0; iter < iterations_per_solution && solutions.size() < num_solutions; iter++) {
                    // 变异当前解的一部分位
                    std::vector<bool> new_assignment = current;
                    
                    // 随机选择变异率
                    double mutation_rate = base_mutation_rate * (0.5 + rng() % 1000 / 1000.0);
                    int mutations = std::max(1, static_cast<int>(input_num * mutation_rate));
                    
                    // 执行变异
                    for (int i = 0; i < mutations; i++) {
                        int pos = rng() % input_num;
                        new_assignment[pos] = !new_assignment[pos];
                    }
                    
                    if (validateSolution(new_assignment, mgr, outputs)) {
                        if (std::find(solutions.begin(), solutions.end(), new_assignment) == solutions.end()) {
                            solutions.push_back(new_assignment);
                            current = new_assignment; // 更新当前解
                            std::cerr << "找到解 #" << solutions.size() << " (增量搜索 round " << round << ")" << std::endl;
                        }
                    }
                    
                    // 如果连续多次未找到解，可能是陷入局部最优，尝试进行更大的变异
                    if (iter % (iterations_per_solution / 10) == 0) {
                        // 创建临时变异解
                        std::vector<bool> temp_assignment = current;
                        int big_mutations = input_num / 5; // 20%的比特发生变异
                        
                        for (int i = 0; i < big_mutations; i++) {
                            int pos = rng() % input_num;
                            temp_assignment[pos] = !temp_assignment[pos];
                        }
                        
                        if (validateSolution(temp_assignment, mgr, outputs)) {
                            if (std::find(solutions.begin(), solutions.end(), temp_assignment) == solutions.end()) {
                                solutions.push_back(temp_assignment);
                                current = temp_assignment; // 更新当前解
                                std::cerr << "找到解 #" << solutions.size() << " (大变异跳跃)" << std::endl;
                            }
                        }
                    }
                }
                
                // 每轮结束后输出进度
                std::cerr << "完成变异轮次 " << round << "，当前共找到 " << solutions.size() << " 个解" << std::endl;
            }
        }
        
        // 阶段3：如果还不够解，尝试智能随机搜索
        if (solutions.size() < num_solutions) {
            std::cerr << "增量式搜索后共找到 " << solutions.size() << " 个解，尝试智能随机搜索..." << std::endl;
            
            int remaining = num_solutions - solutions.size();
            int iterations_left = max_attempts / 10; // 最后尝试次数
            
            for (int iter = 0; iter < iterations_left && solutions.size() < num_solutions; iter++) {
                // 学习已知解的模式
                std::vector<double> bit_probabilities(input_num, 0.5);
                
                if (!solutions.empty()) {
                    // 基于现有解调整每位的概率
                    for (int i = 0; i < input_num; i++) {
                        int count_ones = 0;
                        for (const auto& sol : solutions) {
                            if (sol[i]) count_ones++;
                        }
                        bit_probabilities[i] = std::max(0.1, std::min(0.9, static_cast<double>(count_ones) / solutions.size()));
                    }
                }
                
                // 使用学习到的概率生成新解
                std::vector<bool> assignment(input_num, false);
                for (int i = 0; i < input_num; i++) {
                    std::bernoulli_distribution dist(bit_probabilities[i]);
                    assignment[i] = dist(rng);
                }
                
                if (validateSolution(assignment, mgr, outputs)) {
                    if (std::find(solutions.begin(), solutions.end(), assignment) == solutions.end()) {
                        solutions.push_back(assignment);
                        std::cerr << "找到解 #" << solutions.size() << " (智能随机)" << std::endl;
                    }
                }
                
                // 每1000次尝试输出一次进度
                if (iter % 1000 == 0 && iter > 0) {
                    std::cerr << "完成 " << iter << " 次智能随机尝试，找到 " << solutions.size() << " 个解" << std::endl;
                }
            }
        }
    }
    
    if (solutions.empty()) {
        std::cerr << "警告: 未能找到任何满足所有约束的解！" << std::endl;
    } else if (solutions.size() < num_solutions) {
        std::cerr << "警告: 只找到 " << solutions.size() << " 个解，少于请求的 " << num_solutions << " 个解。" << std::endl;
    }
    
    return solutions;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "用法: " << argv[0] << " <aag_file> <random_seed> <num_solutions>" << std::endl;
        return 1;
    }

    std::string aag_file = argv[1];
    unsigned int random_seed = std::stoul(argv[2]);
    int num_solutions = std::stoi(argv[3]);

    AAGBDD aagbdd;
    if (aagbdd.load_aag(aag_file) != 0) {
        std::cerr << "加载 AAG 文件失败: " << aag_file << std::endl;
        return 1;
    }

    aagbdd.initBDDManager();
    if (aagbdd.buildBDD() != 0) {
        std::cerr << "构建 BDD 失败" << std::endl;
        return 1;
    }

    // 获取输入变量数目及名称
    auto& var_names = aagbdd.getVarNames();
    int input_num = var_names.size();
    
    // 使用改进的搜索算法查找解，大幅增加尝试次数
    std::vector<std::vector<bool>> solutions = findSolutions(aagbdd, random_seed, num_solutions, 10000000);
    
    // 分析变量名，确定不同变量的基本名称和位索引
    std::map<std::string, std::vector<int>> var_bits_indices; // 存储变量名到位索引的映射
    std::map<std::string, std::vector<int>> var_bits_values; // 存储每个位的实际值（在变量名中的索引）
    
    // 存储按照顺序排列的变量名
    std::vector<std::string> var_names_in_order;
    for (int i = 0; i < var_names.size(); i++) {
        auto [base_name, bit_index] = parseNameAndIndex(var_names[i]);
        
        // 保证按顺序收集变量名
        if (std::find(var_names_in_order.begin(), var_names_in_order.end(), base_name) == var_names_in_order.end()) {
            var_names_in_order.push_back(base_name);
        }
        var_bits_indices[base_name].push_back(i);
        var_bits_values[base_name].push_back(bit_index); // 保存位在变量中的索引值
    }
    
    // 确保每个变量的位是按照从高位到低位排序的
    for (const std::string& var_name : var_names_in_order) {
        auto& indices = var_bits_indices[var_name];
        auto& bit_values = var_bits_values[var_name];
        
        // 创建一个临时向量来存储索引和对应的位值
        std::vector<std::pair<int, int>> idx_value_pairs;
        for (size_t i = 0; i < indices.size(); i++) {
            idx_value_pairs.push_back({indices[i], bit_values[i]});
        }
        
        // 根据位值（从大到小）排序
        std::sort(idx_value_pairs.begin(), idx_value_pairs.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // 更新排序后的索引
        for (size_t i = 0; i < indices.size(); i++) {
            indices[i] = idx_value_pairs[i].first;
        }
    }
    
    // 输出 JSON 格式
    std::cout << "{\n  \"assignment_list\": [" << std::endl;
    
    // 按照按顺序排列的变量名输出解
    for (size_t i = 0; i < solutions.size(); i++) {
        std::cout << "    [" << std::endl;
        
        int var_id = 0;
        for (const std::string& var_name : var_names_in_order) {
            std::vector<bool> var_bits;
            
            // 收集该变量的所有位
            for (int bit_index : var_bits_indices[var_name]) {
                var_bits.push_back(solutions[i][bit_index]);
            }
            
            // 将位值转换为16进制
            std::string hex_value = bits_to_hex(var_bits);
            
            std::cout << "      { \"value\": \"" << hex_value << "\" }";
            if (var_id < var_names_in_order.size() - 1) {
                std::cout << ",";
            }
            std::cout << std::endl;
            
            var_id++;
        }
        
        std::cout << "    ]";
        if (i < solutions.size() - 1) {
            std::cout << ",";
        }
        std::cout << std::endl;
    }
    
    std::cout << "  ]\n}" << std::endl;

    return 0;
}