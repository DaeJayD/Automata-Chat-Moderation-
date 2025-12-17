#ifndef DFA_ENGINE_HPP
#define DFA_ENGINE_HPP

#include "nfa_engine.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <set>  // ADD THIS LINE

// DFA State structure
struct DFAState {
    int id;
    bool is_final;
    std::unordered_map<char, int> transitions; // char -> next state id
    
};

// DFA class
class DFA {
private:
    std::vector<DFAState> states;
    int start_state;

public:
    DFA() : start_state(0) {}
    
    // Add a new state and return its ID
    int add_state(bool is_final = false) {
        int id = static_cast<int>(states.size());
        states.push_back({id, is_final, {}});
        return id;
    }
    
    // Add a transition
    void add_transition(int from, char symbol, int to) {
        if (from >= 0 && from < static_cast<int>(states.size())) {
            states[from].transitions[symbol] = to;
        }
    }
    
    // Set start state
    void set_start_state(int state_id) {
        if (state_id >= 0 && state_id < static_cast<int>(states.size())) {
            start_state = state_id;
        }
    }
    
    // Set final state
    void set_final_state(int state_id, bool final = true) {
        if (state_id >= 0 && state_id < static_cast<int>(states.size())) {
            states[state_id].is_final = final;
        }
    }
    
    // Get states (for conversion)
    std::vector<DFAState>& get_states() { return states; }
    const std::vector<DFAState>& get_states() const { return states; }
    
    // Get start state
    int get_start_state() const { return start_state; }
    
    // Simulation
    bool simulate(const std::string& input);
    
    // DOT export
    std::string toDot() const;
    std::string toDotWithInput(const std::string& input) const;
    
    // Check if a state is a dead state - SIMPLIFIED VERSION
    bool is_dead_state(int state_id) const;
};

// Conversion function
DFA convert_nfa_to_dfa(const NFA& nfa);

#endif // DFA_ENGINE_HPP