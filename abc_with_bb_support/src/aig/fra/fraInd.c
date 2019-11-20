/**CFile****************************************************************

  FileName    [fraInd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    [Inductive prover.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraInd.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"
#include "cnf.h"
#include "dar.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs AIG rewriting on the constaint manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_FraigInductionRewrite( Fra_Man_t * p )
{
    Aig_Man_t * pTemp;
    Aig_Obj_t * pObj, * pObjPo;
    int nTruePis, k, i, clk = clock();
    // perform AIG rewriting on the speculated frames
    pTemp = Aig_ManDup( p->pManFraig, 0 );
//    pTemp = Dar_ManRwsat( pTemp, 1, 0 );
    pTemp = Dar_ManRewriteDefault( pTemp );

//    printf( "Before = %6d.  After = %6d.\n", Aig_ManNodeNum(p->pManFraig), Aig_ManNodeNum(pTemp) ); 
//Aig_ManDumpBlif( p->pManFraig, "1.blif" );
//Aig_ManDumpBlif( pTemp, "2.blif" );

//    Fra_FramesWriteCone( pTemp );
//    Aig_ManStop( pTemp );
    // transfer PI/register pointers
    assert( p->pManFraig->nRegs == pTemp->nRegs );
    assert( p->pManFraig->nAsserts == pTemp->nAsserts );
    nTruePis = Aig_ManPiNum(p->pManAig) - Aig_ManRegNum(p->pManAig);
    memset( p->pMemFraig, 0, sizeof(Aig_Obj_t *) * p->nSizeAlloc * p->nFramesAll );
    Fra_ObjSetFraig( Aig_ManConst1(p->pManAig), p->pPars->nFramesK, Aig_ManConst1(pTemp) );
    Aig_ManForEachPiSeq( p->pManAig, pObj, i )
        Fra_ObjSetFraig( pObj, p->pPars->nFramesK, Aig_ManPi(pTemp,nTruePis*p->pPars->nFramesK+i) );
    k = 0;
    assert( Aig_ManRegNum(p->pManAig) == Aig_ManPoNum(pTemp) - pTemp->nAsserts );
    Aig_ManForEachLoSeq( p->pManAig, pObj, i )
    {
        pObjPo = Aig_ManPo(pTemp, pTemp->nAsserts + k++);
        Fra_ObjSetFraig( pObj, p->pPars->nFramesK, Aig_ObjChild0(pObjPo) );
    }
    // exchange
    Aig_ManStop( p->pManFraig );
    p->pManFraig = pTemp;
p->timeRwr += clock() - clk;
}

/**Function*************************************************************

  Synopsis    [Performs speculative reduction for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fra_FramesConstrainNode( Aig_Man_t * pManFraig, Aig_Obj_t * pObj, int iFrame )
{
    Aig_Obj_t * pObjNew, * pObjNew2, * pObjRepr, * pObjReprNew, * pMiter;
    // skip nodes without representative
    if ( (pObjRepr = Fra_ClassObjRepr(pObj)) == NULL )
        return;
    assert( pObjRepr->Id < pObj->Id );
    // get the new node 
    pObjNew = Fra_ObjFraig( pObj, iFrame );
    // get the new node of the representative
    pObjReprNew = Fra_ObjFraig( pObjRepr, iFrame );
    // if this is the same node, no need to add constraints
    if ( Aig_Regular(pObjNew) == Aig_Regular(pObjReprNew) )
        return;
    // these are different nodes - perform speculative reduction
    pObjNew2 = Aig_NotCond( pObjReprNew, pObj->fPhase ^ pObjRepr->fPhase );
    // set the new node
    Fra_ObjSetFraig( pObj, iFrame, pObjNew2 );
    // add the constraint
    pMiter = Aig_Exor( pManFraig, Aig_Regular(pObjNew), Aig_Regular(pObjReprNew) );
    pMiter = Aig_NotCond( pMiter, Aig_Regular(pMiter)->fPhase ^ Aig_IsComplement(pMiter) );
    pMiter = Aig_Not( pMiter );
    Aig_ObjCreatePo( pManFraig, pMiter );
}

/**Function*************************************************************

  Synopsis    [Prepares the inductive case with speculative reduction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_FramesWithClasses( Fra_Man_t * p )
{
    Aig_Man_t * pManFraig;
    Aig_Obj_t * pObj, * pObjNew;
    Aig_Obj_t ** pLatches;
    int i, k, f;
    assert( p->pManFraig == NULL );
    assert( Aig_ManRegNum(p->pManAig) > 0 );
    assert( Aig_ManRegNum(p->pManAig) < Aig_ManPiNum(p->pManAig) );

    // start the fraig package
    pManFraig = Aig_ManStart( (Aig_ManObjIdMax(p->pManAig) + 1) * p->nFramesAll );
    pManFraig->nRegs = p->pManAig->nRegs;
    // create PI nodes for the frames
    for ( f = 0; f < p->nFramesAll; f++ )
        Fra_ObjSetFraig( Aig_ManConst1(p->pManAig), f, Aig_ManConst1(pManFraig) );
    for ( f = 0; f < p->nFramesAll; f++ )
        Aig_ManForEachPiSeq( p->pManAig, pObj, i )
            Fra_ObjSetFraig( pObj, f, Aig_ObjCreatePi(pManFraig) );
    // create latches for the first frame
    Aig_ManForEachLoSeq( p->pManAig, pObj, i )
        Fra_ObjSetFraig( pObj, 0, Aig_ObjCreatePi(pManFraig) );

    // add timeframes
    pLatches = ALLOC( Aig_Obj_t *, Aig_ManRegNum(p->pManAig) );
    for ( f = 0; f < p->nFramesAll - 1; f++ )
    {
        // set the constraints on the latch outputs
        Aig_ManForEachLoSeq( p->pManAig, pObj, i )
            Fra_FramesConstrainNode( pManFraig, pObj, f );
        // add internal nodes of this frame
        Aig_ManForEachNode( p->pManAig, pObj, i )
        {
            pObjNew = Aig_And( pManFraig, Fra_ObjChild0Fra(pObj,f), Fra_ObjChild1Fra(pObj,f) );
            Fra_ObjSetFraig( pObj, f, pObjNew );
            Fra_FramesConstrainNode( pManFraig, pObj, f );
        }
        // save the latch input values
        k = 0;
        Aig_ManForEachLiSeq( p->pManAig, pObj, i )
            pLatches[k++] = Fra_ObjChild0Fra(pObj,f);
        assert( k == Aig_ManRegNum(p->pManAig) );
        // insert them to the latch output values
        k = 0;
        Aig_ManForEachLoSeq( p->pManAig, pObj, i )
            Fra_ObjSetFraig( pObj, f+1, pLatches[k++] );
        assert( k == Aig_ManRegNum(p->pManAig) );
    }
    free( pLatches );
    // mark the asserts
    pManFraig->nAsserts = Aig_ManPoNum(pManFraig);
    // add the POs for the latch inputs
    Aig_ManForEachLiSeq( p->pManAig, pObj, i )
        Aig_ObjCreatePo( pManFraig, Fra_ObjChild0Fra(pObj,f-1) );

    // remove dangling nodes
    Aig_ManCleanup( pManFraig );
    // make sure the satisfying assignment is node assigned
    assert( pManFraig->pData == NULL );
    return pManFraig;
}

/**Function*************************************************************

  Synopsis    [Performs choicing of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_FraigInduction( Aig_Man_t * pManAig, int nFramesK, int fRewrite, int fVerbose, int * pnIter )
{
    Fra_Man_t * p;
    Fra_Par_t Pars, * pPars = &Pars; 
    Aig_Obj_t * pObj;
    Cnf_Dat_t * pCnf;
    Aig_Man_t * pManAigNew;
    int nIter, i, clk = clock(), clk2;

    if ( Aig_ManNodeNum(pManAig) == 0 )
    {
        if ( pnIter ) *pnIter = 0;
        return Aig_ManDup(pManAig, 1);
    }
    assert( Aig_ManLatchNum(pManAig) == 0 );
    assert( Aig_ManRegNum(pManAig) > 0 );
    assert( nFramesK > 0 );
//Aig_ManShow( pManAig, 0, NULL );

    // get parameters
    Fra_ParamsDefaultSeq( pPars );
    pPars->nFramesK = nFramesK;
    pPars->fVerbose = fVerbose;
    pPars->fRewrite = fRewrite;

    // start the fraig manager for this run
    p = Fra_ManStart( pManAig, pPars );
    // derive and refine e-classes using K initialized frames
    Fra_Simulate( p, 1 );
    // refine e-classes using sequential simulation?
 
    p->nLitsZero = Vec_PtrSize( p->pCla->vClasses1 );
    p->nLitsBeg  = Fra_ClassesCountLits( p->pCla );
    p->nNodesBeg = Aig_ManNodeNum(pManAig);
    p->nRegsBeg  = Aig_ManRegNum(pManAig);

    // iterate the inductive case
    p->pCla->fRefinement = 1;
    for ( nIter = 0; p->pCla->fRefinement; nIter++ )
    {
        // mark the classes as non-refined
        p->pCla->fRefinement = 0;
        // derive non-init K-timeframes while implementing e-classes
        p->pManFraig = Fra_FramesWithClasses( p );
        // perform AIG rewriting
        if ( p->pPars->fRewrite )
            Fra_FraigInductionRewrite( p );
        // report the intermediate results
        if ( fVerbose )
        {
            printf( "%3d : Const = %6d. Class = %6d.  L = %6d. LR = %6d.  N = %6d. NR = %6d.\n", 
                nIter, Vec_PtrSize(p->pCla->vClasses1), Vec_PtrSize(p->pCla->vClasses), 
                Fra_ClassesCountLits(p->pCla), p->pManFraig->nAsserts,
                Aig_ManNodeNum(p->pManAig), Aig_ManNodeNum(p->pManFraig) );
        }

        // convert the manager to SAT solver (the last nLatches outputs are inputs)
        pCnf = Cnf_Derive( p->pManFraig, Aig_ManRegNum(p->pManFraig) );
//        pCnf = Cnf_DeriveSimple( p->pManFraig, Aig_ManRegNum(p->pManFraig) );
//Cnf_DataWriteIntoFile( pCnf, "temp.cnf", 1 );

        p->pSat = Cnf_DataWriteIntoSolver( pCnf );
        p->nSatVars = pCnf->nVars;
        assert( p->pSat != NULL );
        if ( p->pSat == NULL )
            printf( "Fra_FraigInduction(): Computed CNF is not valid.\n" );

        // set the pointers to the manager
        Aig_ManForEachObj( p->pManFraig, pObj, i )
            pObj->pData = p;

        // transfer PI/LO variable numbers
        pObj = Aig_ManConst1( p->pManFraig );
        Fra_ObjSetSatNum( pObj, pCnf->pVarNums[pObj->Id] );
        Aig_ManForEachPi( p->pManFraig, pObj, i )
            Fra_ObjSetSatNum( pObj, pCnf->pVarNums[pObj->Id] );
        // transfer LI variable numbers
        Aig_ManForEachLiSeq( p->pManFraig, pObj, i )
        {
            Fra_ObjSetSatNum( pObj, pCnf->pVarNums[pObj->Id] );
            Fra_ObjSetFaninVec( pObj, (void *)1 );
        }
        Cnf_DataFree( pCnf );
/*
        Aig_ManForEachObj( p->pManFraig, pObj, i )
        {
            Fra_ObjSetSatNum( pObj, pCnf->pVarNums[pObj->Id] );
            Fra_ObjSetFaninVec( pObj, (void *)1 );
        }
        Cnf_DataFree( pCnf );
*/

        // perform sweeping
        Fra_FraigSweep( p );
        assert( p->vTimeouts == NULL );
        if ( p->vTimeouts )
           printf( "Fra_FraigInduction(): SAT solver timed out!\n" );

        // cleanup
        Fra_ManClean( p );
    }
    // move the classes into representatives and reduce AIG
clk2 = clock();
    Fra_ClassesCopyReprs( p->pCla, p->vTimeouts );
    pManAigNew = Aig_ManDupRepr( pManAig );
    Aig_ManSeqCleanup( pManAigNew );
p->timeTrav += clock() - clk2;
p->timeTotal = clock() - clk;
    // get the final stats
    p->nLitsEnd  = Fra_ClassesCountLits( p->pCla );
    p->nNodesEnd = Aig_ManNodeNum(pManAigNew);
    p->nRegsEnd  = Aig_ManRegNum(pManAigNew);
    // free the manager
    Fra_ManStop( p );
    // check the output
//    if ( Aig_ManPoNum(pManAigNew) - Aig_ManRegNum(pManAigNew) == 1 )
//        if ( Aig_ObjChild0( Aig_ManPo(pManAigNew,0) ) == Aig_ManConst0(pManAigNew) )
//            printf( "Proved output constant 0.\n" );
    if ( pnIter ) *pnIter = nIter;
    return pManAigNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


