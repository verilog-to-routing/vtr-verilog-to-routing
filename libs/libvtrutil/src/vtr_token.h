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

///@brief Returns a vector of tokens for a given string.
std::vector<t_token> GetTokensFromString(std::string_view inString);

bool checkTokenType(const t_token& token, enum e_token_type token_type);

void my_atof_2D(float** matrix, const int max_i, const int max_j, const char* instring);

bool check_my_atof_2D(const int max_i, const int max_j, const char* instring, int* num_entries);
