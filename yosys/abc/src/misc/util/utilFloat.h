/**CFile****************************************************************

  FileName    [utilFloat.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName []

  Synopsis    [Floating point number implementation.]

  Author      [Alan Mishchenko, Bruno Schmitt]

  Affiliation [UC Berkeley / UFRGS]

  Date        [Ver. 1.0. Started - January 28, 2017.]

  Revision    []

***********************************************************************/
#ifndef ABC__sat__xSAT__xsatFloat_h
#define ABC__sat__xSAT__xsatFloat_h

#include "misc/util/abc_global.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/* 
    The xFloat_t floating-point number is represented as a 32-bit unsigned int.
    The number is (2^Exp)*Mnt, where Exp is a 16-bit exponent and Mnt is a 
    16-bit mantissa.  The decimal point is located between the MSB of Mnt,
    which is always 1, and the remaining 15 digits of Mnt.

    Currently, only positive numbers are represented.

    The range of possible values is [1.0; 2^(2^16-1)*1.111111111111111]
    that is, the smallest possible number is 1.0 and the largest possible 
    number is 2^(---16 ones---).(1.---15 ones---)

    Comparison of numbers can be done by comparing the underlying unsigned ints.

    Only addition, multiplication, and division by 2^n are currently implemented.
*/

typedef struct xFloat_t_ xFloat_t;
struct xFloat_t_
{
    unsigned  Mnt : 16;
    unsigned  Exp : 16;
};

static inline unsigned   xSat_Float2Uint( xFloat_t f )                   { union { xFloat_t f; unsigned u; } temp; temp.f = f; return temp.u; }
static inline xFloat_t   xSat_Uint2Float( unsigned u )                   { union { xFloat_t f; unsigned u; } temp; temp.u = u; return temp.f; }
static inline int        xSat_LessThan( xFloat_t a, xFloat_t b )         { return a.Exp < b.Exp || (a.Exp == b.Exp && a.Mnt < b.Mnt);         }
static inline int        xSat_Equal( xFloat_t a, xFloat_t b )            { return a.Exp == b.Exp && a.Mnt == b.Mnt;                           }

static inline xFloat_t   xSat_FloatCreate( unsigned Exp, unsigned Mnt )  { xFloat_t res; res.Exp = Exp; res.Mnt = Mnt; return res;            }

static inline xFloat_t   xSat_FloatCreateConst1()                        { return xSat_FloatCreate( 0, 1 << 15 );                             }
static inline xFloat_t   xSat_FloatCreateConst2()                        { return xSat_FloatCreate( 1, 1 << 15 );                             }
static inline xFloat_t   xSat_FloatCreateConst3()                        { return xSat_FloatCreate( 1, 3 << 14 );                             }
static inline xFloat_t   xSat_FloatCreateConst12()                       { return xSat_FloatCreate( 3, 3 << 14 );                             }
static inline xFloat_t   xSat_FloatCreateConst1point5()                  { return xSat_FloatCreate( 0, 3 << 14 );                             }
static inline xFloat_t   xSat_FloatCreateConst2point5()                  { return xSat_FloatCreate( 1, 5 << 13 );                             }
static inline xFloat_t   xSat_FloatCreateMaximum()                       { return xSat_Uint2Float( 0xFFFFFFFF );                              }

static inline float      xSat_Float2Float( xFloat_t a )                  { assert(a.Exp < 127); return Abc_Int2Float(((a.Exp + 127) << 23) | ((a.Mnt & 0x7FFF) << 8)); }
static inline xFloat_t   xSat_FloatFromFloat( float a )                  { int A = Abc_Float2Int(a); assert(a >= 1.0); return xSat_FloatCreate((A >> 23)-127, 0x8000 | ((A >> 8) & 0x7FFF)); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Adding two floating-point numbers.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline xFloat_t xSat_FloatAdd( xFloat_t a, xFloat_t b )
{
    unsigned Exp, Mnt;
    if ( a.Exp < b.Exp )
        return xSat_FloatAdd(b, a);
    assert( a.Exp >= b.Exp );
    // compute new mantissa
    Mnt = a.Mnt + (b.Mnt >> (a.Exp - b.Exp));
    // compute new exponent
    Exp = a.Exp;
    // update exponent and mantissa if new MSB is created
    if ( Mnt & 0xFFFF0000 ) // new MSB bit is created
        Exp++, Mnt >>= 1;
    // check overflow
    if ( Exp & 0xFFFF0000 ) // overflow
        return xSat_Uint2Float( 0xFFFFFFFF );
    assert( (Exp & 0xFFFF0000) == 0 );
    assert( (Mnt & 0xFFFF0000) == 0 );
    assert( Mnt & 0x00008000 );
    return xSat_FloatCreate( Exp, Mnt );
}

/**Function*************************************************************

  Synopsis    [Multiplying two floating-point numbers.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline xFloat_t xSat_FloatMul( xFloat_t a, xFloat_t b )
{
    unsigned Exp, Mnt;
    if ( a.Exp < b.Exp )
        return xSat_FloatMul(b, a);
    assert( a.Exp >= b.Exp );
    // compute new mantissa
    Mnt = (a.Mnt * b.Mnt) >> 15;
    // compute new exponent
    Exp = a.Exp + b.Exp;
    // update exponent and mantissa if new MSB is created
    if ( Mnt & 0xFFFF0000 ) // new MSB bit is created
        Exp++, Mnt >>= 1;
    // check overflow
    if ( Exp & 0xFFFF0000 ) // overflow
        return xSat_Uint2Float( 0xFFFFFFFF );
    assert( (Exp & 0xFFFF0000) == 0 );
    assert( (Mnt & 0xFFFF0000) == 0 );
    assert( Mnt & 0x00008000 );
    return xSat_FloatCreate( Exp, Mnt );
}

/**Function*************************************************************

  Synopsis    [Dividing floating point number by a degree of 2.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline xFloat_t xSat_FloatDiv( xFloat_t a, unsigned Deg2 )
{
    assert( Deg2 < 0xFFFF );
    if ( a.Exp >= Deg2 )
        return xSat_FloatCreate( a.Exp - Deg2, a.Mnt );
    return xSat_FloatCreateConst1(); // underflow
}

/**Function*************************************************************

  Synopsis    [Testing procedure.]

  Description [Helpful link https://www.h-schmidt.net/FloatConverter/IEEE754.html]

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSat_FloatTest()
{
    xFloat_t c1   = xSat_FloatCreateConst1();
    xFloat_t c2   = xSat_FloatCreateConst2();
    xFloat_t c3   = xSat_FloatCreateConst3();
    xFloat_t c12  = xSat_FloatCreateConst12();
    xFloat_t c1p5 = xSat_FloatCreateConst1point5();
    xFloat_t c2p5 = xSat_FloatCreateConst2point5();

    xFloat_t c1_   = xSat_FloatFromFloat(1.0);
    xFloat_t c2_   = xSat_FloatFromFloat(2.0);
    xFloat_t c3_   = xSat_FloatFromFloat(3.0);
    xFloat_t c12_  = xSat_FloatFromFloat(12.0);
    xFloat_t c1p5_ = xSat_FloatFromFloat(1.5);
    xFloat_t c2p5_ = xSat_FloatFromFloat(2.5);

    xFloat_t sum1 = xSat_FloatAdd(c1, c1p5);
    xFloat_t mul1 = xSat_FloatMul(c2, c1p5);

    xFloat_t sum2 = xSat_FloatAdd(c1p5, c2p5);
    xFloat_t mul2 = xSat_FloatMul(c1p5, c2p5);

//    float f1 = xSat_Float2Float(c1);
//    Extra_PrintBinary( stdout, (int *)&c1, 32 ); printf( "\n" );
//    Extra_PrintBinary( stdout, (int *)&f1, 32 ); printf( "\n" );

    printf( "1 = %f\n",   xSat_Float2Float(c1) );
    printf( "2 = %f\n",   xSat_Float2Float(c2) );
    printf( "3 = %f\n",   xSat_Float2Float(c3) );
    printf( "12 = %f\n",  xSat_Float2Float(c12) );
    printf( "1.5 = %f\n", xSat_Float2Float(c1p5) );
    printf( "2.5 = %f\n", xSat_Float2Float(c2p5) );

    printf( "Converted 1 = %f\n",   xSat_Float2Float(c1_) );
    printf( "Converted 2 = %f\n",   xSat_Float2Float(c2_) );
    printf( "Converted 3 = %f\n",   xSat_Float2Float(c3_) );
    printf( "Converted 12 = %f\n",  xSat_Float2Float(c12_) );
    printf( "Converted 1.5 = %f\n", xSat_Float2Float(c1p5_) );
    printf( "Converted 2.5 = %f\n", xSat_Float2Float(c2p5_) );

    printf( "1.0 + 1.5 = %f\n", xSat_Float2Float(sum1) );
    printf( "2.0 * 1.5 = %f\n", xSat_Float2Float(mul1) );

    printf( "1.5 + 2.5 = %f\n", xSat_Float2Float(sum2) );
    printf( "1.5 * 2.5 = %f\n", xSat_Float2Float(mul2) );
    printf( "12 / 2^2  = %f\n", xSat_Float2Float(xSat_FloatDiv(c12, 2)) );

    assert( xSat_Equal(sum1, c2p5) );
    assert( xSat_Equal(mul1, c3) );
}

ABC_NAMESPACE_HEADER_END

#endif
