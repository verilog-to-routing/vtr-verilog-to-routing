/**CFile****************************************************************

  FileName    [acecStruct.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecStruct.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"
#include "misc/vec/vecWec.h"
#include "misc/extra/extra.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Acec_StructDetectXorRoots( Gia_Man_t * p )
{
    Vec_Int_t * vXors = Vec_IntAlloc( 100 );
    Vec_Bit_t * vXorIns = Vec_BitStart( Gia_ManObjNum(p) );
    Gia_Obj_t * pFan0, * pFan1, * pObj; 
    int i, k = 0, Entry;
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( !Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
            continue;
        Vec_IntPush( vXors, i );
        Vec_BitWriteEntry( vXorIns, Gia_ObjId(p, Gia_Regular(pFan0)), 1 );
        Vec_BitWriteEntry( vXorIns, Gia_ObjId(p, Gia_Regular(pFan1)), 1 );
    }
    // collect XORs that not inputs of other XORs
    Vec_IntForEachEntry( vXors, Entry, i )
        if ( !Vec_BitEntry(vXorIns, Entry) )
            Vec_IntWriteEntry( vXors, k++, Entry );
    Vec_IntShrink( vXors, k );
    Vec_BitFree( vXorIns );
    return vXors;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Acec_StructAssignRanks( Gia_Man_t * p, Vec_Int_t * vXorRoots )
{
    Vec_Int_t * vDoubles = Vec_IntAlloc( 100 );
    Gia_Obj_t * pFan0, * pFan1, * pObj; 
    int i, k, Fanins[2], Entry, Rank;
    // map roots into their ranks
    Vec_Int_t * vRanks = Vec_IntStartFull( Gia_ManObjNum(p) ); 
    Vec_IntForEachEntry( vXorRoots, Entry, i )
        Vec_IntWriteEntry( vRanks, Entry, i );
    // map nodes into their ranks
    Gia_ManForEachAndReverse( p, pObj, i )
    {
        if ( !Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
            continue;
        Rank = Vec_IntEntry( vRanks, i );
        // skip XORs that are not part of any tree
        if ( Rank == -1 )
            continue;
        // iterate through XOR inputs
        Fanins[0] = Gia_ObjId(p, Gia_Regular(pFan0));
        Fanins[1] = Gia_ObjId(p, Gia_Regular(pFan1));
        for ( k = 0; k < 2; k++ )
        {
            Entry = Vec_IntEntry( vRanks, Fanins[k] );
            if ( Entry == Rank ) // the same tree -- allow fanout in this tree
                continue;
            if ( Entry == -1 )
                Vec_IntWriteEntry( vRanks, Fanins[k], Rank );
            else
                Vec_IntPush( vDoubles, Fanins[k] );
            if ( Entry != -1 && Gia_ObjIsAnd(Gia_ManObj(p, Fanins[k])))
            printf( "Xor node %d belongs to Tree %d and Tree %d.\n", Fanins[k], Entry, Rank );
        }
    }
    // remove duplicated entries
    Vec_IntForEachEntry( vDoubles, Entry, i )
        Vec_IntWriteEntry( vRanks, Entry, -1 );
    Vec_IntFree( vDoubles );
    return vRanks;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wec_t * Acec_FindTreeLeaves( Gia_Man_t * p, Vec_Int_t * vXorRoots, Vec_Int_t * vRanks )
{
    Vec_Bit_t * vMapXors = Vec_BitStart( Gia_ManObjNum(p) );
    Vec_Wec_t * vTreeLeaves = Vec_WecStart( Vec_IntSize(vXorRoots) );
    Gia_Obj_t * pFan0, * pFan1, * pObj; 
    int i, k, Fanins[2], Rank;
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( !Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
            continue;
        Vec_BitWriteEntry( vMapXors, i, 1 );
        Rank = Vec_IntEntry( vRanks, i );
        // skip XORs that are not part of any tree
        if ( Rank == -1 )
            continue;
        // iterate through XOR inputs
        Fanins[0] = Gia_ObjId(p, Gia_Regular(pFan0));
        Fanins[1] = Gia_ObjId(p, Gia_Regular(pFan1));
        for ( k = 0; k < 2; k++ )
        {
            if ( Vec_BitEntry(vMapXors, Fanins[k]) )
            {
                assert( Rank == Vec_IntEntry(vRanks, Fanins[k]) );
                continue;
            }
            Vec_WecPush( vTreeLeaves, Rank, Fanins[k] );
        }
    }
    Vec_BitFree( vMapXors );
    return vTreeLeaves;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Acec_FindShadows( Gia_Man_t * p, Vec_Int_t * vRanks )
{
    Vec_Int_t * vShadows = Vec_IntDup( vRanks );
    Gia_Obj_t * pObj; int i, Shad0, Shad1;
    Gia_ManForEachCi( p, pObj, i )
        Vec_IntWriteEntry( vShadows, Gia_ObjId(p, pObj), -1 );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Vec_IntEntry(vShadows, i) >= 0 )
            continue;
        Shad0 = Vec_IntEntry(vShadows, Gia_ObjFaninId0(pObj, i));
        Shad1 = Vec_IntEntry(vShadows, Gia_ObjFaninId1(pObj, i));
        if ( Shad0 == Shad1 && Shad0 != -1 )
            Vec_IntWriteEntry(vShadows, i, Shad0);
    }
    return vShadows;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Acec_CollectSupp_rec( Gia_Man_t * p, int iNode, int Rank, Vec_Int_t * vRanks )
{
    Gia_Obj_t * pObj;
    int nSize;
    if ( Gia_ObjIsTravIdCurrentId(p, iNode) )
        return 0;
    Gia_ObjSetTravIdCurrentId(p, iNode);
    pObj = Gia_ManObj(p, iNode);
    assert( Gia_ObjIsAnd(pObj) );
    if ( Vec_IntEntry(vRanks, iNode) == Rank )
        return 1;
    nSize = Acec_CollectSupp_rec( p, Gia_ObjFaninId0(pObj, iNode), Rank, vRanks );
    nSize += Acec_CollectSupp_rec( p, Gia_ObjFaninId1(pObj, iNode), Rank, vRanks );
    return nSize;
}
Vec_Wec_t * Acec_FindNexts( Gia_Man_t * p, Vec_Int_t * vRanks, Vec_Int_t * vShadows, Vec_Wec_t * vTreeLeaves )
{
    Vec_Wec_t * vNexts = Vec_WecStart( Vec_WecSize(vTreeLeaves) );
    Vec_Int_t * vTree;
    int i, k, Node, Fanins[2], Shad0, Shad1, Rank, nSupp;
    Vec_WecForEachLevel( vTreeLeaves, vTree, i )
        Vec_IntForEachEntry( vTree, Node, k )
        {
            Gia_Obj_t * pObj = Gia_ManObj(p, Node);
            if ( !Gia_ObjIsAnd(pObj) )
                continue;
            Fanins[0] = Gia_ObjFaninId0(pObj, Node);
            Fanins[1] = Gia_ObjFaninId1(pObj, Node);
            Shad0 = Vec_IntEntry(vShadows, Fanins[0]);
            Shad1 = Vec_IntEntry(vShadows, Fanins[1]);
            if ( Shad0 != Shad1 || Shad0 == -1 )
                continue;
            // check support size of Node in terms of the shadow of its fanins
            Rank = Vec_IntEntry( vRanks, Node );
            assert( Rank != Shad0 );
            Gia_ManIncrementTravId( p );
            nSupp = Acec_CollectSupp_rec( p, Node, Shad0, vRanks );
            assert( nSupp > 1 );
            if ( nSupp > 3 )
                continue;
            Vec_IntPushUniqueOrder( Vec_WecEntry(vNexts, Shad0), Rank );
        }
    return vNexts;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_StructTest( Gia_Man_t * p )
{
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

