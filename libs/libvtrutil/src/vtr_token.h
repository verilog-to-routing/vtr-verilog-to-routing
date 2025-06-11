#pragma once
/**
 * @file
 * @author Jason Luu
 * @Date July 22, 2009
 * @brief Tokenizer
 */

#include <vector>
#include <string>

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

/// @brief Returns a token list of the text for a given string.
std::vector<t_token> GetTokensFromString(const char* inString);

///@brief Returns true if the token's type equals to token_type
bool checkTokenType(const t_token& token, e_token_type token_type);

void my_atof_2D(float** matrix, const int max_i, const int max_j, const char* instring);

bool check_my_atof_2D(const int max_i, const int max_j, const char* instring, int* num_entries);
