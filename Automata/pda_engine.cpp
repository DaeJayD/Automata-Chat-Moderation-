#include "pda_engine.hpp"
#include "approximate_matcher.hpp"
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <stack>

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

// ==================== MARKDOWN/NESTED STRUCTURE PDA ====================

PDA BracketPDA::create_markdown_pda(bool strict_nesting) {
    PDA pda;
    
    // States:
    // q0: Start, outside formatting (scanning text)
    // q1: Inside formatting level 1 (e.g., bold **)
    // q2: Inside formatting level 2 (e.g., italic * inside bold)
    // q3: Error state (mismatch detected)
    // q4: Accept state (balanced formatting)
    
    int q0 = pda.add_node(false);  // Start/outside formatting
    int q1 = pda.add_node(false);  // Inside level 1 formatting (bold/strikethrough)
    int q2 = pda.add_node(false);  // Inside level 2 formatting (italic inside bold)
    int q3 = pda.add_node(false);  // Error state (mismatch)
    int q4 = pda.add_node(true);   // Accept state
    
    // Stack symbols: 
    // B = ** (bold), I = * (italic), S = ~~ (strikethrough), P = (, [, {, <
    
    // === OPENING TRANSITIONS ===
    
    // Enter bold (**)
    pda.add_transition(q0, q1, '*', 0, "B");
    pda.add_transition(q0, q1, '*', 0, "B");
    
    // Enter italic (*) - can be inside or outside
    pda.add_transition(q0, q1, '*', 0, "I");
    pda.add_transition(q1, q2, '*', 0, "I");  // Nested italic inside bold
    
    // Enter strikethrough (~~)
    pda.add_transition(q0, q1, '~', 0, "S");
    pda.add_transition(q0, q1, '~', 0, "S");
    
    // Enter brackets
    pda.add_transition(q0, q1, '(', 0, "P");
    pda.add_transition(q0, q1, '[', 0, "P");
    pda.add_transition(q0, q1, '{', 0, "P");
    pda.add_transition(q0, q1, '<', 0, "P");
    
    // === CLOSING TRANSITIONS ===
    
    // Close bold (**) - must match B on stack
    pda.add_transition(q1, q0, '*', 'B', "");
    pda.add_transition(q1, q0, '*', 'B', "");
    
    // Close italic (*) - can be I or B
    pda.add_transition(q1, q0, '*', 'I', "");  // Close italic at level 1
    pda.add_transition(q2, q1, '*', 'I', "");  // Close nested italic, return to bold
    
    // Close strikethrough (~~) - must match S on stack
    pda.add_transition(q1, q0, '~', 'S', "");
    pda.add_transition(q1, q0, '~', 'S', "");
    
    // Close brackets - must match corresponding opening
    pda.add_transition(q1, q0, ')', '(', "");
    pda.add_transition(q1, q0, ']', '[', "");
    pda.add_transition(q1, q0, '}', '{', "");
    pda.add_transition(q1, q0, '>', '<', "");
    
    // === ERROR TRANSITIONS ===
    
    // Mismatch closing - goes to error state
    if (strict_nesting) {
        pda.add_transition(q0, q3, '*', 0, "");  // Closing * without opening
        pda.add_transition(q0, q3, '~', 0, "");
        pda.add_transition(q0, q3, ')', 0, "");
        pda.add_transition(q0, q3, ']', 0, "");
        pda.add_transition(q0, q3, '}', 0, "");
        pda.add_transition(q0, q3, '>', 0, "");
        
        // Invalid nesting
        pda.add_transition(q1, q3, '*', 'I', "");  // Try to close bold with *
        pda.add_transition(q1, q3, '*', 'B', "");  // Try to close italic with **
    }
    
    // === ACCEPT TRANSITIONS ===
    
    // Accept when stack is empty (only $ marker remains)
    pda.add_transition(q0, q4, 0, '$', "");  // End of input
    pda.add_transition(q1, q4, 0, '$', "");
    
    // Self-loop for text characters (not formatting)
    for (char c = 32; c <= 126; c++) {
        if (c != '*' && c != '~' && c != '(' && c != ')' && 
            c != '[' && c != ']' && c != '{' && c != '}' && 
            c != '<' && c != '>') {
            pda.add_transition(q0, q0, c, 0, "");  // Stay in q0 for text
            pda.add_transition(q1, q1, c, 0, "");  // Stay in q1 for text inside formatting
            pda.add_transition(q2, q2, c, 0, "");  // Stay in q2 for text inside nested formatting
        }
    }
    
    return pda;
}

// ==================== SIMULATION WITH STACK TRACKING ====================

bool PDA::simulate_markdown(const string& input, 
                           vector<pair<int, int>>& error_positions,
                           string& error_message) {
    
    struct StackFrame {
        char symbol;      // B, I, S, P
        int position;     // Position in input where opened
        int level;        // Nesting level
    };
    
    stack<StackFrame> stk;
    stk.push({'$', -1, 0});  // Bottom marker
    
    int current_state = start_state;
    bool error = false;
    
    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        
        // Check for multi-character symbols
        bool processed_multi = false;
        
        // Check for ** (bold)
        if (c == '*' && i + 1 < input.length() && input[i + 1] == '*') {
            if (!stk.empty() && stk.top().symbol == 'I') {
                // Cannot open ** inside * (invalid nesting)
                error_positions.push_back({static_cast<int>(i), static_cast<int>(i + 1)});
                error_message = "Invalid nesting: bold (**) cannot be inside italic (*)";
                return false;
            }
            
            // Opening **
            stk.push({'B', static_cast<int>(i), static_cast<int>(stk.size())});
            i++; // Skip second *
            processed_multi = true;
        }
        // Check for ~~ (strikethrough)
        else if (c == '~' && i + 1 < input.length() && input[i + 1] == '~') {
            stk.push({'S', static_cast<int>(i), static_cast<int>(stk.size())});
            i++;
            processed_multi = true;
        }
        
        if (!processed_multi) {
            // Single character symbols
            if (c == '*') {
                if (!stk.empty() && stk.top().symbol == 'B') {
                    // Opening italic inside bold
                    stk.push({'I', static_cast<int>(i), static_cast<int>(stk.size())});
                } else if (!stk.empty() && stk.top().symbol == 'I') {
                    // Closing italic
                    if (stk.top().symbol != 'I') {
                        error_positions.push_back({static_cast<int>(i), static_cast<int>(i)});
                        error_message = "Mismatch: expected to close italic (*)";
                        return false;
                    }
                    stk.pop();
                } else {
                    // Opening italic at top level
                    stk.push({'I', static_cast<int>(i), static_cast<int>(stk.size())});
                }
            }
            // Handle brackets
            else if (c == '(' || c == '[' || c == '{' || c == '<') {
                stk.push({'P', static_cast<int>(i), static_cast<int>(stk.size())});
            }
            else if (c == ')' || c == ']' || c == '}' || c == '>') {
                if (stk.empty() || stk.top().symbol != 'P') {
                    error_positions.push_back({static_cast<int>(i), static_cast<int>(i)});
                    error_message = "Mismatch closing bracket";
                    return false;
                }
                // Check for proper bracket matching
                char opening = input[stk.top().position];
                if ((c == ')' && opening != '(') ||
                    (c == ']' && opening != '[') ||
                    (c == '}' && opening != '{') ||
                    (c == '>' && opening != '<')) {
                    error_positions.push_back({stk.top().position, static_cast<int>(i)});
                    error_message = "Bracket type mismatch";
                    return false;
                }
                stk.pop();
            }
        }
    }
    
    // Check for unclosed formatting
    while (!stk.empty() && stk.top().symbol != '$') {
        StackFrame unclosed = stk.top();
        stk.pop();
        
        if (unclosed.symbol == 'B') {
            error_positions.push_back({unclosed.position, unclosed.position + 1});
            error_message = "Unclosed bold formatting (**)";
        } else if (unclosed.symbol == 'I') {
            error_positions.push_back({unclosed.position, unclosed.position});
            error_message = "Unclosed italic formatting (*)";
        } else if (unclosed.symbol == 'S') {
            error_positions.push_back({unclosed.position, unclosed.position + 1});
            error_message = "Unclosed strikethrough formatting (~~)";
        } else if (unclosed.symbol == 'P') {
            error_positions.push_back({unclosed.position, unclosed.position});
            error_message = "Unclosed bracket";
        }
    }
    
    return error_positions.empty();
}

// ==================== INJECTION ATTEMPT DETECTION ====================

bool PDA::detect_injection_attempts(const string& input, 
                                   vector<string>& warnings) {
    
    vector<pair<int, int>> error_positions;
    string error_msg;
    
    // First check for valid nesting
    bool valid = simulate_markdown(input, error_positions, error_msg);
    
    if (!valid) {
        warnings.push_back("Invalid formatting structure: " + error_msg);
    }
    
    // Check for suspicious patterns
    
    // 1. Odd number of asterisks (could be broken formatting)
    int asterisk_count = count(input.begin(), input.end(), '*');
    if (asterisk_count % 2 != 0 && asterisk_count > 0) {
        warnings.push_back("Warning: Odd number of asterisks - possible broken formatting");
    }
    
    // 2. JavaScript/HTML injection patterns
    if (input.find("<script") != string::npos || 
        input.find("javascript:") != string::npos ||
        input.find("onload=") != string::npos ||
        input.find("onclick=") != string::npos) {
        warnings.push_back("ALERT: Potential script injection detected");
        return true;
    }
    
    // 3. SQL injection patterns (simplified)
    if (input.find("' OR '1'='1") != string::npos ||
        input.find("DROP TABLE") != string::npos ||
        input.find("UNION SELECT") != string::npos) {
        warnings.push_back("ALERT: Potential SQL injection pattern");
        return true;
    }
    
    // 4. Excessive nesting (could be DoS attempt)
    int nesting_level = 0;
    int max_nesting = 0;
    for (char c : input) {
        if (c == '(' || c == '[' || c == '{' || c == '<') {
            nesting_level++;
            max_nesting = max(max_nesting, nesting_level);
        } else if (c == ')' || c == ']' || c == '}' || c == '>') {
            nesting_level--;
        }
    }
    
    if (max_nesting > 10) {
        warnings.push_back("Warning: Excessive nesting depth (" + 
                          to_string(max_nesting) + " levels)");
    }
    
    // 5. Mixed encoding attempts
    if (input.find("%3C") != string::npos ||  // URL encoded <
        input.find("%3E") != string::npos ||  // URL encoded >
        input.find("&lt;") != string::npos || // HTML encoded <
        input.find("&gt;") != string::npos) { // HTML encoded >
        warnings.push_back("Warning: Mixed encoding detected");
    }
    
    return warnings.size() > 0;
}

// ==================== TOXIC DETECTION PDA (KEEPS YOUR EXISTING) ====================

PDA BracketPDA::create_toxic_detection_pda(const vector<string>& toxic_patterns, int max_edits) {
    PDA pda;
    
    // Create 5 states to match your diagram
    int q0 = pda.add_node(false);  // Start/Scan (state 0)
    int q1 = pda.add_node(false);  // Inside Brackets (state 1)
    int q2 = pda.add_node(false);  // Toxic Detected (state 2)
    int q3 = pda.add_node(false);  // Intermediate (state 3)
    int q4 = pda.add_node(true);   // Accept (state 4 - final
    
    // Opening brackets (push)
    pda.add_transition(q0, q1, '(', '$', "($");
    pda.add_transition(q0, q1, '[', '$', "[$");
    pda.add_transition(q0, q1, '{', '$', "{$");
    pda.add_transition(q0, q1, '<', '$', "<$");
    
    // Closing brackets (pop)
    pda.add_transition(q1, q0, ')', '(', "");
    pda.add_transition(q1, q0, ']', '[', "");
    pda.add_transition(q1, q0, '}', '{', "");
    pda.add_transition(q1, q0, '>', '<', "");
    
    // Word boundary (space) - trigger toxicity check
    pda.add_transition(q1, q2, ' ', 0, "");
    pda.add_transition(q2, q1, 0, 0, "");
    
    // Accept transitions
    pda.add_transition(q0, q3, 0, '$', "");  // End with balanced brackets
    pda.add_transition(q1, q3, 0, '$', "");
    pda.add_transition(q2, q3, 0, '$', "");
    
    // Self-loops for text characters
    for (char c = 'a'; c <= 'z'; c++) {
        pda.add_transition(q0, q0, c, 0, "");
        pda.add_transition(q1, q1, c, 0, "");
        pda.add_transition(q2, q2, c, 0, "");
    }
    
    return pda;
}

// ==================== BALANCED BRACKET PDA ====================

PDA BracketPDA::create_balanced_bracket_pda() {
    PDA pda;
    
    // Create states
    int q0 = pda.add_node(false);  // Start state
    int q1 = pda.add_node(true);   // Accept state
    
    // Push stack marker
    pda.add_transition(q0, q0, 0, 0, "$");
    
    // Opening brackets - push onto stack
    pda.add_transition(q0, q0, '(', 0, "(");
    pda.add_transition(q0, q0, '[', 0, "[");
    pda.add_transition(q0, q0, '{', 0, "{");
    pda.add_transition(q0, q0, '<', 0, "<");
    
    // Closing brackets - pop from stack if matching
    pda.add_transition(q0, q0, ')', '(', "");
    pda.add_transition(q0, q0, ']', '[', "");
    pda.add_transition(q0, q0, '}', '{', "");
    pda.add_transition(q0, q0, '>', '<', "");
    
    // Accept when stack has only $ marker left
    pda.add_transition(q0, q1, 0, '$', "");
    
    return pda;
}

// ==================== DOT GENERATION ====================

std::string PDA::toDot() const {
    std::string dot = "digraph PDA {\n";
    dot += "  rankdir=LR;\n";
    dot += "  node [shape=circle];\n";
    
    // Title
    dot += "  labelloc=\"t\";\n";
    dot += "  label=\"Pushdown Automaton (Context-Free)\\n";
    dot += "Recognizes balanced brackets with toxic word detection\\n";
    dot += "Language class: CONTEXT-FREE (requires stack)\";\n";
    dot += "  fontsize=12;\n";
    
    // States - using your exact labels
    dot += "  0 [label=\"q0\\nStart/Scan\", color=\"blue\"];\n";
    dot += "  1 [label=\"q1\\nInside Brackets\", color=\"blue\"];\n";
    dot += "  2 [label=\"q2\\nToxic Detected\", color=\"blue\"];\n";
    dot += "  3 [label=\"q3\\nAccept\", color=\"blue\"];\n";
    dot += "  4 [label=\"q4\", shape=doublecircle, color=\"green\"];\n";
    
    // Transitions - exactly as you specified
    dot += "  1 -> 1 [label=\"ε / $ → $\", color=\"black\"];\n";
    dot += "  1 -> 2 [label=\"( / $ → $(\", color=\"orange\"];\n";
    dot += "  1 -> 2 [label=\"[ / $ → $[\", color=\"orange\"];\n";
    dot += "  1 -> 2 [label=\"{ / $ → ${\", color=\"orange\"];\n";
    dot += "  1 -> 2 [label=\"< / $ → $<\", color=\"orange\"];\n";
    dot += "  1 -> 4 [label=\"ε / $ → \", color=\"black\"];\n";
    
    dot += "  2 -> 2 [label=\"ε / $ → $\", color=\"black\"];\n";
    dot += "  2 -> 1 [label=\") / ( → \", color=\"red\"];\n";
    dot += "  2 -> 1 [label=\"] / [ → \", color=\"red\"];\n";
    dot += "  2 -> 1 [label=\"} / { → \", color=\"red\"];\n";
    dot += "  2 -> 1 [label=\"> / < → \", color=\"red\"];\n";
    dot += "  2 -> 3 [label=\"space / $ → $\", color=\"purple\"];\n";
    dot += "  2 -> 2 [label=\"space / $ → $\", color=\"purple\"];\n";
    
    dot += "  3 -> 2 [label=\"ε / $ → $\", color=\"black\"];\n";
    dot += "  3 -> 4 [label=\"ε / $ → \", color=\"black\"];\n";
    
    // Start arrow
    dot += "  start [shape=point];\n";
    dot += "  start -> 0;\n";
    
    // Legend
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

// ==================== SIMULATE WITH TOXICITY (YOU NEED THIS TOO) ====================

bool PDA::simulate_with_toxicity(const std::string& input, 
                                ApproximateMatcher& matcher,
                                const std::vector<std::string>& toxic_patterns,
                                int max_edits,
                                std::vector<std::string>& found_toxic_words) {
    
    bool balanced = simulate(input);
    
    if (!balanced) {
        return false;
    }
    
    // Extract content inside brackets
    std::stack<char> bracket_stack;
    std::string current_content;
    
    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        
        if (c == '(' || c == '[' || c == '{' || c == '<') {
            bracket_stack.push(c);
            if (bracket_stack.size() == 1) {
                current_content.clear();  // Start new content
            }
        } else if (c == ')' && !bracket_stack.empty() && bracket_stack.top() == '(') {
            bracket_stack.pop();
            if (bracket_stack.empty() && !current_content.empty()) {
                // Check content for toxic words
                for (const auto& pattern : toxic_patterns) {
                    // Use find_matches instead of approximate_match
                    auto matches = matcher.find_matches(current_content, pattern, max_edits);
                    if (!matches.empty()) {
                        found_toxic_words.push_back("Found '" + pattern + 
                                                  "' in: " + current_content);
                    }
                }
            }
        } else if (c == ']' && !bracket_stack.empty() && bracket_stack.top() == '[') {
            bracket_stack.pop();
            if (bracket_stack.empty() && !current_content.empty()) {
                for (const auto& pattern : toxic_patterns) {
                    auto matches = matcher.find_matches(current_content, pattern, max_edits);
                    if (!matches.empty()) {
                        found_toxic_words.push_back("Found '" + pattern + 
                                                  "' in: " + current_content);
                    }
                }
            }
        } else if (c == '}' && !bracket_stack.empty() && bracket_stack.top() == '{') {
            bracket_stack.pop();
            if (bracket_stack.empty() && !current_content.empty()) {
                for (const auto& pattern : toxic_patterns) {
                    auto matches = matcher.find_matches(current_content, pattern, max_edits);
                    if (!matches.empty()) {
                        found_toxic_words.push_back("Found '" + pattern + 
                                                  "' in: " + current_content);
                    }
                }
            }
        } else if (c == '>' && !bracket_stack.empty() && bracket_stack.top() == '<') {
            bracket_stack.pop();
            if (bracket_stack.empty() && !current_content.empty()) {
                for (const auto& pattern : toxic_patterns) {
                    auto matches = matcher.find_matches(current_content, pattern, max_edits);
                    if (!matches.empty()) {
                        found_toxic_words.push_back("Found '" + pattern + 
                                                  "' in: " + current_content);
                    }
                }
            }
        } else if (!bracket_stack.empty()) {
            current_content += c;  // Collect content inside brackets
        }
    }
    
    return balanced && found_toxic_words.empty();
}

// ==================== DOT GENERATION WITH NESTING VISUALIZATION ====================

string PDA::toDotNested() const {
    string dot = "digraph NestedStructurePDA {\n";
    dot += "  rankdir=LR;\n";
    dot += "  node [shape=Mrecord];\n";
    
    // Title
    dot += "  labelloc=\"t\";\n";
    dot += "  label=\"Nested Structure PDA\\n";
    dot += "Validates: **bold**, *italic*, ~~strikethrough~~, (nested brackets)\\n";
    dot += "Detects: Mismatches, Injection attempts, Broken formatting\";\n";
    dot += "  fontsize=14;\n";
    
    // States with descriptions
    for (const auto& node : nodes) {
        dot += "  q" + to_string(node.id) + " [label=\"";
        
        // State descriptions
        if (node.id == 0) dot += "Outside Formatting\\n(Scan text)";
        else if (node.id == 1) dot += "Inside Formatting\\n(Bold/Strikethrough/Brackets)";
        else if (node.id == 2) dot += "Nested Inside\\n(Italic inside Bold)";
        else if (node.id == 3) dot += "ERROR\\n(Mismatch detected)";
        else if (node.id == 4) dot += "ACCEPT\\n(Valid structure)";
        else dot += "State q" + to_string(node.id);
        
        // Visual styling
        if (node.id == 3) {
            dot += "\", shape=octagon, color=red, fillcolor=red, style=filled";
        } else if (node.id == 4) {
            dot += "\", shape=doublecircle, color=green, fillcolor=lightgreen, style=filled";
        } else if (node.is_final) {
            dot += "\", shape=doublecircle, color=blue";
        } else {
            dot += "\", color=black";
        }
        
        dot += "];\n";
    }
    
    // Transitions
    for (const auto& node : nodes) {
        for (const auto& trans : node.transitions) {
            char input = get<0>(trans);
            char pop = get<1>(trans);
            string push = get<2>(trans);
            int to = get<3>(trans);
            
            string label;
            if (input == 0) {
                label = "ε";
            } else {
                label = string(1, input);
            }
            
            label += " / ";
            if (pop == 0) label += "ε";
            else label += string(1, pop);
            
            label += " → ";
            if (push.empty()) label += "ε";
            else label += push;
            
            // Color code by operation type
            string color = "black";
            if (!push.empty() && push.find('$') == string::npos) {
                color = "orange";  // Push operations
            } else if (pop != 0 && pop != '$') {
                color = "red";     // Pop operations
            } else if (input == '*') {
                color = "purple";  // Formatting operations
            } else if (input == '~') {
                color = "brown";
            }
            
            dot += "  q" + to_string(node.id) + " -> q" + to_string(to) +
                   " [label=\"" + label + "\", color=\"" + color + "\"];\n";
        }
    }
    
    // Start arrow
    dot += "  start [shape=point];\n";
    dot += "  start -> q0;\n";
    
    // Legend
    dot += "  subgraph cluster_legend {\n";
    dot += "    label=\"PDA Operations\";\n";
    dot += "    style=filled;\n";
    dot += "    fillcolor=lightgrey;\n";
    dot += "    node [shape=plaintext];\n";
    dot += "    legend1 [label=\"Orange: Push onto stack\"];\n";
    dot += "    legend2 [label=\"Red: Pop from stack\"];\n";
    dot += "    legend3 [label=\"Purple: Bold/Italic (*)\"];\n";
    dot += "    legend4 [label=\"Brown: Strikethrough (~)\"];\n";
    dot += "    legend5 [label=\"Black: Scan characters\"];\n";
    dot += "  }\n";
    
    dot += "}\n";
    return dot;
}

// ==================== SIMPLE BRACKET CHECK (LEGACY) ====================

bool PDA::simulate(const string& input) {
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