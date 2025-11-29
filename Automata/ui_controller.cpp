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
    cout << CYAN << "========================================\n";
    cout << "   CHAT MODERATION SYSTEM\n";
    cout << "   Formal Languages & Automata\n";
    cout << "========================================\n" << RESET;

    while (true) {
        print_menu();
        int choice = get_choice(); // always valid 1-6

        switch (choice) {
            case 1: analyze_toxic_phrases(); break;
            case 2: analyze_disguised_toxicity(); break;
            case 3: validate_structures(); break;
            case 4: view_analysis_reports(); break;
            case 5: analyze_chat_log(); break;
            case 6:
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
    cout << "4. View Analysis Reports\n";
    cout << "5. Analyze Chat Log File\n";
    cout << "6. Exit\n";
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
    cout << "\n" << CYAN << "=== TOXIC PHRASE DETECTION (Regex → NFA/DFA) ===\n" << RESET;

    cout << "Enter regex pattern to match toxic phrases: ";
    string regex_pattern;
    getline(cin, regex_pattern);

    NFA toxic_nfa = RegexToNFA::from_regex(regex_pattern);

    cout << "Do you want to convert NFA to DFA for faster matching? (y/n): ";
    string input; 
    getline(cin, input);
    char dfa_choice = !input.empty() ? input[0] : 'n';

    DFA toxic_dfa;
    bool use_dfa = false;
    if (dfa_choice == 'y' || dfa_choice == 'Y') {
        toxic_dfa = convert_nfa_to_dfa(toxic_nfa);
        use_dfa = true;
        cout << GREEN << "DFA conversion completed.\n" << RESET;
    }

    // Step 1: User enters message to analyze
    cout << "Enter message to analyze: ";
    string message;
    getline(cin, message);

    string lower_msg = message;
    transform(lower_msg.begin(), lower_msg.end(), lower_msg.begin(), ::tolower);

    vector<pair<int,int>> match_positions;
    for (size_t i = 0; i < lower_msg.size(); ++i) {
        for (size_t j = i+1; j <= lower_msg.size(); ++j) {
            string substr = lower_msg.substr(i, j-i);
            bool matched = use_dfa ? toxic_dfa.simulate(substr) : toxic_nfa.simulate(substr);
            if (matched) match_positions.push_back({i, j});
        }
    }

    string highlighted = message;
    int offset = 0;
    for (auto [start, end] : match_positions) {
        highlighted.insert(start + offset, RED);
        offset += strlen(RED);
        highlighted.insert(end + offset, RESET);
        offset += strlen(RESET);
    }

    bool has_matches = !match_positions.empty();
    cout << "\nAnalysis Result:\n";
    cout << "Message: " << message << "\n";
    cout << "Matches regex: " << (has_matches ? RED "YES" : GREEN "NO") << RESET << "\n";

    if (has_matches) {
        cout << "Highlighted Message: " << highlighted << "\n";
        cout << YELLOW << "Warning: Toxic content detected!\n" << RESET;
    }

    //
    // === FIXED MENU LOGIC ===
    //

    if (use_dfa) {
        // ONLY ask this if user chose DFA
        cout << "Do you want to generate the DFA diagram? (y/n): ";
        char dfa_diag;
        cin >> dfa_diag;
        cin.ignore();

        if (dfa_diag == 'y' || dfa_diag == 'Y') {
            string dfa_dot = "regex_dfa.dot";
            string dfa_png = "regex_dfa.png";
            ofstream fout(dfa_dot);
            if (fout.is_open()) {
                fout << toxic_dfa.toDotWithInput(message);  
                fout.close();
                string command = "dot -Tpng " + dfa_dot + " -o " + dfa_png;
                if (system(command.c_str()) == 0) {
#ifdef _WIN32
                    ShellExecuteA(NULL,"open",dfa_png.c_str(),NULL,NULL,SW_SHOWNORMAL);
#else
                    system(("xdg-open " + dfa_png + " &").c_str());
#endif
                    cout << GREEN << "DFA Diagram generated: " << dfa_png << "\n" << RESET;
                }
            }
        }
    } 
    else {
        // ONLY ask this if user DID NOT choose DFA (NFA mode)
        cout << "Do you want to generate the NFA diagram? (y/n): ";
        getline(cin, input);
        char nfa_diag = !input.empty() ? input[0] : 'n';

        if (nfa_diag == 'y' || nfa_diag == 'Y') {
            string dot_filename = "regex_nfa.dot";
            string png_filename = "regex_nfa.png";
            ofstream fout(dot_filename);
            if (fout.is_open()) {
                fout << toxic_nfa.toDot();
                fout.close();
                string command = "dot -Tpng " + dot_filename + " -o " + png_filename;
                if (system(command.c_str()) == 0) {
#ifdef _WIN32
                    ShellExecuteA(NULL, "open", png_filename.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
                    system(("xdg-open " + png_filename + " &").c_str());
#endif
                    cout << GREEN << "NFA Diagram generated: " << png_filename << "\n" << RESET;
                }
            }
        }
    }
}



void ChatModerationUI::analyze_disguised_toxicity() {
    cout << "\n" << CYAN << "=== DISGUISED TOXICITY DETECTION ===\n" << RESET;
    cout << "Enter message to analyze: ";
    string message; getline(cin, message);

    ApproximateMatcher matcher;
    auto matches = matcher.find_matches(message, 1);

    cout << "\nApproximate Matching Results:\n";
    if (matches.empty()) cout << GREEN << "No disguised toxicity detected.\n" << RESET;
    else {
        cout << YELLOW << "Disguised toxicity detected:\n" << RESET;
        for (const auto& match : matches) {
            cout << "  '" << match.original << "' -> '" << match.matched_word 
                 << "' (edit distance: " << match.distance 
                 << ", similarity: " << fixed << setprecision(1) 
                 << match.similarity << "%)\n";
        }
    }
}

void ChatModerationUI::validate_structures() {
    cout << "\n" << CYAN << "=== STRUCTURE VALIDATION (PDA) ===\n" << RESET;
    
    // Test with known examples first
    vector<pair<string, bool>> test_cases = {
        {"()", true},           // Simple balanced
        {"(())", true},         // Nested balanced  
        {"()()", true},         // Multiple pairs
        {"(", false},           // Unmatched open
        {")", false},           // Unmatched close
        {"())", false},         // Extra close
        {"(()", false},         // Extra open
        {"Hello (world)", true}, // With text
        {"(a(b)c)", true},      // Nested with text
        {"(a[b]c)", true},      // Mixed brackets
        {"(a[b)c]", false},     // Crossed brackets
    };

    PDA pda = BracketPDA::create_balanced_bracket_pda();
    
    cout << "Testing PDA Implementation:\n";
    cout << "============================\n";
    
    int passed = 0;
    for (const auto& [test, expected] : test_cases) {
        bool result = pda.simulate(test);
        bool correct = (result == expected);
        
        if (correct) passed++;
        
        cout << "  \"" << test << "\" -> " 
             << (result ? GREEN "VALID" : RED "INVALID") << RESET
             << " " << (correct ? "✓" : "✗") << "\n";
    }
    
    cout << "\nResults: " << passed << "/" << test_cases.size() << " tests passed\n";
    
    if (passed == test_cases.size()) {
        cout << GREEN << "✅ PDA is working correctly!\n" << RESET;
    } else {
        cout << RED << "❌ PDA has issues - " << (test_cases.size() - passed) << " tests failed\n" << RESET;
    }

    // Now test user input
    cout << "\n" << CYAN << "Enter your own test string: " << RESET;
    string message; 
    getline(cin, message);
    
    bool result = pda.simulate(message);
    cout << "Result: \"" << message << "\" is " 
         << (result ? GREEN "VALID (balanced)" : RED "INVALID (unbalanced)") << RESET << "\n";
    
    // Educational explanation
    cout << "\n" << YELLOW << "=== FORMAL LANGUAGE EXPLANATION ===\n" << RESET;
    cout << "This demonstrates why we need Pushdown Automata (PDA) instead of NFA:\n";
    cout << "• NFA/DFA (Regular Languages): Can only match patterns, cannot count\n";
    cout << "• PDA (Context-Free Languages): Can count nested structures using a stack\n";
    cout << "• Language L = { balanced brackets } is context-free, not regular!\n";
}

void ChatModerationUI::view_analysis_reports() {
    cout << "\n" << CYAN << "=== COMPREHENSIVE ANALYSIS REPORT ===\n" << RESET;
    cout << "Enter message for complete analysis: ";
    string message; getline(cin, message);

    auto result = analyzer.analyze_message(message);

    cout << "\n" << BLUE << "COMPREHENSIVE ANALYSIS:\n" << RESET;
    cout << "Message: " << result.message << "\n";

    string toxicity_color = RESET;
    string toxicity_level;
    if (result.toxicity_score >= 70) { toxicity_color = RED; toxicity_level = "HIGH"; }
    else if (result.toxicity_score >= 30) { toxicity_color = YELLOW; toxicity_level = "MEDIUM"; }
    else { toxicity_color = GREEN; toxicity_level = "LOW"; }

    cout << "Toxicity Score: " << toxicity_color << result.toxicity_score 
         << "/100 (" << toxicity_level << ")" << RESET << "\n";

    if (!result.exact_matches.empty()) {
        cout << RED << "Exact Matches: ";
        for (const auto& match : result.exact_matches) cout << match << " ";
        cout << RESET << "\n";
    }

    if (!result.approx_matches.empty()) {
        cout << YELLOW << "Approximate Matches: ";
        for (const auto& match : result.approx_matches)
            cout << match.original << "(->" << match.matched_word << ") ";
        cout << RESET << "\n";
    }

    cout << "Structure: " << (result.valid_structure ? GREEN : RED) 
         << result.structure_type << RESET << "\n";

    cout << "\n" << CYAN << "RECOMMENDATION: ";
    if (result.toxicity_score >= 70) cout << RED << "BLOCK MESSAGE - High toxicity detected\n" << RESET;
    else if (result.toxicity_score >= 30) cout << YELLOW << "FLAG FOR REVIEW - Moderate toxicity detected\n" << RESET;
    else cout << GREEN << "ALLOW MESSAGE - Low toxicity\n" << RESET;
}

void ChatModerationUI::analyze_chat_log() {
    cout << "\n" << CYAN << "=== CHAT LOG ANALYSIS ===\n" << RESET;
    cout << "Enter chat log filename: ";
    string filename; getline(cin, filename);
    log_analyzer.analyze_file(filename);
}
