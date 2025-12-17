#include "pda_engine.hpp"
#include "approximate_matcher.hpp"
#include <cctype>
#include <fstream>
#include <sstream>

using namespace std;

// ==================== PDA IMPLEMENTATION ====================

PDA::PDA() {
    start_state = 0;
    add_node(false); // at least one state
}

int PDA::add_node(bool is_final) {
    int id = nodes.size();
    nodes.push_back({id, is_final, {}});
    if (is_final) final_states.insert(id);
    return id;
}

void PDA::add_transition(int from, int to, char input, char pop, const std::string& push) {
    nodes[from].transitions.emplace_back(input, pop, push, to);
}

// ==================== TOXIC WORD DETECTION PDA ====================

PDA BracketPDA::create_toxic_detection_pda(const vector<string>& toxic_patterns, int max_edits) {
    PDA pda;
    
    // States:
    // q0: Start, outside brackets (scanning)
    // q1: Inside brackets (collecting words)
    // q2: Toxic word detected
    // q3: Accept (balanced and optionally toxic found)
    
    int q0 = pda.add_node(false);  // Start/scanning state
    int q1 = pda.add_node(false);  // Inside brackets state
    int q2 = pda.add_node(false);  // Toxic word detected state
    int q3 = pda.add_node(true);   // Accept state (balanced)
    
    // Start transitions
    pda.add_transition(q0, q0, 0, '$', "$");  // Stay in q0 for non-bracket chars
    pda.add_transition(q0, q1, '(', '$', "$(");  // Enter bracket
    pda.add_transition(q0, q1, '[', '$', "$[");
    pda.add_transition(q0, q1, '{', '$', "${");
    pda.add_transition(q0, q1, '<', '$', "$<");
    
    // Inside bracket transitions
    pda.add_transition(q1, q1, 0, '$', "$");  // Stay in q1 for word characters
    pda.add_transition(q1, q0, ')', '(', "");  // Exit bracket
    pda.add_transition(q1, q0, ']', '[', "");
    pda.add_transition(q1, q0, '}', '{', "");
    pda.add_transition(q1, q0, '>', '<', "");
    
    // Add transition for space - word boundary (check for toxicity)
    pda.add_transition(q1, q2, ' ', '$', "$");  // Space triggers toxicity check
    
    // If toxic found, go back to collecting
    pda.add_transition(q2, q1, 0, '$', "$");
    
    // If not toxic, continue in q1
    pda.add_transition(q1, q1, ' ', '$', "$");  // Space but not toxic
    
    // Accept if stack empty and in q0 or q2
    pda.add_transition(q0, q3, 0, '$', "");  // End of input, stack has only $
    pda.add_transition(q2, q3, 0, '$', "");  // End with toxic detected
    
    return pda;
}

// ==================== SIMULATION WITH TOXIC DETECTION ====================

bool PDA::simulate_with_toxicity(const std::string& input, 
                                ApproximateMatcher& matcher,
                                const vector<string>& toxic_patterns,
                                int max_edits,
                                vector<string>& found_toxic_words) {
    
    stack<char> st;
    string current_word;
    bool inside_brackets = false;
    bool toxic_found = false;
    
    for (size_t i = 0; i <= input.length(); i++) {
        char c = (i < input.length()) ? input[i] : ' ';  // Treat end as space
        
        // Check word boundary
        if (!isalpha(c) && c != '\'' && c != '-' && i < input.length()) {
            if (!current_word.empty()) {
                // Check if word is toxic (only if inside brackets)
                if (inside_brackets) {
                    for (const auto& pattern : toxic_patterns) {
                        auto matches = matcher.find_matches(current_word, pattern, max_edits);
                        if (!matches.empty()) {
                            toxic_found = true;
                            found_toxic_words.push_back(current_word);
                            break;
                        }
                    }
                }
                current_word.clear();
            }
            
            // Handle brackets
            if (c == '(' || c == '[' || c == '{' || c == '<') {
                st.push(c);
                inside_brackets = true;
            } else if (c == ')' && !st.empty() && st.top() == '(') {
                st.pop();
                inside_brackets = !st.empty();  // Still inside if nested
            } else if (c == ']' && !st.empty() && st.top() == '[') {
                st.pop();
                inside_brackets = !st.empty();
            } else if (c == '}' && !st.empty() && st.top() == '{') {
                st.pop();
                inside_brackets = !st.empty();
            } else if (c == '>' && !st.empty() && st.top() == '<') {
                st.pop();
                inside_brackets = !st.empty();
            }
        } else if (isalpha(c)) {
            current_word += tolower(c);
        }
    }
    
    // Final check for balanced brackets
    bool brackets_balanced = st.empty();
    
    return brackets_balanced;  // Accept if brackets balanced
}

// ==================== SIMPLE BRACKET CHECK (original) ====================

bool PDA::simulate(const std::string& input) {
    stack<char> st;
    for (char c : input) {
        if (c == '(' || c == '[' || c == '{' || c == '<') {
            st.push(c);
        } else if (c == ')' && !st.empty() && st.top() == '(') {
            st.pop();
        } else if (c == ']' && !st.empty() && st.top() == '[') {
            st.pop();
        } else if (c == '}' && !st.empty() && st.top() == '{') {
            st.pop();
        } else if (c == '>' && !st.empty() && st.top() == '<') {
            st.pop();
        }
    }
    return st.empty();
}

// ==================== DOT GENERATION ====================

std::string PDA::toDot() const {
    std::string dot = "digraph PDA {\n";
    dot += "  rankdir=LR;\n";
    dot += "  node [shape=circle];\n";
    
    // Add educational label
    dot += "  labelloc=\"t\";\n";
    dot += "  label=\"Pushdown Automaton (Context-Free)\\n";
    dot += "Recognizes balanced brackets with toxic word detection\\n";
    dot += "Language class: CONTEXT-FREE (requires stack)\";\n";
    dot += "  fontsize=12;\n";
    
    // --- States ---
    for (const auto& node : nodes) {
        dot += "  " + std::to_string(node.id) + " [label=\"q" + std::to_string(node.id);
        
        // Add state descriptions
        if (node.id == 0) dot += "\\nStart/Scan";
        else if (node.id == 1) dot += "\\nInside Brackets";
        else if (node.id == 2) dot += "\\nToxic Detected";
        else if (node.id == 3) dot += "\\nAccept";
        
        dot += "\"";
        
        if (node.is_final) {
            dot += ", shape=doublecircle, color=\"green\"];\n";
        } else {
            dot += ", color=\"blue\"];\n";
        }
    }
    
    // --- Transitions ---
    for (const auto& node : nodes) {
        for (const auto& trans : node.transitions) {
            char input = std::get<0>(trans);
            char pop = std::get<1>(trans);
            std::string push = std::get<2>(trans);
            int to = std::get<3>(trans);
            
            std::string label;
            if (input == 0) {
                label = "ε";
            } else if (input == ' ') {
                label = "space";
            } else {
                label = std::string(1, input);
            }
            
            label += " / " + std::string(1, pop) + " → " + push;
            
            // Color code transitions
            std::string color = "black";
            if (pop == '(' || pop == '[' || pop == '{' || pop == '<') color = "red";
            if (push.find('(') != string::npos || push.find('[') != string::npos || 
                push.find('{') != string::npos || push.find('<') != string::npos) color = "orange";
            if (input == ' ') color = "purple";  // Word boundary
            
            dot += "  " + std::to_string(node.id) + " -> " + std::to_string(to)
                   + " [label=\"" + label + "\", color=\"" + color + "\"];\n";
        }
    }
    
    // --- Start pointer ---
    dot += "  start [shape=point];\n";
    dot += "  start -> " + std::to_string(start_state) + ";\n";
    
    // Add legend
    dot += "  subgraph cluster_legend {\n";
    dot += "    label=\"PDA Transitions (Context-Free)\";\n";
    dot += "    style=filled;\n";
    dot += "    color=lightgrey;\n";
    dot += "    node [shape=rectangle];\n";
    dot += "    legend1 [label=\"Red: Pop bracket\"];\n";
    dot += "    legend2 [label=\"Orange: Push bracket\"];\n";
    dot += "    legend3 [label=\"Purple: Word boundary\"];\n";
    dot += "    legend4 [label=\"Black: Scan characters\"];\n";
    dot += "    legend5 [label=\"Key: Uses STACK → Context-Free Language\"];\n";
    dot += "  }\n";
    
    dot += "}\n";
    return dot;
}

// ==================== SIMPLE BRACKET PDA ====================

PDA BracketPDA::create_balanced_bracket_pda() {
    PDA pda;
    
    int q0 = pda.add_node(false);  // Start
    int q1 = pda.add_node(false);  // Reading
    int q2 = pda.add_node(true);   // Accept
    
    // Opening brackets
    pda.add_transition(q0, q1, '(', '$', "($");
    pda.add_transition(q0, q1, '[', '$', "[$");
    pda.add_transition(q0, q1, '{', '$', "{$");
    pda.add_transition(q0, q1, '<', '$', "<$");
    
    // Closing brackets
    pda.add_transition(q1, q1, ')', '(', "");
    pda.add_transition(q1, q1, ']', '[', "");
    pda.add_transition(q1, q1, '}', '{', "");
    pda.add_transition(q1, q1, '>', '<', "");
    
    // End of input
    pda.add_transition(q1, q2, 0, '$', "");  // ε transition when stack has only $
    
    return pda;
}