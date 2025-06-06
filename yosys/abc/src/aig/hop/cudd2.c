/**CFile****************************************************************

  FileName    [cudd2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic And-Inverter Graph package.]

  Synopsis    [Recording AIGs for the BDD operations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 3, 2006.]

  Revision    [$Id: cudd2.c,v 1.00 2006/10/03 00:00:00 alanmi Exp $]

***********************************************************************/

#include "hop.h"
#include "misc/st/st.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Aig_CuddMan_t_        Aig_CuddMan_t;
struct Aig_CuddMan_t_
{
    Aig_Man_t *  pAig;   // internal AIG package
    st__table *   pTable; // hash table mapping BDD nodes into AIG nodes
};

// static Cudd AIG manager used in this experiment
static Aig_CuddMan_t * s_pCuddMan = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Start AIG recording.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_Init( unsigned int numVars, unsigned int numVarsZ, unsigned int numSlots, unsigned int cacheSize, unsigned long maxMemory, void * pCudd )
{
    int v;
    // start the BDD-to-AIG manager when the first BDD manager is allocated
    if ( s_pCuddMan != NULL )
        return;
    s_pCuddMan = ALLOC( Aig_CuddMan_t, 1 );
    s_pCuddMan->pAig = Aig_ManStart();
    s_pCuddMan->pTable = st__init_table( st__ptrcmp, st__ptrhash );
    for ( v = 0; v < (int)numVars; v++ )
        Aig_ObjCreatePi( s_pCuddMan->pAig );
}

/**Function*************************************************************

  Synopsis    [Stops AIG recording.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_Quit( void * pCudd )
{
    assert( s_pCuddMan != NULL );
    Aig_ManDumpBlif( s_pCuddMan->pAig, "aig_temp.blif", NULL, NULL );
    Aig_ManStop( s_pCuddMan->pAig );
    st__free_table( s_pCuddMan->pTable );
    free( s_pCuddMan );
    s_pCuddMan = NULL;
}

/**Function*************************************************************

  Synopsis    [Fetches AIG node corresponding to the BDD node from the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Aig_Obj_t * Cudd2_GetArg( void * pArg )
{
    Aig_Obj_t * pNode;
    assert( s_pCuddMan != NULL );
    if ( ! st__lookup( s_pCuddMan->pTable, (char *)Aig_Regular(pArg), (char **)&pNode ) )
    {
        printf( "Cudd2_GetArg(): An argument BDD is not in the hash table.\n" );
        return NULL;
    }
    return Aig_NotCond( pNode, Aig_IsComplement(pArg) );
}

/**Function*************************************************************

  Synopsis    [Inserts the AIG node corresponding to the BDD node into the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Cudd2_SetArg( Aig_Obj_t * pNode, void * pResult )
{
    assert( s_pCuddMan != NULL );
    if ( st__is_member( s_pCuddMan->pTable, (char *)Aig_Regular(pResult) ) )
        return;
    pNode = Aig_NotCond( pNode,  Aig_IsComplement(pResult) );
    st__insert( s_pCuddMan->pTable, (char *)Aig_Regular(pResult), (char *)pNode );
}

/**Function*************************************************************

  Synopsis    [Registers constant 1 node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_bddOne( void * pCudd, void * pResult )
{
    Cudd2_SetArg( Aig_ManConst1(s_pCuddMan->pAig), pResult );
}

/**Function*************************************************************

  Synopsis    [Adds elementary variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_bddIthVar( void * pCudd, int iVar, void * pResult )
{
    int v;
    assert( s_pCuddMan != NULL );
    for ( v = Aig_ManPiNum(s_pCuddMan->pAig); v <= iVar; v++ )
        Aig_ObjCreatePi( s_pCuddMan->pAig );
    Cudd2_SetArg( Aig_ManPi(s_pCuddMan->pAig, iVar), pResult );
}

/**Function*************************************************************

  Synopsis    [Performs BDD operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_bddAnd( void * pCudd, void * pArg0, void * pArg1, void * pResult )
{
    Aig_Obj_t * pNode0, * pNode1, * pNode;
    pNode0 = Cudd2_GetArg( pArg0 );
    pNode1 = Cudd2_GetArg( pArg1 );
    pNode  = Aig_And( s_pCuddMan->pAig, pNode0, pNode1 );
    Cudd2_SetArg( pNode, pResult );
}

/**Function*************************************************************

  Synopsis    [Performs BDD operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_bddOr( void * pCudd, void * pArg0, void * pArg1, void * pResult )
{
    Cudd2_bddAnd( pCudd, Aig_Not(pArg0), Aig_Not(pArg1), Aig_Not(pResult) );
}

/**Function*************************************************************

  Synopsis    [Performs BDD operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_bddNand( void * pCudd, void * pArg0, void * pArg1, void * pResult )
{
    Cudd2_bddAnd( pCudd, pArg0, pArg1, Aig_Not(pResult) );
}

/**Function*************************************************************

  Synopsis    [Performs BDD operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_bddNor( void * pCudd, void * pArg0, void * pArg1, void * pResult )
{
    Cudd2_bddAnd( pCudd, Aig_Not(pArg0), Aig_Not(pArg1), pResult );
}

/**Function*************************************************************

  Synopsis    [Performs BDD operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_bddXor( void * pCudd, void * pArg0, void * pArg1, void * pResult )
{
    Aig_Obj_t * pNode0, * pNode1, * pNode;
    pNode0 = Cudd2_GetArg( pArg0 );
    pNode1 = Cudd2_GetArg( pArg1 );
    pNode  = Aig_Exor( s_pCuddMan->pAig, pNode0, pNode1 );
    Cudd2_SetArg( pNode, pResult );
}

/**Function*************************************************************

  Synopsis    [Performs BDD operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_bddXnor( void * pCudd, void * pArg0, void * pArg1, void * pResult )
{
    Cudd2_bddXor( pCudd, pArg0, pArg1, Aig_Not(pResult) );
}

/**Function*************************************************************

  Synopsis    [Performs BDD operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_bddIte( void * pCudd, void * pArg0, void * pArg1, void * pArg2, void * pResult )
{
    Aig_Obj_t * pNode0, * pNode1, * pNode2, * pNode;
    pNode0 = Cudd2_GetArg( pArg0 );
    pNode1 = Cudd2_GetArg( pArg1 );
    pNode2 = Cudd2_GetArg( pArg2 );
    pNode  = Aig_Mux( s_pCuddMan->pAig, pNode0, pNode1, pNode2 );
    Cudd2_SetArg( pNode, pResult );
}

/**Function*************************************************************

  Synopsis    [Performs BDD operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_bddCompose( void * pCudd, void * pArg0, void * pArg1, int v, void * pResult )
{
    Aig_Obj_t * pNode0, * pNode1, * pNode;
    pNode0 = Cudd2_GetArg( pArg0 );
    pNode1 = Cudd2_GetArg( pArg1 );
    pNode  = Aig_Compose( s_pCuddMan->pAig, pNode0, pNode1, v );
    Cudd2_SetArg( pNode, pResult );
}

/**Function*************************************************************

  Synopsis    [Should be called after each containment check.]

  Description [Result should be 1 if Cudd2_bddLeq returned 1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_bddLeq( void * pCudd, void * pArg0, void * pArg1, int Result )
{
    Aig_Obj_t * pNode0, * pNode1, * pNode;
    pNode0 = Cudd2_GetArg( pArg0 );
    pNode1 = Cudd2_GetArg( pArg1 );
    pNode  = Aig_And( s_pCuddMan->pAig, pNode0, Aig_Not(pNode1) );
    Aig_ObjCreatePo( s_pCuddMan->pAig, pNode );
}

/**Function*************************************************************

  Synopsis    [Should be called after each equality check.]

  Description [Result should be 1 if they are equal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cudd2_bddEqual( void * pCudd, void * pArg0, void * pArg1, int Result )
{
    Aig_Obj_t * pNode0, * pNode1, * pNode;
    pNode0 = Cudd2_GetArg( pArg0 );
    pNode1 = Cudd2_GetArg( pArg1 );
    pNode  = Aig_Exor( s_pCuddMan->pAig, pNode0, pNode1 );
    Aig_ObjCreatePo( s_pCuddMan->pAig, pNode );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

