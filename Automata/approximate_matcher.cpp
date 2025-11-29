// approximate_matcher.cpp
#include "approximate_matcher.hpp"

std::vector<ApproximateMatcher::MatchResult> ApproximateMatcher::find_matches(
    const std::string& message, int max_distance) {
    
    std::vector<MatchResult> results;
    std::string clean_msg = preprocess_message(message);
    
    for (const auto& word : toxic_words) {
        auto matches = find_word_matches(clean_msg, word, max_distance);
        results.insert(results.end(), matches.begin(), matches.end());
    }
    
    return results;
}

int ApproximateMatcher::levenshtein_distance(const std::string& s1, const std::string& s2) {
    int m = s1.length();
    int n = s2.length();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

    for (int i = 0; i <= m; i++) dp[i][0] = i;
    for (int j = 0; j <= n; j++) dp[0][j] = j;

    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            dp[i][j] = std::min({
                dp[i-1][j] + 1,
                dp[i][j-1] + 1,
                dp[i-1][j-1] + cost
            });
        }
    }

    return dp[m][n];
}

std::string ApproximateMatcher::preprocess_message(const std::string& message) {
    std::string result = message;
    std::unordered_map<char, char> leet_map = {
        {'1', 'i'}, {'0', 'o'}, {'3', 'e'}, {'4', 'a'}, {'5', 's'},
        {'7', 't'}, {'@', 'a'}, {'$', 's'}, {'!', 'i'}
    };

    for (char& c : result) {
        auto it = leet_map.find(std::tolower(c));
        if (it != leet_map.end()) {
            c = it->second;
        }
    }

    result.erase(std::remove_if(result.begin(), result.end(), 
        [](char c) { return !std::isalnum(c) && !std::isspace(c); }), result.end());

    return result;
}

std::vector<ApproximateMatcher::MatchResult> ApproximateMatcher::find_word_matches(
    const std::string& message, const std::string& toxic_word, int max_distance) {
    
    std::vector<MatchResult> matches;
    std::istringstream ss(message);
    std::string word;

    while (ss >> word) {
        int distance = levenshtein_distance(word, toxic_word);
        if (distance <= max_distance) {
            double similarity = (1.0 - static_cast<double>(distance) / 
                               std::max(word.length(), toxic_word.length())) * 100;
            
            matches.emplace_back(word, toxic_word, distance, similarity);
        }
    }

    return matches;
}