/**
 * Jason Luu
 * July 22, 2009
 * Tokenizer
 */

#include "vtr_token.h"

#include <cctype>

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_memory.h"


static e_token_type GetTokenTypeFromChar(const e_token_type cur_token_type,
                                         const char cur);


std::vector<t_token> GetTokensFromString(const char* inString) {
    if (inString == nullptr) {
        return {};
    }

    std::vector<t_token> tokens;
    const char* cur = inString;
    const char* token_start = nullptr;
    e_token_type cur_token_type = e_token_type::NULL_TOKEN;

    while (*cur) {
        e_token_type new_token_type = GetTokenTypeFromChar(cur_token_type, *cur);

        if (new_token_type != cur_token_type) {
            // End the previous token if it exists
            if (cur_token_type != e_token_type::NULL_TOKEN && token_start) {
                tokens.back().data = std::string(token_start, cur); // assign the substring
            }

            // Start a new token if not null
            if (new_token_type != e_token_type::NULL_TOKEN) {
                t_token token;
                token.type = new_token_type;
                // We'll assign token.data when the token ends
                tokens.push_back(token);
                token_start = cur;
            } else {
                token_start = nullptr;
            }

            cur_token_type = new_token_type;
        }
        ++cur;
    }

    // Handle the last token if the string ends in a token
    if (cur_token_type != e_token_type::NULL_TOKEN && token_start) {
        tokens.back().data = std::string(token_start, cur);
    }

    return tokens;
}

///@brief Returns a token type of the given char
static e_token_type GetTokenTypeFromChar(const enum e_token_type cur_token_type,
                                         const char cur) {
    if (std::isspace(cur)) {
        return e_token_type::NULL_TOKEN;
    } else {
        if (cur == '[') {
            return e_token_type::OPEN_SQUARE_BRACKET;
        } else if (cur == ']') {
            return e_token_type::CLOSE_SQUARE_BRACKET;
        } else if (cur == '{') {
            return e_token_type::OPEN_SQUIG_BRACKET;
        } else if (cur == '}') {
            return e_token_type::CLOSE_SQUIG_BRACKET;
        } else if (cur == ':') {
            return e_token_type::COLON;
        } else if (cur == '.') {
            return e_token_type::DOT;
        } else if (cur >= '0' && cur <= '9' && cur_token_type != e_token_type::STRING) {
            return e_token_type::INT;
        } else {
            return e_token_type::STRING;
        }
    }
}

bool checkTokenType(const t_token& token, e_token_type token_type) {
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
        while (std::isspace(*cur) && cur != final) {
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
        while (!std::isspace(*cur2) && cur2 != final) {
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
        if (!std::isspace(*cur) && !in_str) {
            in_str = true;
            entry_count++;
        } else if (std::isspace(*cur)) {
            in_str = false;
        }
        cur++;
    }
    *num_entries = entry_count;

    if (max_i * max_j != entry_count) return false;
    return true;
}
