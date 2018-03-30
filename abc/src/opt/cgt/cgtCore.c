/**CFile****************************************************************

  FileName    [cgtCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Clock gating package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cgtCore.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cgtInt.h"
#include "misc/bar/bar.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_SetDefaultParams( Cgt_Par_t * p )
{
    memset( p, 0, sizeof(Cgt_Par_t) );
    p->nLevelMax  =    25;   // the max number of levels to look for clock-gates
    p->nCandMax   =  1000;   // the max number of candidates at each node
    p->nOdcMax    =     0;   // the max number of ODC levels to consider
    p->nConfMax   =    10;   // the max number of conflicts at a node
    p->nVarsMin   =  1000;   // the min number of vars to recycle the SAT solver
    p->nFlopsMin  =    10;   // the min number of flops to recycle the SAT solver
    p->fAreaOnly  =     0;   // derive clock-gating to minimize area
    p->fVerbose   =     0;   // verbosity flag
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation does not filter out this candidate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cgt_SimulationFilter( Cgt_Man_t * p, Aig_Obj_t * pCandPart, Aig_Obj_t * pMiterPart )
{
    unsigned * pInfoCand, * pInfoMiter;
    int w, nWords = Abc_BitWordNum( p->nPatts );  
    pInfoCand  = (unsigned *)Vec_PtrEntry( p->vPatts, Aig_ObjId(Aig_Regular(pCandPart)) );
    pInfoMiter = (unsigned *)Vec_PtrEntry( p->vPatts, Aig_ObjId(pMiterPart) );
    // C => !M -- true   is the same as    C & M -- false
    if ( !Aig_IsComplement(pCandPart) )
    {
        for ( w = 0; w < nWords; w++ )
            if ( pInfoCand[w] & pInfoMiter[w] )
                return 0;
    }
    else
    {
        for ( w = 0; w < nWords; w++ )
            if ( ~pInfoCand[w] & pInfoMiter[w] )
                return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Saves one simulation pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_SimulationRecord( Cgt_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( p->pPart, pObj, i )
        if ( sat_solver_var_value( p->pSat, p->pCnf->pVarNums[i] ) )
            Abc_InfoSetBit( (unsigned *)Vec_PtrEntry(p->vPatts, i), p->nPatts );
    p->nPatts++;
    if ( p->nPatts == 32 * p->nPattWords )
    {
        Vec_PtrReallocSimInfo( p->vPatts );
        Vec_PtrCleanSimInfo( p->vPatts, p->nPattWords, 2 * p->nPattWords );
        p->nPattWords *= 2;
    }
}

/**Function*************************************************************

  Synopsis    [Performs clock-gating for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_ClockGatingRangeCheck( Cgt_Man_t * p, int iStart, int nOutputs )
{
    Vec_Ptr_t * vNodes = p->vFanout;
    Aig_Obj_t * pMiter, * pCand, * pMiterFrame, * pCandFrame, * pMiterPart, * pCandPart;
    int i, k, RetValue, nCalls;
    assert( Vec_VecSize(p->vGatesAll) == Aig_ManCoNum(p->pFrame) );
    // go through all the registers inputs of this range
    for ( i = iStart; i < iStart + nOutputs; i++ )
    {
        nCalls = p->nCalls;
        pMiter = Saig_ManLi( p->pAig, i );
        Cgt_ManDetectCandidates( p->pAig, p->vUseful, Aig_ObjFanin0(pMiter), p->pPars->nLevelMax, vNodes );
        // go through the candidates of this PO
        Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pCand, k )
        {
            // get the corresponding nodes from the frames
            pCandFrame  = (Aig_Obj_t *)pCand->pData;
            pMiterFrame = (Aig_Obj_t *)pMiter->pData;
            // get the corresponding nodes from the part
            pCandPart   = (Aig_Obj_t *)pCandFrame->pData;
            pMiterPart  = (Aig_Obj_t *)pMiterFrame->pData;
            // try direct polarity
            if ( Cgt_SimulationFilter( p, pCandPart, pMiterPart ) )
            {
                RetValue = Cgt_CheckImplication( p, pCandPart, pMiterPart );
                if ( RetValue == 1 )
                {
                    Vec_VecPush( p->vGatesAll, i, pCand );
                    continue;
                }
                if ( RetValue == 0 )
                    Cgt_SimulationRecord( p );
            }
            else
                p->nCallsFiltered++;
            // try reverse polarity
            if ( Cgt_SimulationFilter( p, Aig_Not(pCandPart), pMiterPart ) )
            {
                RetValue = Cgt_CheckImplication( p, Aig_Not(pCandPart), pMiterPart );
                if ( RetValue == 1 )
                {
                    Vec_VecPush( p->vGatesAll, i, Aig_Not(pCand) );
                    continue;
                }
                if ( RetValue == 0 )
                    Cgt_SimulationRecord( p );
            }
            else
                p->nCallsFiltered++;
        }

        if ( p->pPars->fVerbose )
        {
//            printf( "Flop %3d : Cand = %4d. Gate = %4d. SAT calls = %3d.\n", 
//                i, Vec_PtrSize(vNodes), Vec_PtrSize(Vec_VecEntry(p->vGatesAll, i)), p->nCalls-nCalls );
        }

    }
} 

/**Function*************************************************************

  Synopsis    [Performs clock-gating for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cgt_ClockGatingRange( Cgt_Man_t * p, int iStart )
{
    int nOutputs, iStop;
    abctime clk, clkTotal = Abc_Clock();
    int nCallsUnsat    = p->nCallsUnsat;
    int nCallsSat      = p->nCallsSat;
    int nCallsUndec    = p->nCallsUndec;
    int nCallsFiltered = p->nCallsFiltered;
clk = Abc_Clock();
    p->pPart = Cgt_ManDupPartition( p->pFrame, p->pPars->nVarsMin, p->pPars->nFlopsMin, iStart, p->pCare, p->vSuppsInv, &nOutputs );
    p->pCnf  = Cnf_DeriveSimple( p->pPart, nOutputs );
    p->pSat  = (sat_solver *)Cnf_DataWriteIntoSolver( p->pCnf, 1, 0 );
    sat_solver_compress( p->pSat );
    p->vPatts = Vec_PtrAllocSimInfo( Aig_ManObjNumMax(p->pPart), p->nPattWords );
    Vec_PtrCleanSimInfo( p->vPatts, 0, p->nPattWords );
p->timePrepare += Abc_Clock() - clk;
    Cgt_ClockGatingRangeCheck( p, iStart, nOutputs );
    iStop = iStart + nOutputs;
    if ( p->pPars->fVeryVerbose )
    {
        printf( "%5d : D =%4d. C =%5d. Var =%6d. Pr =%5d. Cex =%5d. F =%4d. Saved =%6d. ",
            iStart, iStop-iStart, Aig_ManCoNum(p->pPart)-nOutputs, p->pSat->size, 
            p->nCallsUnsat-nCallsUnsat, 
            p->nCallsSat  -nCallsSat, 
            p->nCallsUndec-nCallsUndec,
            p->nCallsFiltered-nCallsFiltered );
        ABC_PRT( "Time", Abc_Clock() - clkTotal );
    }
    Cgt_ManClean( p );
    p->nRecycles++;
    return iStop;
}

/**Function*************************************************************

  Synopsis    [Performs clock-gating for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Cgt_ClockGatingCandidates( Aig_Man_t * pAig, Aig_Man_t * pCare, Cgt_Par_t * pPars, Vec_Int_t * vUseful )
{
    Bar_Progress_t * pProgress = NULL;
    Cgt_Par_t Pars; 
    Cgt_Man_t * p;
    Vec_Vec_t * vGatesAll;
    int iStart;
    abctime clk = Abc_Clock(), clkTotal = Abc_Clock();
    // reset random numbers
    Aig_ManRandom( 1 );
    if ( pPars == NULL )
        Cgt_SetDefaultParams( pPars = &Pars );    
    p = Cgt_ManCreate( pAig, pCare, pPars );
    p->vUseful = vUseful;
    p->pFrame = Cgt_ManDeriveAigForGating( p );
p->timeAig += Abc_Clock() - clk;
    assert( Aig_ManCoNum(p->pFrame) == Saig_ManRegNum(p->pAig) );
    pProgress = Bar_ProgressStart( stdout, Aig_ManCoNum(p->pFrame) );
    for ( iStart = 0; iStart < Aig_ManCoNum(p->pFrame); )
    {
        Bar_ProgressUpdate( pProgress, iStart, NULL );
        iStart = Cgt_ClockGatingRange( p, iStart );
    }
    Bar_ProgressStop( pProgress );
    vGatesAll = p->vGatesAll;
    p->vGatesAll = NULL;
p->timeTotal = Abc_Clock() - clkTotal;
    Cgt_ManStop( p );
    return vGatesAll;
}

/**Function*************************************************************

  Synopsis    [Performs clock-gating for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Cgt_ClockGatingInt( Aig_Man_t * pAig, Aig_Man_t * pCare, Cgt_Par_t * pPars, Vec_Int_t * vUseful )
{
    Vec_Vec_t * vGatesAll, * vGates;
    vGatesAll = Cgt_ClockGatingCandidates( pAig, pCare, pPars, vUseful );
    if ( pPars->fAreaOnly )
        vGates = Cgt_ManDecideArea( pAig, vGatesAll, pPars->nOdcMax, pPars->fVerbose );
    else
        vGates = Cgt_ManDecideSimple( pAig, vGatesAll, pPars->nOdcMax, pPars->fVerbose );
    Vec_VecFree( vGatesAll );
    return vGates;
}
Aig_Man_t * Cgt_ClockGating( Aig_Man_t * pAig, Aig_Man_t * pCare, Cgt_Par_t * pPars )
{
    Aig_Man_t * pGated;
    Vec_Vec_t * vGates = Cgt_ClockGatingInt( pAig, pCare, pPars, NULL );
    int nNodesUsed;
    if ( pPars->fVerbose )
    {
//        printf( "Before CG: " );
//        Aig_ManPrintStats( pAig );
    }
    pGated = Cgt_ManDeriveGatedAig( pAig, vGates, pPars->fAreaOnly, &nNodesUsed );
    if ( pPars->fVerbose )
    {
//        printf( "After  CG: " );
//        Aig_ManPrintStats( pGated );
        printf( "Nodes: Before CG = %6d. After CG = %6d. (%6.2f %%).  Total after CG = %6d.\n", 
            Aig_ManNodeNum(pAig), nNodesUsed, 
            100.0*nNodesUsed/Aig_ManNodeNum(pAig), 
            Aig_ManNodeNum(pGated) );
    }
    Vec_VecFree( vGates );
    return pGated;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

