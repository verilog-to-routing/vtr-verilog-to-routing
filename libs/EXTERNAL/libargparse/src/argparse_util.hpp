#ifndef ARGPARSE_UTIL_HPP
#define ARGPARSE_UTIL_HPP
#include <array>
#include <vector>
#include <map>
#include <memory>

namespace argparse {
    class Argument;

    //Splits off the leading dashes of a string, returning the dashes (index 0)
    //and the rest of the string (index 1)
    std::array<std::string,2> split_leading_dashes(std::string str);

    //Converts a string to upper case
    std::string toupper(std::string str);

    //Converts a string to lower case
    std::string tolower(std::string str);

    //Returns true if str represents a named argument starting with
    //'-' or '--' followed by one or more letters
    bool is_argument(std::string str, const std::map<std::string,std::shared_ptr<Argument>>& arg_map);

    //Returns true if str is in choices, or choices is empty
    bool is_valid_choice(std::string str, const std::vector<std::string>& choices);

    //Returns 'str' interpreted as type T
    // Throws an exception if conversion fails
    template<typename T> 
    T as(std::string str);

    template<typename Container>
    std::string join(Container container, std::string join_str);

    char* strdup(const char* str);

    std::vector<std::string> wrap_width(std::string str, size_t width, std::vector<std::string> split_str={" ", "/"});

    std::string basename(std::string filepath);
} //namespace

#include "argparse_util.tpp"

#endif
