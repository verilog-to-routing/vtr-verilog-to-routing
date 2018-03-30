/**CFile****************************************************************

  FileName    [fsimSim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast sequential AIG simulator.]

  Synopsis    [Simulation procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: fsimSim.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fsimInt.h"
#include "aig/ssw/ssw.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManSimInfoRandom( Fsim_Man_t * p, unsigned * pInfo )
{
    int w;
    for ( w = p->nWords-1; w >= 0; w-- )
        pInfo[w] = Aig_ManRandom( 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManSimInfoZero( Fsim_Man_t * p, unsigned * pInfo )
{
    int w;
    for ( w = p->nWords-1; w >= 0; w-- )
        pInfo[w] = 0;
}

/**Function*************************************************************

  Synopsis    [Returns index of the first pattern that failed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Fsim_ManSimInfoIsZero( Fsim_Man_t * p, unsigned * pInfo )
{
    int w;
    for ( w = p->nWords-1; w >= 0; w-- )
        if ( pInfo[w] )
            return 32*(w-1) + Aig_WordFindFirstBit( pInfo[w] );
    return -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManSimInfoOne( Fsim_Man_t * p, unsigned * pInfo )
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
static inline void Fsim_ManSimInfoCopy( Fsim_Man_t * p, unsigned * pInfo, unsigned * pInfo0 )
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
static inline void Fsim_ManSimulateCi( Fsim_Man_t * p, int iNode, int iCi )
{
    unsigned * pInfo  = Fsim_SimData( p, iNode % p->nFront );
    unsigned * pInfo0 = Fsim_SimDataCi( p, iCi );
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
static inline void Fsim_ManSimulateCo( Fsim_Man_t * p, int iCo, int iFan0 )
{
    unsigned * pInfo  = Fsim_SimDataCo( p, iCo );
    unsigned * pInfo0 = Fsim_SimData( p, Fsim_Lit2Var(iFan0) % p->nFront );
    int w;
    if ( Fsim_LitIsCompl(iFan0) )
        for ( w = p->nWords-1; w >= 0; w-- )
            pInfo[w] = ~pInfo0[w];
    else //if ( !Fsim_LitIsCompl(iFan0) )
        for ( w = p->nWords-1; w >= 0; w-- )
            pInfo[w] = pInfo0[w];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManSimulateNode( Fsim_Man_t * p, int iNode, int iFan0, int iFan1 )
{
    unsigned * pInfo  = Fsim_SimData( p, iNode % p->nFront );
    unsigned * pInfo0 = Fsim_SimData( p, Fsim_Lit2Var(iFan0) % p->nFront );
    unsigned * pInfo1 = Fsim_SimData( p, Fsim_Lit2Var(iFan1) % p->nFront );
    int w;
    if ( Fsim_LitIsCompl(iFan0) && Fsim_LitIsCompl(iFan1) )
        for ( w = p->nWords-1; w >= 0; w-- )
            pInfo[w] = ~(pInfo0[w] | pInfo1[w]);
    else if ( Fsim_LitIsCompl(iFan0) && !Fsim_LitIsCompl(iFan1) )
        for ( w = p->nWords-1; w >= 0; w-- )
            pInfo[w] = ~pInfo0[w] & pInfo1[w];
    else if ( !Fsim_LitIsCompl(iFan0) && Fsim_LitIsCompl(iFan1) )
        for ( w = p->nWords-1; w >= 0; w-- )
            pInfo[w] = pInfo0[w] & ~pInfo1[w];
    else //if ( !Fsim_LitIsCompl(iFan0) && !Fsim_LitIsCompl(iFan1) )
        for ( w = p->nWords-1; w >= 0; w-- )
            pInfo[w] = pInfo0[w] & pInfo1[w];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManSimInfoInit( Fsim_Man_t * p )
{
    int iPioNum, i;
    Vec_IntForEachEntry( p->vCis2Ids, iPioNum, i )
    {
        if ( iPioNum < p->nPis )
            Fsim_ManSimInfoRandom( p, Fsim_SimDataCi(p, i) );
        else
            Fsim_ManSimInfoZero( p, Fsim_SimDataCi(p, i) );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManSimInfoTransfer( Fsim_Man_t * p )
{
    int iPioNum, i;
    Vec_IntForEachEntry( p->vCis2Ids, iPioNum, i )
    {
        if ( iPioNum < p->nPis )
            Fsim_ManSimInfoRandom( p, Fsim_SimDataCi(p, i) );
        else
            Fsim_ManSimInfoCopy( p, Fsim_SimDataCi(p, i), Fsim_SimDataCo(p, p->nPos+iPioNum-p->nPis) );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Fsim_ManRestoreNum( Fsim_Man_t * p )
{
    int ch, i, x = 0;
    for ( i = 0; (ch = *p->pDataCur++) & 0x80; i++ )
        x |= (ch & 0x7f) << (7 * i);
    assert( p->pDataCur - p->pDataAig < p->nDataAig );
    return x | (ch << (7 * i));
}

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Fsim_ManRestoreObj( Fsim_Man_t * p, Fsim_Obj_t * pObj )
{
    int iValue = Fsim_ManRestoreNum( p );
    if ( (iValue & 3) == 3 ) // and
    {
        pObj->iNode = (iValue >> 2) + p->iNodePrev;
        pObj->iFan0 = (pObj->iNode << 1) - Fsim_ManRestoreNum( p );
        pObj->iFan1 = pObj->iFan0 - Fsim_ManRestoreNum( p );
        p->iNodePrev = pObj->iNode;
    }
    else if ( (iValue & 3) == 1 ) // ci
    {
        pObj->iNode = (iValue >> 2) + p->iNodePrev;
        pObj->iFan0 = 0;
        pObj->iFan1 = 0;
        p->iNodePrev = pObj->iNode;
    }
    else // if ( (iValue & 1) == 0 ) // co
    {
        pObj->iNode = 0;
        pObj->iFan0 = ((p->iNodePrev << 1) | 1) - (iValue >> 1);
        pObj->iFan1 = 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManSimulateRound2( Fsim_Man_t * p )
{
    Fsim_Obj_t * pObj;
    int i, iCis = 0, iCos = 0;
    if ( Aig_ObjRefs(Aig_ManConst1(p->pAig)) )
        Fsim_ManSimInfoOne( p, Fsim_SimData(p, 1) );
    Fsim_ManForEachObj( p, pObj, i )
    {
        if ( pObj->iFan0 == 0 )
            Fsim_ManSimulateCi( p, pObj->iNode, iCis++ );
        else if ( pObj->iFan1 == 0 )
            Fsim_ManSimulateCo( p, iCos++, pObj->iFan0 );
        else
            Fsim_ManSimulateNode( p, pObj->iNode, pObj->iFan0, pObj->iFan1 );
    }
    assert( iCis == p->nCis );
    assert( iCos == p->nCos );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManSimulateRound( Fsim_Man_t * p )
{
    int * pCur, * pEnd;
    int iCis = 0, iCos = 0;
    if ( p->pDataAig2 == NULL )
    {
        Fsim_ManSimulateRound2( p );
        return;
    }
    if ( Aig_ObjRefs(Aig_ManConst1(p->pAig)) )
        Fsim_ManSimInfoOne( p, Fsim_SimData(p, 1) );
    pCur = p->pDataAig2 + 6;
    pEnd = p->pDataAig2 + 3 * p->nObjs;
    while ( pCur < pEnd )
    {
        if ( pCur[1] == 0 )
            Fsim_ManSimulateCi( p, pCur[0], iCis++ );
        else if ( pCur[2] == 0 )
            Fsim_ManSimulateCo( p, iCos++, pCur[1] );
        else
            Fsim_ManSimulateNode( p, pCur[0], pCur[1], pCur[2] );
        pCur += 3;
    }
    assert( iCis == p->nCis );
    assert( iCos == p->nCos );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fsim_ManSimulateRoundTest( Fsim_Man_t * p )
{
    Fsim_Obj_t * pObj;
    int i;
    clock_t clk = clock();
    Fsim_ManForEachObj( p, pObj, i )
    {
    }
//    ABC_PRT( "Unpacking time", p->pPars->nIters * (clock() - clk) );
}

/**Function*************************************************************

  Synopsis    [Returns index of the PO and pattern that failed it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Fsim_ManCheckPos( Fsim_Man_t * p, int * piPo, int * piPat )
{
    int i, iPat;
    for ( i = 0; i < p->nPos; i++ )
    {
        iPat = Fsim_ManSimInfoIsZero( p, Fsim_SimDataCo(p, i) );
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
Abc_Cex_t * Fsim_ManGenerateCounter( Aig_Man_t * pAig, int iFrame, int iOut, int nWords, int iPat, Vec_Int_t * vCis2Ids )
{
    Abc_Cex_t * p;
    unsigned * pData;
    int f, i, w, iPioId, Counter;
    p = Abc_CexAlloc( Aig_ManRegNum(pAig), Saig_ManPiNum(pAig), iFrame+1 );
    p->iFrame = iFrame;
    p->iPo = iOut;
    // fill in the binary data
    Aig_ManRandom( 1 );
    Counter = p->nRegs;
    pData = ABC_ALLOC( unsigned, nWords );
    for ( f = 0; f <= iFrame; f++, Counter += p->nPis )
    for ( i = 0; i < Aig_ManPiNum(pAig); i++ )
    {
        iPioId = Vec_IntEntry( vCis2Ids, i );
        if ( iPioId >= p->nPis )
            continue;
        for ( w = nWords-1; w >= 0; w-- )
            pData[w] = Aig_ManRandom( 0 );
        if ( Aig_InfoHasBit( pData, iPat ) )
            Aig_InfoSetBit( p->pData, Counter + iPioId );
    }
    ABC_FREE( pData );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fsim_ManSimulate( Aig_Man_t * pAig, Fsim_ParSim_t * pPars )
{
    Fsim_Man_t * p;
    Sec_MtrStatus_t Status;
    int i, iOut, iPat;
    clock_t clk, clkTotal = clock(), clk2, clk2Total = 0;
    assert( Aig_ManRegNum(pAig) > 0 );
    if ( pPars->fCheckMiter )
    {
        Status = Sec_MiterStatus( pAig );
        if ( Status.nSat > 0 )
        {
            printf( "Miter is trivially satisfiable (output %d).\n", Status.iOut );
            return 1;
        }
        if ( Status.nUndec == 0 )
        {
            printf( "Miter is trivially unsatisfiable.\n" );
            return 0;
        }
    }
    // create manager
    clk = clock();
    p = Fsim_ManCreate( pAig );
    p->nWords = pPars->nWords;
    if ( pPars->fVerbose )
    {
        printf( "Obj = %8d (%8d). Cut = %6d. Front = %6d.  FrtMem = %7.2f MB. ", 
            p->nObjs, p->nCis + p->nNodes, p->nCrossCutMax, p->nFront, 
            4.0*p->nWords*(p->nFront)/(1<<20) );
        ABC_PRT( "Time", clock() - clk );
    }
    // create simulation frontier
    clk = clock();
    Fsim_ManFront( p, pPars->fCompressAig );
    if ( pPars->fVerbose )
    {
        printf( "Max ID = %8d. Log max ID = %2d.  AigMem = %7.2f MB (%5.2f byte/obj).  ", 
            p->iNumber, Aig_Base2Log(p->iNumber), 
            1.0*(p->pDataCur-p->pDataAig)/(1<<20), 
            1.0*(p->pDataCur-p->pDataAig)/p->nObjs ); 
        ABC_PRT( "Time", clock() - clk );
    }
    // perform simulation
    Aig_ManRandom( 1 );
    assert( p->pDataSim == NULL );
    p->pDataSim = ABC_ALLOC( unsigned, p->nWords * p->nFront );
    p->pDataSimCis = ABC_ALLOC( unsigned, p->nWords * p->nCis );
    p->pDataSimCos = ABC_ALLOC( unsigned, p->nWords * p->nCos );
    Fsim_ManSimInfoInit( p );
    for ( i = 0; i < pPars->nIters; i++ )
    { 
        Fsim_ManSimulateRound( p );
        if ( pPars->fVerbose )
        {
            printf( "Frame %4d out of %4d and timeout %3d sec. ", i+1, pPars->nIters, pPars->TimeLimit );
            printf( "Time = %7.2f sec\r", (1.0*clock()-clkTotal)/CLOCKS_PER_SEC );
        }
        if ( pPars->fCheckMiter && Fsim_ManCheckPos( p, &iOut, &iPat ) )
        {
            assert( pAig->pSeqModel == NULL );
            pAig->pSeqModel = Fsim_ManGenerateCounter( pAig, i, iOut, p->nWords, iPat, p->vCis2Ids );
            if ( pPars->fVerbose )
            printf( "Miter is satisfiable after simulation (output %d).\n", iOut );
            break;
        }
        if ( (clock() - clkTotal)/CLOCKS_PER_SEC >= pPars->TimeLimit )
            break;
        clk2 = clock();
        if ( i < pPars->nIters - 1 )
            Fsim_ManSimInfoTransfer( p );
        clk2Total += clock() - clk2;
    }
    if ( pAig->pSeqModel == NULL )
        printf( "No bug detected after %d frames with time limit %d seconds.\n", i+1, pPars->TimeLimit );
    if ( pPars->fVerbose )
    {
        printf( "Maxcut = %8d.  AigMem = %7.2f MB.  SimMem = %7.2f MB.  ", 
            p->nCrossCutMax, 
            p->pDataAig2? 12.0*p->nObjs/(1<<20) : 1.0*(p->pDataCur-p->pDataAig)/(1<<20), 
            4.0*p->nWords*(p->nFront+p->nCis+p->nCos)/(1<<20) );
        ABC_PRT( "Sim time", clock() - clkTotal );

//        ABC_PRT( "Additional time", clk2Total );
//        Fsim_ManSimulateRoundTest( p );
//        Fsim_ManSimulateRoundTest2( p );
    }
    Fsim_ManDelete( p );
    return pAig->pSeqModel != NULL;

}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

