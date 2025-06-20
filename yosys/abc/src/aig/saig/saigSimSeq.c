/**CFile****************************************************************

  FileName    [saigSimSeq.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Fast sequential AIG simulator.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigSimSeq.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"
#include "proof/ssw/ssw.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// combinational simulation manager
typedef struct Raig_Man_t_ Raig_Man_t;
struct Raig_Man_t_
{
    // parameters
    Aig_Man_t *     pAig;         // the AIG to be used for simulation
    int             nWords;       // the number of words to simulate
    // AIG representation
    int             nPis;         // the number of primary inputs
    int             nPos;         // the number of primary outputs
    int             nCis;         // the number of combinational inputs
    int             nCos;         // the number of combinational outputs
    int             nNodes;       // the number of internal nodes
    int             nObjs;        // nCis + nNodes + nCos + 2
    int *           pFans0;       // fanin0 for all objects
    int *           pFans1;       // fanin1 for all objects
    Vec_Int_t *     vCis2Ids;     // mapping of CIs into their PI ids
    Vec_Int_t *     vLos;         // register outputs
    Vec_Int_t *     vLis;         // register inputs
    // simulation info
    int *           pRefs;        // reference counter for each node
    unsigned *      pSims;        // simlulation information for each node
    // recycable memory
    unsigned *      pMems;        // allocated simulaton memory
    int             nWordsAlloc;  // the number of allocated entries
    int             nMems;        // the number of used entries  
    int             nMemsMax;     // the max number of used entries 
    int             MemFree;      // next free entry
};

static inline int  Raig_Var2Lit( int Var, int fCompl )  { return Var + Var + fCompl; }
static inline int  Raig_Lit2Var( int Lit )              { return Lit >> 1;           }
static inline int  Raig_LitIsCompl( int Lit )           { return Lit & 1;            }
static inline int  Raig_LitNot( int Lit )               { return Lit ^ 1;            }
static inline int  Raig_LitNotCond( int Lit, int c )    { return Lit ^ (int)(c > 0); }
static inline int  Raig_LitRegular( int Lit )           { return Lit & ~01;          }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Find the PO corresponding to the PO driver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Raig_ManFindPo( Aig_Man_t * pAig, int iNode )
{
    Aig_Obj_t * pObj;
    int i;
    Saig_ManForEachPo( pAig, pObj, i )
        if ( pObj->iData == iNode )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Creates fast simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Raig_ManCreate_rec( Raig_Man_t * p, Aig_Obj_t * pObj )
{
    int iFan0, iFan1;
    assert( !Aig_IsComplement(pObj) );
    if ( pObj->iData )
        return pObj->iData;
    assert( !Aig_ObjIsConst1(pObj) );
    if ( Aig_ObjIsNode(pObj) )
    {
        iFan0 = Raig_ManCreate_rec( p, Aig_ObjFanin0(pObj) );
        iFan0 = (iFan0 << 1) | Aig_ObjFaninC0(pObj);
        iFan1 = Raig_ManCreate_rec( p, Aig_ObjFanin1(pObj) );
        iFan1 = (iFan1 << 1) | Aig_ObjFaninC1(pObj);
    }
    else if ( Aig_ObjIsCo(pObj) )
    {
        iFan0 = Raig_ManCreate_rec( p, Aig_ObjFanin0(pObj) );
        iFan0 = (iFan0 << 1) | Aig_ObjFaninC0(pObj);
        iFan1 = 0;
    }
    else
    {
        iFan0 = iFan1 = 0;
        Vec_IntPush( p->vCis2Ids, Aig_ObjCioId(pObj) );
    }
    p->pFans0[p->nObjs] = iFan0;
    p->pFans1[p->nObjs] = iFan1;
    p->pRefs[p->nObjs] = Aig_ObjRefs(pObj);
    return pObj->iData = p->nObjs++;
}

/**Function*************************************************************

  Synopsis    [Creates fast simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Raig_Man_t * Raig_ManCreate( Aig_Man_t * pAig )
{
    Raig_Man_t * p;
    Aig_Obj_t * pObj;
    int i, nObjs;
    Aig_ManCleanData( pAig );
    p = (Raig_Man_t *)ABC_ALLOC( Raig_Man_t, 1 );
    memset( p, 0, sizeof(Raig_Man_t) );
    p->pAig = pAig;
    p->nPis = Saig_ManPiNum(pAig);
    p->nPos = Saig_ManPoNum(pAig);
    p->nCis = Aig_ManCiNum(pAig);
    p->nCos = Aig_ManCoNum(pAig);
    p->nNodes = Aig_ManNodeNum(pAig);
    nObjs = p->nCis + p->nCos + p->nNodes + 2;
    p->pFans0 = ABC_ALLOC( int, nObjs );
    p->pFans1 = ABC_ALLOC( int, nObjs );
    p->pRefs = ABC_ALLOC( int, nObjs );
    p->pSims = ABC_CALLOC( unsigned, nObjs );
    p->vCis2Ids = Vec_IntAlloc( Aig_ManCiNum(pAig) );
    // add objects (0=unused; 1=const1)
    p->nObjs = 2;
    pObj = Aig_ManConst1( pAig );
    pObj->iData = 1;
    Aig_ManForEachCi( pAig, pObj, i )
        if ( Aig_ObjRefs(pObj) == 0 )
            Raig_ManCreate_rec( p, pObj );
    Aig_ManForEachCo( pAig, pObj, i )
        Raig_ManCreate_rec( p, pObj );
    assert( Vec_IntSize(p->vCis2Ids) == Aig_ManCiNum(pAig) );
    assert( p->nObjs == nObjs );
    // collect flop outputs
    p->vLos = Vec_IntAlloc( Aig_ManRegNum(pAig) );
    Saig_ManForEachLo( pAig, pObj, i )
        Vec_IntPush( p->vLos, pObj->iData );
    // collect flop inputs
    p->vLis = Vec_IntAlloc( Aig_ManRegNum(pAig) );
    Saig_ManForEachLi( pAig, pObj, i )
    {
        Vec_IntPush( p->vLis, pObj->iData );
        assert( p->pRefs[ pObj->iData ] == 0 );
        p->pRefs[ pObj->iData ]++;
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates fast simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Raig_ManDelete( Raig_Man_t * p )
{
    Vec_IntFree( p->vCis2Ids );
    Vec_IntFree( p->vLos );
    Vec_IntFree( p->vLis );
    ABC_FREE( p->pFans0 );
    ABC_FREE( p->pFans1 );
    ABC_FREE( p->pRefs );
    ABC_FREE( p->pSims );
    ABC_FREE( p->pMems );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [References simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Raig_ManSimRef( Raig_Man_t * p, int i )
{
    unsigned * pSim;
    assert( i > 1 );
    assert( p->pSims[i] == 0 );
    if ( p->MemFree == 0 )
    {
        unsigned * pPlace, Ent;
        if ( p->nWordsAlloc == 0 )
        {
            assert( p->pMems == NULL );
            p->nWordsAlloc = (1<<17); // -> 1Mb
            p->nMems = 1;
        }
        p->nWordsAlloc *= 2;
        p->pMems = ABC_REALLOC( unsigned, p->pMems, p->nWordsAlloc );
        memset( p->pMems, 0xff, sizeof(unsigned) * (p->nWords + 1) );
        pPlace = (unsigned *)&p->MemFree;
        for ( Ent = p->nMems * (p->nWords + 1); 
              Ent + p->nWords + 1 < (unsigned)p->nWordsAlloc; 
              Ent += p->nWords + 1 )
        {
            *pPlace = Ent;
            pPlace = p->pMems + Ent;
        }
        *pPlace = 0;
    }
    p->pSims[i] = p->MemFree;
    pSim = p->pMems + p->MemFree;
    p->MemFree = pSim[0];
    pSim[0] = p->pRefs[i];
    p->nMems++;
    if ( p->nMemsMax < p->nMems )
        p->nMemsMax = p->nMems;
    return pSim;
}

/**Function*************************************************************

  Synopsis    [Dereference simulaton info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Raig_ManSimDeref( Raig_Man_t * p, int i )
{
    unsigned * pSim;
    assert( i );
    if ( i == 1 ) // const 1
        return p->pMems;
    assert( p->pSims[i] > 0 );
    pSim = p->pMems + p->pSims[i];
    if ( --pSim[0] == 0 )
    {
        pSim[0] = p->MemFree;
        p->MemFree = p->pSims[i];
        p->pSims[i] = 0;
        p->nMems--;
    }
    return pSim;
}

/**Function*************************************************************

  Synopsis    [Simulates one round.]

  Description [Returns the number of PO entry if failed; 0 otherwise.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Raig_ManSimulateRound( Raig_Man_t * p, int fMiter, int fFirst, int * piPat )
{ 
    unsigned * pRes0, * pRes1, * pRes;
    int i, w, nCis, nCos, iFan0, iFan1, iPioNum;
    // nove the values to the register outputs
    Vec_IntForEachEntry( p->vCis2Ids, iPioNum, i )
    {
        if ( iPioNum < p->nPis )
            continue;
        pRes = Raig_ManSimRef( p, Vec_IntEntry(p->vLos, iPioNum-p->nPis) );
        if ( fFirst )
            memset( pRes + 1, 0, sizeof(unsigned) * p->nWords );
        else
        {
            pRes0 = Raig_ManSimDeref( p, Vec_IntEntry(p->vLis, iPioNum-p->nPis) );
            for ( w = 1; w <= p->nWords; w++ )
                pRes[w] = pRes0[w];
        }
        // handle unused PIs
        if ( pRes[0] == 0 ) 
        {
            pRes[0] = 1;
            Raig_ManSimDeref( p, Vec_IntEntry(p->vLos, iPioNum-p->nPis) );
        }
    }
    // simulate the logic
    nCis = nCos = 0;
    for ( i = 2; i < p->nObjs; i++ )
    {
        if ( p->pFans0[i] == 0 ) // ci always has zero first fanin
        {
            iPioNum = Vec_IntEntry( p->vCis2Ids, nCis );
            if ( iPioNum < p->nPis )
            {
                pRes = Raig_ManSimRef( p, i );
                for ( w = 1; w <= p->nWords; w++ )
                    pRes[w] = Aig_ManRandom( 0 );
                // handle unused PIs
                if ( pRes[0] == 0 ) 
                {
                    pRes[0] = 1;
                    Raig_ManSimDeref( p, i );
                }
            }
            else
                assert( Vec_IntEntry(p->vLos, iPioNum-p->nPis) == i );
            nCis++;
            continue;
        }
        if ( p->pFans1[i] == 0 ) // co always has non-zero 1st fanin and zero 2nd fanin
        {
            pRes0 = Raig_ManSimDeref( p, Raig_Lit2Var(p->pFans0[i]) );
            if ( nCos < p->nPos && fMiter )
            {
                unsigned Const = Raig_LitIsCompl(p->pFans0[i])? ~0 : 0;
                for ( w = 1; w <= p->nWords; w++ )
                    if ( pRes0[w] != Const )
                    {
                        *piPat = 32*(w-1) + Aig_WordFindFirstBit( pRes0[w] ^ Const );
                        return i;
                    }
            }
            else
            {
                pRes = Raig_ManSimRef( p, i );
                assert( pRes[0] == 1 );
                if ( Raig_LitIsCompl(p->pFans0[i]) )
                    for ( w = 1; w <= p->nWords; w++ )
                        pRes[w] = ~pRes0[w];
                else
                    for ( w = 1; w <= p->nWords; w++ )
                        pRes[w] = pRes0[w];
            }
            nCos++;
            continue;
        }
        pRes  = Raig_ManSimRef( p, i );
        assert( pRes[0] > 0 );
        iFan0 = p->pFans0[i];
        iFan1 = p->pFans1[i];
        pRes0 = Raig_ManSimDeref( p, Raig_Lit2Var(p->pFans0[i]) );
        pRes1 = Raig_ManSimDeref( p, Raig_Lit2Var(p->pFans1[i]) );
        if ( Raig_LitIsCompl(iFan0) && Raig_LitIsCompl(iFan1) )
            for ( w = 1; w <= p->nWords; w++ )
                pRes[w] = ~(pRes0[w] | pRes1[w]);
        else if ( Raig_LitIsCompl(iFan0) && !Raig_LitIsCompl(iFan1) )
            for ( w = 1; w <= p->nWords; w++ )
                pRes[w] = ~pRes0[w] & pRes1[w];
        else if ( !Raig_LitIsCompl(iFan0) && Raig_LitIsCompl(iFan1) )
            for ( w = 1; w <= p->nWords; w++ )
                pRes[w] = pRes0[w] & ~pRes1[w];
        else if ( !Raig_LitIsCompl(iFan0) && !Raig_LitIsCompl(iFan1) )
            for ( w = 1; w <= p->nWords; w++ )
                pRes[w] = pRes0[w] & pRes1[w];
    }
    assert( nCis == p->nCis );
    assert( nCos == p->nCos );
    assert( p->nMems == 1 + Vec_IntSize(p->vLis) );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Raig_ManGenerateCounter( Aig_Man_t * pAig, int iFrame, int iOut, int nWords, int iPat, Vec_Int_t * vCis2Ids )
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
    for ( i = 0; i < Aig_ManCiNum(pAig); i++ )
    {
        iPioId = Vec_IntEntry( vCis2Ids, i );
        if ( iPioId >= p->nPis )
            continue;
        for ( w = 0; w < nWords; w++ )
            pData[w] = Aig_ManRandom( 0 );
        if ( Abc_InfoHasBit( pData, iPat ) )
            Abc_InfoSetBit( p->pData, Counter + iPioId );
    }
    ABC_FREE( pData );
    return p;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the bug is detected, 0 otherwise.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Raig_ManSimulate( Aig_Man_t * pAig, int nWords, int nIters, int TimeLimit, int fMiter, int fVerbose )
{
    Raig_Man_t * p;
    Sec_MtrStatus_t Status;
    int i, iPat, RetValue = 0;
    abctime clk, clkTotal = Abc_Clock();
    assert( Aig_ManRegNum(pAig) > 0 );
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
    Aig_ManRandom( 1 );
    p = Raig_ManCreate( pAig );
    p->nWords = nWords;
    // iterate through objects
    for ( i = 0; i < nIters; i++ )
    {
        clk = Abc_Clock();
        RetValue = Raig_ManSimulateRound( p, fMiter, i==0, &iPat );
        if ( fVerbose )
        {
            printf( "Frame %4d out of %4d and timeout %3d sec. ", i+1, nIters, TimeLimit );
            printf("Time = %7.2f sec\r", (1.0*Abc_Clock()-clkTotal)/CLOCKS_PER_SEC);
        }
        if ( RetValue > 0 )
        {
            int iOut = Raig_ManFindPo(p->pAig, RetValue);
            assert( pAig->pSeqModel == NULL );
            pAig->pSeqModel = Raig_ManGenerateCounter( pAig, i, iOut, nWords, iPat, p->vCis2Ids );
            if ( fVerbose )
            printf( "Miter is satisfiable after simulation (output %d).\n", iOut );
            break;
        }
        if ( (Abc_Clock() - clk)/CLOCKS_PER_SEC >= TimeLimit )
        {
            printf( "No bug detected after %d frames with time limit %d seconds.\n", i+1, TimeLimit );
            break;
        }
    }
    if ( fVerbose )
    {
        printf( "Maxcut = %8d.  AigMem = %7.2f MB.  SimMem = %7.2f MB.  ", 
            p->nMemsMax, 
            1.0*(p->nObjs * 16)/(1<<20), 
            1.0*(p->nMemsMax * 4 * (nWords+1))/(1<<20) );
        ABC_PRT( "Total time", Abc_Clock() - clkTotal );
    }
    Raig_ManDelete( p );
    return RetValue > 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

