#pragma once
/**
 * @file
 * @author Jason Luu
 * @Date July 22, 2009
 * @brief Tokenizer
 */

#include <string>
#include <string_view>
#include <vector>

///@brief Token types
enum class e_token_type {
    NULL_TOKEN,
    STRING,
    INT,
    OPEN_SQUARE_BRACKET,
    CLOSE_SQUARE_BRACKET,
    OPEN_SQUIG_BRACKET,
    CLOSE_SQUIG_BRACKET,
    COLON,
    DOT
};

///@brief Token structure
struct t_token {
    e_token_type type;
    std::string data;
};

class Tokens {
  public:
    /// @brief Creates tokens for a given string
    Tokens(std::string_view inString);

    const t_token& operator[](size_t idx) const;

    size_t size() const { return tokens_.size(); }

  private:
    static const t_token null_token_;
    std::vector<t_token> tokens_;
};

/// @brief Returns a 2D array representing the atof result of all the input string entries separated by whitespace
void my_atof_2D(float** matrix, const int max_i, const int max_j, std::string_view instring);

/**
 * @brief Checks if the number of entries (separated by whitespace)	matches the expected number (max_i * max_j)
 *
 * can be used before calling my_atof_2D
 */
bool check_my_atof_2D(const int max_i, const int max_j, std::string_view instring, int* num_entries);
