/**CFile****************************************************************

  FileName    [bar.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Progress bar.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bar.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__bar__bar_h
#define ABC__aig__bar__bar_h


#ifdef _WIN32
#define inline __inline // compatible with MS VS 6.0
#endif

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


#define BAR_PROGRESS_USE   1

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////
 
typedef struct Bar_Progress_t_ Bar_Progress_t;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== bar.c ==========================================================*/
extern Bar_Progress_t *  Bar_ProgressStart( FILE * pFile, int nItemsTotal );
extern void              Bar_ProgressStop( Bar_Progress_t * p );
extern void              Bar_ProgressUpdate_int( Bar_Progress_t * p, int nItemsCur, char * pString );

static inline void       Bar_ProgressUpdate( Bar_Progress_t * p, int nItemsCur, char * pString ) {  
    if ( BAR_PROGRESS_USE && p && (nItemsCur < *((int*)p)) ) return;
    Bar_ProgressUpdate_int(p, nItemsCur, pString);
}




ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

