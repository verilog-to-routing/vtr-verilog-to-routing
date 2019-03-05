/* Authors: Aaron Graham (aaron.graham@unb.ca, aarongraham9@gmail.com),
 *           Jean-Philippe Legault (jlegault@unb.ca, jeanphilippe.legault@gmail.com) and
 *            Dr. Kenneth B. Kent (ken@unb.ca)
 *            for the Reconfigurable Computing Research Lab at the
 *             Univerity of New Brunswick in Fredericton, New Brunswick, Canada
 */

#ifndef RTL_INT_H
#define RTL_INT_H

#include <string>
#include "internal_bits.hpp"

/**
 * Unary Operator
 */

bool V_TRUE(VNumber& a);

VNumber V_ADD(VNumber& a);
VNumber V_MINUS(VNumber& a);

VNumber V_BITWISE_NOT(VNumber& a);
VNumber V_BITWISE_AND(VNumber& a);
VNumber V_BITWISE_OR(VNumber& a);
VNumber V_BITWISE_XOR(VNumber& a);
VNumber V_BITWISE_NAND(VNumber& a);
VNumber V_BITWISE_NOR(VNumber& a);
VNumber V_BITWISE_XNOR(VNumber& a);
VNumber V_LOGICAL_NOT(VNumber& a);

/**
 * Binary Operator
 */
VNumber V_BITWISE_AND(VNumber& a,VNumber& b);
VNumber V_BITWISE_OR(VNumber& a,VNumber& b);
VNumber V_BITWISE_XOR(VNumber& a,VNumber& b);
VNumber V_BITWISE_NAND(VNumber& a,VNumber& b);
VNumber V_BITWISE_NOR(VNumber& a,VNumber& b);
VNumber V_BITWISE_XNOR(VNumber& a,VNumber& b);

VNumber V_SIGNED_SHIFT_LEFT(VNumber& a, VNumber& b);
VNumber V_SIGNED_SHIFT_RIGHT(VNumber& a, VNumber& b);
VNumber V_SHIFT_LEFT(VNumber& a, VNumber& b);
VNumber V_SHIFT_RIGHT(VNumber& a, VNumber& b);

VNumber V_LOGICAL_AND(VNumber& a,VNumber& b);
VNumber V_LOGICAL_OR(VNumber& a,VNumber& b);

VNumber V_LT(VNumber& a,VNumber& b);
VNumber V_GT(VNumber& a,VNumber& b);
VNumber V_LE(VNumber& a,VNumber& b);
VNumber V_GE(VNumber& a,VNumber& b);
VNumber V_EQUAL(VNumber& a,VNumber& b);
VNumber V_NOT_EQUAL(VNumber& a,VNumber& b);
VNumber V_CASE_EQUAL(VNumber& a,VNumber& b);
VNumber V_CASE_NOT_EQUAL(VNumber& a,VNumber& b);

VNumber V_ADD(VNumber& a,VNumber& b);
VNumber V_MINUS(VNumber& a,VNumber& b);
VNumber V_MULTIPLY(VNumber& a,VNumber& b);
VNumber V_POWER(VNumber& a,VNumber& b);
VNumber V_DIV(VNumber& a,VNumber& b);
VNumber V_MOD(VNumber& a,VNumber& b);

/**
 * Ternary Operator
 */
VNumber V_TERNARY(VNumber& a, VNumber& b, VNumber& c);

#endif //RTL_INT_H
