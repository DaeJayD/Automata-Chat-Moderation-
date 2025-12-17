#ifndef APPROXIMATE_MATCHER_HPP
#define APPROXIMATE_MATCHER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <regex>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <fstream>

/**
 * @class ApproximateMatcher
 * @brief Performs approximate pattern matching with regex support and FSM visualization
 * 
 * This class is designed for chat moderation systems where patterns need to be
 * matched approximately (allowing for typos, leet speak, and minor variations)
 * while visualizing the finite state machine that performs the matching.
 */
class ApproximateMatcher {
private:
    bool verbose_mode;  // Already added
    
public:
    /**
     * @struct MatchResult
     * @brief Represents a matching result with similarity metrics
     */
    struct MatchResult {
        std::string original;        ///< The original word/text that was matched
        std::string matched_pattern; ///< The pattern that was matched against
        int distance;                ///< Levenshtein edit distance (0 = exact match)
        double similarity;           ///< Similarity percentage (0-100%)
        
        MatchResult(const std::string& orig, const std::string& matched, int dist, double sim)
            : original(orig), matched_pattern(matched), distance(dist), similarity(sim) {}
    };

    // INLINE CONSTRUCTOR - ADD THIS
    ApproximateMatcher(bool verbose = true) : verbose_mode(verbose) {}

    std::vector<MatchResult> find_matches(const std::string& message, 
                                          const std::string& regex_pattern, 
                                          int maxEdits = 2);

    
    std::string toDotRegexFSM(const std::string& regex_pattern, int maxEdits);
    std::string preprocess_message(const std::string& message);

    void set_verbose(bool verbose) { verbose_mode = verbose; }

private:
    std::vector<MatchResult> find_word_matches(const std::string& word, 
                                               const std::string& regex_pattern, 
                                               int maxEdits);
    std::string escape_dot_label(const std::string& s);
    
    // ADD THIS PRIVATE METHOD DECLARATION
    int levenshtein_distance(const std::string& s1, const std::string& s2);
};

#endif // APPROXIMATE_MATCHER_HPP