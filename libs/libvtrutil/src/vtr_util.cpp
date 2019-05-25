#include <cstdarg>
#include <cstdlib>
#include <cerrno> //For errno
#include <cstring>
#include <memory>
#include <sstream>

#include "vtr_util.h"
#include "vtr_assert.h"
#include "vtr_memory.h"
#include "vtr_error.h"

namespace vtr {

std::string out_file_prefix;     /* used by fopen */
static int file_line_number = 0; /* file in line number being parsed (used by fgets) */
static int cont;                 /* line continued? (used by strtok)*/

//Splits the string 'text' along the specified delimiter characters in 'delims'
//The split strings (excluding the delimiters) are returned
std::vector<std::string> split(const char* text, const std::string delims) {
    if (text) {
        std::string text_str(text);
        return split(text_str, delims);
    }
    return std::vector<std::string>();
}

std::vector<std::string> split(const std::string& text, const std::string delims) {
    std::vector<std::string> tokens;

    std::string curr_tok;
    for (char c : text) {
        if (delims.find(c) != std::string::npos) {
            //Delimeter character
            if (!curr_tok.empty()) {
                //At the end of the token

                //Save it
                tokens.push_back(curr_tok);

                //Reset token
                curr_tok.clear();
            } else {
                //Pass
            }
        } else {
            //Non-delimeter append to token
            curr_tok += c;
        }
    }

    //Add last token
    if (!curr_tok.empty()) {
        //Save it
        tokens.push_back(curr_tok);
    }
    return tokens;
}

std::string replace_first(const std::string& input, const std::string& search, const std::string& replace) {
    auto pos = input.find(search);

    std::string output(input, 0, pos);
    output += replace;
    output += std::string(input, pos + search.size());

    return output;
}

std::string replace_all(const std::string& input, const std::string& search, const std::string& replace) {
    std::string output;

    size_t last = 0;
    size_t pos = input.find(search, last); //Find the first instance of 'search' starting at or after 'last'
    while (pos != std::string::npos) {
        output += input.substr(last, pos - last); //Append anything in the input string between last and current match
        output += replace;                        //Add the replacement

        last = pos + search.size(); //Advance past the current match

        pos = input.find(search, last); //Look for the next match
    }
    output += input.substr(last, pos - last); //Append anything in 'input' after the last match

    return output;
}

//Retruns true if str starts with prefix
bool starts_with(std::string str, std::string prefix) {
    return str.find(prefix) == 0;
}

//Returns a std::string formatted using a printf-style format string
std::string string_fmt(const char* fmt, ...) {
    // Make a variable argument list
    va_list va_args;

    // Initialize variable argument list
    va_start(va_args, fmt);

    //Format string
    std::string str = vstring_fmt(fmt, va_args);

    // Reset variable argument list
    va_end(va_args);

    return str;
}

//Returns a std::string formatted using a printf-style format string taking
//an explicit va_list
std::string vstring_fmt(const char* fmt, va_list args) {
    // We need to copy the args so we don't change them before the true formating
    va_list va_args_copy;
    va_copy(va_args_copy, args);

    //Determine the formatted length using a copy of the args
    int len = std::vsnprintf(nullptr, 0, fmt, va_args_copy);

    va_end(va_args_copy); //Clean-up

    //Negative if there is a problem with the format string
    VTR_ASSERT_MSG(len >= 0, "Problem decoding format string");

    size_t buf_size = len + 1; //For terminator

    //Allocate a buffer
    //  unique_ptr will free buffer automatically
    std::unique_ptr<char[]> buf(new char[buf_size]);

    //Format into the buffer using the original args
    len = std::vsnprintf(buf.get(), buf_size, fmt, args);

    VTR_ASSERT_MSG(len >= 0, "Problem decoding format string");
    VTR_ASSERT(static_cast<size_t>(len) == buf_size - 1);

    //Build the string from the buffer
    return std::string(buf.get(), len);
}

/* An alternate for strncpy since strncpy doesn't work as most
 * people would expect. This ensures null termination */
char* strncpy(char* dest, const char* src, size_t size) {
    /* Find string's length */
    size_t len = std::strlen(src);

    /* Cap length at (num - 1) to leave room for \0 */
    if (size <= len)
        len = (size - 1);

    /* Copy as much of string as we can fit */
    std::memcpy(dest, src, len);

    /* explicit null termination */
    dest[len] = '\0';

    return dest;
}

char* strdup(const char* str) {
    if (str == nullptr) {
        return nullptr;
    }

    size_t Len = std::strlen(str);
    //use calloc to already make the last char '\0'
    return (char*)std::memcpy(vtr::calloc(Len + 1, sizeof(char)), str, Len);
    ;
}

template<class T>
T atoT(const std::string& value, const std::string& type_name) {
    //The c version of atof doesn't catch errors.
    //
    //This version uses stringstream to detect conversion errors
    std::istringstream ss(value);

    T val;
    ss >> val;

    if (ss.fail() || !ss.eof()) {
        //Failed to convert, or did not consume all input
        std::stringstream msg;
        msg << "Failed to convert string '" << value << "' to " << type_name;
        throw VtrError(msg.str(), __FILE__, __LINE__);
    }

    return val;
}

int atoi(const std::string& value) {
    return atoT<int>(value, "int");
}

double atod(const std::string& value) {
    return atoT<double>(value, "double");
}

float atof(const std::string& value) {
    return atoT<float>(value, "float");
}

unsigned atou(const std::string& value) {
    return atoT<unsigned>(value, "unsigned int");
}

char* strtok(char* ptr, const char* tokens, FILE* fp, char* buf) {
    /* Get next token, and wrap to next line if \ at end of line.    *
     * There is a bit of a "gotcha" in strtok.  It does not make a   *
     * copy of the character array which you pass by pointer on the  *
     * first call.  Thus, you must make sure this array exists for   *
     * as long as you are using strtok to parse that line.  Don't    *
     * use local buffers in a bunch of subroutines calling each      *
     * other; the local buffer may be overwritten when the stack is  *
     * restored after return from the subroutine.                    */

    char* val;

    val = std::strtok(ptr, tokens);
    for (;;) {
        if (val != nullptr || cont == 0)
            return (val);

        /* return unless we have a null value and a continuation line */
        if (vtr::fgets(buf, bufsize, fp) == nullptr)
            return (nullptr);

        val = std::strtok(buf, tokens);
    }
}

FILE* fopen(const char* fname, const char* flag) {
    FILE* fp;
    size_t Len;
    char* new_fname = nullptr;
    file_line_number = 0;

    /* Appends a prefix string for output files */
    if (!out_file_prefix.empty()) {
        if (std::strchr(flag, 'w')) {
            Len = 1; /* NULL char */
            Len += std::strlen(out_file_prefix.c_str());
            Len += std::strlen(fname);
            new_fname = (char*)vtr::malloc(Len * sizeof(char));
            strcpy(new_fname, out_file_prefix.c_str());
            strcat(new_fname, fname);
            fname = new_fname;
        }
    }

    if (nullptr == (fp = std::fopen(fname, flag))) {
        throw VtrError(string_fmt("Error opening file %s for %s access: %s.\n", fname, flag, strerror(errno)), __FILE__, __LINE__);
    }

    if (new_fname)
        std::free(new_fname);

    return (fp);
}

int fclose(FILE* f) {
    return std::fclose(f);
}

char* fgets(char* buf, int max_size, FILE* fp) {
    /* Get an input line, update the line number and cut off *
     * any comment part.  A \ at the end of a line with no   *
     * comment part (#) means continue. vtr::fgets should give *
     * identical results for Windows (\r\n) and Linux (\n)   *
     * newlines, since it replaces each carriage return \r   *
     * by a newline character \n.  Returns NULL after EOF.     */

    int ch;
    int i;

    cont = 0;           /* line continued? */
    file_line_number++; /* global variable */

    for (i = 0; i < max_size - 1; i++) { /* Keep going until the line finishes or the buffer is full */

        ch = std::fgetc(fp);

        if (std::feof(fp)) { /* end of file */
            if (i == 0) {
                return nullptr; /* required so we can write while (vtr::fgets(...) != NULL) */
            } else {            /* no newline before end of file - last line must be returned */
                buf[i] = '\0';
                return buf;
            }
        }

        if (ch == '#') { /* comment */
            buf[i] = '\0';
            while ((ch = std::fgetc(fp)) != '\n' && !std::feof(fp))
                ; /* skip the rest of the line */
            return buf;
        }

        if (ch == '\r' || ch == '\n') {         /* newline (cross-platform) */
            if (i != 0 && buf[i - 1] == '\\') { /* if \ at end of line, line continued */
                cont = 1;
                buf[i - 1] = '\n'; /* May need this for tokens */
                buf[i] = '\0';
            } else {
                buf[i] = '\n';
                buf[i + 1] = '\0';
            }
            return buf;
        }

        buf[i] = ch; /* copy character into the buffer */
    }

    /* Buffer is full but line has not terminated, so error */
    throw VtrError(string_fmt("Error on line %d -- line is too long for input buffer.\n"
                              "All lines must be at most %d characters long.\n",
                              bufsize - 2),
                   __FILE__, __LINE__);
    return nullptr;
}

/*
 * Returns line number of last opened and read file
 */
int get_file_line_number_of_last_opened_file() {
    return file_line_number;
}

bool file_exists(const char* filename) {
    FILE* file;

    if (filename == nullptr) {
        return false;
    }

    file = std::fopen(filename, "r");
    if (file) {
        std::fclose(file);
        return true;
    }
    return false;
}

/* Date:July 17th, 2013
 * Author: Daniel Chen
 * Purpose: Checks the file extension of an file to ensure
 *            correct file format. Returns true if format is
 *            correct, and false otherwise.
 * Note:    This is probably a fragile check, but at least
 *            should prevent common problems such as swapping
 *            architecture file and blif file on the VPR
 *            command line.
 */

bool check_file_name_extension(const char* file_name,
                               const char* file_extension) {
    const char* str;
    int len_extension;

    len_extension = std::strlen(file_extension);
    str = std::strstr(file_name, file_extension);
    if (str == nullptr || (*(str + len_extension) != '\0')) {
        return false;
    }

    return true;
}

std::vector<std::string> ReadLineTokens(FILE* InFile, int* LineNum) {
    std::unique_ptr<char[]> buf(new char[vtr::bufsize]);

    const char* line = vtr::fgets(buf.get(), vtr::bufsize, InFile);

    ++(*LineNum);

    return vtr::split(line);
}

} // namespace vtr
