/**CFile****************************************************************

  FileName    [sswClass.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [Representation of candidate equivalence classes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswClass.c,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sswInt.h"

ABC_NAMESPACE_IMPL_START


/*
    The candidate equivalence classes are stored as a vector of pointers 
    to the array of pointers to the nodes in each class.
    The first node of the class is its representative node.
    The representative has the smallest topological order among the class nodes.
    The nodes inside each class are ordered according to their topological order.
    The classes are ordered according to the topo order of their representatives.
*/

// internal representation of candidate equivalence classes
struct Ssw_Cla_t_
{
    // class information
    Aig_Man_t *      pAig;             // original AIG manager
    Aig_Obj_t ***    pId2Class;        // non-const classes by ID of repr node
    int *            pClassSizes;      // sizes of each equivalence class
    int              fConstCorr;
    // statistics
    int              nClasses;         // the total number of non-const classes
    int              nCands1;          // the total number of const candidates
    int              nLits;            // the number of literals in all classes
    // memory
    Aig_Obj_t **     pMemClasses;      // memory allocated for equivalence classes
    Aig_Obj_t **     pMemClassesFree;  // memory allocated for equivalence classes to be used
    // temporary data
    Vec_Ptr_t *      vClassOld;        // old equivalence class after splitting
    Vec_Ptr_t *      vClassNew;        // new equivalence class(es) after splitting
    Vec_Ptr_t *      vRefined;         // the nodes refined since the last iteration
    // procedures used for class refinement
    void *           pManData;
    unsigned (*pFuncNodeHash) (void *,Aig_Obj_t *);              // returns hash key of the node
    int (*pFuncNodeIsConst)   (void *,Aig_Obj_t *);              // returns 1 if the node is a constant
    int (*pFuncNodesAreEqual) (void *,Aig_Obj_t *, Aig_Obj_t *); // returns 1 if nodes are equal up to a complement
};

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline Aig_Obj_t *  Ssw_ObjNext( Aig_Obj_t ** ppNexts, Aig_Obj_t * pObj )                       { return ppNexts[pObj->Id];  }
static inline void         Ssw_ObjSetNext( Aig_Obj_t ** ppNexts, Aig_Obj_t * pObj, Aig_Obj_t * pNext ) { ppNexts[pObj->Id] = pNext; }

// iterator through the equivalence classes
#define Ssw_ManForEachClass( p, ppClass, i )                 \
    for ( i = 0; i < Aig_ManObjNumMax(p->pAig); i++ )        \
        if ( ((ppClass) = p->pId2Class[i]) == NULL ) {} else
// iterator through the nodes in one class
#define Ssw_ClassForEachNode( p, pRepr, pNode, i )           \
    for ( i = 0; i < p->pClassSizes[pRepr->Id]; i++ )        \
        if ( ((pNode) = p->pId2Class[pRepr->Id][i]) == NULL ) {} else

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates one equivalence class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Ssw_ObjAddClass( Ssw_Cla_t * p, Aig_Obj_t * pRepr, Aig_Obj_t ** pClass, int nSize )
{
    assert( p->pId2Class[pRepr->Id] == NULL );
    assert( pClass[0] == pRepr );
    p->pId2Class[pRepr->Id] = pClass;
    assert( p->pClassSizes[pRepr->Id] == 0 );
    assert( nSize > 1 );
    p->pClassSizes[pRepr->Id] = nSize;
    p->nClasses++;
    p->nLits += nSize - 1;
}

/**Function*************************************************************

  Synopsis    [Removes one equivalence class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Aig_Obj_t ** Ssw_ObjRemoveClass( Ssw_Cla_t * p, Aig_Obj_t * pRepr )
{
    Aig_Obj_t ** pClass = p->pId2Class[pRepr->Id];
    int nSize;
    assert( pClass != NULL );
    p->pId2Class[pRepr->Id] = NULL;
    nSize = p->pClassSizes[pRepr->Id];
    assert( nSize > 1 );
    p->nClasses--;
    p->nLits -= nSize - 1;
    p->pClassSizes[pRepr->Id] = 0;
    return pClass;
}

/**Function*************************************************************

  Synopsis    [Starts representation of equivalence classes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Ssw_Cla_t * Ssw_ClassesStart( Aig_Man_t * pAig )
{
    Ssw_Cla_t * p;
    p = ABC_ALLOC( Ssw_Cla_t, 1 );
    memset( p, 0, sizeof(Ssw_Cla_t) );
    p->pAig         = pAig;
    p->pId2Class    = ABC_CALLOC( Aig_Obj_t **, Aig_ManObjNumMax(pAig) );
    p->pClassSizes  = ABC_CALLOC( int, Aig_ManObjNumMax(pAig) );
    p->vClassOld    = Vec_PtrAlloc( 100 );
    p->vClassNew    = Vec_PtrAlloc( 100 );
    p->vRefined     = Vec_PtrAlloc( 1000 );
    if ( pAig->pReprs == NULL )
        Aig_ManReprStart( pAig, Aig_ManObjNumMax(pAig) );
    return p;
}

/**Function*************************************************************

  Synopsis    [Starts representation of equivalence classes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ClassesSetData( Ssw_Cla_t * p, void * pManData,
    unsigned (*pFuncNodeHash)(void *,Aig_Obj_t *),               // returns hash key of the node
    int (*pFuncNodeIsConst)(void *,Aig_Obj_t *),                 // returns 1 if the node is a constant
    int (*pFuncNodesAreEqual)(void *,Aig_Obj_t *, Aig_Obj_t *) ) // returns 1 if nodes are equal up to a complement
{
    p->pManData           = pManData;
    p->pFuncNodeHash      = pFuncNodeHash;
    p->pFuncNodeIsConst   = pFuncNodeIsConst;
    p->pFuncNodesAreEqual = pFuncNodesAreEqual;
}

/**Function*************************************************************

  Synopsis    [Stop representation of equivalence classes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ClassesStop( Ssw_Cla_t * p )
{
    if ( p->vClassNew )    Vec_PtrFree( p->vClassNew );
    if ( p->vClassOld )    Vec_PtrFree( p->vClassOld );
    Vec_PtrFree( p->vRefined );
    ABC_FREE( p->pId2Class );
    ABC_FREE( p->pClassSizes );
    ABC_FREE( p->pMemClasses );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Stop representation of equivalence classes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Ssw_ClassesReadAig( Ssw_Cla_t * p )
{
    return p->pAig;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Ssw_ClassesGetRefined( Ssw_Cla_t * p )
{
    return p->vRefined;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ClassesClearRefined( Ssw_Cla_t * p )
{
    Vec_PtrClear( p->vRefined );
}

/**Function*************************************************************

  Synopsis    [Stop representation of equivalence classes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ClassesCand1Num( Ssw_Cla_t * p )
{
    return p->nCands1;
}

/**Function*************************************************************

  Synopsis    [Stop representation of equivalence classes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ClassesClassNum( Ssw_Cla_t * p )
{
    return p->nClasses;
}

/**Function*************************************************************

  Synopsis    [Stop representation of equivalence classes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ClassesLitNum( Ssw_Cla_t * p )
{
    return p->nLits;
}

/**Function*************************************************************

  Synopsis    [Stop representation of equivalence classes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t ** Ssw_ClassesReadClass( Ssw_Cla_t * p, Aig_Obj_t * pRepr, int * pnSize )
{
    if ( p->pId2Class[pRepr->Id] == NULL )
        return NULL;
    assert( p->pId2Class[pRepr->Id] != NULL );
    assert( p->pClassSizes[pRepr->Id] > 1 );
    *pnSize = p->pClassSizes[pRepr->Id];
    return p->pId2Class[pRepr->Id];
}

/**Function*************************************************************

  Synopsis    [Stop representation of equivalence classes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ClassesCollectClass( Ssw_Cla_t * p, Aig_Obj_t * pRepr, Vec_Ptr_t * vClass )
{
    int i;
    Vec_PtrClear( vClass );
    if ( p->pId2Class[pRepr->Id] == NULL )
        return;
    assert( p->pClassSizes[pRepr->Id] > 1 );
    for ( i = 1; i < p->pClassSizes[pRepr->Id]; i++ )
        Vec_PtrPush( vClass, p->pId2Class[pRepr->Id][i] );
}

/**Function*************************************************************

  Synopsis    [Checks candidate equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ClassesCheck( Ssw_Cla_t * p )
{
    Aig_Obj_t * pObj, * pPrev, ** ppClass;
    int i, k, nLits, nClasses, nCands1;
    nClasses = nLits = 0;
    Ssw_ManForEachClass( p, ppClass, k )
    {
        pPrev = NULL;
        assert( p->pClassSizes[ppClass[0]->Id] >= 2 );
        Ssw_ClassForEachNode( p, ppClass[0], pObj, i )
        {
            if ( i == 0 )
                assert( Aig_ObjRepr(p->pAig, pObj) == NULL );
            else
            {
                assert( Aig_ObjRepr(p->pAig, pObj) == ppClass[0] );
                assert( pPrev->Id < pObj->Id );
                nLits++;
            }
            pPrev = pObj;
        }
        nClasses++;
    }
    nCands1 = 0;
    Aig_ManForEachObj( p->pAig, pObj, i )
        nCands1 += Ssw_ObjIsConst1Cand( p->pAig, pObj );
    assert( p->nLits == nLits );
    assert( p->nCands1 == nCands1 );
    assert( p->nClasses == nClasses );
}

/**Function*************************************************************

  Synopsis    [Prints simulation classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ClassesPrintOne( Ssw_Cla_t * p, Aig_Obj_t * pRepr )
{
    Aig_Obj_t * pObj;
    int i;
    Abc_Print( 1, "{ " );
    Ssw_ClassForEachNode( p, pRepr, pObj, i )
        Abc_Print( 1, "%d(%d,%d,%d) ", pObj->Id, pObj->Level,
        Aig_SupportSize(p->pAig,pObj), Aig_NodeMffcSupp(p->pAig,pObj,0,NULL) );
    Abc_Print( 1, "}\n" );
}

/**Function*************************************************************

  Synopsis    [Prints simulation classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ClassesPrint( Ssw_Cla_t * p, int fVeryVerbose )
{
    Aig_Obj_t ** ppClass;
    Aig_Obj_t * pObj;
    int i;
    Abc_Print( 1, "Equiv classes: Const1 = %5d. Class = %5d. Lit = %5d.\n",
        p->nCands1, p->nClasses, p->nCands1+p->nLits );
    if ( !fVeryVerbose )
        return;
    Abc_Print( 1, "Constants { " );
    Aig_ManForEachObj( p->pAig, pObj, i )
        if ( Ssw_ObjIsConst1Cand( p->pAig, pObj ) )
            Abc_Print( 1, "%d(%d,%d,%d) ", pObj->Id, pObj->Level,
            Aig_SupportSize(p->pAig,pObj), Aig_NodeMffcSupp(p->pAig,pObj,0,NULL) );
    Abc_Print( 1, "}\n" );
    Ssw_ManForEachClass( p, ppClass, i )
    {
        Abc_Print( 1, "%3d (%3d) : ", i, p->pClassSizes[i] );
        Ssw_ClassesPrintOne( p, ppClass[0] );
    }
    Abc_Print( 1, "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints simulation classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ClassesRemoveNode( Ssw_Cla_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t * pRepr, * pTemp;
    assert( p->pClassSizes[pObj->Id] == 0 );
    assert( p->pId2Class[pObj->Id] == NULL );
    pRepr = Aig_ObjRepr( p->pAig, pObj );
    assert( pRepr != NULL );
//    Vec_PtrPush( p->vRefined, pObj );
    if ( Ssw_ObjIsConst1Cand( p->pAig, pObj ) )
    {
        assert( p->pClassSizes[pRepr->Id] == 0 );
        assert( p->pId2Class[pRepr->Id] == NULL );
        Aig_ObjSetRepr( p->pAig, pObj, NULL );
        p->nCands1--;
        return;
    }
//    Vec_PtrPush( p->vRefined, pRepr );
    Aig_ObjSetRepr( p->pAig, pObj, NULL );
    assert( p->pId2Class[pRepr->Id][0] == pRepr );
    assert( p->pClassSizes[pRepr->Id] >= 2 );
    if ( p->pClassSizes[pRepr->Id] == 2 )
    {
        p->pId2Class[pRepr->Id] = NULL;
        p->nClasses--;
        p->pClassSizes[pRepr->Id] = 0;
        p->nLits--;
    }
    else
    {
        int i, k = 0;
        // remove the entry from the class
        Ssw_ClassForEachNode( p, pRepr, pTemp, i )
            if ( pTemp != pObj )
                p->pId2Class[pRepr->Id][k++] = pTemp;
        assert( k + 1 == p->pClassSizes[pRepr->Id] );
        // reduce the class
        p->pClassSizes[pRepr->Id]--;
        p->nLits--;
    }
}

/**Function*************************************************************

  Synopsis    [Takes the set of const1 cands and rehashes them using sim info.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ClassesPrepareRehash( Ssw_Cla_t * p, Vec_Ptr_t * vCands, int fConstCorr )
{
//    Aig_Man_t * pAig = p->pAig;
    Aig_Obj_t ** ppTable, ** ppNexts, ** ppClassNew;
    Aig_Obj_t * pObj, * pTemp, * pRepr;
    int i, k, nTableSize, nNodes, iEntry, nEntries, nEntries2;

    // allocate the hash table hashing simulation info into nodes
    nTableSize = Abc_PrimeCudd( Vec_PtrSize(vCands)/2 );
    ppTable = ABC_CALLOC( Aig_Obj_t *, nTableSize );
    ppNexts = ABC_CALLOC( Aig_Obj_t *, Aig_ManObjNumMax(p->pAig) );

    // sort through the candidates
    nEntries = 0;
    p->nCands1 = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vCands, pObj, i )
    {
        assert( p->pClassSizes[pObj->Id] == 0 );
        Aig_ObjSetRepr( p->pAig, pObj, NULL );
        // check if the node belongs to the class of constant 1
        if ( p->pFuncNodeIsConst( p->pManData, pObj ) )
        {
            Ssw_ObjSetConst1Cand( p->pAig, pObj );
            p->nCands1++;
            continue;
        }
        if ( fConstCorr )
            continue;
        // hash the node by its simulation info
        iEntry = p->pFuncNodeHash( p->pManData, pObj ) % nTableSize;
        // add the node to the class
        if ( ppTable[iEntry] == NULL )
        {
            ppTable[iEntry] = pObj;
        }
        else
        {
            // set the representative of this node
            pRepr = ppTable[iEntry];
            Aig_ObjSetRepr( p->pAig, pObj, pRepr );
            // add node to the table
            if ( Ssw_ObjNext( ppNexts, pRepr ) == NULL )
            { // this will be the second entry
                p->pClassSizes[pRepr->Id]++;
                nEntries++;
            }
            // add the entry to the list
            Ssw_ObjSetNext( ppNexts, pObj, Ssw_ObjNext( ppNexts, pRepr ) );
            Ssw_ObjSetNext( ppNexts, pRepr, pObj );
            p->pClassSizes[pRepr->Id]++;
            nEntries++;
        }
    }

    // copy the entries into storage in the topological order
    nEntries2 = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vCands, pObj, i )
    {
        nNodes = p->pClassSizes[pObj->Id];
        // skip the nodes that are not representatives of non-trivial classes
        if ( nNodes == 0 )
            continue;
        assert( nNodes > 1 );
        // add the nodes to the class in the topological order
        ppClassNew = p->pMemClassesFree + nEntries2;
        ppClassNew[0] = pObj;
        for ( pTemp = Ssw_ObjNext(ppNexts, pObj), k = 1; pTemp;
              pTemp = Ssw_ObjNext(ppNexts, pTemp), k++ )
        {
            ppClassNew[nNodes-k] = pTemp;
        }
        // add the class of nodes
        p->pClassSizes[pObj->Id] = 0;
        Ssw_ObjAddClass( p, pObj, ppClassNew, nNodes );
        // increment the number of entries
        nEntries2 += nNodes;
    }
    p->pMemClassesFree += nEntries2;
    assert( nEntries == nEntries2 );
    ABC_FREE( ppTable );
    ABC_FREE( ppNexts );
    // now it is time to refine the classes
    return Ssw_ClassesRefine( p, 1 );
}

/**Function*************************************************************

  Synopsis    [Creates initial simulation classes.]

  Description [Assumes that simulation info is assigned.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ssw_Cla_t * Ssw_ClassesPrepare( Aig_Man_t * pAig, int nFramesK, int fLatchCorr, int fConstCorr, int fOutputCorr, int nMaxLevs, int fVerbose )
{
//    int nFrames =  4;
//    int nWords  =  1;
//    int nIters  = 16;

//    int nFrames = 32;
//    int nWords  =  4;
//    int nIters  =  0;

    int nFrames =  Abc_MaxInt( nFramesK, 4 );
    int nWords  =  2;
    int nIters  = 16;
    Ssw_Cla_t * p;
    Ssw_Sml_t * pSml;
    Vec_Ptr_t * vCands;
    Aig_Obj_t * pObj;
    int i, k, RetValue;
    abctime clk;

    // start the classes
    p = Ssw_ClassesStart( pAig );
    p->fConstCorr = fConstCorr;

    // perform sequential simulation
clk = Abc_Clock();
    pSml = Ssw_SmlSimulateSeq( pAig, 0, nFrames, nWords );
if ( fVerbose )
{
    Abc_Print( 1, "Allocated %.2f MB to store simulation information.\n",
        1.0*(sizeof(unsigned) * Aig_ManObjNumMax(pAig) * nFrames * nWords)/(1<<20) );
    Abc_Print( 1, "Initial simulation of %d frames with %d words.     ", nFrames, nWords );
    ABC_PRT( "Time", Abc_Clock() - clk );
}

    // set comparison procedures
clk = Abc_Clock();
    Ssw_ClassesSetData( p, pSml, (unsigned(*)(void *,Aig_Obj_t *))Ssw_SmlObjHashWord, (int(*)(void *,Aig_Obj_t *))Ssw_SmlObjIsConstWord, (int(*)(void *,Aig_Obj_t *,Aig_Obj_t *))Ssw_SmlObjsAreEqualWord );

    // collect nodes to be considered as candidates
    vCands = Vec_PtrAlloc( 1000 );
    Aig_ManForEachObj( p->pAig, pObj, i )
    {
        if ( fLatchCorr )
        {
            if ( !Saig_ObjIsLo(p->pAig, pObj) )
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
        Vec_PtrPush( vCands, pObj );
    }

    // this change will consider all PO drivers
    if ( fOutputCorr )
    {
        Vec_PtrClear( vCands );
        Aig_ManForEachObj( p->pAig, pObj, i )
            pObj->fMarkB = 0;
        Saig_ManForEachPo( p->pAig, pObj, i )
            if ( Aig_ObjIsCand(Aig_ObjFanin0(pObj)) )
                Aig_ObjFanin0(pObj)->fMarkB = 1;
        Aig_ManForEachObj( p->pAig, pObj, i )
            if ( pObj->fMarkB )
                Vec_PtrPush( vCands, pObj );
        Aig_ManForEachObj( p->pAig, pObj, i )
            pObj->fMarkB = 0;
    }

    // allocate room for classes
    p->pMemClasses = ABC_ALLOC( Aig_Obj_t *, Vec_PtrSize(vCands) );
    p->pMemClassesFree = p->pMemClasses;

    // now it is time to refine the classes
    Ssw_ClassesPrepareRehash( p, vCands, fConstCorr );
if ( fVerbose )
{
    Abc_Print( 1, "Collecting candidate equivalence classes.        " );
ABC_PRT( "Time", Abc_Clock() - clk );
}

clk = Abc_Clock();
    // perform iterative refinement using simulation
    for ( i = 1; i < nIters; i++ )
    {
        // collect const1 candidates
        Vec_PtrClear( vCands );
        Aig_ManForEachObj( p->pAig, pObj, k )
            if ( Ssw_ObjIsConst1Cand( p->pAig, pObj ) )
                Vec_PtrPush( vCands, pObj );
        assert( Vec_PtrSize(vCands) == p->nCands1 );
        // perform new round of simulation
        Ssw_SmlResimulateSeq( pSml );
        // check equivalence classes
        RetValue = Ssw_ClassesPrepareRehash( p, vCands, fConstCorr );
        if ( RetValue == 0 )
            break;
    }
    Ssw_SmlStop( pSml );
    Vec_PtrFree( vCands );
if ( fVerbose )
{
    Abc_Print( 1, "Simulation of %d frames with %d words (%2d rounds). ",
        nFrames, nWords, i-1 );
    ABC_PRT( "Time", Abc_Clock() - clk );
}
    Ssw_ClassesCheck( p );
//    Ssw_ClassesPrint( p, 0 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates initial simulation classes.]

  Description [Assumes that simulation info is assigned.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ssw_Cla_t * Ssw_ClassesPrepareSimple( Aig_Man_t * pAig, int fLatchCorr, int nMaxLevs )
{
    Ssw_Cla_t * p;
    Aig_Obj_t * pObj;
    int i;
    // start the classes
    p = Ssw_ClassesStart( pAig );
    // go through the nodes
    p->nCands1 = 0;
    Aig_ManForEachObj( pAig, pObj, i )
    {
        if ( fLatchCorr )
        {
            if ( !Saig_ObjIsLo(pAig, pObj) )
                continue;
        }
        else
        {
            if ( !Aig_ObjIsNode(pObj) && !Saig_ObjIsLo(pAig, pObj) )
                continue;
            // skip the node with more that the given number of levels
            if ( nMaxLevs && (int)pObj->Level > nMaxLevs )
                continue;
        }
        Ssw_ObjSetConst1Cand( pAig, pObj );
        p->nCands1++;
    }
    // allocate room for classes
    p->pMemClassesFree = p->pMemClasses = ABC_ALLOC( Aig_Obj_t *, p->nCands1 );
//    Ssw_ClassesPrint( p, 0 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates initial simulation classes.]

  Description [Assumes that simulation info is assigned.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
Ssw_Cla_t * Ssw_ClassesPrepareFromReprs( Aig_Man_t * pAig )
{
    Ssw_Cla_t * p;
    Aig_Obj_t * pObj, * pRepr;
    int * pClassSizes, nEntries, i;
    // start the classes
    p = Ssw_ClassesStart( pAig );
    // allocate memory for classes
    p->pMemClasses = ABC_CALLOC( Aig_Obj_t *, Aig_ManObjNumMax(pAig) );
    // count classes
    p->nCands1 = 0;
    Aig_ManForEachObj( pAig, pObj, i )
    {
        if ( Ssw_ObjIsConst1Cand(pAig, pObj) )
        {
            p->nCands1++;
            continue;
        }
        if ( (pRepr = Aig_ObjRepr(pAig, pObj)) )
        {
            if ( p->pClassSizes[pRepr->Id]++ == 0 )
                p->pClassSizes[pRepr->Id]++;
        }
    }
    // add nodes
    nEntries = 0;
    p->nClasses = 0;
    pClassSizes = ABC_CALLOC( int, Aig_ManObjNumMax(pAig) );
    Aig_ManForEachObj( pAig, pObj, i )
    {
        if ( p->pClassSizes[i] )
        {
            p->pId2Class[i] = p->pMemClasses + nEntries;
            nEntries += p->pClassSizes[i];
            p->pId2Class[i][pClassSizes[i]++] = pObj;
            p->nClasses++;
            continue;
        }
        if ( Ssw_ObjIsConst1Cand(pAig, pObj) )
            continue;
        if ( (pRepr = Aig_ObjRepr(pAig, pObj)) )
            p->pId2Class[pRepr->Id][pClassSizes[pRepr->Id]++] = pObj;
    }
    p->pMemClassesFree = p->pMemClasses + nEntries;
    p->nLits = nEntries - p->nClasses;
    assert( memcmp(pClassSizes, p->pClassSizes, sizeof(int)*Aig_ManObjNumMax(pAig)) == 0 );
    ABC_FREE( pClassSizes );
//    Abc_Print( 1, "After converting:\n" );
//    Ssw_ClassesPrint( p, 0 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates initial simulation classes.]

  Description [Assumes that simulation info is assigned.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ssw_Cla_t * Ssw_ClassesPrepareTargets( Aig_Man_t * pAig )
{
    Ssw_Cla_t * p;
    Aig_Obj_t * pObj;
    int i;
    // start the classes
    p = Ssw_ClassesStart( pAig );
    // go through the nodes
    p->nCands1 = 0;
    Saig_ManForEachPo( pAig, pObj, i )
    {
        Ssw_ObjSetConst1Cand( pAig, Aig_ObjFanin0(pObj) );
        p->nCands1++;
    }
    // allocate room for classes
    p->pMemClassesFree = p->pMemClasses = ABC_ALLOC( Aig_Obj_t *, p->nCands1 );
//    Ssw_ClassesPrint( p, 0 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates classes from the temporary representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ssw_Cla_t * Ssw_ClassesPreparePairs( Aig_Man_t * pAig, Vec_Int_t ** pvClasses )
{
    Ssw_Cla_t * p;
    Aig_Obj_t ** ppClassNew;
    Aig_Obj_t * pObj, * pRepr, * pPrev;
    int i, k, nTotalObjs, nEntries, Entry;
    // start the classes
    p = Ssw_ClassesStart( pAig );
    // count the number of entries in the classes
    nTotalObjs = 0;
    for ( i = 0; i < Aig_ManObjNumMax(pAig); i++ )
        nTotalObjs += pvClasses[i] ? Vec_IntSize(pvClasses[i]) : 0;
    // allocate memory for classes
    p->pMemClasses = ABC_ALLOC( Aig_Obj_t *, nTotalObjs );
    // create constant-1 class
    if ( pvClasses[0] )
    Vec_IntForEachEntry( pvClasses[0], Entry, i )
    {
        assert( (i == 0) == (Entry == 0) );
        if ( i == 0 )
            continue;
        pObj = Aig_ManObj( pAig, Entry );
        Ssw_ObjSetConst1Cand( pAig, pObj );
        p->nCands1++;
    }
    // create classes
    nEntries = 0;
    for ( i = 1; i < Aig_ManObjNumMax(pAig); i++ )
    {
        if ( pvClasses[i] == NULL )
            continue;
        // get room for storing the class
        ppClassNew = p->pMemClasses + nEntries;
        nEntries  += Vec_IntSize( pvClasses[i] );
        // store the nodes of the class
        pPrev = pRepr = Aig_ManObj( pAig, Vec_IntEntry(pvClasses[i],0) );
        ppClassNew[0] = pRepr;
        Vec_IntForEachEntryStart( pvClasses[i], Entry, k, 1 )
        {
            pObj = Aig_ManObj( pAig, Entry );
            assert( pPrev->Id < pObj->Id );
            pPrev = pObj;
            ppClassNew[k] = pObj;
            Aig_ObjSetRepr( pAig, pObj, pRepr );
        }
        // create new class
        Ssw_ObjAddClass( p, pRepr, ppClassNew, Vec_IntSize(pvClasses[i]) );
    }
    // prepare room for new classes
    p->pMemClassesFree = p->pMemClasses + nEntries;
    Ssw_ClassesCheck( p );
//    Ssw_ClassesPrint( p, 0 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates classes from the temporary representation.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Ssw_Cla_t * Ssw_ClassesPreparePairsSimple( Aig_Man_t * pMiter, Vec_Int_t * vPairs )
{
    Ssw_Cla_t * p;
    Aig_Obj_t ** ppClassNew;
    Aig_Obj_t * pObj, * pRepr;
    int i;
    // start the classes
    p = Ssw_ClassesStart( pMiter );
    // allocate memory for classes
    p->pMemClasses = ABC_ALLOC( Aig_Obj_t *, Vec_IntSize(vPairs) );
    // create classes
    for ( i = 0; i < Vec_IntSize(vPairs); i += 2 )
    {
        pRepr = Aig_ManObj( pMiter, Vec_IntEntry(vPairs, i) );
        pObj  = Aig_ManObj( pMiter, Vec_IntEntry(vPairs, i+1) );
        assert( Aig_ObjId(pRepr) < Aig_ObjId(pObj) );
        Aig_ObjSetRepr( pMiter, pObj, pRepr );
        // get room for storing the class
        ppClassNew = p->pMemClasses + i;
        ppClassNew[0] = pRepr;
        ppClassNew[1] = pObj;
        // create new class
        Ssw_ObjAddClass( p, pRepr, ppClassNew, 2 );
    }
    // prepare room for new classes
    p->pMemClassesFree = NULL;
    Ssw_ClassesCheck( p );
//    Ssw_ClassesPrint( p, 0 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Iteratively refines the classes after simulation.]

  Description [Returns the number of refinements performed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ClassesRefineOneClass( Ssw_Cla_t * p, Aig_Obj_t * pReprOld, int fRecursive )
{
    Aig_Obj_t ** pClassOld, ** pClassNew;
    Aig_Obj_t * pObj, * pReprNew;
    int i;

    // split the class
    Vec_PtrClear( p->vClassOld );
    Vec_PtrClear( p->vClassNew );
    Ssw_ClassForEachNode( p, pReprOld, pObj, i )
        if ( p->pFuncNodesAreEqual(p->pManData, pReprOld, pObj) )
            Vec_PtrPush( p->vClassOld, pObj );
        else
            Vec_PtrPush( p->vClassNew, pObj );
    // check if splitting happened
    if ( Vec_PtrSize(p->vClassNew) == 0 )
        return 0;
    // remember that this class is refined
//    Ssw_ClassForEachNode( p, pReprOld, pObj, i )
//        Vec_PtrPush( p->vRefined, pObj );

    // get the new representative
    pReprNew = (Aig_Obj_t *)Vec_PtrEntry( p->vClassNew, 0 );
    assert( Vec_PtrSize(p->vClassOld) > 0 );
    assert( Vec_PtrSize(p->vClassNew) > 0 );

    // create old class
    pClassOld = Ssw_ObjRemoveClass( p, pReprOld );
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vClassOld, pObj, i )
    {
        pClassOld[i] = pObj;
        Aig_ObjSetRepr( p->pAig, pObj, i? pReprOld : NULL );
    }
    // create new class
    pClassNew = pClassOld + i;
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vClassNew, pObj, i )
    {
        pClassNew[i] = pObj;
        Aig_ObjSetRepr( p->pAig, pObj, i? pReprNew : NULL );
    }

    // put classes back
    if ( Vec_PtrSize(p->vClassOld) > 1 )
        Ssw_ObjAddClass( p, pReprOld, pClassOld, Vec_PtrSize(p->vClassOld) );
    if ( Vec_PtrSize(p->vClassNew) > 1 )
        Ssw_ObjAddClass( p, pReprNew, pClassNew, Vec_PtrSize(p->vClassNew) );

    // check if the class should be recursively refined
    if ( fRecursive && Vec_PtrSize(p->vClassNew) > 1 )
        return 1 + Ssw_ClassesRefineOneClass( p, pReprNew, 1 );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Refines the classes after simulation.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ClassesRefine( Ssw_Cla_t * p, int fRecursive )
{
    Aig_Obj_t ** ppClass;
    int i, nRefis = 0;
    Ssw_ManForEachClass( p, ppClass, i )
        nRefis += Ssw_ClassesRefineOneClass( p, ppClass[0], fRecursive );
    return nRefis;
}

/**Function*************************************************************

  Synopsis    [Refines the classes after simulation.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ClassesRefineGroup( Ssw_Cla_t * p, Vec_Ptr_t * vReprs, int fRecursive )
{
    Aig_Obj_t * pObj;
    int i, nRefis = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vReprs, pObj, i )
        nRefis += Ssw_ClassesRefineOneClass( p, pObj, fRecursive );
    return nRefis;
}

/**Function*************************************************************

  Synopsis    [Refine the group of constant 1 nodes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ClassesRefineConst1Group( Ssw_Cla_t * p, Vec_Ptr_t * vRoots, int fRecursive )
{
    Aig_Obj_t * pObj, * pReprNew, ** ppClassNew;
    int i;
    if ( Vec_PtrSize(vRoots) == 0 )
        return 0;
    // collect the nodes to be refined
    Vec_PtrClear( p->vClassNew );
    Vec_PtrForEachEntry( Aig_Obj_t *, vRoots, pObj, i )
        if ( !p->pFuncNodeIsConst( p->pManData, pObj ) )
            Vec_PtrPush( p->vClassNew, pObj );
    // check if there is a new class
    if ( Vec_PtrSize(p->vClassNew) == 0 )
        return 0;
    p->nCands1 -= Vec_PtrSize(p->vClassNew);
    pReprNew = (Aig_Obj_t *)Vec_PtrEntry( p->vClassNew, 0 );
    Aig_ObjSetRepr( p->pAig, pReprNew, NULL );
    if ( Vec_PtrSize(p->vClassNew) == 1 )
        return 1;
    // create a new class composed of these nodes
    ppClassNew = p->pMemClassesFree;
    p->pMemClassesFree += Vec_PtrSize(p->vClassNew);
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vClassNew, pObj, i )
    {
        ppClassNew[i] = pObj;
        Aig_ObjSetRepr( p->pAig, pObj, i? pReprNew : NULL );
    }
    Ssw_ObjAddClass( p, pReprNew, ppClassNew, Vec_PtrSize(p->vClassNew) );
    // refine them recursively
    if ( fRecursive )
        return 1 + Ssw_ClassesRefineOneClass( p, pReprNew, 1 );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Refine the group of constant 1 nodes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ClassesRefineConst1( Ssw_Cla_t * p, int fRecursive )
{
    Aig_Obj_t * pObj, * pReprNew, ** ppClassNew;
    int i;
    // collect the nodes to be refined
    Vec_PtrClear( p->vClassNew );
    for ( i = 0; i < Vec_PtrSize(p->pAig->vObjs); i++ )
        if ( p->pAig->pReprs[i] == Aig_ManConst1(p->pAig) )
        {
            pObj = Aig_ManObj( p->pAig, i );
            if ( !p->pFuncNodeIsConst( p->pManData, pObj ) )
            {
                Vec_PtrPush( p->vClassNew, pObj );
//                Vec_PtrPush( p->vRefined, pObj );
            }
        }
    // check if there is a new class
    if ( Vec_PtrSize(p->vClassNew) == 0 )
        return 0;
    if ( p->fConstCorr )
    {
        Vec_PtrForEachEntry( Aig_Obj_t *, p->vClassNew, pObj, i )
            Aig_ObjSetRepr( p->pAig, pObj, NULL );
        return 1;
    }
    p->nCands1 -= Vec_PtrSize(p->vClassNew);
    pReprNew = (Aig_Obj_t *)Vec_PtrEntry( p->vClassNew, 0 );
    Aig_ObjSetRepr( p->pAig, pReprNew, NULL );
    if ( Vec_PtrSize(p->vClassNew) == 1 )
        return 1;
    // create a new class composed of these nodes
    ppClassNew = p->pMemClassesFree;
    p->pMemClassesFree += Vec_PtrSize(p->vClassNew);
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vClassNew, pObj, i )
    {
        ppClassNew[i] = pObj;
        Aig_ObjSetRepr( p->pAig, pObj, i? pReprNew : NULL );
    }
    Ssw_ObjAddClass( p, pReprNew, ppClassNew, Vec_PtrSize(p->vClassNew) );
    // refine them recursively
    if ( fRecursive )
        return 1 + Ssw_ClassesRefineOneClass( p, pReprNew, 1 );
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
