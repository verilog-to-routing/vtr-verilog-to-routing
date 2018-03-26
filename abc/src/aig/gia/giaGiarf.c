/**CFile****************************************************************

  FileName    [gia.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: gia.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// combinational simulation manager
typedef struct Hcd_Man_t_ Hcd_Man_t;
struct Hcd_Man_t_
{
    // parameters
    Gia_Man_t *      pGia;           // the AIG to be used for simulation
    int              nBTLimit;       // internal backtrack limit 
    int              fVerbose;       // internal verbose flag 
    // internal variables
    unsigned *       pSimInfo;       // simulation info for each object
    Vec_Ptr_t *      vSimInfo;       // pointers to the CI simulation info
    Vec_Ptr_t *      vSimPres;       // pointers to the presense of simulation info
    // temporaries
    Vec_Int_t *      vClassOld;      // old class numbers
    Vec_Int_t *      vClassNew;      // new class numbers
    Vec_Int_t *      vClassTemp;     // temporary storage
    Vec_Int_t *      vRefinedC;      // refined const reprs
};

static inline unsigned   Hcd_ObjSim( Hcd_Man_t * p, int Id )                  { return p->pSimInfo[Id];      }
static inline unsigned * Hcd_ObjSimP( Hcd_Man_t * p, int Id )                 { return p->pSimInfo + Id;     }
static inline unsigned   Hcd_ObjSetSim( Hcd_Man_t * p, int Id, unsigned n )   { return p->pSimInfo[Id] = n;  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hcd_Man_t * Gia_ManEquivStart( Gia_Man_t * pGia, int nBTLimit, int fVerbose )
{
    Hcd_Man_t * p;
    Gia_Obj_t * pObj;
    int i;
    p = ABC_CALLOC( Hcd_Man_t, 1 );
    p->pGia       = pGia;
    p->nBTLimit   = nBTLimit;
    p->fVerbose   = fVerbose;
    p->pSimInfo   = ABC_ALLOC( unsigned, Gia_ManObjNum(pGia) );
    p->vClassOld  = Vec_IntAlloc( 100 );
    p->vClassNew  = Vec_IntAlloc( 100 );
    p->vClassTemp = Vec_IntAlloc( 100 );
    p->vRefinedC  = Vec_IntAlloc( 100 );
    // collect simulation info
    p->vSimInfo = Vec_PtrAlloc( 1000 );
    Gia_ManForEachCi( pGia, pObj, i )
        Vec_PtrPush( p->vSimInfo, Hcd_ObjSimP(p, Gia_ObjId(pGia,pObj)) );
    p->vSimPres = Vec_PtrAllocSimInfo( Gia_ManCiNum(pGia), 1 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Starts the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEquivStop( Hcd_Man_t * p )
{
    Vec_PtrFree( p->vSimInfo );
    Vec_PtrFree( p->vSimPres );
    Vec_IntFree( p->vClassOld );
    Vec_IntFree( p->vClassNew );
    Vec_IntFree( p->vClassTemp );
    Vec_IntFree( p->vRefinedC );
    ABC_FREE( p->pSimInfo );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Compared two simulation infos.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Hcd_ManCompareEqual( unsigned s0, unsigned s1 )
{
    if ( (s0 & 1) == (s1 & 1) )
        return s0 == s1;
    else
        return s0 ==~s1;
}

/**Function*************************************************************

  Synopsis    [Compares one simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Hcd_ManCompareConst( unsigned s )
{
    if ( s & 1 )
        return s ==~0;
    else
        return s == 0;
}

/**Function*************************************************************

  Synopsis    [Creates one equivalence class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hcd_ManClassCreate( Gia_Man_t * pGia, Vec_Int_t * vClass )
{
    int Repr = -1, EntPrev = -1, Ent, i;
    assert( Vec_IntSize(vClass) > 0 );
    Vec_IntForEachEntry( vClass, Ent, i )
    {
        if ( i == 0 )
        {
            Repr = Ent;
            Gia_ObjSetRepr( pGia, Ent, -1 );
            EntPrev = Ent;
        }
        else
        {
            Gia_ObjSetRepr( pGia, Ent, Repr );
            Gia_ObjSetNext( pGia, EntPrev, Ent );
            EntPrev = Ent;
        }
    }
    Gia_ObjSetNext( pGia, EntPrev, 0 );
}

/**Function*************************************************************

  Synopsis    [Refines one equivalence class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hcd_ManClassClassRemoveOne( Hcd_Man_t * p, int i )
{
    int iRepr, Ent;
    if ( Gia_ObjIsConst(p->pGia, i) )
    {
        Gia_ObjSetRepr( p->pGia, i, GIA_VOID );
        return 1;
    }
    if ( !Gia_ObjIsClass(p->pGia, i) )
        return 0;
    assert( Gia_ObjIsClass(p->pGia, i) );
    iRepr = Gia_ObjRepr( p->pGia, i );
    if ( iRepr == GIA_VOID )
        iRepr = i;
    // collect nodes
    Vec_IntClear( p->vClassOld );
    Vec_IntClear( p->vClassNew );
    Gia_ClassForEachObj( p->pGia, iRepr, Ent )
    {
        if ( Ent == i )
            Vec_IntPush( p->vClassNew, Ent );
        else
            Vec_IntPush( p->vClassOld, Ent );
    }
    assert( Vec_IntSize( p->vClassNew ) == 1 );
    Hcd_ManClassCreate( p->pGia, p->vClassOld );
    Hcd_ManClassCreate( p->pGia, p->vClassNew );
    assert( !Gia_ObjIsClass(p->pGia, i) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Refines one equivalence class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hcd_ManClassRefineOne( Hcd_Man_t * p, int i )
{
    unsigned Sim0, Sim1;
    int Ent;
    Vec_IntClear( p->vClassOld );
    Vec_IntClear( p->vClassNew );
    Vec_IntPush( p->vClassOld, i );
    Sim0 = Hcd_ObjSim(p, i);
    Gia_ClassForEachObj1( p->pGia, i, Ent )
    {
        Sim1 = Hcd_ObjSim(p, Ent);
        if ( Hcd_ManCompareEqual( Sim0, Sim1 ) )
            Vec_IntPush( p->vClassOld, Ent );
        else
            Vec_IntPush( p->vClassNew, Ent );
    }
    if ( Vec_IntSize( p->vClassNew ) == 0 )
        return 0;
    Hcd_ManClassCreate( p->pGia, p->vClassOld );
    Hcd_ManClassCreate( p->pGia, p->vClassNew );
    if ( Vec_IntSize(p->vClassNew) > 1 )
        return 1 + Hcd_ManClassRefineOne( p, Vec_IntEntry(p->vClassNew,0) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes hash key of the simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hcd_ManHashKey( unsigned * pSim, int nWords, int nTableSize )
{
    static int s_Primes[16] = { 
        1291, 1699, 1999, 2357, 2953, 3313, 3907, 4177, 
        4831, 5147, 5647, 6343, 6899, 7103, 7873, 8147 };
    unsigned uHash = 0;
    int i;
    if ( pSim[0] & 1 )
        for ( i = 0; i < nWords; i++ )
            uHash ^= ~pSim[i] * s_Primes[i & 0xf];
    else
        for ( i = 0; i < nWords; i++ )
            uHash ^=  pSim[i] * s_Primes[i & 0xf];
    return (int)(uHash % nTableSize);

}

/**Function*************************************************************

  Synopsis    [Rehashes the refined classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hcd_ManClassesRehash( Hcd_Man_t * p, Vec_Int_t * vRefined )
{
    int * pTable, nTableSize, Key, i, k;
    nTableSize = Abc_PrimeCudd( 100 + Vec_IntSize(vRefined) / 5 );
    pTable = ABC_CALLOC( int, nTableSize );
    Vec_IntForEachEntry( vRefined, i, k )
    {
        assert( !Hcd_ManCompareConst( Hcd_ObjSim(p, i) ) );
        Key = Hcd_ManHashKey( Hcd_ObjSimP(p, i), 1, nTableSize );
        if ( pTable[Key] == 0 )
            Gia_ObjSetRepr( p->pGia, i, GIA_VOID );
        else
        {
            Gia_ObjSetNext( p->pGia, pTable[Key], i );
            Gia_ObjSetRepr( p->pGia, i, Gia_ObjRepr(p->pGia, pTable[Key]) );
            if ( Gia_ObjRepr(p->pGia, i) == GIA_VOID )
                Gia_ObjSetRepr( p->pGia, i, pTable[Key] );
        }
        pTable[Key] = i;
    }
    ABC_FREE( pTable );
//    Gia_ManEquivPrintClasses( p->pGia, 0, 0.0 );
    // refine classes in the table
    Vec_IntForEachEntry( vRefined, i, k )
    {
        if ( Gia_ObjIsHead( p->pGia, i ) )
            Hcd_ManClassRefineOne( p, i );
    }
    Gia_ManEquivPrintClasses( p->pGia, 0, 0.0 );
}

/**Function*************************************************************

  Synopsis    [Refines equivalence classes after simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hcd_ManClassesRefine( Hcd_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    Vec_IntClear( p->vRefinedC );
    Gia_ManForEachAnd( p->pGia, pObj, i )
    {
        if ( Gia_ObjIsTail(p->pGia, i) ) // add check for the class level
        {
            Hcd_ManClassRefineOne( p, Gia_ObjRepr(p->pGia, i) );
        }
        else if ( Gia_ObjIsConst(p->pGia, i) )
        {
            if ( !Hcd_ManCompareConst( Hcd_ObjSim(p, i) ) )
                Vec_IntPush( p->vRefinedC, i );
        }
    }
    Hcd_ManClassesRehash( p, p->vRefinedC );
}

/**Function*************************************************************

  Synopsis    [Creates equivalence classes for the first time.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hcd_ManClassesCreate( Hcd_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    assert( p->pGia->pReprs == NULL );
    p->pGia->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(p->pGia) );
    p->pGia->pNexts = ABC_CALLOC( int, Gia_ManObjNum(p->pGia) );
    Gia_ManForEachObj( p->pGia, pObj, i )
        Gia_ObjSetRepr( p->pGia, i, Gia_ObjIsAnd(pObj) ? 0 : GIA_VOID );
}

/**Function*************************************************************

  Synopsis    [Initializes simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hcd_ManSimulationInit( Hcd_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachCi( p->pGia, pObj, i )
        Hcd_ObjSetSim( p, i, (Gia_ManRandom(0) << 1) );
}

/**Function*************************************************************

  Synopsis    [Performs one round of simple combinational simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hcd_ManSimulateSimple( Hcd_Man_t * p )
{
    Gia_Obj_t * pObj;
    unsigned Res0, Res1;
    int i;
    Gia_ManForEachAnd( p->pGia, pObj, i )
    {
        Res0 = Hcd_ObjSim( p, Gia_ObjFaninId0(pObj, i) );
        Res1 = Hcd_ObjSim( p, Gia_ObjFaninId1(pObj, i) );
        Hcd_ObjSetSim( p, i, (Gia_ObjFaninC0(pObj)? ~Res0: Res0) &
                             (Gia_ObjFaninC1(pObj)? ~Res1: Res1) );
    }
}


/**Function*************************************************************

  Synopsis    [Resimulate and refine one equivalence class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Gia_Resimulate_rec( Hcd_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj;
    unsigned Res0, Res1;
    if ( Gia_ObjIsTravIdCurrentId( p->pGia, iObj ) )
        return Hcd_ObjSim( p, iObj );
    Gia_ObjSetTravIdCurrentId( p->pGia, iObj );
    pObj = Gia_ManObj(p->pGia, iObj);
    if ( Gia_ObjIsCi(pObj) )
        return Hcd_ObjSim( p, iObj );
    assert( Gia_ObjIsAnd(pObj) );
    Res0 = Gia_Resimulate_rec( p, Gia_ObjFaninId0(pObj, iObj) );
    Res1 = Gia_Resimulate_rec( p, Gia_ObjFaninId1(pObj, iObj) );
    return Hcd_ObjSetSim( p, iObj, (Gia_ObjFaninC0(pObj)? ~Res0: Res0) &
                                   (Gia_ObjFaninC1(pObj)? ~Res1: Res1) );
}

/**Function*************************************************************

  Synopsis    [Resimulate and refine one equivalence class.]

  Description [Assumes that the counter-example is assigned at the PIs.
  The counter-example should have the first bit set to 0 at each PI.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ResimulateAndRefine( Hcd_Man_t * p, int i )
{
    int RetValue, iObj;
    Gia_ManIncrementTravId( p->pGia );
    Gia_ClassForEachObj( p->pGia, i, iObj )
        Gia_Resimulate_rec( p, iObj );
    RetValue = Hcd_ManClassRefineOne( p, i );
    if ( RetValue == 0 )
        printf( "!!! no refinement !!!\n" );
//    assert( RetValue );
}

/**Function*************************************************************

  Synopsis    [Returns temporary representative of the node.]

  Description [The temp repr is the first node among the nodes in the class that
  (a) precedes the given node, and (b) whose level is lower than the given node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Gia_ObjTempRepr( Gia_Man_t * p, int i, int Level )
{
    int iRepr, iMember;
    iRepr = Gia_ObjRepr( p, i ); 
    if ( !Gia_ObjProved(p, i) )
        return NULL;
    if ( Gia_ObjFailed(p, i) )
        return NULL;
    if ( iRepr == GIA_VOID )
        return NULL;
    if ( iRepr == 0 )
        return Gia_ManConst0( p );
//    if ( p->pLevels[iRepr] < Level )
//        return Gia_ManObj( p, iRepr );
    Gia_ClassForEachObj( p, iRepr, iMember )
    {
        if ( Gia_ObjFailed(p, iMember) )
            continue;
        if ( iMember >= i )
            return NULL;
        if ( Gia_ObjLevelId(p, iMember) < Level )
            return Gia_ManObj( p, iMember );
    }
    assert( 0 );
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Generates reduced AIG for the given level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_GenerateReducedLevel( Gia_Man_t * p, int Level, Vec_Ptr_t ** pvRoots )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj, * pRepr;
    Vec_Ptr_t * vRoots;
    int i;
    vRoots = Vec_PtrAlloc( 100 );
    // copy unmarked nodes
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Gia_ObjLevelId(p, i) > Level )
            continue;
        if ( Gia_ObjLevelId(p, i) == Level )
            Vec_PtrPush( vRoots, pObj );
        if ( Gia_ObjLevelId(p, i) < Level && (pRepr = Gia_ObjTempRepr(p, i, Level)) )
        {
//            printf( "Substituting %d <--- %d\n", Gia_ObjId(p, pRepr), Gia_ObjId(p, pObj) ); 
            assert( pRepr < pObj );
            pObj->Value  = Abc_LitNotCond( pRepr->Value, Gia_ObjPhase(pRepr) ^ Gia_ObjPhase(pObj) );
            continue;
        }
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    }
    *pvRoots = vRoots;
    // required by SAT solving
    Gia_ManCreateRefs( pNew );
    Gia_ManFillValue( pNew );
    Gia_ManIncrementTravId( pNew ); // needed for MiniSat to record cexes
//    Gia_ManSetPhase( pNew ); // needed if MiniSat is using polarity -- should not be enabled for TAS because fPhase is used to label
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Collects relevant classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Gia_CollectRelatedClasses( Gia_Man_t * pGia, Vec_Ptr_t * vRoots )
{
    Vec_Ptr_t * vClasses;
    Gia_Obj_t * pRoot, * pRepr;
    int i;
    vClasses = Vec_PtrAlloc( 100 );
    Gia_ManConst0( pGia )->fMark0 = 1;
    Vec_PtrForEachEntry( Gia_Obj_t *, vRoots, pRoot, i )
    {
        pRepr = Gia_ObjReprObj( pGia, Gia_ObjId(pGia, pRoot) );
        if ( pRepr == NULL || pRepr->fMark0 )
            continue;
        pRepr->fMark0 = 1;
        Vec_PtrPush( vClasses, pRepr );
    }
    Gia_ManConst0( pGia )->fMark0 = 0;
    Vec_PtrForEachEntry( Gia_Obj_t *, vClasses, pRepr, i )
        pRepr->fMark0 = 0;
    return vClasses;
}

/**Function*************************************************************

  Synopsis    [Collects class members.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Obj_t * Gia_CollectClassMembers( Gia_Man_t * p, Gia_Obj_t * pRepr, Vec_Ptr_t * vMembers, int Level )
{
    Gia_Obj_t * pTempRepr = NULL;
    int iRepr, iMember;
    iRepr = Gia_ObjId( p, pRepr );
    Vec_PtrClear( vMembers );
    Gia_ClassForEachObj( p, iRepr, iMember )
    {
        if ( Gia_ObjLevelId(p, iMember) == Level )
            Vec_PtrPush( vMembers, Gia_ManObj( p, iMember ) );
        if ( pTempRepr == NULL && Gia_ObjLevelId(p, iMember) < Level )
            pTempRepr = Gia_ManObj( p, iMember );
    }
    return pTempRepr;
}


/**Function*************************************************************

  Synopsis    [Packs patterns into array of simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

*************************************`**********************************/
int Gia_GiarfStorePatternTry( Vec_Ptr_t * vInfo, Vec_Ptr_t * vPres, int iBit, int * pLits, int nLits )
{
    unsigned * pInfo, * pPres;
    int i;
    for ( i = 0; i < nLits; i++ )
    {
        pInfo = (unsigned *)Vec_PtrEntry(vInfo, Abc_Lit2Var(pLits[i]));
        pPres = (unsigned *)Vec_PtrEntry(vPres, Abc_Lit2Var(pLits[i]));
        if ( Abc_InfoHasBit( pPres, iBit ) && 
             Abc_InfoHasBit( pInfo, iBit ) == Abc_LitIsCompl(pLits[i]) )
             return 0;
    }
    for ( i = 0; i < nLits; i++ )
    {
        pInfo = (unsigned *)Vec_PtrEntry(vInfo, Abc_Lit2Var(pLits[i]));
        pPres = (unsigned *)Vec_PtrEntry(vPres, Abc_Lit2Var(pLits[i]));
        Abc_InfoSetBit( pPres, iBit );
        if ( Abc_InfoHasBit( pInfo, iBit ) == Abc_LitIsCompl(pLits[i]) )
            Abc_InfoXorBit( pInfo, iBit );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Procedure to test the new SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_GiarfStorePattern( Vec_Ptr_t * vSimInfo, Vec_Ptr_t * vPres, Vec_Int_t * vCex )
{
    int k;
    for ( k = 1; k < 32; k++ )
        if ( Gia_GiarfStorePatternTry( vSimInfo, vPres, k, (int *)Vec_IntArray(vCex), Vec_IntSize(vCex) ) )
            break;
    return (int)(k < 32);
}

/**Function*************************************************************

  Synopsis    [Inserts pattern into simulation info for the PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_GiarfInsertPattern( Hcd_Man_t * p, Vec_Int_t * vCex, int k )
{
    Gia_Obj_t * pObj;
    unsigned * pInfo;
    int Lit, i;
    Vec_IntForEachEntry( vCex, Lit, i )
    {
        pObj = Gia_ManCi( p->pGia, Abc_Lit2Var(Lit) );
        pInfo = Hcd_ObjSimP( p, Gia_ObjId( p->pGia, pObj ) );
        if ( Abc_InfoHasBit( pInfo, k ) == Abc_LitIsCompl(Lit) )
            Abc_InfoXorBit( pInfo, k );
    }
}

/**Function*************************************************************

  Synopsis    [Inserts pattern into simulation info for the PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_GiarfPrintClasses( Gia_Man_t * pGia )
{
    int nFails  = 0;
    int nProves = 0;
    int nTotal  = 0;
    int nBoth   = 0;
    int i;
    for ( i = 0; i < Gia_ManObjNum(pGia); i++ )
    {
        nFails  += Gia_ObjFailed(pGia, i);
        nProves += Gia_ObjProved(pGia, i);
        nTotal  += Gia_ObjReprObj(pGia, i) != NULL;
        nBoth   += Gia_ObjFailed(pGia, i) && Gia_ObjProved(pGia, i);
    }
    printf( "nFails = %7d.  nProves = %7d.  nBoth = %7d.  nTotal = %7d.\n", 
        nFails, nProves, nBoth, nTotal );
}

ABC_NAMESPACE_IMPL_END

#include "proof/cec/cecInt.h"

ABC_NAMESPACE_IMPL_START


/**Function*************************************************************

  Synopsis    [Performs computation of AIGs with choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ComputeEquivalencesLevel( Hcd_Man_t * p, Gia_Man_t * pGiaLev, Vec_Ptr_t * vOldRoots, int Level, int fUseMiniSat )
{
    int fUse2Solver = 0;
    Cec_ManSat_t * pSat;
    Cec_ParSat_t Pars;
    Tas_Man_t * pTas;
    Vec_Int_t * vCex;
    Vec_Ptr_t * vClasses, * vMembers, * vOldRootsNew;
    Gia_Obj_t * pRoot, * pMember, * pMemberPrev, * pRepr, * pTempRepr;
    int i, k, nIter, iRoot, iRootNew, iMember, iMemberPrev, status, fOneFailed;//, iRepr;//, fTwoMember;
    int nSaved = 0, nRecords = 0, nUndec = 0, nClassRefs = 0, nTsat = 0, nMiniSat = 0;
    clock_t clk, timeTsat = 0, timeMiniSat = 0, timeSim = 0, timeTotal = clock();
    if ( Vec_PtrSize(vOldRoots) == 0 )
        return 0;
    // start SAT solvers
    Cec_ManSatSetDefaultParams( &Pars );
    Pars.fPolarFlip = 0;
    Pars.nBTLimit   = p->nBTLimit;
    pSat = Cec_ManSatCreate( pGiaLev, &Pars );
    pTas = Tas_ManAlloc( pGiaLev, p->nBTLimit );
    if ( fUseMiniSat )
        vCex = Cec_ManSatReadCex( pSat );
    else
        vCex = Tas_ReadModel( pTas );
    vMembers = Vec_PtrAlloc( 100 );
    Vec_PtrCleanSimInfo( p->vSimPres, 0, 1 );
    // resolve constants
    Vec_PtrForEachEntry( Gia_Obj_t *, vOldRoots, pRoot, i )
    {
        iRoot = Gia_ObjId( p->pGia, pRoot );
        if ( !Gia_ObjIsConst( p->pGia, iRoot ) )
            continue;
        iRootNew = Abc_LitNotCond( pRoot->Value, pRoot->fPhase );
        assert( iRootNew != 1 );
        if ( fUse2Solver )
        {
            nTsat++;
           clk = clock();
            status = Tas_ManSolve( pTas, Gia_ObjFromLit(pGiaLev, iRootNew), NULL );
           timeTsat += clock() - clk;
            if ( status == -1 )
            {
                nMiniSat++;
               clk = clock();
                status = Cec_ManSatCheckNode( pSat, Gia_ObjFromLit(pGiaLev, iRootNew) );
               timeMiniSat += clock() - clk;
                if ( status == 0 )
                {
                    Cec_ManSavePattern( pSat, Gia_ObjFromLit(pGiaLev, iRootNew), NULL );
                    vCex = Cec_ManSatReadCex( pSat );
                }
            }
            else if ( status == 0 )
                vCex = Tas_ReadModel( pTas );
        }
        else if ( fUseMiniSat )
        {
            nMiniSat++;
           clk = clock();
            status = Cec_ManSatCheckNode( pSat, Gia_ObjFromLit(pGiaLev, iRootNew) );
           timeMiniSat += clock() - clk;
            if ( status == 0 )
                Cec_ManSavePattern( pSat, Gia_ObjFromLit(pGiaLev, iRootNew), NULL );
        }
        else
        {
            nTsat++;
           clk = clock();
            status = Tas_ManSolve( pTas, Gia_ObjFromLit(pGiaLev, iRootNew), NULL );
           timeTsat += clock() - clk;
        }
        if ( status == -1 ) // undec
        {
//            Gia_ObjSetFailed( p->pGia, iRoot );
            nUndec++;
//            Hcd_ManClassClassRemoveOne( p, iRoot );
            Gia_ObjSetFailed( p->pGia, iRoot );
        }
        else if ( status == 1 ) // unsat
        {
            Gia_ObjSetProved( p->pGia, iRoot );
//            printf( "proved constant %d\n", iRoot );
        }
        else  // sat
        {
//            printf( "Disproved constant %d\n", iRoot );
            Gia_ObjUnsetRepr( p->pGia, iRoot ); // do we need this?
            nRecords++;
            nSaved += Gia_GiarfStorePattern( p->vSimInfo, p->vSimPres, vCex );
        }
    }

    vClasses = Vec_PtrAlloc( 100 );
    vOldRootsNew = Vec_PtrAlloc( 100 );
    for ( nIter = 0; Vec_PtrSize(vOldRoots) > 0; nIter++ )
    {
//        printf( "Iter = %d  (Size = %d)\n", nIter, Vec_PtrSize(vOldRoots) );
        // resolve equivalences 
        Vec_PtrClear( vClasses );
        Vec_PtrClear( vOldRootsNew );
        Gia_ManConst0( p->pGia )->fMark0 = 1;
        Vec_PtrForEachEntry( Gia_Obj_t *, vOldRoots, pRoot, i )
        {
            iRoot = Gia_ObjId( p->pGia, pRoot );
            if ( Gia_ObjIsHead( p->pGia, iRoot ) )
                pRepr = pRoot;
            else if ( Gia_ObjIsClass( p->pGia, iRoot ) )
                pRepr = Gia_ObjReprObj( p->pGia, iRoot );
            else
                continue;
            if ( pRepr->fMark0 )
                continue;
            pRepr->fMark0 = 1;
            Vec_PtrPush( vClasses, pRepr );
//            iRepr = Gia_ObjId( p->pGia, pRepr );
//            fTwoMember = Gia_ClassIsPair(p->pGia, iRepr)
            // derive temp repr and members on this level
            pTempRepr = Gia_CollectClassMembers( p->pGia, pRepr, vMembers, Level );
            if ( pTempRepr )
                Vec_PtrPush( vMembers, pTempRepr );
            if ( Vec_PtrSize(vMembers) < 2 )
                continue;
            // try proving the members
            fOneFailed = 0;
            pMemberPrev = (Gia_Obj_t *)Vec_PtrEntryLast( vMembers );
            Vec_PtrForEachEntry( Gia_Obj_t *, vMembers, pMember, k )
            {
                iMemberPrev = Abc_LitNotCond( pMemberPrev->Value,  pMemberPrev->fPhase ); 
                iMember     = Abc_LitNotCond( pMember->Value,     !pMember->fPhase ); 
                assert( iMemberPrev != iMember );
                if ( fUse2Solver )
                {
                    nTsat++;
                   clk = clock();
                    status = Tas_ManSolve( pTas, Gia_ObjFromLit(pGiaLev, iMemberPrev), Gia_ObjFromLit(pGiaLev, iMember) );
                   timeTsat += clock() - clk;
                    if ( status == -1 )
                    {
                        nMiniSat++;
                       clk = clock();
                        status = Cec_ManSatCheckNodeTwo( pSat, Gia_ObjFromLit(pGiaLev, iMemberPrev), Gia_ObjFromLit(pGiaLev, iMember) );
                       timeMiniSat += clock() - clk;
                        if ( status == 0 )
                        {
                            Cec_ManSavePattern( pSat, Gia_ObjFromLit(pGiaLev, iMemberPrev), Gia_ObjFromLit(pGiaLev, iMember) );
                            vCex = Cec_ManSatReadCex( pSat );
                        }
                    }
                    else if ( status == 0 )
                        vCex = Tas_ReadModel( pTas );
                }
                else if ( fUseMiniSat )
                {
                    nMiniSat++;
                   clk = clock();
                    status = Cec_ManSatCheckNodeTwo( pSat, Gia_ObjFromLit(pGiaLev, iMemberPrev), Gia_ObjFromLit(pGiaLev, iMember) );
                   timeMiniSat += clock() - clk;
                    if ( status == 0 )
                        Cec_ManSavePattern( pSat, Gia_ObjFromLit(pGiaLev, iMemberPrev), Gia_ObjFromLit(pGiaLev, iMember) );
                }
                else
                {
                    nTsat++;
                   clk = clock();
                    status = Tas_ManSolve( pTas, Gia_ObjFromLit(pGiaLev, iMemberPrev), Gia_ObjFromLit(pGiaLev, iMember) );
                   timeTsat += clock() - clk;
                }
                if ( status == -1 ) // undec
                {
//                    Gia_ObjSetFailed( p->pGia, iRoot );
                    nUndec++;
                    if ( Gia_ObjLevel(p->pGia, pMemberPrev) > Gia_ObjLevel(p->pGia, pMember) )
                    {
//                        Hcd_ManClassClassRemoveOne( p, Gia_ObjId(p->pGia, pMemberPrev) );
                        Gia_ObjSetFailed( p->pGia, Gia_ObjId(p->pGia, pMemberPrev) );
                        Gia_ObjSetFailed( p->pGia, Gia_ObjId(p->pGia, pMember) );
                    }
                    else
                    {
//                        Hcd_ManClassClassRemoveOne( p, Gia_ObjId(p->pGia, pMember) );
                        Gia_ObjSetFailed( p->pGia, Gia_ObjId(p->pGia, pMemberPrev) );
                        Gia_ObjSetFailed( p->pGia, Gia_ObjId(p->pGia, pMember) );
                    }
                }
                else if ( status == 1 ) // unsat
                {
//                    Gia_ObjSetProved( p->pGia, iRoot );
                }
                else  // sat
                {
//                    iRepr = Gia_ObjId( p->pGia, pRepr );
//                    if ( Gia_ClassIsPair(p->pGia, iRepr) )
//                        Gia_ClassUndoPair(p->pGia, iRepr);
//                    else
                    {
                        fOneFailed = 1;
                        nRecords++;
                        nSaved += Gia_GiarfStorePattern( p->vSimInfo, p->vSimPres, vCex );
                        Gia_GiarfInsertPattern( p, vCex, (k % 31) + 1 );
                    }
                }
                pMemberPrev = pMember;
//                if ( fOneFailed )
//                    k += Vec_PtrSize(vMembers) / 4;
            }
            // if fail, quit this class
            if ( fOneFailed )
            {
                nClassRefs++;
                Vec_PtrForEachEntry( Gia_Obj_t *, vMembers, pMember, k )
                    if ( pMember != pTempRepr && !Gia_ObjFailed(p->pGia, Gia_ObjId(p->pGia, pMember)) )
                        Vec_PtrPush( vOldRootsNew, pMember );
                clk = clock();
                Gia_ResimulateAndRefine( p, Gia_ObjId(p->pGia, pRepr) );
                timeSim += clock() - clk;
            }
            else
            {
                Vec_PtrForEachEntry( Gia_Obj_t *, vMembers, pMember, k )
                    Gia_ObjSetProved( p->pGia, Gia_ObjId(p->pGia, pMember) );
/*
//            }
//            else
//            {
                printf( "Proved equivalent: " );
                Vec_PtrForEachEntry( Gia_Obj_t *, vMembers, pMember, k )
                    printf( "%d(L=%d)  ", Gia_ObjId(p->pGia, pMember), p->pGia->pLevels[Gia_ObjId(p->pGia, pMember)] );
                printf( "\n" );
*/
            }

        }
        Vec_PtrClear( vOldRoots );
        Vec_PtrForEachEntry( Gia_Obj_t *, vOldRootsNew, pMember, i )
            Vec_PtrPush( vOldRoots, pMember );
        // clean up
        Gia_ManConst0( p->pGia )->fMark0 = 0;
        Vec_PtrForEachEntry( Gia_Obj_t *, vClasses, pRepr, i )
            pRepr->fMark0 = 0;
    }
    Vec_PtrFree( vClasses );
    Vec_PtrFree( vOldRootsNew );
    printf( "nSaved = %d   nRecords = %d   nUndec = %d   nClassRefs = %d   nMiniSat = %d   nTas = %d\n", 
        nSaved, nRecords, nUndec, nClassRefs, nMiniSat, nTsat );
    ABC_PRT( "Tas    ",  timeTsat );
    ABC_PRT( "MiniSat",  timeMiniSat );
    ABC_PRT( "Sim    ",  timeSim );
    ABC_PRT( "Total  ",  clock() - timeTotal );

    // resimulate
//    clk = clock();
    Hcd_ManSimulateSimple( p );
    Hcd_ManClassesRefine( p );
//    ABC_PRT( "Simulate/refine", clock() - clk );

    // verify the results
    Vec_PtrFree( vMembers );
    Tas_ManStop( pTas );
    Cec_ManSatStop( pSat );
    return nIter;
}

/**Function*************************************************************

  Synopsis    [Performs computation of AIGs with choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ComputeEquivalences( Gia_Man_t * pGia, int nBTLimit, int fUseMiniSat, int fVerbose )
{
    Hcd_Man_t * p;
    Vec_Ptr_t * vRoots;
    Gia_Man_t * pGiaLev;
    int i, Lev, nLevels, nIters;
    clock_t clk;
    Gia_ManRandom( 1 );
    Gia_ManSetPhase( pGia );
    nLevels = Gia_ManLevelNum( pGia );
    Gia_ManIncrementTravId( pGia );
    // start the manager
    p = Gia_ManEquivStart( pGia, nBTLimit, fVerbose );
    // create trivial classes
    Hcd_ManClassesCreate( p );
    // refine
    for ( i = 0; i < 3; i++ )
    {
        clk = clock();       
        Hcd_ManSimulationInit( p );
        Hcd_ManSimulateSimple( p );
        ABC_PRT( "Sim", clock() - clk );
        clk = clock();
        Hcd_ManClassesRefine( p );
        ABC_PRT( "Ref", clock() - clk );
    }
    // process in the levelized order
    for ( Lev = 1; Lev < nLevels; Lev++ )
    {
        clk = clock();
        printf( "LEVEL %3d  (out of %3d)   ", Lev, nLevels );
        pGiaLev = Gia_GenerateReducedLevel( pGia, Lev, &vRoots );
        nIters = Gia_ComputeEquivalencesLevel( p, pGiaLev, vRoots, Lev, fUseMiniSat );
        Gia_ManStop( pGiaLev );
        Vec_PtrFree( vRoots );
        printf( "Iters = %3d   " );
        ABC_PRT( "Time", clock() - clk );
    }
    Gia_GiarfPrintClasses( pGia );
    // clean up
    Gia_ManEquivStop( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

