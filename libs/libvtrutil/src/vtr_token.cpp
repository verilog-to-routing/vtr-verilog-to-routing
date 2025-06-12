/**
 * Jason Luu
 * July 22, 2009
 * Tokenizer
 */

#include "vtr_token.h"

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_memory.h"

enum e_token_type GetTokenTypeFromChar(const enum e_token_type cur_token_type,
                                       const char cur);

bool IsWhitespace(char c);

///@brief Returns true if character is whatspace between tokens
bool IsWhitespace(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            return true;
        default:
            return false;
    }
}

std::vector<t_token> GetTokensFromString(std::string_view inString) {
    std::vector<t_token> tokens;

    if (inString.empty()) {
        return tokens;
    }

    e_token_type cur_token_type = TOKEN_NULL;
    size_t in_string_index = 0;
    size_t prev_in_string_index = 0;

    for (char cur : inString) {
        e_token_type new_token_type = GetTokenTypeFromChar(cur_token_type, cur);
        if (new_token_type != cur_token_type) {
            if (cur_token_type != TOKEN_NULL) {
                // Finalize the current token
                t_token& current_token = tokens.back();
                current_token.data = std::string(inString.substr(prev_in_string_index,
                                                                 in_string_index - prev_in_string_index));
            }
            if (new_token_type != TOKEN_NULL) {
                // Start a new token
                t_token new_token;
                new_token.type = new_token_type;
                tokens.push_back(new_token);
                prev_in_string_index = in_string_index;
            }
            cur_token_type = new_token_type;
        }
        in_string_index++;
    }

    // Finalize the last token if it exists
    if (cur_token_type != TOKEN_NULL && !tokens.empty()) {
        t_token& current_token = tokens.back();
        current_token.data = std::string(inString.substr(prev_in_string_index,
                                                         in_string_index - prev_in_string_index));
    }

    return tokens;
}

///@brief Returns a token type of the given char
enum e_token_type GetTokenTypeFromChar(const enum e_token_type cur_token_type,
                                       const char cur) {
    if (IsWhitespace(cur)) {
        return TOKEN_NULL;
    } else {
        if (cur == '[') {
            return TOKEN_OPEN_SQUARE_BRACKET;
        } else if (cur == ']') {
            return TOKEN_CLOSE_SQUARE_BRACKET;
        } else if (cur == '{') {
            return TOKEN_OPEN_SQUIG_BRACKET;
        } else if (cur == '}') {
            return TOKEN_CLOSE_SQUIG_BRACKET;
        } else if (cur == ':') {
            return TOKEN_COLON;
        } else if (cur == '.') {
            return TOKEN_DOT;
        } else if (cur >= '0' && cur <= '9' && cur_token_type != TOKEN_STRING) {
            return TOKEN_INT;
        } else {
            return TOKEN_STRING;
        }
    }
}

///@brief Returns true if the token's type equals to token_type
bool checkTokenType(const t_token& token, enum e_token_type token_type) {
    if (token.type != token_type) {
        return false;
    }
    return true;
}

///@brief Returns a 2D array representing the atof result of all the input string entries seperated by whitespace
void my_atof_2D(float** matrix, const int max_i, const int max_j, const char* instring) {
    int i, j;
    char *cur, *cur2, *copy, *final;

    copy = vtr::strdup(instring);
    final = copy;
    while (*final != '\0') {
        final++;
    }

    cur = copy;
    i = j = 0;
    while (cur != final) {
        while (IsWhitespace(*cur) && cur != final) {
            if (j == max_j) {
                i++;
                j = 0;
            }
            cur++;
        }
        if (cur == final) {
            break;
        }
        cur2 = cur;
        while (!IsWhitespace(*cur2) && cur2 != final) {
            cur2++;
        }
        *cur2 = '\0';
        VTR_ASSERT(i < max_i && j < max_j);
        matrix[i][j] = vtr::atof(cur);
        j++;
        cur = cur2;
        *cur = ' ';
    }

    VTR_ASSERT((i == max_i && j == 0) || (i == max_i - 1 && j == max_j));

    free(copy);
}

/** 
 * @brief Checks if the number of entries (separated by whitespace)	matches the the expected number (max_i * max_j)
 *
 * can be used before calling my_atof_2D						
 */
bool check_my_atof_2D(const int max_i, const int max_j, const char* instring, int* num_entries) {
    /* Check if max_i * max_j matches number of entries in instring */
    const char* cur = instring;
    bool in_str = false;
    int entry_count = 0;

    /* First count number of entries in instring */
    while (*cur != '\0') {
        if (!IsWhitespace(*cur) && !in_str) {
            in_str = true;
            entry_count++;
        } else if (IsWhitespace(*cur)) {
            in_str = false;
        }
        cur++;
    }
    *num_entries = entry_count;

    if (max_i * max_j != entry_count) return false;
    return true;
}
