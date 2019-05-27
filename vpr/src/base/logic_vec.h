#ifndef LOGIC_VEC_H
#define LOGIC_VEC_H

#include <vector>
#include <ostream>

#include "vtr_logic.h"

std::ostream& operator<<(std::ostream& os, vtr::LogicValue val);

//A vector-like object containing logic values.
class LogicVec {
  public:
    LogicVec() = default;
    LogicVec(size_t size_val,            //Number of logic values
             vtr::LogicValue init_value) //Default value
        : values_(size_val, init_value) {}
    LogicVec(std::vector<vtr::LogicValue> values)
        : values_(values) {}

    //Array indexing operator
    const vtr::LogicValue& operator[](size_t i) const { return values_[i]; }
    vtr::LogicValue& operator[](size_t i) { return values_[i]; }

    // Size accessor
    size_t size() const { return values_.size(); }

    //Output operator which writes the logic vector in verilog format
    friend std::ostream& operator<<(std::ostream& os, LogicVec logic_vec) {
        os << logic_vec.values_.size() << "'b";
        //Print in reverse since th convention is MSB on the left, LSB on the right
        //but we store things in array order (putting LSB on left, MSB on right)
        for (auto iter = logic_vec.begin(); iter != logic_vec.end(); iter++) {
            os << *iter;
        }
        return os;
    }

    //Standard iterators
    std::vector<vtr::LogicValue>::reverse_iterator begin() { return values_.rbegin(); }
    std::vector<vtr::LogicValue>::reverse_iterator end() { return values_.rend(); }
    std::vector<vtr::LogicValue>::const_reverse_iterator begin() const { return values_.crbegin(); }
    std::vector<vtr::LogicValue>::const_reverse_iterator end() const { return values_.crend(); }

  private:
    std::vector<vtr::LogicValue> values_; //The logic values
};

#endif
