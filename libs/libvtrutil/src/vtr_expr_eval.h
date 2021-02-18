#ifndef EXPR_EVAL_H
#define EXPR_EVAL_H
#include <map>
#include <string>
#include <vector>
#include <stack>
#include <cstring>
#include <iostream>

#include "vtr_util.h"
#include "vtr_error.h"
#include "vtr_string_view.h"
#include "vtr_flat_map.h"
#include "../../../vpr/src/draw/breakpoint_state_globals.h"

/**
 * @file
 * @brief   This file implements an expressopn evaluator
 *
 * The expression evaluator is capable of performing many operations on given variables, 
 * after parsing the expression. The parser goes character by character and identifies 
 * the type of char or chars. (e.g bracket, comma, number, operator, variable). 
 * The supported operations include addition, subtraction, multiplication, division, 
 * finding max, min, gcd, lcm, as well as boolean operators such as &&, ||, ==, >=, <= etc. 
 * The result is returned as an int value and operation precedance is taken into account. 
 * (e.g given 3-2*4, the result will be -5). This class is also used to parse expressions 
 * indicating breakpoints. The breakpoint expressions consist of variable names such as 
 * move_num, temp_num, from_block etc, and boolean operators (e.g move_num == 3). 
 * Multiple breakpoints can be expressed in one expression
 */

//function declarations
///@brief returns the global variable that holds all values that can trigger a breakpoint and are updated by the router and placer
BreakpointStateGlobals* get_bp_state_globals();

namespace vtr {

/**** Structs ****/

///@brief a class to hold the formula data
class t_formula_data {
  public:
    ///@brief clears all the formula data
    void clear() {
        vars_.clear();
    }

    ///@brief set the value of a specific part of the formula
    void set_var_value(vtr::string_view var, int value) { vars_[var] = value; }

    ///@brief set the value of a specific part of the formula (the var can be c-style string)
    void set_var_value(const char* var, int value) { vars_[vtr::string_view(var)] = value; }

    ///@brief get the value of a specific part of the formula
    int get_var_value(const std::string& var) const {
        return get_var_value(vtr::string_view(var.data(), var.size()));
    }

    ///@brief get the value of a specific part of the formula (the var can be c-style string)
    int get_var_value(vtr::string_view var) const {
        auto iter = vars_.find(var);
        if (iter == vars_.end()) {
            std::string copy(var.data(), var.size());
            throw vtr::VtrError(vtr::string_fmt("No value found for variable '%s' from expression\n", copy.c_str()), __FILE__, __LINE__);
        }

        return iter->second;
    }

  private:
    vtr::flat_map<vtr::string_view, int> vars_;
};

/**** Enums ****/
///@brief Used to identify the type of symbolic formula object
typedef enum e_formula_obj {
    E_FML_UNDEFINED = 0,
    E_FML_NUMBER,
    E_FML_BRACKET,
    E_FML_COMMA,
    E_FML_OPERATOR,
    E_FML_VARIABLE,
    E_FML_NUM_FORMULA_OBJS
} t_formula_obj;

///@brief Used to identify an operator in a formula
typedef enum e_operator {
    E_OP_UNDEFINED = 0,
    E_OP_ADD,
    E_OP_SUB,
    E_OP_MULT,
    E_OP_DIV,
    E_OP_MIN,
    E_OP_MAX,
    E_OP_GCD,
    E_OP_LCM,
    E_OP_AND,
    E_OP_OR,
    E_OP_GT,
    E_OP_LT,
    E_OP_GTE,
    E_OP_LTE,
    E_OP_EQ,
    E_OP_MOD,
    E_OP_AA,
    E_OP_NUM_OPS
} t_operator;

///@brief Used to identify operators with more than one character
typedef enum e_compound_operator {
    E_COM_OP_UNDEFINED = 0,
    E_COM_OP_AND,
    E_COM_OP_OR,
    E_COM_OP_EQ,
    E_COM_OP_AA,
    E_COM_OP_GTE,
    E_COM_OP_LTE

} t_compound_operator;

/**** Class Definitions ****/
/** 
 * @brief A class represents an object in a formula
 *
 * This object can be any of the following:
 *      - a number
 *      - a bracket
 *      - an operator
 *      - a variable
 */
class Formula_Object {
  public:
    ///@brief indicates the type of formula object this is
    t_formula_obj type;

    /**
     * @brief object data, accessed based on what kind of object this is 
     */
    union u_Data {
        int num;           ///< for number objects
        t_operator op;     ///< for operator objects
        bool left_bracket; ///< for bracket objects -- specifies if this is a left bracket
        //std::string variable;

        u_Data() { memset(this, 0, sizeof(u_Data)); }
    } data;

    ///@brief constructor
    Formula_Object() {
        this->type = E_FML_UNDEFINED;
    }

    ///@brief convert enum to string
    std::string to_string() const {
        if (type == E_FML_NUMBER || type == E_FML_VARIABLE) {
            return std::to_string(data.num);
        } else if (type == E_FML_BRACKET) {
            if (data.left_bracket) {
                return "(";
            } else {
                return ")";
            }
        } else if (type == E_FML_COMMA) {
            return ",";
        } else if (type == E_FML_OPERATOR) {
            if (data.op == E_OP_ADD) {
                return "+";
            } else if (data.op == E_OP_SUB) {
                return "-";
            } else if (data.op == E_OP_MULT) {
                return "*";
            } else if (data.op == E_OP_DIV) {
                return "/";
            } else if (data.op == E_OP_MOD) {
                return "%";
            } else if (data.op == E_OP_AND) {
                return "&&";
            } else if (data.op == E_OP_OR) {
                return "||";
            } else if (data.op == E_OP_GT) {
                return ">";
            } else if (data.op == E_OP_LT) {
                return "<";
            } else if (data.op == E_OP_GTE) {
                return ">=";
            } else if (data.op == E_OP_LTE) {
                return "<=";
            } else if (data.op == E_OP_EQ) {
                return "==";
            } else if (data.op == E_OP_MIN) {
                return "min";
            } else if (data.op == E_OP_MAX) {
                return "max";
            } else if (data.op == E_OP_GCD) {
                return "gcd";
            } else if (data.op == E_OP_LCM) {
                return "lcm";
            } else if (data.op == E_OP_AA) {
                return "+=";
            } else {
                return "???"; //Unkown
            }
        } else {
            return "???"; //Unkown
        }
    }
};

///@brief A class to parse formula
class FormulaParser {
  public:
    FormulaParser() = default;
    FormulaParser(const FormulaParser&) = delete;
    FormulaParser& operator=(const FormulaParser&) = delete;

    ///@brief returns integer result according to specified formula and data
    int parse_formula(std::string formula, const t_formula_data& mydata, bool is_breakpoint = false);

    ///@brief returns integer result according to specified piece-wise formula and data
    int parse_piecewise_formula(const char* formula, const t_formula_data& mydata);

    ///@brief checks if the specified formula is piece-wise defined
    static bool is_piecewise_formula(const char* formula);

  private:
    std::vector<Formula_Object> rpn_output_;

    // stack for handling operators and brackets in formula
    std::stack<Formula_Object> op_stack_;
};

} // namespace vtr

#endif
