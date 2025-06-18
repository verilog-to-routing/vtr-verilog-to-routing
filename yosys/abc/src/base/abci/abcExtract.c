/**CFile****************************************************************

  FileName    [abcShare.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Shared logic extraction.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcShare.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "aig/gia/giaAig.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define SHARE_NUM 2

typedef struct Abc_ShaMan_t_ Abc_ShaMan_t;
struct Abc_ShaMan_t_ 
{
    int             nMultiSize;
    int             fVerbose;
    Abc_Ntk_t *     pNtk;
    Vec_Ptr_t *     vBuckets;
    Vec_Int_t *     vObj2Lit;
    int             nStartCols;
    int             nCountGates;
    int             nFoundGates;
};

static inline word  Abc_NtkSharePack( int Lev, int Id )  { return (((word)Lev) << 32) | Id; }
static inline int   Abc_NtkShareUnpackLev( word Num )    { return (Num >> 32);              }
static inline int   Abc_NtkShareUnpackId( word Num )     { return Num & 0xFFFFFFFF;         }


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Working with the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_ShaMan_t * Abc_ShaManStart( Abc_Ntk_t * pNtk )
{
    Abc_ShaMan_t * p;
    p = ABC_CALLOC( Abc_ShaMan_t, 1 );
    p->pNtk      = pNtk;
    p->vObj2Lit  = Vec_IntAlloc( 1000 );
    return p;
}
void Abc_ShaManStop( Abc_ShaMan_t * p )
{
    Vec_Ptr_t * vBucket;
    int i;
    Vec_PtrForEachEntry( Vec_Ptr_t *, p->vBuckets, vBucket, i )
        Vec_VecFree( (Vec_Vec_t *)vBucket );
    Vec_PtrFreeP( &p->vBuckets );
    Vec_IntFreeP( &p->vObj2Lit );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Collects one multi-input gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wrd_t * Abc_NtkShareSuperXor( Abc_Obj_t * pObj, int * pfCompl, int * pCounter )
{
    Abc_Ntk_t * pNtk = Abc_ObjNtk(pObj);
    Abc_Obj_t * pObjC, * pObj0, * pObj1, * pRoot = NULL;
    Vec_Wrd_t * vSuper;
    word Num, NumNext;
    int i, k, fCompl = 0;
    assert( !Abc_ObjIsComplement(pObj) );
    assert( Abc_NodeIsExorType(pObj) );
    // start iteration
    vSuper = Vec_WrdAlloc( 10 );
    Vec_WrdPush( vSuper, Abc_NtkSharePack(Abc_ObjLevel(pObj), Abc_ObjId(pObj)) );
    while ( Vec_WrdSize(vSuper) > 0 )
    {
        // make sure there are no duplicates
        Num = Vec_WrdEntry( vSuper, 0 );
        Vec_WrdForEachEntryStart( vSuper, NumNext, i, 1 )
        {
            assert( Num < NumNext );
            Num = NumNext;
        }
        // extract XOR gate decomposable on the topmost level
        Vec_WrdForEachEntryReverse( vSuper, Num, i )
        {
            pRoot = Abc_NtkObj( pNtk, Abc_NtkShareUnpackId(Num) );
            if ( Abc_NodeIsExorType(pRoot) )
            {
                Vec_WrdRemove( vSuper, Num );
                break;
            }
        }
        if ( i == -1 )
            break;
        // extract
        pObjC = Abc_NodeRecognizeMux( pRoot, &pObj1, &pObj0 );
        assert( pObj1 == Abc_ObjNot(pObj0) );
        fCompl ^= Abc_ObjIsComplement(pObjC);  pObjC = Abc_ObjRegular(pObjC);
        fCompl ^= Abc_ObjIsComplement(pObj0);  pObj0 = Abc_ObjRegular(pObj0);
        Vec_WrdPushOrder( vSuper, Abc_NtkSharePack(Abc_ObjLevel(pObjC), Abc_ObjId(pObjC)) );
        Vec_WrdPushOrder( vSuper, Abc_NtkSharePack(Abc_ObjLevel(pObj0), Abc_ObjId(pObj0)) );
        (*pCounter)++;
        // remove duplicates
        k = 0;
        Vec_WrdForEachEntry( vSuper, Num, i )
        {
            if ( i + 1 == Vec_WrdSize(vSuper) )
            {
                Vec_WrdWriteEntry( vSuper, k++, Num );
                break;
            }
            NumNext = Vec_WrdEntry( vSuper, i+1 );
            assert( Num <= NumNext );
            if ( Num == NumNext )
                i++;
            else
                Vec_WrdWriteEntry( vSuper, k++, Num );
        }
        Vec_WrdShrink( vSuper, k );
    }
    *pfCompl = fCompl;
    Vec_WrdForEachEntry( vSuper, Num, i )
        Vec_WrdWriteEntry( vSuper, i, Abc_NtkShareUnpackId(Num) );
    return vSuper;
}
Vec_Wrd_t * Abc_NtkShareSuperAnd( Abc_Obj_t * pObj, int * pCounter )
{
    Abc_Ntk_t * pNtk = Abc_ObjNtk(pObj);
    Abc_Obj_t * pObj0, * pObj1, * pRoot = NULL;
    Vec_Wrd_t * vSuper;
    word Num, NumNext;
    int i, k;
    assert( !Abc_ObjIsComplement(pObj) );
    // start iteration
    vSuper = Vec_WrdAlloc( 10 );
    Vec_WrdPush( vSuper, Abc_NtkSharePack(Abc_ObjLevel(pObj), Abc_ObjToLit(pObj)) );
    while ( Vec_WrdSize(vSuper) > 0 )
    {
        // make sure there are no duplicates
        Num = Vec_WrdEntry( vSuper, 0 );
        Vec_WrdForEachEntryStart( vSuper, NumNext, i, 1 )
        {
            assert( Num < NumNext );
            Num = NumNext;
        }
        // extract AND gate decomposable on the topmost level
        Vec_WrdForEachEntryReverse( vSuper, Num, i )
        {
            pRoot = Abc_ObjFromLit( pNtk, Abc_NtkShareUnpackId(Num) );
            if ( !Abc_ObjIsComplement(pRoot) && Abc_ObjIsNode(pRoot) )
            {
                Vec_WrdRemove( vSuper, Num );
                break;
            }
        }
        if ( i == -1 )
            break;
        assert( Abc_ObjIsNode(pRoot) );
        // extract
        pObj0 = Abc_ObjChild0(pRoot);
        pObj1 = Abc_ObjChild1(pRoot);
        assert( Abc_ObjIsNode(Abc_ObjRegular(pObj0)) || Abc_ObjIsCi(Abc_ObjRegular(pObj0)) );
        assert( Abc_ObjIsNode(Abc_ObjRegular(pObj1)) || Abc_ObjIsCi(Abc_ObjRegular(pObj1)) );
        Vec_WrdPushOrder( vSuper, Abc_NtkSharePack(Abc_ObjLevel(Abc_ObjRegular(pObj0)), Abc_ObjToLit(pObj0)) );
        Vec_WrdPushOrder( vSuper, Abc_NtkSharePack(Abc_ObjLevel(Abc_ObjRegular(pObj1)), Abc_ObjToLit(pObj1)) );
        (*pCounter)++;
        // remove duplicates
        k = 0;
        Vec_WrdForEachEntry( vSuper, Num, i )
        {
            if ( i + 1 == Vec_WrdSize(vSuper) )
            {
                Vec_WrdWriteEntry( vSuper, k++, Num );
                break;
            }
            NumNext = Vec_WrdEntry( vSuper, i+1 );
            assert( Num <= NumNext );
            if ( Num + 1 == NumNext && (NumNext & 1) ) // pos_lit & neg_lit = 0
            {
                Vec_WrdClear( vSuper );
                return vSuper;
            }
            if ( Num < NumNext )
                Vec_WrdWriteEntry( vSuper, k++, Num );
        }
        Vec_WrdShrink( vSuper, k );
    }
    Vec_WrdForEachEntry( vSuper, Num, i )
        Vec_WrdWriteEntry( vSuper, i, Abc_NtkShareUnpackId(Num) );
    return vSuper;
}

/**Function*************************************************************

  Synopsis    [Creates multi-input XOR representation for the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTraverseSupersXor_rec( Abc_ShaMan_t * p, Abc_Obj_t * pObj, Vec_Ptr_t * vInputs )
{
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    if ( Abc_ObjIsCi(pObj) )
        return;
    assert( Abc_ObjIsNode(pObj) );
    if ( Abc_NodeIsExorType(pObj) )
    {
        Vec_Wrd_t * vSuper;
        int k, fCompl;
        word Num;
        vSuper = Abc_NtkShareSuperXor( pObj, &fCompl, &p->nFoundGates );
        if ( Vec_WrdSize(vSuper) <= 1 || Vec_WrdSize(vSuper) >= p->nMultiSize )
        {
            Vec_WrdForEachEntry( vSuper, Num, k )
            {
                Vec_Int_t * vInput = (Vec_Int_t *)Vec_PtrEntry( vInputs, (int)Num );
                if ( vInput == NULL )
                {
                    vInput = Vec_IntAlloc( 10 );
                    Vec_IntPush( vInput, Abc_Var2Lit((int)Num, 0) );
                    Vec_IntPush( vInput, Abc_ObjLevel(Abc_NtkObj(p->pNtk, (int)Num)) );
                    assert( SHARE_NUM == Vec_IntSize(vInput) );
                    Vec_PtrWriteEntry( vInputs, (int)Num, vInput );
                }
                Vec_IntPush( vInput, Vec_IntSize(p->vObj2Lit) );
            }
            Vec_IntPush( p->vObj2Lit, Abc_Var2Lit(Abc_ObjId(pObj), fCompl) );
        }
        // call recursively
        Vec_WrdForEachEntry( vSuper, Num, k )
            Abc_NtkTraverseSupersXor_rec( p, Abc_NtkObj(p->pNtk, (int)Num), vInputs );
        Vec_WrdFree( vSuper );
    }
    else
    {
        Abc_NtkTraverseSupersXor_rec( p, Abc_ObjFanin0(pObj), vInputs );
        Abc_NtkTraverseSupersXor_rec( p, Abc_ObjFanin1(pObj), vInputs );
    }
}
void Abc_NtkTraverseSupersAnd_rec( Abc_ShaMan_t * p, Abc_Obj_t * pObj, Vec_Ptr_t * vInputs )
{
    Vec_Wrd_t * vSuper;
    word Num;
    int k;
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    if ( Abc_ObjIsCi(pObj) )
        return;
    assert( Abc_ObjIsNode(pObj) );
    vSuper = Abc_NtkShareSuperAnd( pObj, &p->nFoundGates );
    if ( Vec_WrdSize(vSuper) <= 1 || Vec_WrdSize(vSuper) >= p->nMultiSize )
    {
        Vec_WrdForEachEntry( vSuper, Num, k )
        {
            Vec_Int_t * vInput = (Vec_Int_t *)Vec_PtrEntry( vInputs, (int)Num );
            if ( vInput == NULL )
            {
                vInput = Vec_IntAlloc( 10 );
                Vec_IntPush( vInput, (int)Num );
                Vec_IntPush( vInput, Abc_ObjLevel(Abc_NtkObj(p->pNtk, Abc_Lit2Var((int)Num))) );
                assert( SHARE_NUM == Vec_IntSize(vInput) );
                Vec_PtrWriteEntry( vInputs, (int)Num, vInput );
            }
            Vec_IntPush( vInput, Vec_IntSize(p->vObj2Lit) );
        }
        Vec_IntPush( p->vObj2Lit, Abc_ObjToLit(pObj) );
    }
    // call recursively
    Vec_WrdForEachEntry( vSuper, Num, k )
        Abc_NtkTraverseSupersAnd_rec( p, Abc_NtkObj(p->pNtk, Abc_Lit2Var((int)Num)), vInputs );
    Vec_WrdFree( vSuper );
}
void Abc_NtkTraverseSupers( Abc_ShaMan_t * p, int fAnd )
{
    Vec_Ptr_t * vInputs;
    Vec_Int_t * vInput;
    Abc_Obj_t * pObj;
    int i, nOnesMax;

    // create mapping of nodes into their column vectors
    vInputs = Vec_PtrStart( Abc_NtkObjNumMax(p->pNtk) * (1 + fAnd) );
    Abc_NtkIncrementTravId( p->pNtk );
    if ( fAnd )
    {
        Abc_NtkForEachCo( p->pNtk, pObj, i )
            if ( Abc_ObjIsNode(Abc_ObjFanin0(pObj)) )
                Abc_NtkTraverseSupersAnd_rec( p, Abc_ObjFanin0(pObj), vInputs );
    }
    else
    {
        Abc_NtkForEachCo( p->pNtk, pObj, i )
            if ( Abc_ObjIsNode(Abc_ObjFanin0(pObj)) )
                Abc_NtkTraverseSupersXor_rec( p, Abc_ObjFanin0(pObj), vInputs );
    }
    p->nStartCols = Vec_IntSize(p->vObj2Lit);

    // find the largest number of 1s
    nOnesMax = 0;
    Vec_PtrForEachEntry( Vec_Int_t *, vInputs, vInput, i )
        if ( vInput )
            nOnesMax = Abc_MaxInt( nOnesMax, Vec_IntSize(vInput)-SHARE_NUM );

    // create buckets
    assert( p->vBuckets == NULL );
    p->vBuckets = Vec_PtrAlloc( nOnesMax + 1 );
    for ( i = 0; i <= nOnesMax; i++ )
        Vec_PtrPush( p->vBuckets, Vec_PtrAlloc(10) );

    // load vectors into buckets
    Vec_PtrForEachEntry( Vec_Int_t *, vInputs, vInput, i )
        if ( vInput )
            Vec_PtrPush( (Vec_Ptr_t *)Vec_PtrEntry(p->vBuckets, Vec_IntSize(vInput)-SHARE_NUM), vInput );
    Vec_PtrFree( vInputs );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkSharePrint( Abc_ShaMan_t * p )
{
    Vec_Ptr_t * vBucket;
    Vec_Int_t * vInput;
    int i, k, j, ObjId;
    char * pBuffer = ABC_ALLOC( char, Vec_IntSize(p->vObj2Lit) + 1 );
    int * pCounters = ABC_CALLOC( int, Vec_IntSize(p->vObj2Lit) + 1 );
    int nTotal = 0;
    Vec_PtrForEachEntry( Vec_Ptr_t *, p->vBuckets, vBucket, i )
    Vec_PtrForEachEntry( Vec_Int_t *, vBucket, vInput, j )
    {
        for ( k = 0; k < Vec_IntSize(p->vObj2Lit); k++ )
            pBuffer[k] = '0';
        pBuffer[k] = 0;

        Vec_IntForEachEntryStart( vInput, ObjId, k, SHARE_NUM )
        {
            assert( ObjId < Vec_IntSize(p->vObj2Lit) );
            pBuffer[ObjId] = '1';
            pCounters[ObjId]++;
        }
        printf( "%4d%3d: %s\n", Vec_IntEntry(vInput, 0), Vec_IntEntry(vInput, 1), pBuffer );
    }

    for ( i = 0; i < Vec_IntSize(p->vObj2Lit); i++ )
        if ( pCounters[i] > 0 )
            printf( "%d=%d ", i, pCounters[i] ); 
    printf( "\n" );

    nTotal = 0;
    for ( i = 0; i < p->nStartCols; i++ )
        nTotal += pCounters[i] - 1;
    printf( "Total = %d.  ", nTotal );
    printf( "Gates = %d.\n", Vec_IntSize(p->vObj2Lit) - p->nStartCols + nTotal );

    ABC_FREE( pCounters );
    ABC_FREE( pBuffer );

    printf( "Bucket contents: " );
    Vec_PtrForEachEntry( Vec_Ptr_t *, p->vBuckets, vBucket, i )
        printf( "%d ", Vec_PtrSize(vBucket) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDumpBlif( Abc_Ntk_t * p )
{
    FILE * pFile;
    Vec_Ptr_t * vSupp;
    Abc_Obj_t * pObj;
    int i, k;
    pFile = fopen( "multi_and.blif", "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open output file.\n" );
        return;
    }
    fprintf( pFile, ".model %s\n", "multi_and" );
    fprintf( pFile, ".inputs" );
    for ( i = 0; i < Abc_NtkCiNum(p); i++ )
        fprintf( pFile, " i%d", i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs" );
    for ( i = 0; i < Abc_NtkCoNum(p); i++ )
        fprintf( pFile, " o%d", i );
    fprintf( pFile, "\n" );
    Abc_NtkForEachCi( p, pObj, i )
        pObj->iTemp = i;
    for ( i = 0; i < Abc_NtkCoNum(p); i++ )
    {
        pObj = Abc_NtkCo( p, i );
        vSupp = Abc_NtkNodeSupport( p, &pObj, 1 );
        fprintf( pFile, ".names" );
        Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pObj, k )
            fprintf( pFile, " i%d", pObj->iTemp );
        fprintf( pFile, " o%d\n", i );
        Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pObj, k )
            fprintf( pFile, "1" );
        fprintf( pFile, " 1\n" );
        Vec_PtrFree( vSupp );
    }
    fprintf( pFile, ".end\n\n" );
    fclose( pFile );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkShareFindBestMatch( Vec_Ptr_t * vBuckets, Vec_Int_t ** pvInput, Vec_Int_t ** pvInput2 )
{
    int nPoolSize = 40;
    Vec_Ptr_t * vPool = Vec_PtrAlloc( nPoolSize );
    Vec_Ptr_t * vBucket;
    Vec_Int_t * vInput, * vInput2, * vInputBest = NULL, * vInputBest2 = NULL;
    int i, k, Cost, CostBest = 0, Delay, DelayBest = 0;

    Vec_PtrForEachEntryReverse( Vec_Ptr_t *, vBuckets, vBucket, i )
        Vec_PtrForEachEntry( Vec_Int_t *, vBucket, vInput, k )
        {
            Vec_PtrPush( vPool, vInput );
            if ( Vec_PtrSize(vPool) == nPoolSize )
                goto outside;
        }
outside:

    Vec_PtrForEachEntryReverse( Vec_Int_t *, vPool, vInput, i )
    Vec_PtrForEachEntryReverse( Vec_Int_t *, vPool, vInput2, k )
    {
        if ( i == k )
            continue;

        vInput->pArray += SHARE_NUM;
        vInput2->pArray += SHARE_NUM;
        vInput->nSize -= SHARE_NUM;
        vInput2->nSize -= SHARE_NUM;

        Cost = Vec_IntTwoCountCommon(vInput, vInput2);

        vInput->pArray -= SHARE_NUM;
        vInput2->pArray -= SHARE_NUM;
        vInput->nSize += SHARE_NUM;
        vInput2->nSize += SHARE_NUM;

        if ( Cost < 2 )
            continue;

        Delay = Abc_MaxInt( Vec_IntEntry(vInput, 1), Vec_IntEntry(vInput2, 1) );

        if ( CostBest < Cost || (CostBest == Cost && (DelayBest > Delay)) )
        {
            CostBest = Cost;
            DelayBest = Delay;
            vInputBest = vInput;
            vInputBest2 = vInput2;
        }
    }
    Vec_PtrFree( vPool );

    *pvInput  = vInputBest;
    *pvInput2 = vInputBest2;

    if ( vInputBest == NULL )
        return;

    Vec_PtrRemove( (Vec_Ptr_t *)Vec_PtrEntry(vBuckets, Vec_IntSize(vInputBest)-SHARE_NUM),  (Vec_Int_t *)vInputBest ); 
    Vec_PtrRemove( (Vec_Ptr_t *)Vec_PtrEntry(vBuckets, Vec_IntSize(vInputBest2)-SHARE_NUM), (Vec_Int_t *)vInputBest2 ); 
}
void Abc_NtkShareOptimize( Abc_ShaMan_t * p, int fAnd )
{
    Abc_Obj_t * pObj, * pObj0, * pObj1;
    Vec_Int_t * vInput, * vInput2;
    Vec_Int_t * vNew, * vOld1, * vOld2;
    int i;
    for ( i = 0; ; i++ )
    {
        Abc_NtkShareFindBestMatch( p->vBuckets, &vInput, &vInput2 );
        if ( vInput == NULL )
            break;

        // create new node
        pObj0 = Abc_ObjFromLit( p->pNtk, Vec_IntEntry(vInput,  0) );
        pObj1 = Abc_ObjFromLit( p->pNtk, Vec_IntEntry(vInput2, 0) );
        if ( fAnd )
            pObj  = Abc_AigAnd( (Abc_Aig_t *)p->pNtk->pManFunc, pObj0, pObj1 );
        else
            pObj  = Abc_AigXor( (Abc_Aig_t *)p->pNtk->pManFunc, pObj0, pObj1 );
        p->nCountGates++;

        // save new node
        vOld1 = Vec_IntAlloc( 16 );  Vec_IntPush( vOld1, Vec_IntEntry(vInput,  0) );  Vec_IntPush( vOld1, Vec_IntEntry(vInput,  1) );
        vOld2 = Vec_IntAlloc( 16 );  Vec_IntPush( vOld2, Vec_IntEntry(vInput2, 0) );  Vec_IntPush( vOld2, Vec_IntEntry(vInput2, 1) );
        vNew  = Vec_IntAlloc( 16 );  Vec_IntPush( vNew,  Abc_ObjToLit(pObj) );        Vec_IntPush( vNew, Abc_ObjLevel(Abc_ObjRegular(pObj)) );

        // compute new arrays
        vInput->pArray += SHARE_NUM;
        vInput2->pArray += SHARE_NUM;
        vInput->nSize -= SHARE_NUM;
        vInput2->nSize -= SHARE_NUM;

        Vec_IntTwoSplit( vInput, vInput2, vNew, vOld1, vOld2 );

        vInput->pArray -= SHARE_NUM;
        vInput2->pArray -= SHARE_NUM;
        vInput->nSize += SHARE_NUM;
        vInput2->nSize += SHARE_NUM;

        // add to the old ones
        Vec_IntPush( vOld1, Vec_IntSize(p->vObj2Lit) );
        Vec_IntPush( vOld2, Vec_IntSize(p->vObj2Lit) );
        Vec_IntPush( p->vObj2Lit, Abc_ObjToLit(pObj) );

        Vec_PtrPush( (Vec_Ptr_t *)Vec_PtrEntry(p->vBuckets, Vec_IntSize(vOld1)-SHARE_NUM), vOld1 );
        Vec_PtrPush( (Vec_Ptr_t *)Vec_PtrEntry(p->vBuckets, Vec_IntSize(vOld2)-SHARE_NUM), vOld2 );
        Vec_PtrPush( (Vec_Ptr_t *)Vec_PtrEntry(p->vBuckets, Vec_IntSize(vNew)-SHARE_NUM), vNew );

        Vec_IntFree( vInput );
        Vec_IntFree( vInput2 );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkUpdateNetwork( Abc_ShaMan_t * p, int fAnd )
{
    Abc_Ntk_t * pNtk;
    Vec_Int_t * vInput, * vMap2Repl;
    Vec_Ptr_t * vOrig, * vRepl, * vBucket;
    Abc_Obj_t * pObj, * pNew;
    int i, j, k, ObjId, iLit;
    int iLitConst1 = Abc_ObjToLit( Abc_AigConst1(p->pNtk) );

    vOrig = Vec_PtrAlloc( p->nStartCols );
    vRepl = Vec_PtrAlloc( p->nStartCols );
    for ( i = 0; i < p->nStartCols; i++ )
    {
        iLit = Vec_IntEntry( p->vObj2Lit, i );
        assert( !fAnd || !Abc_LitIsCompl(iLit) );

        pObj = Abc_NtkObj( p->pNtk, Abc_Lit2Var(iLit) );

        if ( fAnd )
            pNew = Abc_AigConst1(p->pNtk);
        else
            pNew = Abc_ObjNotCond( Abc_AigConst1(p->pNtk), !Abc_LitIsCompl(iLit) );

        Vec_PtrPush( vOrig, pObj );
        Vec_PtrPush( vRepl, pNew );

        p->nCountGates--;
    }

    // go through the columns
    Vec_PtrForEachEntry( Vec_Ptr_t *, p->vBuckets, vBucket, i )
    Vec_PtrForEachEntry( Vec_Int_t *, vBucket, vInput, j )
    {
        Vec_IntForEachEntryStart( vInput, ObjId, k, SHARE_NUM )
        {
            assert( ObjId < Vec_IntSize(p->vObj2Lit) );
            if ( ObjId >= p->nStartCols )
                break;
            assert( ObjId < p->nStartCols );
            iLit = Vec_IntEntry( vInput, 0 );

            pNew = (Abc_Obj_t *)Vec_PtrEntry( vRepl, ObjId );
            if ( fAnd )
                pNew = Abc_AigAnd( (Abc_Aig_t *)p->pNtk->pManFunc, pNew, Abc_ObjFromLit(p->pNtk, iLit) );
            else
                pNew = Abc_AigXor( (Abc_Aig_t *)p->pNtk->pManFunc, pNew, Abc_ObjFromLit(p->pNtk, iLit) );
            Vec_PtrWriteEntry( vRepl, ObjId, pNew );
            p->nCountGates++;
        }
    }

    if ( p->fVerbose )
        printf( "Total gates collected = %d.  Total gates constructed = %d.\n", p->nFoundGates, p->nCountGates );

    // create map of originals
    vMap2Repl = Vec_IntStartFull( Abc_NtkObjNumMax(p->pNtk) );
    Vec_PtrForEachEntry( Abc_Obj_t *, vOrig, pObj, i )
    {
//        printf( "Replacing %d by %d.\n", Abc_ObjId(pObj), Abc_ObjToLit((Abc_Obj_t *)Vec_PtrEntry(vRepl, i)) );
        Vec_IntWriteEntry( vMap2Repl, Abc_ObjId(pObj), Abc_ObjToLit((Abc_Obj_t *)Vec_PtrEntry(vRepl, i)) );
    }
    Vec_PtrFree( vOrig );
    Vec_PtrFree( vRepl );

    // update fanin pointers
    Abc_NtkForEachObj( p->pNtk, pObj, i )
    {
        if ( Abc_ObjIsCo(pObj) || Abc_ObjIsNode(pObj) )
        {
            iLit = Vec_IntEntry( vMap2Repl, Abc_ObjFaninId0(pObj) );
            if ( iLit >= 0 )
            {
                if ( iLit == iLitConst1 && fAnd )
                {
                    pObj->fCompl0 ^= 1;
                    Vec_IntWriteEntry( &pObj->vFanins, 0, Abc_Lit2Var(iLitConst1) );
                }
                else
                {
                    pObj->fCompl0 ^= Abc_LitIsCompl(iLit);
                    Vec_IntWriteEntry( &pObj->vFanins, 0, Abc_Lit2Var(iLit) );
                }
            }
        }
        if ( Abc_ObjIsNode(pObj) )
        {
            iLit = Vec_IntEntry( vMap2Repl, Abc_ObjFaninId1(pObj) );
            if ( iLit >= 0 )
            {
                if ( iLit == iLitConst1 && fAnd )
                {
                    pObj->fCompl1 ^= 1;
                    Vec_IntWriteEntry( &pObj->vFanins, 1, Abc_Lit2Var(iLitConst1) );
                }
                else
                {
                    pObj->fCompl1 ^= Abc_LitIsCompl(iLit);
                    Vec_IntWriteEntry( &pObj->vFanins, 1, Abc_Lit2Var(iLit) );
                }
            }
        }
    }
    Vec_IntFree( vMap2Repl );

//    pNtk = Abc_NtkRestrash( p->pNtk, 1 );
    if ( fAnd )
        pNtk = Abc_NtkBalance( p->pNtk, 0, 0, 1 );
    else
        pNtk = Abc_NtkBalanceExor( p->pNtk, 1, 0 );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Extracts one multi-output XOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkShareXor( Abc_Ntk_t * pNtk, int nMultiSize, int fAnd, int fVerbose )
{
    Abc_Ntk_t * pNtkNew;
    Abc_ShaMan_t * p;
    assert( Abc_NtkIsStrash(pNtk) );
//    Abc_NtkDumpBlif( pNtk );
    p = Abc_ShaManStart( pNtk );
    p->nMultiSize = nMultiSize;
    p->fVerbose   = fVerbose;
    Abc_NtkTraverseSupers( p, fAnd );
    if ( p->nStartCols < 2 )
    {
        Abc_ShaManStop( p );
        return Abc_NtkDup( pNtk );
    }
    if ( fVerbose )
        Abc_NtkSharePrint( p );
    Abc_NtkShareOptimize( p, fAnd );
    if ( fVerbose )
        Abc_NtkSharePrint( p );
    pNtkNew = Abc_NtkUpdateNetwork( p, fAnd );
    Abc_ShaManStop( p );
    return pNtkNew;
}
Gia_Man_t * Abc_NtkShareXorGia( Gia_Man_t * p, int nMultiSize, int fAnd, int fVerbose )
{
    extern Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
    extern Abc_Ntk_t * Abc_NtkFromAigPhase( Aig_Man_t * pMan );
    Aig_Man_t * pMan = Gia_ManToAig( p, 0 );
    Abc_Ntk_t * pNtk = Abc_NtkFromAigPhase( pMan );
    Abc_Ntk_t * pNtkNew = Abc_NtkShareXor( pNtk, nMultiSize, fAnd, fVerbose );
    Aig_Man_t * pAig = Abc_NtkToDar( pNtkNew, 0, 0 );
    Gia_Man_t * pNew = Gia_ManFromAig( pAig );
    Abc_NtkDelete( pNtkNew );
    Abc_NtkDelete( pNtk );
    Aig_ManStop( pAig );
    Aig_ManStop( pMan );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

