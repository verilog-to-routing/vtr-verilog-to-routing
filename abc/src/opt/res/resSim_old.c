/**CFile****************************************************************

  FileName    [resSim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resynthesis package.]

  Synopsis    [Simulation engine.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 15, 2007.]

  Revision    [$Id: resSim.c,v 1.00 2007/01/15 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "resInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocate simulation engine.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
Res_Sim_t * Res_SimAlloc( int nWords )
{
    Res_Sim_t * p;
    p = ALLOC( Res_Sim_t, 1 );
    memset( p, 0, sizeof(Res_Sim_t) );
    // simulation parameters
    p->nWords    = nWords;
    p->nPats     = 8 * sizeof(unsigned) * p->nWords;
    p->nWordsOut = p->nPats * p->nWords;
    p->nPatsOut  = p->nPats * p->nPats;
    // simulation info
    p->vPats     = Vec_PtrAllocSimInfo( 1024, p->nWords );
    p->vPats0    = Vec_PtrAllocSimInfo( 128,  p->nWords );
    p->vPats1    = Vec_PtrAllocSimInfo( 128,  p->nWords );
    p->vOuts     = Vec_PtrAllocSimInfo( 128,  p->nWordsOut );
    // resub candidates
    p->vCands    = Vec_VecStart( 16 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocate simulation engine.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimAdjust( Res_Sim_t * p, Abc_Ntk_t * pAig )
{
    srand( 0xABC );

    assert( Abc_NtkIsStrash(pAig) );
    p->pAig = pAig;
    if ( Vec_PtrSize(p->vPats) < Abc_NtkObjNumMax(pAig)+1 )
    {
        Vec_PtrFree( p->vPats );
        p->vPats = Vec_PtrAllocSimInfo( Abc_NtkObjNumMax(pAig)+1, p->nWords );
    }
    if ( Vec_PtrSize(p->vPats0) < Abc_NtkPiNum(pAig) )
    {
        Vec_PtrFree( p->vPats0 );
        p->vPats0 = Vec_PtrAllocSimInfo( Abc_NtkPiNum(pAig), p->nWords );
    }
    if ( Vec_PtrSize(p->vPats1) < Abc_NtkPiNum(pAig) )
    {
        Vec_PtrFree( p->vPats1 );
        p->vPats1 = Vec_PtrAllocSimInfo( Abc_NtkPiNum(pAig), p->nWords );
    }
    if ( Vec_PtrSize(p->vOuts) < Abc_NtkPoNum(pAig) )
    {
        Vec_PtrFree( p->vOuts );
        p->vOuts = Vec_PtrAllocSimInfo( Abc_NtkPoNum(pAig), p->nWordsOut );
    }
    // clean storage info for patterns
    Abc_InfoClear( Vec_PtrEntry(p->vPats0,0), p->nWords * Abc_NtkPiNum(pAig) );
    Abc_InfoClear( Vec_PtrEntry(p->vPats1,0), p->nWords * Abc_NtkPiNum(pAig) );
    p->nPats0 = 0;
    p->nPats1 = 0;
}

/**Function*************************************************************

  Synopsis    [Free simulation engine.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimFree( Res_Sim_t * p )
{
    Vec_PtrFree( p->vPats );
    Vec_PtrFree( p->vPats0 );
    Vec_PtrFree( p->vPats1 );
    Vec_PtrFree( p->vOuts );
    Vec_VecFree( p->vCands );
    free( p );
}


/**Function*************************************************************

  Synopsis    [Sets random PI simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimSetRandom( Res_Sim_t * p )
{
    Abc_Obj_t * pObj;
    unsigned * pInfo;
    int i;
    Abc_NtkForEachPi( p->pAig, pObj, i )
    {
        pInfo = Vec_PtrEntry( p->vPats, pObj->Id );
        Abc_InfoRandom( pInfo, p->nWords );
    }
}

/**Function*************************************************************

  Synopsis    [Sets given PI simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimSetGiven( Res_Sim_t * p, Vec_Ptr_t * vInfo )
{
    Abc_Obj_t * pObj;
    unsigned * pInfo, * pInfo2;
    int i, w;
    Abc_NtkForEachPi( p->pAig, pObj, i )
    {
        pInfo = Vec_PtrEntry( p->vPats, pObj->Id );
        pInfo2 = Vec_PtrEntry( vInfo, i );
        for ( w = 0; w < p->nWords; w++ )
            pInfo[w] = pInfo2[w];
    }
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimPerformOne( Abc_Obj_t * pNode, Vec_Ptr_t * vSimInfo, int nSimWords )
{
    unsigned * pInfo, * pInfo1, * pInfo2;
    int k, fComp1, fComp2;
    // simulate the internal nodes
    assert( Abc_ObjIsNode(pNode) );
    pInfo  = Vec_PtrEntry(vSimInfo, pNode->Id);
    pInfo1 = Vec_PtrEntry(vSimInfo, Abc_ObjFaninId0(pNode));
    pInfo2 = Vec_PtrEntry(vSimInfo, Abc_ObjFaninId1(pNode));
    fComp1 = Abc_ObjFaninC0(pNode);
    fComp2 = Abc_ObjFaninC1(pNode);
    if ( fComp1 && fComp2 )
        for ( k = 0; k < nSimWords; k++ )
            pInfo[k] = ~pInfo1[k] & ~pInfo2[k];
    else if ( fComp1 && !fComp2 )
        for ( k = 0; k < nSimWords; k++ )
            pInfo[k] = ~pInfo1[k] &  pInfo2[k];
    else if ( !fComp1 && fComp2 )
        for ( k = 0; k < nSimWords; k++ )
            pInfo[k] =  pInfo1[k] & ~pInfo2[k];
    else // if ( fComp1 && fComp2 )
        for ( k = 0; k < nSimWords; k++ )
            pInfo[k] =  pInfo1[k] &  pInfo2[k];
}

/**Function*************************************************************

  Synopsis    [Simulates one CO node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimTransferOne( Abc_Obj_t * pNode, Vec_Ptr_t * vSimInfo, int nSimWords )
{
    unsigned * pInfo, * pInfo1;
    int k, fComp1;
    // simulate the internal nodes
    assert( Abc_ObjIsCo(pNode) );
    pInfo  = Vec_PtrEntry(vSimInfo, pNode->Id);
    pInfo1 = Vec_PtrEntry(vSimInfo, Abc_ObjFaninId0(pNode));
    fComp1 = Abc_ObjFaninC0(pNode);
    if ( fComp1 )
        for ( k = 0; k < nSimWords; k++ )
            pInfo[k] = ~pInfo1[k];
    else 
        for ( k = 0; k < nSimWords; k++ )
            pInfo[k] =  pInfo1[k];
}

/**Function*************************************************************

  Synopsis    [Performs one round of simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimPerformRound( Res_Sim_t * p )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_InfoFill( Vec_PtrEntry(p->vPats,0), p->nWords );
    Abc_AigForEachAnd( p->pAig, pObj, i )
        Res_SimPerformOne( pObj, p->vPats, p->nWords );
    Abc_NtkForEachPo( p->pAig, pObj, i )
        Res_SimTransferOne( pObj, p->vPats, p->nWords );
}

/**Function*************************************************************

  Synopsis    [Processes simulation patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimProcessPats( Res_Sim_t * p )
{
    Abc_Obj_t * pObj;
    unsigned * pInfoCare, * pInfoNode;
    int i, j, nDcs = 0;
    pInfoCare = Vec_PtrEntry( p->vPats, Abc_NtkPo(p->pAig, 0)->Id );
    pInfoNode = Vec_PtrEntry( p->vPats, Abc_NtkPo(p->pAig, 1)->Id );
    for ( i = 0; i < p->nPats; i++ )
    {
        // skip don't-care patterns
        if ( !Abc_InfoHasBit(pInfoCare, i) )
        {
            nDcs++;
            continue;
        }
        // separate offset and onset patterns
        if ( !Abc_InfoHasBit(pInfoNode, i) )
        {
            if ( p->nPats0 >= p->nPats )
                continue;
            Abc_NtkForEachPi( p->pAig, pObj, j )
                if ( Abc_InfoHasBit( Vec_PtrEntry(p->vPats, pObj->Id), i ) )
                    Abc_InfoSetBit( Vec_PtrEntry(p->vPats0, j), p->nPats0 );
            p->nPats0++;
        }
        else
        {
            if ( p->nPats1 >= p->nPats )
                continue;
            Abc_NtkForEachPi( p->pAig, pObj, j )
                if ( Abc_InfoHasBit( Vec_PtrEntry(p->vPats, pObj->Id), i ) )
                    Abc_InfoSetBit( Vec_PtrEntry(p->vPats1, j), p->nPats1 );
            p->nPats1++;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Pads the extra space with duplicated simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimPadSimInfo( Vec_Ptr_t * vPats, int nPats, int nWords )
{
    unsigned * pInfo;
    int i, w, iWords;
    assert( nPats > 0 && nPats < nWords * 8 * (int) sizeof(unsigned) );
    // pad the first word
    if ( nPats < 8 * sizeof(unsigned) )
    {
        Vec_PtrForEachEntry( unsigned *, vPats, pInfo, i )
            if ( pInfo[0] & 1 )
                pInfo[0] |= ((~0) << nPats);
        nPats = 8 * sizeof(unsigned);
    }
    // pad the empty words
    iWords = nPats / (8 * sizeof(unsigned));
    Vec_PtrForEachEntry( unsigned *, vPats, pInfo, i )
    {
        for ( w = iWords; w < nWords; w++ )
            pInfo[w] = pInfo[0];
    }
}

/**Function*************************************************************

  Synopsis    [Duplicates the simulation info to fill the space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimDeriveInfoReplicate( Res_Sim_t * p )
{
    unsigned * pInfo, * pInfo2;
    Abc_Obj_t * pObj;
    int i, j, w;
    Abc_NtkForEachPo( p->pAig, pObj, i )
    {
        pInfo = Vec_PtrEntry( p->vPats, pObj->Id );
        pInfo2 = Vec_PtrEntry( p->vOuts, i );
        for ( j = 0; j < p->nPats; j++ )
            for ( w = 0; w < p->nWords; w++ )
                *pInfo2++ = pInfo[w];
    }
}

/**Function*************************************************************

  Synopsis    [Complement the simulation info if necessary.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimDeriveInfoComplement( Res_Sim_t * p )
{
    unsigned * pInfo, * pInfo2;
    Abc_Obj_t * pObj;
    int i, j, w;
    Abc_NtkForEachPo( p->pAig, pObj, i )
    {
        pInfo = Vec_PtrEntry( p->vPats, pObj->Id );
        pInfo2 = Vec_PtrEntry( p->vOuts, i );
        for ( j = 0; j < p->nPats; j++, pInfo2 += p->nWords )
            if ( Abc_InfoHasBit( pInfo, j ) )
                for ( w = 0; w < p->nWords; w++ )
                    pInfo2[w] = ~pInfo2[w];
    }
}

/**Function*************************************************************

  Synopsis    [Free simulation engine.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimReportOne( Res_Sim_t * p )
{
    unsigned * pInfoCare, * pInfoNode;
    int i, nDcs, nOnes, nZeros;
    pInfoCare = Vec_PtrEntry( p->vPats, Abc_NtkPo(p->pAig, 0)->Id );
    pInfoNode = Vec_PtrEntry( p->vPats, Abc_NtkPo(p->pAig, 1)->Id );
    nDcs = nOnes = nZeros = 0;
    for ( i = 0; i < p->nPats; i++ )
    {
        // skip don't-care patterns
        if ( !Abc_InfoHasBit(pInfoCare, i) )
        {
            nDcs++;
            continue;
        }
        // separate offset and onset patterns
        if ( !Abc_InfoHasBit(pInfoNode, i) )
            nZeros++;
        else
            nOnes++;
    }
    printf( "On = %3d (%7.2f %%)  ",  nOnes,  100.0*nOnes/p->nPats );
    printf( "Off = %3d (%7.2f %%)  ", nZeros, 100.0*nZeros/p->nPats );
    printf( "Dc = %3d (%7.2f %%)  ",  nDcs,   100.0*nDcs/p->nPats );
    printf( "P0 = %3d ", p->nPats0 );
    printf( "P1 = %3d ", p->nPats1 );
    if ( p->nPats0 < 4 || p->nPats1 < 4 )
        printf( "*" );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints output patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimPrintOutPatterns( Res_Sim_t * p, Abc_Ntk_t * pAig )
{
    Abc_Obj_t * pObj;
    unsigned * pInfo2;
    int i;
    Abc_NtkForEachPo( pAig, pObj, i )
    {
        pInfo2 = Vec_PtrEntry( p->vOuts, i );
        Extra_PrintBinary( stdout, pInfo2, p->nPatsOut );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Prepares simulation info for candidate filtering.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_SimPrepare( Res_Sim_t * p, Abc_Ntk_t * pAig, int nTruePis, int fVerbose )
{
    int Limit;
    // prepare the manager
    Res_SimAdjust( p, pAig );
    // collect 0/1 simulation info
    for ( Limit = 0; Limit < 10; Limit++ )
    {
        Res_SimSetRandom( p );
        Res_SimPerformRound( p );
        Res_SimProcessPats( p );
        if ( !(p->nPats0 < p->nPats || p->nPats1 < p->nPats) )
            break;
    }
//    printf( "%d   ", Limit );
    // report the last set of patterns
//    Res_SimReportOne( p );
//    printf( "\n" );
    // quit if there is not enough
//    if ( p->nPats0 < 4 || p->nPats1 < 4 )
    if ( p->nPats0 < 4 || p->nPats1 < 4 )
    {
//        Res_SimReportOne( p );
        return 0;
    }
    // create bit-matrix info
    if ( p->nPats0 < p->nPats )
        Res_SimPadSimInfo( p->vPats0, p->nPats0, p->nWords );
    if ( p->nPats1 < p->nPats )
        Res_SimPadSimInfo( p->vPats1, p->nPats1, p->nWords );
    // resimulate 0-patterns
    Res_SimSetGiven( p, p->vPats0 );
    Res_SimPerformRound( p );
    Res_SimDeriveInfoReplicate( p );
    // resimulate 1-patterns
    Res_SimSetGiven( p, p->vPats1 );
    Res_SimPerformRound( p );
    Res_SimDeriveInfoComplement( p );
    // print output patterns
//    Res_SimPrintOutPatterns( p, pAig );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

