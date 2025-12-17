#ifndef PDA_ENGINE_HPP
#define PDA_ENGINE_HPP

#include <vector>
#include <string>
#include <stack>
#include <unordered_set>
#include <tuple>
#include <memory>

// Forward declaration to avoid circular dependency
class ApproximateMatcher;

/**
 * @struct PDANode
 * @brief Represents a state in the Pushdown Automaton
 */
struct PDANode {
    int id;  ///< Unique identifier for the state
    bool is_final;  ///< Whether this is an accepting state
    std::vector<std::tuple<char, char, std::string, int>> transitions;  ///< Transitions: (input, pop, push, to_state)
};

/**
 * @class PDA
 * @brief Pushdown Automaton implementation for context-free language recognition
 * 
 * This class implements a PDA that can:
 * 1. Check balanced brackets (parentheses, braces, brackets)
 * 2. Detect toxic words within nested structures
 * 3. Generate visualization diagrams
 * 
 * Unlike finite state machines, PDAs use a stack for memory,
 * allowing them to recognize context-free languages like balanced brackets.
 */
class PDA {
private:
    std::vector<PDANode> nodes;  ///< All states in the PDA
    std::unordered_set<int> final_states;  ///< Set of accepting states
    int start_state;  ///< Starting state ID

public:
    /**
     * @brief Default constructor
     */
    PDA();
    
    /**
     * @brief Add a new state to the PDA
     * @param is_final Whether the state is accepting
     * @return ID of the newly created state
     */
    int add_node(bool is_final);
    
    /**
     * @brief Add a transition between states
     * @param from Source state ID
     * @param to Destination state ID
     * @param input Input character (0 for epsilon/empty transition)
     * @param pop Character to pop from stack
     * @param push String to push onto stack
     */
    void add_transition(int from, int to, char input, char pop, const std::string& push);
    
    /**
     * @brief Simple bracket balancing check
     * 
     * Checks if all brackets in the input are properly balanced.
     * This is a context-free language property.
     * 
     * @param input String to check
     * @return true if brackets are balanced, false otherwise
     */
    bool simulate(const std::string& input);
    
    /**
     * @brief Enhanced simulation with toxic word detection
     * 
     * Checks bracket balance AND detects toxic words within brackets
     * using approximate matching. Demonstrates the combination of:
     * - Context-free language (brackets) via PDA stack
     * - Regular language (toxic words) via FSM approximate matching
     * 
     * @param input String to analyze
     * @param matcher Approximate matcher for toxic word detection
     * @param toxic_patterns Patterns to match against
     * @param max_edits Maximum edit distance for approximate matching
     * @param found_toxic_words Output vector for found toxic words
     * @return true if brackets are balanced, false otherwise
     */
    bool simulate_with_toxicity(const std::string& input, 
                               ApproximateMatcher& matcher,
                               const std::vector<std::string>& toxic_patterns,
                               int max_edits,
                               std::vector<std::string>& found_toxic_words);
    
    /**
     * @brief Simulate Markdown-like nested structure parsing
     * 
     * This demonstrates how a PDA can parse nested structures
     * similar to Markdown headings, lists, and code blocks.
     * 
     * @param input String to simulate
     * @param error_positions Output vector for error positions
     * @param error_message Output string for error description
     * @return true if valid nested structure, false otherwise
     */
    bool simulate_markdown(const std::string& input, 
                          std::vector<std::pair<int, int>>& error_positions,
                          std::string& error_message);
    
    /**
     * @brief Detect injection attempts in nested structures
     * 
     * Checks for patterns that might indicate code injection
     * attempts within nested brackets or markdown.
     * 
     * @param input String to analyze
     * @param warnings Output vector for warnings and alerts
     * @return true if injection attempts detected, false otherwise
     */
    bool detect_injection_attempts(const std::string& input,
                                  std::vector<std::string>& warnings);
    
    /**
     * @brief Generate DOT language representation of the PDA
     * 
     * Creates a visualization that shows:
     * - States and their types (start, accepting, regular)
     * - Transitions with input/pop/push operations
     * - Stack operations demonstrating context-free nature
     * 
     * @return DOT language string for Graphviz visualization
     */
    std::string toDot() const;
    
    /**
     * @brief Generate nested DOT representation
     * 
     * Creates a hierarchical DOT representation for complex
     * nested automata structures.
     * 
     * @return DOT language string with nested structure
     */
    std::string toDotNested() const;
    
    /**
     * @brief Get the start state ID
     * @return Start state ID
     */
    int get_start_state() const { return start_state; }
    
    /**
     * @brief Get the number of states
     * @return Total number of states in the PDA
     */
    size_t get_state_count() const { return nodes.size(); }
    
    /**
     * @brief Check if a state is accepting
     * @param state_id State ID to check
     * @return true if state is accepting, false otherwise
     */
    bool is_final_state(int state_id) const { 
        return final_states.find(state_id) != final_states.end(); 
    }
};

/**
 * @class BracketPDA
 * @brief Factory class for creating predefined PDA configurations
 */
class BracketPDA {
public:
    /**
     * @brief Create a PDA for balanced bracket checking
     * 
     * Creates a minimal PDA that recognizes strings with balanced:
     * - Parentheses: ()
     * - Square brackets: []
     * - Curly braces: {}
     * - Angle brackets: <>
     * 
     * This demonstrates a classic context-free language.
     * 
     * @return PDA configured for bracket balancing
     */
    static PDA create_balanced_bracket_pda();
    
    /**
     * @brief Create a PDA for Markdown-like nested structure parsing
     * 
     * Creates a PDA that can parse nested markdown structures
     * with proper balancing of headings, lists, and code blocks.
     * 
     * @param strict_nesting Whether to enforce strict nesting rules
     * @return PDA configured for markdown parsing
     */
    static PDA create_markdown_pda(bool strict_nesting = true);
    
    /**
     * @brief Create a PDA for toxic word detection within brackets
     * 
     * Creates a PDA that:
     * 1. Checks for balanced brackets (context-free)
     * 2. Detects toxic words inside brackets using approximate matching (regular)
     * 
     * This demonstrates the Chomsky hierarchy: Regular ⊆ Context-Free
     * 
     * @param toxic_patterns Patterns to detect as toxic
     * @param max_edits Maximum edit distance for approximate matching
     * @return PDA configured for toxic detection in nested structures
     */
    static PDA create_toxic_detection_pda(const std::vector<std::string>& toxic_patterns, 
                                         int max_edits = 1);
};

#endif // PDA_ENGINE_HPP