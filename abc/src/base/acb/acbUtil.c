/**CFile****************************************************************

  FileName    [acbUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Various utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 21, 2015.]

  Revision    [$Id: acbUtil.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acb.h"
#include "base/abc/abc.h"
#include "base/io/ioAbc.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collecting TFI and TFO.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_ObjCollectTfi_rec( Acb_Ntk_t * p, int iObj, int fTerm )
{
    int * pFanin, iFanin, i;
    if ( Acb_ObjSetTravIdCur(p, iObj) )
        return;
    if ( !fTerm && Acb_ObjIsCi(p, iObj) )
        return;
    Acb_ObjForEachFaninFast( p, iObj, pFanin, iFanin, i )
        Acb_ObjCollectTfi_rec( p, iFanin, fTerm );
    Vec_IntPush( &p->vArray0, iObj );
}
Vec_Int_t * Acb_ObjCollectTfi( Acb_Ntk_t * p, int iObj, int fTerm )
{
    int i, Node;
    Vec_IntClear( &p->vArray0 );
    Acb_NtkIncTravId( p );
    if ( iObj > 0 )
    {
        Vec_IntForEachEntry( &p->vSuppOld, Node, i )
            Acb_ObjCollectTfi_rec( p, Node, fTerm );
        Acb_ObjCollectTfi_rec( p, iObj, fTerm );
    }
    else
        Acb_NtkForEachCo( p, iObj, i )
            Acb_ObjCollectTfi_rec( p, iObj, fTerm );
    return &p->vArray0;
}
Vec_Int_t * Acb_ObjCollectTfiVec( Acb_Ntk_t * p, Vec_Int_t * vObjs )
{
    int i, iObj;
    Vec_IntClear( &p->vArray0 );
    Acb_NtkIncTravId( p );
    Vec_IntForEachEntry( vObjs, iObj, i )
        Acb_ObjCollectTfi_rec( p, iObj, 0 );
    return &p->vArray0;
}

void Acb_ObjCollectTfo_rec( Acb_Ntk_t * p, int iObj, int fTerm )
{
    int iFanout, i;
    if ( Acb_ObjSetTravIdCur(p, iObj) )
        return;
    if ( !fTerm && Acb_ObjIsCo(p, iObj) )
        return;
    Acb_ObjForEachFanout( p, iObj, iFanout, i )
        Acb_ObjCollectTfo_rec( p, iFanout, fTerm );
    Vec_IntPush( &p->vArray1, iObj );
}
Vec_Int_t * Acb_ObjCollectTfo( Acb_Ntk_t * p, int iObj, int fTerm )
{
    int i;
    Vec_IntClear( &p->vArray1 );
    Acb_NtkIncTravId( p );
    if ( iObj > 0 )
        Acb_ObjCollectTfo_rec( p, iObj, fTerm );
    else
        Acb_NtkForEachCi( p, iObj, i )
            Acb_ObjCollectTfo_rec( p, iObj, fTerm );
    return &p->vArray1;
}
Vec_Int_t * Acb_ObjCollectTfoVec( Acb_Ntk_t * p, Vec_Int_t * vObjs )
{
    int i, iObj;
    if ( !Acb_NtkHasObjFanout(p) )
        Acb_NtkCreateFanout( p );
    Vec_IntClear( &p->vArray1 );
    Acb_NtkIncTravId( p );
    Vec_IntForEachEntry( vObjs, iObj, i )
        Acb_ObjCollectTfo_rec( p, iObj, 0 );
    return &p->vArray1;
}

/**Function*************************************************************

  Synopsis    [Count the number of nodes driving the POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Acb_NtkIsPiBuffers( Acb_Ntk_t * p, int iObj )
{
    if ( Acb_ObjIsCi(p, iObj) )
        return 1;
    if ( Acb_ObjFaninNum(p, iObj) != 1 )
        return 0;
    return Acb_NtkIsPiBuffers( p, Acb_ObjFanin(p, iObj, 0) );
}
int Acb_NtkCountPiBuffers( Acb_Ntk_t * p, Vec_Int_t * vObjs )
{
    int i, iObj, Count = 0;
    Vec_IntForEachEntry( vObjs, iObj, i )
        Count += Acb_NtkIsPiBuffers( p, iObj );
    return Count;
}
int Acb_NtkCountPoDrivers( Acb_Ntk_t * p, Vec_Int_t * vObjs )
{
    int i, iObj, Count = 0;
    Acb_NtkIncTravId( p );
    Acb_NtkForEachCo( p, iObj, i )
    {
        int Fanin0 = Acb_ObjFanin0(p, iObj);
        Acb_ObjSetTravIdCur( p, iObj );
        Acb_ObjSetTravIdCur( p, Fanin0 );
        if ( Acb_ObjFaninNum(p, Fanin0) == 1 )
            Acb_ObjSetTravIdCur( p, Acb_ObjFanin0(p, Fanin0) );
    }
    Vec_IntForEachEntry( vObjs, iObj, i )
        Count += Acb_ObjIsTravIdCur(p, iObj);
    return Count;
}

/**Function*************************************************************

  Synopsis    [Compute MFFC size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Acb_NtkNodeDeref_rec( Vec_Int_t * vRefs, Acb_Ntk_t * p, int iObj )
{
    int i, Fanin, * pFanins, Counter = 1;
    if ( Acb_ObjIsCi(p, iObj) )
        return 0;
    Acb_ObjForEachFaninFast( p, iObj, pFanins, Fanin, i )
    {
        assert( Vec_IntEntry(vRefs, Fanin) > 0 );
        Vec_IntAddToEntry( vRefs, Fanin, -1 );
        if ( Vec_IntEntry(vRefs, Fanin) == 0 )
            Counter += Acb_NtkNodeDeref_rec( vRefs, p, Fanin );
    }
    return Counter;
}
int Acb_NtkNodeRef_rec( Vec_Int_t * vRefs, Acb_Ntk_t * p, int iObj )
{
    int i, Fanin, * pFanins, Counter = 1;
    if ( Acb_ObjIsCi(p, iObj) )
        return 0;
    Acb_ObjForEachFaninFast( p, iObj, pFanins, Fanin, i )
    {
        if ( Vec_IntEntry(vRefs, Fanin) == 0 )
            Counter += Acb_NtkNodeRef_rec( vRefs, p, Fanin );
        Vec_IntAddToEntry( vRefs, Fanin, 1 );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Computing and updating direct and reverse logic level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_NtkCollectDeref_rec( Vec_Int_t * vRefs, Acb_Ntk_t * p, int iObj, Vec_Int_t * vRes )
{
    int i, Fanin, * pFanins;
    if ( Acb_ObjIsCi(p, iObj) )
        return;
    Vec_IntPush( vRes, iObj );
    Acb_ObjForEachFaninFast( p, iObj, pFanins, Fanin, i )
    {
        assert( Vec_IntEntry(vRefs, Fanin) > 0 );
        Vec_IntAddToEntry( vRefs, Fanin, -1 );
        if ( Vec_IntEntry(vRefs, Fanin) == 0 )
            Acb_NtkCollectDeref_rec( vRefs, p, Fanin, vRes );
    }
}
Vec_Int_t * Acb_NtkCollectMffc( Acb_Ntk_t * p, Vec_Int_t * vObjsRefed, Vec_Int_t * vObjsDerefed )
{
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    Vec_Int_t * vRefs = Vec_IntStart( Acb_NtkObjNumMax(p) );
    int i, iObj, Fanin, * pFanins;
    Acb_NtkForEachObj( p, iObj )
        Acb_ObjForEachFaninFast( p, iObj, pFanins, Fanin, i )
            Vec_IntAddToEntry( vRefs, Fanin, 1 );
    Acb_NtkForEachCo( p, iObj, i )
        Vec_IntAddToEntry( vRefs, iObj, 1 );
    if ( vObjsRefed )
        Vec_IntForEachEntry( vObjsRefed, iObj, i )
            Vec_IntAddToEntry( vRefs, iObj, 1 );
    Vec_IntForEachEntry( vObjsDerefed, iObj, i )
    {
        if ( Acb_ObjIsCo(p, iObj) )
            iObj = Acb_ObjFanin0(p, iObj);
        if ( Vec_IntEntry(vRefs, iObj) != 0 )
            Acb_NtkCollectDeref_rec( vRefs, p, iObj, vRes );
    }
    Vec_IntFree( vRefs );
    Vec_IntUniqify( vRes );
    return vRes;
}

Vec_Int_t * Acb_NamesToIds( Acb_Ntk_t * pNtk, Vec_Int_t * vNamesInv, Vec_Ptr_t * vNames )
{
    Vec_Int_t * vRes = Vec_IntAlloc( Vec_PtrSize(vNames) );
    char * pName; int i;
    Vec_PtrForEachEntry( char *, vNames, pName, i )
    {
        int NameId = Acb_NtkStrId(pNtk, pName);
        int iObjId = 0;
        if ( NameId < 1 )
            printf( "Cannot find name \"%s\" in the network \"%s\".\n", pName, pNtk->pDesign->pName );
        else
            iObjId = Vec_IntEntry( vNamesInv, NameId );
        Vec_IntPush( vRes, iObjId );
    }
    return vRes;
}
int Acb_NtkCollectMfsGates( char * pFileName, Vec_Ptr_t * vNamesRefed, Vec_Ptr_t * vNamesDerefed, int nGates[5] )
{
    Acb_Ntk_t * pNtkF        = Acb_VerilogSimpleRead( pFileName, NULL );
    Vec_Int_t * vNamesInv    = Vec_IntInvert( &pNtkF->vObjName, 0 ) ;
    Vec_Int_t * vObjsRefed   = Acb_NamesToIds( pNtkF, vNamesInv, vNamesRefed );
    Vec_Int_t * vObjsDerefed = Acb_NamesToIds( pNtkF, vNamesInv, vNamesDerefed );
    Vec_Int_t * vNodes       = Acb_NtkCollectMffc( pNtkF, vObjsRefed, vObjsDerefed );
    int i, iObj, RetValue    = Vec_IntSize(vNodes);
    Vec_IntFree( vNamesInv );
    Vec_IntFree( vObjsRefed );
    Vec_IntFree( vObjsDerefed );
    for ( i = 0; i < 5; i++ )
        nGates[i] = 0;
    Vec_IntForEachEntry( vNodes, iObj, i )
    {
        int nFan = Acb_ObjFaninNum(pNtkF, iObj);
        int Type = Acb_ObjType( pNtkF, iObj );
        if ( Type == ABC_OPER_CONST_F ) 
            nGates[0]++;
        else if ( Type == ABC_OPER_CONST_T ) 
            nGates[1]++;
        else if ( Type == ABC_OPER_BIT_BUF || Type == ABC_OPER_CO ) 
            nGates[2]++;
        else if ( Type == ABC_OPER_BIT_INV ) 
            nGates[3]++;
        else
        {
            assert( nFan >= 2 );
            nGates[4] += Acb_ObjFaninNum(pNtkF, iObj)-1;
        }
    }
    Vec_IntFree( vNodes );
    Acb_ManFree( pNtkF->pDesign );
    return RetValue;
}
Vec_Ptr_t * Acb_NtkReturnMfsGates( char * pFileName, Vec_Ptr_t * vNodes )
{
    Vec_Ptr_t * vMffc     = Vec_PtrAlloc( 100 );
    Acb_Ntk_t * pNtkF     = Acb_VerilogSimpleRead( pFileName, NULL );
    Vec_Int_t * vNamesInv = Vec_IntInvert( &pNtkF->vObjName, 0 ) ;
    Vec_Int_t * vNodeObjs = Acb_NamesToIds( pNtkF, vNamesInv, vNodes );
    Vec_Int_t * vNodeMffc = Acb_NtkCollectMffc( pNtkF, NULL, vNodeObjs );
    int i, iObj;
    Vec_IntForEachEntry( vNodeMffc, iObj, i )
        Vec_PtrPush( vMffc, Abc_UtilStrsav( Acb_ObjNameStr(pNtkF, iObj) ) );
//Vec_IntPrint( vNodeMffc );
//Vec_PtrPrintNames( vMffc );
    Vec_IntFree( vNodeMffc );
    Vec_IntFree( vNodeObjs );
    Vec_IntFree( vNamesInv );
    Acb_ManFree( pNtkF->pDesign );
    return vMffc;
}

/**Function*************************************************************

  Synopsis    [Computing and updating direct and reverse logic level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Acb_ObjComputeLevelD( Acb_Ntk_t * p, int iObj )
{
    int * pFanins, iFanin, k, Level = 0;
    Acb_ObjForEachFaninFast( p, iObj, pFanins, iFanin, k )
        Level = Abc_MaxInt( Level, Acb_ObjLevelD(p, iFanin) );
    return Acb_ObjSetLevelD( p, iObj, Level + !Acb_ObjIsCio(p, iObj) );
}
int Acb_NtkComputeLevelD( Acb_Ntk_t * p, Vec_Int_t * vTfo )
{
    // it is assumed that vTfo contains CO nodes and level of new nodes was already updated
    int i, iObj, Level = 0;
    if ( !Acb_NtkHasObjLevelD( p ) )
        Acb_NtkCleanObjLevelD( p );
    Vec_IntForEachEntryReverse( vTfo, iObj, i )
        Acb_ObjComputeLevelD( p, iObj );
    Acb_NtkForEachCo( p, iObj, i )
        Level = Abc_MaxInt( Level, Acb_ObjLevelD(p, iObj) );
    p->LevelMax = Level;
    return Level;
}

int Acb_ObjComputeLevelR( Acb_Ntk_t * p, int iObj )
{
    int iFanout, k, Level = 0;
    Acb_ObjForEachFanout( p, iObj, iFanout, k )
        Level = Abc_MaxInt( Level, Acb_ObjLevelR(p, iFanout) );
    return Acb_ObjSetLevelR( p, iObj, Level + !Acb_ObjIsCio(p, iObj) );
}
int Acb_NtkComputeLevelR( Acb_Ntk_t * p, Vec_Int_t * vTfi )
{
    // it is assumed that vTfi contains CI nodes
    int i, iObj, Level = 0;
    if ( !Acb_NtkHasObjLevelR( p ) )
        Acb_NtkCleanObjLevelR( p );
    Vec_IntForEachEntryReverse( vTfi, iObj, i )
        Acb_ObjComputeLevelR( p, iObj );
    Acb_NtkForEachCi( p, iObj, i )
        Level = Abc_MaxInt( Level, Acb_ObjLevelR(p, iObj) );
//    assert( p->LevelMax == Level );
    p->LevelMax = Level;
    return Level;
}

void Acb_NtkUpdateLevelD( Acb_Ntk_t * p, int Pivot )
{
    Vec_Int_t * vTfo = Acb_ObjCollectTfo( p, Pivot, 1 );
    Acb_NtkComputeLevelD( p, vTfo );
}

/**Function*************************************************************

  Synopsis    [Computing and updating direct and reverse path count.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Acb_ObjSlack( Acb_Ntk_t * p, int iObj )
{
    int LevelSum = Acb_ObjLevelD(p, iObj) + Acb_ObjLevelR(p, iObj);
    assert( !Acb_ObjIsCio(p, iObj) + p->LevelMax >= LevelSum );
    return !Acb_ObjIsCio(p, iObj) + p->LevelMax - LevelSum;
}

int Acb_ObjComputePathD( Acb_Ntk_t * p, int iObj )
{
    int * pFanins, iFanin, k, Path = 0;
    assert( !Acb_ObjIsCi(p, iObj) );
    Acb_ObjForEachFaninFast( p, iObj, pFanins, iFanin, k )
        if ( !Acb_ObjSlack(p, iFanin) )
            Path += Acb_ObjPathD(p, iFanin);
    return Acb_ObjSetPathD( p, iObj, Path );
}
int Acb_NtkComputePathsD( Acb_Ntk_t * p, Vec_Int_t * vTfo, int fReverse )
{
    int i, iObj, Path = 0;
    //Vec_IntPrint( vTfo );
    if ( !Acb_NtkHasObjPathD( p ) )
        Acb_NtkCleanObjPathD( p );
    // it is assumed that vTfo contains CI nodes
    //assert( Acb_ObjSlack(p, Vec_IntEntry(vTfo, 0)) );
    if ( fReverse )
    {
        Vec_IntForEachEntryReverse( vTfo, iObj, i )
        {
            if ( Acb_ObjIsCi(p, iObj) )
                Acb_ObjSetPathD( p, iObj, Acb_ObjSlack(p, iObj) == 0 );
            else if ( Acb_ObjSlack(p, iObj) )
                Acb_ObjSetPathD( p, iObj, 0 );
            else
                Acb_ObjComputePathD( p, iObj );
        }
    }
    else
    {
        Vec_IntForEachEntry( vTfo, iObj, i )
        {
            if ( Acb_ObjIsCi(p, iObj) )
                Acb_ObjSetPathD( p, iObj, Acb_ObjSlack(p, iObj) == 0 );
            else if ( Acb_ObjSlack(p, iObj) )
                Acb_ObjSetPathD( p, iObj, 0 );
            else
                Acb_ObjComputePathD( p, iObj );
        }
    }
    Acb_NtkForEachCo( p, iObj, i )
        Path += Acb_ObjPathD(p, iObj);
    p->nPaths = Path;
    return Path;
}

int Acb_ObjComputePathR( Acb_Ntk_t * p, int iObj )
{
    int iFanout, k, Path = 0;
    assert( !Acb_ObjIsCo(p, iObj) );
    Acb_ObjForEachFanout( p, iObj, iFanout, k )
        if ( !Acb_ObjSlack(p, iFanout) )
            Path += Acb_ObjPathR(p, iFanout);
    return Acb_ObjSetPathR( p, iObj, Path );
}
int Acb_NtkComputePathsR( Acb_Ntk_t * p, Vec_Int_t * vTfi, int fReverse )
{
    int i, iObj, Path = 0;
    if ( !Acb_NtkHasObjPathR( p ) )
        Acb_NtkCleanObjPathR( p );
    // it is assumed that vTfi contains CO nodes
    //assert( Acb_ObjSlack(p, Vec_IntEntry(vTfi, 0)) );
    if ( fReverse )
    {
        Vec_IntForEachEntryReverse( vTfi, iObj, i )
        {
            if ( Acb_ObjIsCo(p, iObj) )
                Acb_ObjSetPathR( p, iObj, Acb_ObjSlack(p, iObj) == 0 );
            else if ( Acb_ObjSlack(p, iObj) )
                Acb_ObjSetPathR( p, iObj, 0 );
            else
                Acb_ObjComputePathR( p, iObj );
        }
    }
    else
    {
        Vec_IntForEachEntry( vTfi, iObj, i )
        {
            if ( Acb_ObjIsCo(p, iObj) )
                Acb_ObjSetPathR( p, iObj, Acb_ObjSlack(p, iObj) == 0 );
            else if ( Acb_ObjSlack(p, iObj) )
                Acb_ObjSetPathR( p, iObj, 0 );
            else
                Acb_ObjComputePathR( p, iObj );
        }
    }
    Acb_NtkForEachCi( p, iObj, i )
        Path += Acb_ObjPathR(p, iObj);
//    assert( p->nPaths == Path );
    p->nPaths = Path;
    return Path;
}

void Acb_NtkPrintPaths( Acb_Ntk_t * p )
{
    int iObj;
    Acb_NtkForEachObj( p, iObj )
    {
        printf( "Obj = %5d :   ",   iObj );
        printf( "LevelD = %5d  ",   Acb_ObjLevelD(p, iObj) );
        printf( "LevelR = %5d    ", Acb_ObjLevelR(p, iObj) );
        printf( "PathD = %5d  ",    Acb_ObjPathD(p, iObj) );
        printf( "PathR = %5d    ",  Acb_ObjPathR(p, iObj) );
        printf( "Paths = %5d  ",    Acb_ObjPathD(p, iObj) * Acb_ObjPathR(p, iObj) );
        printf( "\n" );
    }
}

int Acb_NtkComputePaths( Acb_Ntk_t * p )
{
    int LevelD, LevelR;
    Vec_Int_t * vTfi = Acb_ObjCollectTfi( p, -1, 1 );
    Vec_Int_t * vTfo = Acb_ObjCollectTfo( p, -1, 1 );
    Acb_NtkComputeLevelD( p, vTfo );
    LevelD = p->LevelMax;
    Acb_NtkComputeLevelR( p, vTfi );
    LevelR = p->LevelMax;
    assert( LevelD == LevelR );
    Acb_NtkComputePathsD( p, vTfo, 1 );
    Acb_NtkComputePathsR( p, vTfi, 1 );
    return p->nPaths;
}
void Abc_NtkComputePaths( Abc_Ntk_t * p )
{
    extern Acb_Ntk_t * Acb_NtkFromAbc( Abc_Ntk_t * p );
    Acb_Ntk_t * pNtk = Acb_NtkFromAbc( p );
    Acb_NtkCreateFanout( pNtk );
    Acb_NtkCleanObjCounts( pNtk );
    printf( "Computed %d paths.\n", Acb_NtkComputePaths(pNtk) );
    Acb_NtkPrintPaths( pNtk );
    Acb_ManFree( pNtk->pDesign );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_ObjUpdatePriority( Acb_Ntk_t * p, int iObj )
{
    int nPaths;
    if ( Acb_ObjIsCio(p, iObj) || Acb_ObjLevelD(p, iObj) == 1 )
        return;
    if ( p->vQue == NULL )
    {
        Acb_NtkCleanObjCounts( p );
        p->vQue = Vec_QueAlloc( 1000 );
        Vec_QueSetPriority( p->vQue, Vec_FltArrayP(&p->vCounts) );
    }
    nPaths = Acb_ObjPathD(p, iObj) * Acb_ObjPathR(p, iObj);
    Acb_ObjSetCounts( p, iObj, (float)nPaths );
    if ( Vec_QueIsMember( p->vQue, iObj ) )
    {
//printf( "Updating object %d with count %d\n", iObj, nPaths );
        Vec_QueUpdate( p->vQue, iObj );
    }
    else if ( nPaths )
    {
//printf( "Adding object %d with count %d\n", iObj, nPaths );
        Vec_QuePush( p->vQue, iObj );
    }
}
void Acb_NtkUpdateTiming( Acb_Ntk_t * p, int iObj )
{
    int i, Entry, LevelMax = p->LevelMax;
    int LevelD, LevelR, nPaths1, nPaths2;
    // assuming that direct level of the new nodes (including iObj) is up to date
    Vec_Int_t * vTfi = Acb_ObjCollectTfi( p, iObj, 1 );
    Vec_Int_t * vTfo = Acb_ObjCollectTfo( p, iObj, 1 );
    if ( iObj > 0 )
    {
        assert( Vec_IntEntryLast(vTfi) == iObj );
        assert( Vec_IntEntryLast(vTfo) == iObj );
        Vec_IntPop( vTfo );
    }
    Acb_NtkComputeLevelD( p, vTfo );
    LevelD = p->LevelMax;
    Acb_NtkComputeLevelR( p, vTfi );
    LevelR = p->LevelMax;
    assert( LevelD == LevelR );
    if ( iObj > 0 && LevelMax > p->LevelMax ) // reduced level
    {
        iObj = -1;
        vTfi = Acb_ObjCollectTfi( p, -1, 1 );
        vTfo = Acb_ObjCollectTfo( p, -1, 1 );   
        Vec_QueClear( p->vQue );
        // add backup here
    }
    if ( iObj > 0 )
    Acb_NtkComputePathsD( p, vTfi, 0 );
    Acb_NtkComputePathsD( p, vTfo, 1 );
    nPaths1 = p->nPaths;
    if ( iObj > 0 )
    Acb_NtkComputePathsR( p, vTfo, 0 );
    Acb_NtkComputePathsR( p, vTfi, 1 );
    nPaths2 = p->nPaths;
    assert( nPaths1 == nPaths2 );
    Vec_IntForEachEntry( vTfi, Entry, i )
        Acb_ObjUpdatePriority( p, Entry );
    if ( iObj > 0 )
    Vec_IntForEachEntry( vTfo, Entry, i )
        Acb_ObjUpdatePriority( p, Entry );

//    printf( "Updating timing for object %d.\n", iObj );
//    Acb_NtkPrintPaths( p );
//    while ( (Entry = (int)Vec_QueTopPriority(p->vQue)) > 0 )
//        printf( "Obj = %5d : Prio = %d.\n", Vec_QuePop(p->vQue), Entry );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_NtkPrintNode( Acb_Ntk_t * p, int iObj )
{
    int k, iFanin, * pFanins; 
    printf( "Node %5d : ", iObj );
    Acb_ObjForEachFaninFast( p, iObj, pFanins, iFanin, k )
        printf( "%d ", iFanin );
    printf( "LevelD = %d. LevelR = %d.\n", Acb_ObjLevelD(p, iObj), Acb_ObjLevelR(p, iObj) );
}
int Acb_NtkCreateNode( Acb_Ntk_t * p, word uTruth, Vec_Int_t * vSupp )
{
    int Pivot = Acb_ObjAlloc( p, ABC_OPER_LUT, Vec_IntSize(vSupp), 0 );
    Acb_ObjSetTruth( p, Pivot, uTruth );
    Acb_ObjAddFanins( p, Pivot, vSupp );
    Acb_ObjAddFaninFanout( p, Pivot );
    Acb_ObjComputeLevelD( p, Pivot );
    return Pivot;
}
void Acb_NtkResetNode( Acb_Ntk_t * p, int Pivot, word uTruth, Vec_Int_t * vSupp )
{
    // remember old fanins
    int k, iFanin, * pFanins; 
    Vec_Int_t * vFanins = Vec_IntAlloc( 6 );
    assert( !Acb_ObjIsCio(p, Pivot) );
    Acb_ObjForEachFaninFast( p, Pivot, pFanins, iFanin, k )
        Vec_IntPush( vFanins, iFanin );
    // update function
    Vec_WrdSetEntry( &p->vObjTruth, Pivot, uTruth );
    Vec_IntErase( Vec_WecEntry(&p->vCnfs, Pivot) );
    // remove old fanins
    Acb_ObjRemoveFaninFanout( p, Pivot );
    Acb_ObjRemoveFanins( p, Pivot );
    // add new fanins
    if ( vSupp != NULL )
    {
        assert( Acb_ObjFanoutNum(p, Pivot) > 0 );
        Acb_ObjAddFanins( p, Pivot, vSupp );
        Acb_ObjAddFaninFanout( p, Pivot );
    }
    else if ( Acb_ObjFanoutNum(p, Pivot) == 0 )
        Acb_ObjCleanType( p, Pivot );
    // delete dangling fanins
    Vec_IntForEachEntry( vFanins, iFanin, k )
        if ( !Acb_ObjIsCio(p, iFanin) && Acb_ObjFanoutNum(p, iFanin) == 0 )
            Acb_NtkResetNode( p, iFanin, 0, NULL );
    Vec_IntFree( vFanins );
}
void Acb_NtkSaveSupport( Acb_Ntk_t * p, int iObj )
{
    int k, iFanin, * pFanins; 
    Vec_IntClear( &p->vSuppOld );
    Acb_ObjForEachFaninFast( p, iObj, pFanins, iFanin, k )
        Vec_IntPush( &p->vSuppOld, iFanin );
}
void Acb_NtkUpdateNode( Acb_Ntk_t * p, int Pivot, word uTruth, Vec_Int_t * vSupp )
{
    //int Level = Acb_ObjLevelD(p, Pivot);
    Acb_NtkSaveSupport( p, Pivot );
    //Acb_NtkPrintNode( p, Pivot );
    Acb_NtkResetNode( p, Pivot, uTruth, vSupp );
    Acb_ObjComputeLevelD( p, Pivot );
    //assert( Level > Acb_ObjLevelD(p, Pivot) );
    //Acb_NtkPrintNode( p, Pivot );
    if ( p->vQue == NULL )
        Acb_NtkUpdateLevelD( p, Pivot );
    else
//        Acb_NtkUpdateTiming( p, Pivot );
        Acb_NtkUpdateTiming( p, -1 );
    Vec_IntClear( &p->vSuppOld );
}

/**Function*************************************************************

  Synopsis    [Derive AIG for one network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_NtkFindNodes2_rec( Acb_Ntk_t * p, int iObj, Vec_Int_t * vNodes )
{
    int * pFanin, iFanin, i;
    if ( Acb_ObjSetTravIdCur(p, iObj) )
        return;
    if ( Acb_ObjIsCi(p, iObj) )
        return;
    Acb_ObjForEachFaninFast( p, iObj, pFanin, iFanin, i )
        Acb_NtkFindNodes2_rec( p, iFanin, vNodes );
    assert( !Acb_ObjIsCo(p, iObj) );
    Vec_IntPush( vNodes, iObj );
}
Vec_Int_t * Acb_NtkFindNodes2( Acb_Ntk_t * p )
{
    int i, iObj;
    Vec_Int_t * vNodes = Vec_IntAlloc( 1000 );
    Acb_NtkIncTravId( p );
    Acb_NtkForEachCo( p, iObj, i )
        Acb_NtkFindNodes2_rec( p, Acb_ObjFanin(p, iObj, 0), vNodes );
    return vNodes;
}
int Acb_ObjToGia2( Gia_Man_t * pNew, int fUseBuf, Acb_Ntk_t * p, int iObj, Vec_Int_t * vTemp, int fUseXors )
{
    //char * pName = Abc_NamStr( p->pDesign->pStrs, Acb_ObjName(p, iObj) );
    int * pFanin, iFanin, k, Type, Res;
    assert( !Acb_ObjIsCio(p, iObj) );
    Vec_IntClear( vTemp );
    Acb_ObjForEachFaninFast( p, iObj, pFanin, iFanin, k )
    {
        assert( Acb_ObjCopy(p, iFanin) >= 0 );
        Vec_IntPush( vTemp, Acb_ObjCopy(p, iFanin) );
    }
    Type = Acb_ObjType( p, iObj );
    if ( Type == ABC_OPER_CONST_F ) 
        return 0;
    if ( Type == ABC_OPER_CONST_T ) 
        return 1;
    if ( Type == ABC_OPER_BIT_BUF || Type == ABC_OPER_BIT_INV ) 
    {
        Res = fUseBuf ? Gia_ManAppendBuf(pNew, Vec_IntEntry(vTemp, 0)) : Vec_IntEntry(vTemp, 0);
        return Abc_LitNotCond( Res, Type == ABC_OPER_BIT_INV );
    }
    if ( Type == ABC_OPER_BIT_AND || Type == ABC_OPER_BIT_NAND )
    {
        Res = 1;
        Vec_IntForEachEntry( vTemp, iFanin, k )
            Res = Gia_ManAppendAnd2( pNew, Res, iFanin );
        return Abc_LitNotCond( Res, Type == ABC_OPER_BIT_NAND );
    }
    if ( Type == ABC_OPER_BIT_OR || Type == ABC_OPER_BIT_NOR )
    {
        Res = 0;
        Vec_IntForEachEntry( vTemp, iFanin, k )
            Res = Gia_ManAppendOr2( pNew, Res, iFanin );
        return Abc_LitNotCond( Res, Type == ABC_OPER_BIT_NOR );
    }
    if ( Type == ABC_OPER_BIT_XOR || Type == ABC_OPER_BIT_NXOR )
    {
        Res = 0;
        Vec_IntForEachEntry( vTemp, iFanin, k )
            Res = fUseXors ? Gia_ManAppendXorReal2(pNew, Res, iFanin) : Gia_ManAppendXor2(pNew, Res, iFanin);
        return Abc_LitNotCond( Res, Type == ABC_OPER_BIT_NXOR );
    }
    assert( 0 );
    return -1;
}
Gia_Man_t * Acb_NtkToGia2( Acb_Ntk_t * p, int fUseBuf, int fUseXors, Vec_Int_t * vTargets, int nTargets )
{
    Gia_Man_t * pNew, * pOne;
    Vec_Int_t * vFanins, * vNodes;
    int i, iObj;
    pNew = Gia_ManStart( 2 * Acb_NtkObjNum(p) + 1000 );
    pNew->pName = Abc_UtilStrsav(Acb_NtkName(p));
    Acb_NtkCleanObjCopies( p );
    Acb_NtkForEachCi( p, iObj, i )
        Acb_ObjSetCopy( p, iObj, Gia_ManAppendCi(pNew) );
    if ( vTargets )
        Vec_IntForEachEntry( vTargets, iObj, i )
            Acb_ObjSetCopy( p, iObj, Gia_ManAppendCi(pNew) );
    else
        for ( i = 0; i < nTargets; i++ )
            Gia_ManAppendCi(pNew);
    vFanins = Vec_IntAlloc( 4 );
    vNodes  = Acb_NtkFindNodes2( p );
    Vec_IntForEachEntry( vNodes, iObj, i )
        if ( Acb_ObjCopy(p, iObj) == -1 ) // skip targets assigned above
            Acb_ObjSetCopy( p, iObj, Acb_ObjToGia2(pNew, fUseBuf, p, iObj, vFanins, fUseXors) );
    Vec_IntFree( vNodes );
    Vec_IntFree( vFanins );
    Acb_NtkForEachCo( p, iObj, i )
        Gia_ManAppendCo( pNew, Acb_ObjCopy(p, Acb_ObjFanin(p, iObj, 0)) );
    pNew = Gia_ManCleanup( pOne = pNew );
    Gia_ManUpdateCopy( &p->vObjCopy, pOne );
    Gia_ManStop( pOne );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Acb_NtkCollectCopies( Acb_Ntk_t * p, Gia_Man_t * pGia, Vec_Ptr_t ** pvNodesR, Vec_Bit_t ** pvPolar )
{
    int i, iObj, iLit, nTargets = Vec_IntSize(&p->vTargets);
    Vec_Int_t * vObjs   = Acb_NtkFindNodes2( p );
    Vec_Int_t * vNodes  = Vec_IntAlloc( Acb_NtkObjNum(p) );
    Vec_Ptr_t * vNodesR = Vec_PtrStart( Gia_ManObjNum(pGia) );
    Vec_Bit_t * vDriver = Vec_BitStart( Gia_ManObjNum(pGia) );
    Vec_Bit_t * vPolar  = Vec_BitStart( Gia_ManObjNum(pGia) );
    Gia_ManForEachCiId( pGia, iObj, i )
        if ( i < Gia_ManCiNum(pGia) - nTargets )
            Vec_PtrWriteEntry( vNodesR, iObj, Abc_UtilStrsav(Acb_ObjNameStr(p, Acb_NtkCi(p, i))) );
        else
            Vec_PtrWriteEntry( vNodesR, iObj, Abc_UtilStrsav(Acb_ObjNameStr(p, Vec_IntEntry(&p->vTargets, i-(Gia_ManCiNum(pGia) - nTargets)))) );
    Gia_ManForEachCoId( pGia, iObj, i )
    {
        Vec_BitWriteEntry( vDriver, Gia_ObjFaninId0(Gia_ManObj(pGia, iObj), iObj), 1 );
        Vec_PtrWriteEntry( vNodesR, iObj, Abc_UtilStrsav(Acb_ObjNameStr(p, Acb_NtkCo(p, i))) );
        Vec_IntPush( vNodes, iObj );
    }
    Vec_IntForEachEntry( vObjs, iObj, i )
        if ( (iLit = Acb_ObjCopy(p, iObj)) >= 0 && Gia_ObjIsAnd(Gia_ManObj(pGia, Abc_Lit2Var(iLit))) )
        {
            if ( !Vec_BitEntry(vDriver, Abc_Lit2Var(iLit)) && Vec_PtrEntry(vNodesR, Abc_Lit2Var(iLit)) == NULL )
            {
                Vec_PtrWriteEntry( vNodesR, Abc_Lit2Var(iLit), Abc_UtilStrsav(Acb_ObjNameStr(p, iObj)) );
                Vec_IntPush( vNodes, Abc_Lit2Var(iLit) );
                Vec_BitWriteEntry( vPolar, Abc_Lit2Var(iLit), Abc_LitIsCompl(iLit) );
            }
        }
    Vec_BitFree( vDriver );
    Vec_IntFree( vObjs );
    Vec_IntSort( vNodes, 0 );
    *pvNodesR = vNodesR;
    *pvPolar  = vPolar;
    return vNodes;
}
Vec_Int_t * Acb_NtkCollectUser( Acb_Ntk_t * p, Vec_Ptr_t * vUser )
{
    char * pTemp; int i;
    Vec_Int_t * vRes = Vec_IntAlloc( Vec_PtrSize(vUser) );
    Vec_Int_t * vMap = Vec_IntStart( Abc_NamObjNumMax(Acb_NtkNam(p)) );
    Acb_NtkForEachNode( p, i )
        if ( Acb_ObjName(p, i) > 0 )
            Vec_IntWriteEntry( vMap, Acb_ObjName(p, i), Acb_ObjCopy(p, i) );
    Vec_PtrForEachEntry( char *, vUser, pTemp, i )
        if ( Acb_NtkStrId(p, pTemp) < Vec_IntSize(vMap) )
        {
            int iLit = Vec_IntEntry( vMap, Acb_NtkStrId(p, pTemp) );
            assert( iLit > 0 );
            //printf( "Obj = %3d  Name = %3d  Copy = %3d\n", i, Acb_NtkStrId(p, pTemp), iLit );
            Vec_IntPush( vRes, Abc_Lit2Var(iLit) );
        }
    assert( Vec_IntSize(vRes) == Vec_PtrSize(vUser) );
    Vec_IntFree( vMap );
    Vec_IntUniqify( vRes );
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Acb_NtkExtract( char * pFileName0, char * pFileName1, int fUseXors, int fVerbose, 
                    Gia_Man_t ** ppGiaF, Gia_Man_t ** ppGiaG, int fUseBuf, Vec_Int_t ** pvNodes, Vec_Ptr_t ** pvNodesR, Vec_Bit_t ** pvPolar )
{
    Acb_Ntk_t * pNtkF = Acb_VerilogSimpleRead( pFileName0, NULL );
    Acb_Ntk_t * pNtkG = Acb_VerilogSimpleRead( pFileName1, NULL );
    int RetValue = -1;
    if ( pNtkF && pNtkG )
    {
        int nTargets = Vec_IntSize(&pNtkF->vTargets);
        Gia_Man_t * pGiaF = Acb_NtkToGia2( pNtkF, fUseBuf, fUseXors, &pNtkF->vTargets, 0 );
        Gia_Man_t * pGiaG = Acb_NtkToGia2( pNtkG, 0,       0,        NULL, nTargets );
        pGiaF->pSpec = Abc_UtilStrsav( pNtkF->pDesign->pSpec );
        pGiaG->pSpec = Abc_UtilStrsav( pNtkG->pDesign->pSpec );
        assert( Acb_NtkCiNum(pNtkF) == Acb_NtkCiNum(pNtkG) );
        assert( Acb_NtkCoNum(pNtkF) == Acb_NtkCoNum(pNtkG) );
        *ppGiaF  = pGiaF;
        *ppGiaG  = pGiaG;
        *pvNodes = Acb_NtkCollectCopies( pNtkF, pGiaF, pvNodesR, pvPolar );
        RetValue = nTargets;
    }
    if ( pNtkF ) Acb_ManFree( pNtkF->pDesign );
    if ( pNtkG ) Acb_ManFree( pNtkG->pDesign );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkCollectCopies( Abc_Ntk_t * p, Gia_Man_t * pGia, Vec_Ptr_t ** pvNodesR, Vec_Bit_t ** pvPolar )
{
    int i, iObj, iLit;
    Abc_Obj_t * pObj;
    Vec_Ptr_t * vObjs   = Abc_NtkDfs( p, 0 );
    Vec_Int_t * vNodes  = Vec_IntAlloc( Abc_NtkObjNumMax(p) );
    Vec_Ptr_t * vNodesR = Vec_PtrStart( Gia_ManObjNum(pGia) );
    Vec_Bit_t * vDriver = Vec_BitStart( Gia_ManObjNum(pGia) );
    Vec_Bit_t * vPolar  = Vec_BitStart( Gia_ManObjNum(pGia) );
    Gia_ManForEachCiId( pGia, iObj, i )
        Vec_PtrWriteEntry( vNodesR, iObj, Abc_UtilStrsav(Abc_ObjName(Abc_NtkCi(p, i))) );
    Gia_ManForEachCoId( pGia, iObj, i )
    {
        Vec_BitWriteEntry( vDriver, Gia_ObjFaninId0(Gia_ManObj(pGia, iObj), iObj), 1 );
        Vec_PtrWriteEntry( vNodesR, iObj, Abc_UtilStrsav(Abc_ObjName(Abc_NtkCo(p, i))) );
        Vec_IntPush( vNodes, iObj );
    }
    Vec_PtrForEachEntry( Abc_Obj_t *, vObjs, pObj, i )
        if ( (iLit = pObj->iTemp) >= 0 && Gia_ObjIsAnd(Gia_ManObj(pGia, Abc_Lit2Var(iLit))) )
        {
            if ( !Vec_BitEntry(vDriver, Abc_Lit2Var(iLit)) && Vec_PtrEntry(vNodesR, Abc_Lit2Var(iLit)) == NULL )
            {
                Vec_PtrWriteEntry( vNodesR, Abc_Lit2Var(iLit), Abc_UtilStrsav(Abc_ObjName(pObj)) );
                Vec_IntPush( vNodes, Abc_Lit2Var(iLit) );
                Vec_BitWriteEntry( vPolar, Abc_Lit2Var(iLit), Abc_LitIsCompl(iLit) );
            }
        }
    Vec_BitFree( vDriver );
    Vec_PtrFree( vObjs );
    Vec_IntSort( vNodes, 0 );
    *pvNodesR = vNodesR;
    *pvPolar  = vPolar;
    return vNodes;
}
int Abc_ObjToGia2( Gia_Man_t * pNew, Abc_Ntk_t * p, Abc_Obj_t * pObj, Vec_Int_t * vTemp, int fUseXors )
{
    Abc_Obj_t * pFanin; int k;
    assert( Abc_ObjIsNode(pObj) );
    Vec_IntClear( vTemp );
    Abc_ObjForEachFanin( pObj, pFanin, k )
    {
        assert( pFanin->iTemp >= 0 );
        Vec_IntPush( vTemp, pFanin->iTemp );
    }
    if ( Abc_ObjFaninNum(pObj) == 0 )
        return Abc_SopIsConst0( (char*)pObj->pData ) ? 0 : 1;
    if ( Abc_ObjFaninNum(pObj) == 1 )
        return Abc_SopIsBuf( (char*)pObj->pData ) ? Vec_IntEntry(vTemp, 0) : Abc_LitNot(Vec_IntEntry(vTemp, 0));
    if ( Abc_ObjFaninNum(pObj) == 2 ) // nand2
        return Abc_LitNot( Gia_ManAppendAnd2( pNew, Vec_IntEntry(vTemp, 0), Vec_IntEntry(vTemp, 1) ) );
    assert( 0 );
    return -1;
}
Gia_Man_t * Abc_NtkToGia2( Abc_Ntk_t * p, int fUseXors )
{
    Gia_Man_t * pNew, * pOne;
    Vec_Int_t * vFanins;
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj; int i;
    pNew = Gia_ManStart( 2 * Abc_NtkObjNumMax(p) + 1000 );
    pNew->pName = Abc_UtilStrsav(Abc_NtkName(p));
    Abc_NtkForEachObj( p, pObj, i )
        pObj->iTemp = -1;
    Abc_NtkForEachCi( p, pObj, i )
        pObj->iTemp = Gia_ManAppendCi(pNew);
    vFanins = Vec_IntAlloc( 4 );
    vNodes  = Abc_NtkDfs( p, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        pObj->iTemp = Abc_ObjToGia2(pNew, p, pObj, vFanins, fUseXors);
    Vec_PtrFree( vNodes );
    Vec_IntFree( vFanins );
    Abc_NtkForEachCo( p, pObj, i )
        Gia_ManAppendCo( pNew, Abc_ObjFanin0(pObj)->iTemp );
    pNew = Gia_ManCleanup( pOne = pNew );
    //Gia_ManUpdateCopy( &p->vObjCopy, pOne );
    Gia_ManStop( pOne );
    return pNew;
}
int Abc_NtkExtract( char * pFileName0, char * pFileName1, int fUseXors, int fVerbose, 
                    Gia_Man_t ** ppGiaF, Gia_Man_t ** ppGiaG, Vec_Int_t ** pvNodes, Vec_Ptr_t ** pvNodesR, Vec_Bit_t ** pvPolar )
{
    Abc_Ntk_t * pNtkF = Io_Read( pFileName0, Io_ReadFileType(pFileName0), 1, 0 );
    Abc_Ntk_t * pNtkG = Io_Read( pFileName1, Io_ReadFileType(pFileName1), 1, 0 );
    int RetValue = -1;
    if ( pNtkF && pNtkG )
    {
        Gia_Man_t * pGiaF = Abc_NtkToGia2( pNtkF, fUseXors );
        Gia_Man_t * pGiaG = Abc_NtkToGia2( pNtkG, 0 );
        assert( Abc_NtkCiNum(pNtkF) == Abc_NtkCiNum(pNtkG) );
        assert( Abc_NtkCoNum(pNtkF) == Abc_NtkCoNum(pNtkG) );
        pGiaF->pSpec = Abc_UtilStrsav( pNtkF->pSpec );
        pGiaG->pSpec = Abc_UtilStrsav( pNtkG->pSpec );
        *ppGiaF  = pGiaF;
        *ppGiaG  = pGiaG;
        *pvNodes = Abc_NtkCollectCopies( pNtkF, pGiaF, pvNodesR, pvPolar );
        RetValue = 0;
    }
    if ( pNtkF ) Abc_NtkDelete( pNtkF );
    if ( pNtkG ) Abc_NtkDelete( pNtkG );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Acb_NtkPlaces( char * pFileName, Vec_Ptr_t * vNames )
{
    Vec_Int_t * vPlaces; int First = 1, Pos = -1, fComment = 0;
    char * pTemp, * pBuffer = Extra_FileReadContents( pFileName );
    char * pLimit = pBuffer + strlen(pBuffer);
    if ( pBuffer == NULL )
        return NULL;
    vPlaces = Vec_IntAlloc( Vec_PtrSize(vNames) );
    for ( pTemp = pBuffer; *pTemp; pTemp++ )
    {
        if ( *pTemp == '\n' )
            fComment = 0;
        if ( *pTemp == '/' && *(pTemp + 1) == '/' )
            fComment = 1;
        if ( fComment )
            continue;

        if ( *pTemp == '\n' )
            Pos = pTemp - pBuffer + 1;
        else if ( *pTemp == '(' )
        {
            if ( First )
                First = 0;
            else
            {
                char * pToken = strtok( pTemp+1, "  \n\r\t," );
                char * pName; int i;
                Vec_PtrForEachEntry( char *, vNames, pName, i )
                    if ( !strcmp(pName, pToken) )
                        Vec_IntPushTwo( vPlaces, Pos, i );
                pTemp = pToken + strlen(pToken);
                while ( *pTemp == 0 )
                    pTemp++;
                assert( pTemp < pLimit );
            }
        }
    }
    assert( Vec_IntSize(vPlaces) == 2*Vec_PtrSize(vNames) );
    ABC_FREE( pBuffer );
    return vPlaces;
}
void Acb_NtkInsert( char * pFileNameIn, char * pFileNameOut, Vec_Ptr_t * vNames, int fNumber, int fSkipMffc )
{
    int i, k, Prev = 0, Pos, Pos2, iObj;
    Vec_Int_t * vPlaces;
    char * pName, * pBuffer;
    FILE * pFile = fopen( pFileNameOut, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open output file \"%s\".\n", pFileNameOut );
        return;
    }
    pBuffer = Extra_FileReadContents( pFileNameIn );
    if ( pBuffer == NULL )
    {
        fclose( pFile );
        printf( "Cannot open input file \"%s\".\n", pFileNameIn );
        return;
    }
    if ( fSkipMffc )
    {
        Vec_Ptr_t * vMffcNames = Acb_NtkReturnMfsGates( pFileNameIn, vNames );
        vPlaces = Acb_NtkPlaces( pFileNameIn, vMffcNames );
        Vec_IntForEachEntryDouble( vPlaces, Pos, iObj, i )
        {
            for ( k = Prev; k < Pos; k++ )
                fputc( pBuffer[k], pFile );
            fprintf( pFile, "// MFFC %d = %s //", iObj, (char *)Vec_PtrEntry(vMffcNames, iObj) );
            Prev = Pos;
        }
        Vec_IntFree( vPlaces );
        Vec_PtrFreeFree( vMffcNames );
    }
    else
    {
        vPlaces = Acb_NtkPlaces( pFileNameIn, vNames );
        Vec_IntForEachEntryDouble( vPlaces, Pos, iObj, i )
        {
            for ( k = Prev; k < Pos; k++ )
                fputc( pBuffer[k], pFile );
            fprintf( pFile, "// [t_%d = %s] //", iObj, (char *)Vec_PtrEntry(vNames, iObj) );
            Prev = Pos;
        }
        Vec_IntFree( vPlaces );
    }
    pName = strstr(pBuffer, "endmodule");
    Pos2 = pName - pBuffer;
    for ( k = Prev; k < Pos2; k++ )
        fputc( pBuffer[k], pFile );
    fprintf( pFile, "\n\n" );
    fprintf( pFile, "  wire " );
    if ( fNumber )
    {
        Vec_PtrForEachEntry( char *, vNames, pName, i )
            fprintf( pFile, " t_%d%s", i, i==Vec_PtrSize(vNames)-1 ? ";" : "," );
    }
    else
    {
        Vec_PtrForEachEntry( char *, vNames, pName, i )
            fprintf( pFile, " t%d_%s%s", i, pName, i==Vec_PtrSize(vNames)-1 ? ";" : "," );
    }
    fprintf( pFile, "\n\n" );
    if ( fNumber )
    {
        Vec_PtrForEachEntry( char *, vNames, pName, i )
            fprintf( pFile, "  buf( %s, t_%d );\n", pName, i );
    }
    else
    {
        Vec_PtrForEachEntry( char *, vNames, pName, i )
            fprintf( pFile, "  buf( %s, t%d_%s );\n", pName, i, pName );
    }
    fprintf( pFile, "\n" );
    for ( k = Pos2; pBuffer[k]; k++ )
        fputc( pBuffer[k], pFile );
    ABC_FREE( pBuffer );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_Ntk4CollectAdd( Acb_Ntk_t * pNtk, int iObj, Vec_Int_t * vRes, Vec_Int_t * vDists, int Dist )
{
    if ( Acb_ObjSetTravIdCur(pNtk, iObj) )
        return;
    Vec_IntWriteEntry( vDists, iObj, Dist );
    Vec_IntPush( vRes, iObj );
}
void Acb_Ntk4CollectRing( Acb_Ntk_t * pNtk, Vec_Int_t * vStart, Vec_Int_t * vRes, Vec_Int_t * vDists )
{
    int i, iObj;
    Vec_IntForEachEntry( vStart, iObj, i )
    {
        int k, iFanin, * pFanins, Weight = Vec_IntEntry(vDists, iObj); 
        Acb_ObjForEachFaninFast( pNtk, iObj, pFanins, iFanin, k )
            Acb_Ntk4CollectAdd( pNtk, iFanin, vRes, vDists, Weight + 1*(Acb_ObjFaninNum(pNtk, iObj) > 1) );
        Acb_ObjForEachFanout( pNtk, iObj, iFanin, k )
            Acb_Ntk4CollectAdd( pNtk, iFanin, vRes, vDists, Weight + 2*(Acb_ObjFaninNum(pNtk, iObj) > 1) );
    }
}
void Acb_Ntk4DumpWeightsInt( Acb_Ntk_t * pNtk, Vec_Int_t * vObjs, char * pFileName )
{
    int i, iObj;//, Count = 0;//, Weight;
    Vec_Int_t * vDists, * vStart, * vNexts;
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Canont open input file \"%s\".\n", pFileName );
        return;
    }
    vStart = Vec_IntAlloc( 100 );
    vNexts = Vec_IntAlloc( 100 );
    vDists = Vec_IntStart( Acb_NtkObjNumMax(pNtk) );
    Acb_NtkIncTravId( pNtk );
    Vec_IntForEachEntry( vObjs, iObj, i )
    {
        Acb_ObjSetTravIdCur(pNtk, iObj);
        Vec_IntWriteEntry( vDists, iObj, 1 );
        Vec_IntPush( vStart, iObj );
    }    
    while ( 1 )
    {
        Acb_Ntk4CollectRing( pNtk, vStart, vNexts, vDists );
        if ( Vec_IntSize(vNexts) == 0 )
            break;
        Vec_IntClear( vStart );
        ABC_SWAP( Vec_Int_t, *vStart, *vNexts );
    }
    Vec_IntFree( vStart );
    Vec_IntFree( vNexts );
    // create weights
//    Vec_IntForEachEntry( vDists, Weight, i )
//        if ( Weight && Acb_ObjNameStr(pNtk, i)[0] != '1' )
//            fprintf( pFile, "%s %d\n", Acb_ObjNameStr(pNtk, i), 10000+Weight );
/*
    // mark reachable
    Vec_IntClear( &pNtk->vArray0 );
    Acb_NtkIncTravId( pNtk );
    Acb_NtkForEachCo( pNtk, iObj, i )
        if ( !Vec_IntEntry(vStatus, i) )
            Acb_ObjCollectTfi_rec( pNtk, iObj, 0 );
*/
    Acb_NtkForEachObj( pNtk, iObj )
    {
        char * pName = Acb_ObjNameStr(pNtk, iObj);
        int Weight = Vec_IntEntry(vDists, iObj);
        if ( Weight == 0 )
            Weight = 10000;
/*
        if ( !Acb_ObjSetTravIdCur(pNtk, iObj) )
        {
            Count++;
            continue;
        }
*/
        fprintf( pFile, "%s %d\n", pName, 100000+Weight );
    }
    //printf( "Skipped %d nodes.\n", Count );
    Vec_IntFree( vDists );
    fclose( pFile );
}
void Acb_Ntk4DumpWeights( char * pFileNameIn, Vec_Ptr_t * vObjNames, char * pFileName )
{
    char * pName; int i, iObj;
    Vec_Int_t * vObjs = Vec_IntAlloc( Vec_PtrSize(vObjNames) );
    Acb_Ntk_t * pNtkF = Acb_VerilogSimpleRead( pFileNameIn, NULL );
    Acb_NtkCreateFanout( pNtkF );
    Vec_PtrForEachEntry( char *, vObjNames, pName, i )
    {
        Acb_NtkForEachObj( pNtkF, iObj )
            if ( !strcmp(Acb_ObjNameStr(pNtkF, iObj), pName) )
                Vec_IntPush( vObjs, iObj );
    }
    Acb_Ntk4DumpWeightsInt( pNtkF, vObjs, pFileName );
    Acb_ManFree( pNtkF->pDesign );
    Vec_IntFree( vObjs );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

