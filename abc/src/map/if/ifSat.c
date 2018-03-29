/**CFile****************************************************************

  FileName    [ifSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [SAT-based evaluation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifSat.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"
#include "sat/bsat/satSolver.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    [Builds SAT instance for the given structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * If_ManSatBuildXY( int nLutSize )
{
    int nMintsL = (1 << nLutSize);
    int nMintsF = (1 << (2 * nLutSize - 1));
    int nVars   = 2 * nMintsL + nMintsF;
    int iVarP0  = 0;           // LUT0 parameters (total nMintsL)
    int iVarP1  = nMintsL;     // LUT1 parameters (total nMintsL)
    int m,iVarM = 2 * nMintsL; // MUX vars        (total nMintsF)
    sat_solver * p = sat_solver_new();
    sat_solver_setnvars( p, nVars );
    for ( m = 0; m < nMintsF; m++ )
        sat_solver_add_mux( p, 
            iVarM  + m, 
            iVarP0 + m % nMintsL, 
            iVarP1 + 2 * (m / nMintsL) + 1, 
            iVarP1 + 2 * (m / nMintsL), 
            0, 0, 0, 0 );
    return p;
}
void * If_ManSatBuildXYZ( int nLutSize )
{
    int nMintsL = (1 << nLutSize);
    int nMintsF = (1 << (3 * nLutSize - 2));
    int nVars   = 3 * nMintsL + nMintsF;
    int iVarP0  = 0;           // LUT0 parameters (total nMintsL)
    int iVarP1  = nMintsL;     // LUT1 parameters (total nMintsL)
    int iVarP2  = 2 * nMintsL; // LUT2 parameters (total nMintsL)
    int m,iVarM = 3 * nMintsL; // MUX vars        (total nMintsF)
    sat_solver * p = sat_solver_new();
    sat_solver_setnvars( p, nVars );
    for ( m = 0; m < nMintsF; m++ )
        sat_solver_add_mux41( p, 
            iVarM  + m,
            iVarP0 + m % nMintsL, 
            iVarP1 + (m >> nLutSize) % nMintsL, 
            iVarP2 + 4 * (m >> (2 * nLutSize)) + 0, 
            iVarP2 + 4 * (m >> (2 * nLutSize)) + 1, 
            iVarP2 + 4 * (m >> (2 * nLutSize)) + 2, 
            iVarP2 + 4 * (m >> (2 * nLutSize)) + 3 );
    return p;
}
void If_ManSatUnbuild( void * p )
{
    if ( p )
        sat_solver_delete( (sat_solver *)p );
}

/**Function*************************************************************

  Synopsis    [Verification for 6-input function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static word If_ManSat6ComposeLut4( int t, word f[4], int k )
{
    int m, v, nMints = (1 << k);
    word c, r = 0;
    assert( k <= 4 );
    for ( m = 0; m < nMints; m++ )
    {
        if ( !((t >> m) & 1) )
            continue;
        c = ~(word)0;
        for ( v = 0; v < k; v++ )
            c &= ((m >> v) & 1) ? f[v] : ~f[v];
        r |= c;
    }
    return r;
}
word If_ManSat6Truth( word uBound, word uFree, int * pBSet, int nBSet, int * pSSet, int nSSet, int * pFSet, int nFSet )
{
    word r, q, f[4];
    int i, k = 0;
    // bound set vars
    for ( i = 0; i < nSSet; i++ )
        f[k++] = s_Truths6[pSSet[i]];
    for ( i = 0; i < nBSet; i++ )
        f[k++] = s_Truths6[pBSet[i]];
    q = If_ManSat6ComposeLut4( (int)(uBound & 0xffff), f, k );
    // free set vars
    k = 0;
    f[k++] = q;
    for ( i = 0; i < nSSet; i++ )
        f[k++] = s_Truths6[pSSet[i]];
    for ( i = 0; i < nFSet; i++ )
        f[k++] = s_Truths6[pFSet[i]];
    r = If_ManSat6ComposeLut4( (int)(uFree & 0xffff), f, k );
    return r;
}

/**Function*************************************************************

  Synopsis    [Returns config string for the given truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManSatCheckXY( void * pSat, int nLutSize, word * pTruth, int nVars, unsigned uSet, word * pTBound, word * pTFree, Vec_Int_t * vLits )
{
    sat_solver * p = (sat_solver *)pSat;
    int iBSet, nBSet = 0, pBSet[IF_MAX_FUNC_LUTSIZE];
    int iSSet, nSSet = 0, pSSet[IF_MAX_FUNC_LUTSIZE];
    int iFSet, nFSet = 0, pFSet[IF_MAX_FUNC_LUTSIZE];
    int nMintsL = (1 << nLutSize);
    int nMintsF = (1 << (2 * nLutSize - 1));
    int v, Value, m, mNew, nMintsFNew, nMintsLNew;
    word Res;
    // collect variable sets
    Dau_DecSortSet( uSet, nVars, &nBSet, &nSSet, &nFSet );
    assert( nBSet + nSSet + nFSet == nVars );
    // check variable bounds
    assert( nSSet + nBSet <= nLutSize );
    assert( nLutSize + nSSet + nFSet <= 2*nLutSize - 1 );
    nMintsFNew = (1 << (nLutSize + nSSet + nFSet));
    // remap minterms
    Vec_IntFill( vLits, nMintsF, -1 );
    for ( m = 0; m < (1 << nVars); m++ )
    {
        mNew = iBSet = iSSet = iFSet = 0;
        for ( v = 0; v < nVars; v++ )
        {
            Value = ((uSet >> (v << 1)) & 3);
            if ( Value == 0 ) // FS
            {
                if ( ((m >> v) & 1) )
                    mNew |= 1 << (nLutSize + nSSet + iFSet), pFSet[iFSet] = v;
                iFSet++;
            }
            else if ( Value == 1 ) // BS
            {
                if ( ((m >> v) & 1) )
                    mNew |= 1 << (nSSet + iBSet), pBSet[iBSet] = v;
                iBSet++;
            }
            else if ( Value == 3 ) // SS
            {
                if ( ((m >> v) & 1) )
                {
                    mNew |= 1 << iSSet;
                    mNew |= 1 << (nLutSize + iSSet);
                    pSSet[iSSet] = v;
                }
                iSSet++;
            }
            else assert( Value == 0 );
        }
        assert( iBSet == nBSet && iFSet == nFSet );
        assert( Vec_IntEntry(vLits, mNew) == -1 );
        Vec_IntWriteEntry( vLits, mNew, Abc_TtGetBit(pTruth, m) );
    }
    // find assumptions
    v = 0;
    Vec_IntForEachEntry( vLits, Value, m )
    {
//        printf( "%d", (Value >= 0) ? Value : 2 );
        if ( Value >= 0 )
            Vec_IntWriteEntry( vLits, v++, Abc_Var2Lit(2 * nMintsL + m, !Value) );
    }
    Vec_IntShrink( vLits, v );
//    printf( " %d\n", Vec_IntSize(vLits) );
    // run SAT solver
    Value = sat_solver_solve( p, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits), 0, 0, 0, 0 );
    if ( Value != l_True )
        return 0;
    if ( pTBound && pTFree )
    {
        // collect config
        assert( nSSet + nBSet <= nLutSize );
        *pTBound = 0;
        nMintsLNew = (1 << (nSSet + nBSet));
        for ( m = 0; m < nMintsLNew; m++ )
            if ( sat_solver_var_value(p, m) )
                Abc_TtSetBit( pTBound, m );
        *pTBound = Abc_Tt6Stretch( *pTBound, nSSet + nBSet );
        // collect configs
        assert( nSSet + nFSet + 1 <= nLutSize );
        *pTFree = 0;
        nMintsLNew = (1 << (1 + nSSet + nFSet));
        for ( m = 0; m < nMintsLNew; m++ )
            if ( sat_solver_var_value(p, nMintsL+m) )
                Abc_TtSetBit( pTFree, m );
        *pTFree = Abc_Tt6Stretch( *pTFree, 1 + nSSet + nFSet );
        if ( nVars != 6 || nLutSize != 4 )
            return 1;
        // verify the result
        Res = If_ManSat6Truth( *pTBound, *pTFree, pBSet, nBSet, pSSet, nSSet, pFSet, nFSet );
        if ( pTruth[0] != Res )
        {
            Dau_DsdPrintFromTruth( pTruth,  nVars );
            Dau_DsdPrintFromTruth( &Res,    nVars );
            Dau_DsdPrintFromTruth( pTBound, nSSet+nBSet );
            Dau_DsdPrintFromTruth( pTFree,  nSSet+nFSet+1 );
            printf( "Verification failed!\n" );
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns config string for the given truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned If_ManSatCheckXYall_int( void * pSat, int nLutSize, word * pTruth, int nVars, Vec_Int_t * vLits )
{
    unsigned uSet = 0;
    int nTotal = 2 * nLutSize - 1;
    int nShared = nTotal - nVars;
    int i[6], s[4];
    assert( nLutSize >= 2 && nLutSize <= 6 );
    assert( nLutSize < nVars && nVars <= nTotal );
    assert( nShared >= 0 && nShared < nLutSize - 1 );
    if ( nLutSize == 2 )
    {
        assert( nShared == 0 );
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1]));
            if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet, NULL, NULL, vLits) )
                return uSet;
        }
    }
    else if ( nLutSize == 3 )
    {
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2]));
            if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet, NULL, NULL, vLits) )
                return uSet;
        }
        if ( nShared < 1 )
            return 0;
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2]));
            for ( s[0] = 0; s[0] < nLutSize; s[0]++ )
                if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet | (3 << (2*i[s[0]])), NULL, NULL, vLits) )
                    return uSet | (3 << (2*i[s[0]]));
        }
    }
    else if ( nLutSize == 4 )
    {
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        for ( i[3] = i[2]+1; i[3] < nVars; i[3]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2])) | (1 << (2*i[3]));
            if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet, NULL, NULL, vLits) )
                return uSet;
        }
        if ( nShared < 1 )
            return 0;
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        for ( i[3] = i[2]+1; i[3] < nVars; i[3]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2])) | (1 << (2*i[3]));
            for ( s[0] = 0; s[0] < nLutSize; s[0]++ )
                if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet | (3 << (2*i[s[0]])), NULL, NULL, vLits) )
                    return uSet | (3 << (2*i[s[0]]));
        }
        if ( nShared < 2 )
            return 0;
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        for ( i[3] = i[2]+1; i[3] < nVars; i[3]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2])) | (1 << (2*i[3]));
            {
                for ( s[0] = 0;      s[0] < nLutSize; s[0]++ )
                for ( s[1] = s[0]+1; s[1] < nLutSize; s[1]++ )
                    if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet | (3 << (2*i[s[0]])) | (3 << (2*i[s[1]])), NULL, NULL, vLits) )
                        return uSet | (3 << (2*i[s[0]])) | (3 << (2*i[s[1]]));
            }
        }
    }
    else if ( nLutSize == 5 )
    {
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        for ( i[3] = i[2]+1; i[3] < nVars; i[3]++ )
        for ( i[4] = i[3]+1; i[4] < nVars; i[4]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2])) | (1 << (2*i[3])) | (1 << (2*i[4]));
            if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet, NULL, NULL, vLits) )
                return uSet;
        }
        if ( nShared < 1 )
            return 0;
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        for ( i[3] = i[2]+1; i[3] < nVars; i[3]++ )
        for ( i[4] = i[3]+1; i[4] < nVars; i[4]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2])) | (1 << (2*i[3])) | (1 << (2*i[4]));
            for ( s[0] = 0; s[0] < nLutSize; s[0]++ )
                if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet | (3 << (2*i[s[0]])), NULL, NULL, vLits) )
                    return uSet | (3 << (2*i[s[0]]));
        }
        if ( nShared < 2 )
            return 0;
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        for ( i[3] = i[2]+1; i[3] < nVars; i[3]++ )
        for ( i[4] = i[3]+1; i[4] < nVars; i[4]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2])) | (1 << (2*i[3])) | (1 << (2*i[4]));
            for ( s[0] = 0;      s[0] < nLutSize; s[0]++ )
            for ( s[1] = s[0]+1; s[1] < nLutSize; s[1]++ )
                if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet | (3 << (2*i[s[0]])) | (3 << (2*i[s[1]])), NULL, NULL, vLits) )
                    return uSet | (3 << (2*i[s[0]])) | (3 << (2*i[s[1]]));
        }
        if ( nShared < 3 )
            return 0;
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        for ( i[3] = i[2]+1; i[3] < nVars; i[3]++ )
        for ( i[4] = i[3]+1; i[4] < nVars; i[4]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2])) | (1 << (2*i[3])) | (1 << (2*i[4]));
            for ( s[0] = 0;      s[0] < nLutSize; s[0]++ )
            for ( s[1] = s[0]+1; s[1] < nLutSize; s[1]++ )
            for ( s[2] = s[1]+1; s[2] < nLutSize; s[2]++ )
                if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet | (3 << (2*i[s[0]])) | (3 << (2*i[s[1]])) | (3 << (2*i[s[2]])), NULL, NULL, vLits) )
                    return uSet | (3 << (2*i[s[0]])) | (3 << (2*i[s[1]])) | (3 << (2*i[s[2]]));
        }
    }
    else if ( nLutSize == 6 )
    {
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        for ( i[3] = i[2]+1; i[3] < nVars; i[3]++ )
        for ( i[4] = i[3]+1; i[4] < nVars; i[4]++ )
        for ( i[5] = i[4]+1; i[5] < nVars; i[5]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2])) | (1 << (2*i[3])) | (1 << (2*i[4])) | (1 << (2*i[5]));
            if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet, NULL, NULL, vLits) )
                return uSet;
        }
        if ( nShared < 1 )
            return 0;
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        for ( i[3] = i[2]+1; i[3] < nVars; i[3]++ )
        for ( i[4] = i[3]+1; i[4] < nVars; i[4]++ )
        for ( i[5] = i[4]+1; i[5] < nVars; i[5]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2])) | (1 << (2*i[3])) | (1 << (2*i[4])) | (1 << (2*i[5]));
            for ( s[0] = 0; s[0] < nLutSize; s[0]++ )
                if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet | (3 << (2*i[s[0]])), NULL, NULL, vLits) )
                    return uSet | (3 << (2*i[s[0]]));
        }
        if ( nShared < 2 )
            return 0;
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        for ( i[3] = i[2]+1; i[3] < nVars; i[3]++ )
        for ( i[4] = i[3]+1; i[4] < nVars; i[4]++ )
        for ( i[5] = i[4]+1; i[5] < nVars; i[5]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2])) | (1 << (2*i[3])) | (1 << (2*i[4])) | (1 << (2*i[5]));
            for ( s[0] = 0;      s[0] < nLutSize; s[0]++ )
            for ( s[1] = s[0]+1; s[1] < nLutSize; s[1]++ )
                if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet | (3 << (2*i[s[0]])) | (3 << (2*i[s[1]])), NULL, NULL, vLits) )
                    return uSet | (3 << (2*i[s[0]])) | (3 << (2*i[s[1]]));
        }
        if ( nShared < 3 )
            return 0;
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        for ( i[3] = i[2]+1; i[3] < nVars; i[3]++ )
        for ( i[4] = i[3]+1; i[4] < nVars; i[4]++ )
        for ( i[5] = i[4]+1; i[5] < nVars; i[5]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2])) | (1 << (2*i[3])) | (1 << (2*i[4])) | (1 << (2*i[5]));
            for ( s[0] = 0;      s[0] < nLutSize; s[0]++ )
            for ( s[1] = s[0]+1; s[1] < nLutSize; s[1]++ )
            for ( s[2] = s[1]+1; s[2] < nLutSize; s[2]++ )
                if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet | (3 << (2*i[s[0]])) | (3 << (2*i[s[1]])) | (3 << (2*i[s[2]])), NULL, NULL, vLits) )
                    return uSet | (3 << (2*i[s[0]])) | (3 << (2*i[s[1]])) | (3 << (2*i[s[2]]));
        }
        if ( nShared < 4 )
            return 0;
        for ( i[0] = 0;      i[0] < nVars; i[0]++ )
        for ( i[1] = i[0]+1; i[1] < nVars; i[1]++ )
        for ( i[2] = i[1]+1; i[2] < nVars; i[2]++ )
        for ( i[3] = i[2]+1; i[3] < nVars; i[3]++ )
        for ( i[4] = i[3]+1; i[4] < nVars; i[4]++ )
        for ( i[5] = i[4]+1; i[5] < nVars; i[5]++ )
        {
            uSet = (1 << (2*i[0])) | (1 << (2*i[1])) | (1 << (2*i[2])) | (1 << (2*i[3])) | (1 << (2*i[4])) | (1 << (2*i[5]));
            for ( s[0] = 0;      s[0] < nLutSize; s[0]++ )
            for ( s[1] = s[0]+1; s[1] < nLutSize; s[1]++ )
            for ( s[2] = s[1]+1; s[2] < nLutSize; s[2]++ )
            for ( s[3] = s[1]+1; s[3] < nLutSize; s[3]++ )
                if ( If_ManSatCheckXY(pSat, nLutSize, pTruth, nVars, uSet | (3 << (2*i[s[0]])) | (3 << (2*i[s[1]])) | (3 << (2*i[s[2]])) | (3 << (2*i[s[3]])), NULL, NULL, vLits) )
                    return uSet | (3 << (2*i[s[0]])) | (3 << (2*i[s[1]])) | (3 << (2*i[s[2]])) | (3 << (2*i[s[3]]));
        }
    }
    return 0;
}
unsigned If_ManSatCheckXYall( void * pSat, int nLutSize, word * pTruth, int nVars, Vec_Int_t * vLits )
{
    unsigned uSet = If_ManSatCheckXYall_int( pSat, nLutSize, pTruth, nVars, vLits );
//    Dau_DecPrintSet( uSet, nVars, 1 );
    return uSet;
}

/**Function*************************************************************

  Synopsis    [Test procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManSatTest2()
{
    int nVars = 6;
    int nLutSize = 4;
    sat_solver * p = (sat_solver *)If_ManSatBuildXY( nLutSize );
//    char * pDsd = "(abcdefg)";
//    char * pDsd = "([a!bc]d!e)";
    char * pDsd = "0123456789ABCDEF{abcdef}";
    word * pTruth = Dau_DsdToTruth( pDsd, nVars );
    word uBound, uFree;
    Vec_Int_t * vLits = Vec_IntAlloc( 100 );
//    unsigned uSet = (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6);
//    unsigned uSet = (3 << 0) | (1 << 2) | (1 << 8) | (1 << 4);
    unsigned uSet = (1 << 0) | (3 << 2) | (1 << 4) | (1 << 6);
    int RetValue = If_ManSatCheckXY( p, nLutSize, pTruth, nVars, uSet, &uBound, &uFree, vLits );
    assert( RetValue );
    
//    Abc_TtPrintBinary( pTruth, nVars );
//    Abc_TtPrintBinary( &uBound, nLutSize );
//    Abc_TtPrintBinary( &uFree, nLutSize );

    Dau_DsdPrintFromTruth( pTruth, nVars );
    Dau_DsdPrintFromTruth( &uBound, nLutSize );
    Dau_DsdPrintFromTruth( &uFree, nLutSize );
    sat_solver_delete(p);
    Vec_IntFree( vLits );
}
void If_ManSatTest3()
{
    int nVars = 6;
    int nLutSize = 4;
    sat_solver * p = (sat_solver *)If_ManSatBuildXY( nLutSize );
//    char * pDsd = "(abcdefg)";
//    char * pDsd = "([a!bc]d!e)";
    char * pDsd = "0123456789ABCDEF{abcdef}";
    word * pTruth = Dau_DsdToTruth( pDsd, nVars );
    Vec_Int_t * vLits = Vec_IntAlloc( 100 );
//    unsigned uSet = (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6);
//    unsigned uSet = (3 << 0) | (1 << 2) | (1 << 8) | (1 << 4);
    unsigned uSet = (1 << 0) | (3 << 2) | (1 << 4) | (1 << 6);
    uSet = If_ManSatCheckXYall( p, nLutSize, pTruth, nVars, vLits );
    Dau_DecPrintSet( uSet, nVars, 1 );

    sat_solver_delete(p);
    Vec_IntFree( vLits );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

