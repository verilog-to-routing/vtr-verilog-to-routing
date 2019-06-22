#ifndef EXPR_EVAL_H
#define EXPR_EVAL_H
#include <map>
#include <string>
#include "arch_error.h"

/**** Structs ****/

class t_formula_data {
  public:
    void set_var_value(std::string var, int value) { vars_[var] = value; }

    int get_var_value(std::string var) const {
        auto iter = vars_.find(var);
        if (iter == vars_.end()) {
            archfpga_throw(__FILE__, __LINE__,
                           "No value found for variable '%s' from expression\n", var.c_str());
        }

        return iter->second;
    }

  private:
    std::map<std::string, int> vars_;
};

/* returns integer result according to specified formula and data */
int parse_formula(std::string formula, const t_formula_data& mydata);

/* returns integer result according to specified piece-wise formula and data */
int parse_piecewise_formula(const char* formula, const t_formula_data& mydata);

/* checks if the specified formula is piece-wise defined */
bool is_piecewise_formula(const char* formula);
#endif
