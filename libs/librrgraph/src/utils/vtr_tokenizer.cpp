/************************************************************************
 * Member functions for StringToken class
 ***********************************************************************/
#include <cstring>

/* Headers from vtrutil library */
#include "vtr_tokenizer.h"
#include "vtr_assert.h"

/* namespace openfpga begins */
namespace vtr {

/************************************************************************
 * Constructors
 ***********************************************************************/
StringToken::StringToken(const std::string& data) { set_data(data); }

/************************************************************************
 * Public Accessors
 ***********************************************************************/
/* Get the data string */
std::string StringToken::data() const { return data_; }

/* Split the string using a given delim */
std::vector<std::string> StringToken::split(const std::string& delims) const {
  /* Return vector */
  std::vector<std::string> ret;

  /* Get a writable char array */
  char* tmp = new char[data_.size() + 1];
  std::copy(data_.begin(), data_.end(), tmp);
  tmp[data_.size()] = '\0';
  /* Split using strtok */
  char* result = std::strtok(tmp, delims.c_str());
  while (NULL != result) {
    std::string result_str(result);
    /* Store the token */
    ret.push_back(result_str);
    /* Got to next */
    result = std::strtok(NULL, delims.c_str());
  }

  /* Free the tmp */
  delete[] tmp;

  return ret;
}

/* Split the string using a given delim */
std::vector<std::string> StringToken::split(const char& delim) const {
  /* Create delims */
  std::string delims(1, delim);

  return split(delims);
}

/* Split the string using a given delim */
std::vector<std::string> StringToken::split(const char* delim) const {
  /* Create delims */
  std::string delims(delim);

  return split(delims);
}

/* Split the string using a given delim */
std::vector<std::string> StringToken::split(
  const std::vector<char>& delims) const {
  /* Create delims */
  std::string delims_str;
  for (const auto& delim : delims) {
    delims_str.push_back(delim);
  }

  return split(delims_str);
}

/* Split the string */
std::vector<std::string> StringToken::split() {
  /* Add a default delim */
  if (true == delims_.empty()) {
    add_default_delim();
  }
  /* Create delims */
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
  const char& chunk_delim, const bool& split_odd_chunk) const {
  size_t chunk_idx_mod = 0;
  if (split_odd_chunk) {
    chunk_idx_mod = 1;
  }
  std::vector<std::string> tokens;
  /* There are pairs of quotes, identify the chunk which should be split*/
  std::vector<std::string> token_chunks = split(chunk_delim);
  for (size_t ichunk = 0; ichunk < token_chunks.size(); ichunk++) {
    /* Chunk with even index (including the first) is always out of two quote ->
     * Split! Chunk with odd index is always between two quotes -> Do not split!
     */
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
  return;
}

/* Add a delima to the list */
void StringToken::add_delim(const char& delim) { delims_.push_back(delim); }

/* Remove the string repeated at the beginning of string */
void StringToken::ltrim(const std::string& sensitive_word) {
  size_t start = data_.find_first_not_of(sensitive_word);
  data_ = (start == std::string::npos) ? "" : data_.substr(start);
  return;
}

/* Remove the string repeated at the end of string */
void StringToken::rtrim(const std::string& sensitive_word) {
  size_t end = data_.find_last_not_of(sensitive_word);
  data_ = (end == std::string::npos) ? "" : data_.substr(0, end + 1);
  return;
}

void StringToken::trim() {
  rtrim(" ");
  ltrim(" ");
  return;
}

/************************************************************************
 * Internal Mutators
 ***********************************************************************/
void StringToken::add_default_delim() {
  VTR_ASSERT_SAFE(true == delims_.empty());
  delims_.push_back(' ');
  return;
}

}  // namespace openfpga
