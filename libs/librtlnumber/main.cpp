/* Authors: Aaron Graham (aaron.graham@unb.ca, aarongraham9@gmail.com),
 *           Jean-Philippe Legault (jlegault@unb.ca, jeanphilippe.legault@gmail.com) and
 *            Dr. Kenneth B. Kent (ken@unb.ca)
 *            for the Reconfigurable Computing Research Lab at the
 *             Univerity of New Brunswick in Fredericton, New Brunswick, Canada
 */

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "rtl_int.hpp"
#include "rtl_utils.hpp"

#define bad_ops(test) _bad_ops(test, __func__, __LINE__)
inline static std::string _bad_ops(std::string test, const char* FUNCT, int LINE) {
    std::cerr << "INVALID INPUT OPS: (" << test << ")@" << FUNCT << "::" << std::to_string(LINE) << std::endl;
    std::abort();
}

/***
 *     __   __       ___  __   __           ___       __       
 *    /  ` /  \ |\ |  |  |__) /  \ |       |__  |    /  \ |  | 
 *    \__, \__/ | \|  |  |  \ \__/ |___    |    |___ \__/ |/\| 
 *                                                             
 * 	This is used for testing purposes only, unused in ODIN as the input is already preprocessed
 */
static std::string arithmetic(std::string op, std::string a_in) {
    VNumber a(a_in);
    VNumber result;

    if (op == "is_true") {
        result = VNumber(V_TRUE(a));
    } else if (op == "is_false") {
        result = VNumber(V_FALSE(a));
    } else if (op == "is_unk") {
        result = VNumber(V_UNK(a));
    } else if (op == "is_x") {
        result = VNumber(V_IS_X(a));
    } else if (op == "is_z") {
        result = VNumber(V_IS_Z(a));
    } else if (op == "is_unsigned") {
        result = VNumber(V_IS_UNSIGNED(a));
    } else if (op == "is_signed") {
        result = VNumber(V_IS_SIGNED(a));
    } else if (op == "to_unsigned") {
        result = V_UNSIGNED(a);
    } else if (op == "to_signed") {
        result = V_SIGNED(a);
    } else if (op == "~") {
        result = V_BITWISE_NOT(a);
    } else if (op == "-") {
        result = V_MINUS(a);
    } else if (op == "+") {
        result = V_ADD(a);
    } else if (op == "&") {
        result = V_BITWISE_AND(a);
    } else if (op == "|") {
        result = V_BITWISE_OR(a);
    } else if (op == "^") {
        result = V_BITWISE_XOR(a);
    } else if (op == "~&") {
        result = V_BITWISE_NAND(a);
    } else if (op == "~|") {
        result = V_BITWISE_NOR(a);
    } else if (op == "~^" || op == "^~") {
        result = V_BITWISE_XNOR(a);
    } else if (op == "!") {
        result = V_LOGICAL_NOT(a);
    } else {
        bad_ops(op);
    }

    return result.to_verilog_bitstring();
}

static std::string arithmetic(std::string a_in, std::string op, std::string b_in) {
    VNumber a(a_in);
    VNumber b(b_in);
    VNumber result;

    if (op == "&") {
        result = V_BITWISE_AND(a, b);
    } else if (op == "|") {
        result = V_BITWISE_OR(a, b);
    } else if (op == "^") {
        result = V_BITWISE_XOR(a, b);
    } else if (op == "~&") {
        result = V_BITWISE_NAND(a, b);
    } else if (op == "~|") {
        result = V_BITWISE_NOR(a, b);
    } else if (op == "~^" || op == "^~") {
        result = V_BITWISE_XNOR(a, b);
    } else if (op == "===") {
        result = V_CASE_EQUAL(a, b);
    } else if (op == "!==") {
        result = V_CASE_NOT_EQUAL(a, b);
    } else if (op == "<<") {
        result = V_SHIFT_LEFT(a, b);
    } else if (op == "<<<") {
        result = V_SIGNED_SHIFT_LEFT(a, b);
    } else if (op == ">>") {
        result = V_SHIFT_RIGHT(a, b);
    } else if (op == ">>>") {
        result = V_SIGNED_SHIFT_RIGHT(a, b);
    } else if (op == "&&") {
        result = V_LOGICAL_AND(a, b);
    } else if (op == "||") {
        result = V_LOGICAL_OR(a, b);
    } else if (op == "<") {
        result = V_LT(a, b);
    } else if (op == ">") {
        result = V_GT(a, b);
    } else if (op == "<=") {
        result = V_LE(a, b);
    } else if (op == ">=") {
        result = V_GE(a, b);
    } else if (op == "==") {
        result = V_EQUAL(a, b);
    } else if (op == "!=") {
        result = V_NOT_EQUAL(a, b);
    } else if (op == "+") {
        result = V_ADD(a, b);
    } else if (op == "-") {
        result = V_MINUS(a, b);
    } else if (op == "*") {
        result = V_MULTIPLY(a, b);
    } else if (op == "**") {
        result = V_POWER(a, b);
    } else if (op == "/") {
        result = V_DIV(a, b);
    } else if (op == "%") {
        result = V_MOD(a, b);
    } else {
        bad_ops(op);
    }

    return result.to_verilog_bitstring();
}

int main(int argc, char** argv) {
    std::vector<std::string> input;
    for (int i = 0; i < argc; i++)
        input.push_back(argv[i]);

    std::string result = "";

    if (argc < 3) {
        ERR_MSG("Not Enough Arguments: " << std::to_string(argc - 1));

        return -1;
    } else if (argc == 3) {
        result = arithmetic(input[1], input[2]);
    } else if (argc == 4 && input[1] == "display") {
        VNumber a(input[3]);
        result = V_STRING(a, input[2][0]);
    } else if (argc == 4 && input[1] == "bufif0") {
        VNumber bus(input[2]);
        VNumber trigger(input[3]);
        result = V_BITWISE_BUFIF0(bus, trigger).to_verilog_bitstring();
    } else if (argc == 4 && input[1] == "bufif1") {
        VNumber bus(input[2]);
        VNumber trigger(input[3]);
        result = V_BITWISE_BUFIF1(bus, trigger).to_verilog_bitstring();
    } else if (argc == 4 && input[1] == "notif0") {
        VNumber bus(input[2]);
        VNumber trigger(input[3]);
        result = V_BITWISE_NOTIF0(bus, trigger).to_verilog_bitstring();
    } else if (argc == 4 && input[1] == "notif1") {
        VNumber bus(input[2]);
        VNumber trigger(input[3]);
        result = V_BITWISE_NOTIF1(bus, trigger).to_verilog_bitstring();
    } else if (argc == 4) {
        result = arithmetic(input[1], input[2], input[3]);
    } else if (argc == 6 && (input[2] == "?" && input[4] == ":")) {
        VNumber a(input[1]);
        VNumber b(input[3]);
        VNumber c(input[5]);

        result = V_TERNARY(a, b, c).to_verilog_bitstring();
    } else if (argc == 6 && (input[1] == "{" && input[3] == "," && input[5] == "}")) {
        VNumber a(input[2]);
        VNumber b(input[4]);

        result = V_CONCAT({a, b}).to_verilog_bitstring();
    } else if (argc == 7 && (input[1] == "{" && input[3] == "{" && input[5] == "}" && input[6] == "}")) {
        VNumber n_times(input[2]);
        VNumber replicant(input[4]);

        result = V_REPLICATE(replicant, n_times).to_verilog_bitstring();
    } else {
        ERR_MSG("invalid Arguments: " << std::to_string(argc - 1));
        return -1;
    }

    std::cout << result << std::endl;

    return 0;
}
