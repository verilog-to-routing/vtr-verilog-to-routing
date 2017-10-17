/**CFile****************************************************************

  FileName    [seqLatch.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [Manipulation of latch data structures representing initial states.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: seqLatch.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "seqInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Insert the first Lat on the edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NodeInsertFirst( Abc_Obj_t * pObj, int Edge, Abc_InitType_t Init )  
{ 
    Seq_Lat_t * pLat, * pRing, * pPrev;
    pRing = Seq_NodeGetRing( pObj, Edge );
    pLat  = Seq_NodeCreateLat( pObj );
    if ( pRing == NULL )
    {
        Seq_LatSetPrev( pLat, pLat );
        Seq_LatSetNext( pLat, pLat );
        Seq_NodeSetRing( pObj, Edge, pLat );
    }
    else
    {
        pPrev = Seq_LatPrev( pRing );
        Seq_LatSetPrev( pLat, pPrev );
        Seq_LatSetNext( pPrev, pLat );
        Seq_LatSetPrev( pRing, pLat );
        Seq_LatSetNext( pLat, pRing );
        Seq_NodeSetRing( pObj, Edge, pLat );  // rotate the ring to make pLat the first
    }
    Seq_LatSetInit( pLat, Init );
    Seq_ObjAddFaninL( pObj, Edge, 1 );
    assert( pLat->pLatch == NULL );
}

/**Function*************************************************************

  Synopsis    [Insert the last Lat on the edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NodeInsertLast( Abc_Obj_t * pObj, int Edge, Abc_InitType_t Init )  
{ 
    Seq_Lat_t * pLat, * pRing, * pPrev;
    pRing = Seq_NodeGetRing( pObj, Edge );
    pLat  = Seq_NodeCreateLat( pObj );
    if ( pRing == NULL )
    {
        Seq_LatSetPrev( pLat, pLat );
        Seq_LatSetNext( pLat, pLat );
        Seq_NodeSetRing( pObj, Edge, pLat );
    }
    else
    {
        pPrev = Seq_LatPrev( pRing );
        Seq_LatSetPrev( pLat, pPrev );
        Seq_LatSetNext( pPrev, pLat );
        Seq_LatSetPrev( pRing, pLat );
        Seq_LatSetNext( pLat, pRing );
    }
    Seq_LatSetInit( pLat, Init );
    Seq_ObjAddFaninL( pObj, Edge, 1 );
}

/**Function*************************************************************

  Synopsis    [Delete the first Lat on the edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_InitType_t Seq_NodeDeleteFirst( Abc_Obj_t * pObj, int Edge )  
{ 
    Abc_InitType_t Init;
    Seq_Lat_t * pLat, * pRing, * pPrev, * pNext;
    pRing = Seq_NodeGetRing( pObj, Edge );
    pLat  = pRing; // consider the first latch
    if ( pLat->pNext == pLat )
        Seq_NodeSetRing( pObj, Edge, NULL );
    else
    {
        pPrev = Seq_LatPrev( pLat );
        pNext = Seq_LatNext( pLat );
        Seq_LatSetPrev( pNext, pPrev );
        Seq_LatSetNext( pPrev, pNext );
        Seq_NodeSetRing( pObj, Edge, pNext ); // rotate the ring
    }
    Init = Seq_LatInit( pLat );
    Seq_NodeRecycleLat( pObj, pLat );
    Seq_ObjAddFaninL( pObj, Edge, -1 );
    return Init;
}

/**Function*************************************************************

  Synopsis    [Delete the last Lat on the edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_InitType_t Seq_NodeDeleteLast( Abc_Obj_t * pObj, int Edge )  
{
    Abc_InitType_t Init;
    Seq_Lat_t * pLat, * pRing, * pPrev, * pNext;
    pRing = Seq_NodeGetRing( pObj, Edge );
    pLat  = Seq_LatPrev( pRing ); // consider the last latch
    if ( pLat->pNext == pLat )
        Seq_NodeSetRing( pObj, Edge, NULL );
    else
    {
        pPrev = Seq_LatPrev( pLat );
        pNext = Seq_LatNext( pLat );
        Seq_LatSetPrev( pNext, pPrev );
        Seq_LatSetNext( pPrev, pNext );
    }
    Init = Seq_LatInit( pLat );
    Seq_NodeRecycleLat( pObj, pLat ); 
    Seq_ObjAddFaninL( pObj, Edge, -1 );
    return Init;
}

/**Function*************************************************************

  Synopsis    [Insert the last Lat on the edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NodeDupLats( Abc_Obj_t * pObjNew, Abc_Obj_t * pObj, int Edge )  
{ 
    Seq_Lat_t * pRing, * pLat;
    int i, nLatches;
    pRing = Seq_NodeGetRing( pObj, Edge );
    if ( pRing == NULL )
        return;
    nLatches = Seq_NodeCountLats( pObj, Edge );
    for ( i = 0, pLat = pRing; i < nLatches; i++, pLat = pLat->pNext )
        Seq_NodeInsertLast( pObjNew, Edge, Seq_LatInit(pLat) );
}

/**Function*************************************************************

  Synopsis    [Insert the last Lat on the edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NodeCompareLats( Abc_Obj_t * pObj1, int Edge1, Abc_Obj_t * pObj2, int Edge2 )  
{ 
    Seq_Lat_t * pRing1, * pRing2, * pLat1, * pLat2;
    int i, nLatches1, nLatches2;

    nLatches1 = Seq_NodeCountLats( pObj1, Edge1 );
    nLatches2 = Seq_NodeCountLats( pObj2, Edge2 );
    if ( nLatches1 != nLatches2 )
        return 0;

    pRing1 = Seq_NodeGetRing( pObj1, Edge1 );
    pRing2 = Seq_NodeGetRing( pObj2, Edge2 );
    for ( i = 0, pLat1 = pRing1, pLat2 = pRing2; i < nLatches1; i++, pLat1 = pLat1->pNext, pLat2 = pLat2->pNext )
        if ( Seq_LatInit(pLat1) != Seq_LatInit(pLat2) )
            return 0;

    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


