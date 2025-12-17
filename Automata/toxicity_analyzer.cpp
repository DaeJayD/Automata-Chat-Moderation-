// toxicity_analyzer.cpp
#include "toxicity_analyzer.hpp"
#include <sstream>
#include <algorithm>

ToxicityAnalyzer::ToxicityAnalyzer() 
    : toxic_nfa(std::move(RegexToNFA::from_regex("idiot|stupid|ugly|dumb"))),
      bracket_pda(BracketPDA::create_balanced_bracket_pda()),
      formatting_pda() {  // Changed to default constructor
}

ToxicityAnalyzer::AnalysisResult ToxicityAnalyzer::analyze_message(const std::string& message) {
    AnalysisResult result;
    result.message = message;
    result.toxicity_score = 0;

    result.exact_matches = find_exact_matches(message);
    result.toxicity_score += result.exact_matches.size() * 30;

    result.approx_matches = approx_matcher.find_matches(message, ".*", 1);
    result.toxicity_score += result.approx_matches.size() * 20;

    result.valid_structure = validate_structures(message);
    result.structure_type = result.valid_structure ? "Valid" : "Invalid";
    
    if (!result.valid_structure) {
        result.toxicity_score += 10;
    }

    result.toxicity_score = std::min(100, result.toxicity_score);
    return result;
}

std::vector<std::string> ToxicityAnalyzer::find_exact_matches(const std::string& message) {
    std::vector<std::string> matches;
    std::istringstream ss(message);
    std::string word;

    std::vector<std::string>toxic_words = {
    "idiot","stupid","dumb","trash"};


    while (ss >> word) {
        word.erase(std::remove_if(word.begin(), word.end(), 
            [](char c) { return !std::isalnum(c); }), word.end());
        
        for (const auto& toxic_word : toxic_words) {
            if (word.find(toxic_word) != std::string::npos) {
                matches.push_back(toxic_word);
            }
        }
    }

    return matches;
}

bool ToxicityAnalyzer::validate_structures(const std::string& message) {
    std::string structure_chars;
    for (char c : message) {
        if (c == '(' || c == ')' || c == '{' || c == '}' || 
            c == '[' || c == ']' || c == '<' || c == '>' ||
            c == '*') {
            structure_chars += c;
        }
    }

    if (!structure_chars.empty()) {
        return bracket_pda.simulate(structure_chars) || 
               formatting_pda.simulate(structure_chars);
    }

    return true;
}