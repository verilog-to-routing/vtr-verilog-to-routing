#include "expr_eval.h"
#include "arch_error.h"
#include "vtr_util.h"
#include "vtr_math.h"
#include <vector>
#include <stack>
#include <string>
#include <sstream>
#include <cstring> //memset

using std::stack;
using std::string;
using std::stringstream;
using std::vector;

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

/*---- Functions for Parsing the Symbolic Formulas ----*/

/* converts specified formula to a vector in reverse-polish notation */
static void formula_to_rpn(const char* formula, const t_formula_data& mydata, vector<Formula_Object>& rpn_output);

static void get_formula_object(const char* ch, int& ichar, const t_formula_data& mydata, Formula_Object* fobj);

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

// returns true if ch is an operator
static bool is_operator(const char ch);

// returns true if the specified name is an known function operator
static bool is_function(std::string name);

// returns the length of any identifier (e.g. name, function) starting at the beginning of str
static int identifier_length(const char* str);

/* increments str_ind until it reaches specified char is formula. returns true if character was found, false otherwise */
static bool goto_next_char(int* str_ind, const string& pw_formula, char ch);

/**** Function Implementations ****/
/* returns integer result according to specified non-piece-wise formula and data */
int parse_formula(std::string formula, const t_formula_data& mydata) {
    int result = -1;

    /* output in reverse-polish notation */
    vector<Formula_Object> rpn_output;

    /* now we have to run the shunting-yard algorithm to convert formula to reverse polish notation */
    formula_to_rpn(formula.c_str(), mydata, rpn_output);

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
int parse_piecewise_formula(const char* formula, const t_formula_data& mydata) {
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
        archfpga_throw(__FILE__, __LINE__, "parse_piecewise_formula: the first character in piece-wise formula should always be '{'\n");
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
            archfpga_throw(__FILE__, __LINE__, "parse_piecewise_formula: could not find char %c\n", ':');
        }
        tmp_ind_count = str_ind - tmp_ind_start; /* range start is between { and : */
        substr = pw_formula.substr(tmp_ind_start, tmp_ind_count);
        range_start = parse_formula(substr.c_str(), mydata);

        /* get the end of the range */
        tmp_ind_start = str_ind + 1;
        char_found = goto_next_char(&str_ind, pw_formula, '}');
        if (!char_found) {
            archfpga_throw(__FILE__, __LINE__, "parse_piecewise_formula: could not find char %c\n", '}');
        }
        tmp_ind_count = str_ind - tmp_ind_start; /* range end is between : and } */
        substr = pw_formula.substr(tmp_ind_start, tmp_ind_count);
        range_end = parse_formula(substr.c_str(), mydata);

        if (range_start > range_end) {
            archfpga_throw(__FILE__, __LINE__, "parse_piecewise_formula: range_start, %d, is bigger than range end, %d\n", range_start, range_end);
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
            archfpga_throw(__FILE__, __LINE__, "parse_piecewise_formula: could not find char %c\n", '{');
        }
    }
    /* the string index should never actually get to the end of the string because we should have found the range to which the
     * current wire number corresponds */
    if (str_ind == str_size - 1) {
        archfpga_throw(__FILE__, __LINE__, "parse_piecewise_formula: could not find a closing '}'?\n");
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
        archfpga_throw(__FILE__, __LINE__, "goto_next_char: passed-in str_ind is already at the end of string\n");
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
static void formula_to_rpn(const char* formula, const t_formula_data& mydata, vector<Formula_Object>& rpn_output) {
    stack<Formula_Object> op_stack; /* stack for handling operators and brackets in formula */
    Formula_Object fobj;            /* for parsing formula objects */

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
            get_formula_object(ch, ichar, mydata, &fobj);
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
                default:
                    archfpga_throw(__FILE__, __LINE__, "in formula_to_rpn: unknown formula object type: %d\n", fobj.type);
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
            archfpga_throw(__FILE__, __LINE__, "in formula_to_rpn: Mismatched brackets in user-provided formula\n");
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
static void get_formula_object(const char* ch, int& ichar, const t_formula_data& mydata, Formula_Object* fobj) {
    /* the character can either be part of a number, or it can be an object like W, t, (, +, etc
     * here we have to account for both possibilities */

    int id_len = identifier_length(ch);

    if (id_len != 0) {
        //We have a variable or function name
        std::string var_name(ch, id_len);

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
                archfpga_throw(__FILE__, __LINE__, "in get_formula_object: recognized function: %s\n", var_name.c_str());
            }

        } else {
            //A variable
            fobj->type = E_FML_NUMBER;
            fobj->data.num = mydata.get_var_value(var_name);
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
            case '/':
                fobj->type = E_FML_OPERATOR;
                fobj->data.op = E_OP_DIV;
                break;
            case '*':
                fobj->type = E_FML_OPERATOR;
                fobj->data.op = E_OP_MULT;
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
            default:
                archfpga_throw(__FILE__, __LINE__, "in get_formula_object: unsupported character: %c\n", *ch);
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
            case E_OP_ADD: //fallthrough
            case E_OP_SUB:
                precedence = 2;
                break;
            case E_OP_MULT: //fallthrough
            case E_OP_DIV:
                precedence = 3;
                break;
            case E_OP_MIN: //fallthrough
            case E_OP_MAX: //fallthrough
            case E_OP_LCM: //fallthrough
            case E_OP_GCD: //fallthrough
                precedence = 4;
                break;
            default:
                archfpga_throw(__FILE__, __LINE__, "in get_fobj_precedence: unrecognized operator: %d\n", op);
                break;
        }
    } else {
        archfpga_throw(__FILE__, __LINE__, "in get_fobj_precedence: no precedence possible for formula object type %d\n", fobj.type);
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
        archfpga_throw(__FILE__, __LINE__, "in handle_operator: passed in formula object not of type operator\n");
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
        archfpga_throw(__FILE__, __LINE__, "in handle_bracket: passed-in formula object not of type bracket\n");
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
                archfpga_throw(__FILE__, __LINE__, "Ran out of stack while parsing brackets -- bracket mismatch in user-specified formula\n");
                keep_going = false;
            }

            Formula_Object next_fobj = op_stack.top();
            if (E_FML_BRACKET == next_fobj.type) {
                if (next_fobj.data.left_bracket) {
                    /* matching bracket found -- pop off stack and finish */
                    op_stack.pop();
                    keep_going = false;
                } else {
                    /* should not find two right brackets without a left bracket in-between */
                    archfpga_throw(__FILE__, __LINE__, "Mismatched brackets encountered in user-specified formula\n");
                    keep_going = false;
                }
            } else if (E_FML_OPERATOR == next_fobj.type) {
                /* pop operator off stack onto the back of rpn_output */
                Formula_Object fobj_dummy = op_stack.top();
                rpn_output.push_back(fobj_dummy);
                op_stack.pop();
                keep_going = true;
            } else {
                archfpga_throw(__FILE__, __LINE__, "Found unexpected formula object on operator stack: %d\n", next_fobj.type);
                keep_going = false;
            }
        } while (keep_going);
    }
    return;
}

/* used by the shunting-yard formula parser to deal with commas, ie ','. These occur in function calls*/
static void handle_comma(const Formula_Object& fobj, vector<Formula_Object>& rpn_output, stack<Formula_Object>& op_stack) {
    if (E_FML_COMMA != fobj.type) {
        archfpga_throw(__FILE__, __LINE__, "in handle_comm: passed-in formula object not of type comma\n");
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
            archfpga_throw(__FILE__, __LINE__, "Ran out of stack while parsing comma -- bracket mismatch in user-specified formula\n");
            keep_going = false;
        }

        Formula_Object next_fobj = op_stack.top();
        if (E_FML_BRACKET == next_fobj.type) {
            if (next_fobj.data.left_bracket) {
                /* matching bracket found */
                keep_going = false;
            } else {
                /* should not find two right brackets without a left bracket in-between */
                archfpga_throw(__FILE__, __LINE__, "Mismatched brackets encountered in user-specified formula\n");
                keep_going = false;
            }
        } else if (E_FML_OPERATOR == next_fobj.type) {
            /* pop operator off stack onto the back of rpn_output */
            Formula_Object fobj_dummy = op_stack.top();
            rpn_output.push_back(fobj_dummy);
            op_stack.pop();
            keep_going = true;
        } else {
            archfpga_throw(__FILE__, __LINE__, "Found unexpected formula object on operator stack: %d\n", next_fobj.type);
            keep_going = false;
        }

    } while (keep_going);
}

/* parses a reverse-polish notation vector corresponding to a switchblock formula
 * and returns the integer result */
static int parse_rpn_vector(vector<Formula_Object>& rpn_vec) {
    int result = -1;

    /* first entry should always be a number */
    if (E_FML_NUMBER != rpn_vec[0].type) {
        archfpga_throw(__FILE__, __LINE__, "parse_rpn_vector: first entry is not a number (was %s)\n", rpn_vec[0].to_string().c_str());
    }

    if (rpn_vec.size() == 1) {
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
                    archfpga_throw(__FILE__, __LINE__, "parse_rpn_vector(): found multiple numbers in formula, but no operator\n");
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

    /* arguments must be numbers */
    if (E_FML_NUMBER != arg1.type || E_FML_NUMBER != arg2.type) {
        archfpga_throw(__FILE__, __LINE__, "in apply_rpn_op: one of the arguments is not a number (was '%s %s %s')\n", arg1.to_string().c_str(), op.to_string().c_str(), arg2.to_string().c_str());
    }

    /* check that op is actually an operation */
    if (E_FML_OPERATOR != op.type) {
        archfpga_throw(__FILE__, __LINE__, "in apply_rpn_op: the object specified as the operation is not of operation type\n");
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
        default:
            archfpga_throw(__FILE__, __LINE__, "in apply_rpn_op: invalid operation: %d\n", op.data.op);
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

static bool is_operator(const char ch) {
    switch (ch) {
        case '+': //fallthrough
        case '-': //fallthrough
        case '/': //fallthrough
        case '*': //fallthrough
        case ')': //fallthrough
        case '(': //fallthrough
        case ',':
            return true;
        default:
            return false;
    }
}

static bool is_function(std::string name) {
    if (name == "min"
        || name == "max"
        || name == "gcd"
        || name == "lcm") {
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
bool is_piecewise_formula(const char* formula) {
    bool result = false;
    /* if formula is piecewise, we expect '{' to be the very first character */
    if ('{' == formula[0]) {
        result = true;
    } else {
        result = false;
    }
    return result;
}
