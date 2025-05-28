/**CFile****************************************************************

  FileName    [utilDouble.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName []

  Synopsis    [Double floating point number implementation.]

  Author      [Alan Mishchenko, Bruno Schmitt]

  Affiliation [UC Berkeley / UFRGS]

  Date        [Ver. 1.0. Started - February 11, 2017.]

  Revision    []

***********************************************************************/

#ifndef ABC__sat__Xdbl__Xdbl_h
#define ABC__sat__Xdbl__Xdbl_h

#include "misc/util/abc_global.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/* 
    The xdbl floating-point number is represented as a 64-bit unsigned int.
    The number is (2^Exp)*Mnt, where Exp is a 16-bit exponent and Mnt is a 
    48-bit mantissa.  The decimal point is located between the MSB of Mnt,
    which is always 1, and the remaining 15 digits of Mnt.

    Currently, only positive numbers are represented.

    The range of possible values is [1.0; 2^(2^16-1)*1.111111111111111]
    that is, the smallest possible number is 1.0 and the largest possible 
    number is 2^(---16 ones---).(1.---47 ones---)

    Comparison of numbers can be done by comparing the underlying unsigned ints.

    Only addition, multiplication, and division by 2^n are currently implemented.
*/

typedef word xdbl;

static inline word   Xdbl_Exp( xdbl a )                 { return a >> 48;           }
static inline word   Xdbl_Mnt( xdbl a )                 { return (a << 16) >> 16;   }

static inline xdbl   Xdbl_Create( word Exp, word Mnt )  { assert(!(Exp>>16) && (Mnt>>47)==(word)1); return (Exp<<48) | Mnt;  }

static inline xdbl   Xdbl_Const1()                      { return Xdbl_Create( (word)0, (word)1 << 47 );                      }
static inline xdbl   Xdbl_Const2()                      { return Xdbl_Create( (word)1, (word)1 << 47 );                      }
static inline xdbl   Xdbl_Const3()                      { return Xdbl_Create( (word)1, (word)3 << 46 );                      }
static inline xdbl   Xdbl_Const12()                     { return Xdbl_Create( (word)3, (word)3 << 46 );                      }
static inline xdbl   Xdbl_Const1point5()                { return Xdbl_Create( (word)0, (word)3 << 46 );                      }
static inline xdbl   Xdbl_Const2point5()                { return Xdbl_Create( (word)1, (word)5 << 45 );                      }
static inline xdbl   Xdbl_Maximum()                     { return ~(word)0;                                                   }

static inline double Xdbl_ToDouble( xdbl a )            { assert(Xdbl_Exp(a) < 1023); return Abc_Word2Dbl(((Xdbl_Exp(a) + 1023) << 52) | (((a<<17)>>17) << 5));                  }
static inline xdbl   Xdbl_FromDouble( double a )        { word A = Abc_Dbl2Word(a); assert(a >= 1.0); return Xdbl_Create((A >> 52)-1023, (((word)1) << 47) | ((A << 12) >> 17)); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Adding two floating-point numbers.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline xdbl Xdbl_Add( xdbl a, xdbl b )
{
    word Exp, Mnt;
    if ( a < b ) a ^= b, b ^= a, a ^= b;
    assert( a >= b );
    Mnt = Xdbl_Mnt(a) + (Xdbl_Mnt(b) >> (Xdbl_Exp(a) - Xdbl_Exp(b)));
    Exp = Xdbl_Exp(a);
    if ( Mnt >> 48 ) // new MSB is created
        Exp++, Mnt >>= 1;
    if ( Exp >> 16 ) // overflow
        return Xdbl_Maximum();
    return Xdbl_Create( Exp, Mnt );
}

/**Function*************************************************************

  Synopsis    [Multiplying two floating-point numbers.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline xdbl Xdbl_Mul( xdbl a, xdbl b )
{
    word Exp, Mnt, MntA, MntB, MntAh, MntBh, MntAl, MntBl;
    if ( a < b ) a ^= b, b ^= a, a ^= b;
    assert( a >= b );
    MntA  = Xdbl_Mnt(a);
    MntB  = Xdbl_Mnt(b);
    MntAh = MntA>>32;
    MntBh = MntB>>32;
    MntAl = (MntA<<32)>>32;
    MntBl = (MntB<<32)>>32;
    Mnt = ((MntAh * MntBh) << 17) + ((MntAl * MntBl) >> 47) + ((MntAl * MntBh) >> 15) + ((MntAh * MntBl) >> 15);
    Exp = Xdbl_Exp(a) + Xdbl_Exp(b);
    if ( Mnt >> 48 ) // new MSB is created
        Exp++, Mnt >>= 1;
    if ( Exp >> 16 ) // overflow
        return Xdbl_Maximum();
    return Xdbl_Create( Exp, Mnt );
}

/**Function*************************************************************

  Synopsis    [Dividing floating point number by a degree of 2.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline xdbl Xdbl_Div( xdbl a, unsigned Deg2 )
{
    if ( Xdbl_Exp(a) >= (word)Deg2 )
        return Xdbl_Create( Xdbl_Exp(a) - Deg2, Xdbl_Mnt(a) );
    return Xdbl_Const1(); // underflow
}

/**Function*************************************************************

  Synopsis    [Testing procedure.]

  Description [Helpful link https://www.h-schmidt.net/FloatConverter/IEEE754.html]

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Xdbl_Test()
{
    xdbl c1   = Xdbl_Const1();
    xdbl c2   = Xdbl_Const2();
    xdbl c3   = Xdbl_Const3();
    xdbl c12  = Xdbl_Const12();
    xdbl c1p5 = Xdbl_Const1point5();
    xdbl c2p5 = Xdbl_Const2point5();

    xdbl c1_   = Xdbl_FromDouble(1.0);
    xdbl c2_   = Xdbl_FromDouble(2.0);
    xdbl c3_   = Xdbl_FromDouble(3.0);
    xdbl c12_  = Xdbl_FromDouble(12.0);
    xdbl c1p5_ = Xdbl_FromDouble(1.5);
    xdbl c2p5_ = Xdbl_FromDouble(2.5);

    xdbl sum1 = Xdbl_Add(c1, c1p5);
    xdbl mul1 = Xdbl_Mul(c2, c1p5);

    xdbl sum2 = Xdbl_Add(c1p5, c2p5);
    xdbl mul2 = Xdbl_Mul(c1p5, c2p5);

    xdbl a    = Xdbl_FromDouble(1.2929725);
    xdbl b    = Xdbl_FromDouble(10.28828287);
    xdbl ab   = Xdbl_Mul(a, b);

    xdbl ten100 = Xdbl_FromDouble( 1e100 );
    xdbl ten100_ = ABC_CONST(0x014c924d692ca61b);

    assert( ten100 == ten100_ );

//    float f1 = Xdbl_ToDouble(c1);
//    Extra_PrintBinary( stdout, (int *)&c1, 32 ); printf( "\n" );
//    Extra_PrintBinary( stdout, (int *)&f1, 32 ); printf( "\n" );

    printf( "1 = %lf\n",   Xdbl_ToDouble(c1) );
    printf( "2 = %lf\n",   Xdbl_ToDouble(c2) );
    printf( "3 = %lf\n",   Xdbl_ToDouble(c3) );
    printf( "12 = %lf\n",  Xdbl_ToDouble(c12) );
    printf( "1.5 = %lf\n", Xdbl_ToDouble(c1p5) );
    printf( "2.5 = %lf\n", Xdbl_ToDouble(c2p5) );

    printf( "Converted 1 = %lf\n",   Xdbl_ToDouble(c1_) );
    printf( "Converted 2 = %lf\n",   Xdbl_ToDouble(c2_) );
    printf( "Converted 3 = %lf\n",   Xdbl_ToDouble(c3_) );
    printf( "Converted 12 = %lf\n",  Xdbl_ToDouble(c12_) );
    printf( "Converted 1.5 = %lf\n", Xdbl_ToDouble(c1p5_) );
    printf( "Converted 2.5 = %lf\n", Xdbl_ToDouble(c2p5_) );

    printf( "1.0 + 1.5 = %lf\n", Xdbl_ToDouble(sum1) );
    printf( "2.0 * 1.5 = %lf\n", Xdbl_ToDouble(mul1) );

    printf( "1.5 + 2.5 = %lf\n", Xdbl_ToDouble(sum2) );
    printf( "1.5 * 2.5 = %lf\n", Xdbl_ToDouble(mul2) );
    printf( "12 / 2^2  = %lf\n", Xdbl_ToDouble(Xdbl_Div(c12, 2)) );

    printf( "12 / 2^2  = %lf\n", Xdbl_ToDouble(Xdbl_Div(c12, 2)) );

    printf( "%.16lf * %.16lf = %.16lf (%.16lf)\n", Xdbl_ToDouble(a), Xdbl_ToDouble(b), Xdbl_ToDouble(ab), 1.2929725 * 10.28828287 );

    assert( sum1 == c2p5 );
    assert( mul1 == c3 );
}

ABC_NAMESPACE_HEADER_END

#endif
