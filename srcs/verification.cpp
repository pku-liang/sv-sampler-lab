#include <iostream>
#include <string>
#include "../srcs/aag2BDD.cpp"

// 简单的BDD验证函数，检查给定输入组合的输出是否符合预期
bool verify_bdd_output(AAG2BDD& converter, int output_idx, 
                     const std::vector<int>& input_values, 
                     bool expected_result) {
    DdManager* mgr = converter.manager;
    DdNode* output = (*converter.output_nodes)[output_idx];
    
    // 为每个输入设置值
    int num_inputs = converter.input_num;
    std::vector<DdNode*> vars(num_inputs);
    std::vector<int> values(num_inputs);
    
    for (int i = 0; i < num_inputs; i++) {
        vars[i] = Cudd_bddIthVar(mgr, i);
        values[i] = input_values[i];
    }
    
    // 使用Cudd_Eval函数来评估在给定输入下的输出
    DdNode* result = Cudd_Eval(mgr, output, values.data());
    bool actual_result = (result == Cudd_ReadOne(mgr));
    
    std::cout << "验证输出 " << converter.get_output_name(output_idx) 
              << " 在输入 [";
    for (int i = 0; i < num_inputs; i++) {
        std::cout << input_values[i];
        if (i < num_inputs - 1) std::cout << ",";
    }
    std::cout << "]: ";
    
    if (actual_result == expected_result) {
        std::cout << "通过" << std::endl;
        return true;
    } else {
        std::cout << "失败 (期望: " << expected_result 
                  << ", 实际: " << actual_result << ")" << std::endl;
        return false;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "使用方法: " << argv[0] << " <aag文件路径>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    AAG2BDD converter(filename);
    
    // 验证示例：这些测试用例需要根据您的AAG文件内容调整
    // 以下是示例，您需要根据实际电路替换输入值和期望结果
    
    // 例子：测试AND门 (假设第一个输出是两个输入的AND)
    std::vector<int> test1 = {0, 0}; // 两个输入都为0
    verify_bdd_output(converter, 0, test1, false);
    
    // 正确的测试用例
    std::vector<int> test2 = {1, 1, 0}; // a=1, b=1, cin=0
    verify_bdd_output(converter, 0, test2, false); // sum应该为0，不是1
    
    // 可以添加更多测试用例
    
    return 0;
}