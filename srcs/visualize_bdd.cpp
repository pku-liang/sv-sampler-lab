#include <cstdio>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "cudd.h"
#include "cuddInt.h"
#include "aag2BDD.cpp"

// 安全地导出 BDD 为 DOT 格式
void safeExportBDDtoDot(DdManager* manager, DdNode* node, const std::string& filename, 
                       const std::map<int, std::string>& var_names) {
    if (node == NULL) {
        std::cerr << "Cannot export NULL node to " << filename << std::endl;
        return;
    }
    
    // 检查节点数量，如果太大则不导出
    unsigned int node_count = Cudd_DagSize(node);
    if (node_count > 10000) {
        std::cerr << "BDD too large to export: " << node_count << " nodes. Skipping export to " << filename << std::endl;
        return;
    }
    
    FILE* file = fopen(filename.c_str(), "w");
    if (file == nullptr) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }
    
    try {
        // 创建名称数组以便在图中显示变量名
        char** inames = new char*[Cudd_ReadSize(manager)];
        for (int i = 0; i < Cudd_ReadSize(manager); i++) {
            auto it = var_names.find(i);
            std::string name;
            if (it != var_names.end()) {
                name = it->second;
            } else {
                name = "x" + std::to_string(i);
            }
            
            // 使用 new 和 strcpy 替代 strdup
            inames[i] = new char[name.length() + 1];
            strcpy(inames[i], name.c_str());
        }
        
        // 导出 BDD 为 DOT 格式
        Cudd_DumpDot(manager, 1, &node, inames, nullptr, file);
        
        // 清理
        for (int i = 0; i < Cudd_ReadSize(manager); i++) {
            delete[] inames[i];
        }
        delete[] inames;
        
        fclose(file);
        std::cout << "BDD exported to " << filename << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Exception while exporting BDD: " << e.what() << std::endl;
        fclose(file);
    }
}

// 安全地打印 BDD 统计信息
void safePrintBDDStats(DdManager* manager, DdNode* bdd, const std::string& name) {
    std::cout << "BDD Statistics for " << name << ":" << std::endl;
    
    if (bdd == NULL) {
        std::cout << "  NULL BDD node!" << std::endl;
        return;
    }
    
    try {
        unsigned int node_count = Cudd_DagSize(bdd);
        std::cout << "  Number of nodes: " << node_count << std::endl;
        
        // 检查 BDD 是否是常量
        if (Cudd_IsConstant(bdd)) {
            std::cout << "  Is constant: Yes" << std::endl;
            if (bdd == Cudd_ReadOne(manager)) {
                std::cout << "  Constant value: 1 (Always satisfied)" << std::endl;
            } else {
                std::cout << "  Constant value: 0 (Unsatisfiable)" << std::endl;
            }
        } else {
            std::cout << "  Is constant: No" << std::endl;
            
            // 只对较小的 BDD 尝试寻找满足赋值
            if (node_count < 5000) {
                // 尝试寻找一个满足的赋值
                DdNode* minterm = Cudd_bddPickOneMinterm(manager, bdd, NULL, Cudd_ReadSize(manager));
                if (minterm != NULL) {
                    std::cout << "  Found a satisfying assignment" << std::endl;
                    Cudd_RecursiveDeref(manager, minterm);
                } else {
                    std::cout << "  Could not find a satisfying assignment" << std::endl;
                }
            } else {
                std::cout << "  BDD too large, skipping satisfiability check" << std::endl;
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Exception while printing BDD stats: " << e.what() << std::endl;
    }
    
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [aag_file]" << std::endl;
        return 1;
    }
    
    std::string aag_file = argv[1];
    
    // 加载 AAG 文件并构建 BDD
    AAG2BDD aag_bdd;
    if (!aag_bdd.Build_BDD(aag_file)) {
        std::cerr << "Failed to build BDD from AAG file: " << aag_file << std::endl;
        return 1;
    }
    
    std::cout << "BDD built successfully from " << aag_file << std::endl;
    std::cout << "Number of inputs: " << aag_bdd.input_num << std::endl;
    std::cout << "Number of outputs: " << aag_bdd.output_nodes->size() << std::endl << std::endl;
    
    // 创建变量名称映射
    std::map<int, std::string> var_names;
    for (int i = 0; i < aag_bdd.input_num; i++) {
        var_names[i] = aag_bdd.get_input_name(i);
    }
    
    // 打印并导出各个输出节点的统计信息
    for (size_t i = 0; i < aag_bdd.output_nodes->size(); i++) {
        try {
            std::string output_name = aag_bdd.get_output_name(i);
            DdNode* output_node = (*(aag_bdd.output_nodes))[i];
            
            // 打印统计信息
            safePrintBDDStats(aag_bdd.manager, output_node, "Output " + std::to_string(i) + " (" + output_name + ")");
            
            // 导出为 DOT 文件
            std::string dot_file = "run_dir/bdd_output_" + std::to_string(i) + ".dot";
            safeExportBDDtoDot(aag_bdd.manager, output_node, dot_file, var_names);
        } catch (std::exception& e) {
            std::cerr << "Exception while processing output " << i << ": " << e.what() << std::endl;
        }
    }
    
    // 逐渐构建约束合取，并在每一步检查可满足性
    try {
        std::cout << "Analyzing combined constraints..." << std::endl;
        DdNode* combined = Cudd_ReadOne(aag_bdd.manager);
        Cudd_Ref(combined);
        
        bool still_satisfiable = true;
        
        for (size_t i = 0; i < aag_bdd.output_nodes->size() && still_satisfiable; i++) {
            DdNode* output = (*(aag_bdd.output_nodes))[i];
            if (output == NULL) continue;
            
            DdNode* temp = Cudd_bddAnd(aag_bdd.manager, combined, output);
            if (temp == NULL) {
                std::cerr << "Error computing conjunction with constraint " << i << std::endl;
                continue;
            }
            
            // 检查中间结果
            std::cout << "After adding constraint " << i << ":" << std::endl;
            if (temp == Cudd_ReadZero(aag_bdd.manager)) {
                std::cout << "  Combined BDD became unsatisfiable!" << std::endl;
                std::cout << "  This means constraint " << i << " conflicts with previous constraints." << std::endl;
                still_satisfiable = false;
            }
            
            Cudd_Ref(temp);
            Cudd_RecursiveDeref(aag_bdd.manager, combined);
            combined = temp;
        }
        
        // 打印合并后的 BDD 统计信息
        safePrintBDDStats(aag_bdd.manager, combined, "Combined constraints");
        
        // 导出合并后的 BDD，如果它不是太大
        safeExportBDDtoDot(aag_bdd.manager, combined, "run_dir/bdd_combined.dot", var_names);
        
        // 如果 BDD 不是常量 0 且大小合适，尝试找一个解并打印出来
        if (combined != Cudd_ReadZero(aag_bdd.manager) && Cudd_DagSize(combined) < 5000) {
            std::cout << "Trying to find a satisfying assignment..." << std::endl;
            
            // 创建变量数组
            DdNode** vars = new DdNode*[aag_bdd.input_num];
            for (int i = 0; i < aag_bdd.input_num; i++) {
                vars[i] = Cudd_bddIthVar(aag_bdd.manager, i);
            }
            
            // 寻找一个满足赋值
            DdNode* minterm = Cudd_bddPickOneMinterm(aag_bdd.manager, combined, vars, aag_bdd.input_num);
            
            if (minterm != NULL) {
                std::cout << "Found a solution!" << std::endl;
                
                // 提取并按组显示解
                std::map<std::string, std::vector<std::pair<int, bool>>> grouped_solution;
                
                for (int i = 0; i < aag_bdd.input_num; i++) {
                    std::string name = aag_bdd.get_input_name(i);
                    std::string base_name = name;
                    int bit_index = -1;
                    
                    // 提取基本变量名和位索引
                    size_t open_bracket = name.find('[');
                    size_t close_bracket = name.find(']');
                    
                    if (open_bracket != std::string::npos && close_bracket != std::string::npos) {
                        base_name = name.substr(0, open_bracket);
                        std::string index_str = name.substr(open_bracket + 1, close_bracket - open_bracket - 1);
                        try {
                            bit_index = std::stoi(index_str);
                        } catch (...) {
                            bit_index = -1;
                        }
                    }
                    
                    // 确定变量的值
                    bool value = Cudd_bddLeq(aag_bdd.manager, vars[i], minterm);
                    
                    // 按基本变量名分组
                    grouped_solution[base_name].push_back(std::make_pair(bit_index, value));
                }
                
                // 对每个组内的位按索引排序
                for (auto& [base_name, bits] : grouped_solution) {
                    std::sort(bits.begin(), bits.end(), 
                             [](const auto& a, const auto& b) {
                                 return a.first < b.first;
                             });
                }
                
                // 显示分组结果并计算十六进制值
                std::cout << "Solution by variable groups:" << std::endl;
                for (const auto& [base_name, bits] : grouped_solution) {
                    std::string binary = "";
                    for (const auto& [bit_index, value] : bits) {
                        binary += value ? "1" : "0";
                    }
                    
                    // 计算十六进制值
                    std::string hex_value = "0x";
                    for (size_t i = 0; i < binary.length(); i += 4) {
                        std::string nibble = binary.substr(i, std::min((size_t)4, binary.length() - i));
                        while (nibble.length() < 4) {
                            nibble = "0" + nibble;  // 在前面补0
                        }
                        
                        int val = 0;
                        for (char c : nibble) {
                            val = (val << 1) | (c == '1' ? 1 : 0);
                        }
                        
                        hex_value += "0123456789ABCDEF"[val];
                    }
                    
                    std::cout << "  " << base_name << " = " << hex_value << " (binary: " << binary << ")" << std::endl;
                }
                
                Cudd_RecursiveDeref(aag_bdd.manager, minterm);
            } else {
                std::cout << "Could not find a satisfying assignment!" << std::endl;
            }
            
            delete[] vars;
        }
        
        Cudd_RecursiveDeref(aag_bdd.manager, combined);
    } catch (std::exception& e) {
        std::cerr << "Exception while processing combined constraints: " << e.what() << std::endl;
    }
    
    return 0;
}