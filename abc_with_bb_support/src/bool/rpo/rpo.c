/**CFile****************************************************************

  FileName    [rpo.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [RPO]

  Synopsis    [Performs read polarity once factorization.]

  Author      [Mayler G. A. Martins / Vinicius Callegaro]
  
  Affiliation [UFRGS]

  Date        [Ver. 1.0. Started - May 08, 2013.]

  Revision    [$Id: rpo.c,v 1.00 2013/05/08 00:00:00 mgamartins Exp $]

 ***********************************************************************/

#include <stdio.h>

#include "literal.h"
#include "rpo.h"
#include "bool/kit/kit.h"
#include "misc/util/abc_global.h"
#include "misc/vec/vec.h"


ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


typedef struct Rpo_Man_t_ Rpo_Man_t;

struct Rpo_Man_t_ {
    unsigned * target;
    int nVars;

    Literal_t ** literals;
    int nLits;
    int nLitsMax;

    Rpo_LCI_Edge_t* lci;
    int nLCIElems;

    int thresholdMax;

};


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Check if two literals are AND-grouped]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/
int Rpo_CheckANDGroup(Literal_t* lit1, Literal_t* lit2, int nVars) {
    unsigned* notLit1Func = ABC_ALLOC(unsigned, Kit_TruthWordNum(nVars));
    unsigned* notLit2Func = ABC_ALLOC(unsigned, Kit_TruthWordNum(nVars));
    unsigned* and1;
    unsigned* and2;
    int isZero;

    Kit_TruthNot(notLit1Func, lit1->function, nVars);
    Kit_TruthNot(notLit2Func, lit2->function, nVars);
    and1 = ABC_ALLOC(unsigned, Kit_TruthWordNum(nVars));
    and2 = ABC_ALLOC(unsigned, Kit_TruthWordNum(nVars));
    Kit_TruthAnd(and1, lit1->transition, notLit2Func, nVars);
    isZero = Kit_TruthIsConst0(and1, nVars);
    if (isZero) {
        Kit_TruthAnd(and2, lit2->transition, notLit1Func, nVars);
        isZero = Kit_TruthIsConst0(and2, nVars);
    }
    ABC_FREE(notLit1Func);
    ABC_FREE(notLit2Func);
    ABC_FREE(and1);
    ABC_FREE(and2);
    return isZero;
}

/**Function*************************************************************

  Synopsis    [Check if two literals are AND-grouped]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/
int Rpo_CheckORGroup(Literal_t* lit1, Literal_t* lit2, int nVars) {
    unsigned* and1 = ABC_ALLOC(unsigned, Kit_TruthWordNum(nVars));
    unsigned* and2 = ABC_ALLOC(unsigned, Kit_TruthWordNum(nVars));
    int isZero;
    Kit_TruthAnd(and1, lit1->transition, lit2->function, nVars);
    isZero = Kit_TruthIsConst0(and1, nVars);
    if (isZero) {
        Kit_TruthAnd(and2, lit2->transition, lit1->function, nVars);
        isZero = Kit_TruthIsConst0(and2, nVars);
    }
    ABC_FREE(and1);
    ABC_FREE(and2);
    return isZero;
}

Rpo_LCI_Edge_t* Rpo_CreateEdge(Operator_t op, int i, int j, int* vertexDegree) {
    Rpo_LCI_Edge_t* edge = ABC_ALLOC(Rpo_LCI_Edge_t, 1);
    edge->connectionType = op;
    edge->idx1 = i;
    edge->idx2 = j;
    edge->visited = 0;
    vertexDegree[i]++;
    vertexDegree[j]++;
    return edge;
}

int Rpo_computeMinEdgeCost(Rpo_LCI_Edge_t** edges, int edgeCount, int* vertexDegree) {
    int minCostIndex = -1;
    int minVertexIndex = -1;
    unsigned int minCost = ~0;
    Rpo_LCI_Edge_t* edge;
    unsigned int edgeCost;
    int minVertex;
    int i;
    for (i = 0; i < edgeCount; ++i) {
        edge = edges[i];
        if (!edge->visited) {
            edgeCost = vertexDegree[edge->idx1] + vertexDegree[edge->idx2];
            minVertex = (edge->idx1 < edge->idx2) ? edge->idx1 : edge->idx2;
            if (edgeCost < minCost) {
                minCost = edgeCost;
                minCostIndex = i;
                minVertexIndex = minVertex;
            } else if ((edgeCost == minCost) && minVertex < minVertexIndex) {
                minCost = edgeCost;
                minCostIndex = i;
                minVertexIndex = minVertex;
            }
        }
    }
    return minCostIndex;
}

void Rpo_PrintEdge(Rpo_LCI_Edge_t* edge) {
    Abc_Print(-2, "Edge (%d,%d)/%d\n", edge->idx1, edge->idx2, edge->connectionType);
}

/**Function*************************************************************

  Synopsis    [Test]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/
Literal_t* Rpo_Factorize(unsigned* target, int nVars, int nThreshold, int verbose) {

    int nLitCap = nVars * 2;
    int nLit = 0;
    int w;
    Literal_t** vecLit;
    Literal_t* lit;
    Literal_t* result;
    int thresholdCount = 0;

    if (Kit_TruthIsConst0(target, nVars)) {
        return Lit_CreateLiteralConst(target, nVars, 0);
    } else if (Kit_TruthIsConst1(target, nVars)) {
        return Lit_CreateLiteralConst(target, nVars, 1);
    }

    //    if (nThreshold == -1) {
    //        nThreshold = nLitCap + nVars;
    //    }
    if (verbose) {
        Abc_Print(-2, "Target: ");
        Lit_PrintTT(target, nVars);
        Abc_Print(-2, "\n");
    }
    vecLit = ABC_ALLOC(Literal_t*, nLitCap);

    for (w = nVars - 1; w >= 0; w--) {
        lit = Lit_Alloc(target, nVars, w, '+');
        if (lit != NULL) {
            vecLit[nLit] = lit;
            nLit++;
        }
        lit = Lit_Alloc(target, nVars, w, '-');
        if (lit != NULL) {
            vecLit[nLit] = lit;
            nLit++;
        }
    }
    if (verbose) {
        Abc_Print(-2, "Allocated %d literal clusters\n", nLit);
    }

    result = Rpo_Recursion(target, vecLit, nLit, nLit, nVars, &thresholdCount, nThreshold, verbose);

    //freeing the memory
    for (w = 0; w < nLit; ++w) {
        Lit_Free(vecLit[w]);
    }
    ABC_FREE(vecLit);

    return result;

}

Literal_t* Rpo_Recursion(unsigned* target, Literal_t** vecLit, int nLit, int nLitCount, int nVars, int* thresholdCount, int thresholdMax, int verbose) {
    int i, j, k;
    Literal_t* copyResult;
    int* vertexDegree;
    int v;
    int edgeSize;
    Rpo_LCI_Edge_t** edges;
    int edgeCount = 0;
    int isAnd;
    int isOr;
    Rpo_LCI_Edge_t* edge;
    Literal_t* result = NULL;
    int edgeIndex;
    int minLitIndex;
    int maxLitIndex;
    Literal_t* oldLit1;
    Literal_t* oldLit2;
    Literal_t* newLit;

    *thresholdCount = *thresholdCount + 1;
    if (*thresholdCount == thresholdMax) {
        return NULL;
    }
    if (verbose) {
        Abc_Print(-2, "Entering recursion %d\n", *thresholdCount);
    }
    // verify if solution is the target or not
    if (nLitCount == 1) {
        if (verbose) {
            Abc_Print(-2, "Checking solution: ");
        }
        for (k = 0; k < nLit; ++k) {
            if (vecLit[k] != NULL) {
                if (Kit_TruthIsEqual(target, vecLit[k]->function, nVars)) {
                    copyResult = Lit_Copy(vecLit[k], nVars);
                    if (verbose) {
                        Abc_Print(-2, "FOUND!\n", thresholdCount);
                    }
                    thresholdCount = 0; //??
                    return copyResult;
                }
            }
        }
        if (verbose) {
            Abc_Print(-2, "FAILED!\n", thresholdCount);
        }
        return NULL;
    }

    vertexDegree = ABC_ALLOC(int, nLit);
    //    if(verbose) {
    //        Abc_Print(-2,"Allocating vertexDegree...\n");
    //    }
    for (v = 0; v < nLit; v++) {
        vertexDegree[v] = 0;
    }
    // building edges
    edgeSize = (nLit * (nLit - 1)) / 2;
    edges = ABC_ALLOC(Rpo_LCI_Edge_t*, edgeSize);
    if (verbose) {
        Abc_Print(-2, "Creating Edges: \n");
    }

    for (i = 0; i < nLit; ++i) {
        if (vecLit[i] == NULL) {
            continue;
        }
        for (j = i; j < nLit; ++j) {
            if (vecLit[j] == NULL) {
                continue;
            }
            isAnd = Rpo_CheckANDGroup(vecLit[i], vecLit[j], nVars);
            isOr = Rpo_CheckORGroup(vecLit[i], vecLit[j], nVars);
            if (isAnd) {
                if (verbose) {
                    Abc_Print(-2, "Grouped: ");
                    Lit_PrintExp(vecLit[j]);
                    Abc_Print(-2, " AND ");
                    Lit_PrintExp(vecLit[i]);
                    Abc_Print(-2, "\n");
                }
                // add edge
                edge = Rpo_CreateEdge(LIT_AND, i, j, vertexDegree);
                edges[edgeCount++] = edge;
            }
            if (isOr) {
                if (verbose) {
                    Abc_Print(-2, "Grouped: ");
                    Lit_PrintExp(vecLit[j]);
                    Abc_Print(-2, " OR ");
                    Lit_PrintExp(vecLit[i]);
                    Abc_Print(-2, "\n");
                }
                // add edge
                edge = Rpo_CreateEdge(LIT_OR, i, j, vertexDegree);
                edges[edgeCount++] = edge;
            }
        }
    }
    if (verbose) {
        Abc_Print(-2, "%d edges created.\n", edgeCount);
    }


    //traverse the edges, grouping new Literal Clusters
    do {
        edgeIndex = Rpo_computeMinEdgeCost(edges, edgeCount, vertexDegree);
        if (edgeIndex < 0) {
            if (verbose) {
                Abc_Print(-2, "There is no edges unvisited... Exiting recursion.\n");
                //exit(-1);
            }
            break;
            //return NULL; // the graph does not have unvisited edges
        }
        edge = edges[edgeIndex];
        edge->visited = 1;
        //Rpo_PrintEdge(edge);
        minLitIndex = (edge->idx1 < edge->idx2) ? edge->idx1 : edge->idx2;
        maxLitIndex = (edge->idx1 > edge->idx2) ? edge->idx1 : edge->idx2;
        oldLit1 = vecLit[minLitIndex];
        oldLit2 = vecLit[maxLitIndex];
        newLit = Lit_GroupLiterals(oldLit1, oldLit2, (Operator_t)edge->connectionType, nVars);
        vecLit[minLitIndex] = newLit;
        vecLit[maxLitIndex] = NULL;

        if (verbose) {
            Abc_Print(-2, "New Literal Cluster found: ");
            Lit_PrintExp(newLit);
            Abc_Print(-2, " -> ");
            Lit_PrintTT(newLit->function, nVars);
            Abc_Print(-2, "\n");
        }
        result = Rpo_Recursion(target, vecLit, nLit, (nLitCount - 1), nVars, thresholdCount, thresholdMax, verbose);
        //independent of result, free the newLit and restore the vector of Literal Clusters
        Lit_Free(newLit);
        vecLit[minLitIndex] = oldLit1;
        vecLit[maxLitIndex] = oldLit2;
        if (*thresholdCount == thresholdMax) {
            break;
        }
    } while (result == NULL);
    //freeing memory
    //    if(verbose) {
    //        Abc_Print(-2,"Freeing vertexDegree...\n");
    //    }
    ABC_FREE(vertexDegree);
    for (i = 0; i < edgeCount; ++i) {
        //Abc_Print(-2, "%p ", edges[i]);
        ABC_FREE(edges[i]);
    }
    ABC_FREE(edges);
    return result;
}

ABC_NAMESPACE_IMPL_END
