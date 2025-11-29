#include "dfa_engine.hpp"
#include <queue>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

// ------------------ DFA Simulation ------------------


bool DFA::simulate(const std::string& input) {
    int current = start_state;
    for (char c : input) {
        auto it = states[current].transitions.find(c);
        if (it == states[current].transitions.end()) return false;
        current = it->second;
    }
    return states[current].is_final;
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

    int current = start_state;
    if (current >= 0 && current < static_cast<int>(states.size())) visited_states.insert(current);

    for (char c : input) {
        if (current < 0 || current >= static_cast<int>(states.size())) break;
        const auto &trans = states[current].transitions;
        auto it = trans.find(c);
        if (it == trans.end()) break;
        int next = it->second;
        // key format: "from->to:ch"
        std::string key = std::to_string(current) + "->" + std::to_string(next) + ":" + std::string(1, c);
        used_edges.insert(key);
        current = next;
        if (current >= 0 && current < static_cast<int>(states.size())) visited_states.insert(current);
    }

    // Draw all states (full DFA), highlight visited states
    for (const DFAState &s : states) {
        ss << "  q" << s.id;
        bool visited = (visited_states.count(s.id) > 0);
        if (s.is_final && visited) {
            ss << " [peripheries=2, style=filled, fillcolor=orange]";
        } else if (s.is_final) {
            ss << " [peripheries=2]";
        } else if (visited) {
            ss << " [style=filled, fillcolor=yellow]";
        }
        ss << ";\n";
    }
    ss << "\n";

    // Draw all transitions, highlight used ones
    for (const DFAState &s : states) {
        for (const auto &p : s.transitions) {
            char symbol = p.first;
            int to = p.second;
            std::string key = std::to_string(s.id) + "->" + std::to_string(to) + ":" + std::string(1, symbol);

            ss << "  q" << s.id << " -> q" << to << " [label=\"";
            // escape double-quote if necessary
            if (symbol == '\"') ss << "\\\"";
            else if (symbol == '\n') ss << "\\n";
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



// ------------------ NFA → DFA Conversion ------------------

DFA convert_nfa_to_dfa(const NFA& nfa) {
    DFA dfa;
    std::unordered_map<std::string, int> state_map; // maps NFA state sets to DFA state IDs
    std::queue<std::unordered_set<int>> q;
    int next_id = 0;

    // Helper: convert a set of NFA states into a unique string key
    auto closure_to_key = [](const std::unordered_set<int>& s) {
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

    DFAState start_state;
    start_state.id = next_id++;
    start_state.is_final = false;
    for (int f : nfa.get_final_states())
        if (start_set.count(f)) start_state.is_final = true;
    dfa.states.push_back(start_state);
    dfa.start_state = 0;

    q.push(start_set);

    while (!q.empty()) {
        auto current_set = q.front(); q.pop();
        std::string current_key = closure_to_key(current_set);
        int current_id = state_map[current_key];

        // Collect all transitions from current NFA set
        std::unordered_map<char, std::unordered_set<int>> trans_map;
        for (int nfa_state : current_set) {
            for (auto& [sym, targets] : nfa.get_nodes()[nfa_state]->transitions) {
                for (int t : targets) trans_map[sym].insert(t);
            }
        }

        // For each symbol, compute the next DFA state
        for (auto& [sym, next_set_raw] : trans_map) {
            std::unordered_set<int> next_set = nfa.epsilon_closure(next_set_raw);
            std::string next_key = closure_to_key(next_set);
            int next_id_local;

            if (!state_map.count(next_key)) {
                next_id_local = next_id++;
                state_map[next_key] = next_id_local;

                DFAState ns;
                ns.id = next_id_local;
                ns.is_final = false;
                for (int f : nfa.get_final_states())
                    if (next_set.count(f)) ns.is_final = true;

                dfa.states.push_back(ns);
                q.push(next_set);
            } else {
                next_id_local = state_map[next_key];
            }

            dfa.states[current_id].transitions[sym] = next_id_local;
        }
    }

    return dfa;
}
