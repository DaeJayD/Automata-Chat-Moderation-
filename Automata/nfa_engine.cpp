// nfa_engine.cpp
#include "nfa_engine.hpp"

#define CYAN    "\033[36m"
#define RESET   "\033[0m"

#include <stack>
#include <iomanip>

// -------------------- NFA core impl --------------------

NFA::NFA() : start_state(0) {
    nodes.push_back(std::make_unique<NFANode>(0));
}

// Move constructor
NFA::NFA(NFA&& other) noexcept 
    : nodes(std::move(other.nodes)),
      start_state(other.start_state),
      final_states(std::move(other.final_states)) {
}

// Move assignment operator
NFA& NFA::operator=(NFA&& other) noexcept {
    if (this != &other) {
        nodes = std::move(other.nodes);
        start_state = other.start_state;
        final_states = std::move(other.final_states);
    }
    return *this;
}

int NFA::add_node(bool is_final) {
    int new_id = nodes.size();
    nodes.push_back(std::make_unique<NFANode>(new_id, is_final));
    if (is_final) {
        final_states.insert(new_id);
    }
    return new_id;
}

void NFA::add_transition(int from, int to, char symbol) {
    nodes[from]->transitions[symbol].push_back(to);
}

void NFA::add_epsilon_transition(int from, int to) {
    nodes[from]->epsilon_transitions.push_back(to);
}

void NFA::set_start_state(int state) { start_state = state; }
void NFA::set_final_state(int state) { 
    nodes[state]->is_final = true;
    final_states.insert(state);
}

bool NFA::simulate(const std::string& input) {
    std::unordered_set<int> current_states = epsilon_closure({start_state});
    
    for (char c : input) {
        std::unordered_set<int> next_states;
        for (int state : current_states) {
            // exact transitions
            auto it = nodes[state]->transitions.find(c);
            if (it != nodes[state]->transitions.end()) {
                for (int next_state : it->second) {
                    next_states.insert(next_state);
                }
            }
            // wildcard transitions (match any character)
            auto it_w = nodes[state]->transitions.find(NFA::WILDCARD);
            if (it_w != nodes[state]->transitions.end()) {
                for (int next_state : it_w->second) {
                    next_states.insert(next_state);
                }
            }
        }
        current_states = epsilon_closure(next_states);
    }

    for (int state : current_states) {
        if (final_states.count(state)) {
            return true;
        }
    }
    return false;
}

std::vector<std::pair<int, char>> NFA::get_transitions(int state) {
    std::vector<std::pair<int, char>> result;
    for (const auto& kv : nodes[state]->transitions) {
        char symbol = kv.first;
        for (int target : kv.second) {
            result.emplace_back(target, symbol);
        }
    }
    for (int target : nodes[state]->epsilon_transitions) {
        result.emplace_back(target, 'E'); // Use 'E' instead of special epsilon character
    }
    return result;
}

void NFA::print_transitions() {
    std::cout << CYAN << "NFA Transitions:\n" << RESET;
    for (const auto& node : nodes) {
        std::cout << "State q" << node->id << (node->is_final ? " [FINAL]" : "") << ":\n";
        auto transitions = get_transitions(node->id);
        for (const auto& [target, symbol] : transitions) {
            if (symbol == 'E') {
                std::cout << "  q" << node->id << " --ε--> q" << target << "\n";
            } else if (symbol == NFA::WILDCARD) {
                std::cout << "  q" << node->id << " --" << "." << "--> q" << target << "\n";
            } else {
                std::cout << "  q" << node->id << " --" << symbol << "--> q" << target << "\n";
            }
        }
    }
}

// DOT exporter
std::string NFA::toDot() const {
    std::ostringstream ss;
    ss << "digraph NFA {\n";
    ss << "  rankdir=LR;\n";
    ss << "  node [shape = circle];\n";
    // invisible start arrow
    ss << "  start [shape=point];\n";
    ss << "  start -> q" << start_state << ";\n";

    for (const auto& node : nodes) {
        ss << "  q" << node->id;
        if (node->is_final) ss << " [peripheries=2]";
        ss << ";\n";
    }

    for (const auto& node : nodes) {
        for (const auto& kv : node->transitions) {
            char symbol = kv.first;
            for (int t : kv.second) {
                ss << "  q" << node->id << " -> q" << t << " [label=\"";
                if (symbol == NFA::WILDCARD) ss << ".";
                else {
                    if (symbol == '\"') ss << "\\\"";
                    else ss << symbol;
                }
                ss << "\"];\n";
            }
        }
        for (int t : node->epsilon_transitions) {
            ss << "  q" << node->id << " -> q" << t << " [label=\"ε\"];\n";
        }
    }

    ss << "}\n";
    return ss.str();
}

std::string NFA::toDotWithInput(const std::string& input) const {
    std::ostringstream ss;
    ss << "digraph NFA {\n";
    ss << "  rankdir=LR;\n";
    ss << "  node [shape=circle];\n";
    ss << "  start [shape=point];\n";
    ss << "  start -> q" << get_start_state() << ";\n";

    // Initial active states = epsilon-closure of start
    std::unordered_set<int> current_states = epsilon_closure({ get_start_state() });
    std::unordered_set<int> next_states;

    // Draw all nodes (basic)
    const auto& node_vec = get_nodes(); // const ref to vector<unique_ptr<NFANode>>
    for (const auto& node_ptr : node_vec) {
        const NFANode* node = node_ptr.get();
        ss << "  q" << node->id;
        if (node->is_final) ss << " [peripheries=2]";
        ss << ";\n";
    }

    // Simulate the NFA in a conservative way to collect all states that can
    // be active while processing the input (we keep closures after each step).
    std::unordered_set<int> visited; // states that were active at some point
    // Mark initial closure states as visited
    visited.insert(current_states.begin(), current_states.end());

    for (char c : input) {
        next_states.clear();
        for (int state : current_states) {
            // exact transitions on c
            auto it = node_vec[state]->transitions.find(c);
            if (it != node_vec[state]->transitions.end()) {
                for (int t : it->second) {
                    // include epsilon-closure of the target
                    auto cl = epsilon_closure({ t });
                    next_states.insert(cl.begin(), cl.end());
                }
            }
            // wildcard transitions
            auto itw = node_vec[state]->transitions.find(NFA::WILDCARD);
            if (itw != node_vec[state]->transitions.end()) {
                for (int t : itw->second) {
                    auto cl = epsilon_closure({ t });
                    next_states.insert(cl.begin(), cl.end());
                }
            }
        }
        // mark all states that are active after consuming this character
        visited.insert(next_states.begin(), next_states.end());
        current_states = next_states;
        if (current_states.empty()) break; // no further progress possible
    }

    // Also ensure start and any final states encountered are in visited
    // (visited already contains initial closure and all subsequent closures)

    // Re-draw states with highlights for visited states
    for (const auto& node_ptr : node_vec) {
        const NFANode* node = node_ptr.get();
        if (visited.count(node->id)) {
            ss << "  q" << node->id << " [style=filled, fillcolor=yellow";
            if (node->is_final) ss << ", peripheries=2";
            ss << "];\n";
        }
    }

    // Draw transitions. Highlight transitions between visited states (approximation).
    // (Exact per-character transition highlighting in NFAs is tricky; this highlights edges between visited states.)
    for (const auto& node_ptr : node_vec) {
        const NFANode* node = node_ptr.get();
        for (const auto& kv : node->transitions) {
            char symbol = kv.first;
            for (int t : kv.second) {
                ss << "  q" << node->id << " -> q" << t << " [label=\"";
                if (symbol == NFA::WILDCARD) ss << ".";
                else {
                    if (symbol == '\"') ss << "\\\"";
                    else ss << symbol;
                }
                ss << "\"";

                if (visited.count(node->id) && visited.count(t)) {
                    ss << ", color=red, penwidth=2";
                }
                ss << "];\n";
            }
        }
        // epsilon transitions
        for (int t : node->epsilon_transitions) {
            ss << "  q" << node->id << " -> q" << t << " [label=\"ε\"";
            if (visited.count(node->id) && visited.count(t)) {
                ss << ", color=red, penwidth=2";
            }
            ss << "];\n";
        }
    }

    ss << "}\n";
    return ss.str();
}

// -------------------- Regex → NFA (Thompson) --------------------

/*
 Pipeline:
 1. insert explicit concatenation operator (we use ASCII unit separator '\x1F' internally)
 2. convert infix (with parentheses and operators) to postfix (shunting-yard)
 3. build via Thompson using a stack of fragments (start, accept)
*/

namespace {
    // internal concat marker (non-printable) to avoid colliding with dot '.' wildcard
    constexpr char CONCAT = '\x1F';

    inline bool is_operator(char c) {
        return (c == '|' || c == '*' || c == '+' || c == '?' || c == CONCAT);
    }

    int precedence(char op) {
        // higher number => higher precedence
        switch (op) {
            case '*': case '+': case '?': return 4; // unary postfix
            case CONCAT: return 3;
            case '|': return 1;
            default: return 0;
        }
    }

    bool is_unary(char op) {
        return (op == '*' || op == '+' || op == '?');
    }
}

// transform: insert explicit concatenation markers
std::string RegexToNFA::insert_concat(const std::string& regex) {
    std::string out;
    out.reserve(regex.size() * 2);
    auto is_literal = [](char c) {
        return c != '|' && c != '*' && c != '+' && c != '?' && c != '(' && c != ')';
    };

    for (size_t i = 0; i < regex.size(); ++i) {
        char c1 = regex[i];
        out.push_back(c1);

        if (i + 1 < regex.size()) {
            char c2 = regex[i+1];
            // if c1 is literal, wildcard ('.') or ')' or '*' '+' '?'
            bool left = (is_literal(c1) || c1 == ')' || c1 == '*' || c1 == '+' || c1 == '?');
            // if c2 is literal, wildcard ('.') or '('
            bool right = (is_literal(c2) || c2 == '(');
            if (left && right) {
                out.push_back(CONCAT);
            }
        }
    }
    return out;
}

// shunting-yard: infix (with CONCAT marker) -> postfix
std::string RegexToNFA::to_postfix(const std::string& infix) {
    std::string out;
    std::stack<char> ops;
    for (size_t i = 0; i < infix.size(); ++i) {
        char c = infix[i];
        if (c == '(') {
            ops.push(c);
        } else if (c == ')') {
            while (!ops.empty() && ops.top() != '(') {
                out.push_back(ops.top()); ops.pop();
            }
            if (!ops.empty() && ops.top() == '(') ops.pop();
        } else if (is_operator(c)) {
            if (is_unary(c)) {
                // unary operators are postfix; push them with highest precedence handling:
                // Because they are postfix they are placed directly in output if previous is literal
                // But simpler: handle precedence and associativity with stack
            }
            while (!ops.empty()) {
                char top = ops.top();
                if (top == '(') break;
                int p1 = precedence(top);
                int p2 = precedence(c);
                // For postfix unary operators: treat them as highest precedence and right-assoc
                if ((p1 > p2) || (p1 == p2 && !(c == CONCAT || c == '|'))) {
                    out.push_back(top); ops.pop();
                } else break;
            }
            ops.push(c);
        } else {
            // literal / wildcard '.'
            out.push_back(c);
        }
    }
    while (!ops.empty()) {
        out.push_back(ops.top()); ops.pop();
    }
    return out;
}

NFA RegexToNFA::from_regex(const std::string& regex) {
    // small edge: empty regex -> NFA that accepts empty string
    if (regex.empty()) {
        NFA nfa;
        int s = nfa.add_node();
        nfa.set_start_state(s);
        nfa.set_final_state(s);
        return nfa;
    }

    std::string with_concat = insert_concat(regex);
    std::string postfix = to_postfix(with_concat);
    return build_from_postfix(postfix);
}

NFA RegexToNFA::build_from_postfix(const std::string& postfix) {
    NFA nfa;
    struct Frag { int start; int accept; };
    std::stack<Frag> st;

    auto new_fragment_for_char = [&](char symbol) -> Frag {
        int s = nfa.add_node(false);
        int a = nfa.add_node(false);
        if (symbol == '.') {
            // wildcard
            nfa.add_transition(s, a, NFA::WILDCARD);
        } else {
            nfa.add_transition(s, a, symbol);
        }
        return {s, a};
    };

    for (size_t i = 0; i < postfix.size(); ++i) {
        char tok = postfix[i];
        if (tok == CONCAT) {
            // concatenation: pop B then A -> A concat B
            if (st.size() < 2) { continue; }
            Frag b = st.top(); st.pop();
            Frag a = st.top(); st.pop();
            // connect a.accept -> b.start with epsilon
            nfa.add_epsilon_transition(a.accept, b.start);
            st.push({a.start, b.accept});
        } else if (tok == '|') {
            // alternation: pop B and A -> new start and accept
            if (st.size() < 2) { continue; }
            Frag b = st.top(); st.pop();
            Frag a = st.top(); st.pop();
            int s = nfa.add_node(false);
            int acc = nfa.add_node(false);
            nfa.add_epsilon_transition(s, a.start);
            nfa.add_epsilon_transition(s, b.start);
            nfa.add_epsilon_transition(a.accept, acc);
            nfa.add_epsilon_transition(b.accept, acc);
            st.push({s, acc});
        } else if (tok == '*') {
            // Kleene star on top fragment
            if (st.empty()) continue;
            Frag a = st.top(); st.pop();
            int s = nfa.add_node(false);
            int acc = nfa.add_node(false);
            nfa.add_epsilon_transition(s, a.start);
            nfa.add_epsilon_transition(s, acc);
            nfa.add_epsilon_transition(a.accept, a.start);
            nfa.add_epsilon_transition(a.accept, acc);
            st.push({s, acc});
        } else if (tok == '+') {
            // one or more: A+ = A concat A*
            if (st.empty()) continue;
            Frag a = st.top(); st.pop();
            // build A*
            int s_star = nfa.add_node(false);
            int acc_star = nfa.add_node(false);
            nfa.add_epsilon_transition(s_star, a.start);
            nfa.add_epsilon_transition(s_star, acc_star);
            nfa.add_epsilon_transition(a.accept, a.start);
            nfa.add_epsilon_transition(a.accept, acc_star);
            // connect a.accept -> s_star (i.e., A followed by A*)
            // Actually build concat: start = a.start, accept = acc_star
            st.push({a.start, acc_star});
        } else if (tok == '?') {
            // optional: A? => new start -> A.start and to new accept
            if (st.empty()) continue;
            Frag a = st.top(); st.pop();
            int s = nfa.add_node(false);
            int acc = nfa.add_node(false);
            nfa.add_epsilon_transition(s, a.start);
            nfa.add_epsilon_transition(s, acc);
            nfa.add_epsilon_transition(a.accept, acc);
            st.push({s, acc});
        } else {
            // literal or wildcard '.' in input (literal '.' is wildcard token in original regex)
            st.push(new_fragment_for_char(tok));
        }
    }

    // after processing, we may have multiple fragments on stack; chain them with concat
    if (st.empty()) {
        // accept empty
        int s = nfa.add_node(false);
        nfa.set_start_state(s);
        nfa.set_final_state(s);
        return nfa;
    }

    Frag result = st.top();
    // If more than one frag left, concatenate left-to-right (this should not happen if postfix is correct)
    while (st.size() > 1) {
        Frag b = result; st.pop();
        Frag a = st.top(); st.pop();
        nfa.add_epsilon_transition(a.accept, b.start);
        result = {a.start, b.accept};
    }

    nfa.set_start_state(result.start);
    nfa.set_final_state(result.accept);
    return nfa;
}

