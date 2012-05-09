/**CFile****************************************************************

  FileName    [fraClass.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraClass.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"

/*
    The candidate equivalence classes are stored as a vector of pointers 
    to the array of pointers to the nodes in each class.
    The first node of the class is its representative node.
    The representative has the smallest topological order among the class nodes.
    The nodes inside each class are ordered according to their topological order.
    The classes are ordered according to the topological order of their representatives.
    The array of pointers to the class nodes is terminated with a NULL pointer.
    To enable dynamic addition of new classes (during class refinement),
    each array has at least as many NULLs in the end, as there are nodes in the class.
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline Aig_Obj_t *  Fra_ObjNext( Aig_Obj_t ** ppNexts, Aig_Obj_t * pObj )                       { return ppNexts[pObj->Id];  }
static inline void         Fra_ObjSetNext( Aig_Obj_t ** ppNexts, Aig_Obj_t * pObj, Aig_Obj_t * pNext ) { ppNexts[pObj->Id] = pNext; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts representation of equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fra_Cla_t * Fra_ClassesStart( Aig_Man_t * pAig )
{
    Fra_Cla_t * p;
    p = ALLOC( Fra_Cla_t, 1 );
    memset( p, 0, sizeof(Fra_Cla_t) );
    p->pAig = pAig;
    p->pMemRepr  = ALLOC( Aig_Obj_t *, (Aig_ManObjIdMax(pAig) + 1) );
    memset( p->pMemRepr, 0, sizeof(Aig_Obj_t *) * (Aig_ManObjIdMax(pAig) + 1) );
    p->vClasses     = Vec_PtrAlloc( 100 );
    p->vClasses1    = Vec_PtrAlloc( 100 );
    p->vClassesTemp = Vec_PtrAlloc( 100 );
    p->vClassOld    = Vec_PtrAlloc( 100 );
    p->vClassNew    = Vec_PtrAlloc( 100 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stop representation of equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClassesStop( Fra_Cla_t * p )
{
    FREE( p->pMemClasses );
    FREE( p->pMemRepr );
    if ( p->vClassesTemp ) Vec_PtrFree( p->vClassesTemp );
    if ( p->vClassNew )    Vec_PtrFree( p->vClassNew );
    if ( p->vClassOld )    Vec_PtrFree( p->vClassOld );
    if ( p->vClasses1 )    Vec_PtrFree( p->vClasses1 );
    if ( p->vClasses )     Vec_PtrFree( p->vClasses );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Starts representation of equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClassesCopyReprs( Fra_Cla_t * p, Vec_Ptr_t * vFailed )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManReprStart( p->pAig, Aig_ManObjIdMax(p->pAig) + 1 );
    memmove( p->pAig->pReprs, p->pMemRepr, sizeof(Aig_Obj_t *) * (Aig_ManObjIdMax(p->pAig) + 1) );
    if ( vFailed )
    Vec_PtrForEachEntry( vFailed, pObj, i )
        p->pAig->pReprs[pObj->Id] = NULL;
}

/**Function*************************************************************

  Synopsis    [Prints simulation classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_PrintClass( Aig_Obj_t ** pClass )
{
    Aig_Obj_t * pTemp;
    int i;
    printf( "{ " );
    for ( i = 0; pTemp = pClass[i]; i++ )
        printf( "%d ", pTemp->Id );
    printf( "}\n" );
}

/**Function*************************************************************

  Synopsis    [Prints simulation classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClassCount( Aig_Obj_t ** pClass )
{
    Aig_Obj_t * pTemp;
    int i;
    for ( i = 0; pTemp = pClass[i]; i++ );
    return i;
}

/**Function*************************************************************

  Synopsis    [Count the number of literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClassesCountLits( Fra_Cla_t * p )
{
    Aig_Obj_t ** pClass;
    int i, nNodes, nLits = 0;
    nLits = Vec_PtrSize( p->vClasses1 );
    Vec_PtrForEachEntry( p->vClasses, pClass, i )
    {
        nNodes = Fra_ClassCount( pClass );
        assert( nNodes > 1 );
        nLits += nNodes - 1;
    }
    return nLits;
}

/**Function*************************************************************

  Synopsis    [Count the number of pairs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClassesCountPairs( Fra_Cla_t * p )
{
    Aig_Obj_t ** pClass;
    int i, nNodes, nPairs = 0;
    Vec_PtrForEachEntry( p->vClasses, pClass, i )
    {
        nNodes = Fra_ClassCount( pClass );
        assert( nNodes > 1 );
        nPairs += nNodes * (nNodes - 1) / 2;
    }
    return nPairs;
}

/**Function*************************************************************

  Synopsis    [Prints simulation classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClassesPrint( Fra_Cla_t * p, int fVeryVerbose )
{
    Aig_Obj_t ** pClass;
    Aig_Obj_t * pObj;
    int i;

    printf( "Consts = %6d. Classes = %6d. Literals = %6d.\n", 
        Vec_PtrSize(p->vClasses1), Vec_PtrSize(p->vClasses), Fra_ClassesCountLits(p) );

    if ( fVeryVerbose )
    {
        printf( "Constants { " );
        Vec_PtrForEachEntry( p->vClasses1, pObj, i )
            printf( "%d ", pObj->Id );
        printf( "}\n" );
        Vec_PtrForEachEntry( p->vClasses, pClass, i )
        {
            printf( "%3d (%3d) : ", i, Fra_ClassCount(pClass) );
            Fra_PrintClass( pClass );
        }
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Creates initial simulation classes.]

  Description [Assumes that simulation info is assigned.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClassesPrepare( Fra_Cla_t * p )
{
    Aig_Obj_t ** ppTable, ** ppNexts;
    Aig_Obj_t * pObj, * pTemp;
    int i, k, nTableSize, nEntries, nNodes, iEntry;

    // allocate the hash table hashing simulation info into nodes
    nTableSize = Aig_PrimeCudd( Aig_ManObjIdMax(p->pAig) + 1 );
    ppTable = ALLOC( Aig_Obj_t *, nTableSize ); 
    ppNexts = ALLOC( Aig_Obj_t *, nTableSize ); 
    memset( ppTable, 0, sizeof(Aig_Obj_t *) * nTableSize );

    // add all the nodes to the hash table
    Vec_PtrClear( p->vClasses1 );
    Aig_ManForEachObj( p->pAig, pObj, i )
    {
        if ( !Aig_ObjIsNode(pObj) && !Aig_ObjIsPi(pObj) )
            continue;
//printf( "%3d : ", pObj->Id );
//Extra_PrintBinary( stdout, Fra_ObjSim(pObj), 32 );
//printf( "\n" );
        // hash the node by its simulation info
        iEntry = Fra_NodeHashSims( pObj ) % nTableSize;
        // check if the node belongs to the class of constant 1
        if ( iEntry == 0 && Fra_NodeHasZeroSim( pObj ) )
        {
            Vec_PtrPush( p->vClasses1, pObj );
            Fra_ClassObjSetRepr( pObj, Aig_ManConst1(p->pAig) );
            continue;
        }
        // add the node to the class
        if ( ppTable[iEntry] == NULL )
        {
            ppTable[iEntry] = pObj;
            Fra_ObjSetNext( ppNexts, pObj, pObj );
        }
        else
        {
            Fra_ObjSetNext( ppNexts, pObj, Fra_ObjNext(ppNexts,ppTable[iEntry]) );
            Fra_ObjSetNext( ppNexts, ppTable[iEntry], pObj );
        }
    }

    // count the total number of nodes in the non-trivial classes
    // mark the representative nodes of each equivalence class
    nEntries = 0;
    for ( i = 0; i < nTableSize; i++ )
        if ( ppTable[i] && ppTable[i] != Fra_ObjNext(ppNexts, ppTable[i]) )
        {
            for ( pTemp = Fra_ObjNext(ppNexts, ppTable[i]), k = 1; 
                  pTemp != ppTable[i]; 
                  pTemp = Fra_ObjNext(ppNexts, pTemp), k++ );
            assert( k > 1 );
            nEntries += k;
            // mark the node
            assert( ppTable[i]->fMarkA == 0 );
            ppTable[i]->fMarkA = 1;
        }

    // allocate room for classes
    p->pMemClasses = ALLOC( Aig_Obj_t *, 2*(nEntries + Vec_PtrSize(p->vClasses1)) );
    p->pMemClassesFree = p->pMemClasses + 2*nEntries;

    // copy the entries into storage in the topological order
    Vec_PtrClear( p->vClasses );
    nEntries = 0;
    Aig_ManForEachObj( p->pAig, pObj, i )
    {
        if ( !Aig_ObjIsNode(pObj) && !Aig_ObjIsPi(pObj) )
            continue;
        // skip the nodes that are not representatives of non-trivial classes
        if ( pObj->fMarkA == 0 )
            continue;
        pObj->fMarkA = 0;
        // add the class of nodes
        Vec_PtrPush( p->vClasses, p->pMemClasses + 2*nEntries );
        // count the number of entries in this class
        for ( pTemp = Fra_ObjNext(ppNexts, pObj), k = 1; 
              pTemp != pObj; 
              pTemp = Fra_ObjNext(ppNexts, pTemp), k++ );
        nNodes = k;
        assert( nNodes > 1 );
        // add the nodes to the class in the topological order
        p->pMemClasses[2*nEntries] = pObj;
        for ( pTemp = Fra_ObjNext(ppNexts, pObj), k = 1; 
              pTemp != pObj; 
              pTemp = Fra_ObjNext(ppNexts, pTemp), k++ )
        {
            p->pMemClasses[2*nEntries+nNodes-k] = pTemp;
            Fra_ClassObjSetRepr( pTemp, pObj );
        }
        // add as many empty entries
//        memset( p->pMemClasses + 2*nEntries + nNodes, 0, sizeof(Aig_Obj_t *) * nNodes );
        p->pMemClasses[2*nEntries + nNodes] = NULL;
        // increment the number of entries
        nEntries += k;
    }
    free( ppTable );
    free( ppNexts );
    // now it is time to refine the classes
    Fra_ClassesRefine( p );
}

/**Function*************************************************************

  Synopsis    [Refines one class using simulation info.]

  Description [Returns the new class if refinement happened.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t ** Fra_RefineClassOne( Fra_Cla_t * p, Aig_Obj_t ** ppClass )
{
    Aig_Obj_t * pObj, ** ppThis;
    int i;
    assert( ppClass[0] != NULL && ppClass[1] != NULL );

    // check if the class is going to be refined
    for ( ppThis = ppClass + 1; pObj = *ppThis; ppThis++ )        
        if ( !Fra_NodeCompareSims(ppClass[0], pObj) )
            break;
    if ( pObj == NULL )
        return NULL;
    // split the class
    Vec_PtrClear( p->vClassOld );
    Vec_PtrClear( p->vClassNew );
    Vec_PtrPush( p->vClassOld, ppClass[0] );
    for ( ppThis = ppClass + 1; pObj = *ppThis; ppThis++ )        
        if ( Fra_NodeCompareSims(ppClass[0], pObj) )
            Vec_PtrPush( p->vClassOld, pObj );
        else
            Vec_PtrPush( p->vClassNew, pObj );
/*
    printf( "Refining class (" );
    Vec_PtrForEachEntry( p->vClassOld, pObj, i )
        printf( "%d,", pObj->Id );
    printf( ") + (" );
    Vec_PtrForEachEntry( p->vClassNew, pObj, i )
        printf( "%d,", pObj->Id );
    printf( ")\n" );
*/
    // put the nodes back into the class memory
    Vec_PtrForEachEntry( p->vClassOld, pObj, i )
    {
        ppClass[i] = pObj;
        ppClass[Vec_PtrSize(p->vClassOld)+i] = NULL;
        Fra_ClassObjSetRepr( pObj, i? ppClass[0] : NULL );
    }
    ppClass += 2*Vec_PtrSize(p->vClassOld);
    // put the new nodes into the class memory
    Vec_PtrForEachEntry( p->vClassNew, pObj, i )
    {
        ppClass[i] = pObj;
        ppClass[Vec_PtrSize(p->vClassNew)+i] = NULL;
        Fra_ClassObjSetRepr( pObj, i? ppClass[0] : NULL );
    }
    return ppClass;
}

/**Function*************************************************************

  Synopsis    [Iteratively refines the classes after simulation.]

  Description [Returns the number of refinements performed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_RefineClassLastIter( Fra_Cla_t * p, Vec_Ptr_t * vClasses )
{
    Aig_Obj_t ** pClass, ** pClass2;
    int nRefis;
    pClass = Vec_PtrEntryLast( vClasses );
    for ( nRefis = 0; pClass2 = Fra_RefineClassOne( p, pClass ); nRefis++ )
    {
        // if the original class is trivial, remove it
        if ( pClass[1] == NULL )
            Vec_PtrPop( vClasses );
        // if the new class is trivial, stop
        if ( pClass2[1] == NULL )
        {
            nRefis++;
            break;
        }
        // othewise, add the class and continue
        assert( pClass2[0] != NULL );
        Vec_PtrPush( vClasses, pClass2 );
        pClass = pClass2;
    }
    return nRefis;
}

/**Function*************************************************************

  Synopsis    [Refines the classes after simulation.]

  Description [Assumes that simulation info is assigned. Returns the
  number of classes refined.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClassesRefine( Fra_Cla_t * p )
{
    Vec_Ptr_t * vTemp;
    Aig_Obj_t ** pClass;
    int i, nRefis;
    // refine the classes
    nRefis = 0;
    Vec_PtrClear( p->vClassesTemp );
    Vec_PtrForEachEntry( p->vClasses, pClass, i )
    {
        // add the class to the new array
        assert( pClass[0] != NULL );
        Vec_PtrPush( p->vClassesTemp, pClass );
        // refine the class iteratively
        nRefis += Fra_RefineClassLastIter( p, p->vClassesTemp );
    }
    // exchange the class representation
    vTemp = p->vClassesTemp;
    p->vClassesTemp = p->vClasses;
    p->vClasses = vTemp;
    p->fRefinement = (nRefis > 0);
    return nRefis;
}

/**Function*************************************************************

  Synopsis    [Refines constant 1 equivalence class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClassesRefine1( Fra_Cla_t * p )
{
    Aig_Obj_t * pObj, ** ppClass;
    int i, k, nRefis;
    // check if there is anything to refine
    if ( Vec_PtrSize(p->vClasses1) == 0 )
        return 0;
    // make sure constant 1 class contains only non-constant nodes
    assert( Vec_PtrEntry(p->vClasses1,0) != Aig_ManConst1(p->pAig) );
    // collect all the nodes to be refined
    k = 0;
    Vec_PtrClear( p->vClassNew );
    Vec_PtrForEachEntry( p->vClasses1, pObj, i )
    {
        if ( Fra_NodeHasZeroSim( pObj ) )
            Vec_PtrWriteEntry( p->vClasses1, k++, pObj );
        else 
            Vec_PtrPush( p->vClassNew, pObj );
    }
    Vec_PtrShrink( p->vClasses1, k );
    if ( Vec_PtrSize(p->vClassNew) == 0 )
        return 0;
    p->fRefinement = 1;
    if ( Vec_PtrSize(p->vClassNew) == 1 )
    {
        Fra_ClassObjSetRepr( Vec_PtrEntry(p->vClassNew,0), NULL );
        return 1;
    }
    // create a new class composed of these nodes
    ppClass = p->pMemClassesFree;
    p->pMemClassesFree += 2 * Vec_PtrSize(p->vClassNew);
    Vec_PtrForEachEntry( p->vClassNew, pObj, i )
    {
        ppClass[i] = pObj;
        ppClass[Vec_PtrSize(p->vClassNew)+i] = NULL;
        Fra_ClassObjSetRepr( pObj, i? ppClass[0] : NULL );
    }
    assert( ppClass[0] != NULL );
    Vec_PtrPush( p->vClasses, ppClass );
    // iteratively refine this class
    nRefis = 1 + Fra_RefineClassLastIter( p, p->vClasses );
    return nRefis;
}

/**Function*************************************************************

  Synopsis    [Starts representation of equivalence classes with one class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClassesTest( Fra_Cla_t * p, int Id1, int Id2 )
{
    Aig_Obj_t ** pClass;
    p->pMemClasses = ALLOC( Aig_Obj_t *, 4 );
    pClass = p->pMemClasses;
    assert( Id1 < Id2 );
    pClass[0] = Aig_ManObj( p->pAig, Id1 );
    pClass[1] = Aig_ManObj( p->pAig, Id2 );
    pClass[2] = NULL;
    pClass[3] = NULL;
    Fra_ClassObjSetRepr( pClass[1], pClass[0] );
    Vec_PtrPush( p->vClasses, pClass );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


