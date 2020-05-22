#include "odin_error.h"
#include "config_t.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

std::vector<std::pair<std::string, int>> include_file_names;

int delayed_errors = 0;

const char* odin_error_STR[] = {
    "",
    "UTIL",
    "PARSE_ARGS",
    "PARSE_TO_AST",
    "AST",
    "NETLIST",
    "PARSE_BLIF",
    "OUTPUT_BLIF",
    "SIMULATION",
};

void verify_delayed_error(odin_error error_type) {
    if (delayed_errors) {
        error_message(error_type, -1, -1, "Parser found (%d) errors in your syntax, exiting", delayed_errors);
    }
}

static std::string make_marker_from_str(std::string str, unsigned long column) {
    str.erase(column);
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] != ' ' && str[i] != '\t') {
            str[i] = ' ';
        }
    }

    str += "^~~~";
    return str;
}

static std::string get_culprit_line(long line_number, const char* file) {
    std::string culprit_line = "";
    FILE* input_file = fopen(file, "r");
    if (input_file) {
        bool copy_characters = false;
        int current_line_number = 0;

        for (;;) {
            int c = fgetc(input_file);
            if (EOF == c) {
                break;
            } else if ('\n' == c) {
                ++current_line_number;
                if (line_number == current_line_number) {
                    copy_characters = true;
                } else if (copy_characters) {
                    break;
                }
            } else if (copy_characters) {
                culprit_line.push_back(c);
            }
        }
        fclose(input_file);
    }
    return culprit_line;
}

static void print_culprit_line(long column, long line_number, const char* file) {
    std::string culprit_line = get_culprit_line(line_number, file);
    if (!culprit_line.empty()) {
        int num_printed;
        fprintf(stderr, "%ld: %n%s\n", line_number + 1, &num_printed, culprit_line.c_str());
        if (column >= 0) {
            fprintf(stderr, "%s\n", make_marker_from_str(culprit_line, num_printed + column).c_str());
        }
    }
}

static void print_culprit_line_with_context(long target_line, const char* file, long num_context_lines) {
    for (long curr_line = std::max(target_line - num_context_lines, 0l); curr_line <= target_line + num_context_lines; curr_line++) {
        std::string culprit_line = get_culprit_line(curr_line, file);
        if (!culprit_line.empty()) {
            int num_printed;
            fprintf(stderr, "%ld: %n%s\n", curr_line + 1, &num_printed, culprit_line.c_str());
            if (curr_line == target_line) {
                const unsigned long first_char_pos = culprit_line.find_first_not_of(" \t");
                fprintf(stderr, "%s\n", make_marker_from_str(culprit_line, num_printed + first_char_pos).c_str());
            }
        }
    }
}

void _log_message(odin_error error_type, long column, long line_number, long file, bool fatal_error, const char* function_file_name, long function_line, const char* function_name, const char* message, ...) {
    fflush(stdout);

    va_list ap;

    if (!fatal_error) {
        fprintf(stderr, "Warning::%s", odin_error_STR[error_type]);
    } else {
        fprintf(stderr, "Error::%s", odin_error_STR[error_type]);
    }

    if (file >= 0 && (size_t)file < include_file_names.size()) {
        std::string file_name = include_file_names[file].first;

        // print filename only
        auto slash_location = file_name.find_last_of('/');
        if (slash_location != std::string::npos) {
            file_name = file_name.substr(slash_location + 1);
        }

        fprintf(stderr, " %s:%ld", file_name.c_str(), line_number + 1);
    }

    if (message != NULL) {
        fprintf(stderr, " ");
        va_start(ap, message);
        vfprintf(stderr, message, ap);
        va_end(ap);

        if (message[strlen(message) - 1] != '\n')
            fprintf(stderr, "\n");
    }

    if (file >= 0 && (size_t)file < include_file_names.size()
        && line_number >= 0) {
        print_culprit_line(column, line_number, include_file_names[file].first.c_str());
    }

    fflush(stderr);
    _verbose_assert(!fatal_error, NULL, function_file_name, function_line, function_name);
}

void _verbose_assert(bool condition, const char* condition_str, const char* odin_file_name, long odin_line_number, const char* odin_function_name) {
    fflush(stdout);
    if (!condition) {
        fprintf(stderr, "%s:%ld: %s: ", odin_file_name, odin_line_number, odin_function_name);
        if (condition_str) {
            // We are an assertion, print the condition that failed and which line it occurred on
            fprintf(stderr, "Assertion %s failed\n", condition_str);
            // odin_line_number-1 since __LINE__ starts from 1
            print_culprit_line_with_context(odin_line_number - 1, odin_file_name, 2);
        } else {
            // We are a parsing error, dont print the culprit line
            fprintf(stderr, "Fatal error\n");
        }
        fflush(stderr);
        std::abort();
    }
}