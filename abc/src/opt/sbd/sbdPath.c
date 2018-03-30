/**CFile****************************************************************

  FileName    [sbdPath.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [Critical path.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sbdPath.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sbdInt.h"
#include "misc/tim/tim.h"

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
int Sbc_ManAddInternalToPath_rec( Gia_Man_t * p, int iObj, Vec_Bit_t * vPath )
{
    Gia_Obj_t * pObj; 
    int k, iFan, Value = 0;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return Vec_BitEntry(vPath, iObj);
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsCi(pObj) )
        return Vec_BitEntry(vPath, iObj);
    assert( Gia_ObjIsAnd(pObj) );
    Gia_LutForEachFanin( p, iObj, iFan, k )
        Value |= Sbc_ManAddInternalToPath_rec( p, iFan, vPath );
    if ( Value )
        Vec_BitWriteEntry( vPath, iObj, 1 );
    return Value;
}
void Sbc_ManAddInternalToPath( Gia_Man_t * p, Vec_Bit_t * vPath )
{
    int k, iFan, iObj;
    Gia_ManForEachLut( p, iObj )
    {
        if ( !Vec_BitEntry(vPath, iObj) )
            continue;
        Gia_ManIncrementTravId( p );
        Gia_LutForEachFanin( p, iObj, iFan, k )
            Gia_ObjSetTravIdCurrentId(p, iFan);
        Sbc_ManAddInternalToPath_rec( p, iObj, vPath );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sbc_ManCriticalPath_rec( Gia_Man_t * p, int * pLevels, int iObj, int LevelFan, Vec_Bit_t * vPath, int Slack )
{
    Gia_Obj_t * pObj; int k, iFan;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    Vec_BitWriteEntry( vPath, iObj, 1 );
    if ( Gia_ObjIsCi(pObj) )
    {
        Tim_Man_t * pManTime = (Tim_Man_t *)p->pManTime;
        int iBox = pManTime ? Tim_ManBoxForCi( pManTime, Gia_ObjCioId(pObj) ) : -1;
        if ( iBox >= 0 )
        {
            int curCo = Tim_ManBoxInputFirst( pManTime, iBox );
            int nBoxInputs = Tim_ManBoxInputNum( pManTime, iBox );
            for ( k = 0; k < nBoxInputs; k++ )
            {
                Gia_Obj_t * pCo = Gia_ManCo( p, curCo + k );
                int iDriver = Gia_ObjFaninId0p( p, pCo );
                if ( (pLevels[iDriver]+Slack >= LevelFan-1) && iDriver )
                    Sbc_ManCriticalPath_rec( p, pLevels, iDriver, pLevels[iDriver], vPath, Abc_MaxInt(0, pLevels[iDriver]+Slack-(LevelFan-1)) );
            }
        }
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Gia_LutForEachFanin( p, iObj, iFan, k )
        if ( pLevels[iFan]+Slack >= LevelFan-1 )
            Sbc_ManCriticalPath_rec( p, pLevels, iFan, pLevels[iFan], vPath, Abc_MaxInt(0, pLevels[iFan]+Slack-(LevelFan-1)) );
}
Vec_Bit_t * Sbc_ManCriticalPath( Gia_Man_t * p )
{
    int * pLevels = NULL, k, iDriver, Slack = 1;
    int nLevels = p->pManTime ? Gia_ManLutLevelWithBoxes(p) : Gia_ManLutLevel(p, &pLevels);
    Vec_Bit_t * vPath = Vec_BitStart( Gia_ManObjNum(p) );
    if ( p->pManTime )
        pLevels = Vec_IntArray( p->vLevels );
    Gia_ManIncrementTravId( p );
    Gia_ManForEachCoDriverId( p, iDriver, k )
        if ( (pLevels[iDriver] == nLevels) && iDriver )
            Sbc_ManCriticalPath_rec( p, pLevels, iDriver, pLevels[iDriver], vPath, Slack );
    if ( !p->pManTime )
        ABC_FREE( pLevels );
    Sbc_ManAddInternalToPath( p, vPath );
    return vPath;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sbc_ManDelayTrace( Gia_Man_t * p )
{
    Vec_Bit_t * vPath = Vec_BitStart( Gia_ManObjNum(p) );
    int i, k, iFan, nLevels, * pLevels;
    int nLuts = 0, nNodes = 0, nEdges = 0, nEdgesAll = 0;
    if ( !Gia_ManHasMapping(p) )
    {
        printf( "No mapping is available.\n" );
        return;
    }
    assert( Gia_ManHasMapping(p) );
    // set critical CO drivers
    nLevels = Gia_ManLutLevel( p, &pLevels );
    Gia_ManForEachCoDriverId( p, iFan, i )
        if ( pLevels[iFan] == nLevels )
            Vec_BitWriteEntry( vPath, iFan, 1 );
    // set critical internal nodes
    Gia_ManForEachLutReverse( p, i )
    { 
        nLuts++;
        if ( !Vec_BitEntry(vPath, i) )
            continue;
        nNodes++;
        Gia_LutForEachFanin( p, i, iFan, k )
        {
            if ( pLevels[iFan] +1 < pLevels[i] )
                continue;
            assert( pLevels[iFan] + 1 == pLevels[i] );
            Vec_BitWriteEntry( vPath, iFan, 1 );
            nEdges++;
            //printf( "%d -> %d\n", i, iFan );
        }
    }
    Gia_ManForEachLut( p, i )
        Gia_LutForEachFanin( p, i, iFan, k )
            nEdgesAll += (Vec_BitEntry(vPath, i) && Vec_BitEntry(vPath, iFan));

    ABC_FREE( pLevels );
    Vec_BitFree( vPath );
    printf( "AIG = %d. LUT = %d. Lev = %d.   Path nodes = %d.  Path edges = %d. (%d.)\n", 
        Gia_ManAndNum(p), nLuts, nLevels, nNodes, nEdges, nEdgesAll );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

