/**CFile****************************************************************

  FileName    [sswPart.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [Partitioned signal correspondence.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswPart.c,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sswInt.h"
#include "aig/ioa/ioa.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs partitioned sequential SAT sweeping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Ssw_SignalCorrespondencePart( Aig_Man_t * pAig, Ssw_Pars_t * pPars )
{
    int fPrintParts = 0;
    char Buffer[100];
    Aig_Man_t * pTemp, * pNew;
    Vec_Ptr_t * vResult;
    Vec_Int_t * vPart;
    int * pMapBack;
    int i, nCountPis, nCountRegs;
    int nClasses, nPartSize, fVerbose;
    abctime clk = Abc_Clock();
    if ( pPars->fConstrs )
    {
        Abc_Print( 1, "Cannot use partitioned computation with constraints.\n" );
        return NULL;
    }
    // save parameters
    nPartSize = pPars->nPartSize; pPars->nPartSize = 0;
    fVerbose  = pPars->fVerbose;  pPars->fVerbose  = 0;
    // generate partitions
    if ( pAig->vClockDoms )
    {
        // divide large clock domains into separate partitions
        vResult = Vec_PtrAlloc( 100 );
        Vec_PtrForEachEntry( Vec_Int_t *, (Vec_Ptr_t *)pAig->vClockDoms, vPart, i )
        {
            if ( nPartSize && Vec_IntSize(vPart) > nPartSize )
                Aig_ManPartDivide( vResult, vPart, nPartSize, pPars->nOverSize );
            else
                Vec_PtrPush( vResult, Vec_IntDup(vPart) );
        }
    }
    else
        vResult = Aig_ManRegPartitionSimple( pAig, nPartSize, pPars->nOverSize );
//    vResult = Aig_ManPartitionSmartRegisters( pAig, nPartSize, 0 ); 
//    vResult = Aig_ManRegPartitionSmart( pAig, nPartSize );
    if ( fPrintParts )
    {
        // print partitions
        Abc_Print( 1, "Simple partitioning. %d partitions are saved:\n", Vec_PtrSize(vResult) );
        Vec_PtrForEachEntry( Vec_Int_t *, vResult, vPart, i )
        {
//            extern void Ioa_WriteAiger( Aig_Man_t * pMan, char * pFileName, int fWriteSymbols, int fCompact );
            sprintf( Buffer, "part%03d.aig", i );
            pTemp = Aig_ManRegCreatePart( pAig, vPart, &nCountPis, &nCountRegs, NULL );
            Ioa_WriteAiger( pTemp, Buffer, 0, 0 );
            Abc_Print( 1, "part%03d.aig : Reg = %4d. PI = %4d. (True = %4d. Regs = %4d.) And = %5d.\n",
                i, Vec_IntSize(vPart), Aig_ManCiNum(pTemp)-Vec_IntSize(vPart), nCountPis, nCountRegs, Aig_ManNodeNum(pTemp) );
            Aig_ManStop( pTemp );
        }
    }

    // perform SSW with partitions
    Aig_ManReprStart( pAig, Aig_ManObjNumMax(pAig) );
    Vec_PtrForEachEntry( Vec_Int_t *, vResult, vPart, i )
    {
        pTemp = Aig_ManRegCreatePart( pAig, vPart, &nCountPis, &nCountRegs, &pMapBack );
        Aig_ManSetRegNum( pTemp, pTemp->nRegs );
        // create the projection of 1-hot registers
        if ( pAig->vOnehots )
            pTemp->vOnehots = Aig_ManRegProjectOnehots( pAig, pTemp, pAig->vOnehots, fVerbose );
        // run SSW
        if (nCountPis>0) {
            pNew = Ssw_SignalCorrespondence( pTemp, pPars );
            nClasses = Aig_TransferMappedClasses( pAig, pTemp, pMapBack );
            if ( fVerbose )
                Abc_Print( 1, "%3d : Reg = %4d. PI = %4d. (True = %4d. Regs = %4d.) And = %5d. It = %3d. Cl = %5d.\n",
                    i, Vec_IntSize(vPart), Aig_ManCiNum(pTemp)-Vec_IntSize(vPart), nCountPis, nCountRegs, Aig_ManNodeNum(pTemp), pPars->nIters, nClasses );
            Aig_ManStop( pNew );
        }
        Aig_ManStop( pTemp );
        ABC_FREE( pMapBack );
    }
    // remap the AIG
    pNew = Aig_ManDupRepr( pAig, 0 );
    Aig_ManSeqCleanup( pNew );
//    Aig_ManPrintStats( pAig );
//    Aig_ManPrintStats( pNew );
    Vec_VecFree( (Vec_Vec_t *)vResult );
    pPars->nPartSize = nPartSize;
    pPars->fVerbose = fVerbose;
    if ( fVerbose )
    {
        ABC_PRT( "Total time", Abc_Clock() - clk );
    }
    return pNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
