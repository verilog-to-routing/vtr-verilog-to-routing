/**CFile****************************************************************

  FileName    [cnfMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG-to-CNF conversion.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: cnfMan.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cnf.h"
#include "satSolver.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int Cnf_Lit2Var( int Lit )        { return (Lit & 1)? -(Lit >> 1)-1 : (Lit >> 1)+1;  }
static inline int Cnf_Lit2Var2( int Lit )       { return (Lit & 1)? -(Lit >> 1)   : (Lit >> 1);    }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cnf_Man_t * Cnf_ManStart()
{
    Cnf_Man_t * p;
    int i;
    // allocate the manager
    p = ALLOC( Cnf_Man_t, 1 );
    memset( p, 0, sizeof(Cnf_Man_t) );
    // derive internal data structures
    Cnf_ReadMsops( &p->pSopSizes, &p->pSops );
    // allocate memory manager for cuts
    p->pMemCuts = Aig_MmFlexStart();
    p->nMergeLimit = 10;
    // allocate temporary truth tables
    p->pTruths[0] = ALLOC( unsigned, 4 * Aig_TruthWordNum(p->nMergeLimit) );
    for ( i = 1; i < 4; i++ )
        p->pTruths[i] = p->pTruths[i-1] + Aig_TruthWordNum(p->nMergeLimit);
    p->vMemory = Vec_IntAlloc( 1 << 18 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_ManStop( Cnf_Man_t * p )
{
    Vec_IntFree( p->vMemory );
    free( p->pTruths[0] );
    Aig_MmFlexStop( p->pMemCuts, 0 );
    free( p->pSopSizes );
    free( p->pSops[1] );
    free( p->pSops );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Returns the array of CI IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Cnf_DataCollectPiSatNums( Cnf_Dat_t * pCnf, Aig_Man_t * p )
{
    Vec_Int_t * vCiIds;
    Aig_Obj_t * pObj;
    int i;
    vCiIds = Vec_IntAlloc( Aig_ManPiNum(p) );
    Aig_ManForEachPi( p, pObj, i )
        Vec_IntPush( vCiIds, pCnf->pVarNums[pObj->Id] );
    return vCiIds;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_DataFree( Cnf_Dat_t * p )
{
    if ( p == NULL )
        return;
    free( p->pClauses[0] );
    free( p->pClauses );
    free( p->pVarNums );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Writes CNF into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_DataWriteIntoFile( Cnf_Dat_t * p, char * pFileName, int fReadable )
{
    FILE * pFile;
    int * pLit, * pStop, i;
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Cnf_WriteIntoFile(): Output file cannot be opened.\n" );
        return;
    }
    fprintf( pFile, "c Result of efficient AIG-to-CNF conversion using package CNF\n" );
    fprintf( pFile, "p %d %d\n", p->nVars, p->nClauses );
    for ( i = 0; i < p->nClauses; i++ )
    {
        for ( pLit = p->pClauses[i], pStop = p->pClauses[i+1]; pLit < pStop; pLit++ )
            fprintf( pFile, "%d ", fReadable? Cnf_Lit2Var2(*pLit) : Cnf_Lit2Var(*pLit) );
        fprintf( pFile, "0\n" );
    }
    fprintf( pFile, "\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Writes CNF into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Cnf_DataWriteIntoSolver( Cnf_Dat_t * p )
{
    sat_solver * pSat;
    int i, status;
    pSat = sat_solver_new();
    sat_solver_setnvars( pSat, p->nVars );
    for ( i = 0; i < p->nClauses; i++ )
    {
        if ( !sat_solver_addclause( pSat, p->pClauses[i], p->pClauses[i+1] ) )
        {
            sat_solver_delete( pSat );
            return NULL;
        }
    }
    status = sat_solver_simplify(pSat);
    if ( status == 0 )
    {
        sat_solver_delete( pSat );
        return NULL;
    }
    return pSat;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


