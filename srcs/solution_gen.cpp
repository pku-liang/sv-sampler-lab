#include <cstdio>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <cassert>
#include <random>
#include <iomanip>
#include <regex>
#include <algorithm>
#include <set>
#include <quadmath.h>

#include "nlohmann/json.hpp"
#include "cudd.h"
#include "cuddInt.h"
using json = nlohmann::json;
using namespace std;

// procedure:
// 1. convert the AAG to BDD
//     (1) read the AAG file
//     (2) inputs: use Cudd_bddIthVar to create BDD variables
//     (3) ands: use Cudd_bddAnd to create BDD nodes
//     (4) outputs: there is only one output, no need to deal with it
//     (5) names: create a map to from BDD variable index to its name

// 2. generate random solutions
//    (1) calculate complement arc number of all nodes by dynamic programming
//    (2) generate random solutions using dfs:
//        entering the child node according to probability by the number of complement arcs
//        to make sure finally generate a route with odd complement arcs
//    (3) for don't care variables, randomly assign them

// 3. output the solutions: using nlohmann/json to output the solutions in certain format:
//        {
//        "assignment_list": [
//            // 第一组解
//            [
//            { "value": "2fd3d29"}, // id=0的变量赋值
//            { "value": "a0a1"}, // id=1的变量赋值
//            ...
//            ],
//            // 第二组解
//            [
//            ...
//            ]
//        ]
//        }

// 4. clean up

class BDD_Solver {
    public:
        string input_file;
        string output_file;
        int random_seed;
        int solution_num;

        DdManager *manager;
        DdNode *out_node;
        vector<DdNode*> nodes; // map from AAG index to BDD node

        // map from BDD index to its name
        // name format: var_x[y] -> so record int x and y is ok
        vector<pair<int, int>> idx_to_name;

        // aag config
        int max_idx, input_num, latch_num, output_num, and_num, ori_input_num;
        vector<int> idx_to_len;// the length of each original input variable

        // record path number from current node to 1-th node with odd or even complement arcs
        // 0: odd_cnt, 1: even_cnt
        map<DdNode*, pair<__float128, __float128>> dp;

        // random number generator
        std::mt19937 rng;

        // solution list
        vector<vector<bool>> solutions;
        vector<vector<vector<bool>>> reshaped_solutions;

        BDD_Solver(const string& input, const string& output, int seed, int num_solutions) 
            : input_file(input), output_file(output), random_seed(seed), solution_num(num_solutions) {
            
            rng = std::mt19937(random_seed);
            
            manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
            if (!manager) {
                cerr << "Error: Failed to initialize CUDD manager" << endl;
                throw runtime_error("CUDD initialization failed");
            }
            
            max_idx = 0;
            input_num = 0;
            latch_num = 0;
            output_num = 0;
            and_num = 0;
            ori_input_num = 0;
        }
        
        ~BDD_Solver() {
            for (auto node : nodes) {
                if (node != nullptr) {
                    Cudd_RecursiveDeref(manager, node);
                }
            }
            nodes.clear();
            if (manager != nullptr) {
                Cudd_Quit(manager);
                manager = nullptr;
            }
        }

        int aag_to_BDD(){
            ifstream aag_file(input_file);
            if (!aag_file.is_open()) {
                cerr << "Error: Failed to open AAG file" << endl;
                return -1;
            }

            string line;
            aag_file >> line;
            if (line != "aag") {
                cerr << "Error: Invalid AAG file format" << endl;
                return -1;
            }

            aag_file >> max_idx >> input_num >> latch_num >> output_num >> and_num;
            
            // initialize 
            nodes.resize(max_idx);
            idx_to_name.resize(input_num);

            // inputs
            for(int i = 0 ; i < input_num ; i++){
                int idx;
                aag_file >> idx;
                DdNode* node = Cudd_bddIthVar(manager, i);
                nodes[idx / 2 - 1] = node;// aag input first index is 2

                // store dp
                dp[node] = {(__float128)0.0, (__float128)1.0}; // 0: odd_cnt, 1: even_cnt
            }

            // outputs(only 1 output)
            int output_idx;
            aag_file >> output_idx;

            // ands
            for(int i = 0 ; i < and_num ; i++){
                int out, in1, in2;
                aag_file >> out >> in1 >> in2;
                DdNode *In1, *In2;

                if(in1 / 2 == 0){// constant
                     In1 = (in1 % 2 == 0) ? Cudd_ReadOne(manager) : Cudd_ReadLogicZero(manager);
                } else{
                    In1 = (in1 % 2 == 0) ? nodes[in1 / 2 - 1] : Cudd_Not(nodes[in1 / 2 - 1]);
                    Cudd_Ref(In1);
                }

                if(in2 / 2 == 0){// constant
                     In2 = (in2 % 2 == 0) ? Cudd_ReadOne(manager) : Cudd_ReadLogicZero(manager);
                } else{
                    In2 = (in2 % 2 == 0) ? nodes[in2 / 2 - 1] : Cudd_Not(nodes[in2 / 2 - 1]);
                    Cudd_Ref(In2);
                }

                DdNode *Out = Cudd_bddAnd(manager, In1, In2);
                Cudd_Ref(Out);
                nodes[out / 2 - 1] = Out;
            }

            // if output is a odd, then an additional inverter is needed
            out_node = output_idx % 2 == 0 ? nodes[output_idx / 2 - 1] : Cudd_Not(nodes[output_idx / 2 - 1]);
            Cudd_Ref(out_node);

            // names
            for(int i = 0 ; i < input_num ; i++){
                string head, name;
                aag_file >> head >> name;

                regex p1("^i(\\d+)$");
                regex p2("^var_(\\d+)\\[(\\d+)\\]$");

                int idx = 0, x = 0, y = 0;
                    smatch match;
                // i_idx
                if (regex_match(head, match, p1)) {
                   idx = stoi(match[1]);
                }
                
                // var_x[y]
                if (regex_match(name, match, p2)) {
                    x = stoi(match[1]);
                    y = stoi(match[2]);
                    ori_input_num = max(ori_input_num, x + 1);
                }

                if (idx < idx_to_name.size()) {
                    idx_to_name[idx] = {x, y};
                }
            }

            ori_input_num = max(ori_input_num, 1);
            idx_to_len.resize(ori_input_num, 0);
            
            // calculate the length of each original input variable
            for (int i = 0; i < input_num && i < idx_to_name.size(); i++) {
                int x = idx_to_name[i].first;
                int y = idx_to_name[i].second;
                if (x < ori_input_num) {
                    idx_to_len[x] = max(idx_to_len[x], y + 1);
                }
            }

            // output names is not needed
            // done
            aag_file.close();   

            // for debug
            cout << "AAG to BDD conversion completed" << endl;
            cout << "BDD infomations:" << endl;
            Cudd_PrintInfo(manager, stdout); 
            FILE* dot_file = fopen("./run_dir/bdd.dot", "w");
            Cudd_DumpDot(manager, 1, &out_node, NULL, NULL, dot_file);
            fclose(dot_file);
            return 0;
        }

        pair<__float128, __float128> cal_dp(DdNode* node) {
            if (dp.find(node) != dp.end()) {
                return dp[node];
            }
            
            // leaves
            if (Cudd_IsConstant(node)) {
                if (node == Cudd_ReadOne(manager)) { 
                    return dp[node] = {(__float128)0.0, (__float128)1.0};
                } else {
                    return dp[node] = {(__float128)0.0, (__float128)0.0}; 
                }
            }

            // node without complementation
            DdNode* regular_node = Cudd_Regular(node);
            bool is_complemented = (regular_node != node);
            
            DdNode* T = Cudd_T(regular_node);
            DdNode* E = Cudd_E(regular_node);
            
            if (is_complemented) {
                T = Cudd_NotCond(T, 1);
                E = Cudd_NotCond(E, 1);
            }
            
            pair<__float128, __float128> t_paths = cal_dp(T);
            pair<__float128, __float128> e_paths = cal_dp(E);
            
            pair<__float128, __float128> result;
            
            result.first = t_paths.first + e_paths.first;   // odd
            result.second = t_paths.second + e_paths.second; // even
            
            dp[node] = result;
            return result;
        }

        bool dfs_generate_solution(DdNode* node, bool odd, vector<bool>& solution) {
            if (Cudd_IsConstant(node)) {
                // check if find a solution
                if ((odd && node == Cudd_ReadOne(manager)) || 
                    (!odd && node != Cudd_ReadOne(manager))) {
                    return true; 
                }
                return false; 
            }

            // node without complementation
            DdNode* regular_node = Cudd_Regular(node);
            bool is_complemented = (regular_node != node);
            int var_index = regular_node->index;

            DdNode* T = Cudd_T(regular_node);
            DdNode* E = Cudd_E(regular_node);

            bool new_odd = is_complemented ? !odd : odd; 
            bool odd_T = new_odd;
            if (Cudd_IsComplement(T)) odd_T = !odd_T;

            bool odd_E = new_odd;
            if (Cudd_IsComplement(E)) odd_E = !odd_E;

            __float128 cnt_T = odd_T ? dp[T].first : dp[T].second;
            __float128 cnt_E = odd_E ? dp[E].first : dp[E].second;

            double prob = 0.5;
            prob = static_cast<double>(cnt_T) / static_cast<double>(cnt_T + cnt_E);
            double rand = std::uniform_real_distribution<double>(0.0, 1.0)(rng);
            
            cout << "var_index: " << var_index 
                << " cnt_T: " << static_cast<double>(cnt_T)
                << " cnt_E: " << static_cast<double>(cnt_E)
                << " prob: " << prob 
                << " rand: " << rand 
                << " choice: " << (rand < prob ? "T" : "E") << endl;

            bool success = false;
            if (rand < prob) {
                solution[var_index] = true;
                success = dfs_generate_solution(T, odd_T, solution);
            } else {
                solution[var_index] = false;
                success = dfs_generate_solution(E, odd_E, solution);
            }

            return success;
        }

        int generate_solutions(int num_solutions) {
            solutions.clear();
            solutions.resize(num_solutions);
            
            for(auto &solution : solutions){
                solution.resize(input_num, false);
            }

            cal_dp(out_node);
            
            const int MAX_ATTEMPTS = 1000;
            
            for(int i = 0; i < num_solutions; i++) {
                int attempts = 0;
                bool success = false;
                
                while (attempts < MAX_ATTEMPTS && !success) {
                    attempts++;
                    success = dfs_generate_solution(out_node, true, solutions[i]);
                }
                
                if (!success) {
                    cout << "Warning: Failed to generate valid solution #" << i+1 << " after " << MAX_ATTEMPTS << " attempts" << endl;
                }
            }
            
            return 0;
        }

        int reshape_solutions() {
            // in solutions, each solution is a vector of bool
            // which represents the value of each input variable like var_0[1],var_2[1]...
            // so we need to reshape it to a reshaped_solutions
            // which is a vector of solutions,
            // each solution is a vector of vector of bool
            // each vector of bool represents the value of an original input variable
            // such as [12:0]var_0, [7:0]var_1, ...

            reshaped_solutions.clear();
            reshaped_solutions.resize(solution_num);
            for(int i = 0 ; i < solution_num; i++){
                reshaped_solutions[i].resize(ori_input_num);
                for(int j = 0 ; j < ori_input_num; j++){
                    reshaped_solutions[i][j].resize(idx_to_len[j], false);
                }
            }

            for(int i = 0 ; i< solution_num ; i++){
                for(int j = 0 ; j < input_num ; j++){
                    int x = idx_to_name[j].first;
                    int y = idx_to_name[j].second;
                    reshaped_solutions[i][x][idx_to_len[x] - 1 - y] = solutions[i][j];
                }
            }
            return 0;
        }

        string binary_to_hex(const vector<bool>& binary) {
            if(binary.empty()) {
                return "0";
            }

            stringstream ss;
            // padding 0s to make the length a multiple of 4
            int len = binary.size();
            int fill = (4 - len % 4) % 4;

            vector<bool> new_binary = vector<bool>(fill, false);
            new_binary.insert(new_binary.end(), binary.begin(), binary.end());
            len += fill;

            // process 4 bits at a time
                for(int i = 0; i < len; i += 4) {
                    int value = 0;
                    for(int j = 0; j < 4 && i+j < len; j++) {
                        value = (value << 1) | (new_binary[i+j] ? 1 : 0);
                    }
                    ss << std::hex << value;
                }
                
                string result = ss.str();

                // remove leading zeros but keep at least one zero
                size_t start = result.find_first_not_of('0');
                if (start == string::npos) {
                    return "0"; 
                }
                return result.substr(start);
        }

        int output_solutions() {
            reshape_solutions();
            
            json j;
            j["assignment_list"] = json::array();

            for (const auto& solution : reshaped_solutions) {
                json j_solution = json::array();
                
                // sort by the original input variable order
                for (int var_id = 0; var_id < ori_input_num; var_id++) {
                    if (var_id < solution.size()) {
                        string hex_value = binary_to_hex(solution[var_id]);
                        j_solution.push_back({{"value", hex_value}});
                    }
                }
                
                j["assignment_list"].push_back(j_solution);
            }

            ofstream out_file(output_file);
            if (!out_file.is_open()) {
                cerr << "Error: Failed to open output file: " << output_file << endl;
                return -1;
            }
            
            out_file << j.dump(4);
            out_file.close();
            
            return 0;
        }
};

int main(int argc, char** argv) {
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << "<input_file> <random_seed> <solution_num> <output_file>" << endl;
        return 1;
    }
    
    string input_file = argv[1];
    int random_seed = stoi(argv[2]);
    int solution_num = stoi(argv[3]);
    string output_file = argv[4];
    
    try {
        BDD_Solver solver(input_file, output_file, random_seed, solution_num);
        
        if (solver.aag_to_BDD() != 0) {
            cerr << "Error building BDD from AAG file" << endl;
            return 1;
        }
        
        if (solver.generate_solutions(solution_num) != 0) {
            cerr << "Error generating solutions" << endl;
            return 1;
        }
        
        if (solver.output_solutions() != 0) {
            cerr << "Error outputting solutions" << endl;
            return 1;
        }
        
        cout << "Successfully generated " << solution_num << " solutions" << endl;
        return 0;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}