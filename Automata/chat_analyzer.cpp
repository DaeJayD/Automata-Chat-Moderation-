// chat_analyzer.cpp
#include "chat_analyzer.hpp"
#include <iostream>

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define RESET   "\033[0m"

void ChatLogAnalyzer::analyze_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << RED << "Error: Could not open file " << filename << RESET << "\n";
        return;
    }

    std::string line;
    int line_number = 1;

    std::cout << CYAN << "\n=== CHAT LOG ANALYSIS ===\n" << RESET;
    
    while (std::getline(file, line)) {
        std::cout << "\n" << YELLOW << "Message " << line_number << ":" << RESET << " " << line << "\n";
        
        auto result = analyzer.analyze_message(line);
        print_analysis_result(result);
        
        line_number++;
    }

    file.close();
}

void ChatLogAnalyzer::print_analysis_result(const ToxicityAnalyzer::AnalysisResult& result) {
    std::string color = RESET;
    if (result.toxicity_score >= 70) color = RED;
    else if (result.toxicity_score >= 30) color = YELLOW;
    else color = GREEN;

    std::cout << color << "Toxicity Score: " << result.toxicity_score << "/100" << RESET << "\n";
    
    if (!result.exact_matches.empty()) {
        std::cout << "Exact Matches: ";
        for (const auto& match : result.exact_matches) {
            std::cout << RED << match << RESET << " ";
        }
        std::cout << "\n";
    }

    if (!result.approx_matches.empty()) {
        std::cout << "Approximate Matches: ";
        for (const auto& match : result.approx_matches) {
            std::cout << YELLOW << match.original << " (-> " << match.matched_pattern
                     << ", dist=" << match.distance << ")" << RESET << " ";
        }
        std::cout << "\n";
    }

    std::cout << "Structure: " << (result.valid_structure ? GREEN : RED) 
              << result.structure_type << RESET << "\n";
}