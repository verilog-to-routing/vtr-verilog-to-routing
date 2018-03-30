/**CFile****************************************************************

  FileName    [abcIfif.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Experiment with technology mapping.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcIfif.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "map/if/if.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define IFIF_MAX_LEAVES 6


typedef struct Abc_IffObj_t_       Abc_IffObj_t;
struct Abc_IffObj_t_ 
{
    float            Delay[IFIF_MAX_LEAVES+1];     // separate delay
//    int              nLeaves;
//    int              pLeaves[IFIF_MAX_LEAVES];
};

typedef struct Abc_IffMan_t_       Abc_IffMan_t;
struct Abc_IffMan_t_ 
{
    Abc_Ntk_t *      pNtk;
    Ifif_Par_t *     pPars;
    // internal data
    int              nObjs;
    Abc_IffObj_t *   pObjs;
};

static inline Abc_IffObj_t *  Abc_IffObj( Abc_IffMan_t * p, int i )                             { assert( i >= 0 && i < p->nObjs ); return p->pObjs + i;   }
static inline float           Abc_IffDelay( Abc_IffMan_t * p, Abc_Obj_t * pObj, int fDelay1 )   { return Abc_IffObj(p, Abc_ObjId(pObj))->Delay[fDelay1];   }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_IffMan_t * Abc_NtkIfifStart( Abc_Ntk_t * pNtk, Ifif_Par_t * pPars )
{
    Abc_IffMan_t * p;
    p = ABC_CALLOC( Abc_IffMan_t, 1 );
    p->pNtk       = pNtk;
    p->pPars      = pPars;
    // internal data
    p->nObjs      = Abc_NtkObjNumMax( pNtk );
    p->pObjs      = ABC_CALLOC( Abc_IffObj_t, p->nObjs );
    return p;
}
void Abc_NtkIfifStop( Abc_IffMan_t * p )
{
    // internal data
    ABC_FREE( p->pObjs );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Collects fanins into ppNodes in decreasing order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjSortByDelay( Abc_IffMan_t * p, Abc_Obj_t * pObj, int fDelay1, Abc_Obj_t ** ppNodes )
{
    Abc_Obj_t * pFanin;
    int i, a, k = 0;
    Abc_ObjForEachFanin( pObj, pFanin, i )
    {
        ppNodes[k++] = pFanin;
        if ( Abc_ObjIsCi(pFanin) )
            continue;
        for ( a = k-1; a > 0; a-- )
            if ( Abc_IffDelay(p, ppNodes[a-1], fDelay1) + p->pPars->pLutDelays[a-1] < Abc_IffDelay(p, ppNodes[a], fDelay1) + p->pPars->pLutDelays[a] )
                ABC_SWAP( Abc_Obj_t *, ppNodes[a-1], ppNodes[a] );
    }
/*
    for ( a = 1; a < k; a++ )
    {
        float D1 = Abc_IffDelay(p, ppNodes[a-1], fDelay1);
        float D2 = Abc_IffDelay(p, ppNodes[a], fDelay1);
        if ( Abc_ObjIsCi(ppNodes[a-1]) || Abc_ObjIsCi(ppNodes[a]) )
            continue;
        assert( Abc_IffDelay(p, ppNodes[a-1], fDelay1) + p->pPars->pLutDelays[a-1] >= Abc_IffDelay(p, ppNodes[a], fDelay1) + p->pPars->pLutDelays[a] - 0.01 );
    }
*/
}

/**Function*************************************************************

  Synopsis    [This is the delay which object has all by itself.]

  Description [This delay is stored in Delay0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_ObjDelay0( Abc_IffMan_t * p, Abc_Obj_t * pObj )
{
    int i;
    float Delay0 = 0;
    Abc_Obj_t * ppNodes[6];
    Abc_ObjSortByDelay( p, pObj, 1, ppNodes );
    for ( i = 0; i < Abc_ObjFaninNum(pObj); i++ )
        Delay0 = Abc_MaxFloat( Delay0, Abc_IffDelay(p, ppNodes[i], 1) + p->pPars->pLutDelays[i] );
    return Delay0;
}

/**Function*************************************************************

  Synopsis    [This is the delay object has in the structure.]

  Description [This delay is stored in Delay1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_ObjDelay1( Abc_IffMan_t * p, Abc_Obj_t * pObj )
{
    int i, fVeryVerbose = 0;
//    Abc_IffObj_t * pIfif = Abc_IffObj( p, Abc_ObjId(pObj) );
    Abc_Obj_t * ppNodes[6];
    float Delay1, DelayNew;

    if ( Abc_ObjFaninNum(pObj) == 0 )
        return 0;

    // sort fanins by delay1+LutDelay
    Abc_ObjSortByDelay( p, pObj, 1, ppNodes );

    // print verbose results
    if ( fVeryVerbose )
    {
        printf( "Object %d   Level %d\n", Abc_ObjId(pObj), Abc_ObjLevel(pObj) );
        for ( i = 0; i < Abc_ObjFaninNum(pObj); i++ )
        {
            printf( "Fanin %d : ", i );
            printf( "D0 %3.2f  ",      Abc_IffDelay(p, ppNodes[i], 0) );
            printf( "D0* %3.2f     ",  Abc_IffDelay(p, ppNodes[i], 0) + p->pPars->pLutDelays[i] - p->pPars->DelayWire );
            printf( "D1 %3.2f",        Abc_IffDelay(p, ppNodes[i], 1) + p->pPars->pLutDelays[i] );
            printf( "\n" );
        }
        printf( "\n" );
    }
/*
    // for the first nDegree delays, sort them by the minimum Delay1+LutDelay and Delay0-Wire+LutDelay
    Delay1 = 0;
    pIfif->nLeaves = 0;
    for ( i = 0; i < Abc_ObjFaninNum(pObj); i++ )
    {
        if ( Abc_ObjIsNode(ppNodes[i]) && pIfif->nLeaves < p->pPars->nDegree )
        {
            DelayNew = Abc_MinFloat( Abc_IffDelay(p, ppNodes[i], 1) + p->pPars->pLutDelays[i], 
                                     Abc_IffDelay(p, ppNodes[i], 0) + p->pPars->pLutDelays[i] - p->pPars->DelayWire );
            pIfif->pLeaves[pIfif->nLeaves++] = Abc_ObjId(ppNodes[i]);
        }
        else
            DelayNew = Abc_IffDelay(p, ppNodes[i], 1) + p->pPars->pLutDelays[i];
        Delay1 = Abc_MaxFloat( Delay1, DelayNew );
    }
*/
    // for the first nDegree delays, sort them by the minimum Delay1+LutDelay and Delay0-Wire+LutDelay
    Delay1 = 0;
    for ( i = 0; i < Abc_ObjFaninNum(pObj); i++ )
    {
        if ( i < p->pPars->nDegree )
            DelayNew = Abc_MinFloat( Abc_IffDelay(p, ppNodes[i], 1) + p->pPars->pLutDelays[i], 
                                     Abc_IffDelay(p, ppNodes[i], 0) + p->pPars->pLutDelays[i] - p->pPars->DelayWire );
        else
            DelayNew = Abc_IffDelay(p, ppNodes[i], 1) + p->pPars->pLutDelays[i];
        Delay1 = Abc_MaxFloat( Delay1, DelayNew );
    }
    assert( Delay1 > 0 );
    return Delay1;
}


/**Function*************************************************************

  Synopsis    [This is the delay which object has all by itself.]

  Description [This delay is stored in Delay0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_ObjDelayDegree( Abc_IffMan_t * p, Abc_Obj_t * pObj, int d )
{
    int i;
    float Delay0 = 0, DelayNew;
    Abc_Obj_t * ppNodes[6];
    assert( d >= 0 && d <= p->pPars->nDegree );
    Abc_ObjSortByDelay( p, pObj, p->pPars->nDegree, ppNodes );
    for ( i = 0; i < Abc_ObjFaninNum(pObj); i++ )
    {
        DelayNew = Abc_IffDelay(p, ppNodes[i], p->pPars->nDegree) + p->pPars->pLutDelays[i];
        if ( i == 0 && d > 0 )
            DelayNew = Abc_MinFloat(DelayNew, Abc_IffDelay(p, ppNodes[i], d-1) + p->pPars->pLutDelays[i] - p->pPars->DelayWire );
        Delay0 = Abc_MaxFloat( Delay0, DelayNew );
    }
    return Delay0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPerformIfif( Abc_Ntk_t * pNtk, Ifif_Par_t * pPars )
{
    Abc_IffMan_t * p;
    Abc_IffObj_t * pIffObj;
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    float Delay, Delay10, DegreeFinal;
    int i, d, Count10;
    assert( pPars->pLutLib->LutMax >= 0 && pPars->pLutLib->LutMax <= IFIF_MAX_LEAVES );
    assert( pPars->nLutSize >= 0 && pPars->nLutSize <= IFIF_MAX_LEAVES );
    assert( pPars->nDegree >= 0 && pPars->nDegree <= IFIF_MAX_LEAVES );
    // convert to AIGs
    Abc_NtkToAig( pNtk );
    Abc_NtkLevel( pNtk );

    // print parameters
    if ( pPars->fVerbose )
    {
        printf( "Running mapper into LUT structures with the following parameters:\n" );
        printf( "Pin+Wire: {" );
        for ( i = 0; i < pPars->pLutLib->LutMax; i++ )
            printf( " %3.2f", pPars->pLutDelays[i] );
        printf( " }  " );    
        printf( "Wire %3.2f  Degree %d  Type: %s\n", 
            pPars->DelayWire, pPars->nDegree, pPars->fCascade? "Cascade" : "Cluster" );
    }

    // start manager
    p = Abc_NtkIfifStart( pNtk, pPars );
//    printf( "Running experiment with LUT delay %d and degree %d (LUT size is %d).\n", DelayWire, nDegree, nLutSize );

    // compute the delay
    vNodes = Abc_NtkDfs( pNtk, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        assert( Abc_ObjIsNode(pObj) );
        pIffObj = Abc_IffObj( p, Abc_ObjId(pObj) );

        if ( pPars->fCascade )
        {
            for ( d = 0; d <= pPars->nDegree; d++ )
                pIffObj->Delay[d] = Abc_ObjDelayDegree( p, pObj, d );
        }
        else
        {
            pIffObj->Delay[0] = Abc_ObjDelay0( p, pObj );
            pIffObj->Delay[1] = Abc_ObjDelay1( p, pObj );
        }
    }

    // get final degree number
    if ( pPars->fCascade )
        DegreeFinal = pPars->nDegree;
    else
        DegreeFinal = 1;

    if ( p->pPars->fVeryVerbose )
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        printf( "Node %3d : Lev =%3d   ",  Abc_ObjId(pObj), Abc_ObjLevel(pObj) );
        for ( d = 0; d <= DegreeFinal; d++ )
            printf( "Del%d =%4.2f  ", d, Abc_IffDelay(p, pObj, d) );
        printf( "\n" );
    }
    Vec_PtrFree( vNodes );


    // consider delay at the outputs
    Delay = 0;
    Abc_NtkForEachCo( pNtk, pObj, i )
        Delay = Abc_MaxFloat( Delay, Abc_IffDelay(p, Abc_ObjFanin0(pObj), DegreeFinal) );
    Delay10 = 0.9 * Delay;

    // consider delay at the outputs
    Count10 = 0;
    Abc_NtkForEachCo( pNtk, pObj, i )
        if ( Abc_IffDelay(p, Abc_ObjFanin0(pObj), DegreeFinal) >= Delay10 )
            Count10++;

    printf( "Critical delay %5.2f. Critical outputs %5.2f %%\n", Delay, 100.0 * Count10 / Abc_NtkCoNum(pNtk) );
//    printf( "%.2f %.2f\n", Delay, 100.0 * Count10 / Abc_NtkCoNum(pNtk) );

    // derive a new network

    // stop manager
    Abc_NtkIfifStop( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

