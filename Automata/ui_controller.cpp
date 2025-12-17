#include "ui_controller.hpp"
#include "nfa_engine.hpp"
#include "approximate_matcher.hpp"
#include "pda_engine.hpp"
#include "dfa_engine.hpp"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <filesystem>
#include <regex>
#include <cctype>
#include <algorithm>  // for std::remove, std::remove_if
#include <cstring>    // for strlen
#include <ctime>      // for time, localtime, strftime
#include <cctype>     // for ispunct, isalpha, tolower
#include <regex>      // for regex_search
#include <type_traits>  // for std::is_same_v


namespace fs = std::filesystem;
using namespace std;

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"
#define RESET   "\033[0m"

void ChatModerationUI::run() {
    cout << CYAN << "\n";
    cout << "   CHAT MODERATION SYSTEM\n";
    cout << "   Formal Languages & Automata\n"<< RESET;

    while (true) {
        print_menu();
        int choice = get_choice(); // always valid 1-6

        switch (choice) {
            case 1: analyze_toxic_phrases(); break;
            case 2: analyze_disguised_toxicity(); break;
            case 3: validate_structures(); break;
            case 4: analyze_chat_log(); break;
            case 5:
                cout << GREEN << "Exiting system. Goodbye!\n" << RESET;
                return;
        }
    }
}

void ChatModerationUI::print_menu() {
    cout << "\n" << BLUE << "MAIN MENU:\n" << RESET;
    cout << "1. Scan for Toxic Phrases (Regex -> NFA)\n";
    cout << "2. Scan for Disguised Toxicity (Approx Matching)\n";
    cout << "3. Validate Nested Chat Structures (PDA)\n";
    cout << "4. Analyze Chat Log File\n";
    cout << "5. Exit\n";
    cout << YELLOW << "Enter your choice (1-6): " << RESET;
}

int ChatModerationUI::get_choice() {
    string input;
    int choice = 0;

    while (true) {
        getline(cin, input);
        input.erase(0, input.find_first_not_of(" \t\n\r"));
        input.erase(input.find_last_not_of(" \t\n\r") + 1);

        if (input.empty() || !all_of(input.begin(), input.end(), ::isdigit)) {
            cout << RED << "Invalid input. Enter a number 1-6.\n" << RESET;
            cout << YELLOW << "Enter your choice (1-6): " << RESET;
            continue;
        }

        choice = stoi(input);
        if (choice < 1 || choice > 6) {
            cout << RED << "Invalid choice. Enter a number 1-6.\n" << RESET;
            cout << YELLOW << "Enter your choice (1-6): " << RESET;
            continue;
        }
        break;
    }
    return choice;
}

void ChatModerationUI::analyze_toxic_phrases() {
    cout << "\n" << CYAN << "TOXIC PHRASE DETECTION (Regex -> NFA/DFA)\n" << RESET;

    cout << "Enter regex pattern to match toxic phrases ";
    cout << YELLOW << ">> " << RESET;
    string regex_pattern;
    getline(cin, regex_pattern);

    if (regex_pattern.empty()) {
        cout << RED << "Error: Regex pattern cannot be empty!\n" << RESET;
        return;
    }

    try {
        cout << GREEN << " Building NFA from regex...\n" << RESET;
        NFA toxic_nfa = RegexToNFA::from_regex(regex_pattern);
        cout << GREEN << " NFA built successfully!\n" << RESET;

        toxic_nfa.print_transitions();

        cout << "\n" << CYAN << "OPTIMIZATION OPTIONS:\n" << RESET;
        cout << "Do you want to convert NFA to DFA for faster matching? (y/n): ";
        string input; 
        getline(cin, input);
        char dfa_choice = !input.empty() ? tolower(input[0]) : 'n';

        DFA toxic_dfa;
        bool use_dfa = false;
        
        if (dfa_choice == 'y') {
            cout << GREEN << " Converting NFA to DFA...\n" << RESET;
            toxic_dfa = convert_nfa_to_dfa(toxic_nfa);
            use_dfa = true;
            cout << GREEN << " DFA conversion completed.\n" << RESET;
            
            cout << "\n" << CYAN << "DFA Statistics:\n" << RESET;
            cout << "Number of states: " << toxic_dfa.get_states().size() << "\n";
            int dead_states = 0;
            for (const auto& state : toxic_dfa.get_states()) {
                bool is_dead = true;
                for (const auto& trans : state.transitions) {
                    if (trans.second != state.id) {
                        is_dead = false;
                        break;
                    }
                }
                if (is_dead && !state.is_final) dead_states++;
            }
            cout << "Dead states: " << dead_states << "\n";
        }

        // Test message input
        cout << "\n" << CYAN << "TEST MESSAGE ANALYSIS:\n" << RESET;
        cout << "Enter message to analyze :\n";
        cout << YELLOW << ">> " << RESET;
        string message;
        getline(cin, message);
        
        string lower_msg = message;
        transform(lower_msg.begin(), lower_msg.end(), lower_msg.begin(), ::tolower);

        // Find all matches in the message
        vector<pair<int,int>> match_positions;
        for (size_t i = 0; i < lower_msg.size(); ++i) {
            for (size_t j = i+1; j <= lower_msg.size(); ++j) {
                string substr = lower_msg.substr(i, j-i);
                bool matched = use_dfa ? toxic_dfa.simulate(substr) : toxic_nfa.simulate(substr);
                if (matched) {
                    // Avoid overlapping matches
                    bool overlap = false;
                    for (const auto& [s, e] : match_positions) {
                        if ((i >= s && i < e) || (j > s && j <= e)) {
                            overlap = true;
                            break;
                        }
                    }
                    if (!overlap) {
                        match_positions.push_back({static_cast<int>(i), static_cast<int>(j)});
                    }
                }
            }
        }

        // Highlight the message
        string highlighted = message;
        int offset = 0;
        sort(match_positions.begin(), match_positions.end());
        for (auto [start, end] : match_positions) {
            highlighted.insert(start + offset, RED);
            offset += strlen(RED);
            highlighted.insert(end + offset, RESET);
            offset += strlen(RESET);
        }

        bool has_matches = !match_positions.empty();
        cout << "\n" << CYAN << "ANALYSIS RESULTS:\n" << RESET;
        cout << "Message: \"" << message << "\"\n";
        cout << "Pattern matched: " << (has_matches ? RED "YES" : GREEN "NO") << RESET << "\n";

        if (has_matches) {
            cout << "Matched positions:\n";
            for (const auto& [start, end] : match_positions) {
                cout << "  - Position " << start << "-" << end << ": \"" 
                     << message.substr(start, end-start) << "\"\n";
            }
            cout << "Highlighted Message: " << highlighted << "\n";
            cout << YELLOW << " Warning: Toxic content detected!\n" << RESET;
        } else {
            cout << GREEN << " No toxic content found.\n" << RESET;
        }

        // Diagram generation
        cout << "\n" << CYAN << "VISUALIZATION OPTIONS:\n" << RESET;
        if (use_dfa) {
            cout << "Generate DFA diagram for the input? (y/n): ";
            getline(cin, input);
            if (!input.empty() && tolower(input[0]) == 'y') {
                string dfa_dot = "toxic_dfa.dot";
                string dfa_png = "toxic_dfa.png";
                
                ofstream fout(dfa_dot);
                if (fout.is_open()) {
                    // Generate DOT with the actual input path highlighted
                    fout << toxic_dfa.toDotWithInput(lower_msg);
                    fout.close();
                    
                    string command = "dot -Tpng \"" + dfa_dot + "\" -o \"" + dfa_png + "\"";
                    cout << GREEN << "Generating DFA diagram...\n" << RESET;
                    
                    int result = system(command.c_str());
                    if (result == 0) {
                        cout << GREEN << "DFA Diagram generated: " << dfa_png << "\n" << RESET;
                        
                        // Try to open the image
#ifdef _WIN32
                        ShellExecuteA(NULL, "open", dfa_png.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
                        string open_cmd = "xdg-open \"" + dfa_png + "\" &";
                        system(open_cmd.c_str());
#endif
                    } else {
                        cout << YELLOW << "Note: Graphviz 'dot' command not found or failed.\n";
                        cout << "Install Graphviz from: https://graphviz.org/download/\n" << RESET;
                    }
                }
            }
        } else {
            cout << "Generate NFA diagram? (y/n): ";
            getline(cin, input);
            if (!input.empty() && tolower(input[0]) == 'y') {
                string nfa_dot = "toxic_nfa.dot";
                string nfa_png = "toxic_nfa.png";
                
                ofstream fout(nfa_dot);
                if (fout.is_open()) {
                    fout << toxic_nfa.toDot();
                    fout.close();
                    
                    string command = "dot -Tpng \"" + nfa_dot + "\" -o \"" + nfa_png + "\"";
                    cout << GREEN << " Generating NFA diagram...\n" << RESET;
                    
                    int result = system(command.c_str());
                    if (result == 0) {
                        cout << GREEN << " NFA Diagram generated: " << nfa_png << "\n" << RESET;
                        
#ifdef _WIN32
                        ShellExecuteA(NULL, "open", nfa_png.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
                        string open_cmd = "xdg-open \"" + nfa_png + "\" &";
                        system(open_cmd.c_str());
#endif
                    } else {
                        cout << YELLOW << "Note: Graphviz 'dot' command not found.\n" << RESET;
                    }
                }
            }
        }


    } catch (const exception& e) {
        cout << RED << "Error: " << e.what() << "\n" << RESET;
        cout << "Invalid regex pattern. Please try again.\n";
    }
}

void ChatModerationUI::analyze_disguised_toxicity() {
    cout << "\n" << CYAN << "APPROXIMATE PATTERN MATCHING\n" << RESET;
    cout << "==========================================\n\n";

    cout << "Enter toxic pattern to search for (e.g., 'idiot'):\n";
    cout << YELLOW << ">> " << RESET;
    string pattern;
    getline(cin, pattern);

    if (pattern.empty()) {
        cout << RED << "Error: Pattern cannot be empty!\n" << RESET;
        return;
    }

    cout << "Enter message to analyze:\n";
    cout << YELLOW << ">> " << RESET;
    string message;
    getline(cin, message);

    if (message.empty()) {
        message = "Please enter a message";
        cout << YELLOW << "Using test message: \"" << message << "\"\n" << RESET;
    }

    cout << "Max edit distance (0-3, default=1): ";
    string edit_dist_str;
    getline(cin, edit_dist_str);
    int max_edits = 1;
    try {
        max_edits = stoi(edit_dist_str);
        if (max_edits < 0 || max_edits > 3) {
            max_edits = 1;
            cout << YELLOW << "Using default edit distance: 1\n" << RESET;
        }
    } catch (...) {
        max_edits = 1;
        cout << YELLOW << "Using default edit distance: 1\n" << RESET;
    }

    ApproximateMatcher matcher;
    
    cout << "\n" << CYAN << "ANALYSIS IN PROGRESS...\n" << RESET;
    
    auto matches = matcher.find_matches(message, pattern, max_edits);

    if (matches.empty()) {
        cout << GREEN << "\n No approximate matches found.\n" << RESET;
        return;
    }

    cout << "\n" << CYAN << "APPROXIMATE MATCHES FOUND:\n" << RESET;
    cout << "Pattern: \"" << pattern << "\" (max edit distance: " << max_edits << ")\n";
    cout << "Message: \"" << message << "\"\n";
    cout << "------------------------------------------\n";

    // Group matches by matched pattern
    unordered_map<string, vector<ApproximateMatcher::MatchResult>> grouped_matches;
    for (const auto& match : matches) {
        grouped_matches[match.matched_pattern].push_back(match);
    }

    // Highlight message with approximate matches
    string highlighted_msg = message;
    vector<pair<int, int>> match_positions;
    
    for (const auto& [matched_pattern, pattern_matches] : grouped_matches) {
        for (const auto& match : pattern_matches) {
            // Find position in original message
            size_t pos = highlighted_msg.find(match.original);
            if (pos != string::npos) {
                match_positions.push_back({static_cast<int>(pos), 
                                          static_cast<int>(pos + match.original.length())});
            }
        }
    }
    
    // Sort and highlight
    sort(match_positions.begin(), match_positions.end());
    int offset = 0;
    for (const auto& [start, end] : match_positions) {
        highlighted_msg.insert(start + offset, YELLOW);
        offset += strlen(YELLOW);
        highlighted_msg.insert(end + offset, RESET);
        offset += strlen(RESET);
    }

    cout << "Highlighted Message: " << highlighted_msg << "\n\n";

    // Display matches
    int match_count = 1;
    for (const auto& [matched_pattern, pattern_matches] : grouped_matches) {
        cout << YELLOW << "Match Group " << match_count++ << ":\n" << RESET;
        cout << "  Target Pattern: \"" << matched_pattern << "\"\n";
        
        for (const auto& match : pattern_matches) {
            cout << "  * Found: \"" << match.original << "\"";
            if (match.distance > 0) {
                cout << " [" << match.distance << " edit";
                if (match.distance != 1) cout << "s";
                cout << ", " << fixed << setprecision(0) << match.similarity << "% similar]";
            } else {
                cout << " [exact match]";
            }
            cout << "\n";
        }
        cout << "\n";
    }

    // Generate FSM visualization
    cout << CYAN << "GENERATE FINITE STATE MACHINE DIAGRAM?\n" << RESET;
    cout << "Create FSM visualization for approximate matching? (y/n): ";
    string viz_choice;
    getline(cin, viz_choice);
    
    if (!viz_choice.empty() && tolower(viz_choice[0]) == 'y') {
        string dot_filename = "approx_fsm_" + sanitize(pattern) + ".dot";
        string png_filename = "approx_fsm_" + sanitize(pattern) + ".png";
        
        ofstream fout(dot_filename);
        if (fout.is_open()) {
            fout << matcher.toDotRegexFSM(pattern, max_edits);
            fout.close();
            
            string command = "dot -Tpng \"" + dot_filename + "\" -o \"" + png_filename + "\"";
            
            if (system(command.c_str()) == 0) {
                cout << GREEN << "FSM Diagram generated: " << png_filename << "\n" << RESET;
                
#ifdef _WIN32
                ShellExecuteA(NULL, "open", png_filename.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
                string open_cmd = "xdg-open \"" + png_filename + "\" &";
                system(open_cmd.c_str());
#endif
            } else {
                cout << YELLOW << "Note: Graphviz 'dot' command not found.\n" << RESET;
            }
        }
    }
}

string ChatModerationUI::sanitize(const std::string& s) {
    std::string result = s;
    for (auto& c : result) {
        if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' ||
            c == '"'  || c == '<' || c == '>' || c == '|') c = '_';
    }
    return result;
}


void ChatModerationUI::validate_structures() {
    cout << "\n" << CYAN << "STRUCTURE VALIDATION (PDA + Approx Matching)\n" << RESET;
    cout << "==============================================\n\n";
    
    // Get regex for toxic patterns inside brackets
    cout << "Enter regex pattern for toxic content inside brackets:\n";
    cout << "Examples:\n";
    cout << "  Simple: (bad|hate|stupid|evil)\n";
    cout << "  Variations: (f[uv]ck|sh[i1]t|@ss|h[4a]te)\n";
    cout << "  All toxic: (bad|hate|stupid|fuck|shit|ass|damn|idiot|hell|crap)\n";
    cout << YELLOW << ">> " << RESET;
    string regex_pattern;
    getline(cin, regex_pattern);
    
    vector<string> toxic_patterns;
    if (regex_pattern.empty()) {
        toxic_patterns = {"bad", "hate", "stupid", "evil", "fuck", "shit", "ass", "damn", "idiot", "crap"};
        cout << YELLOW << "Using default toxic patterns\n" << RESET;
    } else {
        toxic_patterns = extract_patterns_from_regex(regex_pattern);
        if (toxic_patterns.empty()) {
            // Fallback: try to extract patterns between parentheses
            size_t start = regex_pattern.find('(');
            size_t end = regex_pattern.find(')');
            if (start != string::npos && end != string::npos && end > start) {
                string pattern_str = regex_pattern.substr(start + 1, end - start - 1);
                size_t pos = 0;
                while ((pos = pattern_str.find('|')) != string::npos) {
                    toxic_patterns.push_back(pattern_str.substr(0, pos));
                    pattern_str.erase(0, pos + 1);
                }
                if (!pattern_str.empty()) {
                    toxic_patterns.push_back(pattern_str);
                }
            }
            
            if (toxic_patterns.empty()) {
                // If still empty, just use the regex as a single pattern
                toxic_patterns.push_back(regex_pattern);
            }
        }
    }
    
    // Display extracted patterns
    cout << "\n" << GREEN << " Extracted " << toxic_patterns.size() << " pattern(s):\n" << RESET;
    for (size_t i = 0; i < toxic_patterns.size(); i++) {
        cout << "  " << i+1 << ". \"" << toxic_patterns[i] << "\"\n";
    }
    
    // Get max edit distance for approximate matching
    cout << "\nMax edit distance for approximate matching inside brackets (0-3): ";
    string edit_dist_str;
    getline(cin, edit_dist_str);
    int max_edits = 1;
    try {
        max_edits = stoi(edit_dist_str);
        if (max_edits < 0 || max_edits > 3) {
            max_edits = 1;
            cout << YELLOW << "Using default edit distance: 1\n" << RESET;
        }
    } catch (...) {
        max_edits = 1;
        cout << YELLOW << "Using default edit distance: 1\n" << RESET;
    }
    
    // Get input string to validate
    cout << "\nEnter a string with brackets to validate:\n";
    cout << "Examples:\n";
    cout << "  Valid toxic: (hate), [stupid], {bad}\n";
    cout << "  Valid non-toxic: (hello), [good], {nice}\n";
    cout << "  Invalid structure: (unclosed, no_close), (mis)matched]\n";
    cout << "  Mixed: (good) and [bad] and {evil}\n";
    cout << YELLOW << ">> " << RESET;
    string user_input;
    getline(cin, user_input);
    
    if (user_input.empty()) {
        user_input = "(hello) this is a [test] with {hate} and (stup1d) content";
        cout << YELLOW << "Using test string: \"" << user_input << "\"\n" << RESET;
    }
    
    cout << "\n" << BLUE << "ANALYZING STRUCTURE...\n" << RESET;
    
    // Create PDA with the user's patterns
    PDA pda = BracketPDA::create_toxic_detection_pda(toxic_patterns, max_edits);
    ApproximateMatcher matcher;
    
    // Analyze the input
    bool valid_structure = pda.simulate(user_input);
    
    // Extract all bracket pairs and their content
    struct BracketContent {
        char open_bracket;
        char close_bracket;
        string content;
        size_t start_pos;
        size_t end_pos;
        bool is_toxic;
        string matched_pattern;
        int edit_distance;
    };
    
    vector<BracketContent> bracket_contents;
    vector<pair<size_t, size_t>> bracket_positions;
    
    // Find all matching bracket pairs
    stack<pair<size_t, char>> bracket_stack;
    
    for (size_t i = 0; i < user_input.length(); i++) {
        char c = user_input[i];
        
        if (c == '(' || c == '[' || c == '{' || c == '<') {
            bracket_stack.push({i, c});
        } 
        else if (c == ')' || c == ']' || c == '}' || c == '>') {
            if (!bracket_stack.empty()) {
                auto [start_idx, open_type] = bracket_stack.top();
                bracket_stack.pop();
                
                // Check if brackets match
                bool matches = false;
                if (open_type == '(' && c == ')') matches = true;
                else if (open_type == '[' && c == ']') matches = true;
                else if (open_type == '{' && c == '}') matches = true;
                else if (open_type == '<' && c == '>') matches = true;
                
                if (matches) {
                    string content = user_input.substr(start_idx + 1, i - start_idx - 1);
                    
                    // Check if content contains toxic patterns
                    bool is_toxic = false;
                    string matched_pattern;
                    int min_distance = INT_MAX;
                    
                    for (const auto& pattern : toxic_patterns) {
                        // First check exact match
                        if (content.find(pattern) != string::npos) {
                            is_toxic = true;
                            matched_pattern = pattern;
                            min_distance = 0;
                            break;
                        }
                        
                        // Check approximate matches
                        auto matches = matcher.find_matches(content, pattern, max_edits);
                        if (!matches.empty()) {
                            is_toxic = true;
                            matched_pattern = pattern;
                            min_distance = min(min_distance, matches[0].distance);
                            break;
                        }
                    }
                    
                    bracket_contents.push_back({
                        open_type, c, content, start_idx, i,
                        is_toxic, matched_pattern, min_distance
                    });
                    
                    bracket_positions.push_back({start_idx, i});
                }
            }
        }
    }
    
    // Check for unmatched brackets
    bool has_unmatched = !bracket_stack.empty() || 
                        (user_input.find_first_of(")]}>") != string::npos && 
                         user_input.find_first_of("([{<") != string::npos);
    
    // DISPLAY RESULTS
    cout << "\n" << CYAN << "=== STRUCTURE VALIDATION RESULTS ===\n" << RESET;
    cout << "Input: \"" << user_input << "\"\n";
    cout << "Patterns: ";
    for (size_t i = 0; i < toxic_patterns.size(); i++) {
        cout << "\"" << toxic_patterns[i] << "\"";
        if (i < toxic_patterns.size() - 1) cout << ", ";
    }
    cout << "\n";
    
    // Overall structure validity
    cout << "\nOverall Structure: ";
    if (valid_structure && !has_unmatched) {
        cout << GREEN << "VALID \n" << RESET;
    } else {
        cout << RED << "INVALID \n" << RESET;
        if (has_unmatched) {
            cout << YELLOW << "  Reason: Unmatched brackets detected\n" << RESET;
        }
        if (!valid_structure) {
            cout << YELLOW << "  Reason: PDA rejected the input\n" << RESET;
        }
    }
    
    // Display bracket analysis
    if (!bracket_contents.empty()) {
        cout << "\n" << MAGENTA << "BRACKET ANALYSIS:\n" << RESET;
        cout << "Found " << bracket_contents.size() << " bracket pair(s)\n\n";
        
        for (size_t i = 0; i < bracket_contents.size(); i++) {
            const auto& bc = bracket_contents[i];
            
            cout << "Pair " << i+1 << ": " << bc.open_bracket << "..." << bc.close_bracket << "\n";
            cout << "  Position: " << bc.start_pos << "-" << bc.end_pos << "\n";
            cout << "  Content: \"" << bc.content << "\"\n";
            
            if (bc.is_toxic) {
                cout << RED << "  Toxicity: DETECTED ✗\n" << RESET;
                cout << "  Matched pattern: \"" << bc.matched_pattern << "\"\n";
                if (bc.edit_distance == 0) {
                    cout << "  Type: Exact match\n";
                } else {
                    cout << "  Type: Approximate match (" << bc.edit_distance << " edit";
                    if (bc.edit_distance != 1) cout << "s";
                    cout << ")\n";
                }
            } else {
                cout << GREEN << "  Toxicity: CLEAN \n" << RESET;
            }
            cout << "\n";
        }
    } else {
        cout << YELLOW << "\nNo complete bracket pairs found.\n" << RESET;
    }
    
    // Highlight the input with colors
    cout << CYAN << "VISUAL HIGHLIGHT:\n" << RESET;
    string highlighted = user_input;
    
    // Apply highlighting in reverse order to maintain positions
    sort(bracket_positions.rbegin(), bracket_positions.rend());
    
    for (const auto& [start, end] : bracket_positions) {
        // Find if this bracket contains toxic content
        bool is_toxic_bracket = false;
        for (const auto& bc : bracket_contents) {
            if (bc.start_pos == start && bc.end_pos == end && bc.is_toxic) {
                is_toxic_bracket = true;
                break;
            }
        }
        
        string color = is_toxic_bracket ? RED : GREEN;
        
        highlighted.insert(end + 1, RESET);
        highlighted.insert(start, color);
    }
    
    cout << "Highlighted: " << highlighted << "\n";
    cout << GREEN << "  Clean brackets\n" << RESET;
    cout << RED << "  Toxic brackets\n" << RESET;
    
    // STATISTICS
    int total_brackets = bracket_contents.size();
    int toxic_brackets = 0;
    int clean_brackets = 0;
    
    for (const auto& bc : bracket_contents) {
        if (bc.is_toxic) toxic_brackets++;
        else clean_brackets++;
    }
    
    cout << "\n" << BLUE << "STATISTICS:\n" << RESET;
    cout << "Total bracket pairs: " << total_brackets << "\n";
    cout << "Clean brackets: " << GREEN << clean_brackets << RESET << "\n";
    cout << "Toxic brackets: " << RED << toxic_brackets << RESET << "\n";
    if (total_brackets > 0) {
        double toxic_percent = (toxic_brackets * 100.0) / total_brackets;
        cout << "Toxicity rate: " << (toxic_percent > 50 ? RED : YELLOW) 
             << fixed << setprecision(1) << toxic_percent << "%\n" << RESET;
    }
    
    // VISUALIZATION OPTIONS
    cout << "\n" << CYAN << "=== VISUALIZATION OPTIONS ===\n" << RESET;
    cout << "Generate PDA state diagram? (y/n): ";
    string viz_choice;
    getline(cin, viz_choice);
    
    if (!viz_choice.empty() && tolower(viz_choice[0]) == 'y') {
        // Generate multiple diagram types
        cout << "\nSelect diagram type:\n";
        cout << "1. Basic PDA State Diagram\n";
        cout << "2. PDA with Input Processing Path\n";
        cout << "3. Bracket Matching Visualization\n";
        cout << "4. All diagrams\n";
        cout << YELLOW << "Choice (1-4): " << RESET;
        
        string choice_str;
        getline(cin, choice_str);
        int choice = choice_str.empty() ? 2 : stoi(choice_str); // Default to option 2
        
        try {
            vector<string> generated_files;
            
            if (choice == 1 || choice == 4) {
                // Basic PDA diagram
                string dot_file = "pda_basic.dot";
                string png_file = "pda_basic.png";
                
                ofstream fout(dot_file);
                if (fout.is_open()) {
                    fout << pda.toDot();
                    fout.close();
                    
                    string command = "dot -Tpng \"" + dot_file + "\" -o \"" + png_file + "\"";
                    if (system(command.c_str()) == 0) {
                        cout << GREEN << "Basic PDA diagram: " << png_file << "\n" << RESET;
                        generated_files.push_back(png_file);
                    }
                }
            }
            
            if (choice == 2 || choice == 4) {
    // PDA with input processing (OLD VERSION STYLE)
    // Create diagrams for EACH unique bracket pattern found
    
    int diagram_count = 0;
    
    if (!bracket_contents.empty()) {
        // Group brackets by their pattern (open_bracket + content)
        unordered_map<string, vector<BracketContent>> grouped_brackets;
        
        for (const auto& bc : bracket_contents) {
            string key = string(1, bc.open_bracket) + bc.content + string(1, bc.close_bracket);
            grouped_brackets[key].push_back(bc);
        }
        
        // Create a diagram for each unique bracket pattern
        for (const auto& [pattern, brackets] : grouped_brackets) {
            if (diagram_count >= 5) { // Limit to 5 diagrams max
                cout << YELLOW << "Note: Showing first 5 diagrams only\n" << RESET;
                break;
            }
            
            const auto& bc = brackets[0]; // Use first one as template
            
            string diagram_name = "pda_input_" + to_string(diagram_count + 1);
            string dot_file = diagram_name + ".dot";
            string png_file = diagram_name + ".png";
            
            ofstream fout(dot_file);
            if (fout.is_open()) {
                fout << "digraph PDA {\n";
                fout << "    rankdir=LR;\n";
                fout << "    node [shape=circle, style=filled, color=lightblue];\n";
                fout << "    labelloc=\"t\";\n";
                fout << "    label=\"Pattern: " << bc.open_bracket << bc.content << bc.close_bracket;
                
                if (bc.is_toxic) {
                    fout << "\\n(Toxic: " << bc.matched_pattern;
                    if (bc.edit_distance > 0) {
                        fout << ", " << bc.edit_distance << " edit";
                        if (bc.edit_distance != 1) fout << "s";
                    }
                    fout << ")";
                }
                fout << "\";\n\n";
                
                // Calculate number of states needed (1 for start, 1 for each char, 1 for accept)
                int num_chars = bc.content.length();
                int total_states = num_chars + 2; // q0 + chars + accept
                
                // States
                fout << "    q0";
                for (int i = 1; i <= num_chars; i++) {
                    fout << "; q" << i;
                }
                fout << ";\n";
                fout << "    qAccept [shape=doublecircle, color=";
                fout << (bc.is_toxic ? "pink" : "lightgreen");
                fout << "];\n\n";
                
                // Start arrow
                fout << "    start [shape=point];\n";
                fout << "    start -> q0;\n\n";
                
                // Read opening bracket (push onto stack)
                fout << "    // Read \"" << bc.open_bracket << "\" push \"" << bc.open_bracket << "\" onto stack\n";
                fout << "    q0 -> q1 [label=\"" << bc.open_bracket << " , Z / " << bc.open_bracket << "Z\"];\n";
                fout << "    q0 -> q1 [label=\"" << bc.open_bracket << " , " << bc.open_bracket << " / " << bc.open_bracket << bc.open_bracket << "\"];\n\n";
                
                // Add transitions for each character in the word
                if (!bc.content.empty()) {
                    for (size_t i = 0; i < bc.content.size(); i++) {
                        char current_char = bc.content[i];
                        int from_state = i + 1;
                        int to_state = i + 2;
                        
                        fout << "    // Match '" << current_char << "'\n";
                        fout << "    q" << from_state << " -> q" << to_state 
                             << " [label=\"" << current_char << " , " << bc.open_bracket << " / " << bc.open_bracket << "\"];\n";
                    }
                    
                    // Accept only the matching closing bracket (pop from stack)
                    fout << "\n    // Accept only the closing bracket \"" << bc.close_bracket << "\"\n";
                    fout << "    // Pop \"" << bc.open_bracket << "\" from stack\n";
                    int last_state = bc.content.size() + 1;
                    fout << "    q" << last_state << " -> qAccept [label=\"" << bc.close_bracket << " , " << bc.open_bracket << " / ε\"];\n";
                } else {
                    // If no word inside, go directly to accept
                    fout << "\n    // Empty brackets\n";
                    fout << "    q1 -> qAccept [label=\"" << bc.close_bracket << " , " << bc.open_bracket << " / ε\"];\n";
                }
                
                fout << "}\n";
                fout.close();
                
                string command = "dot -Tpng \"" + dot_file + "\" -o \"" + png_file + "\"";
                if (system(command.c_str()) == 0) {
                    cout << GREEN << "PDA Diagram " << (diagram_count + 1) << ": " << png_file << RESET << "\n";
                    cout << "  Pattern: " << bc.open_bracket << bc.content << bc.close_bracket;
                    cout << " (" << (bc.is_toxic ? RED "TOXIC" : GREEN "CLEAN") << RESET << ")\n";
                    generated_files.push_back(png_file);
                    diagram_count++;
                }
            }
        }
        
        // Also create a SUMMARY diagram showing all patterns
        if (diagram_count > 1) {
            string summary_dot = "pda_summary.dot";
            string summary_png = "pda_summary.png";
            
            ofstream fout_summary(summary_dot);
            if (fout_summary.is_open()) {
                fout_summary << "digraph PDASummary {\n";
                fout_summary << "    rankdir=TB;\n";
                fout_summary << "    node [shape=box, style=rounded];\n";
                fout_summary << "    labelloc=\"t\";\n";
                fout_summary << "    label=\"PDA Patterns Summary\\nFound " << bracket_contents.size() << " bracket pairs\\n";
                fout_summary << toxic_brackets << " toxic, " << clean_brackets << " clean\";\n\n";
                
                fout_summary << "    start [shape=point];\n";
                fout_summary << "    patterns [label=\"Detected Patterns\", shape=oval, fillcolor=lightblue, style=filled];\n";
                fout_summary << "    start -> patterns;\n\n";
                
                int pattern_num = 1;
                for (const auto& [pattern, brackets] : grouped_brackets) {
                    if (pattern_num > 8) break; // Limit display
                    
                    const auto& bc = brackets[0];
                    string pattern_label = string(1, bc.open_bracket) + bc.content + string(1, bc.close_bracket);
                    string color = bc.is_toxic ? "pink" : "lightgreen";
                    string toxic_label = bc.is_toxic ? 
                        "\\nToxic: " + bc.matched_pattern + 
                        (bc.edit_distance > 0 ? 
                         " (" + to_string(bc.edit_distance) + " edit" + 
                         (bc.edit_distance != 1 ? "s)" : ")") : "") 
                        : "\\nClean";
                    
                    fout_summary << "    pattern" << pattern_num << " [label=\"" << pattern_label << toxic_label 
                                 << "\", fillcolor=" << color << ", style=filled];\n";
                    fout_summary << "    patterns -> pattern" << pattern_num << ";\n";
                    
                    // Show count of this pattern
                    if (brackets.size() > 1) {
                        fout_summary << "    count" << pattern_num << " [label=\"x" << brackets.size() 
                                     << "\", shape=circle, width=0.5, fillcolor=yellow, style=filled];\n";
                        fout_summary << "    pattern" << pattern_num << " -> count" << pattern_num << " [style=dashed];\n";
                    }
                    
                    pattern_num++;
                }
                
                fout_summary << "}\n";
                fout_summary.close();
                
                string command = "dot -Tpng \"" + summary_dot + "\" -o \"" + summary_png + "\"";
                if (system(command.c_str()) == 0) {
                    cout << GREEN << "\n PDA Summary Diagram: " << summary_png << RESET << "\n";
                    generated_files.push_back(summary_png);
                }
            }
        }
        
    } else {
        // If no brackets found, create a simple diagram
        string dot_file = "pda_input_simple.dot";
        string png_file = "pda_input_simple.png";
        
        ofstream fout(dot_file);
        if (fout.is_open()) {
            // Create a generic PDA diagram
            fout << "digraph PDA {\n";
            fout << "    rankdir=LR;\n";
            fout << "    node [shape=circle, style=filled, color=lightblue];\n";
            fout << "    labelloc=\"t\";\n";
            fout << "    label=\"No brackets found in input\";\n\n";
            
            fout << "    q0; q1; qAccept [shape=doublecircle, color=lightgreen];\n\n";
            fout << "    start [shape=point];\n";
            fout << "    start -> q0;\n\n";
            
            // Generic transitions for any bracket
            fout << "    // Read opening bracket\n";
            fout << "    q0 -> q1 [label=\"(,[,{,< , Z / bracket Z\"];\n\n";
            fout << "    // Read closing bracket\n";
            fout << "    q1 -> qAccept [label=\"),],},> , bracket / ε\"];\n";
            
            fout << "}\n";
            fout.close();
            
            string command = "dot -Tpng \"" + dot_file + "\" -o \"" + png_file + "\"";
            if (system(command.c_str()) == 0) {
                cout << GREEN << "PDA Diagram: " << png_file << RESET << "\n";
                cout << "  Generic bracket matching PDA\n";
                generated_files.push_back(png_file);
            }
        }
    }
}
            
            if (choice == 3 || choice == 4) {
    // SIMPLER Bracket matching visualization (no ports)
    string dot_file = "bracket_matching.dot";
    string png_file = "bracket_matching.png";
    
    ofstream fout(dot_file);
    if (fout.is_open()) {
        fout << "digraph BracketMatching {\n";
        fout << "    rankdir=TB;\n";
        fout << "    node [shape=none];\n";
        fout << "    edge [arrowhead=none];\n\n";
        
        // Create nodes for each character
        fout << "    // Input string characters\n";
        for (size_t i = 0; i < user_input.length(); i++) {
            char c = user_input[i];
            string char_str;
            
            // Handle special characters
            if (c == '"') char_str = "\\\"";
            else if (c == '\\') char_str = "\\\\";
            else char_str = string(1, c);
            
            fout << "    char" << i << " [label=\"" << char_str << "\"";
            
            // Color code brackets
            if (c == '(' || c == '[' || c == '{' || c == '<' || 
                c == ')' || c == ']' || c == '}' || c == '>') {
                fout << ", shape=box, style=filled, fillcolor=lightblue";
            }
            fout << "];\n";
        }
        
        // Arrange characters horizontally
        fout << "\n    // Horizontal arrangement\n";
        fout << "    { rank=same; ";
        for (size_t i = 0; i < user_input.length(); i++) {
            fout << "char" << i;
            if (i < user_input.length() - 1) fout << " -> ";
        }
        fout << " [style=invis]; }\n\n";
        
        // Bracket connections (arcs above the text)
        fout << "    // Bracket connections\n";
        for (const auto& bc : bracket_contents) {
            string color = bc.is_toxic ? "red" : "green";
            string label = bc.is_toxic ? "label=\"" + bc.matched_pattern + "\"" : "";
            
            fout << "    char" << bc.start_pos << " -> char" << bc.end_pos 
                 << " [color=" << color << ", penwidth=2, constraint=false, " 
                 << label << "];\n";
        }
        
        fout << "\n    // Legend\n";
        fout << "    subgraph cluster_legend {\n";
        fout << "        label=\"Legend\";\n";
        fout << "        style=filled;\n";
        fout << "        fillcolor=lightyellow;\n";
        fout << "        node [shape=plaintext];\n";
        fout << "        clean [label=\"Clean bracket pair\", color=green, fontcolor=green];\n";
        fout << "        toxic [label=\"Toxic bracket pair\", color=red, fontcolor=red];\n";
        fout << "        clean -> toxic [style=invis];\n";
        fout << "    }\n";
        
        fout << "}\n";
        fout.close();
        
        string command = "dot -Tpng \"" + dot_file + "\" -o \"" + png_file + "\"";
        if (system(command.c_str()) == 0) {
            cout << GREEN << " Bracket matching visualization: " << png_file << "\n" << RESET;
            generated_files.push_back(png_file);
        } else {
            cout << YELLOW << "Note: Graphviz had issues with the diagram\n" << RESET;
        }
    }
}
            
            // Ask if user wants to open generated files
            if (!generated_files.empty()) {
                cout << "\nOpen generated diagrams? (y/n): ";
                string open_choice;
                getline(cin, open_choice);
                
                if (!open_choice.empty() && tolower(open_choice[0]) == 'y') {
                    for (const auto& file : generated_files) {
#ifdef _WIN32
                        ShellExecuteA(NULL, "open", file.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
                        system(("xdg-open \"" + file + "\" &").c_str());
#endif
                    }
                }
            }
            
        } catch (const exception& e) {
            cout << RED << "Error generating diagrams: " << e.what() << "\n" << RESET;
        }
    }
    
    // REMOVED THE SAVE ANALYSIS REPORT SECTION
}

// Helper function to get current timestamp
string ChatModerationUI::get_current_timestamp() {
    time_t now = time(0);
    tm* localtm = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", localtm);
    return string(buffer);
}

// Helper function to extract patterns from regex
vector<string> ChatModerationUI::extract_patterns_from_regex(const string& regex) {
    vector<string> patterns;
    
    // Try to extract patterns between parentheses separated by |
    size_t start_paren = regex.find('(');
    size_t end_paren = regex.find(')');
    
    if (start_paren != string::npos && end_paren != string::npos && end_paren > start_paren) {
        string pattern_str = regex.substr(start_paren + 1, end_paren - start_paren - 1);
        
        // Split by | but handle escaped pipes
        size_t pos = 0;
        size_t last_pos = 0;
        bool in_escape = false;
        
        for (pos = 0; pos < pattern_str.length(); pos++) {
            if (pattern_str[pos] == '\\') {
                in_escape = !in_escape;
            } else if (pattern_str[pos] == '|' && !in_escape) {
                string pattern = pattern_str.substr(last_pos, pos - last_pos);
                if (!pattern.empty()) {
                    patterns.push_back(pattern);
                }
                last_pos = pos + 1;
                in_escape = false;
            } else {
                in_escape = false;
            }
        }
        
        // Add the last pattern
        string last_pattern = pattern_str.substr(last_pos);
        if (!last_pattern.empty()) {
            patterns.push_back(last_pattern);
        }
    }
    
    return patterns;
}


void ChatModerationUI::analyze_chat_log() {
    cout << "\n" << CYAN << "=== XML DOCUMENT ANALYSIS ===\n" << RESET;
    cout << "This option analyzes XML chat logs using NFA, PDA, and approximate matching\n";
    cout << "XML format should contain <message> elements with <text> content\n";
    cout << "Example:\n";
    cout << "  <message>\n";
    cout << "    <text>fuck you</text>\n";
    cout << "  </message>\n\n";
    
    // Get XML filename
    cout << "Enter XML document filename: ";
    string filename; 
    getline(cin, filename);
    
    if (filename.empty()) {
        cout << RED << "Error: Filename cannot be empty!\n" << RESET;
        return;
    }
    
    // Check if file exists
    ifstream file_check(filename);
    if (!file_check.is_open()) {
        cout << RED << "Error: File '" << filename << "' not found!\n" << RESET;
        cout << "Please provide a valid XML file.\n";
        return;
    }
    file_check.close();
    
    // Get toxic patterns to search for
    cout << "\n" << CYAN << "TOXIC PATTERN CONFIGURATION:\n" << RESET;
    cout << "Enter regex pattern for toxic content to search for:\n";
    cout << "Examples:\n";
    cout << "  Simple: (fuck|shit|ass|bitch)\n";
    cout << "  Case variations: (f|F)(u|U)(c|C)(k|K)\n";
    cout << "  Approx patterns: (idiot|stupid|hate|moron)\n";
    cout << YELLOW << ">> " << RESET;
    string regex_pattern;
    getline(cin, regex_pattern);
    
    if (regex_pattern.empty()) {
        cout << RED << "Error: Toxic pattern cannot be empty!\n" << RESET;
        return;
    }
    
    vector<string> toxic_patterns;
    toxic_patterns = extract_patterns_from_regex(regex_pattern);
    if (toxic_patterns.empty()) {
        // If can't extract patterns, use the regex as a single pattern
        toxic_patterns.push_back(regex_pattern);
    }
    
    // Display extracted patterns
    cout << GREEN << "\nSearching for " << toxic_patterns.size() << " pattern(s):\n" << RESET;
    for (size_t i = 0; i < toxic_patterns.size(); i++) {
        cout << "  " << i+1 << ". \"" << toxic_patterns[i] << "\"\n";
    }
    
    // Get edit distance
    cout << "\nMax edit distance for approximate matching (0-3): ";
    string edit_dist_str;
    getline(cin, edit_dist_str);
    int max_edits = 1;
    try {
        max_edits = stoi(edit_dist_str);
        if (max_edits < 0 || max_edits > 3) {
            max_edits = 1;
            cout << YELLOW << "Using edit distance: 1\n" << RESET;
        }
    } catch (...) {
        max_edits = 1;
        cout << YELLOW << "Using edit distance: 1\n" << RESET;
    }
    
    cout << "\n" << BLUE << "ANALYZING XML DOCUMENT...\n" << RESET;
    
    // Parse XML and analyze
    vector<XMLMessageResult> results = parse_and_analyze_xml(filename, toxic_patterns, max_edits);
    
    if (results.empty()) {
        cout << YELLOW << "No messages found in XML file or file format is invalid.\n" << RESET;
        return;
    }
    
    // Display comprehensive results
    display_xml_analysis(results, toxic_patterns, max_edits);
}

// Function to parse and analyze XML
vector<XMLMessageResult> ChatModerationUI::parse_and_analyze_xml(
    const string& filename, 
    const vector<string>& toxic_patterns, 
    int max_edits) {
    
    vector<XMLMessageResult> results;
    ifstream file(filename);
    
    if (!file.is_open()) {
        cout << RED << "Failed to open XML file\n" << RESET;
        return results;
    }
    
    string line;
    string current_text;
    bool in_text_tag = false;
    int message_count = 0;
    
    // Simple XML parser for <text> content
    while (getline(file, line)) {
        // Find <text> tags
        size_t text_start = line.find("<text>");
        if (text_start != string::npos) {
            size_t text_end = line.find("</text>", text_start);
            if (text_end != string::npos) {
                // Extract text content
                string text_content = line.substr(text_start + 6, text_end - text_start - 6);
                
                if (!text_content.empty()) {
                    XMLMessageResult result;
                    result.text = text_content;
                    
                    // Perform comprehensive analysis (like Option 3)
                    analyze_message_content(result, text_content, toxic_patterns, max_edits);
                    
                    results.push_back(result);
                    message_count++;
                    
                    // Show progress for large files
                    if (message_count % 10 == 0) {
                        cout << GREEN << "Processed " << message_count << " messages...\n" << RESET;
                    }
                }
            }
        }
    }
    
    file.close();
    cout << GREEN << " Parsed " << message_count << " messages from XML\n" << RESET;
    return results;
}

// Function to analyze message content (like Option 3)
void ChatModerationUI::analyze_message_content(
    XMLMessageResult& result,
    const string& text,
    const vector<string>& toxic_patterns,
    int max_edits) {
    
    ApproximateMatcher matcher(false);
    result.toxicity_score = 0;
    result.has_toxic_content = false;
    
    // 1. EXACT MATCHES (NFA/DFA style)
    string lower_text = text;
    transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    for (const auto& pattern : toxic_patterns) {
        string lower_pattern = pattern;
        transform(lower_pattern.begin(), lower_pattern.end(), lower_pattern.begin(), ::tolower);
        
        // Simple exact match search
        if (lower_text.find(lower_pattern) != string::npos) {
            result.exact_matches.push_back(pattern);
            result.has_toxic_content = true;
            result.toxicity_score += 20;
        }
    }
    
    // 2. APPROXIMATE MATCHES
    for (const auto& pattern : toxic_patterns) {
        auto matches = matcher.find_matches(text, pattern, max_edits);
        for (const auto& match : matches) {
            // Avoid duplicates with exact matches
            bool already_found = false;
            for (const auto& exact : result.exact_matches) {
                if (exact == match.matched_pattern) {
                    already_found = true;
                    break;
                }
            }
            if (!already_found) {
                result.approx_matches.push_back({match.original, match.matched_pattern});
                result.has_toxic_content = true;
                result.toxicity_score += 10;
            }
        }
    }
    
    // 3. BRACKET STRUCTURE ANALYSIS (PDA style)
    stack<pair<size_t, char>> bracket_stack;
    
    for (size_t i = 0; i < text.length(); i++) {
        char c = text[i];
        
        if (c == '(' || c == '[' || c == '{' || c == '<') {
            bracket_stack.push({i, c});
        } 
        else if (c == ')' || c == ']' || c == '}' || c == '>') {
            if (!bracket_stack.empty()) {
                auto [start_idx, open_type] = bracket_stack.top();
                bracket_stack.pop();
                
                // Check if brackets match
                bool matches = false;
                char close_bracket;
                if (open_type == '(' && c == ')') { matches = true; close_bracket = ')'; }
                else if (open_type == '[' && c == ']') { matches = true; close_bracket = ']'; }
                else if (open_type == '{' && c == '}') { matches = true; close_bracket = '}'; }
                else if (open_type == '<' && c == '>') { matches = true; close_bracket = '>'; }
                
                if (matches) {
                    string content = text.substr(start_idx + 1, i - start_idx - 1);
                    
                    // Check if content contains toxic patterns
                    bool is_toxic = false;
                    string matched_pattern;
                    int edit_distance = INT_MAX;
                    
                    for (const auto& pattern : toxic_patterns) {
                        // Exact match
                        if (content.find(pattern) != string::npos) {
                            is_toxic = true;
                            matched_pattern = pattern;
                            edit_distance = 0;
                            break;
                        }
                        
                        // Approximate match
                        auto matches = matcher.find_matches(content, pattern, max_edits);
                        if (!matches.empty()) {
                            is_toxic = true;
                            matched_pattern = pattern;
                            edit_distance = min(edit_distance, matches[0].distance);
                            break;
                        }
                    }
                    
                    BracketContent bc;
                    bc.open_bracket = open_type;
                    bc.close_bracket = close_bracket;
                    bc.content = content;
                    bc.is_toxic = is_toxic;
                    bc.matched_pattern = matched_pattern;
                    bc.edit_distance = (edit_distance == INT_MAX) ? -1 : edit_distance;
                    
                    result.bracket_contents.push_back(bc);
                    
                    if (is_toxic) {
                        result.has_toxic_content = true;
                        result.toxicity_score += 30;
                    }
                }
            }
        }
    }
    
    // Cap toxicity score
    if (result.toxicity_score > 100) result.toxicity_score = 100;
}

// Function to display XML analysis
void ChatModerationUI::display_xml_analysis(
    const vector<XMLMessageResult>& results,
    const vector<string>& toxic_patterns,
    int max_edits) {
    
    cout << "\n" << CYAN << "=== XML DOCUMENT ANALYSIS RESULTS ===\n" << RESET;
    cout << "File analyzed with " << toxic_patterns.size() << " toxic pattern(s)\n";
    cout << "Max edit distance: " << max_edits << "\n";
    cout << "Total messages analyzed: " << results.size() << "\n\n";
    
    int toxic_messages = 0;
    int clean_messages = 0;
    int total_exact = 0;
    int total_approx = 0;
    int total_toxic_brackets = 0;
    int total_brackets = 0;
    
    // Display each toxic message
    for (size_t i = 0; i < results.size(); i++) {
        const auto& msg = results[i];
        
        if (msg.has_toxic_content) {
            toxic_messages++;
            
            cout << MAGENTA << "\n--- TOXIC MESSAGE #" << (i+1) << " ---\n" << RESET;
            cout << "Text: \"" << msg.text << "\"\n";
            cout << "Toxicity Score: ";
            
            if (msg.toxicity_score >= 70) {
                cout << RED << msg.toxicity_score << "/100 (HIGH)\n" << RESET;
            } else if (msg.toxicity_score >= 30) {
                cout << YELLOW << msg.toxicity_score << "/100 (MODERATE)\n" << RESET;
            } else {
                cout << YELLOW << msg.toxicity_score << "/100 (LOW)\n" << RESET;
            }
            
            // Exact matches
            if (!msg.exact_matches.empty()) {
                cout << RED << "Exact matches: ";
                for (size_t j = 0; j < msg.exact_matches.size(); j++) {
                    cout << msg.exact_matches[j];
                    if (j < msg.exact_matches.size() - 1) cout << ", ";
                }
                cout << RESET << "\n";
                total_exact += msg.exact_matches.size();
            }
            
            // Approximate matches
            if (!msg.approx_matches.empty()) {
                cout << YELLOW << "Approximate matches: ";
                for (size_t j = 0; j < msg.approx_matches.size(); j++) {
                    cout << msg.approx_matches[j].first << "->" << msg.approx_matches[j].second;
                    if (j < msg.approx_matches.size() - 1) cout << ", ";
                }
                cout << RESET << "\n";
                total_approx += msg.approx_matches.size();
            }
            
            // Bracket analysis
            if (!msg.bracket_contents.empty()) {
                cout << CYAN << "Bracket structures: " << msg.bracket_contents.size() << "\n" << RESET;
                total_brackets += msg.bracket_contents.size();
                
                for (const auto& bc : msg.bracket_contents) {
                    if (bc.is_toxic) {
                        cout << RED << "  TOXIC: " << bc.open_bracket << bc.content << bc.close_bracket;
                        cout << " (matches: " << bc.matched_pattern;
                        if (bc.edit_distance > 0) cout << ", " << bc.edit_distance << " edit";
                        if (bc.edit_distance > 1) cout << "s";
                        cout << ")\n" << RESET;
                        total_toxic_brackets++;
                    } else {
                        cout << GREEN << "  CLEAN: " << bc.open_bracket << bc.content << bc.close_bracket << "\n" << RESET;
                    }
                }
            }
            
            cout << string(50, '-') << "\n";
        } else {
            clean_messages++;
        }
    }
    
    // SUMMARY
    cout << "\n" << BLUE << "=== ANALYSIS SUMMARY ===\n" << RESET;
    cout << "Total messages: " << results.size() << "\n";
    cout << "Toxic messages: " << RED << toxic_messages << RESET << "\n";
    cout << "Clean messages: " << GREEN << clean_messages << RESET << "\n";
    
    if (results.size() > 0) {
        double toxic_percent = (toxic_messages * 100.0) / results.size();
        cout << "Toxicity rate: " 
             << (toxic_percent > 50 ? RED : toxic_percent > 20 ? YELLOW : GREEN)
             << fixed << setprecision(1) << toxic_percent << "%\n" << RESET;
    }
    
    cout << "Total exact matches: " << RED << total_exact << RESET << "\n";
    cout << "Total approximate matches: " << YELLOW << total_approx << RESET << "\n";
    cout << "Total bracket structures: " << total_brackets << "\n";
    cout << "Toxic brackets: " << RED << total_toxic_brackets << RESET << "\n";
    
    // RECOMMENDATIONS
    cout << "\n" << CYAN << "=== MODERATION RECOMMENDATIONS ===\n" << RESET;
    if (toxic_messages > results.size() / 2) {
        cout << RED << "CRITICAL: More than 50% of messages are toxic!\n" << RESET;
        cout << "  Consider: Banning users, enabling strict filtering\n";
    } else if (toxic_messages > results.size() / 4) {
        cout << YELLOW << "WARNING: Significant toxicity detected (25-50%)\n" << RESET;
        cout << "  Consider: Warnings, temporary mutes, content review\n";
    } else if (toxic_messages > 0) {
        cout << YELLOW << "MODERATE: Some toxic content found\n" << RESET;
        cout << "  Consider: Flagging specific messages for review\n";
    } else {
        cout << GREEN << "CLEAN: No toxic content detected\n" << RESET;
    }
    
    // ENHANCED VISUALIZATION OPTION
cout << "\n" << CYAN << "=== VISUALIZATION OPTIONS ===\n" << RESET;
cout << "Generate automata diagrams for analysis? (y/n): ";
string viz_choice;
getline(cin, viz_choice);

if (!viz_choice.empty() && tolower(viz_choice[0]) == 'y') {
    // Let user choose which diagrams to generate
    cout << "\n" << CYAN << "Select diagrams to generate:\n" << RESET;
    cout << "1. NFA Diagram (Regex to NFA)\n";
    cout << "2. DFA Diagram (Optimized NFA)\n";
    cout << "3. PDA Diagram (Bracket Analysis)\n";
    cout << "4. All diagrams (NFA, DFA, PDA)\n";
    cout << YELLOW << "Enter your choice (1-4): " << RESET;
    
    string diagram_choice_str;
    getline(cin, diagram_choice_str);
    
    int diagram_choice = 4; // Default to all
    try {
        if (!diagram_choice_str.empty()) {
            diagram_choice = stoi(diagram_choice_str);
            if (diagram_choice < 1 || diagram_choice > 4) {
                diagram_choice = 4;
            }
        }
    } catch (...) {
        diagram_choice = 4;
    }
    
    // Map 4 to 5 since we removed statistics
    if (diagram_choice == 4) diagram_choice = 5;
    
    generate_xml_analysis_diagrams(results, toxic_patterns, diagram_choice);
}
}


// Function to generate diagrams for XML analysis
void ChatModerationUI::generate_xml_analysis_diagrams(
    const vector<XMLMessageResult>& results,
    const vector<string>& toxic_patterns,
    int diagram_type) {
    
    cout << "\nGenerating analysis diagrams...\n";
    
    vector<string> generated_files;  // Track generated files to open
    
    try {
        // Create regex pattern from toxic patterns
        string regex_pattern = "(";
        for (size_t i = 0; i < toxic_patterns.size(); i++) {
            regex_pattern += toxic_patterns[i];
            if (i < toxic_patterns.size() - 1) regex_pattern += "|";
        }
        regex_pattern += ")";
        
        // Generate NFA and DFA for the patterns
        NFA toxic_nfa = RegexToNFA::from_regex(regex_pattern);
        DFA toxic_dfa = convert_nfa_to_dfa(toxic_nfa);
        
        // Create PDA for bracket analysis
        PDA pda = BracketPDA::create_toxic_detection_pda(toxic_patterns, 1);
        
        // Generate selected diagrams based on diagram_type
        switch (diagram_type) {
            case 1: // NFA only
                generate_diagram<NFA>(toxic_nfa, "xml_nfa_analysis", "NFA");
                generated_files.push_back("xml_nfa_analysis.png");
                break;
                
            case 2: // DFA only
                generate_diagram<DFA>(toxic_dfa, "xml_dfa_analysis", "DFA");
                generated_files.push_back("xml_dfa_analysis.png");
                break;
                
            case 3: // PDA only
                generate_diagram<PDA>(pda, "xml_pda_analysis", "PDA");
                generated_files.push_back("xml_pda_analysis.png");
                break;
                
            case 4: // All diagrams (default)
            default:
                generate_diagram<NFA>(toxic_nfa, "xml_nfa_analysis", "NFA");
                generate_diagram<DFA>(toxic_dfa, "xml_dfa_analysis", "DFA");
                generate_diagram<PDA>(pda, "xml_pda_analysis", "PDA");
                generated_files.push_back("xml_nfa_analysis.png");
                generated_files.push_back("xml_dfa_analysis.png");
                generated_files.push_back("xml_pda_analysis.png");
                break;
        }
        
        cout << "\n" << GREEN << "Diagrams generated successfully!\n" << RESET;
        
        // AUTO-OPEN THE GENERATED FILES
        if (!generated_files.empty()) {
            cout << CYAN << "Opening generated diagrams...\n" << RESET;
            
            for (const auto& file : generated_files) {
                // Check if file exists before opening
                ifstream file_check(file);
                if (file_check.is_open()) {
                    file_check.close();
                    
#ifdef _WIN32
                    ShellExecuteA(NULL, "open", file.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    // Small delay to avoid overwhelming the system
                    Sleep(500);
#else
                    system(("xdg-open \"" + file + "\" &").c_str());
#endif
                    cout << GREEN << "  Opened: " << file << "\n" << RESET;
                } else {
                    cout << YELLOW << "   File not found: " << file << "\n" << RESET;
                }
            }
        }
        
    } catch (const exception& e) {
        cout << RED << "Error generating diagrams: " << e.what() << "\n" << RESET;
    }
}


