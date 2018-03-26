/**CFile****************************************************************

  FileName    [simSymSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Satisfiability to determine two variable symmetries.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: simSymSat.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "proof/fraig/fraig.h"
#include "sim.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Sim_SymmsSatProveOne( Sym_Man_t * p, int Out, int Var1, int Var2, unsigned * pPattern );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Tries to prove the remaining pairs using SAT.]

  Description [Continues to prove as long as it encounters symmetric pairs.
  Returns 1 if a non-symmetric pair is found (which gives a counter-example).
  Returns 0 if it finishes considering all pairs for all outputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_SymmsGetPatternUsingSat( Sym_Man_t * p, unsigned * pPattern )
{
    Vec_Int_t * vSupport;
    Extra_BitMat_t * pMatSym, * pMatNonSym;
    int Index1, Index2, Index3, IndexU, IndexV;
    int v, u, i, k, b, out;

    // iterate through outputs
    for ( out = p->iOutput; out < p->nOutputs; out++ )
    {
        pMatSym    = (Extra_BitMat_t *)Vec_PtrEntry( p->vMatrSymms,    out );
        pMatNonSym = (Extra_BitMat_t *)Vec_PtrEntry( p->vMatrNonSymms, out );

        // go through the remaining variable pairs
        vSupport = Vec_VecEntryInt( p->vSupports, out );
        Vec_IntForEachEntry( vSupport, v, Index1 )
        Vec_IntForEachEntryStart( vSupport, u, Index2, Index1+1 )
        {
            if ( Extra_BitMatrixLookup1( pMatSym, v, u ) || Extra_BitMatrixLookup1( pMatNonSym, v, u ) )
                continue;
            p->nSatRuns++;

            // collect the support variables that are symmetric with u and v
            Vec_IntClear( p->vVarsU );
            Vec_IntClear( p->vVarsV );
            Vec_IntForEachEntry( vSupport, b, Index3 )
            {
                if ( Extra_BitMatrixLookup1( pMatSym, u, b ) )
                    Vec_IntPush( p->vVarsU, b );
                if ( Extra_BitMatrixLookup1( pMatSym, v, b ) )
                    Vec_IntPush( p->vVarsV, b );
            }

            if ( Sim_SymmsSatProveOne( p, out, v, u, pPattern ) )
            { // update the symmetric variable info            
                p->nSatRunsUnsat++;
                Vec_IntForEachEntry( p->vVarsU, i, IndexU )
                Vec_IntForEachEntry( p->vVarsV, k, IndexV )
                {
                    Extra_BitMatrixInsert1( pMatSym,  i, k );  // Theorem 1
                    Extra_BitMatrixInsert2( pMatSym,  i, k );  // Theorem 1
                    Extra_BitMatrixOrTwo( pMatNonSym, i, k );  // Theorem 2
                }
            }
            else
            { // update the assymmetric variable info
                p->nSatRunsSat++;
                Vec_IntForEachEntry( p->vVarsU, i, IndexU )
                Vec_IntForEachEntry( p->vVarsV, k, IndexV )
                {
                    Extra_BitMatrixInsert1( pMatNonSym, i, k );   // Theorem 3
                    Extra_BitMatrixInsert2( pMatNonSym, i, k );   // Theorem 3
                }

                // remember the out
                p->iOutput = out;
                p->iVar1Old = p->iVar1;
                p->iVar2Old = p->iVar2;
                p->iVar1 = v;
                p->iVar2 = u;
                return 1;

            }
        }
        // make sure that the symmetry matrix contains only cliques
        assert( Extra_BitMatrixIsClique( pMatSym ) );
    }

    // mark that we finished all outputs
    p->iOutput = p->nOutputs;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the variables are symmetric; 0 otherwise.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_SymmsSatProveOne( Sym_Man_t * p, int Out, int Var1, int Var2, unsigned * pPattern )
{
    Fraig_Params_t Params;
    Fraig_Man_t * pMan;
    Abc_Ntk_t * pMiter;
    int RetValue, i;
    abctime clk;
    int * pModel;

    // get the miter for this problem
    pMiter = Abc_NtkMiterForCofactors( p->pNtk, Out, Var1, Var2 );
    // transform the miter into a fraig
    Fraig_ParamsSetDefault( &Params );
    Params.fInternal = 1;
    Params.nPatsRand = 512;
    Params.nPatsDyna = 512;
    Params.nSeconds = ABC_INFINITY;

clk = Abc_Clock();
    pMan = (Fraig_Man_t *)Abc_NtkToFraig( pMiter, &Params, 0, 0 ); 
p->timeFraig += Abc_Clock() - clk;
clk = Abc_Clock();
    Fraig_ManProveMiter( pMan );
p->timeSat += Abc_Clock() - clk;

    // analyze the result
    RetValue = Fraig_ManCheckMiter( pMan );
//    assert( RetValue >= 0 );
    // save the pattern
    if ( RetValue == 0 )
    {
        // get the pattern
        pModel = Fraig_ManReadModel( pMan );
        assert( pModel != NULL );
//printf( "Disproved by SAT: out = %d  pair = (%d, %d)\n", Out, Var1, Var2 );
        // transfer the model into the pattern
        for ( i = 0; i < p->nSimWords; i++ )
            pPattern[i] = 0;
        for ( i = 0; i < p->nInputs; i++ )
            if ( pModel[i] )
                Sim_SetBit( pPattern, i );
        // make sure these variables have the same value (1)
        Sim_SetBit( pPattern, Var1 );
        Sim_SetBit( pPattern, Var2 );
    }
    else if ( RetValue == -1 )
    {
        // this should never happen; if it happens, such is life
        // we are conservative and assume that there is no symmetry
//printf( "STRANGE THING: out = %d %s  pair = (%d %s, %d %s)\n", 
//                        Out, Abc_ObjName(Abc_NtkCo(p->pNtk,Out)), 
//                        Var1, Abc_ObjName(Abc_NtkCi(p->pNtk,Var1)), 
//                        Var2, Abc_ObjName(Abc_NtkCi(p->pNtk,Var2)) );
        memset( pPattern, 0, sizeof(unsigned) * p->nSimWords );
        RetValue = 0;
    }
    // delete the fraig manager
    Fraig_ManFree( pMan );
    // delete the miter
    Abc_NtkDelete( pMiter );
    return RetValue;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

