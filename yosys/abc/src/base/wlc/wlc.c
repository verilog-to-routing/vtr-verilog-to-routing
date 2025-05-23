/**CFile****************************************************************

  FileName    [wlc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Parses several flavors of word-level Verilog.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlc.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"
#include "sat/bsat/satStore.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_GenerateCodeMax4( int nBits )
{
    int nWidth, nSteps, i;
    FILE * pFile = fopen( "max4.v", "wb" );
    if ( pFile == NULL )
        return;

    for ( nSteps = 0, nWidth = 1; nWidth < nBits; nWidth *= 3, nSteps++ );

    fprintf( pFile, "module max4 ( a, b, c, d, res, addr );\n\n" );
    fprintf( pFile, "  input [%d:0] a, b, c, d;\n", nBits-1 );
    fprintf( pFile, "  output [%d:0] res;\n", nBits-1 );
    fprintf( pFile, "  output [1:0] addr;\n\n" );

    fprintf( pFile, "  wire [%d:0] A = a;\n", nWidth-1 );
    fprintf( pFile, "  wire [%d:0] B = b;\n", nWidth-1 );
    fprintf( pFile, "  wire [%d:0] C = c;\n", nWidth-1 );
    fprintf( pFile, "  wire [%d:0] D = d;\n\n", nWidth-1 );

    fprintf( pFile, "  wire AB, AC, AD, BC, BD, CD;\n\n" );

    fprintf( pFile, "  comp( A, B, AB );\n" );
    fprintf( pFile, "  comp( A, C, AC );\n" );
    fprintf( pFile, "  comp( A, D, AD );\n" );
    fprintf( pFile, "  comp( B, C, BC );\n" );
    fprintf( pFile, "  comp( B, D, BD );\n" );
    fprintf( pFile, "  comp( C, D, CD );\n\n" );

    fprintf( pFile, "  assign addr = AB ?  (CD ? (AC ? 2\'b00 : 2\'b10) : (AD ? 2\'b00 : 2\'b11))  :  (CD ? (BC ? 2\'b01 : 2\'b10) : (BD ? 2\'b01 : 2\'b11));\n\n" );
    fprintf( pFile, "  assign res = addr[1] ? (addr[1] ? d : c) : (addr[0] ? b : a);\n\n" );
    fprintf( pFile, "endmodule\n\n\n" );


    fprintf( pFile, "module comp ( a, b, res );\n\n" );
    fprintf( pFile, "  input [%d:0] a, b;\n", nWidth-1 );
    fprintf( pFile, "  output res;\n" );
    fprintf( pFile, "  wire res2;\n\n" );

    fprintf( pFile, "  wire [%d:0] A =  a & ~b;\n", nWidth-1 );
    fprintf( pFile, "  wire [%d:0] B = ~a &  b;\n\n", nWidth-1 );

    fprintf( pFile, "  comp0( A, B, res, res2 );\n\n" );

    fprintf( pFile, "endmodule\n\n\n" );


    for ( i = 0; i < nSteps; i++ )
    {
        fprintf( pFile, "module comp%d ( a, b, yes, no );\n\n", i );
        fprintf( pFile, "  input [%d:0] a, b;\n", nWidth-1 );
        fprintf( pFile, "  output yes, no;\n\n", nWidth/3-1 );

        fprintf( pFile, "  wire [2:0] y, n;\n\n" );

        if ( i == nSteps - 1 )
        {
            fprintf( pFile, "  assign y = a;\n" );
            fprintf( pFile, "  assign n = b;\n\n" );
        }
        else
        {
            fprintf( pFile, "  wire [%d:0] A0 = a[%d:%d];\n", nWidth/3-1, nWidth/3-1, 0 );
            fprintf( pFile, "  wire [%d:0] A1 = a[%d:%d];\n", nWidth/3-1, 2*nWidth/3-1, nWidth/3 );
            fprintf( pFile, "  wire [%d:0] A2 = a[%d:%d];\n\n", nWidth/3-1, nWidth-1, 2*nWidth/3 );

            fprintf( pFile, "  wire [%d:0] B0 = b[%d:%d];\n", nWidth/3-1, nWidth/3-1, 0 );
            fprintf( pFile, "  wire [%d:0] B1 = b[%d:%d];\n", nWidth/3-1, 2*nWidth/3-1, nWidth/3 );
            fprintf( pFile, "  wire [%d:0] B2 = b[%d:%d];\n\n", nWidth/3-1, nWidth-1, 2*nWidth/3 );

            fprintf( pFile, "  comp%d( A0, B0, y[0], n[0] );\n", i+1 );
            fprintf( pFile, "  comp%d( A1, B1, y[1], n[1] );\n", i+1 );
            fprintf( pFile, "  comp%d( A2, B2, y[2], n[2] );\n\n", i+1 );
        }

        fprintf( pFile, "  assign yes = y[0] | (~y[0] & ~n[0] & y[1]) | (~y[0] & ~n[0] & ~y[1] & ~n[1] & y[2]);\n" );
        fprintf( pFile, "  assign no  = n[0] | (~y[0] & ~n[0] & n[1]) | (~y[0] & ~n[0] & ~y[1] & ~n[1] & n[2]);\n\n" );

        fprintf( pFile, "endmodule\n\n\n" );

        nWidth /= 3;
    }
    fclose( pFile );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_BlastFullAdderCtrlCnf( sat_solver * pSat, int a, int ac, int b, int c, int * pc, int * ps, int * piVars )
{
    int Cnf[12][6] = {            // xabc cs   // abc cs

        { -1, 0, 0, 0,  0, 0 },   // -000 00   // 000 00
        { -1, 0, 0, 1,  0, 1 },   // -001 01   // 001 01
        { -1, 0, 1, 0,  0, 1 },   // -010 01   // 010 01
        { -1, 0, 1, 1,  1, 0 },   // -011 10   // 011 10

        {  0,-1, 0, 0,  0, 0 },   // 0-00 00
        {  0,-1, 0, 1,  0, 1 },   // 0-01 01
        {  0,-1, 1, 0,  0, 1 },   // 0-10 01
        {  0,-1, 1, 1,  1, 0 },   // 0-11 10

        {  1, 1, 0, 0,  0, 1 },   // 1100 01   // 100 01
        {  1, 1, 0, 1,  1, 0 },   // 1101 10   // 101 10
        {  1, 1, 1, 0,  1, 0 },   // 1110 10   // 110 10
        {  1, 1, 1, 1,  1, 1 }    // 1111 11   // 111 11
    };

    int pVars[6] = {a, ac, b, c, *piVars, *piVars+1};
    int i, v, nLits, pLits[6];
    for ( i = 0; i < 12; i++ )
    {
        nLits = 0;
        for ( v = 0; v < 6; v++ )
        {
            if ( Cnf[i][v] == -1 )
                continue;
            if ( pVars[v] == 0 ) // const 0
            {
                if ( Cnf[i][v] == 0 )
                    continue;
                if ( Cnf[i][v] == 1 )
                    break;
            }
            if ( pVars[v] == -1 ) // const -1
            {
                if ( Cnf[i][v] == 0 )
                    break;
                if ( Cnf[i][v] == 1 )
                    continue;
            }
            pLits[nLits++] = Abc_Var2Lit( pVars[v], Cnf[i][v] );
        }
        if ( v < 6 )
            continue;
        assert( nLits > 2 );
        sat_solver_addclause( pSat, pLits, pLits + nLits );
    }
    *pc = (*piVars)++;
    *ps = (*piVars)++;
}
void Wlc_BlastMultiplierCnf( sat_solver * pSat, int * pArgA, int * pArgB, int nArgA, int nArgB, Vec_Int_t * vTemp, Vec_Int_t * vRes, int * piVars )
{
    int * pRes, * pArgC, * pArgS, a, b, Carry = 0;
    assert( nArgA > 0 && nArgB > 0 );
    // prepare result
    Vec_IntFill( vRes, nArgA + nArgB, 0 );
    pRes = Vec_IntArray( vRes );
    // prepare intermediate storage
    Vec_IntFill( vTemp, 2 * nArgA, 0 );
    pArgC = Vec_IntArray( vTemp );
    pArgS = pArgC + nArgA;
    // create matrix
    for ( b = 0; b < nArgB; b++ )
        for ( a = 0; a < nArgA; a++ )
            Wlc_BlastFullAdderCtrlCnf( pSat, pArgA[a], pArgB[b], pArgS[a], pArgC[a], &pArgC[a], a ? &pArgS[a-1] : &pRes[b], piVars );
    // final addition
    pArgS[nArgA-1] = 0;
    for ( a = 0; a < nArgA; a++ )
        Wlc_BlastFullAdderCtrlCnf( pSat, -1, pArgC[a], pArgS[a], Carry, &Carry, &pRes[nArgB+a], piVars );
}
sat_solver * Wlc_BlastMultiplierCnfMain( int nBits )
{
    Vec_Int_t * vRes1 = Vec_IntAlloc( 2*nBits );
    Vec_Int_t * vRes2 = Vec_IntAlloc( 2*nBits );
    Vec_Int_t * vTemp = Vec_IntAlloc( 2*nBits );

    int * pArgA = ABC_ALLOC( int, nBits );
    int * pArgB = ABC_ALLOC( int, nBits );
    int i, Ent1, Ent2, nVars = 1 + 2*nBits;
    int nVarsAll = 1 + 4*nBits + 4*nBits*(nBits + 1);

    sat_solver * pSat = sat_solver_new();
    sat_solver_setnvars( pSat, nVarsAll );

    for ( i = 0; i < nBits; i++ )
        pArgA[i] = 1 + i, pArgB[i] = 1 + nBits + i;
    Wlc_BlastMultiplierCnf( pSat, pArgA, pArgB, nBits, nBits, vTemp, vRes1, &nVars );

    for ( i = 0; i < nBits; i++ )
        pArgA[i] = 1 + nBits + i, pArgB[i] = 1 + i;
    Wlc_BlastMultiplierCnf( pSat, pArgA, pArgB, nBits, nBits, vTemp, vRes2, &nVars );

    Vec_IntClear( vTemp );
    Vec_IntForEachEntryTwo( vRes1, vRes2, Ent1, Ent2, i )
    {
        Vec_IntPush( vTemp, Abc_Var2Lit(nVars, 0) );
        sat_solver_add_xor( pSat, Ent1, Ent2, nVars++, 0 );
    }
    assert( nVars == nVarsAll );
    sat_solver_addclause( pSat, Vec_IntArray(vTemp), Vec_IntLimit(vTemp) );

    ABC_FREE( pArgA );
    ABC_FREE( pArgB );
    Vec_IntFree( vRes1 );
    Vec_IntFree( vRes2 );
    Vec_IntFree( vTemp );
    return pSat;
}
void Wlc_BlastMultiplierCnfTest( int nBits )
{
    abctime clk = Abc_Clock();
    sat_solver * pSat = Wlc_BlastMultiplierCnfMain( nBits );
    int i, status = sat_solver_solve( pSat, NULL, NULL, 0, 0, 0, 0 );
    Sat_SolverWriteDimacs( pSat, "test_mult.cnf", NULL, NULL, 0 );
    for ( i = 0; i < sat_solver_nvars(pSat); i++ )
        printf( "%d=%d ", i, sat_solver_var_value(pSat, i) );
    printf( "\n" );

    printf( "Verifying for %d bits:  %s   ", nBits, status == l_True ? "SAT" : "UNSAT" );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    sat_solver_delete( pSat );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Wlc_ManGenAdderN( Gia_Man_t * p, int nLits, int * pLitsA, int * pLitsB, int Carry )
{
    extern void Wlc_BlastFullAdder( Gia_Man_t * pNew, int a, int b, int c, int * pc, int * ps );
    Vec_Int_t * vRes = Vec_IntStart( nLits + 1 ); 
    int i, * pRes = Vec_IntArray(vRes);
    for ( i = 0; i < nLits; i++ )
        Wlc_BlastFullAdder( p, pLitsA[i], pLitsB[i], Carry, &Carry, &pRes[i] );
    pRes[nLits] = Carry;
    return vRes;
}
Vec_Int_t * Wlc_ManGenAdder2_rec( Gia_Man_t * p, int nLits, int * pLitsA, int * pLitsB, int Carry, int Size )
{
    Vec_Int_t * vRes, * vRes0, * vRes1, * vRes2; int i, iCtrl;
    if ( nLits == Size )
        return Wlc_ManGenAdderN( p, nLits, pLitsA, pLitsB, Carry );
    vRes0 = Wlc_ManGenAdder2_rec( p, nLits/2, pLitsA, pLitsB, Carry, Size );
    vRes1 = Wlc_ManGenAdder2_rec( p, nLits/2, pLitsA + nLits/2, pLitsB + nLits/2, 0, Size );
    vRes2 = Wlc_ManGenAdder2_rec( p, nLits/2, pLitsA + nLits/2, pLitsB + nLits/2, 1, Size );
    vRes  = Vec_IntAlloc( nLits + 1 );
    Vec_IntAppend( vRes, vRes0 );
    iCtrl = Vec_IntPop( vRes );
    for ( i = 0; i <= nLits/2; i++ )
        Vec_IntPush( vRes, Gia_ManHashMux(p, iCtrl, Vec_IntEntry(vRes2, i), Vec_IntEntry(vRes1, i)) );
    assert( Vec_IntSize(vRes) == nLits + 1 );
    Vec_IntFree( vRes0 );
    Vec_IntFree( vRes1 );
    Vec_IntFree( vRes2 );
    return vRes;
}
Gia_Man_t * Wlc_ManGenAdder2( int nBits, int Size, int fSigned )
{
    Gia_Man_t * pTemp, * pNew; int n, i, iLit, nBitsAll;
    Vec_Int_t * vOuts, * vLits = Vec_IntAlloc( 1000 );
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "adder" );
    for ( nBitsAll = Size; nBitsAll < nBits; nBitsAll *= 2 )
        ;
    for ( n = 0; n < 2; n++ )
    {
        for ( i = 0; i < nBits; i++ )
            Vec_IntPush( vLits, Gia_ManAppendCi(pNew) );
        for ( ; i < nBitsAll; i++ )
            Vec_IntPush( vLits, fSigned ? Vec_IntEntry(vLits, nBits-1) : 0 );
    }
    Gia_ManHashAlloc( pNew );
    vOuts = Wlc_ManGenAdder2_rec( pNew, nBitsAll, Vec_IntEntryP(vLits, 0), Vec_IntEntryP(vLits, Vec_IntSize(vLits)/2), 0, Size );
    Gia_ManHashStop( pNew );
    Vec_IntForEachEntry( vOuts, iLit, i )
        Gia_ManAppendCo( pNew, iLit );
    Vec_IntFree( vLits );
    Vec_IntFree( vOuts );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

Vec_Int_t * Wlc_ManGenAdder_rec( Gia_Man_t * p, int nLits, int * pLitsA, int * pLitsB, int Carry, int Size )
{
    Vec_Int_t * vRes, * vRes0, * vRes1, * vRes2, * vRes3, * vRes4; int i, iCtrl;
    if ( nLits == Size )
        return Wlc_ManGenAdderN( p, nLits, pLitsA, pLitsB, Carry );
    assert( nLits % 3 == 0 );
    vRes0 = Wlc_ManGenAdder_rec( p, nLits/3, pLitsA + 0*nLits/3, pLitsB + 0*nLits/3, Carry, Size );
    vRes1 = Wlc_ManGenAdder_rec( p, nLits/3, pLitsA + 1*nLits/3, pLitsB + 1*nLits/3, 0, Size );
    vRes2 = Wlc_ManGenAdder_rec( p, nLits/3, pLitsA + 1*nLits/3, pLitsB + 1*nLits/3, 1, Size );
    vRes3 = Wlc_ManGenAdder_rec( p, nLits/3, pLitsA + 2*nLits/3, pLitsB + 2*nLits/3, 0, Size );
    vRes4 = Wlc_ManGenAdder_rec( p, nLits/3, pLitsA + 2*nLits/3, pLitsB + 2*nLits/3, 1, Size );
    vRes  = Vec_IntAlloc( nLits + 1 );
    Vec_IntAppend( vRes, vRes0 );
    iCtrl = Vec_IntPop( vRes );
    for ( i = 0; i <= nLits/3; i++ )
        Vec_IntPush( vRes, Gia_ManHashMux(p, iCtrl, Vec_IntEntry(vRes2, i), Vec_IntEntry(vRes1, i)) );
    iCtrl = Vec_IntPop( vRes );
    for ( i = 0; i <= nLits/3; i++ )
        Vec_IntPush( vRes, Gia_ManHashMux(p, iCtrl, Vec_IntEntry(vRes4, i), Vec_IntEntry(vRes3, i)) );
    assert( Vec_IntSize(vRes) == nLits + 1 );
    Vec_IntFree( vRes0 );
    Vec_IntFree( vRes1 );
    Vec_IntFree( vRes2 );
    Vec_IntFree( vRes3 );
    Vec_IntFree( vRes4 );
    return vRes;
}
Gia_Man_t * Wlc_ManGenAdder( int nBits )
{
    Gia_Man_t * pTemp, * pNew; int n, i, iLit, nBitsAll;
    Vec_Int_t * vOuts, * vLits = Vec_IntAlloc( 1000 );
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "adder" );
    for ( nBitsAll = 3; nBitsAll < nBits; nBitsAll *= 3 )
        ;
    for ( n = 0; n < 2; n++ )
    {
        for ( i = 0; i < nBits; i++ )
            Vec_IntPush( vLits, Gia_ManAppendCi(pNew) );
        for ( ; i < nBitsAll; i++ )
            Vec_IntPush( vLits, 0 );
    }
    Gia_ManHashAlloc( pNew );
    vOuts = Wlc_ManGenAdder_rec( pNew, nBitsAll, Vec_IntEntryP(vLits, 0), Vec_IntEntryP(vLits, Vec_IntSize(vLits)/2), 0, 3 );
    Gia_ManHashStop( pNew );
    Vec_IntForEachEntryStop( vOuts, iLit, i, nBits+1 )
        Gia_ManAppendCo( pNew, iLit );
    Vec_IntFree( vLits );
    Vec_IntFree( vOuts );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_BuildOne32( Gia_Man_t * p, int * pLitIn, int * pLitOut )
{
    Wlc_BlastFullAdder( p, pLitIn[0], pLitIn[1], pLitIn[2], &pLitIn[5],  &pLitOut[0] );
    Wlc_BlastFullAdder( p, pLitIn[3], pLitIn[4], pLitIn[5], &pLitOut[2], &pLitOut[1] );
}
void Wlc_BuildOne51( Gia_Man_t * p, int * pLitIn, int * pLitOut )
{
    int Lit00, Lit01, Lit11;
    Wlc_BlastFullAdder( p, pLitIn[0], pLitIn[1], pLitIn[2], &Lit01,      &Lit00      );
    Wlc_BlastFullAdder( p, pLitIn[3], pLitIn[4], Lit00,     &Lit11,      &pLitOut[0] );
    Wlc_BlastFullAdder( p, pLitIn[5], Lit01,     Lit11,     &pLitOut[2], &pLitOut[1] );
}
void Wlc_BuildOne6( Gia_Man_t * p, int * pLitIn, int Const1, int * pLitOut )
{
    int Lit00, Lit01, Lit10, Lit11, Lit12;
    Wlc_BlastFullAdder( p, pLitIn[0], pLitIn[1], pLitIn[2], &Lit01,     &Lit00       );
    Wlc_BlastFullAdder( p, pLitIn[3], pLitIn[4], pLitIn[5], &Lit11,     &Lit10       );
    Wlc_BlastFullAdder( p, Lit00,     Lit10,     Const1,    &Lit12,     &pLitOut[0]  );
    Wlc_BlastFullAdder( p, Lit01,     Lit11,     Lit12,     &pLitOut[2],&pLitOut[1]  );
}
Vec_Wec_t * Wlc_ManGenTree_iter( Gia_Man_t * p, Vec_Wec_t * vBits, int * pCounter )
{
    Vec_Wec_t * vBitsNew = Vec_WecStart( Vec_WecSize(vBits) ); 
    int i, k, pLitsIn[16], pLitsOut[16], Count = 0, fSimple = Vec_WecMaxLevelSize(vBits) <= 3;
    for ( i = 0; i < Vec_WecSize(vBits)-1; i++ )
    {
        Vec_Int_t * vBits0 = Vec_WecEntry(vBits, i);
        Vec_Int_t * vBits1 = Vec_WecEntry(vBits, i+1);
        if ( fSimple )
        {
            assert( Vec_IntSize(vBits0) <= 3 );
            for ( k = 0; Vec_IntSize(vBits0) > 0; k++ )
                pLitsIn[k] = Vec_IntPop( vBits0 );
            for (      ; k < 3; k++ )
                pLitsIn[k] = 0;
            assert( k == 3 );
            Wlc_BlastFullAdder( p, pLitsIn[0], pLitsIn[1], pLitsIn[2], &pLitsOut[1], &pLitsOut[0] );
            Vec_WecPush( vBitsNew, i+0, pLitsOut[0] );
            Vec_WecPush( vBitsNew, i+1, pLitsOut[1] );
            Count += 2;
            continue;
        }
        while ( Vec_IntSize(vBits0) >= 6 )
        {
            for ( k = 0; k < 6; k++ )
                pLitsIn[k] = Vec_IntPop( vBits0 );
            assert( k == 6 );
            Wlc_BuildOne6( p, pLitsIn, 0, pLitsOut );
            Vec_WecPush( vBitsNew, i+0, pLitsOut[0] );
            Vec_WecPush( vBitsNew, i+1, pLitsOut[1] );
            Vec_WecPush( vBitsNew, i+2, pLitsOut[2] );
            Count += 3;
        }
        if ( Vec_IntSize(vBits0) == 5 && Vec_IntSize(vBits1) > 0 )
        {
            for ( k = 0; Vec_IntSize(vBits0) > 0; k++ )
                pLitsIn[k] = Vec_IntPop( vBits0 );
            pLitsIn[k++] = Vec_IntPop( vBits1 );
            assert( k == 6 );
            Wlc_BuildOne51( p, pLitsIn, pLitsOut );
            Vec_WecPush( vBitsNew, i+0, pLitsOut[0] );
            Vec_WecPush( vBitsNew, i+1, pLitsOut[1] );
            Vec_WecPush( vBitsNew, i+2, pLitsOut[2] );
            Count += 3;
        }
        if ( Vec_IntSize(vBits0) == 5 && Vec_IntSize(vBits1) == 0 )
        {
            for ( k = 0; Vec_IntSize(vBits0) > 0; k++ )
                pLitsIn[k] = Vec_IntPop( vBits0 );
            pLitsIn[k++] = 0;
            assert( k == 6 );
            Wlc_BuildOne6( p, pLitsIn, 0, pLitsOut );
            Vec_WecPush( vBitsNew, i+0, pLitsOut[0] );
            Vec_WecPush( vBitsNew, i+1, pLitsOut[1] );
            Vec_WecPush( vBitsNew, i+2, pLitsOut[2] );
            Count += 3;
        }
        if ( Vec_IntSize(vBits0) == 4 && Vec_IntSize(vBits1) > 0 )
        {
            for ( k = 0; Vec_IntSize(vBits0) > 0; k++ )
                pLitsIn[k] = Vec_IntPop( vBits0 );
            pLitsIn[k++] = 0;
            pLitsIn[k++] = Vec_IntPop( vBits1 );
            assert( k == 6 );
            Wlc_BuildOne51( p, pLitsIn, pLitsOut );
            Vec_WecPush( vBitsNew, i+0, pLitsOut[0] );
            Vec_WecPush( vBitsNew, i+1, pLitsOut[1] );
            Vec_WecPush( vBitsNew, i+2, pLitsOut[2] );
            Count += 3;
        }
        if ( Vec_IntSize(vBits0) == 3 && Vec_IntSize(vBits1) >= 2 )
        {
            for ( k = 0; Vec_IntSize(vBits0) > 0; k++ )
                pLitsIn[k] = Vec_IntPop( vBits0 );
            pLitsIn[k++] = Vec_IntPop( vBits1 );
            pLitsIn[k++] = Vec_IntPop( vBits1 );
            assert( k == 5 );
            Wlc_BuildOne32( p, pLitsIn, pLitsOut );
            Vec_WecPush( vBitsNew, i+0, pLitsOut[0] );
            Vec_WecPush( vBitsNew, i+1, pLitsOut[1] );
            Vec_WecPush( vBitsNew, i+2, pLitsOut[2] );
            Count += 3;
        }
        if ( Vec_IntSize(vBits0) >= 3 )
        {
            for ( k = 0; k < 3; k++ )
                pLitsIn[k] = Vec_IntPop( vBits0 );
            assert( k == 3 );
            Wlc_BlastFullAdder( p, pLitsIn[0], pLitsIn[1], pLitsIn[2], &pLitsOut[1], &pLitsOut[0] );
            Vec_WecPush( vBitsNew, i+0, pLitsOut[0] );
            Vec_WecPush( vBitsNew, i+1, pLitsOut[1] );
            Count += 2;
        }
/*
        if ( Vec_IntSize(vBits0) == 2 )
        {
            Vec_IntClear( vBits0 );
            Vec_WecPush( vBitsNew, i+0, 0 );
            Vec_WecPush( vBitsNew, i+1, 0 );
            Count += 2;
        }
*/
        for ( k = 0; Vec_IntSize(vBits0) > 0; k++ )
            Vec_WecPush( vBitsNew, i, Vec_IntPop(vBits0) );
    }
    if ( pCounter )
        *pCounter = Count;
    return vBitsNew;
}
void Wlc_ManGenTreeOne( Gia_Man_t * pNew, Vec_Wec_t * vBits0, int fMult, int fVerbose )
{
    extern int Wlc_BlastAdder( Gia_Man_t * pNew, int * pAdd0, int * pAdd1, int nBits, int Carry ); // result is in pAdd0

    Vec_Wec_t * vTemp, * vBits = Vec_WecDup( vBits0 );
    Vec_Int_t * vOuts = Vec_IntAlloc( 1000 ), * vOuts2;
    Vec_Int_t * vLits0 = Vec_IntAlloc( 1000 ); 
    Vec_Int_t * vLits1 = Vec_IntAlloc( 1000 ); 
    int i, iLit, nBitsAll = 0, CounterAll = 0, Counter = 1;
    for ( i = 0; Counter && i < 1000; i++ )
    {
        if ( fVerbose ) printf( "LEVEL %d\n", i );
        if ( fVerbose ) Vec_WecPrint( vBits, 0 );
        if ( Vec_WecMaxLevelSize(vBits) <= 2 )
            break;
        vBits = Wlc_ManGenTree_iter( pNew, vTemp = vBits, &Counter );
        Vec_WecFree( vTemp );
        CounterAll += Counter;
    }
    printf( "Total count = %d.\n", CounterAll );
    if ( !fMult )
    {
        int Carry;
/*
        Vec_WecForEachLevel( vBits, vOuts2, i )
        {
            if ( i == 10 )
                break;
            if ( i == 0 )
            {
                assert( Vec_IntSize(vOuts2) == 1 );
                Vec_IntPush( vOuts, Vec_IntPop(vOuts2) );
                continue;
            }
            assert( Vec_IntSize(vOuts2) == 1 || Vec_IntSize(vOuts2) == 2 );
            Vec_IntPush( vLits0, Vec_IntPop(vOuts2) );
            if ( Vec_IntSize(vOuts2) == 1 )
                Vec_IntPush( vLits1, Vec_IntPop(vOuts2) );
            else
            {
                Vec_IntPush( vLits1, 0 );
            }
        }
        assert( Vec_IntSize(vLits0) == 9 );
        assert( Vec_IntSize(vLits1) == 9 );
*/
        Vec_WecForEachLevel( vBits, vOuts2, i )
        {
            if ( Vec_IntSize(vOuts2) == 0 )
                break;
            assert( Vec_IntSize(vOuts2) == 1 || Vec_IntSize(vOuts2) == 2 );
            Vec_IntPush( vLits0, Vec_IntPop(vOuts2) );
            if ( Vec_IntSize(vOuts2) == 1 )
                Vec_IntPush( vLits1, Vec_IntPop(vOuts2) );
            else
                Vec_IntPush( vLits1, 0 );
        }
        printf( "The adder size is %d.\n", Vec_IntSize(vLits0) );
        Vec_IntShrink( vLits0, 11 );
        Vec_IntShrink( vLits1, 11 );

//        vOuts2 = Wlc_ManGenAdder_rec( pNew, 9, Vec_IntArray(vLits0), Vec_IntArray(vLits1), 0, 3 );
//        Vec_IntAppend( vOuts, vOuts2 );
//        Vec_IntFree( vOuts2 );

        Carry = Wlc_BlastAdder( pNew, Vec_IntArray(vLits0), Vec_IntArray(vLits1), 11, 0 );
        Vec_IntAppend( vOuts, vLits0 );
        Vec_IntPush( vOuts, Carry );


        Gia_ManAppendCo( pNew, Vec_IntEntry(vOuts, 11) );
    }
    else
    {
        Vec_WecForEachLevel( vBits, vOuts2, i )
        {
            if ( Vec_IntSize(vOuts2) == 0 )
                break;
            assert( Vec_IntSize(vOuts2) == 1 || Vec_IntSize(vOuts2) == 2 );
            Vec_IntPush( vLits0, Vec_IntPop(vOuts2) );
            if ( Vec_IntSize(vOuts2) == 1 )
                Vec_IntPush( vLits1, Vec_IntPop(vOuts2) );
            else
                Vec_IntPush( vLits1, 0 );
        }
        printf( "The adder size is %d.\n", Vec_IntSize(vLits0) );
        Vec_IntShrink( vLits0, Gia_ManCiNum(pNew)+1 ); // mult
        Vec_IntShrink( vLits1, Gia_ManCiNum(pNew)+1 ); // mult

        for ( nBitsAll = 3; nBitsAll < Vec_IntSize(vLits0); nBitsAll *= 3 )
            ;
        for ( i = Vec_IntSize(vLits0); i < nBitsAll; i++ )
        {
            Vec_IntPush( vLits0, 0 );
            Vec_IntPush( vLits1, 0 );
        }
        assert( Vec_IntSize(vLits0) == nBitsAll );
        assert( Vec_IntSize(vLits1) == nBitsAll );

        vOuts2 = Wlc_ManGenAdder_rec( pNew, nBitsAll, Vec_IntArray(vLits0), Vec_IntArray(vLits1), 0, 3 );
        Vec_IntAppend( vOuts, vOuts2 );
        Vec_IntFree( vOuts2 );
        //Carry = Wlc_BlastAdder( pNew, Vec_IntArray(vLits0), Vec_IntArray(vLits1), nBitsAll, 0 );
        //Vec_IntAppend( vOuts, vLits0 );
        //Vec_IntPush( vOuts, Carry );

        Vec_IntShrink( vOuts, Gia_ManCiNum(pNew) ); // mult
        //Vec_IntShrink( vOuts, Gia_ManCiNum(pNew)/2 );

        Vec_IntForEachEntry( vOuts, iLit, i )
            Gia_ManAppendCo( pNew, iLit );
    }

    Vec_IntFree( vOuts );
    Vec_IntFree( vLits0 );
    Vec_IntFree( vLits1 );
    Vec_WecFree( vBits );
}
Gia_Man_t * Wlc_ManGenTree( int nInputs, int Value, int nBits, int fVerbose )
{
    Gia_Man_t * pTemp, * pNew; int i, Counter = 0;
    Vec_Wec_t * vBits = Vec_WecStart( nBits+2 );
    for ( i = 0; i < nBits+2; i++ )
        Vec_WecPush( vBits, i, (Value >> i) & 1 );
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "tree" );
    for ( i = 0; i < nInputs; i++ )
        Vec_WecPush( vBits, 0, Gia_ManAppendCi(pNew) );
    Gia_ManHashAlloc( pNew );
    Wlc_ManGenTreeOne( pNew, vBits, 0, fVerbose );
    Gia_ManHashStop( pNew );
    Vec_WecFree( vBits );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Wlc_ManGenProd( int nInputs, int fVerbose )
{
    extern void Wlc_BlastBooth( Gia_Man_t * pNew, int * pArgA, int * pArgB, int nArgA, int nArgB, Vec_Int_t * vRes, int fSigned, int fCla, Vec_Wec_t ** pvProds, int fVerbose );
    Vec_Int_t * vIns = Vec_IntAlloc( 2*nInputs );
    Gia_Man_t * pTemp, * pNew; 
    Vec_Wec_t * vProds; int i;
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "tree" );

    for ( i = 0; i < 2*nInputs; i++ )
        Vec_IntPush( vIns, Gia_ManAppendCi(pNew) );
    //for ( i = 0; i < nInputs; i++ )
    //    Vec_IntPush( vIns, Gia_ManAppendCi(pNew) );
    //for ( i = 0; i < nInputs; i++ )
    //    Vec_IntPush( vIns, Vec_IntEntry(vIns, i) );

    Gia_ManHashAlloc( pNew );
    Wlc_BlastBooth( pNew, Vec_IntArray(vIns), Vec_IntArray(vIns)+nInputs, nInputs, nInputs, NULL, 0, 0, &vProds, 0 );
    Wlc_ManGenTreeOne( pNew, vProds, 1, fVerbose );
    Gia_ManHashStop( pNew );
    Vec_WecFree( vProds );
    Vec_IntFree( vIns );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_PrintTernary( FILE * pFile, word * pFunc, word * pCare, int nBits )
{
    int i;
    for ( i = nBits-1; i >= 0; i-- )
        if ( Abc_TtGetBit(pCare, i) )
            fprintf( pFile, "%c", '0' + Abc_TtGetBit(pFunc, i) );
        else
            fprintf( pFile, "-" );
    fprintf( pFile, "\n" );
}
void Wlc_AdderTreeGen( int n )
{
    word Care[1<<10] = {0}; 
    word Truth[8][1<<10] = {{0}};
    int nIns = 0, pIns[16][2] = {{0}};
    int i, k, x, Res, Mint, nMints = 1 << (n*n);
    assert( n >= 2 && n <= 4 );
    for ( x = 0; x < 2*n; x++ )
    {
        for ( i = 0; i < n; i++ )
        for ( k = 0; k < n; k++ )
            if ( i + k == x )
                pIns[nIns][0] = i, pIns[nIns][1] = k, nIns++;
    }
    for ( x = 0; x < nIns; x++ )
        printf( "(%d, %d) ", pIns[x][0], pIns[x][1] );
    printf( "\n" );
    for ( i = 0; i < (1<<n); i++ )
    for ( k = 0; k < (1<<n); k++ )
    {
        Mint = 0;
        for ( x = 0; x < nIns; x++ )
            if ( ((i >> pIns[x][0]) & 1) && ((k >> pIns[x][1]) & 1) )
                Mint |= 1 << x;
        assert( Mint < (1<<16) );
        Abc_TtSetBit( Care, Mint );

        Res = i * k;
        for ( x = 0; x < 2*n; x++ )
            if ( (Res >> x) & 1 )
                Abc_TtSetBit( Truth[x], Mint );
    }
    if ( n == 2 )
    {
        Care[0] = Abc_Tt6Stretch( Care[0], n*n );
        for ( i = 0; i < 2*n; i++ )
            Truth[i][0] = Abc_Tt6Stretch( Truth[i][0], n*n );
        nMints = 64;
    }
    for ( x = 0; x < nMints; x++ )
        printf( "%d", Abc_TtGetBit(Care, x) );
    printf( "\n\n" );
    for ( i = 0; i < 2*n; i++, printf( "\n" ) )
    for ( x = 0; x < nMints; x++ )
        printf( "%d", Abc_TtGetBit(Truth[i], x) );
    if ( 1 )
    {
        FILE * pFile = fopen( "tadd.truth", "wb" );
        for ( i = 0; i < 2*n; i++ )
            Extra_PrintTernary( pFile, Truth[i], Care, nMints );
        fclose( pFile );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

