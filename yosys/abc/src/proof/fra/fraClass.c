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

ABC_NAMESPACE_IMPL_START


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
    p = ABC_ALLOC( Fra_Cla_t, 1 );
    memset( p, 0, sizeof(Fra_Cla_t) );
    p->pAig = pAig;
    p->pMemRepr  = ABC_ALLOC( Aig_Obj_t *, Aig_ManObjNumMax(pAig) );
    memset( p->pMemRepr, 0, sizeof(Aig_Obj_t *) * Aig_ManObjNumMax(pAig) );
    p->vClasses     = Vec_PtrAlloc( 100 );
    p->vClasses1    = Vec_PtrAlloc( 100 );
    p->vClassesTemp = Vec_PtrAlloc( 100 );
    p->vClassOld    = Vec_PtrAlloc( 100 );
    p->vClassNew    = Vec_PtrAlloc( 100 );
    p->pFuncNodeHash      = Fra_SmlNodeHash;
    p->pFuncNodeIsConst   = Fra_SmlNodeIsConst;
    p->pFuncNodesAreEqual = Fra_SmlNodesAreEqual;
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
    ABC_FREE( p->pMemClasses );
    ABC_FREE( p->pMemRepr );
    if ( p->vClassesTemp ) Vec_PtrFree( p->vClassesTemp );
    if ( p->vClassNew )    Vec_PtrFree( p->vClassNew );
    if ( p->vClassOld )    Vec_PtrFree( p->vClassOld );
    if ( p->vClasses1 )    Vec_PtrFree( p->vClasses1 );
    if ( p->vClasses )     Vec_PtrFree( p->vClasses );
    if ( p->vImps )        Vec_IntFree( p->vImps );
    ABC_FREE( p );
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
    Aig_ManReprStart( p->pAig, Aig_ManObjNumMax(p->pAig) );
    memmove( p->pAig->pReprs, p->pMemRepr, sizeof(Aig_Obj_t *) * Aig_ManObjNumMax(p->pAig) );
    if ( Vec_PtrSize(p->vClasses1) == 0 && Vec_PtrSize(p->vClasses) == 0 )
    {
        Aig_ManForEachObj( p->pAig, pObj, i )
        {
            if ( p->pAig->pReprs[i] != NULL )
                printf( "Classes are not cleared!\n" );
            assert( p->pAig->pReprs[i] == NULL );
        }
    }
    if ( vFailed )
        Vec_PtrForEachEntry( Aig_Obj_t *, vFailed, pObj, i )
            p->pAig->pReprs[pObj->Id] = NULL;
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
    for ( i = 0; (pTemp = pClass[i]); i++ );
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
    Vec_PtrForEachEntry( Aig_Obj_t **, p->vClasses, pClass, i )
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
    Vec_PtrForEachEntry( Aig_Obj_t **, p->vClasses, pClass, i )
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
void Fra_PrintClass( Fra_Cla_t * p, Aig_Obj_t ** pClass )
{
    Aig_Obj_t * pTemp;
    int i;
    for ( i = 1; (pTemp = pClass[i]); i++ )
        assert( Fra_ClassObjRepr(pTemp) == pClass[0] );
    printf( "{ " );
    for ( i = 0; (pTemp = pClass[i]); i++ )
        printf( "%d(%d,%d) ", pTemp->Id, pTemp->Level, Aig_SupportSize(p->pAig,pTemp) );
    printf( "}\n" );
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

    printf( "Const = %5d. Class = %5d. Lit = %5d. ", 
        Vec_PtrSize(p->vClasses1), Vec_PtrSize(p->vClasses), Fra_ClassesCountLits(p) );
    if ( p->vImps && Vec_IntSize(p->vImps) > 0 )
        printf( "Imp = %5d. ", Vec_IntSize(p->vImps) );
    printf( "\n" );

    if ( fVeryVerbose )
    {
        Vec_PtrForEachEntry( Aig_Obj_t *, p->vClasses1, pObj, i )
            assert( Fra_ClassObjRepr(pObj) == Aig_ManConst1(p->pAig) );
        printf( "Constants { " );
        Vec_PtrForEachEntry( Aig_Obj_t *, p->vClasses1, pObj, i )
            printf( "%d(%d,%d) ", pObj->Id, pObj->Level, Aig_SupportSize(p->pAig,pObj) );
        printf( "}\n" );
        Vec_PtrForEachEntry( Aig_Obj_t **, p->vClasses, pClass, i )
        {
            printf( "%3d (%3d) : ", i, Fra_ClassCount(pClass) );
            Fra_PrintClass( p, pClass );
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
void Fra_ClassesPrepare( Fra_Cla_t * p, int fLatchCorr, int nMaxLevs )
{
    Aig_Obj_t ** ppTable, ** ppNexts;
    Aig_Obj_t * pObj, * pTemp;
    int i, k, nTableSize, nEntries, nNodes, iEntry;

    // allocate the hash table hashing simulation info into nodes
    nTableSize = Abc_PrimeCudd( Aig_ManObjNumMax(p->pAig) );
    ppTable = ABC_FALLOC( Aig_Obj_t *, nTableSize ); 
    ppNexts = ABC_FALLOC( Aig_Obj_t *, nTableSize ); 
    memset( ppTable, 0, sizeof(Aig_Obj_t *) * nTableSize );

    // add all the nodes to the hash table
    Vec_PtrClear( p->vClasses1 );
    Aig_ManForEachObj( p->pAig, pObj, i )
    {
        if ( fLatchCorr )
        {
            if ( !Aig_ObjIsCi(pObj) )
                continue;
        }
        else
        {
            if ( !Aig_ObjIsNode(pObj) && !Aig_ObjIsCi(pObj) )
                continue;
            // skip the node with more that the given number of levels
            if ( nMaxLevs && (int)pObj->Level > nMaxLevs )
                continue;
        }
        // hash the node by its simulation info
        iEntry = p->pFuncNodeHash( pObj, nTableSize );
        // check if the node belongs to the class of constant 1
        if ( p->pFuncNodeIsConst( pObj ) )
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
    p->pMemClasses = ABC_ALLOC( Aig_Obj_t *, 2*(nEntries + Vec_PtrSize(p->vClasses1)) );
    p->pMemClassesFree = p->pMemClasses + 2*nEntries;
 
    // copy the entries into storage in the topological order
    Vec_PtrClear( p->vClasses );
    nEntries = 0;
    Aig_ManForEachObj( p->pAig, pObj, i )
    {
        if ( !Aig_ObjIsNode(pObj) && !Aig_ObjIsCi(pObj) )
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
        p->pMemClasses[2*nEntries + nNodes] = NULL;
        // increment the number of entries
        nEntries += k;
    }
    ABC_FREE( ppTable );
    ABC_FREE( ppNexts );
    // now it is time to refine the classes
    Fra_ClassesRefine( p );
//    Fra_ClassesPrint( p, 0 );
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
    for ( ppThis = ppClass + 1; (pObj = *ppThis); ppThis++ )        
        if ( !p->pFuncNodesAreEqual(ppClass[0], pObj) )
            break;
    if ( pObj == NULL )
        return NULL;
    // split the class
    Vec_PtrClear( p->vClassOld );
    Vec_PtrClear( p->vClassNew );
    Vec_PtrPush( p->vClassOld, ppClass[0] );
    for ( ppThis = ppClass + 1; (pObj = *ppThis); ppThis++ )        
        if ( p->pFuncNodesAreEqual(ppClass[0], pObj) )
            Vec_PtrPush( p->vClassOld, pObj );
        else
            Vec_PtrPush( p->vClassNew, pObj );
/*
    printf( "Refining class (" );
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vClassOld, pObj, i )
        printf( "%d,", pObj->Id );
    printf( ") + (" );
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vClassNew, pObj, i )
        printf( "%d,", pObj->Id );
    printf( ")\n" );
*/
    // put the nodes back into the class memory
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vClassOld, pObj, i )
    {
        ppClass[i] = pObj;
        ppClass[Vec_PtrSize(p->vClassOld)+i] = NULL;
        Fra_ClassObjSetRepr( pObj, i? ppClass[0] : NULL );
    }
    ppClass += 2*Vec_PtrSize(p->vClassOld);
    // put the new nodes into the class memory
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vClassNew, pObj, i )
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
    pClass = (Aig_Obj_t **)Vec_PtrEntryLast( vClasses );
    for ( nRefis = 0; (pClass2 = Fra_RefineClassOne( p, pClass )); nRefis++ )
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
    Vec_PtrForEachEntry( Aig_Obj_t **, p->vClasses, pClass, i )
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
    return nRefis;
}

/**Function*************************************************************

  Synopsis    [Refines constant 1 equivalence class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClassesRefine1( Fra_Cla_t * p, int fRefineNewClass, int * pSkipped )
{
    Aig_Obj_t * pObj, ** ppClass;
    int i, k, nRefis = 1;
    // check if there is anything to refine
    if ( Vec_PtrSize(p->vClasses1) == 0 )
        return 0;
    // make sure constant 1 class contains only non-constant nodes
    assert( Vec_PtrEntry(p->vClasses1,0) != Aig_ManConst1(p->pAig) );
    // collect all the nodes to be refined
    k = 0;
    Vec_PtrClear( p->vClassNew );
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vClasses1, pObj, i )
    {
        if ( p->pFuncNodeIsConst( pObj ) )
            Vec_PtrWriteEntry( p->vClasses1, k++, pObj );
        else 
            Vec_PtrPush( p->vClassNew, pObj );
    }
    Vec_PtrShrink( p->vClasses1, k );
    if ( Vec_PtrSize(p->vClassNew) == 0 )
        return 0;
/*
    printf( "Refined const-1 class: {" );
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vClassNew, pObj, i )
        printf( " %d", pObj->Id );
    printf( " }\n" );
*/
    if ( Vec_PtrSize(p->vClassNew) == 1 )
    {
        Fra_ClassObjSetRepr( (Aig_Obj_t *)Vec_PtrEntry(p->vClassNew,0), NULL );
        return 1;
    }
    // create a new class composed of these nodes
    ppClass = p->pMemClassesFree;
    p->pMemClassesFree += 2 * Vec_PtrSize(p->vClassNew);
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vClassNew, pObj, i )
    {
        ppClass[i] = pObj;
        ppClass[Vec_PtrSize(p->vClassNew)+i] = NULL;
        Fra_ClassObjSetRepr( pObj, i? ppClass[0] : NULL );
    }
    assert( ppClass[0] != NULL );
    Vec_PtrPush( p->vClasses, ppClass );
    // iteratively refine this class
    if ( fRefineNewClass )
        nRefis += Fra_RefineClassLastIter( p, p->vClasses );
    else if ( pSkipped )
        (*pSkipped)++;
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
    p->pMemClasses = ABC_ALLOC( Aig_Obj_t *, 4 );
    pClass = p->pMemClasses;
    assert( Id1 < Id2 );
    pClass[0] = Aig_ManObj( p->pAig, Id1 );
    pClass[1] = Aig_ManObj( p->pAig, Id2 );
    pClass[2] = NULL;
    pClass[3] = NULL;
    Fra_ClassObjSetRepr( pClass[1], pClass[0] );
    Vec_PtrPush( p->vClasses, pClass );
}

/**Function*************************************************************

  Synopsis    [Creates latch correspondence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClassesLatchCorr( Fra_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, nEntries = 0;
    Vec_PtrClear( p->pCla->vClasses1 );
    Aig_ManForEachLoSeq( p->pManAig, pObj, i )
    {
        Vec_PtrPush( p->pCla->vClasses1, pObj );
        Fra_ClassObjSetRepr( pObj, Aig_ManConst1(p->pManAig) );
    }
    // allocate room for classes
    p->pCla->pMemClasses = ABC_ALLOC( Aig_Obj_t *, 2*(nEntries + Vec_PtrSize(p->pCla->vClasses1)) );
    p->pCla->pMemClassesFree = p->pCla->pMemClasses + 2*nEntries;
}

/**Function*************************************************************

  Synopsis    [Postprocesses the classes by removing half of the less useful.]

  Description []
               
  SideEffects [] 

  SeeAlso     []

***********************************************************************/
void Fra_ClassesPostprocess( Fra_Cla_t * p )
{
    int Ratio = 2;
    Fra_Sml_t * pComb;
    Aig_Obj_t * pObj, * pRepr, ** ppClass;
    int * pWeights, WeightMax = 0, i, k, c;
    // perform combinational simulation
    pComb = Fra_SmlSimulateComb( p->pAig, 32, 0 );
    // compute the weight of each node in the classes
    pWeights = ABC_ALLOC( int, Aig_ManObjNumMax(p->pAig) );
    memset( pWeights, 0, sizeof(int) * Aig_ManObjNumMax(p->pAig) );
    Aig_ManForEachObj( p->pAig, pObj, i )
    { 
        pRepr = Fra_ClassObjRepr( pObj );
        if ( pRepr == NULL )
            continue;
        pWeights[i] = Fra_SmlNodeNotEquWeight( pComb, pRepr->Id, pObj->Id );
        WeightMax = Abc_MaxInt( WeightMax, pWeights[i] );
    }
    Fra_SmlStop( pComb );
    printf( "Before: Const = %6d. Class = %6d.  ", Vec_PtrSize(p->vClasses1), Vec_PtrSize(p->vClasses) );
    // remove nodes from classes whose weight is less than WeightMax/Ratio
    k = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vClasses1, pObj, i )
    {
        if ( pWeights[pObj->Id] >= WeightMax/Ratio )
            Vec_PtrWriteEntry( p->vClasses1, k++, pObj );
        else
            Fra_ClassObjSetRepr( pObj, NULL );
    }
    Vec_PtrShrink( p->vClasses1, k );
    // in each class, compact the nodes
    Vec_PtrForEachEntry( Aig_Obj_t **, p->vClasses, ppClass, i )
    {
        k = 1;
        for ( c = 1; ppClass[c]; c++ )
        {
            if ( pWeights[ppClass[c]->Id] >= WeightMax/Ratio )
                ppClass[k++] = ppClass[c];
            else
                Fra_ClassObjSetRepr( ppClass[c], NULL );
        }
        ppClass[k] = NULL;
    }
    // remove classes with only repr
    k = 0;
    Vec_PtrForEachEntry( Aig_Obj_t **, p->vClasses, ppClass, i )
        if ( ppClass[1] != NULL )
            Vec_PtrWriteEntry( p->vClasses, k++, ppClass );
    Vec_PtrShrink( p->vClasses, k );
    printf( "After: Const = %6d. Class = %6d. \n", Vec_PtrSize(p->vClasses1), Vec_PtrSize(p->vClasses) );
    ABC_FREE( pWeights );
}

/**Function*************************************************************

  Synopsis    [Postprocesses the classes by selecting representative lowest in top order.]

  Description []
               
  SideEffects [] 

  SeeAlso     []

***********************************************************************/
void Fra_ClassesSelectRepr( Fra_Cla_t * p )
{
    Aig_Obj_t ** pClass, * pNodeMin;
    int i, c, cMinSupp, nSuppSizeMin, nSuppSizeCur;
    // reassign representatives in each class
    Vec_PtrForEachEntry( Aig_Obj_t **, p->vClasses, pClass, i )
    {
        // collect support sizes and find the min-support node
        cMinSupp = -1;
        pNodeMin = NULL;
        nSuppSizeMin = ABC_INFINITY;
        for ( c = 0; pClass[c]; c++ )
        {
            nSuppSizeCur = Aig_SupportSize( p->pAig, pClass[c] );
//            nSuppSizeCur = 1;
            if ( nSuppSizeMin > nSuppSizeCur || 
                (nSuppSizeMin == nSuppSizeCur && pNodeMin->Level > pClass[c]->Level) )
            {
                nSuppSizeMin = nSuppSizeCur;
                pNodeMin = pClass[c];
                cMinSupp = c; 
            }
        }
        // skip the case when the repr did not change
        if ( cMinSupp == 0 )
            continue;
        // make the new node the representative of the class
        pClass[cMinSupp] = pClass[0];
        pClass[0] = pNodeMin;
        // set the representative
        for ( c = 0; pClass[c]; c++ )
            Fra_ClassObjSetRepr( pClass[c], c? pClass[0] : NULL );
    }
}



static inline Aig_Obj_t * Fra_ObjEqu( Aig_Obj_t ** ppEquivs, Aig_Obj_t * pObj )                       { return ppEquivs[pObj->Id];  }
static inline void        Fra_ObjSetEqu( Aig_Obj_t ** ppEquivs, Aig_Obj_t * pObj, Aig_Obj_t * pNode ) { ppEquivs[pObj->Id] = pNode; }

static inline Aig_Obj_t * Fra_ObjChild0Equ( Aig_Obj_t ** ppEquivs, Aig_Obj_t * pObj ) { return Aig_NotCond(Fra_ObjEqu(ppEquivs,Aig_ObjFanin0(pObj)), Aig_ObjFaninC0(pObj));  }
static inline Aig_Obj_t * Fra_ObjChild1Equ( Aig_Obj_t ** ppEquivs, Aig_Obj_t * pObj ) { return Aig_NotCond(Fra_ObjEqu(ppEquivs,Aig_ObjFanin1(pObj)), Aig_ObjFaninC1(pObj));  }

/**Function*************************************************************

  Synopsis    [Add the node and its constraints to the new AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fra_ClassesDeriveNode( Aig_Man_t * pManFraig, Aig_Obj_t * pObj, Aig_Obj_t ** ppEquivs )
{
    Aig_Obj_t * pObjNew, * pObjRepr, * pObjReprNew, * pMiter;//, * pObjNew2;
    // skip nodes without representative
    if ( (pObjRepr = Fra_ClassObjRepr(pObj)) == NULL )
        return;
    assert( pObjRepr->Id < pObj->Id );
    // get the new node 
    pObjNew = Fra_ObjEqu( ppEquivs, pObj );
    // get the new node of the representative
    pObjReprNew = Fra_ObjEqu( ppEquivs, pObjRepr );
    // if this is the same node, no need to add constraints
    if ( Aig_Regular(pObjNew) == Aig_Regular(pObjReprNew) )
        return;
    // these are different nodes - perform speculative reduction
//    pObjNew2 = Aig_NotCond( pObjReprNew, pObj->fPhase ^ pObjRepr->fPhase );
    // set the new node
//    Fra_ObjSetEqu( ppEquivs, pObj, pObjNew2 );
    // add the constraint
    pMiter = Aig_Exor( pManFraig, Aig_Regular(pObjNew), Aig_Regular(pObjReprNew) );
    pMiter = Aig_NotCond( pMiter, Aig_Regular(pMiter)->fPhase ^ Aig_IsComplement(pMiter) );
    pMiter = Aig_Not( pMiter );
    Aig_ObjCreateCo( pManFraig, pMiter );
}

/**Function*************************************************************

  Synopsis    [Derives AIG for the partitioned problem.]

  Description []
               
  SideEffects [] 

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_ClassesDeriveAig( Fra_Cla_t * p, int nFramesK )
{
    Aig_Man_t * pManFraig;
    Aig_Obj_t * pObj, * pObjNew;
    Aig_Obj_t ** pLatches, ** ppEquivs;
    int i, k, f, nFramesAll = nFramesK + 1;
    assert( Aig_ManRegNum(p->pAig) > 0 );
    assert( Aig_ManRegNum(p->pAig) < Aig_ManCiNum(p->pAig) );
    assert( nFramesK > 0 );
    // start the fraig package
    pManFraig = Aig_ManStart( Aig_ManObjNumMax(p->pAig) * nFramesAll );
    pManFraig->pName = Abc_UtilStrsav( p->pAig->pName );
    pManFraig->pSpec = Abc_UtilStrsav( p->pAig->pSpec );
    // allocate place for the node mapping
    ppEquivs = ABC_ALLOC( Aig_Obj_t *, Aig_ManObjNumMax(p->pAig) );
    Fra_ObjSetEqu( ppEquivs, Aig_ManConst1(p->pAig), Aig_ManConst1(pManFraig) );
    // create latches for the first frame
    Aig_ManForEachLoSeq( p->pAig, pObj, i )
        Fra_ObjSetEqu( ppEquivs, pObj, Aig_ObjCreateCi(pManFraig) );
    // add timeframes
    pLatches = ABC_ALLOC( Aig_Obj_t *, Aig_ManRegNum(p->pAig) );
    for ( f = 0; f < nFramesAll; f++ )
    {
        // create PIs for this frame
        Aig_ManForEachPiSeq( p->pAig, pObj, i )
            Fra_ObjSetEqu( ppEquivs, pObj, Aig_ObjCreateCi(pManFraig) );
        // set the constraints on the latch outputs
        Aig_ManForEachLoSeq( p->pAig, pObj, i )
            Fra_ClassesDeriveNode( pManFraig, pObj, ppEquivs );
        // add internal nodes of this frame
        Aig_ManForEachNode( p->pAig, pObj, i )
        {
            pObjNew = Aig_And( pManFraig, Fra_ObjChild0Equ(ppEquivs, pObj), Fra_ObjChild1Equ(ppEquivs, pObj) );
            Fra_ObjSetEqu( ppEquivs, pObj, pObjNew );
            Fra_ClassesDeriveNode( pManFraig, pObj, ppEquivs );
        }
        if ( f == nFramesAll - 1 )
            break;
        if ( f == nFramesAll - 2 )
            pManFraig->nAsserts = Aig_ManCoNum(pManFraig);
        // save the latch input values
        k = 0;
        Aig_ManForEachLiSeq( p->pAig, pObj, i )
            pLatches[k++] = Fra_ObjChild0Equ( ppEquivs, pObj );
        // insert them to the latch output values
        k = 0;
        Aig_ManForEachLoSeq( p->pAig, pObj, i )
            Fra_ObjSetEqu( ppEquivs, pObj, pLatches[k++] );
    }
    ABC_FREE( pLatches );
    ABC_FREE( ppEquivs );
    // mark the asserts
    assert( Aig_ManCoNum(pManFraig) % nFramesAll == 0 );
printf( "Assert miters = %6d. Output miters = %6d.\n", 
       pManFraig->nAsserts, Aig_ManCoNum(pManFraig) - pManFraig->nAsserts );
    // remove dangling nodes
    Aig_ManCleanup( pManFraig );
    return pManFraig;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

