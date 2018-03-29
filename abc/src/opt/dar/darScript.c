/**CFile****************************************************************

  FileName    [darScript.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting.]

  Synopsis    [Rewriting scripts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: darScript.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "darInt.h"
#include "proof/dch/dch.h"
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

  Synopsis    [Performs one iteration of AIG rewriting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dar_ManRewriteDefault( Aig_Man_t * pAig )
{
    Aig_Man_t * pTemp;
    Dar_RwrPar_t Pars, * pPars = &Pars;
    Dar_ManDefaultRwrParams( pPars );
    pAig = Aig_ManDupDfs( pAig ); 
    Dar_ManRewrite( pAig, pPars );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    return pAig;
}

/**Function*************************************************************

  Synopsis    [Reproduces script "rwsat".]

  Description []
               
  SideEffects [This procedure does not tighten level during restructuring.]

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dar_ManRwsat( Aig_Man_t * pAig, int fBalance, int fVerbose )
//alias rwsat       "st; rw -l; b -l; rw -l; rf -l"
{
    Aig_Man_t * pTemp;
    abctime Time = pAig->Time2Quit;

    Dar_RwrPar_t ParsRwr, * pParsRwr = &ParsRwr;
    Dar_RefPar_t ParsRef, * pParsRef = &ParsRef;

    Dar_ManDefaultRwrParams( pParsRwr );
    Dar_ManDefaultRefParams( pParsRef );

    pParsRwr->fUpdateLevel = 0;
    pParsRef->fUpdateLevel = 0;

    pParsRwr->fVerbose = fVerbose;
    pParsRef->fVerbose = fVerbose;
//printf( "1" );
    pAig = Aig_ManDupDfs( pAig ); 
    if ( fVerbose ) printf( "Starting:  " ), Aig_ManPrintStats( pAig );

//printf( "2" );
    // balance
    if ( fBalance )
    {
    pAig->Time2Quit = Time;
    pAig = Dar_ManBalance( pTemp = pAig, 0 );
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Balance:   " ), Aig_ManPrintStats( pAig );
    if ( Time && Abc_Clock() > Time )
        { if ( pAig ) Aig_ManStop( pAig ); return NULL; }
    }
   
//Aig_ManDumpBlif( pAig, "inter.blif", NULL, NULL );
//printf( "3" );
    // rewrite
    pAig->Time2Quit = Time;
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Rewrite:   " ), Aig_ManPrintStats( pAig );
    if ( Time && Abc_Clock() > Time )
        { if ( pAig ) Aig_ManStop( pAig ); return NULL; }

//printf( "4" );
    // refactor
    pAig->Time2Quit = Time;
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Refactor:  " ), Aig_ManPrintStats( pAig );
    if ( Time && Abc_Clock() > Time )
        { if ( pAig ) Aig_ManStop( pAig ); return NULL; }

//printf( "5" );
    // balance
    if ( fBalance )
    {
    pAig->Time2Quit = Time;
    pAig = Dar_ManBalance( pTemp = pAig, 0 );
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Balance:   " ), Aig_ManPrintStats( pAig );
    if ( Time && Abc_Clock() > Time )
        { if ( pAig ) Aig_ManStop( pAig ); return NULL; }
    }

//printf( "6" );
    // rewrite
    pAig->Time2Quit = Time;
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Rewrite:   " ), Aig_ManPrintStats( pAig );
    if ( Time && Abc_Clock() > Time )
        { if ( pAig ) Aig_ManStop( pAig ); return NULL; }

//printf( "7" );
    return pAig;
}

/**Function*************************************************************

  Synopsis    [Reproduces script "compress".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dar_ManCompress( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fPower, int fVerbose )
//alias compress2   "b -l; rw -l; rwz -l; b -l; rwz -l; b -l"
{
    Aig_Man_t * pTemp;

    Dar_RwrPar_t ParsRwr, * pParsRwr = &ParsRwr;
    Dar_RefPar_t ParsRef, * pParsRef = &ParsRef;

    Dar_ManDefaultRwrParams( pParsRwr );
    Dar_ManDefaultRefParams( pParsRef );

    pParsRwr->fUpdateLevel = fUpdateLevel;
    pParsRef->fUpdateLevel = fUpdateLevel;

    pParsRwr->fPower = fPower;

    pParsRwr->fVerbose = 0;//fVerbose;
    pParsRef->fVerbose = 0;//fVerbose;

    pAig = Aig_ManDupDfs( pAig ); 
    if ( fVerbose ) printf( "Starting:  " ), Aig_ManPrintStats( pAig );
/*
    // balance
    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Balance:   " ), Aig_ManPrintStats( pAig );
    }
*/    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Rewrite:   " ), Aig_ManPrintStats( pAig );
    
    // refactor
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Refactor:  " ), Aig_ManPrintStats( pAig );

    // balance
    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Balance:   " ), Aig_ManPrintStats( pAig );
    }

    pParsRwr->fUseZeros = 1;
    pParsRef->fUseZeros = 1;
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "RewriteZ:  " ), Aig_ManPrintStats( pAig );

    return pAig;
}

/**Function*************************************************************

  Synopsis    [Reproduces script "compress2".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dar_ManCompress2( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fFanout, int fPower, int fVerbose )
//alias compress2   "b -l; rw -l; rf -l; b -l; rw -l; rwz -l; b -l; rfz -l; rwz -l; b -l"
{
    Aig_Man_t * pTemp;

    Dar_RwrPar_t ParsRwr, * pParsRwr = &ParsRwr;
    Dar_RefPar_t ParsRef, * pParsRef = &ParsRef;

    Dar_ManDefaultRwrParams( pParsRwr );
    Dar_ManDefaultRefParams( pParsRef );

    pParsRwr->fUpdateLevel = fUpdateLevel;
    pParsRef->fUpdateLevel = fUpdateLevel;
    pParsRwr->fFanout = fFanout;
    pParsRwr->fPower = fPower;

    pParsRwr->fVerbose = 0;//fVerbose;
    pParsRef->fVerbose = 0;//fVerbose;

    pAig = Aig_ManDupDfs( pAig ); 
    if ( fVerbose ) printf( "Starting:  " ), Aig_ManPrintStats( pAig );
/*
    // balance
    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Balance:   " ), Aig_ManPrintStats( pAig );
    }
*/
    // rewrite
//    Dar_ManRewrite( pAig, pParsRwr );
    pParsRwr->fUpdateLevel = 0;  // disable level update
    Dar_ManRewrite( pAig, pParsRwr );
    pParsRwr->fUpdateLevel = fUpdateLevel;  // reenable level update if needed

    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Rewrite:   " ), Aig_ManPrintStats( pAig );
    
    // refactor
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Refactor:  " ), Aig_ManPrintStats( pAig );

    // balance
//    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Balance:   " ), Aig_ManPrintStats( pAig );
    }
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Rewrite:   " ), Aig_ManPrintStats( pAig );

    pParsRwr->fUseZeros = 1;
    pParsRef->fUseZeros = 1;
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "RewriteZ:  " ), Aig_ManPrintStats( pAig );

    // balance
    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Balance:   " ), Aig_ManPrintStats( pAig );
    }
    
    // refactor
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "RefactorZ: " ), Aig_ManPrintStats( pAig );
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "RewriteZ:  " ), Aig_ManPrintStats( pAig );

    // balance
    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Balance:   " ), Aig_ManPrintStats( pAig );
    }
    return pAig;
}

/**Function*************************************************************

  Synopsis    [Reproduces script "compress2".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Dar_ManChoiceSynthesis( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fPower, int fVerbose )
//alias resyn    "b; rw; rwz; b; rwz; b"
//alias resyn2   "b; rw; rf; b; rw; rwz; b; rfz; rwz; b"
{
    Vec_Ptr_t * vAigs;

    vAigs = Vec_PtrAlloc( 3 );
    pAig = Aig_ManDupDfs(pAig);
    Vec_PtrPush( vAigs, pAig );

    pAig = Dar_ManCompress(pAig, fBalance, fUpdateLevel, fPower, fVerbose);
    Vec_PtrPush( vAigs, pAig );
//Aig_ManPrintStats( pAig );

    pAig = Dar_ManCompress2(pAig, fBalance, fUpdateLevel, 1, fPower, fVerbose);
    Vec_PtrPush( vAigs, pAig );
//Aig_ManPrintStats( pAig );

    pAig = (Aig_Man_t *)Vec_PtrEntry( vAigs, 1 );
    return vAigs;
}

/**Function*************************************************************

  Synopsis    [Reproduces script "compress2".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dar_ManChoice( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fConstruct, int nConfMax, int nLevelMax, int fVerbose )
{
    Aig_Man_t * pMan, * pTemp;
    Vec_Ptr_t * vAigs;
    int i;
    abctime clk;

clk = Abc_Clock();
//    vAigs = Dar_ManChoiceSynthesisExt();
    vAigs = Dar_ManChoiceSynthesis( pAig, fBalance, fUpdateLevel, 0, fVerbose );

    // swap the first and last network
    // this should lead to the primary choice being "better" because of synthesis
    // (it is also important when constructing choices)
    if ( !fConstruct )
    {
        pMan = (Aig_Man_t *)Vec_PtrPop( vAigs );
        Vec_PtrPush( vAigs, Vec_PtrEntry(vAigs,0) );
        Vec_PtrWriteEntry( vAigs, 0, pMan );
    }

if ( fVerbose )
{
ABC_PRT( "Synthesis time", Abc_Clock() - clk );
}
clk = Abc_Clock();
    if ( fConstruct )
        pMan = Aig_ManChoiceConstructive( vAigs, fVerbose );
    else
        pMan = Aig_ManChoicePartitioned( vAigs, 300, nConfMax, nLevelMax, fVerbose );
    Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pTemp, i )
        Aig_ManStop( pTemp );
    Vec_PtrFree( vAigs );
if ( fVerbose )
{
ABC_PRT( "Choicing time ", Abc_Clock() - clk );
}
    return pMan;
//    return NULL;
}


/**Function*************************************************************

  Synopsis    [Reproduces script "compress".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dar_NewCompress( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fPower, int fVerbose )
//alias compress2   "b -l; rw -l; rwz -l; b -l; rwz -l; b -l"
{
    Aig_Man_t * pTemp;

    Dar_RwrPar_t ParsRwr, * pParsRwr = &ParsRwr;
    Dar_RefPar_t ParsRef, * pParsRef = &ParsRef;

    Dar_ManDefaultRwrParams( pParsRwr );
    Dar_ManDefaultRefParams( pParsRef );

    pParsRwr->fUpdateLevel = fUpdateLevel;
    pParsRef->fUpdateLevel = fUpdateLevel;

    pParsRwr->fPower = fPower;

    pParsRwr->fVerbose = 0;//fVerbose;
    pParsRef->fVerbose = 0;//fVerbose;

//    pAig = Aig_ManDupDfs( pAig ); 
    if ( fVerbose ) printf( "Starting:  " ), Aig_ManPrintStats( pAig );

    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Rewrite:   " ), Aig_ManPrintStats( pAig );
    
    // refactor
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Refactor:  " ), Aig_ManPrintStats( pAig );

    // balance
    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Balance:   " ), Aig_ManPrintStats( pAig );
    }

    pParsRwr->fUseZeros = 1;
    pParsRef->fUseZeros = 1;
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "RewriteZ:  " ), Aig_ManPrintStats( pAig );

    return pAig;
}

/**Function*************************************************************

  Synopsis    [Reproduces script "compress2".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dar_NewCompress2( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fFanout, int fPower, int fLightSynth, int fVerbose )
//alias compress2   "b -l; rw -l; rf -l; b -l; rw -l; rwz -l; b -l; rfz -l; rwz -l; b -l"
{
    Aig_Man_t * pTemp;

    Dar_RwrPar_t ParsRwr, * pParsRwr = &ParsRwr;
    Dar_RefPar_t ParsRef, * pParsRef = &ParsRef;

    Dar_ManDefaultRwrParams( pParsRwr );
    Dar_ManDefaultRefParams( pParsRef );

    pParsRwr->fUpdateLevel = fUpdateLevel;
    pParsRef->fUpdateLevel = fUpdateLevel;
    pParsRwr->fFanout = fFanout;
    pParsRwr->fPower = fPower;

    pParsRwr->fVerbose = 0;//fVerbose;
    pParsRef->fVerbose = 0;//fVerbose;

//    pAig = Aig_ManDupDfs( pAig ); 
    if ( fVerbose ) printf( "Starting:  " ), Aig_ManPrintStats( pAig );

    // skip if lighter synthesis is requested
    if ( !fLightSynth )
    {
        // rewrite
        //Dar_ManRewrite( pAig, pParsRwr );
//        pParsRwr->fUpdateLevel = 0;  // disable level update  // this change was requested in July and later disabled
        Dar_ManRewrite( pAig, pParsRwr );
//        pParsRwr->fUpdateLevel = fUpdateLevel;  // reenable level update if needed

        pAig = Aig_ManDupDfs( pTemp = pAig ); 
        Aig_ManStop( pTemp );
        if ( fVerbose ) printf( "Rewrite:   " ), Aig_ManPrintStats( pAig );
    
        // refactor
        Dar_ManRefactor( pAig, pParsRef );
        pAig = Aig_ManDupDfs( pTemp = pAig ); 
        Aig_ManStop( pTemp );
        if ( fVerbose ) printf( "Refactor:  " ), Aig_ManPrintStats( pAig );
    }

    // balance
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Balance:   " ), Aig_ManPrintStats( pAig );
    
    // skip if lighter synthesis is requested
    if ( !fLightSynth )
    {
        // rewrite
        Dar_ManRewrite( pAig, pParsRwr );
        pAig = Aig_ManDupDfs( pTemp = pAig ); 
        Aig_ManStop( pTemp );
        if ( fVerbose ) printf( "Rewrite:   " ), Aig_ManPrintStats( pAig );
    }

    pParsRwr->fUseZeros = 1;
    pParsRef->fUseZeros = 1;
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "RewriteZ:  " ), Aig_ManPrintStats( pAig );

    // skip if lighter synthesis is requested
    if ( !fLightSynth )
    {
        // balance
        if ( fBalance )
        {
        pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
        Aig_ManStop( pTemp );
        if ( fVerbose ) printf( "Balance:   " ), Aig_ManPrintStats( pAig );
        }
    }
    
    // refactor
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "RefactorZ: " ), Aig_ManPrintStats( pAig );
    
    // skip if lighter synthesis is requested
    if ( !fLightSynth )
    {
        // rewrite
        Dar_ManRewrite( pAig, pParsRwr );
        pAig = Aig_ManDupDfs( pTemp = pAig ); 
        Aig_ManStop( pTemp );
        if ( fVerbose ) printf( "RewriteZ:  " ), Aig_ManPrintStats( pAig );
    }

    // balance
    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    if ( fVerbose ) printf( "Balance:   " ), Aig_ManPrintStats( pAig );
    }
    return pAig;
}

/**Function*************************************************************

  Synopsis    [Count the number of nodes with very high fanout count.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar_NewChoiceSynthesisGuard( Aig_Man_t * pAig )
{
    Aig_Obj_t * pObj;
    int i, Count = 0;
    Aig_ManForEachNode( pAig, pObj, i )
        if ( Aig_ObjRefs(pObj) > 1000 )
            Count += Aig_ObjRefs(pObj) / 1000;
    return (int)(Count > 10);
}

/**Function*************************************************************

  Synopsis    [Reproduces script "compress2".]

  Description [Takes AIG manager, consumes it, and produces GIA manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Dar_NewChoiceSynthesis( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fPower, int fLightSynth, int fVerbose )
//alias resyn    "b; rw; rwz; b; rwz; b"
//alias resyn2   "b; rw; rf; b; rw; rwz; b; rfz; rwz; b"
{
    Vec_Ptr_t * vGias;
    Gia_Man_t * pGia, * pTemp;
    int i;

    if ( fUpdateLevel && Dar_NewChoiceSynthesisGuard(pAig) )
    {
        if ( fVerbose )
            printf( "Warning: Due to high fanout count of some nodes, level updating is disabled.\n" );
        fUpdateLevel = 0;
    }

    vGias = Vec_PtrAlloc( 3 );
    pGia = Gia_ManFromAig(pAig);
    Vec_PtrPush( vGias, pGia );

    pAig = Dar_NewCompress( pAig, fBalance, fUpdateLevel, fPower, fVerbose );
    pGia = Gia_ManFromAig(pAig);
    Vec_PtrPush( vGias, pGia );
//Aig_ManPrintStats( pAig );

    pAig = Dar_NewCompress2( pAig, fBalance, fUpdateLevel, 1, fPower, fLightSynth, fVerbose );
    pGia = Gia_ManFromAig(pAig);
    Vec_PtrPush( vGias, pGia );
//Aig_ManPrintStats( pAig );

    Aig_ManStop( pAig );

    // swap around the first and the last
    pTemp = (Gia_Man_t *)Vec_PtrPop( vGias );
    Vec_PtrPush( vGias, Vec_PtrEntry(vGias,0) );
    Vec_PtrWriteEntry( vGias, 0, pTemp );

//    Aig_Man_t * pAig;
//    int i;
//    printf( "Choicing will be performed with %d AIGs:\n", Vec_PtrSize(p->vAigs) );
//    Vec_PtrForEachEntry( Aig_Man_t *, p->vAigs, pAig, i )
//        Aig_ManPrintStats( pAig );

    // derive the miter
    pGia = Gia_ManChoiceMiter( vGias );

    // cleanup
    Vec_PtrForEachEntry( Gia_Man_t *, vGias, pTemp, i )
        Gia_ManStop( pTemp );
    Vec_PtrFree( vGias );
    return pGia;
}

/**Function*************************************************************

  Synopsis    [Reproduces script "compress2".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
Aig_Man_t * Dar_ManChoiceNew( Aig_Man_t * pAig, Dch_Pars_t * pPars )
{
    extern Aig_Man_t * Dch_ComputeChoices( Vec_Ptr_t * vAigs, Dch_Pars_t * pPars );
    extern Aig_Man_t * Cec_ComputeChoices( Vec_Ptr_t * vAigs, Dch_Pars_t * pPars );

    int fVerbose = pPars->fVerbose;
    int fConstruct = 0;
    Aig_Man_t * pMan, * pTemp;
    Vec_Ptr_t * vAigs;
    int i;
    abctime clk;

clk = Abc_Clock();
//    vAigs = Dar_ManChoiceSynthesisExt();
//    vAigs = Dar_ManChoiceSynthesis( pAig, 1, 1, pPars->fPower, fVerbose );
    vAigs = Dar_ManChoiceSynthesis( pAig, 1, 1, pPars->fPower, 0 );

    // swap the first and last network
    // this should lead to the primary choice being "better" because of synthesis
    // (it is also important when constructing choices)
    if ( !fConstruct )
    {
        pMan = Vec_PtrPop( vAigs );
        Vec_PtrPush( vAigs, Vec_PtrEntry(vAigs,0) );
        Vec_PtrWriteEntry( vAigs, 0, pMan );
    }

if ( fVerbose )
{
//ABC_PRT( "Synthesis time", Abc_Clock() - clk );
}
    pPars->timeSynth = Abc_Clock() - clk;

clk = Abc_Clock();
    // perform choice computation
    if ( pPars->fUseGia )
        pMan = Cec_ComputeChoices( vAigs, pPars );
    else
        pMan = Dch_ComputeChoices( vAigs, pPars );

    // reconstruct the network
    pMan = Aig_ManDupDfsGuided( pTemp = pMan, Vec_PtrEntry(vAigs,0) );
    Aig_ManStop( pTemp );
    // duplicate the timing manager
    pTemp = Vec_PtrEntry( vAigs, 0 );
    if ( pTemp->pManTime )
    {
        extern void * Tim_ManDup( void * p, int fDiscrete );     
        pMan->pManTime = Tim_ManDup( pTemp->pManTime, 0 );
    }
    // reset levels
    Aig_ManChoiceLevel( pMan );
    ABC_FREE( pMan->pName );
    ABC_FREE( pMan->pSpec );
    pMan->pName = Abc_UtilStrsav( pTemp->pName );
    pMan->pSpec = Abc_UtilStrsav( pTemp->pSpec );

    // cleanup
    Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pTemp, i )
        Aig_ManStop( pTemp );
    Vec_PtrFree( vAigs );

if ( fVerbose )
{
//ABC_PRT( "Choicing time ", Abc_Clock() - clk );
}
    return pMan;
//    return NULL;
}
*/

/**Function*************************************************************

  Synopsis    [Reproduces script "compress2".]

  Description [Consumes the input AIG to reduce memory usage.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dar_ManChoiceNewAig( Aig_Man_t * pAig, Dch_Pars_t * pPars )
{
//    extern Aig_Man_t * Dch_DeriveTotalAig( Vec_Ptr_t * vAigs );
    extern Aig_Man_t * Dch_ComputeChoices( Aig_Man_t * pAig, Dch_Pars_t * pPars );
    int fVerbose = pPars->fVerbose;
    Aig_Man_t * pMan, * pTemp;
    Vec_Ptr_t * vAigs;
    Vec_Ptr_t * vPios;
    void * pManTime;
    char * pName, * pSpec;
    int i;
    abctime clk;

clk = Abc_Clock();
    vAigs = Dar_ManChoiceSynthesis( pAig, 1, 1, pPars->fPower, fVerbose );
pPars->timeSynth = Abc_Clock() - clk;
    // swap the first and last network
    // this should lead to the primary choice being "better" because of synthesis
    // (it is also important when constructing choices)
    pMan = (Aig_Man_t *)Vec_PtrPop( vAigs );
    Vec_PtrPush( vAigs, Vec_PtrEntry(vAigs,0) );
    Vec_PtrWriteEntry( vAigs, 0, pMan );

    // derive the total AIG
    pMan = Dch_DeriveTotalAig( vAigs );
    // cleanup
    Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pTemp, i )
        Aig_ManStop( pTemp );
    Vec_PtrFree( vAigs );

    // compute choices
    pMan = Dch_ComputeChoices( pTemp = pMan, pPars );
    Aig_ManStop( pTemp );

    // save useful things
    pManTime = pAig->pManTime; pAig->pManTime = NULL;
    pName = Abc_UtilStrsav( pAig->pName );
    pSpec = Abc_UtilStrsav( pAig->pSpec );

    // create guidence
    vPios = Aig_ManOrderPios( pMan, pAig ); 
    Aig_ManStop( pAig );

    // reconstruct the network
    pMan = Aig_ManDupDfsGuided( pTemp = pMan, vPios );
    Aig_ManStop( pTemp );
    Vec_PtrFree( vPios );

    // reset levels
    pMan->pManTime = pManTime;
    Aig_ManChoiceLevel( pMan );

    // copy names
    ABC_FREE( pMan->pName );
    ABC_FREE( pMan->pSpec );
    pMan->pName = pName;
    pMan->pSpec = pSpec;
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Reproduces script "compress2".]

  Description [Consumes the input AIG to reduce memory usage.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dar_ManChoiceNew( Aig_Man_t * pAig, Dch_Pars_t * pPars )
{
    extern Aig_Man_t * Cec_ComputeChoices( Gia_Man_t * pGia, Dch_Pars_t * pPars );
//    extern Aig_Man_t * Dch_DeriveTotalAig( Vec_Ptr_t * vAigs );
    extern Aig_Man_t * Dch_ComputeChoices( Aig_Man_t * pAig, Dch_Pars_t * pPars );
//    int fVerbose = pPars->fVerbose;
    Aig_Man_t * pMan, * pTemp;
    Gia_Man_t * pGia;
    Vec_Ptr_t * vPios;
    void * pManTime;
    char * pName, * pSpec;
    abctime clk;

    // save useful things
    pManTime = pAig->pManTime; pAig->pManTime = NULL;
    pName = Abc_UtilStrsav( pAig->pName );
    pSpec = Abc_UtilStrsav( pAig->pSpec );

    // perform synthesis
clk = Abc_Clock();
    pGia = Dar_NewChoiceSynthesis( Aig_ManDupDfs(pAig), 1, 1, pPars->fPower, pPars->fLightSynth, pPars->fVerbose );
pPars->timeSynth = Abc_Clock() - clk;

    // perform choice computation
    if ( pPars->fUseGia )
        pMan = Cec_ComputeChoices( pGia, pPars );
    else
    {
        pMan = Gia_ManToAigSkip( pGia, 3 );
        Gia_ManStop( pGia );
        pMan = Dch_ComputeChoices( pTemp = pMan, pPars );
        Aig_ManStop( pTemp );
    }

    // create guidence
    vPios = Aig_ManOrderPios( pMan, pAig ); 
    Aig_ManStop( pAig );

    // reconstruct the network
    pMan = Aig_ManDupDfsGuided( pTemp = pMan, vPios );
    Aig_ManStop( pTemp );
    Vec_PtrFree( vPios );

    // reset levels
    pMan->pManTime = pManTime;
    Aig_ManChoiceLevel( pMan );

    // copy names
    ABC_FREE( pMan->pName );
    ABC_FREE( pMan->pSpec );
    pMan->pName = pName;
    pMan->pSpec = pSpec;
    return pMan;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

