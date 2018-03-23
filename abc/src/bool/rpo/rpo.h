/**CFile****************************************************************

  FileName    [rpo.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [RPO]

  Synopsis    [Rpo Header]

  Author      [Mayler G. A. Martins / Vinicius Callegaro]
  
  Affiliation [UFRGS]

  Date        [Ver. 1.0. Started - May 08, 2013.]

  Revision    [$Id: rpo.h,v 1.00 2013/05/08 00:00:00 mgamartins Exp $]

***********************************************************************/
 
#ifndef ABC__bool__rpo__rpo_h
#define ABC__bool__rpo__rpo_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "literal.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Rpo_LCI_Edge_t_ Rpo_LCI_Edge_t;

struct Rpo_LCI_Edge_t_ {
    unsigned long visited : 1;
    unsigned long connectionType : 2;
    unsigned long reserved : 1;
    unsigned long idx1 : 30;
    unsigned long idx2 : 30;
};

void Rpo_PrintEdge(Rpo_LCI_Edge_t* edge);
int Rpo_CheckANDGroup(Literal_t* lit1, Literal_t* lit2, int nVars);
int Rpo_CheckORGroup(Literal_t* lit1, Literal_t* lit2, int nVars);
Literal_t* Rpo_Factorize(unsigned* target, int nVars, int nThreshold, int verbose);
Literal_t* Rpo_Recursion(unsigned* target, Literal_t** vecLit, int nLit, int nLitCount, int nVars, int* thresholdCount, int thresholdMax, int verbose);
Rpo_LCI_Edge_t* Rpo_CreateEdge(Operator_t op, int i, int j, int* vertexDegree);
int Rpo_computeMinEdgeCost(Rpo_LCI_Edge_t** edges, int edgeCount, int* vertexDegree);

ABC_NAMESPACE_HEADER_END
        
#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

