/**CFile****************************************************************

  FileName    [kitPerm.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Computation kit.]

  Synopsis    [Procedures for permuting truth tables.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Oct 26, 2011.]

  Revision    [$Id: kitPerm.c,v 1.00 2011/11/26 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "kit.h"

#define inline __inline // compatible with MS VS 6.0

//ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef unsigned __int64 word;
typedef unsigned short   shot;
typedef unsigned char    byte;

static shot S[256] =
{
    0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015,
    0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055,
    0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115,
    0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155,
    0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415,
    0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455,
    0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515,
    0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555,
    0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015,
    0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055,
    0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115,
    0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155,
    0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415,
    0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455,
    0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515,
    0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555,
    0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015,
    0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055,
    0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115,
    0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155,
    0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415,
    0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455,
    0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515,
    0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555,
    0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015,
    0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055,
    0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115,
    0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155,
    0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415,
    0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455,
    0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515,
    0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
};

static byte P[256] =
{
    0x00, 0x01, 0x10, 0x11, 0x04, 0x05, 0x14, 0x15, 0x40, 0x41, 0x50, 0x51, 0x44, 0x45, 0x54, 0x55,
    0x02, 0x03, 0x12, 0x13, 0x06, 0x07, 0x16, 0x17, 0x42, 0x43, 0x52, 0x53, 0x46, 0x47, 0x56, 0x57,
    0x20, 0x21, 0x30, 0x31, 0x24, 0x25, 0x34, 0x35, 0x60, 0x61, 0x70, 0x71, 0x64, 0x65, 0x74, 0x75,
    0x22, 0x23, 0x32, 0x33, 0x26, 0x27, 0x36, 0x37, 0x62, 0x63, 0x72, 0x73, 0x66, 0x67, 0x76, 0x77,
    0x08, 0x09, 0x18, 0x19, 0x0c, 0x0d, 0x1c, 0x1d, 0x48, 0x49, 0x58, 0x59, 0x4c, 0x4d, 0x5c, 0x5d,
    0x0a, 0x0b, 0x1a, 0x1b, 0x0e, 0x0f, 0x1e, 0x1f, 0x4a, 0x4b, 0x5a, 0x5b, 0x4e, 0x4f, 0x5e, 0x5f,
    0x28, 0x29, 0x38, 0x39, 0x2c, 0x2d, 0x3c, 0x3d, 0x68, 0x69, 0x78, 0x79, 0x6c, 0x6d, 0x7c, 0x7d,
    0x2a, 0x2b, 0x3a, 0x3b, 0x2e, 0x2f, 0x3e, 0x3f, 0x6a, 0x6b, 0x7a, 0x7b, 0x6e, 0x6f, 0x7e, 0x7f,
    0x80, 0x81, 0x90, 0x91, 0x84, 0x85, 0x94, 0x95, 0xc0, 0xc1, 0xd0, 0xd1, 0xc4, 0xc5, 0xd4, 0xd5,
    0x82, 0x83, 0x92, 0x93, 0x86, 0x87, 0x96, 0x97, 0xc2, 0xc3, 0xd2, 0xd3, 0xc6, 0xc7, 0xd6, 0xd7,
    0xa0, 0xa1, 0xb0, 0xb1, 0xa4, 0xa5, 0xb4, 0xb5, 0xe0, 0xe1, 0xf0, 0xf1, 0xe4, 0xe5, 0xf4, 0xf5,
    0xa2, 0xa3, 0xb2, 0xb3, 0xa6, 0xa7, 0xb6, 0xb7, 0xe2, 0xe3, 0xf2, 0xf3, 0xe6, 0xe7, 0xf6, 0xf7,
    0x88, 0x89, 0x98, 0x99, 0x8c, 0x8d, 0x9c, 0x9d, 0xc8, 0xc9, 0xd8, 0xd9, 0xcc, 0xcd, 0xdc, 0xdd,
    0x8a, 0x8b, 0x9a, 0x9b, 0x8e, 0x8f, 0x9e, 0x9f, 0xca, 0xcb, 0xda, 0xdb, 0xce, 0xcf, 0xde, 0xdf,
    0xa8, 0xa9, 0xb8, 0xb9, 0xac, 0xad, 0xbc, 0xbd, 0xe8, 0xe9, 0xf8, 0xf9, 0xec, 0xed, 0xfc, 0xfd,
    0xaa, 0xab, 0xba, 0xbb, 0xae, 0xaf, 0xbe, 0xbf, 0xea, 0xeb, 0xfa, 0xfb, 0xee, 0xef, 0xfe, 0xff
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_PermCreateS()
{
    int i, n, r;
    for ( i = 0; i < 256; i++ )
    {
        if ( i % 8 == 0 )
            printf( "\n" );
        for ( r = n = 0; n < 8; n++ )
            r |= ((i & (1 << n)) << n);
        printf( "0x%04x, ", r );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_PermCreateP()
{
    int i, s1, s2, r;
    for ( i = 0; i < 256; i++ )
    {
        if ( i % 16 == 0 )
            printf( "\n" );
        s1 = i & 0x0A;
        s2 = i & 0x50;
        r = i ^ s1 ^ s2 ^ (s1 << 3) ^ (s2 >> 3);
        assert( r < 256 );
        printf( "0x%02x, ", r );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Kit_PermCycleOne( shot * s, byte * b, int v )
{
    int i, n = (1 << (v-3));
    assert( v > 2 && v < 16 );
    for ( i = 0; i < n; i++ )
        s[i] = S[b[i]] | (S[b[i+n]] << 1);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Kit_PermCycleMany( shot * s, byte * b, int V, int v )
{
    int i, n = (1 << (V - 1 - v)), m = (1 << (v-2));
    assert( v > 2 && v < V );
    for ( i = 0; i < n; i++, s += (m >> 1), b += m )
        Kit_PermCycleOne( s, b, v );   
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_PermCompute( word * o, word * i, int V )
{
    word * t;
    int v, n = (1 << (V-3));
    assert( V >= 6 && V <= 16 );
    for ( v = 0; v < n; v++ )
        ((byte *)i)[v] = P[((byte *)i)[v]];
    for ( v = 3; v < V; v++ )
    {
        Kit_PermCycleMany( (shot *)o, (byte *)i, V, v );
        t = i; i = o; o = t;
    }
    if ( V & 1 )
    {
        n = (1 << (V-6));
        for ( v = 0; v < n; v++ )
            o[v] = i[v];
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_PermComputeNaive( word * F, int nVars )
{
    extern void If_CluReverseOrder( word * pF, int nVars, int * V2P, int * P2V, int iVarStart );
    int i, V2P[16], P2V[16];
    for ( i = 0; i < nVars; i++ )
        V2P[i] = P2V[i] = i;
    If_CluReverseOrder( F, nVars, V2P, P2V, 0 );
}


word M ( word f1, word f2, int n)
{
    word temp = 0;
    word a = 1;
    int i;
    for(  i = 0; i < n; i++)
        temp = temp + (((f1>>i)&a) << (2*i) ) + (((f2>>i)&a) << (2*i+1));
    return temp;
}

word Tf ( word f, int n)
{
    if(n==1)
        return f;
    else
    {
//        int x = (int)pow(2,n-1);
        int x;
        x = (1 << (n-1));
        return (  M (Tf( (f << x) >> x, n-1), Tf( (f >> x), n-1), x) );     //def. of M just below the function
     }
}


//#define ABC_PRT(a,t)    (printf("%s = ", (a)), printf("%7.2f sec\n", (float)(t)/(float)(CLOCKS_PER_SEC)))
#define NFUNCS (1<<20)

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_PermComputeTest()
{
    word * T = (word *)malloc( 8 * NFUNCS );
    word i, o, w = 0;
    int k, b;
    abctime clk;

    srand( 0 );

    clk = Abc_Clock();
    for ( k = 0; k < NFUNCS; k++ )
        for ( b = 0; b < 8; b++ )
            ((byte *)(T + k))[b] = (byte)(rand() & 0xFF);
    ABC_PRT( "Assign", Abc_Clock() - clk );

//    T[0] = 0xacaccacaaccaacca;
//    Kit_DsdPrintFromTruth( T, 6 );

    // perform measurements
    clk = Abc_Clock();
    for ( k = 0; k < NFUNCS; k++ )
    {
        i = T[k];
//        Kit_PermComputeNaive( &i, 6 );
        Tf( i, 6 );
    }
    ABC_PRT( "Perm1 ", Abc_Clock() - clk );

    // perform measurements
    clk = Abc_Clock();
    for ( k = 0; k < NFUNCS; k++ )
    {
        i = T[k];
        Kit_PermCompute( &o, &i, 6 );

//        w = T[k];
//        Kit_PermComputeNaive( &w, 6 );
//        assert( w == o );
    }
    ABC_PRT( "Perm2 ", Abc_Clock() - clk );

    assert( w == 0 );
    free( T );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []
`
***********************************************************************/
void Kit_PermComputeTest_()
{
    word t, s;
    t = 0xacaccacaaccaacca;
//    Kit_DsdPrintFromTruth( &t, 6 ); printf( "\n" );
    s = Tf( t, 6 );
//    Kit_PermComputeNaive( &t, 6 );
//    Kit_DsdPrintFromTruth( &s, 6 ); printf( "\n" );
}

/*
    {
        extern void Kit_PermComputeTest();
        Kit_PermComputeTest();
    }
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


//ABC_NAMESPACE_IMPL_END

