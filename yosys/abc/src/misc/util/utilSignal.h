/**CFile****************************************************************

  FileName    [utilSignal.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Signal handling utilities.]

  Synopsis    [Signal handling utilities.]

  Author      [Baruch Sterin]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2011.]

  Revision    [$Id: utilSignal.h,v 1.00 2011/02/01 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__util__utilSignal_h
#define ABC__misc__util__utilSignal_h

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

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== utilSignal.c ==========================================================*/

extern int       Util_SignalTmpFile(const char* prefix, const char* suffix, char** out_name);
extern void      Util_SignalTmpFileRemove(const char* fname, int fLeave);
extern int       Util_SignalSystem(const char* cmd);

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
