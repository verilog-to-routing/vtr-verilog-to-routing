/**CFile****************************************************************

  FileName    [sswSim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [Sequential simulator used by the inductive prover.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswSim.c,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sswInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// simulation manager
struct Ssw_Sml_t_
{
    Aig_Man_t *      pAig;              // the original AIG manager
    int              nPref;             // the number of timeframes in the prefix
    int              nFrames;           // the number of timeframes 
    int              nWordsFrame;       // the number of words in each timeframe
    int              nWordsTotal;       // the total number of words at a node
    int              nWordsPref;        // the number of word in the prefix
    int              fNonConstOut;      // have seen a non-const-0 output during simulation
    int              nSimRounds;        // statistics
    abctime          timeSim;           // statistics
    unsigned         pData[0];          // simulation data for the nodes
};

static inline unsigned * Ssw_ObjSim( Ssw_Sml_t * p, int Id )  { return p->pData + p->nWordsTotal * Id; }
static inline unsigned   Ssw_ObjRandomSim()                   { return Aig_ManRandom(0);               }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes hash value of the node using its simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Ssw_SmlObjHashWord( Ssw_Sml_t * p, Aig_Obj_t * pObj )
{
    static int s_SPrimes[128] = {
        1009, 1049, 1093, 1151, 1201, 1249, 1297, 1361, 1427, 1459,
        1499, 1559, 1607, 1657, 1709, 1759, 1823, 1877, 1933, 1997,
        2039, 2089, 2141, 2213, 2269, 2311, 2371, 2411, 2467, 2543,
        2609, 2663, 2699, 2741, 2797, 2851, 2909, 2969, 3037, 3089,
        3169, 3221, 3299, 3331, 3389, 3461, 3517, 3557, 3613, 3671,
        3719, 3779, 3847, 3907, 3943, 4013, 4073, 4129, 4201, 4243,
        4289, 4363, 4441, 4493, 4549, 4621, 4663, 4729, 4793, 4871,
        4933, 4973, 5021, 5087, 5153, 5227, 5281, 5351, 5417, 5471,
        5519, 5573, 5651, 5693, 5749, 5821, 5861, 5923, 6011, 6073,
        6131, 6199, 6257, 6301, 6353, 6397, 6481, 6563, 6619, 6689,
        6737, 6803, 6863, 6917, 6977, 7027, 7109, 7187, 7237, 7309,
        7393, 7477, 7523, 7561, 7607, 7681, 7727, 7817, 7877, 7933,
        8011, 8039, 8059, 8081, 8093, 8111, 8123, 8147
    };
    unsigned * pSims;
    unsigned uHash;
    int i;
//    assert( p->nWordsTotal <= 128 );
    uHash = 0;
    pSims = Ssw_ObjSim(p, pObj->Id);
    for ( i = p->nWordsPref; i < p->nWordsTotal; i++ )
        uHash ^= pSims[i] * s_SPrimes[i & 0x7F];
    return uHash;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlObjIsConstWord( Ssw_Sml_t * p, Aig_Obj_t * pObj )
{
    unsigned * pSims;
    int i;
    pSims = Ssw_ObjSim(p, pObj->Id);
    for ( i = p->nWordsPref; i < p->nWordsTotal; i++ )
        if ( pSims[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation infos are equal.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlObjsAreEqualWord( Ssw_Sml_t * p, Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 )
{
    unsigned * pSims0, * pSims1;
    int i;
    pSims0 = Ssw_ObjSim(p, pObj0->Id);
    pSims1 = Ssw_ObjSim(p, pObj1->Id);
    for ( i = p->nWordsPref; i < p->nWordsTotal; i++ )
        if ( pSims0[i] != pSims1[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node appears to be constant 1 candidate.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlObjIsConstBit( void * p, Aig_Obj_t * pObj )
{
    return pObj->fPhase == pObj->fMarkB;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the nodes appear equal.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlObjsAreEqualBit( void * p, Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 )
{
    return (pObj0->fPhase == pObj1->fPhase) == (pObj0->fMarkB == pObj1->fMarkB);
}


/**Function*************************************************************

  Synopsis    [Counts the number of 1s in the XOR of simulation data.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlNodeNotEquWeight( Ssw_Sml_t * p, int Left, int Right )
{
    unsigned * pSimL, * pSimR;
    int k, Counter = 0;
    pSimL = Ssw_ObjSim( p, Left );
    pSimR = Ssw_ObjSim( p, Right );
    for ( k = p->nWordsPref; k < p->nWordsTotal; k++ )
        Counter += Aig_WordCountOnes( pSimL[k] ^ pSimR[k] );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Checks implication.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlCheckXorImplication( Ssw_Sml_t * p, Aig_Obj_t * pObjLi, Aig_Obj_t * pObjLo, Aig_Obj_t * pCand )
{
    unsigned * pSimLi, * pSimLo, * pSimCand;
    int k;
    assert( pObjLo->fPhase == 0 );
    // pObjLi->fPhase may be 1, but the LI simulation data is not complemented!
    pSimCand = Ssw_ObjSim( p, Aig_Regular(pCand)->Id );
    pSimLi   = Ssw_ObjSim( p, pObjLi->Id );
    pSimLo   = Ssw_ObjSim( p, pObjLo->Id );
    if ( Aig_Regular(pCand)->fPhase ^ Aig_IsComplement(pCand) )
    {
        for ( k = p->nWordsPref; k < p->nWordsTotal; k++ )
            if ( ~pSimCand[k] & (pSimLi[k] ^ pSimLo[k]) )
                return 0;
    }
    else
    {
        for ( k = p->nWordsPref; k < p->nWordsTotal; k++ )
            if ( pSimCand[k] & (pSimLi[k] ^ pSimLo[k]) )
                return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Counts the number of 1s in the implication.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlCountXorImplication( Ssw_Sml_t * p, Aig_Obj_t * pObjLi, Aig_Obj_t * pObjLo, Aig_Obj_t * pCand )
{
    unsigned * pSimLi, * pSimLo, * pSimCand;
    int k, Counter = 0;
    assert( pObjLo->fPhase == 0 );
    // pObjLi->fPhase may be 1, but the LI simulation data is not complemented!
    pSimCand = Ssw_ObjSim( p, Aig_Regular(pCand)->Id );
    pSimLi   = Ssw_ObjSim( p, pObjLi->Id );
    pSimLo   = Ssw_ObjSim( p, pObjLo->Id );
    if ( Aig_Regular(pCand)->fPhase ^ Aig_IsComplement(pCand) )
    {
        for ( k = p->nWordsPref; k < p->nWordsTotal; k++ )
            Counter += Aig_WordCountOnes(~pSimCand[k] & ~(pSimLi[k] ^ pSimLo[k]));
    }
    else
    {
        for ( k = p->nWordsPref; k < p->nWordsTotal; k++ )
            Counter += Aig_WordCountOnes(pSimCand[k] & ~(pSimLi[k] ^ pSimLo[k]));
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of 1s in the implication.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlCountEqual( Ssw_Sml_t * p, Aig_Obj_t * pObjLi, Aig_Obj_t * pObjLo )
{
    unsigned * pSimLi, * pSimLo;
    int k, Counter = 0;
    assert( pObjLo->fPhase == 0 );
    // pObjLi->fPhase may be 1, but the LI simulation data is not complemented!
    pSimLi   = Ssw_ObjSim( p, pObjLi->Id );
    pSimLo   = Ssw_ObjSim( p, pObjLo->Id );
    for ( k = p->nWordsPref; k < p->nWordsTotal; k++ )
        Counter += Aig_WordCountOnes( ~(pSimLi[k] ^ pSimLo[k]) );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlNodeIsZero( Ssw_Sml_t * p, Aig_Obj_t * pObj )
{
    unsigned * pSims;
    int i;
    pSims = Ssw_ObjSim(p, pObj->Id);
    for ( i = p->nWordsPref; i < p->nWordsTotal; i++ )
        if ( pSims[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlNodeIsZeroFrame( Ssw_Sml_t * p, Aig_Obj_t * pObj, int f )
{
    unsigned * pSims = Ssw_ObjSim(p, pObj->Id);
    return pSims[f] == 0;
}

/**Function*************************************************************

  Synopsis    [Counts the number of one's in the pattern of the object.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlNodeCountOnesReal( Ssw_Sml_t * p, Aig_Obj_t * pObj )
{
    unsigned * pSims;
    int i, Counter = 0;
    pSims = Ssw_ObjSim(p, Aig_Regular(pObj)->Id);
    if ( Aig_Regular(pObj)->fPhase ^ Aig_IsComplement(pObj) )
    {
        for ( i = 0; i < p->nWordsTotal; i++ )
            Counter += Aig_WordCountOnes( ~pSims[i] );
    }
    else
    {
        for ( i = 0; i < p->nWordsTotal; i++ )
            Counter += Aig_WordCountOnes( pSims[i] );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of one's in the pattern of the objects.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlNodeCountOnesRealVec( Ssw_Sml_t * p, Vec_Ptr_t * vObjs )
{
    Aig_Obj_t * pObj;
    unsigned * pSims, uWord;
    int i, k, Counter = 0;
    if ( Vec_PtrSize(vObjs) == 0 )
        return 0;
    for ( i = 0; i < p->nWordsTotal; i++ )
    {
        uWord = 0;
        Vec_PtrForEachEntry( Aig_Obj_t *, vObjs, pObj, k )
        {
            pSims = Ssw_ObjSim(p, Aig_Regular(pObj)->Id);
            if ( Aig_Regular(pObj)->fPhase ^ Aig_IsComplement(pObj) )
                uWord |= ~pSims[i];
            else
                uWord |= pSims[i];
        }
        Counter += Aig_WordCountOnes( uWord );
    }
    return Counter;
}



/**Function*************************************************************

  Synopsis    [Generated const 0 pattern.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlSavePattern0( Ssw_Man_t * p, int fInit )
{
    memset( p->pPatWords, 0, sizeof(unsigned) * p->nPatWords );
}

/**Function*************************************************************

  Synopsis    [[Generated const 1 pattern.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlSavePattern1( Ssw_Man_t * p, int fInit )
{
    Aig_Obj_t * pObj;
    int i, k, nTruePis;
    memset( p->pPatWords, 0xff, sizeof(unsigned) * p->nPatWords );
    if ( !fInit )
        return;
    // clear the state bits to correspond to all-0 initial state
    nTruePis = Saig_ManPiNum(p->pAig);
    k = 0;
    Saig_ManForEachLo( p->pAig, pObj, i )
        Abc_InfoXorBit( p->pPatWords, nTruePis * p->nFrames + k++ );
}


/**Function*************************************************************

  Synopsis    [Creates the counter-example from the successful pattern.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Ssw_SmlCheckOutputSavePattern( Ssw_Sml_t * p, Aig_Obj_t * pObjPo )
{
    Aig_Obj_t * pFanin, * pObjPi;
    unsigned * pSims;
    int i, k, BestPat, * pModel;
    // find the word of the pattern
    pFanin = Aig_ObjFanin0(pObjPo);
    pSims = Ssw_ObjSim(p, pFanin->Id);
    for ( i = 0; i < p->nWordsTotal; i++ )
        if ( pSims[i] )
            break;
    assert( i < p->nWordsTotal );
    // find the bit of the pattern
    for ( k = 0; k < 32; k++ )
        if ( pSims[i] & (1 << k) )
            break;
    assert( k < 32 );
    // determine the best pattern
    BestPat = i * 32 + k;
    // fill in the counter-example data
    pModel = ABC_ALLOC( int, Aig_ManCiNum(p->pAig)+1 );
    Aig_ManForEachCi( p->pAig, pObjPi, i )
    {
        pModel[i] = Abc_InfoHasBit(Ssw_ObjSim(p, pObjPi->Id), BestPat);
//        Abc_Print( 1, "%d", pModel[i] );
    }
    pModel[Aig_ManCiNum(p->pAig)] = pObjPo->Id;
//    Abc_Print( 1, "\n" );
    return pModel;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the one of the output is already non-constant 0.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Ssw_SmlCheckOutput( Ssw_Sml_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    // make sure the reference simulation pattern does not detect the bug
    pObj = Aig_ManCo( p->pAig, 0 );
    assert( Aig_ObjFanin0(pObj)->fPhase == (unsigned)Aig_ObjFaninC0(pObj) );
    Aig_ManForEachCo( p->pAig, pObj, i )
    {
        if ( !Ssw_SmlObjIsConstWord( p, Aig_ObjFanin0(pObj) ) )
        {
            // create the counter-example from this pattern
            return Ssw_SmlCheckOutputSavePattern( p, pObj );
        }
    }
    return NULL;
}



/**Function*************************************************************

  Synopsis    [Assigns random patterns to the PI node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlAssignRandom( Ssw_Sml_t * p, Aig_Obj_t * pObj )
{
    unsigned * pSims;
    int i, f;
    assert( Aig_ObjIsCi(pObj) );
    pSims = Ssw_ObjSim( p, pObj->Id );
    for ( i = 0; i < p->nWordsTotal; i++ )
        pSims[i] = Ssw_ObjRandomSim();
    // set the first bit 0 in each frame
    assert( p->nWordsFrame * p->nFrames == p->nWordsTotal );
    for ( f = 0; f < p->nFrames; f++ )
        pSims[p->nWordsFrame*f] <<= 1;
}

/**Function*************************************************************

  Synopsis    [Assigns random patterns to the PI node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlAssignRandomFrame( Ssw_Sml_t * p, Aig_Obj_t * pObj, int iFrame )
{
    unsigned * pSims;
    int i;
    assert( iFrame < p->nFrames );
    assert( Aig_ObjIsCi(pObj) );
    pSims = Ssw_ObjSim( p, pObj->Id ) + p->nWordsFrame * iFrame;
    for ( i = 0; i < p->nWordsFrame; i++ )
        pSims[i] = Ssw_ObjRandomSim();
}

/**Function*************************************************************

  Synopsis    [Assigns constant patterns to the PI node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlObjAssignConst( Ssw_Sml_t * p, Aig_Obj_t * pObj, int fConst1, int iFrame )
{
    unsigned * pSims;
    int i;
    assert( iFrame < p->nFrames );
    assert( Aig_ObjIsCi(pObj) );
    pSims = Ssw_ObjSim( p, pObj->Id ) + p->nWordsFrame * iFrame;
    for ( i = 0; i < p->nWordsFrame; i++ )
        pSims[i] = fConst1? ~(unsigned)0 : 0;
}

/**Function*************************************************************

  Synopsis    [Assigns constant patterns to the PI node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlObjAssignConstWord( Ssw_Sml_t * p, Aig_Obj_t * pObj, int fConst1, int iFrame, int iWord )
{
    unsigned * pSims;
    assert( iFrame < p->nFrames );
    assert( iWord < p->nWordsFrame );
    assert( Aig_ObjIsCi(pObj) );
    pSims = Ssw_ObjSim( p, pObj->Id ) + p->nWordsFrame * iFrame;
    pSims[iWord] = fConst1? ~(unsigned)0 : 0;
}

/**Function*************************************************************

  Synopsis    [Assigns constant patterns to the PI node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlObjSetWord( Ssw_Sml_t * p, Aig_Obj_t * pObj, unsigned Word, int iWord, int iFrame )
{
    unsigned * pSims;
    assert( iFrame < p->nFrames );
    assert( Aig_ObjIsCi(pObj) );
    pSims = Ssw_ObjSim( p, pObj->Id ) + p->nWordsFrame * iFrame;
    pSims[iWord] = Word;
}

/**Function*************************************************************

  Synopsis    [Assings distance-1 simulation info for the PIs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlAssignDist1( Ssw_Sml_t * p, unsigned * pPat )
{
    Aig_Obj_t * pObj;
    int f, i, k, Limit, nTruePis;
    assert( p->nFrames > 0 );
    if ( p->nFrames == 1 )
    {
        // copy the PI info
        Aig_ManForEachCi( p->pAig, pObj, i )
            Ssw_SmlObjAssignConst( p, pObj, Abc_InfoHasBit(pPat, i), 0 );
        // flip one bit
        Limit = Abc_MinInt( Aig_ManCiNum(p->pAig), p->nWordsTotal * 32 - 1 );
        for ( i = 0; i < Limit; i++ )
            Abc_InfoXorBit( Ssw_ObjSim( p, Aig_ManCi(p->pAig,i)->Id ), i+1 );
    }
    else
    {
        int fUseDist1 = 0;

        // copy the PI info for each frame
        nTruePis = Aig_ManCiNum(p->pAig) - Aig_ManRegNum(p->pAig);
        for ( f = 0; f < p->nFrames; f++ )
            Saig_ManForEachPi( p->pAig, pObj, i )
                Ssw_SmlObjAssignConst( p, pObj, Abc_InfoHasBit(pPat, nTruePis * f + i), f );
        // copy the latch info
        k = 0;
        Saig_ManForEachLo( p->pAig, pObj, i )
            Ssw_SmlObjAssignConst( p, pObj, Abc_InfoHasBit(pPat, nTruePis * p->nFrames + k++), 0 );
//        assert( p->pFrames == NULL || nTruePis * p->nFrames + k == Aig_ManCiNum(p->pFrames) );

        // flip one bit of the last frame
        if ( fUseDist1 ) //&& p->nFrames == 2 )
        {
            Limit = Abc_MinInt( nTruePis, p->nWordsFrame * 32 - 1 );
            for ( i = 0; i < Limit; i++ )
                Abc_InfoXorBit( Ssw_ObjSim( p, Aig_ManCi(p->pAig, i)->Id ) + p->nWordsFrame*(p->nFrames-1), i+1 );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Assings distance-1 simulation info for the PIs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlAssignDist1Plus( Ssw_Sml_t * p, unsigned * pPat )
{
    Aig_Obj_t * pObj;
    int f, i, Limit;
    assert( p->nFrames > 0 );

    // copy the pattern into the primary inputs
    Aig_ManForEachCi( p->pAig, pObj, i )
        Ssw_SmlObjAssignConst( p, pObj, Abc_InfoHasBit(pPat, i), 0 );

    // set distance one PIs for the first frame
    Limit = Abc_MinInt( Saig_ManPiNum(p->pAig), p->nWordsFrame * 32 - 1 );
    for ( i = 0; i < Limit; i++ )
        Abc_InfoXorBit( Ssw_ObjSim( p, Aig_ManCi(p->pAig, i)->Id ), i+1 );

    // create random info for the remaining timeframes
    for ( f = 1; f < p->nFrames; f++ )
        Saig_ManForEachPi( p->pAig, pObj, i )
            Ssw_SmlAssignRandomFrame( p, pObj, f );
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlNodeSimulate( Ssw_Sml_t * p, Aig_Obj_t * pObj, int iFrame )
{
    unsigned * pSims, * pSims0, * pSims1;
    int fCompl, fCompl0, fCompl1, i;
    assert( iFrame < p->nFrames );
    assert( !Aig_IsComplement(pObj) );
    assert( Aig_ObjIsNode(pObj) );
    assert( iFrame == 0 || p->nWordsFrame < p->nWordsTotal );
    // get hold of the simulation information
    pSims  = Ssw_ObjSim(p, pObj->Id) + p->nWordsFrame * iFrame;
    pSims0 = Ssw_ObjSim(p, Aig_ObjFanin0(pObj)->Id) + p->nWordsFrame * iFrame;
    pSims1 = Ssw_ObjSim(p, Aig_ObjFanin1(pObj)->Id) + p->nWordsFrame * iFrame;
    // get complemented attributes of the children using their random info
    fCompl  = pObj->fPhase;
    fCompl0 = Aig_ObjPhaseReal(Aig_ObjChild0(pObj));
    fCompl1 = Aig_ObjPhaseReal(Aig_ObjChild1(pObj));
    // simulate
    if ( fCompl0 && fCompl1 )
    {
        if ( fCompl )
            for ( i = 0; i < p->nWordsFrame; i++ )
                pSims[i] = (pSims0[i] | pSims1[i]);
        else
            for ( i = 0; i < p->nWordsFrame; i++ )
                pSims[i] = ~(pSims0[i] | pSims1[i]);
    }
    else if ( fCompl0 && !fCompl1 )
    {
        if ( fCompl )
            for ( i = 0; i < p->nWordsFrame; i++ )
                pSims[i] = (pSims0[i] | ~pSims1[i]);
        else
            for ( i = 0; i < p->nWordsFrame; i++ )
                pSims[i] = (~pSims0[i] & pSims1[i]);
    }
    else if ( !fCompl0 && fCompl1 )
    {
        if ( fCompl )
            for ( i = 0; i < p->nWordsFrame; i++ )
                pSims[i] = (~pSims0[i] | pSims1[i]);
        else
            for ( i = 0; i < p->nWordsFrame; i++ )
                pSims[i] = (pSims0[i] & ~pSims1[i]);
    }
    else // if ( !fCompl0 && !fCompl1 )
    {
        if ( fCompl )
            for ( i = 0; i < p->nWordsFrame; i++ )
                pSims[i] = ~(pSims0[i] & pSims1[i]);
        else
            for ( i = 0; i < p->nWordsFrame; i++ )
                pSims[i] = (pSims0[i] & pSims1[i]);
    }
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlNodesCompareInFrame( Ssw_Sml_t * p, Aig_Obj_t * pObj0, Aig_Obj_t * pObj1, int iFrame0, int iFrame1 )
{
    unsigned * pSims0, * pSims1;
    int i;
    assert( iFrame0 < p->nFrames );
    assert( iFrame1 < p->nFrames );
    assert( !Aig_IsComplement(pObj0) );
    assert( !Aig_IsComplement(pObj1) );
    assert( iFrame0 == 0 || p->nWordsFrame < p->nWordsTotal );
    assert( iFrame1 == 0 || p->nWordsFrame < p->nWordsTotal );
    // get hold of the simulation information
    pSims0  = Ssw_ObjSim(p, pObj0->Id) + p->nWordsFrame * iFrame0;
    pSims1  = Ssw_ObjSim(p, pObj1->Id) + p->nWordsFrame * iFrame1;
    // compare
    for ( i = 0; i < p->nWordsFrame; i++ )
        if ( pSims0[i] != pSims1[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlNodeCopyFanin( Ssw_Sml_t * p, Aig_Obj_t * pObj, int iFrame )
{
    unsigned * pSims, * pSims0;
    int fCompl, fCompl0, i;
    assert( iFrame < p->nFrames );
    assert( !Aig_IsComplement(pObj) );
    assert( Aig_ObjIsCo(pObj) );
    assert( iFrame == 0 || p->nWordsFrame < p->nWordsTotal );
    // get hold of the simulation information
    pSims  = Ssw_ObjSim(p, pObj->Id) + p->nWordsFrame * iFrame;
    pSims0 = Ssw_ObjSim(p, Aig_ObjFanin0(pObj)->Id) + p->nWordsFrame * iFrame;
    // get complemented attributes of the children using their random info
    fCompl  = pObj->fPhase;
    fCompl0 = Aig_ObjPhaseReal(Aig_ObjChild0(pObj));
    // copy information as it is
    if ( fCompl0 )
        for ( i = 0; i < p->nWordsFrame; i++ )
            pSims[i] = ~pSims0[i];
    else
        for ( i = 0; i < p->nWordsFrame; i++ )
            pSims[i] = pSims0[i];
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlNodeTransferNext( Ssw_Sml_t * p, Aig_Obj_t * pOut, Aig_Obj_t * pIn, int iFrame )
{
    unsigned * pSims0, * pSims1;
    int i;
    assert( iFrame < p->nFrames );
    assert( !Aig_IsComplement(pOut) );
    assert( !Aig_IsComplement(pIn) );
    assert( Aig_ObjIsCo(pOut) );
    assert( Aig_ObjIsCi(pIn) );
    assert( iFrame == 0 || p->nWordsFrame < p->nWordsTotal );
    // get hold of the simulation information
    pSims0 = Ssw_ObjSim(p, pOut->Id) + p->nWordsFrame * iFrame;
    pSims1 = Ssw_ObjSim(p, pIn->Id) + p->nWordsFrame * (iFrame+1);
    // copy information as it is
    for ( i = 0; i < p->nWordsFrame; i++ )
        pSims1[i] = pSims0[i];
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlNodeTransferFirst( Ssw_Sml_t * p, Aig_Obj_t * pOut, Aig_Obj_t * pIn )
{
    unsigned * pSims0, * pSims1;
    int i;
    assert( !Aig_IsComplement(pOut) );
    assert( !Aig_IsComplement(pIn) );
    assert( Aig_ObjIsCo(pOut) );
    assert( Aig_ObjIsCi(pIn) );
    assert( p->nWordsFrame < p->nWordsTotal );
    // get hold of the simulation information
    pSims0 = Ssw_ObjSim(p, pOut->Id) + p->nWordsFrame * (p->nFrames-1);
    pSims1 = Ssw_ObjSim(p, pIn->Id);
    // copy information as it is
    for ( i = 0; i < p->nWordsFrame; i++ )
        pSims1[i] = pSims0[i];
}


/**Function*************************************************************

  Synopsis    [Assings random simulation info for the PIs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlInitialize( Ssw_Sml_t * p, int fInit )
{
    Aig_Obj_t * pObj;
    int i;
    if ( fInit )
    {
        assert( Aig_ManRegNum(p->pAig) > 0 );
        assert( Aig_ManRegNum(p->pAig) <= Aig_ManCiNum(p->pAig) );
        // assign random info for primary inputs
        Saig_ManForEachPi( p->pAig, pObj, i )
            Ssw_SmlAssignRandom( p, pObj );
        // assign the initial state for the latches
        Saig_ManForEachLo( p->pAig, pObj, i )
            Ssw_SmlObjAssignConst( p, pObj, 0, 0 );
    }
    else
    {
        Aig_ManForEachCi( p->pAig, pObj, i )
            Ssw_SmlAssignRandom( p, pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Assings random simulation info for the PIs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlInitializeSpecial( Ssw_Sml_t * p, Vec_Int_t * vInit )
{
    Aig_Obj_t * pObj;
    int Entry, i, nRegs;
    nRegs = Aig_ManRegNum(p->pAig);
    assert( nRegs > 0 );
    assert( nRegs <= Aig_ManCiNum(p->pAig) );
    assert( Vec_IntSize(vInit) == nRegs * p->nWordsFrame );
    // assign random info for primary inputs
    Saig_ManForEachPi( p->pAig, pObj, i )
        Ssw_SmlAssignRandom( p, pObj );
    // assign the initial state for the latches
    Vec_IntForEachEntry( vInit, Entry, i )
        Ssw_SmlObjAssignConstWord( p, Saig_ManLo(p->pAig, i % nRegs), Entry, 0, i / nRegs );
}

/**Function*************************************************************

  Synopsis    [Assings random simulation info for the PIs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlReinitialize( Ssw_Sml_t * p )
{
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i;
    assert( Aig_ManRegNum(p->pAig) > 0 );
    assert( Aig_ManRegNum(p->pAig) < Aig_ManCiNum(p->pAig) );
    // assign random info for primary inputs
    Saig_ManForEachPi( p->pAig, pObj, i )
        Ssw_SmlAssignRandom( p, pObj );
    // copy simulation info into the inputs
    Saig_ManForEachLiLo( p->pAig, pObjLi, pObjLo, i )
        Ssw_SmlNodeTransferFirst( p, pObjLi, pObjLo );
}

/**Function*************************************************************

  Synopsis    [Check if any of the POs becomes non-constant.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlCheckNonConstOutputs( Ssw_Sml_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Saig_ManForEachPo( p->pAig, pObj, i )
    {
        if ( p->pAig->nConstrs && i >= Saig_ManPoNum(p->pAig) - p->pAig->nConstrs )
            return 0;
        if ( !Ssw_SmlNodeIsZero(p, pObj) )
            return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Simulates AIG manager.]

  Description [Assumes that the PI simulation info is attached.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlSimulateOne( Ssw_Sml_t * p )
{
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int f, i;
    abctime clk;
clk = Abc_Clock();
    for ( f = 0; f < p->nFrames; f++ )
    {
        // simulate the nodes
        Aig_ManForEachNode( p->pAig, pObj, i )
            Ssw_SmlNodeSimulate( p, pObj, f );
        // copy simulation info into outputs
        Saig_ManForEachPo( p->pAig, pObj, i )
            Ssw_SmlNodeCopyFanin( p, pObj, f );
        // copy simulation info into outputs
        Saig_ManForEachLi( p->pAig, pObj, i )
            Ssw_SmlNodeCopyFanin( p, pObj, f );
        // quit if this is the last timeframe
        if ( f == p->nFrames - 1 )
            break;
        // copy simulation info into the inputs
        Saig_ManForEachLiLo( p->pAig, pObjLi, pObjLo, i )
            Ssw_SmlNodeTransferNext( p, pObjLi, pObjLo, f );
    }
p->timeSim += Abc_Clock() - clk;
p->nSimRounds++;
}

/**Function*************************************************************

  Synopsis    [Converts simulation information to be not normallized.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlUnnormalize( Ssw_Sml_t * p )
{
    Aig_Obj_t * pObj;
    unsigned * pSims;
    int i, k;
    // convert constant 1
    pSims  = Ssw_ObjSim( p, 0 );
    for ( i = 0; i < p->nWordsFrame; i++ )
        pSims[i] = ~pSims[i];
    // convert internal nodes
    Aig_ManForEachNode( p->pAig, pObj, k )
    {
        if ( pObj->fPhase == 0 )
            continue;
        pSims  = Ssw_ObjSim( p, pObj->Id );
        for ( i = 0; i < p->nWordsFrame; i++ )
            pSims[i] = ~pSims[i];
    }
    // PIs/POs are always stored in their natural state
}

/**Function*************************************************************

  Synopsis    [Simulates AIG manager.]

  Description [Assumes that the PI simulation info is attached.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlSimulateOneDyn_rec( Ssw_Sml_t * p, Aig_Obj_t * pObj, int f, int * pVisited, int nVisCounter )
{
//    if ( Aig_ObjIsTravIdCurrent(p->pAig, pObj) )
//        return;
//    Aig_ObjSetTravIdCurrent(p->pAig, pObj);
    if ( pVisited[p->nFrames*pObj->Id+f] == nVisCounter )
        return;
    pVisited[p->nFrames*pObj->Id+f] = nVisCounter;
    if ( Saig_ObjIsPi( p->pAig, pObj ) || Aig_ObjIsConst1(pObj) )
        return;
    if ( Saig_ObjIsLo( p->pAig, pObj ) )
    {
        if ( f == 0 )
            return;
        Ssw_SmlSimulateOneDyn_rec( p, Saig_ObjLoToLi(p->pAig, pObj), f-1, pVisited, nVisCounter );
        Ssw_SmlNodeTransferNext( p, Saig_ObjLoToLi(p->pAig, pObj), pObj, f-1 );
        return;
    }
    if ( Saig_ObjIsLi( p->pAig, pObj ) )
    {
        Ssw_SmlSimulateOneDyn_rec( p, Aig_ObjFanin0(pObj), f, pVisited, nVisCounter );
        Ssw_SmlNodeCopyFanin( p, pObj, f );
        return;
    }
    assert( Aig_ObjIsNode(pObj) );
    Ssw_SmlSimulateOneDyn_rec( p, Aig_ObjFanin0(pObj), f, pVisited, nVisCounter );
    Ssw_SmlSimulateOneDyn_rec( p, Aig_ObjFanin1(pObj), f, pVisited, nVisCounter );
    Ssw_SmlNodeSimulate( p, pObj, f );
}

/**Function*************************************************************

  Synopsis    [Simulates AIG manager.]

  Description [Assumes that the PI simulation info is attached.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlSimulateOneFrame( Ssw_Sml_t * p )
{
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i;
    abctime clk;
clk = Abc_Clock();
    // simulate the nodes
    Aig_ManForEachNode( p->pAig, pObj, i )
        Ssw_SmlNodeSimulate( p, pObj, 0 );
    // copy simulation info into outputs
    Saig_ManForEachLi( p->pAig, pObj, i )
        Ssw_SmlNodeCopyFanin( p, pObj, 0 );
    // copy simulation info into the inputs
    Saig_ManForEachLiLo( p->pAig, pObjLi, pObjLo, i )
        Ssw_SmlNodeTransferNext( p, pObjLi, pObjLo, 0 );
p->timeSim += Abc_Clock() - clk;
p->nSimRounds++;
}


/**Function*************************************************************

  Synopsis    [Allocates simulation manager.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Ssw_Sml_t * Ssw_SmlStart( Aig_Man_t * pAig, int nPref, int nFrames, int nWordsFrame )
{
    Ssw_Sml_t * p;
    p = (Ssw_Sml_t *)ABC_ALLOC( char, sizeof(Ssw_Sml_t) + sizeof(unsigned) * Aig_ManObjNumMax(pAig) * (nPref + nFrames) * nWordsFrame );
    memset( p, 0, sizeof(Ssw_Sml_t) + sizeof(unsigned) * (nPref + nFrames) * nWordsFrame );
    p->pAig        = pAig;
    p->nPref       = nPref;
    p->nFrames     = nPref + nFrames;
    p->nWordsFrame = nWordsFrame;
    p->nWordsTotal = (nPref + nFrames) * nWordsFrame;
    p->nWordsPref  = nPref * nWordsFrame;
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates simulation manager.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlClean( Ssw_Sml_t * p )
{
    memset( p->pData, 0, sizeof(unsigned) * Aig_ManObjNumMax(p->pAig) * p->nWordsTotal );
}

/**Function*************************************************************

  Synopsis    [Get simulation data.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Ssw_SmlSimDataPointers( Ssw_Sml_t * p )
{
    Vec_Ptr_t * vSimInfo;
    Aig_Obj_t * pObj;
    int i;
    vSimInfo = Vec_PtrStart( Aig_ManObjNumMax(p->pAig) );
    Aig_ManForEachObj( p->pAig, pObj, i )
        Vec_PtrWriteEntry( vSimInfo, i, Ssw_ObjSim(p, i) );
    return vSimInfo;
}

/**Function*************************************************************

  Synopsis    [Deallocates simulation manager.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlStop( Ssw_Sml_t * p )
{
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Performs simulation of the uninitialized circuit.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Ssw_Sml_t * Ssw_SmlSimulateComb( Aig_Man_t * pAig, int nWords )
{
    Ssw_Sml_t * p;
    p = Ssw_SmlStart( pAig, 0, 1, nWords );
    Ssw_SmlInitialize( p, 0 );
    Ssw_SmlSimulateOne( p );
    return p;
}

/**Function*************************************************************

  Synopsis    [Performs simulation of the initialized circuit.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Ssw_Sml_t * Ssw_SmlSimulateSeq( Aig_Man_t * pAig, int nPref, int nFrames, int nWords )
{
    Ssw_Sml_t * p;
    p = Ssw_SmlStart( pAig, nPref, nFrames, nWords );
    Ssw_SmlInitialize( p, 1 );
    Ssw_SmlSimulateOne( p );
    p->fNonConstOut = Ssw_SmlCheckNonConstOutputs( p );
    return p;
}

/**Function*************************************************************

  Synopsis    [Performs next round of sequential simulation.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlResimulateSeq( Ssw_Sml_t * p )
{
    Ssw_SmlReinitialize( p );
    Ssw_SmlSimulateOne( p );
    p->fNonConstOut = Ssw_SmlCheckNonConstOutputs( p );
}


/**Function*************************************************************

  Synopsis    [Returns the number of frames simulated in the manager.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlNumFrames( Ssw_Sml_t * p )
{
    return p->nFrames;
}

/**Function*************************************************************

  Synopsis    [Returns the total number of simulation words.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SmlNumWordsTotal( Ssw_Sml_t * p )
{
    return p->nWordsTotal;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the simulation info of the node.]

  Description [The simulation info is normalized unless procedure
  Ssw_SmlUnnormalize() is called in advance.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Ssw_SmlSimInfo( Ssw_Sml_t * p, Aig_Obj_t * pObj )
{
    assert( !Aig_IsComplement(pObj) );
    return Ssw_ObjSim( p, pObj->Id );
}

/**Function*************************************************************

  Synopsis    [Creates sequential counter-example from the simulation info.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Ssw_SmlGetCounterExample( Ssw_Sml_t * p )
{
    Abc_Cex_t * pCex;
    Aig_Obj_t * pObj;
    unsigned * pSims;
    int iPo, iFrame, iBit, i, k;

    // make sure the simulation manager has it
    assert( p->fNonConstOut );

    // find the first output that failed
    iPo = -1;
    iBit = -1;
    iFrame = -1;
    Saig_ManForEachPo( p->pAig, pObj, iPo )
    {
        if ( Ssw_SmlNodeIsZero(p, pObj) )
            continue;
        pSims = Ssw_ObjSim( p, pObj->Id );
        for ( i = p->nWordsPref; i < p->nWordsTotal; i++ )
            if ( pSims[i] )
            {
                iFrame = i / p->nWordsFrame;
                iBit = 32 * (i % p->nWordsFrame) + Aig_WordFindFirstBit( pSims[i] );
                break;
            }
        break;
    }
    assert( iPo < Aig_ManCoNum(p->pAig)-Aig_ManRegNum(p->pAig) );
    assert( iFrame < p->nFrames );
    assert( iBit < 32 * p->nWordsFrame );

    // allocate the counter example
    pCex = Abc_CexAlloc( Aig_ManRegNum(p->pAig), Aig_ManCiNum(p->pAig) - Aig_ManRegNum(p->pAig), iFrame + 1 );
    pCex->iPo    = iPo;
    pCex->iFrame = iFrame;

    // copy the bit data
    Saig_ManForEachLo( p->pAig, pObj, k )
    {
        pSims = Ssw_ObjSim( p, pObj->Id );
        if ( Abc_InfoHasBit( pSims, iBit ) )
            Abc_InfoSetBit( pCex->pData, k );
    }
    for ( i = 0; i <= iFrame; i++ )
    {
        Saig_ManForEachPi( p->pAig, pObj, k )
        {
            pSims = Ssw_ObjSim( p, pObj->Id );
            if ( Abc_InfoHasBit( pSims, 32 * p->nWordsFrame * i + iBit ) )
                Abc_InfoSetBit( pCex->pData, pCex->nRegs + pCex->nPis * i + k );
        }
    }
    // verify the counter example
    if ( !Saig_ManVerifyCex( p->pAig, pCex ) )
    {
        Abc_Print( 1, "Ssw_SmlGetCounterExample(): Counter-example is invalid.\n" );
        Abc_CexFree( pCex );
        pCex = NULL;
    }
    return pCex;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
