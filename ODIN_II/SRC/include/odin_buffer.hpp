#ifndef ODIN_BUFFER_HPP
#define ODIN_BUFFER_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include "vtr_memory.h"

#define CHUNK_SIZE 128
/* General Utility methods ------------------------------------------------- */

class dynamic_buffer_t {
  private:
    char* str = NULL;
    size_t len = 0;

  public:
    void push(char in) {
        if (this->len % CHUNK_SIZE == 0) {
            this->str = (char*)vtr::realloc(this->str, sizeof(char) * (this->len + CHUNK_SIZE));
            memset(&this->str[this->len], 0, CHUNK_SIZE);
        }

        this->str[this->len] = in;
        this->len += 1;
    }

    char* dump_str() {
        char* temp = str;
        str = NULL;
        return temp;
    }
};

class buffered_reader_t {
  private:
    FILE* source = NULL;

    bool eof = false;
    bool multiline_comment = false;

    const char* one_line_comment = "";
    size_t one_line_comment_len = 0;

    const char* n_line_comment_ST = "";
    size_t n_line_comment_ST_len = 0;

    const char* n_line_comment_END = "";
    size_t n_line_comment_END_len = 0;

    size_t buffer_size = 2;

    static bool is_eof(int in) {
        return (EOF == in);
    }

    static bool is_nl(int in) {
        return is_eof(in) || ('\n' == (char)in || '\r' == (char)in);
    }

    static bool is_whitespace(int in) {
        return is_nl(in) || (' ' == (char)in || '\t' == (char)in);
    }

    bool is_one_line_comment(const char* in) {
        return (one_line_comment_len && strncmp(in, one_line_comment, one_line_comment_len) == 0);
    }

    bool is_n_line_comment_ST(const char* in) {
        return (n_line_comment_ST_len && strncmp(in, n_line_comment_ST, n_line_comment_ST_len) == 0);
    }

    bool is_n_line_comment_END(const char* in) {
        return (n_line_comment_END_len && strncmp(in, n_line_comment_END, n_line_comment_END_len) == 0);
    }

    buffered_reader_t() {}

  public:
    buffered_reader_t(FILE* _source, const char* _one_line_comment, const char* _n_line_comment_ST, const char* _n_line_comment_END) {
        this->source = _source;

        if (_one_line_comment) {
            this->one_line_comment = _one_line_comment;
            this->one_line_comment_len = strlen(_one_line_comment);
        }

        if (_n_line_comment_ST && _n_line_comment_END) {
            this->n_line_comment_ST = _n_line_comment_ST;
            this->n_line_comment_END = _n_line_comment_END;

            this->n_line_comment_ST_len = strlen(_n_line_comment_ST);
            this->n_line_comment_END_len = strlen(_n_line_comment_END);
        }

        this->buffer_size = (std::max((size_t)2, // for whitespace duplicate
                                      std::max(one_line_comment_len,
                                               std::max(n_line_comment_ST_len,
                                                        n_line_comment_END_len))));
    }

    /*
     * Reads a line from a file stream character-by-character
     * to dynamically build a string.
     * strip duplicate whitespace
     * skip comments
     */
    char* get_line() {
        char* line = NULL;

        if (!(eof)) {
            dynamic_buffer_t my_line;

            char* buffer = (char*)vtr::calloc(this->buffer_size, sizeof(char));

            bool single_line_comment = false;
            size_t skip_count = this->buffer_size - 1;
            bool eol = false;

            bool first_write = true;

            // trim head and compress
            while (!is_nl(buffer[0])) {
                int c = '\n';
                if (!eol && !(eof))
                    c = fgetc(source);

                for (size_t i = 1; i < this->buffer_size; i++) {
                    buffer[i - 1] = buffer[i];
                }

                buffer[this->buffer_size - 1] = (char)c;

                eof = (eof || is_eof(c));
                eol = (eol || is_nl(c));

                if (skip_count) {
                    skip_count--;
                } else if (multiline_comment) {
                    if (is_n_line_comment_END(buffer)) {
                        multiline_comment = false;
                        skip_count = strlen(n_line_comment_END);
                    }
                } else if (single_line_comment) {
                    // read until the end
                } else if (is_one_line_comment(buffer)) {
                    single_line_comment = true;
                } else if (is_n_line_comment_ST(buffer)) {
                    multiline_comment = true;
                } else if (!is_whitespace(buffer[0])) {
                    first_write = false;
                    my_line.push(buffer[0]);
                } else if (!is_whitespace(buffer[1])) {
                    if (!first_write)
                        my_line.push(' ');
                }
            }

            vtr::free(buffer);

            my_line.push('\0');

            line = my_line.dump_str();
        }

        return line;
    }
};

#endif //ODIN_BUFFER_HPP
