#include "approximate_matcher.hpp"
#include <regex>
#include <unordered_set>
#include <queue>
#include <iomanip>
#include <fstream>

// ========== PRIVATE METHODS ==========

std::string ApproximateMatcher::preprocess_message(const std::string& message) {
    std::string result = message;
    std::unordered_map<char, char> leet_map = {
        {'1', 'i'}, {'0', 'o'}, {'3', 'e'}, {'4', 'a'}, {'5', 's'},
        {'7', 't'}, {'@', 'a'}, {'$', 's'}, {'!', 'i'}
    };

    for (char& c : result) {
        auto it = leet_map.find(std::tolower(c));
        if (it != leet_map.end()) c = it->second;
    }

    result.erase(std::remove_if(result.begin(), result.end(),
        [](char c) { return !std::isalnum(c) && !std::isspace(c); }), result.end());

    return result;
}

int ApproximateMatcher::levenshtein_distance(const std::string& s1, const std::string& s2) {
    int m = s1.size(), n = s2.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

    for (int i = 0; i <= m; ++i) dp[i][0] = i;
    for (int j = 0; j <= n; ++j) dp[0][j] = j;

    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            int cost = (tolower(s1[i - 1]) == tolower(s2[j - 1])) ? 0 : 1;
            dp[i][j] = std::min({ dp[i - 1][j] + 1,
                                  dp[i][j - 1] + 1,
                                  dp[i - 1][j - 1] + cost });
        }
    }

    return dp[m][n];
}

std::string ApproximateMatcher::escape_dot_label(const std::string& s) {
    std::string result;
    for (char c : s) {
        if (c == '"' || c == '\\') result += '\\';
        result += c;
    }
    return result;
}

std::vector<ApproximateMatcher::MatchResult> ApproximateMatcher::find_word_matches(
    const std::string& word, const std::string& regex_pattern, int maxEdits) {
    
    std::vector<MatchResult> matches;

    // Simple regex match first
    try {
        std::regex pattern(regex_pattern, std::regex_constants::icase);
        if (std::regex_match(word, pattern)) {
            matches.emplace_back(word, regex_pattern, 0, 100.0);
        } else {
            // If not exact, compute Levenshtein distance
            int dist = levenshtein_distance(word, regex_pattern);
            if (dist <= maxEdits) {
                double sim = (1.0 - static_cast<double>(dist) /
                              std::max(word.length(), regex_pattern.length())) * 100;
                matches.emplace_back(word, regex_pattern, dist, sim);
            }
        }
    } catch (...) {
        // invalid regex
    }

    return matches;
}

// ========== PUBLIC METHODS ==========

std::vector<ApproximateMatcher::MatchResult> ApproximateMatcher::find_matches(
    const std::string& message, 
    const std::string& regex_pattern, 
    int maxEdits) {
    
    std::vector<MatchResult> all_matches;
    
    // Preprocess the message
    std::string processed_text = preprocess_message(message);
    
    // ADD VERBOSE CHECK HERE:
    if (verbose_mode) {
        std::cout << "\nPattern: \"" << regex_pattern << "\"" << std::endl;
        std::cout << "Max edits: " << maxEdits << std::endl;
        std::cout << "Preprocessed: \"" << processed_text << "\"" << std::endl << std::endl;
    }
    
    // Tokenize the message into words
    std::istringstream iss(processed_text);
    std::string word;
    int word_count = 0;
    
    while (iss >> word) {
        word_count++;
        
        // ADD VERBOSE CHECK HERE:
        if (verbose_mode) {
            std::cout << "Word " << word_count << ": \"" << word << "\" -> ";
        }
        
        // Find matches for this word
        auto word_matches = find_word_matches(word, regex_pattern, maxEdits);
        
        if (!word_matches.empty()) {
            // ADD VERBOSE CHECK HERE:
            if (verbose_mode) {
                for (const auto& match : word_matches) {
                    std::cout << "MATCH: \"" << match.original << "\" -> \"" 
                             << match.matched_pattern << "\" (distance: " 
                             << match.distance << ")" << std::endl;
                }
            }
            all_matches.insert(all_matches.end(), word_matches.begin(), word_matches.end());
        } else {
            // ADD VERBOSE CHECK HERE:
            if (verbose_mode) {
                std::cout << "NO MATCH" << std::endl;
            }
        }
    }
    
    return all_matches;
}



std::string ApproximateMatcher::toDotRegexFSM(const std::string& regex_pattern, int maxEdits) {
    // First, convert regex to NFA (simplified for visualization)
    std::string dot = "digraph ApproxFSM {\n";
    dot += "  rankdir=LR;\n";
    dot += "  node [shape=circle];\n";
    dot += "  start [shape=point];\n";
    
    // ADD THIS FOR BETTER CONSOLE COMPATIBILITY
    dot += "  labelloc=\"t\";\n";
    dot += "  label=\"Finite State Machine\\nPattern: '";
    dot += regex_pattern + "' with max " + std::to_string(maxEdits) + " edits\";\n";
    
    // Simple regex to NFA conversion (basic patterns only)
    std::vector<std::string> states;
    std::vector<std::tuple<int, int, std::string, std::string>> transitions;
    
    // Build basic states from regex pattern
    int len = regex_pattern.size();  // ADD THIS LINE
    for (int pos = 0; pos <= len; ++pos) {
        for (int edits = 0; edits <= maxEdits; ++edits) {
            std::string state_name = "q" + std::to_string(pos) + "_" + std::to_string(edits);
            states.push_back(state_name);
            
            // Accepting states (end of pattern)
            if (pos == len) {
                dot += "  " + state_name + " [shape=doublecircle];\n";
            }
        }
    }
    
    // Add start transition
    dot += "  start -> q0_0;\n";
    
    // Character classes for readability
    std::unordered_map<char, std::string> char_classes = {
        {'a', "alpha"}, {'e', "alpha"}, {'i', "alpha"}, {'o', "alpha"}, {'u', "alpha"},
        {'0', "digit"}, {'1', "digit"}, {'2', "digit"}, {'3', "digit"}, {'4', "digit"},
        {'5', "digit"}, {'6', "digit"}, {'7', "digit"}, {'8', "digit"}, {'9', "digit"},
        {'@', "special"}, {'$', "special"}, {'!', "special"}, {'*', "special"}, 
        {'#', "special"}, {'%', "special"}
    };
    
    // Leet map for approximate matching - ADD THIS
    std::unordered_map<char, std::vector<char>> leet_equiv = {
        {'a', {'4', '@'}},
        {'e', {'3'}},
        {'i', {'1', '!', '|'}},
        {'o', {'0'}},
        {'s', {'5', '$'}},
        {'t', {'7', '+'}},
        {'b', {'8'}},
        {'g', {'9'}},
        {'l', {'1', '|'}}
    };
    
    // Create transitions
    for (int pos = 0; pos < len; ++pos) {
        for (int edits = 0; edits <= maxEdits; ++edits) {
            std::string from_state = "q" + std::to_string(pos) + "_" + std::to_string(edits);
            char expected_char = regex_pattern[pos];
            
            // 1. EXACT MATCH TRANSITION (0 edits)
            if (edits <= maxEdits) {
                std::string to_state = "q" + std::to_string(pos + 1) + "_" + std::to_string(edits);
                std::string label = "match '" + std::string(1, expected_char) + "'";
                dot += "  " + from_state + " -> " + to_state + 
                       " [label=\"" + escape_dot_label(label) + "\", color=\"green\"];\n";
                
                // Use ASCII arrow for leet
                if (leet_equiv.find(std::tolower(expected_char)) != leet_equiv.end()) {
                    for (char leet_char : leet_equiv[std::tolower(expected_char)]) {
                        std::string leet_label = "leet " + std::string(1, leet_char) + "->" + 
                                                std::string(1, expected_char);
                        dot += "  " + from_state + " -> " + to_state + 
                               " [label=\"" + escape_dot_label(leet_label) + "\", color=\"blue\", style=\"dashed\"];\n";
                    }
                }
            }
            
            // 2. CASE INSENSITIVE (0 edits for case variation)
            if (edits <= maxEdits) {
                char upper_char = std::toupper(expected_char);
                char lower_char = std::tolower(expected_char);
                if (upper_char != lower_char) {
                    std::string to_state = "q" + std::to_string(pos + 1) + "_" + std::to_string(edits);
                    std::string case_label = "case: '" + std::string(1, upper_char) + "'/'" + 
                                           std::string(1, lower_char) + "'";
                    dot += "  " + from_state + " -> " + to_state + 
                           " [label=\"" + escape_dot_label(case_label) + "\", color=\"purple\"];\n";
                }
            }
            
            // 3. SUBSTITUTIONS (1 edit)
            if (edits < maxEdits) {
                std::string to_state = "q" + std::to_string(pos + 1) + "_" + std::to_string(edits + 1);
                
                // Group substitutions by character class for readability
                std::string char_class = "other";
                if (std::isalpha(expected_char)) char_class = "alpha";
                else if (std::isdigit(expected_char)) char_class = "digit";
                else char_class = "special";
                
                std::string sub_label = "sub[" + char_class + "]: 'X'->'" + 
                                       std::string(1, expected_char) + "'";
                dot += "  " + from_state + " -> " + to_state + 
                       " [label=\"" + escape_dot_label(sub_label) + "\", color=\"orange\"];\n";
            }
            
            // 4. INSERTIONS (1 edit) - stay at same pattern position
            if (edits < maxEdits) {
                std::string to_state = "q" + std::to_string(pos) + "_" + std::to_string(edits + 1);
                
                // Group insertions
                std::string ins_label;
                if (std::isalpha(expected_char)) {
                    ins_label = "ins[alpha]: '?'";
                } else if (std::isdigit(expected_char)) {
                    ins_label = "ins[digit]: '?'";
                } else {
                    ins_label = "ins[special]: '?'";
                }
                
                dot += "  " + from_state + " -> " + to_state + 
                       " [label=\"" + escape_dot_label(ins_label) + "\", color=\"red\", style=\"dotted\"];\n";
            }
            
            // 5. DELETIONS (1 edit) - skip expected character
            if (edits < maxEdits) {
                std::string to_state = "q" + std::to_string(pos + 1) + "_" + std::to_string(edits + 1);
                std::string del_label = "del: '" + std::string(1, expected_char) + "'";
                dot += "  " + from_state + " -> " + to_state + 
                       " [label=\"" + escape_dot_label(del_label) + "\", color=\"brown\"];\n";
            }
            
            // 6. WILDCARD/CATCH-ALL for any character (regex . operator)
            if (expected_char == '.' && edits <= maxEdits) {
                std::string to_state = "q" + std::to_string(pos + 1) + "_" + std::to_string(edits);
                dot += "  " + from_state + " -> " + to_state + 
                       " [label=\"wildcard: any char\", color=\"darkgreen\", penwidth=2];\n";
            }
            
            // 7. CHARACTER CLASSES (regex [abc] style)
            if (pos + 1 < len && regex_pattern[pos] == '[') {
                size_t end_pos = regex_pattern.find(']', pos);
                if (end_pos != std::string::npos) {
                    std::string char_class = regex_pattern.substr(pos + 1, end_pos - pos - 1);
                    std::string to_state = "q" + std::to_string(end_pos + 1) + "_" + std::to_string(edits);
                    dot += "  " + from_state + " -> " + to_state + 
                           " [label=\"class: [" + escape_dot_label(char_class) + "]\", color=\"darkblue\"];\n";
                }
            }
        }
    }
    
    // Add epsilon transitions (regex * and + operators)
    for (int pos = 0; pos < len; ++pos) {
        if (regex_pattern[pos] == '*' || regex_pattern[pos] == '+') {
            for (int edits = 0; edits <= maxEdits; ++edits) {
                std::string state = "q" + std::to_string(pos) + "_" + std::to_string(edits);
                // Self-loop for repetition
                dot += "  " + state + " -> " + state + 
                       " [label=\"repeat: '" + std::string(1, regex_pattern[pos-1]) + 
                       (regex_pattern[pos] == '*' ? "* (0+)\"" : "+ (1+)\"") + 
                       ", color=\"goldenrod\", style=\"dashed\"];\n";
            }
        }
    }
    

    dot += "}\n";
    return dot;
}

