#include "dfa_engine.hpp"
#include <queue>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <set>
#include <iomanip>

// ------------------ DFA Simulation ------------------

bool DFA::simulate(const std::string& input) {
    int current = start_state;
    for (char c : input) {
        auto it = states[current].transitions.find(c);
        if (it == states[current].transitions.end()) {
            // No transition for this character
            return false;
        }
        current = it->second;
    }
    return states[current].is_final;
}

// ------------------ DFA Basic DOT Export ------------------

std::string DFA::toDot() const {
    std::ostringstream ss;
    ss << "digraph DFA {\n";
    ss << "  rankdir=LR;\n";
    ss << "  node [shape=circle];\n";
    ss << "  start [shape=point];\n";
    ss << "  start -> q" << start_state << ";\n\n";

    // Draw all states
    for (const DFAState &s : states) {
        ss << "  q" << s.id;
        if (s.is_final) {
            ss << " [peripheries=2]";
        }
        ss << ";\n";
    }
    ss << "\n";

    // Draw all transitions
    for (const DFAState &s : states) {
        for (const auto &p : s.transitions) {
            char symbol = p.first;
            int to = p.second;
            
            ss << "  q" << s.id << " -> q" << to << " [label=\"";
            
            // Escape special characters
            if (symbol == '\"') ss << "\\\"";
            else if (symbol == '\n') ss << "\\n";
            else if (symbol == '\t') ss << "\\t";
            else if (symbol == ' ') ss << "␣";
            else if (symbol == NFA::WILDCARD) ss << ".";
            else ss << symbol;
            
            ss << "\"];\n";
        }
    }

    ss << "}\n";
    return ss.str();
}

// ------------------ DFA DOT Export with Input Highlighting ------------------

std::string DFA::toDotWithInput(const std::string &input) const {
    std::ostringstream ss;
    ss << "digraph DFA {\n";
    ss << "  rankdir=LR;\n";
    ss << "  node [shape=circle];\n";
    ss << "  start [shape=point];\n";
    ss << "  start -> q" << start_state << ";\n\n";

    // Record visited states and used transitions while simulating the input
    std::unordered_set<std::string> used_edges;
    std::unordered_set<int> visited_states;
    std::vector<int> path_states;

    int current = start_state;
    if (current >= 0 && current < static_cast<int>(states.size())) {
        visited_states.insert(current);
        path_states.push_back(current);
    }

    for (char c : input) {
        if (current < 0 || current >= static_cast<int>(states.size())) 
            break;
        
        const auto &trans = states[current].transitions;
        auto it = trans.find(c);
        if (it == trans.end()) {
            // No transition - input rejected
            break;
        }
        
        int next = it->second;
        std::string key = std::to_string(current) + "->" + 
                         std::to_string(next) + ":" + 
                         std::string(1, c);
        used_edges.insert(key);
        current = next;
        visited_states.insert(current);
        path_states.push_back(current);
    }

    // Draw all states with highlighting
    for (const DFAState &s : states) {
        ss << "  q" << s.id;
        
        bool visited = (visited_states.count(s.id) > 0);
        bool is_in_path = false;
        for (size_t i = 0; i < path_states.size(); i++) {
            if (path_states[i] == s.id) {
                is_in_path = true;
                break;
            }
        }
        
        if (s.is_final && is_in_path) {
            ss << " [peripheries=2, style=filled, fillcolor=orange]";
        } else if (s.is_final) {
            ss << " [peripheries=2]";
        } else if (is_in_path) {
            ss << " [style=filled, fillcolor=yellow]";
        } else if (visited) {
            ss << " [style=filled, fillcolor=lightblue]";
        }
        ss << ";\n";
    }
    ss << "\n";

    // Draw all transitions
    for (const DFAState &s : states) {
        for (const auto &p : s.transitions) {
            char symbol = p.first;
            int to = p.second;
            std::string key = std::to_string(s.id) + "->" + 
                             std::to_string(to) + ":" + 
                             std::string(1, symbol);

            ss << "  q" << s.id << " -> q" << to << " [label=\"";
            
            // Escape special characters
            if (symbol == '\"') ss << "\\\"";
            else if (symbol == '\n') ss << "\\n";
            else if (symbol == '\t') ss << "\\t";
            else if (symbol == ' ') ss << "␣";
            else if (symbol == NFA::WILDCARD) ss << ".";
            else ss << symbol;
            
            ss << "\"";
            
            if (used_edges.count(key)) {
                ss << ", color=red, penwidth=2";
            }
            ss << "];\n";
        }
    }

    ss << "}\n";
    return ss.str();
}

// ------------------ Helper Function ------------------

bool DFA::is_dead_state(int state_id) const {
    if (state_id < 0 || state_id >= static_cast<int>(states.size())) 
        return false;
    
    const DFAState& state = states[state_id];
    
    // Check if all transitions go to this state itself
    for (const auto& trans : state.transitions) {
        if (trans.second != state_id) {
            return false;
        }
    }
    
    // If state has at least one transition, it's a dead state only if all go to self
    // AND state is not final
    return !state.transitions.empty() && !state.is_final;
}

// ------------------ NFA → DFA Conversion ------------------

DFA convert_nfa_to_dfa(const NFA& nfa) {
    DFA dfa;
    std::unordered_map<std::string, int> state_map;
    std::queue<std::unordered_set<int>> q;
    int next_id = 0;
    
    // Define alphabet based on NFA transitions
    std::set<char> alphabet;
    
    // Collect all symbols from NFA transitions
    const auto& nfa_nodes = nfa.get_nodes();
    for (const auto& node : nfa_nodes) {
        for (const auto& trans : node->transitions) {
            char symbol = trans.first;
            if (symbol != NFA::WILDCARD) {
                alphabet.insert(symbol);
            }
        }
    }
    
    // If no symbols found, use default alphabet
    if (alphabet.empty()) {
        // Add lowercase letters
        for (char c = 'a'; c <= 'z'; c++) alphabet.insert(c);
        // Add uppercase letters
        for (char c = 'A'; c <= 'Z'; c++) alphabet.insert(c);
        // Add digits
        for (char c = '0'; c <= '9'; c++) alphabet.insert(c);
        // Add space
        alphabet.insert(' ');
    }
    
    // Helper: convert a set of NFA states into a unique string key
    auto closure_to_key = [](const std::unordered_set<int>& s) {
        if (s.empty()) return std::string("DEAD");
        std::vector<int> v(s.begin(), s.end());
        std::sort(v.begin(), v.end());
        std::string key;
        for (int x : v) key += std::to_string(x) + ",";
        return key;
    };

    // Start state: epsilon-closure of NFA start state
    std::unordered_set<int> start_set = nfa.epsilon_closure({ nfa.get_start_state() });
    std::string start_key = closure_to_key(start_set);
    state_map[start_key] = next_id;

    DFAState start_dfa_state;
    start_dfa_state.id = next_id++;
    start_dfa_state.is_final = false;
    for (int f : nfa.get_final_states())
        if (start_set.count(f)) start_dfa_state.is_final = true;
    dfa.get_states().push_back(start_dfa_state);
    dfa.set_start_state(0);

    if (!start_set.empty()) {
        q.push(start_set);
    }

    while (!q.empty()) {
        auto current_set = q.front(); 
        q.pop();
        std::string current_key = closure_to_key(current_set);
        int current_id = state_map[current_key];

        // Collect all wildcard destinations for this DFA state
        std::unordered_set<int> wildcard_dests;
        for (int nfa_state : current_set) {
            const auto& node = nfa_nodes[nfa_state];
            auto it_wild = node->transitions.find(NFA::WILDCARD);
            if (it_wild != node->transitions.end()) {
                for (int t : it_wild->second) {
                    wildcard_dests.insert(t);
                }
            }
        }
        // Take epsilon-closure of wildcard destinations
        std::unordered_set<int> wildcard_closures = nfa.epsilon_closure(wildcard_dests);

        // For each character in alphabet, compute transition
        for (char symbol : alphabet) {
            std::unordered_set<int> next_set_raw = wildcard_closures; // Start with wildcard closures
            
            // Get all NFA states reachable on this exact symbol from current_set
            for (int nfa_state : current_set) {
                const auto& node = nfa_nodes[nfa_state];
                
                // Check for exact match
                auto it = node->transitions.find(symbol);
                if (it != node->transitions.end()) {
                    for (int t : it->second) {
                        next_set_raw.insert(t);
                    }
                }
            }
            
            // Take epsilon-closure of combined destinations
            std::unordered_set<int> next_set = nfa.epsilon_closure(next_set_raw);
            std::string next_key = closure_to_key(next_set);
            
            // Skip if this leads to dead state (we'll handle dead state later)
            if (next_set.empty()) {
                continue; // Will be filled with dead state later
            }
            
            int next_id_local;
            if (!state_map.count(next_key)) {
                next_id_local = next_id++;
                state_map[next_key] = next_id_local;

                DFAState ns;
                ns.id = next_id_local;
                ns.is_final = false;
                for (int f : nfa.get_final_states())
                    if (next_set.count(f)) ns.is_final = true;

                dfa.get_states().push_back(ns);
                q.push(next_set);
            } else {
                next_id_local = state_map[next_key];
            }
            
            dfa.get_states()[current_id].transitions[symbol] = next_id_local;
        }
    }
    
    // Create dead state (if needed)
    int dead_id = -1;
    bool dead_state_needed = false;
    
    // Check if any state is missing transitions
    for (auto& state : dfa.get_states()) {
        for (char c : alphabet) {
            if (state.transitions.find(c) == state.transitions.end()) {
                dead_state_needed = true;
                break;
            }
        }
        if (dead_state_needed) break;
    }
    
    if (dead_state_needed) {
        dead_id = next_id++;
        DFAState dead_state;
        dead_state.id = dead_id;
        dead_state.is_final = false;
        
        // Add self-loops for all alphabet characters
        for (char c : alphabet) {
            dead_state.transitions[c] = dead_id;
        }
        dfa.get_states().push_back(dead_state);
        
        // Fill in missing transitions to dead state
        for (auto& state : dfa.get_states()) {
            if (state.id == dead_id) continue; // Skip dead state itself
            
            for (char c : alphabet) {
                if (state.transitions.find(c) == state.transitions.end()) {
                    state.transitions[c] = dead_id;
                }
            }
        }
    }
    
    return dfa;
}