// toxicity_analyzer.hpp
#ifndef TOXICITY_ANALYZER_HPP
#define TOXICITY_ANALYZER_HPP

#include "nfa_engine.hpp"
#include "approximate_matcher.hpp"
#include "pda_engine.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

class ToxicityAnalyzer {
private:
    NFA toxic_nfa;
    ApproximateMatcher approx_matcher;
    PDA bracket_pda;
    PDA formatting_pda;

    std::vector<std::string> find_exact_matches(const std::string& message);
    bool validate_structures(const std::string& message);

public:
    ToxicityAnalyzer();

    struct AnalysisResult {
        int toxicity_score;
        std::vector<std::string> exact_matches;
        std::vector<ApproximateMatcher::MatchResult> approx_matches;
        bool valid_structure;
        std::string structure_type;
        std::string message;
    };

    AnalysisResult analyze_message(const std::string& message);
};

#endif