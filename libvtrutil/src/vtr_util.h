#ifndef VTR_UTIL_H
#define VTR_UTIL_H

#include <vector>
#include <string>

namespace vtr {

    //Splits the string 'text' along the specified delimiter characters in 'delims'
    //The split strings (excluding the delimiters) are returned
    std::vector<std::string> split(const std::string& text, const std::string delims=" \t\n");

    //Returns a std::string formatted using a printf-style format string
    std::string string_fmt(const char* fmt, ...);

    //Returns a std::string formatted using a printf-style format string taking
    //an explicit va_list
    std::string vstring_fmt(const char* fmt, va_list args);

    /*
     * Legacy c-style function replacements, typically these add extra error checking
     * and/or correct 'unexpected' behaviour of the standard c-functions
     */
    constexpr size_t BUFSIZE = 32768; /* Maximum line length for various parsing proc. */
    char* strncpy(char *dest, const char *src, size_t size);
    char* strdup(const char *str);
    int atoi(const char *str);
    char* strtok(char *ptr, const char *tokens, FILE * fp, char *buf);
    FILE* fopen(const char *fname, const char *flag);
    char* fgets(char *buf, int max_size, FILE * fp);

    /*
     * File utilities
     */
    int get_file_line_number_of_last_opened_file();
    bool file_exists(const char * filename);
    bool check_file_name_extension(const char* file_name, 
                                   const char* file_extension);

    extern char *out_file_prefix;
}

#endif
