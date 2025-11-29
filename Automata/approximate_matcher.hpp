// approximate_matcher.hpp
#ifndef APPROXIMATE_MATCHER_HPP
#define APPROXIMATE_MATCHER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <cctype>

class ApproximateMatcher {
public:
    struct MatchResult {
        std::string original;
        std::string matched_word;
        int distance;
        double similarity;
        
        MatchResult(const std::string& orig, const std::string& matched, int dist, double sim)
            : original(orig), matched_word(matched), distance(dist), similarity(sim) {}
    };

    std::vector<MatchResult> find_matches(const std::string& message, int max_distance = 2);

private:
    std::vector<std::string> toxic_words = {
        "stupid", "idiot", "ugly", "dumb", "hate", "fuck", "n00b"
    };

    std::string preprocess_message(const std::string& message);
    std::vector<MatchResult> find_word_matches(const std::string& message, 
                                             const std::string& toxic_word, 
                                             int max_distance);
    int levenshtein_distance(const std::string& s1, const std::string& s2);
};

#endif