/**CFile****************************************************************
 
  FileName    [aigCheck.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [AIG checking procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigCheck.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

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

  Synopsis    [Checks the consistency of the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManCheck( Aig_Man_t * p )
{
    Aig_Obj_t * pObj, * pObj2;
    int i;
    // check primary inputs
    Aig_ManForEachCi( p, pObj, i )
    {
        if ( Aig_ObjFanin0(pObj) || Aig_ObjFanin1(pObj) )
        {
            printf( "Aig_ManCheck: The PI node \"%p\" has fanins.\n", pObj );
            return 0;
        }
    }
    // check primary outputs
    Aig_ManForEachCo( p, pObj, i )
    {
        if ( !Aig_ObjFanin0(pObj) )
        {
            printf( "Aig_ManCheck: The PO node \"%p\" has NULL fanin.\n", pObj );
            return 0;
        }
        if ( Aig_ObjFanin1(pObj) )
        {
            printf( "Aig_ManCheck: The PO node \"%p\" has second fanin.\n", pObj );
            return 0;
        }
    }
    // check internal nodes
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( !Aig_ObjIsNode(pObj) )
            continue;
        if ( !Aig_ObjFanin0(pObj) || !Aig_ObjFanin1(pObj) )
        {
            printf( "Aig_ManCheck: The AIG has internal node \"%p\" with a NULL fanin.\n", pObj );
            return 0;
        }
        if ( Aig_ObjFanin0(pObj)->Id >= Aig_ObjFanin1(pObj)->Id )
        {
            printf( "Aig_ManCheck: The AIG has node \"%p\" with a wrong ordering of fanins.\n", pObj );
            return 0;
        }
        pObj2 = Aig_TableLookup( p, pObj );
        if ( pObj2 != pObj )
        {
            printf( "Aig_ManCheck: Node \"%p\" is not in the structural hashing table.\n", pObj );
            return 0;
        }
    }
    // count the total number of nodes
    if ( Aig_ManObjNum(p) != 1 + Aig_ManCiNum(p) + Aig_ManCoNum(p) + 
        Aig_ManBufNum(p) + Aig_ManAndNum(p) + Aig_ManExorNum(p) )
    {
        printf( "Aig_ManCheck: The number of created nodes is wrong.\n" );
        printf( "C1 = %d. Pi = %d. Po = %d. Buf = %d. And = %d. Xor = %d. Total = %d.\n",
            1, Aig_ManCiNum(p), Aig_ManCoNum(p), Aig_ManBufNum(p), Aig_ManAndNum(p), Aig_ManExorNum(p), 
            1 + Aig_ManCiNum(p) + Aig_ManCoNum(p) + Aig_ManBufNum(p) + Aig_ManAndNum(p) + Aig_ManExorNum(p) );
        printf( "Created = %d. Deleted = %d. Existing = %d.\n",
            Aig_ManObjNumMax(p), p->nDeleted, Aig_ManObjNum(p) );
        return 0;
    }
    // count the number of nodes in the table
    if ( Aig_TableCountEntries(p) != Aig_ManAndNum(p) + Aig_ManExorNum(p) )
    {
        printf( "Aig_ManCheck: The number of nodes in the structural hashing table is wrong.\n" );
        printf( "Entries = %d. And = %d. Xor = %d. Total = %d.\n", 
            Aig_TableCountEntries(p), Aig_ManAndNum(p), Aig_ManExorNum(p), 
            Aig_ManAndNum(p) + Aig_ManExorNum(p) );

        return 0;
    }
//    if ( !Aig_ManIsAcyclic(p) )
//        return 0;
    return 1; 
}

/**Function*************************************************************

  Synopsis    [Checks if the markA is reset.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCheckMarkA( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( p, pObj, i )
        assert( pObj->fMarkA == 0 );
}

/**Function*************************************************************

  Synopsis    [Checks the consistency of phase assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCheckPhase( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( p, pObj, i )
        if ( Aig_ObjIsCi(pObj) )
            assert( (int)pObj->fPhase == 0 );
        else
            assert( (int)pObj->fPhase == (Aig_ObjPhaseReal(Aig_ObjChild0(pObj)) & Aig_ObjPhaseReal(Aig_ObjChild1(pObj))) );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

