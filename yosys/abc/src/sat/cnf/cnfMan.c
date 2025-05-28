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
#include "sat/bsat/satSolver.h"
#include "sat/bsat/satSolver2.h"
#include "misc/zlib/zlib.h"

ABC_NAMESPACE_IMPL_START


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
    p = ABC_ALLOC( Cnf_Man_t, 1 );
    memset( p, 0, sizeof(Cnf_Man_t) );
    // derive internal data structures
    Cnf_ReadMsops( &p->pSopSizes, &p->pSops );
    // allocate memory manager for cuts
    p->pMemCuts = Aig_MmFlexStart();
    p->nMergeLimit = 10;
    // allocate temporary truth tables
    p->pTruths[0] = ABC_ALLOC( unsigned, 4 * Abc_TruthWordNum(p->nMergeLimit) );
    for ( i = 1; i < 4; i++ )
        p->pTruths[i] = p->pTruths[i-1] + Abc_TruthWordNum(p->nMergeLimit);
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
    ABC_FREE( p->pTruths[0] );
    Aig_MmFlexStop( p->pMemCuts, 0 );
    ABC_FREE( p->pSopSizes );
    ABC_FREE( p->pSops[1] );
    ABC_FREE( p->pSops );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns the array of CI IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Cnf_DataCollectPiSatNums( Cnf_Dat_t * pCnf, Aig_Man_t * p )
{
    Aig_Obj_t * pObj; int i;
    Vec_Int_t * vCiIds = Vec_IntAlloc( Aig_ManCiNum(p) );
    Aig_ManForEachCi( p, pObj, i )
        Vec_IntPush( vCiIds, pCnf->pVarNums[pObj->Id] );
    return vCiIds;
}

/**Function*************************************************************

  Synopsis    [Allocates the new CNF.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cnf_Dat_t * Cnf_DataAlloc( Aig_Man_t * pAig, int nVars, int nClauses, int nLiterals )
{
    Cnf_Dat_t * pCnf;
    pCnf = ABC_CALLOC( Cnf_Dat_t, 1 );
    pCnf->pMan = pAig;
    pCnf->nVars = nVars;
    pCnf->nClauses = nClauses;
    pCnf->nLiterals = nLiterals;
    pCnf->pClauses = ABC_ALLOC( int *, nClauses + 1 );
    pCnf->pClauses[0] = ABC_ALLOC( int, nLiterals );
    pCnf->pClauses[nClauses] = pCnf->pClauses[0] + nLiterals;
    if ( pCnf->pVarNums )
    pCnf->pVarNums = ABC_FALLOC( int, Aig_ManObjNumMax(pAig) );
    return pCnf;
}

/**Function*************************************************************

  Synopsis    [Allocates the new CNF.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cnf_Dat_t * Cnf_DataDup( Cnf_Dat_t * p )
{
    Cnf_Dat_t * pCnf;
    int i;
    pCnf = Cnf_DataAlloc( p->pMan, p->nVars, p->nClauses, p->nLiterals );
    memcpy( pCnf->pClauses[0], p->pClauses[0], sizeof(int) * p->nLiterals );
    if ( p->pVarNums )
    memcpy( pCnf->pVarNums, p->pVarNums, sizeof(int) * Aig_ManObjNumMax(p->pMan) );
    for ( i = 1; i < p->nClauses; i++ )
        pCnf->pClauses[i] = pCnf->pClauses[0] + (p->pClauses[i] - p->pClauses[0]);
    return pCnf;
}
Cnf_Dat_t * Cnf_DataDupCof( Cnf_Dat_t * p, int Lit )
{
    Cnf_Dat_t * pCnf;
    int i;
    pCnf = Cnf_DataAlloc( p->pMan, p->nVars, p->nClauses+1, p->nLiterals+1 );
    memcpy( pCnf->pClauses[0], p->pClauses[0], sizeof(int) * p->nLiterals );
    if ( pCnf->pVarNums )
    memcpy( pCnf->pVarNums, p->pVarNums, sizeof(int) * Aig_ManObjNumMax(p->pMan) );
    for ( i = 1; i < p->nClauses; i++ )
        pCnf->pClauses[i] = pCnf->pClauses[0] + (p->pClauses[i] - p->pClauses[0]);
    pCnf->pClauses[p->nClauses] = pCnf->pClauses[0] + p->nLiterals;
    pCnf->pClauses[p->nClauses][0] = Lit;
    assert( pCnf->pClauses[p->nClauses+1] == pCnf->pClauses[0] + p->nLiterals+1 );
    return pCnf;
}
Cnf_Dat_t * Cnf_DataDupCofArray( Cnf_Dat_t * p, Vec_Int_t * vLits )
{
    Cnf_Dat_t * pCnf;
    int i, iLit;
    pCnf = Cnf_DataAlloc( p->pMan, p->nVars, p->nClauses+Vec_IntSize(vLits), p->nLiterals+Vec_IntSize(vLits) );
    memcpy( pCnf->pClauses[0], p->pClauses[0], sizeof(int) * p->nLiterals );
    if ( pCnf->pVarNums )
    memcpy( pCnf->pVarNums, p->pVarNums, sizeof(int) * Aig_ManObjNumMax(p->pMan) );
    for ( i = 1; i < p->nClauses; i++ )
        pCnf->pClauses[i] = pCnf->pClauses[0] + (p->pClauses[i] - p->pClauses[0]);
    Vec_IntForEachEntry( vLits, iLit, i ) {
        pCnf->pClauses[p->nClauses+i] = pCnf->pClauses[0] + p->nLiterals+i;
        pCnf->pClauses[p->nClauses+i][0] = iLit;
    }
    assert( pCnf->pClauses[p->nClauses+Vec_IntSize(vLits)] == pCnf->pClauses[0] + p->nLiterals+Vec_IntSize(vLits) );
    return pCnf;
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
    Vec_IntFreeP( &p->vMapping );
    ABC_FREE( p->pClaPols );
    ABC_FREE( p->pObj2Clause );
    ABC_FREE( p->pObj2Count );
    ABC_FREE( p->pClauses[0] );
    ABC_FREE( p->pClauses );
    ABC_FREE( p->pVarNums );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_DataLift( Cnf_Dat_t * p, int nVarsPlus )
{
    Aig_Obj_t * pObj;
    int v;
    if ( p->pMan )
    {
        Aig_ManForEachObj( p->pMan, pObj, v )
            if ( p->pVarNums[pObj->Id] >= 0 )
                p->pVarNums[pObj->Id] += nVarsPlus;
    }
    for ( v = 0; v < p->nLiterals; v++ )
        p->pClauses[0][v] += 2*nVarsPlus;
}
void Cnf_DataCollectFlipLits( Cnf_Dat_t * p, int iFlipVar, Vec_Int_t * vFlips )
{
    int v;
    assert( p->pMan == NULL );
    Vec_IntClear( vFlips );
    for ( v = 0; v < p->nLiterals; v++ )
        if ( Abc_Lit2Var(p->pClauses[0][v]) == iFlipVar )
            Vec_IntPush( vFlips, v );
}
void Cnf_DataLiftAndFlipLits( Cnf_Dat_t * p, int nVarsPlus, Vec_Int_t * vLits )
{
    int i, iLit;
    assert( p->pMan == NULL );
    Vec_IntForEachEntry( vLits, iLit, i )
        p->pClauses[0][iLit] = Abc_LitNot(p->pClauses[0][iLit]) + 2*nVarsPlus;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_DataPrint( Cnf_Dat_t * p, int fReadable )
{
    FILE * pFile = stdout;
    int * pLit, * pStop, i;
    fprintf( pFile, "p cnf %d %d\n", p->nVars, p->nClauses );
    for ( i = 0; i < p->nClauses; i++ )
    {
        for ( pLit = p->pClauses[i], pStop = p->pClauses[i+1]; pLit < pStop; pLit++ )
            fprintf( pFile, "%s%d ", Abc_LitIsCompl(*pLit) ? "-":"", fReadable? Abc_Lit2Var(*pLit) : Abc_Lit2Var(*pLit)+1 );
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Writes CNF into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_DataWriteIntoFileGz( Cnf_Dat_t * p, char * pFileName, int fReadable, Vec_Int_t * vForAlls, Vec_Int_t * vExists )
{
    gzFile pFile;
    int * pLit, * pStop, i, VarId;
    pFile = gzopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cnf_WriteIntoFile(): Output file cannot be opened.\n" );
        return;
    }
    gzprintf( pFile, "c Result of efficient AIG-to-CNF conversion using package CNF\n" );
    gzprintf( pFile, "p cnf %d %d\n", p->nVars, p->nClauses );
    if ( vForAlls )
    {
        gzprintf( pFile, "a " );
        Vec_IntForEachEntry( vForAlls, VarId, i )
            gzprintf( pFile, "%d ", fReadable? VarId : VarId+1 );
        gzprintf( pFile, "0\n" );
    }
    if ( vExists )
    {
        gzprintf( pFile, "e " );
        Vec_IntForEachEntry( vExists, VarId, i )
            gzprintf( pFile, "%d ", fReadable? VarId : VarId+1 );
        gzprintf( pFile, "0\n" );
    }
    for ( i = 0; i < p->nClauses; i++ )
    {
        for ( pLit = p->pClauses[i], pStop = p->pClauses[i+1]; pLit < pStop; pLit++ )
            gzprintf( pFile, "%d ", fReadable? Cnf_Lit2Var2(*pLit) : Cnf_Lit2Var(*pLit) );
        gzprintf( pFile, "0\n" );
    }
    gzprintf( pFile, "\n" );
    gzclose( pFile );
}
void Cnf_DataWriteIntoFileInvGz( Cnf_Dat_t * p, char * pFileName, int fReadable, Vec_Int_t * vExists1, Vec_Int_t * vForAlls, Vec_Int_t * vExists2 )
{
    gzFile pFile;
    int * pLit, * pStop, i, VarId;
    pFile = gzopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cnf_WriteIntoFile(): Output file cannot be opened.\n" );
        return;
    }
    gzprintf( pFile, "c Result of efficient AIG-to-CNF conversion using package CNF\n" );
    gzprintf( pFile, "p cnf %d %d\n", p->nVars, p->nClauses );
    if ( vExists1 )
    {
        gzprintf( pFile, "e " );
        Vec_IntForEachEntry( vExists1, VarId, i )
            gzprintf( pFile, "%d ", fReadable? VarId : VarId+1 );
        gzprintf( pFile, "0\n" );
    }
    if ( vForAlls )
    {
        gzprintf( pFile, "a " );
        Vec_IntForEachEntry( vForAlls, VarId, i )
            gzprintf( pFile, "%d ", fReadable? VarId : VarId+1 );
        gzprintf( pFile, "0\n" );
    }
    if ( vExists2 )
    {
        gzprintf( pFile, "e " );
        Vec_IntForEachEntry( vExists2, VarId, i )
            gzprintf( pFile, "%d ", fReadable? VarId : VarId+1 );
        gzprintf( pFile, "0\n" );
    }
    for ( i = 0; i < p->nClauses; i++ )
    {
        for ( pLit = p->pClauses[i], pStop = p->pClauses[i+1]; pLit < pStop; pLit++ )
            gzprintf( pFile, "%d ", fReadable? Cnf_Lit2Var2(*pLit) : Cnf_Lit2Var(*pLit) );
        gzprintf( pFile, "0\n" );
    }
    gzprintf( pFile, "\n" );
    gzclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Writes CNF into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_DataWriteIntoFile( Cnf_Dat_t * p, char * pFileName, int fReadable, Vec_Int_t * vForAlls, Vec_Int_t * vExists )
{
    FILE * pFile;
    int * pLit, * pStop, i, VarId;
    if ( !strncmp(pFileName+strlen(pFileName)-3,".gz",3) ) 
    {
        Cnf_DataWriteIntoFileGz( p, pFileName, fReadable, vForAlls, vExists );
        return;
    }
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Cnf_WriteIntoFile(): Output file cannot be opened.\n" );
        return;
    }
    fprintf( pFile, "c Result of efficient AIG-to-CNF conversion using package CNF\n" );
    fprintf( pFile, "p cnf %d %d\n", p->nVars, p->nClauses );
    if ( vForAlls )
    {
        fprintf( pFile, "a " );
        Vec_IntForEachEntry( vForAlls, VarId, i )
            fprintf( pFile, "%d ", fReadable? VarId : VarId+1 );
        fprintf( pFile, "0\n" );
    }
    if ( vExists )
    {
        fprintf( pFile, "e " );
        Vec_IntForEachEntry( vExists, VarId, i )
            fprintf( pFile, "%d ", fReadable? VarId : VarId+1 );
        fprintf( pFile, "0\n" );
    }
    for ( i = 0; i < p->nClauses; i++ )
    {
        for ( pLit = p->pClauses[i], pStop = p->pClauses[i+1]; pLit < pStop; pLit++ )
            fprintf( pFile, "%d ", fReadable? Cnf_Lit2Var2(*pLit) : Cnf_Lit2Var(*pLit) );
        fprintf( pFile, "0\n" );
    }
    fprintf( pFile, "\n" );
    fclose( pFile );
}
void Cnf_DataWriteIntoFileInv( Cnf_Dat_t * p, char * pFileName, int fReadable, Vec_Int_t * vExists1, Vec_Int_t * vForAlls, Vec_Int_t * vExists2 )
{
    FILE * pFile;
    int * pLit, * pStop, i, VarId;
    if ( !strncmp(pFileName+strlen(pFileName)-3,".gz",3) ) 
    {
        Cnf_DataWriteIntoFileInvGz( p, pFileName, fReadable, vExists1, vForAlls, vExists2 );
        return;
    }
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Cnf_WriteIntoFile(): Output file cannot be opened.\n" );
        return;
    }
    fprintf( pFile, "c Result of efficient AIG-to-CNF conversion using package CNF\n" );
    fprintf( pFile, "p cnf %d %d\n", p->nVars, p->nClauses );
    if ( vExists1 )
    {
        fprintf( pFile, "e " );
        Vec_IntForEachEntry( vExists1, VarId, i )
            fprintf( pFile, "%d ", fReadable? VarId : VarId+1 );
        fprintf( pFile, "0\n" );
    }
    if ( vForAlls )
    {
        fprintf( pFile, "a " );
        Vec_IntForEachEntry( vForAlls, VarId, i )
            fprintf( pFile, "%d ", fReadable? VarId : VarId+1 );
        fprintf( pFile, "0\n" );
    }
    if ( vExists2 )
    {
        fprintf( pFile, "e " );
        Vec_IntForEachEntry( vExists2, VarId, i )
            fprintf( pFile, "%d ", fReadable? VarId : VarId+1 );
        fprintf( pFile, "0\n" );
    }
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
void * Cnf_DataWriteIntoSolverInt( void * pSolver, Cnf_Dat_t * p, int nFrames, int fInit )
{
    sat_solver * pSat = (sat_solver *)pSolver;
    int i, f, status;
    assert( nFrames > 0 );
    assert( pSat );
//    pSat = sat_solver_new();
    sat_solver_setnvars( pSat, p->nVars * nFrames );
    for ( i = 0; i < p->nClauses; i++ )
    {
        if ( !sat_solver_addclause( pSat, p->pClauses[i], p->pClauses[i+1] ) )
        {
            sat_solver_delete( pSat );
            return NULL;
        }
    }
    if ( nFrames > 1 )
    {
        Aig_Obj_t * pObjLo, * pObjLi;
        int nLitsAll, * pLits, Lits[2];
        nLitsAll = 2 * p->nVars;
        pLits = p->pClauses[0];
        for ( f = 1; f < nFrames; f++ )
        {
            // add equality of register inputs/outputs for different timeframes
            Aig_ManForEachLiLoSeq( p->pMan, pObjLi, pObjLo, i )
            {
                Lits[0] = (f-1)*nLitsAll + toLitCond( p->pVarNums[pObjLi->Id], 0 );
                Lits[1] =  f   *nLitsAll + toLitCond( p->pVarNums[pObjLo->Id], 1 );
                if ( !sat_solver_addclause( pSat, Lits, Lits + 2 ) )
                {
                    sat_solver_delete( pSat );
                    return NULL;
                }
                Lits[0]++;
                Lits[1]--;
                if ( !sat_solver_addclause( pSat, Lits, Lits + 2 ) )
                {
                    sat_solver_delete( pSat );
                    return NULL;
                }
            }
            // add clauses for the next timeframe
            for ( i = 0; i < p->nLiterals; i++ )
                pLits[i] += nLitsAll;
            for ( i = 0; i < p->nClauses; i++ )
            {
                if ( !sat_solver_addclause( pSat, p->pClauses[i], p->pClauses[i+1] ) )
                {
                    sat_solver_delete( pSat );
                    return NULL;
                }
            }
        }
        // return literals to their original state
        nLitsAll = (f-1) * nLitsAll;
        for ( i = 0; i < p->nLiterals; i++ )
            pLits[i] -= nLitsAll;
    }
    if ( fInit )
    {
        Aig_Obj_t * pObjLo;
        int Lits[1];
        Aig_ManForEachLoSeq( p->pMan, pObjLo, i )
        {
            Lits[0] = toLitCond( p->pVarNums[pObjLo->Id], 1 );
            if ( !sat_solver_addclause( pSat, Lits, Lits + 1 ) )
            {
                sat_solver_delete( pSat );
                return NULL;
            }
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

/**Function*************************************************************

  Synopsis    [Writes CNF into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Cnf_DataWriteIntoSolver( Cnf_Dat_t * p, int nFrames, int fInit )
{
    return Cnf_DataWriteIntoSolverInt( sat_solver_new(), p, nFrames, fInit );
}

/**Function*************************************************************

  Synopsis    [Writes CNF into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Cnf_DataWriteIntoSolver2( Cnf_Dat_t * p, int nFrames, int fInit )
{
    sat_solver2 * pSat;
    int i, f, status;
    assert( nFrames > 0 );
    pSat = sat_solver2_new();
    sat_solver2_setnvars( pSat, p->nVars * nFrames );
    for ( i = 0; i < p->nClauses; i++ )
    {
        if ( !sat_solver2_addclause( pSat, p->pClauses[i], p->pClauses[i+1], 0 ) )
        {
            sat_solver2_delete( pSat );
            return NULL;
        }
    }
    if ( nFrames > 1 )
    {
        Aig_Obj_t * pObjLo, * pObjLi;
        int nLitsAll, * pLits, Lits[2];
        nLitsAll = 2 * p->nVars;
        pLits = p->pClauses[0];
        for ( f = 1; f < nFrames; f++ )
        {
            // add equality of register inputs/outputs for different timeframes
            Aig_ManForEachLiLoSeq( p->pMan, pObjLi, pObjLo, i )
            {
                Lits[0] = (f-1)*nLitsAll + toLitCond( p->pVarNums[pObjLi->Id], 0 );
                Lits[1] =  f   *nLitsAll + toLitCond( p->pVarNums[pObjLo->Id], 1 );
                if ( !sat_solver2_addclause( pSat, Lits, Lits + 2, 0 ) )
                {
                    sat_solver2_delete( pSat );
                    return NULL;
                }
                Lits[0]++;
                Lits[1]--;
                if ( !sat_solver2_addclause( pSat, Lits, Lits + 2, 0 ) )
                {
                    sat_solver2_delete( pSat );
                    return NULL;
                }
            }
            // add clauses for the next timeframe
            for ( i = 0; i < p->nLiterals; i++ )
                pLits[i] += nLitsAll;
            for ( i = 0; i < p->nClauses; i++ )
            {
                if ( !sat_solver2_addclause( pSat, p->pClauses[i], p->pClauses[i+1], 0 ) )
                {
                    sat_solver2_delete( pSat );
                    return NULL;
                }
            }
        }
        // return literals to their original state
        nLitsAll = (f-1) * nLitsAll;
        for ( i = 0; i < p->nLiterals; i++ )
            pLits[i] -= nLitsAll;
    }
    if ( fInit )
    {
        Aig_Obj_t * pObjLo;
        int Lits[1];
        Aig_ManForEachLoSeq( p->pMan, pObjLo, i )
        {
            Lits[0] = toLitCond( p->pVarNums[pObjLo->Id], 1 );
            if ( !sat_solver2_addclause( pSat, Lits, Lits + 1, 0 ) )
            {
                sat_solver2_delete( pSat );
                return NULL;
            }
        }
    }
    status = sat_solver2_simplify(pSat);
    if ( status == 0 )
    {
        sat_solver2_delete( pSat );
        return NULL;
    }
    return pSat;
}

/**Function*************************************************************

  Synopsis    [Adds the OR-clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cnf_DataWriteOrClause( void * p, Cnf_Dat_t * pCnf )
{
    sat_solver * pSat = (sat_solver *)p;
    Aig_Obj_t * pObj;
    int i, * pLits;
    pLits = ABC_ALLOC( int, Aig_ManCoNum(pCnf->pMan) );
    Aig_ManForEachCo( pCnf->pMan, pObj, i )
        pLits[i] = toLitCond( pCnf->pVarNums[pObj->Id], 0 );
    if ( !sat_solver_addclause( pSat, pLits, pLits + Aig_ManCoNum(pCnf->pMan) ) )
    {
        ABC_FREE( pLits );
        return 0;
    }
    ABC_FREE( pLits );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds the OR-clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cnf_DataWriteOrClause2( void * p, Cnf_Dat_t * pCnf )
{
    sat_solver2 * pSat = (sat_solver2 *)p;
    Aig_Obj_t * pObj;
    int i, * pLits;
    pLits = ABC_ALLOC( int, Aig_ManCoNum(pCnf->pMan) );
    Aig_ManForEachCo( pCnf->pMan, pObj, i )
        pLits[i] = toLitCond( pCnf->pVarNums[pObj->Id], 0 );
    if ( !sat_solver2_addclause( pSat, pLits, pLits + Aig_ManCoNum(pCnf->pMan), 0 ) )
    {
        ABC_FREE( pLits );
        return 0;
    }
    ABC_FREE( pLits );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds the OR-clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cnf_DataWriteAndClauses( void * p, Cnf_Dat_t * pCnf )
{
    sat_solver * pSat = (sat_solver *)p;
    Aig_Obj_t * pObj;
    int i, Lit;
    Aig_ManForEachCo( pCnf->pMan, pObj, i )
    {
        Lit = toLitCond( pCnf->pVarNums[pObj->Id], 0 );
        if ( !sat_solver_addclause( pSat, &Lit, &Lit+1 ) )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Transforms polarity of the internal veriables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_DataTranformPolarity( Cnf_Dat_t * pCnf, int fTransformPos )
{
    Aig_Obj_t * pObj;
    int * pVarToPol;
    int i, iVar;
    // create map from the variable number to its polarity
    pVarToPol = ABC_CALLOC( int, pCnf->nVars );
    Aig_ManForEachObj( pCnf->pMan, pObj, i )
    {
        if ( !fTransformPos && Aig_ObjIsCo(pObj) )
            continue;
        if ( pCnf->pVarNums[pObj->Id] >= 0 )
            pVarToPol[ pCnf->pVarNums[pObj->Id] ] = pObj->fPhase;
    }
    // transform literals
    for ( i = 0; i < pCnf->nLiterals; i++ )
    {
        iVar = lit_var(pCnf->pClauses[0][i]);
        assert( iVar < pCnf->nVars );
        if ( pVarToPol[iVar] )
            pCnf->pClauses[0][i] = lit_neg( pCnf->pClauses[0][i] );
    }
    ABC_FREE( pVarToPol );
}

/**Function*************************************************************

  Synopsis    [Adds constraints for the two-input AND-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cnf_DataAddXorClause( void * pSat, int iVarA, int iVarB, int iVarC )
{
    lit Lits[3];
    assert( iVarA > 0 && iVarB > 0 && iVarC > 0 );

    Lits[0] = toLitCond( iVarA, 1 );
    Lits[1] = toLitCond( iVarB, 1 );
    Lits[2] = toLitCond( iVarC, 1 );
    if ( !sat_solver_addclause( (sat_solver *)pSat, Lits, Lits + 3 ) )
        return 0;

    Lits[0] = toLitCond( iVarA, 1 );
    Lits[1] = toLitCond( iVarB, 0 );
    Lits[2] = toLitCond( iVarC, 0 );
    if ( !sat_solver_addclause( (sat_solver *)pSat, Lits, Lits + 3 ) )
        return 0;

    Lits[0] = toLitCond( iVarA, 0 );
    Lits[1] = toLitCond( iVarB, 1 );
    Lits[2] = toLitCond( iVarC, 0 );
    if ( !sat_solver_addclause( (sat_solver *)pSat, Lits, Lits + 3 ) )
        return 0;

    Lits[0] = toLitCond( iVarA, 0 );
    Lits[1] = toLitCond( iVarB, 0 );
    Lits[2] = toLitCond( iVarC, 1 );
    if ( !sat_solver_addclause( (sat_solver *)pSat, Lits, Lits + 3 ) )
        return 0;

    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

