/**CFile****************************************************************

  FileName    [cudd2.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic And-Inverter Graph package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 3, 2006.]

  Revision    [$Id: cudd2.h,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__hop__cudd2_h
#define ABC__aig__hop__cudd2_h


// HA: Added for printing messages
#ifndef MSG 
#define MSG(msg) (printf("%s = \n",(msg)));
#endif

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
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

extern void Cudd2_Init      ( unsigned int numVars, unsigned int numVarsZ, unsigned int numSlots, unsigned int cacheSize, unsigned long maxMemory, void * pCudd );
extern void Cudd2_Quit      ( void * pCudd );
extern void Cudd2_bddOne    ( void * pCudd, void * pResult );
extern void Cudd2_bddIthVar ( void * pCudd, int iVar, void * pResult );
extern void Cudd2_bddAnd    ( void * pCudd, void * pArg0, void * pArg1, void * pResult );
extern void Cudd2_bddOr     ( void * pCudd, void * pArg0, void * pArg1, void * pResult );
extern void Cudd2_bddNand   ( void * pCudd, void * pArg0, void * pArg1, void * pResult );
extern void Cudd2_bddNor    ( void * pCudd, void * pArg0, void * pArg1, void * pResult );
extern void Cudd2_bddXor    ( void * pCudd, void * pArg0, void * pArg1, void * pResult );
extern void Cudd2_bddXnor   ( void * pCudd, void * pArg0, void * pArg1, void * pResult );
extern void Cudd2_bddIte    ( void * pCudd, void * pArg0, void * pArg1, void * pArg2, void * pResult );
extern void Cudd2_bddCompose( void * pCudd, void * pArg0, void * pArg1, int v, void * pResult );
extern void Cudd2_bddLeq    ( void * pCudd, void * pArg0, void * pArg1, int Result );
extern void Cudd2_bddEqual  ( void * pCudd, void * pArg0, void * pArg1, int Result );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

