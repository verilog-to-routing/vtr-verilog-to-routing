/**CFile****************************************************************

  FileName    [giaHcd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [New choice computation package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaHcd.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "giaAig.h"
#include "aig/aig/aig.h"
#include "opt/dar/dar.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// choicing parameters
typedef struct Hcd_Pars_t_ Hcd_Pars_t;
struct Hcd_Pars_t_
{
    int              nWords;        // the number of simulation words
    int              nBTLimit;      // conflict limit at a node
    int              nSatVarMax;    // the max number of SAT variables
    int              fSynthesis;    // set to 1 to perform synthesis
    int              fPolarFlip;    // uses polarity adjustment
    int              fSimulateTfo;  // uses simulation of TFO classes
    int              fPower;        // uses power-aware rewriting
    int              fUseGia;       // uses GIA package 
    int              fUseCSat;      // uses circuit-based solver
    int              fVerbose;      // verbose stats
    clock_t          timeSynth;     // synthesis runtime
    int              nNodesAhead;   // the lookahead in terms of nodes
    int              nCallsRecycle; // calls to perform before recycling SAT solver
};

extern void Gia_ComputeEquivalences( Gia_Man_t * pMiter, int nBTLimit, int fUseMiniSat, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hcd_ManSetDefaultParams( Hcd_Pars_t * p )
{
    memset( p, 0, sizeof(Hcd_Pars_t) );
    p->nWords         =     8;  // the number of simulation words
    p->nBTLimit       =  1000;  // conflict limit at a node
    p->nSatVarMax     =  5000;  // the max number of SAT variables
    p->fSynthesis     =     1;  // derives three snapshots
    p->fPolarFlip     =     1;  // uses polarity adjustment
    p->fSimulateTfo   =     1;  // simulate TFO
    p->fPower         =     0;  // power-aware rewriting
    p->fVerbose       =     0;  // verbose stats
    p->nNodesAhead    =  1000;  // the lookahead in terms of nodes
    p->nCallsRecycle  =   100;  // calls to perform before recycling SAT solver
}


/**Function*************************************************************

  Synopsis    [Reproduces script "compress".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Hcd_Compress( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fPower, int fVerbose )
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
    if ( fVerbose ) Aig_ManPrintStats( pAig );

    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );
    
    // refactor
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );

    // balance
    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );
    }

    pParsRwr->fUseZeros = 1;
    pParsRef->fUseZeros = 1;
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );

    return pAig;
}

/**Function*************************************************************

  Synopsis    [Reproduces script "compress2".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Hcd_Compress2( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fFanout, int fPower, int fVerbose )
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
    if ( fVerbose ) Aig_ManPrintStats( pAig );

    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );
    
    // refactor
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );

    // balance
//    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );
    }
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );

    pParsRwr->fUseZeros = 1;
    pParsRef->fUseZeros = 1;
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );

    // balance
    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );
    }
    
    // refactor
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDupDfs( pTemp = pAig ); 
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );

    // balance
    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    if ( fVerbose ) Aig_ManPrintStats( pAig );
    }
    return pAig;
}

/**Function*************************************************************

  Synopsis    [Reproduces script "compress2".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Hcd_ChoiceSynthesis( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fPower, int fVerbose )
//alias resyn    "b; rw; rwz; b; rwz; b"
//alias resyn2   "b; rw; rf; b; rw; rwz; b; rfz; rwz; b"
{
    Vec_Ptr_t * vGias;
    Gia_Man_t * pGia;

    vGias = Vec_PtrAlloc( 3 );
    pGia = Gia_ManFromAig(pAig);
    Vec_PtrPush( vGias, pGia );

    pAig = Hcd_Compress( pAig, fBalance, fUpdateLevel, fPower, fVerbose );
    pGia = Gia_ManFromAig(pAig);
    Vec_PtrPush( vGias, pGia );
//Aig_ManPrintStats( pAig );

    pAig = Hcd_Compress2( pAig, fBalance, fUpdateLevel, 1, fPower, fVerbose );
    pGia = Gia_ManFromAig(pAig);
    Vec_PtrPush( vGias, pGia );
//Aig_ManPrintStats( pAig );

    Aig_ManStop( pAig );
    return vGias;
}


/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hcd_ManChoiceMiter_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return pObj->Value;
    Hcd_ManChoiceMiter_rec( pNew, p, Gia_ObjFanin0(pObj) );
    if ( Gia_ObjIsCo(pObj) )
        return pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Hcd_ManChoiceMiter_rec( pNew, p, Gia_ObjFanin1(pObj) );
    return pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
} 

/**Function*************************************************************

  Synopsis    [Derives the miter of several AIGs for choice computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Hcd_ManChoiceMiter( Vec_Ptr_t * vGias )
{
    Gia_Man_t * pNew, * pGia, * pGia0;
    int i, k, iNode, nNodes;
    // make sure they have equal parameters
    assert( Vec_PtrSize(vGias) > 0 );
    pGia0 = (Gia_Man_t *)Vec_PtrEntry( vGias, 0 );
    Vec_PtrForEachEntry( Gia_Man_t *, vGias, pGia, i )
    {
        assert( Gia_ManCiNum(pGia)  == Gia_ManCiNum(pGia0) );
        assert( Gia_ManCoNum(pGia)  == Gia_ManCoNum(pGia0) );
        assert( Gia_ManRegNum(pGia) == Gia_ManRegNum(pGia0) );
        Gia_ManFillValue( pGia );
        Gia_ManConst0(pGia)->Value = 0;
    }
    // start the new manager
    pNew = Gia_ManStart( Vec_PtrSize(vGias) * Gia_ManObjNum(pGia0) );
    pNew->pName = Abc_UtilStrsav( pGia0->pName );
    pNew->pSpec = Abc_UtilStrsav( pGia0->pSpec );
    // create new CIs and assign them to the old manager CIs
    for ( k = 0; k < Gia_ManCiNum(pGia0); k++ )
    {
        iNode = Gia_ManAppendCi(pNew);
        Vec_PtrForEachEntry( Gia_Man_t *, vGias, pGia, i )
            Gia_ManCi( pGia, k )->Value = iNode; 
    }
    // create internal nodes
    Gia_ManHashAlloc( pNew );
    for ( k = 0; k < Gia_ManCoNum(pGia0); k++ )
    {
        Vec_PtrForEachEntry( Gia_Man_t *, vGias, pGia, i )
            Hcd_ManChoiceMiter_rec( pNew, pGia, Gia_ManCo( pGia, k ) );
    }
    Gia_ManHashStop( pNew );
    // check the presence of dangling nodes
    nNodes = Gia_ManHasDangling( pNew );
    assert( nNodes == 0 );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pOld is in the TFI of pNode.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hcd_ObjCheckTfi_rec( Gia_Man_t * p, Gia_Obj_t * pOld, Gia_Obj_t * pNode, Vec_Ptr_t * vVisited )
{
    // check the trivial cases
    if ( pNode == NULL )
        return 0;
    if ( Gia_ObjIsCi(pNode) )
        return 0;
//    if ( pNode->Id < pOld->Id ) // cannot use because of choices of pNode
//        return 0;
    if ( pNode == pOld )
        return 1;
    // skip the visited node
    if ( pNode->fMark0 )
        return 0;
    pNode->fMark0 = 1;
    Vec_PtrPush( vVisited, pNode );
    // check the children
    if ( Hcd_ObjCheckTfi_rec( p, pOld, Gia_ObjFanin0(pNode), vVisited ) )
        return 1;
    if ( Hcd_ObjCheckTfi_rec( p, pOld, Gia_ObjFanin1(pNode), vVisited ) )
        return 1;
    // check equivalent nodes
    return Hcd_ObjCheckTfi_rec( p, pOld, Gia_ObjNextObj(p, Gia_ObjId(p, pNode)), vVisited );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pOld is in the TFI of pNode.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hcd_ObjCheckTfi( Gia_Man_t * p, Gia_Obj_t * pOld, Gia_Obj_t * pNode )
{
    Vec_Ptr_t * vVisited;
    Gia_Obj_t * pObj;
    int RetValue, i;
    assert( !Gia_IsComplement(pOld) );
    assert( !Gia_IsComplement(pNode) );
    vVisited = Vec_PtrAlloc( 100 );
    RetValue = Hcd_ObjCheckTfi_rec( p, pOld, pNode, vVisited );
    Vec_PtrForEachEntry( Gia_Obj_t *, vVisited, pObj, i )
        pObj->fMark0 = 0;
    Vec_PtrFree( vVisited );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Adds the next entry while making choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hcd_ManAddNextEntry_rec( Gia_Man_t * p, Gia_Obj_t * pOld, Gia_Obj_t * pNode )
{
    if ( Gia_ObjNext(p, Gia_ObjId(p, pOld)) == 0 )
    {
        Gia_ObjSetNext( p, Gia_ObjId(p, pOld), Gia_ObjId(p, pNode) );
        return;
    }
    Hcd_ManAddNextEntry_rec( p, Gia_ObjNextObj(p, Gia_ObjId(p, pOld)), pNode );
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hcd_ManEquivToChoices_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    Gia_Obj_t * pRepr, * pReprNew, * pObjNew;
    if ( ~pObj->Value )
        return;
    if ( (pRepr = Gia_ObjReprObj(p, Gia_ObjId(p, pObj))) )
    {
        if ( Gia_ObjIsConst0(pRepr) )
        {
            pObj->Value = Abc_LitNotCond( pRepr->Value, Gia_ObjPhaseReal(pRepr) ^ Gia_ObjPhaseReal(pObj) );
            return;
        }
        Hcd_ManEquivToChoices_rec( pNew, p, pRepr );
        assert( Gia_ObjIsAnd(pObj) );
        Hcd_ManEquivToChoices_rec( pNew, p, Gia_ObjFanin0(pObj) );
        Hcd_ManEquivToChoices_rec( pNew, p, Gia_ObjFanin1(pObj) );
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        if ( Abc_LitRegular(pObj->Value) == Abc_LitRegular(pRepr->Value) )
        {
            assert( (int)pObj->Value == Abc_LitNotCond( pRepr->Value, Gia_ObjPhaseReal(pRepr) ^ Gia_ObjPhaseReal(pObj) ) );
            return;
        }
        if ( pRepr->Value > pObj->Value ) // should never happen with high resource limit
            return;
        assert( pRepr->Value < pObj->Value );
        pReprNew = Gia_ManObj( pNew, Abc_Lit2Var(pRepr->Value) );
        pObjNew  = Gia_ManObj( pNew, Abc_Lit2Var(pObj->Value) );
        if ( Gia_ObjReprObj( pNew, Gia_ObjId(pNew, pObjNew) ) )
        {
            assert( Gia_ObjReprObj( pNew, Gia_ObjId(pNew, pObjNew) ) == pReprNew );
            pObj->Value = Abc_LitNotCond( pRepr->Value, Gia_ObjPhaseReal(pRepr) ^ Gia_ObjPhaseReal(pObj) );
            return;
        }
        if ( !Hcd_ObjCheckTfi( pNew, pReprNew, pObjNew ) )
        {
            assert( Gia_ObjNext(pNew, Gia_ObjId(pNew, pObjNew)) == 0 );
            Gia_ObjSetRepr( pNew, Gia_ObjId(pNew, pObjNew), Gia_ObjId(pNew, pReprNew) );
            Hcd_ManAddNextEntry_rec( pNew, pReprNew, pObjNew ); 
        }
        pObj->Value = Abc_LitNotCond( pRepr->Value, Gia_ObjPhaseReal(pRepr) ^ Gia_ObjPhaseReal(pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Hcd_ManEquivToChoices_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Hcd_ManEquivToChoices_rec( pNew, p, Gia_ObjFanin1(pObj) );
    pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    [Removes choices, which contain fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hcd_ManRemoveBadChoices( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, iObj, iPrev, Counter = 0;
    // mark nodes with fanout
    Gia_ManForEachObj( p, pObj, i )
    {
        pObj->fMark0 = 0;
        if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjFanin0(pObj)->fMark0 = 1;
            Gia_ObjFanin1(pObj)->fMark0 = 1;
        }
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjFanin0(pObj)->fMark0 = 1;
    }
    // go through the classes and remove 
    Gia_ManForEachClass( p, i )
    {
        for ( iPrev = i, iObj = Gia_ObjNext(p, i); iObj; iObj = Gia_ObjNext(p, iPrev) )
        {
            if ( !Gia_ManObj(p, iObj)->fMark0 )
            {
                iPrev = iObj; 
                continue;
            }
            Gia_ObjSetRepr( p, iObj, GIA_VOID );
            Gia_ObjSetNext( p, iPrev, Gia_ObjNext(p, iObj) );
            Gia_ObjSetNext( p, iObj, 0 );
            Counter++;
        } 
    }
    // remove the marks
    Gia_ManCleanMark0( p );
//    printf( "Removed %d bad choices.\n", Counter );
}

/**Function*************************************************************

  Synopsis    [Reduces AIG using equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Hcd_ManEquivToChoices( Gia_Man_t * p, int nSnapshots )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pRepr;
    int i;
    assert( (Gia_ManCoNum(p) % nSnapshots) == 0 );
    Gia_ManSetPhase( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(p) );
    pNew->pNexts = ABC_CALLOC( int, Gia_ManObjNum(p) );
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
        Gia_ObjSetRepr( pNew, i, GIA_VOID );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachRo( p, pObj, i )
        if ( (pRepr = Gia_ObjReprObj(p, Gia_ObjId(p, pObj))) )
        {
            assert( Gia_ObjIsConst0(pRepr) || Gia_ObjIsRo(p, pRepr) );
            pObj->Value = pRepr->Value;
        }
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCo( p, pObj, i )
        Hcd_ManEquivToChoices_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        if ( i % nSnapshots == 0 )
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    Hcd_ManRemoveBadChoices( pNew );
//    Gia_ManEquivPrintClasses( pNew, 0, 0 );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
//    Gia_ManEquivPrintClasses( pNew, 0, 0 );
    return pNew;
} 

/**Function*************************************************************

  Synopsis    [Performs computation of AIGs with choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Hcd_ComputeChoices( Aig_Man_t * pAig, int nBTLimit, int fSynthesis, int fUseMiniSat, int fVerbose )
{
    Vec_Ptr_t * vGias;
    Gia_Man_t * pGia, * pMiter;
    Aig_Man_t * pAigNew;
    int i;
    clock_t clk = clock();
    // perform synthesis
    if ( fSynthesis )
    {
        vGias = Hcd_ChoiceSynthesis( Aig_ManDupDfs(pAig), 1, 1, 0, 0 );
        if ( fVerbose )
            ABC_PRT( "Synthesis time", clock() - clk );
        // create choices
        clk = clock();
        pMiter = Hcd_ManChoiceMiter( vGias );
        Vec_PtrForEachEntry( Gia_Man_t *, vGias, pGia, i )
            Gia_ManStop( pGia );

        Gia_AigerWrite( pMiter, "m3.aig", 0, 0, 0 );
    }
    else 
    {
        vGias = Vec_PtrStart( 3 );
        pMiter = Gia_ManFromAig(pAig);
    }
    // perform choicing
    Gia_ComputeEquivalences( pMiter, nBTLimit, fUseMiniSat, fVerbose );
    // derive AIG with choices
    pGia = Hcd_ManEquivToChoices( pMiter, Vec_PtrSize(vGias) );
    Gia_ManSetRegNum( pGia, Aig_ManRegNum(pAig) );
    Gia_ManStop( pMiter );
    Vec_PtrFree( vGias );
    if ( fVerbose )
        ABC_PRT( "Choicing time", clock() - clk );
//    Gia_ManHasChoices_very_old( pGia );
    // transform back
    pAigNew = Gia_ManToAig( pGia, 1 );
    Gia_ManStop( pGia );

    if ( fVerbose )
    {
        extern int Dch_DeriveChoiceCountReprs( Aig_Man_t * pAig );
        extern int Dch_DeriveChoiceCountEquivs( Aig_Man_t * pAig );
        printf( "Choices   : Reprs = %5d. Equivs = %5d. Choices = %5d.\n", 
            Dch_DeriveChoiceCountReprs( pAigNew ), 
            Dch_DeriveChoiceCountEquivs( pAigNew ), 
            Aig_ManChoiceNum( pAigNew ) );
    }

    return pAigNew;
}

/**Function*************************************************************

  Synopsis    [Performs computation of AIGs with choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hcd_ComputeChoicesTest( Gia_Man_t * pGia, int nBTLimit, int fSynthesis, int fUseMiniSat, int fVerbose )
{
    Aig_Man_t * pAig, * pAigNew;
    pAig = Gia_ManToAig( pGia, 0 );
    pAigNew = Hcd_ComputeChoices( pAig, nBTLimit, fSynthesis, fUseMiniSat, fVerbose );
    Aig_ManStop( pAigNew );
    Aig_ManStop( pAig );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

