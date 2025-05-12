#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <regex>
#include "aag2BDD.cpp"

// 辅助函数：利用 BDD 结构对给定赋值进行求值
int evaluateBDD(DdManager* mgr, DdNode* f, const std::vector<bool>& assignment) {
    DdNode* node = f;
    // 当节点无子节点时即为终值节点
    while (!Cudd_IsConstant(node)) {
        int index = Cudd_NodeReadIndex(node);
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

// 从变量名中解析位宽
// 例如: "var_0[10:0]" 返回 11, "var_1" 返回 1
int parseWidthFromName(const std::string& name) {
    // 使用正则表达式匹配 [数字:数字] 格式
    std::regex range_pattern("\\[(\\d+):(\\d+)\\]");
    std::smatch matches;
    
    if (std::regex_search(name, matches, range_pattern) && matches.size() >= 3) {
        int high = std::stoi(matches[1].str());
        int low = std::stoi(matches[2].str());
        return high - low + 1;
    }
    
    return 1; // 默认为单比特
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

    // 选取第一个输出 BDD 作为可行解判断标准
    auto& outputs = aagbdd.getOutputBdds();
    if (outputs.empty()) {
        std::cerr << "没有可用的输出 BDD!" << std::endl;
        return 1;
    }
    DdNode* f = outputs[0];

    // 获取输入变量数目及名称
    auto& var_names = aagbdd.getVarNames();
    int input_num = var_names.size();
    
    // 设置随机数生成器
    std::mt19937 rng(random_seed);
    std::uniform_int_distribution<int> dist(0, 1);

    // 收集满足条件的解
    std::vector<std::vector<bool>> solutions;
    int found = 0;
    int attempts = 0;
    const int max_attempts = 1000000; // 设置最大尝试次数防止无限循环
    
    while (found < num_solutions && attempts < max_attempts) {
        attempts++;
        std::vector<bool> assignment(input_num, false);
        for (int i = 0; i < input_num; i++) {
            assignment[i] = (dist(rng) == 1);
        }
        
        // 对当前赋值进行 BDD 求值
        int res = evaluateBDD(aagbdd.getManager(), f, assignment);
        if (res == 1) {
            solutions.push_back(assignment);
            found++;
        }
    }

    if (found < num_solutions) {
        std::cerr << "警告: 只找到 " << found << " 个解，少于请求的 " << num_solutions << " 个解。" << std::endl;
    }

    // 分析变量名，确定不同变量的基本名称和位索引
    std::map<std::string, std::vector<int>> var_bits_indices; // 存储变量名到位索引的映射
    
    // 分析每个变量名
    for (int i = 0; i < var_names.size(); i++) {
        auto [base_name, bit_index] = parseNameAndIndex(var_names[i]);
        
        // 将这一位添加到对应变量名下
        var_bits_indices[base_name].push_back(i); // i 是位在solutions中的索引
    }

    // 以 JSON 格式输出结果
    std::cout << "{\n  \"assignment_list\": [" << std::endl;
    
    // 分组输出每个解
    for (size_t i = 0; i < solutions.size(); i++) {
        // 添加注释
        std::cout << "    // 解 #" << (i + 1) << std::endl;
        std::cout << "    [" << std::endl;
        
        // 按变量名分组输出
        int var_id = 0;
        for (const auto& [var_name, bit_indices] : var_bits_indices) {
            std::vector<bool> var_bits;
            
            // 收集该变量的所有位
            for (int bit_index : bit_indices) {
                var_bits.push_back(solutions[i][bit_index]);
            }
            
            // 将位值转换为16进制
            std::string hex_value = bits_to_hex(var_bits);
            
            std::cout << "      { \"value\": \"" << hex_value << "\" }";
            if (var_id < var_bits_indices.size() - 1) {
                std::cout << ",";
            }
            std::cout << " // id=" << var_id << "的变量赋值：" << var_name << std::endl;
            
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