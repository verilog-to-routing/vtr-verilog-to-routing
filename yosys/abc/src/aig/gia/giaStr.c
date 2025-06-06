/**CFile****************************************************************

  FileName    [giaStr.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [AIG structuring.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaStr.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/util/utilNam.h"
#include "misc/vec/vecWec.h"
#include "misc/tim/tim.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define STR_SUPER  100
#define MAX_TREE 10000

enum { 
    STR_NONE   =  0,
    STR_CONST0 =  1,
    STR_PI     =  2,
    STR_AND    =  3,
    STR_XOR    =  4,
    STR_MUX    =  5,
    STR_BUF    =  6,
    STR_PO     =  7,
    STR_UNUSED =  8      
};

typedef struct Str_Obj_t_ Str_Obj_t; 
struct Str_Obj_t_
{
    unsigned       Type    :  4;     // object type
    unsigned       nFanins : 28;     // fanin count
    int            iOffset;          // place where fanins are stored
    int            iTop;             // top level MUX
    int            iCopy;            // copy of this node
};
typedef struct Str_Ntk_t_ Str_Ntk_t; 
struct Str_Ntk_t_
{
    int            nObjs;            // object count
    int            nObjsAlloc;       // alloc objects
    Str_Obj_t *    pObjs;            // objects 
    Vec_Int_t      vFanins;          // object fanins
    int            nObjCount[STR_UNUSED];
    int            nTrees;
    int            nGroups;
    int            DelayGain;
};
typedef struct Str_Man_t_ Str_Man_t; 
struct Str_Man_t_
{
    // user data
    Gia_Man_t *     pOld;            // manager
    int             nLutSize;        // LUT size
    int             fCutMin;         // cut minimization
    // internal data
    Str_Ntk_t *     pNtk;            // balanced network
    // AIG under construction
    Gia_Man_t *     pNew;            // newly constructed 
    Vec_Int_t *     vDelays;         // delays of each object
};

static inline Str_Obj_t * Str_NtkObj( Str_Ntk_t * p, int i )                         { assert( i < p->nObjs );  return p->pObjs + i;                                        }
static inline int         Str_ObjFaninId( Str_Ntk_t * p, Str_Obj_t * pObj, int i )   { return Abc_Lit2Var( Vec_IntEntry(&p->vFanins, pObj->iOffset + i) );                  }
static inline Str_Obj_t * Str_ObjFanin( Str_Ntk_t * p, Str_Obj_t * pObj, int i )     { return Str_NtkObj( p, Str_ObjFaninId(p, pObj, i) );                                  }
static inline int         Str_ObjFaninC( Str_Ntk_t * p, Str_Obj_t * pObj, int i )    { return Abc_LitIsCompl( Vec_IntEntry(&p->vFanins, pObj->iOffset + i) );               }
static inline int         Str_ObjFaninCopy( Str_Ntk_t * p, Str_Obj_t * pObj, int i ) { return Abc_LitNotCond( Str_ObjFanin(p, pObj, i)->iCopy, Str_ObjFaninC(p, pObj, i) ); }
static inline int         Str_ObjId( Str_Ntk_t * p, Str_Obj_t * pObj )               { return pObj - p->pObjs;                                                              }

#define Str_NtkManForEachObj( p, pObj )  \
    for ( pObj = p->pObjs; Str_ObjId(p, pObj) < p->nObjs; pObj++ )
#define Str_NtkManForEachObjVec( vVec, p, pObj, i )  \
    for ( i = 0; (i < Vec_IntSize(vVec)) && ((pObj) = Str_NtkObj(p, Vec_IntEntry(vVec,i))); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Logic network manipulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Str_ObjCreate( Str_Ntk_t * p, int Type, int nFanins, int * pFanins )
{
    Str_Obj_t * pObj = p->pObjs + p->nObjs; int i;
    assert( p->nObjs < p->nObjsAlloc );
    pObj->Type    = Type;
    pObj->nFanins = nFanins;
    pObj->iOffset = Vec_IntSize(&p->vFanins);
    pObj->iTop    = pObj->iCopy = -1;
    for ( i = 0; i < nFanins; i++ )
    {
        Vec_IntPush( &p->vFanins, pFanins[i] );
        assert( pFanins[i] >= 0 );
    }
    p->nObjCount[Type]++;
    return Abc_Var2Lit( p->nObjs++, 0 );
}
static inline Str_Ntk_t * Str_NtkCreate( int nObjsAlloc, int nFaninsAlloc )
{
    Str_Ntk_t * p;
    p = ABC_CALLOC( Str_Ntk_t, 1 );
    p->pObjs = ABC_ALLOC( Str_Obj_t, nObjsAlloc );
    p->nObjsAlloc = nObjsAlloc;
    Str_ObjCreate( p, STR_CONST0, 0, NULL );
    Vec_IntGrow( &p->vFanins, nFaninsAlloc );
    return p;
}
static inline void Str_NtkDelete( Str_Ntk_t * p )
{
//    printf( "Total delay gain = %d.\n", p->DelayGain );
    ABC_FREE( p->vFanins.pArray );
    ABC_FREE( p->pObjs );
    ABC_FREE( p );
}
static inline void Str_NtkPs( Str_Ntk_t * p, abctime clk )
{
    printf( "Network contains %d ands, %d xors, %d muxes (%d trees in %d groups).  ", 
        p->nObjCount[STR_AND], p->nObjCount[STR_XOR], p->nObjCount[STR_MUX], p->nTrees, p->nGroups );
    Abc_PrintTime( 1, "Time", clk );
}
static inline void Str_ObjReadGroup( Str_Ntk_t * p, Str_Obj_t * pObj, int * pnGroups, int * pnMuxes )
{
    Str_Obj_t * pObj1, * pObj2;
    *pnGroups = *pnMuxes = 0;
    if ( pObj->iTop == 0 )
        return;
    pObj1 = Str_NtkObj( p, pObj->iTop ); 
    pObj2 = Str_NtkObj( p, pObj1->iTop );
    *pnMuxes  = pObj1 - pObj + 1;
    *pnGroups = (pObj2 - pObj + 1) / *pnMuxes;
}
static inline void Str_NtkPrintGroups( Str_Ntk_t * p )
{
    Str_Obj_t * pObj; 
    int nGroups, nMuxes;
    Str_NtkManForEachObj( p, pObj )
        if ( pObj->Type == STR_MUX && pObj->iTop > 0 )
        {
            Str_ObjReadGroup( p, pObj, &nGroups, &nMuxes );
            pObj += nGroups * nMuxes - 1;
            printf( "%d x %d  ", nGroups, nMuxes );
        }
    printf( "\n" );
}
Gia_Man_t * Str_NtkToGia( Gia_Man_t * pGia, Str_Ntk_t * p )
{
    Gia_Man_t * pNew, * pTemp;
    Str_Obj_t * pObj; int k;
    assert( pGia->pMuxes == NULL );
    pNew = Gia_ManStart( 3 * Gia_ManObjNum(pGia) / 2 );
    pNew->pName = Abc_UtilStrsav( pGia->pName );
    pNew->pSpec = Abc_UtilStrsav( pGia->pSpec );
    Gia_ManHashStart( pNew );
    Str_NtkManForEachObj( p, pObj )
    {
        if ( pObj->Type == STR_PI )
            pObj->iCopy = Gia_ManAppendCi( pNew );
        else if ( pObj->Type == STR_AND )
        {
            pObj->iCopy = 1;
            for ( k = 0; k < (int)pObj->nFanins; k++ )
                pObj->iCopy = Gia_ManHashAnd( pNew, pObj->iCopy, Str_ObjFaninCopy(p, pObj, k) );
        }
        else if ( pObj->Type == STR_XOR )
        {
            pObj->iCopy = 0;
            for ( k = 0; k < (int)pObj->nFanins; k++ )
                pObj->iCopy = Gia_ManHashXor( pNew, pObj->iCopy, Str_ObjFaninCopy(p, pObj, k) );
        }
        else if ( pObj->Type == STR_MUX )
            pObj->iCopy = Gia_ManHashMux( pNew, Str_ObjFaninCopy(p, pObj, 2), Str_ObjFaninCopy(p, pObj, 1), Str_ObjFaninCopy(p, pObj, 0) );
        else if ( pObj->Type == STR_PO )
            pObj->iCopy = Gia_ManAppendCo( pNew, Str_ObjFaninCopy(p, pObj, 0) );
        else if ( pObj->Type == STR_CONST0 )
            pObj->iCopy = 0;
        else assert( 0 );
    }
    Gia_ManHashStop( pNew );
//    assert( Gia_ManObjNum(pNew) <= Gia_ManObjNum(pGia) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(pGia) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Constructs a normalized AIG without structural hashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupMuxesNoHash( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj, * pFan0, * pFan1, * pFanC;
    int i, iLit0, iLit1, fCompl;
    assert( p->pMuxes == NULL );
    ABC_FREE( p->pRefs );
    Gia_ManCreateRefs( p ); 
    // discount nodes with one fanout pointed to by MUX type
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( !Gia_ObjIsMuxType(pObj) )
            continue;
        Gia_ObjRefDec(p, Gia_ObjFanin0(pObj));
        Gia_ObjRefDec(p, Gia_ObjFanin1(pObj));
    }
    // start the new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName  = Abc_UtilStrsav( p->pName );
    pNew->pSpec  = Abc_UtilStrsav( p->pSpec );
    pNew->pMuxes = ABC_CALLOC( unsigned, pNew->nObjsAlloc );
    Gia_ManFillValue(p);
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( !Gia_ObjRefNumId(p, i) )
            continue;
        if ( !Gia_ObjIsMuxType(pObj) )
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
        {
            iLit0 = Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan0));
            iLit1 = Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan1));
            fCompl = Abc_LitIsCompl(iLit0) ^ Abc_LitIsCompl(iLit1);
            pObj->Value = fCompl ^ Gia_ManAppendXorReal( pNew, Abc_LitRegular(iLit0), Abc_LitRegular(iLit1) );
        }
        else
        {
            pFanC = Gia_ObjRecognizeMux( pObj, &pFan1, &pFan0 );
            iLit0 = Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan0));
            iLit1 = Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan1));
            if ( iLit0 == iLit1 )
                pObj->Value = iLit0;
            else if ( Abc_Lit2Var(iLit0) == Abc_Lit2Var(iLit1) )
            {
                iLit1 = Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFanC));
                fCompl = Abc_LitIsCompl(iLit0) ^ Abc_LitIsCompl(iLit1);
                pObj->Value = fCompl ^ Gia_ManAppendXorReal( pNew, Abc_LitRegular(iLit0), Abc_LitRegular(iLit1) );
            }
            else
                pObj->Value = Gia_ManAppendMuxReal( pNew, Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFanC)), Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan1)), Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan0)) );
        }
    }
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    assert( !Gia_ManHasDangling(pNew) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Constructs AIG ordered for balancing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Str_MuxInputsCollect_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes )
{
    if ( !pObj->fMark0 )
    {
        Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
        return;
    }
    Vec_IntPush( vNodes, Gia_ObjFaninId2p(p, pObj) );
    Str_MuxInputsCollect_rec( p, Gia_ObjFanin0(pObj), vNodes );
    Str_MuxInputsCollect_rec( p, Gia_ObjFanin1(pObj), vNodes );
}
void Str_MuxInputsCollect( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes )
{
    assert( !pObj->fMark0 );
    pObj->fMark0 = 1;
    Vec_IntClear( vNodes );
    Str_MuxInputsCollect_rec( p, pObj, vNodes );
    pObj->fMark0 = 0;
}
void Str_MuxStructCollect_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes )
{
    if ( !pObj->fMark0 )
        return;
    Str_MuxStructCollect_rec( p, Gia_ObjFanin0(pObj), vNodes );
    Str_MuxStructCollect_rec( p, Gia_ObjFanin1(pObj), vNodes );
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
}
void Str_MuxStructCollect( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes )
{
    assert( !pObj->fMark0 );
    pObj->fMark0 = 1;
    Vec_IntClear( vNodes );
    Str_MuxStructCollect_rec( p, pObj, vNodes );
    pObj->fMark0 = 0;
}
void Str_MuxStructDump_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Str_t * vStr )
{
    if ( !pObj->fMark0 )
        return;
    Vec_StrPush( vStr, '[' );
    Vec_StrPush( vStr, '(' );
    Vec_StrPrintNum( vStr, Gia_ObjFaninId2p(p, pObj) );
    Vec_StrPush( vStr, ')' );
    Str_MuxStructDump_rec( p, Gia_ObjFaninC2(p, pObj) ? Gia_ObjFanin0(pObj) : Gia_ObjFanin1(pObj), vStr );
    Vec_StrPush( vStr, '|' );
    Str_MuxStructDump_rec( p, Gia_ObjFaninC2(p, pObj) ? Gia_ObjFanin1(pObj) : Gia_ObjFanin0(pObj), vStr );
    Vec_StrPush( vStr, ']' );
}
void Str_MuxStructDump( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Str_t * vStr )
{
    assert( !pObj->fMark0 );
    pObj->fMark0 = 1;
    Vec_StrClear( vStr );
    Str_MuxStructDump_rec( p, pObj, vStr );
    Vec_StrPush( vStr, '\0' );
    pObj->fMark0 = 0;
}
int Str_ManMuxCountOne( char * p )
{
    int Count = 0;
    for ( ; *p; p++ )
        Count += (*p == '[');
    return Count;
}
Vec_Wec_t * Str_ManDeriveTrees( Gia_Man_t * p )
{
    int fPrintStructs = 0;
    Abc_Nam_t * pNames; 
    Vec_Wec_t * vGroups;  
    Vec_Str_t * vStr;
    Gia_Obj_t * pObj, * pFanin;
    int i, iStructId, fFound;
    assert( p->pMuxes != NULL );
    // mark MUXes whose only fanout is a MUX
    ABC_FREE( p->pRefs );
    Gia_ManCreateRefs( p ); 
    Gia_ManForEachMuxId( p, i )
    {
        pObj = Gia_ManObj(p, i);
        pFanin = Gia_ObjFanin0(pObj);
        if ( Gia_ObjIsMux(p, pFanin) && Gia_ObjRefNum(p, pFanin) == 1 )
            pFanin->fMark0 = 1;
        pFanin = Gia_ObjFanin1(pObj);
        if ( Gia_ObjIsMux(p, pFanin) && Gia_ObjRefNum(p, pFanin) == 1 )
            pFanin->fMark0 = 1;
    }
    // traverse for top level MUXes
    vStr   = Vec_StrAlloc( 1000 );
    pNames = Abc_NamStart( 10000, 50 );
    vGroups = Vec_WecAlloc( 1000 );
    Vec_WecPushLevel( vGroups );
    Gia_ManForEachMuxId( p, i )
    {
        // skip internal
        pObj = Gia_ManObj(p, i);
        if ( pObj->fMark0 )
            continue;
        // skip trees of size one
        if ( !Gia_ObjFanin0(pObj)->fMark0 && !Gia_ObjFanin1(pObj)->fMark0 )
            continue;
        // hash the tree
        Str_MuxStructDump( p, pObj, vStr );
        iStructId = Abc_NamStrFindOrAdd( pNames, Vec_StrArray(vStr), &fFound );
        if ( !fFound ) Vec_WecPushLevel( vGroups );
        assert( Abc_NamObjNumMax(pNames) == Vec_WecSize(vGroups) );
        Vec_IntPush( Vec_WecEntry(vGroups, iStructId), i );
    }
    if ( fPrintStructs )
    {
        char * pTemp; 
        Abc_NamManForEachObj( pNames, pTemp, i )
        {
            printf( "%5d : ", i );
            printf( "Occur = %4d   ", Vec_IntSize(Vec_WecEntry(vGroups,i)) );
            printf( "Size = %4d   ",  Str_ManMuxCountOne(pTemp) );
            printf( "%s\n", pTemp );
        }
    }
    Abc_NamStop( pNames );
    Vec_StrFree( vStr );
    return vGroups;
}
Vec_Int_t * Str_ManCreateRoots( Vec_Wec_t * vGroups, int nObjs )
{   // map tree MUXes into their classes
    Vec_Int_t * vRoots;
    Vec_Int_t * vGroup;
    int i, k, Entry;
    vRoots = Vec_IntStartFull( nObjs );
    Vec_WecForEachLevel( vGroups, vGroup, i )
        Vec_IntForEachEntry( vGroup, Entry, k )
            Vec_IntWriteEntry( vRoots, Entry, i );
    return vRoots;
}

void Str_MuxTraverse_rec( Gia_Man_t * p, int i )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p, i) )
        return;
    Gia_ObjSetTravIdCurrentId(p, i);
    pObj = Gia_ManObj(p, i);
    if ( !Gia_ObjIsAnd(pObj) )
        return;
    Str_MuxTraverse_rec(p, Gia_ObjFaninId0(pObj, i) );
    Str_MuxTraverse_rec(p, Gia_ObjFaninId1(pObj, i) );
    if ( Gia_ObjIsMux(p, pObj) )
        Str_MuxTraverse_rec(p, Gia_ObjFaninId2(p, i) );
}
void Str_ManCheckOverlap( Gia_Man_t * p, Vec_Wec_t * vGroups )
{   // check that members of each group are not in the TFI of each other
    Vec_Int_t * vGroup, * vGroup2;
    int i, k, n, iObj, iObj2;

//    vGroup = Vec_WecEntry(vGroups, 7);
//    Vec_IntForEachEntry( vGroup, iObj, n )
//        Gia_ManPrintCone2( p, Gia_ManObj(p, iObj) ), printf( "\n" );

    Vec_WecForEachLevel( vGroups, vGroup, i )
    Vec_IntForEachEntry( vGroup, iObj, k )
    {
        if ( Vec_IntSize(vGroup) == 1 )
            continue;
        // high light the cone
        Gia_ManIncrementTravId( p );
        Str_MuxTraverse_rec( p, iObj );
        // check that none of the others are highlighted
        Vec_IntForEachEntry( vGroup, iObj2, n )
            if ( iObj != iObj2 && Gia_ObjIsTravIdCurrentId(p, iObj2) )
                break;
        if ( n == Vec_IntSize(vGroup) )
            continue;
        // split the group into individual trees
        Vec_IntForEachEntryStart( vGroup, iObj2, n, 1 )
        {
            vGroup2 = Vec_WecPushLevel( vGroups );
            vGroup  = Vec_WecEntry( vGroups, i );
            Vec_IntPush( vGroup2, iObj2 );
        }
        Vec_IntShrink( vGroup, 1 );

/*
        // this does not work because there can be a pair of independent trees
        // with another tree squeezed in between them, so that there is a combo loop

        // divide this group
        nNew = 0;
        vGroup2 = Vec_WecPushLevel( vGroups );
        vGroup  = Vec_WecEntry( vGroups, i );
        Vec_IntForEachEntry( vGroup, iObj2, n )
        {
            if ( iObj != iObj2 && Gia_ObjIsTravIdCurrentId(p, iObj2) )
                Vec_IntPush( vGroup2, iObj2 );
            else
                Vec_IntWriteEntry( vGroup, nNew++, iObj2 );
        }
        Vec_IntShrink( vGroup, nNew );
        i--;
        break;
*/

/*
        // check that none of the others are highlighted
        Vec_IntForEachEntry( vGroup, iObj, n )
            if ( n != k && Gia_ObjIsTravIdCurrentId(p, iObj) )
            {
                printf( "Overlap of TFI cones of trees %d and %d in group %d of size %d!\n", k, n, i, Vec_IntSize(vGroup) );
                Vec_IntShrink( vGroup, 1 );
                break;
            }
*/
    }
}

/**Function*************************************************************

  Synopsis    [Simplify multi-input AND/XOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSimplifyXor( Vec_Int_t * vSuper )
{
    int i, k = 0, Prev = -1, This, fCompl = 0;
    Vec_IntForEachEntry( vSuper, This, i )
    {
        if ( This == 0 )
            continue;
        if ( This == 1 )
            fCompl ^= 1; 
        else if ( Prev != This )
            Vec_IntWriteEntry( vSuper, k++, This ), Prev = This;
        else
            Prev = -1, k--;
    }
    Vec_IntShrink( vSuper, k );
    if ( Vec_IntSize( vSuper ) == 0 )
        Vec_IntPush( vSuper, fCompl );
    else if ( fCompl )
        Vec_IntWriteEntry( vSuper, 0, Abc_LitNot(Vec_IntEntry(vSuper, 0)) );
}
static inline void Gia_ManSimplifyAnd( Vec_Int_t * vSuper )
{
    int i, k = 0, Prev = -1, This;
    Vec_IntForEachEntry( vSuper, This, i )
    {
        if ( This == 0 )
            { Vec_IntFill(vSuper, 1, 0); return; }
        if ( This == 1 )
            continue;
        if ( Prev == -1 || Abc_Lit2Var(Prev) != Abc_Lit2Var(This) )
            Vec_IntWriteEntry( vSuper, k++, This ), Prev = This;
        else if ( Prev != This )
            { Vec_IntFill(vSuper, 1, 0); return; }
    }
    Vec_IntShrink( vSuper, k );
    if ( Vec_IntSize( vSuper ) == 0 )
        Vec_IntPush( vSuper, 1 );
}

/**Function*************************************************************

  Synopsis    [Collect multi-input AND/XOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSuperCollectXor_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    assert( !Gia_IsComplement(pObj) );
    if ( !Gia_ObjIsXor(pObj) || 
        Gia_ObjRefNum(p, pObj) > 1 || 
//        Gia_ObjRefNum(p, pObj) > 3 || 
//        (Gia_ObjRefNum(p, pObj) == 2 && (Gia_ObjRefNum(p, Gia_ObjFanin0(pObj)) == 1 || Gia_ObjRefNum(p, Gia_ObjFanin1(pObj)) == 1)) || 
        Vec_IntSize(p->vSuper) > STR_SUPER )
    {
        Vec_IntPush( p->vSuper, Gia_ObjToLit(p, pObj) );
        return;
    }
    assert( !Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) );
    Gia_ManSuperCollectXor_rec( p, Gia_ObjFanin0(pObj) );
    Gia_ManSuperCollectXor_rec( p, Gia_ObjFanin1(pObj) );
}
static inline void Gia_ManSuperCollectAnd_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_IsComplement(pObj) || 
        !Gia_ObjIsAndReal(p, pObj) || 
        Gia_ObjRefNum(p, pObj) > 1 || 
//        Gia_ObjRefNum(p, pObj) > 3 || 
//        (Gia_ObjRefNum(p, pObj) == 2 && (Gia_ObjRefNum(p, Gia_ObjFanin0(pObj)) == 1 || Gia_ObjRefNum(p, Gia_ObjFanin1(pObj)) == 1)) || 
        Vec_IntSize(p->vSuper) > STR_SUPER )
    {
        Vec_IntPush( p->vSuper, Gia_ObjToLit(p, pObj) );
        return;
    }
    Gia_ManSuperCollectAnd_rec( p, Gia_ObjChild0(pObj) );
    Gia_ManSuperCollectAnd_rec( p, Gia_ObjChild1(pObj) );
}
static inline void Gia_ManSuperCollect( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( p->vSuper == NULL )
        p->vSuper = Vec_IntAlloc( STR_SUPER );
    else
        Vec_IntClear( p->vSuper );
    if ( Gia_ObjIsXor(pObj) )
    {
        assert( !Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) );
        Gia_ManSuperCollectXor_rec( p, Gia_ObjFanin0(pObj) );
        Gia_ManSuperCollectXor_rec( p, Gia_ObjFanin1(pObj) );
        Vec_IntSort( p->vSuper, 0 );
        Gia_ManSimplifyXor( p->vSuper );
    }
    else if ( Gia_ObjIsAndReal(p, pObj) )
    {
        Gia_ManSuperCollectAnd_rec( p, Gia_ObjChild0(pObj) );
        Gia_ManSuperCollectAnd_rec( p, Gia_ObjChild1(pObj) );
        Vec_IntSort( p->vSuper, 0 );
        Gia_ManSimplifyAnd( p->vSuper );
    }
    else assert( 0 );
    assert( Vec_IntSize(p->vSuper) > 0 );
}

/**Function*************************************************************

  Synopsis    [Constructs AIG ordered for balancing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Str_ManNormalize_rec( Str_Ntk_t * pNtk, Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Wec_t * vGroups, Vec_Int_t * vRoots )
{
    int i, k, iVar, iLit, iBeg, iEnd;
    if ( ~pObj->Value )
        return;
    pObj->Value = 0;
    assert( Gia_ObjIsAnd(pObj) );
    if ( Gia_ObjIsMux(p, pObj) )
    {
        Vec_Int_t * vGroup;
        Gia_Obj_t * pRoot, * pMux;
        int pFanins[3];
        if ( Vec_IntEntry(vRoots, Gia_ObjId(p, pObj)) == -1 )
        {
            Str_ManNormalize_rec( pNtk, p, Gia_ObjFanin0(pObj), vGroups, vRoots );
            Str_ManNormalize_rec( pNtk, p, Gia_ObjFanin1(pObj), vGroups, vRoots );
            Str_ManNormalize_rec( pNtk, p, Gia_ObjFanin2(p, pObj), vGroups, vRoots );
            pFanins[0] = Gia_ObjFanin0Copy(pObj);
            pFanins[1] = Gia_ObjFanin1Copy(pObj);
            pFanins[2] = Gia_ObjFanin2Copy(p, pObj);
            if ( Abc_LitIsCompl(pFanins[2]) )
            {
                pFanins[2] = Abc_LitNot(pFanins[2]);
                ABC_SWAP( int, pFanins[0], pFanins[1] );
            }
            pObj->Value = Str_ObjCreate( pNtk, STR_MUX, 3, pFanins );
            return;
        }
        vGroup = Vec_WecEntry( vGroups, Vec_IntEntry(vRoots, Gia_ObjId(p, pObj)) );
        // build data-inputs for each tree
        Gia_ManForEachObjVec( vGroup, p, pRoot, i )
        {
            Str_MuxInputsCollect( p, pRoot, p->vSuper );
            iBeg = Vec_IntSize( p->vStore );
            Vec_IntAppend( p->vStore, p->vSuper );
            iEnd = Vec_IntSize( p->vStore );
            Vec_IntForEachEntryStartStop( p->vStore, iVar, k, iBeg, iEnd )
                Str_ManNormalize_rec( pNtk, p, Gia_ManObj(p, iVar), vGroups, vRoots );
            Vec_IntShrink( p->vStore, iBeg );
        }
        // build internal structures
        Gia_ManForEachObjVec( vGroup, p, pRoot, i )
        {
            Str_MuxStructCollect( p, pRoot, p->vSuper );
            Gia_ManForEachObjVec( p->vSuper, p, pMux, k )
            {
                pFanins[0] = Gia_ObjFanin0Copy(pMux);
                pFanins[1] = Gia_ObjFanin1Copy(pMux);
                pFanins[2] = Gia_ObjFanin2Copy(p, pMux);
                if ( Abc_LitIsCompl(pFanins[2]) )
                {
                    pFanins[2] = Abc_LitNot(pFanins[2]);
                    ABC_SWAP( int, pFanins[0], pFanins[1] );
                }
                pMux->Value = Str_ObjCreate( pNtk, STR_MUX, 3, pFanins );
            }
            assert( ~pRoot->Value );
            // set mapping
            Gia_ManForEachObjVec( p->vSuper, p, pMux, k )
                Str_NtkObj(pNtk, Abc_Lit2Var(pMux->Value))->iTop = Abc_Lit2Var(pRoot->Value);
            pNtk->nTrees++;
        }
        assert( ~pObj->Value );
        // set mapping
        pObj = Gia_ManObj( p, Vec_IntEntryLast(vGroup) );
        Gia_ManForEachObjVec( vGroup, p, pRoot, i )
            Str_NtkObj(pNtk, Abc_Lit2Var(pRoot->Value))->iTop = Abc_Lit2Var(pObj->Value);
        pNtk->nGroups++;
        //printf( "%d x %d  ", Vec_IntSize(vGroup), Vec_IntSize(p->vSuper) );
        return;
    }
    // find supergate
    Gia_ManSuperCollect( p, pObj );
    // save entries
    iBeg = Vec_IntSize( p->vStore );
    Vec_IntAppend( p->vStore, p->vSuper );
    iEnd = Vec_IntSize( p->vStore );
    // call recursively
    Vec_IntForEachEntryStartStop( p->vStore, iLit, i, iBeg, iEnd )
    {
        Gia_Obj_t * pTemp = Gia_ManObj( p, Abc_Lit2Var(iLit) );
        Str_ManNormalize_rec( pNtk, p, pTemp, vGroups, vRoots );
        Vec_IntWriteEntry( p->vStore, i, Abc_LitNotCond(pTemp->Value, Abc_LitIsCompl(iLit)) );
    }
    assert( Vec_IntSize(p->vStore) == iEnd );
    // consider general case
    pObj->Value = Str_ObjCreate( pNtk, Gia_ObjIsXor(pObj) ? STR_XOR : STR_AND, iEnd-iBeg, Vec_IntEntryP(p->vStore, iBeg) );
    Vec_IntShrink( p->vStore, iBeg );
}
Str_Ntk_t * Str_ManNormalizeInt( Gia_Man_t * p, Vec_Wec_t * vGroups, Vec_Int_t * vRoots )
{
    Str_Ntk_t * pNtk;
    Gia_Obj_t * pObj;
    int i, iFanin;
    assert( p->pMuxes != NULL );
    if ( p->vSuper == NULL )
        p->vSuper = Vec_IntAlloc( STR_SUPER );
    if ( p->vStore == NULL )
        p->vStore = Vec_IntAlloc( STR_SUPER );
    Gia_ManFillValue( p );
    pNtk = Str_NtkCreate( Gia_ManObjNum(p) + 10000, 1 + Gia_ManCoNum(p) + 2 * Gia_ManAndNum(p) + Gia_ManMuxNum(p) + 10000 );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Str_ObjCreate( pNtk, STR_PI, 0, NULL );
        else if ( Gia_ObjIsCo(pObj) )
        {
            Str_ManNormalize_rec( pNtk, p, Gia_ObjFanin0(pObj), vGroups, vRoots );
            iFanin = Gia_ObjFanin0Copy(pObj);
            pObj->Value = Str_ObjCreate( pNtk, STR_PO, 1, &iFanin );
        }
    }
    //assert( pNtk->nObjs <= Gia_ManObjNum(p) );
    return pNtk;
}
Str_Ntk_t * Str_ManNormalize( Gia_Man_t * p )
{
    Str_Ntk_t * pNtk;
    Gia_Man_t * pMuxes = Gia_ManDupMuxes( p, 5 );
    Vec_Wec_t * vGroups = Str_ManDeriveTrees( pMuxes );
    Vec_Int_t * vRoots;
    Str_ManCheckOverlap( pMuxes, vGroups );
    vRoots = Str_ManCreateRoots( vGroups, Gia_ManObjNum(pMuxes) );
    pNtk = Str_ManNormalizeInt( pMuxes, vGroups, vRoots );
    Gia_ManCleanMark0( pMuxes );
    Gia_ManStop( pMuxes );
    Vec_IntFree( vRoots );
    Vec_WecFree( vGroups );
    return pNtk;
}
 
/**Function*************************************************************

  Synopsis    [Delay computation]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Str_Delay2( int d0, int d1, int nLutSize )
{
    int n, d = Abc_MaxInt( d0 >> 4, d1 >> 4 );
    n  = (d == (d0 >> 4)) ? (d0 & 15) : 1;
    n += (d == (d1 >> 4)) ? (d1 & 15) : 1;
    return (d << 4) + (n > nLutSize ? 18 : n);
}
static inline int Str_Delay3( int d0, int d1, int d2, int nLutSize )
{
    int n, d = Abc_MaxInt( Abc_MaxInt(d0 >> 4, d1 >> 4), d2 >> 4 );
    n  = (d == (d0 >> 4)) ? (d0 & 15) : 1;
    n += (d == (d1 >> 4)) ? (d1 & 15) : 1;
    n += (d == (d2 >> 4)) ? (d2 & 15) : 1;
    return (d << 4) + (n > nLutSize ? 19 : n);
}
static inline int Str_ObjDelay( Gia_Man_t * pNew, int iObj, int nLutSize, Vec_Int_t * vDelay )
{
    int Delay = Vec_IntEntry( vDelay, iObj );
    if ( Delay == 0 )
    {
        if ( Gia_ObjIsMuxId(pNew, iObj) )
        {
            int d0 = Vec_IntEntry( vDelay, Gia_ObjFaninId0(Gia_ManObj(pNew, iObj), iObj) );
            int d1 = Vec_IntEntry( vDelay, Gia_ObjFaninId1(Gia_ManObj(pNew, iObj), iObj) );
            int d2 = Vec_IntEntry( vDelay, Gia_ObjFaninId2(pNew, iObj) );
            Delay = Str_Delay3( d0, d1, d2, nLutSize );
        }
        else
        {
            int d0 = Vec_IntEntry( vDelay, Gia_ObjFaninId0(Gia_ManObj(pNew, iObj), iObj) );
            int d1 = Vec_IntEntry( vDelay, Gia_ObjFaninId1(Gia_ManObj(pNew, iObj), iObj) );
            Delay = Str_Delay2( d0, d1, nLutSize );
        }
        Vec_IntWriteEntry( vDelay, iObj, Delay );
    }
    return Delay;
}



/**Function*************************************************************

  Synopsis    [Transposing 64-bit matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void transpose64( word A[64] )
{
    int j, k;
    word t, m = 0x00000000FFFFFFFF;
    for ( j = 32; j != 0; j = j >> 1, m = m ^ (m << j) )
    {
        for ( k = 0; k < 64; k = (k + j + 1) & ~j )
        {
            t = (A[k] ^ (A[k+j] >> j)) & m;
            A[k] = A[k] ^ t;
            A[k+j] = A[k+j] ^ (t << j);
        }
    }
}

/**Function*************************************************************

  Synopsis    [Perform affinity computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int  Str_ManNum( Gia_Man_t * p, int iObj )             { return Vec_IntEntry(&p->vCopies, iObj);     }
static inline void Str_ManSetNum( Gia_Man_t * p, int iObj, int Num ) { Vec_IntWriteEntry(&p->vCopies, iObj, Num);  }

int Str_ManVectorAffinity( Gia_Man_t * p, Vec_Int_t * vSuper, Vec_Int_t * vDelay, word * Matrix, int nLimit )
{
    int fVerbose = 0;
    int * Levels = NULL;
    int nSize = Vec_IntSize(vSuper);
    int Prev = nSize, nLevels = 1;
    int i, k, iLit, iFanin, nSizeNew;
    word Mask; 
    assert( nSize > 2 );
    assert( nSize <= nLimit );
    if ( nSize > 64 )
    {
        for ( i = 0; i < 64; i++ )
            Matrix[i] = 0;
        return 0;
    }
    Levels = ABC_ALLOC( int, nLimit+256 );
    // mark current nodes
    Gia_ManIncrementTravId( p );
    Vec_IntForEachEntry( vSuper, iLit, i )
    {
        Gia_ObjSetTravIdCurrentId( p, Abc_Lit2Var(iLit) );
        Str_ManSetNum( p, Abc_Lit2Var(iLit), i );
        Matrix[i] = ((word)1) << (63-i);
        Levels[i] = 0;
    }
    // collect 64 nodes
    Vec_IntForEachEntry( vSuper, iLit, i )
    {
        Gia_Obj_t * pObj = Gia_ManObj( p, Abc_Lit2Var(iLit) );
        if ( Gia_ObjIsAnd(pObj) )
        {
            for ( k = 0; k < 2; k++ )
            {
                iFanin = k ? Gia_ObjFaninId1p(p, pObj) : Gia_ObjFaninId0p(p, pObj);
                if ( !Gia_ObjIsTravIdCurrentId(p, iFanin) )
                {
                    if ( Vec_IntSize(vSuper) == nLimit )
                        break;
                    Gia_ObjSetTravIdCurrentId( p, iFanin );
                    Matrix[Vec_IntSize(vSuper)] = 0;
                    Levels[Vec_IntSize(vSuper)] = nLevels;
                    Str_ManSetNum( p, iFanin, Vec_IntSize(vSuper) );
                    Vec_IntPush( vSuper, Abc_Var2Lit(iFanin, 0) );
                }
                Matrix[Str_ManNum(p, iFanin)] |= Matrix[i];
            }
        }
        if ( Gia_ObjIsMux(p, pObj) )
        {
            iFanin = Gia_ObjFaninId2p(p, pObj);
            if ( !Gia_ObjIsTravIdCurrentId(p, iFanin) )
            {
                if ( Vec_IntSize(vSuper) == nLimit )
                    break;
                Gia_ObjSetTravIdCurrentId( p, iFanin );
                Matrix[Vec_IntSize(vSuper)] = 0;
                Levels[Vec_IntSize(vSuper)] = nLevels;
                Str_ManSetNum( p, iFanin, Vec_IntSize(vSuper) );
                Vec_IntPush( vSuper, Abc_Var2Lit(iFanin, 0) );
            }
            Matrix[Str_ManNum(p, iFanin)] |= Matrix[i];
        }
        if ( Prev == i )
            Prev = Vec_IntSize(vSuper), nLevels++;
        if ( nLevels == 8 )
            break;
    }

    // remove those that have all 1s or only one 1
    Mask = (~(word)0) << (64 - nSize);
    for ( k = i = 0; i < Vec_IntSize(vSuper); i++ )
    {
        assert( Matrix[i] );
        if ( (Matrix[i] & (Matrix[i] - 1)) == 0 )
            continue;
        if ( Matrix[i] == Mask )
            continue;
        Matrix[k] = Matrix[i];
        Levels[k] = Levels[i];
        k++;
        if ( k == 64 )
            break;
    }
    // clean the remaining ones
    for ( i = k; i < 64; i++ )
        Matrix[i] = 0;
    nSizeNew = k;
    if ( nSizeNew == 0 )
    {
        Vec_IntShrink( vSuper, nSize );
        ABC_FREE( Levels );
        return 0;
    }
/*
    // report
    if ( fVerbose && nSize > 20 )
    {
        for ( i = 0; i < nSizeNew; i++ )
            Extra_PrintBinary( stdout, Matrix+i, 64 ), printf( "\n" );
        printf( "\n" );
    }
*/
    transpose64( Matrix );

    // report
    if ( fVerbose && nSize > 10 )
    {
        printf( "Gate inputs = %d.  Collected fanins = %d.  All = %d.  Good = %d.  Levels = %d\n", 
            nSize, Vec_IntSize(vSuper) - nSize, Vec_IntSize(vSuper), nSizeNew, nLevels );
        printf( "                     " );
        for ( i = 0; i < nSizeNew; i++ )
            printf( "%d", Levels[i] );
        printf( "\n" );
        for ( i = 0; i < nSize; i++ )
        {
            printf( "%6d : ", Abc_Lit2Var(Vec_IntEntry(vSuper, i)) );
            printf( "%3d   ", Vec_IntEntry(vDelay, i) >> 4 );
            printf( "%3d   ", Vec_IntEntry(vDelay, i) & 15 );
//            Extra_PrintBinary( stdout, Matrix+i, 64 ), printf( "\n" );
        }
        i = 0;
    }
    ABC_FREE( Levels );
    Vec_IntShrink( vSuper, nSize );
    return nSizeNew;
}

/**Function*************************************************************

  Synopsis    [Count 1s.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Str_CountBits( word i )
{
    if ( i == 0 )
        return 0;
    i = (i & (i - 1));
    if ( i == 0 )
        return 1;
    i = (i & (i - 1));
    if ( i == 0 )
        return 2;
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    i = ((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0F);
    return (i*(0x0101010101010101))>>56;
}

static inline void Str_PrintState( int * pCost, int * pSuper, word * pMatrix, int nSize )
{
    int i;
    for ( i = 0; i < nSize; i++ )
    {
        printf( "%6d : ", i );
        printf( "%6d : ", Abc_Lit2Var(pSuper[i]) );
        printf( "%3d   ", pCost[i] >> 4 );
        printf( "%3d   ", pCost[i] & 15 );
//        Extra_PrintBinary( stdout, pMatrix+i, 64 ), printf( "\n" );
    }
    printf( "\n" );
}


/**Function*************************************************************

  Synopsis    [Perform balancing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Str_NtkBalanceMulti2( Gia_Man_t * pNew, Str_Ntk_t * p, Str_Obj_t * pObj, Vec_Int_t * vDelay, int nLutSize )
{
    int k;
    pObj->iCopy = (pObj->Type == STR_AND);
    for ( k = 0; k < (int)pObj->nFanins; k++ )
    {
        if ( pObj->Type == STR_AND )
            pObj->iCopy = Gia_ManHashAnd( pNew, pObj->iCopy, Str_ObjFaninCopy(p, pObj, k) );
        else
            pObj->iCopy = Gia_ManHashXorReal( pNew, pObj->iCopy, Str_ObjFaninCopy(p, pObj, k) );
        Str_ObjDelay( pNew, Abc_Lit2Var(pObj->iCopy), nLutSize, vDelay );
    }
}

int Str_NtkBalanceTwo( Gia_Man_t * pNew, Str_Ntk_t * p, Str_Obj_t * pObj, int i, int j, Vec_Int_t * vDelay, int * pCost, int * pSuper, word * pMatrix, int nSize, int nLutSize, int CostBest )
{
    int k, iLitRes, Delay;
    assert( i < j );
//    printf( "Merging node %d and %d\n", i, j );
    if ( pObj->Type == STR_AND )
        iLitRes = Gia_ManHashAnd( pNew, pSuper[i], pSuper[j] );
    else
        iLitRes = Gia_ManHashXorReal( pNew, pSuper[i], pSuper[j] );
    Delay = Str_ObjDelay( pNew, Abc_Lit2Var(iLitRes), nLutSize, vDelay );
    // update
    pCost[i] = Delay;
    pSuper[i] = iLitRes;
    pMatrix[i] |= pMatrix[j];
//    assert( (pCost[i] & 15) == CostBest || CostBest == -1 );
    // remove entry j
    nSize--;
    for ( k = j; k < nSize; k++ )
    {
        pCost[k] = pCost[k+1];
        pSuper[k] = pSuper[k+1];
        pMatrix[k] = pMatrix[k+1];
    }
    // move up the first one
    nSize--;
    for ( k = 0; k < nSize; k++ )
    {
        if ( pCost[k] <= pCost[k+1] )
            break;
        ABC_SWAP( int, pCost[k], pCost[k+1] );
        ABC_SWAP( int, pSuper[k], pSuper[k+1] );
        ABC_SWAP( word, pMatrix[k], pMatrix[k+1] );
    }
    return iLitRes;
}

void Str_NtkBalanceMulti( Gia_Man_t * pNew, Str_Ntk_t * p, Str_Obj_t * pObj, Vec_Int_t * vDelay, int nLutSize )
{
    word * pMatrix = ABC_ALLOC( word, pObj->nFanins+256 );
    Vec_Int_t * vSuper = pNew->vSuper;
    Vec_Int_t * vCosts = pNew->vStore;
    int * pSuper = Vec_IntArray(vSuper);
    int * pCost  = Vec_IntArray(vCosts);
    int k, iLit, MatrixSize = 0;
    assert( (int)pObj->nFanins <= Vec_IntCap(vSuper) );
    assert( (int)pObj->nFanins <= Vec_IntCap(vCosts) );

    // collect nodes
    Vec_IntClear( vSuper );
    for ( k = 0; k < (int)pObj->nFanins; k++ )
        Vec_IntPush( vSuper, Str_ObjFaninCopy(p, pObj, k) );
    Vec_IntSort( vSuper, 0 );
    if ( pObj->Type == STR_AND )
        Gia_ManSimplifyAnd( vSuper );
    else
        Gia_ManSimplifyXor( vSuper );
    assert( Vec_IntSize(vSuper) > 0 );
    if ( Vec_IntSize(vSuper) == 1 )
    {
        pObj->iCopy = Vec_IntEntry(vSuper, 0);
        ABC_FREE( pMatrix );
        return;
    }
    if ( Vec_IntSize(vSuper) == 2 )
    {
        pObj->iCopy = Str_NtkBalanceTwo( pNew, p, pObj, 0, 1, vDelay, pCost, pSuper, pMatrix, 2, nLutSize, -1 );
        ABC_FREE( pMatrix );
        return;
    }

    // sort by cost
    Vec_IntClear( vCosts );
    Vec_IntForEachEntry( vSuper, iLit, k )
        Vec_IntPush( vCosts, Vec_IntEntry(vDelay, Abc_Lit2Var(iLit)) );
    Vec_IntSelectSortCost2( pSuper, Vec_IntSize(vSuper), pCost );

    // compute affinity
    if ( Vec_IntSize(vSuper) < 64 )
        MatrixSize = Str_ManVectorAffinity( pNew, vSuper, vCosts, pMatrix, pObj->nFanins );

    // start the new product
    while ( Vec_IntSize(vSuper) > 2 )
    {
        // pair the first entry with another one on the same level
        int i, iStop, iBest,iBest2;
        int CostNew, CostBest, CostBest2;
        int OccurNew, OccurBest, OccurBest2;

        if ( Vec_IntSize(vSuper) > 64 )
        {
            Str_NtkBalanceTwo( pNew, p, pObj, 0, 1, vDelay, pCost, pSuper, pMatrix, Vec_IntSize(vSuper), nLutSize, -1 );
            vSuper->nSize--;
            vCosts->nSize--;
            continue;
        }

        // compute affinity
        if ( Vec_IntSize(vSuper) == 64 )
            MatrixSize = Str_ManVectorAffinity( pNew, vSuper, vCosts, pMatrix, pObj->nFanins );
        assert( Vec_IntSize(vSuper) <= 64 );
//        Str_PrintState( pCost, pSuper, pMatrix, Vec_IntSize(vSuper) );

        // if the first two are PIs group them
        if ( pCost[0] == 17 && pCost[1] == 17 )
        {
            Str_NtkBalanceTwo( pNew, p, pObj, 0, 1, vDelay, pCost, pSuper, pMatrix, Vec_IntSize(vSuper), nLutSize, 2 );
            vSuper->nSize--;
            vCosts->nSize--;
            continue;
        }

        // find the end of the level
        for ( iStop = 0; iStop < Vec_IntSize(vSuper); iStop++ )
            if ( (pCost[iStop] >> 4) != (pCost[0] >> 4) )
                break;
        // if there is only one this level, pair it with the best match in the next level
        if ( iStop == 1 )
        {
            iBest = iStop, OccurBest = Str_CountBits(pMatrix[0] & pMatrix[iStop]);
            for ( i = iStop + 1; i < Vec_IntSize(vSuper); i++ )
            {
                if ( (pCost[i] >> 4) != (pCost[iStop] >> 4) )
                    break;
                OccurNew = Str_CountBits(pMatrix[0] & pMatrix[i]);
                if ( OccurBest < OccurNew )
                    iBest = i, OccurBest = OccurNew;
            }
            assert( iBest > 0 && iBest < Vec_IntSize(vSuper) );
            Str_NtkBalanceTwo( pNew, p, pObj, 0, iBest, vDelay, pCost, pSuper, pMatrix, Vec_IntSize(vSuper), nLutSize, -1 );
            vSuper->nSize--;
            vCosts->nSize--;
            continue;
        }
        // pair the first entry with another one on the same level
        iBest = -1; CostBest = -1; OccurBest2 = -1; OccurBest = -1;
        for ( i = 1; i < iStop; i++ )
        {
            CostNew = (pCost[0] & 15) + (pCost[i] & 15);
            if ( CostNew > nLutSize )
                continue;
            OccurNew = Str_CountBits(pMatrix[0] & pMatrix[i]);
            if ( CostBest < CostNew || (CostBest == CostNew && OccurBest < OccurNew) )
                CostBest = CostNew, iBest = i, OccurBest = OccurNew;
        }
        // if the best found is perfect, take it
        if ( CostBest == nLutSize )
        {
            assert( iBest > 0 && iBest < Vec_IntSize(vSuper) );
            Str_NtkBalanceTwo( pNew, p, pObj, 0, iBest, vDelay, pCost, pSuper, pMatrix, Vec_IntSize(vSuper), nLutSize, CostBest );
            vSuper->nSize--;
            vCosts->nSize--;
            continue;
        }
        // find the best pair on this level
        iBest = iBest2 = -1; CostBest = CostBest2 = -1, OccurBest = OccurBest2 = -1;
        for ( i = 0; i < iStop; i++ )
        for ( k = i+1; k < iStop; k++ )
        {
            CostNew  = (pCost[i] & 15) + (pCost[k] & 15);
            OccurNew = Str_CountBits(pMatrix[i] & pMatrix[k]);
            if ( CostNew <= nLutSize ) // the same level
            {
                if ( OccurBest < OccurNew || (OccurBest == OccurNew && CostBest < CostNew ))
                    CostBest = CostNew, iBest = (i << 16) | k, OccurBest = OccurNew;
            }
            else // overflow to the next level
            {
                if ( OccurBest2 < OccurNew || (OccurBest2 == OccurNew && CostBest2 < CostNew) )
                    CostBest2 = CostNew, iBest2 = (i << 16) | k, OccurBest2 = OccurNew;
            }
        }
        if ( iBest >= 0 )
        {
            assert( iBest > 0 );
            Str_NtkBalanceTwo( pNew, p, pObj, iBest>>16, iBest&0xFFFF, vDelay, pCost, pSuper, pMatrix, Vec_IntSize(vSuper), nLutSize, CostBest );
            vSuper->nSize--;
            vCosts->nSize--;
            continue;
        }
        // take any remaining pair
        assert( iBest2 > 0 );
        Str_NtkBalanceTwo( pNew, p, pObj, iBest2>>16, iBest2&0xFFFF, vDelay, pCost, pSuper, pMatrix, Vec_IntSize(vSuper), nLutSize, -1 );
        vSuper->nSize--;
        vCosts->nSize--;
        continue;
    }
    pObj->iCopy = Str_NtkBalanceTwo( pNew, p, pObj, 0, 1, vDelay, pCost, pSuper, pMatrix, 2, nLutSize, -1 );
    ABC_FREE( pMatrix );

/*
    // simple
    pObj->iCopy = (pObj->Type == STR_AND);
    for ( k = 0; k < Vec_IntSize(vSuper); k++ )
    {
        if ( pObj->Type == STR_AND )
            pObj->iCopy = Gia_ManHashAnd( pNew, pObj->iCopy, Vec_IntEntry(vSuper, k) );
        else
            pObj->iCopy = Gia_ManHashXorReal( pNew, pObj->iCopy, Vec_IntEntry(vSuper, k) );
        Str_ObjDelay( pNew, Abc_Lit2Var(pObj->iCopy), nLutSize, vDelay );
    }
*/
}
void Str_NtkBalanceMux( Gia_Man_t * pNew, Str_Ntk_t * p, Str_Obj_t * pObj, Vec_Int_t * vDelay, int nLutSize, int nGroups, int nMuxes, int fRecursive, int fOptArea, int fVerbose )
{
    extern int Str_MuxRestructure( Gia_Man_t * pNew, Str_Ntk_t * pNtk, int iMux, int nMuxes, Vec_Int_t * vDelay, int nLutSize, int fRecursive, int fOptArea, int fVerbose );
    int n, m, iRes, fUseRestruct = 1;
    if ( fUseRestruct )
    {
        for ( n = 0; n < nGroups; n++ )
        {            
            iRes = Str_MuxRestructure( pNew, p, Str_ObjId(p, pObj), nMuxes, vDelay, nLutSize, fRecursive, fOptArea, fVerbose );
            if ( iRes == -1 )
            {
                for ( m = 0; m < nMuxes; m++, pObj++ )
                {
                    pObj->iCopy = Gia_ManHashMuxReal( pNew, Str_ObjFaninCopy(p, pObj, 2), Str_ObjFaninCopy(p, pObj, 1), Str_ObjFaninCopy(p, pObj, 0) );
                    Str_ObjDelay( pNew, Abc_Lit2Var(pObj->iCopy), nLutSize, vDelay );
                }
            }
            else
            {
                pObj += nMuxes - 1;
                pObj->iCopy = iRes;
                pObj++;
            }
        }
    }
    else
    {
        for ( n = 0; n < nGroups * nMuxes; n++, pObj++ )
        {
            pObj->iCopy = Gia_ManHashMuxReal( pNew, Str_ObjFaninCopy(p, pObj, 2), Str_ObjFaninCopy(p, pObj, 1), Str_ObjFaninCopy(p, pObj, 0) );
            Str_ObjDelay( pNew, Abc_Lit2Var(pObj->iCopy), nLutSize, vDelay );
        }
    }
}
Gia_Man_t * Str_NtkBalance( Gia_Man_t * pGia, Str_Ntk_t * p, int nLutSize, int fUseMuxes, int fRecursive, int fOptArea, int fVerbose )
{
    Gia_Man_t * pNew, * pTemp;
    Vec_Int_t * vDelay;
    Str_Obj_t * pObj; 
    int nGroups, nMuxes, CioId;
    int arrTime, Delay = 0;
    assert( nLutSize < 16 );
    assert( pGia->pMuxes == NULL );
    pNew = Gia_ManStart( Gia_ManObjNum(pGia) );
    pNew->pName = Abc_UtilStrsav( pGia->pName );
    pNew->pSpec = Abc_UtilStrsav( pGia->pSpec );
    pNew->pMuxes = ABC_CALLOC( unsigned, pNew->nObjsAlloc );
    Vec_IntFill( &pNew->vCopies, pNew->nObjsAlloc, -1 );
    if ( pNew->vSuper == NULL )
        pNew->vSuper = Vec_IntAlloc( 1000 );
    if ( pNew->vStore == NULL )
        pNew->vStore = Vec_IntAlloc( 1000 );
    vDelay = Vec_IntStart( 2*pNew->nObjsAlloc );
    Gia_ManHashStart( pNew );
    if ( pGia->pManTime != NULL ) // Tim_Man with unit delay 16
    {
        Tim_ManInitPiArrivalAll( (Tim_Man_t *)pGia->pManTime, 17 );
        Tim_ManIncrementTravId( (Tim_Man_t *)pGia->pManTime );
    }
    Str_NtkManForEachObj( p, pObj )
    {
        if ( pObj->Type == STR_PI )
        {
            pObj->iCopy = Gia_ManAppendCi( pNew );
            arrTime = 17;
            if ( pGia->pManTime != NULL )
            {
                CioId = Gia_ObjCioId( Gia_ManObj(pNew, Abc_Lit2Var(pObj->iCopy)) );
                arrTime = (int)Tim_ManGetCiArrival( (Tim_Man_t *)pGia->pManTime, CioId );
            }
            Vec_IntWriteEntry( vDelay, Abc_Lit2Var(pObj->iCopy), arrTime );
        }
        else if ( pObj->Type == STR_AND || pObj->Type == STR_XOR )
            Str_NtkBalanceMulti( pNew, p, pObj, vDelay, nLutSize );
        else if ( pObj->Type == STR_MUX && pObj->iTop >= 0 && fUseMuxes )
        {
            Str_ObjReadGroup( p, pObj, &nGroups, &nMuxes );
            assert( nGroups * nMuxes >= 2 );
            Str_NtkBalanceMux( pNew, p, pObj, vDelay, nLutSize, nGroups, nMuxes, fRecursive, fOptArea, fVerbose );
            pObj += nGroups * nMuxes - 1;
        }
        else if ( pObj->Type == STR_MUX )
        {
            pObj->iCopy = Gia_ManHashMuxReal( pNew, Str_ObjFaninCopy(p, pObj, 2), Str_ObjFaninCopy(p, pObj, 1), Str_ObjFaninCopy(p, pObj, 0) );
            Str_ObjDelay( pNew, Abc_Lit2Var(pObj->iCopy), nLutSize, vDelay );
        }
        else if ( pObj->Type == STR_PO )
        {
            pObj->iCopy = Gia_ManAppendCo( pNew, Str_ObjFaninCopy(p, pObj, 0) );
            arrTime = Vec_IntEntry(vDelay, Abc_Lit2Var(Str_ObjFaninCopy(p, pObj, 0)) );
            Delay = Abc_MaxInt( Delay, arrTime );
            if ( pGia->pManTime != NULL )
            {
                CioId = Gia_ObjCioId( Gia_ManObj(pNew, Abc_Lit2Var(pObj->iCopy)) );
                Tim_ManSetCoArrival( (Tim_Man_t *)pGia->pManTime, CioId, (float)arrTime );
            }
        }
        else if ( pObj->Type == STR_CONST0 )
            pObj->iCopy = 0, Vec_IntWriteEntry(vDelay, 0, 17);
        else assert( 0 );
    }
    if ( fVerbose )
        printf( "Max delay = %d.  Old objs = %d.  New objs = %d.\n", Delay >> 4, Gia_ManObjNum(pGia), Gia_ManObjNum(pNew) );
    Vec_IntFree( vDelay );
    ABC_FREE( pNew->vCopies.pArray );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(pGia) );
    pNew = Gia_ManDupNoMuxes( pTemp = pNew, 0 );
    Gia_ManStop( pTemp );
//    if ( pGia->pManTime != NULL )
//        pNew->pManTime = Tim_ManDup( (Tim_Man_t *)pGia->pManTime, 0 );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Test normalization procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManLutBalance( Gia_Man_t * p, int nLutSize, int fUseMuxes, int fRecursive, int fOptArea, int fVerbose )
{
    Str_Ntk_t * pNtk;
    Gia_Man_t * pNew;
    abctime clk = Abc_Clock();
    if ( p->pManTime && Tim_ManBoxNum((Tim_Man_t*)p->pManTime) && Gia_ManIsNormalized(p) )
    {
        Tim_Man_t * pTimOld = (Tim_Man_t *)p->pManTime;
        p->pManTime = Tim_ManDup( pTimOld, 16 );
        pNew = Gia_ManDupUnnormalize( p );
        if ( pNew == NULL )
            return NULL;
        Gia_ManTransferTiming( pNew, p );
        p = pNew;
        // optimize
        pNtk = Str_ManNormalize( p );
        pNew = Str_NtkBalance( p, pNtk, nLutSize, fUseMuxes, fRecursive, fOptArea, fVerbose );
        Gia_ManTransferTiming( pNew, p );
        Gia_ManStop( p );
        // normalize
        pNew = Gia_ManDupNormalize( p = pNew, 0 );
        Gia_ManTransferTiming( pNew, p );
        Gia_ManStop( p );
        // cleanup
        Tim_ManStop( (Tim_Man_t *)pNew->pManTime );
        pNew->pManTime = pTimOld;
        assert( Gia_ManIsNormalized(pNew) );
    }
    else 
    {
        pNtk = Str_ManNormalize( p );
    //    Str_NtkPrintGroups( pNtk );
        pNew = Str_NtkBalance( p, pNtk, nLutSize, fUseMuxes, fRecursive, fOptArea, fVerbose );
        Gia_ManTransferTiming( pNew, p );
    }
    if ( fVerbose )
        Str_NtkPs( pNtk, Abc_Clock() - clk );
    Str_NtkDelete( pNtk );   
    return pNew;
}





/**Function*************************************************************

  Synopsis    [Perform MUX restructuring.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
typedef struct Str_Edg_t_ Str_Edg_t; 
struct Str_Edg_t_
{
    int            Fan;      // fanin ID
    int            fCompl;   // fanin complement
    int            FanDel;   // fanin delay
    int            Copy;     // fanin copy
};

typedef struct Str_Mux_t_ Str_Mux_t; // 64 bytes
struct Str_Mux_t_
{
    int            Id;       // node ID
    int            Delay;    // node delay
    int            Copy;     // node copy
    int            nLutSize; // LUT size
    Str_Edg_t      Edge[3];  // fanins
};

static inline Str_Mux_t * Str_MuxFanin( Str_Mux_t * pMux, int i )     { return pMux - pMux->Id + pMux->Edge[i].Fan;                         }
static inline int         Str_MuxHasFanin( Str_Mux_t * pMux, int i )  { return pMux->Edge[i].Fan > 0 && Str_MuxFanin(pMux, i)->Copy != -2;  }

void Str_MuxDelayPrint_rec( Str_Mux_t * pMux, int i )
{
    int fShowDelay = 1;
    Str_Mux_t * pFanin;
    if ( pMux->Edge[i].Fan <= 0 )
    {
        printf( "%d", -pMux->Edge[i].Fan );
        if ( fShowDelay )
            printf( "{%d}", pMux->Edge[i].FanDel );
        return;
    }
    pFanin = Str_MuxFanin( pMux, i );
    printf( "[ " );
    if ( pFanin->Edge[0].fCompl )
        printf( "!" );
    Str_MuxDelayPrint_rec( pFanin, 0 );
    printf( "|" );
    if ( pFanin->Edge[1].fCompl )
        printf( "!" );
    Str_MuxDelayPrint_rec( pFanin, 1 );
    printf( "(" );
    if ( pFanin->Edge[2].fCompl )
        printf( "!" );
    Str_MuxDelayPrint_rec( pFanin, 2 );
    printf( ")" );
    printf( " ]" );
}
int Str_MuxDelayEdge_rec( Str_Mux_t * pMux, int i )
{
    if ( pMux->Edge[i].Fan > 0 )
    {
        Str_Mux_t * pFanin = Str_MuxFanin( pMux, i );
        Str_MuxDelayEdge_rec( pFanin, 0 );
        Str_MuxDelayEdge_rec( pFanin, 1 );
        pMux->Edge[i].FanDel = Str_Delay3( pFanin->Edge[0].FanDel, pFanin->Edge[1].FanDel, pFanin->Edge[2].FanDel, pFanin->nLutSize );
    }
    return pMux->Edge[i].FanDel;
}
void Str_MuxCreate( Str_Mux_t * pTree, Str_Ntk_t * pNtk, int iMux, int nMuxes, Vec_Int_t * vDelay, int nLutSize )
{
    Str_Obj_t * pObj;
    Str_Mux_t * pMux;
    int i, k, nPis = 0;
    assert( nMuxes >= 2 );
    memset( pTree, 0, sizeof(Str_Mux_t) * (nMuxes + 1) );
    pTree->nLutSize = nLutSize;
    pTree->Edge[0].Fan = 1;
    for ( i = 1; i <= nMuxes; i++ )
    {
        pMux = pTree + i;
        pMux->Id = i;
        pMux->nLutSize = nLutSize;
        pMux->Delay = pMux->Copy = -1;
        // assign fanins
        pObj = Str_NtkObj( pNtk, iMux + nMuxes - i );
        assert( pObj->Type == STR_MUX );
        for ( k = 0; k < 3; k++ )
        {
            pMux->Edge[k].fCompl = Str_ObjFaninC(pNtk, pObj, k);
            if ( Str_ObjFaninId(pNtk, pObj, k) >= iMux )
                pMux->Edge[k].Fan = iMux + nMuxes - Str_ObjFaninId(pNtk, pObj, k);
            else
            {
                pMux->Edge[k].Fan = -nPis++; // count external inputs, including controls
                pMux->Edge[k].Copy = Str_ObjFanin(pNtk, pObj, k)->iCopy;
                pMux->Edge[k].FanDel = Vec_IntEntry( vDelay, Abc_Lit2Var(pMux->Edge[k].Copy) );
            }
        }
    }
}
int Str_MuxToGia_rec( Gia_Man_t * pNew, Str_Mux_t * pMux, int i, Vec_Int_t * vDelay )
{
    if ( pMux->Edge[i].Fan > 0 )
    {
        Str_Mux_t * pFanin = Str_MuxFanin( pMux, i );
        int iLit0 = Str_MuxToGia_rec( pNew, pFanin, 0, vDelay );
        int iLit1 = Str_MuxToGia_rec( pNew, pFanin, 1, vDelay );
        assert( pFanin->Edge[2].Fan <= 0 );
        assert( pFanin->Edge[2].fCompl == 0 );
        pMux->Edge[i].Copy = Gia_ManHashMuxReal( pNew, pFanin->Edge[2].Copy, iLit1, iLit0 );
        Str_ObjDelay( pNew, Abc_Lit2Var(pMux->Edge[i].Copy), pFanin->nLutSize, vDelay );
    }
    return Abc_LitNotCond( pMux->Edge[i].Copy, pMux->Edge[i].fCompl );
}
void Str_MuxChangeOnce( Str_Mux_t * pTree, int * pPath, int i, int k, Str_Mux_t * pBackup, Gia_Man_t * pNew, Vec_Int_t * vDelay )
{
    Str_Mux_t * pSpots[3];
    int pInds[3], MidFan, MidCom, MidDel, MidCop, c;
    int iRes, iCond, fCompl;
    // save backup
    assert( i + 1 < k );
    if ( pBackup )
    {
        pBackup[0] = pTree[ Abc_Lit2Var(pPath[k])  ];
        pBackup[1] = pTree[ Abc_Lit2Var(pPath[i+1])];
        pBackup[2] = pTree[ Abc_Lit2Var(pPath[i])  ];
    }
    // perform changes
    pSpots[0] = pTree + Abc_Lit2Var(pPath[k]); 
    pSpots[1] = pTree + Abc_Lit2Var(pPath[i+1]); 
    pSpots[2] = pTree + Abc_Lit2Var(pPath[i]); 
    pInds[0] = Abc_LitIsCompl(pPath[k]);
    pInds[1] = Abc_LitIsCompl(pPath[i+1]);
    pInds[2] = Abc_LitIsCompl(pPath[i]);
    // check
    assert( pSpots[0]->Edge[pInds[0]].Fan > 0 );
    assert( pSpots[1]->Edge[pInds[1]].Fan > 0 );
    // collect complement
    fCompl = 0;
    for ( c = i+1; c < k; c++ )
        fCompl ^= pTree[Abc_Lit2Var(pPath[c])].Edge[Abc_LitIsCompl(pPath[c])].fCompl;
    // remember bottom side
    MidFan = pSpots[2]->Edge[!pInds[2]].Fan;
    MidCom = pSpots[2]->Edge[!pInds[2]].fCompl;
    MidDel = pSpots[2]->Edge[!pInds[2]].FanDel;
    MidCop = pSpots[2]->Edge[!pInds[2]].Copy;
    // update bottom
    pSpots[2]->Edge[!pInds[2]].Fan    = pSpots[0]->Edge[pInds[0]].Fan;
    pSpots[2]->Edge[!pInds[2]].fCompl = 0;
    // update top 
    pSpots[0]->Edge[pInds[0]].Fan     = pSpots[2]->Id;
    // update middle
    pSpots[1]->Edge[pInds[1]].Fan     = MidFan;
    pSpots[1]->Edge[pInds[1]].fCompl ^= MidCom;
    pSpots[1]->Edge[pInds[1]].FanDel  = MidDel;
    pSpots[1]->Edge[pInds[1]].Copy    = MidCop;
    // update delay of the control
    for ( c = i + 1; c < k; c++ )
        pSpots[2]->Edge[2].FanDel = Str_Delay2( pSpots[2]->Edge[2].FanDel, pTree[Abc_Lit2Var(pPath[c])].Edge[2].FanDel, pTree->nLutSize );
    if ( pNew == NULL )
        return;
    // create AND gates
    iRes = 1;
    for ( c = i; c < k; c++ )
    {
        assert( pTree[Abc_Lit2Var(pPath[c])].Edge[2].fCompl == 0 );
        iCond = pTree[Abc_Lit2Var(pPath[c])].Edge[2].Copy;
        iCond = Abc_LitNotCond( iCond, !Abc_LitIsCompl(pPath[c]) );
        iRes  = Gia_ManHashAnd( pNew, iRes, iCond );
        Str_ObjDelay( pNew, Abc_Lit2Var(iRes), pTree->nLutSize, vDelay );
    }
    // complement the condition
    pSpots[2]->Edge[2].Copy = Abc_LitNotCond( iRes, !Abc_LitIsCompl(pPath[i]) );
    // complement the path
    pSpots[2]->Edge[pInds[2]].fCompl ^= fCompl;
}
void Str_MuxChangeUndo( Str_Mux_t * pTree, int * pPath, int i, int k, Str_Mux_t * pBackup )
{
    pTree[ Abc_Lit2Var(pPath[k])  ] = pBackup[0]; 
    pTree[ Abc_Lit2Var(pPath[i+1])] = pBackup[1]; 
    pTree[ Abc_Lit2Var(pPath[i])  ] = pBackup[2]; 
}
int Str_MuxFindPathEdge_rec( Str_Mux_t * pMux, int i, int * pPath, int * pnLength )
{
    extern int Str_MuxFindPath_rec( Str_Mux_t * pMux, int * pPath, int * pnLength );
    if ( pMux->Edge[i].Fan > 0 && !Str_MuxFindPath_rec(Str_MuxFanin(pMux, i), pPath, pnLength) )
        return 0;
    pPath[ (*pnLength)++ ] = Abc_Var2Lit(pMux->Id, i);
    return 1;
}
int Str_MuxFindPath_rec( Str_Mux_t * pMux, int * pPath, int * pnLength )
{
    int i, DelayMax = Abc_MaxInt( pMux->Edge[0].FanDel, Abc_MaxInt(pMux->Edge[1].FanDel, pMux->Edge[2].FanDel) );
    for ( i = 0; i < 2; i++ )
        if ( pMux->Edge[i].FanDel == DelayMax )
            return Str_MuxFindPathEdge_rec( pMux, i, pPath, pnLength );
    if ( pMux->Edge[2].FanDel == DelayMax )
        return 0;
    assert( 0 );
    return -1;
}
// return node whose both branches are non-trivial
Str_Mux_t * Str_MuxFindBranching( Str_Mux_t * pRoot, int i )
{
    Str_Mux_t * pMux;
    if ( pRoot->Edge[i].Fan <= 0 )
        return NULL;
    pMux = Str_MuxFanin( pRoot, i );
    while ( 1 )
    {
        if ( pMux->Edge[0].Fan <= 0 && pMux->Edge[1].Fan <= 0 )
            return NULL;
        if ( pMux->Edge[0].Fan > 0 && pMux->Edge[1].Fan > 0 )
            return pMux;
        if ( pMux->Edge[0].Fan > 0 )
            pMux = Str_MuxFanin( pMux, 0 );
        if ( pMux->Edge[1].Fan > 0 )
            pMux = Str_MuxFanin( pMux, 1 );
    }
    assert( 0 );
    return NULL;
}
int Str_MuxTryOnce( Gia_Man_t * pNew, Str_Ntk_t * pNtk, Str_Mux_t * pTree, Str_Mux_t * pRoot, int Edge, Vec_Int_t * vDelay, int fVerbose )
{
    int pPath[MAX_TREE];
    Str_Mux_t pBackup[3];
    int Delay, DelayBest = Str_MuxDelayEdge_rec( pRoot, Edge ), DelayInit = DelayBest;
    int i, k, nLength = 0, ForkBest = -1, nChecks = 0;
    int RetValue = Str_MuxFindPathEdge_rec( pRoot, Edge, pPath, &nLength );
    if ( RetValue == 0 )
        return 0;
    if ( fVerbose )
        printf( "Trying node %d with path of length %d.\n", pRoot->Id, nLength );
    for ( i = 0; i < nLength; i++ )
    for ( k = i+2; k < nLength; k++ )
    {
        Str_MuxChangeOnce( pTree, pPath, i, k, pBackup, NULL, NULL );
        Delay = Str_MuxDelayEdge_rec( pRoot, Edge );
        Str_MuxChangeUndo( pTree, pPath, i, k, pBackup );
        if ( DelayBest > Delay || (ForkBest > 0 && DelayBest == Delay) )
            DelayBest = Delay, ForkBest = (i << 16) | k;
        if ( fVerbose )
            printf( "%2d %2d -> %3d (%3d)\n", i, k, Delay, DelayBest );
        nChecks++;
    }
    if ( ForkBest == -1 )
    {
        if ( fVerbose )
            printf( "Did not find!\n" );
        return 0;
    }
//    Str_MuxDelayPrint_rec( pRoot, Edge ); printf( "\n" );
    Str_MuxChangeOnce( pTree, pPath, ForkBest >> 16, ForkBest & 0xFFFF, NULL, pNew, vDelay );
//    Str_MuxDelayPrint_rec( pRoot, Edge ); printf( "\n" );
    if ( fVerbose )
        printf( "Node %6d (%3d %3d) : Checks = %d. Delay: %d -> %d.\n", 
            pRoot->Id, ForkBest >> 16, ForkBest & 0xFFFF, nChecks, DelayInit, DelayBest );
    if ( fVerbose )
        printf( "\n" );
    return 1;
}
int Str_MuxRestruct_rec( Gia_Man_t * pNew, Str_Ntk_t * pNtk, Str_Mux_t * pTree, Str_Mux_t * pRoot, int Edge, Vec_Int_t * vDelay, int fVerbose )
{
    int fChanges = 0;
    Str_Mux_t * pMux = Str_MuxFindBranching( pRoot, Edge );
    if ( pMux != NULL )
        fChanges |= Str_MuxRestruct_rec( pNew, pNtk, pTree, pMux, 0, vDelay, fVerbose );
    if ( pMux != NULL )
        fChanges |= Str_MuxRestruct_rec( pNew, pNtk, pTree, pMux, 1, vDelay, fVerbose );
    fChanges |= Str_MuxTryOnce( pNew, pNtk, pTree, pRoot, Edge, vDelay, fVerbose );
    return fChanges;
}
int Str_MuxRestructure2( Gia_Man_t * pNew, Str_Ntk_t * pNtk, int iMux, int nMuxes, Vec_Int_t * vDelay, int nLutSize, int fVerbose )
{
    int Limit = MAX_TREE;
    Str_Mux_t pTree[MAX_TREE];
    int Delay, Delay2, fChanges = 0;
    if ( nMuxes >= Limit )
        return -1;
    assert( nMuxes < Limit );
    Str_MuxCreate( pTree, pNtk, iMux, nMuxes, vDelay, nLutSize );
    Delay = Str_MuxDelayEdge_rec( pTree, 0 );
    while ( 1 )
    {     
        if ( !Str_MuxRestruct_rec(pNew, pNtk, pTree, pTree, 0, vDelay, fVerbose) )
            break;
        fChanges = 1;
    }
    if ( !fChanges )
        return -1;
    Delay2 = Str_MuxDelayEdge_rec( pTree, 0 );
//    printf( "Improved delay for tree %d with %d MUXes (%d -> %d).\n", iMux, nMuxes, Delay, Delay2 );
    pNtk->DelayGain += Delay - Delay2;
    return Str_MuxToGia_rec( pNew, pTree, 0, vDelay );
}
int Str_MuxRestructure1( Gia_Man_t * pNew, Str_Ntk_t * pNtk, int iMux, int nMuxes, Vec_Int_t * vDelay, int nLutSize, int fVerbose )
{
    int Limit = MAX_TREE;
    Str_Mux_t pTree[MAX_TREE];
    int Delay, Delay2, fChanges = 0;
    if ( nMuxes >= Limit )
        return -1;
    assert( nMuxes < Limit );
    Str_MuxCreate( pTree, pNtk, iMux, nMuxes, vDelay, nLutSize );
    Delay = Str_MuxDelayEdge_rec( pTree, 0 );
    while ( 1 )
    {     
        if ( !Str_MuxTryOnce(pNew, pNtk, pTree, pTree, 0, vDelay, fVerbose) )
            break;
        fChanges = 1;
    }
    if ( !fChanges )
        return -1;
    Delay2 = Str_MuxDelayEdge_rec( pTree, 0 );
//    printf( "Improved delay for tree %d with %d MUXes (%d -> %d).\n", iMux, nMuxes, Delay, Delay2 );
    pNtk->DelayGain += Delay - Delay2;
    return Str_MuxToGia_rec( pNew, pTree, 0, vDelay );
}
int Str_MuxRestructure( Gia_Man_t * pNew, Str_Ntk_t * pNtk, int iMux, int nMuxes, Vec_Int_t * vDelay, int nLutSize, int fRecursive, int fOptArea, int fVerbose )
{
    extern int Str_MuxRestructureArea( Gia_Man_t * pNew, Str_Ntk_t * pNtk, int iMux, int nMuxes, Vec_Int_t * vDelay, int nLutSize, int fVerbose );
    if ( fOptArea )
    {
        if ( nMuxes < 2 )
            return Str_MuxRestructure1( pNew, pNtk, iMux, nMuxes, vDelay, nLutSize, fVerbose );
        return Str_MuxRestructureArea( pNew, pNtk, iMux, nMuxes, vDelay, nLutSize, fVerbose );
    }
    if ( fRecursive )
        return Str_MuxRestructure2( pNew, pNtk, iMux, nMuxes, vDelay, nLutSize, fVerbose );
    return Str_MuxRestructure1( pNew, pNtk, iMux, nMuxes, vDelay, nLutSize, fVerbose );
}

/**Function*************************************************************

  Synopsis    [Perform MUX restructuring for area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Str_MuxRestructAreaThree( Gia_Man_t * pNew, Str_Mux_t * pMux, Vec_Int_t * vDelay, int fVerbose )
{
    int iRes;
    Str_Mux_t * pFanin0 = Str_MuxFanin( pMux, 0 );
    Str_Mux_t * pFanin1 = Str_MuxFanin( pMux, 1 );
    assert( pMux->Copy == -1 );
    pMux->Copy = -2;
    if ( pFanin0->Edge[2].Copy == pFanin1->Edge[2].Copy )
        return 0;
    iRes = Gia_ManHashMuxReal( pNew, pMux->Edge[2].Copy, pFanin1->Edge[2].Copy, pFanin0->Edge[2].Copy );
    Str_ObjDelay( pNew, Abc_Lit2Var(iRes), pMux->nLutSize, vDelay );
    pFanin0->Edge[2].Copy = pFanin1->Edge[2].Copy = iRes;
//    printf( "Created triple\n" );
    return 0;
}
int Str_MuxRestructArea_rec( Gia_Man_t * pNew, Str_Mux_t * pTree, Str_Mux_t * pRoot, int i, Vec_Int_t * vDelay, int fVerbose )
{
    int Path[4];
    int fSkipMoving = 1;
    Str_Mux_t * pMux, * pFanin0, * pFanin1;
    int nMuxes0, nMuxes1;
    if ( pRoot->Edge[i].Fan <= 0 )
        return 0;
    pMux    = Str_MuxFanin( pRoot, i );
    nMuxes0 = Str_MuxRestructArea_rec( pNew, pTree, pMux, 0, vDelay, fVerbose );
    nMuxes1 = Str_MuxRestructArea_rec( pNew, pTree, pMux, 1, vDelay, fVerbose );
    if ( nMuxes0 + nMuxes1 < 2 )
        return 1 + nMuxes0 + nMuxes1;
    if ( nMuxes0 + nMuxes1 == 2 )
    {
        if ( nMuxes0 == 2 || nMuxes1 == 2 )
        {
            pFanin0 = Str_MuxFanin( pMux, (int)(nMuxes1 == 2) );
            assert( Str_MuxHasFanin(pFanin0, 0) != Str_MuxHasFanin(pFanin0, 1) );
            Path[2] = Abc_Var2Lit(pRoot->Id, i);
            Path[1] = Abc_Var2Lit(pMux->Id, (int)(nMuxes1 == 2) );
            Path[0] = Abc_Var2Lit(pFanin0->Id, Str_MuxHasFanin(pFanin0, 1));
            Str_MuxChangeOnce( pTree, Path, 0, 2, NULL, pNew, vDelay );
        }
        Str_MuxRestructAreaThree( pNew, Str_MuxFanin(pRoot, i), vDelay, fVerbose );
        return 0;
    }
    assert( nMuxes0 + nMuxes1 == 3 || nMuxes0 + nMuxes1 == 4 );
    assert( nMuxes0 == 2 || nMuxes1 == 2 );
    if ( fSkipMoving )
    {
        Str_MuxRestructAreaThree( pNew, pMux, vDelay, fVerbose );
        return 0;
    }
    if ( nMuxes0 == 2 )
    {
        pFanin0 = Str_MuxFanin( pMux, 0 );
        assert( Str_MuxHasFanin(pFanin0, 0) != Str_MuxHasFanin(pFanin0, 1) );
        Path[3] = Abc_Var2Lit(pRoot->Id, i);
        Path[2] = Abc_Var2Lit(pMux->Id, 0 );
        Path[1] = Abc_Var2Lit(pFanin0->Id, Str_MuxHasFanin(pFanin0, 1));
        pFanin1 = Str_MuxFanin( pFanin0, Str_MuxHasFanin(pFanin0, 1) );
        assert( !Str_MuxHasFanin(pFanin1, 0) && !Str_MuxHasFanin(pFanin1, 1) );
        Path[0] = Abc_Var2Lit(pFanin1->Id, 0);
        Str_MuxChangeOnce( pTree, Path, 0, 3, NULL, pNew, vDelay );
    }
    if ( nMuxes1 == 2 )
    {
        pFanin0 = Str_MuxFanin( pMux, 1 );
        assert( Str_MuxHasFanin(pFanin0, 0) != Str_MuxHasFanin(pFanin0, 1) );
        Path[3] = Abc_Var2Lit(pRoot->Id, i);
        Path[2] = Abc_Var2Lit(pMux->Id, 1 );
        Path[1] = Abc_Var2Lit(pFanin0->Id, Str_MuxHasFanin(pFanin0, 1));
        pFanin1 = Str_MuxFanin( pFanin0, Str_MuxHasFanin(pFanin0, 1) );
        assert( !Str_MuxHasFanin(pFanin1, 0) && !Str_MuxHasFanin(pFanin1, 1) );
        Path[0] = Abc_Var2Lit(pFanin1->Id, 0);
        Str_MuxChangeOnce( pTree, Path, 0, 3, NULL, pNew, vDelay );
    }
    Str_MuxRestructAreaThree( pNew, pMux, vDelay, fVerbose );
    return nMuxes0 + nMuxes1 - 2;
}
int Str_MuxRestructureArea( Gia_Man_t * pNew, Str_Ntk_t * pNtk, int iMux, int nMuxes, Vec_Int_t * vDelay, int nLutSize, int fVerbose )
{
    int Limit = MAX_TREE;
    Str_Mux_t pTree[MAX_TREE];
    int Result;
    if ( nMuxes >= Limit )
        return -1;
    assert( nMuxes < Limit );
    Str_MuxCreate( pTree, pNtk, iMux, nMuxes, vDelay, nLutSize );
    Result = Str_MuxRestructArea_rec( pNew, pTree, pTree, 0, vDelay, fVerbose );
    assert( Result >= 0 && Result <= 2 );
    return Str_MuxToGia_rec( pNew, pTree, 0, vDelay );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

