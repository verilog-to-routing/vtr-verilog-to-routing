#include "logic_vec.h"
#include "vtr_assert.h"

//Output operator for vtr::LogicValue
std::ostream& operator<<(std::ostream& os, vtr::LogicValue val) {
    if (val == vtr::LogicValue::FALSE)
        os << "0";
    else if (val == vtr::LogicValue::TRUE)
        os << "1";
    else if (val == vtr::LogicValue::DONT_CARE)
        os << "-";
    else if (val == vtr::LogicValue::UNKOWN)
        os << "x";
    else
        VTR_ASSERT(false);
    return os;
}
