// ui_controller.hpp
#ifndef UI_CONTROLLER_HPP
#define UI_CONTROLLER_HPP

#include "toxicity_analyzer.hpp"
#include "chat_analyzer.hpp"
#include <iostream>
#include <string>

class ChatModerationUI {
private:
    ToxicityAnalyzer analyzer;
    ChatLogAnalyzer log_analyzer;

    void print_menu();
    int get_choice();
    void analyze_toxic_phrases();
    void analyze_disguised_toxicity();
    void validate_structures();
    void view_analysis_reports();
    void analyze_chat_log();

public:
    void run();
};

#endif