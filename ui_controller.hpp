#ifndef UI_CONTROLLER_HPP
#define UI_CONTROLLER_HPP

#include "toxicity_analyzer.hpp"
#include "chat_analyzer.hpp"
#include "nfa_engine.hpp"      // ADD THIS
#include "dfa_engine.hpp"      // ADD THIS
#include "pda_engine.hpp"      // ADD THIS
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <utility>
#include <memory>
#include <cstdlib>            // ADD THIS for system()

// Forward declarations
struct BracketContent {
    char open_bracket;
    char close_bracket;
    std::string content;
    bool is_toxic;
    std::string matched_pattern;
    int edit_distance;
};

struct XMLMessageResult {
    std::string text;
    std::vector<std::string> exact_matches;
    std::vector<std::pair<std::string, std::string>> approx_matches;
    std::vector<BracketContent> bracket_contents;
    bool has_toxic_content;
    int toxicity_score;
    
    XMLMessageResult() : has_toxic_content(false), toxicity_score(0) {}
    explicit XMLMessageResult(const std::string& t) : text(t), has_toxic_content(false), toxicity_score(0) {}
};

class ChatModerationUI {
private:
    ToxicityAnalyzer analyzer;
    ChatLogAnalyzer log_analyzer;

    // Color constants
    static constexpr const char* RED = "\033[31m";
    static constexpr const char* GREEN = "\033[32m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* BLUE = "\033[34m";
    static constexpr const char* CYAN = "\033[36m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* RESET = "\033[0m";

    // Core UI functions
    void print_menu();
    int get_choice();
    
    // Analysis functions
    void analyze_toxic_phrases();
    void analyze_disguised_toxicity();
    void validate_structures();
    void view_analysis_reports();
    void analyze_chat_log();

    // Helper functions
    std::string sanitize(const std::string& s);
    std::vector<std::string> extract_patterns_from_regex(const std::string& regex);
    std::vector<std::string> parse_simple_regex(const std::string& regex);
    std::string get_current_timestamp();

    // XML Analysis functions
    std::vector<XMLMessageResult> parse_and_analyze_xml(
        const std::string& filename, 
        const std::vector<std::string>& toxic_patterns, 
        int max_edits = 2);
    
    void analyze_message_content(
        XMLMessageResult& result,
        const std::string& text,
        const std::vector<std::string>& toxic_patterns,
        int max_edits = 2);
    
    void display_xml_analysis(
        const std::vector<XMLMessageResult>& results,
        const std::vector<std::string>& toxic_patterns,
        int max_edits = 2);
    
    void generate_xml_analysis_diagrams(
        const std::vector<XMLMessageResult>& results,
        const std::vector<std::string>& toxic_patterns,
        int diagram_type = 4);

    // Template helper for diagram generation
    template<typename Automaton>
    bool generate_diagram(const Automaton& automaton,
                         const std::string& filename,
                         const std::string& type,
                         const std::string& input = "") {
        std::string dot_file = filename + ".dot";
        std::string png_file = filename + ".png";
        
        std::ofstream fout(dot_file);
        if (!fout.is_open()) {
            std::cout << RED << " Cannot create DOT file: " << dot_file << "\n" << RESET;
            return false;
        }
        
        // Generate DOT content based on automaton type
        if constexpr (std::is_same_v<Automaton, NFA>) {
            fout << automaton.toDot();
        } else if constexpr (std::is_same_v<Automaton, DFA>) {
            // DFA might not have toDotWithInput method
            if (!input.empty() && typeid(automaton).name() == typeid(DFA).name()) {
                // Try toDotWithInput if it exists, fallback to toDot
                try {
                    fout << automaton.toDotWithInput(input);
                } catch(...) {
                    fout << automaton.toDot();
                }
            } else {
                fout << automaton.toDot();
            }
        } else if constexpr (std::is_same_v<Automaton, PDA>) {
            // PDA might have toDot() method, use it if available
            try {
                fout << automaton.toDot();
            } catch(...) {
                // Fallback simple PDA diagram
                fout << "digraph PDA {\n";
                fout << "    rankdir=LR;\n";
                fout << "    node [shape=circle];\n";
                fout << "    start [shape=point];\n";
                fout << "    start -> q0;\n";
                fout << "    q0 -> q1 [label=\"Push bracket\"];\n";
                fout << "    q1 -> q2 [label=\"Read content\"];\n";
                fout << "    q2 -> q3 [label=\"Pop bracket\"];\n";
                fout << "    q3 [shape=doublecircle];\n";
                fout << "}\n";
            }
        }
        
        fout.close();
        
        // Convert to PNG
        std::string command = "dot -Tpng \"" + dot_file + "\" -o \"" + png_file + "\"";
        if (system(command.c_str()) == 0) {
            std::cout << GREEN << "✓ " << type << " diagram: " << png_file << "\n" << RESET;
            return true;
        } else {
            std::cout << YELLOW << "Note: Graphviz not installed or failed\n" << RESET;
            return false;
        }
    }

public:
    void run();
    std::string create_chat_moderation_pda_dot(const std::vector<std::string>& toxic_patterns);
};

#endif // UI_CONTROLLER_HPP