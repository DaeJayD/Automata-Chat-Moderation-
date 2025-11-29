#ifndef PDA_ENGINE_HPP
#define PDA_ENGINE_HPP

#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <stack>
#include <tuple>

class PDA {
private:
    struct PDANode {
        int id;
        bool is_final;
        std::vector<std::tuple<char, char, std::string, int>> transitions;
    };

    std::vector<PDANode> nodes;
    int start_state;
    std::unordered_set<int> final_states;
    char initial_stack_symbol = '$';

public:
    PDA();
    int add_node(bool is_final = false);
    void add_transition(int from, int to, char input, char pop, const std::string& push);
    bool simulate(const std::string& input);
    void print_transitions();
};

class BracketPDA {
public:
    static PDA create_balanced_bracket_pda();
    static PDA create_formatting_pda();
};

#endif