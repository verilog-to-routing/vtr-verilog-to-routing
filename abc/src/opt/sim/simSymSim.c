/**CFile****************************************************************

  FileName    [simSymSim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Simulation to determine two-variable symmetries.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: simSymSim.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "sim.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Sim_SymmsCreateSquare( Sym_Man_t * p, unsigned * pPat );
static void Sim_SymmsDeriveInfo( Sym_Man_t * p, unsigned * pPat, Abc_Obj_t * pNode, Vec_Ptr_t * vMatrsNonSym, int Output );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Detects non-symmetric pairs using one pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SymmsSimulate( Sym_Man_t * p, unsigned * pPat, Vec_Ptr_t * vMatrsNonSym )
{
    Abc_Obj_t * pNode;
    int i, nPairsTotal, nPairsSym, nPairsNonSym;
    abctime clk;

    // create the simulation matrix
    Sim_SymmsCreateSquare( p, pPat );
    // simulate each node in the DFS order
clk = Abc_Clock();
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vNodes, pNode, i )
    {
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        Sim_UtilSimulateNodeOne( pNode, p->vSim, p->nSimWords, 0 );
    }
p->timeSim += Abc_Clock() - clk;
    // collect info into the CO matrices
clk = Abc_Clock();
    Abc_NtkForEachCo( p->pNtk, pNode, i )
    {
        pNode = Abc_ObjFanin0(pNode);
//        if ( Abc_ObjIsCi(pNode) || Abc_AigNodeIsConst(pNode) )
//            continue;
        nPairsTotal  = Vec_IntEntry(p->vPairsTotal, i);
        nPairsSym    = Vec_IntEntry(p->vPairsSym,   i);
        nPairsNonSym = Vec_IntEntry(p->vPairsNonSym,i);
        assert( nPairsTotal >= nPairsSym + nPairsNonSym ); 
        if ( nPairsTotal == nPairsSym + nPairsNonSym )
            continue;
        Sim_SymmsDeriveInfo( p, pPat, pNode, vMatrsNonSym, i );
    }
p->timeMatr += Abc_Clock() - clk;
}

/**Function*************************************************************

  Synopsis    [Creates the square matrix of simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SymmsCreateSquare( Sym_Man_t * p, unsigned * pPat )
{
    unsigned * pSimInfo;
    Abc_Obj_t * pNode;
    int i, w;
    // for each PI var copy the pattern
    Abc_NtkForEachCi( p->pNtk, pNode, i )
    {
        pSimInfo = (unsigned *)Vec_PtrEntry( p->vSim, pNode->Id );
        if ( Sim_HasBit(pPat, i) )
        {
            for ( w = 0; w < p->nSimWords; w++ )
                pSimInfo[w] = SIM_MASK_FULL;
        }
        else
        {
            for ( w = 0; w < p->nSimWords; w++ )
                pSimInfo[w] = 0;
        }
        // flip one bit
        Sim_XorBit( pSimInfo, i );
    }
}

/**Function*************************************************************

  Synopsis    [Transfers the info to the POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SymmsDeriveInfo( Sym_Man_t * p, unsigned * pPat, Abc_Obj_t * pNode, Vec_Ptr_t * vMatrsNonSym, int Output )
{
    Extra_BitMat_t * pMat;
    Vec_Int_t * vSupport;
    unsigned * pSupport;
    unsigned * pSimInfo;
    int i, w, Index;
    // get the matrix, the support, and the simulation info
    pMat = (Extra_BitMat_t *)Vec_PtrEntry( vMatrsNonSym, Output );
    vSupport = Vec_VecEntryInt( p->vSupports, Output );
    pSupport = (unsigned *)Vec_PtrEntry( p->vSuppFun, Output );
    pSimInfo = (unsigned *)Vec_PtrEntry( p->vSim, pNode->Id );
    // generate vectors A1 and A2
    for ( w = 0; w < p->nSimWords; w++ )
    {
        p->uPatCol[w] = pSupport[w] & pPat[w] &  pSimInfo[w];
        p->uPatRow[w] = pSupport[w] & pPat[w] & ~pSimInfo[w];
    }
    // add two dimensions
    Vec_IntForEachEntry( vSupport, i, Index )
        if ( Sim_HasBit( p->uPatCol, i ) )
            Extra_BitMatrixOr( pMat, i, p->uPatRow );
    // add two dimensions
    Vec_IntForEachEntry( vSupport, i, Index )
        if ( Sim_HasBit( p->uPatRow, i ) )
            Extra_BitMatrixOr( pMat, i, p->uPatCol );
    // generate vectors B1 and B2
    for ( w = 0; w < p->nSimWords; w++ )
    {
        p->uPatCol[w] = pSupport[w] & ~pPat[w] &  pSimInfo[w];
        p->uPatRow[w] = pSupport[w] & ~pPat[w] & ~pSimInfo[w];
    }
    // add two dimensions
    Vec_IntForEachEntry( vSupport, i, Index )
        if ( Sim_HasBit( p->uPatCol, i ) )
            Extra_BitMatrixOr( pMat, i, p->uPatRow );
    // add two dimensions
    Vec_IntForEachEntry( vSupport, i, Index )
        if ( Sim_HasBit( p->uPatRow, i ) )
            Extra_BitMatrixOr( pMat, i, p->uPatCol );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

