// filepath: /root/finalProject/COSMOS-BDD-solver/srcs/test_verification.cpp
#include <gtest/gtest.h>
#include "verification.cpp"
#include <memory>

// 创建一个简单的AAG2BDD测试夹具
class VerificationTest : public ::testing::Test {
protected:
    AAG2BDD* converter;
    
    void SetUp() override {
        // 我们可以使用一个简单的示例AAG文件进行测试
        // 假设我们有一个包含基本门的示例文件
        std::string test_file = "../examples/simple_and.aag";
        converter = new AAG2BDD(test_file);
    }
    
    void TearDown() override {
        delete converter;
    }
};

// 测试基本的验证功能
TEST_F(VerificationTest, BasicVerification) {
    // 测试AND门的基本功能 (假设第一个输出是AND门)
    std::vector<int> inputs_00 = {0, 0};
    EXPECT_TRUE(verify_bdd_output(*converter, 0, inputs_00, false));
    
    std::vector<int> inputs_01 = {0, 1};
    EXPECT_TRUE(verify_bdd_output(*converter, 0, inputs_01, false));
    
    std::vector<int> inputs_10 = {1, 0};
    EXPECT_TRUE(verify_bdd_output(*converter, 0, inputs_10, false));
    
    std::vector<int> inputs_11 = {1, 1};
    EXPECT_TRUE(verify_bdd_output(*converter, 0, inputs_11, true));
}

// 测试错误情况下的验证
TEST_F(VerificationTest, FailedVerification) {
    // 测试不匹配的期望值
    std::vector<int> inputs_00 = {0, 0};
    EXPECT_FALSE(verify_bdd_output(*converter, 0, inputs_00, true)); // 错误的期望
    
    std::vector<int> inputs_11 = {1, 1};
    EXPECT_FALSE(verify_bdd_output(*converter, 0, inputs_11, false)); // 错误的期望
}

// 测试多输出情况
TEST_F(VerificationTest, MultipleOutputs) {
    // 假设第二个输出是OR门
    // 注意：这需要您的AAG文件实际有多个输出
    if (converter->output_num > 1) {
        std::vector<int> inputs_00 = {0, 0};
        EXPECT_TRUE(verify_bdd_output(*converter, 1, inputs_00, false));
        
        std::vector<int> inputs_01 = {0, 1};
        EXPECT_TRUE(verify_bdd_output(*converter, 1, inputs_01, true));
        
        std::vector<int> inputs_10 = {1, 0};
        EXPECT_TRUE(verify_bdd_output(*converter, 1, inputs_10, true));
        
        std::vector<int> inputs_11 = {1, 1};
        EXPECT_TRUE(verify_bdd_output(*converter, 1, inputs_11, true));
    }
}

// 测试异常输入情况
TEST_F(VerificationTest, InvalidInputs) {
    // 测试输入向量长度不匹配的情况
    std::vector<int> too_short = {0};
    // 这里我们期望程序不会崩溃，但验证应该失败
    // 注意：这可能需要修改verify_bdd_output函数来处理这种情况
    EXPECT_ANY_THROW(verify_bdd_output(*converter, 0, too_short, false));
    
    // 测试无效的输出索引
    std::vector<int> inputs = {0, 0};
    // 这应该引发异常或至少返回验证失败
    EXPECT_ANY_THROW(verify_bdd_output(*converter, 99, inputs, false));
}

// 模拟AAG2BDD类进行更精确的单元测试
class MockAAG2BDD : public AAG2BDD {
public:
    MockAAG2BDD() : AAG2BDD("") {
        // 设置一个简单的测试环境
        input_num = 2;
        output_num = 1;
        output_names = std::vector<std::string>{"test_output"};
        
        // 初始化CUDD管理器
        manager = Cudd_Init(input_num, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
        
        // 创建输入变量
        input_nodes = new std::vector<DdNode*>;
        for (int i = 0; i < input_num; i++) {
            input_nodes->push_back(Cudd_bddIthVar(manager, i));
        }
        
        // 创建一个AND门作为输出
        output_nodes = new std::vector<DdNode*>;
        DdNode* and_node = Cudd_bddAnd(manager, (*input_nodes)[0], (*input_nodes)[1]);
        Cudd_Ref(and_node);
        output_nodes->push_back(and_node);
    }
    
    ~MockAAG2BDD() {
        // 清理
        for (DdNode* node : *output_nodes) {
            Cudd_RecursiveDeref(manager, node);
        }
        delete output_nodes;
        delete input_nodes;
        Cudd_Quit(manager);
    }
};

// 使用模拟对象的测试
TEST(VerificationMockTest, MockedBDD) {
    MockAAG2BDD mock_converter;
    
    std::vector<int> inputs_00 = {0, 0};
    EXPECT_TRUE(verify_bdd_output(mock_converter, 0, inputs_00, false));
    
    std::vector<int> inputs_11 = {1, 1};
    EXPECT_TRUE(verify_bdd_output(mock_converter, 0, inputs_11, true));
}

// 主测试函数
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}