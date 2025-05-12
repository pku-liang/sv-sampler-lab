#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "cudd.h"
#include "cuddInt.h"
#include <fstream>
#include <sstream>

class AAG2BDD {
private:
    struct AAG {
        int max_idx;
        int input_num;
        int output_num;
        int and_num;
        std::vector<std::pair<int, std::string>> inputs;
        std::vector<std::pair<int, std::string>> outputs;
        std::vector<std::vector<int>> ands;
    };

    AAG aag;
    DdManager* manager;
    std::map<int, DdNode*> var_to_bdd;
    std::vector<DdNode*> output_bdds;
    std::vector<std::string> var_names;

public:
    AAGBDD() : manager(nullptr) {}

    ~AAGBDD() {
        if (manager) {
            for (auto node : output_bdds) {
                Cudd_RecursiveDeref(manager, node);
            }
            Cudd_Quit(manager);
        }
    }

    // load AAG file
    int loadAAG(const std::string& filename) {
        // open file
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return -1;
        }

        // check file format
        std::string type; int t;
        if (file >> type && type != "aag") {
            std::cerr << "Invalid file format: " << filename << ". file format must be .aag!" << std::endl;
            return -1;
        }

        // headers
        file >> aag.max_idx >> aag.input_num >> t >> aag.output_num >> aag.and_num;

        // inputs and outputs
        for (int i = 0; i < aag.input_num; i++) {
            int idx;
            file >> idx;
            aag.inputs.emplace_back(idx, "");
        }
        
        for (int i = 0; i < aag.output_num; i++) {
            int idx;
            file >> idx;
            aag.outputs.emplace_back(idx, "");
        }

        // and gates
        for (int i = 0; i < aag.and_num; i++) {
            int out, in1, in2;
            file >> out >> in1 >> in2;
            aag.ands.push_back({out, in1, in2});
        }

        // input and output names
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            if (line[0] == 'i') { 
                std::istringstream iss(line.substr(1));
                int index;
                std::string name;
                if (iss >> index >> name) {
                    aag.inputs[index].second = name;
                }
            } 
            else if (line[0] == 'o') { 
                std::istringstream iss(line.substr(1));
                int index;
                std::string name;
                if (iss >> index >> name) {
                    aag.outputs[index].second = name;
                }
            }
            else if (line[0] == 'c') break;
        }

        // close file
        std::cout << "File loaded successfully: " << filename << std::endl;
        file.close();
        return 0;
    }

    // initialize BDD manager
    void initBDDManager() {
        if (manager) {
            Cudd_Quit(manager);
        }
        manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
        // optimize: using dynamic reordering provided by CUDD 
        Cudd_AutodynEnable(manager, CUDD_REORDER_SIFT);
    }

    // build BDD
    int buildBDD() {
        if (!manager) {
            std::cerr << "BDD Manager not initialized!" << std::endl;
            return -1;
        }

        // clear previous BDDs
        for (auto node : output_bdds) {
            Cudd_RecursiveDeref(manager, node);
        }
        output_bdds.clear();
        var_to_bdd.clear();
        var_names.resize(aag.input_num);
        
        // inputs
        for (int i = 0; i < aag.input_num; i++) {
            int var_idx = aag.inputs[i].first;
            std::string var_name = aag.inputs[i].second;
            
            // save name
            var_names[i] = var_name;
            
            // create BDD variable
            DdNode* var = Cudd_bddIthVar(manager, i);
            Cudd_Ref(var); 
            var_to_bdd[var_idx] = var;
            
            // inverse variable
            DdNode* not_var = Cudd_Not(var);
            var_to_bdd[var_idx + 1] = not_var;
            
            std::cout << "Created variable " << var_names[i] << " (index: " << var_idx << ")" << std::endl;
        }
        
        // and gates
        for (const auto& and_gate : aag.ands) {
            int out = and_gate[0];
            int in1 = and_gate[1];
            int in2 = and_gate[2];
            
            DdNode* bdd_in1 = var_to_bdd[in1];
            DdNode* bdd_in2 = var_to_bdd[in2];
            
            // create BDD
            DdNode* bdd_out = Cudd_bddAnd(manager, bdd_in1, bdd_in2);
            Cudd_Ref(bdd_out);  
            var_to_bdd[out] = bdd_out;
            
            // inverse BDD
            DdNode* not_bdd_out = Cudd_Not(bdd_out);
            var_to_bdd[out + 1] = not_bdd_out;
        }
        
        // outputs
        for (int i = 0; i < aag.output_num; i++) {
            int out_idx = aag.outputs[i].first;
            // output name is useless, so we can ignore it
            
            if (var_to_bdd.find(out_idx) != var_to_bdd.end()) {
                DdNode* bdd_out = var_to_bdd[out_idx];
                Cudd_Ref(bdd_out); 
                output_bdds.push_back(bdd_out);
            }
        }
        
        return 0;
    }
};

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "cudd.h"
#include "cuddInt.h"
#include <fstream>
#include <sstream>

class AAGBDD {
private:
    // AAG 结构
    struct AAG {
        int max_idx;
        int input_num;
        int output_num;
        int and_num;
        std::vector<std::pair<int, std::string>> inputs;
        std::vector<std::pair<int, std::string>> outputs;
        std::vector<std::vector<int>> ands;
    };

    AAG aag;
    DdManager* manager;
    std::map<int, DdNode*> var_to_bdd;
    std::vector<DdNode*> output_bdds;
    std::vector<std::string> var_names;

public:
    AAGBDD() : manager(nullptr) {}

    ~AAGBDD() {
        // 释放 BDD 资源
        if (manager) {
            // 释放输出 BDD 节点
            for (auto node : output_bdds) {
                Cudd_RecursiveDeref(manager, node);
            }
            // 退出 CUDD 管理器
            Cudd_Quit(manager);
        }
    }

    // 从文件加载 AAG
    int load_aag(const std::string& filename) {
        // 打开文件
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return -1;
        }

        // 检查文件格式
        std::string type; int t;
        if (file >> type && type != "aag") {
            std::cerr << "Invalid file format: " << filename << ". file format must be .aag!" << std::endl;
            return -1;
        }

        // 读取头部
        file >> aag.max_idx >> aag.input_num >> t >> aag.output_num >> aag.and_num;

        // 读取输入和输出
        for (int i = 0; i < aag.input_num; i++) {
            int idx;
            file >> idx;
            aag.inputs.emplace_back(idx, "");
        }
        
        for (int i = 0; i < aag.output_num; i++) {
            int idx;
            file >> idx;
            aag.outputs.emplace_back(idx, "");
        }

        // 读取 AND 门
        for (int i = 0; i < aag.and_num; i++) {
            int out, in1, in2;
            file >> out >> in1 >> in2;
            aag.ands.push_back({out, in1, in2});
        }

        // 读取名称
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            if (line[0] == 'i') { 
                std::istringstream iss(line.substr(1));
                int index;
                std::string name;
                if (iss >> index >> name) {
                    aag.inputs[index].second = name;
                }
            } 
            else if (line[0] == 'o') { 
                std::istringstream iss(line.substr(1));
                int index;
                std::string name;
                if (iss >> index >> name) {
                    aag.outputs[index].second = name;
                }
            }
            else if (line[0] == 'c') break;
        }

        // 关闭文件
        std::cout << "File loaded successfully: " << filename << std::endl;
        file.close();
        return 0;
    }

    // 初始化 BDD 管理器
    void initBDDManager() {
        if (manager) {
            Cudd_Quit(manager);
        }
        manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
        // 启用动态重排序优化 BDD
        Cudd_AutodynEnable(manager, CUDD_REORDER_SIFT);
    }

    // 构建 BDD
    int buildBDD() {
        if (!manager) {
            std::cerr << "BDD Manager not initialized!" << std::endl;
            return -1;
        }

        // 清理之前的 BDD 资源
        for (auto node : output_bdds) {
            Cudd_RecursiveDeref(manager, node);
        }
        output_bdds.clear();
        var_to_bdd.clear();
        var_names.resize(aag.input_num);
        
        // 1. 为输入变量创建 BDD 变量
        for (int i = 0; i < aag.input_num; i++) {
            int var_idx = aag.inputs[i].first;
            std::string var_name = aag.inputs[i].second;
            
            // 保存变量名
            var_names[i] = var_name.empty() ? "var_" + std::to_string(i) : var_name;
            
            // 创建 BDD 变量
            DdNode* var = Cudd_bddIthVar(manager, i);
            Cudd_Ref(var);  // 增加引用计数
            var_to_bdd[var_idx] = var;
            
            // 存储变量的反相形式
            DdNode* not_var = Cudd_Not(var);
            var_to_bdd[var_idx + 1] = not_var;
            
            std::cout << "Created variable " << var_names[i] << " (index: " << var_idx << ")" << std::endl;
        }
        
        // 2. 处理 AND 门
        for (const auto& and_gate : aag.ands) {
            int out = and_gate[0];
            int in1 = and_gate[1];
            int in2 = and_gate[2];
            
            // 获取输入对应的 BDD
            DdNode* bdd_in1 = var_to_bdd[in1];
            DdNode* bdd_in2 = var_to_bdd[in2];
            
            // 创建 AND 门的 BDD
            DdNode* bdd_out = Cudd_bddAnd(manager, bdd_in1, bdd_in2);
            Cudd_Ref(bdd_out);  // 增加引用计数
            var_to_bdd[out] = bdd_out;
            
            // 存储输出的反相形式
            DdNode* not_bdd_out = Cudd_Not(bdd_out);
            var_to_bdd[out + 1] = not_bdd_out;
        }
        
        // 3. 处理输出
        for (int i = 0; i < aag.output_num; i++) {
            int out_idx = aag.outputs[i].first;
            std::string out_name = aag.outputs[i].second;
            
            if (var_to_bdd.find(out_idx) != var_to_bdd.end()) {
                DdNode* bdd_out = var_to_bdd[out_idx];
                Cudd_Ref(bdd_out);  // 引用计数
                output_bdds.push_back(bdd_out);
                
                // 使用输出名称
                std::cout << "Output " << i << ": " 
                          << (out_name.empty() ? "out_" + std::to_string(i) : out_name)
                          << " (index: " << out_idx << ")" << std::endl;
            }
        }
        
        return 0;
    }

    // 导出 BDD 到 DOT 格式文件
    void exportBDDs(const std::string& prefix = "") {
        if (!manager || output_bdds.empty()) {
            std::cerr << "No BDDs to export!" << std::endl;
            return;
        }

        // 为 CUDD 设置变量名
        for (int i = 0; i < var_names.size(); i++) {
            Cudd_SetVarName(manager, i, const_cast<char*>(var_names[i].c_str()));
        }

        // 导出每个输出的 BDD
        for (int i = 0; i < output_bdds.size(); i++) {
            std::string out_name = aag.outputs[i].second.empty() ? 
                                  "out_" + std::to_string(i) : aag.outputs[i].second;
            
            std::string filename = prefix + out_name + ".dot";
            FILE* file = fopen(filename.c_str(), "w");
            if (file) {
                // 导出为 DOT 格式
                Cudd_DumpDot(manager, 1, &output_bdds[i], NULL, NULL, file);
                fclose(file);
                std::cout << "BDD exported to: " << filename << std::endl;
            } else {
                std::cerr << "Failed to open file for writing: " << filename << std::endl;
            }
        }
    }

    // 打印 BDD 统计信息
    void printBDDStats() {
        if (!manager) {
            std::cerr << "BDD Manager not initialized!" << std::endl;
            return;
        }

        std::cout << "\nBDD Statistics:" << std::endl;
        std::cout << "----------------" << std::endl;
        std::cout << "Total nodes: " << Cudd_ReadNodeCount(manager) << std::endl;
        std::cout << "Peak nodes: " << Cudd_ReadPeakNodeCount(manager) << std::endl;
        
        for (int i = 0; i < output_bdds.size(); i++) {
            std::string out_name = aag.outputs[i].second.empty() ? 
                                 "out_" + std::to_string(i) : aag.outputs[i].second;
            
            std::cout << "Output " << i << " (" << out_name << "):" << std::endl;
            std::cout << "  - Node count: " << Cudd_DagSize(output_bdds[i]) << std::endl;
            
            // 检查输出是否为恒定值
            if (output_bdds[i] == Cudd_ReadOne(manager)) {
                std::cout << "  - Constant: TRUE" << std::endl;
            } else if (output_bdds[i] == Cudd_ReadLogicZero(manager)) {
                std::cout << "  - Constant: FALSE" << std::endl;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Syntax error. Try " << argv[0] << " <aag_file>" << std::endl;
        return 1;
    }

    AAG2BDD aag2bdd;
    
    if (aagbdd.load_aag(argv[1]) != 0) {
        std::cerr << "Failed to load AAG file: " << argv[1] << std::endl;
        return 1;
    }
    
    aagbdd.initBDDManager();
    
    if (aagbdd.buildBDD() != 0) {
        std::cerr << "Failed to build BDD" << std::endl;
        return 1;
    }
    
    return 0;
}