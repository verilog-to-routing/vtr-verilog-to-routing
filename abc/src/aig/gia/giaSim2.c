/**CFile****************************************************************

  FileName    [giaSim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Fast sequential simulator.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaSim.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Gia_Sim2_t_ Gia_Sim2_t;
struct Gia_Sim2_t_
{
    Gia_Man_t *    pAig;
    Gia_ParSim_t * pPars; 
    int            nWords;
    unsigned *     pDataSim; 
    Vec_Int_t *    vClassOld;
    Vec_Int_t *    vClassNew;
};

static inline unsigned * Gia_Sim2Data( Gia_Sim2_t * p, int i )    { return p->pDataSim + i * p->nWords;    }

extern void Gia_ManResetRandom( Gia_ParSim_t * pPars );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_Sim2Delete( Gia_Sim2_t * p )
{
    Vec_IntFreeP( &p->vClassOld );
    Vec_IntFreeP( &p->vClassNew );
    ABC_FREE( p->pDataSim );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Creates fast simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Sim2_t * Gia_Sim2Create( Gia_Man_t * pAig, Gia_ParSim_t * pPars )
{
    Gia_Sim2_t * p;
    Gia_Obj_t * pObj;
    int i;
    p = ABC_CALLOC( Gia_Sim2_t, 1 );
    p->pAig      = pAig;
    p->pPars     = pPars;
    p->nWords    = pPars->nWords;
    p->pDataSim  = ABC_ALLOC( unsigned, p->nWords * Gia_ManObjNum(p->pAig) );
    if ( !p->pDataSim  )
    { 
        Abc_Print( 1, "Simulator could not allocate %.2f GB for simulation info.\n", 
            4.0 * p->nWords * Gia_ManObjNum(p->pAig) / (1<<30) );
        Gia_Sim2Delete( p );
        return NULL;
    }
    p->vClassOld = Vec_IntAlloc( 100 );
    p->vClassNew = Vec_IntAlloc( 100 );
    if ( pPars->fVerbose )
        Abc_Print( 1, "Memory: AIG = %7.2f MB.  SimInfo = %7.2f MB.\n", 
            12.0*Gia_ManObjNum(p->pAig)/(1<<20), 4.0*p->nWords*Gia_ManObjNum(p->pAig)/(1<<20) );
    // prepare AIG
    Gia_ManSetPhase( pAig );
    Gia_ManForEachObj( pAig, pObj, i )
        pObj->Value = i;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_Sim2InfoRandom( Gia_Sim2_t * p, unsigned * pInfo )
{
    int w;
    for ( w = p->nWords-1; w >= 0; w-- )
        pInfo[w] = Gia_ManRandom( 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_Sim2InfoZero( Gia_Sim2_t * p, unsigned * pInfo )
{
    int w;
    for ( w = p->nWords-1; w >= 0; w-- )
        pInfo[w] = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_Sim2InfoOne( Gia_Sim2_t * p, unsigned * pInfo )
{
    int w;
    for ( w = p->nWords-1; w >= 0; w-- )
        pInfo[w] = ~0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_Sim2InfoCopy( Gia_Sim2_t * p, unsigned * pInfo, unsigned * pInfo0 )
{
    int w;
    for ( w = p->nWords-1; w >= 0; w-- )
        pInfo[w] = pInfo0[w];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_Sim2SimulateCo( Gia_Sim2_t * p, Gia_Obj_t * pObj )
{
    unsigned * pInfo  = Gia_Sim2Data( p, Gia_ObjValue(pObj) );
    unsigned * pInfo0 = Gia_Sim2Data( p, Gia_ObjFaninId0(pObj, Gia_ObjValue(pObj)) );
    int w;
    if ( Gia_ObjFaninC0(pObj) )
        for ( w = p->nWords-1; w >= 0; w-- )
            pInfo[w] = ~pInfo0[w];
    else 
        for ( w = p->nWords-1; w >= 0; w-- )
            pInfo[w] = pInfo0[w];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_Sim2SimulateNode( Gia_Sim2_t * p, Gia_Obj_t * pObj )
{
    unsigned * pInfo  = Gia_Sim2Data( p, Gia_ObjValue(pObj) );
    unsigned * pInfo0 = Gia_Sim2Data( p, Gia_ObjFaninId0(pObj, Gia_ObjValue(pObj)) );
    unsigned * pInfo1 = Gia_Sim2Data( p, Gia_ObjFaninId1(pObj, Gia_ObjValue(pObj)) );
    int w;
    if ( Gia_ObjFaninC0(pObj) )
    {
        if (  Gia_ObjFaninC1(pObj) )
            for ( w = p->nWords-1; w >= 0; w-- )
                pInfo[w] = ~(pInfo0[w] | pInfo1[w]);
        else 
            for ( w = p->nWords-1; w >= 0; w-- )
                pInfo[w] = ~pInfo0[w] & pInfo1[w];
    }
    else 
    {
        if (  Gia_ObjFaninC1(pObj) )
            for ( w = p->nWords-1; w >= 0; w-- )
                pInfo[w] = pInfo0[w] & ~pInfo1[w];
        else 
            for ( w = p->nWords-1; w >= 0; w-- )
                pInfo[w] = pInfo0[w] & pInfo1[w];
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_Sim2InfoTransfer( Gia_Sim2_t * p )
{
    Gia_Obj_t * pObjRo, * pObjRi;
    unsigned * pInfo0, * pInfo1;
    int i;
    Gia_ManForEachRiRo( p->pAig, pObjRi, pObjRo, i )
    {
        pInfo0 = Gia_Sim2Data( p, Gia_ObjValue(pObjRo) );
        pInfo1 = Gia_Sim2Data( p, Gia_ObjValue(pObjRi) );
        Gia_Sim2InfoCopy( p, pInfo0, pInfo1 );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_Sim2SimulateRound( Gia_Sim2_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    pObj = Gia_ManConst0(p->pAig);
    assert( Gia_ObjValue(pObj) == 0 );
    Gia_Sim2InfoZero( p, Gia_Sim2Data(p, Gia_ObjValue(pObj)) );
    Gia_ManForEachPi( p->pAig, pObj, i )
        Gia_Sim2InfoRandom( p, Gia_Sim2Data(p, Gia_ObjValue(pObj)) );
    Gia_ManForEachAnd( p->pAig, pObj, i )
    {
        assert( Gia_ObjValue(pObj) == i );
        Gia_Sim2SimulateNode( p, pObj );
    }
    Gia_ManForEachCo( p->pAig, pObj, i )
        Gia_Sim2SimulateCo( p, pObj );
}



/**Function*************************************************************

  Synopsis    [Compares simulation info of two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_Sim2CompareEqual( unsigned * p0, unsigned * p1, int nWords, int fCompl )
{
    int w;
    if ( !fCompl )
    {
        for ( w = 0; w < nWords; w++ )
            if ( p0[w] != p1[w] )
                return 0;
        return 1;
    }
    else
    {
        for ( w = 0; w < nWords; w++ )
            if ( p0[w] != ~p1[w] )
                return 0;
        return 1;
    }
}

/**Function*************************************************************

  Synopsis    [Compares simulation info of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_Sim2CompareZero( unsigned * p0, int nWords, int fCompl )
{
    int w;
    if ( !fCompl )
    {
        for ( w = 0; w < nWords; w++ )
            if ( p0[w] != 0 )
                return 0;
        return 1;
    }
    else
    {
        for ( w = 0; w < nWords; w++ )
            if ( p0[w] != ~0 )
                return 0;
        return 1;
    }
}

/**Function*************************************************************

  Synopsis    [Creates equivalence class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_Sim2ClassCreate( Gia_Man_t * p, Vec_Int_t * vClass )
{
    int Repr = GIA_VOID, EntPrev = -1, Ent, i;
    assert( Vec_IntSize(vClass) > 0 );
    Vec_IntForEachEntry( vClass, Ent, i )
    {
        if ( i == 0 )
        {
            Repr = Ent;
            Gia_ObjSetRepr( p, Ent, GIA_VOID );
            EntPrev = Ent;
        }
        else
        {
            assert( Repr < Ent );
            Gia_ObjSetRepr( p, Ent, Repr );
            Gia_ObjSetNext( p, EntPrev, Ent );
            EntPrev = Ent;
        }
    }
    Gia_ObjSetNext( p, EntPrev, 0 );
}

/**Function*************************************************************

  Synopsis    [Refines one equivalence class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_Sim2ClassRefineOne( Gia_Sim2_t * p, int i )
{
    Gia_Obj_t * pObj0, * pObj1;
    unsigned * pSim0, * pSim1;
    int Ent;
    Vec_IntClear( p->vClassOld );
    Vec_IntClear( p->vClassNew );
    Vec_IntPush( p->vClassOld, i );
    pObj0 = Gia_ManObj( p->pAig, i );
    pSim0 = Gia_Sim2Data( p, i );
    Gia_ClassForEachObj1( p->pAig, i, Ent )
    {
        pObj1 = Gia_ManObj( p->pAig, Ent );
        pSim1 = Gia_Sim2Data( p, Ent );
        if ( Gia_Sim2CompareEqual( pSim0, pSim1, p->nWords, Gia_ObjPhase(pObj0) ^ Gia_ObjPhase(pObj1) ) )
            Vec_IntPush( p->vClassOld, Ent );
        else
            Vec_IntPush( p->vClassNew, Ent );
    }
    if ( Vec_IntSize( p->vClassNew ) == 0 )
        return 0;
    Gia_Sim2ClassCreate( p->pAig, p->vClassOld );
    Gia_Sim2ClassCreate( p->pAig, p->vClassNew );
    if ( Vec_IntSize(p->vClassNew) > 1 )
        return 1 + Gia_Sim2ClassRefineOne( p, Vec_IntEntry(p->vClassNew,0) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes hash key of the simuation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_Sim2HashKey( unsigned * pSim, int nWords, int nTableSize )
{
    static int s_Primes[16] = { 
        1291, 1699, 1999, 2357, 2953, 3313, 3907, 4177, 
        4831, 5147, 5647, 6343, 6899, 7103, 7873, 8147 };
    unsigned uHash = 0;
    int i;
    if ( pSim[0] & 1 )
        for ( i = 0; i < nWords; i++ )
            uHash ^= ~pSim[i] * s_Primes[i & 0xf];
    else
        for ( i = 0; i < nWords; i++ )
            uHash ^= pSim[i] * s_Primes[i & 0xf];
    return (int)(uHash % nTableSize);

}

/**Function*************************************************************

  Synopsis    [Refines nodes belonging to candidate constant class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_Sim2ProcessRefined( Gia_Sim2_t * p, Vec_Int_t * vRefined )
{
    unsigned * pSim;
    int * pTable, nTableSize, i, k, Key;
    if ( Vec_IntSize(vRefined) == 0 )
        return;
    nTableSize = Abc_PrimeCudd( 1000 + Vec_IntSize(vRefined) / 3 );
    pTable = ABC_CALLOC( int, nTableSize );
    Vec_IntForEachEntry( vRefined, i, k )
    {
        pSim = Gia_Sim2Data( p, i );
        Key = Gia_Sim2HashKey( pSim, p->nWords, nTableSize );
        if ( pTable[Key] == 0 )
        {
            assert( Gia_ObjRepr(p->pAig, i) == 0 );
            assert( Gia_ObjNext(p->pAig, i) == 0 );
            Gia_ObjSetRepr( p->pAig, i, GIA_VOID );
        }
        else
        {
            Gia_ObjSetNext( p->pAig, pTable[Key], i );
            Gia_ObjSetRepr( p->pAig, i, Gia_ObjRepr(p->pAig, pTable[Key]) );
            if ( Gia_ObjRepr(p->pAig, i) == GIA_VOID )
                Gia_ObjSetRepr( p->pAig, i, pTable[Key] );
            assert( Gia_ObjRepr(p->pAig, i) > 0 );
        }
        pTable[Key] = i;
    }
/*
    Vec_IntForEachEntry( vRefined, i, k )
    {
        if ( Gia_ObjIsHead( p->pAig, i ) )
            Gia_Sim2ClassRefineOne( p, i );
    }
*/
    ABC_FREE( pTable );
}
/**Function*************************************************************

  Synopsis    [Refines equivalences after one simulation round.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_Sim2InfoRefineEquivs( Gia_Sim2_t * p )
{
    Vec_Int_t * vRefined;
    Gia_Obj_t * pObj;
    unsigned * pSim;
    int i, Count = 0;
    // process constant candidates
    vRefined = Vec_IntAlloc( 100 );
    Gia_ManForEachObj1( p->pAig, pObj, i )
    {
        if ( !Gia_ObjIsConst(p->pAig, i) )
            continue;
        pSim = Gia_Sim2Data( p, i );
//Extra_PrintBinary( stdout, pSim, 32 * p->nWords );  printf( "\n" );
        if ( !Gia_Sim2CompareZero( pSim, p->nWords, Gia_ObjPhase(pObj) ) )
        {
            Vec_IntPush( vRefined, i );
            Count++;
        }
    }
    Gia_Sim2ProcessRefined( p, vRefined );
    Vec_IntFree( vRefined );
    // process other classes
    Gia_ManForEachClass( p->pAig, i )
        Count += Gia_Sim2ClassRefineOne( p, i );
//    if ( Count )
//        printf( "Refined %d times.\n", Count );
}

/**Function*************************************************************

  Synopsis    [Returns index of the first pattern that failed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_Sim2InfoIsZero( Gia_Sim2_t * p, unsigned * pInfo )
{
    int w;
    for ( w = 0; w < p->nWords; w++ )
        if ( pInfo[w] )
            return 32*w + Gia_WordFindFirstBit( pInfo[w] );
    return -1;
}

/**Function*************************************************************

  Synopsis    [Returns index of the PO and pattern that failed it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_Sim2CheckPos( Gia_Sim2_t * p, int * piPo, int * piPat )
{
    Gia_Obj_t * pObj;
    int i, iPat;
    Gia_ManForEachPo( p->pAig, pObj, i )
    {
        iPat = Gia_Sim2InfoIsZero( p, Gia_Sim2Data( p, Gia_ObjValue(pObj) ) );
        if ( iPat >= 0 )
        {
            *piPo = i;
            *piPat = iPat;
            return 1;
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Gia_Sim2GenerateCounter( Gia_Man_t * pAig, int iFrame, int iOut, int nWords, int iPat )
{
    Abc_Cex_t * p;
    unsigned * pData;
    int f, i, w, Counter;
    p = Abc_CexAlloc( Gia_ManRegNum(pAig), Gia_ManPiNum(pAig), iFrame+1 );
    p->iFrame = iFrame;
    p->iPo = iOut;
    // fill in the binary data
    Counter = p->nRegs;
    pData = ABC_ALLOC( unsigned, nWords );
    for ( f = 0; f <= iFrame; f++, Counter += p->nPis )
    for ( i = 0; i < Gia_ManPiNum(pAig); i++ )
    {
        for ( w = nWords-1; w >= 0; w-- )
            pData[w] = Gia_ManRandom( 0 );
        if ( Abc_InfoHasBit( pData, iPat ) )
            Abc_InfoSetBit( p->pData, Counter + i );
    }
    ABC_FREE( pData );
    return p;
}
 
/**Function*************************************************************

  Synopsis    [Performs simulation to refine equivalence classes.]

  Description [Returns 1 if counter-example is detected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSimSimulateEquiv( Gia_Man_t * pAig, Gia_ParSim_t * pPars )
{
    Gia_Sim2_t * p;
    Gia_Obj_t * pObj;
    abctime clkTotal = Abc_Clock();
    int i, RetValue = 0, iOut, iPat;
    abctime nTimeToStop = pPars->TimeLimit ? pPars->TimeLimit * CLOCKS_PER_SEC + Abc_Clock(): 0;
    assert( pAig->pReprs && pAig->pNexts );
    ABC_FREE( pAig->pCexSeq );
    p = Gia_Sim2Create( pAig, pPars );
    Gia_ManResetRandom( pPars );
    Gia_ManForEachRo( p->pAig, pObj, i )
        Gia_Sim2InfoZero( p, Gia_Sim2Data(p, Gia_ObjValue(pObj)) );
    for ( i = 0; i < pPars->nIters; i++ )
    {
        Gia_Sim2SimulateRound( p );
        if ( pPars->fVerbose )
        {
            Abc_Print( 1, "Frame %4d out of %4d and timeout %3d sec. ", i+1, pPars->nIters, pPars->TimeLimit );
            if ( pAig->pReprs && pAig->pNexts )
                Abc_Print( 1, "Lits = %4d. ", Gia_ManEquivCountLitsAll(pAig) );
            Abc_Print( 1, "Time = %7.2f sec\r", (1.0*Abc_Clock()-clkTotal)/CLOCKS_PER_SEC );
        }
        if ( pPars->fCheckMiter && Gia_Sim2CheckPos( p, &iOut, &iPat ) )
        {
            Gia_ManResetRandom( pPars );
            pPars->iOutFail = iOut;
            pAig->pCexSeq = Gia_Sim2GenerateCounter( pAig, i, iOut, p->nWords, iPat );
            Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d.  ", iOut, pAig->pName, i );
            if ( !Gia_ManVerifyCex( pAig, pAig->pCexSeq, 0 ) )
            {
//                Abc_Print( 1, "\n" );
                Abc_Print( 1, "\nGenerated counter-example is INVALID.                    " );
//                Abc_Print( 1, "\n" );
            }
            else
            {
//                Abc_Print( 1, "\n" );
//                if ( pPars->fVerbose )
//                Abc_Print( 1, "\nGenerated counter-example is verified correctly.         " );
//                Abc_Print( 1, "\n" );
            }
            RetValue = 1;
            break;
        }
        if ( pAig->pReprs && pAig->pNexts )
            Gia_Sim2InfoRefineEquivs( p );
        if ( Abc_Clock() > nTimeToStop )
        {
            i++;
            break;
        }
        if ( i < pPars->nIters - 1 )
            Gia_Sim2InfoTransfer( p );
    }
    Gia_Sim2Delete( p );
    if ( pAig->pCexSeq == NULL )
        Abc_Print( 1, "No bug detected after simulating %d frames with %d words.  ", i, pPars->nWords );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

