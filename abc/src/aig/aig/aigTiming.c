/**CFile****************************************************************

  FileName    [aigTiming.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Incremental updating of direct/reverse AIG levels.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigTiming.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the reverse level of the node.]

  Description [The reverse level is the level of the node in reverse
  topological order, starting from the COs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_ObjReverseLevel( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    assert( p->vLevelR );
    Vec_IntFillExtra( p->vLevelR, pObj->Id + 1, 0 );
    return Vec_IntEntry(p->vLevelR, pObj->Id);
}

/**Function*************************************************************

  Synopsis    [Sets the reverse level of the node.]

  Description [The reverse level is the level of the node in reverse
  topological order, starting from the COs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Aig_ObjSetReverseLevel( Aig_Man_t * p, Aig_Obj_t * pObj, int LevelR )
{
    assert( p->vLevelR );
    Vec_IntFillExtra( p->vLevelR, pObj->Id + 1, 0 );
    Vec_IntWriteEntry( p->vLevelR, pObj->Id, LevelR );
}

/**Function*************************************************************

  Synopsis    [Resets reverse level of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjClearReverseLevel( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_ObjSetReverseLevel( p, pObj, 0 );
}

/**Function*************************************************************

  Synopsis    [Returns required level of the node.]

  Description [Converts the reverse levels of the node into its required 
  level as follows: ReqLevel(Node) = MaxLevels(Ntk) + 1 - LevelR(Node).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjRequiredLevel( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    assert( p->vLevelR );
    return p->nLevelMax + 1 - Aig_ObjReverseLevel(p, pObj);
}

/**Function*************************************************************

  Synopsis    [Computes the reverse level of the node using its fanout levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjReverseLevelNew( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t * pFanout;
    int i, iFanout = -1, LevelCur, Level = 0;
    Aig_ObjForEachFanout( p, pObj, pFanout, iFanout, i )
    {
        LevelCur = Aig_ObjReverseLevel( p, pFanout );
        Level = Abc_MaxInt( Level, LevelCur );
    }
    return Level + 1;
}

/**Function*************************************************************

  Synopsis    [Prepares for the computation of required levels.]

  Description [This procedure should be called before the required times
  are used. It starts internal data structures, which records the level 
  from the COs of the network nodes in reverse topologogical order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManStartReverseLevels( Aig_Man_t * p, int nMaxLevelIncrease )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    assert( p->pFanData != NULL );
    assert( p->vLevelR == NULL );
    // remember the maximum number of direct levels
    p->nLevelMax = Aig_ManLevels(p) + nMaxLevelIncrease;
    // start the reverse levels
    p->vLevelR = Vec_IntAlloc( 0 );
    Vec_IntFill( p->vLevelR, Aig_ManObjNumMax(p), 0 );
    // compute levels in reverse topological order
    vNodes = Aig_ManDfsReverse( p );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        assert( pObj->fMarkA == 0 );
        Aig_ObjSetReverseLevel( p, pObj, Aig_ObjReverseLevelNew(p, pObj) );
    }
    Vec_PtrFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Cleans the data structures used to compute required levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManStopReverseLevels( Aig_Man_t * p )
{
    assert( p->vLevelR != NULL );
    Vec_IntFree( p->vLevelR );
    p->vLevelR = NULL;
    p->nLevelMax = 0;

}

/**Function*************************************************************

  Synopsis    [Incrementally updates level of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManUpdateLevel( Aig_Man_t * p, Aig_Obj_t * pObjNew )
{
    Aig_Obj_t * pFanout, * pTemp;
    int iFanout = -1, LevelOld, Lev, k, m;
    assert( p->pFanData != NULL );
    assert( Aig_ObjIsNode(pObjNew) );
    // allocate level if needed
    if ( p->vLevels == NULL )
        p->vLevels = Vec_VecAlloc( Aig_ManLevels(p) + 8 );
    // check if level has changed
    LevelOld = Aig_ObjLevel(pObjNew);
    if ( LevelOld == Aig_ObjLevelNew(pObjNew) )
        return;
    // start the data structure for level update
    // we cannot fail to visit a node when using this structure because the 
    // nodes are stored by their _old_ levels, which are assumed to be correct
    Vec_VecClear( p->vLevels );
    Vec_VecPush( p->vLevels, LevelOld, pObjNew );
    pObjNew->fMarkA = 1;
    // recursively update level
    Vec_VecForEachEntryStart( Aig_Obj_t *, p->vLevels, pTemp, Lev, k, LevelOld )
    {
        pTemp->fMarkA = 0;
        assert( Aig_ObjLevel(pTemp) == Lev );
        pTemp->Level = Aig_ObjLevelNew(pTemp);
        // if the level did not change, no need to check the fanout levels
        if ( Aig_ObjLevel(pTemp) == Lev )
            continue;
        // schedule fanout for level update
        Aig_ObjForEachFanout( p, pTemp, pFanout, iFanout, m )
        {
            if ( Aig_ObjIsNode(pFanout) && !pFanout->fMarkA )
            {
                assert( Aig_ObjLevel(pFanout) >= Lev );
                Vec_VecPush( p->vLevels, Aig_ObjLevel(pFanout), pFanout );
                pFanout->fMarkA = 1;
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Incrementally updates level of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManUpdateReverseLevel( Aig_Man_t * p, Aig_Obj_t * pObjNew )
{
    Aig_Obj_t * pFanin, * pTemp;
    int LevelOld, LevFanin, Lev, k;
    assert( p->vLevelR != NULL );
    assert( Aig_ObjIsNode(pObjNew) );
    // allocate level if needed
    if ( p->vLevels == NULL )
        p->vLevels = Vec_VecAlloc( Aig_ManLevels(p) + 8 );
    // check if level has changed
    LevelOld = Aig_ObjReverseLevel(p, pObjNew);
    if ( LevelOld == Aig_ObjReverseLevelNew(p, pObjNew) )
        return;
    // start the data structure for level update
    // we cannot fail to visit a node when using this structure because the 
    // nodes are stored by their _old_ levels, which are assumed to be correct
    Vec_VecClear( p->vLevels );
    Vec_VecPush( p->vLevels, LevelOld, pObjNew );
    pObjNew->fMarkA = 1;
    // recursively update level
    Vec_VecForEachEntryStart( Aig_Obj_t *, p->vLevels, pTemp, Lev, k, LevelOld )
    {
        pTemp->fMarkA = 0;
        LevelOld = Aig_ObjReverseLevel(p, pTemp); 
        assert( LevelOld == Lev );
        Aig_ObjSetReverseLevel( p, pTemp, Aig_ObjReverseLevelNew(p, pTemp) );
        // if the level did not change, to need to check the fanout levels
        if ( Aig_ObjReverseLevel(p, pTemp) == Lev )
            continue;
        // schedule fanins for level update
        pFanin = Aig_ObjFanin0(pTemp);
        if ( Aig_ObjIsNode(pFanin) && !pFanin->fMarkA )
        {
            LevFanin = Aig_ObjReverseLevel( p, pFanin );
            assert( LevFanin >= Lev );
            Vec_VecPush( p->vLevels, LevFanin, pFanin );
            pFanin->fMarkA = 1;
        }
        pFanin = Aig_ObjFanin1(pTemp);
        if ( Aig_ObjIsNode(pFanin) && !pFanin->fMarkA )
        {
            LevFanin = Aig_ObjReverseLevel( p, pFanin );
            assert( LevFanin >= Lev );
            Vec_VecPush( p->vLevels, LevFanin, pFanin );
            pFanin->fMarkA = 1;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Verifies direct level of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManVerifyLevel( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    assert( p->pFanData );
    Aig_ManForEachNode( p, pObj, i )
        if ( Aig_ObjLevel(pObj) != Aig_ObjLevelNew(pObj) )
        {
            printf( "Level of node %6d should be %4d instead of %4d.\n", 
                pObj->Id, Aig_ObjLevelNew(pObj), Aig_ObjLevel(pObj) );
            Counter++;
        }
    if ( Counter )
    printf( "Levels of %d nodes are incorrect.\n", Counter );
}

/**Function*************************************************************

  Synopsis    [Verifies reverse level of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManVerifyReverseLevel( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    assert( p->vLevelR );
    Aig_ManForEachNode( p, pObj, i )
        if ( Aig_ObjLevel(pObj) != Aig_ObjLevelNew(pObj) )
        {
            printf( "Reverse level of node %6d should be %4d instead of %4d.\n", 
                pObj->Id, Aig_ObjReverseLevelNew(p, pObj), Aig_ObjReverseLevel(p, pObj) );
            Counter++;
        }
    if ( Counter )
    printf( "Reverse levels of %d nodes are incorrect.\n", Counter );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

