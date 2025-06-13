/**CFile****************************************************************

  FileName    [fraSim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraSim.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"
#include "aig/saig/saig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes hash value of the node using its simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_SmlNodeHash( Aig_Obj_t * pObj, int nTableSize )
{
    Fra_Man_t * p = (Fra_Man_t *)pObj->pData;
    static int s_FPrimes[128] = { 
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
//    assert( p->pSml->nWordsTotal <= 128 );
    uHash = 0;
    pSims = Fra_ObjSim(p->pSml, pObj->Id);
    for ( i = p->pSml->nWordsPref; i < p->pSml->nWordsTotal; i++ )
        uHash ^= pSims[i] * s_FPrimes[i & 0x7F];
    return uHash % nTableSize;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_SmlNodeIsConst( Aig_Obj_t * pObj )
{
    Fra_Man_t * p = (Fra_Man_t *)pObj->pData;
    unsigned * pSims;
    int i;
    pSims = Fra_ObjSim(p->pSml, pObj->Id);
    for ( i = p->pSml->nWordsPref; i < p->pSml->nWordsTotal; i++ )
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
int Fra_SmlNodesAreEqual( Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 )
{
    Fra_Man_t * p = (Fra_Man_t *)pObj0->pData;
    unsigned * pSims0, * pSims1;
    int i;
    pSims0 = Fra_ObjSim(p->pSml, pObj0->Id);
    pSims1 = Fra_ObjSim(p->pSml, pObj1->Id);
    for ( i = p->pSml->nWordsPref; i < p->pSml->nWordsTotal; i++ )
        if ( pSims0[i] != pSims1[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Counts the number of 1s in the XOR of simulation data.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_SmlNodeNotEquWeight( Fra_Sml_t * p, int Left, int Right )
{
    unsigned * pSimL, * pSimR;
    int k, Counter = 0;
    pSimL = Fra_ObjSim( p, Left );
    pSimR = Fra_ObjSim( p, Right );
    for ( k = p->nWordsPref; k < p->nWordsTotal; k++ )
        Counter += Aig_WordCountOnes( pSimL[k] ^ pSimR[k] );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_SmlNodeIsZero( Fra_Sml_t * p, Aig_Obj_t * pObj )
{
    unsigned * pSims;
    int i;
    pSims = Fra_ObjSim(p, pObj->Id);
    for ( i = p->nWordsPref; i < p->nWordsTotal; i++ )
        if ( pSims[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Counts the number of one's in the patten of the output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_SmlNodeCountOnes( Fra_Sml_t * p, Aig_Obj_t * pObj )
{
    unsigned * pSims;
    int i, Counter = 0;
    pSims = Fra_ObjSim(p, pObj->Id);
    for ( i = 0; i < p->nWordsTotal; i++ )
        Counter += Aig_WordCountOnes( pSims[i] );
    return Counter;
}



/**Function*************************************************************

  Synopsis    [Generated const 0 pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlSavePattern0( Fra_Man_t * p, int fInit )
{
    memset( p->pPatWords, 0, sizeof(unsigned) * p->nPatWords );
}

/**Function*************************************************************

  Synopsis    [[Generated const 1 pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlSavePattern1( Fra_Man_t * p, int fInit )
{
    Aig_Obj_t * pObj;
    int i, k, nTruePis;
    memset( p->pPatWords, 0xff, sizeof(unsigned) * p->nPatWords );
    if ( !fInit )
        return;
    // clear the state bits to correspond to all-0 initial state
    nTruePis = Aig_ManCiNum(p->pManAig) - Aig_ManRegNum(p->pManAig);
    k = 0;
    Aig_ManForEachLoSeq( p->pManAig, pObj, i )
        Abc_InfoXorBit( p->pPatWords, nTruePis * p->nFramesAll + k++ );
}

/**Function*************************************************************

  Synopsis    [Copy pattern from the solver into the internal storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlSavePattern( Fra_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    memset( p->pPatWords, 0, sizeof(unsigned) * p->nPatWords );
    Aig_ManForEachCi( p->pManFraig, pObj, i )
//        if ( p->pSat->model.ptr[Fra_ObjSatNum(pObj)] == l_True )
        if ( sat_solver_var_value(p->pSat, Fra_ObjSatNum(pObj)) )
            Abc_InfoSetBit( p->pPatWords, i );

    if ( p->vCex )
    {
        Vec_IntClear( p->vCex );
        for ( i = 0; i < Aig_ManCiNum(p->pManAig) - Aig_ManRegNum(p->pManAig); i++ )
            Vec_IntPush( p->vCex, Abc_InfoHasBit( p->pPatWords, i ) );
        for ( i = Aig_ManCiNum(p->pManFraig) - Aig_ManRegNum(p->pManFraig); i < Aig_ManCiNum(p->pManFraig); i++ )
            Vec_IntPush( p->vCex, Abc_InfoHasBit( p->pPatWords, i ) );
    }

/*
    printf( "Pattern: " );
    Aig_ManForEachCi( p->pManFraig, pObj, i )
        printf( "%d", Abc_InfoHasBit( p->pPatWords, i ) );
    printf( "\n" );
*/
}



/**Function*************************************************************

  Synopsis    [Creates the counter-example from the successful pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlCheckOutputSavePattern( Fra_Man_t * p, Aig_Obj_t * pObjPo )
{ 
    Aig_Obj_t * pFanin, * pObjPi;
    unsigned * pSims;
    int i, k, BestPat, * pModel;
    // find the word of the pattern
    pFanin = Aig_ObjFanin0(pObjPo);
    pSims = Fra_ObjSim(p->pSml, pFanin->Id);
    for ( i = 0; i < p->pSml->nWordsTotal; i++ )
        if ( pSims[i] )
            break;
    assert( i < p->pSml->nWordsTotal );
    // find the bit of the pattern
    for ( k = 0; k < 32; k++ )
        if ( pSims[i] & (1 << k) )
            break;
    assert( k < 32 );
    // determine the best pattern
    BestPat = i * 32 + k;
    // fill in the counter-example data
    pModel = ABC_ALLOC( int, Aig_ManCiNum(p->pManFraig)+1 );
    Aig_ManForEachCi( p->pManAig, pObjPi, i )
    {
        pModel[i] = Abc_InfoHasBit(Fra_ObjSim(p->pSml, pObjPi->Id), BestPat);
//        printf( "%d", pModel[i] );
    }
    pModel[Aig_ManCiNum(p->pManAig)] = pObjPo->Id;
//    printf( "\n" );
    // set the model
    assert( p->pManFraig->pData == NULL );
    p->pManFraig->pData = pModel;
    return;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the one of the output is already non-constant 0.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_SmlCheckOutput( Fra_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    // make sure the reference simulation pattern does not detect the bug
    pObj = Aig_ManCo( p->pManAig, 0 );
    assert( Aig_ObjFanin0(pObj)->fPhase == (unsigned)Aig_ObjFaninC0(pObj) ); 
    Aig_ManForEachCo( p->pManAig, pObj, i )
    {
        if ( !Fra_SmlNodeIsConst( Aig_ObjFanin0(pObj) ) )
        {
            // create the counter-example from this pattern
            Fra_SmlCheckOutputSavePattern( p, pObj );
            return 1;
        }
    }
    return 0;
}



/**Function*************************************************************

  Synopsis    [Assigns random patterns to the PI node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlAssignRandom( Fra_Sml_t * p, Aig_Obj_t * pObj )
{
    unsigned * pSims;
    int i;
    assert( Aig_ObjIsCi(pObj) );
    pSims = Fra_ObjSim( p, pObj->Id );
    for ( i = 0; i < p->nWordsTotal; i++ )
        pSims[i] = Fra_ObjRandomSim();
}

/**Function*************************************************************

  Synopsis    [Assigns constant patterns to the PI node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlAssignConst( Fra_Sml_t * p, Aig_Obj_t * pObj, int fConst1, int iFrame )
{
    unsigned * pSims;
    int i;
    assert( Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj) );
    pSims = Fra_ObjSim( p, pObj->Id ) + p->nWordsFrame * iFrame;
    for ( i = 0; i < p->nWordsFrame; i++ )
        pSims[i] = fConst1? ~(unsigned)0 : 0;
}

/**Function*************************************************************

  Synopsis    [Assings random simulation info for the PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlInitialize( Fra_Sml_t * p, int fInit )
{
    Aig_Obj_t * pObj;
    int i;
    if ( fInit )
    {
        assert( Aig_ManRegNum(p->pAig) > 0 );
        assert( Aig_ManRegNum(p->pAig) < Aig_ManCiNum(p->pAig) );
        // assign random info for primary inputs
        Aig_ManForEachPiSeq( p->pAig, pObj, i )
            Fra_SmlAssignRandom( p, pObj );
        // assign the initial state for the latches
        Aig_ManForEachLoSeq( p->pAig, pObj, i )
            Fra_SmlAssignConst( p, pObj, 0, 0 );
    }
    else
    {
        Aig_ManForEachCi( p->pAig, pObj, i )
            Fra_SmlAssignRandom( p, pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Assings distance-1 simulation info for the PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlAssignDist1( Fra_Sml_t * p, unsigned * pPat )
{
    Aig_Obj_t * pObj;
    int f, i, k, Limit, nTruePis;
    assert( p->nFrames > 0 );
    if ( p->nFrames == 1 )
    {
        // copy the PI info 
        Aig_ManForEachCi( p->pAig, pObj, i )
            Fra_SmlAssignConst( p, pObj, Abc_InfoHasBit(pPat, i), 0 );
        // flip one bit
        Limit = Abc_MinInt( Aig_ManCiNum(p->pAig), p->nWordsTotal * 32 - 1 );
        for ( i = 0; i < Limit; i++ )
            Abc_InfoXorBit( Fra_ObjSim( p, Aig_ManCi(p->pAig,i)->Id ), i+1 );
    }
    else
    {
        int fUseDist1 = 0;

        // copy the PI info for each frame
        nTruePis = Aig_ManCiNum(p->pAig) - Aig_ManRegNum(p->pAig);
        for ( f = 0; f < p->nFrames; f++ )
            Aig_ManForEachPiSeq( p->pAig, pObj, i )
                Fra_SmlAssignConst( p, pObj, Abc_InfoHasBit(pPat, nTruePis * f + i), f );
        // copy the latch info 
        k = 0;
        Aig_ManForEachLoSeq( p->pAig, pObj, i )
            Fra_SmlAssignConst( p, pObj, Abc_InfoHasBit(pPat, nTruePis * p->nFrames + k++), 0 );
//        assert( p->pManFraig == NULL || nTruePis * p->nFrames + k == Aig_ManCiNum(p->pManFraig) );

        // flip one bit of the last frame
        if ( fUseDist1 ) //&& p->nFrames == 2 )
        {
            Limit = Abc_MinInt( nTruePis, p->nWordsFrame * 32 - 1 );
            for ( i = 0; i < Limit; i++ )
                Abc_InfoXorBit( Fra_ObjSim( p, Aig_ManCi(p->pAig, i)->Id ) + p->nWordsFrame*(p->nFrames-1), i+1 );
        }
    }
}


/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlNodeSimulate( Fra_Sml_t * p, Aig_Obj_t * pObj, int iFrame )
{
    unsigned * pSims, * pSims0, * pSims1;
    int fCompl, fCompl0, fCompl1, i;
    assert( !Aig_IsComplement(pObj) );
    assert( Aig_ObjIsNode(pObj) );
    assert( iFrame == 0 || p->nWordsFrame < p->nWordsTotal );
    // get hold of the simulation information
    pSims  = Fra_ObjSim(p, pObj->Id) + p->nWordsFrame * iFrame;
    pSims0 = Fra_ObjSim(p, Aig_ObjFanin0(pObj)->Id) + p->nWordsFrame * iFrame;
    pSims1 = Fra_ObjSim(p, Aig_ObjFanin1(pObj)->Id) + p->nWordsFrame * iFrame;
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
int Fra_SmlNodesCompareInFrame( Fra_Sml_t * p, Aig_Obj_t * pObj0, Aig_Obj_t * pObj1, int iFrame0, int iFrame1 )
{
    unsigned * pSims0, * pSims1;
    int i;
    assert( !Aig_IsComplement(pObj0) );
    assert( !Aig_IsComplement(pObj1) );
    assert( iFrame0 == 0 || p->nWordsFrame < p->nWordsTotal );
    assert( iFrame1 == 0 || p->nWordsFrame < p->nWordsTotal );
    // get hold of the simulation information
    pSims0  = Fra_ObjSim(p, pObj0->Id) + p->nWordsFrame * iFrame0;
    pSims1  = Fra_ObjSim(p, pObj1->Id) + p->nWordsFrame * iFrame1;
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
void Fra_SmlNodeCopyFanin( Fra_Sml_t * p, Aig_Obj_t * pObj, int iFrame )
{
    unsigned * pSims, * pSims0;
    int fCompl, fCompl0, i;
    assert( !Aig_IsComplement(pObj) );
    assert( Aig_ObjIsCo(pObj) );
    assert( iFrame == 0 || p->nWordsFrame < p->nWordsTotal );
    // get hold of the simulation information
    pSims  = Fra_ObjSim(p, pObj->Id) + p->nWordsFrame * iFrame;
    pSims0 = Fra_ObjSim(p, Aig_ObjFanin0(pObj)->Id) + p->nWordsFrame * iFrame;
    // get complemented attributes of the children using their random info
    fCompl  = pObj->fPhase;
    fCompl0 = Aig_ObjPhaseReal(Aig_ObjChild0(pObj));
    // copy information as it is
//    if ( Aig_ObjFaninC0(pObj) )
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
void Fra_SmlNodeTransferNext( Fra_Sml_t * p, Aig_Obj_t * pOut, Aig_Obj_t * pIn, int iFrame )
{
    unsigned * pSims0, * pSims1;
    int i;
    assert( !Aig_IsComplement(pOut) );
    assert( !Aig_IsComplement(pIn) );
    assert( Aig_ObjIsCo(pOut) );
    assert( Aig_ObjIsCi(pIn) );
    assert( iFrame == 0 || p->nWordsFrame < p->nWordsTotal );
    // get hold of the simulation information
    pSims0 = Fra_ObjSim(p, pOut->Id) + p->nWordsFrame * iFrame;
    pSims1 = Fra_ObjSim(p, pIn->Id) + p->nWordsFrame * (iFrame+1);
    // copy information as it is
    for ( i = 0; i < p->nWordsFrame; i++ )
        pSims1[i] = pSims0[i];
}


/**Function*************************************************************

  Synopsis    [Check if any of the POs becomes non-constant.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_SmlCheckNonConstOutputs( Fra_Sml_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachPoSeq( p->pAig, pObj, i )
        if ( !Fra_SmlNodeIsZero(p, pObj) )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Simulates AIG manager.]

  Description [Assumes that the PI simulation info is attached.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlSimulateOne( Fra_Sml_t * p )
{
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int f, i;
    abctime clk;
clk = Abc_Clock();
    for ( f = 0; f < p->nFrames; f++ )
    {
        // simulate the nodes
        Aig_ManForEachNode( p->pAig, pObj, i )
            Fra_SmlNodeSimulate( p, pObj, f );
        // copy simulation info into outputs
        Aig_ManForEachPoSeq( p->pAig, pObj, i )
            Fra_SmlNodeCopyFanin( p, pObj, f );
        // quit if this is the last timeframe
        if ( f == p->nFrames - 1 )
            break;
        // copy simulation info into outputs
        Aig_ManForEachLiSeq( p->pAig, pObj, i )
            Fra_SmlNodeCopyFanin( p, pObj, f );
        // copy simulation info into the inputs
        Aig_ManForEachLiLoSeq( p->pAig, pObjLi, pObjLo, i )
            Fra_SmlNodeTransferNext( p, pObjLi, pObjLo, f );
    }
p->timeSim += Abc_Clock() - clk;
p->nSimRounds++;
}


/**Function*************************************************************

  Synopsis    [Resimulates fraiging manager after finding a counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlResimulate( Fra_Man_t * p )
{
    int nChanges;
    abctime clk;
    Fra_SmlAssignDist1( p->pSml, p->pPatWords );
    Fra_SmlSimulateOne( p->pSml );
//    if ( p->pPars->fPatScores )
//        Fra_CleanPatScores( p );
    if ( p->pPars->fProve && Fra_SmlCheckOutput(p) )
        return;
clk = Abc_Clock();
    nChanges = Fra_ClassesRefine( p->pCla );
    nChanges += Fra_ClassesRefine1( p->pCla, 1, NULL );
    if ( p->pCla->vImps )
        nChanges += Fra_ImpRefineUsingCex( p, p->pCla->vImps );
    if ( p->vOneHots )
        nChanges += Fra_OneHotRefineUsingCex( p, p->vOneHots );
p->timeRef += Abc_Clock() - clk;
    if ( !p->pPars->nFramesK && nChanges < 1 )
        printf( "Error: A counter-example did not refine classes!\n" );
//    assert( nChanges >= 1 );
//printf( "Refined classes = %5d.   Changes = %4d.\n", Vec_PtrSize(p->vClasses), nChanges );
}

/**Function*************************************************************

  Synopsis    [Performs simulation of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlSimulate( Fra_Man_t * p, int fInit )
{
    int fVerbose = 0;
    int nChanges, nClasses;
    abctime clk;
    assert( !fInit || Aig_ManRegNum(p->pManAig) );
    // start the classes
    Fra_SmlInitialize( p->pSml, fInit );
    Fra_SmlSimulateOne( p->pSml );
    if ( p->pPars->fProve && Fra_SmlCheckOutput(p) )
        return;
    Fra_ClassesPrepare( p->pCla, p->pPars->fLatchCorr, 0 );
//    Fra_ClassesPrint( p->pCla, 0 );
if ( fVerbose )
printf( "Starting classes = %5d.   Lits = %6d.\n", Vec_PtrSize(p->pCla->vClasses), Fra_ClassesCountLits(p->pCla) );

//return;

    // refine classes by walking 0/1 patterns
    Fra_SmlSavePattern0( p, fInit );
    Fra_SmlAssignDist1( p->pSml, p->pPatWords );
    Fra_SmlSimulateOne( p->pSml );
    if ( p->pPars->fProve && Fra_SmlCheckOutput(p) )
        return;
clk = Abc_Clock();
    nChanges = Fra_ClassesRefine( p->pCla );
    nChanges += Fra_ClassesRefine1( p->pCla, 1, NULL );
p->timeRef += Abc_Clock() - clk;
if ( fVerbose )
printf( "Refined classes  = %5d.   Changes = %4d.   Lits = %6d.\n", Vec_PtrSize(p->pCla->vClasses), nChanges, Fra_ClassesCountLits(p->pCla) );
    Fra_SmlSavePattern1( p, fInit );
    Fra_SmlAssignDist1( p->pSml, p->pPatWords );
    Fra_SmlSimulateOne( p->pSml );
    if ( p->pPars->fProve && Fra_SmlCheckOutput(p) )
        return;
clk = Abc_Clock();
    nChanges = Fra_ClassesRefine( p->pCla );
    nChanges += Fra_ClassesRefine1( p->pCla, 1, NULL );
p->timeRef += Abc_Clock() - clk;

if ( fVerbose )
printf( "Refined classes  = %5d.   Changes = %4d.   Lits = %6d.\n", Vec_PtrSize(p->pCla->vClasses), nChanges, Fra_ClassesCountLits(p->pCla) );
    // refine classes by random simulation
    do {
        Fra_SmlInitialize( p->pSml, fInit );
        Fra_SmlSimulateOne( p->pSml );
        nClasses = Vec_PtrSize(p->pCla->vClasses);
        if ( p->pPars->fProve && Fra_SmlCheckOutput(p) )
            return;
clk = Abc_Clock();
        nChanges = Fra_ClassesRefine( p->pCla );
        nChanges += Fra_ClassesRefine1( p->pCla, 1, NULL );
p->timeRef += Abc_Clock() - clk;
if ( fVerbose )
printf( "Refined classes  = %5d.   Changes = %4d.   Lits = %6d.\n", Vec_PtrSize(p->pCla->vClasses), nChanges, Fra_ClassesCountLits(p->pCla) );
    } while ( (double)nChanges / nClasses > p->pPars->dSimSatur );

//    if ( p->pPars->fVerbose )
//    printf( "Consts = %6d. Classes = %6d. Literals = %6d.\n", 
//        Vec_PtrSize(p->pCla->vClasses1), Vec_PtrSize(p->pCla->vClasses), Fra_ClassesCountLits(p->pCla) );
//    Fra_ClassesPrint( p->pCla, 0 );
}
 

/**Function*************************************************************

  Synopsis    [Allocates simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fra_Sml_t * Fra_SmlStart( Aig_Man_t * pAig, int nPref, int nFrames, int nWordsFrame )
{
    Fra_Sml_t * p;
    p = (Fra_Sml_t *)ABC_ALLOC( char, sizeof(Fra_Sml_t) + sizeof(unsigned) * Aig_ManObjNumMax(pAig) * (nPref + nFrames) * nWordsFrame );
    memset( p, 0, sizeof(Fra_Sml_t) + sizeof(unsigned) * (nPref + nFrames) * nWordsFrame );
    p->pAig        = pAig;
    p->nPref       = nPref;
    p->nFrames     = nPref + nFrames;
    p->nWordsFrame = nWordsFrame;
    p->nWordsTotal = (nPref + nFrames) * nWordsFrame;
    p->nWordsPref  = nPref * nWordsFrame;
    // constant 1 is initialized to 0 because we store values modulus phase (pObj->fPhase)
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlStop( Fra_Sml_t * p )
{
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Performs simulation of the uninitialized circuit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fra_Sml_t * Fra_SmlSimulateComb( Aig_Man_t * pAig, int nWords, int fCheckMiter )
{
    Fra_Sml_t * p;
    p = Fra_SmlStart( pAig, 0, 1, nWords );
    Fra_SmlInitialize( p, 0 );
    Fra_SmlSimulateOne( p );
    if ( fCheckMiter )
        p->fNonConstOut = Fra_SmlCheckNonConstOutputs( p );
    return p;
}

/**Function*************************************************************

  Synopsis    [Reads simulation patterns from file.]

  Description [Each pattern contains the given number (nInputs) of binary digits.
  No other symbols (except spaces and line endings) are allowed in the file.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Fra_SmlSimulateReadFile( char * pFileName )
{
    Vec_Str_t * vRes;
    FILE * pFile;
    int c;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" with simulation patterns.\n", pFileName );
        return NULL;
    }
    vRes = Vec_StrAlloc( 1000 );
    while ( (c = fgetc(pFile)) != EOF )
    {
        if ( c == '0' || c == '1' )
            Vec_StrPush( vRes, (char)(c - '0') );
        else if ( c != ' ' && c != '\r' && c != '\n' && c != '\t' )
        {
            printf( "File \"%s\" contains symbol (%c) other than \'0\' or \'1\'.\n", pFileName, (char)c );
            Vec_StrFreeP( &vRes );
            break;
        }
    }
    fclose( pFile );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Assigns simulation patters derived from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlInitializeGiven( Fra_Sml_t * p, Vec_Str_t * vSimInfo )
{
    Aig_Obj_t * pObj;
    unsigned * pSims;
    int i, k, nPats = Vec_StrSize(vSimInfo) / Aig_ManCiNum(p->pAig);
    int nPatsPadded = p->nWordsTotal * 32;
    assert( Aig_ManRegNum(p->pAig) == 0 );
    assert( Vec_StrSize(vSimInfo) % Aig_ManCiNum(p->pAig) == 0 );
    assert( nPats <= nPatsPadded );
    Aig_ManForEachCi( p->pAig, pObj, i )
    {
        pSims = Fra_ObjSim( p, pObj->Id );
        // clean data
        for ( k = 0; k < p->nWordsTotal; k++ )
            pSims[k] = 0;
        // load patterns
        for ( k = 0; k < nPats; k++ )
            if ( Vec_StrEntry(vSimInfo, k * Aig_ManCiNum(p->pAig) + i) )
                Abc_InfoSetBit( pSims, k );
        // pad the remaining bits with the value of the last pattern
        for ( ; k < nPatsPadded; k++ )
            if ( Vec_StrEntry(vSimInfo, (nPats-1) * Aig_ManCiNum(p->pAig) + i) )
                Abc_InfoSetBit( pSims, k );
    }
}

/**Function*************************************************************

  Synopsis    [Prints output values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SmlPrintOutputs( Fra_Sml_t * p, int nPatterns )
{
    Aig_Obj_t * pObj;
    unsigned * pSims;
    int i, k;
    for ( k = 0; k < nPatterns; k++ )
    {
        Aig_ManForEachCi( p->pAig, pObj, i )
        {
            pSims = Fra_ObjSim( p, pObj->Id );
            printf( "%d", Abc_InfoHasBit( pSims, k ) );
        }
        printf( " " );               ;
        Aig_ManForEachCo( p->pAig, pObj, i )
        {
            pSims = Fra_ObjSim( p, pObj->Id );
            printf( "%d", Abc_InfoHasBit( pSims, k ) );
        }
        printf( "\n" );               ;
    }
}

/**Function*************************************************************

  Synopsis    [Assigns simulation patters derived from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fra_Sml_t * Fra_SmlSimulateCombGiven( Aig_Man_t * pAig, char * pFileName, int fCheckMiter, int fVerbose )
{
    Vec_Str_t * vSimInfo;
    Fra_Sml_t * p;
    int nPatterns;
    assert( Aig_ManRegNum(pAig) == 0 );
    // read comb patterns from file
    vSimInfo = Fra_SmlSimulateReadFile( pFileName );
    if ( vSimInfo == NULL )
        return NULL;
    if ( Vec_StrSize(vSimInfo) % Aig_ManCiNum(pAig) != 0 )
    {
        printf( "File \"%s\": The number of binary digits (%d) is not divisible by the number of primary inputs (%d).\n", 
            pFileName, Vec_StrSize(vSimInfo), Aig_ManCiNum(pAig) );
        Vec_StrFree( vSimInfo );
        return NULL;
    }
    p = Fra_SmlStart( pAig, 0, 1, Abc_BitWordNum(Vec_StrSize(vSimInfo) / Aig_ManCiNum(pAig)) );
    Fra_SmlInitializeGiven( p, vSimInfo );
    nPatterns = Vec_StrSize(vSimInfo) / Aig_ManCiNum(pAig);
    Vec_StrFree( vSimInfo );
    Fra_SmlSimulateOne( p );
    if ( fCheckMiter )
        p->fNonConstOut = Fra_SmlCheckNonConstOutputs( p );
    if ( fVerbose )
        Fra_SmlPrintOutputs( p, nPatterns );
    return p;
}

/**Function*************************************************************

  Synopsis    [Performs simulation of the initialized circuit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fra_Sml_t * Fra_SmlSimulateSeq( Aig_Man_t * pAig, int nPref, int nFrames, int nWords, int fCheckMiter )
{
    Fra_Sml_t * p;
    p = Fra_SmlStart( pAig, nPref, nFrames, nWords );
    Fra_SmlInitialize( p, 1 );
    Fra_SmlSimulateOne( p );
    if ( fCheckMiter )
        p->fNonConstOut = Fra_SmlCheckNonConstOutputs( p );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates sequential counter-example from the simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Fra_SmlGetCounterExample( Fra_Sml_t * p )
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
    Aig_ManForEachPoSeq( p->pAig, pObj, iPo )
    {
        if ( Fra_SmlNodeIsZero(p, pObj) )
            continue;
        pSims = Fra_ObjSim( p, pObj->Id );
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
    Aig_ManForEachLoSeq( p->pAig, pObj, k )
    {
        pSims = Fra_ObjSim( p, pObj->Id );
        if ( Abc_InfoHasBit( pSims, iBit ) )
            Abc_InfoSetBit( pCex->pData, k );
    }
    for ( i = 0; i <= iFrame; i++ )
    {
        Aig_ManForEachPiSeq( p->pAig, pObj, k )
        {
            pSims = Fra_ObjSim( p, pObj->Id );
            if ( Abc_InfoHasBit( pSims, 32 * p->nWordsFrame * i + iBit ) )
                Abc_InfoSetBit( pCex->pData, pCex->nRegs + pCex->nPis * i + k );
        }
    }
    // verify the counter example
    if ( !Saig_ManVerifyCex( p->pAig, pCex ) )
    {
        printf( "Fra_SmlGetCounterExample(): Counter-example is invalid.\n" );
        Abc_CexFree( pCex );
        pCex = NULL;
    }
    return pCex;
}
 
/**Function*************************************************************

  Synopsis    [Generates seq counter-example from the combinational one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Fra_SmlCopyCounterExample( Aig_Man_t * pAig, Aig_Man_t * pFrames, int * pModel )
{
    Abc_Cex_t * pCex;
    Aig_Obj_t * pObj;
    int i, nFrames, nTruePis, nTruePos, iPo, iFrame;
    // get the number of frames
    assert( Aig_ManRegNum(pAig) > 0 );
    assert( Aig_ManRegNum(pFrames) == 0 );
    nTruePis = Aig_ManCiNum(pAig)-Aig_ManRegNum(pAig);
    nTruePos = Aig_ManCoNum(pAig)-Aig_ManRegNum(pAig);
    nFrames = Aig_ManCiNum(pFrames) / nTruePis;
    assert( nTruePis * nFrames == Aig_ManCiNum(pFrames) );
    assert( nTruePos * nFrames == Aig_ManCoNum(pFrames) );
    // find the PO that failed
    iPo = -1;
    iFrame = -1;
    Aig_ManForEachCo( pFrames, pObj, i )
        if ( pObj->Id == pModel[Aig_ManCiNum(pFrames)] )
        {
            iPo = i % nTruePos;
            iFrame = i / nTruePos;
            break;
        }
    assert( iPo >= 0 );
    // allocate the counter example
    pCex = Abc_CexAlloc( Aig_ManRegNum(pAig), nTruePis, iFrame + 1 );
    pCex->iPo    = iPo;
    pCex->iFrame = iFrame;

    // copy the bit data
    for ( i = 0; i < Aig_ManCiNum(pFrames); i++ )
    {
        if ( pModel[i] )
            Abc_InfoSetBit( pCex->pData, pCex->nRegs + i );
        if ( pCex->nRegs + i == pCex->nBits - 1 )
            break;
    }

    // verify the counter example
    if ( !Saig_ManVerifyCex( pAig, pCex ) )
    {
        printf( "Fra_SmlGetCounterExample(): Counter-example is invalid.\n" );
        Abc_CexFree( pCex );
        pCex = NULL;
    }
    return pCex;

}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

