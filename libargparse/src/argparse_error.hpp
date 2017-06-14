#ifndef ARGPARSE_ERROR_HPP
#define ARGPARSE_ERROR_HPP
#include <stdexcept>
namespace argparse {

    class ArgParseError : public std::runtime_error {
        using std::runtime_error::runtime_error; //Constructors
    };

    class ArgParseConversionError : public ArgParseError {
        using ArgParseError::ArgParseError;
    };

    class ArgParseHelp {

    };

    class ArgParseVersion {

    };

}
#endif
