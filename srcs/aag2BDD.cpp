#include <cstdio>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <cassert>
#include "cudd.h"
#include "cuddInt.h"

class AAG2BDD {
public:
    DdManager *manager;
    std::vector<DdNode*> *output_nodes;
    int max_idx, input_num, latch_num, output_num, and_num;
    std::vector<DdNode*> nodes; 
    std::map<int, std::string> input_names;  // 存储输入节点名称（索引到名称）
    std::map<int, std::string> output_names; // 存储输出节点名称
    std::map<int, std::string> node_names;   // 新增：存储所有节点的名称（包括中间节点）

    AAG2BDD() {
        manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
        Cudd_AutodynEnable(manager, CUDD_REORDER_SIFT); // 启用动态重排序
        output_nodes = new std::vector<DdNode*>();
        max_idx = 0;
        input_num = 0;
        latch_num = 0;
        output_num = 0;
        and_num = 0;
    }

    AAG2BDD(const std::string& filename) {
        manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
        Cudd_AutodynEnable(manager, CUDD_REORDER_SIFT); // 启用动态重排序
        output_nodes = new std::vector<DdNode*>();
        Build_BDD(filename);
    }

    ~AAG2BDD() {
        for (DdNode* node : *output_nodes) {
            if (node != nullptr) {
                Cudd_RecursiveDeref(manager, node);
            }
        }
        delete output_nodes;
        
        for (DdNode* node : nodes) {
            if (node != nullptr) {
                Cudd_RecursiveDeref(manager, node);
            }
        }
        
        Cudd_Quit(manager);
    }
    
    // Load AAG file and build BDD
    bool Build_BDD(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "无法打开文件: " << filename << std::endl;
            return false;
        }
        
        std::string line;
        std::getline(file, line);
        std::istringstream header(line);
        std::string format;
        header >> format >> max_idx >> input_num >> latch_num >> output_num >> and_num;
        
        if (format != "aag") {
            std::cerr << "无效的 AAG 文件格式" << std::endl;
            return false;
        }

        std::cout << "解析 AAG 文件: max_idx=" << max_idx << ", inputs=" << input_num
                  << ", latches=" << latch_num << ", outputs=" << output_num
                  << ", AND gates=" << and_num << std::endl;
        
        nodes.resize(max_idx + 1, nullptr);
        
        // 常数节点（AIG 中的 0，对应 BDD 中的常数 1）
        nodes[0] = Cudd_ReadOne(manager); 
        Cudd_Ref(nodes[0]);
        node_names[0] = "CONST_1";
        
        // 输入节点
        for (int i = 0; i < input_num; i++) {
            int idx;
            if (!(file >> idx)) {
                std::cerr << "读取输入索引失败" << std::endl;
                return false;
            }
            if (idx % 2 != 0 || idx < 2) {
                std::cerr << "无效输入索引: " << idx << std::endl;
                return false;
            }
            DdNode* node = Cudd_bddIthVar(manager, i);
            Cudd_Ref(node);
            nodes[idx / 2] = node;
            std::cout << "输入 " << i << ": 索引=" << idx << ", BDD 变量=" << i << std::endl;
        }
        
        // 输出索引
        std::vector<int> output_idxs(output_num);
        for (int i = 0; i < output_num; i++) {
            if (!(file >> output_idxs[i])) {
                std::cerr << "读取输出索引失败" << std::endl;
                return false;
            }
        }
        
        // AND 门
        for (int i = 0; i < and_num; i++) {
            int out, in1, in2;
            if (!(file >> out >> in1 >> in2)) {
                std::cerr << "读取 AND 门失败" << std::endl;
                return false;
            }
            if (out % 2 != 0 || out < 2) {
                std::cerr << "无效 AND 门输出索引: " << out << std::endl;
                return false;
            }
            
            // 输入 1
            DdNode *In1 = nullptr;
            if (in1 == 0) {
                In1 = Cudd_ReadOne(manager); // AIG 0 -> BDD 1
                Cudd_Ref(In1);
            } else if (in1 == 1) {
                In1 = Cudd_ReadLogicZero(manager); // AIG 1 -> BDD 0
                Cudd_Ref(In1);
            } else {
                DdNode* base_node = nodes[in1 >> 1];
                if (base_node == nullptr) {
                    std::cerr << "AND 门输入 1 索引 " << in1 << " 未定义" << std::endl;
                    return false;
                }
                In1 = (in1 & 1) ? Cudd_Not(base_node) : base_node;
                Cudd_Ref(In1);
            }
            
            // 输入 2
            DdNode *In2 = nullptr;
            if (in2 == 0) {
                In2 = Cudd_ReadOne(manager);
                Cudd_Ref(In2);
            } else if (in2 == 1) {
                In2 = Cudd_ReadLogicZero(manager);
                Cudd_Ref(In2);
            } else {
                DdNode* base_node = nodes[in2 >> 1];
                if (base_node == nullptr) {
                    std::cerr << "AND 门输入 2 索引 " << in2 << " 未定义" << std::endl;
                    return false;
                }
                In2 = (in2 & 1) ? Cudd_Not(base_node) : base_node;
                Cudd_Ref(In2);
            }
            
            // 计算 AND 门
            DdNode *Out = Cudd_bddAnd(manager, In1, In2);
            if (Out == nullptr) {
                std::cerr << "AND 门计算失败: out=" << out << std::endl;
                Cudd_RecursiveDeref(manager, In1);
                Cudd_RecursiveDeref(manager, In2);
                return false;
            }
            Cudd_Ref(Out);
            nodes[out >> 1] = Out;
            node_names[out >> 1] = "AND_" + std::to_string(out >> 1); // 为 AND 节点命名
            
            // 清理临时节点
            Cudd_RecursiveDeref(manager, In1);
            Cudd_RecursiveDeref(manager, In2);
            
            std::cout << "AND 门 " << i << ": out=" << out << ", in1=" << in1 << ", in2=" << in2 << std::endl;
        }
        
        // 输出节点
        for (int i = 0; i < output_num; i++) {
            int out_idx = output_idxs[i];
            DdNode *out_node = nullptr;
            
            if (out_idx == 0) {
                out_node = Cudd_ReadOne(manager); // AIG 0 -> BDD 1
            } else if (out_idx == 1) {
                out_node = Cudd_ReadLogicZero(manager); // AIG 1 -> BDD 0
            } else {
                DdNode* base_node = nodes[out_idx >> 1];
                if (base_node == nullptr) {
                    std::cerr << "输出节点索引 " << out_idx << " 未定义" << std::endl;
                    return false;
                }
                out_node = (out_idx & 1) ? Cudd_Not(base_node) : base_node;
            }
            
            Cudd_Ref(out_node);
            output_nodes->push_back(out_node);
            std::cout << "输出 " << i << ": 索引=" << out_idx << ", 节点="
                      << (out_node == Cudd_ReadOne(manager) ? "常数1" :
                          out_node == Cudd_ReadLogicZero(manager) ? "常数0" : "计算节点")
                      << std::endl;
        }
        
        // 解析符号表
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            if (line[0] == 'i') {
                std::istringstream iss(line.substr(1));
                int index;
                std::string name;
                iss >> index >> name;
                input_names[index] = name;
                node_names[index + 1] = name; // 输入节点索引为 2*index+2
                std::cout << "输入名称: i" << index << " -> " << name << std::endl;
            } else if (line[0] == 'o') {
                std::istringstream iss(line.substr(1));
                int index;
                std::string name;
                iss >> index >> name;
                output_names[index] = name;
                node_names[output_idxs[index] >> 1] = name; // 输出节点名称
                std::cout << "输出名称: o" << index << " -> " << name << std::endl;
            }
        }
        
        file.close();
        return true;
    }
    
    // 获取节点名称（输入、输出或中间节点）
    std::string get_node_name(int index) const {
        auto it = node_names.find(index);
        if (it != node_names.end()) {
            return it->second;
        }
        return "node_" + std::to_string(index);
    }
    
    // 获取输入名称
    std::string get_input_name(int index) const {
        auto it = input_names.find(index);
        if (it != input_names.end()) {
            return it->second;
        }
        std::cerr << "未找到输入名称，索引: " << index << std::endl;
        return "input_" + std::to_string(index);
    }
    
    // 获取输出名称
    std::string get_output_name(int index) const {
        auto it = output_names.find(index);
        if (it != output_names.end()) {
            return it->second;
        }
        std::cerr << "未找到输出名称，索引: " << index << std::endl;
        return "output_" + std::to_string(index);
    }
    
    // 获取输出节点数量
    int GetOutputCount() const {
        return output_nodes->size();
    }
};