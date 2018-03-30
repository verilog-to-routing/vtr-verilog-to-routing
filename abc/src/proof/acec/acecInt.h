/**CFile****************************************************************

  FileName    [acecInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__proof__acec__acec__int_h
#define ABC__proof__acec__acec__int_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/gia/gia.h"
#include "acec.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_HEADER_START
 

typedef struct Acec_Box_t_ Acec_Box_t;
struct Acec_Box_t_
{
    Gia_Man_t *    pGia;      // AIG manager
    Vec_Wec_t *    vAdds;     // adders by rank
    Vec_Wec_t *    vLeafLits; // leaf literals by rank
    Vec_Wec_t *    vRootLits; // root literals by rank
    Vec_Wec_t *    vShared;   // shared leaves
    Vec_Wec_t *    vUnique;   // unique leaves
};

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline int Acec_SignBit( Vec_Int_t * vAdds, int iBox, int b )  { return (Vec_IntEntry(vAdds, 6*iBox+5) >> b)      & 1; }
static inline int Acec_SignBit2( Vec_Int_t * vAdds, int iBox, int b ) { return (Vec_IntEntry(vAdds, 6*iBox+5) >> (16+b)) & 1; }

static inline void Acec_SignSetBit( Vec_Int_t * vAdds, int iBox, int b, int v )  { if ( v ) *Vec_IntEntryP(vAdds, 6*iBox+5) |= (1 << b);      }
static inline void Acec_SignSetBit2( Vec_Int_t * vAdds, int iBox, int b, int v ) { if ( v ) *Vec_IntEntryP(vAdds, 6*iBox+5) |= (1 << (16+b)); }

////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== acecCo.c ========================================================*/
extern Vec_Int_t *   Gia_PolynCoreOrder( Gia_Man_t * pGia, Vec_Int_t * vAdds, Vec_Int_t * vAddCos, Vec_Int_t ** pvIns, Vec_Int_t ** pvOuts );
extern Vec_Wec_t *   Gia_PolynCoreOrderArray( Gia_Man_t * pGia, Vec_Int_t * vAdds, Vec_Int_t * vRootBoxes );
/*=== acecMult.c ========================================================*/
extern Vec_Int_t *   Acec_MultDetectInputs( Gia_Man_t * p, Vec_Wec_t * vLeafLits, Vec_Wec_t * vRootLits );
extern Vec_Bit_t *   Acec_BoothFindPPG( Gia_Man_t * p );
extern Vec_Bit_t *   Acec_MultMarkPPs( Gia_Man_t * p );
/*=== acecNorm.c ========================================================*/
extern void          Acec_InsertFadd( Gia_Man_t * pNew, int In[3], int Out[2] );
extern Gia_Man_t *   Acec_InsertBox( Acec_Box_t * pBox, int fAll );
/*=== acecTree.c ========================================================*/
extern void          Acec_PrintAdders( Vec_Wec_t * vBoxes, Vec_Int_t * vAdds );
extern void          Acec_TreePrintBox( Acec_Box_t * pBox, Vec_Int_t * vAdds );
extern Acec_Box_t *  Acec_DeriveBox( Gia_Man_t * p, Vec_Bit_t * vIgnore, int fFilterIn, int fFilterOut, int fVerbose );
extern void          Acec_BoxFreeP( Acec_Box_t ** ppBox );
/*=== acecUtil.c ========================================================*/
extern void          Gia_PolynAnalyzeXors( Gia_Man_t * pGia, int fVerbose );
extern Vec_Int_t *   Gia_PolynCollectLastXor( Gia_Man_t * pGia, int fVerbose );
/*=== acecUtil.c ========================================================*/
extern Acec_Box_t *  Acec_ProduceBox( Gia_Man_t * p, int fVerbose );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

