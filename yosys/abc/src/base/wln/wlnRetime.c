/**CFile****************************************************************

  FileName    [wlnRetime.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Word-level network.]

  Synopsis    [Retiming.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 23, 2018.]

  Revision    [$Id: wlnRetime.c,v 1.00 2018/09/23 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wln.h"
#include "misc/vec/vecHsh.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Wln_Ret_t_ Wln_Ret_t;
struct Wln_Ret_t_ 
{
    Wln_Ntk_t *       pNtk;         // static netlist
    Vec_Int_t         vFanins;      // fanins and edge places
    Vec_Int_t         vFanouts;     // fanouts and edge places
    Vec_Int_t         vEdgeLinks;   // edge links
    Vec_Int_t         vFfClasses;   // flop classes
    Vec_Int_t         vNodeDelays;  // object delays
    Vec_Int_t         vPathDelays;  // delays from sources to sinks
    Vec_Int_t         vSources;     // critical sources
    Vec_Int_t         vSinks;       // critical sinks
    Vec_Int_t         vFront;       // retiming frontier
    Vec_Int_t         vMoves;       // retiming moves (paired with delay)
    int               nClasses;     // the number of flop classes
    int               DelayMax;     // critical delay at any time
};

static inline int *   Wln_RetFanins( Wln_Ret_t * p, int i )  { return Vec_IntEntryP( &p->vFanins,  Vec_IntEntry(&p->vFanins, i) );  }
static inline int *   Wln_RetFanouts( Wln_Ret_t * p, int i ) { return Vec_IntEntryP( &p->vFanouts, Vec_IntEntry(&p->vFanouts, i) ); }

#define Wln_RetForEachFanin( p, iObj, iFanin, pLink, i )    \
    for ( i = 0; (i < Wln_ObjFaninNum(p->pNtk, iObj)) &&    \
         (((iFanin) = Wln_RetFanins(p, iObj)[2*i]), 1) &&   \
           ((pLink) = (Wln_RetFanins(p, iObj)+2*i+1)); i++ ) if ( !iFanin || (!Wln_ObjFaninNum(p->pNtk, iFanin) && !Wln_ObjIsCi(p->pNtk, iFanin)) )  {} else

#define Wln_RetForEachFanout( p, iObj, iFanout, pLink, i )  \
    for ( i = 0; (i < Wln_ObjRefs(p->pNtk, iObj)) &&        \
        (((iFanout) = Wln_RetFanouts(p, iObj)[2*i]), 1) &&  \
           ((pLink) = Vec_IntEntryP(&p->vFanins, Wln_RetFanouts(p, iObj)[2*i+1])); i++ ) if ( !iFanout ) {} else

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Printing procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wln_RetPrintObj( Wln_Ret_t * p, int iObj )
{
    int k, iFanin, Type = Wln_ObjType(p->pNtk, iObj), * pLink;
    printf( "Obj %6d : Type = %6s  NameId = %5d  InstId = %5d  Fanins = %d : ", 
        iObj, Abc_OperName(Type), Wln_ObjNameId(p->pNtk, iObj), Wln_ObjInstId(p->pNtk, iObj), Wln_ObjFaninNum(p->pNtk, iObj) );
    Wln_RetForEachFanin( p, iObj, iFanin, pLink, k )
    {
        printf( "%5d ", iFanin );
        if ( !pLink[0] )
            continue;
        printf( "(%d : %d %d) ", pLink[0], 
            Vec_IntEntry(&p->vEdgeLinks, pLink[0]), 
            Vec_IntEntry(&p->vEdgeLinks, pLink[0]+1) );
    }
    printf( "\n" );
}
void Wln_RetPrint( Wln_Ret_t * p, int fVerbose )
{
    int iObj, nCount = 0;
    Wln_NtkForEachObj( p->pNtk, iObj )
        if ( Wln_ObjInstId(p->pNtk, iObj) > 1 )
            nCount++;
    printf( "Total number of objects = %d.  Objects with non-trivial delay = %d.\n", Wln_NtkObjNum(p->pNtk), nCount );
    if ( !fVerbose )
    {
        int nPrints = 0, nLimit = 5;
        printf( "The following %d objects have non-trivial delays:\n", nLimit );
        Wln_NtkForEachObj( p->pNtk, iObj )
        {
            if ( Wln_ObjInstId(p->pNtk, iObj) <= 1 )
                continue;
            Wln_RetPrintObj( p, iObj );
            if ( ++nPrints == nLimit )
                break;
        }
        return;
    }
    printf( "Printing %d objects of network \"%s\":\n", Wln_NtkObjNum(p->pNtk), p->pNtk->pName );
    Wln_NtkForEachObj( p->pNtk, iObj )
        Wln_RetPrintObj( p, iObj );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Retiming manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wln_RetComputeFfClasses( Wln_Ntk_t * pNtk, Vec_Int_t * vClasses )
{
    int i, k, iObj, nClasses;
    Hsh_VecMan_t * p = Hsh_VecManStart( 10 );
    Vec_Int_t * vFlop = Vec_IntAlloc( 6 ); 
    Vec_IntFill( vClasses, Wln_NtkObjNum(pNtk), -1 );
    Wln_NtkForEachFf( pNtk, iObj, i )
    {
        Vec_IntClear( vFlop );
        for ( k = 1; k <= 6; k++ )
            Vec_IntPush( vFlop, Wln_ObjFanin(pNtk, iObj, k) );
        Vec_IntWriteEntry( vClasses, iObj, Hsh_VecManAdd(p, vFlop) );
    }
    nClasses = Hsh_VecSize( p );
    Hsh_VecManStop( p );
    Vec_IntFree( vFlop );
    printf( "Detected %d flops and %d flop classes.\n", Wln_NtkFfNum(pNtk), nClasses );
    return nClasses;
}
Wln_Ret_t * Wln_RetAlloc( Wln_Ntk_t * pNtk )
{
    Wln_Ret_t * p; int k, iObj, iFanin, fFirst = 1;
    Vec_Int_t * vRefsCopy = Vec_IntAlloc(0);
    p = ABC_CALLOC( Wln_Ret_t, 1 );
    p->pNtk = pNtk;
    Wln_NtkCreateRefs( pNtk );
    // print objects without fanout
    Wln_NtkForEachObj( pNtk, iObj )
        if ( Wln_ObjRefs(pNtk, iObj) == 0 && !Wln_ObjIsCio(pNtk, iObj) )
        {
            if ( fFirst )
            {
                fFirst = 0;
                printf( "Objects without fanout:\n" );
            }
            Wln_ObjPrint(pNtk, iObj);
        }
    // start fanin/fanout maps
    Wln_NtkStartFaninMap( pNtk, &p->vFanins, 2 );
    Wln_NtkStartFanoutMap( pNtk, &p->vFanouts, &pNtk->vRefs, 2 );
    ABC_SWAP( Vec_Int_t, *vRefsCopy, pNtk->vRefs );
    Wln_NtkCleanRefs( pNtk );
    Vec_IntGrow( &p->vEdgeLinks, 10*Wln_NtkFfNum(pNtk) );
    Vec_IntPushTwo( &p->vEdgeLinks, -1, -1 );
    Wln_NtkForEachObj( pNtk, iObj )
        Wln_ObjForEachFanin( pNtk, iObj, iFanin, k )
        {
            int * pFanins  = Wln_RetFanins( p, iObj );
            int * pFanouts = Wln_RetFanouts( p, iFanin );
            int Index      = Wln_ObjRefsInc( pNtk, iFanin );
            pFanins[2*k+0]      = iFanin;
            pFanins[2*k+1]      = Wln_ObjIsFf(pNtk, iFanin) ? Vec_IntSize(&p->vEdgeLinks) : 0;
            pFanouts[2*Index+0] = iObj;
            pFanouts[2*Index+1] = Vec_IntEntry(&p->vFanins, iObj) + 2*k + 1;
            if ( Wln_ObjIsFf(pNtk, iFanin) )
                Vec_IntPushTwo( &p->vEdgeLinks, 0, iFanin );
        }
    // double-check the current number of fanouts added
    Wln_NtkForEachObj( pNtk, iObj )
        assert( Wln_ObjRefs(pNtk, iObj) == Vec_IntEntry(vRefsCopy, iObj) );
    Vec_IntFree( vRefsCopy );
    // other data
    p->nClasses = Wln_RetComputeFfClasses( pNtk, &p->vFfClasses );
    //ABC_SWAP( Vec_Int_t, p->vNodeDelays, pNtk->vInstIds );
    Vec_IntAppend( &p->vNodeDelays, &pNtk->vInstIds );
    Vec_IntGrow( &p->vSources, 1000 );
    Vec_IntGrow( &p->vSinks, 1000 );
    Vec_IntGrow( &p->vFront, 1000 );
    Vec_IntGrow( &p->vMoves, 1000 );
    return p;
}
void Wln_RetFree( Wln_Ret_t * p )
{
    ABC_FREE( p->vFanins.pArray );
    ABC_FREE( p->vFanouts.pArray );
    ABC_FREE( p->vEdgeLinks.pArray );
    ABC_FREE( p->vFfClasses.pArray );
    ABC_FREE( p->vNodeDelays.pArray );
    ABC_FREE( p->vPathDelays.pArray );
    ABC_FREE( p->vSources.pArray );
    ABC_FREE( p->vSinks.pArray );
    ABC_FREE( p->vFront.pArray );
    ABC_FREE( p->vMoves.pArray );
    ABC_FREE( p );
}
int Wln_RetMemUsage( Wln_Ret_t * p )
{
    int Mem = sizeof(Wln_Ret_t);
    Mem += 4 * p->vFanins.nCap;
    Mem += 4 * p->vFanouts.nCap;
    Mem += 4 * p->vEdgeLinks.nCap;
    Mem += 4 * p->vFfClasses.nCap;
    Mem += 4 * p->vNodeDelays.nCap;
    Mem += 4 * p->vPathDelays.nCap;
    Mem += 4 * p->vSources.nCap;
    Mem += 4 * p->vSinks.nCap;
    Mem += 4 * p->vFront.nCap;
    Mem += 4 * p->vMoves.nCap;
    return Mem;
}

/**Function*************************************************************

  Synopsis    [Delay propagation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wln_RetMarkChanges_rec( Wln_Ret_t * p, int iObj )
{
    int k, iFanout, * pLink, * pDelay = Vec_IntEntryP( &p->vPathDelays, iObj );
    if ( *pDelay < 0 )
        return;
    *pDelay = -1;
    Wln_RetForEachFanout( p, iObj, iFanout, pLink, k )
        if ( !pLink[0] )
            Wln_RetMarkChanges_rec( p, iFanout );
}
void Wln_RetMarkChanges( Wln_Ret_t * p, Vec_Int_t * vFront )
{
    int i, iObj;
    if ( vFront )
    {
        Vec_IntForEachEntry( vFront, iObj, i )
            Wln_RetMarkChanges_rec( p, iObj );
    }
    else
    {
        Vec_IntFill( &p->vPathDelays, Wln_NtkObjNum(p->pNtk), -1 );
        Wln_NtkForEachCi( p->pNtk, iObj, i )
            Vec_IntWriteEntry( &p->vPathDelays, iObj, 0 );
    }
}
int Wln_RetPropDelay_rec( Wln_Ret_t * p, int iObj )
{
    int k, iFanin, * pLink, * pDelay = Vec_IntEntryP( &p->vPathDelays, iObj );
    if ( *pDelay >= 0 )
        return *pDelay;
    *pDelay = 0;
    Wln_RetForEachFanin( p, iObj, iFanin, pLink, k )
    {
        if ( Wln_ObjIsFf(p->pNtk, iObj) && k > 0 )
            continue;
        if ( pLink[0] )
            *pDelay = Abc_MaxInt(*pDelay, 0);
        else
            *pDelay = Abc_MaxInt(*pDelay, Wln_RetPropDelay_rec(p, iFanin));
    }
    *pDelay += Vec_IntEntry( &p->vNodeDelays, iObj );
    return *pDelay;
}
int Wln_RetPropDelay( Wln_Ret_t * p )
{
    int iObj, DelayMax = 0;
    Vec_IntClear( &p->vSinks );
    Wln_NtkForEachObj( p->pNtk, iObj )
        if ( !Wln_ObjIsCio(p->pNtk, iObj) )
        {
            int Delay = Wln_RetPropDelay_rec(p, iObj);
            if ( DelayMax == Delay )
                Vec_IntPush( &p->vSinks, iObj );
            else if ( DelayMax < Delay )
            {
                DelayMax = Delay;
                Vec_IntFill( &p->vSinks, 1, iObj );
            }
        }

//    Vec_IntForEachEntry( &p->vPathDelays, iObj, i )
//        printf( "Obj = %d.  Delay = %d.\n", i, iObj );
//    printf( "\n" );

//    printf( "Sinks: " );
//    Vec_IntPrint( &p->vSinks );

    return DelayMax;
}

void Wln_RetFindSources_rec( Wln_Ret_t * p, int iObj )
{
    int k, iFanin, * pLink, FaninDelay;
    if ( Wln_ObjIsCi(p->pNtk, iObj) || Wln_ObjCheckTravId(p->pNtk, iObj) )
        return;
    FaninDelay = Vec_IntEntry( &p->vPathDelays, iObj ) - Vec_IntEntry( &p->vNodeDelays, iObj );
    Wln_RetForEachFanin( p, iObj, iFanin, pLink, k )
        if ( !pLink[0] && Vec_IntEntry(&p->vPathDelays, iFanin) == FaninDelay )
            Wln_RetFindSources_rec( p, iFanin );
    if ( FaninDelay == 0 )
        Vec_IntPush( &p->vSources, iObj );
}
void Wln_RetFindSources( Wln_Ret_t * p )
{
    int i, iObj;
    Vec_IntClear( &p->vSources );
    Wln_NtkIncrementTravId( p->pNtk );
    Vec_IntForEachEntry( &p->vSinks, iObj, i )
        Wln_RetFindSources_rec( p, iObj );

//    printf( "Sources: " );
//    Vec_IntPrint( &p->vSources );
}

/**Function*************************************************************

  Synopsis    [Mark paths from PIs to POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wln_RetMarkPaths_rec( Wln_Ntk_t * p, int iObj, int fVerbose )
{
    int k, iFanin, fPrev = 1;
    if ( Wln_ObjIsTravIdPrevious(p, iObj) )
        return 1;
    if ( Wln_ObjIsTravIdCurrent(p, iObj) )
        return 0;
    if ( Wln_ObjIsCio(p, iObj) || Wln_ObjIsFf(p, iObj) )
        return 0;
    Wln_ObjForEachFanin( p, iObj, iFanin, k )
        fPrev &= Wln_RetMarkPaths_rec( p, iFanin, fVerbose );
    if ( fPrev )
    {
        Wln_ObjSetTravIdPrevious( p, iObj );
        if ( Vec_IntEntry(&p->vInstIds, iObj) > 0 )
        {
            if ( fVerbose )
                printf( "Updating delay %5d -> %5d : ", Vec_IntEntry(&p->vInstIds, iObj), 1 );
            if ( fVerbose )
                Wln_ObjPrint( p, iObj );
            Vec_IntWriteEntry( &p->vInstIds, iObj, 1 );
        }
        return 1;
    }
    Wln_ObjSetTravIdCurrent( p, iObj );
    return 0;
}
void Wln_RetMarkPaths( Wln_Ntk_t * p, int fVerbose )
{
    int i, iObj;
    Wln_NtkIncrementTravId( p );
    Wln_NtkIncrementTravId( p );
    Wln_NtkForEachPi( p, iObj, i )
        Wln_ObjSetTravIdPrevious( p, iObj );
    Wln_NtkForEachPo( p, iObj, i )
        Wln_RetMarkPaths_rec( p, Wln_ObjFanin0(p, iObj), fVerbose );
}

/**Function*************************************************************

  Synopsis    [Retimability check.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Wln_RetHeadToTail( Wln_Ret_t * p, int * pHead )
{
    int * pLink;
    assert( pHead[0] );
    pLink = Vec_IntEntryP( &p->vEdgeLinks, pHead[0] );
    if ( pLink[0] == 0 )
        return pHead;
    return Wln_RetHeadToTail( p, pLink );
}

static inline int Wln_RetCheckForwardOne( Wln_Ret_t * p, int iObj )
{
    int k, iFanin, * pLink, iFlop, Class = -1;
    Wln_RetForEachFanin( p, iObj, iFanin, pLink, k )
    {
        if ( Wln_ObjIsFf(p->pNtk, iObj) && k > 0 )
            continue;
        if ( !pLink[0] )
            return 0;
        iFlop = Vec_IntEntry( &p->vEdgeLinks, pLink[0] + 1 );
        assert( Wln_ObjIsFf( p->pNtk, iFlop ) );
        if ( Class == -1 )
            Class = Vec_IntEntry( &p->vFfClasses, iFlop );
        else if ( Class != Vec_IntEntry( &p->vFfClasses, iFlop ) )
            return 0;
    }
    return 1;
}
int Wln_RetCheckForward( Wln_Ret_t * p, Vec_Int_t * vSet )
{
    int i, iObj;
    Vec_IntForEachEntry( vSet, iObj, i )
        if ( !Wln_RetCheckForwardOne( p, iObj ) )
            return 0;
    return 1;
}

static inline int Wln_RetCheckBackwardOne( Wln_Ret_t * p, int iObj )
{
    int k, iFanin, * pLink, iFlop, Class = -1;
    if ( Wln_ObjRefs(p->pNtk, iObj) == 0 )
        return 0;
    Wln_RetForEachFanout( p, iObj, iFanin, pLink, k )
    {
        if ( !pLink[0] )
            return 0;
        pLink = Wln_RetHeadToTail( p, pLink );
        iFlop = Vec_IntEntry( &p->vEdgeLinks, pLink[0] + 1 );
        assert( Wln_ObjIsFf( p->pNtk, iFlop ) );
        if ( Class == -1 )
            Class = Vec_IntEntry( &p->vFfClasses, iFlop );
        else if ( Class != Vec_IntEntry( &p->vFfClasses, iFlop ) )
            return 0;
    }
    return 1;
}
int Wln_RetCheckBackward( Wln_Ret_t * p, Vec_Int_t * vSet )
{
    int i, iObj;
    Vec_IntForEachEntry( vSet, iObj, i )
        if ( !Wln_RetCheckBackwardOne( p, iObj ) )
            return 0;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Moving flops.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wln_RetRemoveOneFanin( Wln_Ret_t * p, int iObj )
{
    int k, iFanin, * pLink, iFlop, iFlop1 = -1;
    int * pFanins  = Wln_RetFanins( p, iObj );
    Wln_RetForEachFanin( p, iObj, iFanin, pLink, k )
    {
        if ( Wln_ObjIsFf(p->pNtk, iObj) && k > 0 )
            continue;
        assert( pLink[0] );
        iFlop = Vec_IntEntry( &p->vEdgeLinks, pLink[0] + 1 );
        pFanins[2*k+1] = Vec_IntEntry( &p->vEdgeLinks, pLink[0] );
        assert( Wln_ObjIsFf( p->pNtk, iFlop ) );
        if ( iFlop1 == -1 )
            iFlop1 = iFlop;
    }
    return iFlop1;
}
int Wln_RetRemoveOneFanout( Wln_Ret_t * p, int iObj )
{
    int k, iFanin, * pLink, iFlop, iFlop1 = -1;
    //int * pFanins  = Wln_RetFanins( p, iObj );
    Wln_RetForEachFanout( p, iObj, iFanin, pLink, k )
    {
        assert( pLink[0] );
        pLink = Wln_RetHeadToTail( p, pLink );
        iFlop = Vec_IntEntry( &p->vEdgeLinks, pLink[0] + 1 );
        pLink[0] = 0;
        assert( Wln_ObjIsFf( p->pNtk, iFlop ) );
        if ( iFlop1 == -1 )
            iFlop1 = iFlop;
    }
    return iFlop1;
}
void Wln_RetInsertOneFanin( Wln_Ret_t * p, int iObj, int iFlop )
{
    int k, iHead, iFanin, * pLink;
    int * pFanins  = Wln_RetFanins( p, iObj );
    assert( Wln_ObjIsFf( p->pNtk, iFlop ) );
    Wln_RetForEachFanin( p, iObj, iFanin, pLink, k )
    {
        if ( Wln_ObjIsFf(p->pNtk, iObj) && k > 0 )
            continue;
        iHead = pFanins[2*k+1];
        pFanins[2*k+1] = Vec_IntSize(&p->vEdgeLinks);
        Vec_IntPushTwo( &p->vEdgeLinks, iHead, iFlop );
    }
}
void Wln_RetInsertOneFanout( Wln_Ret_t * p, int iObj, int iFlop )
{
    int k, iFanin, * pLink;
    assert( Wln_ObjIsFf( p->pNtk, iFlop ) );
    Wln_RetForEachFanout( p, iObj, iFanin, pLink, k )
    {
        if ( pLink[0] )
            pLink = Wln_RetHeadToTail( p, pLink );
        //assert( pLink[0] == 0 );
        pLink[0] = Vec_IntSize(&p->vEdgeLinks);
        Vec_IntPushTwo( &p->vEdgeLinks, 0, iFlop );
    }
}
void Wln_RetRetimeForward( Wln_Ret_t * p, Vec_Int_t * vSet )
{
    int i, iObj, iFlop;
    Vec_IntForEachEntry( vSet, iObj, i )
    {
        iFlop = Wln_RetRemoveOneFanin( p, iObj );
        if ( iFlop == -1 )
            continue;
        Wln_RetInsertOneFanout( p, iObj, iFlop );
    }
}
void Wln_RetRetimeBackward( Wln_Ret_t * p, Vec_Int_t * vSet )
{
    int i, iObj, iFlop;
    Vec_IntForEachEntry( vSet, iObj, i )
    {
        iFlop = Wln_RetRemoveOneFanout( p, iObj );
        if ( iFlop == -1 )
            continue;
        Wln_RetInsertOneFanin( p, iObj, iFlop );
    }
}
void Wln_RetAddToMoves( Wln_Ret_t * p, Vec_Int_t * vSet, int Delay, int fForward, int nMoves, int fSkipSimple, int fVerbose )
{
    int i, iObj;
    if ( vSet == NULL )
    {
        printf( "Move %4d : Recording initial state     (delay = %6d)\n", nMoves, Delay );
        Vec_IntPushTwo( &p->vMoves, Delay, 0 );
        return;
    }
    printf( "Move %4d : Recording %s retiming (delay = %6d) :", nMoves, fForward ? "forward " : "backward", Delay );
    Vec_IntPush( &p->vMoves, Delay );
    Vec_IntForEachEntry( vSet, iObj, i )
    {
        int NameId = Vec_IntEntry( &p->pNtk->vNameIds, iObj );
        if ( fSkipSimple && (Wln_ObjIsFf(p->pNtk, iObj) || Wln_ObjType(p->pNtk, iObj) == ABC_OPER_SLICE || Wln_ObjType(p->pNtk, iObj) == ABC_OPER_CONCAT) )
            continue;
        Vec_IntPush( &p->vMoves, fForward ? -NameId : NameId );
        if ( fVerbose )
            printf( " %d (NameID = %d)  ", fForward ? -iObj : iObj, fForward ? -NameId : NameId );
    }
    Vec_IntPush( &p->vMoves, 0 );
    if ( !fVerbose )
        printf( " %3d retimed objects", Vec_IntSize(vSet) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Retiming computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wln_NtkRetimeCreateDelayInfo( Wln_Ntk_t * pNtk )
{
    int i, iObj;
//    if ( Wln_NtkHasInstId(pNtk) )
//        Vec_IntErase( &pNtk->vInstIds );
    if ( Wln_NtkHasInstId(pNtk) )
    {
        printf( "Using delays given by the user in the input file.\n" );
        Wln_NtkForEachObj( pNtk, iObj )
            if ( !Wln_ObjIsCio(pNtk, iObj) && !Wln_ObjIsConst(pNtk, iObj) && Wln_ObjInstId(pNtk, iObj) == 0 )
                printf( "Warning: Object %d of type %s has zero delay. Retiming will not work correctly.\n", iObj, Abc_OperName(Wln_ObjType(pNtk, iObj)) );
    }
    else
    {
        printf( "The design has no delay information.\n" );
        Wln_NtkCleanInstId(pNtk);
        Wln_NtkForEachObj( pNtk, iObj )
        {
            if ( Wln_ObjIsFf(pNtk, iObj) || Wln_ObjType(pNtk, iObj) == ABC_OPER_SLICE || Wln_ObjType(pNtk, iObj) == ABC_OPER_CONCAT )
                Wln_ObjSetInstId( pNtk, iObj, 1 );
            else if ( !Wln_ObjIsCio(pNtk, iObj) && Wln_ObjFaninNum(pNtk, iObj) > 0 )
                Wln_ObjSetInstId( pNtk, iObj, 10 );
        }
        Wln_NtkForEachCo( pNtk, iObj, i ) 
        {
            if ( Wln_ObjType(pNtk, Wln_ObjFanin0(pNtk, iObj)) != ABC_OPER_LUT )
                Wln_ObjSetInstId( pNtk, Wln_ObjFanin0(pNtk, iObj), 1 );
        }
        printf( "Assuming default delays: 10 units for most nodes and 1 unit for bit-slice, concat, and buffers driving COs.\n" );
    }
}
Vec_Int_t * Wln_NtkRetime_int( Wln_Ntk_t * pNtk, int fSkipSimple, int fVerbose )
{
    Wln_Ret_t * p = Wln_RetAlloc( pNtk );
    Vec_Int_t * vSources = &p->vSources;
    Vec_Int_t * vSinks   = &p->vSinks;
    Vec_Int_t * vFront   = &p->vFront;
    Vec_Int_t * vMoves   = Vec_IntAlloc(0);
    int nMoves = 0, fPrevFwd = 0, fPrevBwd = 0, nCountIncrease = 0;
    int DelayInit = 0, DelayBest = 0, nChange = 0;
    Wln_RetPrint( p, fVerbose );
    Wln_RetMarkChanges( p, NULL );
    p->DelayMax = DelayInit = DelayBest = Wln_RetPropDelay( p );
    Wln_RetFindSources( p );
    Wln_RetAddToMoves( p, NULL, p->DelayMax, 0, nMoves, fSkipSimple, fVerbose );
    while ( Vec_IntSize(vSources) || Vec_IntSize(vSinks) )
    {
        int DelayMaxPrev = p->DelayMax;
        int fForward  = Vec_IntSize(vSources) && Wln_RetCheckForward( p, vSources );
        int fBackward = Vec_IntSize(vSinks)   && Wln_RetCheckBackward( p, vSinks );
        Vec_IntSort( vSources, 0 );
        Vec_IntSort( vSinks, 0 );

        if ( !fForward && !fBackward )
        {
            printf( "Cannot retime forward and backward.\n" );
            break;
        }
        if ( Vec_IntTwoCountCommon(vSources, vSinks) )
        {
            printf( "Cannot reduce delay by retiming.\n" );
            break;
        }
        nMoves++;
        Vec_IntClear( vFront );
        if ( (fPrevFwd && fForward) || (!(fPrevBwd && fBackward) && ((fForward && !fBackward) || (fForward && fBackward && Vec_IntSize(vSources) < Vec_IntSize(vSinks)))) )
        {
            Vec_IntAppend( vFront, vSources );
            Wln_RetMarkChanges( p, vFront );
            Wln_RetRetimeForward( p, vFront );
            p->DelayMax = Wln_RetPropDelay( p );
            fForward = 1, fBackward = 0;
            fPrevFwd = 1;
        }
        else
        {
            Vec_IntAppend( vFront, vSinks );
            Wln_RetRetimeBackward( p, vFront );
            Wln_RetMarkChanges( p, vFront );
            p->DelayMax = Wln_RetPropDelay( p );
            fForward = 0, fBackward = 1;
            fPrevBwd = 1;
        }
        DelayBest = Abc_MinInt( DelayBest, p->DelayMax );
        //Wln_RetPrint( p );
        if ( fVerbose )
            printf( "\n" );
        Wln_RetAddToMoves( p, vFront, p->DelayMax, fForward, nMoves, fSkipSimple, fVerbose );
        if ( fVerbose )
        {
            printf( "Sinks: " );
            Vec_IntPrint( &p->vSinks );
            printf( "Sources: " );
            Vec_IntPrint( &p->vSources );
        }
        if ( p->DelayMax >= DelayMaxPrev )
            nCountIncrease++;
        else
        {
            if ( nCountIncrease > 0 )
                nChange++;
            nCountIncrease = 0;
        }
        if ( nCountIncrease > 3 )
            break;
        if ( nChange > 5 )
            break;
        Wln_RetFindSources( p );
        if ( 2*Vec_IntSize(&p->vEdgeLinks) > Vec_IntCap(&p->vEdgeLinks) )
            Vec_IntGrow( &p->vEdgeLinks, 4*Vec_IntSize(&p->vEdgeLinks) );
    }
    ABC_SWAP( Vec_Int_t, *vMoves, p->vMoves );
    Wln_RetFree( p );
    if ( fVerbose )
    {
        printf( "\nThe resulting moves recorded in terms of name IDs of the NDR nodes:\n" );
        Vec_IntPrint( vMoves );
    }
    else
    {
        printf( "Retiming instruction contains %d moves and %d total retimed objects.\n", nMoves, Vec_IntSize(vMoves)-2*nMoves-2 );
        printf( "Initial delay = %d.  The best delay achieved = %d.  Improvement = %d. (%6.2f %%)\n", 
            DelayInit, DelayBest, DelayInit - DelayBest, 100.0 * (DelayInit - DelayBest) / DelayInit );
    }
    return vMoves;
}
Vec_Int_t * Wln_NtkRetime( Wln_Ntk_t * pNtk, int fIgnoreIO, int fSkipSimple, int fVerbose )
{
    if ( fIgnoreIO )
        Wln_RetMarkPaths( pNtk, fVerbose );
    return Wln_NtkRetime_int( pNtk, fSkipSimple, fVerbose );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

