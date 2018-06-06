#ifndef ARGPARSE_DEFAULT_CONVERTER_HPP
#define ARGPARSE_DEFAULT_CONVERTER_HPP
#include <sstream>
#include <vector>
#include <typeinfo>
#include "argparse_error.hpp"
#include "argparse_util.hpp"
#include "argparse_value.hpp"

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
        ConvertedValue<T> from_str(std::string str) {
            std::stringstream ss(str);

            T val = T();
            ss >> val;

            bool eof = ss.eof();
            bool fail = ss.fail();
            bool converted_ok = eof && !fail;

            ConvertedValue<T> converted_value;
            if (!converted_ok) {
                std::stringstream msg;
                msg << "Invalid conversion from '" << str << "'";
                std::string arg_type_str = arg_type<T>();
                if (!arg_type_str.empty()) {
                    msg << " to " << arg_type_str;
                }
                converted_value.set_error(msg.str());
            } else {
                converted_value.set_value(val);

            }

            return converted_value;
        }

        ConvertedValue<std::string> to_str(T val) {
            std::stringstream ss;
            ss << val;

            bool converted_ok = ss.eof() && !ss.fail();

            ConvertedValue<std::string> converted_value;
            if (!converted_ok) {
                std::stringstream msg;
                msg << "Invalid conversion from '" << val << "' to string";
                converted_value.set_error(msg.str());
            } else {
                converted_value.set_value(ss.str());
            }
            return converted_value;
        }
        std::vector<std::string> default_choices() { return {}; }
};

//DefaultConverter specializations for bool
// By default std::stringstream doesn't accept "true" or "false"
// as boolean values.
template<>
class DefaultConverter<bool> {
    public:
        ConvertedValue<bool> from_str(std::string str) {
            ConvertedValue<bool> converted_value;

            str = tolower(str);
            if (str == "0" || str == "false") {
                converted_value.set_value(false); 
            } else if (str == "1" || str == "true") {
                converted_value.set_value(true); 
            } else {
                converted_value.set_error("Unexpected value '" + str + "' (expected one of: " + join(default_choices(), ", ") + ")");
            }
            return converted_value;
        }

        ConvertedValue<std::string> to_str(bool val) {
            ConvertedValue<std::string> converted_value;
            if (val) converted_value.set_value("true");
            else     converted_value.set_value("false");
            return converted_value;
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
        ConvertedValue<std::string> from_str(std::string str) { 
            ConvertedValue<std::string> converted_value;
            converted_value.set_value(str);
            return converted_value;
        }
        ConvertedValue<std::string> to_str(std::string val) {
            ConvertedValue<std::string> converted_value;
            converted_value.set_value(val);
            return converted_value;
        }
        std::vector<std::string> default_choices() { return {}; }
};

//DefaultConverter specializations for const char*
//  This allocates memory that the user is responsible for freeing
template<>
class DefaultConverter<const char*> {
    public:
        ConvertedValue<const char*> from_str(std::string str) { 
            ConvertedValue<const char*> val;
            val.set_value(strdup(str.c_str()));
            return val;
        }
        ConvertedValue<std::string> to_str(const char* val) {
            ConvertedValue<std::string> converted_value;
            converted_value.set_value(val);
            return converted_value;
        }
        std::vector<std::string> default_choices() { return {}; }
};

//DefaultConverter specializations for char*
//  This allocates memory that the user is responsible for freeing
template<>
class DefaultConverter<char*> {
    public:
        ConvertedValue<char*> from_str(std::string str) { 
            ConvertedValue<char*> val;
            val.set_value(strdup(str.c_str()));
            return val;
        }
        ConvertedValue<std::string> to_str(const char* val) {
            ConvertedValue<std::string> converted_value;
            converted_value.set_value(val);
            return converted_value;
        }
        std::vector<std::string> default_choices() { return {}; }
};
} //namespace

#endif
