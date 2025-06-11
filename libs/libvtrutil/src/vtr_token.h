#pragma once
/**
 * @file
 * @author Jason Luu
 * @Date July 22, 2009
 * @brief Tokenizer
 */

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
    char* data;
};

t_token* GetTokensFromString(const char* inString, int* num_tokens);

void freeTokens(t_token* tokens, const int num_tokens);

bool checkTokenType(const t_token token, enum e_token_type token_type);

void my_atof_2D(float** matrix, const int max_i, const int max_j, const char* instring);

bool check_my_atof_2D(const int max_i, const int max_j, const char* instring, int* num_entries);
