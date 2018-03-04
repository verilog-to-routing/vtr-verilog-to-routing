/**CFile****************************************************************

  FileName    [saigSwitch.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Returns switching propabilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigSwitch.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"

#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Saig_SimObj_t_ Saig_SimObj_t;
struct Saig_SimObj_t_
{
    int      iFan0;
    int      iFan1;
    unsigned Type   :  8;
    unsigned Number : 24;
    unsigned pData[1];
};

static inline int Saig_SimObjFaninC0( Saig_SimObj_t * pObj )  { return pObj->iFan0 & 1;  }
static inline int Saig_SimObjFaninC1( Saig_SimObj_t * pObj )  { return pObj->iFan1 & 1;  }
static inline int Saig_SimObjFanin0( Saig_SimObj_t * pObj )   { return pObj->iFan0 >> 1; }
static inline int Saig_SimObjFanin1( Saig_SimObj_t * pObj )   { return pObj->iFan1 >> 1; }

//typedef struct Aig_CMan_t_ Aig_CMan_t;

//static Aig_CMan_t * Aig_CManCreate( Aig_Man_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates fast simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Saig_SimObj_t * Saig_ManCreateMan( Aig_Man_t * p )
{
    Saig_SimObj_t * pAig, * pEntry;
    Aig_Obj_t * pObj;
    int i;
    pAig = ABC_CALLOC( Saig_SimObj_t, Aig_ManObjNumMax(p)+1 );
//    printf( "Allocating %7.2f MB.\n", 1.0 * sizeof(Saig_SimObj_t) * (Aig_ManObjNumMax(p)+1)/(1<<20) );
    Aig_ManForEachObj( p, pObj, i )
    {
        pEntry = pAig + i;
        pEntry->Type = pObj->Type;
        if ( Aig_ObjIsCi(pObj) || i == 0 )
        {
            if ( Saig_ObjIsLo(p, pObj) )
            {
                pEntry->iFan0 = (Saig_ObjLoToLi(p, pObj)->Id << 1);
                pEntry->iFan1 = -1;
            }
            continue;
        }
        pEntry->iFan0 = (Aig_ObjFaninId0(pObj) << 1) | Aig_ObjFaninC0(pObj);
        if ( Aig_ObjIsCo(pObj) )
            continue;
        assert( Aig_ObjIsNode(pObj) );
        pEntry->iFan1 = (Aig_ObjFaninId1(pObj) << 1) | Aig_ObjFaninC1(pObj);
    }
    pEntry = pAig + Aig_ManObjNumMax(p);
    pEntry->Type = AIG_OBJ_VOID;
    return pAig;
}

/**Function*************************************************************

  Synopsis    [Simulated one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Saig_ManSimulateNode2( Saig_SimObj_t * pAig, Saig_SimObj_t * pObj )
{
    Saig_SimObj_t * pObj0 = pAig + Saig_SimObjFanin0( pObj );
    Saig_SimObj_t * pObj1 = pAig + Saig_SimObjFanin1( pObj );
    if ( Saig_SimObjFaninC0(pObj) && Saig_SimObjFaninC1(pObj) )
        pObj->pData[0] = ~(pObj0->pData[0] | pObj1->pData[0]);
    else if ( Saig_SimObjFaninC0(pObj) && !Saig_SimObjFaninC1(pObj) )
        pObj->pData[0] = (~pObj0->pData[0] & pObj1->pData[0]);
    else if ( !Saig_SimObjFaninC0(pObj) && Saig_SimObjFaninC1(pObj) )
        pObj->pData[0] = (pObj0->pData[0] & ~pObj1->pData[0]);
    else // if ( !Saig_SimObjFaninC0(pObj) && !Saig_SimObjFaninC1(pObj) )
        pObj->pData[0] = (pObj0->pData[0] & pObj1->pData[0]);
}

/**Function*************************************************************

  Synopsis    [Simulated one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Saig_ManSimulateNode( Saig_SimObj_t * pAig, Saig_SimObj_t * pObj )
{
    Saig_SimObj_t * pObj0 = pAig + Saig_SimObjFanin0( pObj );
    Saig_SimObj_t * pObj1 = pAig + Saig_SimObjFanin1( pObj );
    pObj->pData[0] = (Saig_SimObjFaninC0(pObj)? ~pObj0->pData[0] : pObj0->pData[0])
        & (Saig_SimObjFaninC1(pObj)? ~pObj1->pData[0] : pObj1->pData[0]);
}

/**Function*************************************************************

  Synopsis    [Simulated buffer/inverter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Saig_ManSimulateOneInput( Saig_SimObj_t * pAig, Saig_SimObj_t * pObj )
{
    Saig_SimObj_t * pObj0 = pAig + Saig_SimObjFanin0( pObj );
    if ( Saig_SimObjFaninC0(pObj) )
        pObj->pData[0] = ~pObj0->pData[0];
    else // if ( !Saig_SimObjFaninC0(pObj) )
        pObj->pData[0] = pObj0->pData[0];
}

/**Function*************************************************************

  Synopsis    [Simulates the timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManSimulateFrames( Saig_SimObj_t * pAig, int nFrames, int nPref )
{
    Saig_SimObj_t * pEntry;
    int f;
    for ( f = 0; f < nFrames; f++ )
    {
        for ( pEntry = pAig; pEntry->Type != AIG_OBJ_VOID; pEntry++ )
        {
            if ( pEntry->Type == AIG_OBJ_AND )
                Saig_ManSimulateNode( pAig, pEntry );
            else if ( pEntry->Type == AIG_OBJ_CO )
                Saig_ManSimulateOneInput( pAig, pEntry );
            else if ( pEntry->Type == AIG_OBJ_CI )
            {
                if ( pEntry->iFan0 == 0 ) // true PI
                    pEntry->pData[0] = Aig_ManRandom( 0 );
                else if ( f > 0 ) // register output
                    Saig_ManSimulateOneInput( pAig, pEntry );
            }
            else if ( pEntry->Type == AIG_OBJ_CONST1 )
                pEntry->pData[0] = ~0;
            else if ( pEntry->Type != AIG_OBJ_NONE )
                assert( 0 );
            if ( f >= nPref )
                pEntry->Number += Aig_WordCountOnes( pEntry->pData[0] );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Computes switching activity of one node.]

  Description [Uses the formula: Switching = 2 * nOnes * nZeros / (nTotal ^ 2) ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Saig_ManComputeSwitching( int nOnes, int nSimWords )
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
float Saig_ManComputeProbOne( int nOnes, int nSimWords )
{
    int nTotal = 32 * nSimWords;
    return (float)nOnes / nTotal;
}

/**Function*************************************************************

  Synopsis    [Computes switching activity of one node.]

  Description [Uses the formula: Switching = 2 * nOnes * nZeros / (nTotal ^ 2) ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Saig_ManComputeProbOnePlus( int nOnes, int nSimWords, int fCompl )
{
    int nTotal = 32 * nSimWords;
    if ( fCompl )
        return (float)(nTotal-nOnes) / nTotal;
    else
        return (float)nOnes / nTotal;
}

/**Function*************************************************************

  Synopsis    [Compute switching probabilities of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManComputeSwitchProb4s( Aig_Man_t * p, int nFrames, int nPref, int fProbOne )
{
    Saig_SimObj_t * pAig, * pEntry;
    Vec_Int_t * vSwitching;
    float * pSwitching;
    int nFramesReal;
    abctime clk;//, clkTotal = Abc_Clock();
    vSwitching = Vec_IntStart( Aig_ManObjNumMax(p) );
    pSwitching = (float *)vSwitching->pArray;
clk = Abc_Clock();
    pAig = Saig_ManCreateMan( p );
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
        printf( "The total number of frames (%d) should exceed prefix (%d).\n", nFramesReal, nPref );\
        printf( "Setting the total number of frames to be %d.\n", nFrames );
        nFramesReal = nFrames;
    }
//printf( "Simulating %d frames.\n", nFramesReal );
clk = Abc_Clock();
    Saig_ManSimulateFrames( pAig, nFramesReal, nPref );
//ABC_PRT( "Simulation", Abc_Clock() - clk );
clk = Abc_Clock();
    for ( pEntry = pAig; pEntry->Type != AIG_OBJ_VOID; pEntry++ )
    {
/*
        if ( pEntry->Type == AIG_OBJ_AND )
        {
        Saig_SimObj_t * pObj0 = pAig + Saig_SimObjFanin0( pEntry );
        Saig_SimObj_t * pObj1 = pAig + Saig_SimObjFanin1( pEntry );
        printf( "%5.2f = %5.2f * %5.2f  (%7.4f)\n", 
            Saig_ManComputeProbOnePlus( pEntry->Number, nFrames - nPref, 0 ),
            Saig_ManComputeProbOnePlus( pObj0->Number, nFrames - nPref, Saig_SimObjFaninC0(pEntry) ),
            Saig_ManComputeProbOnePlus( pObj1->Number, nFrames - nPref, Saig_SimObjFaninC1(pEntry) ),
            Saig_ManComputeProbOnePlus( pEntry->Number, nFrames - nPref, 0 ) -
            Saig_ManComputeProbOnePlus( pObj0->Number, nFrames - nPref, Saig_SimObjFaninC0(pEntry) ) *
            Saig_ManComputeProbOnePlus( pObj1->Number, nFrames - nPref, Saig_SimObjFaninC1(pEntry) )
            );
        }
*/
        if ( fProbOne )
            pSwitching[pEntry-pAig] = Saig_ManComputeProbOne( pEntry->Number, nFramesReal - nPref );
        else
            pSwitching[pEntry-pAig] = Saig_ManComputeSwitching( pEntry->Number, nFramesReal - nPref );
//printf( "%3d : %7.2f\n", pEntry-pAig, pSwitching[pEntry-pAig] );
    }
    ABC_FREE( pAig );
//ABC_PRT( "Switch    ", Abc_Clock() - clk );
//ABC_PRT( "TOTAL     ", Abc_Clock() - clkTotal );

//    Aig_CManCreate( p );
    return vSwitching;
}




typedef struct Aig_CMan_t_ Aig_CMan_t;
struct Aig_CMan_t_
{
    // parameters
    int             nIns;
    int             nNodes;
    int             nOuts;
    // current state
    int             iNode;
    int             iDiff0;
    int             iDiff1;
    unsigned char * pCur;
    // stored data
    int             iPrev;
    int             nBytes;
    unsigned char   Data[0];
};


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_CMan_t * Aig_CManStart( int nIns, int nNodes, int nOuts )
{
    Aig_CMan_t * p;
    p = (Aig_CMan_t *)ABC_ALLOC( char, sizeof(Aig_CMan_t) + 2*(2*nNodes + nOuts) );
    memset( p, 0, sizeof(Aig_CMan_t) );
    // set parameters
    p->nIns = nIns;
    p->nOuts = nOuts;
    p->nNodes = nNodes;
    p->nBytes = 2*(2*nNodes + nOuts);
    // prepare the manager
    p->iNode = 1 + p->nIns;
    p->iPrev = -1;
    p->pCur = p->Data;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_CManStop( Aig_CMan_t * p )
{
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_CManRestart( Aig_CMan_t * p )
{
    assert( p->iNode == 1 + p->nIns + p->nNodes + p->nOuts );
    p->iNode = 1 + p->nIns;
    p->iPrev = -1;
    p->pCur = p->Data;
}


/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_CManStoreNum( Aig_CMan_t * p, unsigned x )
{
    while ( x & ~0x7f )
    {
        *p->pCur++ = (x & 0x7f) | 0x80;
        x >>= 7;
    }
    *p->pCur++ = x;
    assert( p->pCur - p->Data < p->nBytes - 10 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_CManRestoreNum( Aig_CMan_t * p )
{
    int ch, i, x = 0;
    for ( i = 0; (ch = *p->pCur++) & 0x80; i++ )
        x |= (ch & 0x7f) << (7 * i);
    return x | (ch << (7 * i));
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_CManAddNode( Aig_CMan_t * p, int iFan0, int iFan1 )
{
    assert( iFan0 < iFan1 );
    assert( iFan1 < (p->iNode << 1) );
    Aig_CManStoreNum( p, (p->iNode++ << 1) - iFan1 );
    Aig_CManStoreNum( p, iFan1 - iFan0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_CManAddPo( Aig_CMan_t * p, int iFan0 )
{
    if ( p->iPrev == -1 )
        Aig_CManStoreNum( p, p->iNode - iFan0 );
    else if ( p->iPrev <= iFan0 )
        Aig_CManStoreNum( p, (iFan0 - p->iPrev) << 1 );
    else 
        Aig_CManStoreNum( p,((p->iPrev - iFan0) << 1) | 1 );
    p->iPrev = iFan0;
    p->iNode++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_CManGetNode( Aig_CMan_t * p, int * piFan0, int * piFan1 )
{
    *piFan1 = (p->iNode++ << 1) - Aig_CManRestoreNum( p );
    *piFan0 = *piFan1 - Aig_CManRestoreNum( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_CManGetPo( Aig_CMan_t * p )
{
    int Num = Aig_CManRestoreNum( p );
    if ( p->iPrev == -1 )
        p->iPrev = p->iNode;
    p->iNode++;
    if ( Num & 1 )
        return p->iPrev = p->iPrev + (Num >> 1);
    return p->iPrev = p->iPrev - (Num >> 1);
}

/**Function*************************************************************

  Synopsis    [Compute switching probabilities of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_CMan_t * Aig_CManCreate( Aig_Man_t * p )
{
    Aig_CMan_t * pCMan;
    Aig_Obj_t * pObj;
    int i;
    pCMan = Aig_CManStart( Aig_ManCiNum(p), Aig_ManNodeNum(p), Aig_ManCoNum(p) );
    Aig_ManForEachNode( p, pObj, i )
        Aig_CManAddNode( pCMan, 
            (Aig_ObjFaninId0(pObj) << 1) | Aig_ObjFaninC0(pObj), 
            (Aig_ObjFaninId1(pObj) << 1) | Aig_ObjFaninC1(pObj) );
    Aig_ManForEachCo( p, pObj, i )
        Aig_CManAddPo( pCMan, 
            (Aig_ObjFaninId0(pObj) << 1) | Aig_ObjFaninC0(pObj) ); 
    printf( "\nBytes alloc = %5d.  Bytes used = %7d.  Ave per node = %4.2f. \n", 
        pCMan->nBytes, (int)(pCMan->pCur - pCMan->Data), 
        1.0 * (pCMan->pCur - pCMan->Data) / (pCMan->nNodes + pCMan->nOuts ) );
//    Aig_CManStop( pCMan );
    return pCMan;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

