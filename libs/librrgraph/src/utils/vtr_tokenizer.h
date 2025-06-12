#ifndef VTR_TOKENIZER_H
#define VTR_TOKENIZER_H

/********************************************************************
 * Include header files that are required by data structure declaration
 *******************************************************************/
#include <string>
#include <vector>

/* namespace openfpga begins */
namespace vtr {

/************************************************************************
 * This file includes a tokenizer for string objects
 * It splits a string with given delima and return a vector of tokens
 * It can accept different delima in splitting strings
 ***********************************************************************/

class StringToken {
 public: /* Constructors*/
  StringToken(const std::string& data);

 public: /* Public Accessors */
  std::string data() const;
  std::vector<std::string> split(const std::string& delims) const;
  std::vector<std::string> split(const char& delim) const;
  std::vector<std::string> split(const char* delim) const;
  std::vector<std::string> split(const std::vector<char>& delim) const;
  std::vector<std::string> split();
  /** @brief Find the position (i-th charactor) in a string for a given
   * delimiter, it will return a list of positions For example, to find the
   * position of all quotes (") in a string: "we" are good The following code is
   * suggested: StringToken tokenizer("\"we\" are good"); std::vector<size_t>
   * anchors = tokenizer.find_positions('\"') The following vector will be
   * returned: [0, 3] */
  std::vector<size_t> find_positions(const char& delim) const;
  /** @brief split the string for each chunk. This is useful where there are
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

}  // namespace openfpga

#endif
