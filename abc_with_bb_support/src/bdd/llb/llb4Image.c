/**CFile****************************************************************

  FileName    [llb3Image.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Computes image using partitioned structure.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb3Image.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Llb_Var_t_ Llb_Var_t;
struct Llb_Var_t_ 
{
    int           iVar;      // variable number
    int           nScore;    // variable score
    Vec_Int_t *   vParts;    // partitions
};

typedef struct Llb_Prt_t_ Llb_Prt_t;
struct Llb_Prt_t_ 
{
    int           iPart;     // partition number
    int           nSize;     // the number of BDD nodes
    DdNode *      bFunc;     // the partition
    Vec_Int_t *   vVars;     // support
};

typedef struct Llb_Mgr_t_ Llb_Mgr_t;
struct Llb_Mgr_t_
{
    DdManager *   dd;        // working BDD manager
    Vec_Int_t *   vVars2Q;   // variables to quantify
    int           nSizeMax;  // maximum size of the cluster
    // internal
    Llb_Prt_t **  pParts;    // partitions
    Llb_Var_t **  pVars;     // variables
    int           iPartFree; // next free partition
    int           nVars;     // the number of BDD variables
    int           nSuppMax;  // maximum support size
    // temporary
    int *         pSupp;     // temporary support storage
};

static inline Llb_Var_t * Llb_MgrVar( Llb_Mgr_t * p, int i )   { return p->pVars[i];  }
static inline Llb_Prt_t * Llb_MgrPart( Llb_Mgr_t * p, int i )  { return p->pParts[i]; }

// iterator over vars
#define Llb_MgrForEachVar( p, pVar, i )     \
    for ( i = 0; (i < p->nVars) && (((pVar) = Llb_MgrVar(p, i)), 1); i++ ) if ( pVar == NULL ) {} else
// iterator over parts
#define Llb_MgrForEachPart( p, pPart, i )   \
    for ( i = 0; (i < p->iPartFree) && (((pPart) = Llb_MgrPart(p, i)), 1); i++ ) if ( pPart == NULL ) {} else

// iterator over vars of one partition
#define Llb_PartForEachVar( p, pPart, pVar, i )   \
    for ( i = 0; (i < Vec_IntSize(pPart->vVars)) && (((pVar) = Llb_MgrVar(p, Vec_IntEntry(pPart->vVars,i))), 1); i++ )
// iterator over parts of one variable
#define Llb_VarForEachPart( p, pVar, pPart, i )   \
    for ( i = 0; (i < Vec_IntSize(pVar->vParts)) && (((pPart) = Llb_MgrPart(p, Vec_IntEntry(pVar->vParts,i))), 1); i++ )

// statistics
//abctime timeBuild, timeAndEx, timeOther;
//int nSuppMax;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Removes one variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4RemoveVar( Llb_Mgr_t * p, Llb_Var_t * pVar )
{
    assert( p->pVars[pVar->iVar] == pVar );
    p->pVars[pVar->iVar] = NULL;
    Vec_IntFree( pVar->vParts );
    ABC_FREE( pVar );
}

/**Function*************************************************************

  Synopsis    [Removes one partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4RemovePart( Llb_Mgr_t * p, Llb_Prt_t * pPart )
{
//printf( "Removing %d\n", pPart->iPart );
    assert( p->pParts[pPart->iPart] == pPart );
    p->pParts[pPart->iPart] = NULL;
    Vec_IntFree( pPart->vVars );
    Cudd_RecursiveDeref( p->dd, pPart->bFunc );
    ABC_FREE( pPart );
}

/**Function*************************************************************

  Synopsis    [Create cube with singleton variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_Nonlin4CreateCube1( Llb_Mgr_t * p, Llb_Prt_t * pPart )
{
    DdNode * bCube, * bTemp;
    Llb_Var_t * pVar;
    int i;
    abctime TimeStop;
    TimeStop = p->dd->TimeStop; p->dd->TimeStop = 0;
    bCube = Cudd_ReadOne(p->dd);   Cudd_Ref( bCube );
    Llb_PartForEachVar( p, pPart, pVar, i )
    {
        assert( Vec_IntSize(pVar->vParts) > 0 );
        if ( Vec_IntSize(pVar->vParts) != 1 )
            continue;
        assert( Vec_IntEntry(pVar->vParts, 0) == pPart->iPart );
        bCube = Cudd_bddAnd( p->dd, bTemp = bCube, Cudd_bddIthVar(p->dd, pVar->iVar) );    Cudd_Ref( bCube );
        Cudd_RecursiveDeref( p->dd, bTemp );
    }
    Cudd_Deref( bCube );
    p->dd->TimeStop = TimeStop;
    return bCube;
}

/**Function*************************************************************

  Synopsis    [Create cube of variables appearing only in two partitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_Nonlin4CreateCube2( Llb_Mgr_t * p, Llb_Prt_t * pPart1, Llb_Prt_t * pPart2 )
{
    DdNode * bCube, * bTemp;
    Llb_Var_t * pVar;
    int i;
    abctime TimeStop;
    TimeStop = p->dd->TimeStop; p->dd->TimeStop = 0;
    bCube = Cudd_ReadOne(p->dd);   Cudd_Ref( bCube );
    Llb_PartForEachVar( p, pPart1, pVar, i )
    {
        assert( Vec_IntSize(pVar->vParts) > 0 );
        if ( Vec_IntSize(pVar->vParts) != 2 )
            continue;
        if ( (Vec_IntEntry(pVar->vParts, 0) == pPart1->iPart && Vec_IntEntry(pVar->vParts, 1) == pPart2->iPart) ||
             (Vec_IntEntry(pVar->vParts, 0) == pPart2->iPart && Vec_IntEntry(pVar->vParts, 1) == pPart1->iPart) )
        {
            bCube = Cudd_bddAnd( p->dd, bTemp = bCube, Cudd_bddIthVar(p->dd, pVar->iVar) );   Cudd_Ref( bCube );
            Cudd_RecursiveDeref( p->dd, bTemp );
        }
    }
    Cudd_Deref( bCube );
    p->dd->TimeStop = TimeStop;
    return bCube;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if partition has singleton variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_Nonlin4HasSingletonVars( Llb_Mgr_t * p, Llb_Prt_t * pPart )
{
    Llb_Var_t * pVar;
    int i;
    Llb_PartForEachVar( p, pPart, pVar, i )
        if ( Vec_IntSize(pVar->vParts) == 1 )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if partition has singleton variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4Print( Llb_Mgr_t * p )
{
    Llb_Prt_t * pPart;
    Llb_Var_t * pVar;
    int i, k;
    printf( "\n" );
    Llb_MgrForEachVar( p, pVar, i )
    {
        printf( "Var %3d : ", i );
        Llb_VarForEachPart( p, pVar, pPart, k )
            printf( "%d ", pPart->iPart );
        printf( "\n" );
    }
    Llb_MgrForEachPart( p, pPart, i )
    {
        printf( "Part %3d : ", i );
        Llb_PartForEachVar( p, pPart, pVar, k )
            printf( "%d ", pVar->iVar );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Quantifies singles belonging to one partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_Nonlin4Quantify1( Llb_Mgr_t * p, Llb_Prt_t * pPart )
{
    Llb_Var_t * pVar;
    Llb_Prt_t * pTemp;
    Vec_Ptr_t * vSingles;
    DdNode * bCube, * bTemp;
    int i, RetValue, nSizeNew;
    // create cube to be quantified
    bCube = Llb_Nonlin4CreateCube1( p, pPart );   Cudd_Ref( bCube );
//    assert( !Cudd_IsConstant(bCube) );
    // derive new function
    pPart->bFunc = Cudd_bddExistAbstract( p->dd, bTemp = pPart->bFunc, bCube );  Cudd_Ref( pPart->bFunc );
    Cudd_RecursiveDeref( p->dd, bTemp );
    Cudd_RecursiveDeref( p->dd, bCube );
    // get support
    vSingles = Vec_PtrAlloc( 0 );
    nSizeNew = Cudd_DagSize(pPart->bFunc);
    Extra_SupportArray( p->dd, pPart->bFunc, p->pSupp );
    Llb_PartForEachVar( p, pPart, pVar, i )
        if ( p->pSupp[pVar->iVar] )
        {
            assert( Vec_IntSize(pVar->vParts) > 1 );
            pVar->nScore -= pPart->nSize - nSizeNew;
        }
        else
        {
            RetValue = Vec_IntRemove( pVar->vParts, pPart->iPart );
            assert( RetValue );
            pVar->nScore -= pPart->nSize;
            if ( Vec_IntSize(pVar->vParts) == 0 )
                Llb_Nonlin4RemoveVar( p, pVar );
            else if ( Vec_IntSize(pVar->vParts) == 1 )
                Vec_PtrPushUnique( vSingles, Llb_MgrPart(p, Vec_IntEntry(pVar->vParts,0)) );
        }

    // update partition
    pPart->nSize = nSizeNew;
    Vec_IntClear( pPart->vVars );
    for ( i = 0; i < p->nVars; i++ )
        if ( p->pSupp[i] && Vec_IntEntry(p->vVars2Q, i) )
            Vec_IntPush( pPart->vVars, i );
    // remove other variables
    Vec_PtrForEachEntry( Llb_Prt_t *, vSingles, pTemp, i )
        Llb_Nonlin4Quantify1( p, pTemp );
    Vec_PtrFree( vSingles );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Quantifies singles belonging to one partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_Nonlin4Quantify2( Llb_Mgr_t * p, Llb_Prt_t * pPart1, Llb_Prt_t * pPart2 )
{
    int fVerbose = 0;
    Llb_Var_t * pVar;
    Llb_Prt_t * pTemp;
    Vec_Ptr_t * vSingles;
    DdNode * bCube, * bFunc;
    int i, RetValue, nSuppSize;
//    int iPart1 = pPart1->iPart;
//    int iPart2 = pPart2->iPart;
    int liveBeg, liveEnd;

    // create cube to be quantified
    bCube = Llb_Nonlin4CreateCube2( p, pPart1, pPart2 );   Cudd_Ref( bCube );

//printf( "Quantifying  " ); Extra_bddPrintSupport( p->dd, bCube );  printf( "\n" );

if ( fVerbose )
{
printf( "\n" );
printf( "\n" );
Llb_Nonlin4Print( p );
printf( "Conjoining partitions %d and %d.\n", pPart1->iPart, pPart2->iPart );
Extra_bddPrintSupport( p->dd, bCube );  printf( "\n" );
}
liveBeg = p->dd->keys - p->dd->dead;
    bFunc = Cudd_bddAndAbstract( p->dd, pPart1->bFunc, pPart2->bFunc, bCube );  
liveEnd = p->dd->keys - p->dd->dead;
//printf( "%d ", liveEnd-liveBeg );

    if ( bFunc == NULL )
    {
        Cudd_RecursiveDeref( p->dd, bCube );
        return 0;
    }
    Cudd_Ref( bFunc );
    Cudd_RecursiveDeref( p->dd, bCube );

//printf( "Creating part %d ", p->iPartFree ); Extra_bddPrintSupport( p->dd, bFunc );  printf( "\n" );

//printf( "Creating %d\n", p->iPartFree );

    // create new partition
    pTemp = p->pParts[p->iPartFree] = ABC_CALLOC( Llb_Prt_t, 1 );
    pTemp->iPart = p->iPartFree++;
    pTemp->nSize = Cudd_DagSize(bFunc);
    pTemp->bFunc = bFunc;
    pTemp->vVars = Vec_IntAlloc( 8 );
    // update variables
    Llb_PartForEachVar( p, pPart1, pVar, i )
    {
        RetValue = Vec_IntRemove( pVar->vParts, pPart1->iPart );
        assert( RetValue );
        pVar->nScore -= pPart1->nSize;
    }
    // update variables
    Llb_PartForEachVar( p, pPart2, pVar, i )
    {
        RetValue = Vec_IntRemove( pVar->vParts, pPart2->iPart );
        assert( RetValue );
        pVar->nScore -= pPart2->nSize;
    }
    // add variables to the new partition
    nSuppSize = 0;
    Extra_SupportArray( p->dd, bFunc, p->pSupp );
    for ( i = 0; i < p->nVars; i++ )
    {
        nSuppSize += p->pSupp[i];
        if ( p->pSupp[i] && Vec_IntEntry(p->vVars2Q, i) )
        {
            pVar = Llb_MgrVar( p, i );
            pVar->nScore += pTemp->nSize;
            Vec_IntPush( pVar->vParts, pTemp->iPart );
            Vec_IntPush( pTemp->vVars, i );
        }
    }
    p->nSuppMax = Abc_MaxInt( p->nSuppMax, nSuppSize ); 
    // remove variables and collect partitions with singleton variables
    vSingles = Vec_PtrAlloc( 0 );
    Llb_PartForEachVar( p, pPart1, pVar, i )
    {
        if ( Vec_IntSize(pVar->vParts) == 0 )
            Llb_Nonlin4RemoveVar( p, pVar );
        else if ( Vec_IntSize(pVar->vParts) == 1 )
        {
            if ( fVerbose )
                printf( "Adding partition %d because of var %d.\n", 
                    Llb_MgrPart(p, Vec_IntEntry(pVar->vParts,0))->iPart, pVar->iVar );
            Vec_PtrPushUnique( vSingles, Llb_MgrPart(p, Vec_IntEntry(pVar->vParts,0)) );
        }
    }
    Llb_PartForEachVar( p, pPart2, pVar, i )
    {
        if ( pVar == NULL )
            continue;
        if ( Vec_IntSize(pVar->vParts) == 0 )
            Llb_Nonlin4RemoveVar( p, pVar );
        else if ( Vec_IntSize(pVar->vParts) == 1 )
        {
            if ( fVerbose )
                printf( "Adding partition %d because of var %d.\n", 
                    Llb_MgrPart(p, Vec_IntEntry(pVar->vParts,0))->iPart, pVar->iVar );
            Vec_PtrPushUnique( vSingles, Llb_MgrPart(p, Vec_IntEntry(pVar->vParts,0)) );
        }
    }
    // remove partitions
    Llb_Nonlin4RemovePart( p, pPart1 );
    Llb_Nonlin4RemovePart( p, pPart2 );
    // remove other variables
if ( fVerbose )
Llb_Nonlin4Print( p );
    Vec_PtrForEachEntry( Llb_Prt_t *, vSingles, pTemp, i )
    {
if ( fVerbose )
printf( "Updating partitiong %d with singlton vars.\n", pTemp->iPart );
        Llb_Nonlin4Quantify1( p, pTemp );
    }
if ( fVerbose )
Llb_Nonlin4Print( p );
    Vec_PtrFree( vSingles );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes volume of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4CutNodes_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Saig_ObjIsLi(p, pObj) )
    {
        Llb_Nonlin4CutNodes_rec(p, Aig_ObjFanin0(pObj), vNodes);
        return;
    }
    if ( Aig_ObjIsConst1(pObj) )
        return;
    assert( Aig_ObjIsNode(pObj) );
    Llb_Nonlin4CutNodes_rec(p, Aig_ObjFanin0(pObj), vNodes);
    Llb_Nonlin4CutNodes_rec(p, Aig_ObjFanin1(pObj), vNodes);
    Vec_PtrPush( vNodes, pObj );
}

/**Function*************************************************************

  Synopsis    [Computes volume of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_Nonlin4CutNodes( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    // mark the lower cut with the traversal ID
    Aig_ManIncrementTravId(p);
    Vec_PtrForEachEntry( Aig_Obj_t *, vLower, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    // count the upper cut
    vNodes = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vUpper, pObj, i )
        Llb_Nonlin4CutNodes_rec( p, pObj, vNodes );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Starts non-linear quantification scheduling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4AddPair( Llb_Mgr_t * p, int iPart, int iVar )
{
    if ( p->pVars[iVar] == NULL )
    {
        p->pVars[iVar] = ABC_CALLOC( Llb_Var_t, 1 );
        p->pVars[iVar]->iVar   = iVar;
        p->pVars[iVar]->nScore = 0;
        p->pVars[iVar]->vParts = Vec_IntAlloc( 8 );
    }
    Vec_IntPush( p->pVars[iVar]->vParts, iPart );
    Vec_IntPush( p->pParts[iPart]->vVars, iVar );
}

/**Function*************************************************************

  Synopsis    [Starts non-linear quantification scheduling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4AddPartition( Llb_Mgr_t * p, int i, DdNode * bFunc )
{
    int k, nSuppSize;
    assert( !Cudd_IsConstant(bFunc) );
//printf( "Creating init %d\n", i );
    // create partition
    p->pParts[i] = ABC_CALLOC( Llb_Prt_t, 1 );
    p->pParts[i]->iPart = i;
    p->pParts[i]->bFunc = bFunc;  Cudd_Ref( bFunc );
    p->pParts[i]->vVars = Vec_IntAlloc( 8 );
    // add support dependencies
    nSuppSize = 0;
    Extra_SupportArray( p->dd, bFunc, p->pSupp );
    for ( k = 0; k < p->nVars; k++ )
    {
        nSuppSize += p->pSupp[k];
        if ( p->pSupp[k] && Vec_IntEntry(p->vVars2Q, k) )
            Llb_Nonlin4AddPair( p, i, k );
    }
    p->nSuppMax = Abc_MaxInt( p->nSuppMax, nSuppSize ); 
}

/**Function*************************************************************

  Synopsis    [Checks that each var appears in at least one partition.]

  Description []
               
  SideEffects []

  SeeAlso     []
**********************************************************************/
void Llb_Nonlin4CheckVars( Llb_Mgr_t * p )
{
    Llb_Var_t * pVar;
    int i;
    Llb_MgrForEachVar( p, pVar, i )
        assert( Vec_IntSize(pVar->vParts) > 1 );
}

/**Function*************************************************************

  Synopsis    [Find next partition to quantify]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_Nonlin4NextPartitions( Llb_Mgr_t * p, Llb_Prt_t ** ppPart1, Llb_Prt_t ** ppPart2 )
{
    Llb_Var_t * pVar, * pVarBest = NULL;
    Llb_Prt_t * pPart, * pPart1Best = NULL, * pPart2Best = NULL;
    int i;
    Llb_Nonlin4CheckVars( p );
    // find variable with minimum score
    Llb_MgrForEachVar( p, pVar, i )
    {
        if ( p->nSizeMax && pVar->nScore > p->nSizeMax )
            continue;
//        if ( pVarBest == NULL || Vec_IntSize(pVarBest->vParts) * pVarBest->nScore > Vec_IntSize(pVar->vParts) * pVar->nScore )
        if ( pVarBest == NULL || pVarBest->nScore > pVar->nScore )
            pVarBest = pVar;
//        printf( "%d ", pVar->nScore );
    }
//printf( "\n" );
    if ( pVarBest == NULL )
        return 0;
    // find two partitions with minimum size
    Llb_VarForEachPart( p, pVarBest, pPart, i )
    {
        if ( pPart1Best == NULL )
            pPart1Best = pPart;
        else if ( pPart2Best == NULL )
            pPart2Best = pPart;
        else if ( pPart1Best->nSize > pPart->nSize || pPart2Best->nSize > pPart->nSize )
        {
            if ( pPart1Best->nSize > pPart2Best->nSize )
                pPart1Best = pPart;
            else
                pPart2Best = pPart;
        }
    }
//printf( "Selecting %d and parts %d and %d\n", pVarBest->iVar, pPart1Best->nSize, pPart2Best->nSize );
//Extra_bddPrintSupport( p->dd, pPart1Best->bFunc ); printf( "\n" );
//Extra_bddPrintSupport( p->dd, pPart2Best->bFunc ); printf( "\n" );

    *ppPart1 = pPart1Best;
    *ppPart2 = pPart2Best;
    return 1; 
}

/**Function*************************************************************

  Synopsis    [Recomputes scores after variable reordering.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4RecomputeScores( Llb_Mgr_t * p )
{
    Llb_Prt_t * pPart;
    Llb_Var_t * pVar;
    int i, k;
    Llb_MgrForEachPart( p, pPart, i )
        pPart->nSize = Cudd_DagSize(pPart->bFunc);
    Llb_MgrForEachVar( p, pVar, i )
    {
        pVar->nScore = 0;
        Llb_VarForEachPart( p, pVar, pPart, k )
            pVar->nScore += pPart->nSize;
    }
}

/**Function*************************************************************

  Synopsis    [Recomputes scores after variable reordering.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4VerifyScores( Llb_Mgr_t * p )
{
    Llb_Prt_t * pPart;
    Llb_Var_t * pVar;
    int i, k, nScore;
    Llb_MgrForEachPart( p, pPart, i )
        assert( pPart->nSize == Cudd_DagSize(pPart->bFunc) );
    Llb_MgrForEachVar( p, pVar, i )
    {
        nScore = 0;
        Llb_VarForEachPart( p, pVar, pPart, k )
            nScore += pPart->nSize;
        assert( nScore == pVar->nScore );
    }
}

/**Function*************************************************************

  Synopsis    [Starts non-linear quantification scheduling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Llb_Mgr_t * Llb_Nonlin4Alloc( DdManager * dd, Vec_Ptr_t * vParts, DdNode * bCurrent, Vec_Int_t * vVars2Q, int nSizeMax )
{
    Llb_Mgr_t * p;
    DdNode * bFunc;
    int i;
    p = ABC_CALLOC( Llb_Mgr_t, 1 );
    p->dd        = dd;
    p->nSizeMax  = nSizeMax;
    p->vVars2Q   = vVars2Q;
    p->nVars     = Cudd_ReadSize(dd);
    p->iPartFree = Vec_PtrSize(vParts);
    p->pVars     = ABC_CALLOC( Llb_Var_t *, p->nVars );
    p->pParts    = ABC_CALLOC( Llb_Prt_t *, 2 * p->iPartFree + 2 );
    p->pSupp     = ABC_ALLOC( int, Cudd_ReadSize(dd) );
    // add pairs (refs are consumed inside)
    Vec_PtrForEachEntry( DdNode *, vParts, bFunc, i )
        Llb_Nonlin4AddPartition( p, i, bFunc );
    // add partition
    if ( bCurrent )
        Llb_Nonlin4AddPartition( p, p->iPartFree++, bCurrent );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops non-linear quantification scheduling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4Free( Llb_Mgr_t * p )
{
    Llb_Prt_t * pPart;
    Llb_Var_t * pVar;
    int i;
    Llb_MgrForEachVar( p, pVar, i )
        Llb_Nonlin4RemoveVar( p, pVar );
    Llb_MgrForEachPart( p, pPart, i )
        Llb_Nonlin4RemovePart( p, pPart );
    ABC_FREE( p->pVars );
    ABC_FREE( p->pParts );
    ABC_FREE( p->pSupp );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_Nonlin4Image( DdManager * dd, Vec_Ptr_t * vParts, DdNode * bCurrent, Vec_Int_t * vVars2Q )
{
    Llb_Prt_t * pPart, * pPart1, * pPart2;
    Llb_Mgr_t * p;
    DdNode * bFunc, * bTemp;
    int i, nReorders;
    // start the manager
    p = Llb_Nonlin4Alloc( dd, vParts, bCurrent, vVars2Q, 0 );
    // remove singles
    Llb_MgrForEachPart( p, pPart, i )
        if ( Llb_Nonlin4HasSingletonVars(p, pPart) )
            Llb_Nonlin4Quantify1( p, pPart );
    // compute scores
    Llb_Nonlin4RecomputeScores( p );
    // iteratively quantify variables
    while ( Llb_Nonlin4NextPartitions(p, &pPart1, &pPart2) )
    {
        nReorders = Cudd_ReadReorderings(dd);
        if ( !Llb_Nonlin4Quantify2( p, pPart1, pPart2 ) )
        {
            Llb_Nonlin4Free( p );
            return NULL;
        }
        if ( nReorders < Cudd_ReadReorderings(dd) )
            Llb_Nonlin4RecomputeScores( p );
//        else
//            Llb_Nonlin4VerifyScores( p );
    }
    // load partitions
    bFunc = Cudd_ReadOne(p->dd);   Cudd_Ref( bFunc );
    Llb_MgrForEachPart( p, pPart, i )
    {
        bFunc = Cudd_bddAnd( p->dd, bTemp = bFunc, pPart->bFunc );   Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( p->dd, bTemp );
    }
//    nSuppMax = p->nSuppMax;
    Llb_Nonlin4Free( p );
//printf( "\n" );
    // return
    Cudd_Deref( bFunc );
    return bFunc;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_Nonlin4Group( DdManager * dd, Vec_Ptr_t * vParts, Vec_Int_t * vVars2Q, int nSizeMax )
{
    Vec_Ptr_t * vGroups;
    Llb_Prt_t * pPart, * pPart1, * pPart2;
    Llb_Mgr_t * p;
    int i, nReorders;//, clk = Abc_Clock();
    // start the manager
    p = Llb_Nonlin4Alloc( dd, vParts, NULL, vVars2Q, nSizeMax );
    // remove singles
    Llb_MgrForEachPart( p, pPart, i )
        if ( Llb_Nonlin4HasSingletonVars(p, pPart) )
            Llb_Nonlin4Quantify1( p, pPart );
    // compute scores
    Llb_Nonlin4RecomputeScores( p );
    // iteratively quantify variables
    while ( Llb_Nonlin4NextPartitions(p, &pPart1, &pPart2) )
    {
        nReorders = Cudd_ReadReorderings(dd);
        if ( !Llb_Nonlin4Quantify2( p, pPart1, pPart2 ) )
        {
            Llb_Nonlin4Free( p );
            return NULL;
        }
        if ( nReorders < Cudd_ReadReorderings(dd) )
            Llb_Nonlin4RecomputeScores( p );
//        else
//            Llb_Nonlin4VerifyScores( p );
    }
    // load partitions
    vGroups = Vec_PtrAlloc( 1000 );
    Llb_MgrForEachPart( p, pPart, i )
    {
//printf( "Iteration %d ", pPart->iPart );
        if ( Cudd_IsConstant(pPart->bFunc) )
        {
//printf( "Constant\n" );
            assert( !Cudd_IsComplement(pPart->bFunc) );
            continue;
        }
//printf( "\n" );
        Vec_PtrPush( vGroups, pPart->bFunc );
        Cudd_Ref( pPart->bFunc );
//printf( "Part %d  ", pPart->iPart );
//Extra_bddPrintSupport( p->dd, pPart->bFunc ); printf( "\n" );
    }
    Llb_Nonlin4Free( p );
//Abc_PrintTime( 1, "Reparametrization time", Abc_Clock() - clk );
    return vGroups;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

