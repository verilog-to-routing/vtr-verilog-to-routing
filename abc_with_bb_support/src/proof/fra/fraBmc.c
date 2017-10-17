/**CFile****************************************************************

  FileName    [fraBmc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    [Bounded model checking.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraBmc.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// simulation manager
struct Fra_Bmc_t_
{
    // parameters
    int              nPref;             // the size of the prefix
    int              nDepth;            // the depth of the frames
    int              nFramesAll;        // the total number of timeframes
    // implications to be filtered
    Vec_Int_t *      vImps;
    // AIG managers
    Aig_Man_t *      pAig;              // the original AIG manager
    Aig_Man_t *      pAigFrames;        // initialized timeframes
    Aig_Man_t *      pAigFraig;         // the fraiged initialized timeframes
    // mapping of nodes
    Aig_Obj_t **     pObjToFrames;      // mapping of the original node into frames
    Aig_Obj_t **     pObjToFraig;       // mapping of the frames node into fraig
};

static inline Aig_Obj_t *  Bmc_ObjFrames( Aig_Obj_t * pObj, int i )                       { return ((Fra_Man_t *)pObj->pData)->pBmc->pObjToFrames[((Fra_Man_t *)pObj->pData)->pBmc->nFramesAll*pObj->Id + i];  }
static inline void         Bmc_ObjSetFrames( Aig_Obj_t * pObj, int i, Aig_Obj_t * pNode ) { ((Fra_Man_t *)pObj->pData)->pBmc->pObjToFrames[((Fra_Man_t *)pObj->pData)->pBmc->nFramesAll*pObj->Id + i] = pNode; }

static inline Aig_Obj_t *  Bmc_ObjFraig( Aig_Obj_t * pObj )                               { return ((Fra_Man_t *)pObj->pData)->pBmc->pObjToFraig[pObj->Id];  }
static inline void         Bmc_ObjSetFraig( Aig_Obj_t * pObj, Aig_Obj_t * pNode )         { ((Fra_Man_t *)pObj->pData)->pBmc->pObjToFraig[pObj->Id] = pNode; }

static inline Aig_Obj_t *  Bmc_ObjChild0Frames( Aig_Obj_t * pObj, int i ) { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin0(pObj)? Aig_NotCond(Bmc_ObjFrames(Aig_ObjFanin0(pObj),i), Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t *  Bmc_ObjChild1Frames( Aig_Obj_t * pObj, int i ) { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin1(pObj)? Aig_NotCond(Bmc_ObjFrames(Aig_ObjFanin1(pObj),i), Aig_ObjFaninC1(pObj)) : NULL;  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns 1 if the nodes are equivalent.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_BmcNodesAreEqual( Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 )
{
    Fra_Man_t * p = (Fra_Man_t *)pObj0->pData;
    Aig_Obj_t * pObjFrames0, * pObjFrames1;
    Aig_Obj_t * pObjFraig0, * pObjFraig1;
    int i;
    for ( i = p->pBmc->nPref; i < p->pBmc->nFramesAll; i++ )
    {
        pObjFrames0 = Aig_Regular( Bmc_ObjFrames(pObj0, i) );
        pObjFrames1 = Aig_Regular( Bmc_ObjFrames(pObj1, i) );
        if ( pObjFrames0 == pObjFrames1 )
            continue;
        pObjFraig0 = Aig_Regular( Bmc_ObjFraig(pObjFrames0) );
        pObjFraig1 = Aig_Regular( Bmc_ObjFraig(pObjFrames1) );
        if ( pObjFraig0 != pObjFraig1 )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is costant.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_BmcNodeIsConst( Aig_Obj_t * pObj )
{
    Fra_Man_t * p = (Fra_Man_t *)pObj->pData;
    return Fra_BmcNodesAreEqual( pObj, Aig_ManConst1(p->pManAig) );
}

/**Function*************************************************************

  Synopsis    [Refines implications using BMC.]

  Description [The input is the combinational FRAIG manager,
  which is used to FRAIG the timeframes. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_BmcFilterImplications( Fra_Man_t * p, Fra_Bmc_t * pBmc )
{
    Aig_Obj_t * pLeft, * pRight;
    Aig_Obj_t * pLeftT, * pRightT;
    Aig_Obj_t * pLeftF, * pRightF;
    int i, f, Imp, Left, Right;
    int fComplL, fComplR;
    assert( p->nFramesAll == 1 );
    assert( p->pManAig == pBmc->pAigFrames );
    Vec_IntForEachEntry( pBmc->vImps, Imp, i )
    {
        if ( Imp == 0 )
            continue;
        Left  = Fra_ImpLeft(Imp);
        Right = Fra_ImpRight(Imp);
        // get the corresponding nodes
        pLeft  = Aig_ManObj( pBmc->pAig, Left );
        pRight = Aig_ManObj( pBmc->pAig, Right );
        // iterate through the timeframes
        for ( f = pBmc->nPref; f < pBmc->nFramesAll; f++ )
        {
            // get timeframe nodes
            pLeftT  = Bmc_ObjFrames( pLeft, f );
            pRightT = Bmc_ObjFrames( pRight, f );
            // get the corresponding FRAIG nodes
            pLeftF  = Fra_ObjFraig( Aig_Regular(pLeftT), 0 );
            pRightF = Fra_ObjFraig( Aig_Regular(pRightT), 0 );
            // get the complemented attributes
            fComplL = pLeft->fPhase ^ Aig_IsComplement(pLeftF) ^ Aig_IsComplement(pLeftT);
            fComplR = pRight->fPhase ^ Aig_IsComplement(pRightF) ^ Aig_IsComplement(pRightT);
            // check equality
            if ( Aig_Regular(pLeftF) == Aig_Regular(pRightF) )
            {
                if ( fComplL == fComplR ) // x => x  - always true
                    continue;
                assert( fComplL != fComplR );
                // consider 4 possibilities:
                // NOT(1) => 1    or   0 => 1  - always true
                // 1 => NOT(1)    or   1 => 0  - never true
                // NOT(x) => x    or   x       - not always true
                // x => NOT(x)    or   NOT(x)  - not always true
                if ( Aig_ObjIsConst1(Aig_Regular(pLeftF)) && fComplL ) // proved implication
                    continue;
                // disproved implication
                Vec_IntWriteEntry( pBmc->vImps, i, 0 ); 
                break;
            }
            // check the implication 
            if ( Fra_NodesAreImp( p, Aig_Regular(pLeftF), Aig_Regular(pRightF), fComplL, fComplR ) != 1 )
            {
                Vec_IntWriteEntry( pBmc->vImps, i, 0 );
                break;
            }
        }
    }
    Fra_ImpCompactArray( pBmc->vImps );
}


/**Function*************************************************************

  Synopsis    [Starts the BMC manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fra_Bmc_t * Fra_BmcStart( Aig_Man_t * pAig, int nPref, int nDepth )
{
    Fra_Bmc_t * p;
    p = ABC_ALLOC( Fra_Bmc_t, 1 );
    memset( p, 0, sizeof(Fra_Bmc_t) );
    p->pAig = pAig;
    p->nPref = nPref;
    p->nDepth = nDepth;
    p->nFramesAll = nPref + nDepth;
    p->pObjToFrames  = ABC_ALLOC( Aig_Obj_t *, p->nFramesAll * Aig_ManObjNumMax(pAig) );
    memset( p->pObjToFrames, 0, sizeof(Aig_Obj_t *) * p->nFramesAll * Aig_ManObjNumMax(pAig) );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the BMC manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_BmcStop( Fra_Bmc_t * p )
{
    Aig_ManStop( p->pAigFrames );
    if ( p->pAigFraig )
        Aig_ManStop( p->pAigFraig );
    ABC_FREE( p->pObjToFrames );
    ABC_FREE( p->pObjToFraig );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Constructs initialized timeframes of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_BmcFrames( Fra_Bmc_t * p, int fKeepPos )
{
    Aig_Man_t * pAigFrames;
    Aig_Obj_t * pObj, * pObjNew;
    Aig_Obj_t ** pLatches;
    int i, k, f;

    // start the fraig package
    pAigFrames = Aig_ManStart( Aig_ManObjNumMax(p->pAig) * p->nFramesAll );
    pAigFrames->pName = Abc_UtilStrsav( p->pAig->pName );
    pAigFrames->pSpec = Abc_UtilStrsav( p->pAig->pSpec );
    // create PI nodes for the frames
    for ( f = 0; f < p->nFramesAll; f++ )
        Bmc_ObjSetFrames( Aig_ManConst1(p->pAig), f, Aig_ManConst1(pAigFrames) );
    for ( f = 0; f < p->nFramesAll; f++ )
        Aig_ManForEachPiSeq( p->pAig, pObj, i )
            Bmc_ObjSetFrames( pObj, f, Aig_ObjCreateCi(pAigFrames) );
    // set initial state for the latches
    Aig_ManForEachLoSeq( p->pAig, pObj, i )
        Bmc_ObjSetFrames( pObj, 0, Aig_ManConst0(pAigFrames) );

    // add timeframes
    pLatches = ABC_ALLOC( Aig_Obj_t *, Aig_ManRegNum(p->pAig) );
    for ( f = 0; f < p->nFramesAll; f++ )
    {
        // add internal nodes of this frame
        Aig_ManForEachNode( p->pAig, pObj, i )
        {
            pObjNew = Aig_And( pAigFrames, Bmc_ObjChild0Frames(pObj,f), Bmc_ObjChild1Frames(pObj,f) );
            Bmc_ObjSetFrames( pObj, f, pObjNew );
        }
        if ( f == p->nFramesAll - 1 )
            break;
        // save the latch input values
        k = 0;
        Aig_ManForEachLiSeq( p->pAig, pObj, i )
            pLatches[k++] = Bmc_ObjChild0Frames(pObj,f);
        assert( k == Aig_ManRegNum(p->pAig) );
        // insert them to the latch output values
        k = 0;
        Aig_ManForEachLoSeq( p->pAig, pObj, i )
            Bmc_ObjSetFrames( pObj, f+1, pLatches[k++] );
        assert( k == Aig_ManRegNum(p->pAig) );
    }
    ABC_FREE( pLatches );
    if ( fKeepPos )
    {
        for ( f = 0; f < p->nFramesAll; f++ )
            Aig_ManForEachPoSeq( p->pAig, pObj, i )
                Aig_ObjCreateCo( pAigFrames, Bmc_ObjChild0Frames(pObj,f) );
        Aig_ManCleanup( pAigFrames );
    }
    else
    {
        // add POs to all the dangling nodes
        Aig_ManForEachObj( pAigFrames, pObjNew, i )
            if ( Aig_ObjIsNode(pObjNew) && pObjNew->nRefs == 0 )
                Aig_ObjCreateCo( pAigFrames, pObjNew );
    }
    // return the new manager
    return pAigFrames;
}

/**Function*************************************************************

  Synopsis    [Performs BMC for the given AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_BmcPerform( Fra_Man_t * p, int nPref, int nDepth )
{
    Aig_Obj_t * pObj;
    int i, nImpsOld = 0;
    abctime clk = Abc_Clock();
    assert( p->pBmc == NULL );
    // derive and fraig the frames
    p->pBmc = Fra_BmcStart( p->pManAig, nPref, nDepth );
    p->pBmc->pAigFrames = Fra_BmcFrames( p->pBmc, 0 );
    // if implications are present, configure the AIG manager to check them
    if ( p->pCla->vImps )
    {
        p->pBmc->pAigFrames->pImpFunc = (void (*) (void*, void*))Fra_BmcFilterImplications;
        p->pBmc->pAigFrames->pImpData = p->pBmc;
        p->pBmc->vImps = p->pCla->vImps;
        nImpsOld = Vec_IntSize(p->pCla->vImps);
    } 
    p->pBmc->pAigFraig = Fra_FraigEquivence( p->pBmc->pAigFrames, 1000000, 0 );
    p->pBmc->pObjToFraig = p->pBmc->pAigFrames->pObjCopies;
    p->pBmc->pAigFrames->pObjCopies = NULL;
    // annotate frames nodes with pointers to the manager
    Aig_ManForEachObj( p->pBmc->pAigFrames, pObj, i )
        pObj->pData = p;
    // report the results
    if ( p->pPars->fVerbose )
    {
        printf( "Original AIG = %d. Init %d frames = %d. Fraig = %d.  ", 
            Aig_ManNodeNum(p->pBmc->pAig), p->pBmc->nFramesAll, 
            Aig_ManNodeNum(p->pBmc->pAigFrames), Aig_ManNodeNum(p->pBmc->pAigFraig) );
        ABC_PRT( "Time", Abc_Clock() - clk );
        printf( "Before BMC: " );  
//        Fra_ClassesPrint( p->pCla, 0 );
        printf( "Const = %5d. Class = %5d. Lit = %5d. ", 
            Vec_PtrSize(p->pCla->vClasses1), Vec_PtrSize(p->pCla->vClasses), Fra_ClassesCountLits(p->pCla) );
        if ( p->pCla->vImps )
            printf( "Imp = %5d. ", nImpsOld );
        printf( "\n" );
    }
    // refine the classes
    p->pCla->pFuncNodeIsConst   = Fra_BmcNodeIsConst;
    p->pCla->pFuncNodesAreEqual = Fra_BmcNodesAreEqual;
    Fra_ClassesRefine( p->pCla );
    Fra_ClassesRefine1( p->pCla, 1, NULL );
    p->pCla->pFuncNodeIsConst   = Fra_SmlNodeIsConst;
    p->pCla->pFuncNodesAreEqual = Fra_SmlNodesAreEqual;
    // report the results
    if ( p->pPars->fVerbose )
    {
        printf( "After  BMC: " );  
//        Fra_ClassesPrint( p->pCla, 0 );
        printf( "Const = %5d. Class = %5d. Lit = %5d. ", 
            Vec_PtrSize(p->pCla->vClasses1), Vec_PtrSize(p->pCla->vClasses), Fra_ClassesCountLits(p->pCla) );
        if ( p->pCla->vImps )
            printf( "Imp = %5d. ", Vec_IntSize(p->pCla->vImps) );
        printf( "\n" );
    }
    // free the BMC manager
    Fra_BmcStop( p->pBmc );  
    p->pBmc = NULL;
}

/**Function*************************************************************

  Synopsis    [Performs BMC for the given AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_BmcPerformSimple( Aig_Man_t * pAig, int nFrames, int nBTLimit, int fRewrite, int fVerbose )
{
    extern Fra_Man_t * Fra_LcrAigPrepare( Aig_Man_t * pAig );
    Fra_Man_t * pTemp;
    Fra_Bmc_t * pBmc;
    Aig_Man_t * pAigTemp;
    abctime clk;
    int iOutput;
    // derive and fraig the frames
    clk = Abc_Clock();
    pBmc = Fra_BmcStart( pAig, 0, nFrames );
    pTemp = Fra_LcrAigPrepare( pAig );
    pTemp->pBmc = pBmc;
    pBmc->pAigFrames = Fra_BmcFrames( pBmc, 1 );
    if ( fVerbose )
    {
        printf( "AIG:  PI/PO/Reg = %d/%d/%d.  Node = %6d. Lev = %5d.\n", 
            Aig_ManCiNum(pAig)-Aig_ManRegNum(pAig), Aig_ManCoNum(pAig)-Aig_ManRegNum(pAig), Aig_ManRegNum(pAig),
            Aig_ManNodeNum(pAig), Aig_ManLevelNum(pAig) );
        printf( "Time-frames (%d):  PI/PO = %d/%d.  Node = %6d. Lev = %5d.  ", 
            nFrames, Aig_ManCiNum(pBmc->pAigFrames), Aig_ManCoNum(pBmc->pAigFrames), 
            Aig_ManNodeNum(pBmc->pAigFrames), Aig_ManLevelNum(pBmc->pAigFrames) );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
    if ( fRewrite )
    {
        clk = Abc_Clock();
        pBmc->pAigFrames = Dar_ManRwsat( pAigTemp = pBmc->pAigFrames, 1, 0 );
        Aig_ManStop( pAigTemp );
        if ( fVerbose )
        {
            printf( "Time-frames after rewriting:  Node = %6d. Lev = %5d.  ", 
                Aig_ManNodeNum(pBmc->pAigFrames), Aig_ManLevelNum(pBmc->pAigFrames) );
            ABC_PRT( "Time", Abc_Clock() - clk );
        }
    }
    clk = Abc_Clock();
    iOutput = Fra_FraigMiterAssertedOutput( pBmc->pAigFrames );
    if ( iOutput >= 0 )
        pAig->pSeqModel = Abc_CexMakeTriv( Aig_ManRegNum(pAig), Aig_ManCiNum(pAig)-Aig_ManRegNum(pAig), Aig_ManCoNum(pAig)-Aig_ManRegNum(pAig), iOutput );
    else
    {
        pBmc->pAigFraig = Fra_FraigEquivence( pBmc->pAigFrames, nBTLimit, 1 );
        iOutput = Fra_FraigMiterAssertedOutput( pBmc->pAigFraig );
        if ( pBmc->pAigFraig->pData )
        {
            pAig->pSeqModel = Fra_SmlCopyCounterExample( pAig, pBmc->pAigFrames, (int *)pBmc->pAigFraig->pData );
            ABC_FREE( pBmc->pAigFraig->pData );
        }
        else if ( iOutput >= 0 )
            pAig->pSeqModel = Abc_CexMakeTriv( Aig_ManRegNum(pAig), Aig_ManCiNum(pAig)-Aig_ManRegNum(pAig), Aig_ManCoNum(pAig)-Aig_ManRegNum(pAig), iOutput );
    }
    if ( fVerbose )
    {
        printf( "Fraiged init frames: Node = %6d. Lev = %5d.  ", 
            pBmc->pAigFraig? Aig_ManNodeNum(pBmc->pAigFraig) : -1,
            pBmc->pAigFraig? Aig_ManLevelNum(pBmc->pAigFraig) : -1 );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
    Fra_BmcStop( pBmc );  
    ABC_FREE( pTemp );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

