/**CFile****************************************************************

  FileName    [dsd.h]

  PackageName [DSD: Disjoint-support decomposition package.]

  Synopsis    [External declarations of the package.
  This fast BDD-based recursive algorithm for simple 
  (single-output) DSD is based on the following papers:
  (1) V. Bertacco and M. Damiani, "Disjunctive decomposition of 
  logic functions," Proc. ICCAD '97, pp. 78-82.
  (2) Y. Matsunaga, "An exact and efficient algorithm for disjunctive 
  decomposition", Proc. SASIMI '98, pp. 44-50.
  The scope of detected decompositions is the same as in the paper:
  T. Sasao and M. Matsuura, "DECOMPOS: An integrated system for 
  functional decomposition," Proc. IWLS '98, pp. 471-477.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 8.0. Started - September 22, 2003.]

  Revision    [$Id: dsd.h,v 1.0 2002/22/09 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__bdd__dsd__dsd_h
#define ABC__bdd__dsd__dsd_h


////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


// types of DSD nodes
enum Dsd_Type_t_ { 
    DSD_NODE_NONE   = 0,
    DSD_NODE_CONST1 = 1,
    DSD_NODE_BUF    = 2,
    DSD_NODE_OR     = 3,
    DSD_NODE_EXOR   = 4,
    DSD_NODE_PRIME  = 5,
};

////////////////////////////////////////////////////////////////////////
///                      TYPEDEF DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Dsd_Manager_t_   Dsd_Manager_t;
typedef struct Dsd_Node_t_      Dsd_Node_t;
typedef enum   Dsd_Type_t_      Dsd_Type_t;

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

// complementation and testing for pointers for decomposition entries
#define Dsd_IsComplement(p)  (((int)((ABC_PTRUINT_T) (p) & 01)))
#define Dsd_Regular(p)       ((Dsd_Node_t *)((ABC_PTRUINT_T)(p) & ~01)) 
#define Dsd_Not(p)           ((Dsd_Node_t *)((ABC_PTRUINT_T)(p) ^ 01)) 
#define Dsd_NotCond(p,c)     ((Dsd_Node_t *)((ABC_PTRUINT_T)(p) ^ (c)))

////////////////////////////////////////////////////////////////////////
///                         ITERATORS                                ///
////////////////////////////////////////////////////////////////////////

// iterator through the transitions
#define Dsd_NodeForEachChild( Node, Index, Child )        \
    for ( Index = 0;                                      \
          Index < Dsd_NodeReadDecsNum(Node) &&            \
             ((Child = Dsd_NodeReadDec(Node,Index))!=0);  \
          Index++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== dsdApi.c =======================================================*/
extern Dsd_Type_t      Dsd_NodeReadType( Dsd_Node_t * p ); 
extern DdNode *        Dsd_NodeReadFunc( Dsd_Node_t * p );
extern DdNode *        Dsd_NodeReadSupp( Dsd_Node_t * p );
extern Dsd_Node_t **   Dsd_NodeReadDecs( Dsd_Node_t * p );
extern Dsd_Node_t *    Dsd_NodeReadDec ( Dsd_Node_t * p, int i );
extern int             Dsd_NodeReadDecsNum( Dsd_Node_t * p );
extern word            Dsd_NodeReadMark( Dsd_Node_t * p );
extern void            Dsd_NodeSetMark( Dsd_Node_t * p, word Mark ); 
extern DdManager *     Dsd_ManagerReadDd( Dsd_Manager_t * pMan );
extern Dsd_Node_t *    Dsd_ManagerReadRoot( Dsd_Manager_t * pMan, int i );
extern Dsd_Node_t *    Dsd_ManagerReadInput( Dsd_Manager_t * pMan, int i );
extern Dsd_Node_t *    Dsd_ManagerReadConst1( Dsd_Manager_t * pMan );
/*=== dsdMan.c =======================================================*/
extern Dsd_Manager_t * Dsd_ManagerStart( DdManager * dd, int nSuppMax, int fVerbose );
extern void            Dsd_ManagerStop( Dsd_Manager_t * dMan );
/*=== dsdProc.c =======================================================*/
extern void            Dsd_Decompose( Dsd_Manager_t * dMan, DdNode ** pbFuncs, int nFuncs );
extern Dsd_Node_t *    Dsd_DecomposeOne( Dsd_Manager_t * pDsdMan, DdNode * bFunc );
/*=== dsdTree.c =======================================================*/
extern void            Dsd_TreeNodeGetInfo( Dsd_Manager_t * dMan, int * DepthMax, int * GateSizeMax );
extern void            Dsd_TreeNodeGetInfoOne( Dsd_Node_t * pNode, int * DepthMax, int * GateSizeMax );
extern int             Dsd_TreeGetAigCost( Dsd_Node_t * pNode );
extern int             Dsd_TreeCountNonTerminalNodes( Dsd_Manager_t * dMan );
extern int             Dsd_TreeCountNonTerminalNodesOne( Dsd_Node_t * pRoot );
extern int             Dsd_TreeCountPrimeNodes( Dsd_Manager_t * pDsdMan );
extern int             Dsd_TreeCountPrimeNodesOne( Dsd_Node_t * pRoot );
extern int             Dsd_TreeCollectDecomposableVars( Dsd_Manager_t * dMan, int * pVars );
extern Dsd_Node_t **   Dsd_TreeCollectNodesDfs( Dsd_Manager_t * dMan, int * pnNodes );
extern Dsd_Node_t **   Dsd_TreeCollectNodesDfsOne( Dsd_Manager_t * pDsdMan, Dsd_Node_t * pNode, int * pnNodes );
extern void            Dsd_TreePrint( FILE * pFile, Dsd_Manager_t * dMan, char * pInputNames[], char * pOutputNames[], int fShortNames, int Output );
extern void            Dsd_TreePrint2( FILE * pFile, Dsd_Manager_t * dMan, char * pInputNames[], char * pOutputNames[], int Output );
extern void            Dsd_NodePrint( FILE * pFile, Dsd_Node_t * pNode );
/*=== dsdLocal.c =======================================================*/
extern DdNode *        Dsd_TreeGetPrimeFunction( DdManager * dd, Dsd_Node_t * pNode );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                           END OF FILE                            ///
////////////////////////////////////////////////////////////////////////
