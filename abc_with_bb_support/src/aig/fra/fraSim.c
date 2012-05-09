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

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Assigns random patterns to the PI node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_NodeAssignRandom( Fra_Man_t * p, Aig_Obj_t * pObj )
{
    unsigned * pSims;
    int i;
    assert( Aig_ObjIsPi(pObj) );
    pSims = Fra_ObjSim(pObj);
    for ( i = 0; i < p->nSimWords; i++ )
        pSims[i] = Fra_ObjRandomSim();
}

/**Function*************************************************************

  Synopsis    [Assigns constant patterns to the PI node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_NodeAssignConst( Fra_Man_t * p, Aig_Obj_t * pObj, int fConst1, int iFrame )
{
    unsigned * pSims;
    int i;
    assert( Aig_ObjIsPi(pObj) );
    pSims = Fra_ObjSim(pObj) + p->pPars->nSimWords * iFrame;
    for ( i = 0; i < p->pPars->nSimWords; i++ )
        pSims[i] = fConst1? ~(unsigned)0 : 0;
}

/**Function*************************************************************

  Synopsis    [Assings random simulation info for the PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_AssignRandom( Fra_Man_t * p, int fInit )
{
    Aig_Obj_t * pObj;
    int i;
    if ( fInit )
    {
        assert( Aig_ManRegNum(p->pManAig) > 0 );
        assert( Aig_ManRegNum(p->pManAig) < Aig_ManPiNum(p->pManAig) );
        // assign random info for primary inputs
        Aig_ManForEachPiSeq( p->pManAig, pObj, i )
            Fra_NodeAssignRandom( p, pObj );
        // assign the initial state for the latches
        Aig_ManForEachLoSeq( p->pManAig, pObj, i )
            Fra_NodeAssignConst( p, pObj, 0, 0 );
    }
    else
    {
        Aig_ManForEachPi( p->pManAig, pObj, i )
            Fra_NodeAssignRandom( p, pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Assings distance-1 simulation info for the PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_AssignDist1( Fra_Man_t * p, unsigned * pPat )
{
    Aig_Obj_t * pObj;
    int f, i, k, Limit, nTruePis;
    if ( p->pPars->nFramesK == 0 )
    {
        assert( p->nFramesAll == 1 );
        // copy the PI info 
        Aig_ManForEachPi( p->pManAig, pObj, i )
            Fra_NodeAssignConst( p, pObj, Aig_InfoHasBit(pPat, i), 0 );
        // flip one bit
        Limit = AIG_MIN( Aig_ManPiNum(p->pManAig), p->nSimWords * 32 - 1 );
        for ( i = 0; i < Limit; i++ )
            Aig_InfoXorBit( Fra_ObjSim( Aig_ManPi(p->pManAig,i) ), i+1 );
    }
    else
    {
        // copy the PI info for each frame
        nTruePis = Aig_ManPiNum(p->pManAig) - Aig_ManRegNum(p->pManAig);
        for ( f = 0; f < p->nFramesAll; f++ )
            Aig_ManForEachPiSeq( p->pManAig, pObj, i )
                Fra_NodeAssignConst( p, pObj, Aig_InfoHasBit(pPat, nTruePis * f + i), f );
        // copy the latch info 
        k = 0;
        Aig_ManForEachLoSeq( p->pManAig, pObj, i )
            Fra_NodeAssignConst( p, pObj, Aig_InfoHasBit(pPat, nTruePis * p->nFramesAll + k++), 0 );
        assert( p->pManFraig == NULL || nTruePis * p->nFramesAll + k == Aig_ManPiNum(p->pManFraig) );

        // flip one bit of the last frame
        if ( p->nFramesAll == 2 )
        {
            Limit = AIG_MIN( nTruePis, p->pPars->nSimWords * 32 - 1 );
            for ( i = 0; i < Limit; i++ )
                Aig_InfoXorBit( Fra_ObjSim( Aig_ManPi(p->pManAig, i) ), i+1 );
//            Aig_InfoXorBit( Fra_ObjSim( Aig_ManPi(p->pManAig, nTruePis*(p->nFramesAll-2) + i) ), i+1 );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_NodeHasZeroSim( Aig_Obj_t * pObj )
{
    Fra_Man_t * p = pObj->pData;
    unsigned * pSims;
    int i;
    pSims = Fra_ObjSim(pObj);
    for ( i = 0; i < p->nSimWords; i++ )
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
int Fra_NodeCompareSims( Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 )
{
    Fra_Man_t * p = pObj0->pData;
    unsigned * pSims0, * pSims1;
    int i;
    pSims0 = Fra_ObjSim(pObj0);
    pSims1 = Fra_ObjSim(pObj1);
    for ( i = 0; i < p->nSimWords; i++ )
        if ( pSims0[i] != pSims1[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes hash value of the node using its simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Fra_NodeHashSims( Aig_Obj_t * pObj )
{
    Fra_Man_t * p = pObj->pData;
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
    assert( p->nSimWords <= 128 );
    uHash = 0;
    pSims = Fra_ObjSim(pObj);
    for ( i = 0; i < p->nSimWords; i++ )
        uHash ^= pSims[i] * s_FPrimes[i];
    return uHash;
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_NodeSimulate( Fra_Man_t * p, Aig_Obj_t * pObj, int iFrame )
{
    unsigned * pSims, * pSims0, * pSims1;
    int fCompl, fCompl0, fCompl1, i;
    int nSimWords = p->pPars->nSimWords;
    assert( !Aig_IsComplement(pObj) );
    assert( Aig_ObjIsNode(pObj) );
    assert( iFrame == 0 || nSimWords < p->nSimWords );
    // get hold of the simulation information
    pSims  = Fra_ObjSim(pObj) + nSimWords * iFrame;
    pSims0 = Fra_ObjSim(Aig_ObjFanin0(pObj)) + nSimWords * iFrame;
    pSims1 = Fra_ObjSim(Aig_ObjFanin1(pObj)) + nSimWords * iFrame;
    // get complemented attributes of the children using their random info
    fCompl  = pObj->fPhase;
    fCompl0 = Aig_ObjPhaseReal(Aig_ObjChild0(pObj));
    fCompl1 = Aig_ObjPhaseReal(Aig_ObjChild1(pObj));
    // simulate
    if ( fCompl0 && fCompl1 )
    {
        if ( fCompl )
            for ( i = 0; i < nSimWords; i++ )
                pSims[i] = (pSims0[i] | pSims1[i]);
        else
            for ( i = 0; i < nSimWords; i++ )
                pSims[i] = ~(pSims0[i] | pSims1[i]);
    }
    else if ( fCompl0 && !fCompl1 )
    {
        if ( fCompl )
            for ( i = 0; i < nSimWords; i++ )
                pSims[i] = (pSims0[i] | ~pSims1[i]);
        else
            for ( i = 0; i < nSimWords; i++ )
                pSims[i] = (~pSims0[i] & pSims1[i]);
    }
    else if ( !fCompl0 && fCompl1 )
    {
        if ( fCompl )
            for ( i = 0; i < nSimWords; i++ )
                pSims[i] = (~pSims0[i] | pSims1[i]);
        else
            for ( i = 0; i < nSimWords; i++ )
                pSims[i] = (pSims0[i] & ~pSims1[i]);
    }
    else // if ( !fCompl0 && !fCompl1 )
    {
        if ( fCompl )
            for ( i = 0; i < nSimWords; i++ )
                pSims[i] = ~(pSims0[i] & pSims1[i]);
        else
            for ( i = 0; i < nSimWords; i++ )
                pSims[i] = (pSims0[i] & pSims1[i]);
    }
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_NodeCopyFanin( Fra_Man_t * p, Aig_Obj_t * pObj, int iFrame )
{
    unsigned * pSims, * pSims0;
    int fCompl, fCompl0, i;
    int nSimWords = p->pPars->nSimWords;
    assert( !Aig_IsComplement(pObj) );
    assert( Aig_ObjIsPo(pObj) );
    assert( iFrame == 0 || nSimWords < p->nSimWords );
    // get hold of the simulation information
    pSims  = Fra_ObjSim(pObj) + nSimWords * iFrame;
    pSims0 = Fra_ObjSim(Aig_ObjFanin0(pObj)) + nSimWords * iFrame;
    // get complemented attributes of the children using their random info
    fCompl  = pObj->fPhase;
    fCompl0 = Aig_ObjPhaseReal(Aig_ObjChild0(pObj));
    // copy information as it is
    if ( fCompl0 )
        for ( i = 0; i < nSimWords; i++ )
            pSims[i] = ~pSims0[i];
    else
        for ( i = 0; i < nSimWords; i++ )
            pSims[i] = pSims0[i];
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_NodeTransferNext( Fra_Man_t * p, Aig_Obj_t * pOut, Aig_Obj_t * pIn, int iFrame )
{
    unsigned * pSims0, * pSims1;
    int i, nSimWords = p->pPars->nSimWords;
    assert( !Aig_IsComplement(pOut) );
    assert( !Aig_IsComplement(pIn) );
    assert( Aig_ObjIsPo(pOut) );
    assert( Aig_ObjIsPi(pIn) );
    assert( iFrame == 0 || nSimWords < p->nSimWords );
    // get hold of the simulation information
    pSims0 = Fra_ObjSim(pOut) + nSimWords * iFrame;
    pSims1 = Fra_ObjSim(pIn) + nSimWords * (iFrame+1);
    // copy information as it is
    for ( i = 0; i < nSimWords; i++ )
        pSims1[i] = pSims0[i];
}


/**Function*************************************************************

  Synopsis    [Generated const 0 pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SavePattern0( Fra_Man_t * p, int fInit )
{
    memset( p->pPatWords, 0, sizeof(unsigned) * p->nPatWords );
}

/**Function*************************************************************

  Synopsis    [[Generated const 1 pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SavePattern1( Fra_Man_t * p, int fInit )
{
    Aig_Obj_t * pObj;
    int i, k, nTruePis;
    memset( p->pPatWords, 0xff, sizeof(unsigned) * p->nPatWords );
    if ( !fInit )
        return;
    nTruePis = Aig_ManPiNum(p->pManAig) - Aig_ManRegNum(p->pManAig);
    k = 0;
    Aig_ManForEachLoSeq( p->pManAig, pObj, i )
        Aig_InfoXorBit( p->pPatWords, nTruePis * p->nFramesAll + k++ );
}

/**Function*************************************************************

  Synopsis    [Copy pattern from the solver into the internal storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SavePattern( Fra_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    memset( p->pPatWords, 0, sizeof(unsigned) * p->nPatWords );
    Aig_ManForEachPi( p->pManFraig, pObj, i )
        if ( p->pSat->model.ptr[Fra_ObjSatNum(pObj)] == l_True )
            Aig_InfoSetBit( p->pPatWords, i );
/*
    printf( "Pattern: " );
    Aig_ManForEachPi( p->pManFraig, pObj, i )
        printf( "%d", Aig_InfoHasBit( p->pPatWords, i ) );
    printf( "\n" );
*/
}

/**Function*************************************************************

  Synopsis    [Cleans pattern scores.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_CleanPatScores( Fra_Man_t * p )
{
    int i, nLimit = p->nSimWords * 32;
    for ( i = 0; i < nLimit; i++ )
        p->pPatScores[i] = 0;
}

/**Function*************************************************************

  Synopsis    [Adds to pattern scores.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_AddToPatScores( Fra_Man_t * p, Aig_Obj_t * pClass, Aig_Obj_t * pClassNew )
{
    unsigned * pSims0, * pSims1;
    unsigned uDiff;
    int i, w;
    // get hold of the simulation information
    pSims0 = Fra_ObjSim(pClass);
    pSims1 = Fra_ObjSim(pClassNew);
    // iterate through the differences and record the score
    for ( w = 0; w < p->nSimWords; w++ )
    {
        uDiff = pSims0[w] ^ pSims1[w];
        if ( uDiff == 0 )
            continue;
        for ( i = 0; i < 32; i++ )
            if ( uDiff & ( 1 << i ) )
                p->pPatScores[w*32+i]++;
    }
}

/**Function*************************************************************

  Synopsis    [Selects the best pattern.]

  Description [Returns 1 if such pattern is found.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_SelectBestPat( Fra_Man_t * p )
{
    unsigned * pSims;
    Aig_Obj_t * pObj;
    int i, nLimit = p->nSimWords * 32, MaxScore = 0, BestPat = -1;
    for ( i = 1; i < nLimit; i++ )
    {
        if ( MaxScore < p->pPatScores[i] )
        {
            MaxScore = p->pPatScores[i];
            BestPat = i;
        }
    }
    if ( MaxScore == 0 )
        return 0;
//    if ( MaxScore > p->pPars->MaxScore )
//    printf( "Max score is %3d.  ", MaxScore );
    // copy the best pattern into the selected pattern
    memset( p->pPatWords, 0, sizeof(unsigned) * p->nPatWords );
    Aig_ManForEachPi( p->pManAig, pObj, i )
    {
        pSims = Fra_ObjSim(pObj);
        if ( Aig_InfoHasBit(pSims, BestPat) )
            Aig_InfoSetBit(p->pPatWords, i);
    }
    return MaxScore;
}

/**Function*************************************************************

  Synopsis    [Simulates AIG manager.]

  Description [Assumes that the PI simulation info is attached.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SimulateOne( Fra_Man_t * p )
{
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int f, i, clk;
clk = clock();
    for ( f = 0; f < p->nFramesAll; f++ )
    {
        // simulate the nodes
        Aig_ManForEachNode( p->pManAig, pObj, i )
            Fra_NodeSimulate( p, pObj, f );
        if ( f == p->nFramesAll - 1 )
            break;
        // copy simulation info into outputs
        Aig_ManForEachLiSeq( p->pManAig, pObj, i )
            Fra_NodeCopyFanin( p, pObj, f );
        // copy simulation info into the inputs
//        for ( i = 0; i < Aig_ManRegNum(p->pManAig); i++ )
//            Fra_NodeTransferNext( p, Aig_ManLi(p->pManAig, i), Aig_ManLo(p->pManAig, i), f );
        Aig_ManForEachLiLoSeq( p->pManAig, pObjLi, pObjLo, i )
            Fra_NodeTransferNext( p, pObjLi, pObjLo, f );
    }
p->timeSim += clock() - clk;
p->nSimRounds++;
}

/**Function*************************************************************

  Synopsis    [Resimulates fraiging manager after finding a counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_Resimulate( Fra_Man_t * p )
{
    int nChanges, clk;
    Fra_AssignDist1( p, p->pPatWords );
    Fra_SimulateOne( p );
    if ( p->pPars->fPatScores )
        Fra_CleanPatScores( p );
    if ( p->pPars->fProve && Fra_CheckOutputSims(p) )
        return;
clk = clock();
    nChanges = Fra_ClassesRefine( p->pCla );
    nChanges += Fra_ClassesRefine1( p->pCla );
p->timeRef += clock() - clk;
    if ( nChanges < 1 )
        printf( "Error: A counter-example did not refine classes!\n" );
    assert( nChanges >= 1 );
//printf( "Refined classes = %5d.   Changes = %4d.\n", Vec_PtrSize(p->vClasses), nChanges );

    if ( !p->pPars->fPatScores )
        return;

    // perform additional simulation using dist1 patterns derived from successful patterns
    while ( Fra_SelectBestPat(p) > p->pPars->MaxScore )
    {
        Fra_AssignDist1( p, p->pPatWords );
        Fra_SimulateOne( p );
        Fra_CleanPatScores( p );
        if ( p->pPars->fProve && Fra_CheckOutputSims(p) )
            return;
clk = clock();
        nChanges = Fra_ClassesRefine( p->pCla );
        nChanges += Fra_ClassesRefine1( p->pCla );
p->timeRef += clock() - clk;
//printf( "Refined class!!! = %5d.   Changes = %4d.   Pairs = %6d.\n", p->lClasses.nItems, nChanges, Fra_CountPairsClasses(p) );
        if ( nChanges == 0 )
            break;
    }
}

/**Function*************************************************************

  Synopsis    [Performs simulation of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_Simulate( Fra_Man_t * p, int fInit )
{
    int nChanges, nClasses, clk;
    assert( !fInit || Aig_ManRegNum(p->pManAig) );
    // start the classes
    Fra_AssignRandom( p, fInit );
    Fra_SimulateOne( p );
    Fra_ClassesPrepare( p->pCla );
//    Fra_ClassesPrint( p->pCla, 0 );
//printf( "Starting classes = %5d.   Pairs = %6d.\n", p->lClasses.nItems, Fra_CountPairsClasses(p) );

    // refine classes by walking 0/1 patterns
    Fra_SavePattern0( p, fInit );
    Fra_AssignDist1( p, p->pPatWords );
    Fra_SimulateOne( p );
    if ( p->pPars->fProve && Fra_CheckOutputSims(p) )
        return;
clk = clock();
    nChanges = Fra_ClassesRefine( p->pCla );
    nChanges += Fra_ClassesRefine1( p->pCla );
p->timeRef += clock() - clk;
//printf( "Refined classes  = %5d.   Changes = %4d.   Pairs = %6d.\n", p->lClasses.nItems, nChanges, Fra_CountPairsClasses(p) );
    Fra_SavePattern1( p, fInit );
    Fra_AssignDist1( p, p->pPatWords );
    Fra_SimulateOne( p );
    if ( p->pPars->fProve && Fra_CheckOutputSims(p) )
        return;
clk = clock();
    nChanges = Fra_ClassesRefine( p->pCla );
    nChanges += Fra_ClassesRefine1( p->pCla );
p->timeRef += clock() - clk;

//printf( "Refined classes  = %5d.   Changes = %4d.   Pairs = %6d.\n", p->lClasses.nItems, nChanges, Fra_CountPairsClasses(p) );
    // refine classes by random simulation
    do {
        Fra_AssignRandom( p, fInit );
        Fra_SimulateOne( p );
        nClasses = Vec_PtrSize(p->pCla->vClasses);
        if ( p->pPars->fProve && Fra_CheckOutputSims(p) )
            return;
clk = clock();
        nChanges = Fra_ClassesRefine( p->pCla );
        nChanges += Fra_ClassesRefine1( p->pCla );
p->timeRef += clock() - clk;
//printf( "Refined classes  = %5d.   Changes = %4d.   Pairs = %6d.\n", p->lClasses.nItems, nChanges, Fra_CountPairsClasses(p) );
    } while ( (double)nChanges / nClasses > p->pPars->dSimSatur );

//    if ( p->pPars->fVerbose )
//    printf( "Consts = %6d. Classes = %6d. Literals = %6d.\n", 
//        Vec_PtrSize(p->pCla->vClasses1), Vec_PtrSize(p->pCla->vClasses), Fra_ClassesCountLits(p->pCla) );

//    Fra_ClassesPrint( p->pCla, 0 );
}
 
/**Function*************************************************************

  Synopsis    [Creates the counter-example from the successful pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_CheckOutputSimsSavePattern( Fra_Man_t * p, Aig_Obj_t * pObj )
{ 
    unsigned * pSims;
    int i, k, BestPat, * pModel;
    // find the word of the pattern
    pSims = Fra_ObjSim(pObj);
    for ( i = 0; i < p->nSimWords; i++ )
        if ( pSims[i] )
            break;
    assert( i < p->nSimWords );
    // find the bit of the pattern
    for ( k = 0; k < 32; k++ )
        if ( pSims[i] & (1 << k) )
            break;
    assert( k < 32 );
    // determine the best pattern
    BestPat = i * 32 + k;
    // fill in the counter-example data
    pModel = ALLOC( int, Aig_ManPiNum(p->pManFraig) );
    Aig_ManForEachPi( p->pManAig, pObj, i )
    {
        pModel[i] = Aig_InfoHasBit(Fra_ObjSim(pObj), BestPat);
//        printf( "%d", pModel[i] );
    }
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
int Fra_CheckOutputSims( Fra_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    // make sure the reference simulation pattern does not detect the bug
    pObj = Aig_ManPo( p->pManAig, 0 );
    assert( Aig_ObjFanin0(pObj)->fPhase == (unsigned)Aig_ObjFaninC0(pObj) ); 
    Aig_ManForEachPo( p->pManAig, pObj, i )
    {
        if ( !Fra_NodeHasZeroSim( Aig_ObjFanin0(pObj) ) )
        {
            // create the counter-example from this pattern
            Fra_CheckOutputSimsSavePattern( p, Aig_ObjFanin0(pObj) );
            return 1;
        }
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


