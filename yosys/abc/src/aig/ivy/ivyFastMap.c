/**CFile****************************************************************

  FileName    [ivyFastMap.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [Fast FPGA mapping.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyFastMap.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define IVY_INFINITY   10000

typedef struct Ivy_SuppMan_t_ Ivy_SuppMan_t;
struct Ivy_SuppMan_t_ 
{
    int         nLimit;    // the limit on the number of inputs
    int         nObjs;     // the number of entries
    int         nSize;     // size of each entry in bytes
    char *      pMem;      // memory allocated
    Vec_Vec_t * vLuts;     // the array of nodes used in the mapping
};

typedef struct Ivy_Supp_t_ Ivy_Supp_t;
struct Ivy_Supp_t_ 
{
    char        nSize;     // the number of support nodes
    char        fMark;     // multipurpose mask
    char        fMark2;    // multipurpose mask 
    char        fMark3;    // multipurpose mask 
    int         nRefs;     // the number of references
    short       Delay;     // the delay of the node
    short       DelayR;    // the reverse delay of the node
    int         pArray[0]; // the support nodes
};

static inline Ivy_Supp_t * Ivy_ObjSupp( Ivy_Man_t * pAig, Ivy_Obj_t * pObj )      
{ 
    return (Ivy_Supp_t *)(((Ivy_SuppMan_t*)pAig->pData)->pMem + pObj->Id * ((Ivy_SuppMan_t*)pAig->pData)->nSize); 
}
static inline Ivy_Supp_t * Ivy_ObjSuppStart( Ivy_Man_t * pAig, Ivy_Obj_t * pObj ) 
{ 
    Ivy_Supp_t * pSupp;
    pSupp = Ivy_ObjSupp( pAig, pObj );
    pSupp->fMark = 0;
    pSupp->Delay = 0;
    pSupp->nSize = 1;
    pSupp->pArray[0] = pObj->Id;
    return pSupp;
}

static void Ivy_FastMapPrint( Ivy_Man_t * pAig, int Delay, int Area, abctime Time, char * pStr );
static int  Ivy_FastMapDelay( Ivy_Man_t * pAig );
static int  Ivy_FastMapArea( Ivy_Man_t * pAig );
static void Ivy_FastMapNode( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit );
static void Ivy_FastMapNodeArea( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit );
static int  Ivy_FastMapMerge( Ivy_Supp_t * pSupp0, Ivy_Supp_t * pSupp1, Ivy_Supp_t * pSupp, int nLimit );
static void Ivy_FastMapRequired( Ivy_Man_t * pAig, int Delay, int fSetInter );
static void Ivy_FastMapRecover( Ivy_Man_t * pAig, int nLimit );
static int  Ivy_FastMapNodeDelay( Ivy_Man_t * pAig, Ivy_Obj_t * pObj );
static int  Ivy_FastMapNodeAreaRefed( Ivy_Man_t * pAig, Ivy_Obj_t * pObj );
static int  Ivy_FastMapNodeAreaDerefed( Ivy_Man_t * pAig, Ivy_Obj_t * pObj );
static void Ivy_FastMapNodeRecover( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vFrontOld );
static int  Ivy_FastMapNodeRef( Ivy_Man_t * pAig, Ivy_Obj_t * pObj );
static int  Ivy_FastMapNodeDeref( Ivy_Man_t * pAig, Ivy_Obj_t * pObj );


extern abctime s_MappingTime;
extern int s_MappingMem;


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs fast K-LUT mapping of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapPerform( Ivy_Man_t * pAig, int nLimit, int fRecovery, int fVerbose )
{
    Ivy_SuppMan_t * pMan;
    Ivy_Obj_t * pObj;
    int i, Delay, Area;
    abctime clk, clkTotal = Abc_Clock();
    // start the memory for supports
    pMan = ABC_ALLOC( Ivy_SuppMan_t, 1 );
    memset( pMan, 0, sizeof(Ivy_SuppMan_t) );
    pMan->nLimit = nLimit;
    pMan->nObjs  = Ivy_ManObjIdMax(pAig) + 1;
    pMan->nSize  = sizeof(Ivy_Supp_t) + nLimit * sizeof(int);
    pMan->pMem   = (char *)ABC_ALLOC( char, pMan->nObjs * pMan->nSize );
    memset( pMan->pMem, 0, pMan->nObjs * pMan->nSize );
    pMan->vLuts  = Vec_VecAlloc( 100 );
    pAig->pData  = pMan;
clk = Abc_Clock();
    // set the PI mapping
    Ivy_ObjSuppStart( pAig, Ivy_ManConst1(pAig) );
    Ivy_ManForEachPi( pAig, pObj, i )
        Ivy_ObjSuppStart( pAig, pObj );
    // iterate through all nodes in the topological order
    Ivy_ManForEachNode( pAig, pObj, i )
        Ivy_FastMapNode( pAig, pObj, nLimit );
    // find the best arrival time and area
    Delay = Ivy_FastMapDelay( pAig );
    Area = Ivy_FastMapArea(pAig);
    if ( fVerbose )
        Ivy_FastMapPrint( pAig, Delay, Area, Abc_Clock() - clk, "Delay oriented mapping: " );

// 2-1-2 (doing 2-1-2-1-2 improves 0.5%)

    if ( fRecovery )
    {
clk = Abc_Clock();
    Ivy_FastMapRequired( pAig, Delay, 0 );
    // remap the nodes
    Ivy_FastMapRecover( pAig, nLimit );
    Delay = Ivy_FastMapDelay( pAig );
    Area = Ivy_FastMapArea(pAig);
    if ( fVerbose )
        Ivy_FastMapPrint( pAig, Delay, Area, Abc_Clock() - clk, "Area recovery 2       : " );

clk = Abc_Clock();
    Ivy_FastMapRequired( pAig, Delay, 0 );
    // iterate through all nodes in the topological order
    Ivy_ManForEachNode( pAig, pObj, i )
        Ivy_FastMapNodeArea( pAig, pObj, nLimit );
    Delay = Ivy_FastMapDelay( pAig );
    Area = Ivy_FastMapArea(pAig);
    if ( fVerbose )
        Ivy_FastMapPrint( pAig, Delay, Area, Abc_Clock() - clk, "Area recovery 1       : " );

clk = Abc_Clock();
    Ivy_FastMapRequired( pAig, Delay, 0 );
    // remap the nodes
    Ivy_FastMapRecover( pAig, nLimit );
    Delay = Ivy_FastMapDelay( pAig );
    Area = Ivy_FastMapArea(pAig);
    if ( fVerbose )
        Ivy_FastMapPrint( pAig, Delay, Area, Abc_Clock() - clk, "Area recovery 2       : " );
    }


    s_MappingTime = Abc_Clock() - clkTotal;
    s_MappingMem = pMan->nObjs * pMan->nSize;
/*
    {
        Vec_Ptr_t * vNodes;
        vNodes = Vec_PtrAlloc( 100 );
        Vec_VecForEachEntry( Ivy_Obj_t *, pMan->vLuts, pObj, i, k )
            Vec_PtrPush( vNodes, pObj );
        Ivy_ManShow( pAig, 0, vNodes );
        Vec_PtrFree( vNodes );
    }
*/
}

/**Function*************************************************************

  Synopsis    [Cleans memory used for decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapStop( Ivy_Man_t * pAig )
{
    Ivy_SuppMan_t * p = (Ivy_SuppMan_t *)pAig->pData;
    Vec_VecFree( p->vLuts );
    ABC_FREE( p->pMem );
    ABC_FREE( p );
    pAig->pData = NULL;
}

/**Function*************************************************************

  Synopsis    [Prints statistics.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapPrint( Ivy_Man_t * pAig, int Delay, int Area, abctime Time, char * pStr )
{
    printf( "%s : Delay = %3d. Area = %6d. ", pStr, Delay, Area );
    ABC_PRT( "Time", Time );
}

/**Function*************************************************************

  Synopsis    [Computes delay after LUT mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FastMapDelay( Ivy_Man_t * pAig )
{
    Ivy_Supp_t * pSupp;
    Ivy_Obj_t * pObj;
    int i, DelayMax = 0;
    Ivy_ManForEachPo( pAig, pObj, i )
    {
        pObj = Ivy_ObjFanin0(pObj);
        if ( !Ivy_ObjIsNode(pObj) )
            continue;
        pSupp = Ivy_ObjSupp( pAig, pObj );
        if ( DelayMax < pSupp->Delay )
            DelayMax = pSupp->Delay;
    }
    return DelayMax;
}

/**Function*************************************************************

  Synopsis    [Computes area after mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FastMapArea_rec( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, Vec_Vec_t * vLuts )
{
    Ivy_Supp_t * pSupp;
    int i, Counter;
    pSupp = Ivy_ObjSupp( pAig, pObj );
    // skip visited nodes and PIs
    if ( pSupp->fMark || pSupp->nSize == 1 )
        return 0;
    pSupp->fMark = 1;
    // compute the area of this node
    Counter = 0;
    for ( i = 0; i < pSupp->nSize; i++ )
        Counter += Ivy_FastMapArea_rec( pAig, Ivy_ManObj(pAig, pSupp->pArray[i]), vLuts );
    // add the node to the array of LUTs
    Vec_VecPush( vLuts, pSupp->Delay, pObj );
    return 1 + Counter;
}

/**Function*************************************************************

  Synopsis    [Computes area after mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FastMapArea( Ivy_Man_t * pAig )
{
    Vec_Vec_t * vLuts;
    Ivy_Obj_t * pObj;
    int i, Counter = 0;
    // get the array to store the nodes
    vLuts = ((Ivy_SuppMan_t *)pAig->pData)->vLuts;
    Vec_VecClear( vLuts );
    // explore starting from each node
    Ivy_ManForEachPo( pAig, pObj, i )
        Counter += Ivy_FastMapArea_rec( pAig, Ivy_ObjFanin0(pObj), vLuts );
    // clean the marks
    Ivy_ManForEachNode( pAig, pObj, i )
        Ivy_ObjSupp( pAig, pObj )->fMark = 0;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Performs fast mapping for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Ivy_ObjIsNodeInt1( Ivy_Obj_t * pObj )
{
    return Ivy_ObjIsNode(pObj) && Ivy_ObjRefs(pObj) == 1;
}

/**Function*************************************************************

  Synopsis    [Performs fast mapping for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Ivy_ObjIsNodeInt2( Ivy_Obj_t * pObj )
{
    return Ivy_ObjIsNode(pObj) && Ivy_ObjRefs(pObj) <= 2;
}

/**Function*************************************************************

  Synopsis    [Performs fast mapping for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntRemoveDup( int * pArray, int nSize )
{
    int i, k;
    if ( nSize < 2 )
        return nSize;
    for ( i = k = 1; i < nSize; i++ )
        if ( pArray[i] != pArray[i-1] )
            pArray[k++] = pArray[i];
    return k;
}

/**Function*************************************************************

  Synopsis    [Performs fast mapping for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapNodeArea2( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit )
{
    static int Store[32], StoreSize;
    static char Supp0[16], Supp1[16];
    static Ivy_Supp_t * pTemp0 = (Ivy_Supp_t *)Supp0;
    static Ivy_Supp_t * pTemp1 = (Ivy_Supp_t *)Supp1;
    Ivy_Obj_t * pFanin0, * pFanin1;
    Ivy_Supp_t * pSupp0, * pSupp1, * pSupp;
    int RetValue, DelayOld;
    assert( nLimit <= 32 );
    assert( Ivy_ObjIsNode(pObj) );
    // get the fanins
    pFanin0 = Ivy_ObjFanin0(pObj);
    pFanin1 = Ivy_ObjFanin1(pObj);
    // get the supports
    pSupp0 = Ivy_ObjSupp( pAig, pFanin0 );
    pSupp1 = Ivy_ObjSupp( pAig, pFanin1 );
    pSupp  = Ivy_ObjSupp( pAig, pObj );
    assert( pSupp->fMark == 0 );
    // get the old delay of the node
    DelayOld = Ivy_FastMapNodeDelay(pAig, pObj);
    assert( DelayOld <= pSupp->DelayR );
    // copy the current cut
    memcpy( Store, pSupp->pArray, sizeof(int) * pSupp->nSize );
    StoreSize = pSupp->nSize;
    // get the fanin support
    if ( Ivy_ObjRefs(pFanin0) > 1 && pSupp0->Delay < pSupp->DelayR )
    {
        pSupp0 = pTemp0;
        pSupp0->nSize = 1;
        pSupp0->pArray[0] = Ivy_ObjFaninId0(pObj);
    }
    // get the fanin support
    if ( Ivy_ObjRefs(pFanin1) > 1 && pSupp1->Delay < pSupp->DelayR )
    {
        pSupp1 = pTemp1;
        pSupp1->nSize = 1;
        pSupp1->pArray[0] = Ivy_ObjFaninId1(pObj);
    }
    // merge the cuts
    if ( pSupp0->nSize < pSupp1->nSize )
        RetValue = Ivy_FastMapMerge( pSupp1, pSupp0, pSupp, nLimit );
    else
        RetValue = Ivy_FastMapMerge( pSupp0, pSupp1, pSupp, nLimit );
    if ( !RetValue )
    {
        pSupp->nSize = 2;
        pSupp->pArray[0] = Ivy_ObjFaninId0(pObj);
        pSupp->pArray[1] = Ivy_ObjFaninId1(pObj);
    }
    // check the resulting delay
    pSupp->Delay = Ivy_FastMapNodeDelay(pAig, pObj);
    if ( pSupp->Delay > pSupp->DelayR )
    {
        pSupp->nSize = StoreSize;
        memcpy( pSupp->pArray, Store, sizeof(int) * pSupp->nSize );
        pSupp->Delay = DelayOld;
    }
}

/**Function*************************************************************

  Synopsis    [Performs fast mapping for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapNodeArea( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit )
{
    static int Store[32], StoreSize;
    static char Supp0[16], Supp1[16];
    static Ivy_Supp_t * pTemp0 = (Ivy_Supp_t *)Supp0;
    static Ivy_Supp_t * pTemp1 = (Ivy_Supp_t *)Supp1;
    Ivy_Obj_t * pFanin0, * pFanin1;
    Ivy_Supp_t * pSupp0, * pSupp1, * pSupp;
    int RetValue, DelayOld, RefsOld;
    int AreaBef, AreaAft;
    assert( nLimit <= 32 );
    assert( Ivy_ObjIsNode(pObj) );
    // get the fanins
    pFanin0 = Ivy_ObjFanin0(pObj);
    pFanin1 = Ivy_ObjFanin1(pObj);
    // get the supports
    pSupp0 = Ivy_ObjSupp( pAig, pFanin0 );
    pSupp1 = Ivy_ObjSupp( pAig, pFanin1 );
    pSupp  = Ivy_ObjSupp( pAig, pObj );
    assert( pSupp->fMark == 0 );

    // get the area
    if ( pSupp->nRefs == 0 )
        AreaBef = Ivy_FastMapNodeAreaDerefed( pAig, pObj );
    else
        AreaBef = Ivy_FastMapNodeAreaRefed( pAig, pObj );
//    if ( AreaBef == 1 )
//        return;

    // deref the cut if the node is refed
    if ( pSupp->nRefs != 0 )
        Ivy_FastMapNodeDeref( pAig, pObj );

    // get the old delay of the node
    DelayOld = Ivy_FastMapNodeDelay(pAig, pObj);
    assert( DelayOld <= pSupp->DelayR );
    // copy the current cut
    memcpy( Store, pSupp->pArray, sizeof(int) * pSupp->nSize );
    StoreSize = pSupp->nSize;
    // get the fanin support
    if ( Ivy_ObjRefs(pFanin0) > 2 && pSupp0->Delay < pSupp->DelayR )
//    if ( pSupp0->nRefs > 0 && pSupp0->Delay < pSupp->DelayR ) // this leads to 2% worse results
    {
        pSupp0 = pTemp0;
        pSupp0->nSize = 1;
        pSupp0->pArray[0] = Ivy_ObjFaninId0(pObj);
    }
    // get the fanin support
    if ( Ivy_ObjRefs(pFanin1) > 2 && pSupp1->Delay < pSupp->DelayR )
//    if ( pSupp1->nRefs > 0 && pSupp1->Delay < pSupp->DelayR )
    {
        pSupp1 = pTemp1;
        pSupp1->nSize = 1;
        pSupp1->pArray[0] = Ivy_ObjFaninId1(pObj);
    }
    // merge the cuts
    if ( pSupp0->nSize < pSupp1->nSize )
        RetValue = Ivy_FastMapMerge( pSupp1, pSupp0, pSupp, nLimit );
    else
        RetValue = Ivy_FastMapMerge( pSupp0, pSupp1, pSupp, nLimit );
    if ( !RetValue )
    {
        pSupp->nSize = 2;
        pSupp->pArray[0] = Ivy_ObjFaninId0(pObj);
        pSupp->pArray[1] = Ivy_ObjFaninId1(pObj);
    }

    // check the resulting delay
    pSupp->Delay = Ivy_FastMapNodeDelay(pAig, pObj);

    RefsOld = pSupp->nRefs; pSupp->nRefs = 0;
    AreaAft = Ivy_FastMapNodeAreaDerefed( pAig, pObj );
    pSupp->nRefs = RefsOld;

    if ( AreaAft > AreaBef || pSupp->Delay > pSupp->DelayR )
//    if ( pSupp->Delay > pSupp->DelayR )
    {
        pSupp->nSize = StoreSize;
        memcpy( pSupp->pArray, Store, sizeof(int) * pSupp->nSize );
        pSupp->Delay = DelayOld;
//        printf( "-" );
    }
//    else
//        printf( "+" );

    if ( pSupp->nRefs != 0 )
        Ivy_FastMapNodeRef( pAig, pObj );
}

/**Function*************************************************************

  Synopsis    [Performs fast mapping for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapNode( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit )
{
    Ivy_Supp_t * pSupp0, * pSupp1, * pSupp;
    int fFaninParam = 2;
    int RetValue;
    assert( Ivy_ObjIsNode(pObj) );
    // get the supports
    pSupp0 = Ivy_ObjSupp( pAig, Ivy_ObjFanin0(pObj) );
    pSupp1 = Ivy_ObjSupp( pAig, Ivy_ObjFanin1(pObj) );
    pSupp  = Ivy_ObjSupp( pAig, pObj );
    pSupp->fMark = 0;
    // get the delays
    if ( pSupp0->Delay == pSupp1->Delay )
        pSupp->Delay = (pSupp0->Delay == 0) ? pSupp0->Delay + 1: pSupp0->Delay;
    else if ( pSupp0->Delay > pSupp1->Delay )
    {
        pSupp->Delay = pSupp0->Delay;
        pSupp1 = Ivy_ObjSupp( pAig, Ivy_ManConst1(pAig) );
        pSupp1->pArray[0] = Ivy_ObjFaninId1(pObj);
    }
    else // if ( pSupp0->Delay < pSupp1->Delay )
    {
        pSupp->Delay = pSupp1->Delay;
        pSupp0 = Ivy_ObjSupp( pAig, Ivy_ManConst1(pAig) );
        pSupp0->pArray[0] = Ivy_ObjFaninId0(pObj);
    }
    // merge the cuts
    if ( pSupp0->nSize < pSupp1->nSize )
        RetValue = Ivy_FastMapMerge( pSupp1, pSupp0, pSupp, nLimit );
    else
        RetValue = Ivy_FastMapMerge( pSupp0, pSupp1, pSupp, nLimit );
    if ( !RetValue )
    {
        pSupp->Delay++;
        if ( fFaninParam == 2 )
        {
            pSupp->nSize = 2;
            pSupp->pArray[0] = Ivy_ObjFaninId0(pObj);
            pSupp->pArray[1] = Ivy_ObjFaninId1(pObj);
        }
        else if ( fFaninParam == 3 )
        {
            Ivy_Obj_t * pFanin0, * pFanin1, * pFaninA, * pFaninB;
            pFanin0 = Ivy_ObjFanin0(pObj);
            pFanin1 = Ivy_ObjFanin1(pObj);
            pSupp->nSize = 0;
            // process the first fanin
            if ( Ivy_ObjIsNodeInt1(pFanin0) )
            {
                pFaninA = Ivy_ObjFanin0(pFanin0);
                pFaninB = Ivy_ObjFanin1(pFanin0);
                if ( Ivy_ObjIsNodeInt1(pFaninA) && Ivy_ObjIsNodeInt1(pFaninB) )
                    pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFanin0);
                else
                {
                    pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFaninA);
                    pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFaninB);
                }
            }
            else
                pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFanin0);
            // process the second fanin
            if ( Ivy_ObjIsNodeInt1(pFanin1) )
            {
                pFaninA = Ivy_ObjFanin0(pFanin1);
                pFaninB = Ivy_ObjFanin1(pFanin1);
                if ( Ivy_ObjIsNodeInt1(pFaninA) && Ivy_ObjIsNodeInt1(pFaninB) )
                    pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFanin1);
                else
                {
                    pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFaninA);
                    pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFaninB);
                }
            }
            else
                pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFanin1);
            // sort the fanins
            Vec_IntSelectSort( pSupp->pArray, pSupp->nSize );
            pSupp->nSize = Vec_IntRemoveDup( pSupp->pArray, pSupp->nSize );
            assert( pSupp->pArray[0] < pSupp->pArray[1] );
        }
        else if ( fFaninParam == 4 )
        {
            Ivy_Obj_t * pFanin0, * pFanin1, * pFaninA, * pFaninB;
            pFanin0 = Ivy_ObjFanin0(pObj);
            pFanin1 = Ivy_ObjFanin1(pObj);
            pSupp->nSize = 0;
            // consider the case when exactly one of them is internal
            if ( Ivy_ObjIsNodeInt1(pFanin0) ^ Ivy_ObjIsNodeInt1(pFanin1) )
            {
                pSupp0 = Ivy_ObjSupp( pAig, Ivy_ObjFanin0(pObj) );
                pSupp1 = Ivy_ObjSupp( pAig, Ivy_ObjFanin1(pObj) );
                if ( Ivy_ObjIsNodeInt1(pFanin0) && pSupp0->nSize < nLimit )
                {
                    pSupp->Delay = IVY_MAX( pSupp0->Delay, pSupp1->Delay + 1 );
                    pSupp1 = Ivy_ObjSupp( pAig, Ivy_ManConst1(pAig) );
                    pSupp1->pArray[0] = Ivy_ObjId(pFanin1);
                    // merge the cuts
                    RetValue = Ivy_FastMapMerge( pSupp0, pSupp1, pSupp, nLimit );
                    assert( RetValue );
                    assert( pSupp->nSize > 1 );
                    return;
                }
                if ( Ivy_ObjIsNodeInt1(pFanin1) && pSupp1->nSize < nLimit )
                {
                    pSupp->Delay = IVY_MAX( pSupp1->Delay, pSupp0->Delay + 1 );
                    pSupp0 = Ivy_ObjSupp( pAig, Ivy_ManConst1(pAig) );
                    pSupp0->pArray[0] = Ivy_ObjId(pFanin0);
                    // merge the cuts
                    RetValue = Ivy_FastMapMerge( pSupp1, pSupp0, pSupp, nLimit );
                    assert( RetValue );
                    assert( pSupp->nSize > 1 );
                    return;
                }
            }
            // process the first fanin
            if ( Ivy_ObjIsNodeInt1(pFanin0) )
            {
                pFaninA = Ivy_ObjFanin0(pFanin0);
                pFaninB = Ivy_ObjFanin1(pFanin0);
                if ( Ivy_ObjIsNodeInt1(pFaninA) && Ivy_ObjIsNodeInt1(pFaninB) )
                    pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFanin0);
                else
                {
                    pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFaninA);
                    pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFaninB);
                }
            }
            else
                pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFanin0);
            // process the second fanin
            if ( Ivy_ObjIsNodeInt1(pFanin1) )
            {
                pFaninA = Ivy_ObjFanin0(pFanin1);
                pFaninB = Ivy_ObjFanin1(pFanin1);
                if ( Ivy_ObjIsNodeInt1(pFaninA) && Ivy_ObjIsNodeInt1(pFaninB) )
                    pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFanin1);
                else
                {
                    pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFaninA);
                    pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFaninB);
                }
            }
            else
                pSupp->pArray[(int)(pSupp->nSize++)] = Ivy_ObjId(pFanin1);
            // sort the fanins
            Vec_IntSelectSort( pSupp->pArray, pSupp->nSize );
            pSupp->nSize = Vec_IntRemoveDup( pSupp->pArray, pSupp->nSize );
            assert( pSupp->pArray[0] < pSupp->pArray[1] );
            assert( pSupp->nSize > 1 );
        }
    }
    assert( pSupp->Delay > 0 );
}

/**Function*************************************************************

  Synopsis    [Merges two supports]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FastMapMerge( Ivy_Supp_t * pSupp0, Ivy_Supp_t * pSupp1, Ivy_Supp_t * pSupp, int nLimit )
{ 
    int i, k, c;
    assert( pSupp0->nSize >= pSupp1->nSize );
    // the case of the largest cut sizes
    if ( pSupp0->nSize == nLimit && pSupp1->nSize == nLimit )
    {
        for ( i = 0; i < pSupp0->nSize; i++ )
            if ( pSupp0->pArray[i] != pSupp1->pArray[i] )
                return 0;
        for ( i = 0; i < pSupp0->nSize; i++ )
            pSupp->pArray[i] = pSupp0->pArray[i];
        pSupp->nSize = pSupp0->nSize;
        return 1;
    }
    // the case when one of the cuts is the largest
    if ( pSupp0->nSize == nLimit )
    {
        for ( i = 0; i < pSupp1->nSize; i++ )
        {
            for ( k = pSupp0->nSize - 1; k >= 0; k-- )
                if ( pSupp0->pArray[k] == pSupp1->pArray[i] )
                    break;
            if ( k == -1 ) // did not find
                return 0;
        }
        for ( i = 0; i < pSupp0->nSize; i++ )
            pSupp->pArray[i] = pSupp0->pArray[i];
        pSupp->nSize = pSupp0->nSize;
        return 1;
    }

    // compare two cuts with different numbers
    i = k = 0;
    for ( c = 0; c < nLimit; c++ )
    {
        if ( k == pSupp1->nSize )
        {
            if ( i == pSupp0->nSize )
            {
                pSupp->nSize = c;
                return 1;
            }
            pSupp->pArray[c] = pSupp0->pArray[i++];
            continue;
        }
        if ( i == pSupp0->nSize )
        {
            if ( k == pSupp1->nSize )
            {
                pSupp->nSize = c;
                return 1;
            }
            pSupp->pArray[c] = pSupp1->pArray[k++];
            continue;
        }
        if ( pSupp0->pArray[i] < pSupp1->pArray[k] )
        {
            pSupp->pArray[c] = pSupp0->pArray[i++];
            continue;
        }
        if ( pSupp0->pArray[i] > pSupp1->pArray[k] )
        {
            pSupp->pArray[c] = pSupp1->pArray[k++];
            continue;
        }
        pSupp->pArray[c] = pSupp0->pArray[i++]; 
        k++;
    }
    if ( i < pSupp0->nSize || k < pSupp1->nSize )
        return 0;
    pSupp->nSize = c;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Creates integer vector with the support of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapReadSupp( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, Vec_Int_t * vLeaves )
{
    Ivy_Supp_t * pSupp;
    pSupp = Ivy_ObjSupp( pAig, pObj );
    vLeaves->nCap   = 8;
    vLeaves->nSize  = pSupp->nSize;
    vLeaves->pArray = pSupp->pArray;
}

/**Function*************************************************************

  Synopsis    [Sets the required times of the intermediate nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapRequired_rec( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, Ivy_Obj_t * pRoot, int DelayR )
{
    Ivy_Supp_t * pSupp;
    pSupp = Ivy_ObjSupp( pAig, pObj );
    if ( pObj != pRoot && (pSupp->nRefs > 0 || Ivy_ObjIsCi(pObj)) )
        return;
    Ivy_FastMapRequired_rec( pAig, Ivy_ObjFanin0(pObj), pRoot, DelayR );
    Ivy_FastMapRequired_rec( pAig, Ivy_ObjFanin1(pObj), pRoot, DelayR );
//    assert( pObj == pRoot || pSupp->DelayR == IVY_INFINITY );
    pSupp->DelayR = DelayR;
}

/**Function*************************************************************

  Synopsis    [Computes the required times for each node.]

  Description [Sets reference counters for each node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapRequired( Ivy_Man_t * pAig, int Delay, int fSetInter )
{
    Vec_Vec_t * vLuts;
    Vec_Ptr_t * vNodes;
    Ivy_Obj_t * pObj;
    Ivy_Supp_t * pSupp, * pSuppF;
    int i, k, c;
    // clean the required times
    Ivy_ManForEachPi( pAig, pObj, i )
    {
        pSupp = Ivy_ObjSupp( pAig, pObj );
        pSupp->DelayR = IVY_INFINITY;
        pSupp->nRefs = 0;
    }
    Ivy_ManForEachNode( pAig, pObj, i )
    {
        pSupp = Ivy_ObjSupp( pAig, pObj );
        pSupp->DelayR = IVY_INFINITY;
        pSupp->nRefs = 0;
    }
    // set the required times of the POs
    Ivy_ManForEachPo( pAig, pObj, i )
    {
        pSupp = Ivy_ObjSupp( pAig, Ivy_ObjFanin0(pObj) );
        pSupp->DelayR = Delay;
        pSupp->nRefs++;
    }
    // get the levelized nodes used in the mapping
    vLuts = ((Ivy_SuppMan_t *)pAig->pData)->vLuts;
    // propagate the required times
    Vec_VecForEachLevelReverse( vLuts, vNodes, i )
    Vec_PtrForEachEntry( Ivy_Obj_t *, vNodes, pObj, k )
    {
        pSupp = Ivy_ObjSupp( pAig, pObj );
        assert( pSupp->nRefs > 0 );
        for ( c = 0; c < pSupp->nSize; c++ )
        {
            pSuppF = Ivy_ObjSupp( pAig, Ivy_ManObj(pAig, pSupp->pArray[c]) );
            pSuppF->DelayR = IVY_MIN( pSuppF->DelayR, pSupp->DelayR - 1 );
            pSuppF->nRefs++;
        }
    }
/*
    // print out some of the required times
    Ivy_ManForEachPi( pAig, pObj, i )
    {
        pSupp = Ivy_ObjSupp( pAig, pObj );
        printf( "%d ", pSupp->DelayR );
    }
    printf( "\n" );    
*/

    if ( fSetInter )
    {
        // set the required times of the intermediate nodes
        Vec_VecForEachLevelReverse( vLuts, vNodes, i )
        Vec_PtrForEachEntry( Ivy_Obj_t *, vNodes, pObj, k )
        {
            pSupp = Ivy_ObjSupp( pAig, pObj );
            Ivy_FastMapRequired_rec( pAig, pObj, pObj, pSupp->DelayR ); 
        }
        // make sure that all required times are assigned
        Ivy_ManForEachNode( pAig, pObj, i )
        {
            pSupp = Ivy_ObjSupp( pAig, pObj );
            assert( pSupp->DelayR < IVY_INFINITY );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Performs area recovery for each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapRecover( Ivy_Man_t * pAig, int nLimit )
{
    Vec_Ptr_t * vFront, * vFrontOld;
    Ivy_Obj_t * pObj;
    int i;
    vFront = Vec_PtrAlloc( nLimit );
    vFrontOld = Vec_PtrAlloc( nLimit );
    Ivy_ManCleanTravId( pAig );
    // iterate through all nodes in the topological order
    Ivy_ManForEachNode( pAig, pObj, i )
        Ivy_FastMapNodeRecover( pAig, pObj, nLimit, vFront, vFrontOld );
    Vec_PtrFree( vFrontOld );
    Vec_PtrFree( vFront );
}

/**Function*************************************************************

  Synopsis    [Computes the delay of the cut rooted at this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FastMapNodeDelay( Ivy_Man_t * pAig, Ivy_Obj_t * pObj )
{
    Ivy_Supp_t * pSupp, * pSuppF;
    int c, Delay = 0;
    pSupp = Ivy_ObjSupp( pAig, pObj );
    for ( c = 0; c < pSupp->nSize; c++ )
    {
        pSuppF = Ivy_ObjSupp( pAig, Ivy_ManObj(pAig, pSupp->pArray[c]) );
        Delay = IVY_MAX( Delay, pSuppF->Delay );
    }
    return 1 + Delay;
}


/**function*************************************************************

  synopsis    [References the cut.]

  description [This procedure is similar to the procedure NodeReclaim.]
               
  sideeffects []

  seealso     []

***********************************************************************/
int Ivy_FastMapNodeRef( Ivy_Man_t * pAig, Ivy_Obj_t * pObj )
{
    Ivy_Supp_t * pSupp, * pSuppF;
    Ivy_Obj_t * pNodeChild;
    int aArea, i;
    // start the area of this cut
    aArea = 1;
    // go through the children
    pSupp = Ivy_ObjSupp( pAig, pObj );
    assert( pSupp->nSize > 1 );
    for ( i = 0; i < pSupp->nSize; i++ )
    {
        pNodeChild = Ivy_ManObj(pAig, pSupp->pArray[i]);
        pSuppF = Ivy_ObjSupp( pAig, pNodeChild );
        assert( pSuppF->nRefs >= 0 );
        if ( pSuppF->nRefs++ > 0 )  
            continue;
        if ( pSuppF->nSize == 1 ) 
            continue;
        aArea += Ivy_FastMapNodeRef( pAig, pNodeChild );
    }
    return aArea;
}

/**function*************************************************************

  synopsis    [Dereferences the cut.]

  description [This procedure is similar to the procedure NodeRecusiveDeref.]
               
  sideeffects []

  seealso     []

***********************************************************************/
int Ivy_FastMapNodeDeref( Ivy_Man_t * pAig, Ivy_Obj_t * pObj )
{
    Ivy_Supp_t * pSupp, * pSuppF;
    Ivy_Obj_t * pNodeChild;
    int aArea, i;
    // start the area of this cut
    aArea = 1;
    // go through the children
    pSupp = Ivy_ObjSupp( pAig, pObj );
    assert( pSupp->nSize > 1 );
    for ( i = 0; i < pSupp->nSize; i++ )
    {
        pNodeChild = Ivy_ManObj(pAig, pSupp->pArray[i]);
        pSuppF = Ivy_ObjSupp( pAig, pNodeChild );
        assert( pSuppF->nRefs > 0 );
        if ( --pSuppF->nRefs > 0 )  
            continue;
        if ( pSuppF->nSize == 1 ) 
            continue;
        aArea += Ivy_FastMapNodeDeref( pAig, pNodeChild );
    }
    return aArea;
}

/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
int Ivy_FastMapNodeAreaRefed( Ivy_Man_t * pAig, Ivy_Obj_t * pObj )
{
    Ivy_Supp_t * pSupp;
    int aResult, aResult2;
    if ( Ivy_ObjIsCi(pObj) )
        return 0;
    assert( Ivy_ObjIsNode(pObj) );
    pSupp = Ivy_ObjSupp( pAig, pObj );
    assert( pSupp->nRefs > 0 );
    aResult  = Ivy_FastMapNodeDeref( pAig, pObj );
    aResult2 = Ivy_FastMapNodeRef( pAig, pObj );
    assert( aResult == aResult2 );
    return aResult;
}

/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
int Ivy_FastMapNodeAreaDerefed( Ivy_Man_t * pAig, Ivy_Obj_t * pObj )
{
    Ivy_Supp_t * pSupp;
    int aResult, aResult2;
    if ( Ivy_ObjIsCi(pObj) )
        return 0;
    assert( Ivy_ObjIsNode(pObj) );
    pSupp = Ivy_ObjSupp( pAig, pObj );
    assert( pSupp->nRefs == 0 );
    aResult2 = Ivy_FastMapNodeRef( pAig, pObj );
    aResult  = Ivy_FastMapNodeDeref( pAig, pObj );
    assert( aResult == aResult2 );
    return aResult;
}




/**Function*************************************************************

  Synopsis    [Counts the number of nodes with no external fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FastMapCutCost( Ivy_Man_t * pAig, Vec_Ptr_t * vFront )
{
    Ivy_Supp_t * pSuppF;
    Ivy_Obj_t * pFanin;
    int i, Counter = 0;
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pFanin, i )
    {
        pSuppF = Ivy_ObjSupp( pAig, pFanin );
        if ( pSuppF->nRefs == 0 )
            Counter++;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Performs area recovery for each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapMark_rec( Ivy_Man_t * pAig, Ivy_Obj_t * pObj )
{
    if ( Ivy_ObjIsTravIdCurrent(pAig, pObj) )
        return;
    assert( Ivy_ObjIsNode(pObj) );
    Ivy_FastMapMark_rec( pAig, Ivy_ObjFanin0(pObj) );
    Ivy_FastMapMark_rec( pAig, Ivy_ObjFanin1(pObj) );
    Ivy_ObjSetTravIdCurrent(pAig, pObj);
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the number of fanins will grow.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FastMapNodeWillGrow( Ivy_Man_t * pAig, Ivy_Obj_t * pObj )
{
    Ivy_Obj_t * pFanin0, * pFanin1;
    assert( Ivy_ObjIsNode(pObj) );
    pFanin0 = Ivy_ObjFanin0(pObj);
    pFanin1 = Ivy_ObjFanin1(pObj);
    return !Ivy_ObjIsTravIdCurrent(pAig, pFanin0) && !Ivy_ObjIsTravIdCurrent(pAig, pFanin1);
}

/**Function*************************************************************

  Synopsis    [Returns the increase in the number of fanins with no external refs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FastMapNodeFaninCost( Ivy_Man_t * pAig, Ivy_Obj_t * pObj )
{
    Ivy_Supp_t * pSuppF;
    Ivy_Obj_t * pFanin;
    int Counter = 0;
    assert( Ivy_ObjIsNode(pObj) );
    // check if the node has external refs
    pSuppF = Ivy_ObjSupp( pAig, pObj );
    if ( pSuppF->nRefs == 0 )
        Counter--;
    // increment the number of fanins without external refs
    pFanin = Ivy_ObjFanin0(pObj);
    pSuppF = Ivy_ObjSupp( pAig, pFanin );
    if ( !Ivy_ObjIsTravIdCurrent(pAig, pFanin) && pSuppF->nRefs == 0 )
        Counter++;
    // increment the number of fanins without external refs
    pFanin = Ivy_ObjFanin1(pObj);
    pSuppF = Ivy_ObjSupp( pAig, pFanin );
    if ( !Ivy_ObjIsTravIdCurrent(pAig, pFanin) && pSuppF->nRefs == 0 )
        Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Updates the frontier.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapNodeFaninUpdate( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, Vec_Ptr_t * vFront )
{
    Ivy_Obj_t * pFanin;
    assert( Ivy_ObjIsNode(pObj) );
    Vec_PtrRemove( vFront, pObj );
    pFanin = Ivy_ObjFanin0(pObj);
    if ( !Ivy_ObjIsTravIdCurrent(pAig, pFanin) )
    {
        Ivy_ObjSetTravIdCurrent(pAig, pFanin);
        Vec_PtrPush( vFront, pFanin );
    }
    pFanin = Ivy_ObjFanin1(pObj);
    if ( !Ivy_ObjIsTravIdCurrent(pAig, pFanin) )
    {
        Ivy_ObjSetTravIdCurrent(pAig, pFanin);
        Vec_PtrPush( vFront, pFanin );
    }
}

/**Function*************************************************************

  Synopsis    [Compacts the number of external refs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FastMapNodeFaninCompact0( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront )
{
    Ivy_Obj_t * pFanin;
    int i;
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pFanin, i )
    {
        if ( Ivy_ObjIsCi(pFanin) )
            continue;
        if ( Ivy_FastMapNodeWillGrow(pAig, pFanin) )
            continue;
        if ( Ivy_FastMapNodeFaninCost(pAig, pFanin) <= 0 )
        {
            Ivy_FastMapNodeFaninUpdate( pAig, pFanin, vFront );
            return 1;
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compacts the number of external refs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FastMapNodeFaninCompact1( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront )
{
    Ivy_Obj_t * pFanin;
    int i;
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pFanin, i )
    {
        if ( Ivy_ObjIsCi(pFanin) )
            continue;
        if ( Ivy_FastMapNodeFaninCost(pAig, pFanin) < 0 )
        {
            Ivy_FastMapNodeFaninUpdate( pAig, pFanin, vFront );
            return 1;
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compacts the number of external refs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FastMapNodeFaninCompact2( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront )
{
    Ivy_Obj_t * pFanin;
    int i;
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pFanin, i )
    {
        if ( Ivy_ObjIsCi(pFanin) )
            continue;
        if ( Ivy_FastMapNodeFaninCost(pAig, pFanin) <= 0 )
        {
            Ivy_FastMapNodeFaninUpdate( pAig, pFanin, vFront );
            return 1;
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compacts the number of external refs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FastMapNodeFaninCompact_int( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront )
{
    if ( Ivy_FastMapNodeFaninCompact0(pAig, pObj, nLimit, vFront) )
        return 1;
    if (  Vec_PtrSize(vFront) < nLimit && Ivy_FastMapNodeFaninCompact1(pAig, pObj, nLimit, vFront) )
        return 1;
    if ( Vec_PtrSize(vFront) < nLimit && Ivy_FastMapNodeFaninCompact2(pAig, pObj, nLimit, vFront) )
        return 1;
    assert( Vec_PtrSize(vFront) <= nLimit );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compacts the number of external refs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapNodeFaninCompact( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront )
{
    while ( Ivy_FastMapNodeFaninCompact_int( pAig, pObj, nLimit, vFront ) );
}

/**Function*************************************************************

  Synopsis    [Prepares node mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapNodePrepare( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vFrontOld )
{
    Ivy_Supp_t * pSupp;
    Ivy_Obj_t * pFanin;
    int i;
    pSupp = Ivy_ObjSupp( pAig, pObj );
    // expand the cut downwards from the given place
    Vec_PtrClear( vFront );
    Vec_PtrClear( vFrontOld );
    Ivy_ManIncrementTravId( pAig );
    for ( i = 0; i < pSupp->nSize; i++ )
    {
        pFanin = Ivy_ManObj(pAig, pSupp->pArray[i]);
        Vec_PtrPush( vFront, pFanin );
        Vec_PtrPush( vFrontOld, pFanin );
        Ivy_ObjSetTravIdCurrent( pAig, pFanin );
    }
    // mark the nodes in the cone
    Ivy_FastMapMark_rec( pAig, pObj );
}

/**Function*************************************************************

  Synopsis    [Updates the frontier.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapNodeUpdate( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, Vec_Ptr_t * vFront )
{
    Ivy_Supp_t * pSupp;
    Ivy_Obj_t * pFanin;
    int i;
    pSupp = Ivy_ObjSupp( pAig, pObj );
    // deref node's cut
    Ivy_FastMapNodeDeref( pAig, pObj );
    // update the node's cut
    pSupp->nSize = Vec_PtrSize(vFront);
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pFanin, i )
        pSupp->pArray[i] = pFanin->Id;
    // ref the new cut
    Ivy_FastMapNodeRef( pAig, pObj );
}

/**Function*************************************************************

  Synopsis    [Performs area recovery for each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapNodeRecover2( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vFrontOld )
{
    Ivy_Supp_t * pSupp;
    int CostBef, CostAft;
    int AreaBef, AreaAft;
    pSupp = Ivy_ObjSupp( pAig, pObj );
//    if ( pSupp->nRefs == 0 )
//        return;
    if ( pSupp->nRefs == 0 )
        AreaBef = Ivy_FastMapNodeAreaDerefed( pAig, pObj );
    else
        AreaBef = Ivy_FastMapNodeAreaRefed( pAig, pObj );
    // get the area
    if ( AreaBef == 1 )
        return;

    if ( pSupp->nRefs == 0 )
    {
        pSupp->nRefs = 1000000;
        Ivy_FastMapNodeRef( pAig, pObj );
    }
    // the cut is non-trivial
    Ivy_FastMapNodePrepare( pAig, pObj, nLimit, vFront, vFrontOld );
    // iteratively modify the cut
    CostBef = Ivy_FastMapCutCost( pAig, vFront );
    Ivy_FastMapNodeFaninCompact( pAig, pObj, nLimit, vFront );
    CostAft = Ivy_FastMapCutCost( pAig, vFront );
    assert( CostBef >= CostAft );
    // update the node
    Ivy_FastMapNodeUpdate( pAig, pObj, vFront );
    // get the new area
    AreaAft = Ivy_FastMapNodeAreaRefed( pAig, pObj );
    if ( AreaAft > AreaBef )
    {
        Ivy_FastMapNodeUpdate( pAig, pObj, vFrontOld );
        AreaAft = Ivy_FastMapNodeAreaRefed( pAig, pObj );
        assert( AreaAft == AreaBef );
    }
    if ( pSupp->nRefs == 1000000 )
    {
        pSupp->nRefs = 0;
        Ivy_FastMapNodeDeref( pAig, pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Performs area recovery for each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapNodeRecover( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vFrontOld )
{
    Ivy_Supp_t * pSupp;
    int CostBef, CostAft;
    int AreaBef, AreaAft;
    int DelayOld;
    pSupp = Ivy_ObjSupp( pAig, pObj );
    DelayOld = pSupp->Delay = Ivy_FastMapNodeDelay( pAig, pObj );
    assert( pSupp->Delay <= pSupp->DelayR );
    if ( pSupp->nRefs == 0 )
        return;
    // get the area
    AreaBef = Ivy_FastMapNodeAreaRefed( pAig, pObj );
//    if ( AreaBef == 1 )
//        return;
    // the cut is non-trivial
    Ivy_FastMapNodePrepare( pAig, pObj, nLimit, vFront, vFrontOld );
    // iteratively modify the cut
    Ivy_FastMapNodeDeref( pAig, pObj );
    CostBef = Ivy_FastMapCutCost( pAig, vFront );
    Ivy_FastMapNodeFaninCompact( pAig, pObj, nLimit, vFront );
    CostAft = Ivy_FastMapCutCost( pAig, vFront );
    Ivy_FastMapNodeRef( pAig, pObj );
    assert( CostBef >= CostAft );
    // update the node
    Ivy_FastMapNodeUpdate( pAig, pObj, vFront );
    pSupp->Delay = Ivy_FastMapNodeDelay( pAig, pObj );
    // get the new area
    AreaAft = Ivy_FastMapNodeAreaRefed( pAig, pObj );
    if ( AreaAft > AreaBef || pSupp->Delay > pSupp->DelayR )
    {
        Ivy_FastMapNodeUpdate( pAig, pObj, vFrontOld );
        AreaAft = Ivy_FastMapNodeAreaRefed( pAig, pObj );
        assert( AreaAft == AreaBef );
        pSupp->Delay = DelayOld;
    }
}

/**Function*************************************************************

  Synopsis    [Performs area recovery for each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FastMapNodeRecover4( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vFrontOld )
{
    Ivy_Supp_t * pSupp;
    int CostBef, CostAft;
    int AreaBef, AreaAft;
    int DelayOld;
    pSupp = Ivy_ObjSupp( pAig, pObj );
    DelayOld = pSupp->Delay = Ivy_FastMapNodeDelay( pAig, pObj );
    assert( pSupp->Delay <= pSupp->DelayR );
//    if ( pSupp->nRefs == 0 )
//        return;
//    AreaBef = Ivy_FastMapNodeAreaRefed( pAig, pObj );
    // get the area
    if ( pSupp->nRefs == 0 )
        AreaBef = Ivy_FastMapNodeAreaDerefed( pAig, pObj );
    else
        AreaBef = Ivy_FastMapNodeAreaRefed( pAig, pObj );
    if ( AreaBef == 1 )
        return;

    if ( pSupp->nRefs == 0 )
    {
        pSupp->nRefs = 1000000;
        Ivy_FastMapNodeRef( pAig, pObj );
    }
    // the cut is non-trivial
    Ivy_FastMapNodePrepare( pAig, pObj, nLimit, vFront, vFrontOld );
    // iteratively modify the cut
    CostBef = Ivy_FastMapCutCost( pAig, vFront );
    Ivy_FastMapNodeFaninCompact( pAig, pObj, nLimit, vFront );
    CostAft = Ivy_FastMapCutCost( pAig, vFront );
    assert( CostBef >= CostAft );
    // update the node
    Ivy_FastMapNodeUpdate( pAig, pObj, vFront );
    pSupp->Delay = Ivy_FastMapNodeDelay( pAig, pObj );
    // get the new area
    AreaAft = Ivy_FastMapNodeAreaRefed( pAig, pObj );
    if ( AreaAft > AreaBef || pSupp->Delay > pSupp->DelayR )
    {
        Ivy_FastMapNodeUpdate( pAig, pObj, vFrontOld );
        AreaAft = Ivy_FastMapNodeAreaRefed( pAig, pObj );
        assert( AreaAft == AreaBef );
        pSupp->Delay = DelayOld;
    }
    if ( pSupp->nRefs == 1000000 )
    {
        pSupp->nRefs = 0;
        Ivy_FastMapNodeDeref( pAig, pObj );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

