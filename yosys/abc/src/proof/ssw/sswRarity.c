/**CFile****************************************************************

  FileName    [sswRarity.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [Rarity-driven refinement of equivalence classes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswRarity.c,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sswInt.h"
#include "aig/gia/giaAig.h"
#include "base/main/main.h"
#include "sat/bmc/bmc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Ssw_RarMan_t_ Ssw_RarMan_t;
struct Ssw_RarMan_t_
{
    // parameters
    Ssw_RarPars_t* pPars;
    int            nGroups;      // the number of flop groups
    int            nWordsReg;    // the number of words in the registers
    // internal data
    Aig_Man_t *    pAig;         // AIG with equivalence classes
    Ssw_Cla_t *    ppClasses;    // equivalence classes
    Vec_Int_t *    vInits;       // initial state
    // simulation data
    word *         pObjData;     // simulation info for each obj
    word *         pPatData;     // pattern data for each reg
    // candidates to update
    Vec_Ptr_t *    vUpdConst;    // constant 1 candidates
    Vec_Ptr_t *    vUpdClass;    // class representatives
    // rarity data
    int *          pRarity;      // occur counts for patterns in groups
    double *       pPatCosts;    // pattern costs
    // best patterns
    Vec_Int_t *    vPatBests;    // best patterns
    int            iFailPo;      // failed primary output
    int            iFailPat;     // failed pattern
    // counter-examples
    Vec_Ptr_t *    vCexes;
};


static inline int  Ssw_RarGetBinPat( Ssw_RarMan_t * p, int iBin, int iPat )
{
    assert( iBin >= 0 && iBin < Aig_ManRegNum(p->pAig) / p->pPars->nBinSize );
    assert( iPat >= 0 && iPat < (1 << p->pPars->nBinSize) );
    return p->pRarity[iBin * (1 << p->pPars->nBinSize) + iPat];
}
static inline void Ssw_RarSetBinPat( Ssw_RarMan_t * p, int iBin, int iPat, int Value )
{
    assert( iBin >= 0 && iBin < Aig_ManRegNum(p->pAig) / p->pPars->nBinSize );
    assert( iPat >= 0 && iPat < (1 << p->pPars->nBinSize) );
    p->pRarity[iBin * (1 << p->pPars->nBinSize) + iPat] = Value;
}
static inline void Ssw_RarAddToBinPat( Ssw_RarMan_t * p, int iBin, int iPat )
{
    assert( iBin >= 0 && iBin < Aig_ManRegNum(p->pAig) / p->pPars->nBinSize );
    assert( iPat >= 0 && iPat < (1 << p->pPars->nBinSize) );
    p->pRarity[iBin * (1 << p->pPars->nBinSize) + iPat]++;
}

static inline int    Ssw_RarBitWordNum( int nBits )             { return (nBits>>6) + ((nBits&63) > 0);  }

static inline word * Ssw_RarObjSim( Ssw_RarMan_t * p, int Id )  { assert( Id < Aig_ManObjNumMax(p->pAig) ); return p->pObjData + p->pPars->nWords * Id;    }
static inline word * Ssw_RarPatSim( Ssw_RarMan_t * p, int Id )  { assert( Id < 64 * p->pPars->nWords );     return p->pPatData + p->nWordsReg * Id;        }


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_RarSetDefaultParams( Ssw_RarPars_t * p )
{
    memset( p, 0, sizeof(Ssw_RarPars_t) );
    p->nFrames       =  20;
    p->nWords        =  50;
    p->nBinSize      =   8;
    p->nRounds       =   0;
    p->nRestart      =   0;
    p->nRandSeed     =   0;
    p->TimeOut       =   0;
    p->TimeOutGap    =   0;
    p->fSolveAll     =   0;
    p->fDropSatOuts  =   0;
    p->fSetLastState =   0;
    p->fVerbose      =   0;
    p->fNotVerbose   =   0;
}

/**Function*************************************************************

  Synopsis    [Prepares random number generator.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_RarManPrepareRandom( int nRandSeed )
{
    int i;
    Aig_ManRandom( 1 );
    for ( i = 0; i < nRandSeed; i++ )
        Aig_ManRandom( 0 );
}

/**Function*************************************************************

  Synopsis    [Initializes random primary inputs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_RarManAssingRandomPis( Ssw_RarMan_t * p )
{
    word * pSim;
    Aig_Obj_t * pObj;
    int w, i;
    Saig_ManForEachPi( p->pAig, pObj, i )
    {
        pSim = Ssw_RarObjSim( p, Aig_ObjId(pObj) );
        for ( w = 0; w < p->pPars->nWords; w++ )
            pSim[w] = Aig_ManRandom64(0);
//        pSim[0] <<= 1;
//        pSim[0] = (pSim[0] << 2) | 2;
        pSim[0] = (pSim[0] << 4) | ((i & 1) ? 0xA : 0xC);
    }
}

/**Function*************************************************************

  Synopsis    [Derives the counter-example.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Ssw_RarDeriveCex( Ssw_RarMan_t * p, int iFrame, int iPo, int iPatFinal, int fVerbose )
{
    Abc_Cex_t * pCex;
    Aig_Obj_t * pObj;
    Vec_Int_t * vTrace;
    word * pSim;
    int i, r, f, iBit, iPatThis;
    // compute the pattern sequence
    iPatThis = iPatFinal;
    vTrace = Vec_IntStartFull( iFrame / p->pPars->nFrames + 1 );
    Vec_IntWriteEntry( vTrace, iFrame / p->pPars->nFrames, iPatThis );
    for ( r = iFrame / p->pPars->nFrames - 1; r >= 0; r-- )
    {
        iPatThis = Vec_IntEntry( p->vPatBests, r * p->pPars->nWords + iPatThis / 64 );
        Vec_IntWriteEntry( vTrace, r, iPatThis );
    }
    // create counter-example
    pCex = Abc_CexAlloc( Aig_ManRegNum(p->pAig), Saig_ManPiNum(p->pAig), iFrame+1 );
    pCex->iFrame = iFrame;
    pCex->iPo = iPo;
    // insert the bits
    iBit = Aig_ManRegNum(p->pAig);
    for ( f = 0; f <= iFrame; f++ )
    {
        Ssw_RarManAssingRandomPis( p );
        iPatThis = Vec_IntEntry( vTrace, f / p->pPars->nFrames );
        Saig_ManForEachPi( p->pAig, pObj, i )
        {
            pSim = Ssw_RarObjSim( p, Aig_ObjId(pObj) );
            if ( Abc_InfoHasBit( (unsigned *)pSim, iPatThis ) )
                Abc_InfoSetBit( pCex->pData, iBit );
            iBit++;
        }
    }
    Vec_IntFree( vTrace );
    assert( iBit == pCex->nBits );
    // verify the counter example
    if ( !Saig_ManVerifyCex( p->pAig, pCex ) )
    {
        Abc_Print( 1, "Ssw_RarDeriveCex(): Counter-example is invalid.\n" );
//        Abc_CexFree( pCex );
//        pCex = NULL;
    }
    else
    {
//      Abc_Print( 1, "Counter-example verification is successful.\n" );
    }
    return pCex;
}


/**Function*************************************************************

  Synopsis    [Transposing 32-bit matrix.]

  Description [Borrowed from "Hacker's Delight", by Henry Warren.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void transpose32( unsigned A[32] )
{
    int j, k;
    unsigned t, m = 0x0000FFFF;
    for ( j = 16; j != 0; j = j >> 1, m = m ^ (m << j) )
    {
        for ( k = 0; k < 32; k = (k + j + 1) & ~j )
        {
            t = (A[k] ^ (A[k+j] >> j)) & m;
            A[k] = A[k] ^ t;
            A[k+j] = A[k+j] ^ (t << j);
        }
    }
}

/**Function*************************************************************

  Synopsis    [Transposing 64-bit matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void transpose64( word A[64] )
{
    int j, k;
    word t, m = 0x00000000FFFFFFFF;
    for ( j = 32; j != 0; j = j >> 1, m = m ^ (m << j) )
    {
        for ( k = 0; k < 64; k = (k + j + 1) & ~j )
        {
            t = (A[k] ^ (A[k+j] >> j)) & m;
            A[k] = A[k] ^ t;
            A[k+j] = A[k+j] ^ (t << j);
        }
    }
}

/**Function*************************************************************

  Synopsis    [Transposing 64-bit matrix.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void transpose64Simple( word A[64], word B[64] )
{
    int i, k;
    for ( i = 0; i < 64; i++ )
        B[i] = 0;
    for ( i = 0; i < 64; i++ )
    for ( k = 0; k < 64; k++ )
        if ( (A[i] >> k) & 1 )
            B[k] |= ((word)1 << (63-i));
}

/**Function*************************************************************

  Synopsis    [Testing the transposing code.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void TransposeTest()
{
    word M[64], N[64];
    int i;
    abctime clk;
    Aig_ManRandom64( 1 );
//    for ( i = 0; i < 64; i++ )
//        M[i] = Aig_ManRandom64( 0 );
    for ( i = 0; i < 64; i++ )
        M[i] = i? (word)0 : ~(word)0;
//    for ( i = 0; i < 64; i++ )
//        Extra_PrintBinary( stdout, (unsigned *)&M[i], 64 ), Abc_Print( 1, "\n" );

    clk = Abc_Clock();
    for ( i = 0; i < 100001; i++ )
        transpose64Simple( M, N );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    clk = Abc_Clock();
    for ( i = 0; i < 100001; i++ )
        transpose64( M );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    for ( i = 0; i < 64; i++ )
        if ( M[i] != N[i] )
            Abc_Print( 1, "Mismatch\n" );
/*
    Abc_Print( 1, "\n" );
    for ( i = 0; i < 64; i++ )
        Extra_PrintBinary( stdout, (unsigned *)&M[i], 64 ), Abc_Print( 1, "\n" );
    Abc_Print( 1, "\n" );
    for ( i = 0; i < 64; i++ )
        Extra_PrintBinary( stdout, (unsigned *)&N[i], 64 ), Abc_Print( 1, "\n" );
*/
}

/**Function*************************************************************

  Synopsis    [Transposing pObjData[ nRegs x nWords ] -> pPatData[ nWords x nRegs ].]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_RarTranspose( Ssw_RarMan_t * p )
{
    Aig_Obj_t * pObj;
    word M[64];
    int w, r, i;
    for ( w = 0; w < p->pPars->nWords; w++ )
    for ( r = 0; r < p->nWordsReg; r++ )
    {
        // save input
        for ( i = 0; i < 64; i++ )
        {
            if ( r*64 + 63-i < Aig_ManRegNum(p->pAig) )
            {
                pObj = Saig_ManLi( p->pAig, r*64 + 63-i );
                M[i] = Ssw_RarObjSim( p, Aig_ObjId(pObj) )[w];
            }
            else
                M[i] = 0;
        }
        // transpose
        transpose64( M );
        // save output
        for ( i = 0; i < 64; i++ )
            Ssw_RarPatSim( p, w*64 + 63-i )[r] = M[i];
    }
/*
    Saig_ManForEachLi( p->pAig, pObj, i )
    {
        word * pBitData = Ssw_RarObjSim( p, Aig_ObjId(pObj) );
        Extra_PrintBinary( stdout, (unsigned *)pBitData, 64*p->pPars->nWords ); Abc_Print( 1, "\n" );
    }
    Abc_Print( 1, "\n" );
    for ( i = 0; i < p->pPars->nWords*64; i++ )
    {
        word * pBitData = Ssw_RarPatSim( p, i );
        Extra_PrintBinary( stdout, (unsigned *)pBitData, Aig_ManRegNum(p->pAig) ); Abc_Print( 1, "\n" );
    }
    Abc_Print( 1, "\n" );
*/
}




/**Function*************************************************************

  Synopsis    [Sets random inputs and specialied flop outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_RarManInitialize( Ssw_RarMan_t * p, Vec_Int_t * vInit )
{
    Aig_Obj_t * pObj, * pObjLi;
    word * pSim, * pSimLi;
    int w, i;
    // constant
    pObj = Aig_ManConst1( p->pAig );
    pSim = Ssw_RarObjSim( p, Aig_ObjId(pObj) );
    for ( w = 0; w < p->pPars->nWords; w++ )
        pSim[w] = ~(word)0;
    // primary inputs
    Ssw_RarManAssingRandomPis( p );
    // flop outputs
    if ( vInit )
    {
        assert( Vec_IntSize(vInit) == Saig_ManRegNum(p->pAig) * p->pPars->nWords );
        Saig_ManForEachLo( p->pAig, pObj, i )
        {
            pSim = Ssw_RarObjSim( p, Aig_ObjId(pObj) );
            for ( w = 0; w < p->pPars->nWords; w++ )
                pSim[w] = Vec_IntEntry(vInit, w * Saig_ManRegNum(p->pAig) + i) ? ~(word)0 : (word)0;
        }
    }
    else
    {
        Saig_ManForEachLiLo( p->pAig, pObjLi, pObj, i )
        {
            pSimLi = Ssw_RarObjSim( p, Aig_ObjId(pObjLi) );
            pSim   = Ssw_RarObjSim( p, Aig_ObjId(pObj) );
            for ( w = 0; w < p->pPars->nWords; w++ )
                pSim[w] = pSimLi[w];
        }
    }
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_RarManPoIsConst0( void * pMan, Aig_Obj_t * pObj )
{
    Ssw_RarMan_t * p = (Ssw_RarMan_t *)pMan;
    word * pSim = Ssw_RarObjSim( p, Aig_ObjId(pObj) );
    int w;
    for ( w = 0; w < p->pPars->nWords; w++ )
        if ( pSim[w] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_RarManObjIsConst( void * pMan, Aig_Obj_t * pObj )
{
    Ssw_RarMan_t * p = (Ssw_RarMan_t *)pMan;
    word * pSim = Ssw_RarObjSim( p, Aig_ObjId(pObj) );
    word Flip = pObj->fPhase ? ~(word)0 : 0;
    int w;
    for ( w = 0; w < p->pPars->nWords; w++ )
        if ( pSim[w] ^ Flip )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation infos are equal.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_RarManObjsAreEqual( void * pMan, Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 )
{
    Ssw_RarMan_t * p = (Ssw_RarMan_t *)pMan;
    word * pSim0 = Ssw_RarObjSim( p, pObj0->Id );
    word * pSim1 = Ssw_RarObjSim( p, pObj1->Id );
    word Flip = (pObj0->fPhase != pObj1->fPhase) ? ~(word)0 : 0;
    int w;
    for ( w = 0; w < p->pPars->nWords; w++ )
        if ( pSim0[w] ^ pSim1[w] ^ Flip )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes hash value of the node using its simulation info.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Ssw_RarManObjHashWord( void * pMan, Aig_Obj_t * pObj )
{
    Ssw_RarMan_t * p = (Ssw_RarMan_t *)pMan;
    static int s_SPrimes[128] = {
        1009, 1049, 1093, 1151, 1201, 1249, 1297, 1361, 1427, 1459,
        1499, 1559, 1607, 1657, 1709, 1759, 1823, 1877, 1933, 1997,
        2039, 2089, 2141, 2213, 2269, 2311, 2371, 2411, 2467, 2543,
        2609, 2663, 2699, 2741, 2797, 2851, 2909, 2969, 3037, 3089,
        3169, 3221, 3299, 3331, 3389, 3461, 3517, 3557, 3613, 3671,
        3719, 3779, 3847, 3907, 3943, 4013, 4073, 4129, 4201, 4243,
        4289, 4363, 4441, 4493, 4549, 4621, 4663, 4729, 4793, 4871,
        4933, 4973, 5021, 5087, 5153, 5227, 5281, 5351, 5417, 5471,
        5519, 5573, 5651, 5693, 5749, 5821, 5861, 5923, 6011, 6073,
        6131, 6199, 6257, 6301, 6353, 6397, 6481, 6563, 6619, 6689,
        6737, 6803, 6863, 6917, 6977, 7027, 7109, 7187, 7237, 7309,
        7393, 7477, 7523, 7561, 7607, 7681, 7727, 7817, 7877, 7933,
        8011, 8039, 8059, 8081, 8093, 8111, 8123, 8147
    };
    unsigned * pSims;
    unsigned uHash;
    int i;
    uHash = 0;
    pSims = (unsigned *)Ssw_RarObjSim( p, pObj->Id );
    for ( i = 0; i < 2 * p->pPars->nWords; i++ )
        uHash ^= pSims[i] * s_SPrimes[i & 0x7F];
    return uHash;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_RarManObjWhichOne( Ssw_RarMan_t * p, Aig_Obj_t * pObj )
{
    word * pSim = Ssw_RarObjSim( p, Aig_ObjId(pObj) );
    word Flip = 0;//pObj->fPhase ? ~(word)0 : 0; // bug fix!
    int w, i;
    for ( w = 0; w < p->pPars->nWords; w++ )
        if ( pSim[w] ^ Flip )
        {
            for ( i = 0; i < 64; i++ )
                if ( ((pSim[w] ^ Flip) >> i) & 1 )
                    break;
            assert( i < 64 );
            return w * 64 + i;
        }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Check if any of the POs becomes non-constant.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_RarManCheckNonConstOutputs( Ssw_RarMan_t * p, int iFrame, abctime Time )
{
    Aig_Obj_t * pObj;
    int i;
    p->iFailPo  = -1;
    p->iFailPat = -1;
    Saig_ManForEachPo( p->pAig, pObj, i )
    {
        if ( p->pAig->nConstrs && i >= Saig_ManPoNum(p->pAig) - p->pAig->nConstrs )
            break;
        if ( p->vCexes && Vec_PtrEntry(p->vCexes, i) )
            continue;
        if ( Ssw_RarManPoIsConst0(p, pObj) )
            continue;
        p->iFailPo  = i;
        p->iFailPat = Ssw_RarManObjWhichOne( p, pObj );
        if ( !p->pPars->fSolveAll )
            break;
        // remember the one solved
        p->pPars->nSolved++;
        if ( p->vCexes == NULL )
            p->vCexes = Vec_PtrStart( Saig_ManPoNum(p->pAig) );
        assert( Vec_PtrEntry(p->vCexes, i) == NULL );
        Vec_PtrWriteEntry( p->vCexes, i, (void *)(ABC_PTRINT_T)1 );
        if ( p->pPars->pFuncOnFail && p->pPars->pFuncOnFail(i, NULL) )
            return 2; // quitting due to callback
        // print final report
        if ( !p->pPars->fNotVerbose )
        {
            int nOutDigits = Abc_Base10Log( Saig_ManPoNum(p->pAig) );
            Abc_Print( 1, "Output %*d was asserted in frame %4d (solved %*d out of %*d outputs).  ", 
                nOutDigits, p->iFailPo, iFrame, 
                nOutDigits, p->pPars->nSolved, 
                nOutDigits, Saig_ManPoNum(p->pAig) );
            Abc_PrintTime( 1, "Time", Time );
        }
    }
    if ( p->iFailPo >= 0 ) // found CEX
        return 1;
    else
        return 0;
}

/**Function*************************************************************

  Synopsis    [Performs one round of simulation.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_RarManSimulate( Ssw_RarMan_t * p, Vec_Int_t * vInit, int fUpdate, int fFirst )
{
    Aig_Obj_t * pObj, * pRepr;
    word * pSim, * pSim0, * pSim1;
    word Flip, Flip0, Flip1;
    int w, i;
    // initialize
    Ssw_RarManInitialize( p, vInit );
    Vec_PtrClear( p->vUpdConst );
    Vec_PtrClear( p->vUpdClass );
    Aig_ManIncrementTravId( p->pAig );
    // check comb inputs
    if ( fUpdate )
    Aig_ManForEachCi( p->pAig, pObj, i )
    {
        pRepr = Aig_ObjRepr(p->pAig, pObj);
        if ( pRepr == NULL || Aig_ObjIsTravIdCurrent( p->pAig, pRepr ) )
            continue;
        if ( Ssw_RarManObjsAreEqual( p, pObj, pRepr ) )
            continue;
        // save for update
        if ( pRepr == Aig_ManConst1(p->pAig) )
            Vec_PtrPush( p->vUpdConst, pObj );
        else
        {
            Vec_PtrPush( p->vUpdClass, pRepr );
            Aig_ObjSetTravIdCurrent( p->pAig, pRepr );
        }
    }
    // simulate
    Aig_ManForEachNode( p->pAig, pObj, i )
    {
        pSim  = Ssw_RarObjSim( p, Aig_ObjId(pObj) );
        pSim0 = Ssw_RarObjSim( p, Aig_ObjFaninId0(pObj) );
        pSim1 = Ssw_RarObjSim( p, Aig_ObjFaninId1(pObj) );
        Flip0 = Aig_ObjFaninC0(pObj) ? ~(word)0 : 0;
        Flip1 = Aig_ObjFaninC1(pObj) ? ~(word)0 : 0;
        for ( w = 0; w < p->pPars->nWords; w++ )
            pSim[w] = (Flip0 ^ pSim0[w]) & (Flip1 ^ pSim1[w]);


        if ( !fUpdate )
            continue;
        // check classes
        pRepr = Aig_ObjRepr(p->pAig, pObj);
        if ( pRepr == NULL || Aig_ObjIsTravIdCurrent( p->pAig, pRepr ) )
            continue;
        if ( Ssw_RarManObjsAreEqual( p, pObj, pRepr ) )
            continue;
        // save for update
        if ( pRepr == Aig_ManConst1(p->pAig) )
            Vec_PtrPush( p->vUpdConst, pObj );
        else
        {
            Vec_PtrPush( p->vUpdClass, pRepr );
            Aig_ObjSetTravIdCurrent( p->pAig, pRepr );
        }
    }
    // transfer to POs
    Aig_ManForEachCo( p->pAig, pObj, i )
    {
        pSim  = Ssw_RarObjSim( p, Aig_ObjId(pObj) );
        pSim0 = Ssw_RarObjSim( p, Aig_ObjFaninId0(pObj) );
        Flip  = Aig_ObjFaninC0(pObj) ? ~(word)0 : 0;
        for ( w = 0; w < p->pPars->nWords; w++ )
            pSim[w] = Flip ^ pSim0[w];
    }
    // refine classes
    if ( fUpdate )
    {
        if ( fFirst )
        {
            Vec_Ptr_t * vCands = Vec_PtrAlloc( 1000 );
            Aig_ManForEachObj( p->pAig, pObj, i )
                if ( Ssw_ObjIsConst1Cand( p->pAig, pObj ) )
                    Vec_PtrPush( vCands, pObj );
            assert( Vec_PtrSize(vCands) == Ssw_ClassesCand1Num(p->ppClasses) );
            Ssw_ClassesPrepareRehash( p->ppClasses, vCands, 0 );
            Vec_PtrFree( vCands );
        }
        else
        {
            Ssw_ClassesRefineConst1Group( p->ppClasses, p->vUpdConst, 1 );
            Ssw_ClassesRefineGroup( p->ppClasses, p->vUpdClass, 1 );
        }
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static Ssw_RarMan_t * Ssw_RarManStart( Aig_Man_t * pAig, Ssw_RarPars_t * pPars )
{
    Ssw_RarMan_t * p;
//    if ( Aig_ManRegNum(pAig) < nBinSize || nBinSize <= 0 )
//        return NULL;
    p = ABC_CALLOC( Ssw_RarMan_t, 1 );
    p->pAig      = pAig;
    p->pPars     = pPars;
    p->nGroups   = Aig_ManRegNum(pAig) / pPars->nBinSize;
    p->pRarity   = ABC_CALLOC( int, (1 << pPars->nBinSize) * p->nGroups );
    p->pPatCosts = ABC_CALLOC( double, p->pPars->nWords * 64 );
    p->nWordsReg = Ssw_RarBitWordNum( Aig_ManRegNum(pAig) );
    p->pObjData  = ABC_ALLOC( word, Aig_ManObjNumMax(pAig) * p->pPars->nWords );
    p->pPatData  = ABC_ALLOC( word, 64 * p->pPars->nWords * p->nWordsReg );
    p->vUpdConst = Vec_PtrAlloc( 100 );
    p->vUpdClass = Vec_PtrAlloc( 100 );
    p->vPatBests = Vec_IntAlloc( 100 );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Ssw_RarManStop( Ssw_RarMan_t * p )
{
//    Vec_PtrFreeP( &p->vCexes );
    if ( p->vCexes )
    {
        assert( p->pAig->vSeqModelVec == NULL );
        p->pAig->vSeqModelVec = p->vCexes;
        p->vCexes = NULL;
    }
    if ( p->ppClasses ) Ssw_ClassesStop( p->ppClasses );
    Vec_IntFreeP( &p->vInits );
    Vec_IntFreeP( &p->vPatBests );
    Vec_PtrFreeP( &p->vUpdConst );
    Vec_PtrFreeP( &p->vUpdClass );
    ABC_FREE( p->pObjData );
    ABC_FREE( p->pPatData );
    ABC_FREE( p->pPatCosts );
    ABC_FREE( p->pRarity );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Select best patterns.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Ssw_RarTransferPatterns( Ssw_RarMan_t * p, Vec_Int_t * vInits )
{
//    Aig_Obj_t * pObj;
    unsigned char * pData;
    unsigned * pPattern;
    int i, k, Value;

    // more data from regs to pats
    Ssw_RarTranspose( p );

    // update counters
    for ( k = 0; k < p->pPars->nWords * 64; k++ )
    {
        pData = (unsigned char *)Ssw_RarPatSim( p, k );
        for ( i = 0; i < p->nGroups; i++ )
            Ssw_RarAddToBinPat( p, i, pData[i] );
    }

    // for each pattern
    for ( k = 0; k < p->pPars->nWords * 64; k++ )
    {
        pData = (unsigned char *)Ssw_RarPatSim( p, k );
        // find the cost of its values
        p->pPatCosts[k] = 0.0;
        for ( i = 0; i < p->nGroups; i++ )
        {
            Value = Ssw_RarGetBinPat( p, i, pData[i] );
            assert( Value > 0 );
            p->pPatCosts[k] += 1.0/(Value*Value);
        }
        // print the result
//Abc_Print( 1, "%3d : %9.6f\n", k, p->pPatCosts[k] );
    }

    // choose as many as there are words
    Vec_IntClear( vInits );
    for ( i = 0; i < p->pPars->nWords; i++ )
    {
        // select the best
        int iPatBest = -1;
        double iCostBest = -ABC_INFINITY;
        for ( k = 0; k < p->pPars->nWords * 64; k++ )
            if ( iCostBest < p->pPatCosts[k] )
            {
                iCostBest = p->pPatCosts[k];
                iPatBest  = k;
            }
        // remove from costs
        assert( iPatBest >= 0 );
        p->pPatCosts[iPatBest] = -ABC_INFINITY;
        // set the flops
        pPattern = (unsigned *)Ssw_RarPatSim( p, iPatBest );
        for ( k = 0; k < Aig_ManRegNum(p->pAig); k++ )
            Vec_IntPush( vInits, Abc_InfoHasBit(pPattern, k) );
//Abc_Print( 1, "Best pattern %5d\n", iPatBest );
        Vec_IntPush( p->vPatBests, iPatBest );
    }
    assert( Vec_IntSize(vInits) == Aig_ManRegNum(p->pAig) * p->pPars->nWords );
}


/**Function*************************************************************

  Synopsis    [Performs fraiging for one node.]

  Description [Returns the fraiged node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Vec_Int_t * Ssw_RarFindStartingState( Aig_Man_t * pAig, Abc_Cex_t * pCex )
{
    Vec_Int_t * vInit;
    Aig_Obj_t * pObj, * pObjLi;
    int f, i, iBit;
    // assign register outputs
    Saig_ManForEachLi( pAig, pObj, i )
        pObj->fMarkB = Abc_InfoHasBit( pCex->pData, i );
    // simulate the timeframes
    iBit = pCex->nRegs;
    for ( f = 0; f <= pCex->iFrame; f++ )
    {
        // set the PI simulation information
        Aig_ManConst1(pAig)->fMarkB = 1;
        Saig_ManForEachPi( pAig, pObj, i )
            pObj->fMarkB = Abc_InfoHasBit( pCex->pData, iBit++ );
        Saig_ManForEachLiLo( pAig, pObjLi, pObj, i )
            pObj->fMarkB = pObjLi->fMarkB;
        // simulate internal nodes
        Aig_ManForEachNode( pAig, pObj, i )
            pObj->fMarkB = ( Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj) )
                         & ( Aig_ObjFanin1(pObj)->fMarkB ^ Aig_ObjFaninC1(pObj) );
        // assign the COs
        Aig_ManForEachCo( pAig, pObj, i )
            pObj->fMarkB = ( Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj) );
    }
    assert( iBit == pCex->nBits );
    // check that the output failed as expected -- cannot check because it is not an SRM!
//    pObj = Aig_ManCo( pAig, pCex->iPo );
//    if ( pObj->fMarkB != 1 )
//        Abc_Print( 1, "The counter-example does not refine the output.\n" );
    // record the new pattern
    vInit = Vec_IntAlloc( Saig_ManRegNum(pAig) );
    Saig_ManForEachLo( pAig, pObj, i )
    {
//Abc_Print( 1, "%d", pObj->fMarkB );
        Vec_IntPush( vInit, pObj->fMarkB );
    }
//Abc_Print( 1, "\n" );
    Aig_ManCleanMarkB( pAig );
    return vInit;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_RarCheckTrivial( Aig_Man_t * pAig, int fVerbose )
{
    Aig_Obj_t * pObj;
    int i;
    Saig_ManForEachPo( pAig, pObj, i )
    {
        if ( pAig->nConstrs && i >= Saig_ManPoNum(pAig) - pAig->nConstrs )
            return 0;
        if ( pObj->fPhase )
        {
            ABC_FREE( pAig->pSeqModel );
            pAig->pSeqModel = Abc_CexAlloc( Aig_ManRegNum(pAig), Saig_ManPiNum(pAig), 1 );
            pAig->pSeqModel->iPo = i;
            if ( fVerbose )
                Abc_Print( 1, "Output %d is trivally SAT in frame 0. \n", i );
            return 1;
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Perform sequential simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_RarSimulate( Aig_Man_t * pAig, Ssw_RarPars_t * pPars )
{
    int fTryBmc = 0;
    int fMiter = 1;
    Ssw_RarMan_t * p;
    int r, f = -1;
    abctime clk, clkTotal = Abc_Clock();
    abctime nTimeToStop = pPars->TimeOut ? pPars->TimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0;
    abctime timeLastSolved = 0;
    int nNumRestart = 0;
    int nSavedSeed = pPars->nRandSeed;
    int RetValue = -1;
    int iFrameFail = -1;
    assert( Aig_ManRegNum(pAig) > 0 );
    assert( Aig_ManConstrNum(pAig) == 0 );
    ABC_FREE( pAig->pSeqModel );
    // consider the case of empty AIG
//    if ( Aig_ManNodeNum(pAig) == 0 )
//        return -1;
    // check trivially SAT miters
//    if ( fMiter && Ssw_RarCheckTrivial( pAig, fVerbose ) )
//        return 0;
    if ( pPars->fVerbose )
        Abc_Print( 1, "Rarity simulation with %d words, %d frames, %d rounds, %d restart, %d seed, and %d sec timeout.\n",
            pPars->nWords, pPars->nFrames, pPars->nRounds, pPars->nRestart, pPars->nRandSeed, pPars->TimeOut );
    // reset random numbers
    Ssw_RarManPrepareRandom( nSavedSeed );

    // create manager
    p = Ssw_RarManStart( pAig, pPars );
    p->vInits = Vec_IntStart( Aig_ManRegNum(pAig) * pPars->nWords );

    // perform simulation rounds
    pPars->nSolved = 0;
    timeLastSolved = Abc_Clock();
    for ( r = 0; !pPars->nRounds || (nNumRestart * pPars->nRestart + r < pPars->nRounds); r++ )
    {
        clk = Abc_Clock();
        if ( fTryBmc )
        {
            Aig_Man_t * pNewAig = Saig_ManDupWithPhase( pAig, p->vInits );
            Saig_BmcPerform( pNewAig, 0, 100, 2000, 3, 0, 0, 1 /*fVerbose*/, 0, &iFrameFail, 0, 0 );
//            if ( pNewAig->pSeqModel != NULL )
//                Abc_Print( 1, "BMC has found a counter-example in frame %d.\n", iFrameFail );
            Aig_ManStop( pNewAig );
        }
        // simulate
        for ( f = 0; f < pPars->nFrames; f++ )
        {
            Ssw_RarManSimulate( p, f ? NULL : p->vInits, 0, 0 );
            if ( fMiter )
            {
                int Status = Ssw_RarManCheckNonConstOutputs(p, r * p->pPars->nFrames + f, Abc_Clock() - clkTotal);
                if ( Status == 2 )
                {
                    Abc_Print( 1, "Quitting due to callback on fail.\n" );
                    goto finish;
                }
                if ( Status == 1 ) // found CEX
                {
                    RetValue = 0;
                    if ( !pPars->fSolveAll )
                    {
                        if ( pPars->fVerbose ) Abc_Print( 1, "\n" );
        //                Abc_Print( 1, "Simulation asserted a PO in frame f: %d <= f < %d.\n", r * nFrames, (r+1) * nFrames );
                        Ssw_RarManPrepareRandom( nSavedSeed );
                        if ( pPars->fVerbose )
                            Abc_Print( 1, "Simulated %d frames for %d rounds with %d restarts.\n", pPars->nFrames, nNumRestart * pPars->nRestart + r, nNumRestart );
                        pAig->pSeqModel = Ssw_RarDeriveCex( p, r * p->pPars->nFrames + f, p->iFailPo, p->iFailPat, pPars->fVerbose );
                        // print final report
                        if ( !pPars->fSilent ) {
                            Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d.  ", pAig->pSeqModel->iPo, pAig->pName, pAig->pSeqModel->iFrame );
                            Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
                        }
                        goto finish;
                    }
                    timeLastSolved = Abc_Clock();
                }
                // else - did not find a counter example
            }
            // check timeout
            if ( pPars->TimeOut && Abc_Clock() > nTimeToStop )
            {
                if ( !pPars->fSilent )
                {
                if ( pPars->fVerbose && !pPars->fSolveAll ) Abc_Print( 1, "\n" );
                Abc_Print( 1, "Simulated %d frames for %d rounds with %d restarts and solved %d outputs.  ", pPars->nFrames, nNumRestart * pPars->nRestart + r, nNumRestart, pPars->nSolved );
                Abc_Print( 1, "Reached timeout (%d sec).\n",  pPars->TimeOut );
                }
                goto finish;
            }
            if ( pPars->TimeOutGap && timeLastSolved && Abc_Clock() > timeLastSolved + pPars->TimeOutGap * CLOCKS_PER_SEC )
            {
                if ( !pPars->fSilent )
                {
                if ( pPars->fVerbose && !pPars->fSolveAll ) Abc_Print( 1, "\n" );
                Abc_Print( 1, "Simulated %d frames for %d rounds with %d restarts and solved %d outputs.  ", pPars->nFrames, nNumRestart * pPars->nRestart + r, nNumRestart, pPars->nSolved );
                Abc_Print( 1, "Reached gap timeout (%d sec).\n",  pPars->TimeOutGap );
                }
                goto finish;
            }
            // check if all outputs are solved by now
            if ( pPars->fSolveAll && p->vCexes && Vec_PtrCountZero(p->vCexes) == 0 )
                goto finish;
        }
        // get initialization patterns
        if ( pPars->nRestart && r == pPars->nRestart )
        {
            r = -1;
            nSavedSeed = (nSavedSeed + 1) % 1000;
            Ssw_RarManPrepareRandom( nSavedSeed );
            Vec_IntFill( p->vInits, Aig_ManRegNum(pAig) * pPars->nWords, 0 );
            nNumRestart++;
            Vec_IntClear( p->vPatBests );
            // clean rarity info
//            memset( p->pRarity, 0, sizeof(int) * (1 << nBinSize) * p->nGroups );
        }
        else
            Ssw_RarTransferPatterns( p, p->vInits );
        // printout
        if ( pPars->fVerbose )
        {
            if ( pPars->fSolveAll )
            {
                Abc_Print( 1, "Starts =%6d   ",  nNumRestart );
                Abc_Print( 1, "Rounds =%6d   ",  nNumRestart * pPars->nRestart + ((r==-1)?0:r) );
                Abc_Print( 1, "Frames =%6d   ", (nNumRestart * pPars->nRestart + r) * pPars->nFrames );
                Abc_Print( 1, "CEX =%6d (%6.2f %%)   ", pPars->nSolved, 100.0*pPars->nSolved/Saig_ManPoNum(p->pAig) );
                Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
            }
            else
                Abc_Print( 1, "." );
        }

    }
finish:
    if ( pPars->fSetLastState && p->vInits )
    {
        assert( Vec_IntSize(p->vInits) % Aig_ManRegNum(pAig) == 0 );
        Vec_IntShrink( p->vInits, Aig_ManRegNum(pAig) );
        pAig->pData = p->vInits;  p->vInits = NULL;
    }
    if ( pPars->nSolved )
    {
/*
        if ( !pPars->fSilent )
        {
        if ( pPars->fVerbose && !pPars->fSolveAll ) Abc_Print( 1, "\n" );
        Abc_Print( 1, "Simulation of %d frames for %d rounds with %d restarts asserted %d (out of %d) POs.    ", pPars->nFrames, nNumRestart * pPars->nRestart + r, nNumRestart, pPars->nSolved, Saig_ManPoNum(p->pAig) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
        }
*/
    }
    else if ( r == pPars->nRounds && f == pPars->nFrames )
    {
        if ( !pPars->fSilent )
        {
        if ( pPars->fVerbose ) Abc_Print( 1, "\n" );
        Abc_Print( 1, "Simulation of %d frames for %d rounds with %d restarts did not assert POs.    ", pPars->nFrames, nNumRestart * pPars->nRestart + r, nNumRestart );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
        }
    }
    // cleanup
    Ssw_RarManStop( p );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Derive random flop permutation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Ssw_RarRandomPermFlop( int nFlops, int nUnused )
{
    Vec_Int_t * vPerm;
    int i, k, * pArray;
    srand( 1 );
    printf( "Generating random permutation of %d flops.\n", nFlops );
    vPerm = Vec_IntStartNatural( nFlops );
    pArray = Vec_IntArray( vPerm );
    for ( i = 0; i < nFlops; i++ )
    {
        k = rand() % nFlops;
        ABC_SWAP( int, pArray[i], pArray[k] );
    }
    printf( "Randomly adding %d unused flops.\n", nUnused );
    for ( i = 0; i < nUnused; i++ )
    {
        k = rand() % Vec_IntSize(vPerm);
        Vec_IntPush( vPerm, -1 );
        pArray = Vec_IntArray( vPerm );
        ABC_SWAP( int, pArray[Vec_IntSize(vPerm)-1], pArray[k] );
    }
//    Vec_IntPrint(vPerm);
    return vPerm;
}

/**Function*************************************************************

  Synopsis    [Perform sequential simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_RarSimulateGia( Gia_Man_t * p, Ssw_RarPars_t * pPars )
{
    Aig_Man_t * pAig;
    int RetValue;
    if ( pPars->fUseFfGrouping )
    {
        Vec_Int_t * vPerm = Ssw_RarRandomPermFlop( Gia_ManRegNum(p), 10 );
        Gia_Man_t * pTemp = Gia_ManDupPermFlopGap( p, vPerm );
        Vec_IntFree( vPerm );
        pAig = Gia_ManToAigSimple( pTemp );
        Gia_ManStop( pTemp );
    }
    else
        pAig = Gia_ManToAigSimple( p );
    RetValue = Ssw_RarSimulate( pAig, pPars );
    // save counter-example
    Abc_CexFree( p->pCexSeq );
    p->pCexSeq = pAig->pSeqModel; pAig->pSeqModel = NULL;
    Aig_ManStop( pAig );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Filter equivalence classes of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_RarSignalFilter( Aig_Man_t * pAig, Ssw_RarPars_t * pPars )
{
    Ssw_RarMan_t * p;
    int r, f = -1, i, k;
    abctime clkTotal = Abc_Clock();
    abctime nTimeToStop = pPars->TimeOut ? pPars->TimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0;
    int nNumRestart = 0;
    int nSavedSeed = pPars->nRandSeed;
    int RetValue = -1;
    assert( Aig_ManRegNum(pAig) > 0 );
    assert( Aig_ManConstrNum(pAig) == 0 );
    // consider the case of empty AIG
    if ( Aig_ManNodeNum(pAig) == 0 )
        return -1;
    // check trivially SAT miters
    if ( pPars->fMiter && Ssw_RarCheckTrivial( pAig, 1 ) )
        return 0;
    if ( pPars->fVerbose )
        Abc_Print( 1, "Rarity equiv filtering with %d words, %d frames, %d rounds, %d seed, and %d sec timeout.\n",
            pPars->nWords, pPars->nFrames, pPars->nRounds, pPars->nRandSeed, pPars->TimeOut );
    // reset random numbers
    Ssw_RarManPrepareRandom( nSavedSeed );

    // create manager
    p = Ssw_RarManStart( pAig, pPars );
    // compute starting state if needed
    assert( p->vInits == NULL );
    if ( pPars->pCex )
    {
        p->vInits = Ssw_RarFindStartingState( pAig, pPars->pCex );
        Abc_Print( 1, "Beginning simulation from the state derived using the counter-example.\n" );
    }
    else
        p->vInits = Vec_IntStart( Aig_ManRegNum(pAig) );
    // duplicate the array
    for ( i = 1; i < pPars->nWords; i++ )
        for ( k = 0; k < Aig_ManRegNum(pAig); k++ )
            Vec_IntPush( p->vInits, Vec_IntEntry(p->vInits, k) );
    assert( Vec_IntSize(p->vInits) == Aig_ManRegNum(pAig) * pPars->nWords );

    // create trivial equivalence classes with all nodes being candidates for constant 1
    if ( pAig->pReprs == NULL )
        p->ppClasses = Ssw_ClassesPrepareSimple( pAig, pPars->fLatchOnly, 0 );
    else
        p->ppClasses = Ssw_ClassesPrepareFromReprs( pAig );
    Ssw_ClassesSetData( p->ppClasses, p, Ssw_RarManObjHashWord, Ssw_RarManObjIsConst, Ssw_RarManObjsAreEqual );
    // print the stats
    if ( pPars->fVerbose )
    {
        Abc_Print( 1, "Initial  :  " );
        Ssw_ClassesPrint( p->ppClasses, 0 );
    }
    // refine classes using BMC
    for ( r = 0; !pPars->nRounds || (nNumRestart * pPars->nRestart + r < pPars->nRounds); r++ )
    {
        // start filtering equivalence classes
        if ( Ssw_ClassesCand1Num(p->ppClasses) == 0 && Ssw_ClassesClassNum(p->ppClasses) == 0 )
        {
            Abc_Print( 1, "All equivalences are refined away.\n" );
            break;
        }
        // simulate
        for ( f = 0; f < pPars->nFrames; f++ )
        {
            Ssw_RarManSimulate( p, f ? NULL : p->vInits, 1, !r && !f );
            if ( pPars->fMiter && Ssw_RarManCheckNonConstOutputs(p, -1, 0) )
            {
                if ( !pPars->fVerbose )
                    Abc_Print( 1, "%s", Abc_FrameIsBatchMode() ? "\n" : "\r" );
//                Abc_Print( 1, "Simulation asserted a PO in frame f: %d <= f < %d.\n", r * pPars->nFrames, (r+1) * pPars->nFrames );
                if ( pPars->fVerbose )
                    Abc_Print( 1, "Simulated %d frames for %d rounds with %d restarts.\n", pPars->nFrames, nNumRestart * pPars->nRestart + r, nNumRestart );
                Ssw_RarManPrepareRandom( nSavedSeed );
                Abc_CexFree( pAig->pSeqModel );
                pAig->pSeqModel = Ssw_RarDeriveCex( p, r * p->pPars->nFrames + f, p->iFailPo, p->iFailPat, 1 );
                // print final report
                Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d.  ", pAig->pSeqModel->iPo, pAig->pName, pAig->pSeqModel->iFrame );
                Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
                RetValue = 0;
                goto finish;
            }
            // check timeout
            if ( pPars->TimeOut && Abc_Clock() > nTimeToStop )
            {
                if ( pPars->fVerbose ) Abc_Print( 1, "\n" );
                Abc_Print( 1, "Simulated %d frames for %d rounds with %d restarts.  ", pPars->nFrames, nNumRestart * pPars->nRestart + r, nNumRestart );
                Abc_Print( 1, "Reached timeout (%d sec).\n",  pPars->TimeOut );
                goto finish;
            }
        }
        // get initialization patterns
        if ( pPars->pCex == NULL && pPars->nRestart && r == pPars->nRestart )
        {
            r = -1;
            nSavedSeed = (nSavedSeed + 1) % 1000;
            Ssw_RarManPrepareRandom( nSavedSeed );
            Vec_IntFill( p->vInits, Aig_ManRegNum(pAig) * pPars->nWords, 0 );
            nNumRestart++;
            Vec_IntClear( p->vPatBests );
            // clean rarity info
//            memset( p->pRarity, 0, sizeof(int) * (1 << nBinSize) * p->nGroups );
        }
        else
            Ssw_RarTransferPatterns( p, p->vInits );
        // printout
        if ( pPars->fVerbose )
        {
            Abc_Print( 1, "Round %3d:  ", r );
            Ssw_ClassesPrint( p->ppClasses, 0 );
        }
        else
        {
            Abc_Print( 1, "." );
        }
    }
finish:
    // report
    if ( r == pPars->nRounds && f == pPars->nFrames )
    {
        if ( !pPars->fVerbose )
            Abc_Print( 1, "%s", Abc_FrameIsBatchMode() ? "\n" : "\r" );
        Abc_Print( 1, "Simulation of %d frames for %d rounds with %d restarts did not assert POs.    ", pPars->nFrames, nNumRestart * pPars->nRestart + r, nNumRestart );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
    }
    // cleanup
    Ssw_RarManStop( p );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Filter equivalence classes of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_RarSignalFilterGia( Gia_Man_t * p, Ssw_RarPars_t * pPars )
{
    Aig_Man_t * pAig;
    int RetValue;
    pAig = Gia_ManToAigSimple( p );
    if ( p->pReprs != NULL )
    {
        Gia_ManReprToAigRepr2( pAig, p );
        ABC_FREE( p->pReprs );
        ABC_FREE( p->pNexts );
    }
    RetValue = Ssw_RarSignalFilter( pAig, pPars );
    Gia_ManReprFromAigRepr( pAig, p );
    // save counter-example
    Abc_CexFree( p->pCexSeq );
    p->pCexSeq = pAig->pSeqModel; pAig->pSeqModel = NULL;
    Aig_ManStop( pAig );
    return RetValue;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
