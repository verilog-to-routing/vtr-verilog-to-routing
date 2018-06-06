#ifndef ARGPARSE_VALUE_HPP
#define ARGPARSE_VALUE_HPP
#include <iostream>
#include "argparse_error.hpp"

namespace argparse {

    template<class T>
    class ConvertedValue {
        public:
            typedef T value_type;
        public:
            void set_value(T val) { errored_ = false; value_ = val; }
            void set_error(std::string msg) { errored_ = true; error_msg_ = msg; }

            T value() const { return value_; }
            std::string error() const { return error_msg_; }

            operator bool() { return valid(); }
            bool valid() const { return !errored_; }
        private:
            T value_;
            std::string error_msg_;
            bool errored_ = true;
    };

    //How the value associated with an argumetn was initialized
    enum class Provenance {
        UNSPECIFIED,//The value was default constructed
        DEFAULT,    //The value was set by a default (e.g. as a command-line argument default value)
        SPECIFIED,  //The value was explicitly specified (e.g. explicitly specified on the command-line)
        INFERRED,   //The value was inferred, or conditionally set based on other values
    };

    /*
     * ArgValue represents the 'value' of a command-line option/argument
     *
     * It supports implicit conversion to the underlying value_type, which means it can
     * be seamlessly used as the value_type in most situations.
     *
     * It additionally tracks the provenance off the option, along with it's associated argument group.
     */
    template<typename T>
    class ArgValue {
        public:
            typedef T value_type;

        public: //Accessors
            //Automatic conversion to underlying value type
            operator T() const { return value_; }

            //Returns the value assoicated with this argument
            const T& value() const { return value_; }

            //Returns the provenance of this argument (i.e. how it was initialized)
            Provenance provenance() const { return provenance_; }

            //Returns the group this argument is associated with (or an empty string if none)
            const std::string& argument_group() const { return argument_group_; }

            const std::string& argument_name() const { return argument_name_; }

        public: //Mutators
            void set(ConvertedValue<T> val, Provenance prov) {
                if (!val.valid()) {
                    //If the value didn't convert properly, it should
                    //have an error message so raise it
                    throw ArgParseConversionError(val.error());
                }
                value_ = val.value();
                provenance_ = prov;
            }

            void set(T val, Provenance prov) {
                value_ = val;
                provenance_ = prov;
            }

            T& mutable_value(Provenance prov) {
                provenance_ = prov;
                return value_;
            }

            void set_argument_group(std::string grp) {
                argument_group_ = grp;
            }

            void set_argument_name(std::string name_str) {
                argument_name_ = name_str;
            }
        private:
            T value_ = T();
            Provenance provenance_ = Provenance::UNSPECIFIED;
            std::string argument_group_ = "";
            std::string argument_name_ = "";
    };

    //Automatically convert to the underlying type for ostream output
    template<typename T>
    std::ostream& operator<<(std::ostream& os, const ArgValue<T> t) {
        return os << T(t);
    }

}
#endif
