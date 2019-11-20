/**CFile****************************************************************

  FileName    [fraCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraCore.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reports the status of the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_FraigMiterStatus( Aig_Man_t * p )
{
    Aig_Obj_t * pObj, * pObjNew;
    int i, CountConst0 = 0, CountNonConst0 = 0, CountUndecided = 0;
    if ( p->pData )
        return 0;
    Aig_ManForEachPoSeq( p, pObj, i )
    {
        pObjNew = Aig_ObjChild0(pObj);
        // check if the output is constant 0
        if ( pObjNew == Aig_ManConst0(p) )
        {
            CountConst0++;
            continue;
        }
        // check if the output is constant 1
        if ( pObjNew == Aig_ManConst1(p) )
        {
            CountNonConst0++;
            continue;
        }
        // check if the output can be constant 0
        if ( Aig_Regular(pObjNew)->fPhase != (unsigned)Aig_IsComplement(pObjNew) )
        {
            CountNonConst0++;
            continue;
        }
        CountUndecided++;
    }
/*
    if ( p->pParams->fVerbose )
    {
        printf( "Miter has %d outputs. ", Aig_ManPoNum(p->pManAig) );
        printf( "Const0 = %d.  ", CountConst0 );
        printf( "NonConst0 = %d.  ", CountNonConst0 );
        printf( "Undecided = %d.  ", CountUndecided );
        printf( "\n" );
    }
*/
    if ( CountNonConst0 )
        return 0;
    if ( CountUndecided )
        return -1;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Write speculative miter for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fra_FraigNodeSpeculate( Fra_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pObjFraig, Aig_Obj_t * pObjReprFraig )
{ 
    static int Counter = 0;
    char FileName[20];
    Aig_Man_t * pTemp;
    Aig_Obj_t * pNode;
    int i;
    // create manager with the logic for these two nodes
    pTemp = Aig_ManExtractMiter( p->pManFraig, pObjFraig, pObjReprFraig );
    // dump the logic into a file
    sprintf( FileName, "aig\\%03d.blif", ++Counter );
    Aig_ManDumpBlif( pTemp, FileName );
    printf( "Speculation cone with %d nodes was written into file \"%s\".\n", Aig_ManNodeNum(pTemp), FileName );
    // clean up
    Aig_ManStop( pTemp );
    Aig_ManForEachObj( p->pManFraig, pNode, i )
        pNode->pData = p;
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for one node.]

  Description [Returns the fraiged node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fra_FraigNode( Fra_Man_t * p, Aig_Obj_t * pObj )
{ 
    Aig_Obj_t * pObjRepr, * pObjFraig, * pObjFraig2, * pObjReprFraig;
    int RetValue;
    assert( !Aig_IsComplement(pObj) );
    // get representative of this class
    pObjRepr = Fra_ClassObjRepr( pObj );
    if ( pObjRepr == NULL || // this is a unique node
       (!p->pPars->fDoSparse && pObjRepr == Aig_ManConst1(p->pManAig)) ) // this is a sparse node
        return;
    // get the fraiged node
    pObjFraig = Fra_ObjFraig( pObj, p->pPars->nFramesK );
    // get the fraiged representative
    pObjReprFraig = Fra_ObjFraig( pObjRepr, p->pPars->nFramesK );
    // if the fraiged nodes are the same, return
    if ( Aig_Regular(pObjFraig) == Aig_Regular(pObjReprFraig) )
        return;
    assert( p->pPars->nFramesK || Aig_Regular(pObjFraig) != Aig_ManConst1(p->pManFraig) );
    // if they are proved different, the c-ex will be in p->pPatWords
    RetValue = Fra_NodesAreEquiv( p, Aig_Regular(pObjReprFraig), Aig_Regular(pObjFraig) );
    if ( RetValue == 1 )  // proved equivalent
    {
//        if ( p->pPars->fChoicing )
//            Aig_ObjCreateRepr( p->pManFraig, Aig_Regular(pObjReprFraig), Aig_Regular(pObjFraig) );
        // the nodes proved equal
        pObjFraig2 = Aig_NotCond( pObjReprFraig, pObj->fPhase ^ pObjRepr->fPhase );
        Fra_ObjSetFraig( pObj, p->pPars->nFramesK, pObjFraig2 );
        return;
    }
    if ( RetValue == -1 ) // failed
    {
        if ( p->vTimeouts == NULL )
            p->vTimeouts = Vec_PtrAlloc( 100 );
        Vec_PtrPush( p->vTimeouts, pObj );
        if ( !p->pPars->fSpeculate )
            return;
        assert( 0 );
        // speculate
        p->nSpeculs++;
        pObjFraig2 = Aig_NotCond( pObjReprFraig, pObj->fPhase ^ pObjRepr->fPhase );
        Fra_ObjSetFraig( pObj, p->pPars->nFramesK, pObjFraig2 );
        Fra_FraigNodeSpeculate( p, pObj, Aig_Regular(pObjFraig), Aig_Regular(pObjReprFraig) );
        return;
    }
//printf( "Disproved %d and %d.\n", pObj->Id, pObjRepr->Id );
    // disprove the nodes
    // if we do not include the node into those disproved, we may end up 
    // merging this node with another representative, for which proof has timed out
    if ( p->vTimeouts )
        Vec_PtrPush( p->vTimeouts, pObj );
    // simulate the counter-example and return the Fraig node
    Fra_Resimulate( p );
    assert( Fra_ClassObjRepr(pObj) != pObjRepr );
    if ( Fra_ClassObjRepr(pObj) == pObjRepr )
        printf( "Fra_FraigNode(): Error in class refinement!\n" );
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_FraigSweep( Fra_Man_t * p )
{
    ProgressBar * pProgress;
    Aig_Obj_t * pObj, * pObjNew;
    int i, k = 0;
    // duplicate internal nodes
    pProgress = Extra_ProgressBarStart( stdout, Aig_ManObjIdMax(p->pManAig) );
    // fraig latch outputs
    Aig_ManForEachLoSeq( p->pManAig, pObj, i )
        Fra_FraigNode( p, pObj );
    // fraig internal nodes
    Aig_ManForEachNode( p->pManAig, pObj, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // derive and remember the new fraig node
        pObjNew = Aig_And( p->pManFraig, Fra_ObjChild0Fra(pObj,p->pPars->nFramesK), Fra_ObjChild1Fra(pObj,p->pPars->nFramesK) );
        Fra_ObjSetFraig( pObj, p->pPars->nFramesK, pObjNew );
        Aig_Regular(pObjNew)->pData = p;
        // quit if simulation detected a counter-example for a PO
        if ( p->pManFraig->pData )
            continue;
        // perform fraiging
        Fra_FraigNode( p, pObj );
    }
    Extra_ProgressBarStop( pProgress );
    // try to prove the outputs of the miter
    p->nNodesMiter = Aig_ManNodeNum(p->pManFraig);
//    Fra_MiterStatus( p->pManFraig );
//    if ( p->pPars->fProve && p->pManFraig->pData == NULL )
//        Fra_MiterProve( p );
}

/**Function*************************************************************

  Synopsis    [Performs fraiging of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_FraigPerform( Aig_Man_t * pManAig, Fra_Par_t * pPars )
{
    Fra_Man_t * p;
    Aig_Man_t * pManAigNew;
    int clk;
    if ( Aig_ManNodeNum(pManAig) == 0 )
        return Aig_ManDup(pManAig, 1);
clk = clock();
    assert( Aig_ManLatchNum(pManAig) == 0 );
    p = Fra_ManStart( pManAig, pPars );
    p->pManFraig = Fra_ManPrepareComb( p );
    Fra_Simulate( p, 0 );
    if ( p->pPars->fChoicing )
        Aig_ManReprStart( p->pManFraig, Aig_ManObjIdMax(p->pManAig)+1 );
    // collect initial states
    p->nLitsZero = Vec_PtrSize( p->pCla->vClasses1 );
    p->nLitsBeg  = Fra_ClassesCountLits( p->pCla );
    p->nNodesBeg = Aig_ManNodeNum(pManAig);
    p->nRegsBeg  = Aig_ManRegNum(pManAig);
    // perform fraig sweep
    Fra_FraigSweep( p );
    Fra_ManFinalizeComb( p );
    if ( p->pPars->fChoicing )
    {
        int clk2 = clock();
        Fra_ClassesCopyReprs( p->pCla, p->vTimeouts );
        pManAigNew = Aig_ManDupRepr( p->pManAig );
        Aig_ManCreateChoices( pManAigNew );
        Aig_ManStop( p->pManFraig );
        p->pManFraig = NULL;
p->timeTrav += clock() - clk2;
    }
    else
    {
        Aig_ManCleanup( p->pManFraig );
        pManAigNew = p->pManFraig;
        p->pManFraig = NULL;
/*
        Fra_ClassesCopyReprs( p->pCla, p->vTimeouts );
        pManAigNew = Aig_ManDupRepr( p->pManAig );
//        Aig_ManCreateChoices( pManAigNew );
        Aig_ManCleanup( pManAigNew );
        Aig_ManStop( p->pManFraig );
        p->pManFraig = NULL;
*/
    }
p->timeTotal = clock() - clk;
    // collect final stats
    p->nLitsEnd  = Fra_ClassesCountLits( p->pCla );
    p->nNodesEnd = Aig_ManNodeNum(pManAigNew);
    p->nRegsEnd  = Aig_ManRegNum(pManAigNew);
    Fra_ManStop( p );
    return pManAigNew;
}

/**Function*************************************************************

  Synopsis    [Performs choicing of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_FraigChoice( Aig_Man_t * pManAig )
{
    Fra_Par_t Pars, * pPars = &Pars; 
    Fra_ParamsDefault( pPars );
    pPars->nBTLimitNode = 1000;
    pPars->fVerbose     = 0;
    pPars->fProve       = 0;
    pPars->fDoSparse    = 1;
    pPars->fSpeculate   = 0;
    pPars->fChoicing    = 1;
    return Fra_FraigPerform( pManAig, pPars );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


