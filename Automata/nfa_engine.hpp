#ifndef NFA_ENGINE_HPP
#define NFA_ENGINE_HPP

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <memory>
#include <sstream>
#include <utility>

struct NFANode {
    int id;
    bool is_final;
    std::unordered_map<char, std::vector<int>> transitions;
    std::vector<int> epsilon_transitions;
    
    NFANode(int node_id, bool final_state = false) : id(node_id), is_final(final_state) {}
};

class NFA {
private:
    std::vector<std::unique_ptr<NFANode>> nodes;
    int start_state;
    std::unordered_set<int> final_states;

public:
    // special char used internally to represent wildcard '.' in regex
    static constexpr char WILDCARD = '\x7F';

    NFA();
    // Delete copy operations
    NFA(const NFA&) = delete;
    NFA& operator=(const NFA&) = delete;
    
    // Allow move operations
    NFA(NFA&& other) noexcept;
    NFA& operator=(NFA&& other) noexcept;
    
    int add_node(bool is_final = false);
    
    void add_transition(int from, int to, char symbol);
    void add_epsilon_transition(int from, int to);
    void set_start_state(int state);
    void set_final_state(int state);
    bool simulate(const std::string& input);
    std::vector<std::pair<int, char>> get_transitions(int state);
    void print_transitions();

    // Public getters
    int get_start_state() const { return start_state; }
    const std::unordered_set<int>& get_final_states() const { return final_states; }
    const std::vector<std::unique_ptr<NFANode>>& get_nodes() const { return nodes; }

    // Public epsilon closure
    std::string toDotWithInput(const std::string& input) const; 

    std::unordered_set<int> epsilon_closure(const std::unordered_set<int>& states) const {
        std::unordered_set<int> closure = states;
        std::queue<int> queue;
        for (int state : states) queue.push(state);

        while (!queue.empty()) {
            int current = queue.front();
            queue.pop();
            for (int next_state : nodes[current]->epsilon_transitions) {
                if (closure.insert(next_state).second) {
                    queue.push(next_state);
                }
            }
        }
        return closure;
    }

    // Export NFA as DOT (Graphviz)
    std::string toDot() const;
};

// Regex → NFA using Thompson's construction
class RegexToNFA {
public:
    // Convert a POSIX-like regex (supports: |, (), *, +, ?, .) to an NFA
    // Note: this implementation does NOT support escapes like \* or character classes yet.
    static NFA from_regex(const std::string& regex);

private:
    // helper pipeline: tokenize, insert explicit concatenation, shunting-yard to postfix,
    // then Thompson build from postfix.
    static std::string insert_concat(const std::string& regex);
    static std::string to_postfix(const std::string& infix);
    static NFA build_from_postfix(const std::string& postfix);
    // helpers building small fragments
    struct Frag { int start; int accept; };
};

#endif
