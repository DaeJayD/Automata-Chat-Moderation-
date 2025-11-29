// chat_analyzer.hpp
#ifndef CHAT_ANALYZER_HPP
#define CHAT_ANALYZER_HPP

#include "toxicity_analyzer.hpp"
#include <fstream>
#include <string>

class ChatLogAnalyzer {
private:
    ToxicityAnalyzer analyzer;

    void print_analysis_result(const ToxicityAnalyzer::AnalysisResult& result);

public:
    void analyze_file(const std::string& filename);
};

#endif