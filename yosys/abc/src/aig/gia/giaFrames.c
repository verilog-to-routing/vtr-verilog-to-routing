/**CFile****************************************************************

  FileName    [giaFrames.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Timeframe unrolling.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaFrames.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Gia_ManFra_t_ Gia_ManFra_t;
struct Gia_ManFra_t_
{
    Gia_ParFra_t *  pPars;    // parameters
    Gia_Man_t *     pAig;     // AIG to unroll
    Vec_Ptr_t *     vIns;     // inputs of each timeframe    
    Vec_Ptr_t *     vAnds;    // nodes of each timeframe    
    Vec_Ptr_t *     vOuts;    // outputs of each timeframe    
};


typedef struct Gia_ManUnr_t_ Gia_ManUnr_t;
struct Gia_ManUnr_t_
{
    Gia_ParFra_t *  pPars;    // parameters
    Gia_Man_t *     pAig;     // AIG to unroll (points to pOrder)
    // internal data 
    Gia_Man_t *     pOrder;   // AIG reordered (points to pAig)
    Vec_Int_t *     vLimit;   // limits of each timeframe
    // data for each ordered node
    Vec_Int_t *     vRank;    // rank of each node
    Vec_Int_t *     vDegree;  // degree of each node
    Vec_Int_t *     vDegDiff; // degree of each node
    Vec_Int_t *     vFirst;   // first entry in the store
    Vec_Int_t *     vStore;   // store for saved data
    // the resulting AIG
    Gia_Man_t *     pNew;     // the resulting AIG
    int             LastLit;  // the place to store the last literal
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Duplicates AIG for unrolling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManUnrollDup_rec( Gia_Man_t * pNew, Gia_Obj_t * pObj, int Id ) 
{
    if ( ~pObj->Value )
        return;
    if ( Gia_ObjIsCi(pObj) )
        pObj->Value = Gia_ManAppendCi(pNew);
    else if ( Gia_ObjIsCo(pObj) )
    {
        Gia_ManUnrollDup_rec( pNew, Gia_ObjFanin0(pObj), Gia_ObjFaninId0(pObj, Id) );
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    else if ( Gia_ObjIsAnd(pObj) )
    {
        Gia_ManUnrollDup_rec( pNew, Gia_ObjFanin0(pObj), Gia_ObjFaninId0(pObj, Id) );
        Gia_ManUnrollDup_rec( pNew, Gia_ObjFanin1(pObj), Gia_ObjFaninId1(pObj, Id) );
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    }
    else assert( 0 );
    Gia_ManObj(pNew, Abc_Lit2Var(pObj->Value))->Value = Id;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG for unrolling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManUnrollDup( Gia_Man_t * p, Vec_Int_t * vLimit )  
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    assert( Vec_IntSize(vLimit) == 0 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );

    // save constant class
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Vec_IntPush( vLimit, Gia_ManObjNum(pNew) );
 
    // create first class
    Gia_ManForEachPo( p, pObj, i )
        Gia_ManUnrollDup_rec( pNew, pObj, Gia_ObjId(p, pObj) );
    Vec_IntPush( vLimit, Gia_ManObjNum(pNew) );

    // create next classes
    for ( i = 1; i < Gia_ManObjNum(pNew); i++ )
    {
        if ( i == Vec_IntEntryLast(vLimit) )
            Vec_IntPush( vLimit, Gia_ManObjNum(pNew) );
        pObj = Gia_ManObj( p, Gia_ManObj(pNew, i)->Value );
        if ( Gia_ObjIsRo(p, pObj) )
        {
            pObj = Gia_ObjRoToRi(p, pObj);
            assert( !~pObj->Value );
            Gia_ManUnrollDup_rec( pNew, pObj, Gia_ObjId(p, pObj) );
        }
    }
    Gia_ManSetRegNum( pNew, 0 );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG for unrolling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Gia_ManUnrollAbs( Gia_Man_t * p, int nFrames )  
{
    int fVerbose = 0;
    Vec_Ptr_t * vFrames;
    Vec_Int_t * vLimit, * vOne;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int nObjBits, nObjMask;
    int f, fMax, k, Entry, Prev, iStart, iStop, Size;
    // get the bitmasks
    nObjBits = Abc_Base2Log( Gia_ManObjNum(p) );
    nObjMask = (1 << nObjBits) - 1;
    assert( Gia_ManObjNum(p) <= nObjMask );
    // derive the tents
    vLimit = Vec_IntAlloc( 1000 );
    pNew = Gia_ManUnrollDup( p, vLimit );
    // debug printout
    if ( fVerbose )
    {
        Prev = 1;
        printf( "Tents: " );
        Vec_IntForEachEntryStart( vLimit, Entry, k, 1 )
            printf( "%d=%d ", k, Entry-Prev ), Prev = Entry;
        printf( "  Unused=%d", Gia_ManObjNum(p) - Gia_ManObjNum(pNew) );
        printf( "\n" );
    }
    // create abstraction
    vFrames = Vec_PtrAlloc( Vec_IntSize(vLimit) );
    for ( fMax = 0; fMax < nFrames; fMax++ )
    {
        Size = (fMax+1 < Vec_IntSize(vLimit)) ? Vec_IntEntry(vLimit, fMax+1) : Gia_ManObjNum(pNew);
        vOne = Vec_IntAlloc( Size );
        for ( f = 0; f <= fMax; f++ )
        {
            iStart = (f   < Vec_IntSize(vLimit)) ? Vec_IntEntry(vLimit, f  ) : 0;
            iStop  = (f+1 < Vec_IntSize(vLimit)) ? Vec_IntEntry(vLimit, f+1) : 0;
            for ( k = iStop - 1; k >= iStart; k-- )
            {
                assert( Gia_ManObj(pNew, k)->Value > 0 );
                pObj  = Gia_ManObj(p, Gia_ManObj(pNew, k)->Value);
                if ( Gia_ObjIsCo(pObj) || Gia_ObjIsPi(p, pObj) )
                    continue;
                assert( Gia_ObjIsRo(p, pObj) || Gia_ObjIsAnd(pObj) );
                Entry = ((fMax-f) << nObjBits) | Gia_ObjId(p, pObj);
                Vec_IntPush( vOne, Entry );
//                printf( "%d ", Gia_ManObj(pNew, k)->Value );
            }
//            printf( "\n" );
        }
        // add in reverse topological order
        Vec_IntSort( vOne, 1 );
        Vec_PtrPush( vFrames, vOne );
        assert( Vec_IntSize(vOne) <= Size - 1 );
    }
    Vec_IntFree( vLimit );
    Gia_ManStop( pNew );
    return vFrames;
}

/**Function*************************************************************

  Synopsis    [Creates manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_ManUnr_t * Gia_ManUnrStart( Gia_Man_t * pAig, Gia_ParFra_t * pPars )  
{ 
    Gia_ManUnr_t * p;
    Gia_Obj_t * pObj;
    int i, k, iRank, iFanin, Degree, Shift;
    abctime clk = Abc_Clock();

    p = ABC_CALLOC( Gia_ManUnr_t, 1 );
    p->pAig   = pAig;
    p->pPars  = pPars;
    // create order
    p->vLimit = Vec_IntAlloc( 0 );
    p->pOrder = Gia_ManUnrollDup( pAig, p->vLimit );
/*
    Vec_IntForEachEntryStart( p->vLimit, Shift, i, 1 )
        printf( "%d=%d ", i, Shift-Vec_IntEntry(p->vLimit, i-1) );
    printf( "\n" );
*/
    // assign rank
    p->vRank  = Vec_IntAlloc( Gia_ManObjNum(p->pOrder) );
    for ( iRank = i = 0; i < Gia_ManObjNum(p->pOrder); Vec_IntPush(p->vRank, iRank), i++ )
        if ( Vec_IntEntry(p->vLimit, iRank) == i )
            iRank++;
    assert( iRank == Vec_IntSize(p->vLimit)-1 );
    assert( Vec_IntSize(p->vRank) == Gia_ManObjNum(p->pOrder) );
    // assign degree
    p->vDegree = Vec_IntStart( Gia_ManObjNum(p->pOrder) );
    p->vDegDiff= Vec_IntStart( 2* Gia_ManObjNum(p->pOrder) );
    Gia_ManForEachAnd( p->pOrder, pObj, i )
    {
        for ( k = 0; k < 2; k++ )
        {
            iFanin = k ? Gia_ObjFaninId1(pObj, i) : Gia_ObjFaninId0(pObj, i);
            Degree = Vec_IntEntry(p->vRank, i) - Vec_IntEntry(p->vRank, iFanin);
            Vec_IntWriteEntry( p->vDegDiff, 2*i + k, Degree );
            if ( Vec_IntEntry(p->vDegree, iFanin) < Degree )
                Vec_IntWriteEntry( p->vDegree, iFanin, Degree );
        }
    }
    Gia_ManForEachCo( p->pOrder, pObj, k )
    {
        i      = Gia_ObjId( p->pOrder, pObj );
        iFanin = Gia_ObjFaninId0(pObj, i);
        Degree = Vec_IntEntry(p->vRank, i) - Vec_IntEntry(p->vRank, iFanin);
        Vec_IntWriteEntry( p->vDegDiff, 2*i, Degree );
        if ( Vec_IntEntry(p->vDegree, iFanin) < Degree )
            Vec_IntWriteEntry( p->vDegree, iFanin, Degree );
    }
    // assign first
    p->vFirst = Vec_IntAlloc( Gia_ManObjNum(p->pOrder) );
    p->vStore = Vec_IntStartFull( 2* Gia_ManObjNum(p->pOrder) + Vec_IntSum(p->vDegree) );
    for ( Shift = i = 0; i < Gia_ManObjNum(p->pOrder); i++ )
    {
        Vec_IntPush( p->vFirst, Shift );
        Vec_IntWriteEntry( p->vStore, Shift, 1 + Vec_IntEntry(p->vDegree, i) ); 
        Shift += 2 + Vec_IntEntry(p->vDegree, i);
    }
    assert( Shift == Vec_IntSize(p->vStore) );
    // cleanup
    Vec_IntFreeP( &p->vRank );
    Vec_IntFreeP( &p->vDegree );

    // print verbose output
    if ( pPars->fVerbose )
    {
        printf( "Convergence = %d.  Dangling objects = %d.  Average degree = %.3f   ", 
            Vec_IntSize(p->vLimit) - 1,
            Gia_ManObjNum(pAig) - Gia_ManObjNum(p->pOrder),
            1.0*Vec_IntSize(p->vStore)/Gia_ManObjNum(p->pOrder) - 1.0  );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManUnrollStop( void * pMan )  
{
    Gia_ManUnr_t * p = (Gia_ManUnr_t *)pMan;
    Gia_ManStopP( &p->pOrder );
    Vec_IntFreeP( &p->vLimit );
    Vec_IntFreeP( &p->vRank );
    Vec_IntFreeP( &p->vDegree );
    Vec_IntFreeP( &p->vDegDiff );
    Vec_IntFreeP( &p->vFirst );
    Vec_IntFreeP( &p->vStore );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Reading/writing entry from storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ObjUnrWrite( Gia_ManUnr_t * p, int Id, int Entry )  
{
    int i, * pArray = Vec_IntEntryP( p->vStore, Vec_IntEntry(p->vFirst, Id) );
    for ( i = pArray[0]; i > 1; i-- )
        pArray[i] = pArray[i-1];
    pArray[1] = Entry;
}
static inline int Gia_ObjUnrRead( Gia_ManUnr_t * p, int Id, int Degree )  
{
    int * pArray = Vec_IntEntryP( p->vStore, Vec_IntEntry(p->vFirst, Id) );
    if ( Id == 0 )
        return 0;
    assert( Degree >= 0 && Degree < pArray[0] );
    if ( Degree )
        Degree--;
    return pArray[Degree + 1];
}
static inline int Gia_ObjUnrReadCopy0( Gia_ManUnr_t * p, Gia_Obj_t * pObj, int Id )  
{
    int Lit = Gia_ObjUnrRead(p, Gia_ObjFaninId0(pObj, Id), Vec_IntEntry(p->vDegDiff, 2*Id));
    return Abc_LitNotCond( Lit, Gia_ObjFaninC0(pObj) );
}
static inline int Gia_ObjUnrReadCopy1( Gia_ManUnr_t * p, Gia_Obj_t * pObj, int Id )  
{
    int Lit = Gia_ObjUnrRead(p, Gia_ObjFaninId1(pObj, Id), Vec_IntEntry(p->vDegDiff, 2*Id+1));
    return Abc_LitNotCond( Lit, Gia_ObjFaninC1(pObj) );
}
static inline int Gia_ObjUnrReadCi( Gia_ManUnr_t * p, int Id, int f, Gia_Man_t * pNew )  
{
    Gia_Obj_t * pObj = Gia_ManObj( p->pOrder, Id );
    Gia_Obj_t * pObjReal = Gia_ManObj( p->pAig, pObj->Value );
    assert( Gia_ObjIsCi(pObjReal) );
    if ( Gia_ObjIsPi(p->pAig, pObjReal) )
    {
        if ( !p->pPars->fSaveLastLit ) 
            pObj = Gia_ManPi( pNew, Gia_ManPiNum(p->pAig) * f + Gia_ObjCioId(pObjReal) );
        else
            pObj = Gia_ManPi( pNew, Gia_ManRegNum(p->pAig) + Gia_ManPiNum(p->pAig) * f + Gia_ObjCioId(pObjReal) );
        return Abc_Var2Lit( Gia_ObjId(pNew, pObj), 0 );
    }
    if ( f == 0 ) // initialize!
    {
        if ( p->pPars->fInit )
            return 0;
        assert( Gia_ObjCioId(pObjReal) >= Gia_ManPiNum(p->pAig) );
        if ( !p->pPars->fSaveLastLit ) 
            pObj = Gia_ManPi( pNew, Gia_ManPiNum(p->pAig) * p->pPars->nFrames + Gia_ObjCioId(pObjReal)-Gia_ManPiNum(p->pAig) );
        else
            pObj = Gia_ManPi( pNew, Gia_ObjCioId(pObjReal)-Gia_ManPiNum(p->pAig) );
        return Abc_Var2Lit( Gia_ObjId(pNew, pObj), 0 );
    }
    pObj = Gia_ManObj( p->pOrder, Abc_Lit2Var(Gia_ObjRoToRi(p->pAig, pObjReal)->Value) );
    assert( Gia_ObjIsCo(pObj) );
    return Gia_ObjUnrRead( p, Gia_ObjId(p->pOrder, pObj), 0 );
}

/**Function*************************************************************

  Synopsis    [Computes init/non-init unrolling without flops.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Gia_ManUnrollStart( Gia_Man_t * pAig, Gia_ParFra_t * pPars )
{
    Gia_ManUnr_t * p;
    int f, i;
    // start 
    p = Gia_ManUnrStart( pAig, pPars );
    // start timeframes
    assert( p->pNew == NULL );
    p->pNew = Gia_ManStart( 10000 );
    p->pNew->pName = Abc_UtilStrsav( p->pAig->pName );
    p->pNew->pSpec = Abc_UtilStrsav( p->pAig->pSpec );
    Gia_ManHashAlloc( p->pNew );
    // create combinational inputs
    if ( !p->pPars->fSaveLastLit ) // only in the case when unrolling depth is known
        for ( f = 0; f < p->pPars->nFrames; f++ )
            for ( i = 0; i < Gia_ManPiNum(p->pAig); i++ )
                Gia_ManAppendCi(p->pNew);
    // create flop outputs
    if ( !p->pPars->fInit ) // only in the case when initialization is not performed
        for ( i = 0; i < Gia_ManRegNum(p->pAig); i++ )
            Gia_ManAppendCi(p->pNew);
    return p;
}

/**Function*************************************************************

  Synopsis    [Computes init/non-init unrolling without flops.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Gia_ManUnrollAdd( void * pMan, int fMax )
{
    Gia_ManUnr_t * p = (Gia_ManUnr_t *)pMan;
    Gia_Obj_t * pObj;
    int f, i, Lit = 0, Beg, End;
    // create PIs on demand
    if ( p->pPars->fSaveLastLit ) 
        for ( i = 0; i < Gia_ManPiNum(p->pAig); i++ )
            Gia_ManAppendCi(p->pNew);
    // unroll another timeframe
    for ( f = 0; f < fMax; f++ )
    {
        if ( Vec_IntSize(p->vLimit) <= fMax-f )
            continue;
        Beg = Vec_IntEntry( p->vLimit, fMax-f-1 );
        End = Vec_IntEntry( p->vLimit, fMax-f );
        for ( i = Beg; i < End; i++ )
        {
            pObj = Gia_ManObj( p->pOrder, i );
            if ( Gia_ObjIsAnd(pObj) )
                Lit = Gia_ManHashAnd( p->pNew, Gia_ObjUnrReadCopy0(p, pObj, i), Gia_ObjUnrReadCopy1(p, pObj, i) );
            else if ( Gia_ObjIsCo(pObj) )
            {
                Lit = Gia_ObjUnrReadCopy0(p, pObj, i);
                if ( f == fMax-1 )
                {
                    if ( p->pPars->fSaveLastLit )
                        p->LastLit = Lit;
                    else
                        Gia_ManAppendCo( p->pNew, Lit );
                }
            }
            else if ( Gia_ObjIsCi(pObj) )
                Lit = Gia_ObjUnrReadCi( p, i, f, p->pNew );
            else assert( 0 );
            assert( Lit >= 0 );
            Gia_ObjUnrWrite( p, i, Lit ); // should be exactly one call for each obj!
        }
    }
    return p->pNew;
}

/**Function*************************************************************

  Synopsis    [Read the last literal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManUnrollLastLit( void * pMan )
{
    Gia_ManUnr_t * p = (Gia_ManUnr_t *)pMan;
    return p->LastLit;
}

/**Function*************************************************************

  Synopsis    [Computes init/non-init unrolling without flops.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManUnroll( Gia_Man_t * pAig, Gia_ParFra_t * pPars )
{
    Gia_ManUnr_t * p;
    Gia_Man_t * pNew, * pTemp;
    int fMax;
    p = (Gia_ManUnr_t *)Gia_ManUnrollStart( pAig, pPars );
    for ( fMax = 1; fMax <= p->pPars->nFrames; fMax++ )
        Gia_ManUnrollAdd( p, fMax );
    assert( Gia_ManPoNum(p->pNew) == p->pPars->nFrames * Gia_ManPoNum(p->pAig) );
    Gia_ManHashStop( p->pNew );
    Gia_ManSetRegNum( p->pNew, 0 );
//    Gia_ManPrintStats( pNew, 0 );
    // cleanup
    p->pNew = Gia_ManCleanup( pTemp = p->pNew );
    Gia_ManStop( pTemp );
//    Gia_ManPrintStats( pNew, 0 );
    pNew = p->pNew;  p->pNew = NULL;
    Gia_ManUnrollStop( p );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Computes init/non-init unrolling without flops.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
Gia_Man_t * Gia_ManUnroll( Gia_ManUnr_t * p )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int fMax, f, i, Lit, Beg, End;
    // start timeframes
    pNew = Gia_ManStart( 10000 );
    pNew->pName = Abc_UtilStrsav( p->pAig->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pAig->pSpec );
    Gia_ManHashAlloc( pNew );
    // create combinational inputs
    for ( f = 0; f < p->pPars->nFrames; f++ )
        for ( i = 0; i < Gia_ManPiNum(p->pAig); i++ )
            Gia_ManAppendCi(pNew);
    if ( !p->pPars->fInit )
        for ( i = 0; i < Gia_ManRegNum(p->pAig); i++ )
            Gia_ManAppendCi(pNew);
    // add nodes to each time-frame
//    Gia_ObjUnrWrite( p, 0, 0 );
    for ( fMax = 1; fMax <= p->pPars->nFrames; fMax++ )
        for ( f = 0; f < fMax; f++ )
        {
            if ( Vec_IntSize(p->vLimit) <= fMax-f )
                continue;
            Beg = Vec_IntEntry( p->vLimit, fMax-f-1 );
            End = Vec_IntEntry( p->vLimit, fMax-f );
            for ( i = Beg; i < End; i++ )
            {
                pObj = Gia_ManObj( p->pOrder, i );
                if ( Gia_ObjIsAnd(pObj) )
                    Lit = Gia_ManHashAnd( pNew, Gia_ObjUnrReadCopy0(p, pObj, i), Gia_ObjUnrReadCopy1(p, pObj, i) );
                else if ( Gia_ObjIsCo(pObj) )
                {
                    Lit = Gia_ObjUnrReadCopy0(p, pObj, i);
                    if ( f == fMax-1 )
                        Gia_ManAppendCo( pNew, Lit );
                }
                else if ( Gia_ObjIsCi(pObj) )
                    Lit = Gia_ObjUnrReadCi( p, i, f, pNew );
                else assert( 0 );
                assert( Lit >= 0 );
                Gia_ObjUnrWrite( p, i, Lit ); // should be exactly one call for each obj!
            }
        }
    assert( Gia_ManPoNum(pNew) == p->pPars->nFrames * Gia_ManPoNum(p->pAig) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, 0 );
//    Gia_ManPrintStats( pNew, 0 );
    // cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
//    Gia_ManPrintStats( pNew, 0 );
    return pNew;
} 
*/

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFrames2( Gia_Man_t * pAig, Gia_ParFra_t * pPars )
{
    Gia_Man_t * pNew;
    abctime clk = Abc_Clock();
    pNew = Gia_ManUnroll( pAig, pPars );
    if ( pPars->fVerbose )
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return pNew;
}



/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManFraSetDefaultParams( Gia_ParFra_t * p )
{
    memset( p, 0, sizeof(Gia_ParFra_t) );
    p->nFrames      =  32;    // the number of frames to unroll
    p->fInit        =   0;    // initialize the timeframes
    p->fVerbose     =   0;    // enables verbose output
}

/**Function*************************************************************

  Synopsis    [Creates manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_ManFra_t * Gia_ManFraStart( Gia_Man_t * pAig, Gia_ParFra_t * pPars )  
{ 
    Gia_ManFra_t * p;
    p = ABC_ALLOC( Gia_ManFra_t, 1 );
    memset( p, 0, sizeof(Gia_ManFra_t) );
    p->pAig  = pAig;
    p->pPars = pPars;
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManFraStop( Gia_ManFra_t * p )  
{
    Vec_VecFree( (Vec_Vec_t *)p->vIns );
    Vec_VecFree( (Vec_Vec_t *)p->vAnds );
    Vec_VecFree( (Vec_Vec_t *)p->vOuts );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Computes supports of all timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManFraSupports( Gia_ManFra_t * p )
{
    Vec_Int_t * vIns = NULL, * vAnds, * vOuts;
    Gia_Obj_t * pObj;
    int f, i;
    p->vIns  = Vec_PtrStart( p->pPars->nFrames );
    p->vAnds = Vec_PtrStart( p->pPars->nFrames );
    p->vOuts = Vec_PtrStart( p->pPars->nFrames );
    Gia_ManIncrementTravId( p->pAig );
    for ( f = p->pPars->nFrames - 1; f >= 0; f-- )
    {
        vOuts = Gia_ManCollectPoIds( p->pAig );
        if ( vIns )
        Gia_ManForEachObjVec( vIns, p->pAig, pObj, i )
            if ( Gia_ObjIsRo(p->pAig, pObj) )
                Vec_IntPush( vOuts, Gia_ObjId( p->pAig, Gia_ObjRoToRi(p->pAig, pObj) ) );
        vIns = Vec_IntAlloc( 100 );
        Gia_ManCollectCis( p->pAig, Vec_IntArray(vOuts), Vec_IntSize(vOuts), vIns );
        vAnds = Vec_IntAlloc( 100 );
        Gia_ManCollectAnds( p->pAig, Vec_IntArray(vOuts), Vec_IntSize(vOuts), vAnds, NULL );
        Vec_PtrWriteEntry( p->vIns,  f, vIns );
        Vec_PtrWriteEntry( p->vAnds, f, vAnds );
        Vec_PtrWriteEntry( p->vOuts, f, vOuts );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFramesInit( Gia_Man_t * pAig, Gia_ParFra_t * pPars )
{
    int fUseAllPis = 1;
    Gia_Man_t * pFrames, * pTemp;
    Gia_ManFra_t * p;
    Gia_Obj_t * pObj;
    Vec_Int_t * vIns, * vAnds, * vOuts;
    int i, f;
    p = Gia_ManFraStart( pAig, pPars );
    Gia_ManFraSupports( p );
    pFrames = Gia_ManStart( Vec_VecSizeSize((Vec_Vec_t*)p->vIns)+
        Vec_VecSizeSize((Vec_Vec_t*)p->vAnds)+Vec_VecSizeSize((Vec_Vec_t*)p->vOuts) );
    pFrames->pName = Abc_UtilStrsav( pAig->pName );
    pFrames->pSpec = Abc_UtilStrsav( pAig->pSpec );
    Gia_ManHashAlloc( pFrames );
    Gia_ManConst0(pAig)->Value = 0;
    for ( f = 0; f < pPars->nFrames; f++ )
    {
        vIns  = (Vec_Int_t *)Vec_PtrEntry( p->vIns,  f );
        vAnds = (Vec_Int_t *)Vec_PtrEntry( p->vAnds, f );
        vOuts = (Vec_Int_t *)Vec_PtrEntry( p->vOuts, f );
        if ( pPars->fVerbose )
            printf( "Frame %3d : CI = %6d. AND = %6d. CO = %6d.\n", 
            f, Vec_IntSize(vIns), Vec_IntSize(vAnds), Vec_IntSize(vOuts) );
        if ( fUseAllPis )
        {
            Gia_ManForEachPi( pAig, pObj, i )
                pObj->Value = Gia_ManAppendCi( pFrames );
            if ( f == 0 )
            {
                Gia_ManForEachObjVec( vIns, pAig, pObj, i )
                {
                    assert( Gia_ObjIsCi(pObj) );
                    if ( !Gia_ObjIsPi(pAig, pObj) )
                        pObj->Value = 0;
                }
            }
            else
            {
                Gia_ManForEachObjVec( vIns, pAig, pObj, i )
                {
                    assert( Gia_ObjIsCi(pObj) );
                    if ( !Gia_ObjIsPi(pAig, pObj) )
                        pObj->Value = Gia_ObjRoToRi(pAig, pObj)->Value;
                }
            }
        }
        else
        {
            if ( f == 0 )
            {
                Gia_ManForEachObjVec( vIns, pAig, pObj, i )
                {
                    assert( Gia_ObjIsCi(pObj) );
                    if ( Gia_ObjIsPi(pAig, pObj) )
                        pObj->Value = Gia_ManAppendCi( pFrames );
                    else
                        pObj->Value = 0;
                }
            }
            else
            {
                Gia_ManForEachObjVec( vIns, pAig, pObj, i )
                {
                    assert( Gia_ObjIsCi(pObj) );
                    if ( Gia_ObjIsPi(pAig, pObj) )
                        pObj->Value = Gia_ManAppendCi( pFrames );
                    else
                        pObj->Value = Gia_ObjRoToRi(pAig, pObj)->Value;
                }
            }
        }
        Gia_ManForEachObjVec( vAnds, pAig, pObj, i )
        {
            assert( Gia_ObjIsAnd(pObj) );
            pObj->Value = Gia_ManHashAnd( pFrames, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        }
        Gia_ManForEachObjVec( vOuts, pAig, pObj, i )
        {
            assert( Gia_ObjIsCo(pObj) );
            if ( Gia_ObjIsPo(pAig, pObj) )
                pObj->Value = Gia_ManAppendCo( pFrames, Gia_ObjFanin0Copy(pObj) );
            else
                pObj->Value = Gia_ObjFanin0Copy(pObj);
        }
    }
    Gia_ManFraStop( p );
    Gia_ManHashStop( pFrames );
    if ( Gia_ManCombMarkUsed(pFrames) < Gia_ManAndNum(pFrames) )
    {
        pFrames = Gia_ManDupMarked( pTemp = pFrames );
        if ( pPars->fVerbose )
            printf( "Before cleanup = %d nodes. After cleanup = %d nodes.\n", 
                Gia_ManAndNum(pTemp), Gia_ManAndNum(pFrames) );
        Gia_ManStop( pTemp );
    }
    else if ( pPars->fVerbose )
            printf( "Before cleanup = %d nodes. After cleanup = %d nodes.\n", 
                Gia_ManAndNum(pFrames), Gia_ManAndNum(pFrames) );
    return pFrames;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFrames( Gia_Man_t * pAig, Gia_ParFra_t * pPars )
{
    Gia_Man_t * pFrames, * pTemp;
    Gia_Obj_t * pObj;
    Vec_Int_t * vPoLits = NULL;
    int i, f;
    assert( Gia_ManRegNum(pAig) > 0 );
    assert( pPars->nFrames > 0 );
    if ( pPars->fInit )
        return Gia_ManFramesInit( pAig, pPars );
    if ( pPars->fOrPos )
        vPoLits = Vec_IntStart( Gia_ManPoNum(pAig) );
    pFrames = Gia_ManStart( pPars->nFrames * Gia_ManObjNum(pAig) );
    pFrames->pName = Abc_UtilStrsav( pAig->pName );
    pFrames->pSpec = Abc_UtilStrsav( pAig->pSpec );
    if ( !pPars->fDisableSt )
        Gia_ManHashAlloc( pFrames );
    Gia_ManConst0(pAig)->Value = 0;
    // create primary inputs
    for ( f = 0; f < pPars->nFrames; f++ )
        Gia_ManForEachPi( pAig, pObj, i )
            pObj->Value = Gia_ManAppendCi( pFrames );
    // add internal nodes for each timeframe
    for ( f = 0; f < pPars->nFrames; f++ )
    {
        if ( f == 0 )
        {
            Gia_ManForEachRo( pAig, pObj, i )
                pObj->Value = Gia_ManAppendCi( pFrames );
        }
        else
        {
            Gia_ManForEachRo( pAig, pObj, i )
                pObj->Value = Gia_ObjRoToRi( pAig, pObj )->Value;
        }
        Gia_ManForEachPi( pAig, pObj, i )
            pObj->Value = Gia_Obj2Lit( pFrames, Gia_ManPi(pFrames, f * Gia_ManPiNum(pAig) + i) );
        if ( !pPars->fDisableSt )
            Gia_ManForEachAnd( pAig, pObj, i )
                pObj->Value = Gia_ManHashAnd( pFrames, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else
            Gia_ManForEachAnd( pAig, pObj, i )
                pObj->Value = Gia_ManAppendAnd2( pFrames, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        if ( vPoLits )
        {
            if ( !pPars->fDisableSt )
                Gia_ManForEachPo( pAig, pObj, i )
                    Vec_IntWriteEntry( vPoLits, i, Gia_ManHashOr(pFrames, Vec_IntEntry(vPoLits, i), Gia_ObjFanin0Copy(pObj)) );
            else
                Gia_ManForEachPo( pAig, pObj, i )
                    Vec_IntWriteEntry( vPoLits, i, Abc_LitNot(Gia_ManAppendAnd2(pFrames, Abc_LitNot(Vec_IntEntry(vPoLits, i)), Abc_LitNot(Gia_ObjFanin0Copy(pObj)))) );
        }
        else
        {
            Gia_ManForEachPo( pAig, pObj, i )
                pObj->Value = Gia_ManAppendCo( pFrames, Gia_ObjFanin0Copy(pObj) );
        }
        if ( f == pPars->nFrames - 1 )
        {
            if ( vPoLits )
                Gia_ManForEachPo( pAig, pObj, i )
                    pObj->Value = Gia_ManAppendCo( pFrames, Vec_IntEntry(vPoLits, i) );
            Gia_ManForEachRi( pAig, pObj, i )
                pObj->Value = Gia_ManAppendCo( pFrames, Gia_ObjFanin0Copy(pObj) );
        }
        else
        {
            Gia_ManForEachRi( pAig, pObj, i )
                pObj->Value = Gia_ObjFanin0Copy(pObj);
        }
    }
    Vec_IntFreeP( &vPoLits );
    if ( !pPars->fDisableSt )
        Gia_ManHashStop( pFrames );
    Gia_ManSetRegNum( pFrames, Gia_ManRegNum(pAig) );
    if ( Gia_ManCombMarkUsed(pFrames) < Gia_ManAndNum(pFrames) )
    {
        pFrames = Gia_ManDupMarked( pTemp = pFrames );
        if ( pPars->fVerbose )
            printf( "Before cleanup = %d nodes. After cleanup = %d nodes.\n", 
                Gia_ManAndNum(pTemp), Gia_ManAndNum(pFrames) );
        Gia_ManStop( pTemp );
    }
    else if ( pPars->fVerbose )
            printf( "Before cleanup = %d nodes. After cleanup = %d nodes.\n", 
                Gia_ManAndNum(pFrames), Gia_ManAndNum(pFrames) );
    return pFrames;
}


/**Function*************************************************************

  Synopsis    [Perform init unrolling as long as PO(s) are constant 0.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFramesInitSpecial( Gia_Man_t * pAig, int nFrames, int fVerbose )
{
    Gia_Man_t * pFrames, * pTemp;
    Gia_Obj_t * pObj;
    int i, f;
    assert( Gia_ManRegNum(pAig) > 0 );
    if ( nFrames > 0 )
        printf( "Computing specialized unrolling with %d frames...\n", nFrames );
    pFrames = Gia_ManStart( Gia_ManObjNum(pAig) );
    pFrames->pName = Abc_UtilStrsav( pAig->pName );
    pFrames->pSpec = Abc_UtilStrsav( pAig->pSpec );
    Gia_ManHashAlloc( pFrames );
    Gia_ManConst0(pAig)->Value = 0;
    for ( f = 0; nFrames == 0 || f < nFrames; f++ )
    {
        if ( fVerbose && (f % 100 == 0) )
        {
            printf( "%6d : ", f );
            Gia_ManPrintStats( pFrames, NULL );
        }
        Gia_ManForEachRo( pAig, pObj, i )
            pObj->Value = f ? Gia_ObjRoToRi( pAig, pObj )->Value : 0;
        Gia_ManForEachPi( pAig, pObj, i )
            pObj->Value = Gia_ManAppendCi( pFrames );
        Gia_ManForEachAnd( pAig, pObj, i )
            pObj->Value = Gia_ManHashAnd( pFrames, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachPo( pAig, pObj, i )
            if ( Gia_ObjFanin0Copy(pObj) != 0 )
                break;
        if ( i < Gia_ManPoNum(pAig) )
            break;
        Gia_ManForEachRi( pAig, pObj, i )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
    }
    if ( fVerbose )
        printf( "Computed prefix of %d frames.\n", f );
    Gia_ManForEachRi( pAig, pObj, i )
        Gia_ManAppendCo( pFrames, pObj->Value );
    Gia_ManHashStop( pFrames );
    pFrames = Gia_ManCleanup( pTemp = pFrames );
    if ( fVerbose )
        printf( "Before cleanup = %d nodes. After cleanup = %d nodes.\n", 
            Gia_ManAndNum(pTemp), Gia_ManAndNum(pFrames) );
    Gia_ManStop( pTemp );
    return pFrames;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

