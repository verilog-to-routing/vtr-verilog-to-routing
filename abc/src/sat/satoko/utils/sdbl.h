//===--- sdbl.h -------------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
// by Alan Mishchenko
#ifndef satoko__utils__sdbl_h
#define satoko__utils__sdbl_h

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START
/*
    The sdbl_t floating-point number is represented as a 64-bit unsigned int.
    The number is (2^expt)*mnt, where expt is a 16-bit exponent and mnt is a
    48-bit mantissa.  The decimal point is located between the MSB of mnt,
    which is always 1, and the remaining 15 digits of mnt.

    Currently, only positive numbers are represented.

    The range of possible values is [1.0; 2^(2^16-1)*1.111111111111111]
    that is, the smallest possible number is 1.0 and the largest possible
    number is 2^(---16 ones---).(1.---47 ones---)

    Comparison of numbers can be done by comparing the underlying unsigned ints.

    Only addition, multiplication, and division by 2^n are currently implemented.
*/

typedef word sdbl_t;

static sdbl_t SDBL_CONST1 = ABC_CONST(0x0000800000000000);
static sdbl_t SDBL_MAX = ~(sdbl_t)(0);

union ui64_dbl { word ui64; double dbl; };

static inline word sdbl_exp(sdbl_t a) { return a >> 48;         }
static inline word sdbl_mnt(sdbl_t a) { return (a << 16) >> 16; }

static inline double sdbl2double(sdbl_t a) {
    union ui64_dbl temp;
    assert(sdbl_exp(a) < 1023);
    temp.ui64 = ((sdbl_exp(a) + 1023) << 52) | (((a << 17) >> 17) << 5);
    return temp.dbl;
}

static inline sdbl_t double2sdbl(double value)
{
    union ui64_dbl temp;
    sdbl_t expt, mnt;
    assert(value >= 1.0);
    temp.dbl = value;
    expt = (temp.ui64 >> 52) - 1023;
    mnt = SDBL_CONST1 | ((temp.ui64 << 12) >> 17);
    return (expt << 48) + mnt;
}

static inline sdbl_t sdbl_add(sdbl_t a, sdbl_t b)
{
    sdbl_t expt, mnt;
    if (a < b) {
        a ^= b;
        b ^= a;
        a ^= b;
    }
    assert(a >= b);
    expt = sdbl_exp(a);
    mnt = sdbl_mnt(a) + (sdbl_mnt(b) >> (sdbl_exp(a) - sdbl_exp(b)));
    /* Check for carry */
    if (mnt >> 48) {
        expt++;
        mnt >>= 1;
    }
    if (expt >> 16) /* overflow */
        return SDBL_MAX;
    return (expt << 48) + mnt;
}

static inline sdbl_t sdbl_mult(sdbl_t a, sdbl_t b)
{
    sdbl_t expt, mnt;
    sdbl_t a_mnt, a_mnt_hi, a_mnt_lo;
    sdbl_t b_mnt, b_mnt_hi, b_mnt_lo;
    if (a < b) {
        a ^= b;
        b ^= a;
        a ^= b;
    }
    assert( a >= b );
    a_mnt  = sdbl_mnt(a);
    b_mnt  = sdbl_mnt(b);
    a_mnt_hi = a_mnt>>32;
    b_mnt_hi = b_mnt>>32;
    a_mnt_lo = (a_mnt<<32)>>32;
    b_mnt_lo = (b_mnt<<32)>>32;
    mnt = ((a_mnt_hi * b_mnt_hi) << 17) +
          ((a_mnt_lo * b_mnt_lo) >> 47) +
          ((a_mnt_lo * b_mnt_hi) >> 15) +
          ((a_mnt_hi * b_mnt_lo) >> 15);
    expt = sdbl_exp(a) + sdbl_exp(b);
    /* Check for carry */
    if (mnt >> 48) {
        expt++;
        mnt >>= 1;
    }
    if (expt >> 16) /* overflow */
        return SDBL_MAX;
    return (expt << 48) + mnt;
}

static inline sdbl_t sdbl_div(sdbl_t a, unsigned deg2)
{
    if (sdbl_exp(a) >= (word)deg2)
        return ((sdbl_exp(a) - deg2) << 48) + sdbl_mnt(a);
    return SDBL_CONST1;
}

static inline void sdbl_test()
{
    sdbl_t ten100_ = ABC_CONST(0x014c924d692ca61b);
    printf("%f\n", sdbl2double(ten100_));
    printf("%016lX\n", double2sdbl(1 /0.95));
    printf("%016lX\n", SDBL_CONST1);
    printf("%f\n", sdbl2double(SDBL_CONST1));
    printf("%f\n", sdbl2double(ABC_CONST(0x000086BCA1AF286B)));

}

ABC_NAMESPACE_HEADER_END

#endif /* satoko__utils__sdbl_h */
