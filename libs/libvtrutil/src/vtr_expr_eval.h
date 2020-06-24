#ifndef EXPR_EVAL_H
#define EXPR_EVAL_H
#include <map>
#include <string>
#include <vector>
#include <stack>
#include <cstring>

#include "vtr_util.h"
#include "vtr_error.h"
#include "vtr_string_view.h"
#include "vtr_flat_map.h"

namespace vtr {

/**** Structs ****/

class t_formula_data {
  public:
    void clear() {
        vars_.clear();
    }

    void set_var_value(vtr::string_view var, int value) { vars_[var] = value; }
    void set_var_value(const char* var, int value) { vars_[vtr::string_view(var)] = value; }

    int get_var_value(const std::string& var) const {
        return get_var_value(vtr::string_view(var.data(), var.size()));
    }

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
/* Used to identify the type of symbolic formula object */
typedef enum e_formula_obj {
    E_FML_UNDEFINED = 0,
    E_FML_NUMBER,
    E_FML_BRACKET,
    E_FML_COMMA,
    E_FML_OPERATOR,
    E_FML_NUM_FORMULA_OBJS
} t_formula_obj;

/* Used to identify an operator in a formula */
typedef enum e_operator {
    E_OP_UNDEFINED = 0,
    E_OP_ADD,
    E_OP_SUB,
    E_OP_MULT,
    E_OP_DIV,
    E_OP_MOD,
    E_OP_GT,
    E_OP_LT,
    E_OP_MIN,
    E_OP_MAX,
    E_OP_GCD,
    E_OP_LCM,
    E_OP_NUM_OPS
} t_operator;

/**** Class Definitions ****/
/* This class is used to represent an object in a formula, such as
 * a number, a bracket, an operator, or a variable */
class Formula_Object {
  public:
    /* indicates the type of formula object this is */
    t_formula_obj type;

    /* object data, accessed based on what kind of object this is */
    union u_Data {
        int num;           /*for number objects*/
        t_operator op;     /*for operator objects*/
        bool left_bracket; /*for bracket objects -- specifies if this is a left bracket*/

        u_Data() { memset(this, 0, sizeof(u_Data)); }
    } data;

    Formula_Object() {
        this->type = E_FML_UNDEFINED;
    }

    std::string to_string() const {
        if (type == E_FML_NUMBER) {
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
            } else if (data.op == E_OP_GT) {
                return ">";
            } else if (data.op == E_OP_LT) {
                return "<";
            } else if (data.op == E_OP_MIN) {
                return "min";
            } else if (data.op == E_OP_MAX) {
                return "max";
            } else if (data.op == E_OP_GCD) {
                return "gcd";
            } else if (data.op == E_OP_LCM) {
                return "lcm";
            } else {
                return "???"; //Unkown
            }
        } else {
            return "???"; //Unkown
        }
    }
};

class FormulaParser {
  public:
    FormulaParser() = default;
    FormulaParser(const FormulaParser&) = delete;
    FormulaParser& operator=(const FormulaParser&) = delete;

    /* returns integer result according to specified formula and data */
    int parse_formula(std::string formula, const t_formula_data& mydata);

    /* returns integer result according to specified piece-wise formula and data */
    int parse_piecewise_formula(const char* formula, const t_formula_data& mydata);

    /* checks if the specified formula is piece-wise defined */
    static bool is_piecewise_formula(const char* formula);

  private:
    std::vector<Formula_Object> rpn_output_;
    std::stack<Formula_Object> op_stack_; /* stack for handling operators and brackets in formula */
};

} //namespace vtr
#endif
