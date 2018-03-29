/**CFile****************************************************************

  FileName    [abcCascade.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Collapsing the network into two-levels.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcCollapse.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

#ifdef ABC_USE_CUDD
#include "bdd/reo/reo.h"
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#ifdef ABC_USE_CUDD

#define BDD_FUNC_MAX 256

//extern void Abc_NodeShowBddOne( DdManager * dd, DdNode * bFunc );
extern DdNode * Abc_ConvertSopToBdd( DdManager * dd, char * pSop, DdNode ** pbVars );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derive BDD of the characteristic function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_ResBuildBdd( Abc_Ntk_t * pNtk, DdManager * dd )
{
    Vec_Ptr_t * vNodes, * vBdds, * vLocals;
    Abc_Obj_t * pObj, * pFanin;
    DdNode * bFunc, * bPart, * bTemp, * bVar;
    int i, k;
    assert( Abc_NtkIsSopLogic(pNtk) );
    assert( Abc_NtkCoNum(pNtk) <= 3 );
    vBdds = Vec_PtrStart( Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachCi( pNtk, pObj, i )
        Vec_PtrWriteEntry( vBdds, Abc_ObjId(pObj), Cudd_bddIthVar(dd, i) );
    // create internal node BDDs
    vNodes = Abc_NtkDfs( pNtk, 0 );
    vLocals = Vec_PtrAlloc( 6 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        if ( Abc_ObjFaninNum(pObj) == 0 )
        {
            bFunc = Cudd_NotCond( Cudd_ReadOne(dd), Abc_SopIsConst0((char *)pObj->pData) );  Cudd_Ref( bFunc );
            Vec_PtrWriteEntry( vBdds, Abc_ObjId(pObj), bFunc );
            continue;
        }
        Vec_PtrClear( vLocals );
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Vec_PtrPush( vLocals, Vec_PtrEntry(vBdds, Abc_ObjId(pFanin)) );
        bFunc = Abc_ConvertSopToBdd( dd, (char *)pObj->pData, (DdNode **)Vec_PtrArray(vLocals) );  Cudd_Ref( bFunc );
        Vec_PtrWriteEntry( vBdds, Abc_ObjId(pObj), bFunc );
    }
    Vec_PtrFree( vLocals );
    // create char function
    bFunc = Cudd_ReadOne( dd );  Cudd_Ref( bFunc );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        bVar  = Cudd_bddIthVar( dd, i + Abc_NtkCiNum(pNtk) );
        bTemp = (DdNode *)Vec_PtrEntry( vBdds, Abc_ObjFaninId0(pObj) );
        bPart = Cudd_bddXnor( dd, bTemp, bVar );          Cudd_Ref( bPart );
        bFunc = Cudd_bddAnd( dd, bTemp = bFunc, bPart );  Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bPart );
    }
    // dereference
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        Cudd_RecursiveDeref( dd, (DdNode *)Vec_PtrEntry(vBdds, Abc_ObjId(pObj)) );
    Vec_PtrFree( vBdds );
    Vec_PtrFree( vNodes );
    // reorder
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
    Cudd_Deref( bFunc );
    return bFunc;
}
 
/**Function*************************************************************

  Synopsis    [Initializes variable partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ResStartPart( int nInputs, unsigned uParts[], int nParts )
{
    int i, Group, Left, Shift = 0, Count = 0;
    Group = nInputs / nParts;
    Left  = nInputs % nParts;
    for ( i = 0; i < Left; i++ )
    {
        uParts[i] = (~((~0) << (Group+1))) << Shift;
        Shift += Group+1;
    }
    for (      ; i < nParts; i++ )
    {
        uParts[i] = (~((~0) << Group)) << Shift;
        Shift += Group;
    }
    for ( i = 0; i < nParts; i++ )
        Count += Extra_WordCountOnes( uParts[i] );
    assert( Count == nInputs );
}
 
/**Function*************************************************************

  Synopsis    [Initializes variable partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ResStartPart2( int nInputs, unsigned uParts[], int nParts )
{
    int i, Count = 0;
    for ( i = 0; i < nParts; i++ )
        uParts[i] = 0;
    for ( i = 0; i < nInputs; i++ )
        uParts[i % nParts] |= (1 << i);
    for ( i = 0; i < nParts; i++ )
        Count += Extra_WordCountOnes( uParts[i] );
    assert( Count == nInputs );
}
 
/**Function*************************************************************

  Synopsis    [Returns one if unique pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ResCheckUnique( char Pats[], int nPats, int pat )
{
    int i;
    for ( i = 0; i < nPats; i++ )
        if ( Pats[i] == pat )
            return 0;
    return 1;
}
 
/**Function*************************************************************

  Synopsis    [Check if pattern is decomposable with non-strict.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ResCheckNonStrict( char Pattern[], int nVars, int nBits )
{
    static char Pat0[256], Pat1[256];
    int v, m, nPats0, nPats1, nNumber = (1 << (nBits - 1));
    int Result = 0;
    for ( v = 0; v < nVars; v++ )
    {
        nPats0 = nPats1 = 0;
        for ( m = 0; m < (1<<nVars); m++ )
        {
            if ( (m & (1 << v)) == 0 )
            {
                if ( Abc_ResCheckUnique( Pat0, nPats0, Pattern[m] ) )
                {
                    Pat0[ nPats0++ ] = Pattern[m];
                    if ( nPats0 > nNumber )
                        break;
                }
            }
            else
            {
                if ( Abc_ResCheckUnique( Pat1, nPats1, Pattern[m] ) )
                {
                    Pat1[ nPats1++ ] = Pattern[m];
                    if ( nPats1 > nNumber )
                        break;
                }
            }
        }
        if ( m == (1<<nVars) )
            Result++;
    }
    return Result;
}

/**Function*************************************************************

  Synopsis    [Compute the number of distinct cofactors in the BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ResCofCount( DdManager * dd, DdNode * bFunc, unsigned uMask, int * pCheck )
{
    static char Pattern[256];
    DdNode * pbVars[32];
    Vec_Ptr_t * vCofs;
    DdNode * bCof, * bCube, * bTemp;
    int i, k, Result, nVars = 0;
    // collect variables
    for ( i = 0; i < 32; i++ )
        if ( uMask & (1 << i) )
            pbVars[nVars++] = dd->vars[i];
    assert( nVars <= 8 );
    // compute cofactors
    vCofs = Vec_PtrAlloc( 100 );
    for ( i = 0; i < (1 << nVars); i++ )
    {
        bCube = Extra_bddBitsToCube( dd, i, nVars, pbVars, 1 );  Cudd_Ref( bCube );
        bCof  = Cudd_Cofactor( dd, bFunc, bCube );               Cudd_Ref( bCof );
        Cudd_RecursiveDeref( dd, bCube );
        Vec_PtrForEachEntry( DdNode *, vCofs, bTemp, k )
            if ( bTemp == bCof )
                break;
        if ( k < Vec_PtrSize(vCofs) )
            Cudd_RecursiveDeref( dd, bCof );
        else
            Vec_PtrPush( vCofs, bCof );
        Pattern[i] = k;
    }
    Result = Vec_PtrSize( vCofs );
    Vec_PtrForEachEntry( DdNode *, vCofs, bCof, i )
        Cudd_RecursiveDeref( dd, bCof );
    Vec_PtrFree( vCofs );
    if ( pCheck )
    {
        *pCheck = Abc_ResCheckNonStrict( Pattern, nVars, Abc_Base2Log(Result) );
/*
        if ( *pCheck == 1 && nVars == 4 && Result == 8 )
        {
            for ( i = 0; i < (1 << nVars); i++ )
                printf( "%d ", Pattern[i] );
            i = 0;
        }
*/
    }
    return Result;
}

/**Function*************************************************************

  Synopsis    [Computes cost of the partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ResCost( DdManager * dd, DdNode * bFunc, unsigned uMask, int * pnCofs, int * pCheck )
{
    int nCofs = Abc_ResCofCount( dd, bFunc, uMask, pCheck );
    int n2Log = Abc_Base2Log( nCofs );
    if ( pnCofs ) *pnCofs = nCofs;
    return 10000 * n2Log + (nCofs - (1 << (n2Log-1))) * (nCofs - (1 << (n2Log-1)));
}

/**Function*************************************************************

  Synopsis    [Migrates variables between the two groups.]

  Description [Returns 1 if there is change.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ResMigrate( DdManager * dd, DdNode * bFunc, int nInputs, unsigned uParts[], int iPart1, int iPart2 )
{
    unsigned uParts2[2] = { uParts[iPart1], uParts[iPart2] };
    int i, k, CostCur, CostBest, fChange = 0;
    assert( (uParts[iPart1] & uParts[iPart2]) == 0 );
    CostBest = Abc_ResCost( dd, bFunc, uParts[iPart1], NULL, NULL ) 
             + Abc_ResCost( dd, bFunc, uParts[iPart2], NULL, NULL );
    for ( i = 0; i < nInputs; i++ )
    if ( uParts[iPart1] & (1 << i) )
    {
        for ( k = 0; k < nInputs; k++ )
        if ( uParts[iPart2] & (1 << k) )
        {
            if ( i == k )
                continue;
            uParts[iPart1] ^= (1 << i) | (1 << k);
            uParts[iPart2] ^= (1 << i) | (1 << k);
            CostCur = Abc_ResCost( dd, bFunc, uParts[iPart1], NULL, NULL ) + Abc_ResCost( dd, bFunc, uParts[iPart2], NULL, NULL );
            if ( CostCur < CostBest )
            {
                CostCur    = CostBest;
                uParts2[0] = uParts[iPart1];
                uParts2[1] = uParts[iPart2];
                fChange = 1;
            }
            uParts[iPart1] ^= (1 << i) | (1 << k);
            uParts[iPart2] ^= (1 << i) | (1 << k);
        }
    }
    uParts[iPart1] = uParts2[0];
    uParts[iPart2] = uParts2[1];
    return fChange;
}

/**Function*************************************************************

  Synopsis    [Migrates variables between the two groups.]

  Description [Returns 1 if there is change.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ResPrint( DdManager * dd, DdNode * bFunc, int nInputs, unsigned uParts[], int nParts )
{
    int i, k, nCofs, Cost, CostAll = 0, fCheck;
    for ( i = 0; i < nParts; i++ )
    {
        Cost = Abc_ResCost( dd, bFunc, uParts[i], &nCofs, &fCheck );
        CostAll += Cost;
        for ( k = 0; k < nInputs; k++ )
            printf( "%c", (uParts[i] & (1 << k))? 'a' + k : '-' );
        printf( " %2d %d-%d %6d   ", nCofs, Abc_Base2Log(nCofs), fCheck, Cost );
    }
    printf( "%4d\n", CostAll );
}

/**Function*************************************************************

  Synopsis    [PrintCompute the number of distinct cofactors in the BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ResPrintAllCofs( DdManager * dd, DdNode * bFunc, int nInputs, int nCofMax )
{
    int i, k, nBits, nCofs, Cost, fCheck;
    for ( i = 0; i < (1<<nInputs); i++ )
    {
        nBits = Extra_WordCountOnes( i );
        if ( nBits < 3 || nBits > 6 )
            continue;
        Cost = Abc_ResCost( dd, bFunc, i, &nCofs, &fCheck );
        if ( nCofs > nCofMax )
            continue;
        for ( k = 0; k < nInputs; k++ )
            printf( "%c", (i & (1 << k))? 'a' + k : '-' );
        printf( "  n=%2d  c=%2d  l=%d-%d   %6d\n", 
            Extra_WordCountOnes(i), nCofs, Abc_Base2Log(nCofs), fCheck, Cost );
    }
}

/**Function*************************************************************

  Synopsis    [Compute the number of distinct cofactors in the BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ResSwapRandom( DdManager * dd, DdNode * bFunc, int nInputs, unsigned uParts[], int nParts, int nTimes )
{
    int i, k, n, iPart1, iPart2;
    for ( n = 0; n < nTimes; )
    {
        // get the vars
        i = k = 0;
        while ( i == k )
        {
            i = rand() % nInputs;
            k = rand() % nInputs;
        }
        // find the groups
        for ( iPart1 = 0; iPart1 < nParts; iPart1++ )
            if ( uParts[iPart1] & (1 << i) )
                break;
        for ( iPart2 = 0; iPart2 < nParts; iPart2++ )
            if ( uParts[iPart2] & (1 << k) )
                break;
        if ( iPart1 == iPart2 )
            continue;
        // swap the vars
        uParts[iPart1] ^= (1 << i) | (1 << k);
        uParts[iPart2] ^= (1 << i) | (1 << k);
        n++;
//printf( "   " );
//Abc_ResPrint( dd, bFunc, nInputs, uParts, nParts );
    }
}

/**Function*************************************************************

  Synopsis    [Compute the number of distinct cofactors in the BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ResPartition( DdManager * dd, DdNode * bFunc, int nInputs )
{
    int nIters = 5;
    unsigned uParts[10];
    int i, fChange = 1;
    int nSuppSize = Cudd_SupportSize( dd, bFunc );
    printf( "Ins =%3d. Outs =%2d. Nodes =%3d. Supp =%2d.\n", 
        nInputs, dd->size-nInputs, Cudd_DagSize(bFunc), nSuppSize );
//Abc_ResPrintAllCofs( dd, bFunc, nInputs, 4 );

    if ( nSuppSize <= 6 )
    {
        printf( "Support is less or equal than 6\n" );
        return;
    }
    if ( nInputs <= 12 )
    {
        Abc_ResStartPart( nInputs, uParts, 2 );
        Abc_ResPrint( dd, bFunc, nInputs, uParts, 2 );
        for ( i = 0; i < nIters; i++ )
        {
            if ( i ) 
            {
                printf( "Randomizing... \n" );
                Abc_ResSwapRandom( dd, bFunc, nInputs, uParts, 2, 20 );
                Abc_ResPrint( dd, bFunc, nInputs, uParts, 2 );
            }
            fChange = 1;
            while ( fChange )
            {
                fChange  = Abc_ResMigrate( dd, bFunc, nInputs, uParts, 0, 1 );
                Abc_ResPrint( dd, bFunc, nInputs, uParts, 2 );
            }
        }
    }
    else if ( nInputs > 12 && nInputs <= 18 )
    {
        Abc_ResStartPart( nInputs, uParts, 3 );
        Abc_ResPrint( dd, bFunc, nInputs, uParts, 3 );
        for ( i = 0; i < nIters; i++ )
        {
            if ( i ) 
            {
                printf( "Randomizing... \n" );
                Abc_ResSwapRandom( dd, bFunc, nInputs, uParts, 3, 20 );
                Abc_ResPrint( dd, bFunc, nInputs, uParts, 3 );
            }
            fChange = 1;
            while ( fChange )
            {
                fChange  = Abc_ResMigrate( dd, bFunc, nInputs, uParts, 0, 1 );
                Abc_ResPrint( dd, bFunc, nInputs, uParts, 3 );
                fChange |= Abc_ResMigrate( dd, bFunc, nInputs, uParts, 0, 2 );
                Abc_ResPrint( dd, bFunc, nInputs, uParts, 3 );
                fChange |= Abc_ResMigrate( dd, bFunc, nInputs, uParts, 1, 2 );
                Abc_ResPrint( dd, bFunc, nInputs, uParts, 3 );
            }
        }
    }
    else if ( nInputs > 18 && nInputs <= 24 )
    {
        Abc_ResStartPart( nInputs, uParts, 4 );
        Abc_ResPrint( dd, bFunc, nInputs, uParts, 4 );
        for ( i = 0; i < nIters; i++ )
        {
            if ( i )
            {
                printf( "Randomizing... \n" );
                Abc_ResSwapRandom( dd, bFunc, nInputs, uParts, 4, 20 );
                Abc_ResPrint( dd, bFunc, nInputs, uParts, 4 );
            }
            fChange = 1;
            while ( fChange )
            {
                fChange  = Abc_ResMigrate( dd, bFunc, nInputs, uParts, 0, 1 );
                Abc_ResPrint( dd, bFunc, nInputs, uParts, 4 );
                fChange |= Abc_ResMigrate( dd, bFunc, nInputs, uParts, 0, 2 );
                Abc_ResPrint( dd, bFunc, nInputs, uParts, 4 );
                fChange |= Abc_ResMigrate( dd, bFunc, nInputs, uParts, 0, 3 );
                Abc_ResPrint( dd, bFunc, nInputs, uParts, 4 );
                fChange |= Abc_ResMigrate( dd, bFunc, nInputs, uParts, 1, 2 );
                Abc_ResPrint( dd, bFunc, nInputs, uParts, 4 );
                fChange |= Abc_ResMigrate( dd, bFunc, nInputs, uParts, 1, 3 );
                Abc_ResPrint( dd, bFunc, nInputs, uParts, 4 );
                fChange |= Abc_ResMigrate( dd, bFunc, nInputs, uParts, 2, 3 );
                Abc_ResPrint( dd, bFunc, nInputs, uParts, 4 );
            }
        }
    }
//    else assert( 0 );
}

/**Function*************************************************************

  Synopsis    [Compute the number of distinct cofactors in the BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ResPartitionTest( Abc_Ntk_t * pNtk )
{
    DdManager * dd;
    DdNode * bFunc;
    dd = Cudd_Init( Abc_NtkCiNum(pNtk) + Abc_NtkCoNum(pNtk), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    bFunc = Abc_ResBuildBdd( pNtk, dd );   Cudd_Ref( bFunc );
    Abc_ResPartition( dd, bFunc, Abc_NtkCiNum(pNtk) );
    Cudd_RecursiveDeref( dd, bFunc );
    Extra_StopManager( dd );
}







/**Function*************************************************************

  Synopsis    [Compute the number of distinct cofactors in the BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkBddCofCount( DdManager * dd, DdNode * bFunc, DdNode ** pbVars, int nVars )
{
    Vec_Ptr_t * vCofs;
    DdNode * bCof, * bCube;
    int i, Result;
    vCofs = Vec_PtrAlloc( 100 );
    for ( i = 0; i < (1 << nVars); i++ )
    {
        bCube = Extra_bddBitsToCube( dd, i, nVars, pbVars, 1 );  Cudd_Ref( bCube );
        bCof  = Cudd_Cofactor( dd, bFunc, bCube );               Cudd_Ref( bCof );
        Cudd_RecursiveDeref( dd, bCube );
        if ( Vec_PtrPushUnique( vCofs, bCof ) )
            Cudd_RecursiveDeref( dd, bCof );
    }
    Result = Vec_PtrSize( vCofs );
    Vec_PtrForEachEntry( DdNode *, vCofs, bCof, i )
        Cudd_RecursiveDeref( dd, bCof );
    Vec_PtrFree( vCofs );
    return Result;
}

/**Function*************************************************************

  Synopsis    [Compute the number of distinct cofactors in the BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkExploreCofs2( DdManager * dd, DdNode * bFunc, DdNode ** pbVars, int nIns, int nLutSize )
{
    int i;
    printf( "Inputs = %2d.  Nodes = %2d.  LutSize = %2d.\n", nIns, Cudd_DagSize(bFunc), nLutSize );
    for ( i = 0; i <= nIns - nLutSize; i++ )
        printf( "[%2d %2d] : %3d\n", i, i+nLutSize-1, Abc_NtkBddCofCount(dd, bFunc, dd->vars+i, nLutSize) );
}

/**Function*************************************************************

  Synopsis    [Compute the number of distinct cofactors in the BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkExploreCofs( DdManager * dd, DdNode * bFunc, DdNode ** pbVars, int nIns, int nLutSize )
{
    DdManager * ddNew;
    DdNode * bFuncNew;
    DdNode * pbVarsNew[32];
    int i, k, c, nCofs, nBits;

    ddNew = Cudd_Init( dd->size, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_ShuffleHeap( ddNew, dd->invperm );
    bFuncNew = Cudd_bddTransfer( dd, ddNew, bFunc );  Cudd_Ref( bFuncNew );

    for ( i = 0; i < (1 << nIns); i++ )
    {
        nBits = Extra_WordCountOnes(i);
        if ( nBits != nLutSize && nBits != nLutSize -1 && nBits != nLutSize -2  )
            continue;
        for ( c = k = 0; k < nIns; k++ )
        {
            if ( (i & (1 << k)) == 0 )
                continue;
//            pbVarsNew[c++] = pbVars[k];
            pbVarsNew[c++] = ddNew->vars[k];
        }
        nCofs = Abc_NtkBddCofCount(ddNew, bFuncNew, pbVarsNew, c);
        if ( nCofs > 8 )
            continue;

        for ( c = k = 0; k < nIns; k++ )
        {
            if ( (i & (1 << k)) == 0 )
            {
                printf( "-" );
                continue;
            }
            printf( "%c", k + 'a' );
        }
        printf( " : %2d\n", nCofs );
    }

    Cudd_RecursiveDeref( ddNew, bFuncNew );
    Extra_StopManager( ddNew );
}

/**Function*************************************************************

  Synopsis    [Find the constant node corresponding to the encoded output value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkBddFindAddConst( DdManager * dd, DdNode * bFunc, int nOuts )
{
    int i, TermMask = 0;
    DdNode * bFunc0, * bFunc1, * bConst0, * bConst1;
    bConst0 = Cudd_ReadLogicZero( dd );
    bConst1 = Cudd_ReadOne( dd );
    for ( i = 0; i < nOuts; i++ )
    {
        if ( Cudd_IsComplement(bFunc) )
        {
            bFunc0 = Cudd_Not(Cudd_E(bFunc));
            bFunc1 = Cudd_Not(Cudd_T(bFunc));
        }
        else
        {
            bFunc0 = Cudd_E(bFunc);
            bFunc1 = Cudd_T(bFunc);
        }
        assert( bFunc0 == bConst0 || bFunc1 == bConst0 );
        if ( bFunc0 == bConst0 )
        {
            TermMask ^= (1 << i);
            bFunc = bFunc1;
        }
        else
            bFunc = bFunc0;
    }
    assert( bFunc == bConst1 );
    return Cudd_addConst( dd, TermMask );
}

/**Function*************************************************************

  Synopsis    [Recursively construct ADD for BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkBddToAdd_rec( DdManager * dd, DdNode * bFunc, int nOuts, stmm_table * tTable, int fCompl )
{
    DdNode * aFunc0, * aFunc1, * aFunc;
    DdNode ** ppSlot;
    assert( !Cudd_IsComplement(bFunc) );
    if ( stmm_find_or_add( tTable, (char *)bFunc, (char ***)&ppSlot ) )
        return *ppSlot;
    if ( (int)bFunc->index >= Cudd_ReadSize(dd) - nOuts )
    {
        assert( Cudd_ReadPerm(dd, bFunc->index) >= Cudd_ReadSize(dd) - nOuts );
        aFunc = Abc_NtkBddFindAddConst( dd, Cudd_NotCond(bFunc, fCompl), nOuts ); Cudd_Ref( aFunc );
    }
    else
    {
        aFunc0 = Abc_NtkBddToAdd_rec( dd, Cudd_Regular(cuddE(bFunc)), nOuts, tTable, fCompl ^ Cudd_IsComplement(cuddE(bFunc)) );
        aFunc1 = Abc_NtkBddToAdd_rec( dd, cuddT(bFunc), nOuts, tTable, fCompl );                                                
        aFunc  = Cudd_addIte( dd, Cudd_addIthVar(dd, bFunc->index), aFunc1, aFunc0 );  Cudd_Ref( aFunc );
    }
    return (*ppSlot = aFunc);
}

/**Function*************************************************************

  Synopsis    [R]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkBddToAdd( DdManager * dd, DdNode * bFunc, int nOuts )
{
    DdNode * aFunc, * aTemp, * bTemp;
    stmm_table * tTable;
    stmm_generator * gen;
    tTable = stmm_init_table( st__ptrcmp, st__ptrhash );
    aFunc = Abc_NtkBddToAdd_rec( dd, Cudd_Regular(bFunc), nOuts, tTable, Cudd_IsComplement(bFunc) );  
    stmm_foreach_item( tTable, gen, (char **)&bTemp, (char **)&aTemp )
        Cudd_RecursiveDeref( dd, aTemp );
    stmm_free_table( tTable );
    Cudd_Deref( aFunc );
    return aFunc;
}

/**Function*************************************************************

  Synopsis    [Recursively construct ADD for BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkAddToBdd_rec( DdManager * dd, DdNode * aFunc, int nIns, int nOuts, stmm_table * tTable )
{
    DdNode * bFunc0, * bFunc1, * bFunc;
    DdNode ** ppSlot;
    assert( !Cudd_IsComplement(aFunc) );
    if ( stmm_find_or_add( tTable, (char *)aFunc, (char ***)&ppSlot ) )
        return *ppSlot;
    if ( Cudd_IsConstant(aFunc) )
    {
        assert( Cudd_ReadSize(dd) >= nIns + nOuts );
        bFunc  = Extra_bddBitsToCube( dd, (int)Cudd_V(aFunc), nOuts, dd->vars + nIns, 1 );  Cudd_Ref( bFunc );
    }
    else
    {
        assert( aFunc->index < nIns );
        bFunc0 = Abc_NtkAddToBdd_rec( dd, cuddE(aFunc), nIns, nOuts, tTable );
        bFunc1 = Abc_NtkAddToBdd_rec( dd, cuddT(aFunc), nIns, nOuts, tTable );                                                
        bFunc  = Cudd_bddIte( dd, Cudd_bddIthVar(dd, aFunc->index), bFunc1, bFunc0 );  Cudd_Ref( bFunc );
    }
    return (*ppSlot = bFunc);
}

/**Function*************************************************************

  Synopsis    [R]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkAddToBdd( DdManager * dd, DdNode * aFunc, int nIns, int nOuts )
{
    DdNode * bFunc, * bTemp, * aTemp;
    stmm_table * tTable;
    stmm_generator * gen;
    tTable = stmm_init_table( st__ptrcmp, st__ptrhash );
    bFunc = Abc_NtkAddToBdd_rec( dd, aFunc, nIns, nOuts, tTable );  
    stmm_foreach_item( tTable, gen, (char **)&aTemp, (char **)&bTemp )
        Cudd_RecursiveDeref( dd, bTemp );
    stmm_free_table( tTable );
    Cudd_Deref( bFunc );
    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Computes the characteristic function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkBddDecCharFunc( DdManager * dd, DdNode ** pFuncs, int nOuts, int Mask, int nBits )
{
    DdNode * bFunc, * bTemp, * bExor, * bVar;
    int i, Count = 0;
    bFunc = Cudd_ReadOne( dd );  Cudd_Ref( bFunc );
    for ( i = 0; i < nOuts; i++ )
    {
        if ( (Mask & (1 << i)) == 0 )
            continue;
        Count++;
        bVar  = Cudd_bddIthVar( dd, dd->size - nOuts + i );
        bExor = Cudd_bddXor( dd, pFuncs[i], bVar );                  Cudd_Ref( bExor );
        bFunc = Cudd_bddAnd( dd, bTemp = bFunc, Cudd_Not(bExor) );   Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bExor );
    }
    Cudd_Deref( bFunc );
    assert( Count == nBits );
    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Evaluate Sasao's decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkBddDecTry( reo_man * pReo, DdManager * dd, DdNode ** pFuncs, int nIns, int nOuts, int Mask, int nBits )
{
//    int fReorder = 0;
    DdNode * bFunc;//, * aFunc, * aFuncNew;
    // derive the characteristic function
    bFunc = Abc_NtkBddDecCharFunc( dd, pFuncs, nOuts, Mask, nBits );    Cudd_Ref( bFunc );
/*
    // transfer to ADD
    aFunc = Abc_NtkBddToAdd( dd, bFunc, nOuts );                        Cudd_Ref( aFunc );
    Cudd_RecursiveDeref( dd, bFunc );
//Abc_NodeShowBddOne( dd, aFunc );

    // perform reordering for BDD width
    if ( fReorder )
    {
        aFuncNew = Extra_Reorder( pReo, dd, aFunc, NULL );              Cudd_Ref( aFuncNew );
        printf( "Before = %d.  After = %d.\n", Cudd_DagSize(aFunc), Cudd_DagSize(aFuncNew) );
        Cudd_RecursiveDeref( dd, aFunc );
    }
    else
        aFuncNew = aFunc;

    // get back to BDD
    bFunc = Abc_NtkAddToBdd( dd, aFuncNew, nIns, nOuts );  Cudd_Ref( bFunc );
    Cudd_RecursiveDeref( dd, aFuncNew );
//Abc_NodeShowBddOne( dd, bFunc );
    // print the result
//    reoProfileWidthPrint( pReo );
*/
    Cudd_Deref( bFunc );
    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Evaluate Sasao's decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkBddDecInt( reo_man * pReo, DdManager * dd, DdNode ** pFuncs, int nIns, int nOuts )
{
/*
    int i, k;
    for ( i = 1; i <= nOuts; i++ )
    {
        for ( k = 0; k < (1<<nOuts); k++ )
            if ( Extra_WordCountOnes(k) == i )
            {
                Extra_PrintBinary( stdout, (unsigned *)&k, nOuts );
                Abc_NtkBddDecTry( pReo, dd, pFuncs, nOuts, k, i );
                printf( "\n" );
            }
    }
*/
    return Abc_NtkBddDecTry( pReo, dd, pFuncs, nIns, nOuts, ~(1<<(32-nOuts)), nOuts );

}

/**Function*************************************************************

  Synopsis    [Evaluate Sasao's decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCreateFromCharFunc( Abc_Ntk_t * pNtk, DdManager * dd, DdNode * bFunc )
{    
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pNode, * pNodeNew, * pNodePo;
    int i;
    // start the network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_BDD, 1 );
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    // create inputs for CIs
    pNodeNew = Abc_NtkCreateNode( pNtkNew );
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        pNode->pCopy = Abc_NtkCreatePi( pNtkNew );
        Abc_ObjAddFanin( pNodeNew, pNode->pCopy );
        Abc_ObjAssignName( pNode->pCopy, Abc_ObjName(pNode), NULL );
    }
    // create inputs for COs
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pNode->pCopy = Abc_NtkCreatePi( pNtkNew );
        Abc_ObjAddFanin( pNodeNew, pNode->pCopy );
        Abc_ObjAssignName( pNode->pCopy, Abc_ObjName(pNode), NULL );
    }
    // transfer BDD
    pNodeNew->pData = Extra_TransferLevelByLevel( dd, (DdManager *)pNtkNew->pManFunc, bFunc ); Cudd_Ref( (DdNode *)pNodeNew->pData );
    // transfer BDD into to be the local function
    pNodePo = Abc_NtkCreatePo( pNtkNew );
    Abc_ObjAddFanin( pNodePo, pNodeNew );
    Abc_ObjAssignName( pNodePo, "out", NULL );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkCreateFromCharFunc(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Evaluate Sasao's decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkBddDec( Abc_Ntk_t * pNtk, int fVerbose )
{
    int nBddSizeMax   = 1000000;
    int fDropInternal =       0;
    int fReorder      =       1;
    Abc_Ntk_t * pNtkNew;
    reo_man * pReo;
    DdManager * dd;
    DdNode * pFuncs[BDD_FUNC_MAX];
    DdNode * bFunc;
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsStrash(pNtk) );
    assert( Abc_NtkCoNum(pNtk) <= BDD_FUNC_MAX );
    dd = (DdManager *)Abc_NtkBuildGlobalBdds( pNtk, nBddSizeMax, fDropInternal, fReorder, fVerbose );
    if ( dd == NULL )
    {
        Abc_Print( -1, "Construction of global BDDs has failed.\n" );
        return NULL;
    }
    // collect global BDDs
    Abc_NtkForEachCo( pNtk, pNode, i )
        pFuncs[i] = (DdNode *)Abc_ObjGlobalBdd(pNode);

    // create new variables at the bottom
    assert( dd->size == Abc_NtkCiNum(pNtk) );
    for ( i = 0; i < Abc_NtkCoNum(pNtk); i++ )
        Cudd_addNewVarAtLevel( dd, dd->size );

    // prepare reordering engine
    pReo = Extra_ReorderInit( Abc_NtkCiNum(pNtk), 1000 );
    Extra_ReorderSetMinimizationType( pReo, REO_MINIMIZE_WIDTH );
    Extra_ReorderSetVerification( pReo, 1 );
    Extra_ReorderSetVerbosity( pReo, 1 );

    // derive characteristic function
    bFunc = Abc_NtkBddDecInt( pReo, dd, pFuncs, Abc_NtkCiNum(pNtk), Abc_NtkCoNum(pNtk) );  Cudd_Ref( bFunc );
    Extra_ReorderQuit( pReo );

Abc_NtkExploreCofs( dd, bFunc, dd->vars, Abc_NtkCiNum(pNtk), 6 );

    // create new network
//    pNtkNew = Abc_NtkCreateFromCharFunc( pNtk, dd, bFunc );
    pNtkNew = Abc_NtkDup( pNtk );

    // cleanup
    Cudd_RecursiveDeref( dd, bFunc );
    Abc_NtkFreeGlobalBdds( pNtk, 1 );
    return pNtkNew;
}

#else

Abc_Ntk_t * Abc_NtkBddDec( Abc_Ntk_t * pNtk, int fVerbose ) { return NULL; }

#endif

ABC_NAMESPACE_IMPL_END

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


