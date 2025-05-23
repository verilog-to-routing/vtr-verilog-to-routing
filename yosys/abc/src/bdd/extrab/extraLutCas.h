/**CFile****************************************************************

  FileName    [extraLutCas.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [LUT cascade decomposition.]

  Description [LUT cascade decomposition.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 6, 2024.]

  Revision    [$Id: extraLutCas.h,v 1.00 2024/08/06 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__misc__extra__extra_lutcas_h
#define ABC__misc__extra__extra_lutcas_h

#ifdef _WIN32
#define inline __inline // compatible with MS VS 6.0
#endif

#ifdef _MSC_VER
#  include <intrin.h>
#  define __builtin_popcount __popcnt
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "aig/miniaig/miniaig.h"
#include "bdd/cudd/cuddInt.h"
#include "bdd/dsd/dsd.h"

ABC_NAMESPACE_HEADER_START

/*
    The decomposed structure of the LUT cascade is represented as an array of 64-bit integers (words).
    The first word in the record is the number of LUT info blocks in the record, which follow one by one.
    Each LUT info block contains the following:
    - the number of words in this block
    - the number of fanins
    - the list of fanins
    - the variable ID of the output (can be one of the fanin variables)
    - truth tables (one word for 6 vars or less; more words as needed for more than 6 vars)
    For a 6-input node, the LUT info block takes 10 words (block size, fanin count, 6 fanins, output ID, truth table).
    For a 4-input node, the LUT info block takes  8 words (block size, fanin count, 4 fanins, output ID, truth table).
    If the LUT cascade contains a 6-LUT followed by a 4-LUT, the record contains 1+10+8=19 words.
*/

#define Mini_AigForEachNonPo( p, i )  for (i = 1; i < Mini_AigNodeNum(p); i++) if ( Mini_AigNodeIsPo(p, i) ) {} else 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_LutCasCollapseDeref( DdManager * dd, Vec_Ptr_t * vFuncs )
{
    DdNode * bFunc; int i;
    Vec_PtrForEachEntry( DdNode *, vFuncs, bFunc, i )
        if ( bFunc )
            Cudd_RecursiveDeref( dd, bFunc );
    Vec_PtrFree( vFuncs );
}
Vec_Ptr_t * Abc_LutCasCollapse( Mini_Aig_t * p, DdManager * dd, int nBddLimit, int fVerbose )
{
    DdNode * bFunc0, * bFunc1, * bFunc;  int Id, nOuts = 0;
    Vec_Ptr_t * vFuncs = Vec_PtrStart( Mini_AigNodeNum(p) );
    Vec_PtrWriteEntry( vFuncs, 0, Cudd_ReadLogicZero(dd) ), Cudd_Ref(Cudd_ReadLogicZero(dd));
    Mini_AigForEachPi( p, Id )
      Vec_PtrWriteEntry( vFuncs, Id, Cudd_bddIthVar(dd,Id-1) ), Cudd_Ref(Cudd_bddIthVar(dd,Id-1));
    Mini_AigForEachAnd( p, Id ) {
        bFunc0 = Cudd_NotCond( (DdNode *)Vec_PtrEntry(vFuncs, Mini_AigLit2Var(Mini_AigNodeFanin0(p, Id))), Mini_AigLitIsCompl(Mini_AigNodeFanin0(p, Id)) );
        bFunc1 = Cudd_NotCond( (DdNode *)Vec_PtrEntry(vFuncs, Mini_AigLit2Var(Mini_AigNodeFanin1(p, Id))), Mini_AigLitIsCompl(Mini_AigNodeFanin1(p, Id)) );
        bFunc  = Cudd_bddAndLimit( dd, bFunc0, bFunc1, nBddLimit );  
        if ( bFunc == NULL )
        {
            Abc_LutCasCollapseDeref( dd, vFuncs );
            return NULL;
        }        
        Cudd_Ref( bFunc );
        Vec_PtrWriteEntry( vFuncs, Id, bFunc );
    }
    Mini_AigForEachPo( p, Id ) {
        bFunc0 = Cudd_NotCond( (DdNode *)Vec_PtrEntry(vFuncs, Mini_AigLit2Var(Mini_AigNodeFanin0(p, Id))), Mini_AigLitIsCompl(Mini_AigNodeFanin0(p, Id)) );
        Vec_PtrWriteEntry( vFuncs, Id, bFunc0 ); Cudd_Ref( bFunc0 );
    }
    Mini_AigForEachNonPo( p, Id ) {
      bFunc = (DdNode *)Vec_PtrEntry(vFuncs, Id);
      Cudd_RecursiveDeref( dd, bFunc );
      Vec_PtrWriteEntry( vFuncs, Id, NULL );
    }
    Mini_AigForEachPo( p, Id ) 
        Vec_PtrWriteEntry( vFuncs, nOuts++, Vec_PtrEntry(vFuncs, Id) );
    Vec_PtrShrink( vFuncs, nOuts );
    return vFuncs;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_LutBddScan( DdManager * dd, DdNode * bFunc, int nVars )
{
    Vec_Ptr_t * vRes = Vec_PtrAlloc( 1 << nVars );
    Vec_Ptr_t * vRes2 = Vec_PtrAlloc( 1 << nVars );
    Vec_PtrPush( vRes, bFunc );
    int v, Level = Cudd_ReadPerm( dd, Cudd_NodeReadIndex(bFunc) );
    for ( v = 0; v < dd->size; v++ )
        printf( "%2d : perm = %d  invperm = %d\n", v, dd->perm[v], dd->invperm[v] );
    for ( v = 0; v < nVars; v++ )
    {
        int i, LevelCur = Level + v;
        Vec_PtrClear( vRes2 );
        DdNode * bTemp;
        Vec_PtrForEachEntry( DdNode *, vRes, bTemp, i )  {
            int LevelTemp = Cudd_ReadPerm( dd, Cudd_NodeReadIndex(bTemp) );
            if ( LevelTemp == LevelCur ) {
                Vec_PtrPush( vRes2, Cudd_NotCond(Cudd_E(bTemp), Cudd_IsComplement(bTemp)) );
                Vec_PtrPush( vRes2, Cudd_NotCond(Cudd_T(bTemp), Cudd_IsComplement(bTemp)) );
            } 
            else if ( LevelTemp > LevelCur ) {
                Vec_PtrPush( vRes2, bTemp );
                Vec_PtrPush( vRes2, bTemp );
            }
            else assert( 0 );
        }
        ABC_SWAP( Vec_Ptr_t *, vRes, vRes2 );
        //Vec_PtrForEachEntry( DdNode *, vRes, bTemp, i )
        //    printf( "%p ", bTemp );
        //printf( "\n" );
    }
    Vec_PtrFree( vRes2 );
    assert( Vec_PtrSize(vRes) == (1 << nVars) );
    return vRes;
}
char * Abc_LutBddToTruth( Vec_Ptr_t * vFuncs )
{
    assert( Vec_PtrSize(vFuncs) <= 256 );
    char * pRes = ABC_CALLOC( char, Vec_PtrSize(vFuncs)+1 );
    void * pTemp, * pStore[256] = {Vec_PtrEntry(vFuncs, 0)}; 
    int i, k, nStore = 1; pRes[0] = 'a';
    Vec_PtrForEachEntryStart( void *, vFuncs, pTemp, i, 1 ) {
        for ( k = 0; k < nStore; k++ )
            if ( pStore[k] == pTemp )
                break;
        if ( k == nStore )
            pStore[nStore++] = pTemp;
        pRes[i] = 'a' + (char)k;
    }
    return pRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_NtkPrecomputeData()
{
    char * pRes = ABC_CALLOC( char, 1 << 16 );
    int i, k, b;
    for ( i = 0; i < 256; i++ ) {
        int nOnes = __builtin_popcount(i);
        char * pTemp = pRes + 256*i;
        for ( k = 0; k < 256; k++ ) {
            int iMask = 0, Counts[2] = {nOnes, 0};
            for ( b = 0; b < 8; Counts[(i >> b++)&1]++ )
                if ( (k >> b) & 1 )
                    iMask |= 1 << Counts[(i >> b)&1];
            pTemp[iMask] = (char)k;
            assert( Counts[1] == nOnes );
        }
    }
    for ( i = 0; i < 16; i++, printf("\n") )
    for ( printf("%x : ", i), k = 0; k < 16; k++ )
        printf( "%x=%x ", k, (int)pRes[i*256+k] );    
    return pRes;
}

int Abc_NtkDecPatCount( int iFirst, int nStep, int MyuMax, char * pDecPat, char * pPermInfo )
{
    int s, k, nPats = 1; 
    char Pats[256] = { pDecPat[(int)pPermInfo[iFirst]] };
    assert( MyuMax <= 256 );
    for ( s = 1; s < nStep; s++ ) {
        char Entry = pDecPat[(int)pPermInfo[iFirst+s]];
        for ( k = 0; k < nPats; k++ )
            if ( Pats[k] == Entry )
                break;
        if ( k < nPats )
            continue;
        if ( nPats == MyuMax )
            return MyuMax + 1;
        assert( nPats < 256 );    
        Pats[nPats++] = Entry;
    }
    return nPats;
}
int Abc_NtkDecPatDecompose_rec( int Mask, int nMaskVars, int iStart, int nVars, int nDiffs, int nRails, char * pDecPat, char * pPermInfo )
{
    if ( nDiffs == 0 || iStart == nVars ) 
        return 0;
    int v, m, nMints = 1 << nVars;
    for ( v = iStart; v < nVars; v++ ) {
        int MaskThis = Mask & ~(1 << v);
        int nStep = 1 << (nMaskVars - 1);
        int MyuMax = 0;
        for ( m = 0; m < nMints; m += nStep ) {
            int MyuCur = Abc_NtkDecPatCount( m, nStep, 1 << nDiffs, pDecPat, pPermInfo+256*MaskThis );
            MyuMax = Abc_MaxInt( MyuMax, MyuCur );
        }
        if ( MyuMax > (1 << nDiffs) )
            continue;
        if ( MyuMax <= (1 << nRails) )
            return MaskThis;
        MaskThis = Abc_NtkDecPatDecompose_rec( MaskThis, nMaskVars-1, v+1, nVars, nDiffs-1, nRails, pDecPat, pPermInfo );
        if ( MaskThis )
            return MaskThis;
    }
    return 0;
}
int Abc_NtkDecPatDecompose( int nVars, int nRails, char * pDecPat, char * pPermInfo )
{
    int BoundSet = ~(~0 << nVars);
    int Myu = Abc_NtkDecPatCount( 0, 1 << nVars, 256, pDecPat, pPermInfo + 256*BoundSet );
    int Log2 = Abc_Base2Log( Myu );
    if ( Log2 <= nRails )
        return BoundSet;
    return Abc_NtkDecPatDecompose_rec( BoundSet, nVars, 0, nVars, Log2 - nRails, nRails, pDecPat, pPermInfo );
}

int Abc_NtkCascadeDecompose( int nVars, int nRails, char * pDecPat, char * pPermInfo )
{
    return 0;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_LutCasFakeNames( int nNames )
{
    Vec_Ptr_t * vNames;
    char Buffer[5];
    int i;

    vNames = Vec_PtrAlloc( nNames );
    for ( i = 0; i < nNames; i++ )
    {
        if ( nNames < 26 )
        {
            Buffer[0] = 'a' + i;
            Buffer[1] = 0;
        }
        else
        {
            Buffer[0] = 'a' + i%26;
            Buffer[1] = '0' + i/26;
            Buffer[2] = 0;
        }
        Vec_PtrPush( vNames, Extra_UtilStrsav(Buffer) );
    }
    return vNames;
}
void Abc_LutCasPrintDsd( DdManager * dd, DdNode * bFunc, int fVerbose )
{
    Dsd_Manager_t * pManDsd = Dsd_ManagerStart( dd, dd->size, 0 );
    Dsd_Decompose( pManDsd, &bFunc, 1 );
    if ( fVerbose )
    {
        Vec_Ptr_t * vNamesCi = Abc_LutCasFakeNames( dd->size );
        Vec_Ptr_t * vNamesCo = Abc_LutCasFakeNames( 1 );
        char ** ppNamesCi = (char **)Vec_PtrArray( vNamesCi );
        char ** ppNamesCo = (char **)Vec_PtrArray( vNamesCo );
        Dsd_TreePrint( stdout, pManDsd, ppNamesCi, ppNamesCo, 0, -1, 0 );
        Vec_PtrFreeFree( vNamesCi );
        Vec_PtrFreeFree( vNamesCo );
    }
    Dsd_ManagerStop( pManDsd );
}    
DdNode * Abc_LutCasBuildBdds( Mini_Aig_t * p, DdManager ** pdd, int fReorder )
{
    DdManager * dd = Cudd_Init( Mini_AigPiNum(p), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    if ( fReorder ) Cudd_AutodynEnable( dd,  CUDD_REORDER_SYMM_SIFT );
    Vec_Ptr_t * vFuncs = Abc_LutCasCollapse( p, dd, 10000, 0 );
    if ( fReorder ) Cudd_AutodynDisable( dd );
    if ( vFuncs == NULL ) {
        Extra_StopManager( dd );
        return NULL;
    }
    DdNode * bNode = (DdNode *)Vec_PtrEntry(vFuncs, 0);
    Vec_PtrFree(vFuncs);
    *pdd = dd;
    return bNode;
}
static inline word * Abc_LutCascade( Mini_Aig_t * p, int nLutSize, int nLuts, int nRails, int nIters, int fVerbose )
{
    DdManager * dd = NULL;
    DdNode * bFunc = Abc_LutCasBuildBdds( p, &dd, 0 );
    if ( bFunc == NULL ) return NULL;
    char * pPermInfo = Abc_NtkPrecomputeData();
    Abc_LutCasPrintDsd( dd, bFunc, 1 );

    Vec_Ptr_t * vTemp = Abc_LutBddScan( dd, bFunc, nLutSize );
    char * pTruth = Abc_LutBddToTruth( vTemp );

    int BoundSet = Abc_NtkDecPatDecompose( nLutSize, nRails, pTruth, pPermInfo );
    printf( "Pattern %s : Bound set = %d\n", pTruth, BoundSet );

    ABC_FREE( pTruth );
    Vec_PtrFree( vTemp );

    Cudd_RecursiveDeref( dd, bFunc );
    Extra_StopManager( dd );
    ABC_FREE( pPermInfo );

    printf( "\n" );
    word * pLuts = NULL;
    return pLuts;
}

ABC_NAMESPACE_HEADER_END

#endif /* ABC__misc__extra__extra_lutcas_h */
