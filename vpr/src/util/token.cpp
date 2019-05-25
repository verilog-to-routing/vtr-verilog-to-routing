/**
 * Jason Luu
 * July 22, 2009
 * Tokenizer
 */

#include <cstring>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_memory.h"

#include "token.h"
#include "read_xml_util.h"

enum e_token_type GetTokenTypeFromChar(const enum e_token_type cur_token_type,
                                       const char cur);

bool IsWhitespace(char c);

/* Returns true if character is whatspace between tokens */
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

/* Returns a token list of the text for a given string. */
t_token* GetTokensFromString(const char* inString, int* num_tokens) {
    const char* cur;
    t_token* tokens;
    int i, in_string_index, prev_in_string_index;
    bool has_null;
    enum e_token_type cur_token_type, new_token_type;

    *num_tokens = i = 0;
    cur_token_type = TOKEN_NULL;

    if (inString == nullptr) {
        return nullptr;
    };

    cur = inString;

    /* Count number of tokens */
    while (*cur) {
        new_token_type = GetTokenTypeFromChar(cur_token_type, *cur);
        if (new_token_type != cur_token_type) {
            cur_token_type = new_token_type;
            if (new_token_type != TOKEN_NULL) {
                i++;
            }
        }
        ++cur;
    }
    *num_tokens = i;

    if (*num_tokens > 0) {
        tokens = (t_token*)vtr::calloc(*num_tokens + 1, sizeof(t_token));
    } else {
        return nullptr;
    }

    /* populate tokens */
    i = 0;
    in_string_index = 0;
    has_null = true;
    prev_in_string_index = 0;
    cur_token_type = TOKEN_NULL;

    cur = inString;

    while (*cur) {
        new_token_type = GetTokenTypeFromChar(cur_token_type, *cur);
        if (new_token_type != cur_token_type) {
            if (!has_null) {
                tokens[i - 1].data[in_string_index - prev_in_string_index] = '\0'; /* NULL the end of the data string */
                has_null = true;
            }
            if (new_token_type != TOKEN_NULL) {
                tokens[i].type = new_token_type;
                tokens[i].data = vtr::strdup(inString + in_string_index);
                prev_in_string_index = in_string_index;
                has_null = false;
                i++;
            }
            cur_token_type = new_token_type;
        }
        ++cur;
        in_string_index++;
    }

    VTR_ASSERT(i == *num_tokens);

    tokens[*num_tokens].type = TOKEN_NULL;
    tokens[*num_tokens].data = nullptr;

    /* Return the list */
    return tokens;
}

void freeTokens(t_token* tokens, const int num_tokens) {
    int i;
    for (i = 0; i < num_tokens; i++) {
        free(tokens[i].data);
    }
    free(tokens);
}

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

bool checkTokenType(const t_token token, enum e_token_type token_type) {
    if (token.type != token_type) {
        return false;
    }
    return true;
}

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

/* Date:July 2nd, 2013													*
 * Author: Daniel Chen													*
 * Purpose: Checks if the number of entries (separated by whitespace)	*
 *	        matches the the expected number (max_i * max_j),			*
 *			can be used before calling my_atof_2D						*/
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
