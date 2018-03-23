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
    Aig_Man_t *   pAig;      // AIG manager
    Vec_Ptr_t *   vLeaves;   // leaves in the AIG manager
    Vec_Ptr_t *   vRoots;    // roots in the AIG manager
    DdManager *   dd;        // working BDD manager
    int *         pVars2Q;   // variables to quantify
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
abctime timeBuild, timeAndEx, timeOther;
int nSuppMax;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Removes one variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_NonlinRemoveVar( Llb_Mgr_t * p, Llb_Var_t * pVar )
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
void Llb_NonlinRemovePart( Llb_Mgr_t * p, Llb_Prt_t * pPart )
{
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
DdNode * Llb_NonlinCreateCube1( Llb_Mgr_t * p, Llb_Prt_t * pPart )
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
DdNode * Llb_NonlinCreateCube2( Llb_Mgr_t * p, Llb_Prt_t * pPart1, Llb_Prt_t * pPart2 )
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
int Llb_NonlinHasSingletonVars( Llb_Mgr_t * p, Llb_Prt_t * pPart )
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
void Llb_NonlinPrint( Llb_Mgr_t * p )
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
int Llb_NonlinQuantify1( Llb_Mgr_t * p, Llb_Prt_t * pPart, int fSubset )
{
    Llb_Var_t * pVar;
    Llb_Prt_t * pTemp;
    Vec_Ptr_t * vSingles;
    DdNode * bCube, * bTemp;
    int i, RetValue, nSizeNew;
    if ( fSubset )
    {        
        int Length;
//        int nSuppSize = Cudd_SupportSize( p->dd, pPart->bFunc );
//        pPart->bFunc = Cudd_SubsetHeavyBranch( p->dd, bTemp = pPart->bFunc, nSuppSize, 3*pPart->nSize/4 );  Cudd_Ref( pPart->bFunc );
        pPart->bFunc = Cudd_LargestCube( p->dd, bTemp = pPart->bFunc, &Length );  Cudd_Ref( pPart->bFunc );

        printf( "Subsetting %3d : ", pPart->iPart );
        printf( "(Supp =%3d  Node =%5d) -> ", Cudd_SupportSize(p->dd, bTemp),        Cudd_DagSize(bTemp) );
        printf( "(Supp =%3d  Node =%5d)\n",   Cudd_SupportSize(p->dd, pPart->bFunc), Cudd_DagSize(pPart->bFunc) );

        RetValue = (Cudd_DagSize(bTemp) == Cudd_DagSize(pPart->bFunc));

        Cudd_RecursiveDeref( p->dd, bTemp );

        if ( RetValue )
            return 1;
    }
    else
    {
        // create cube to be quantified
        bCube = Llb_NonlinCreateCube1( p, pPart );   Cudd_Ref( bCube );
//        assert( !Cudd_IsConstant(bCube) );
        // derive new function
        pPart->bFunc = Cudd_bddExistAbstract( p->dd, bTemp = pPart->bFunc, bCube );  Cudd_Ref( pPart->bFunc );
        Cudd_RecursiveDeref( p->dd, bTemp );
        Cudd_RecursiveDeref( p->dd, bCube );
    }
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
                Llb_NonlinRemoveVar( p, pVar );
            else if ( Vec_IntSize(pVar->vParts) == 1 )
                Vec_PtrPushUnique( vSingles, Llb_MgrPart(p, Vec_IntEntry(pVar->vParts,0)) );
        }

    // update partition
    pPart->nSize = nSizeNew;
    Vec_IntClear( pPart->vVars );
    for ( i = 0; i < p->nVars; i++ )
        if ( p->pSupp[i] && p->pVars2Q[i] )
            Vec_IntPush( pPart->vVars, i );
    // remove other variables
    Vec_PtrForEachEntry( Llb_Prt_t *, vSingles, pTemp, i )
        Llb_NonlinQuantify1( p, pTemp, 0 );
    Vec_PtrFree( vSingles );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Quantifies singles belonging to one partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_NonlinQuantify2( Llb_Mgr_t * p, Llb_Prt_t * pPart1, Llb_Prt_t * pPart2 )
{
    int fVerbose = 0;
    Llb_Var_t * pVar;
    Llb_Prt_t * pTemp;
    Vec_Ptr_t * vSingles;
    DdNode * bCube, * bFunc;
    int i, RetValue, nSuppSize;
//    int iPart1 = pPart1->iPart;
//    int iPart2 = pPart2->iPart;

    // create cube to be quantified
    bCube = Llb_NonlinCreateCube2( p, pPart1, pPart2 );   Cudd_Ref( bCube );
if ( fVerbose )
{
printf( "\n" );
printf( "\n" );
Llb_NonlinPrint( p );
printf( "Conjoining partitions %d and %d.\n", pPart1->iPart, pPart2->iPart );
Extra_bddPrintSupport( p->dd, bCube );  printf( "\n" );
}
 
    // derive new function
//    bFunc = Cudd_bddAndAbstract( p->dd, pPart1->bFunc, pPart2->bFunc, bCube );  Cudd_Ref( bFunc );
/*
    bFunc = Cudd_bddAndAbstractLimit( p->dd, pPart1->bFunc, pPart2->bFunc, bCube, Limit );  
    if ( bFunc == NULL )
    {
        int RetValue;
        Cudd_RecursiveDeref( p->dd, bCube );
        if ( pPart1->nSize < pPart2->nSize )
            RetValue = Llb_NonlinQuantify1( p, pPart1, 1 );
        else
            RetValue = Llb_NonlinQuantify1( p, pPart2, 1 );
        if ( RetValue )
            Limit = Limit + 1000;
        Llb_NonlinQuantify2( p, pPart1, pPart2 );
        return 0;
    }
    Cudd_Ref( bFunc );
*/

//    bFunc = Extra_bddAndAbstractTime( p->dd, pPart1->bFunc, pPart2->bFunc, bCube, TimeOut );  
    bFunc = Cudd_bddAndAbstract( p->dd, pPart1->bFunc, pPart2->bFunc, bCube );  
    if ( bFunc == NULL )
    {
        Cudd_RecursiveDeref( p->dd, bCube );
        return 0;
    }
    Cudd_Ref( bFunc );
    Cudd_RecursiveDeref( p->dd, bCube );

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
        if ( p->pSupp[i] && p->pVars2Q[i] )
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
            Llb_NonlinRemoveVar( p, pVar );
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
            Llb_NonlinRemoveVar( p, pVar );
        else if ( Vec_IntSize(pVar->vParts) == 1 )
        {
            if ( fVerbose )
                printf( "Adding partition %d because of var %d.\n", 
                    Llb_MgrPart(p, Vec_IntEntry(pVar->vParts,0))->iPart, pVar->iVar );
            Vec_PtrPushUnique( vSingles, Llb_MgrPart(p, Vec_IntEntry(pVar->vParts,0)) );
        }
    }
    // remove partitions
    Llb_NonlinRemovePart( p, pPart1 );
    Llb_NonlinRemovePart( p, pPart2 );
    // remove other variables
if ( fVerbose )
Llb_NonlinPrint( p );
    Vec_PtrForEachEntry( Llb_Prt_t *, vSingles, pTemp, i )
    {
if ( fVerbose )
printf( "Updating partitiong %d with singlton vars.\n", pTemp->iPart );
        Llb_NonlinQuantify1( p, pTemp, 0 );
    }
if ( fVerbose )
Llb_NonlinPrint( p );
    Vec_PtrFree( vSingles );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes volume of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_NonlinCutNodes_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Saig_ObjIsLi(p, pObj) )
    {
        Llb_NonlinCutNodes_rec(p, Aig_ObjFanin0(pObj), vNodes);
        return;
    }
    if ( Aig_ObjIsConst1(pObj) )
        return;
    assert( Aig_ObjIsNode(pObj) );
    Llb_NonlinCutNodes_rec(p, Aig_ObjFanin0(pObj), vNodes);
    Llb_NonlinCutNodes_rec(p, Aig_ObjFanin1(pObj), vNodes);
    Vec_PtrPush( vNodes, pObj );
}

/**Function*************************************************************

  Synopsis    [Computes volume of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_NonlinCutNodes( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper )
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
        Llb_NonlinCutNodes_rec( p, pObj, vNodes );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Returns array of BDDs for the roots in terms of the leaves.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_NonlinBuildBdds( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper, DdManager * dd )
{
    Vec_Ptr_t * vNodes, * vResult;
    Aig_Obj_t * pObj;
    DdNode * bBdd0, * bBdd1, * bProd;
    int i, k;

    Aig_ManConst1(p)->pData = Cudd_ReadOne( dd );
    Vec_PtrForEachEntry( Aig_Obj_t *, vLower, pObj, i )
        pObj->pData = Cudd_bddIthVar( dd, Aig_ObjId(pObj) );

    vNodes = Llb_NonlinCutNodes( p, vLower, vUpper );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        bBdd0 = Cudd_NotCond( (DdNode *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
        bBdd1 = Cudd_NotCond( (DdNode *)Aig_ObjFanin1(pObj)->pData, Aig_ObjFaninC1(pObj) );
//        pObj->pData = Extra_bddAndTime( dd, bBdd0, bBdd1, TimeOut );
        pObj->pData = Cudd_bddAnd( dd, bBdd0, bBdd1 );
        if ( pObj->pData == NULL )
        {
            Vec_PtrForEachEntryStop( Aig_Obj_t *, vNodes, pObj, k, i )
                if ( pObj->pData )
                    Cudd_RecursiveDeref( dd, (DdNode *)pObj->pData );
            Vec_PtrFree( vNodes );
            return NULL;
        }
        Cudd_Ref( (DdNode *)pObj->pData );
    }

    vResult = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vUpper, pObj, i )
    {
        if ( Aig_ObjIsNode(pObj) )
        {
            bProd = Cudd_bddXnor( dd, Cudd_bddIthVar(dd, Aig_ObjId(pObj)), (DdNode *)pObj->pData );  Cudd_Ref( bProd );
        }
        else
        {
            assert( Saig_ObjIsLi(p, pObj) );
            bBdd0 = Cudd_NotCond( (DdNode *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
            bProd = Cudd_bddXnor( dd, Cudd_bddIthVar(dd, Aig_ObjId(pObj)), bBdd0 );                  Cudd_Ref( bProd );
        }
        Vec_PtrPush( vResult, bProd );
    }
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        Cudd_RecursiveDeref( dd, (DdNode *)pObj->pData );

    Vec_PtrFree( vNodes );
    return vResult;
}

/**Function*************************************************************

  Synopsis    [Starts non-linear quantification scheduling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_NonlinAddPair( Llb_Mgr_t * p, DdNode * bFunc, int iPart, int iVar )
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
void Llb_NonlinAddPartition( Llb_Mgr_t * p, int i, DdNode * bFunc )
{
    int k, nSuppSize;
    assert( !Cudd_IsConstant(bFunc) );
    // create partition
    p->pParts[i] = ABC_CALLOC( Llb_Prt_t, 1 );
    p->pParts[i]->iPart = i;
    p->pParts[i]->bFunc = bFunc;
    p->pParts[i]->vVars = Vec_IntAlloc( 8 );
    // add support dependencies
    nSuppSize = 0;
    Extra_SupportArray( p->dd, bFunc, p->pSupp );
    for ( k = 0; k < p->nVars; k++ )
    {
        nSuppSize += p->pSupp[k];
        if ( p->pSupp[k] && p->pVars2Q[k] )
            Llb_NonlinAddPair( p, bFunc, i, k );
    }
    p->nSuppMax = Abc_MaxInt( p->nSuppMax, nSuppSize ); 
}

/**Function*************************************************************

  Synopsis    [Starts non-linear quantification scheduling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_NonlinStart( Llb_Mgr_t * p )
{
    Vec_Ptr_t * vRootBdds;
    DdNode * bFunc;
    int i;
    // create and collect BDDs
    vRootBdds = Llb_NonlinBuildBdds( p->pAig, p->vLeaves, p->vRoots, p->dd ); // come referenced
    if ( vRootBdds == NULL )
        return 0;
    // add pairs (refs are consumed inside)
    Vec_PtrForEachEntry( DdNode *, vRootBdds, bFunc, i )
        Llb_NonlinAddPartition( p, i, bFunc );
    Vec_PtrFree( vRootBdds );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks that each var appears in at least one partition.]

  Description []
               
  SideEffects []

  SeeAlso     []
**********************************************************************/
void Llb_NonlinCheckVars( Llb_Mgr_t * p )
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
int Llb_NonlinNextPartitions( Llb_Mgr_t * p, Llb_Prt_t ** ppPart1, Llb_Prt_t ** ppPart2 )
{
    Llb_Var_t * pVar, * pVarBest = NULL;
    Llb_Prt_t * pPart, * pPart1Best = NULL, * pPart2Best = NULL;
    int i;
    Llb_NonlinCheckVars( p );
    // find variable with minimum score
    Llb_MgrForEachVar( p, pVar, i )
        if ( pVarBest == NULL || pVarBest->nScore > pVar->nScore )
            pVarBest = pVar;
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
    *ppPart1 = pPart1Best;
    *ppPart2 = pPart2Best;
    return 1; 
}

/**Function*************************************************************

  Synopsis    [Reorders BDDs in the working manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_NonlinReorder( DdManager * dd, int fTwice, int fVerbose )
{
    abctime clk = Abc_Clock();
    if ( fVerbose )
        Abc_Print( 1, "Reordering... Before =%5d. ", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    if ( fVerbose )
        Abc_Print( 1, "After =%5d. ", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );
    if ( fTwice )
    {
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
        if ( fVerbose )
            Abc_Print( 1, "After =%5d. ", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );
    }
    if ( fVerbose )
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    [Recomputes scores after variable reordering.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_NonlinRecomputeScores( Llb_Mgr_t * p )
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
void Llb_NonlinVerifyScores( Llb_Mgr_t * p )
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
Llb_Mgr_t * Llb_NonlinAlloc( Aig_Man_t * pAig, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vRoots, int * pVars2Q, DdManager * dd )
{
    Llb_Mgr_t * p;
    p = ABC_CALLOC( Llb_Mgr_t, 1 );
    p->pAig      = pAig;
    p->vLeaves   = vLeaves;
    p->vRoots    = vRoots;
    p->dd        = dd;
    p->pVars2Q   = pVars2Q;
    p->nVars     = Cudd_ReadSize(dd);
    p->iPartFree = Vec_PtrSize(vRoots);
    p->pVars     = ABC_CALLOC( Llb_Var_t *, p->nVars );
    p->pParts    = ABC_CALLOC( Llb_Prt_t *, 2 * p->iPartFree + 2 );
    p->pSupp     = ABC_ALLOC( int, Cudd_ReadSize(dd) );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops non-linear quantification scheduling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_NonlinFree( Llb_Mgr_t * p )
{
    Llb_Prt_t * pPart;
    Llb_Var_t * pVar;
    int i;
    Llb_MgrForEachVar( p, pVar, i )
        Llb_NonlinRemoveVar( p, pVar );
    Llb_MgrForEachPart( p, pPart, i )
        Llb_NonlinRemovePart( p, pPart );
    ABC_FREE( p->pVars );
    ABC_FREE( p->pParts );
    ABC_FREE( p->pSupp );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Performs image computation.]

  Description [Computes image of BDDs (vFuncs).]
               
  SideEffects [BDDs in vFuncs are derefed inside. The result is refed.]

  SeeAlso     []

***********************************************************************/
DdNode * Llb_NonlinImage( Aig_Man_t * pAig, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vRoots, int * pVars2Q, 
    DdManager * dd, DdNode * bCurrent, int fReorder, int fVerbose, int * pOrder )
{
    Llb_Prt_t * pPart, * pPart1, * pPart2;
    Llb_Mgr_t * p;
    DdNode * bFunc, * bTemp;
    int i, nReorders, timeInside;
    abctime clk = Abc_Clock(), clk2;
    // start the manager
    clk2 = Abc_Clock();
    p = Llb_NonlinAlloc( pAig, vLeaves, vRoots, pVars2Q, dd );
    if ( !Llb_NonlinStart( p ) )
    {
        Llb_NonlinFree( p );
        return NULL;
    }
    // add partition
    Llb_NonlinAddPartition( p, p->iPartFree++, bCurrent );
    // remove singles
    Llb_MgrForEachPart( p, pPart, i )
        if ( Llb_NonlinHasSingletonVars(p, pPart) )
            Llb_NonlinQuantify1( p, pPart, 0 );
    timeBuild += Abc_Clock() - clk2;
    timeInside = Abc_Clock() - clk2;
    // compute scores
    Llb_NonlinRecomputeScores( p );
    // save permutation
    if ( pOrder )
    memcpy( pOrder, dd->invperm, sizeof(int) * dd->size );
    // iteratively quantify variables
    while ( Llb_NonlinNextPartitions(p, &pPart1, &pPart2) )
    {
        clk2 = Abc_Clock();
        nReorders = Cudd_ReadReorderings(dd);
        if ( !Llb_NonlinQuantify2( p, pPart1, pPart2 ) )
        {
            Llb_NonlinFree( p );
            return NULL;
        }
        timeAndEx  += Abc_Clock() - clk2;
        timeInside += Abc_Clock() - clk2;
        if ( nReorders < Cudd_ReadReorderings(dd) )
            Llb_NonlinRecomputeScores( p );
//        else
//            Llb_NonlinVerifyScores( p );
    }
    // load partitions
    bFunc = Cudd_ReadOne(p->dd);   Cudd_Ref( bFunc );
    Llb_MgrForEachPart( p, pPart, i )
    {
        bFunc = Cudd_bddAnd( p->dd, bTemp = bFunc, pPart->bFunc );   Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( p->dd, bTemp );
    }
    nSuppMax = p->nSuppMax;
    Llb_NonlinFree( p );
    // reorder variables
    if ( fReorder )
        Llb_NonlinReorder( dd, 0, fVerbose );
    timeOther += Abc_Clock() - clk - timeInside;
    // return
    Cudd_Deref( bFunc );
    return bFunc;
}



static Llb_Mgr_t * p = NULL;

/**Function*************************************************************

  Synopsis    [Starts image computation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager * Llb_NonlinImageStart( Aig_Man_t * pAig, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vRoots, int * pVars2Q, int * pOrder, int fFirst, abctime TimeTarget )
{
    DdManager * dd;
    abctime clk = Abc_Clock();
    assert( p == NULL );
    // start a new manager (disable reordering)
    dd = Cudd_Init( Aig_ManObjNumMax(pAig), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    dd->TimeStop = TimeTarget;
    Cudd_ShuffleHeap( dd, pOrder );
//    if ( fFirst )
        Cudd_AutodynEnable( dd,  CUDD_REORDER_SYMM_SIFT );
    // start the manager
    p = Llb_NonlinAlloc( pAig, vLeaves, vRoots, pVars2Q, dd );
    if ( !Llb_NonlinStart( p ) )
    {
        Llb_NonlinFree( p );
        p = NULL;
        return NULL;
    }
    timeBuild += Abc_Clock() - clk;
//    if ( !fFirst )
//        Cudd_AutodynEnable( dd,  CUDD_REORDER_SYMM_SIFT );
    return dd;
}

/**Function*************************************************************

  Synopsis    [Performs image computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_NonlinImageCompute( DdNode * bCurrent, int fReorder, int fDrop, int fVerbose, int * pOrder )
{
    Llb_Prt_t * pPart, * pPart1, * pPart2;
    DdNode * bFunc, * bTemp;
    int i, nReorders, timeInside = 0;
    abctime clk = Abc_Clock(), clk2;

    // add partition
    Llb_NonlinAddPartition( p, p->iPartFree++, bCurrent );
    // remove singles
    Llb_MgrForEachPart( p, pPart, i )
        if ( Llb_NonlinHasSingletonVars(p, pPart) )
            Llb_NonlinQuantify1( p, pPart, 0 );
    // reorder
    if ( fReorder )
        Llb_NonlinReorder( p->dd, 0, 0 );
    // save permutation
    memcpy( pOrder, p->dd->invperm, sizeof(int) * p->dd->size );

    // compute scores
    Llb_NonlinRecomputeScores( p );
    // iteratively quantify variables
    while ( Llb_NonlinNextPartitions(p, &pPart1, &pPart2) )
    {
        clk2 = Abc_Clock();
        nReorders = Cudd_ReadReorderings(p->dd);
        if ( !Llb_NonlinQuantify2( p, pPart1, pPart2 ) )
        {
            Llb_NonlinFree( p );
            return NULL;
        }
        timeAndEx  += Abc_Clock() - clk2;
        timeInside += Abc_Clock() - clk2;
        if ( nReorders < Cudd_ReadReorderings(p->dd) )
            Llb_NonlinRecomputeScores( p );
//        else
//            Llb_NonlinVerifyScores( p );
    }
    // load partitions
    bFunc = Cudd_ReadOne(p->dd);   Cudd_Ref( bFunc );
    Llb_MgrForEachPart( p, pPart, i )
    {
        bFunc = Cudd_bddAnd( p->dd, bTemp = bFunc, pPart->bFunc );
        if ( bFunc == NULL )
        {
            Cudd_RecursiveDeref( p->dd, bTemp );
            Llb_NonlinFree( p );
            return NULL;
        }
        Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( p->dd, bTemp );
    }
    nSuppMax = p->nSuppMax;
    // reorder variables
//    if ( fReorder )
//        Llb_NonlinReorder( p->dd, 0, fVerbose );
    // save permutation
//    memcpy( pOrder, p->dd->invperm, sizeof(int) * Cudd_ReadSize(p->dd) );

    timeOther += Abc_Clock() - clk - timeInside;
    // return
    Cudd_Deref( bFunc );
    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Quits image computation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_NonlinImageQuit()
{
    DdManager * dd;
    if ( p == NULL )
        return;
    dd = p->dd;
    Llb_NonlinFree( p );
    if ( dd->bFunc )
        Cudd_RecursiveDeref( dd, dd->bFunc );
    Extra_StopManager( dd );
//    Cudd_Quit ( dd );
    p = NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

