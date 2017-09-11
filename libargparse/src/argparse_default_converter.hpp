#ifndef ARGPARSE_DEFAULT_CONVERTER_HPP
#define ARGPARSE_DEFAULT_CONVERTER_HPP
#include <sstream>
#include <vector>
#include <typeinfo>
#include "argparse_error.hpp"
#include "argparse_util.hpp"

namespace argparse {

/*
 * Get a useful description of the argument type
 */
//Signed Integer
template<typename T>
typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, std::string>::type
arg_type() { return "integer"; }

//Unsigned Integer
template<typename T>
typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, std::string>::type
arg_type() { return "non-negative integer"; }

//Float
template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, std::string>::type
arg_type() { return "float"; }

//Unkown
template<typename T>
typename std::enable_if<!std::is_floating_point<T>::value && !std::is_integral<T>::value, std::string>::type
arg_type() { return ""; } //Empty

/*
 * Default Conversions to/from strings
 */
template<typename T>
class DefaultConverter {
    public:
        T from_str(std::string str) {
            std::stringstream ss(str);

            T val = T();
            ss >> val;

            bool eof = ss.eof();
            bool fail = ss.fail();
            bool converted_ok = eof && !fail;
            if (!converted_ok) {
                std::stringstream msg;
                msg << "Invalid conversion from '" << str << "'";
                std::string arg_type_str = arg_type<T>();
                if (!arg_type_str.empty()) {
                    msg << " to " << arg_type_str;
                }
                throw ArgParseConversionError(msg.str());
            }

            return val;
        }

        std::string to_str(T val) {
            std::stringstream ss;
            ss << val;

            bool converted_ok = ss.eof() && !ss.fail();
            if (!converted_ok) {
                std::stringstream msg;
                msg << "Invalid conversion from '" << val << "' to string";
                throw ArgParseConversionError(msg.str());
            }
            return ss.str();
        }
        std::vector<std::string> default_choices() { return {}; }
};

//DefaultConverter specializations for bool
// By default std::stringstream doesn't accept "true" or "false"
// as boolean values.
template<>
class DefaultConverter<bool> {
    public:
        bool from_str(std::string str) {
            str = tolower(str);
            if      (str == "0" || str == "false") return false; 
            else if (str == "1" || str == "true")  return true;

            std::string msg = "Unexpected value '" + str + "' (expected one of: " + join(default_choices(), ", ") + ")";
            throw ArgParseConversionError(msg);
        }

        std::string to_str(bool val) {
            if (val) return "true";
            else     return "false";
        }

        std::vector<std::string> default_choices() {
            return {"true", "false"};
        }
};

//DefaultConverter specializations for std::string
// The default conversion checks for eof() which is not set for empty strings,
// nessesitating the specialization
template<>
class DefaultConverter<std::string> {
    public:
        std::string from_str(std::string str) { return str; }
        std::string to_str(std::string val) { return val; }
        std::vector<std::string> default_choices() { return {}; }
};

//DefaultConverter specializations for const char*
//  This allocates memory that the user is responsible for freeing
template<>
class DefaultConverter<const char*> {
    public:
        const char* from_str(std::string str) { 
            return strdup(str.c_str()); 
        }
        std::string to_str(const char* val) { return val; }
        std::vector<std::string> default_choices() { return {}; }
};

//DefaultConverter specializations for char*
//  This allocates memory that the user is responsible for freeing
template<>
class DefaultConverter<char*> {
    public:
        char* from_str(std::string str) { 
            return strdup(str.c_str()); 
        }
        std::string to_str(const char* val) { return val; }
        std::vector<std::string> default_choices() { return {}; }
};

} //namespace

#endif
