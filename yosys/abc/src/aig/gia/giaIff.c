/**CFile****************************************************************

  FileName    [giaIff.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Hierarchical mapping of AIG with white boxes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaIff.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "map/if/if.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Iff_Man_t_ Iff_Man_t;
struct Iff_Man_t_
{
    Gia_Man_t *     pGia;       // mapped GIA
    If_LibLut_t *   pLib;       // LUT library
    int             nLutSize;   // LUT size
    int             nDegree;    // degree 
    Vec_Flt_t *     vTimes;     // arrival times
    Vec_Int_t *     vMatch[4];  // matches
};

static inline float Iff_ObjTimeId( Iff_Man_t * p, int iObj )                                { return Vec_FltEntry( p->vTimes, iObj );                       }
static inline float Iff_ObjTime( Iff_Man_t * p, Gia_Obj_t * pObj )                          { return Iff_ObjTimeId( p, Gia_ObjId(p->pGia, pObj) );          }
static inline void  Iff_ObjSetTimeId( Iff_Man_t * p, int iObj, float Time )                 { Vec_FltWriteEntry( p->vTimes, iObj, Time );                   }
static inline void  Iff_ObjSetTime( Iff_Man_t * p, Gia_Obj_t * pObj, float Time )           { Iff_ObjSetTimeId( p, Gia_ObjId(p->pGia, pObj), Time );        }

static inline int   Iff_ObjMatchId( Iff_Man_t * p, int iObj, int Type )                     { return Vec_IntEntry( p->vMatch[Type], iObj );                 }
static inline int   Iff_ObjMatch( Iff_Man_t * p, Gia_Obj_t * pObj, int Type )               { return Iff_ObjMatchId( p, Gia_ObjId(p->pGia, pObj), Type );   }
static inline void  Iff_ObjSetMatchId( Iff_Man_t * p, int iObj, int Type, int Match )       { Vec_IntWriteEntry( p->vMatch[Type], iObj, Match );            }
static inline void  Iff_ObjSetMatch( Iff_Man_t * p, Gia_Obj_t * pObj, int Type, int Match ) { Iff_ObjSetMatchId( p, Gia_ObjId(p->pGia, pObj), Type, Match );}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Iff_Man_t * Gia_ManIffStart( Gia_Man_t * pGia )
{
    Iff_Man_t * p = ABC_CALLOC( Iff_Man_t, 1 );
    p->vTimes    = Vec_FltStartFull( Gia_ManObjNum(pGia) );
    p->vMatch[2] = Vec_IntStartFull( Gia_ManObjNum(pGia) );
    p->vMatch[3] = Vec_IntStartFull( Gia_ManObjNum(pGia) );
    return p;
}
void Gia_ManIffStop( Iff_Man_t * p )
{
    Vec_FltFree( p->vTimes );
    Vec_IntFree( p->vMatch[2] );
    Vec_IntFree( p->vMatch[3] );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Count the number of unique fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_IffObjCount( Gia_Man_t * pGia, int iObj, int iFaninSkip2, int iFaninSkip3 )
{
    int i, iFanin, Count = 0;
    Gia_ManIncrementTravId( pGia );
    Gia_LutForEachFanin( pGia, iObj, iFanin, i )
    {
        if ( iFanin == iFaninSkip2 || iFanin == iFaninSkip3 )
            continue;
        if ( Gia_ObjIsTravIdCurrentId( pGia, iFanin ) )
            continue;
        Gia_ObjSetTravIdCurrentId( pGia, iFanin );
        Count++;
    }
    if ( iFaninSkip2 >= 0 )
    {
        Gia_LutForEachFanin( pGia, iFaninSkip2, iFanin, i )
        {
            if ( iFanin == iFaninSkip3 )
                continue;
            if ( Gia_ObjIsTravIdCurrentId( pGia, iFanin ) )
                continue;
            Gia_ObjSetTravIdCurrentId( pGia, iFanin );
            Count++;
        }
    }
    if ( iFaninSkip3 >= 0 )
    {
        Gia_LutForEachFanin( pGia, iFaninSkip3, iFanin, i )
        {
            if ( iFanin == iFaninSkip2 )
                continue;
            if ( Gia_ObjIsTravIdCurrentId( pGia, iFanin ) )
                continue;
            Gia_ObjSetTravIdCurrentId( pGia, iFanin );
            Count++;
        }
    }
    return Count;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Gia_IffObjTimeOne( Iff_Man_t * p, int iObj, int iFaninSkip2, int iFaninSkip3 )
{
    int i, iFanin;
    float DelayMax = -ABC_INFINITY;
    Gia_LutForEachFanin( p->pGia, iObj, iFanin, i )
        if ( iFanin != iFaninSkip2 && iFanin != iFaninSkip3 && DelayMax < Iff_ObjTimeId(p, iFanin) )
            DelayMax = Iff_ObjTimeId(p, iFanin);
    assert( i == Gia_ObjLutSize(p->pGia, iObj) );
    if ( iFaninSkip2 == -1 )
        return DelayMax;
    Gia_LutForEachFanin( p->pGia, iFaninSkip2, iFanin, i )
        if ( iFanin != iFaninSkip3 && DelayMax < Iff_ObjTimeId(p, iFanin) )
            DelayMax = Iff_ObjTimeId(p, iFanin);
    if ( iFaninSkip3 == -1 )
        return DelayMax;
    Gia_LutForEachFanin( p->pGia, iFaninSkip3, iFanin, i )
        if ( iFanin != iFaninSkip2 && DelayMax < Iff_ObjTimeId(p, iFanin) )
            DelayMax = Iff_ObjTimeId(p, iFanin);
    assert( DelayMax >= 0 );
    return DelayMax;
}
float Gia_IffObjTimeTwo( Iff_Man_t * p, int iObj, int * piFanin, float DelayMin )
{
    int i, iFanin, nSize;
    float This;
    *piFanin = -1;
    Gia_LutForEachFanin( p->pGia, iObj, iFanin, i )
    {
        if ( Gia_ObjIsCi(Gia_ManObj(p->pGia, iFanin)) )
            continue;
        This = Gia_IffObjTimeOne( p, iObj, iFanin, -1 );
        nSize = Gia_IffObjCount( p->pGia, iObj, iFanin, -1 );
        assert( nSize <= p->pLib->LutMax );
        This += p->pLib->pLutDelays[nSize][0];
        if ( DelayMin > This )
        {
            DelayMin = This;
            *piFanin = iFanin;
        }
    }
    return DelayMin;
}
float Gia_IffObjTimeThree( Iff_Man_t * p, int iObj, int * piFanin, int * piFanin2, float DelayMin )
{
    int i, k, iFanin, iFanin2, nSize;
    float This;
    *piFanin = -1;
    *piFanin2 = -1;
    Gia_LutForEachFanin( p->pGia, iObj, iFanin, i )
    Gia_LutForEachFanin( p->pGia, iObj, iFanin2, k )
    {
        if ( iFanin == iFanin2 )
            continue;
        if ( Gia_ObjIsCi(Gia_ManObj(p->pGia, iFanin)) )
            continue;
        if ( Gia_ObjIsCi(Gia_ManObj(p->pGia, iFanin2)) )
            continue;
        This  = Gia_IffObjTimeOne( p, iObj, iFanin, iFanin2 );
        nSize = Gia_IffObjCount( p->pGia, iObj, iFanin, iFanin2 );
        assert( nSize <= p->pLib->LutMax );
        This += p->pLib->pLutDelays[nSize][0];
        if ( DelayMin > This )
        {
            DelayMin  = This;
            *piFanin  = iFanin;
            *piFanin2 = iFanin2;
        }
    }
    return DelayMin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Iff_Man_t * Gia_ManIffPerform( Gia_Man_t * pGia, If_LibLut_t * pLib, Tim_Man_t * pTime, int nLutSize, int nDegree )
{
    Iff_Man_t * p;
    Gia_Obj_t * pObj;
    int iObj, iFanin, iFanin1, iFanin2;
    int CountAll = 0, Count2 = 0, Count3 = 0;
    float arrTime1, arrTime2, arrTime3, arrMax = -ABC_INFINITY; 
    assert( nDegree == 2 || nDegree == 3 );
    // start the mapping manager and set its parameters
    p = Gia_ManIffStart( pGia );
    p->pGia     = pGia;
    p->pLib     = pLib;
    p->nLutSize = nLutSize;
    p->nDegree  = nDegree;
    // compute arrival times of each node
    Iff_ObjSetTimeId( p, 0, 0 );
    Tim_ManIncrementTravId( pTime );
    Gia_ManForEachObj1( pGia, pObj, iObj )
    {
        if ( Gia_ObjIsAnd(pObj) )
        {
            if ( !Gia_ObjIsLut(pGia, iObj) )
                continue;
            CountAll++;
            // compute arrival times of LUT inputs
            arrTime1 = Gia_IffObjTimeOne( p, iObj, -1, -1 );
            arrTime1 += p->pLib->pLutDelays[Gia_ObjLutSize(pGia, iObj)][0];
            // compute arrival times of LUT pairs
            arrTime2 = Gia_IffObjTimeTwo( p, iObj, &iFanin, arrTime1 );
            if ( nDegree == 2 )
            {
                // set arrival times
                Iff_ObjSetTimeId( p, iObj, arrTime2 );
                if ( arrTime2 < arrTime1 )
                    Iff_ObjSetMatchId( p, iObj, 2, iFanin ), Count2++;                
            }
            else if ( nDegree == 3 )
            {                
                // compute arrival times of LUT triples
                arrTime3 = Gia_IffObjTimeThree( p, iObj, &iFanin1, &iFanin2, arrTime2 );
                // set arrival times
                Iff_ObjSetTimeId( p, iObj, arrTime3 );
                if ( arrTime3 == arrTime1 )
                    continue;
                if ( arrTime3 == arrTime2 )
                    Iff_ObjSetMatchId( p, iObj, 2, iFanin ), Count2++;
                else
                {
                    assert( arrTime3 < arrTime2 );
                    Iff_ObjSetMatchId( p, iObj, 2, iFanin1 );
                    Iff_ObjSetMatchId( p, iObj, 3, iFanin2 ), Count3++;
                }
            }
            else assert( 0 );
        }
        else if ( Gia_ObjIsCi(pObj) )
        {
            arrTime1 = Tim_ManGetCiArrival( pTime, Gia_ObjCioId(pObj) );
            Iff_ObjSetTime( p, pObj, arrTime1 );
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            arrTime1 = Iff_ObjTimeId( p, Gia_ObjFaninId0p(pGia, pObj) );
            Tim_ManSetCoArrival( pTime, Gia_ObjCioId(pObj), arrTime1 );
            Iff_ObjSetTime( p, pObj, arrTime1 );
            arrMax = Abc_MaxFloat( arrMax, arrTime1 );
        }
        else assert( 0 );
    }
    printf( "Max delay = %.2f.  Count1 = %d.  Count2 = %d.  Count3 = %d.\n", 
        arrMax, CountAll - Count2 - Count3, Count2, Count3 );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
void Gia_ManIffSelect_rec( Iff_Man_t * p, int iObj, Vec_Int_t * vPacking )
{
    int i, iFanin, iFaninSkip2, iFaninSkip3;
    if ( Gia_ObjIsTravIdCurrentId( p->pGia, iObj ) )
        return;
    Gia_ObjSetTravIdCurrentId( p->pGia, iObj );
    assert( Gia_ObjIsLut(p->pGia, iObj) );
    iFaninSkip2 = Iff_ObjMatchId(p, iObj, 2);
    iFaninSkip3 = Iff_ObjMatchId(p, iObj, 3);
    if ( iFaninSkip2 == -1 )
    {
        assert( iFaninSkip3 == -1 );
        Gia_LutForEachFanin( p->pGia, iObj, iFanin, i )
            Gia_ManIffSelect_rec( p, iFanin, vPacking );
        Vec_IntPush( vPacking, 1 );
        Vec_IntPush( vPacking, iObj );
    }
    else if ( iFaninSkip3 == -1 )
    {
        assert( iFaninSkip2 > 0 );
        Gia_LutForEachFanin( p->pGia, iObj, iFanin, i )
            if ( iFanin != iFaninSkip2 )
                Gia_ManIffSelect_rec( p, iFanin, vPacking );
        Gia_LutForEachFanin( p->pGia, iFaninSkip2, iFanin, i )
            Gia_ManIffSelect_rec( p, iFanin, vPacking );
        Vec_IntPush( vPacking, 2 );
        Vec_IntPush( vPacking, iFaninSkip2 );
        Vec_IntPush( vPacking, iObj );
    }
    else
    {
        assert( iFaninSkip2 > 0 && iFaninSkip3 > 0 );
        Gia_LutForEachFanin( p->pGia, iObj, iFanin, i )
            if ( iFanin != iFaninSkip2 && iFanin != iFaninSkip3 )
                Gia_ManIffSelect_rec( p, iFanin, vPacking );
        Gia_LutForEachFanin( p->pGia, iFaninSkip2, iFanin, i )
            if ( iFanin != iFaninSkip3 )
                Gia_ManIffSelect_rec( p, iFanin, vPacking );
        Gia_LutForEachFanin( p->pGia, iFaninSkip3, iFanin, i )
            if ( iFanin != iFaninSkip2 )
                Gia_ManIffSelect_rec( p, iFanin, vPacking );
        Vec_IntPush( vPacking, 3 );
        Vec_IntPush( vPacking, iFaninSkip2 );
        Vec_IntPush( vPacking, iFaninSkip3 );
        Vec_IntPush( vPacking, iObj );
    }
    Vec_IntAddToEntry( vPacking, 0, 1 );
}
Vec_Int_t * Gia_ManIffSelect( Iff_Man_t * p )
{
    Vec_Int_t * vPacking;
    Gia_Obj_t * pObj; int i;
    vPacking = Vec_IntAlloc( Gia_ManObjNum(p->pGia) );
    Vec_IntPush( vPacking, 0 );
    // mark const0 and PIs
    Gia_ManIncrementTravId( p->pGia );
    Gia_ObjSetTravIdCurrentId( p->pGia, 0 );
    Gia_ManForEachCi( p->pGia, pObj, i )
        Gia_ObjSetTravIdCurrent( p->pGia, pObj );
    // recursively collect internal nodes
    Gia_ManForEachCo( p->pGia, pObj, i )
        Gia_ManIffSelect_rec( p, Gia_ObjFaninId0p(p->pGia, pObj), vPacking );
    return vPacking;
}

/**Function*************************************************************

  Synopsis    [This command performs hierarhical mapping.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
void Gia_ManIffTest( Gia_Man_t * pGia, If_LibLut_t * pLib, int fVerbose )
{
    Iff_Man_t * p;
    Tim_Man_t * pTemp = NULL;
    int nDegree = -1;
    int nLutSize = Gia_ManLutSizeMax( pGia );
    if ( nLutSize <= 4 )
    {
        nLutSize = 4;
        if ( pLib->LutMax == 7 )
            nDegree = 2;
        else if ( pLib->LutMax == 10 )
            nDegree = 3;
        else
            { printf( "LUT library for packing 4-LUTs should have 7 or 10 inputs.\n" ); return; }
    }
    else if ( nLutSize <= 6 )
    {
        nLutSize = 6;
        if ( pLib->LutMax == 11 )
            nDegree = 2;
        else if ( pLib->LutMax == 16 )
            nDegree = 3;
        else
            { printf( "LUT library for packing 6-LUTs should have 11 or 16 inputs.\n" ); return; }
    }
    else
    {
        printf( "The LUT size is more than 6.\n" );
        return;
    }
    if ( fVerbose )
        printf( "Performing %d-clustering with %d-LUTs:\n", nDegree, nLutSize );
    // create timing manager
    if ( pGia->pManTime == NULL )
        pGia->pManTime = pTemp = Tim_ManStart( Gia_ManCiNum(pGia), Gia_ManCoNum(pGia) );
    // perform timing computation
    p = Gia_ManIffPerform( pGia, pLib, (Tim_Man_t *)pGia->pManTime, nLutSize, nDegree );
    // remove timing manager
    if ( pGia->pManTime == pTemp )
        pGia->pManTime = NULL;
    Tim_ManStopP( (Tim_Man_t **)&pTemp );
    // derive clustering
    Vec_IntFreeP( &pGia->vPacking );
    pGia->vPacking = Gia_ManIffSelect( p );
    Gia_ManIffStop( p );
    // print statistics
    if ( fVerbose )
        Gia_ManPrintPackingStats( pGia );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

