/**CFile****************************************************************

  FileName    [sclLib.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Standard-cell library representation.]

  Synopsis    [Simplified library representation for STA.]

  Author      [Alan Mishchenko, Niklas Een]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 24, 2012.]

  Revision    [$Id: sclLib.h,v 1.0 2012/08/24 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__map__scl__sclLib_h
#define ABC__map__scl__sclLib_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "misc/vec/vec.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define ABC_SCL_CUR_VERSION 8

typedef enum  
{
    sc_dir_NULL,
    sc_dir_Input,
    sc_dir_Output,
    sc_dir_InOut,
    sc_dir_Internal,
} SC_Dir;

typedef enum      // -- timing sense, positive-, negative- or non-unate
{
    sc_ts_NULL,
    sc_ts_Pos,
    sc_ts_Neg,
    sc_ts_Non,
} SC_TSense;

typedef struct SC_Pair_         SC_Pair;
struct SC_Pair_ 
{
    float      rise;
    float      fall;
};
typedef struct SC_PairI_        SC_PairI;
struct SC_PairI_ 
{
    int        rise;
    int        fall;
};

typedef struct SC_SizePars_    SC_SizePars;
struct SC_SizePars_
{
    int        nIters;
    int        nIterNoChange;
    int        Window;           // used for upsizing
    int        Ratio;            // used for upsizing
    int        Notches;
    int        DelayUser;
    int        DelayGap;
    int        TimeOut;
    int        BuffTreeEst;      // ratio for buffer tree estimation
    int        BypassFreq;       // frequency to try bypassing
    int        fUseDept;
    int        fDumpStats;
    int        fUseWireLoads;
    int        fVerbose;
    int        fVeryVerbose;
};

typedef struct SC_BusPars_     SC_BusPars;
struct SC_BusPars_
{
    int        GainRatio;       // target gain
    int        Slew;            // target slew
    int        nDegree;         // max branching factor
    int        fSizeOnly;       // perform only sizing
    int        fAddBufs;        // add buffers
    int        fBufPis;         // use CI buffering
    int        fUseWireLoads;   // wire loads
    int        fVerbose;        // verbose
    int        fVeryVerbose;    // verbose
};

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct SC_WireLoad_    SC_WireLoad;
typedef struct SC_WireLoadSel_ SC_WireLoadSel;
typedef struct SC_TableTempl_  SC_TableTempl;
typedef struct SC_Surface_     SC_Surface;
typedef struct SC_Timing_      SC_Timing;
typedef struct SC_Timings_     SC_Timings;
typedef struct SC_Pin_         SC_Pin;
typedef struct SC_Cell_        SC_Cell;
typedef struct SC_Lib_         SC_Lib;

struct SC_WireLoad_ 
{
    char *         pName;
    float          cap;            // }- multiply estimation in 'fanout_len[].snd' with this value
    float          slope;          // used to extrapolate wireload for large fanout count
    Vec_Int_t      vFanout;        // Vec<Pair<uint,float> > -- pairs '(#fanouts, est-wire-len)'
    Vec_Flt_t      vLen;
};

struct SC_WireLoadSel_
{
    char *         pName;
    Vec_Flt_t      vAreaFrom;      // Vec<Trip<float,float,Str> > -- triplets '(from-area, upto-area, wire-load-model)'; range is [from, upto[
    Vec_Flt_t      vAreaTo;
    Vec_Ptr_t      vWireLoadModel;
};

struct SC_TableTempl_ 
{
    char *         pName;
    Vec_Ptr_t      vVars;          // Vec<Str>         -- name of variable (numbered from 0, not 1 as in the Liberty file) 
    Vec_Ptr_t      vIndex;         // Vec<Vec<float> > -- this is the point of measurement in table for the given variable 
};

struct SC_Surface_ 
{
    char *         pName;
    Vec_Flt_t      vIndex0;        // Vec<float>       -- correspondes to "index_1" in the liberty file (for timing: slew)
    Vec_Flt_t      vIndex1;        // Vec<float>       -- correspondes to "index_2" in the liberty file (for timing: load)
    Vec_Ptr_t      vData;          // Vec<Vec<float> > -- 'data[i0][i1]' gives value at '(index0[i0], index1[i1])' 
    Vec_Int_t      vIndex0I;       // Vec<float>       -- correspondes to "index_1" in the liberty file (for timing: slew)
    Vec_Int_t      vIndex1I;       // Vec<float>       -- correspondes to "index_2" in the liberty file (for timing: load)
    Vec_Ptr_t      vDataI;         // Vec<Vec<float> > -- 'data[i0][i1]' gives value at '(index0[i0], index1[i1])' 
    float          approx[3][6];
};

struct SC_Timing_ 
{
    char *         related_pin;    // -- related pin
    SC_TSense      tsense;         // -- timing sense (positive_unate, negative_unate, non_unate)
    char *         when_text;      // -- logic condition on inputs triggering this delay model for the output (currently not used)
    SC_Surface     pCellRise;      // -- Used to compute pin-to-pin delay
    SC_Surface     pCellFall;
    SC_Surface     pRiseTrans;     // -- Used to compute output slew
    SC_Surface     pFallTrans;
};

struct SC_Timings_ 
{
    char *         pName;          // -- the 'related_pin' field
    Vec_Ptr_t      vTimings;       // structures of type SC_Timing
};

struct SC_Pin_ 
{
    char *         pName;
    SC_Dir         dir;
    float          cap;            // -- this value is used if 'rise_cap' and 'fall_cap' is missing (copied by 'postProcess()'). (not used)
    float          rise_cap;       // }- used for input pins ('cap' too).
    float          fall_cap;       // }
    int            rise_capI;      // }- used for input pins ('cap' too).
    int            fall_capI;      // }
    float          max_out_cap;    // } (not used)
    float          max_out_slew;   // }- used only for output pins (max values must not be exceeded or else mapping is illegal) (not used)
    char *         func_text;      // }
    Vec_Wrd_t      vFunc;          // }
    Vec_Ptr_t      vRTimings;      // -- for output pins
//    SC_Timing      Timing;         // -- for output pins  
};

struct SC_Cell_ 
{
    char *         pName;
    int            Id;
    int            fSkip;          // skip this cell during genlib computation
    int            seq;            // -- set to TRUE by parser if a sequential element
    int            unsupp;         // -- set to TRUE by parser if cell contains information we cannot handle
    float          area;
    float          leakage;
    int            areaI;
    int            leakageI;
    int            drive_strength; // -- some library files provide this field (currently unused, but may be a good hint for sizing) (not used)
    Vec_Ptr_t      vPins;          // NamedSet<SC_Pin> 
    int            n_inputs;       // -- 'pins[0 .. n_inputs-1]' are input pins
    int            n_outputs;      // -- 'pins[n_inputs .. n_inputs+n_outputs-1]' are output pins
    SC_Cell *      pNext;          // same-functionality cells linked into a ring by area
    SC_Cell *      pPrev;          // same-functionality cells linked into a ring by area
    SC_Cell *      pRepr;          // representative of the class
    SC_Cell *      pAve;           // average size cell of this class
    int            Order;          // order of the gate in the list
    int            nGates;         // the number of gates in the list      
};

struct SC_Lib_ 
{
    char *         pName;
    char *         pFileName;
    char *         default_wire_load;
    char *         default_wire_load_sel;
    float          default_max_out_slew;   // -- 'default_max_transition'; this is copied to each output pin where 'max_transition' is not defined  (not used)
    int            unit_time;      // -- Valid 9..12. Unit is '10^(-val)' seconds (e.g. 9=1ns, 10=100ps, 11=10ps, 12=1ps)
    float          unit_cap_fst;   // -- First part is a multiplier, second either 12 or 15 for 'pf' or 'ff'.
    int            unit_cap_snd;
    Vec_Ptr_t      vWireLoads;     // NamedSet<SC_WireLoad>
    Vec_Ptr_t      vWireLoadSels;  // NamedSet<SC_WireLoadSel>
    Vec_Ptr_t      vTempls;        // NamedSet<SC_TableTempl>  
    Vec_Ptr_t      vCells;         // NamedSet<SC_Cell>
    Vec_Ptr_t      vCellClasses;   // NamedSet<SC_Cell>
    int *          pBins;          // hashing gateName -> gateId
    int            nBins;
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

static inline void        SC_PairClean( SC_Pair * d )               { d->rise = d->fall = 0;                 }
static inline float       SC_PairMax( SC_Pair * d )                 { return Abc_MaxFloat(d->rise, d->fall); }
static inline float       SC_PairMin( SC_Pair * d )                 { return Abc_MinFloat(d->rise, d->fall); }
static inline float       SC_PairAve( SC_Pair * d )                 { return 0.5 * d->rise + 0.5 * d->fall;  }
static inline void        SC_PairDup( SC_Pair * d, SC_Pair * s )    { *d = *s;                               }
static inline void        SC_PairMove( SC_Pair * d, SC_Pair * s )   { *d = *s; s->rise = s->fall = 0;        }
static inline void        SC_PairAdd( SC_Pair * d, SC_Pair * s )    { d->rise += s->rise; d->fall += s->fall;}
static inline int         SC_PairEqual( SC_Pair * d, SC_Pair * s )  { return d->rise == s->rise && d->fall == s->fall;                }
static inline int         SC_PairEqualE( SC_Pair * d, SC_Pair * s, float E )  { return d->rise - s->rise < E && s->rise - d->rise < E &&  d->fall - s->fall < E && s->fall - d->fall < E;    }

static inline int         SC_LibCellNum( SC_Lib * p )               { return Vec_PtrSize(&p->vCells);                                  }
static inline SC_Cell *   SC_LibCell( SC_Lib * p, int i )           { return (SC_Cell *)Vec_PtrEntry(&p->vCells, i);                   }
static inline SC_Pin  *   SC_CellPin( SC_Cell * p, int i )          { return (SC_Pin *)Vec_PtrEntry(&p->vPins, i);                     }
static inline Vec_Wrd_t * SC_CellFunc( SC_Cell * p )                { return &SC_CellPin(p, p->n_inputs)->vFunc;                       }
static inline float       SC_CellPinCap( SC_Cell * p, int i )       { return 0.5 * SC_CellPin(p, i)->rise_cap + 0.5 * SC_CellPin(p, i)->fall_cap; }
static inline float       SC_CellPinCapAve( SC_Cell * p )           { int i; float c = 0; for (i = 0; i < p->n_inputs; i++) c += SC_CellPinCap(p, i); return c / Abc_MaxInt(1, p->n_inputs); }
static inline char *      SC_CellPinOutFunc( SC_Cell * p, int i )   { return SC_CellPin(p, p->n_inputs + i)->func_text;               }
static inline char *      SC_CellPinName( SC_Cell * p, int i )      { return SC_CellPin(p, i)->pName;                                 }

#define SC_LibForEachCell( p, pCell, i )         Vec_PtrForEachEntry( SC_Cell *, &p->vCells, pCell, i )
#define SC_LibForEachCellClass( p, pCell, i )    Vec_PtrForEachEntry( SC_Cell *, &p->vCellClasses, pCell, i )
#define SC_LibForEachWireLoad( p, pWL, i )       Vec_PtrForEachEntry( SC_WireLoad *, &p->vWireLoads, pWL, i )
#define SC_LibForEachWireLoadSel( p, pWLS, i )   Vec_PtrForEachEntry( SC_WireLoadSel *, &p->vWireLoadSels, pWLS, i )
#define SC_LibForEachTempl( p, pTempl, i )       Vec_PtrForEachEntry( SC_TableTempl *, &p->vTempls, pTempl, i )
#define SC_CellForEachPin( p, pPin, i )          Vec_PtrForEachEntry( SC_Pin *, &p->vPins, pPin, i )
#define SC_CellForEachPinIn( p, pPin, i )        Vec_PtrForEachEntryStop( SC_Pin *, &p->vPins, pPin, i, p->n_inputs )
#define SC_CellForEachPinOut( p, pPin, i )       Vec_PtrForEachEntryStart( SC_Pin *, &p->vPins, pPin, i, p->n_inputs )
#define SC_RingForEachCell( pRing, pCell, i )    for ( i = 0, pCell = pRing; i == 0 || pCell != pRing; pCell = pCell->pNext, i++ )
#define SC_RingForEachCellRev( pRing, pCell, i ) for ( i = 0, pCell = pRing; i == 0 || pCell != pRing; pCell = pCell->pPrev, i++ )
#define SC_PinForEachRTiming( p, pRTime, i )     Vec_PtrForEachEntry( SC_Timings *, &p->vRTimings, pRTime, i )


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Constructors of the library data-structures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline SC_WireLoad * Abc_SclWireLoadAlloc()
{
    SC_WireLoad * p;
    p = ABC_CALLOC( SC_WireLoad, 1 );
    return p;
}
static inline SC_WireLoadSel * Abc_SclWireLoadSelAlloc()
{
    SC_WireLoadSel * p;
    p = ABC_CALLOC( SC_WireLoadSel, 1 );
    return p;
}
static inline SC_TableTempl * Abc_SclTableTemplAlloc()
{
    SC_TableTempl * p;
    p = ABC_CALLOC( SC_TableTempl, 1 );
    return p;
}
static inline SC_Surface * Abc_SclSurfaceAlloc()
{
    SC_Surface * p;
    p = ABC_CALLOC( SC_Surface, 1 );
    return p;
}
static inline SC_Timing * Abc_SclTimingAlloc()
{
    SC_Timing * p;
    p = ABC_CALLOC( SC_Timing, 1 );
    return p;
}
static inline SC_Timings * Abc_SclTimingsAlloc()
{
    SC_Timings * p;
    p = ABC_CALLOC( SC_Timings, 1 );
    return p;
}
static inline SC_Pin * Abc_SclPinAlloc()
{
    SC_Pin * p;
    p = ABC_CALLOC( SC_Pin, 1 );
    p->max_out_slew = -1;
    return p;
}
static inline SC_Cell * Abc_SclCellAlloc()
{
    SC_Cell * p;
    p = ABC_CALLOC( SC_Cell, 1 );
    return p;
}
static inline SC_Lib * Abc_SclLibAlloc()
{
    SC_Lib * p;
    p = ABC_CALLOC( SC_Lib, 1 );
    p->default_max_out_slew = -1;
    p->unit_time      = 9;
    p->unit_cap_fst   = 1;
    p->unit_cap_snd   = 12;
    return p;
}


/**Function*************************************************************

  Synopsis    [Destructors of the library data-structures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_SclWireLoadFree( SC_WireLoad * p )
{
    Vec_IntErase( &p->vFanout );
    Vec_FltErase( &p->vLen );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}
static inline void Abc_SclWireLoadSelFree( SC_WireLoadSel * p )
{
    Vec_FltErase( &p->vAreaFrom );
    Vec_FltErase( &p->vAreaTo );
    Vec_PtrFreeData( &p->vWireLoadModel );
    Vec_PtrErase( &p->vWireLoadModel );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}
static inline void Abc_SclTableTemplFree( SC_TableTempl * p )
{
    Vec_PtrFreeData( &p->vVars );
    Vec_PtrErase( &p->vVars );
    Vec_VecErase( (Vec_Vec_t *)&p->vIndex );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}
static inline void Abc_SclSurfaceFree( SC_Surface * p )
{
    Vec_FltErase( &p->vIndex0 );
    Vec_FltErase( &p->vIndex1 );
    Vec_IntErase( &p->vIndex0I );
    Vec_IntErase( &p->vIndex1I );
    Vec_VecErase( (Vec_Vec_t *)&p->vData );
    Vec_VecErase( (Vec_Vec_t *)&p->vDataI );
    ABC_FREE( p->pName );
//    ABC_FREE( p );
}
static inline void Abc_SclTimingFree( SC_Timing * p )
{
    Abc_SclSurfaceFree( &p->pCellRise );
    Abc_SclSurfaceFree( &p->pCellFall );
    Abc_SclSurfaceFree( &p->pRiseTrans );
    Abc_SclSurfaceFree( &p->pFallTrans );
    ABC_FREE( p->related_pin );
    ABC_FREE( p->when_text );
    ABC_FREE( p );
}
static inline void Abc_SclTimingsFree( SC_Timings * p )
{
    SC_Timing * pTemp;
    int i;
    Vec_PtrForEachEntry( SC_Timing *, &p->vTimings, pTemp, i )
        Abc_SclTimingFree( pTemp );
    Vec_PtrErase( &p->vTimings );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}
static inline void Abc_SclPinFree( SC_Pin * p )
{
    SC_Timings * pTemp;
    int i;
    SC_PinForEachRTiming( p, pTemp, i )
        Abc_SclTimingsFree( pTemp );
    Vec_PtrErase( &p->vRTimings );
    Vec_WrdErase( &p->vFunc );
    ABC_FREE( p->func_text );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}
static inline void Abc_SclCellFree( SC_Cell * p )
{
    SC_Pin * pTemp;
    int i;
    SC_CellForEachPin( p, pTemp, i )
        Abc_SclPinFree( pTemp );
    Vec_PtrErase( &p->vPins );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}
static inline void Abc_SclLibFree( SC_Lib * p )
{
    SC_WireLoad * pWL;
    SC_WireLoadSel * pWLS;
    SC_TableTempl * pTempl;
    SC_Cell * pCell;
    int i;
    SC_LibForEachWireLoad( p, pWL, i )
        Abc_SclWireLoadFree( pWL );
    Vec_PtrErase( &p->vWireLoads );
    SC_LibForEachWireLoadSel( p, pWLS, i )
        Abc_SclWireLoadSelFree( pWLS );
    Vec_PtrErase( &p->vWireLoadSels );
    SC_LibForEachTempl( p, pTempl, i )
        Abc_SclTableTemplFree( pTempl );
    Vec_PtrErase( &p->vTempls );
    SC_LibForEachCell( p, pCell, i )
        Abc_SclCellFree( pCell );
    Vec_PtrErase( &p->vCells );
    Vec_PtrErase( &p->vCellClasses );
    ABC_FREE( p->pName );
    ABC_FREE( p->pFileName );
    ABC_FREE( p->default_wire_load );
    ABC_FREE( p->default_wire_load_sel );
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Lookup table delay computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float Scl_LibLookup( SC_Surface * p, float slew, float load )
{
    float * pIndex0, * pIndex1, * pDataS, * pDataS1;
    float sfrac, lfrac, p0, p1;
    int s, l;

    // handle constant table
    if ( Vec_FltSize(&p->vIndex0) == 1 && Vec_FltSize(&p->vIndex1) == 1 )
    {
        Vec_Flt_t * vTemp = (Vec_Flt_t *)Vec_PtrEntry(&p->vData, 0);
        assert( Vec_PtrSize(&p->vData) == 1 );
        assert( Vec_FltSize(vTemp) == 1 );
        return Vec_FltEntry(vTemp, 0);
    }

    // Find closest sample points in surface:
    pIndex0 = Vec_FltArray(&p->vIndex0);
    for ( s = 1; s < Vec_FltSize(&p->vIndex0)-1; s++ )
        if ( pIndex0[s] > slew )
            break;
    s--;

    pIndex1 = Vec_FltArray(&p->vIndex1);
    for ( l = 1; l < Vec_FltSize(&p->vIndex1)-1; l++ )
        if ( pIndex1[l] > load )
            break;
    l--;

    // Interpolate (or extrapolate) function value from sample points:
    sfrac = (slew - pIndex0[s]) / (pIndex0[s+1] - pIndex0[s]);
    lfrac = (load - pIndex1[l]) / (pIndex1[l+1] - pIndex1[l]);

    pDataS  = Vec_FltArray( (Vec_Flt_t *)Vec_PtrEntry(&p->vData, s) );
    pDataS1 = Vec_FltArray( (Vec_Flt_t *)Vec_PtrEntry(&p->vData, s+1) );

    p0 = pDataS [l] + lfrac * (pDataS [l+1] - pDataS [l]);
    p1 = pDataS1[l] + lfrac * (pDataS1[l+1] - pDataS1[l]);

    return p0 + sfrac * (p1 - p0);      // <<== multiply result with K factor here 
}
static inline void Scl_LibPinArrival( SC_Timing * pTime, SC_Pair * pArrIn, SC_Pair * pSlewIn, SC_Pair * pLoad, SC_Pair * pArrOut, SC_Pair * pSlewOut )
{
    if (pTime->tsense == sc_ts_Pos || pTime->tsense == sc_ts_Non)
    {
        pArrOut->rise  = Abc_MaxFloat( pArrOut->rise,  pArrIn->rise + Scl_LibLookup(&pTime->pCellRise,  pSlewIn->rise, pLoad->rise) );
        pArrOut->fall  = Abc_MaxFloat( pArrOut->fall,  pArrIn->fall + Scl_LibLookup(&pTime->pCellFall,  pSlewIn->fall, pLoad->fall) );
        pSlewOut->rise = Abc_MaxFloat( pSlewOut->rise,                Scl_LibLookup(&pTime->pRiseTrans, pSlewIn->rise, pLoad->rise) );
        pSlewOut->fall = Abc_MaxFloat( pSlewOut->fall,                Scl_LibLookup(&pTime->pFallTrans, pSlewIn->fall, pLoad->fall) );
    }
    if (pTime->tsense == sc_ts_Neg || pTime->tsense == sc_ts_Non)
    {
        pArrOut->rise  = Abc_MaxFloat( pArrOut->rise,  pArrIn->fall + Scl_LibLookup(&pTime->pCellRise,  pSlewIn->fall, pLoad->rise) );
        pArrOut->fall  = Abc_MaxFloat( pArrOut->fall,  pArrIn->rise + Scl_LibLookup(&pTime->pCellFall,  pSlewIn->rise, pLoad->fall) );
        pSlewOut->rise = Abc_MaxFloat( pSlewOut->rise,                Scl_LibLookup(&pTime->pRiseTrans, pSlewIn->fall, pLoad->rise) );
        pSlewOut->fall = Abc_MaxFloat( pSlewOut->fall,                Scl_LibLookup(&pTime->pFallTrans, pSlewIn->rise, pLoad->fall) );
    }
}
static inline void Scl_LibPinDeparture( SC_Timing * pTime, SC_Pair * pDepIn, SC_Pair * pSlewIn, SC_Pair * pLoad, SC_Pair * pDepOut )
{
    if (pTime->tsense == sc_ts_Pos || pTime->tsense == sc_ts_Non)
    {
        pDepIn->rise  = Abc_MaxFloat( pDepIn->rise,  pDepOut->rise + Scl_LibLookup(&pTime->pCellRise,  pSlewIn->rise, pLoad->rise) );
        pDepIn->fall  = Abc_MaxFloat( pDepIn->fall,  pDepOut->fall + Scl_LibLookup(&pTime->pCellFall,  pSlewIn->fall, pLoad->fall) );
    }
    if (pTime->tsense == sc_ts_Neg || pTime->tsense == sc_ts_Non)
    {
        pDepIn->fall  = Abc_MaxFloat( pDepIn->fall,  pDepOut->rise + Scl_LibLookup(&pTime->pCellRise,  pSlewIn->fall, pLoad->rise) );
        pDepIn->rise  = Abc_MaxFloat( pDepIn->rise,  pDepOut->fall + Scl_LibLookup(&pTime->pCellFall,  pSlewIn->rise, pLoad->fall) );
    }
}

/**Function*************************************************************

  Synopsis    [Lookup table delay computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Scl_LibLookupI( SC_Surface * p, int slew, int load )
{
    int * pIndex0, * pIndex1, * pDataS, * pDataS1;
    int p0, p1, s, l;
    iword lFrac0, lFrac1, sFrac;

    // handle constant table
    if ( Vec_IntSize(&p->vIndex0I) == 1 && Vec_IntSize(&p->vIndex1I) == 1 )
    {
        Vec_Int_t * vTemp = (Vec_Int_t *)Vec_PtrEntry(&p->vDataI, 0);
        assert( Vec_PtrSize(&p->vDataI) == 1 );
        assert( Vec_IntSize(vTemp) == 1 );
        return Vec_IntEntry(vTemp, 0);
    }

    // Find closest sample points in surface:
    pIndex0 = Vec_IntArray(&p->vIndex0I);
    for ( s = 1; s < Vec_IntSize(&p->vIndex0I)-1; s++ )
        if ( pIndex0[s] > slew )
            break;
    s--;

    pIndex1 = Vec_IntArray(&p->vIndex1I);
    for ( l = 1; l < Vec_IntSize(&p->vIndex1I)-1; l++ )
        if ( pIndex1[l] > load )
            break;
    l--;

    pDataS  = Vec_IntArray( (Vec_Int_t *)Vec_PtrEntry(&p->vDataI, s) );
    pDataS1 = Vec_IntArray( (Vec_Int_t *)Vec_PtrEntry(&p->vDataI, s+1) );

    // Interpolate (or extrapolate) function value from sample points:
//    lfrac = (load - pIndex1[l]) / (pIndex1[l+1] - pIndex1[l]);
//    sfrac = (slew - pIndex0[s]) / (pIndex0[s+1] - pIndex0[s]);

    lFrac0 = (iword)(pDataS [l+1] - pDataS [l]) * (iword)(load - pIndex1[l]) / (iword)(pIndex1[l+1] - pIndex1[l]);
    lFrac1 = (iword)(pDataS1[l+1] - pDataS1[l]) * (iword)(load - pIndex1[l]) / (iword)(pIndex1[l+1] - pIndex1[l]);

//    p0 = pDataS [l] + lfrac * (pDataS [l+1] - pDataS [l]);
//    p1 = pDataS1[l] + lfrac * (pDataS1[l+1] - pDataS1[l]);

    p0 = pDataS [l] + (int)lFrac0;
    p1 = pDataS1[l] + (int)lFrac1;

    sFrac = (iword)(p1 - p0) * (iword)(slew - pIndex0[s]) / (iword)(pIndex0[s+1] - pIndex0[s]);

//    return p0 + sfrac * (p1 - p0); 
    return p0 + (int)sFrac;   
}
static inline void Scl_LibPinArrivalI( SC_Timing * pTime, SC_PairI * pArrIn, SC_PairI * pSlewIn, SC_PairI * pLoad, SC_PairI * pArrOut, SC_PairI * pSlewOut, int * pArray )
{
    if (pTime->tsense == sc_ts_Pos || pTime->tsense == sc_ts_Non)
    {
        pArrOut->rise  = Abc_MaxInt( pArrOut->rise,  pArrIn->rise + (pArray[0] = Scl_LibLookupI(&pTime->pCellRise,  pSlewIn->rise, pLoad->rise)) );
        pArrOut->fall  = Abc_MaxInt( pArrOut->fall,  pArrIn->fall + (pArray[1] = Scl_LibLookupI(&pTime->pCellFall,  pSlewIn->fall, pLoad->fall)) );
        pSlewOut->rise = Abc_MaxInt( pSlewOut->rise,                             Scl_LibLookupI(&pTime->pRiseTrans, pSlewIn->rise, pLoad->rise) );
        pSlewOut->fall = Abc_MaxInt( pSlewOut->fall,                             Scl_LibLookupI(&pTime->pFallTrans, pSlewIn->fall, pLoad->fall) );
    }
    if (pTime->tsense == sc_ts_Neg || pTime->tsense == sc_ts_Non)
    {
        pArrOut->rise  = Abc_MaxInt( pArrOut->rise,  pArrIn->fall + (pArray[2] = Scl_LibLookupI(&pTime->pCellRise,  pSlewIn->fall, pLoad->rise)) );
        pArrOut->fall  = Abc_MaxInt( pArrOut->fall,  pArrIn->rise + (pArray[3] = Scl_LibLookupI(&pTime->pCellFall,  pSlewIn->rise, pLoad->fall)) );
        pSlewOut->rise = Abc_MaxInt( pSlewOut->rise,                             Scl_LibLookupI(&pTime->pRiseTrans, pSlewIn->fall, pLoad->rise) );
        pSlewOut->fall = Abc_MaxInt( pSlewOut->fall,                             Scl_LibLookupI(&pTime->pFallTrans, pSlewIn->rise, pLoad->fall) );
    }
}
static inline void Scl_LibPinRequiredI( SC_Timing * pTime, SC_PairI * pReqIn, SC_PairI * pReqOut, int * pArray )
{
    if (pTime->tsense == sc_ts_Pos || pTime->tsense == sc_ts_Non)
    {
        pReqIn->rise  = Abc_MinInt( pReqIn->rise,  pReqOut->rise - pArray[0] );
        pReqIn->fall  = Abc_MinInt( pReqIn->fall,  pReqOut->fall - pArray[1] );
    }
    if (pTime->tsense == sc_ts_Neg || pTime->tsense == sc_ts_Non)
    {
        pReqIn->fall  = Abc_MinInt( pReqIn->fall,  pReqOut->rise - pArray[2] );
        pReqIn->rise  = Abc_MinInt( pReqIn->rise,  pReqOut->fall - pArray[3] );
    }
}

/**Function*************************************************************

  Synopsis    [Compute one timing edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline SC_Timing * Scl_CellPinTime( SC_Cell * pCell, int iPin )
{
    SC_Pin * pPin;
    SC_Timings * pRTime;
    assert( iPin >= 0 && iPin < pCell->n_inputs );
    pPin = SC_CellPin( pCell, pCell->n_inputs );
    assert( Vec_PtrSize(&pPin->vRTimings) == pCell->n_inputs );
    pRTime = (SC_Timings *)Vec_PtrEntry( &pPin->vRTimings, iPin );
    if ( Vec_PtrSize(&pRTime->vTimings) == 0 )
        return NULL;
    assert( Vec_PtrSize(&pRTime->vTimings) == 1 );
    return (SC_Timing *)Vec_PtrEntry( &pRTime->vTimings, 0 );
}
static inline float Scl_LibPinArrivalEstimate( SC_Cell * pCell, int iPin, float Slew, float Load )
{
    SC_Pair LoadIn = { Load, Load };
    SC_Pair ArrIn  = { 0.0, 0.0 };
    SC_Pair ArrOut = { 0.0, 0.0 };
    SC_Pair SlewIn = { 0.0, 0.0 };
    SC_Pair SlewOut = { 0.0, 0.0 };
//    Vec_Flt_t * vIndex0 = pTime->pCellRise->vIndex0; // slew
//    SlewIn.fall = SlewIn.rise = Vec_FltEntry( vIndex0, Vec_FltSize(vIndex0)/2 );
    SlewIn.fall = SlewIn.rise = Slew; 
    Scl_LibPinArrival( Scl_CellPinTime(pCell, iPin), &ArrIn, &SlewIn, &LoadIn, &ArrOut, &SlewOut );
    return  0.5 * ArrOut.fall +  0.5 * ArrOut.rise;
}
static inline void Scl_LibHandleInputDriver( SC_Cell * pCell, SC_Pair * pLoadIn, SC_Pair * pArrOut, SC_Pair * pSlewOut )
{
    SC_Pair LoadIn   = { 0.0, 0.0 }; // zero input load
    SC_Pair ArrIn    = { 0.0, 0.0 }; // zero input time
    SC_Pair SlewIn   = { 0.0, 0.0 }; // zero input slew
    SC_Pair ArrOut0  = { 0.0, 0.0 }; // output time under zero load
    SC_Pair ArrOut1  = { 0.0, 0.0 }; // output time under given load
    SC_Pair SlewOut  = { 0.0, 0.0 }; // output slew under zero load 
    pSlewOut->fall = pSlewOut->rise = 0;
    assert( pCell->n_inputs == 1 );
    Scl_LibPinArrival( Scl_CellPinTime(pCell, 0), &ArrIn, &SlewIn, &LoadIn, &ArrOut0, &SlewOut );
    Scl_LibPinArrival( Scl_CellPinTime(pCell, 0), &ArrIn, &SlewIn, pLoadIn, &ArrOut1, pSlewOut );
    pArrOut->fall = ArrOut1.fall - ArrOut0.fall;
    pArrOut->rise = ArrOut1.rise - ArrOut0.rise;
}

/**Function*************************************************************

  Synopsis    [Compute one timing edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Scl_LibPinArrivalEstimateI( SC_Cell * pCell, int iPin, int Slew, int Load )
{
    int Arrray[4];
    SC_PairI LoadIn = { Load, Load };
    SC_PairI ArrIn  = { 0, 0 };
    SC_PairI ArrOut = { 0, 0 };
    SC_PairI SlewIn = { 0, 0 };
    SC_PairI SlewOut = { 0, 0 };
//    Vec_Flt_t * vIndex0 = pTime->pCellRise->vIndex0; // slew
//    SlewIn.fall = SlewIn.rise = Vec_FltEntry( vIndex0, Vec_FltSize(vIndex0)/2 );
    SlewIn.fall = SlewIn.rise = Slew; 
    Scl_LibPinArrivalI( Scl_CellPinTime(pCell, iPin), &ArrIn, &SlewIn, &LoadIn, &ArrOut, &SlewOut, Arrray );
    return (ArrOut.fall + ArrOut.rise) >> 1;
}
static inline void Scl_LibHandleInputDriver2( SC_Cell * pCell, SC_PairI * pLoadIn, SC_PairI * pArrOut, SC_PairI * pSlewOut )
{
    int Arrray[4];
    SC_PairI LoadIn   = { 0, 0 }; // zero input load
    SC_PairI ArrIn    = { 0, 0 }; // zero input time
    SC_PairI SlewIn   = { 0, 0 }; // zero input slew
    SC_PairI ArrOut0  = { 0, 0 }; // output time under zero load
    SC_PairI ArrOut1  = { 0, 0 }; // output time under given load
    SC_PairI SlewOut  = { 0, 0 }; // output slew under zero load 
    pSlewOut->fall = pSlewOut->rise = 0;
    assert( pCell->n_inputs == 1 );
    Scl_LibPinArrivalI( Scl_CellPinTime(pCell, 0), &ArrIn, &SlewIn, &LoadIn, &ArrOut0, &SlewOut, Arrray );
    Scl_LibPinArrivalI( Scl_CellPinTime(pCell, 0), &ArrIn, &SlewIn, pLoadIn, &ArrOut1, pSlewOut, Arrray );
    pArrOut->fall = ArrOut1.fall - ArrOut0.fall;
    pArrOut->rise = ArrOut1.rise - ArrOut0.rise;
}

/*=== sclLiberty.c ===============================================================*/
extern SC_Lib *      Abc_SclReadLiberty( char * pFileName, int fVerbose, int fVeryVerbose );
/*=== sclLibScl.c ===============================================================*/
extern SC_Lib *      Abc_SclReadFromGenlib( void * pLib );
extern SC_Lib *      Abc_SclReadFromStr( Vec_Str_t * vOut );
extern SC_Lib *      Abc_SclReadFromFile( char * pFileName );
extern void          Abc_SclWriteScl( char * pFileName, SC_Lib * p );
extern void          Abc_SclWriteLiberty( char * pFileName, SC_Lib * p );
/*=== sclLibUtil.c ===============================================================*/
extern void          Abc_SclHashCells( SC_Lib * p );
extern int           Abc_SclCellFind( SC_Lib * p, char * pName );
extern int           Abc_SclClassCellNum( SC_Cell * pClass );
extern void          Abc_SclShortNames( SC_Lib * p );
extern int           Abc_SclLibClassNum( SC_Lib * pLib );
extern void          Abc_SclLinkCells( SC_Lib * p );
extern void          Abc_SclPrintCells( SC_Lib * p, float Slew, float Gain, int fInvOnly, int fShort );
extern void          Abc_SclConvertLeakageIntoArea( SC_Lib * p, float A, float B );
extern void          Abc_SclLibNormalize( SC_Lib * p );
extern SC_Cell *     Abc_SclFindInvertor( SC_Lib * p, int fFindBuff );
extern SC_Cell *     Abc_SclFindSmallestGate( SC_Cell * p, float CinMin );
extern SC_WireLoad * Abc_SclFindWireLoadModel( SC_Lib * p, float Area );
extern SC_WireLoad * Abc_SclFetchWireLoadModel( SC_Lib * p, char * pName );
extern int           Abc_SclHasDelayInfo( void * pScl );
extern float         Abc_SclComputeAverageSlew( SC_Lib * p );
extern void          Abc_SclDumpGenlib( char * pFileName, SC_Lib * p, float Slew, float Gain, int nGatesMin );
extern void          Abc_SclInstallGenlib( void * pScl, float Slew, float Gain, int nGatesMin );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
