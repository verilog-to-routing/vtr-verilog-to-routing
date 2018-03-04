/**CFile****************************************************************

  FileName    [llb1Core.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Top-level procedure.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb1Core.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"
#include "aig/gia/gia.h"
#include "aig/gia/giaAig.h"

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
void Llb_ManSetDefaultParams( Gia_ParLlb_t * p )
{
    memset( p, 0, sizeof(Gia_ParLlb_t) );
    p->nBddMax       = 10000000;
    p->nIterMax      = 10000000;
    p->nClusterMax   =       20;
    p->nHintDepth    =        0;
    p->HintFirst     =        0;
    p->fUseFlow      =        0;  // use flow 
    p->nVolumeMax    =      100;  // max volume
    p->nVolumeMin    =       30;  // min volume
    p->nPartValue    =        5;  // partitioning value
    p->fBackward     =        0;  // forward by default
    p->fReorder      =        1;
    p->fIndConstr    =        0;
    p->fUsePivots    =        0;
    p->fCluster      =        0;
    p->fSchedule     =        0;
    p->fDumpReached  =        0;
    p->fVerbose      =        0;
    p->fVeryVerbose  =        0;
    p->fSilent       =        0;
    p->TimeLimit     =        0;
//    p->TimeLimit     =        0;
    p->TimeLimitGlo  =        0;
    p->TimeTarget    =        0;
    p->iFrame        =       -1;
}


/**Function*************************************************************

  Synopsis    [Prints statistics about MFFCs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManPrintAig( Llb_Man_t * p )
{
    Abc_Print( 1, "pi =%3d  ",    Saig_ManPiNum(p->pAig) );
    Abc_Print( 1, "po =%3d  ",    Saig_ManPoNum(p->pAig) );
    Abc_Print( 1, "ff =%3d  ",    Saig_ManRegNum(p->pAig) );
    Abc_Print( 1, "int =%5d  ",   Vec_IntSize(p->vVar2Obj)-Aig_ManCiNum(p->pAig)-Saig_ManRegNum(p->pAig) );
    Abc_Print( 1, "var =%5d  ",   Vec_IntSize(p->vVar2Obj) );
    Abc_Print( 1, "part =%5d  ",  Vec_PtrSize(p->vGroups)-2 );
    Abc_Print( 1, "and =%5d  ",   Aig_ManNodeNum(p->pAig) );
    Abc_Print( 1, "lev =%4d  ",   Aig_ManLevelNum(p->pAig) );
//    Abc_Print( 1, "cut =%4d  ",   Llb_ManCrossCut(p->pAig) );
    Abc_Print( 1, "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManModelCheckAig( Aig_Man_t * pAigGlo, Gia_ParLlb_t * pPars, Vec_Int_t * vHints, DdManager ** pddGlo )
{
    Llb_Man_t * p = NULL; 
    Aig_Man_t * pAig;
    int RetValue = -1;
    abctime clk = Abc_Clock();

    if ( pPars->fIndConstr )
    {
        assert( vHints == NULL );
        vHints = Llb_ManDeriveConstraints( pAigGlo );
    }

    // derive AIG for hints
    if ( vHints == NULL )
        pAig = Aig_ManDupSimple( pAigGlo );
    else
    {
        if ( pPars->fVerbose )
            Llb_ManPrintEntries( pAigGlo, vHints );
        pAig = Aig_ManDupSimpleWithHints( pAigGlo, vHints );
    }


    if ( pPars->fUseFlow )
    {
//        p = Llb_ManStartFlow( pAigGlo, pAig, pPars );
    }
    else
    {
        p = Llb_ManStart( pAigGlo, pAig, pPars );
        if ( pPars->fVerbose )
        {
            Llb_ManPrintAig( p );
            printf( "Original matrix:          " );
            Llb_MtrPrintMatrixStats( p->pMatrix );
            if ( pPars->fVeryVerbose )
                Llb_MtrPrint( p->pMatrix, 1 );
        }
        if ( pPars->fCluster )
        {
            Llb_ManCluster( p->pMatrix );
            if ( pPars->fVerbose )
            {
                printf( "Matrix after clustering:  " );
                Llb_MtrPrintMatrixStats( p->pMatrix );
                if ( pPars->fVeryVerbose )
                    Llb_MtrPrint( p->pMatrix, 1 );
            }
        }
        if ( pPars->fSchedule )
        {
            Llb_MtrSchedule( p->pMatrix );
            if ( pPars->fVerbose )
            {
                printf( "Matrix after scheduling:  " );
                Llb_MtrPrintMatrixStats( p->pMatrix );
                if ( pPars->fVeryVerbose )
                    Llb_MtrPrint( p->pMatrix, 1 );
            }
        }
    }
    
    if ( !p->pPars->fSkipReach ) 
        RetValue = Llb_ManReachability( p, vHints, pddGlo );
    Llb_ManStop( p );

    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    if ( pPars->fIndConstr )
        Vec_IntFreeP( &vHints );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManModelCheckGia( Gia_Man_t * pGia, Gia_ParLlb_t * pPars )
{
    Gia_Man_t * pGia2;
    Aig_Man_t * pAig;
    int RetValue = -1;
    pGia2 = Gia_ManDupDfs( pGia );
    pAig  = Gia_ManToAigSimple( pGia2 );
    Gia_ManStop( pGia2 );
//Aig_ManShow( pAig, 0, NULL ); 

    if ( pPars->nHintDepth == 0 )
        RetValue = Llb_ManModelCheckAig( pAig, pPars, NULL, NULL );
    else
        RetValue = Llb_ManModelCheckAigWithHints( pAig, pPars );
    pGia->pCexSeq = pAig->pSeqModel; pAig->pSeqModel = NULL;
    Aig_ManStop( pAig );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

