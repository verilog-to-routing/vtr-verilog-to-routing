#pragma once

#include <algorithm>
#include <vector>
#include <string>
#include <string_view>
#include <cstdarg>
#include <array>

namespace vtr {

///@brief Returns 'input' with the first instance of 'search' replaced with 'replace'
std::string replace_first(std::string_view input, std::string_view search, std::string_view replace);

///@brief Returns 'input' with all instances of 'search' replaced with 'replace'
std::string replace_all(std::string_view input, std::string_view search, std::string_view replace);

///@brief Retruns true if str starts with prefix
bool starts_with(const std::string& str, std::string_view prefix);

///@brief Returns a std::string formatted using a printf-style format string
std::string string_fmt(const char* fmt, ...);

///@brief Returns a std::string formatted using a printf-style format string taking an explicit va_list
std::string vstring_fmt(const char* fmt, va_list args);

/**
 * @brief Joins a sequence by a specified delimeter
 *
 *  For example the sequence {"home", "user", "my_files", "test.blif"} with delim="/"
 *  would return "home/user/my_files/test.blif"
 */
template<typename Iter>
std::string join(Iter begin, Iter end, std::string_view delim);

template<typename Container>
std::string join(Container container, std::string_view delim);

template<typename T>
std::string join(std::initializer_list<T> list, std::string_view delim);

/**
 * @brief Checks if exactly `k` conditions are true.
 *
 * @tparam Conditions A variadic template parameter pack representing the types of the conditions,
 *         which should all be convertible to `bool`.
 * @param k The exact number of conditions that should evaluate to true.
 * @param conditions A variable number of boolean expressions or conditions to evaluate.
 * @return `true` if exactly `k` of the provided conditions are true; otherwise `false`.
 *
 * @example
 * @code
 * bool result = exactly_k_conditions(2, true, false, true); // Returns true
 * result = exactly_k_conditions(1, true, false, false);     // Returns true
 * result = exactly_k_conditions(3, true, true, false);      // Returns false
 * @endcode
 */
template<typename... Conditions>
bool exactly_k_conditions(int k, Conditions... conditions);

template<typename Container>
void uniquify(Container container);

constexpr size_t bufsize = 32768; /* Maximum line length for various parsing proc. */
char* strncpy(char* dest, const char* src, size_t size);
char* strdup(const char* str);
char* strtok(char* ptr, const char* tokens, FILE* fp, char* buf);
FILE* fopen(const char* fname, const char* flag);
int fclose(FILE* f);
char* fgets(char* buf, int max_size, FILE* fp);
char* getline(char*& _lineptr, FILE* _stream);

int atoi(const std::string& value);
unsigned atou(const std::string& value);
float atof(const std::string& value);
double atod(const std::string& value);

/**
 * @brief File utilities
 */
int get_file_line_number_of_last_opened_file();
bool file_exists(const char* filename);
bool check_file_name_extension(std::string_view file_name, std::string_view file_extension);

extern std::string out_file_prefix;

/**
 * @brief Legacy ReadLine Tokening
 */
std::vector<std::string> ReadLineTokens(FILE* InFile, int* LineNum);

/**
 * @brief Template join function implementation
 */
template<typename Iter>
std::string join(Iter begin, Iter end, std::string_view delim) {
    std::string joined_str;
    for (auto iter = begin; iter != end; ++iter) {
        joined_str += *iter;
        if (iter != end - 1) {
            joined_str += delim;
        }
    }
    return joined_str;
}

template<typename Container>
std::string join(Container container, std::string_view delim) {
    return join(std::begin(container), std::end(container), delim);
}

template<typename T>
std::string join(std::initializer_list<T> list, std::string_view delim) {
    return join(list.begin(), list.end(), delim);
}

/**
 * @brief Template uniquify function implementation
 *
 * Removes repeated elements in the container
 */
template<typename Container>
void uniquify(Container container) {
    std::sort(container.begin(), container.end());
    container.erase(std::unique(container.begin(), container.end()),
                    container.end());
}

template<typename... Conditions>
bool exactly_k_conditions(int k, Conditions... conditions) {
    bool conditionArray[] = {conditions...};
    int count = 0;
    for (bool condition : conditionArray) {
        if (condition) {
            count++;
        }
    }
    return count == k;
}

int get_pid();

/**
 * @brief A tokenizer for string objects. It splits a string 
 * with given delimiter and return a vector of tokens
 * It can accept different delimiters in splitting strings
 */
class StringToken {
 public: /* Constructors*/
  StringToken(const std::string& data);

  StringToken(std::string_view data);

  StringToken(const char* data);

 public: /* Public Accessors */
  std::string data() const;
  /**
   * @brief Splits the string 'text' along the specified delimiter characters in 'delims'
   *
   * The split strings (excluding the delimiters) are returned
   */
  std::vector<std::string> split(const std::string& delims) const;
  std::vector<std::string> split(const char& delim) const;
  std::vector<std::string> split(const char* delim) const;
  std::vector<std::string> split(const std::vector<char>& delim) const;
  std::vector<std::string> split();
  /** 
   * @brief Find the position (i-th charactor) in a string for a given
   * delimiter, it will return a list of positions For example, to find the
   * position of all quotes (") in a string: "we" are good The following code is
   * suggested: StringToken tokenizer("\"we\" are good"); std::vector<size_t>
   * anchors = tokenizer.find_positions('\"') The following vector will be
   * returned: [0, 3]
   */
  std::vector<size_t> find_positions(const char& delim) const;

  /** 
   * @brief split the string for each chunk. This is useful where there are
   * chunks of substring should not be splitted by the given delimiter For
   * example, to split the string with quotes (") in a string: source "cmdA
   * --opt1 val1;cmdB --opt2 val2" --verbose where the string between the two
   * quotes should not be splitted The following code is suggested: StringToken
   * tokenizer("source \"cmdA --opt1 val1;cmdB --opt2 val2\" --verbose");
   *   std::vector<std::string> tokenizer.split_by_chunks('\"', true);
   * The following vector will be returned:
   *   ["source" "cmdA --opt1 val1;cmdB --opt2 val2" "--verbose"]
   *
   * .. note:: The option ``split_odd_chunk`` is useful when the chunk delimiter
   * appears at the beginning of the string.
   */
  std::vector<std::string> split_by_chunks(
    const char& chunk_delim, const bool& split_odd_chunk = false) const;

 public: /* Public Mutators */
  void set_data(const std::string& data);
  void add_delim(const char& delim);
  void ltrim(const std::string& sensitive_word);
  void rtrim(const std::string& sensitive_word);
  void trim();

 private: /* Private Mutators */
  void add_default_delim();

 private:            /* Internal data */
  std::string data_; /* Lines to be splited */
  std::vector<char> delims_;
};

} // namespace vtr
