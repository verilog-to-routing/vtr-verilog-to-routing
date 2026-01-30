#include <cstdarg>
#include <cstdlib>
#include <cerrno> //For errno
#include <cstring>
#include <filesystem>
#include <memory>
#include <sstream>

#include "vtr_util.h"
#include "vtr_assert.h"
#include "vtr_memory.h"
#include "vtr_error.h"

#if defined(__unix__)
#include <unistd.h> //For getpid()
#endif

namespace vtr {

/// @brief used by fopen
std::string out_file_prefix;

/// @brief file in line number being parsed (used by fgets)
static int file_line_number = 0;

/// @brief line continued? (used by strtok)
static int cont;

std::string replace_first(std::string_view input, std::string_view search, std::string_view replace) {
    auto pos = input.find(search);

    std::string output(input, 0, pos);
    output += replace;
    output += input.substr(pos + search.size());

    return output;
}

std::string replace_all(std::string_view input, std::string_view search, std::string_view replace) {
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

bool starts_with(const std::string& str, std::string_view prefix) {
    return str.find(prefix) == 0;
}

std::string string_fmt(const char* fmt, ...) {
    // Make a variable argument list
    va_list va_args;

    // Initialize variable argument list
    va_start(va_args, fmt);

    // Format string
    std::string str = vstring_fmt(fmt, va_args);

    // Reset variable argument list
    va_end(va_args);

    return str;
}

std::string vstring_fmt(const char* fmt, va_list args) {
    // We need to copy the args so we don't change them before the true formatting
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

char* strncpy(char* dest, const char* src, size_t size) {
    // Find string's length
    size_t len = std::strlen(src);

    // Cap length at (num - 1) to leave room for \0
    if (size <= len)
        len = (size - 1);

    // Copy as much of string as we can fit
    std::memcpy(dest, src, len);

    // explicit null termination
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

/**
 * @brief Legacy c-style function replacements.
 *
 * Typically these add extra error checking
 * and/or correct 'unexpected' behaviour of the standard c-functions
 */
template<class T>
T atoT(std::string_view value, std::string_view type_name) {
    //The c version of atof doesn't catch errors.
    //
    //This version uses stringstream to detect conversion errors
    std::istringstream ss(value.data());

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

int atoi(std::string_view value) {
    return atoT<int>(value, "int");
}

double atod(std::string_view value) {
    return atoT<double>(value, "double");
}

float atof(std::string_view value) {
    return atoT<float>(value, "float");
}

unsigned atou(std::string_view value) {
    return atoT<unsigned>(value, "unsigned int");
}

char* strtok(char* ptr, const char* tokens, FILE* fp, char* buf) {
    char* val;

    val = std::strtok(ptr, tokens);
    for (;;) {
        if (val != nullptr || cont == 0)
            return (val);

        // return unless we have a null value and a continuation line
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

    // Appends a prefix string for output files
    if (!out_file_prefix.empty()) {
        if (std::strchr(flag, 'w')) {
            Len = 1; // NULL char
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
    int ch;
    int i;

    cont = 0;           // line continued?
    file_line_number++; // global variable

    for (i = 0; i < max_size - 1; i++) { // Keep going until the line finishes or the buffer is full

        ch = std::fgetc(fp);

        if (std::feof(fp)) { // end of file
            if (i == 0) {
                return nullptr; // required so we can write while (vtr::fgets(...) != NULL)
            } else {            // no newline before end of file - last line must be returned
                buf[i] = '\0';
                return buf;
            }
        }

        if (ch == '#') { // comment
            buf[i] = '\0';
            while ((ch = std::fgetc(fp)) != '\n' && !std::feof(fp))
                ; // skip the rest of the line
            return buf;
        }

        if (ch == '\r' || ch == '\n') {         // newline (cross-platform)
            if (i != 0 && buf[i - 1] == '\\') { // if \ at end of line, line continued
                cont = 1;
                buf[i - 1] = '\n'; // May need this for tokens
                buf[i] = '\0';
            } else {
                buf[i] = '\n';
                buf[i + 1] = '\0';
            }
            return buf;
        }

        buf[i] = ch; // copy character into the buffer
    }

    // Buffer is full but line has not terminated, so error
    throw VtrError(string_fmt("Error on line %d -- line is too long for input buffer.\n"
                              "All lines must be at most %d characters long.\n",
                              bufsize - 2),
                   __FILE__, __LINE__);
    return nullptr;
}

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

bool check_file_name_extension(std::string_view file_name,
                               std::string_view file_extension) {
    auto ext = std::filesystem::path(file_name).extension();
    return ext == file_extension;
}

std::vector<std::string> ReadLineTokens(FILE* InFile, int* LineNum) {
    std::unique_ptr<char[]> buf(new char[vtr::bufsize]);

    const char* line = vtr::fgets(buf.get(), vtr::bufsize, InFile);

    ++(*LineNum);

    return vtr::StringToken(line).split(" \t\n");
}

int get_pid() {
#if defined(__unix__)
    return getpid();
#else
    return -1;
#endif
}

/************************************************************************
 * Constructors
 ***********************************************************************/
StringToken::StringToken(const std::string& data) {
    set_data(data);
}

StringToken::StringToken(std::string_view data) {
    set_data(std::string(data));
}

StringToken::StringToken(const char* data) {
    set_data(std::string(data));
}

/************************************************************************
 * Public Accessors
 ***********************************************************************/
std::string StringToken::data() const {
    return data_;
}

std::vector<std::string> StringToken::split(const std::string& delims) const {
    // Return vector
    std::vector<std::string> ret;

    // Get a writable char array
    char* tmp = new char[data_.size() + 1];
    std::copy(data_.begin(), data_.end(), tmp);
    tmp[data_.size()] = '\0';
    // Split using strtok
    char* result = std::strtok(tmp, delims.c_str());
    while (nullptr != result) {
        std::string result_str(result);
        // Store the token
        ret.push_back(result_str);
        // Got to next
        result = std::strtok(nullptr, delims.c_str());
    }

    // Free the tmp
    delete[] tmp;

    return ret;
}

std::vector<std::string> StringToken::split(const char& delim) const {
    // Create delims
    std::string delims(1, delim);

    return split(delims);
}

std::vector<std::string> StringToken::split(const char* delim) const {
    // Create delims
    std::string delims(delim);

    return split(delims);
}

std::vector<std::string> StringToken::split(
    const std::vector<char>& delims) const {
    // Create delims
    std::string delims_str;
    for (const auto& delim : delims) {
        delims_str.push_back(delim);
    }

    return split(delims_str);
}

std::vector<std::string> StringToken::split() {
    // Add a default delim
    if (true == delims_.empty()) {
        add_default_delim();
    }
    // Create delims
    std::string delims;
    for (const auto& delim : delims_) {
        delims.push_back(delim);
    }

    return split(delims);
}

std::vector<size_t> StringToken::find_positions(const char& delim) const {
    std::vector<size_t> anchors;
    size_t found = data_.find(delim);
    while (std::string::npos != found) {
        anchors.push_back(found);
        found = data_.find(delim, found + 1);
    }
    return anchors;
}

std::vector<std::string> StringToken::split_by_chunks(
    const char& chunk_delim,
    const bool& split_odd_chunk) const {
    size_t chunk_idx_mod = 0;
    if (split_odd_chunk) {
        chunk_idx_mod = 1;
    }
    std::vector<std::string> tokens;
    // There are pairs of quotes, identify the chunk which should be split
    std::vector<std::string> token_chunks = split(chunk_delim);
    for (size_t ichunk = 0; ichunk < token_chunks.size(); ichunk++) {
        // Chunk with even index (including the first) is always out of two quote ->
        // Split! Chunk with odd index is always between two quotes -> Do not split!
        if (ichunk % 2 == chunk_idx_mod) {
            StringToken chunk_tokenizer(token_chunks[ichunk]);
            for (std::string curr_token : chunk_tokenizer.split()) {
                tokens.push_back(curr_token);
            }
        } else {
            tokens.push_back(token_chunks[ichunk]);
        }
    }
    return tokens;
}

/************************************************************************
 * Public Mutators
 ***********************************************************************/
void StringToken::set_data(const std::string& data) {
    data_ = data;
}

void StringToken::add_delim(const char& delim) {
    delims_.push_back(delim);
}

void StringToken::ltrim(const std::string& sensitive_word) {
    size_t start = data_.find_first_not_of(sensitive_word);
    data_ = (start == std::string::npos) ? "" : data_.substr(start);
}

void StringToken::rtrim(const std::string& sensitive_word) {
    size_t end = data_.find_last_not_of(sensitive_word);
    data_ = (end == std::string::npos) ? "" : data_.substr(0, end + 1);
}

void StringToken::trim() {
    rtrim(" ");
    ltrim(" ");
}

/************************************************************************
 * Internal Mutators
 ***********************************************************************/
void StringToken::add_default_delim() {
    VTR_ASSERT_SAFE(true == delims_.empty());
    delims_.push_back(' ');
}

} // namespace vtr
