/**CFile****************************************************************

  FileName    [saigSimFast.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Fast sequential AIG simulator.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigSimFast.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"

#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// the AIG manager
typedef struct Faig_Man_t_ Faig_Man_t;
struct Faig_Man_t_
{
    // parameters
    int            nPis;
    int            nPos;
    int            nCis;
    int            nCos;
    int            nFfs;
    int            nNos;
    // offsets
    int            nPis1;
    int            nCis1;
    int            nCisNos1;
    int            nCisNosPos1;
    int            nObjs;
    // allocated data
    int            nWords;
    int            pObjs[0];
};

static inline int  Faig_CheckIdPi( Faig_Man_t * p, int i )    { return i >= 1 && i < p->nPis1;                     }
static inline int  Faig_CheckIdLo( Faig_Man_t * p, int i )    { return i >= p->nPis1 && i < p->nCis1;              }
static inline int  Faig_CheckIdNo( Faig_Man_t * p, int i )    { return i >= p->nCis1 && i < p->nCisNos1;           }
static inline int  Faig_CheckIdPo( Faig_Man_t * p, int i )    { return i >= p->nCisNos1 && i < p->nCisNosPos1;     }
static inline int  Faig_CheckIdLi( Faig_Man_t * p, int i )    { return i >= p->nCisNosPos1 && i < p->nObjs;        }
static inline int  Faig_CheckIdCo( Faig_Man_t * p, int i )    { return i >= p->nCisNos1 && i < p->nObjs;           }
static inline int  Faig_CheckIdObj( Faig_Man_t * p, int i )   { return i >= 0 && i < p->nObjs;                     }

static inline int  Faig_ObjIdToNumPi( Faig_Man_t * p, int i ) { assert( Faig_CheckIdPi(p,i) ); return i - 1;              }
static inline int  Faig_ObjIdToNumLo( Faig_Man_t * p, int i ) { assert( Faig_CheckIdLo(p,i) ); return i - p->nPis1;       }
static inline int  Faig_ObjIdToNumNo( Faig_Man_t * p, int i ) { assert( Faig_CheckIdNo(p,i) ); return i - p->nCis1;       }
static inline int  Faig_ObjIdToNumPo( Faig_Man_t * p, int i ) { assert( Faig_CheckIdPo(p,i) ); return i - p->nCisNos1;    }
static inline int  Faig_ObjIdToNumLi( Faig_Man_t * p, int i ) { assert( Faig_CheckIdLi(p,i) ); return i - p->nCisNosPos1; }
static inline int  Faig_ObjIdToNumCo( Faig_Man_t * p, int i ) { assert( Faig_CheckIdCo(p,i) ); return i - p->nCisNos1;    }

static inline int  Faig_ObjLoToLi( Faig_Man_t * p, int i )    { assert( Faig_CheckIdLo(p,i) ); return p->nObjs - (p->nCis1 - i); }
static inline int  Faig_ObjLiToLo( Faig_Man_t * p, int i )    { assert( Faig_CheckIdLi(p,i) ); return p->nCis1 - (p->nObjs - i); }

static inline int  Faig_NodeChild0( Faig_Man_t * p, int n )   { return p->pObjs[n<<1];             }
static inline int  Faig_NodeChild1( Faig_Man_t * p, int n )   { return p->pObjs[(n<<1)+1];         }
static inline int  Faig_CoChild0( Faig_Man_t * p, int n )     { return p->pObjs[(p->nNos<<1)+n];   }
static inline int  Faig_ObjFaninC( int iFan )                 { return iFan & 1;                   }
static inline int  Faig_ObjFanin( int iFan )                  { return iFan >> 1;                  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks if the manager is correct.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Faig_ManIsCorrect( Aig_Man_t * pAig )
{
    return Aig_ManObjNumMax(pAig) == 
        1 + Aig_ManCiNum(pAig) + Aig_ManNodeNum(pAig) + Aig_ManCoNum(pAig);
}

/**Function*************************************************************

  Synopsis    [Creates fast simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Faig_Man_t * Faig_ManAlloc( Aig_Man_t * pAig )
{
    Faig_Man_t * p;
    int nWords;
//    assert( Faig_ManIsCorrect(pAig) );
    nWords = 2 * Aig_ManNodeNum(pAig) + Aig_ManCoNum(pAig);
    p = (Faig_Man_t *)ABC_ALLOC( char, sizeof(Faig_Man_t) + sizeof(int) * nWords );
//printf( "Allocating %7.2f MB.\n", 1.0 * (sizeof(Faig_Man_t) + sizeof(int) * nWords)/(1<<20) );
    memset( p, 0, sizeof(Faig_Man_t) );
    p->nPis   = Aig_ManCiNum(pAig) - Aig_ManRegNum(pAig);
    p->nPos   = Aig_ManCoNum(pAig) - Aig_ManRegNum(pAig);
    p->nCis   = Aig_ManCiNum(pAig);
    p->nCos   = Aig_ManCoNum(pAig);
    p->nFfs   = Aig_ManRegNum(pAig);
    p->nNos   = Aig_ManNodeNum(pAig);
    // offsets
    p->nPis1  = p->nPis + 1;
    p->nCis1  = p->nCis + 1;
    p->nCisNos1 = p->nCis + p->nNos + 1;
    p->nCisNosPos1 = p->nCis + p->nNos + p->nPos + 1;
    p->nObjs  = p->nCis + p->nNos + p->nCos + 1;
    p->nWords = nWords;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates fast simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Faig_Man_t * Faig_ManCreate( Aig_Man_t * pAig )
{
    Faig_Man_t * p;
    Aig_Obj_t * pObj;
    int i, iWord = 0;
    p = Faig_ManAlloc( pAig );
    Aig_ManForEachNode( pAig, pObj, i )
    {
        p->pObjs[iWord++] = (Aig_ObjFaninId0(pObj) << 1) | Aig_ObjFaninC0(pObj);
        p->pObjs[iWord++] = (Aig_ObjFaninId1(pObj) << 1) | Aig_ObjFaninC1(pObj);
    }
    Aig_ManForEachCo( pAig, pObj, i )
        p->pObjs[iWord++] = (Aig_ObjFaninId0(pObj) << 1) | Aig_ObjFaninC0(pObj);
    assert( iWord == p->nWords );
    return p;
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Faig_SimulateNode( Faig_Man_t * p, int Id, unsigned * pSimInfo )
{
    int n = Faig_ObjIdToNumNo( p, Id );
    int iFan0 = Faig_NodeChild0( p, n );
    int iFan1 = Faig_NodeChild1( p, n );
    if ( Faig_ObjFaninC(iFan0) && Faig_ObjFaninC(iFan1) )
        return ~(pSimInfo[Faig_ObjFanin(iFan0)] | pSimInfo[Faig_ObjFanin(iFan1)]);
    if ( Faig_ObjFaninC(iFan0) && !Faig_ObjFaninC(iFan1) )
        return (~pSimInfo[Faig_ObjFanin(iFan0)] & pSimInfo[Faig_ObjFanin(iFan1)]);
    if ( !Faig_ObjFaninC(iFan0) && Faig_ObjFaninC(iFan1) )
        return (pSimInfo[Faig_ObjFanin(iFan0)] & ~pSimInfo[Faig_ObjFanin(iFan1)]);
    // if ( !Faig_ObjFaninC(iFan0) && !Faig_ObjFaninC(iFan1) )
    return (pSimInfo[Faig_ObjFanin(iFan0)] & pSimInfo[Faig_ObjFanin(iFan1)]);
} 

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Faig_SimulateCo( Faig_Man_t * p, int Id, unsigned * pSimInfo )
{
    int n = Faig_ObjIdToNumCo( p, Id );
    int iFan0 = Faig_CoChild0( p, n );
    if ( Faig_ObjFaninC(iFan0) )
        return ~pSimInfo[Faig_ObjFanin(iFan0)];
    // if ( !Faig_ObjFaninC(iFan0) )
    return pSimInfo[Faig_ObjFanin(iFan0)];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Faig_SimulateRandomShift( unsigned uOld )
{
    return (uOld << 16) | ((uOld ^ Aig_ManRandom(0)) & 0xffff);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Faig_SimulateTransferShift( unsigned uOld, unsigned uNew )
{
    return (uOld << 16) | (uNew & 0xffff);
}

/**Function*************************************************************

  Synopsis    [Simulates the timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Faig_ManSimulateFrames( Faig_Man_t * p, int nFrames, int nPref, int fTrans )
{
    int * pNumOnes = ABC_CALLOC( int, p->nObjs );
    unsigned * pSimInfo = ABC_ALLOC( unsigned, p->nObjs );
    int f, i;
//printf( "Allocating %7.2f MB.\n", 1.0 * 4 * p->nObjs/(1<<20) );
//printf( "Allocating %7.2f MB.\n", 1.0 * 4 * p->nObjs/(1<<20) );
    // set constant 1
    pSimInfo[0] = ~0;
    for ( f = 0; f < nFrames; f++ )
    {
        if ( fTrans )
        {
            for ( i = 1; i < p->nPis1; i++ )
                pSimInfo[i] = f? Faig_SimulateRandomShift( pSimInfo[i] ) : Aig_ManRandom( 0 );
            for (      ; i < p->nCis1; i++ )
                pSimInfo[i] = f? Faig_SimulateTransferShift( pSimInfo[i], pSimInfo[Faig_ObjLoToLi(p,i)] ) : 0;
        }
        else
        {
            for ( i = 1; i < p->nPis1; i++ )
                pSimInfo[i] = Aig_ManRandom( 0 );
            for (      ; i < p->nCis1; i++ )
                pSimInfo[i] = f? pSimInfo[Faig_ObjLoToLi(p,i)] : 0;
        }
        for (      ; i < p->nCisNos1; i++ )
            pSimInfo[i] = Faig_SimulateNode( p, i, pSimInfo );
        for (      ; i < p->nObjs; i++ )
            pSimInfo[i] = Faig_SimulateCo( p, i, pSimInfo );
        if ( f < nPref )
            continue;
        if ( fTrans )
        {
            for ( i = 0; i < p->nObjs; i++ )
                pNumOnes[i] += Aig_WordCountOnes( (pSimInfo[i] ^ (pSimInfo[i] >> 16)) & 0xffff );
        }
        else
        {
            for ( i = 0; i < p->nObjs; i++ )
                pNumOnes[i] += Aig_WordCountOnes( pSimInfo[i] );
        }
    }
    ABC_FREE( pSimInfo );
    return pNumOnes;
}

/**Function*************************************************************

  Synopsis    [Computes switching activity of one node.]

  Description [Uses the formula: Switching = 2 * nOnes * nZeros / (nTotal ^ 2) ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Faig_ManComputeSwitching( int nOnes, int nSimWords )
{
    int nTotal = 32 * nSimWords;
    return (float)2.0 * nOnes / nTotal * (nTotal - nOnes) / nTotal;
}

/**Function*************************************************************

  Synopsis    [Computes switching activity of one node.]

  Description [Uses the formula: Switching = 2 * nOnes * nZeros / (nTotal ^ 2) ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Faig_ManComputeProbOne( int nOnes, int nSimWords )
{
    int nTotal = 32 * nSimWords;
    return (float)nOnes / nTotal;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Faig_ManComputeSwitchProbs4( Aig_Man_t * p, int nFrames, int nPref, int fProbOne )
{
    int fTrans = 1;
    Faig_Man_t * pAig;
    Vec_Int_t * vSwitching;
    int * pProbs;
    float * pSwitching;
    int nFramesReal;
    abctime clk;//, clkTotal = Abc_Clock();
    if ( fProbOne )
        fTrans = 0;
    vSwitching = Vec_IntStart( Aig_ManObjNumMax(p) );
    pSwitching = (float *)vSwitching->pArray;
clk = Abc_Clock();
    pAig = Faig_ManCreate( p );
//ABC_PRT( "\nCreation  ", Abc_Clock() - clk );
    Aig_ManRandom( 1 );
    // get the number of  frames to simulate
    // if the parameter "seqsimframes" is defined, use it
    // otherwise, use the given number of frames "nFrames"
    nFramesReal = nFrames;
    if ( Abc_FrameReadFlag("seqsimframes") )
        nFramesReal = atoi( Abc_FrameReadFlag("seqsimframes") );
    if ( nFramesReal <= nPref )
    {
        printf( "The total number of frames (%d) should exceed prefix (%d).\n", nFramesReal, nPref );
        printf( "Setting the total number of frames to be %d.\n", nFrames );
        nFramesReal = nFrames;
    }
//printf( "Simulating %d frames.\n", nFramesReal );
clk = Abc_Clock();
    pProbs = Faig_ManSimulateFrames( pAig, nFramesReal, nPref, fTrans );
//ABC_PRT( "Simulation", Abc_Clock() - clk );
clk = Abc_Clock();
    if ( fTrans )
    {
        Aig_Obj_t * pObj;
        int i, Counter = 0;
        pObj = Aig_ManConst1(p);
        pSwitching[pObj->Id] = Faig_ManComputeProbOne( pProbs[Counter++], (nFramesReal - nPref)/2 );
        Aig_ManForEachCi( p, pObj, i )
            pSwitching[pObj->Id] = Faig_ManComputeProbOne( pProbs[Counter++], (nFramesReal - nPref)/2 );
        Aig_ManForEachNode( p, pObj, i )
            pSwitching[pObj->Id] = Faig_ManComputeProbOne( pProbs[Counter++], (nFramesReal - nPref)/2 );
        Aig_ManForEachCo( p, pObj, i )
            pSwitching[pObj->Id] = Faig_ManComputeProbOne( pProbs[Counter++], (nFramesReal - nPref)/2 );
        assert( Counter == pAig->nObjs );
    }
    else if ( fProbOne )
    {
        Aig_Obj_t * pObj;
        int i, Counter = 0;
        pObj = Aig_ManConst1(p);
        pSwitching[pObj->Id] = Faig_ManComputeProbOne( pProbs[Counter++], nFramesReal - nPref );
        Aig_ManForEachCi( p, pObj, i )
            pSwitching[pObj->Id] = Faig_ManComputeProbOne( pProbs[Counter++], nFramesReal - nPref );
        Aig_ManForEachNode( p, pObj, i )
            pSwitching[pObj->Id] = Faig_ManComputeProbOne( pProbs[Counter++], nFramesReal - nPref );
        Aig_ManForEachCo( p, pObj, i )
            pSwitching[pObj->Id] = Faig_ManComputeProbOne( pProbs[Counter++], nFramesReal - nPref );
        assert( Counter == pAig->nObjs );
    }
    else 
    {
        Aig_Obj_t * pObj;
        int i, Counter = 0;
        pObj = Aig_ManConst1(p);
        pSwitching[pObj->Id] = Faig_ManComputeSwitching( pProbs[Counter++], nFramesReal - nPref );
        Aig_ManForEachCi( p, pObj, i )
            pSwitching[pObj->Id] = Faig_ManComputeSwitching( pProbs[Counter++], nFramesReal - nPref );
        Aig_ManForEachNode( p, pObj, i )
            pSwitching[pObj->Id] = Faig_ManComputeSwitching( pProbs[Counter++], nFramesReal - nPref );
        Aig_ManForEachCo( p, pObj, i )
            pSwitching[pObj->Id] = Faig_ManComputeSwitching( pProbs[Counter++], nFramesReal - nPref );
        assert( Counter == pAig->nObjs );
    }
    ABC_FREE( pProbs );
    ABC_FREE( pAig );
//ABC_PRT( "Switch    ", Abc_Clock() - clk );
//ABC_PRT( "TOTAL     ", Abc_Clock() - clkTotal );
    return vSwitching;
}

/**Function*************************************************************

  Synopsis    [Computes probability of switching (or of being 1).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManComputeSwitchProb3s( Aig_Man_t * p, int nFrames, int nPref, int fProbOne )
{
//    return Faig_ManComputeSwitchProbs( p, nFrames, nPref, fProbOne );
    return NULL;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

