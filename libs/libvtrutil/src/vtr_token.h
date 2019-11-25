/**
 * Jason Luu
 * July 22, 2009
 * Tokenizer
 */

#ifndef TOKEN_H
#define TOKEN_H

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

struct t_token {
    enum e_token_type type;
    char* data;
};

t_token* GetTokensFromString(const char* inString, int* num_tokens);

void freeTokens(t_token* tokens, const int num_tokens);

bool checkTokenType(const t_token token, enum e_token_type token_type);

void my_atof_2D(float** matrix, const int max_i, const int max_j, const char* instring);

bool check_my_atof_2D(const int max_i, const int max_j, const char* instring, int* num_entries);

#endif
