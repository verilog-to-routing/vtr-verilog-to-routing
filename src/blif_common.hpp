#ifndef BLIF_COMMON_HPP
#define BLIF_COMMON_HPP

#include "blifparse.hpp"

namespace blifparse {

/*
 * Function Declarations
 */

struct Names {
    std::vector<std::string> nets;
    std::vector<std::vector<LogicValue>> so_cover;
};

struct SubCkt {
    std::string model;
    std::vector<std::string> ports;
    std::vector<std::string> nets;
};

} //namespace
#endif
