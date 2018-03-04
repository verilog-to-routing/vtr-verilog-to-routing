/**CFile****************************************************************

  FileName    [kit.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Computation kit.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 6, 2006.]

  Revision    [$Id: kit.h,v 1.00 2006/12/06 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__kit__kit_h
#define ABC__aig__kit__kit_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "cloud.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Kit_Sop_t_ Kit_Sop_t;
struct Kit_Sop_t_
{
    int               nLits;          // the number of literals
    int               nCubes;         // the number of cubes
    unsigned *        pCubes;         // the storage for cubes
};

typedef struct Kit_Edge_t_ Kit_Edge_t;
struct Kit_Edge_t_
{
    unsigned          fCompl   :  1;   // the complemented bit
    unsigned          Node     : 30;   // the decomposition node pointed by the edge
};

typedef struct Kit_Node_t_ Kit_Node_t;
struct Kit_Node_t_
{
    Kit_Edge_t        eEdge0;          // the left child of the node
    Kit_Edge_t        eEdge1;          // the right child of the node
    // other info
    union { int       iFunc;           // the function of the node (BDD or AIG)
    void *            pFunc; };        // the function of the node (BDD or AIG)
    unsigned          Level    : 14;   // the level of this node in the global AIG
    // printing info 
    unsigned          fNodeOr  :  1;   // marks the original OR node
    unsigned          fCompl0  :  1;   // marks the original complemented edge
    unsigned          fCompl1  :  1;   // marks the original complemented edge
    // latch info
    unsigned          nLat0    :  5;   // the number of latches on the first edge
    unsigned          nLat1    :  5;   // the number of latches on the second edge
    unsigned          nLat2    :  5;   // the number of latches on the output edge
};

typedef struct Kit_Graph_t_ Kit_Graph_t;
struct Kit_Graph_t_
{
    int               fConst;          // marks the constant 1 graph
    int               nLeaves;         // the number of leaves
    int               nSize;           // the number of nodes (including the leaves) 
    int               nCap;            // the number of allocated nodes
    Kit_Node_t *      pNodes;          // the array of leaves and internal nodes
    Kit_Edge_t        eRoot;           // the pointer to the topmost node
};


// DSD node types
typedef enum { 
    KIT_DSD_NONE  = 0,  // 0: unknown
    KIT_DSD_CONST1,     // 1: constant 1
    KIT_DSD_VAR,        // 2: elementary variable
    KIT_DSD_AND,        // 3: multi-input AND
    KIT_DSD_XOR,        // 4: multi-input XOR
    KIT_DSD_PRIME       // 5: arbitrary function of 3+ variables
} Kit_Dsd_t;

// DSD node
typedef struct Kit_DsdObj_t_ Kit_DsdObj_t;
struct Kit_DsdObj_t_
{ 
    unsigned       Id         : 6;  // the number of this node
    unsigned       Type       : 3;  // none, const, var, AND, XOR, MUX, PRIME
    unsigned       fMark      : 1;  // finished checking output
    unsigned       Offset     : 8;  // offset to the truth table
    unsigned       nRefs      : 8;  // offset to the truth table
    unsigned       nFans      : 6;  // the number of fanins of this node
    unsigned short pFans[0];        // the fanin literals
};

// DSD network
typedef struct Kit_DsdNtk_t_ Kit_DsdNtk_t;
struct Kit_DsdNtk_t_
{
    unsigned short nVars;           // at most 16 (perhaps 18?)
    unsigned short nNodesAlloc;     // the number of allocated nodes (at most nVars)
    unsigned short nNodes;          // the number of nodes
    unsigned short Root;            // the root of the tree
    unsigned *     pMem;            // memory for the truth tables (memory manager?)
    unsigned *     pSupps;          // supports of the nodes
    Kit_DsdObj_t** pNodes;          // the nodes
};

// DSD manager
typedef struct Kit_DsdMan_t_ Kit_DsdMan_t;
struct Kit_DsdMan_t_
{
    int            nVars;           // the maximum number of variables
    int            nWords;          // the number of words in TTs
    Vec_Ptr_t *    vTtElems;        // elementary truth tables
    Vec_Ptr_t *    vTtNodes;        // the node truth tables
    // BDD representation
    CloudManager * dd;              // BDD package
    Vec_Ptr_t *    vTtBdds;         // the node truth tables
    Vec_Int_t *    vNodes;          // temporary array for BDD nodes
};
 
static inline unsigned        Kit_DsdObjOffset( int nFans )          { return (nFans >> 1) + ((nFans & 1) > 0);                    }
static inline unsigned *      Kit_DsdObjTruth( Kit_DsdObj_t * pObj ) { return pObj->Type == KIT_DSD_PRIME ? (unsigned *)pObj->pFans + pObj->Offset: NULL; }
static inline int             Kit_DsdNtkObjNum( Kit_DsdNtk_t * pNtk ){ return pNtk->nVars + pNtk->nNodes; }
static inline Kit_DsdObj_t *  Kit_DsdNtkObj( Kit_DsdNtk_t * pNtk, int Id )      { assert( Id >= 0 && Id < pNtk->nVars + pNtk->nNodes ); return Id < pNtk->nVars ? NULL : pNtk->pNodes[Id - pNtk->nVars]; }
static inline Kit_DsdObj_t *  Kit_DsdNtkRoot( Kit_DsdNtk_t * pNtk )             { return Kit_DsdNtkObj( pNtk, Abc_Lit2Var(pNtk->Root) );                      }
static inline int             Kit_DsdLitIsLeaf( Kit_DsdNtk_t * pNtk, int Lit )   { int Id = Abc_Lit2Var(Lit); assert( Id >= 0 && Id < pNtk->nVars + pNtk->nNodes ); return Id < pNtk->nVars; }
static inline unsigned        Kit_DsdLitSupport( Kit_DsdNtk_t * pNtk, int Lit )  { int Id = Abc_Lit2Var(Lit); assert( Id >= 0 && Id < pNtk->nVars + pNtk->nNodes ); return pNtk->pSupps? (Id < pNtk->nVars? (1 << Id) : pNtk->pSupps[Id - pNtk->nVars]) : 0; }

#define Kit_DsdNtkForEachObj( pNtk, pObj, i )                                      \
    for ( i = 0; (i < (pNtk)->nNodes) && ((pObj) = (pNtk)->pNodes[i]); i++ )
#define Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )                              \
    for ( i = 0; (i < (int)(pObj)->nFans) && ((iLit) = (pObj)->pFans[i], 1); i++ )
#define Kit_DsdObjForEachFaninReverse( pNtk, pObj, iLit, i )                       \
    for ( i = (int)(pObj)->nFans - 1; (i >= 0) && ((iLit) = (pObj)->pFans[i], 1); i-- )

#define Kit_PlaForEachCube( pSop, nFanins, pCube )                \
    for ( pCube = (pSop); *pCube; pCube += (nFanins) + 3 )
#define Kit_PlaCubeForEachVar( pCube, Value, i )                     \
    for ( i = 0; (pCube[i] != ' ') && (Value = pCube[i]); i++ )           

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define KIT_MIN(a,b)       (((a) < (b))? (a) : (b))
#define KIT_MAX(a,b)       (((a) > (b))? (a) : (b))
#define KIT_INFINITY       (100000000)

static inline int          Kit_CubeHasLit( unsigned uCube, int i )                        { return(uCube &  (unsigned)(1<<i)) > 0;  }
static inline unsigned     Kit_CubeSetLit( unsigned uCube, int i )                        { return uCube |  (unsigned)(1<<i);       }
static inline unsigned     Kit_CubeXorLit( unsigned uCube, int i )                        { return uCube ^  (unsigned)(1<<i);       }
static inline unsigned     Kit_CubeRemLit( unsigned uCube, int i )                        { return uCube & ~(unsigned)(1<<i);       }

static inline int          Kit_CubeContains( unsigned uLarge, unsigned uSmall )           { return (uLarge & uSmall) == uSmall;     }
static inline unsigned     Kit_CubeSharp( unsigned uCube, unsigned uMask )                { return uCube & ~uMask;                  }
static inline unsigned     Kit_CubeMask( int nVar )                                       { return (~(unsigned)0) >> (32-nVar);     }

static inline int          Kit_CubeIsMarked( unsigned uCube )                             { return Kit_CubeHasLit( uCube, 31 );     }
static inline unsigned     Kit_CubeMark( unsigned uCube )                                 { return Kit_CubeSetLit( uCube, 31 );     }
static inline unsigned     Kit_CubeUnmark( unsigned uCube )                               { return Kit_CubeRemLit( uCube, 31 );     }

static inline int          Kit_SopCubeNum( Kit_Sop_t * cSop )                             { return cSop->nCubes;                    }
static inline unsigned     Kit_SopCube( Kit_Sop_t * cSop, int i )                         { return cSop->pCubes[i];                 }
static inline void         Kit_SopShrink( Kit_Sop_t * cSop, int nCubesNew )               { cSop->nCubes = nCubesNew;               }
static inline void         Kit_SopPushCube( Kit_Sop_t * cSop, unsigned uCube )            { cSop->pCubes[cSop->nCubes++] = uCube;   }
static inline void         Kit_SopWriteCube( Kit_Sop_t * cSop, unsigned uCube, int i )    { cSop->pCubes[i] = uCube;                }

static inline Kit_Edge_t   Kit_EdgeCreate( int Node, int fCompl )                         { Kit_Edge_t eEdge = { fCompl, Node }; return eEdge;  }
static inline unsigned     Kit_EdgeToInt( Kit_Edge_t eEdge )                              { return (eEdge.Node << 1) | eEdge.fCompl;            }
static inline Kit_Edge_t   Kit_IntToEdge( unsigned Edge )                                 { return Kit_EdgeCreate( Edge >> 1, Edge & 1 );       }
//static inline unsigned     Kit_EdgeToInt_( Kit_Edge_t eEdge )                             { return *(unsigned *)&eEdge;                         }
//static inline Kit_Edge_t   Kit_IntToEdge_( unsigned Edge )                                { return *(Kit_Edge_t *)&Edge;                        }
static inline unsigned     Kit_EdgeToInt_( Kit_Edge_t m )                                 { union { Kit_Edge_t x; unsigned y; } v; v.x = m; return v.y;  }
static inline Kit_Edge_t   Kit_IntToEdge_( unsigned m )                                   { union { Kit_Edge_t x; unsigned y; } v; v.y = m; return v.x;  }

static inline int          Kit_GraphIsConst( Kit_Graph_t * pGraph )                       { return pGraph->fConst;                              }
static inline int          Kit_GraphIsConst0( Kit_Graph_t * pGraph )                      { return pGraph->fConst && pGraph->eRoot.fCompl;      }
static inline int          Kit_GraphIsConst1( Kit_Graph_t * pGraph )                      { return pGraph->fConst && !pGraph->eRoot.fCompl;     }
static inline int          Kit_GraphIsComplement( Kit_Graph_t * pGraph )                  { return pGraph->eRoot.fCompl;                        }
static inline int          Kit_GraphIsVar( Kit_Graph_t * pGraph )                         { return pGraph->eRoot.Node < (unsigned)pGraph->nLeaves; }
static inline void         Kit_GraphComplement( Kit_Graph_t * pGraph )                    { pGraph->eRoot.fCompl ^= 1;                          }
static inline void         Kit_GraphSetRoot( Kit_Graph_t * pGraph, Kit_Edge_t eRoot )     { pGraph->eRoot = eRoot;                              }
static inline int          Kit_GraphLeaveNum( Kit_Graph_t * pGraph )                      { return pGraph->nLeaves;                             }
static inline int          Kit_GraphNodeNum( Kit_Graph_t * pGraph )                       { return pGraph->nSize - pGraph->nLeaves;             }
static inline Kit_Node_t * Kit_GraphNode( Kit_Graph_t * pGraph, int i )                   { return pGraph->pNodes + i;                          }
static inline Kit_Node_t * Kit_GraphNodeLast( Kit_Graph_t * pGraph )                      { return pGraph->pNodes + pGraph->nSize - 1;          }
static inline int          Kit_GraphNodeInt( Kit_Graph_t * pGraph, Kit_Node_t * pNode )   { return pNode - pGraph->pNodes;                      }
static inline int          Kit_GraphNodeIsVar( Kit_Graph_t * pGraph, Kit_Node_t * pNode ) { return Kit_GraphNodeInt(pGraph,pNode) < pGraph->nLeaves; }
static inline Kit_Node_t * Kit_GraphVar( Kit_Graph_t * pGraph )                           { assert( Kit_GraphIsVar( pGraph ) );    return Kit_GraphNode( pGraph, pGraph->eRoot.Node );      }
static inline int          Kit_GraphVarInt( Kit_Graph_t * pGraph )                        { assert( Kit_GraphIsVar( pGraph ) );    return Kit_GraphNodeInt( pGraph, Kit_GraphVar(pGraph) ); }
static inline Kit_Node_t * Kit_GraphNodeFanin0( Kit_Graph_t * pGraph, Kit_Node_t * pNode ){ return Kit_GraphNodeIsVar(pGraph, pNode)? NULL : Kit_GraphNode(pGraph, pNode->eEdge0.Node);     }
static inline Kit_Node_t * Kit_GraphNodeFanin1( Kit_Graph_t * pGraph, Kit_Node_t * pNode ){ return Kit_GraphNodeIsVar(pGraph, pNode)? NULL : Kit_GraphNode(pGraph, pNode->eEdge1.Node);     }
static inline int          Kit_GraphRootLevel( Kit_Graph_t * pGraph )                     { return Kit_GraphNode(pGraph, pGraph->eRoot.Node)->Level;                                        }

static inline int          Kit_SuppIsMinBase( int Supp )                                  { return (Supp & (Supp+1)) == 0;                      }

static inline int          Kit_BitWordNum( int nBits )    { return nBits/(8*sizeof(unsigned)) + ((nBits%(8*sizeof(unsigned))) > 0); }
static inline int          Kit_TruthWordNum( int nVars )  { return nVars <= 5 ? 1 : (1 << (nVars - 5));                             }
static inline unsigned     Kit_BitMask( int nBits )       { assert( nBits <= 32 ); return ~((~(unsigned)0) << nBits);               }

static inline void         Kit_TruthSetBit( unsigned * p, int Bit )   { p[Bit>>5] |= (1<<(Bit & 31));               }
static inline void         Kit_TruthXorBit( unsigned * p, int Bit )   { p[Bit>>5] ^= (1<<(Bit & 31));               }
static inline int          Kit_TruthHasBit( unsigned * p, int Bit )   { return (p[Bit>>5] & (1<<(Bit & 31))) > 0;   }

static inline int Kit_WordFindFirstBit( unsigned uWord )
{
    int i;
    for ( i = 0; i < 32; i++ )
        if ( uWord & (1 << i) )
            return i;
    return -1;
}
static inline int Kit_WordHasOneBit( unsigned uWord )
{
    return (uWord & (uWord - 1)) == 0;
}
static inline int Kit_WordCountOnes( unsigned uWord )
{
    uWord = (uWord & 0x55555555) + ((uWord>>1) & 0x55555555);
    uWord = (uWord & 0x33333333) + ((uWord>>2) & 0x33333333);
    uWord = (uWord & 0x0F0F0F0F) + ((uWord>>4) & 0x0F0F0F0F);
    uWord = (uWord & 0x00FF00FF) + ((uWord>>8) & 0x00FF00FF);
    return  (uWord & 0x0000FFFF) + (uWord>>16);
}
static inline int Kit_TruthCountOnes( unsigned * pIn, int nVars )
{
    int w, Counter = 0;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        Counter += Kit_WordCountOnes(pIn[w]);
    return Counter;
}
static inline int Kit_TruthFindFirstBit( unsigned * pIn, int nVars )
{
    int w;
    for ( w = 0; w < Kit_TruthWordNum(nVars); w++ )
        if ( pIn[w] )
            return 32*w + Kit_WordFindFirstBit(pIn[w]);
    return -1;
}
static inline int Kit_TruthFindFirstZero( unsigned * pIn, int nVars )
{
    int w;
    for ( w = 0; w < Kit_TruthWordNum(nVars); w++ )
        if ( ~pIn[w] )
            return 32*w + Kit_WordFindFirstBit(~pIn[w]);
    return -1;
}
static inline int Kit_TruthIsEqual( unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn0[w] != pIn1[w] )
            return 0;
    return 1;
}
static inline int Kit_TruthIsEqualWithCare( unsigned * pIn0, unsigned * pIn1, unsigned * pCare, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( (pIn0[w] & pCare[w]) != (pIn1[w] & pCare[w]) )
            return 0;
    return 1;
}
static inline int Kit_TruthIsOpposite( unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn0[w] != ~pIn1[w] )
            return 0;
    return 1;
}
static inline int Kit_TruthIsEqualWithPhase( unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    if ( (pIn0[0] & 1) == (pIn1[0] & 1) )
    {
        for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
            if ( pIn0[w] != pIn1[w] )
                return 0;
    }
    else
    {
        for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
            if ( pIn0[w] != ~pIn1[w] )
                return 0;
    }
    return 1;
}
static inline int Kit_TruthIsConst0( unsigned * pIn, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn[w] )
            return 0;
    return 1;
}
static inline int Kit_TruthIsConst1( unsigned * pIn, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn[w] != ~(unsigned)0 )
            return 0;
    return 1;
}
static inline int Kit_TruthIsImply( unsigned * pIn1, unsigned * pIn2, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn1[w] & ~pIn2[w] )
            return 0;
    return 1;
}
static inline int Kit_TruthIsDisjoint( unsigned * pIn1, unsigned * pIn2, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn1[w] & pIn2[w] )
            return 0;
    return 1;
}
static inline int Kit_TruthIsDisjoint3( unsigned * pIn1, unsigned * pIn2, unsigned * pIn3, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn1[w] & pIn2[w] & pIn3[w] )
            return 0;
    return 1;
}
static inline void Kit_TruthCopy( unsigned * pOut, unsigned * pIn, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn[w];
}
static inline void Kit_TruthClear( unsigned * pOut, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = 0;
}
static inline void Kit_TruthFill( unsigned * pOut, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~(unsigned)0;
}
static inline void Kit_TruthNot( unsigned * pOut, unsigned * pIn, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~pIn[w];
}
static inline void Kit_TruthAnd( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn0[w] & pIn1[w];
}
static inline void Kit_TruthOr( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn0[w] | pIn1[w];
}
static inline void Kit_TruthXor( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn0[w] ^ pIn1[w];
}
static inline void Kit_TruthSharp( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn0[w] & ~pIn1[w];
}
static inline void Kit_TruthNand( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~(pIn0[w] & pIn1[w]);
}
static inline void Kit_TruthAndPhase( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars, int fCompl0, int fCompl1 )
{
    int w;
    if ( fCompl0 && fCompl1 )
    {
        for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = ~(pIn0[w] | pIn1[w]);
    }
    else if ( fCompl0 && !fCompl1 )
    {
        for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = ~pIn0[w] & pIn1[w];
    }
    else if ( !fCompl0 && fCompl1 )
    {
        for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = pIn0[w] & ~pIn1[w];
    }
    else // if ( !fCompl0 && !fCompl1 )
    {
        for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = pIn0[w] & pIn1[w];
    }
}
static inline void Kit_TruthOrPhase( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars, int fCompl0, int fCompl1 )
{
    int w;
    if ( fCompl0 && fCompl1 )
    {
        for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = ~(pIn0[w] & pIn1[w]);
    }
    else if ( fCompl0 && !fCompl1 )
    {
        for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = ~pIn0[w] | pIn1[w];
    }
    else if ( !fCompl0 && fCompl1 )
    {
        for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = pIn0[w] | ~pIn1[w];
    }
    else // if ( !fCompl0 && !fCompl1 )
    {
        for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = pIn0[w] | pIn1[w];
    }
}
static inline void Kit_TruthMux( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, unsigned * pCtrl, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = (pIn0[w] & ~pCtrl[w]) | (pIn1[w] & pCtrl[w]);
}
static inline void Kit_TruthMuxPhase( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, unsigned * pCtrl, int nVars, int fComp0 )
{
    int w;
    if ( fComp0 )
        for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = (~pIn0[w] & ~pCtrl[w]) | (pIn1[w] & pCtrl[w]);
    else
        for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = (pIn0[w] & ~pCtrl[w]) | (pIn1[w] & pCtrl[w]);
}
static inline void Kit_TruthIthVar( unsigned * pTruth, int nVars, int iVar )
{
    unsigned Masks[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    int k, nWords = (nVars <= 5 ? 1 : (1 << (nVars - 5)));
    if ( iVar < 5 )
    {
        for ( k = 0; k < nWords; k++ )
            pTruth[k] = Masks[iVar];
    }
    else
    {
        for ( k = 0; k < nWords; k++ )
            if ( k & (1 << (iVar-5)) )
                pTruth[k] = ~(unsigned)0;
            else
                pTruth[k] = 0;
    }
}


////////////////////////////////////////////////////////////////////////
///                           ITERATORS                              ///
////////////////////////////////////////////////////////////////////////

#define Kit_SopForEachCube( cSop, uCube, i )                                      \
    for ( i = 0; (i < Kit_SopCubeNum(cSop)) && ((uCube) = Kit_SopCube(cSop, i)); i++ )
#define Kit_CubeForEachLiteral( uCube, Lit, nLits, i )                            \
    for ( i = 0; (i < (nLits)) && ((Lit) = Kit_CubeHasLit(uCube, i)); i++ )

#define Kit_GraphForEachLeaf( pGraph, pLeaf, i )                                              \
    for ( i = 0; (i < (pGraph)->nLeaves) && (((pLeaf) = Kit_GraphNode(pGraph, i)), 1); i++ )
#define Kit_GraphForEachNode( pGraph, pAnd, i )                                               \
    for ( i = (pGraph)->nLeaves; (i < (pGraph)->nSize) && (((pAnd) = Kit_GraphNode(pGraph, i)), 1); i++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== kitBdd.c ==========================================================*/
#ifdef ABC_USE_CUDD
extern DdNode *        Kit_SopToBdd( DdManager * dd, Kit_Sop_t * cSop, int nVars );
extern DdNode *        Kit_GraphToBdd( DdManager * dd, Kit_Graph_t * pGraph );
extern DdNode *        Kit_TruthToBdd( DdManager * dd, unsigned * pTruth, int nVars, int fMSBonTop );
#endif
/*=== kitCloud.c ==========================================================*/
extern CloudNode *     Kit_TruthToCloud( CloudManager * dd, unsigned * pTruth, int nVars );
extern unsigned *      Kit_CloudToTruth( Vec_Int_t * vNodes, int nVars, Vec_Ptr_t * vStore, int fInv );
extern int             Kit_CreateCloud( CloudManager * dd, CloudNode * pFunc, Vec_Int_t * vNodes );
extern int             Kit_CreateCloudFromTruth( CloudManager * dd, unsigned * pTruth, int nVars, Vec_Int_t * vNodes );
extern unsigned *      Kit_TruthCompose( CloudManager * dd, unsigned * pTruth, int nVars, unsigned ** pInputs, int nVarsAll, Vec_Ptr_t * vStore, Vec_Int_t * vNodes );
extern void            Kit_TruthCofSupports( Vec_Int_t * vBddDir, Vec_Int_t * vBddInv, int nVars, Vec_Int_t * vMemory, unsigned * puSupps );
/*=== kitDsd.c ==========================================================*/
extern Kit_DsdMan_t *  Kit_DsdManAlloc( int nVars, int nNodes );
extern void            Kit_DsdManFree( Kit_DsdMan_t * p );
extern Kit_DsdNtk_t *  Kit_DsdDeriveNtk( unsigned * pTruth, int nVars, int nLutSize );
extern unsigned *      Kit_DsdTruthCompute( Kit_DsdMan_t * p, Kit_DsdNtk_t * pNtk );
extern void            Kit_DsdTruth( Kit_DsdNtk_t * pNtk, unsigned * pTruthRes );
extern void            Kit_DsdTruthPartial( Kit_DsdMan_t * p, Kit_DsdNtk_t * pNtk, unsigned * pTruthRes, unsigned uSupp );
extern void            Kit_DsdTruthPartialTwo( Kit_DsdMan_t * p, Kit_DsdNtk_t * pNtk, unsigned uSupp, int iVar, unsigned * pTruthCo, unsigned * pTruthDec );
extern void            Kit_DsdPrint( FILE * pFile, Kit_DsdNtk_t * pNtk );
extern void            Kit_DsdPrintExpanded( Kit_DsdNtk_t * pNtk );
extern void            Kit_DsdPrintFromTruth( unsigned * pTruth, int nVars );
extern void            Kit_DsdPrintFromTruth2( FILE * pFile, unsigned * pTruth, int nVars );
extern void            Kit_DsdWriteFromTruth( char * pBuffer, unsigned * pTruth, int nVars );
extern Kit_DsdNtk_t *  Kit_DsdDecompose( unsigned * pTruth, int nVars );
extern Kit_DsdNtk_t *  Kit_DsdDecomposeExpand( unsigned * pTruth, int nVars );
extern Kit_DsdNtk_t *  Kit_DsdDecomposeMux( unsigned * pTruth, int nVars, int nDecMux );
extern void            Kit_DsdVerify( Kit_DsdNtk_t * pNtk, unsigned * pTruth, int nVars );
extern void            Kit_DsdNtkFree( Kit_DsdNtk_t * pNtk );
extern int             Kit_DsdNonDsdSizeMax( Kit_DsdNtk_t * pNtk );
extern Kit_DsdObj_t *  Kit_DsdNonDsdPrimeMax( Kit_DsdNtk_t * pNtk );
extern unsigned        Kit_DsdNonDsdSupports( Kit_DsdNtk_t * pNtk );
extern int             Kit_DsdCountAigNodes( Kit_DsdNtk_t * pNtk );
extern unsigned        Kit_DsdGetSupports( Kit_DsdNtk_t * p );
extern Kit_DsdNtk_t *  Kit_DsdExpand( Kit_DsdNtk_t * p );
extern Kit_DsdNtk_t *  Kit_DsdShrink( Kit_DsdNtk_t * p, int pPrios[] );
extern void            Kit_DsdRotate( Kit_DsdNtk_t * p, int pFreqs[] );
extern int             Kit_DsdCofactoring( unsigned * pTruth, int nVars, int * pCofVars, int nLimit, int fVerbose );
/*=== kitFactor.c ==========================================================*/
extern Kit_Graph_t *   Kit_SopFactor( Vec_Int_t * vCover, int fCompl, int nVars, Vec_Int_t * vMemory );
/*=== kitGraph.c ==========================================================*/
extern Kit_Graph_t *   Kit_GraphCreate( int nLeaves );   
extern Kit_Graph_t *   Kit_GraphCreateConst0();   
extern Kit_Graph_t *   Kit_GraphCreateConst1();   
extern Kit_Graph_t *   Kit_GraphCreateLeaf( int iLeaf, int nLeaves, int fCompl );   
extern void            Kit_GraphFree( Kit_Graph_t * pGraph );   
extern Kit_Node_t *    Kit_GraphAppendNode( Kit_Graph_t * pGraph );   
extern Kit_Edge_t      Kit_GraphAddNodeAnd( Kit_Graph_t * pGraph, Kit_Edge_t eEdge0, Kit_Edge_t eEdge1 );
extern Kit_Edge_t      Kit_GraphAddNodeOr( Kit_Graph_t * pGraph, Kit_Edge_t eEdge0, Kit_Edge_t eEdge1 );
extern Kit_Edge_t      Kit_GraphAddNodeXor( Kit_Graph_t * pGraph, Kit_Edge_t eEdge0, Kit_Edge_t eEdge1, int Type );
extern Kit_Edge_t      Kit_GraphAddNodeMux( Kit_Graph_t * pGraph, Kit_Edge_t eEdgeC, Kit_Edge_t eEdgeT, Kit_Edge_t eEdgeE, int Type );
extern unsigned        Kit_GraphToTruth( Kit_Graph_t * pGraph );
extern Kit_Graph_t *   Kit_TruthToGraph( unsigned * pTruth, int nVars, Vec_Int_t * vMemory );
extern int             Kit_GraphLeafDepth_rec( Kit_Graph_t * pGraph, Kit_Node_t * pNode, Kit_Node_t * pLeaf );
/*=== kitHop.c ==========================================================*/
//extern int             Kit_TruthToGia( Gia_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory, Vec_Int_t * vLeaves, int fHash );
//extern Hop_Obj_t *     Kit_GraphToHop( Hop_Man_t * pMan, Kit_Graph_t * pGraph );
//extern Hop_Obj_t *     Kit_TruthToHop( Hop_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory );
//extern Hop_Obj_t *     Kit_CoverToHop( Hop_Man_t * pMan, Vec_Int_t * vCover, int nVars, Vec_Int_t * vMemory );
/*=== kitIsop.c ==========================================================*/
extern int             Kit_TruthIsop( unsigned * puTruth, int nVars, Vec_Int_t * vMemory, int fTryBoth );
extern void            Kit_TruthIsopPrint( unsigned * puTruth, int nVars, Vec_Int_t * vMemory, int fTryBoth );
extern void            Kit_TruthIsopPrintCover( Vec_Int_t * vCover, int nVars, int fCompl );
/*=== kitPla.c ==========================================================*/
extern int             Kit_PlaIsConst0( char * pSop );
extern int             Kit_PlaIsConst1( char * pSop );
extern int             Kit_PlaIsBuf( char * pSop );
extern int             Kit_PlaIsInv( char * pSop );
extern int             Kit_PlaGetVarNum( char * pSop );
extern int             Kit_PlaGetCubeNum( char * pSop );
extern int             Kit_PlaIsComplement( char * pSop );
extern void            Kit_PlaComplement( char * pSop );
extern char *          Kit_PlaStart( void * p, int nCubes, int nVars );
extern char *          Kit_PlaCreateFromIsop( void * p, int nVars, Vec_Int_t * vCover );
extern void            Kit_PlaToIsop( char * pSop, Vec_Int_t * vCover );
extern char *          Kit_PlaStoreSop( void * p, char * pSop );
extern char *          Kit_PlaFromTruth( void * p, unsigned * pTruth, int nVars, Vec_Int_t * vCover );
extern char *          Kit_PlaFromTruthNew( unsigned * pTruth, int nVars, Vec_Int_t * vCover, Vec_Str_t * vStr );
extern ABC_UINT64_T    Kit_PlaToTruth6( char * pSop, int nVars );
extern void            Kit_PlaToTruth( char * pSop, int nVars, Vec_Ptr_t * vVars, unsigned * pTemp, unsigned * pTruth );
/*=== kitSop.c ==========================================================*/
extern void            Kit_SopCreate( Kit_Sop_t * cResult, Vec_Int_t * vInput, int nVars, Vec_Int_t * vMemory );
extern void            Kit_SopCreateInverse( Kit_Sop_t * cResult, Vec_Int_t * vInput, int nVars, Vec_Int_t * vMemory );
extern void            Kit_SopDup( Kit_Sop_t * cResult, Kit_Sop_t * cSop, Vec_Int_t * vMemory );
extern void            Kit_SopDivideByLiteralQuo( Kit_Sop_t * cSop, int iLit );
extern void            Kit_SopDivideByCube( Kit_Sop_t * cSop, Kit_Sop_t * cDiv, Kit_Sop_t * vQuo, Kit_Sop_t * vRem, Vec_Int_t * vMemory );
extern void            Kit_SopDivideInternal( Kit_Sop_t * cSop, Kit_Sop_t * cDiv, Kit_Sop_t * vQuo, Kit_Sop_t * vRem, Vec_Int_t * vMemory );
extern void            Kit_SopMakeCubeFree( Kit_Sop_t * cSop );
extern int             Kit_SopIsCubeFree( Kit_Sop_t * cSop );
extern void            Kit_SopCommonCubeCover( Kit_Sop_t * cResult, Kit_Sop_t * cSop, Vec_Int_t * vMemory );
extern int             Kit_SopAnyLiteral( Kit_Sop_t * cSop, int nLits );
extern int             Kit_SopDivisor( Kit_Sop_t * cResult, Kit_Sop_t * cSop, int nLits, Vec_Int_t * vMemory );
extern void            Kit_SopBestLiteralCover( Kit_Sop_t * cResult, Kit_Sop_t * cSop, unsigned uCube, int nLits, Vec_Int_t * vMemory );
/*=== kitTruth.c ==========================================================*/
extern void            Kit_TruthSwapAdjacentVars( unsigned * pOut, unsigned * pIn, int nVars, int Start );
extern void            Kit_TruthStretch( unsigned * pOut, unsigned * pIn, int nVars, int nVarsAll, unsigned Phase, int fReturnIn );
extern void            Kit_TruthPermute( unsigned * pOut, unsigned * pIn, int nVars, char * pPerm, int fReturnIn );
extern void            Kit_TruthShrink( unsigned * pOut, unsigned * pIn, int nVars, int nVarsAll, unsigned Phase, int fReturnIn );
extern int             Kit_TruthVarInSupport( unsigned * pTruth, int nVars, int iVar );
extern int             Kit_TruthSupportSize( unsigned * pTruth, int nVars );
extern unsigned        Kit_TruthSupport( unsigned * pTruth, int nVars );
extern void            Kit_TruthCofactor0( unsigned * pTruth, int nVars, int iVar );
extern void            Kit_TruthCofactor1( unsigned * pTruth, int nVars, int iVar );
extern void            Kit_TruthCofactor0New( unsigned * pOut, unsigned * pIn, int nVars, int iVar );
extern void            Kit_TruthCofactor1New( unsigned * pOut, unsigned * pIn, int nVars, int iVar );
extern int             Kit_TruthVarIsVacuous( unsigned * pOnset, unsigned * pOffset, int nVars, int iVar );
extern void            Kit_TruthExist( unsigned * pTruth, int nVars, int iVar );
extern void            Kit_TruthExistNew( unsigned * pRes, unsigned * pTruth, int nVars, int iVar );
extern void            Kit_TruthExistSet( unsigned * pRes, unsigned * pTruth, int nVars, unsigned uMask );
extern void            Kit_TruthForall( unsigned * pTruth, int nVars, int iVar );
extern void            Kit_TruthForallNew( unsigned * pRes, unsigned * pTruth, int nVars, int iVar );
extern void            Kit_TruthForallSet( unsigned * pRes, unsigned * pTruth, int nVars, unsigned uMask );
extern void            Kit_TruthUniqueNew( unsigned * pRes, unsigned * pTruth, int nVars, int iVar );
extern void            Kit_TruthMuxVar( unsigned * pOut, unsigned * pCof0, unsigned * pCof1, int nVars, int iVar );
extern void            Kit_TruthMuxVarPhase( unsigned * pOut, unsigned * pCof0, unsigned * pCof1, int nVars, int iVar, int fCompl0 );
extern void            Kit_TruthChangePhase( unsigned * pTruth, int nVars, int iVar );
extern int             Kit_TruthVarsSymm( unsigned * pTruth, int nVars, int iVar0, int iVar1, unsigned * pCof0, unsigned * pCof1 );
extern int             Kit_TruthVarsAntiSymm( unsigned * pTruth, int nVars, int iVar0, int iVar1, unsigned * pCof0, unsigned * pCof1 );
extern int             Kit_TruthMinCofSuppOverlap( unsigned * pTruth, int nVars, int * pVarMin );
extern int             Kit_TruthBestCofVar( unsigned * pTruth, int nVars, unsigned * pCof0, unsigned * pCof1 );
extern void            Kit_TruthCountOnesInCofs( unsigned * pTruth, int nVars, int * pStore );
extern void            Kit_TruthCountOnesInCofs0( unsigned * pTruth, int nVars, int * pStore );
extern void            Kit_TruthCountOnesInCofsSlow( unsigned * pTruth, int nVars, int * pStore, unsigned * pAux );
extern unsigned        Kit_TruthHash( unsigned * pIn, int nWords );
extern unsigned        Kit_TruthSemiCanonicize( unsigned * pInOut, unsigned * pAux, int nVars, char * pCanonPerm );
extern char *          Kit_TruthDumpToFile( unsigned * pTruth, int nVars, int nFile );
extern void            Kit_TruthPrintProfile( unsigned * pTruth, int nVars );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

