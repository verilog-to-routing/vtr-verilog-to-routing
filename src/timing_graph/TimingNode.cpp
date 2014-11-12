#include <iosfwd>
#include <sstream>
#include <stdexcept>
#include <string>

#include "TimingNode.hpp"

std::istream& operator>>(std::istream& is, TN_Type& type) {
    std::string tok;
    is >> tok; //Read in a token
    if(tok == "INPAD_SOURCE")               type = TN_Type::INPAD_SOURCE; 
    else if (tok == "INPAD_OPIN")           type = TN_Type::INPAD_OPIN; 
    else if (tok == "OUTPAD_IPIN")          type = TN_Type::OUTPAD_IPIN; 
    else if (tok == "OUTPAD_SINK")          type = TN_Type::OUTPAD_SINK; 
    else if (tok == "PRIMITIVE_IPIN")       type = TN_Type::PRIMITIVE_IPIN; 
    else if (tok == "PRIMITIVE_OPIN")       type = TN_Type::PRIMITIVE_OPIN; 
    else if (tok == "FF_IPIN")              type = TN_Type::FF_IPIN; 
    else if (tok == "FF_OPIN")              type = TN_Type::FF_OPIN; 
    else if (tok == "FF_SINK")              type = TN_Type::FF_SINK; 
    else if (tok == "FF_SOURCE")            type = TN_Type::FF_SOURCE; 
    else if (tok == "FF_CLOCK")             type = TN_Type::FF_CLOCK; 
    else if (tok == "CLOCK_SOURCE")         type = TN_Type::CLOCK_SOURCE; 
    else if (tok == "CLOCK_OPIN")           type = TN_Type::CLOCK_OPIN; 
    else if (tok == "CONSTANT_GEN_SOURCE")  type = TN_Type::CONSTANT_GEN_SOURCE;
    else if (tok == "UNKOWN")               type = TN_Type::UNKOWN;
    else throw std::domain_error(std::string("Unrecognized TN_Type: ") + tok);
    return is;
}
std::ostream& operator<<(std::ostream& os, const TN_Type type) {
    if(type == TN_Type::INPAD_SOURCE)              os << "INPAD_SOURCE"; 
    else if (type == TN_Type::INPAD_OPIN)          os << "INPAD_OPIN"; 
    else if (type == TN_Type::OUTPAD_IPIN)         os << "OUTPAD_IPIN"; 
    else if (type == TN_Type::OUTPAD_SINK)         os << "OUTPAD_SINK"; 
    else if (type == TN_Type::PRIMITIVE_IPIN)      os << "PRIMITIVE_IPIN"; 
    else if (type == TN_Type::PRIMITIVE_OPIN)      os << "PRIMITIVE_OPIN"; 
    else if (type == TN_Type::FF_IPIN)             os << "FF_IPIN"; 
    else if (type == TN_Type::FF_OPIN)             os << "FF_OPIN"; 
    else if (type == TN_Type::FF_SINK)             os << "FF_SINK"; 
    else if (type == TN_Type::FF_SOURCE)           os << "FF_SOURCE"; 
    else if (type == TN_Type::FF_CLOCK)            os << "FF_CLOCK"; 
    else if (type == TN_Type::CLOCK_SOURCE)        os << "CLOCK_SOURCE"; 
    else if (type == TN_Type::CLOCK_OPIN)          os << "CLOCK_OPIN";
    else if (type == TN_Type::CONSTANT_GEN_SOURCE) os << "CONSTANT_GEN_SOURCE";
    else if (type == TN_Type::UNKOWN)              os << "UNKOWN";
    else throw std::domain_error("Unrecognized TN_Type");
    return os;
}

