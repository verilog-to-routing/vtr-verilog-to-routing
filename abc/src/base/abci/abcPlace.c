/**CFile****************************************************************

  FileName    [abcPlace.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Interface with a placer.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcPlace.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

// placement includes
#include "phys/place/place_base.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

AbstractCell *abstractCells = NULL;
ConcreteCell *cells = NULL;
ConcreteNet *nets = NULL;
int nAllocSize = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates a new cell.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_PlaceCreateCell( Abc_Obj_t * pObj, int fAnd )
{
    assert( cells[pObj->Id].m_id == 0 );

    cells[pObj->Id].m_id = pObj->Id;
    cells[pObj->Id].m_label = "";
    cells[pObj->Id].m_parent = &(abstractCells[fAnd]);
    cells[pObj->Id].m_fixed = 0;
    addConcreteCell(&(cells[pObj->Id]));
}

/**Function*************************************************************

  Synopsis    [Updates the net.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_PlaceUpdateNet( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout;
    int k;
    // free the old array of net terminals
    if ( nets[pObj->Id].m_terms )
        free( nets[pObj->Id].m_terms );
    // fill in the net with the new information
    nets[pObj->Id].m_id = pObj->Id;
    nets[pObj->Id].m_weight = 1.0;
    nets[pObj->Id].m_numTerms = Abc_ObjFanoutNum(pObj); //fanout
    nets[pObj->Id].m_terms = ALLOC(ConcreteCell*, Abc_ObjFanoutNum(pObj));
    Abc_ObjForEachFanout( pObj, pFanout, k )
        nets[pObj->Id].m_terms[k] = &(cells[pFanout->Id]);
    addConcreteNet(&(nets[pObj->Id]));
}

/**Function*************************************************************

  Synopsis    [Returns the placement cost of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_PlaceEvaluateCut( Abc_Obj_t * pRoot, Vec_Ptr_t * vFanins )
{
    Abc_Obj_t * pObj;
//    double x, y;
    int i;
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanins, pObj, i )
    {
//        pObj->Id
    }
    return 0.0;
}

/**Function*************************************************************

  Synopsis    [Updates placement after one step of rewriting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_PlaceUpdate( Vec_Ptr_t * vAddedCells, Vec_Ptr_t * vUpdatedNets )
{
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    Vec_Ptr_t * vCells, * vNets;

    // start the arrays of new cells and nets
    vCells = Vec_PtrAlloc( 16 );
    vNets = Vec_PtrAlloc( 32 );

    // go through the new nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vAddedCells, pObj, i )
    {
        assert( !Abc_ObjIsComplement(pObj) );
        Abc_PlaceCreateCell( pObj, 1 );
        Abc_PlaceUpdateNet( pObj );

        // add the new cell and its fanin nets to temporary storage
        Vec_PtrPush( vCells, &(cells[pObj->Id]) );
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Vec_PtrPushUnique( vNets, &(nets[pFanin->Id]) );
    }

    // go through the modified nets
    Vec_PtrForEachEntry( Abc_Obj_t *, vUpdatedNets, pObj, i )
    {
        assert( !Abc_ObjIsComplement(pObj) );
        if ( Abc_ObjType(pObj) == ABC_OBJ_NONE ) // dead node
            continue;
        Abc_PlaceUpdateNet( pObj );
    }

    // update the placement
//    fastPlace( Vec_PtrSize(vCells), (ConcreteCell **)Vec_PtrArray(vCells), 
//               Vec_PtrSize(vNets), (ConcreteNet **)Vec_PtrArray(vNets) );

    // clean up
    Vec_PtrFree( vCells );
    Vec_PtrFree( vNets );
}

/**Function*************************************************************

  Synopsis    [This procedure is called before the writing start.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_PlaceBegin( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;

    // allocate and clean internal storage
    nAllocSize = 5 * Abc_NtkObjNumMax(pNtk);
    cells = REALLOC(ConcreteCell, cells, nAllocSize);
    nets  = REALLOC(ConcreteNet, nets, nAllocSize);
    memset( cells, 0, sizeof(ConcreteCell) * nAllocSize );
    memset( nets, 0, sizeof(ConcreteNet) * nAllocSize );

    // create AbstractCells
    //   1: pad
    //   2: and
    if (!abstractCells)
        abstractCells = ALLOC(AbstractCell,2);

    abstractCells[0].m_height = 1.0;
    abstractCells[0].m_width = 1.0;
    abstractCells[0].m_label = "pio";
    abstractCells[0].m_pad = 1;

    abstractCells[1].m_height = 1.0;
    abstractCells[1].m_width = 1.0;
    abstractCells[1].m_label = "and";
    abstractCells[1].m_pad = 0;

    // input pads
    Abc_NtkForEachCi( pNtk, pObj, i )
        Abc_PlaceCreateCell( pObj, 0 );

    // ouput pads
    Abc_NtkForEachCo( pNtk, pObj, i )
        Abc_PlaceCreateCell( pObj, 0 );

    // AND nodes
    Abc_AigForEachAnd( pNtk, pObj, i )
        Abc_PlaceCreateCell( pObj, 1 );

    // all nets
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( !Abc_ObjIsCi(pObj) && !Abc_ObjIsNode(pObj) )
            continue;
        Abc_PlaceUpdateNet( pObj );
    }

    globalPreplace((float)0.8);
    globalPlace();
}

/**Function*************************************************************

  Synopsis    [This procedure is called after the writing completes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_PlaceEnd( Abc_Ntk_t * pNtk )
{
    int i;


    // clean up
    for ( i = 0; i < nAllocSize; i++ )
        FREE( nets[i].m_terms );
    FREE( abstractCells );
    FREE( cells );
    FREE( nets );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

