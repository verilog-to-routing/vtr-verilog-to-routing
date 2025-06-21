/**CFile****************************************************************

  FileName    [giaProp.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Constraint propagation on the AIG.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaProp.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define GIA_SAT_SHIFT 12
#define GIA_ROOT_MASK 
#define GIA_PATH00_MASK
#define GIA_PATH10_MASK
#define GIA_PATH20_MASK
#define GIA_PATH30_MASK
#define GIA_PATH00_MASK
#define GIA_PATH10_MASK
#define GIA_PATH20_MASK
#define GIA_PATH30_MASK

static inline int Gia_SatObjIsRoot( Gia_Obj_t * p )      { return 0; }
static inline int Gia_SatObjXorRoot( Gia_Obj_t * p )     { return 0; }


static inline int Gia_SatObjIsAssigned( Gia_Obj_t * p )  { return 0; }
static inline int Gia_SatObjIsHeld( Gia_Obj_t * p )      { return 0; }
static inline int Gia_SatObjValue( Gia_Obj_t * p )       { return 0; }


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks if the give cut is satisfied.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_SatPathCheckCutSat_rec( Gia_Obj_t * p, int fCompl )
{
    if ( Gia_SatObjIsRoot(p) )
        return Gia_ObjIsAssigned(p) && Gia_SatObjValue(p) == fCompl;
    if ( Gia_SatObjPath0(p) && !Gia_SatPathCheckCutSat_rec( Gia_ObjFanin0(p), fCompl ^ Gia_ObjFaninC0(p) ) )
        return 0;
    if ( Gia_SatObjPath1(p) && !Gia_SatPathCheckCutSat_rec( Gia_ObjFanin1(p), fCompl ^ Gia_ObjFaninC1(p) ) )
        return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks if the give cut is satisfied.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_SatPathCheckCutSat( Gia_Obj_t * p )
{
    int RetValue;
    assert( Gia_SatObjIsRoot(p) );
    Gia_SatObjXorRoot(p);
    RetValue = Gia_SatPathCheckCutSat_rec( p );
    Gia_SatObjXorRoot(p);
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Unbinds literals on the path.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_SatPathUnbind_rec( Gia_Obj_t * p )
{
}

/**Function*************************************************************

  Synopsis    [Creates a feasible path from the node to a terminal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_SatPathStart_rec( Gia_Obj_t * p, int fDiffs, int fCompl )
{
    if ( Gia_SatObjIsRoot(p) )
        return fDiffs && (!Gia_ObjIsAssigned(p) || Gia_SatObjValue(p) != fCompl);
    if ( fCompl == 0 )
    {
        if ( Gia_SatPathStart_rec( Gia_ObjFanin0(p), fDiffs + !Gia_SatObjPath0(p), fCompl ^ Gia_ObjFaninC0(p) ) &&
             Gia_SatPathStart_rec( Gia_ObjFanin1(p), fDiffs + !Gia_SatObjPath1(p), fCompl ^ Gia_ObjFaninC1(p) ) )
            return Gia_ObjSetDraftPath0(p) + Gia_ObjSetDraftPath1(p);
    }
    else
    {
        if ( Gia_SatPathStart_rec( Gia_ObjFanin0(p), fDiffs + !Gia_SatObjPath0(p), fCompl ^ Gia_ObjFaninC0(p) ) )
        {
            Gia_ObjUnsetDraftPath1(p);
            return Gia_ObjSetDraftPath0(p);
        }
        if ( Gia_SatPathStart_rec( Gia_ObjFanin1(p), fDiffs + !Gia_SatObjPath1(p), fCompl ^ Gia_ObjFaninC1(p) ) )
        {
            Gia_ObjUnsetDraftPath0(p);
            return Gia_ObjSetDraftPath1(p);
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Creates a feasible path from the node to a terminal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_SatPathStart( Gia_Obj_t * p )
{
    int RetValue;
    assert( Gia_SatObjIsRoot(p) );
    Gia_SatObjXorRoot(p);
    RetValue = Gia_SatPathStart_rec( p, 0, 0 );
    Gia_SatObjXorRoot(p);
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

