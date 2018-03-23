/**CFile****************************************************************

  FileName    [dsdInt.h]

  PackageName [DSD: Disjoint-support decomposition package.]

  Synopsis    [Internal declarations of the package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 8.0. Started - September 22, 2003.]

  Revision    [$Id: dsdInt.h,v 1.0 2002/22/09 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__bdd__dsd__dsdInt_h
#define ABC__bdd__dsd__dsdInt_h


#include "bdd/extrab/extraBdd.h"
#include "dsd.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                      TYPEDEF DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
typedef unsigned char byte;

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// DSD manager
struct Dsd_Manager_t_ 
{
    DdManager *    dd;         // the BDD manager
    st__table *     Table;      // the mapping of BDDs into their DEs
    int            nInputs;    // the number of primary inputs
    int            nRoots;     // the number of primary outputs
    int            nRootsAlloc;// the number of primary outputs
    Dsd_Node_t **  pInputs;    // the primary input nodes
    Dsd_Node_t **  pRoots;     // the primary output nodes
    Dsd_Node_t *   pConst1;    // the constant node
    int            fVerbose;   // the verbosity level 
};

// DSD node
struct Dsd_Node_t_
{
    Dsd_Type_t     Type;       // decomposition type
    DdNode *       G;          // function of the node   
    DdNode *       S;          // support of this function
    Dsd_Node_t **  pDecs;      // pointer to structures for formal inputs
    int            Mark;       // the mark used by CASE 4 of disjoint decomposition
    short          nDecs;      // the number of formal inputs
    short          nVisits;    // the counter of visits
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

#define MAXINPUTS 1000

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

/*=== dsdCheck.c =======================================================*/
extern void         Dsd_CheckCacheAllocate( int nEntries );
extern void         Dsd_CheckCacheDeallocate();
extern void         Dsd_CheckCacheClear();
extern int          Dsd_CheckRootFunctionIdentity( DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bC1, DdNode * bC2 );
/*=== dsdTree.c =======================================================*/
extern Dsd_Node_t * Dsd_TreeNodeCreate( int Type, int nDecs, int BlockNum );
extern void         Dsd_TreeNodeDelete( DdManager * dd, Dsd_Node_t * pNode );
extern void         Dsd_TreeUnmark( Dsd_Manager_t * dMan );
extern DdNode *     Dsd_TreeGetPrimeFunctionOld( DdManager * dd, Dsd_Node_t * pNode, int fRemap );



ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                           END OF FILE                            ///
////////////////////////////////////////////////////////////////////////

