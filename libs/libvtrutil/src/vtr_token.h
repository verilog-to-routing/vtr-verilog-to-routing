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
enum e_token_type {
    TOKEN_NULL,
    TOKEN_STRING,
    TOKEN_INT,
    TOKEN_OPEN_SQUARE_BRACKET,
    TOKEN_CLOSE_SQUARE_BRACKET,
    TOKEN_OPEN_SQUIG_BRACKET,
    TOKEN_CLOSE_SQUIG_BRACKET,
    TOKEN_COLON,
    TOKEN_DOT
};

///@brief Token structure
struct t_token {
    enum e_token_type type;
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


bool checkTokenType(const t_token& token, enum e_token_type token_type);

void my_atof_2D(float** matrix, const int max_i, const int max_j, const char* instring);

bool check_my_atof_2D(const int max_i, const int max_j, const char* instring, int* num_entries);
