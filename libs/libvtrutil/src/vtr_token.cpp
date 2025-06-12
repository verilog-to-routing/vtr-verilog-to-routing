/**
 * Jason Luu
 * July 22, 2009
 * Tokenizer
 */

#include "vtr_token.h"

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_memory.h"

#include <cctype>

/// @brief Returns a token type of the given char
static e_token_type get_token_type_from_char(e_token_type cur_token_type, char cur);

const t_token Tokens::null_token_{e_token_type::NULL_TOKEN, ""};

Tokens::Tokens(std::string_view inString) {
    if (inString.empty()) {
        return;
    }

    e_token_type cur_token_type = e_token_type::NULL_TOKEN;
    size_t in_string_index = 0;
    size_t prev_in_string_index = 0;

    for (char cur : inString) {
        e_token_type new_token_type = get_token_type_from_char(cur_token_type, cur);
        if (new_token_type != cur_token_type) {
            if (cur_token_type != e_token_type::NULL_TOKEN) {
                // Finalize the current token
                t_token& current_token = tokens_.back();
                current_token.data = inString.substr(prev_in_string_index,
                                                     in_string_index - prev_in_string_index);
            }
            if (new_token_type != e_token_type::NULL_TOKEN) {
                // Start a new token
                t_token new_token;
                new_token.type = new_token_type;
                tokens_.push_back(new_token);
                prev_in_string_index = in_string_index;
            }
            cur_token_type = new_token_type;
        }
        in_string_index++;
    }

    // Finalize the last token if it exists
    if (cur_token_type != e_token_type::NULL_TOKEN && !tokens_.empty()) {
        t_token& current_token = tokens_.back();
        current_token.data = inString.substr(prev_in_string_index,
                                             in_string_index - prev_in_string_index);
    }
}

const t_token& Tokens::operator[](size_t idx) const {
    if (idx < tokens_.size()) {
        return tokens_[idx];
    } else {
        return null_token_;
    }
}

static e_token_type get_token_type_from_char(e_token_type cur_token_type, char cur) {
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
