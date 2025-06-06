/**CFile****************************************************************

  FileName    [cecSynth.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [Partitioned sequential synthesis.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecSynth.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cecInt.h"
#include "aig/gia/giaAig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Populate sequential synthesis parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_SeqSynthesisSetDefaultParams( Cec_ParSeq_t * p )
{
    memset( p, 0, sizeof(Cec_ParSeq_t) );
    p->fUseLcorr    =    0;  // enables latch correspondence
    p->fUseScorr    =    0;  // enables signal correspondence
    p->nBTLimit     = 1000;  // (scorr/lcorr) conflict limit at a node
    p->nFrames      =    1;  // (scorr only) the number of timeframes
    p->nLevelMax    =   -1;  // (scorr only) the max number of levels
    p->fConsts      =    1;  // (scl only) merging constants
    p->fEquivs      =    1;  // (scl only) merging equivalences
    p->fUseMiniSat  =    0;  // enables MiniSat in lcorr/scorr
    p->nMinDomSize  =  100;  // the size of minimum clock domain
    p->fVeryVerbose =    0;  // verbose stats
    p->fVerbose     =    0;  // verbose stats
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_SeqReadMinDomSize( Cec_ParSeq_t * p )
{
    return p->nMinDomSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_SeqReadVerbose( Cec_ParSeq_t * p )
{
    return p->fVerbose;
}

/**Function*************************************************************

  Synopsis    [Computes partitioning of registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManRegCreatePart( Gia_Man_t * p, Vec_Int_t * vPart, int * pnCountPis, int * pnCountRegs, int ** ppMapBack )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    Vec_Int_t * vNodes, * vRoots;
    int i, iOut, nCountPis, nCountRegs;
    int * pMapBack;
    // collect/mark nodes/PIs in the DFS order from the roots
    Gia_ManIncrementTravId( p );
    vRoots  = Vec_IntAlloc( Vec_IntSize(vPart) );
    Vec_IntForEachEntry( vPart, iOut, i )
        Vec_IntPush( vRoots, Gia_ObjId(p, Gia_ManCo(p, Gia_ManPoNum(p)+iOut)) );
    vNodes = Gia_ManCollectNodesCis( p, Vec_IntArray(vRoots), Vec_IntSize(vRoots) );
    Vec_IntFree( vRoots );
    // unmark register outputs
    Vec_IntForEachEntry( vPart, iOut, i )
        Gia_ObjSetTravIdPrevious( p, Gia_ManCi(p, Gia_ManPiNum(p)+iOut) );
    // count pure PIs
    nCountPis = nCountRegs = 0;
    Gia_ManForEachPi( p, pObj, i )
        nCountPis += Gia_ObjIsTravIdCurrent(p, pObj);
    // count outputs of other registers
    Gia_ManForEachRo( p, pObj, i )
        nCountRegs += Gia_ObjIsTravIdCurrent(p, pObj); // should be !Gia_... ???
    if ( pnCountPis )
        *pnCountPis = nCountPis;
    if ( pnCountRegs )
        *pnCountRegs = nCountRegs;
    // clean old manager
    Gia_ManFillValue(p);
    Gia_ManConst0(p)->Value = 0;
    // create the new manager
    pNew = Gia_ManStart( Vec_IntSize(vNodes) );
    // create the PIs
    Gia_ManForEachCi( p, pObj, i )
        if ( Gia_ObjIsTravIdCurrent(p, pObj) )
            pObj->Value = Gia_ManAppendCi(pNew);
    // add variables for the register outputs
    // create fake POs to hold the register outputs
    Vec_IntForEachEntry( vPart, iOut, i )
    {
        pObj = Gia_ManCi(p, Gia_ManPiNum(p)+iOut);
        pObj->Value = Gia_ManAppendCi(pNew);
        Gia_ManAppendCo( pNew, pObj->Value );
        Gia_ObjSetTravIdCurrent( p, pObj ); // added
    }
    // create the nodes
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // add real POs for the registers
    Vec_IntForEachEntry( vPart, iOut, i )
    {
        pObj = Gia_ManCo( p, Gia_ManPoNum(p)+iOut );
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Gia_ManSetRegNum( pNew, Vec_IntSize(vPart) );
    // create map
    if ( ppMapBack )
    {
        pMapBack = ABC_FALLOC( int, Gia_ManObjNum(pNew) );
        // map constant nodes
        pMapBack[0] = 0;
        // logic cones of register outputs
        Gia_ManForEachObjVec( vNodes, p, pObj, i )
        {
//            pObjNew = Aig_Regular(pObj->pData);
//            pMapBack[pObjNew->Id] = pObj->Id;
            assert( Abc_Lit2Var(Gia_ObjValue(pObj)) >= 0 );
            assert( Abc_Lit2Var(Gia_ObjValue(pObj)) < Gia_ManObjNum(pNew) );
            pMapBack[ Abc_Lit2Var(Gia_ObjValue(pObj)) ] = Gia_ObjId(p, pObj);
        }
        // map register outputs
        Vec_IntForEachEntry( vPart, iOut, i )
        {
            pObj = Gia_ManCi(p, Gia_ManPiNum(p)+iOut);
//            pObjNew = pObj->pData;
//            pMapBack[pObjNew->Id] = pObj->Id;
            assert( Abc_Lit2Var(Gia_ObjValue(pObj)) >= 0 );
            assert( Abc_Lit2Var(Gia_ObjValue(pObj)) < Gia_ManObjNum(pNew) );
            pMapBack[ Abc_Lit2Var(Gia_ObjValue(pObj)) ] = Gia_ObjId(p, pObj);
        }
        *ppMapBack = pMapBack;
    }
    Vec_IntFree( vNodes );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Transfers the classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_TransferMappedClasses( Gia_Man_t * pPart, int * pMapBack, int * pReprs )
{
    Gia_Obj_t * pObj;
    int i, Id1, Id2, nClasses;
    if ( pPart->pReprs == NULL ) 
        return 0;
    nClasses = 0;
    Gia_ManForEachObj( pPart, pObj, i )
    {
        if ( Gia_ObjRepr(pPart, i) == GIA_VOID )
            continue;
        assert( i                     < Gia_ManObjNum(pPart) );
        assert( Gia_ObjRepr(pPart, i) < Gia_ManObjNum(pPart) );
        Id1 = pMapBack[ i ];
        Id2 = pMapBack[ Gia_ObjRepr(pPart, i) ];
        if ( Id1 == Id2 )
            continue;
        if ( Id1 < Id2 )
            pReprs[Id2] = Id1;
        else
            pReprs[Id1] = Id2;
        nClasses++;
    }
    return nClasses;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManFindRepr_rec( int * pReprs, int Id )
{
    if ( pReprs[Id] == 0 )
        return 0;
    if ( pReprs[Id] == ~0 )
        return Id;
    return Gia_ManFindRepr_rec( pReprs, pReprs[Id] );
}

/**Function*************************************************************

  Synopsis    [Normalizes equivalences.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManNormalizeEquivalences( Gia_Man_t * p, int * pReprs )
{
    int i, iRepr;
    assert( p->pReprs == NULL );
    assert( p->pNexts == NULL );
    p->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(p) );
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
        Gia_ObjSetRepr( p, i, GIA_VOID );
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
    {
        if ( pReprs[i] == ~0 )
            continue;
        iRepr = Gia_ManFindRepr_rec( pReprs, i );
        Gia_ObjSetRepr( p, i, iRepr );
    }
    p->pNexts = Gia_ManDeriveNexts( p );
}

/**Function*************************************************************

  Synopsis    [Partitioned sequential synthesis.]

  Description [Returns AIG annotated with equivalence classes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_SequentialSynthesisPart( Gia_Man_t * p, Cec_ParSeq_t * pPars )
{
    int fPrintParts = 0;
    char Buffer[100];
    Gia_Man_t * pTemp;
    Vec_Ptr_t * vParts = (Vec_Ptr_t *)p->vClockDoms;
    Vec_Int_t * vPart;
    int * pMapBack, * pReprs;
    int i, nCountPis, nCountRegs;
    int nClasses;
    abctime clk = Abc_Clock();

    // save parameters
    if ( fPrintParts )
    {
        // print partitions
        Abc_Print( 1, "The following clock domains are used:\n" );
        Vec_PtrForEachEntry( Vec_Int_t *, vParts, vPart, i )
        {
            pTemp = Gia_ManRegCreatePart( p, vPart, &nCountPis, &nCountRegs, NULL );
            sprintf( Buffer, "part%03d.aig", i );
            Gia_AigerWrite( pTemp, Buffer, 0, 0, 0 );
            Abc_Print( 1, "part%03d.aig : Reg = %4d. PI = %4d. (True = %4d. Regs = %4d.) And = %5d.\n", 
                i, Vec_IntSize(vPart), Gia_ManCiNum(pTemp)-Vec_IntSize(vPart), nCountPis, nCountRegs, Gia_ManAndNum(pTemp) );
            Gia_ManStop( pTemp );
        }
    }

    // perform sequential synthesis for clock domains
    pReprs = ABC_FALLOC( int, Gia_ManObjNum(p) );
    Vec_PtrForEachEntry( Vec_Int_t *, vParts, vPart, i )
    {
        pTemp = Gia_ManRegCreatePart( p, vPart, &nCountPis, &nCountRegs, &pMapBack );
        if ( nCountPis > 0 ) 
        {
            if ( pPars->fUseScorr )
            {
                Cec_ParCor_t CorPars, * pCorPars = &CorPars;
                Cec_ManCorSetDefaultParams( pCorPars );
                pCorPars->nBTLimit   = pPars->nBTLimit;
                pCorPars->nLevelMax  = pPars->nLevelMax;
                pCorPars->fVerbose   = pPars->fVeryVerbose;
                pCorPars->fUseCSat   = 1;
                Cec_ManLSCorrespondenceClasses( pTemp, pCorPars );
            }
            else if ( pPars->fUseLcorr )
            {
                Cec_ParCor_t CorPars, * pCorPars = &CorPars;
                Cec_ManCorSetDefaultParams( pCorPars );
                pCorPars->fLatchCorr = 1;
                pCorPars->nBTLimit   = pPars->nBTLimit;
                pCorPars->fVerbose   = pPars->fVeryVerbose;
                pCorPars->fUseCSat   = 1;
                Cec_ManLSCorrespondenceClasses( pTemp, pCorPars );
            }
            else
            {
//                pNew = Gia_ManSeqStructSweep( pTemp, pPars->fConsts, pPars->fEquivs, pPars->fVerbose );
//                Gia_ManStop( pNew );
                Gia_ManSeqCleanupClasses( pTemp, pPars->fConsts, pPars->fEquivs, pPars->fVerbose );
            }
//Abc_Print( 1, "Part equivalences = %d.\n", Gia_ManEquivCountLitsAll(pTemp) );
            nClasses = Gia_TransferMappedClasses( pTemp, pMapBack, pReprs );
            if ( pPars->fVerbose )
            {
                Abc_Print( 1, "%3d : Reg = %4d. PI = %4d. (True = %4d. Regs = %4d.) And = %5d. Cl = %5d.\n", 
                    i, Vec_IntSize(vPart), Gia_ManCiNum(pTemp)-Vec_IntSize(vPart), nCountPis, nCountRegs, Gia_ManAndNum(pTemp), nClasses );
            }
        }
        Gia_ManStop( pTemp );
        ABC_FREE( pMapBack );
    }

    // generate resulting equivalences
    Gia_ManNormalizeEquivalences( p, pReprs );
//Abc_Print( 1, "Total equivalences = %d.\n", Gia_ManEquivCountLitsAll(p) );
    ABC_FREE( pReprs );
    if ( pPars->fVerbose )
    {
        Abc_PrintTime( 1, "Total time", Abc_Clock() - clk );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManTestOnePair_rec( sat_solver * pSat, Gia_Man_t * p, int iObj, Vec_Int_t * vSatVar )
{
    Gia_Obj_t * pObj; 
    int iVar, iVar0, iVar1;
    if ( Vec_IntEntry(vSatVar, iObj) >= 0 )
        return Vec_IntEntry(vSatVar, iObj);
    iVar = sat_solver_addvar(pSat);
    Vec_IntWriteEntry( vSatVar, iObj, iVar );
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsAnd(pObj) )
    {
        iVar0 = Gia_ManTestOnePair_rec( pSat, p, Gia_ObjFaninId0(pObj, iObj), vSatVar );
        iVar1 = Gia_ManTestOnePair_rec( pSat, p, Gia_ObjFaninId1(pObj, iObj), vSatVar );
        sat_solver_add_and( pSat, iVar, iVar0, iVar1, Gia_ObjFaninC0(pObj), Gia_ObjFaninC1(pObj), 0 );
    }
    return iVar;
}
int Gia_ManTestOnePair( Gia_Man_t * p, int iObj1, int iObj2, int fPhase )
{
    int Lits[2] = {1}, status;
    sat_solver * pSat = sat_solver_new();
    Vec_Int_t * vSatVar = Vec_IntStartFull( Gia_ManObjNum(p) );
    Vec_IntWriteEntry( vSatVar, 0, sat_solver_addvar(pSat) );
    sat_solver_addclause( pSat, Lits, Lits + 1 );
    Gia_ManTestOnePair_rec( pSat, p, iObj1, vSatVar );
    Gia_ManTestOnePair_rec( pSat, p, iObj2, vSatVar );
    Lits[0] = Abc_Var2Lit( Vec_IntEntry(vSatVar, iObj1), 1 );
    Lits[1] = Abc_Var2Lit( Vec_IntEntry(vSatVar, iObj2), fPhase );
    status = sat_solver_solve( pSat, Lits, Lits + 2, 0, 0, 0, 0 );
    if ( status == l_False )
    {
        Lits[0] = Abc_LitNot( Lits[0] );
        Lits[1] = Abc_LitNot( Lits[1] );
        status = sat_solver_solve( pSat, Lits, Lits + 2, 0, 0, 0, 0 );
    }
    Vec_IntFree( vSatVar );
    sat_solver_delete( pSat );
    return status;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

