#include "odin_error.h"
#include "config_t.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

std::vector<std::pair<std::string, int>> include_file_names;

#define NUMBER_OF_LINES_DIGIT 5
int delayed_errors = 0;
const loc_t unknown_location = {-1, -1, -1};

const char* odin_error_STR[] = {
    "",
    "UTIL",
    "PARSE_ARGS",
    "PARSE_TO_AST",
    "AST",
    "BLIF ELABORATION",
    "NETLIST",
    "PARSE_BLIF",
    "OUTPUT_BLIF",
    "SIMULATION",
};

void verify_delayed_error(odin_error error_type) {
    if (delayed_errors) {
        error_message(error_type, unknown_location, "Parser found (%d) errors in your syntax, exiting", delayed_errors);
    }
}

static std::string make_marker_from_str(std::string str, int column) {
    std::string to_return = "";

    for (size_t i = 0; i < str.size(); i++) {
        if (column >= 0 && i >= (size_t)column) {
            break;
        } else if (str[i] == ' ' || str[i] == '\t') {
            to_return += str[i];
        } else if (column <= 0) {
            break;
        } else {
            to_return += ' ';
        }
    }

    to_return += "^~~~";
    return to_return;
}

static std::string get_culprit_line(int line_number, const char* file) {
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

static void print_culprit_line_with_context(int column, int target_line, const char* file, int num_context_lines) {
    for (int curr_line = std::max(target_line - num_context_lines, 0); curr_line <= target_line + num_context_lines; curr_line++) {
        std::string culprit_line = get_culprit_line(curr_line, file);
        int num_printed;
        fprintf(stderr, " %*d | %n%s\n", NUMBER_OF_LINES_DIGIT, curr_line + 1, &num_printed, culprit_line.c_str());
        if (curr_line == target_line) {
            fprintf(stderr, "%s\n", make_marker_from_str(culprit_line, num_printed + column).c_str());
        }
    }
}

void _log_message(odin_error error_type, loc_t loc, bool fatal_error, const char* function_file_name, int function_line, const char* function_name, const char* message, ...) {
    fflush(stdout);

    va_list ap;

    if (loc.file >= 0 && (size_t)loc.file < include_file_names.size()) {
        char* path = realpath(include_file_names[loc.file].first.c_str(), NULL);
        fprintf(stderr, "\n%s:%d:%d: ", path, loc.line + 1, loc.col);
        free(path);
    }

    if (!fatal_error) {
        fprintf(stderr, "warning");
    } else {
        fprintf(stderr, "error");
    }

    fprintf(stderr, "[%s]: ", odin_error_STR[error_type]);

    if (message != NULL) {
        fprintf(stderr, " ");
        va_start(ap, message);
        vfprintf(stderr, message, ap);
        va_end(ap);

        if (message[strlen(message) - 1] != '\n')
            fprintf(stderr, "\n");
    }

    if (loc.file >= 0 && (size_t)loc.file < include_file_names.size()
        && loc.line >= 0) {
        print_culprit_line_with_context(loc.col, loc.line, include_file_names[loc.file].first.c_str(), 2);
    }

    fflush(stderr);
    if (fatal_error) {
        _verbose_abort(NULL, function_file_name, function_line, function_name);
    }
}

void _verbose_abort(const char* condition_str, const char* odin_file_name, int odin_line_number, const char* odin_function_name) {
    fflush(stdout);
    fprintf(stderr, "\n%s:%d: %s: ", odin_file_name, odin_line_number, odin_function_name);
    if (condition_str) {
        // We are an assertion, print the condition that failed and which line it occurred on
        fprintf(stderr, "Assertion %s failed\n", condition_str);
        // odin_line_number-1 since __LINE__ starts from 1
        print_culprit_line_with_context(-1, odin_line_number - 1, odin_file_name, 2);
    } else {
        // We are a parsing error, dont print the culprit line
        fprintf(stderr, "Fatal error\n");
    }
    fflush(stderr);
    std::abort();
}
