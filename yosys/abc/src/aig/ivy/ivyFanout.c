/**CFile****************************************************************

  FileName    [ivyFanout.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [Representation of the fanouts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyFanout.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// getting hold of the next fanout of the node
static inline Ivy_Obj_t * Ivy_ObjNextFanout( Ivy_Obj_t * pObj, Ivy_Obj_t * pFanout )
{
    assert( !Ivy_IsComplement(pObj) );
    assert( !Ivy_IsComplement(pFanout) );
    if ( pFanout == NULL )
        return NULL;
    if ( Ivy_ObjFanin0(pFanout) == pObj )
        return pFanout->pNextFan0;
    assert( Ivy_ObjFanin1(pFanout) == pObj );
    return pFanout->pNextFan1;
}

// getting hold of the previous fanout of the node
static inline Ivy_Obj_t * Ivy_ObjPrevFanout( Ivy_Obj_t * pObj, Ivy_Obj_t * pFanout )
{
    assert( !Ivy_IsComplement(pObj) );
    assert( !Ivy_IsComplement(pFanout) );
    if ( pFanout == NULL )
        return NULL;
    if ( Ivy_ObjFanin0(pFanout) == pObj )
        return pFanout->pPrevFan0;
    assert( Ivy_ObjFanin1(pFanout) == pObj );
    return pFanout->pPrevFan1;
}

// getting hold of the place where the next fanout will be attached
static inline Ivy_Obj_t ** Ivy_ObjNextFanoutPlace( Ivy_Obj_t * pObj, Ivy_Obj_t * pFanout )
{
    assert( !Ivy_IsComplement(pObj) );
    assert( !Ivy_IsComplement(pFanout) );
    if ( Ivy_ObjFanin0(pFanout) == pObj )
        return &pFanout->pNextFan0;
    assert( Ivy_ObjFanin1(pFanout) == pObj );
    return &pFanout->pNextFan1;
}

// getting hold of the place where the next fanout will be attached
static inline Ivy_Obj_t ** Ivy_ObjPrevFanoutPlace( Ivy_Obj_t * pObj, Ivy_Obj_t * pFanout )
{
    assert( !Ivy_IsComplement(pObj) );
    assert( !Ivy_IsComplement(pFanout) );
    if ( Ivy_ObjFanin0(pFanout) == pObj )
        return &pFanout->pPrevFan0;
    assert( Ivy_ObjFanin1(pFanout) == pObj );
    return &pFanout->pPrevFan1;
}

// getting hold of the place where the next fanout will be attached
static inline Ivy_Obj_t ** Ivy_ObjPrevNextFanoutPlace( Ivy_Obj_t * pObj, Ivy_Obj_t * pFanout )
{
    Ivy_Obj_t * pTemp;
    assert( !Ivy_IsComplement(pObj) );
    assert( !Ivy_IsComplement(pFanout) );
    pTemp = Ivy_ObjPrevFanout(pObj, pFanout);
    if ( pTemp == NULL )
        return &pObj->pFanout;
    if ( Ivy_ObjFanin0(pTemp) == pObj )
        return &pTemp->pNextFan0;
    assert( Ivy_ObjFanin1(pTemp) == pObj );
    return &pTemp->pNextFan1;
}

// getting hold of the place where the next fanout will be attached
static inline Ivy_Obj_t ** Ivy_ObjNextPrevFanoutPlace( Ivy_Obj_t * pObj, Ivy_Obj_t * pFanout )
{
    Ivy_Obj_t * pTemp;
    assert( !Ivy_IsComplement(pObj) );
    assert( !Ivy_IsComplement(pFanout) );
    pTemp = Ivy_ObjNextFanout(pObj, pFanout);
    if ( pTemp == NULL )
        return NULL;
    if ( Ivy_ObjFanin0(pTemp) == pObj )
        return &pTemp->pPrevFan0;
    assert( Ivy_ObjFanin1(pTemp) == pObj );
    return &pTemp->pPrevFan1;
}

// iterator through the fanouts of the node
#define Ivy_ObjForEachFanoutInt( pObj, pFanout )                 \
    for ( pFanout = (pObj)->pFanout; pFanout;                    \
          pFanout = Ivy_ObjNextFanout(pObj, pFanout) )

// safe iterator through the fanouts of the node
#define Ivy_ObjForEachFanoutIntSafe( pObj, pFanout, pFanout2 )   \
    for ( pFanout  = (pObj)->pFanout,                            \
          pFanout2 = Ivy_ObjNextFanout(pObj, pFanout);           \
          pFanout;                                               \
          pFanout  = pFanout2,                                   \
          pFanout2 = Ivy_ObjNextFanout(pObj, pFanout) )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the fanout representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManStartFanout( Ivy_Man_t * p )
{
    Ivy_Obj_t * pObj;
    int i;
    assert( !p->fFanout );
    p->fFanout = 1;
    Ivy_ManForEachObj( p, pObj, i )
    {
        if ( Ivy_ObjFanin0(pObj) )
            Ivy_ObjAddFanout( p, Ivy_ObjFanin0(pObj), pObj );
        if ( Ivy_ObjFanin1(pObj) )
            Ivy_ObjAddFanout( p, Ivy_ObjFanin1(pObj), pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Stops the fanout representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManStopFanout( Ivy_Man_t * p )
{
    Ivy_Obj_t * pObj;
    int i;
    assert( p->fFanout );
    p->fFanout = 0;
    Ivy_ManForEachObj( p, pObj, i )
        pObj->pFanout = pObj->pNextFan0 = pObj->pNextFan1 = pObj->pPrevFan0 = pObj->pPrevFan1 = NULL;
}

/**Function*************************************************************

  Synopsis    [Add the fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ObjAddFanout( Ivy_Man_t * p, Ivy_Obj_t * pFanin, Ivy_Obj_t * pFanout )
{
    assert( p->fFanout );
    if ( pFanin->pFanout )
    {
        *Ivy_ObjNextFanoutPlace(pFanin, pFanout) = pFanin->pFanout;
        *Ivy_ObjPrevFanoutPlace(pFanin, pFanin->pFanout) = pFanout;
    }
    pFanin->pFanout = pFanout;
}

/**Function*************************************************************

  Synopsis    [Removes the fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ObjDeleteFanout( Ivy_Man_t * p, Ivy_Obj_t * pFanin, Ivy_Obj_t * pFanout )
{
    Ivy_Obj_t ** ppPlace1, ** ppPlace2, ** ppPlaceN;
    assert( pFanin->pFanout != NULL );

    ppPlace1 = Ivy_ObjNextFanoutPlace(pFanin, pFanout);
    ppPlaceN = Ivy_ObjPrevNextFanoutPlace(pFanin, pFanout);
    assert( *ppPlaceN == pFanout );
    if ( ppPlaceN )
        *ppPlaceN = *ppPlace1;

    ppPlace2 = Ivy_ObjPrevFanoutPlace(pFanin, pFanout);
    ppPlaceN = Ivy_ObjNextPrevFanoutPlace(pFanin, pFanout);
    assert( ppPlaceN == NULL || *ppPlaceN == pFanout );
    if ( ppPlaceN )
        *ppPlaceN = *ppPlace2;

    *ppPlace1 = NULL;
    *ppPlace2 = NULL;
}

/**Function*************************************************************

  Synopsis    [Replaces the fanout of pOld to be pFanoutNew.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ObjPatchFanout( Ivy_Man_t * p, Ivy_Obj_t * pFanin, Ivy_Obj_t * pFanoutOld, Ivy_Obj_t * pFanoutNew )
{
    Ivy_Obj_t ** ppPlace;
    ppPlace = Ivy_ObjPrevNextFanoutPlace(pFanin, pFanoutOld);
    assert( *ppPlace == pFanoutOld );
    if ( ppPlace )
        *ppPlace = pFanoutNew;
    ppPlace = Ivy_ObjNextPrevFanoutPlace(pFanin, pFanoutOld);
    assert( ppPlace == NULL || *ppPlace == pFanoutOld );
    if ( ppPlace )
        *ppPlace = pFanoutNew;
    // assuming that pFanoutNew already points to the next fanout
}

/**Function*************************************************************

  Synopsis    [Starts iteration through the fanouts.]

  Description [Copies the currently available fanouts into the array.]
               
  SideEffects [Can be used while the fanouts are being removed.]

  SeeAlso     []

***********************************************************************/
void Ivy_ObjCollectFanouts( Ivy_Man_t * p, Ivy_Obj_t * pObj, Vec_Ptr_t * vArray )
{
    Ivy_Obj_t * pFanout;
    assert( p->fFanout );
    assert( !Ivy_IsComplement(pObj) );
    Vec_PtrClear( vArray );
    Ivy_ObjForEachFanoutInt( pObj, pFanout )
        Vec_PtrPush( vArray, pFanout );
}

/**Function*************************************************************

  Synopsis    [Reads one fanout.]

  Description [Returns fanout if there is only one fanout.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_ObjReadFirstFanout( Ivy_Man_t * p, Ivy_Obj_t * pObj )
{
    return pObj->pFanout;
}

/**Function*************************************************************

  Synopsis    [Reads one fanout.]

  Description [Returns fanout if there is only one fanout.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ObjFanoutNum( Ivy_Man_t * p, Ivy_Obj_t * pObj )
{
    Ivy_Obj_t * pFanout;
    int Counter = 0;
    Ivy_ObjForEachFanoutInt( pObj, pFanout )
        Counter++;
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

