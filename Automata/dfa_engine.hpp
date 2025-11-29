#pragma once
#include "nfa_engine.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>

struct DFAState {
    int id;
    bool is_final;
    std::unordered_map<char,int> transitions;
};

class DFA {
public:
    int start_state;
    std::vector<DFAState> states;

    bool simulate(const std::string& input);
    std::string toDot() const;

    std::string toDotWithInput(const std::string& input) const;
    
};

// NFA → DFA conversion
DFA convert_nfa_to_dfa(const NFA& nfa);
