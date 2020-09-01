#include "vtr_expr_eval.h"
#include "vtr_error.h"
#include "vtr_util.h"
#include "vtr_math.h"

#include <string>
#include <sstream>
#include <iostream>

/** global variables **/

/** bp_state_globals is a variable that holds a member of type BreakpointState. This member is altered by the breakpoint class, the placer, and router and holds the most updated values for variables that can trigger breakpoints (e.g move_num, temp_num etc.) **/
BreakpointStateGlobals bp_state_globals;

namespace vtr {

using std::stack;
using std::string;
using std::stringstream;
using std::vector;

/**this variables is used for the += operator and holds the initial value of the variable that is to be added to. after every addition, the related function compares with initial value to ensure correct incrementation **/
static int before_addition = 0;

/*---- Functions for Parsing the Symbolic Formulas ----*/

/* converts specified formula to a vector in reverse-polish notation */
static void formula_to_rpn(const char* formula, const t_formula_data& mydata, vector<Formula_Object>& rpn_output, stack<Formula_Object>& op_stack, bool is_breakpoint);

static void get_formula_object(const char* ch, int& ichar, const t_formula_data& mydata, Formula_Object* fobj, bool is_breakpoint);

/* returns integer specifying precedence of passed-in operator. higher integer
 * means higher precedence */
static int get_fobj_precedence(const Formula_Object& fobj);

/* Returns associativity of the specified operator */
static bool op_associativity_is_left(const t_operator& op);

/* used by the shunting-yard formula parser to deal with operators such as add and subtract */
static void handle_operator(const Formula_Object& fobj, vector<Formula_Object>& rpn_output, stack<Formula_Object>& op_stack);

/* used by the shunting-yard formula parser to deal with brackets, ie '(' and ')' */
static void handle_bracket(const Formula_Object& fobj, vector<Formula_Object>& rpn_output, stack<Formula_Object>& op_stack);

/* used by the shunting-yard formula parser to deal with commas, ie ','. These occur in function calls*/
static void handle_comma(const Formula_Object& fobj, vector<Formula_Object>& rpn_output, stack<Formula_Object>& op_stack);

/* parses revere-polish notation vector to return formula result */
static int parse_rpn_vector(vector<Formula_Object>& rpn_vec);

/* applies operation specified by 'op' to the given arguments. arg1 comes before arg2 */
static int apply_rpn_op(const Formula_Object& arg1, const Formula_Object& arg2, const Formula_Object& op);

/* checks if specified character represents an ASCII number */
static bool is_char_number(const char ch);

// returns true if ch is an operator (e.g +,-, *, etc.)
static bool is_operator(const char ch);

// returns true if the specified name is a known function operator
static bool is_function(std::string name);

// returns true if the specified name is a known compound operator
t_compound_operator is_compound_op(const char* ch);

// returns true if the specified name is a known variable
static bool is_variable(std::string var);

// returns the length of any identifier (e.g. name, function) starting at the beginning of str
static int identifier_length(const char* str);

/* increments str_ind until it reaches specified char is formula. returns true if character was found, false otherwise */
static bool goto_next_char(int* str_ind, const string& pw_formula, char ch);

//compares two strings while ignoring white space and case
bool same_string(std::string str1, std::string str2);

//checks if the block indicated by the user was one of the moved blocks in the last perturbation
int in_blocks_affected(std::string expression_left);

//the function of += operator
bool additional_assignment_op(int arg1, int arg2);

/**** Function Implementations ****/
/* returns integer result according to specified non-piece-wise formula and data */
int FormulaParser::parse_formula(std::string formula, const t_formula_data& mydata, bool is_breakpoint) {
    int result = -1;

    /* output in reverse-polish notation */
    auto& rpn_output = rpn_output_;
    rpn_output.clear();

    /* now we have to run the shunting-yard algorithm to convert formula to reverse polish notation */
    formula_to_rpn(formula.c_str(), mydata, rpn_output, op_stack_, is_breakpoint);

    /* then we run an RPN parser to get the final result */
    result = parse_rpn_vector(rpn_output);

    return result;
}

/* EXPERIMENTAL:
 *
 * returns integer result according to specified piece-wise formula and data. the piecewise
 * notation specifies different formulas that should be evaluated based on the index of
 * the incoming wire in 'mydata'. for example the formula
 *
 * {0:(W/2)} t-1; {(W/2):W} t+1;
 *
 * indicates that the function "t-1" should be evaluated if the incoming wire index falls
 * within the range [0,W/2) and that "t+1" should be evaluated if it falls within the
 * [W/2,W) range. The piece-wise format is:
 *
 * {start_0:end_0} formula_0; ... {start_i;end_i} formula_i; ...
 */
int FormulaParser::parse_piecewise_formula(const char* formula, const t_formula_data& mydata) {
    int result = -1;
    int str_ind = 0;
    int str_size = 0;

    int t = mydata.get_var_value("t");
    int tmp_ind_start = -1;
    int tmp_ind_count = -1;
    string substr;

    /* convert formula to string format */
    string pw_formula(formula);
    str_size = pw_formula.size();

    if (pw_formula[str_ind] != '{') {
        throw vtr::VtrError(vtr::string_fmt("parse_piecewise_formula: the first character in piece-wise formula should always be '{'\n"), __FILE__, __LINE__);
    }

    /* find the range to which t corresponds */
    /* the first character must be '{' as verified above */
    while (str_ind != str_size - 1) {
        /* set to true when range to which wire number corresponds has been found */
        bool found_range = false;
        bool char_found = false;
        int range_start = -1;
        int range_end = -1;
        tmp_ind_start = -1;
        tmp_ind_count = -1;

        /* get the start of the range */
        tmp_ind_start = str_ind + 1;
        char_found = goto_next_char(&str_ind, pw_formula, ':');
        if (!char_found) {
            throw vtr::VtrError(vtr::string_fmt("parse_piecewise_formula: could not find char %c\n", ':'), __FILE__, __LINE__);
        }
        tmp_ind_count = str_ind - tmp_ind_start; /* range start is between { and : */
        substr = pw_formula.substr(tmp_ind_start, tmp_ind_count);
        range_start = parse_formula(substr.c_str(), mydata);

        /* get the end of the range */
        tmp_ind_start = str_ind + 1;
        char_found = goto_next_char(&str_ind, pw_formula, '}');
        if (!char_found) {
            throw vtr::VtrError(vtr::string_fmt("parse_piecewise_formula: could not find char %c\n", '}'), __FILE__, __LINE__);
        }
        tmp_ind_count = str_ind - tmp_ind_start; /* range end is between : and } */
        substr = pw_formula.substr(tmp_ind_start, tmp_ind_count);
        range_end = parse_formula(substr.c_str(), mydata);

        if (range_start > range_end) {
            throw vtr::VtrError(vtr::string_fmt("parse_piecewise_formula: range_start, %d, is bigger than range end, %d\n", range_start, range_end), __FILE__, __LINE__);
        }

        /* is the incoming wire within this range? (inclusive) */
        if (range_start <= t && range_end >= t) {
            found_range = true;
        } else {
            found_range = false;
        }

        /* we're done if found correct range */
        if (found_range) {
            break;
        }
        char_found = goto_next_char(&str_ind, pw_formula, '{');
        if (!char_found) {
            throw vtr::VtrError(vtr::string_fmt("parse_piecewise_formula: could not find char %c\n", '{'), __FILE__, __LINE__);
        }
    }
    /* the string index should never actually get to the end of the string because we should have found the range to which the
     * current wire number corresponds */
    if (str_ind == str_size - 1) {
        throw vtr::VtrError(vtr::string_fmt("parse_piecewise_formula: could not find a closing '}'?\n"), __FILE__, __LINE__);
    }

    /* at this point str_ind should point to '}' right before the formula we're interested in starts */
    /* get the value corresponding to this formula */
    tmp_ind_start = str_ind + 1;
    goto_next_char(&str_ind, pw_formula, ';');
    tmp_ind_count = str_ind - tmp_ind_start; /* formula is between } and ; */
    substr = pw_formula.substr(tmp_ind_start, tmp_ind_count);

    /* now parse the formula corresponding to the appropriate piece-wise range */
    result = parse_formula(substr.c_str(), mydata);

    return result;
}

/* increments str_ind until it reaches specified char in formula. returns true if character was found, false otherwise */
static bool goto_next_char(int* str_ind, const string& pw_formula, char ch) {
    bool result = true;
    int str_size = pw_formula.size();
    if ((*str_ind) == str_size - 1) {
        throw vtr::VtrError(vtr::string_fmt("goto_next_char: passed-in str_ind is already at the end of string\n"), __FILE__, __LINE__);
    }

    do {
        (*str_ind)++;
        if (pw_formula[*str_ind] == ch) {
            /* found the next requested character */
            break;
        }

    } while ((*str_ind) != str_size - 1);
    if ((*str_ind) == str_size - 1 && pw_formula[*str_ind] != ch) {
        result = false;
    }
    return result;
}

/* Parses the specified formula using a shunting yard algorithm (see wikipedia). The function's result
 * is stored in the rpn_output vector in reverse-polish notation */
static void formula_to_rpn(const char* formula, const t_formula_data& mydata, vector<Formula_Object>& rpn_output, stack<Formula_Object>& op_stack, bool is_breakpoint) {
    // Empty op_stack.
    while (!op_stack.empty()) {
        op_stack.pop();
    }

    Formula_Object fobj; /* for parsing formula objects */

    int ichar = 0;
    const char* ch = nullptr;
    /* go through formula and build rpn_output along with op_stack until \0 character is hit */
    while (true) {
        ch = &formula[ichar];

        if ('\0' == (*ch)) {
            /* we're done */
            break;
        } else if (' ' == (*ch)) {
            /* skip space */
        } else {
            /* parse the character */
            get_formula_object(ch, ichar, mydata, &fobj, is_breakpoint);
            switch (fobj.type) {
                case E_FML_NUMBER:
                    /* add to output vector */
                    rpn_output.push_back(fobj);
                    break;
                case E_FML_OPERATOR:
                    /* operators may be pushed to op_stack or rpn_output */
                    handle_operator(fobj, rpn_output, op_stack);
                    break;
                case E_FML_BRACKET:
                    /* brackets are only ever pushed to op_stack, not rpn_output */
                    handle_bracket(fobj, rpn_output, op_stack);
                    break;
                case E_FML_COMMA:
                    handle_comma(fobj, rpn_output, op_stack);
                    break;
                case E_FML_VARIABLE:
                    /* add to output vector */
                    rpn_output.push_back(fobj);
                    break;
                default:
                    throw vtr::VtrError(vtr::string_fmt("in formula_to_rpn: unknown formula object type: %d\n", fobj.type), __FILE__, __LINE__);
                    break;
            }
        }
        ichar++;
    }

    /* pop all remaining operators off of stack */
    Formula_Object fobj_dummy;
    while (!op_stack.empty()) {
        fobj_dummy = op_stack.top();

        if (E_FML_BRACKET == fobj_dummy.type) {
            throw vtr::VtrError(vtr::string_fmt("in formula_to_rpn: Mismatched brackets in user-provided formula\n"), __FILE__, __LINE__);
        }

        rpn_output.push_back(fobj_dummy);
        op_stack.pop();
    }

    return;
}

/* Fills the formula object fobj according to specified character and mydata,
 * which help determine which numeric value, if any, gets assigned to fobj
 * ichar is incremented by the corresponding count if the need to step through the
 * character array arises */
static void get_formula_object(const char* ch, int& ichar, const t_formula_data& mydata, Formula_Object* fobj, bool is_breakpoint) {
    /* the character can either be part of a number, or it can be an object like W, t, (, +, etc
     * here we have to account for both possibilities */

    int id_len = identifier_length(ch);
    //We have a variable or function name
    std::string var_name(ch, id_len);
    if (id_len != 0) {
        if (is_function(var_name)) {
            fobj->type = E_FML_OPERATOR;
            if (var_name == "min")
                fobj->data.op = E_OP_MIN;
            else if (var_name == "max")
                fobj->data.op = E_OP_MAX;
            else if (var_name == "gcd")
                fobj->data.op = E_OP_GCD;
            else if (var_name == "lcm")
                fobj->data.op = E_OP_LCM;
            else {
                throw vtr::VtrError(vtr::string_fmt("in get_formula_object: recognized function: %s\n", var_name.c_str()), __FILE__, __LINE__);
            }

        } else if (!is_breakpoint) {
            //A number
            fobj->type = E_FML_NUMBER;
            fobj->data.num = mydata.get_var_value(
                vtr::string_view(
                    var_name.data(),
                    var_name.size()));
        } else if (is_variable(var_name)) {
            fobj->type = E_FML_VARIABLE;
            if (same_string(var_name, "temp_count"))
                fobj->data.num = bp_state_globals.get_glob_breakpoint_state()->temp_count;
            else if (same_string(var_name, "from_block"))
                fobj->data.num = bp_state_globals.get_glob_breakpoint_state()->from_block;
            else if (same_string(var_name, "move_num"))
                fobj->data.num = bp_state_globals.get_glob_breakpoint_state()->move_num;
            else if (same_string(var_name, "route_net_id"))
                fobj->data.num = bp_state_globals.get_glob_breakpoint_state()->route_net_id;
            else if (same_string(var_name, "in_blocks_affected"))
                fobj->data.num = in_blocks_affected(std::string(ch));
            else if (same_string(var_name, "router_iter"))
                fobj->data.num = bp_state_globals.get_glob_breakpoint_state()->router_iter;
        }

        ichar += (id_len - 1); //-1 since ichar is incremented at end of loop in formula_to_rpn()

    } else if (is_char_number(*ch)) {
        /* we have a number -- use atoi to convert */
        stringstream ss;
        while (is_char_number(*ch)) {
            ss << (*ch);
            ichar++;
            ch++;
        }
        ichar--;
        fobj->type = E_FML_NUMBER;
        fobj->data.num = vtr::atoi(ss.str().c_str());
    } else if (is_compound_op(ch) != E_COM_OP_UNDEFINED) {
        fobj->type = E_FML_OPERATOR;
        t_compound_operator comp_op_code = is_compound_op(ch);
        if (comp_op_code == E_COM_OP_EQ)
            fobj->data.op = E_OP_EQ;
        else if (comp_op_code == E_COM_OP_GTE)
            fobj->data.op = E_OP_GTE;
        else if (comp_op_code == E_COM_OP_LTE)
            fobj->data.op = E_OP_LTE;
        else if (comp_op_code == E_COM_OP_AND)
            fobj->data.op = E_OP_AND;
        else if (comp_op_code == E_COM_OP_OR)
            fobj->data.op = E_OP_OR;
        else if (comp_op_code == E_COM_OP_AA)
            fobj->data.op = E_OP_AA;
        ichar++;
    } else {
        switch ((*ch)) {
            case '+':
                fobj->type = E_FML_OPERATOR;
                fobj->data.op = E_OP_ADD;
                break;
            case '-':
                fobj->type = E_FML_OPERATOR;
                fobj->data.op = E_OP_SUB;
                break;
            case '*':
                fobj->type = E_FML_OPERATOR;
                fobj->data.op = E_OP_MULT;
                break;
            case '/':
                fobj->type = E_FML_OPERATOR;
                fobj->data.op = E_OP_DIV;
                break;
            case '(':
                fobj->type = E_FML_BRACKET;
                fobj->data.left_bracket = true;
                break;
            case ')':
                fobj->type = E_FML_BRACKET;
                fobj->data.left_bracket = false;
                break;
            case ',':
                fobj->type = E_FML_COMMA;
                break;
            case '>':
                fobj->type = E_FML_OPERATOR;
                fobj->data.op = E_OP_GT;
                break;
            case '<':
                fobj->type = E_FML_OPERATOR;
                fobj->data.op = E_OP_LT;
                break;
            case '%':
                fobj->type = E_FML_OPERATOR;
                fobj->data.op = E_OP_MOD;
                break;
            default:
                throw vtr::VtrError(vtr::string_fmt("in get_formula_object: unsupported character: %c\n", *ch), __FILE__, __LINE__);
                break;
        }
    }

    return;
}

/* returns integer specifying precedence of passed-in operator. higher integer
 * means higher precedence */
static int get_fobj_precedence(const Formula_Object& fobj) {
    int precedence = 0;

    if (E_FML_BRACKET == fobj.type || E_FML_COMMA == fobj.type) {
        precedence = 0;
    } else if (E_FML_OPERATOR == fobj.type) {
        t_operator op = fobj.data.op;
        switch (op) {
            case E_OP_AND: //fallthrough
            case E_OP_OR:  //fallthrough
                precedence = 1;
                break;
            case E_OP_ADD: //fallthrough
            case E_OP_SUB: //fallthrough
            case E_OP_GT:  //fallthrough
            case E_OP_LT:  //fallthrough
            case E_OP_EQ:  //fallthrough
            case E_OP_GTE: //fallthrough
            case E_OP_LTE: //fallthrough
            case E_OP_AA:  //falthrough
                precedence = 2;
                break;
            case E_OP_MULT: //fallthrough
            case E_OP_DIV:  //fallthrough
            case E_OP_MOD:
                precedence = 3;
                break;
            case E_OP_MIN: //fallthrough
            case E_OP_MAX: //fallthrough
            case E_OP_LCM: //fallthrough
            case E_OP_GCD:
                precedence = 4;
                break;
            default:
                throw vtr::VtrError(vtr::string_fmt("in get_fobj_precedence: unrecognized operator: %d\n", op), __FILE__, __LINE__);
                break;
        }
    } else {
        throw vtr::VtrError(vtr::string_fmt("in get_fobj_precedence: no precedence possible for formula object type %d\n", fobj.type), __FILE__, __LINE__);
    }

    return precedence;
}

/* Returns associativity of the specified operator */
static bool op_associativity_is_left(const t_operator& /*op*/) {
    bool is_left = true;

    /* associativity is 'left' for all but the power operator, which is not yet implemented */
    //TODO:
    //if op is 'power' set associativity is_left=false and return

    return is_left;
}

/* used by the shunting-yard formula parser to deal with operators such as add and subtract */
static void handle_operator(const Formula_Object& fobj, vector<Formula_Object>& rpn_output, stack<Formula_Object>& op_stack) {
    if (E_FML_OPERATOR != fobj.type) {
        throw vtr::VtrError(vtr::string_fmt("in handle_operator: passed in formula object not of type operator\n"), __FILE__, __LINE__);
    }
    int op_pr = get_fobj_precedence(fobj);
    bool op_assoc_is_left = op_associativity_is_left(fobj.data.op);

    Formula_Object fobj_dummy;
    bool keep_going = false;
    do {
        /* here we keep popping operators off the stack onto back of rpn_output while
         * associativity of operator is 'left' and precedence op_pr = top_pr, or while
         * precedence op_pr < top_pr */

        /* determine whether we should keep popping operators off the op stack */
        if (op_stack.empty()) {
            keep_going = false;
        } else {
            /* get precedence of top operator */
            int top_pr = get_fobj_precedence(op_stack.top());

            keep_going = ((op_assoc_is_left && op_pr == top_pr)
                          || op_pr < top_pr);

            if (keep_going) {
                /* pop top operator off stack onto the back of rpn_output */
                fobj_dummy = op_stack.top();
                rpn_output.push_back(fobj_dummy);
                op_stack.pop();
            }
        }

    } while (keep_going);

    /* place new operator object on top of stack */
    op_stack.push(fobj);

    return;
}

/* used by the shunting-yard formula parser to deal with brackets, ie '(' and ')' */
static void handle_bracket(const Formula_Object& fobj, vector<Formula_Object>& rpn_output, stack<Formula_Object>& op_stack) {
    if (E_FML_BRACKET != fobj.type) {
        throw vtr::VtrError(vtr::string_fmt("in handle_bracket: passed-in formula object not of type bracket\n"), __FILE__, __LINE__);
    }

    /* check if left or right bracket */
    if (fobj.data.left_bracket) {
        /* left bracket, so simply push it onto operator stack */
        op_stack.push(fobj);
    } else {
        bool keep_going = false;
        do {
            /* here we keep popping operators off op_stack onto back of rpn_output until a
             * left bracket is encountered */

            if (op_stack.empty()) {
                /* didn't find an opening bracket - mismatched brackets */
                keep_going = false;
                throw vtr::VtrError(vtr::string_fmt("Ran out of stack while parsing brackets -- bracket mismatch in user-specified formula\n"), __FILE__, __LINE__);
            }

            Formula_Object next_fobj = op_stack.top();
            if (E_FML_BRACKET == next_fobj.type) {
                if (next_fobj.data.left_bracket) {
                    /* matching bracket found -- pop off stack and finish */
                    op_stack.pop();
                    keep_going = false;
                } else {
                    /* should not find two right brackets without a left bracket in-between */
                    keep_going = false;
                    throw vtr::VtrError(vtr::string_fmt("Mismatched brackets encountered in user-specified formula\n"), __FILE__, __LINE__);
                }
            } else if (E_FML_OPERATOR == next_fobj.type) {
                /* pop operator off stack onto the back of rpn_output */
                Formula_Object fobj_dummy = op_stack.top();
                rpn_output.push_back(fobj_dummy);
                op_stack.pop();
                keep_going = true;
            } else {
                keep_going = false;
                throw vtr::VtrError(vtr::string_fmt("Found unexpected formula object on operator stack: %d\n", next_fobj.type), __FILE__, __LINE__);
            }
        } while (keep_going);
    }
    return;
}

/* used by the shunting-yard formula parser to deal with commas, ie ','. These occur in function calls*/
static void handle_comma(const Formula_Object& fobj, vector<Formula_Object>& rpn_output, stack<Formula_Object>& op_stack) {
    if (E_FML_COMMA != fobj.type) {
        throw vtr::VtrError(vtr::string_fmt("in handle_comm: passed-in formula object not of type comma\n"), __FILE__, __LINE__);
    }

    //Commas are treated as right (closing) bracket since it completes a
    //sub-expression, except that we do not cause the left (opening) brack to
    //be popped

    bool keep_going = true;
    do {
        /* here we keep popping operators off op_stack onto back of rpn_output until a
         * left bracket is encountered */

        if (op_stack.empty()) {
            /* didn't find an opening bracket - mismatched brackets */
            keep_going = false;
            throw vtr::VtrError(vtr::string_fmt("Ran out of stack while parsing comma -- bracket mismatch in user-specified formula\n"), __FILE__, __LINE__);
            keep_going = false;
        }

        Formula_Object next_fobj = op_stack.top();
        if (E_FML_BRACKET == next_fobj.type) {
            if (next_fobj.data.left_bracket) {
                /* matching bracket found */
                keep_going = false;
            } else {
                /* should not find two right brackets without a left bracket in-between */
                throw vtr::VtrError(vtr::string_fmt("Mismatched brackets encountered in user-specified formula\n"), __FILE__, __LINE__);
                keep_going = false;
            }
        } else if (E_FML_OPERATOR == next_fobj.type) {
            /* pop operator off stack onto the back of rpn_output */
            Formula_Object fobj_dummy = op_stack.top();
            rpn_output.push_back(fobj_dummy);
            op_stack.pop();
            keep_going = true;
        } else {
            throw vtr::VtrError(vtr::string_fmt("Found unexpected formula object on operator stack: %d\n", next_fobj.type), __FILE__, __LINE__);
            keep_going = false;
        }

    } while (keep_going);
}

/* parses a reverse-polish notation vector corresponding to a switchblock formula
 * and returns the integer result */
static int parse_rpn_vector(vector<Formula_Object>& rpn_vec) {
    int result = -1;

    /* first entry should always be a number or variable name*/
    if (E_FML_NUMBER != rpn_vec[0].type && E_FML_VARIABLE != rpn_vec[0].type) {
        throw vtr::VtrError(vtr::string_fmt("parse_rpn_vector: first entry is not a number or variable(was %s)\n", rpn_vec[0].to_string().c_str()), __FILE__, __LINE__);
    }

    if (rpn_vec.size() == 1 && rpn_vec[0].type == E_FML_NUMBER) {
        /* if the vector size is 1 then we just have a number (which was verified above) */
        result = rpn_vec[0].data.num;
    } else {
        /* have numbers and operators */
        Formula_Object fobj;
        int ivec = 0;
        /* keep going until we have gone through the whole vector */
        while (!rpn_vec.empty()) {
            /* keep going until we have hit an operator */
            do {
                ivec++; /* first item should never be operator anyway */
                if (ivec == (int)rpn_vec.size()) {
                    throw vtr::VtrError(vtr::string_fmt("parse_rpn_vector(): found multiple numbers in formula, but no operator\n"), __FILE__, __LINE__);
                }
            } while (E_FML_OPERATOR != rpn_vec[ivec].type);

            /* now we apply the selected operation to the two previous entries */
            /* the result is stored in the object that used to be the operation */
            rpn_vec[ivec].data.num = apply_rpn_op(rpn_vec[ivec - 2], rpn_vec[ivec - 1], rpn_vec[ivec]);
            rpn_vec[ivec].type = E_FML_NUMBER;

            /* remove the previous two entries from the vector */
            rpn_vec.erase(rpn_vec.begin() + ivec - 2, rpn_vec.begin() + ivec - 0);
            ivec -= 2;

            /* if we're down to one element, we are done */
            if (1 == rpn_vec.size()) {
                result = rpn_vec[ivec].data.num;
                rpn_vec.erase(rpn_vec.begin() + ivec);
            }
        }
    }
    return result;
}

/* applies operation specified by 'op' to the given arguments. arg1 comes before arg2 */
static int apply_rpn_op(const Formula_Object& arg1, const Formula_Object& arg2, const Formula_Object& op) {
    int result = -1;

    /* arguments must be numbers or variables */
    if (E_FML_NUMBER != arg1.type || E_FML_NUMBER != arg2.type) {
        if (E_FML_VARIABLE != arg1.type && E_FML_VARIABLE != arg2.type) {
            throw vtr::VtrError(vtr::string_fmt("in apply_rpn_op: one of the arguments is not a number or variable(was '%s %s %s')\n", arg1.to_string().c_str(), op.to_string().c_str(), arg2.to_string().c_str()), __FILE__, __LINE__);
        }
    }

    /* check that op is actually an operation */
    if (E_FML_OPERATOR != op.type) {
        throw vtr::VtrError(vtr::string_fmt("in apply_rpn_op: the object specified as the operation is not of operation type\n"), __FILE__, __LINE__);
    }

    /* apply operation to arguments */
    switch (op.data.op) {
        case E_OP_ADD:
            result = arg1.data.num + arg2.data.num;
            break;
        case E_OP_SUB:
            result = arg1.data.num - arg2.data.num;
            break;
        case E_OP_MULT:
            result = arg1.data.num * arg2.data.num;
            break;
        case E_OP_DIV:
            result = arg1.data.num / arg2.data.num;
            break;
        case E_OP_MAX:
            result = std::max(arg1.data.num, arg2.data.num);
            break;
        case E_OP_MIN:
            result = std::min(arg1.data.num, arg2.data.num);
            break;
        case E_OP_GCD:
            result = vtr::gcd(arg1.data.num, arg2.data.num);
            break;
        case E_OP_LCM:
            result = vtr::lcm(arg1.data.num, arg2.data.num);
            break;
        case E_OP_AND:
            result = arg1.data.num && arg2.data.num;
            break;
        case E_OP_OR:
            result = (arg1.data.num || arg2.data.num);
            break;
        case E_OP_GT:
            result = arg1.data.num > arg2.data.num;
            break;
        case E_OP_LT:
            result = arg1.data.num < arg2.data.num;
            break;
        case E_OP_GTE:
            result = (arg1.data.num >= arg2.data.num);
            break;
        case E_OP_LTE:
            result = (arg1.data.num <= arg2.data.num);
            break;
        case E_OP_EQ:
            result = arg1.data.num == arg2.data.num;
            break;
        case E_OP_MOD:
            result = arg1.data.num % arg2.data.num;
            break;
        case E_OP_AA:
            result = additional_assignment_op(arg1.data.num, arg2.data.num);
            break;
        default:
            throw vtr::VtrError(vtr::string_fmt("in apply_rpn_op: invalid operation: %d\n", op.data.op), __FILE__, __LINE__);
            break;
    }

    return result;
}

/* checks if specified character represents an ASCII number */
static bool is_char_number(const char ch) {
    bool result = false;

    if (ch >= '0' && ch <= '9') {
        result = true;
    } else {
        result = false;
    }

    return result;
}

//checks if entered char is a known operator (e.g +,-,<,>,...)
static bool is_operator(const char ch) {
    switch (ch) {
        case '+': //fallthrough
        case '-': //fallthrough
        case '*': //fallthrough
        case '/': //fallthrough
        case ')': //fallthrough
        case '(': //fallthrough
        case ',': //fallthrough
        case '&': //fallthrough
        case '|': //fallthrough
        case '>': //fallthrough
        case '<': //fallthrough
        case '=': //fallthrough
        case '%': //fallthrough
            return true;
        default:
            return false;
    }
}

//returns true if string signifies a function e.g max, min
static bool is_function(std::string name) {
    if (name == "min"
        || name == "max"
        || name == "gcd"
        || name == "lcm") {
        return true;
    }
    return false;
}

//returns enumerated code depending on the compound operator
//compound operators are operators with more than one character e.g &&, >=
t_compound_operator is_compound_op(const char* ch) {
    if (ch[1] != '\0') {
        if (ch[0] == '=' && ch[1] == '=')
            return E_COM_OP_EQ;
        else if (ch[0] == '>' && ch[1] == '=')
            return E_COM_OP_GTE;
        else if (ch[0] == '<' && ch[1] == '=')
            return E_COM_OP_LTE;
        else if (ch[0] == '&' && ch[1] == '&')
            return E_COM_OP_AND;
        else if (ch[0] == '|' && ch[1] == '|')
            return E_COM_OP_OR;
        else if (ch[0] == '+' && ch[1] == '=')
            return E_COM_OP_AA;
    }
    return E_COM_OP_UNDEFINED;
}

//checks if the entered string is a known variable name
static bool is_variable(std::string var_name) {
    if (same_string(var_name, "from_block") || same_string(var_name, "temp_count") || same_string(var_name, "move_num") || same_string(var_name, "route_net_id") || same_string(var_name, "in_blocks_affected") || same_string(var_name, "router_iter")) {
        return true;
    }
    return false;
}

//returns the length of the substring consisting of valid vairable characters from
//the start of the string
static int identifier_length(const char* str) {
    int ichar = 0;

    if (!str) return 0;

    while (str[ichar] != '\0') {
        //No whitespace
        if (str[ichar] == ' ') break;

        //Not an operator
        if (is_operator(str[ichar])) break;

        //First char must not be a number
        if (ichar == 0 && is_char_number(str[ichar])) break;

        ++ichar; //Next character
    }

    return ichar;
}

/* checks if the specified formula is piece-wise defined */
bool FormulaParser::is_piecewise_formula(const char* formula) {
    bool result = false;
    /* if formula is piecewise, we expect '{' to be the very first character */
    if ('{' == formula[0]) {
        result = true;
    } else {
        result = false;
    }
    return result;
}

//compares two string while ignoring case and white space. returns true if strings are the same
bool same_string(std::string str1, std::string str2) {
    //earse any white space in both strings
    str1.erase(remove(str1.begin(), str1.end(), ' '), str1.end());
    str2.erase(remove(str2.begin(), str2.end(), ' '), str2.end());

    //converting both strings to lower case to eliminate case sensivity
    std::transform(str1.begin(), str1.end(), str1.begin(), ::tolower);
    std::transform(str2.begin(), str2.end(), str2.begin(), ::tolower);

    return (str1.compare(str2) == 0);
}

//the += operator
bool additional_assignment_op(int arg1, int arg2) {
    int result = 0;
    if (before_addition == 0)
        before_addition = arg1;
    result = (arg1 == (before_addition + arg2));
    if (result)
        before_addition = 0;
    return result;
}

//recognizes the block_id to look for (entered by the user)
//then looks for that block_id in all the blocks moved in the last perturbation.
//returns the block id if found, else just -1
int in_blocks_affected(std::string expression_left) {
    int wanted_block = -1;
    int found_block;
    std::stringstream ss;
    ss << expression_left;
    std::string s;

    //finds block_id to look for
    while (!ss.eof()) {
        ss >> s;
        if (std::stringstream(s) >> found_block) {
            s = "";
            break;
        }
    }

    //goes through blocks_affected
    for (size_t i = 0; i < bp_state_globals.get_glob_breakpoint_state()->blocks_affected_by_move.size(); i++) {
        if (bp_state_globals.get_glob_breakpoint_state()->blocks_affected_by_move[i] == found_block) {
            bp_state_globals.get_glob_breakpoint_state()->block_affected = found_block;
            return found_block;
        }
    }
    return wanted_block;
}

} //namespace vtr

//returns the global variable that holds all values that can trigger a breakpoint and are updated by the router and placer
BreakpointStateGlobals* get_bp_state_globals() {
    return &bp_state_globals;
}
