#include "odin_error.h"
#include "config_t.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

std::vector<std::pair<std::string, int>> include_file_names;

int delayed_errors = 0;

const char* odin_error_STR[] = {
    "NO_ERROR",
    "ARG_ERROR",
    "PARSE_ERROR",
    "NETLIST_ERROR",
    "BLIF_ERROR",
    "NETLIST_FILE_ERROR",
    "ACTIVATION_ERROR",
    "SIMULATION_ERROR",
    "ACE",
};

void verify_delayed_error() {
    if (delayed_errors) {
        error_message(PARSE_ERROR, -1, -1, "Parser found (%d) errors in your syntax, exiting", delayed_errors);
    }
}

static std::string make_marker_from_str(std::string str, int column) {
    str.erase(column);
    for (int i = 0; i < str.size(); i++) {
        if (str[i] != ' ' && str[i] != '\t') {
            str[i] = ' ';
        }
    }

    str += "^~~~";
    return str;
}

static void print_culprit_line(long column, long line_number, long file) {
    std::string culprit_line = "";
    if (file >= 0 && file < include_file_names.size()
        && line_number >= 0) {
        FILE* input_file = fopen(include_file_names[file].first.c_str(), "r");
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
        fprintf(stderr, "%s\n", culprit_line.c_str());
        if (column > 0) {
            delayed_errors++;
            fprintf(stderr, "%s\n", make_marker_from_str(culprit_line, column).c_str());
        }
    }
}

void _log_message(odin_error error_type, long column, long line_number, long file, bool soft_error, const char* function_file_name, long function_line, const char* function_name, const char* message, ...) {
    va_list ap;

    static long warning_count = 0;

    if (!warning_count) {
        fprintf(stderr, "--------------\nOdin has decided you ");
        if (soft_error)
            fprintf(stderr, "MAY fail ... :\n\n");
        else
            fprintf(stderr, "have failed ;)\n\n");
    }
    warning_count++;

    if (soft_error) {
        if (column == -1)
            fprintf(stderr, "WARNING (%ld)::", warning_count);
    } else
        fprintf(stderr, "ERROR (%ld)::", warning_count);

    fprintf(stderr, "%s", odin_error_STR[error_type]);

    if (file >= 0 && file < include_file_names.size())
        fprintf(stderr, " %s::%ld", include_file_names[file].first.c_str(), line_number + 1);

    if (message != NULL) {
        fprintf(stderr, " ");
        va_start(ap, message);
        vfprintf(stderr, message, ap);
        va_end(ap);

        if (message[strlen(message) - 1] != '\n')
            fprintf(stderr, "\n");
    }

    print_culprit_line(column, line_number, file);

    fflush(stderr);
    _verbose_assert(soft_error, "", function_file_name, function_line, function_name);
}