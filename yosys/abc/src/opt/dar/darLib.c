/**CFile****************************************************************

  FileName    [darLib.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting.]

  Synopsis    [Library of AIG subgraphs used for rewriting.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: darLib.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "darInt.h"
#include "aig/gia/gia.h"
#include "dar.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Dar_Lib_t_            Dar_Lib_t;
typedef struct Dar_LibObj_t_         Dar_LibObj_t;
typedef struct Dar_LibDat_t_         Dar_LibDat_t;

struct Dar_LibObj_t_ // library object (2 words)
{
    unsigned         Fan0    : 16;  // the first fanin
    unsigned         Fan1    : 16;  // the second fanin
    unsigned         fCompl0 :  1;  // the first compl attribute
    unsigned         fCompl1 :  1;  // the second compl attribute
    unsigned         fPhase  :  1;  // the phase of the node
    unsigned         fTerm   :  1;  // indicates a PI
    unsigned         Num     : 28;  // internal use
};

struct Dar_LibDat_t_ // library object data
{
    union { 
    Aig_Obj_t *      pFunc;         // the corresponding AIG node if it exists
    int              iGunc; };      // the corresponding AIG node if it exists
    int              Level;         // level of this node after it is constructured
    int              TravId;        // traversal ID of the library object data
    float            dProb;         // probability of the node being 1
    unsigned char    fMffc;         // set to one if node is part of MFFC
    unsigned char    nLats[3];      // the number of latches on the input/output stem
};

struct Dar_Lib_t_ // library 
{
    // objects
    Dar_LibObj_t *   pObjs;         // the set of library objects
    int              nObjs;         // the number of objects used
    int              iObj;          // the current object
    // structures by class
    int              nSubgr[222];   // the number of subgraphs by class
    int *            pSubgr[222];   // the subgraphs for each class
    int *            pSubgrMem;     // memory for subgraph pointers
    int              nSubgrTotal;   // the total number of subgraph
    // structure priorities
    int *            pPriosMem;     // memory for priority of structures
    int *            pPrios[222];   // pointers to the priority numbers
    // structure places in the priorities
    int *            pPlaceMem;     // memory for places of structures in the priority lists
    int *            pPlace[222];   // pointers to the places numbers
    // structure scores
    int *            pScoreMem;     // memory for scores of structures
    int *            pScore[222];   // pointers to the scores numbers
    // nodes by class
    int              nNodes[222];   // the number of nodes by class
    int *            pNodes[222];   // the nodes for each class
    int *            pNodesMem;     // memory for nodes pointers
    int              nNodesTotal;   // the total number of nodes
    // prepared library
    int              nSubgraphs;
    int              nNodes0Max;
    // nodes by class
    int              nNodes0[222];   // the number of nodes by class
    int *            pNodes0[222];   // the nodes for each class
    int *            pNodes0Mem;     // memory for nodes pointers
    int              nNodes0Total;   // the total number of nodes
    // structures by class
    int              nSubgr0[222];   // the number of subgraphs by class
    int *            pSubgr0[222];   // the subgraphs for each class
    int *            pSubgr0Mem;     // memory for subgraph pointers
    int              nSubgr0Total;   // the total number of subgraph
    // object data
    Dar_LibDat_t *   pDatas;
    int              nDatas;
    // information about NPN classes
    char **          pPerms4;
    unsigned short * puCanons; 
    char *           pPhases; 
    char *           pPerms; 
    unsigned char *  pMap;
};

static Dar_Lib_t * s_DarLib = NULL;

static inline Dar_LibObj_t * Dar_LibObj( Dar_Lib_t * p, int Id )    { return p->pObjs + Id; }
static inline int            Dar_LibObjTruth( Dar_LibObj_t * pObj ) { return pObj->Num < (0xFFFF & ~pObj->Num) ? pObj->Num : (0xFFFF & ~pObj->Num); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dar_Lib_t * Dar_LibAlloc( int nObjs )
{
    unsigned uTruths[4] = { 0xAAAA, 0xCCCC, 0xF0F0, 0xFF00 };
    Dar_Lib_t * p;
    int i;//, clk = Abc_Clock();
    p = ABC_ALLOC( Dar_Lib_t, 1 );
    memset( p, 0, sizeof(Dar_Lib_t) );
    // allocate objects
    p->nObjs = nObjs;
    p->pObjs = ABC_ALLOC( Dar_LibObj_t, nObjs );
    memset( p->pObjs, 0, sizeof(Dar_LibObj_t) * nObjs );
    // allocate canonical data
    p->pPerms4 = Dar_Permutations( 4 );
    Dar_Truth4VarNPN( &p->puCanons, &p->pPhases, &p->pPerms, &p->pMap );
    // start the elementary objects
    p->iObj = 4;
    for ( i = 0; i < 4; i++ )
    {
        p->pObjs[i].fTerm = 1;
        p->pObjs[i].Num = uTruths[i];
    }
//    ABC_PRT( "Library start", Abc_Clock() - clk );
    return p;
}

/**Function*************************************************************

  Synopsis    [Frees the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibFree( Dar_Lib_t * p )
{
    ABC_FREE( p->pObjs );
    ABC_FREE( p->pDatas );
    ABC_FREE( p->pNodesMem );
    ABC_FREE( p->pNodes0Mem );
    ABC_FREE( p->pSubgrMem );
    ABC_FREE( p->pSubgr0Mem );
    ABC_FREE( p->pPriosMem );
    ABC_FREE( p->pPlaceMem );
    ABC_FREE( p->pScoreMem );
    ABC_FREE( p->pPerms4 );
    ABC_FREE( p->puCanons );
    ABC_FREE( p->pPhases );
    ABC_FREE( p->pPerms );
    ABC_FREE( p->pMap );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns canonical truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar_LibReturnClass( unsigned uTruth )
{
    return s_DarLib->pMap[uTruth & 0xffff];
}


/**Function*************************************************************

  Synopsis    [Returns canonical truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibReturnCanonicals( unsigned * pCanons )
{
    int Visits[222] = {0};
    int i, k;
    // find canonical truth tables
    for ( i = k = 0; i < (1<<16); i++ )
        if ( !Visits[s_DarLib->pMap[i]] )
        {
            Visits[s_DarLib->pMap[i]] = 1;
            pCanons[k++] = ((i<<16) | i);
        }
    assert( k == 222 );
}

/**Function*************************************************************

  Synopsis    [Adds one AND to the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibAddNode( Dar_Lib_t * p, int Id0, int Id1, int fCompl0, int fCompl1 )
{
    Dar_LibObj_t * pFan0 = Dar_LibObj( p, Id0 );
    Dar_LibObj_t * pFan1 = Dar_LibObj( p, Id1 );
    Dar_LibObj_t * pObj  = p->pObjs + p->iObj++;
    pObj->Fan0 = Id0;
    pObj->Fan1 = Id1;
    pObj->fCompl0 = fCompl0;
    pObj->fCompl1 = fCompl1;
    pObj->fPhase = (fCompl0 ^ pFan0->fPhase) & (fCompl1 ^ pFan1->fPhase);
    pObj->Num = 0xFFFF & (fCompl0? ~pFan0->Num : pFan0->Num) & (fCompl1? ~pFan1->Num : pFan1->Num);
}

/**Function*************************************************************

  Synopsis    [Adds one AND to the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibSetup_rec( Dar_Lib_t * p, Dar_LibObj_t * pObj, int Class, int fCollect )
{
    if ( pObj->fTerm || (int)pObj->Num == Class )
        return;
    pObj->Num = Class;
    Dar_LibSetup_rec( p, Dar_LibObj(p, pObj->Fan0), Class, fCollect );
    Dar_LibSetup_rec( p, Dar_LibObj(p, pObj->Fan1), Class, fCollect );
    if ( fCollect )
        p->pNodes[Class][ p->nNodes[Class]++ ] = pObj-p->pObjs;
    else
        p->nNodes[Class]++;
}

/**Function*************************************************************

  Synopsis    [Adds one AND to the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibSetup( Dar_Lib_t * p, Vec_Int_t * vOuts, Vec_Int_t * vPrios )
{
    int fTraining = 0;
    Dar_LibObj_t * pObj;
    int nNodesTotal, uTruth, Class, Out, i, k;
    assert( p->iObj == p->nObjs );

    // count the number of representatives of each class
    for ( i = 0; i < 222; i++ )
        p->nSubgr[i] = p->nNodes[i] = 0;
    Vec_IntForEachEntry( vOuts, Out, i )
    {
        pObj = Dar_LibObj( p, Out );
        uTruth = Dar_LibObjTruth( pObj );
        Class = p->pMap[uTruth];
        p->nSubgr[Class]++;
    }
    // allocate memory for the roots of each class
    p->pSubgrMem = ABC_ALLOC( int, Vec_IntSize(vOuts) );
    p->pSubgr0Mem = ABC_ALLOC( int, Vec_IntSize(vOuts) );
    p->nSubgrTotal = 0;
    for ( i = 0; i < 222; i++ )
    {
        p->pSubgr[i] = p->pSubgrMem + p->nSubgrTotal;
        p->pSubgr0[i] = p->pSubgr0Mem + p->nSubgrTotal;
        p->nSubgrTotal += p->nSubgr[i];
        p->nSubgr[i] = 0;
    }
    assert( p->nSubgrTotal == Vec_IntSize(vOuts) );
    // add the outputs to storage
    Vec_IntForEachEntry( vOuts, Out, i )
    {
        pObj = Dar_LibObj( p, Out );
        uTruth = Dar_LibObjTruth( pObj );
        Class = p->pMap[uTruth];
        p->pSubgr[Class][ p->nSubgr[Class]++ ] = Out;
    }

    if ( fTraining )
    {
        // allocate memory for the priority of roots of each class
        p->pPriosMem = ABC_ALLOC( int, Vec_IntSize(vOuts) );
        p->nSubgrTotal = 0;
        for ( i = 0; i < 222; i++ )
        {
            p->pPrios[i] = p->pPriosMem + p->nSubgrTotal;
            p->nSubgrTotal += p->nSubgr[i];
            for ( k = 0; k < p->nSubgr[i]; k++ )
                p->pPrios[i][k] = k;

        }
        assert( p->nSubgrTotal == Vec_IntSize(vOuts) );

        // allocate memory for the priority of roots of each class
        p->pPlaceMem = ABC_ALLOC( int, Vec_IntSize(vOuts) );
        p->nSubgrTotal = 0;
        for ( i = 0; i < 222; i++ )
        {
            p->pPlace[i] = p->pPlaceMem + p->nSubgrTotal;
            p->nSubgrTotal += p->nSubgr[i];
            for ( k = 0; k < p->nSubgr[i]; k++ )
                p->pPlace[i][k] = k;

        }
        assert( p->nSubgrTotal == Vec_IntSize(vOuts) );

        // allocate memory for the priority of roots of each class
        p->pScoreMem = ABC_ALLOC( int, Vec_IntSize(vOuts) );
        p->nSubgrTotal = 0;
        for ( i = 0; i < 222; i++ )
        {
            p->pScore[i] = p->pScoreMem + p->nSubgrTotal;
            p->nSubgrTotal += p->nSubgr[i];
            for ( k = 0; k < p->nSubgr[i]; k++ )
                p->pScore[i][k] = 0;

        }
        assert( p->nSubgrTotal == Vec_IntSize(vOuts) );
    }
    else
    {
        int Counter = 0;
        // allocate memory for the priority of roots of each class
        p->pPriosMem = ABC_ALLOC( int, Vec_IntSize(vOuts) );
        p->nSubgrTotal = 0;
        for ( i = 0; i < 222; i++ )
        {
            p->pPrios[i] = p->pPriosMem + p->nSubgrTotal;
            p->nSubgrTotal += p->nSubgr[i];
            for ( k = 0; k < p->nSubgr[i]; k++ )
                p->pPrios[i][k] = Vec_IntEntry(vPrios, Counter++);

        }
        assert( p->nSubgrTotal == Vec_IntSize(vOuts) );
        assert( Counter == Vec_IntSize(vPrios) );
    }

    // create traversal IDs
    for ( i = 0; i < p->iObj; i++ )
        Dar_LibObj(p, i)->Num = 0xff;
    // count nodes in each class
    for ( i = 0; i < 222; i++ )
        for ( k = 0; k < p->nSubgr[i]; k++ )
            Dar_LibSetup_rec( p, Dar_LibObj(p, p->pSubgr[i][k]), i, 0 );
    // count the total number of nodes
    p->nNodesTotal = 0;
    for ( i = 0; i < 222; i++ )
        p->nNodesTotal += p->nNodes[i];
    // allocate memory for the nodes of each class
    p->pNodesMem = ABC_ALLOC( int, p->nNodesTotal );
    p->pNodes0Mem = ABC_ALLOC( int, p->nNodesTotal );
    p->nNodesTotal = 0;
    for ( i = 0; i < 222; i++ )
    {
        p->pNodes[i] = p->pNodesMem + p->nNodesTotal;
        p->pNodes0[i] = p->pNodes0Mem + p->nNodesTotal;
        p->nNodesTotal += p->nNodes[i];
        p->nNodes[i] = 0;
    }
    // create traversal IDs
    for ( i = 0; i < p->iObj; i++ )
        Dar_LibObj(p, i)->Num = 0xff;
    // add the nodes to storage
    nNodesTotal = 0;
    for ( i = 0; i < 222; i++ )
    {
         for ( k = 0; k < p->nSubgr[i]; k++ )
            Dar_LibSetup_rec( p, Dar_LibObj(p, p->pSubgr[i][k]), i, 1 );
         nNodesTotal += p->nNodes[i];
//printf( "Class %3d : Subgraphs = %4d. Nodes = %5d.\n", i, p->nSubgr[i], p->nNodes[i] );
    }
    assert( nNodesTotal == p->nNodesTotal );
     // prepare the number of the PI nodes
    for ( i = 0; i < 4; i++ )
        Dar_LibObj(p, i)->Num = i;
}

/**Function*************************************************************

  Synopsis    [Starts the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibCreateData( Dar_Lib_t * p, int nDatas )
{
    if ( p->nDatas == nDatas )
        return;
    ABC_FREE( p->pDatas );
    // allocate datas
    p->nDatas = nDatas;
    p->pDatas = ABC_ALLOC( Dar_LibDat_t, nDatas );
    memset( p->pDatas, 0, sizeof(Dar_LibDat_t) * nDatas );
}

/**Function*************************************************************

  Synopsis    [Adds one AND to the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibSetup0_rec( Dar_Lib_t * p, Dar_LibObj_t * pObj, int Class, int fCollect )
{
    if ( pObj->fTerm || (int)pObj->Num == Class )
        return;
    pObj->Num = Class;
    Dar_LibSetup0_rec( p, Dar_LibObj(p, pObj->Fan0), Class, fCollect );
    Dar_LibSetup0_rec( p, Dar_LibObj(p, pObj->Fan1), Class, fCollect );
    if ( fCollect )
        p->pNodes0[Class][ p->nNodes0[Class]++ ] = pObj-p->pObjs;
    else
        p->nNodes0[Class]++;
}

/**Function*************************************************************

  Synopsis    [Starts the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibPrepare( int nSubgraphs )
{
    Dar_Lib_t * p = s_DarLib;
    int i, k, nNodes0Total;
    if ( p->nSubgraphs == nSubgraphs )
        return;

    // favor special classes:
    //  1 : F = (!d*!c*!b*!a)
    //  4 : F = (!d*!c*!(b*a))
    // 12 : F = (!d*!(c*!(!b*!a)))
    // 20 : F = (!d*!(c*b*a))

    // set the subgraph counters 
    p->nSubgr0Total = 0;
    for ( i = 0; i < 222; i++ )
    {
//        if ( i == 1 || i == 4 || i == 12 || i == 20 ) // special classes 
        if ( i == 1 ) // special classes 
            p->nSubgr0[i] = p->nSubgr[i];
        else
            p->nSubgr0[i] = Abc_MinInt( p->nSubgr[i], nSubgraphs );
        p->nSubgr0Total += p->nSubgr0[i];
        for ( k = 0; k < p->nSubgr0[i]; k++ )
            p->pSubgr0[i][k] = p->pSubgr[i][ p->pPrios[i][k] ];
    }

    // count the number of nodes
    // clean node counters
    for ( i = 0; i < 222; i++ )
        p->nNodes0[i] = 0;
    // create traversal IDs
    for ( i = 0; i < p->iObj; i++ )
        Dar_LibObj(p, i)->Num = 0xff;
    // count nodes in each class
    // count the total number of nodes and the largest class
    p->nNodes0Total = 0;
    p->nNodes0Max = 0;
    for ( i = 0; i < 222; i++ )
    {
        for ( k = 0; k < p->nSubgr0[i]; k++ )
            Dar_LibSetup0_rec( p, Dar_LibObj(p, p->pSubgr0[i][k]), i, 0 );
        p->nNodes0Total += p->nNodes0[i];
        p->nNodes0Max = Abc_MaxInt( p->nNodes0Max, p->nNodes0[i] );
    }

    // clean node counters
    for ( i = 0; i < 222; i++ )
        p->nNodes0[i] = 0;
    // create traversal IDs
    for ( i = 0; i < p->iObj; i++ )
        Dar_LibObj(p, i)->Num = 0xff;
    // add the nodes to storage
    nNodes0Total = 0;
    for ( i = 0; i < 222; i++ )
    {
        for ( k = 0; k < p->nSubgr0[i]; k++ )
            Dar_LibSetup0_rec( p, Dar_LibObj(p, p->pSubgr0[i][k]), i, 1 );
         nNodes0Total += p->nNodes0[i];
    }
    assert( nNodes0Total == p->nNodes0Total );
     // prepare the number of the PI nodes
    for ( i = 0; i < 4; i++ )
        Dar_LibObj(p, i)->Num = i;

    // realloc the datas
    Dar_LibCreateData( p, p->nNodes0Max + 32 ); 
    // allocated more because Dar_LibBuildBest() sometimes requires more entries
}

/**Function*************************************************************

  Synopsis    [Reads library from array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dar_Lib_t * Dar_LibRead()
{
    Vec_Int_t * vObjs, * vOuts, * vPrios;
    Dar_Lib_t * p;
    int i;
    // read nodes and outputs
    vObjs  = Dar_LibReadNodes();
    vOuts  = Dar_LibReadOuts();
    vPrios = Dar_LibReadPrios();
    // create library
    p = Dar_LibAlloc( Vec_IntSize(vObjs)/2 + 4 );
    // create nodes
    for ( i = 0; i < vObjs->nSize; i += 2 )
        Dar_LibAddNode( p, vObjs->pArray[i] >> 1, vObjs->pArray[i+1] >> 1,
            vObjs->pArray[i] & 1, vObjs->pArray[i+1] & 1 );
    // create outputs
    Dar_LibSetup( p, vOuts, vPrios );
    Vec_IntFree( vObjs );
    Vec_IntFree( vOuts );
    Vec_IntFree( vPrios );
    return p;
}

/**Function*************************************************************

  Synopsis    [Starts the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibStart()
{
//    abctime clk = Abc_Clock();
    if ( s_DarLib != NULL )
        return;
    assert( s_DarLib == NULL );
    s_DarLib = Dar_LibRead();
//    printf( "The 4-input library started with %d nodes and %d subgraphs. ", s_DarLib->nObjs - 4, s_DarLib->nSubgrTotal );
//    ABC_PRT( "Time", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    [Stops the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibStop()
{
    assert( s_DarLib != NULL );
    Dar_LibFree( s_DarLib );
    s_DarLib = NULL;
}

/**Function*************************************************************

  Synopsis    [Updates the score of the class and adjusts the priority of this class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibIncrementScore( int Class, int Out, int Gain )
{
    int * pPrios = s_DarLib->pPrios[Class];  // pPrios[i] = Out
    int * pPlace = s_DarLib->pPlace[Class];  // pPlace[Out] = i
    int * pScore = s_DarLib->pScore[Class];  // score of Out
    int Out2;
    assert( Class >= 0 && Class < 222 );
    assert( Out >= 0 && Out < s_DarLib->nSubgr[Class] );
    assert( pPlace[pPrios[Out]] == Out );
    // increment the score
    pScore[Out] += Gain;
    // move the out in the order 
    while ( pPlace[Out] > 0 && pScore[Out] > pScore[ pPrios[pPlace[Out]-1] ] )
    {
        // get the previous output in the priority list
        Out2 = pPrios[pPlace[Out]-1];
        // swap Out and Out2
        pPlace[Out]--;
        pPlace[Out2]++;
        pPrios[pPlace[Out]] = Out;
        pPrios[pPlace[Out2]] = Out2;
    }
}

/**Function*************************************************************

  Synopsis    [Prints out the priorities into the file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibDumpPriorities()
{
    int i, k, Out, Out2, Counter = 0, Printed = 0;
    printf( "\nOutput priorities (total = %d):\n", s_DarLib->nSubgrTotal );
    for ( i = 0; i < 222; i++ )
    {
//        printf( "Class%d: ", i );
        for ( k = 0; k < s_DarLib->nSubgr[i]; k++ )
        {
            Out = s_DarLib->pPrios[i][k];
            Out2 = k == 0 ? Out : s_DarLib->pPrios[i][k-1];
            assert( s_DarLib->pScore[i][Out2] >= s_DarLib->pScore[i][Out] );
//            printf( "%d(%d), ", Out, s_DarLib->pScore[i][Out] );
            printf( "%d, ", Out );
            Printed++;
            if ( ++Counter == 15 )
            {
                printf( "\n" );
                Counter = 0;
            }
        }
    }
    printf( "\n" );
    assert( Printed == s_DarLib->nSubgrTotal );
}


/**Function*************************************************************

  Synopsis    [Matches the cut with its canonical form.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar_LibCutMatch( Dar_Man_t * p, Dar_Cut_t * pCut )
{
    Aig_Obj_t * pFanin;
    unsigned uPhase;
    char * pPerm;
    int i;
    assert( pCut->nLeaves == 4 );
    // get the fanin permutation
    uPhase = s_DarLib->pPhases[pCut->uTruth];
    pPerm = s_DarLib->pPerms4[ (int)s_DarLib->pPerms[pCut->uTruth] ];
    // collect fanins with the corresponding permutation/phase
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
    {
        pFanin = Aig_ManObj( p->pAig, pCut->pLeaves[ (int)pPerm[i] ] );
        if ( pFanin == NULL )
        {
            p->nCutsBad++;
            return 0;
        }
        pFanin = Aig_NotCond(pFanin, ((uPhase >> i) & 1) );
        s_DarLib->pDatas[i].pFunc = pFanin;
        s_DarLib->pDatas[i].Level = Aig_Regular(pFanin)->Level;
        // copy the propability of node being one
        if ( p->pPars->fPower )
        {
            float Prob = Abc_Int2Float( Vec_IntEntry( p->pAig->vProbs, Aig_ObjId(Aig_Regular(pFanin)) ) );
            s_DarLib->pDatas[i].dProb = Aig_IsComplement(pFanin)? 1.0-Prob : Prob;
        }
    }
    p->nCutsGood++;
    return 1;
}



/**Function*************************************************************

  Synopsis    [Marks the MFFC of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar_LibCutMarkMffc( Aig_Man_t * p, Aig_Obj_t * pRoot, int nLeaves, float * pPower )
{
    int i, nNodes;
    // mark the cut leaves
    for ( i = 0; i < nLeaves; i++ )
        Aig_Regular(s_DarLib->pDatas[i].pFunc)->nRefs++;
    // label MFFC with current ID
    nNodes = Aig_NodeMffcLabel( p, pRoot, pPower );
    // unmark the cut leaves
    for ( i = 0; i < nLeaves; i++ )
        Aig_Regular(s_DarLib->pDatas[i].pFunc)->nRefs--;
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Evaluates one cut.]

  Description [Returns the best gain.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibObjPrint_rec( Dar_LibObj_t * pObj )
{
    if ( pObj->fTerm )
    {
        printf( "%c", 'a' + (int)(pObj - s_DarLib->pObjs) );
        return;
    }
    printf( "(" );
    Dar_LibObjPrint_rec( Dar_LibObj(s_DarLib, pObj->Fan0) );
    if ( pObj->fCompl0 )
        printf( "\'" );
    Dar_LibObjPrint_rec( Dar_LibObj(s_DarLib, pObj->Fan1) );
    if ( pObj->fCompl0 )
        printf( "\'" );
    printf( ")" );
}


/**Function*************************************************************

  Synopsis    [Assigns numbers to the nodes of one class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibEvalAssignNums( Dar_Man_t * p, int Class, Aig_Obj_t * pRoot )
{
    Dar_LibObj_t * pObj;
    Dar_LibDat_t * pData, * pData0, * pData1;
    Aig_Obj_t * pFanin0, * pFanin1;
    int i;
    for ( i = 0; i < s_DarLib->nNodes0[Class]; i++ )
    {
        // get one class node, assign its temporary number and set its data
        pObj = Dar_LibObj(s_DarLib, s_DarLib->pNodes0[Class][i]);
        pObj->Num = 4 + i;
        assert( (int)pObj->Num < s_DarLib->nNodes0Max + 4 );
        pData = s_DarLib->pDatas + pObj->Num;
        pData->fMffc = 0;
        pData->pFunc = NULL;
        pData->TravId = 0xFFFF;

        // explore the fanins
        assert( (int)Dar_LibObj(s_DarLib, pObj->Fan0)->Num < s_DarLib->nNodes0Max + 4 );
        assert( (int)Dar_LibObj(s_DarLib, pObj->Fan1)->Num < s_DarLib->nNodes0Max + 4 );
        pData0 = s_DarLib->pDatas + Dar_LibObj(s_DarLib, pObj->Fan0)->Num;
        pData1 = s_DarLib->pDatas + Dar_LibObj(s_DarLib, pObj->Fan1)->Num;
        pData->Level = 1 + Abc_MaxInt(pData0->Level, pData1->Level);
        if ( pData0->pFunc == NULL || pData1->pFunc == NULL )
            continue;
        pFanin0 = Aig_NotCond( pData0->pFunc, pObj->fCompl0 );
        pFanin1 = Aig_NotCond( pData1->pFunc, pObj->fCompl1 );
        if ( Aig_Regular(pFanin0) == pRoot || Aig_Regular(pFanin1) == pRoot )
            continue;
        pData->pFunc = Aig_TableLookupTwo( p->pAig, pFanin0, pFanin1 );
        if ( pData->pFunc )
        {
            // update the level to be more accurate
            pData->Level = Aig_Regular(pData->pFunc)->Level;
            // mark the node if it is part of MFFC
            pData->fMffc = Aig_ObjIsTravIdCurrent(p->pAig, Aig_Regular(pData->pFunc));
            // assign the probability
            if ( p->pPars->fPower )
            {
                float Prob = Abc_Int2Float( Vec_IntEntry( p->pAig->vProbs, Aig_ObjId(Aig_Regular(pData->pFunc)) ) );
                pData->dProb = Aig_IsComplement(pData->pFunc)? 1.0-Prob : Prob;
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Evaluates one cut.]

  Description [Returns the best gain.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar_LibEval_rec( Dar_LibObj_t * pObj, int Out, int nNodesSaved, int Required, float * pPower )
{
    Dar_LibDat_t * pData;
    float Power0, Power1;
    int Area;
    if ( pPower )
        *pPower = (float)0.0;
    pData = s_DarLib->pDatas + pObj->Num;
    if ( pData->TravId == Out )
        return 0;
    pData->TravId = Out;
    if ( pObj->fTerm )
    {
        if ( pPower )
            *pPower = pData->dProb;
        return 0;
    }
    assert( pObj->Num > 3 );
    if ( pData->Level > Required )
        return 0xff;
    if ( pData->pFunc && !pData->fMffc )
    {
        if ( pPower )
            *pPower = pData->dProb;
        return 0;
    }
    // this is a new node - get a bound on the area of its branches
    nNodesSaved--;
    Area = Dar_LibEval_rec( Dar_LibObj(s_DarLib, pObj->Fan0), Out, nNodesSaved, Required+1, pPower? &Power0 : NULL );
    if ( Area > nNodesSaved )
        return 0xff;
    Area += Dar_LibEval_rec( Dar_LibObj(s_DarLib, pObj->Fan1), Out, nNodesSaved, Required+1, pPower? &Power1 : NULL );
    if ( Area > nNodesSaved )
        return 0xff;
    if ( pPower )
    {
        Dar_LibDat_t * pData0 = s_DarLib->pDatas + Dar_LibObj(s_DarLib, pObj->Fan0)->Num;
        Dar_LibDat_t * pData1 = s_DarLib->pDatas + Dar_LibObj(s_DarLib, pObj->Fan1)->Num;
        pData->dProb = (pObj->fCompl0? 1.0 - pData0->dProb : pData0->dProb)*
                       (pObj->fCompl1? 1.0 - pData1->dProb : pData1->dProb);
        *pPower = Power0 + 2.0 * pData0->dProb * (1.0 - pData0->dProb) +
                  Power1 + 2.0 * pData1->dProb * (1.0 - pData1->dProb);
    }
    return Area + 1;
}

/**Function*************************************************************

  Synopsis    [Evaluates one cut.]

  Description [Returns the best gain.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibEval( Dar_Man_t * p, Aig_Obj_t * pRoot, Dar_Cut_t * pCut, int Required, int * pnMffcSize )
{
    int fTraining = 0;
    float PowerSaved, PowerAdded;
    Dar_LibObj_t * pObj;
    int Out, k, Class, nNodesSaved, nNodesAdded, nNodesGained;
    abctime clk = Abc_Clock();
    if ( pCut->nLeaves != 4 )
        return;
    // check if the cut exits and assigns leaves and their levels
    if ( !Dar_LibCutMatch(p, pCut) )
        return;
    // mark MFFC of the node
    nNodesSaved = Dar_LibCutMarkMffc( p->pAig, pRoot, pCut->nLeaves, p->pPars->fPower? &PowerSaved : NULL );
    // evaluate the cut
    Class = s_DarLib->pMap[pCut->uTruth];
    Dar_LibEvalAssignNums( p, Class, pRoot );
    // profile outputs by their savings
    p->nTotalSubgs += s_DarLib->nSubgr0[Class];
    p->ClassSubgs[Class] += s_DarLib->nSubgr0[Class];
    for ( Out = 0; Out < s_DarLib->nSubgr0[Class]; Out++ )
    {
        pObj = Dar_LibObj(s_DarLib, s_DarLib->pSubgr0[Class][Out]);
        if ( Aig_Regular(s_DarLib->pDatas[pObj->Num].pFunc) == pRoot )
            continue;
        nNodesAdded = Dar_LibEval_rec( pObj, Out, nNodesSaved - !p->pPars->fUseZeros, Required, p->pPars->fPower? &PowerAdded : NULL );
        nNodesGained = nNodesSaved - nNodesAdded;
        if ( p->pPars->fPower && PowerSaved < PowerAdded )
            continue;
        if ( fTraining && nNodesGained >= 0 )
            Dar_LibIncrementScore( Class, Out, nNodesGained + 1 );
        if ( nNodesGained < 0 || (nNodesGained == 0 && !p->pPars->fUseZeros) )
            continue;
        if ( nNodesGained <  p->GainBest || 
            (nNodesGained == p->GainBest && s_DarLib->pDatas[pObj->Num].Level >= p->LevelBest) )
            continue;
        // remember this possibility
        Vec_PtrClear( p->vLeavesBest );
        for ( k = 0; k < (int)pCut->nLeaves; k++ )
            Vec_PtrPush( p->vLeavesBest, s_DarLib->pDatas[k].pFunc );
        p->OutBest    = s_DarLib->pSubgr0[Class][Out];
        p->OutNumBest = Out;
        p->LevelBest  = s_DarLib->pDatas[pObj->Num].Level;
        p->GainBest   = nNodesGained;
        p->ClassBest  = Class;
        assert( p->LevelBest <= Required );
        *pnMffcSize   = nNodesSaved;
    }
clk = Abc_Clock() - clk;
p->ClassTimes[Class] += clk;
p->timeEval += clk;
}

/**Function*************************************************************

  Synopsis    [Clears the fields of the nodes used in this cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_LibBuildClear_rec( Dar_LibObj_t * pObj, int * pCounter )
{
    if ( pObj->fTerm )
        return;
    pObj->Num = (*pCounter)++;
    s_DarLib->pDatas[ pObj->Num ].pFunc = NULL;
    Dar_LibBuildClear_rec( Dar_LibObj(s_DarLib, pObj->Fan0), pCounter );
    Dar_LibBuildClear_rec( Dar_LibObj(s_DarLib, pObj->Fan1), pCounter );
}

/**Function*************************************************************

  Synopsis    [Reconstructs the best cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Dar_LibBuildBest_rec( Dar_Man_t * p, Dar_LibObj_t * pObj )
{
    Aig_Obj_t * pFanin0, * pFanin1;
    Dar_LibDat_t * pData = s_DarLib->pDatas + pObj->Num;
    if ( pData->pFunc )
        return pData->pFunc;
    pFanin0 = Dar_LibBuildBest_rec( p, Dar_LibObj(s_DarLib, pObj->Fan0) );
    pFanin1 = Dar_LibBuildBest_rec( p, Dar_LibObj(s_DarLib, pObj->Fan1) );
    pFanin0 = Aig_NotCond( pFanin0, pObj->fCompl0 );
    pFanin1 = Aig_NotCond( pFanin1, pObj->fCompl1 );
    pData->pFunc = Aig_And( p->pAig, pFanin0, pFanin1 );
//    assert( pData->Level == (int)Aig_Regular(pData->pFunc)->Level );
    return pData->pFunc;
}

/**Function*************************************************************

  Synopsis    [Reconstructs the best cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Dar_LibBuildBest( Dar_Man_t * p )
{
    int i, Counter = 4;
    for ( i = 0; i < Vec_PtrSize(p->vLeavesBest); i++ )
        s_DarLib->pDatas[i].pFunc = (Aig_Obj_t *)Vec_PtrEntry( p->vLeavesBest, i );
    Dar_LibBuildClear_rec( Dar_LibObj(s_DarLib, p->OutBest), &Counter );
    return Dar_LibBuildBest_rec( p, Dar_LibObj(s_DarLib, p->OutBest) );
}






/**Function*************************************************************

  Synopsis    [Matches the cut with its canonical form.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar2_LibCutMatch( Gia_Man_t * p, Vec_Int_t * vCutLits, unsigned uTruth )
{
    unsigned uPhase;
    char * pPerm;
    int i;
    assert( Vec_IntSize(vCutLits) == 4 );
    // get the fanin permutation
    uPhase = s_DarLib->pPhases[uTruth];
    pPerm  = s_DarLib->pPerms4[ (int)s_DarLib->pPerms[uTruth] ];
    // collect fanins with the corresponding permutation/phase
    for ( i = 0; i < Vec_IntSize(vCutLits); i++ )
    {
//        pFanin = Gia_ManObj( p, pCut->pLeaves[ (int)pPerm[i] ] );
//        pFanin = Gia_ManObj( p, Vec_IntEntry( vCutLits, (int)pPerm[i] ) );
//        pFanin = Gia_ObjFromLit( p, Vec_IntEntry( vCutLits, (int)pPerm[i] ) );
        s_DarLib->pDatas[i].iGunc = Abc_LitNotCond( Vec_IntEntry(vCutLits, (int)pPerm[i]), ((uPhase >> i) & 1) );
        s_DarLib->pDatas[i].Level = Gia_ObjLevel( p, Gia_Regular(Gia_ObjFromLit(p, s_DarLib->pDatas[i].iGunc)) );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Assigns numbers to the nodes of one class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar2_LibEvalAssignNums( Gia_Man_t * p, int Class )
{
    Dar_LibObj_t * pObj;
    Dar_LibDat_t * pData, * pData0, * pData1;
    int iFanin0, iFanin1, i, iLit;
    for ( i = 0; i < s_DarLib->nNodes0[Class]; i++ )
    {
        // get one class node, assign its temporary number and set its data
        pObj = Dar_LibObj(s_DarLib, s_DarLib->pNodes0[Class][i]);
        pObj->Num = 4 + i;
        assert( (int)pObj->Num < s_DarLib->nNodes0Max + 4 );
        pData = s_DarLib->pDatas + pObj->Num;
        pData->fMffc = 0;
        pData->iGunc = -1;
        pData->TravId = 0xFFFF;

        // explore the fanins
        assert( (int)Dar_LibObj(s_DarLib, pObj->Fan0)->Num < s_DarLib->nNodes0Max + 4 );
        assert( (int)Dar_LibObj(s_DarLib, pObj->Fan1)->Num < s_DarLib->nNodes0Max + 4 );
        pData0 = s_DarLib->pDatas + Dar_LibObj(s_DarLib, pObj->Fan0)->Num;
        pData1 = s_DarLib->pDatas + Dar_LibObj(s_DarLib, pObj->Fan1)->Num;
        pData->Level = 1 + Abc_MaxInt(pData0->Level, pData1->Level);
        if ( pData0->iGunc == -1 || pData1->iGunc == -1 )
            continue;
        iFanin0 = Abc_LitNotCond( pData0->iGunc, pObj->fCompl0 );
        iFanin1 = Abc_LitNotCond( pData1->iGunc, pObj->fCompl1 );
        // compute the resulting literal
        if ( iFanin0 == 0 || iFanin1 == 0 || iFanin0 == Abc_LitNot(iFanin1) )
            iLit = 0;
        else if ( iFanin0 == 1 || iFanin0 == iFanin1 )
            iLit = iFanin1;
        else if ( iFanin1 == 1 )
            iLit = iFanin0;
        else
        {
            iLit = Gia_ManHashLookup( p, Gia_ObjFromLit(p, iFanin0), Gia_ObjFromLit(p, iFanin1) );
            if ( iLit == 0 )
                iLit = -1;
        }
        pData->iGunc = iLit;
        if ( pData->iGunc >= 0 )
        {
            // update the level to be more accurate
            pData->Level = Gia_ObjLevel( p, Gia_Regular(Gia_ObjFromLit(p, pData->iGunc)) );
            // mark the node if it is part of MFFC
//            pData->fMffc = Gia_ObjIsTravIdCurrentArray(p, Gia_Regular(pData->pGunc));
        }
    }
}

/**Function*************************************************************

  Synopsis    [Evaluates one cut.]

  Description [Returns the best gain.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar2_LibEval_rec( Dar_LibObj_t * pObj, int Out )
{
    Dar_LibDat_t * pData;
    int Area;
    pData = s_DarLib->pDatas + pObj->Num;
    if ( pData->TravId == Out )
        return 0;
    pData->TravId = Out;
    if ( pObj->fTerm )
        return 0;
    assert( pObj->Num > 3 );
    if ( pData->iGunc >= 0 )//&& !pData->fMffc )
        return 0;
    // this is a new node - get a bound on the area of its branches
//    nNodesSaved--;
    Area = Dar2_LibEval_rec( Dar_LibObj(s_DarLib, pObj->Fan0), Out );
//    if ( Area > nNodesSaved )
//        return 0xff;
    Area += Dar2_LibEval_rec( Dar_LibObj(s_DarLib, pObj->Fan1), Out );
//    if ( Area > nNodesSaved )
//        return 0xff;
    return Area + 1;
}

/**Function*************************************************************

  Synopsis    [Evaluates one cut.]

  Description [Returns the best gain.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar2_LibEval( Gia_Man_t * p, Vec_Int_t * vCutLits, unsigned uTruth, int fKeepLevel, Vec_Int_t * vLeavesBest2 )
{
    int p_OutBest    = -1;
    int p_OutNumBest = -1;
    int p_LevelBest  =  1000000;
    int p_GainBest   = -1000000;
    int p_ClassBest  = -1;
//    int fTraining    =  0;
    Dar_LibObj_t * pObj;
    int Out, k, Class, nNodesSaved, nNodesAdded, nNodesGained;
//    abctime clk = Abc_Clock();
    assert( Vec_IntSize(vCutLits) == 4 );
    assert( (uTruth >> 16) == 0 );
    // check if the cut exits and assigns leaves and their levels
    if ( !Dar2_LibCutMatch(p, vCutLits, uTruth) )
        return -1;
    // mark MFFC of the node
//    nNodesSaved = Dar2_LibCutMarkMffc( p->pAig, pRoot, pCut->nLeaves, p->pPars->fPower? &PowerSaved : NULL );
    nNodesSaved = 0;
    // evaluate the cut
    Class = s_DarLib->pMap[uTruth];
    Dar2_LibEvalAssignNums( p, Class );
    // profile outputs by their savings
//    p->nTotalSubgs += s_DarLib->nSubgr0[Class];
//    p->ClassSubgs[Class] += s_DarLib->nSubgr0[Class];
    for ( Out = 0; Out < s_DarLib->nSubgr0[Class]; Out++ )
    {
        pObj = Dar_LibObj(s_DarLib, s_DarLib->pSubgr0[Class][Out]);
//        nNodesAdded = Dar2_LibEval_rec( pObj, Out, nNodesSaved - !p->pPars->fUseZeros, Required, p->pPars->fPower? &PowerAdded : NULL );
        nNodesAdded = Dar2_LibEval_rec( pObj, Out );
        nNodesGained = nNodesSaved - nNodesAdded;
        if ( fKeepLevel )
        {
            if ( s_DarLib->pDatas[pObj->Num].Level >  p_LevelBest || 
                (s_DarLib->pDatas[pObj->Num].Level == p_LevelBest && nNodesGained <= p_GainBest) )
                continue;
        }
        else
        {
            if ( nNodesGained <  p_GainBest || 
                (nNodesGained == p_GainBest && s_DarLib->pDatas[pObj->Num].Level >= p_LevelBest) )
                continue;
        }
        // remember this possibility
        Vec_IntClear( vLeavesBest2 );
        for ( k = 0; k < Vec_IntSize(vCutLits); k++ )
            Vec_IntPush( vLeavesBest2, s_DarLib->pDatas[k].iGunc );
        p_OutBest    = s_DarLib->pSubgr0[Class][Out];
        p_OutNumBest = Out;
        p_LevelBest  = s_DarLib->pDatas[pObj->Num].Level;
        p_GainBest   = nNodesGained;
        p_ClassBest  = Class;
//        assert( p_LevelBest <= Required );
    }
//clk = Abc_Clock() - clk;
//p->ClassTimes[Class] += clk;
//p->timeEval += clk;
    assert( p_OutBest != -1 );
    return p_OutBest;
}

/**Function*************************************************************

  Synopsis    [Clears the fields of the nodes used i this cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar2_LibBuildClear_rec( Dar_LibObj_t * pObj, int * pCounter )
{
    if ( pObj->fTerm )
        return;
    pObj->Num = (*pCounter)++;
    s_DarLib->pDatas[ pObj->Num ].iGunc = -1;
    Dar2_LibBuildClear_rec( Dar_LibObj(s_DarLib, pObj->Fan0), pCounter );
    Dar2_LibBuildClear_rec( Dar_LibObj(s_DarLib, pObj->Fan1), pCounter );
}

/**Function*************************************************************

  Synopsis    [Reconstructs the best cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar2_LibBuildBest_rec( Gia_Man_t * p, Dar_LibObj_t * pObj )
{
    Gia_Obj_t * pNode;
    Dar_LibDat_t * pData;
    int iFanin0, iFanin1;
    pData = s_DarLib->pDatas + pObj->Num;
    if ( pData->iGunc >= 0 )
        return pData->iGunc;
    iFanin0 = Dar2_LibBuildBest_rec( p, Dar_LibObj(s_DarLib, pObj->Fan0) );
    iFanin1 = Dar2_LibBuildBest_rec( p, Dar_LibObj(s_DarLib, pObj->Fan1) );
    iFanin0 = Abc_LitNotCond( iFanin0, pObj->fCompl0 );
    iFanin1 = Abc_LitNotCond( iFanin1, pObj->fCompl1 );
    pData->iGunc = Gia_ManHashAnd( p, iFanin0, iFanin1 );
    pNode = Gia_ManObj( p, Abc_Lit2Var(pData->iGunc) );
    if ( Gia_ObjIsAnd( pNode ) )
        Gia_ObjSetAndLevel( p, pNode );
    Gia_ObjSetPhase( p, pNode );
    return pData->iGunc;
}

/**Function*************************************************************

  Synopsis    [Reconstructs the best cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar2_LibBuildBest( Gia_Man_t * p, Vec_Int_t * vLeavesBest2, int OutBest )
{
    int i, iLeaf, Counter = 4;
    assert( Vec_IntSize(vLeavesBest2) == 4 );
    Vec_IntForEachEntry( vLeavesBest2, iLeaf, i )
        s_DarLib->pDatas[i].iGunc = iLeaf;
    Dar2_LibBuildClear_rec( Dar_LibObj(s_DarLib, OutBest), &Counter );
    return Dar2_LibBuildBest_rec( p, Dar_LibObj(s_DarLib, OutBest) );
}

/**Function*************************************************************

  Synopsis    [Evaluate and build the new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar_LibEvalBuild( Gia_Man_t * p, Vec_Int_t * vCutLits, unsigned uTruth, int fKeepLevel, Vec_Int_t * vLeavesBest2 )
{
    int OutBest = Dar2_LibEval( p, vCutLits, uTruth, fKeepLevel, vLeavesBest2 );
    return Dar2_LibBuildBest( p, vLeavesBest2, OutBest );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

