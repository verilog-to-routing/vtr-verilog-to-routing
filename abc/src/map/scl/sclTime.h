/**CFile****************************************************************

  FileName    [sclTime.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Standard-cell library representation.]

  Synopsis    [Timing/gate-sizing manager.]

  Author      [Alan Mishchenko, Niklas Een]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 24, 2012.]

  Revision    [$Id: sclTime.h,v 1.0 2012/08/24 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__map__scl__sclTime_h
#define ABC__map__scl__sclTime_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "misc/vec/vec.h"
#include "sclLib.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct SC_Time_           SC_Time;
struct SC_Time_ 
{
    SC_Lib *       pLib;          // library
    Vec_Int_t *    vCis;          // comb inputs
    Vec_Int_t *    vCos;          // comb outputs 
    int            nObjs;         // allocated size
    // get assignment
    Vec_Int_t *    vGates;        // mapping of objId into gateId
    // timing information
    SC_Pair *      pLoads;        // loads for each gate
    SC_Pair *      pLoads2;       // loads for each gate
    SC_Pair *      pDepts;        // departures for each gate
    SC_Pair *      pTimes;        // arrivals for each gate
    SC_Pair *      pSlews;        // slews for each gate
    SC_Pair *      pTimes2;       // arrivals for each gate
    SC_Pair *      pSlews2;       // slews for each gate
    float *        pSlack;        // slacks for each gate
    SC_WireLoad *  pWLoadUsed;    // name of the used WireLoad model
    // optimization parameters
    float          SumArea;       // total area
    float          MaxDelay;      // max delay
    float          SumArea0;      // total area at the begining 
    float          MaxDelay0;     // max delay at the begining
    float          BestDelay;     // best delay in the middle
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

static inline SC_Cell * Scl_ObjCell( SC_Time * p, int i )                     { return SC_LibCell( p->pLib, Vec_IntEntry(p->vGates, i) );   }
static inline void      Scl_ObjSetCell( SC_Time * p, int i, SC_Cell * pCell ) { Vec_IntWriteEntry( p->vGates, i, pCell->Id );               }

static inline SC_Pair * Scl_ObjLoad( SC_Time * p, int i )                     { return p->pLoads + i;  }
static inline SC_Pair * Scl_ObjLoad2( SC_Time * p, int i )                    { return p->pLoads2 + i; }
static inline SC_Pair * Scl_ObjDept( SC_Time * p, int i )                     { return p->pDepts + i;  }
static inline SC_Pair * Scl_ObjTime( SC_Time * p, int i )                     { return p->pTimes + i;  }
static inline SC_Pair * Scl_ObjSlew( SC_Time * p, int i )                     { return p->pSlews + i;  }
static inline SC_Pair * Scl_ObjTime2( SC_Time * p, int i )                    { return p->pTimes2 + i; }
static inline SC_Pair * Scl_ObjSlew2( SC_Time * p, int i )                    { return p->pSlews2 + i; }

static inline float     Scl_ObjTimeMax( SC_Time * p, int i )                  { return Abc_MaxFloat(Scl_ObjTime(p, i)->rise, Scl_ObjTime(p, i)->fall);  }
static inline float     Scl_ObjDepthMax( SC_Time * p, int i )                 { return Abc_MaxFloat(Scl_ObjDept(p, i)->rise, Scl_ObjDept(p, i)->fall);  }
static inline float     Scl_ObjGetSlack( SC_Time * p, int i, float D )        { return D - Abc_MaxFloat(Scl_ObjTime(p, i)->rise + Scl_ObjDept(p, i)->rise, Scl_ObjTime(p, i)->fall + Scl_ObjDept(p, i)->fall);  }
static inline float     Scl_ObjGetSlackR( SC_Time * p, int i, float D )       { return D - (Scl_ObjTime(p, i)->rise + Scl_ObjDept(p, i)->rise);  }
static inline float     Scl_ObjGetSlackF( SC_Time * p, int i, float D )       { return D - (Scl_ObjTime(p, i)->fall + Scl_ObjDept(p, i)->fall);  }
static inline float     Scl_ObjSlack( SC_Time * p, int i )                    { return p->pSlack[i];   }

static inline void      Scl_ObjDupFanin( SC_Time * p, int i, int iFanin )     { *Scl_ObjTime(p, i) = *Scl_ObjTime(p, iFanin);                    }
static inline float     Scl_ObjGain( SC_Time * p, int i )                     { return 0.5*((Scl_ObjTime2(p, i)->rise - Scl_ObjTime(p, i)->rise) + (Scl_ObjTime2(p, i)->fall - Scl_ObjTime(p, i)->fall)); }
static inline int       Scl_ObjLegal( SC_Time * p, int i, float D )           { return Scl_ObjTime(p, i)->rise <= Scl_ObjTime2(p, i)->rise + Scl_ObjGetSlackR(p, i, D) && Scl_ObjTime(p, i)->fall <= Scl_ObjTime2(p, i)->fall + Scl_ObjGetSlackF(p, i, D); }

static inline double    Scl_ObjLoadFf( SC_Time * p, int i, int fRise )        { return fRise ? Scl_ObjLoad(p, i)->rise : Scl_ObjLoad(p, i)->fall; }
static inline double    Scl_ObjTimePs( SC_Time * p, int i, int fRise )        { return fRise ? Scl_ObjTime(p, i)->rise : Scl_ObjTime(p, i)->fall; }
static inline double    Scl_ObjSlewPs( SC_Time * p, int i, int fRise )        { return fRise ? Scl_ObjSlew(p, i)->rise : Scl_ObjSlew(p, i)->fall; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Constructor/destructor of STA manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline SC_Time * Scl_ManAlloc( SC_Lib * pLib, Vec_Int_t * vCis, Vec_Int_t * vCos, int nObjs )
{
    SC_Time * p;
    p = ABC_CALLOC( SC_Time, 1 );
    p->pLib      = pLib;
    p->vCis      = vCis;
    p->vCos      = vCos;
    p->nObjs     = nObjs;
    p->pLoads    = ABC_CALLOC( SC_Pair, p->nObjs );
    p->pLoads2   = ABC_CALLOC( SC_Pair, p->nObjs );
    p->pDepts    = ABC_CALLOC( SC_Pair, p->nObjs );
    p->pTimes    = ABC_CALLOC( SC_Pair, p->nObjs );
    p->pSlews    = ABC_CALLOC( SC_Pair, p->nObjs );
    p->pTimes2   = ABC_CALLOC( SC_Pair, p->nObjs );
    p->pSlews2   = ABC_CALLOC( SC_Pair, p->nObjs );
    p->pSlack    = ABC_FALLOC( float, p->nObjs );
    return p;
}
static inline void Scl_ManFree( SC_Time * p )
{
    Vec_IntFreeP( &p->vGates );
    ABC_FREE( p->pLoads );
    ABC_FREE( p->pLoads2 );
    ABC_FREE( p->pDepts );
    ABC_FREE( p->pTimes );
    ABC_FREE( p->pSlews );
    ABC_FREE( p->pTimes2 );
    ABC_FREE( p->pSlews2 );
    ABC_FREE( p->pSlack );
    ABC_FREE( p );
}
static inline void Scl_ManCleanTime( SC_Time * p )
{
    memset( p->pDepts, 0, sizeof(SC_Pair) * p->nObjs );
    memset( p->pTimes, 0, sizeof(SC_Pair) * p->nObjs );
    memset( p->pSlews, 0, sizeof(SC_Pair) * p->nObjs );
}


/**Function*************************************************************

  Synopsis    [Stores/retrieves timing information for the logic cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Scl_ConeStore( SC_Time * p, Vec_Int_t * vCone )
{
    int i, iObj;
    Vec_IntForEachEntry( vCone, iObj, i )
    {
        *Scl_ObjTime2(p, iObj) = *Scl_ObjTime(p, iObj);
        *Scl_ObjSlew2(p, iObj) = *Scl_ObjSlew(p, iObj);
    }
}
static inline void Scl_ConeRestore( SC_Time * p, Vec_Int_t * vCone )
{
    int i, iObj;
    Vec_IntForEachEntry( vCone, iObj, i )
    {
        *Scl_ObjTime(p, iObj) = *Scl_ObjTime2(p, iObj);
        *Scl_ObjSlew(p, iObj) = *Scl_ObjSlew2(p, iObj);
    }
}
static inline void Scl_ConeClear( SC_Time * p, Vec_Int_t * vCone )
{
    SC_Pair Zero = { 0.0, 0.0 };
    int i, iObj;
    Vec_IntForEachEntry( vCone, iObj, i )
    {
        *Scl_ObjTime(p, iObj) = Zero;
        *Scl_ObjSlew(p, iObj) = Zero;
    }
}

/**Function*************************************************************

  Synopsis    [Timing computation for pin/gate/cone/network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Scl_PinTimeArrival( SC_Time * p, SC_Timing * pTime, int iObj, int iFanin )
{
    SC_Pair * pArrIn   = Scl_ObjTime( p, iFanin );
    SC_Pair * pSlewIn  = Scl_ObjSlew( p, iFanin );
    SC_Pair * pLoad    = Scl_ObjLoad( p, iObj );
    SC_Pair * pArrOut  = Scl_ObjTime( p, iObj );   // modified
    SC_Pair * pSlewOut = Scl_ObjSlew( p, iObj );   // modified
    Scl_LibPinArrival( pTime, pArrIn, pSlewIn, pLoad, pArrOut, pSlewOut );
}
static inline void Scl_PinTimeDeparture( SC_Time * p, SC_Timing * pTime, int iObj, int iFanin )
{
    SC_Pair * pDepIn   = Scl_ObjDept( p, iFanin );   // modified
    SC_Pair * pSlewIn  = Scl_ObjSlew( p, iFanin );
    SC_Pair * pLoad    = Scl_ObjLoad( p, iObj );
    SC_Pair * pDepOut  = Scl_ObjDept( p, iObj );
    Scl_LibPinDeparture( pTime, pDepIn, pSlewIn, pLoad, pDepOut );
}



ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
