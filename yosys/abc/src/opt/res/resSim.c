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
    p = ABC_ALLOC( Res_Sim_t, 1 );
    memset( p, 0, sizeof(Res_Sim_t) );
    // simulation parameters
    p->nWords    = nWords;
    p->nPats     = p->nWords * 8 * sizeof(unsigned);
    p->nWordsIn  = p->nPats;
    p->nBytesIn  = p->nPats * sizeof(unsigned);
    p->nPatsIn   = p->nPats * 8 * sizeof(unsigned);
    p->nWordsOut = p->nPats * p->nWords;
    p->nPatsOut  = p->nPats * p->nPats;
    // simulation info
    p->vPats     = Vec_PtrAllocSimInfo( 1024, p->nWordsIn );
    p->vPats0    = Vec_PtrAllocSimInfo( 128, p->nWords );
    p->vPats1    = Vec_PtrAllocSimInfo( 128, p->nWords );
    p->vOuts     = Vec_PtrAllocSimInfo( 128, p->nWordsOut );
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
void Res_SimAdjust( Res_Sim_t * p, Abc_Ntk_t * pAig, int nTruePis )
{
    srand( 0xABC );

    assert( Abc_NtkIsStrash(pAig) );
    p->pAig = pAig;
    p->nTruePis = nTruePis;
    if ( Vec_PtrSize(p->vPats) < Abc_NtkObjNumMax(pAig)+1 )
    {
        Vec_PtrFree( p->vPats );
        p->vPats = Vec_PtrAllocSimInfo( Abc_NtkObjNumMax(pAig)+1, p->nWordsIn );
    }
    if ( Vec_PtrSize(p->vPats0) < nTruePis )
    {
        Vec_PtrFree( p->vPats0 );
        p->vPats0 = Vec_PtrAllocSimInfo( nTruePis, p->nWords );
    }
    if ( Vec_PtrSize(p->vPats1) < nTruePis )
    {
        Vec_PtrFree( p->vPats1 );
        p->vPats1 = Vec_PtrAllocSimInfo( nTruePis, p->nWords );
    }
    if ( Vec_PtrSize(p->vOuts) < Abc_NtkPoNum(pAig) )
    {
        Vec_PtrFree( p->vOuts );
        p->vOuts = Vec_PtrAllocSimInfo( Abc_NtkPoNum(pAig), p->nWordsOut );
    }
    // clean storage info for patterns
    Abc_InfoClear( (unsigned *)Vec_PtrEntry(p->vPats0,0), p->nWords * nTruePis );
    Abc_InfoClear( (unsigned *)Vec_PtrEntry(p->vPats1,0), p->nWords * nTruePis );
    p->nPats0 = 0;
    p->nPats1 = 0;
    p->fConst0 = 0;
    p->fConst1 = 0;
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
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Sets random PI simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_InfoRandomBytes( unsigned * p, int nWords ) 
{ 
    int i, Num; 
    for ( i = nWords - 1; i >= 0; i-- ) 
    { 
        Num = rand(); 
        p[i] = (Num & 1)? 0xff : 0;
        p[i] = (p[i] << 8) | ((Num & 2)? 0xff : 0);
        p[i] = (p[i] << 8) | ((Num & 4)? 0xff : 0);
        p[i] = (p[i] << 8) | ((Num & 8)? 0xff : 0);
    } 
//    Extra_PrintBinary( stdout, p, 32 ); printf( "\n" );
} 

/**Function*************************************************************

  Synopsis    [Sets random PI simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimSetRandomBytes( Res_Sim_t * p )
{
    Abc_Obj_t * pObj;
    unsigned * pInfo;
    int i;
    Abc_NtkForEachPi( p->pAig, pObj, i )
    {
        pInfo = (unsigned *)Vec_PtrEntry( p->vPats, pObj->Id );
        if ( i < p->nTruePis )
            Abc_InfoRandomBytes( pInfo, p->nWordsIn );
        else
            Abc_InfoRandom( pInfo, p->nWordsIn );
    }
/*
    // double-check that all are byte-patterns
    Abc_NtkForEachPi( p->pAig, pObj, i )
    {
        if ( i == p->nTruePis )
            break;
        pInfoC = (unsigned char *)Vec_PtrEntry( p->vPats, pObj->Id );
        for ( k = 0; k < p->nBytesIn; k++ )
            assert( pInfoC[k] == 0 || pInfoC[k] == 0xff );
    }
*/
}

/**Function*************************************************************

  Synopsis    [Sets random PI simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimSetDerivedBytes( Res_Sim_t * p, int fUseWalk )
{
    Vec_Ptr_t * vPatsSource[2];
    int nPatsSource[2];
    Abc_Obj_t * pObj;
    unsigned char * pInfo;
    int i, k, z, s, nPats;

    // set several random patterns
    assert( p->nBytesIn % 32 == 0 );
    nPats = p->nBytesIn/8;
    Abc_NtkForEachPi( p->pAig, pObj, i )
    {
        if ( i == p->nTruePis )
            break;
        Abc_InfoRandomBytes( (unsigned *)Vec_PtrEntry(p->vPats, pObj->Id), nPats/4 );
    }

    // set special patterns
    if ( fUseWalk )
    {
        for ( z = 0; z < 2; z++ )
        {
            // set the zero pattern
            Abc_NtkForEachPi( p->pAig, pObj, i )
            {
                if ( i == p->nTruePis )
                    break;
                pInfo = (unsigned char *)Vec_PtrEntry( p->vPats, pObj->Id );
                pInfo[nPats] = z ? 0xff : 0;
            }
            if ( ++nPats == p->nBytesIn )
                return;
            // set the walking zero pattern
            for ( k = 0; k < p->nTruePis; k++ )
            {
                Abc_NtkForEachPi( p->pAig, pObj, i )
                {
                    if ( i == p->nTruePis )
                        break;
                    pInfo = (unsigned char *)Vec_PtrEntry( p->vPats, pObj->Id );
                    pInfo[nPats] = ((i == k) ^ z) ? 0xff : 0;
                }
                if ( ++nPats == p->nBytesIn )
                    return;
            }
        }
    }

    // decide what patterns to set first
    if ( p->nPats0 < p->nPats1 )
    {
        nPatsSource[0] = p->nPats0;
        vPatsSource[0] = p->vPats0;
        nPatsSource[1] = p->nPats1;
        vPatsSource[1] = p->vPats1;
    }
    else
    {
        nPatsSource[0] = p->nPats1;
        vPatsSource[0] = p->vPats1;
        nPatsSource[1] = p->nPats0;
        vPatsSource[1] = p->vPats0;
    }
    for ( z = 0; z < 2; z++ )
    {
        for ( s = nPatsSource[z] - 1; s >= 0; s-- )
        {
//            if ( s == 0 )
//            printf( "Patterns:\n" );
            // set the given source pattern
            for ( k = 0; k < p->nTruePis; k++ )
            {
                Abc_NtkForEachPi( p->pAig, pObj, i )
                {
                    if ( i == p->nTruePis )
                        break;
                    pInfo = (unsigned char *)Vec_PtrEntry( p->vPats, pObj->Id );
                    if ( (i == k) ^ Abc_InfoHasBit( (unsigned *)Vec_PtrEntry(vPatsSource[z], i), s ) )
                    {
                        pInfo[nPats] = 0xff;
//                        if ( s == 0 )
//                        printf( "1" );
                    }
                    else
                    {
                        pInfo[nPats] = 0;
//                        if ( s == 0 )
//                        printf( "0" );
                    }
                }
//                if ( s == 0 )
//                printf( "\n" );
                if ( ++nPats == p->nBytesIn )
                    return;
            }
        }
    }
    // clean the rest
    for ( z = nPats; z < p->nBytesIn; z++ )
    {
        Abc_NtkForEachPi( p->pAig, pObj, i )
        {
            if ( i == p->nTruePis )
                break;
            pInfo = (unsigned char *)Vec_PtrEntry( p->vPats, pObj->Id );
            memset( pInfo + nPats, 0, (size_t)(p->nBytesIn - nPats) );
        }
    }
/*
    // double-check that all are byte-patterns
    Abc_NtkForEachPi( p->pAig, pObj, i )
    {
        if ( i == p->nTruePis )
            break;
        pInfo = (unsigned char *)Vec_PtrEntry( p->vPats, pObj->Id );
        for ( k = 0; k < p->nBytesIn; k++ )
            assert( pInfo[k] == 0 || pInfo[k] == 0xff );
    }
*/
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
        if ( i == p->nTruePis )
            break;
        pInfo = (unsigned *)Vec_PtrEntry( p->vPats, pObj->Id );
        pInfo2 = (unsigned *)Vec_PtrEntry( vInfo, i );
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
    pInfo  = (unsigned *)Vec_PtrEntry(vSimInfo, pNode->Id);
    pInfo1 = (unsigned *)Vec_PtrEntry(vSimInfo, Abc_ObjFaninId0(pNode));
    pInfo2 = (unsigned *)Vec_PtrEntry(vSimInfo, Abc_ObjFaninId1(pNode));
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
    pInfo  = (unsigned *)Vec_PtrEntry(vSimInfo, pNode->Id);
    pInfo1 = (unsigned *)Vec_PtrEntry(vSimInfo, Abc_ObjFaninId0(pNode));
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
void Res_SimPerformRound( Res_Sim_t * p, int nWords )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_InfoFill( (unsigned *)Vec_PtrEntry(p->vPats,0), nWords );
    Abc_AigForEachAnd( p->pAig, pObj, i )
        Res_SimPerformOne( pObj, p->vPats, nWords );
    Abc_NtkForEachPo( p->pAig, pObj, i )
        Res_SimTransferOne( pObj, p->vPats, nWords );
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
        pInfo = (unsigned *)Vec_PtrEntry( p->vPats, pObj->Id );
        pInfo2 = (unsigned *)Vec_PtrEntry( p->vOuts, i );
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
        pInfo = (unsigned *)Vec_PtrEntry( p->vPats, pObj->Id );
        pInfo2 = (unsigned *)Vec_PtrEntry( p->vOuts, i );
        for ( j = 0; j < p->nPats; j++, pInfo2 += p->nWords )
            if ( Abc_InfoHasBit( pInfo, j ) )
                for ( w = 0; w < p->nWords; w++ )
                    pInfo2[w] = ~pInfo2[w];
    }
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
        pInfo2 = (unsigned *)Vec_PtrEntry( p->vOuts, i );
        Extra_PrintBinary( stdout, pInfo2, p->nPatsOut );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Prints output patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimPrintNodePatterns( Res_Sim_t * p, Abc_Ntk_t * pAig )
{
    unsigned * pInfo;
    pInfo = (unsigned *)Vec_PtrEntry( p->vPats, Abc_NtkPo(p->pAig, 1)->Id );
    Extra_PrintBinary( stdout, pInfo, p->nPats );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Counts the number of patters of different type.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimCountResults( Res_Sim_t * p, int * pnDcs, int * pnOnes, int * pnZeros, int fVerbose )
{
    unsigned char * pInfoCare, * pInfoNode;
    int i, nTotal = 0;
    pInfoCare = (unsigned char *)Vec_PtrEntry( p->vPats, Abc_NtkPo(p->pAig, 0)->Id );
    pInfoNode = (unsigned char *)Vec_PtrEntry( p->vPats, Abc_NtkPo(p->pAig, 1)->Id );
    for ( i = 0; i < p->nBytesIn; i++ )
    {
        if ( !pInfoCare[i] )
            (*pnDcs)++;
        else if ( !pInfoNode[i] )
            (*pnZeros)++;
        else
            (*pnOnes)++;
    }
    nTotal += *pnDcs;
    nTotal += *pnZeros;
    nTotal += *pnOnes;
    if ( fVerbose )
    {
        printf( "Dc = %7.2f %%  ",  100.0*(*pnDcs)  /nTotal );
        printf( "On = %7.2f %%  ",  100.0*(*pnOnes) /nTotal );
        printf( "Off = %7.2f %%  ", 100.0*(*pnZeros)/nTotal );
    }
}

/**Function*************************************************************

  Synopsis    [Counts the number of patters of different type.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_SimCollectPatterns( Res_Sim_t * p, int fVerbose )
{
    Abc_Obj_t * pObj;
    unsigned char * pInfoCare, * pInfoNode, * pInfo;
    int i, j;
    pInfoCare = (unsigned char *)Vec_PtrEntry( p->vPats, Abc_NtkPo(p->pAig, 0)->Id );
    pInfoNode = (unsigned char *)Vec_PtrEntry( p->vPats, Abc_NtkPo(p->pAig, 1)->Id );
    for ( i = 0; i < p->nBytesIn; i++ )
    {
        // skip don't-care patterns
        if ( !pInfoCare[i] )
            continue;
        // separate offset and onset patterns
        assert( pInfoNode[i] == 0 || pInfoNode[i] == 0xff );
        if ( !pInfoNode[i] )
        {
            if ( p->nPats0 >= p->nPats )
                continue;
            Abc_NtkForEachPi( p->pAig, pObj, j )
            {
                if ( j == p->nTruePis )
                    break;
                pInfo = (unsigned char *)Vec_PtrEntry( p->vPats, pObj->Id );
                assert( pInfo[i] == 0 || pInfo[i] == 0xff );
                if ( pInfo[i] )
                    Abc_InfoSetBit( (unsigned *)Vec_PtrEntry(p->vPats0, j), p->nPats0 );
            }
            p->nPats0++;
        }
        else
        {
            if ( p->nPats1 >= p->nPats )
                continue;
            Abc_NtkForEachPi( p->pAig, pObj, j )
            {
                if ( j == p->nTruePis )
                    break;
                pInfo = (unsigned char *)Vec_PtrEntry( p->vPats, pObj->Id );
                assert( pInfo[i] == 0 || pInfo[i] == 0xff );
                if ( pInfo[i] )
                    Abc_InfoSetBit( (unsigned *)Vec_PtrEntry(p->vPats1, j), p->nPats1 );
            }
            p->nPats1++;
        }
        if ( p->nPats0 >= p->nPats && p->nPats1 >= p->nPats )
            break;
    }
    if ( fVerbose )
    {
        printf( "|  " );
        printf( "On = %3d  ", p->nPats1 );
        printf( "Off = %3d  ", p->nPats0 );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Verifies the last pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_SimVerifyValue( Res_Sim_t * p, int fOnSet )
{
    Abc_Obj_t * pObj;
    unsigned * pInfo, * pInfo2;
    int i, value;
    Abc_NtkForEachPi( p->pAig, pObj, i )
    {
        if ( i == p->nTruePis )
            break;
        if ( fOnSet )
        {
            pInfo2 = (unsigned *)Vec_PtrEntry( p->vPats1, i );
            value = Abc_InfoHasBit( pInfo2, p->nPats1 - 1 );
        }
        else
        {
            pInfo2 = (unsigned *)Vec_PtrEntry( p->vPats0, i );
            value = Abc_InfoHasBit( pInfo2, p->nPats0 - 1 );
        }
        pInfo = (unsigned *)Vec_PtrEntry( p->vPats, pObj->Id );
        pInfo[0] = value ? ~0 : 0;
    }
    Res_SimPerformRound( p, 1 );
    pObj = Abc_NtkPo( p->pAig, 1 );
    pInfo = (unsigned *)Vec_PtrEntry( p->vPats, pObj->Id );
    assert( pInfo[0] == 0 || pInfo[0] == ~0 );
    return pInfo[0] > 0;
}

/**Function*************************************************************

  Synopsis    [Prepares simulation info for candidate filtering.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_SimPrepare( Res_Sim_t * p, Abc_Ntk_t * pAig, int nTruePis, int fVerbose )
{
    int i, nOnes = 0, nZeros = 0, nDcs = 0;
    if ( fVerbose )
        printf( "\n" );
    // prepare the manager
    Res_SimAdjust( p, pAig, nTruePis );
    // estimate the number of patterns
    Res_SimSetRandomBytes( p );
    Res_SimPerformRound( p, p->nWordsIn );
    Res_SimCountResults( p, &nDcs, &nOnes, &nZeros, fVerbose );
    // collect the patterns
    Res_SimCollectPatterns( p, fVerbose );
    // add more patterns using constraint simulation
    if ( p->nPats0 < 8 )
    {
        if ( !Res_SatSimulate( p, 16, 0 ) )
            return p->fConst0 || p->fConst1;
//            return 0;
//        printf( "Value0 = %d\n", Res_SimVerifyValue( p, 0 ) );
    }
    if ( p->nPats1 < 8 )
    {
        if ( !Res_SatSimulate( p, 16, 1 ) )
            return p->fConst0 || p->fConst1;
//            return 0;
//        printf( "Value1 = %d\n", Res_SimVerifyValue( p, 1 ) );
    }
    // generate additional patterns
    for ( i = 0; i < 2; i++ )
    {
        if ( p->nPats0 > p->nPats*7/8 && p->nPats1 > p->nPats*7/8 )
            break;
        Res_SimSetDerivedBytes( p, i==0 );
        Res_SimPerformRound( p, p->nWordsIn );
        Res_SimCountResults( p, &nDcs, &nOnes, &nZeros, fVerbose );
        Res_SimCollectPatterns( p, fVerbose );
    }
    // create bit-matrix info
    if ( p->nPats0 < p->nPats )
        Res_SimPadSimInfo( p->vPats0, p->nPats0, p->nWords );
    if ( p->nPats1 < p->nPats )
        Res_SimPadSimInfo( p->vPats1, p->nPats1, p->nWords );
    // resimulate 0-patterns
    Res_SimSetGiven( p, p->vPats0 );
    Res_SimPerformRound( p, p->nWords );
//Res_SimPrintNodePatterns( p, pAig );
    Res_SimDeriveInfoReplicate( p );
    // resimulate 1-patterns
    Res_SimSetGiven( p, p->vPats1 );
    Res_SimPerformRound( p, p->nWords );
//Res_SimPrintNodePatterns( p, pAig );
    Res_SimDeriveInfoComplement( p );
    // print output patterns
//    Res_SimPrintOutPatterns( p, pAig );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

