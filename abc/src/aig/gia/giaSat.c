/**CFile****************************************************************

  FileName    [giaSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [New constraint-propagation procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaSat.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define GIA_LIMIT 10


typedef struct Gia_ManSat_t_ Gia_ManSat_t;
struct Gia_ManSat_t_
{
    Aig_MmFlex_t *    pMem;
};

typedef struct Gia_ObjSat1_t_ Gia_ObjSat1_t;
struct Gia_ObjSat1_t_
{
    char              nFans;
    char              nOffset;
    char              PathsH;
    char              PathsV;
};

typedef struct Gia_ObjSat2_t_ Gia_ObjSat2_t;
struct Gia_ObjSat2_t_
{
    unsigned          fTerm :  1;
    unsigned          iLit  : 31;
};

typedef struct Gia_ObjSat_t_ Gia_ObjSat_t;
struct Gia_ObjSat_t_
{
    union {
        Gia_ObjSat1_t Obj1;
        Gia_ObjSat2_t Obj2;
    };
};


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_ManSat_t * Gia_ManSatStart()
{
    Gia_ManSat_t * p;
    p = ABC_CALLOC( Gia_ManSat_t, 1 );
    p->pMem = Aig_MmFlexStart();
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSatStop( Gia_ManSat_t * p )
{
    Aig_MmFlexStop( p->pMem, 0 );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Collects the supergate rooted at this ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSatPartCollectSuper( Gia_Man_t * p, Gia_Obj_t * pObj, int * pLits, int * pnLits )
{
    Gia_Obj_t * pFanin;
    assert( Gia_ObjIsAnd(pObj) );
    assert( pObj->fMark0 == 0 );
    pFanin = Gia_ObjFanin0(pObj);
    if ( pFanin->fMark0 || Gia_ObjFaninC0(pObj) )
        pLits[(*pnLits)++] = Gia_Var2Lit(Gia_ObjId(p, pFanin), Gia_ObjFaninC0(pObj));
    else
        Gia_ManSatPartCollectSuper(p, pFanin, pLits, pnLits);
    pFanin = Gia_ObjFanin1(pObj);
    if ( pFanin->fMark0 || Gia_ObjFaninC1(pObj) )
        pLits[(*pnLits)++] = Gia_Var2Lit(Gia_ObjId(p, pFanin), Gia_ObjFaninC1(pObj));
    else
        Gia_ManSatPartCollectSuper(p, pFanin, pLits, pnLits);
}

/**Function*************************************************************

  Synopsis    [Returns the number of words used.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSatPartCreate_rec( Gia_Man_t * p, Gia_Obj_t * pObj, int * pObjPlace, int * pStore )
{
    Gia_Obj_t * pFanin;
    int i, nWordsUsed, nSuperSize = 0, Super[2*GIA_LIMIT];
    // make sure this is a valid node
    assert( Gia_ObjIsAnd(pObj) );
    assert( pObj->fMark0 == 0 );
    // collect inputs to the supergate
    Gia_ManSatPartCollectSuper( p, pObj, Super, &nSuperSize );
    assert( nSuperSize <= 2*GIA_LIMIT );
    // create the root entry
    *pObjPlace = 0;
    ((Gia_ObjSat1_t *)pObjPlace)->nFans   = Gia_Var2Lit( nSuperSize, 0 );
    ((Gia_ObjSat1_t *)pObjPlace)->nOffset = pStore - pObjPlace;
    nWordsUsed = nSuperSize;
    for ( i = 0; i < nSuperSize; i++ )
    {
        pFanin = Gia_ManObj( p, Gia_Lit2Var(Super[i]) );
        if ( pFanin->fMark0 )
        {
            ((Gia_ObjSat2_t *)(pStore + i))->fTerm = 1;
            ((Gia_ObjSat2_t *)(pStore + i))->iLit  = Super[i];
        }
        else
        {
            assert( Gia_LitIsCompl(Super[i]) );
            nWordsUsed += Gia_ManSatPartCreate_rec( p, pFanin, pStore + i, pStore + nWordsUsed );
        }
    }
    return nWordsUsed;
}

/**Function*************************************************************

  Synopsis    [Creates part and returns the number of words used.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSatPartCreate( Gia_Man_t * p, Gia_Obj_t * pObj, int * pStore )
{
    return 1 + Gia_ManSatPartCreate_rec( p, pObj, pStore, pStore + 1 );
}


/**Function*************************************************************

  Synopsis    [Count the number of internal nodes in the leaf-DAG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSatPartCountClauses( Gia_Man_t * p, Gia_Obj_t * pObj, int * pnOnset, int * pnOffset )
{
    Gia_Obj_t * pFanin;
    int nOnset0, nOnset1, nOffset0, nOffset1;
    assert( Gia_ObjIsAnd(pObj) );
    pFanin = Gia_ObjFanin0(pObj);
    if ( pFanin->fMark0 )
        nOnset0 = 1, nOffset0 = 1;
    else
    {
        Gia_ManSatPartCountClauses(p, pFanin, &nOnset0, &nOffset0);
        if ( Gia_ObjFaninC0(pObj) )
        {
            int Temp = nOnset0;
            nOnset0 = nOffset0;
            nOffset0 = Temp;
        }
    }
    pFanin = Gia_ObjFanin1(pObj);
    if ( pFanin->fMark0 )
        nOnset1 = 1, nOffset1 = 1;
    else
    {
        Gia_ManSatPartCountClauses(p, pFanin, &nOnset1, &nOffset1);
        if ( Gia_ObjFaninC1(pObj) )
        {
            int Temp = nOnset1;
            nOnset1 = nOffset1;
            nOffset1 = Temp;
        }
    }
    *pnOnset  = nOnset0  * nOnset1;
    *pnOffset = nOffset0 + nOffset1;
}

/**Function*************************************************************

  Synopsis    [Count the number of internal nodes in the leaf-DAG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSatPartCount( Gia_Man_t * p, Gia_Obj_t * pObj, int * pnLeaves, int * pnNodes )
{
    Gia_Obj_t * pFanin;
    int Level0 = 0, Level1 = 0;
    assert( Gia_ObjIsAnd(pObj) );
    assert( pObj->fMark0 == 0 );
    (*pnNodes)++;
    pFanin = Gia_ObjFanin0(pObj);
    if ( pFanin->fMark0 )
        (*pnLeaves)++;
    else
        Level0 = Gia_ManSatPartCount(p, pFanin, pnLeaves, pnNodes) + Gia_ObjFaninC0(pObj);
    pFanin = Gia_ObjFanin1(pObj);
    if ( pFanin->fMark0 )
        (*pnLeaves)++;
    else
        Level1 = Gia_ManSatPartCount(p, pFanin, pnLeaves, pnNodes) + Gia_ObjFaninC1(pObj);
    return Abc_MaxInt( Level0, Level1 );
}

/**Function*************************************************************

  Synopsis    [Count the number of internal nodes in the leaf-DAG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSatPartCountNodes( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    Gia_Obj_t * pFanin;
    int nNodes0 = 0, nNodes1 = 0;
    assert( Gia_ObjIsAnd(pObj) );
    assert( pObj->fMark0 == 0 );
    pFanin = Gia_ObjFanin0(pObj);
    if ( !(pFanin->fMark0) )
        nNodes0 = Gia_ManSatPartCountNodes(p, pFanin);
    pFanin = Gia_ObjFanin1(pObj);
    if ( !(pFanin->fMark0) )
        nNodes1 = Gia_ManSatPartCountNodes(p, pFanin);
    return nNodes0 + nNodes1 + 1;
}

/**Function*************************************************************

  Synopsis    [Count the number of internal nodes in the leaf-DAG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSatPartPrint( Gia_Man_t * p, Gia_Obj_t * pObj, int Step )
{
    Gia_Obj_t * pFanin;
    assert( Gia_ObjIsAnd(pObj) );
    assert( pObj->fMark0 == 0 );
    pFanin = Gia_ObjFanin0(pObj);
    if ( pFanin->fMark0 )
        printf( "%s%d", Gia_ObjFaninC0(pObj)?"!":"", Gia_ObjId(p,pFanin) );
    else
    {
        if ( Gia_ObjFaninC0(pObj) )
            printf( "(" );
        Gia_ManSatPartPrint(p, pFanin, Step + Gia_ObjFaninC0(pObj));
        if ( Gia_ObjFaninC0(pObj) )
            printf( ")" );
    }
    printf( "%s", (Step & 1)? " + " : "*" );
    pFanin = Gia_ObjFanin1(pObj);
    if ( pFanin->fMark0 )
        printf( "%s%d", Gia_ObjFaninC1(pObj)?"!":"", Gia_ObjId(p,pFanin) );
    else
    {
        if ( Gia_ObjFaninC1(pObj) )
            printf( "(" );
        Gia_ManSatPartPrint(p, pFanin, Step + Gia_ObjFaninC1(pObj));
        if ( Gia_ObjFaninC1(pObj) )
            printf( ")" );
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSatExperiment( Gia_Man_t * p )
{
    int nStored, Storage[1000], * pStart; 
    Gia_ManSat_t * pMan;
    Gia_Obj_t * pObj;
    int i, nLevels, nLeaves, nNodes, nCount[2*GIA_LIMIT+2] = {0}, nCountAll = 0;
    int Num0 = 0, Num1 = 0;
    clock_t clk = clock();
    int nWords = 0, nWords2 = 0;
    pMan = Gia_ManSatStart();
    // mark the nodes to become roots of leaf-DAGs
    Gia_ManCreateValueRefs( p );
    Gia_ManForEachObj( p, pObj, i )
    {
        pObj->fMark0 = 0;
        if ( Gia_ObjIsCo(pObj) )
            Gia_ObjFanin0(pObj)->fMark0 = 1;
        else if ( Gia_ObjIsCi(pObj) )
            pObj->fMark0 = 1;
        else if ( Gia_ObjIsAnd(pObj) )
        {
            if ( pObj->Value > 1 || Gia_ManSatPartCountNodes(p,pObj) >= GIA_LIMIT )
                pObj->fMark0 = 1;
        }
        pObj->Value = 0;
    }
    // compute the sizes of leaf-DAGs
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( pObj->fMark0 == 0 )
            continue;
        pObj->fMark0 = 0;

        nCountAll++;
        nStored = Gia_ManSatPartCreate( p, pObj, Storage );
        nWords2 += nStored;
        assert( nStored < 500 );
        pStart = (int *)Aig_MmFlexEntryFetch( pMan->pMem, sizeof(int) * nStored );
        memcpy( pStart, Storage, sizeof(int) * nStored );

        nLeaves = nNodes = 0;
        nLevels = 1+Gia_ManSatPartCount( p, pObj, &nLeaves, &nNodes );
        nWords += nLeaves + nNodes;
        if ( nNodes <= 2*GIA_LIMIT )
            nCount[nNodes]++;
        else
            nCount[2*GIA_LIMIT+1]++;
//        if ( nNodes > 10 && i % 100 == 0 )
//        if ( nNodes > 5 )
        if ( 0 )
        {
            Gia_ManSatPartCountClauses( p, pObj, &Num0, &Num1 );
            printf( "%8d :  And = %3d.  Lev = %2d. Clauses = %3d. (%3d + %3d).\n", i, nNodes, nLevels, Num0+Num1, Num0, Num1 );
            Gia_ManSatPartPrint( p, pObj, 0 );
            printf( "\n" );
        }

        pObj->fMark0 = 1;
    }
    printf( "\n" );
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark0 = 0;
    Gia_ManSatStop( pMan );
    for ( i = 0; i < 2*GIA_LIMIT+2; i++ )
        printf( "%2d=%6d  %7.2f %%  %7.2f %%\n", i, nCount[i], 100.0*nCount[i]/nCountAll, 100.0*i*nCount[i]/Gia_ManAndNum(p) );
    ABC_PRMn( "MemoryEst", 4*nWords );
    ABC_PRMn( "MemoryReal", 4*nWords2 );
    printf( "%5.2f bpn  ", 4.0*nWords2/Gia_ManObjNum(p) );
    ABC_PRT( "Time", clock() - clk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

