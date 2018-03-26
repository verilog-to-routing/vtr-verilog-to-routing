/**CFile****************************************************************

  FileName    [sbd.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sbd.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__opt_sbd__h
#define ABC__opt_sbd__h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Sbd_Par_t_ Sbd_Par_t;
struct Sbd_Par_t_
{
    int             nLutSize;     // target LUT size
    int             nLutNum;      // target LUT count
    int             nCutSize;     // target cut size
    int             nCutNum;      // target cut count
    int             nTfoLevels;   // the number of TFO levels (windowing)
    int             nTfoFanMax;   // the max number of fanouts (windowing)
    int             nWinSizeMax;  // maximum window size (windowing)
    int             nBTLimit;     // maximum number of SAT conflicts 
    int             nWords;       // simulation word count
    int             fMapping;     // generate mapping
    int             fMoreCuts;    // use several cuts
    int             fFindDivs;    // perform divisor search
    int             fUsePath;     // optimize only critical path
    int             fArea;        // area-oriented optimization
    int             fCover;       // use complete cover procedure
    int             fVerbose;     // verbose flag
    int             fVeryVerbose; // verbose flag
};


////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== sbdCnf.c ==========================================================*/
/*=== sbdCore.c ==========================================================*/
extern void         Sbd_ParSetDefault( Sbd_Par_t * pPars );
extern Gia_Man_t *  Sbd_NtkPerform( Gia_Man_t * p, Sbd_Par_t * pPars );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

