/**CFile****************************************************************

  FileName    [abcVerify.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Combinational and sequential verification for two networks.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcVerify.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/cmd/cmd.h"
#include "proof/fraig/fraig.h"
#include "opt/sim/sim.h"
#include "aig/aig/aig.h"
#include "aig/saig/saig.h"
#include "aig/gia/gia.h"
#include "proof/ssw/ssw.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
static void  Abc_NtkVerifyReportError( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int * pModel );
extern void  Abc_NtkVerifyReportErrorSeq( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int * pModel, int nFrames );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Verifies combinational equivalence by brute-force SAT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCecSat( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nConfLimit, int nInsLimit )
{
    extern Abc_Ntk_t * Abc_NtkMulti( Abc_Ntk_t * pNtk, int nThresh, int nFaninMax, int fCnf, int fMulti, int fSimple, int fFactor );
    Abc_Ntk_t * pMiter;
    Abc_Ntk_t * pCnf;
    int RetValue;

    // get the miter of the two networks
    pMiter = Abc_NtkMiter( pNtk1, pNtk2, 1, 0, 0, 0 );
    if ( pMiter == NULL )
    {
        printf( "Miter computation has failed.\n" );
        return;
    }
    RetValue = Abc_NtkMiterIsConstant( pMiter );
    if ( RetValue == 0 )
    {
        printf( "Networks are NOT EQUIVALENT after structural hashing.\n" );
        // report the error
        pMiter->pModel = Abc_NtkVerifyGetCleanModel( pMiter, 1 );
        Abc_NtkVerifyReportError( pNtk1, pNtk2, pMiter->pModel );
        ABC_FREE( pMiter->pModel );
        Abc_NtkDelete( pMiter );
        return;
    }
    if ( RetValue == 1 )
    {
        Abc_NtkDelete( pMiter );
        printf( "Networks are equivalent after structural hashing.\n" );
        return;
    }

    // convert the miter into a CNF
    pCnf = Abc_NtkMulti( pMiter, 0, 100, 1, 0, 0, 0 );
    Abc_NtkDelete( pMiter );
    if ( pCnf == NULL )
    {
        printf( "Renoding for CNF has failed.\n" );
        return;
    }

    // solve the CNF using the SAT solver
    RetValue = Abc_NtkMiterSat( pCnf, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)nInsLimit, 0, NULL, NULL );
    if ( RetValue == -1 )
        printf( "Networks are undecided (SAT solver timed out).\n" );
    else if ( RetValue == 0 )
        printf( "Networks are NOT EQUIVALENT after SAT.\n" );
    else
        printf( "Networks are equivalent after SAT.\n" );
    if ( pCnf->pModel )
        Abc_NtkVerifyReportError( pNtk1, pNtk2, pCnf->pModel );
    ABC_FREE( pCnf->pModel );
    Abc_NtkDelete( pCnf );
}


/**Function*************************************************************

  Synopsis    [Verifies sequential equivalence by fraiging followed by SAT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCecFraig( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nSeconds, int fVerbose )
{
    abctime clk = Abc_Clock();
    Prove_Params_t Params, * pParams = &Params;
//    Fraig_Params_t Params;
//    Fraig_Man_t * pMan;
    Abc_Ntk_t * pMiter, * pTemp;
    Abc_Ntk_t * pExdc = NULL;
    int RetValue;

    if ( pNtk1->pExdc != NULL || pNtk2->pExdc != NULL )
    {
        if ( pNtk1->pExdc != NULL && pNtk2->pExdc != NULL )
        {
            printf( "Comparing EXDC of the two networks:\n" );
            Abc_NtkCecFraig( pNtk1->pExdc, pNtk2->pExdc, nSeconds, fVerbose );
            printf( "Comparing networks under EXDC of the first network.\n" );
            pExdc = pNtk1->pExdc;
        }
        else if ( pNtk1->pExdc != NULL )
        {
            printf( "Second network has no EXDC. Comparing main networks under EXDC of the first network.\n" );
            pExdc = pNtk1->pExdc;
        }
        else if ( pNtk2->pExdc != NULL ) 
        {
            printf( "First network has no EXDC. Comparing main networks under EXDC of the second network.\n" );
            pExdc = pNtk2->pExdc;
        }
        else assert( 0 );
    }

    // get the miter of the two networks
    pMiter = Abc_NtkMiter( pNtk1, pNtk2, 1, 0, 0, 0 );
    if ( pMiter == NULL )
    {
        printf( "Miter computation has failed.\n" );
        return;
    }
    // add EXDC to the miter
    if ( pExdc )
    {
        assert( Abc_NtkPoNum(pMiter) == 1 );
        assert( Abc_NtkPoNum(pExdc) == 1 );
        pMiter = Abc_NtkMiter( pTemp = pMiter, pExdc, 1, 0, 1, 0 );
        Abc_NtkDelete( pTemp );
    }
    // handle trivial case
    RetValue = Abc_NtkMiterIsConstant( pMiter );
    if ( RetValue == 0 )
    {
        printf( "Networks are NOT EQUIVALENT after structural hashing.  " );
        // report the error
        pMiter->pModel = Abc_NtkVerifyGetCleanModel( pMiter, 1 );
        Abc_NtkVerifyReportError( pNtk1, pNtk2, pMiter->pModel );
        ABC_FREE( pMiter->pModel );
        Abc_NtkDelete( pMiter );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        return;
    }
    if ( RetValue == 1 )
    {
        printf( "Networks are equivalent after structural hashing.  " );
        Abc_NtkDelete( pMiter );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        return;
    }
/*
    // convert the miter into a FRAIG
    Fraig_ParamsSetDefault( &Params );
    Params.fVerbose = fVerbose;
    Params.nSeconds = nSeconds;
//    Params.fFuncRed = 0;
//    Params.nPatsRand = 0;
//    Params.nPatsDyna = 0;
    pMan = (Fraig_Man_t *)Abc_NtkToFraig( pMiter, &Params, 0, 0 ); 
    Fraig_ManProveMiter( pMan );

    // analyze the result
    RetValue = Fraig_ManCheckMiter( pMan );
    // report the result
    if ( RetValue == -1 )
        printf( "Networks are undecided (SAT solver timed out on the final miter).\n" );
    else if ( RetValue == 1 )
        printf( "Networks are equivalent after fraiging.\n" );
    else if ( RetValue == 0 )
    {
        printf( "Networks are NOT EQUIVALENT after fraiging.\n" );
        Abc_NtkVerifyReportError( pNtk1, pNtk2, Fraig_ManReadModel(pMan) );
    }
    else assert( 0 );
    // delete the fraig manager
    Fraig_ManFree( pMan );
    // delete the miter
    Abc_NtkDelete( pMiter );
*/
    // solve the CNF using the SAT solver
    Prove_ParamsSetDefault( pParams );
    pParams->nItersMax = 5;
//    RetValue = Abc_NtkMiterProve( &pMiter, pParams );
//    pParams->fVerbose = 1;
    RetValue = Abc_NtkIvyProve( &pMiter, pParams );
    if ( RetValue == -1 )
        printf( "Networks are undecided (resource limits is reached).  " );
    else if ( RetValue == 0 )
    {
        int * pSimInfo = Abc_NtkVerifySimulatePattern( pMiter, pMiter->pModel );
        if ( pSimInfo[0] != 1 )
            printf( "ERROR in Abc_NtkMiterProve(): Generated counter-example is invalid.\n" );
        else
            printf( "Networks are NOT EQUIVALENT.  " );
        ABC_FREE( pSimInfo );
    }
    else
        printf( "Networks are equivalent.  " );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( pMiter->pModel )
        Abc_NtkVerifyReportError( pNtk1, pNtk2, pMiter->pModel );
    Abc_NtkDelete( pMiter );
}

/**Function*************************************************************

  Synopsis    [Verifies sequential equivalence by fraiging followed by SAT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCecFraigPart( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nSeconds, int nPartSize, int fVerbose )
{
    Prove_Params_t Params, * pParams = &Params;
    Abc_Ntk_t * pMiter, * pMiterPart;
    Abc_Obj_t * pObj;
    int i, RetValue, Status, nOutputs;

    // solve the CNF using the SAT solver
    Prove_ParamsSetDefault( pParams );
    pParams->nItersMax = 5;
    //    pParams->fVerbose = 1;

    assert( nPartSize > 0 );

    // get the miter of the two networks
    pMiter = Abc_NtkMiter( pNtk1, pNtk2, 1, nPartSize, 0, 0 );
    if ( pMiter == NULL )
    {
        printf( "Miter computation has failed.\n" );
        return;
    }
    RetValue = Abc_NtkMiterIsConstant( pMiter );
    if ( RetValue == 0 )
    {
        printf( "Networks are NOT EQUIVALENT after structural hashing.\n" );
        // report the error
        pMiter->pModel = Abc_NtkVerifyGetCleanModel( pMiter, 1 );
        Abc_NtkVerifyReportError( pNtk1, pNtk2, pMiter->pModel );
        ABC_FREE( pMiter->pModel );
        Abc_NtkDelete( pMiter );
        return;
    }
    if ( RetValue == 1 )
    {
        printf( "Networks are equivalent after structural hashing.\n" );
        Abc_NtkDelete( pMiter );
        return;
    }

    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "unset progressbar" );

    // solve the problem iteratively for each output of the miter
    Status = 1;
    nOutputs = 0;
    Abc_NtkForEachPo( pMiter, pObj, i )
    {
        if ( Abc_ObjFanin0(pObj) == Abc_AigConst1(pMiter) )
        {
            if ( Abc_ObjFaninC0(pObj) ) // complemented -> const 0
                RetValue = 1;
            else
                RetValue = 0;
            pMiterPart = NULL;
        }
        else
        {
            // get the cone of this output
            pMiterPart = Abc_NtkCreateCone( pMiter, Abc_ObjFanin0(pObj), Abc_ObjName(pObj), 0 );
            if ( Abc_ObjFaninC0(pObj) )
                Abc_ObjXorFaninC( Abc_NtkPo(pMiterPart,0), 0 );
            // solve the cone
        //    RetValue = Abc_NtkMiterProve( &pMiterPart, pParams );
            RetValue = Abc_NtkIvyProve( &pMiterPart, pParams );
        }

        if ( RetValue == -1 )
        {
            printf( "Networks are undecided (resource limits is reached).\r" );
            Status = -1;
        }
        else if ( RetValue == 0 )
        {
            int * pSimInfo = Abc_NtkVerifySimulatePattern( pMiterPart, pMiterPart->pModel );
            if ( pSimInfo[0] != 1 )
                printf( "ERROR in Abc_NtkMiterProve(): Generated counter-example is invalid.\n" );
            else
                printf( "Networks are NOT EQUIVALENT.                 \n" );
            ABC_FREE( pSimInfo );
            Status = 0;
            break;
        }
        else
        {
            printf( "Finished part %5d (out of %5d)\r", i+1, Abc_NtkPoNum(pMiter) );
            nOutputs += nPartSize;
        }
//        if ( pMiter->pModel )
//            Abc_NtkVerifyReportError( pNtk1, pNtk2, pMiter->pModel );
        if ( pMiterPart )
            Abc_NtkDelete( pMiterPart );
    }
  
    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "set progressbar" );

    if ( Status == 1 )
        printf( "Networks are equivalent.                         \n" );
    else if ( Status == -1 )
        printf( "Timed out after verifying %d outputs (out of %d).\n", nOutputs, Abc_NtkCoNum(pNtk1) );
    Abc_NtkDelete( pMiter );
}

/**Function*************************************************************

  Synopsis    [Verifies sequential equivalence by fraiging followed by SAT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCecFraigPartAuto( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nSeconds, int fVerbose )
{
    extern Vec_Ptr_t * Abc_NtkPartitionSmart( Abc_Ntk_t * pNtk, int nPartSizeLimit, int fVerbose );
    extern void Abc_NtkConvertCos( Abc_Ntk_t * pNtk, Vec_Int_t * vOuts, Vec_Ptr_t * vOnePtr );

    Vec_Ptr_t * vParts, * vOnePtr;
    Vec_Int_t * vOne;
    Prove_Params_t Params, * pParams = &Params;
    Abc_Ntk_t * pMiter, * pMiterPart;
    int i, RetValue, Status, nOutputs;

    // solve the CNF using the SAT solver
    Prove_ParamsSetDefault( pParams );
    pParams->nItersMax = 5;
    //    pParams->fVerbose = 1;

    // get the miter of the two networks
    pMiter = Abc_NtkMiter( pNtk1, pNtk2, 1, 1, 0, 0 );
    if ( pMiter == NULL )
    {
        printf( "Miter computation has failed.\n" );
        return;
    }
    RetValue = Abc_NtkMiterIsConstant( pMiter );
    if ( RetValue == 0 )
    {
        printf( "Networks are NOT EQUIVALENT after structural hashing.\n" );
        // report the error
        pMiter->pModel = Abc_NtkVerifyGetCleanModel( pMiter, 1 );
        Abc_NtkVerifyReportError( pNtk1, pNtk2, pMiter->pModel );
        ABC_FREE( pMiter->pModel );
        Abc_NtkDelete( pMiter );
        return;
    }
    if ( RetValue == 1 )
    {
        printf( "Networks are equivalent after structural hashing.\n" );
        Abc_NtkDelete( pMiter );
        return;
    }

    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "unset progressbar" );

    // partition the outputs
    vParts = Abc_NtkPartitionSmart( pMiter, 300, 0 );

    // fraig each partition
    Status = 1;
    nOutputs = 0;
    vOnePtr = Vec_PtrAlloc( 1000 );
    Vec_PtrForEachEntry( Vec_Int_t *, vParts, vOne, i )
    {
        // get this part of the miter
        Abc_NtkConvertCos( pMiter, vOne, vOnePtr );
        pMiterPart = Abc_NtkCreateConeArray( pMiter, vOnePtr, 0 );
        Abc_NtkCombinePos( pMiterPart, 0, 0 );
        // check the miter for being constant
        RetValue = Abc_NtkMiterIsConstant( pMiterPart );
        if ( RetValue == 0 )
        {
            printf( "Networks are NOT EQUIVALENT after partitioning.\n" );
            Abc_NtkDelete( pMiterPart );
            break;
        }
        if ( RetValue == 1 )
        {
            Abc_NtkDelete( pMiterPart );
            continue;
        }
        printf( "Verifying part %4d  (out of %4d)  PI = %5d. PO = %5d. And = %6d. Lev = %4d.\r", 
            i+1, Vec_PtrSize(vParts), Abc_NtkPiNum(pMiterPart), Abc_NtkPoNum(pMiterPart), 
            Abc_NtkNodeNum(pMiterPart), Abc_AigLevel(pMiterPart) );
        fflush( stdout );
        // solve the problem
        RetValue = Abc_NtkIvyProve( &pMiterPart, pParams );
        if ( RetValue == -1 )
        {
            printf( "Networks are undecided (resource limits is reached).\r" );
            Status = -1;
        }
        else if ( RetValue == 0 )
        {
            int * pSimInfo = Abc_NtkVerifySimulatePattern( pMiterPart, pMiterPart->pModel );
            if ( pSimInfo[0] != 1 )
                printf( "ERROR in Abc_NtkMiterProve(): Generated counter-example is invalid.\n" );
            else
                printf( "Networks are NOT EQUIVALENT.                 \n" );
            ABC_FREE( pSimInfo );
            Status = 0;
            Abc_NtkDelete( pMiterPart );
            break;
        }
        else
        {
//            printf( "Finished part %5d (out of %5d)\r", i+1, Vec_PtrSize(vParts) );
            nOutputs += Vec_IntSize(vOne);
        }
        Abc_NtkDelete( pMiterPart );
    }
    printf( "                                                                                          \r" );
    Vec_VecFree( (Vec_Vec_t *)vParts );
    Vec_PtrFree( vOnePtr );

    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "set progressbar" );

    if ( Status == 1 )
        printf( "Networks are equivalent.                         \n" );
    else if ( Status == -1 )
        printf( "Timed out after verifying %d outputs (out of %d).\n", nOutputs, Abc_NtkCoNum(pNtk1) );
    Abc_NtkDelete( pMiter );
}

/**Function*************************************************************

  Synopsis    [Verifies sequential equivalence by brute-force SAT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkSecSat( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nConfLimit, int nInsLimit, int nFrames )
{
    extern Abc_Ntk_t * Abc_NtkMulti( Abc_Ntk_t * pNtk, int nThresh, int nFaninMax, int fCnf, int fMulti, int fSimple, int fFactor );
    Abc_Ntk_t * pMiter;
    Abc_Ntk_t * pFrames;
    Abc_Ntk_t * pCnf;
    int RetValue;

    // get the miter of the two networks
    pMiter = Abc_NtkMiter( pNtk1, pNtk2, 0, 0, 0, 0 );
    if ( pMiter == NULL )
    {
        printf( "Miter computation has failed.\n" );
        return;
    }
    RetValue = Abc_NtkMiterIsConstant( pMiter );
    if ( RetValue == 0 )
    {
        Abc_NtkDelete( pMiter );
        printf( "Networks are NOT EQUIVALENT after structural hashing.\n" );
        return;
    }
    if ( RetValue == 1 )
    {
        Abc_NtkDelete( pMiter );
        printf( "Networks are equivalent after structural hashing.\n" );
        return;
    }

    // create the timeframes
    pFrames = Abc_NtkFrames( pMiter, nFrames, 1, 0 );
    Abc_NtkDelete( pMiter );
    if ( pFrames == NULL )
    {
        printf( "Frames computation has failed.\n" );
        return;
    }
    RetValue = Abc_NtkMiterIsConstant( pFrames );
    if ( RetValue == 0 )
    {
        Abc_NtkDelete( pFrames );
        printf( "Networks are NOT EQUIVALENT after framing.\n" );
        return;
    }
    if ( RetValue == 1 )
    {
        Abc_NtkDelete( pFrames );
        printf( "Networks are equivalent after framing.\n" );
        return;
    }

    // convert the miter into a CNF
    pCnf = Abc_NtkMulti( pFrames, 0, 100, 1, 0, 0, 0 );
    Abc_NtkDelete( pFrames );
    if ( pCnf == NULL )
    {
        printf( "Renoding for CNF has failed.\n" );
        return;
    }

    // solve the CNF using the SAT solver
    RetValue = Abc_NtkMiterSat( pCnf, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)nInsLimit, 0, NULL, NULL );
    if ( RetValue == -1 )
        printf( "Networks are undecided (SAT solver timed out).\n" );
    else if ( RetValue == 0 )
        printf( "Networks are NOT EQUIVALENT after SAT.\n" );
    else
        printf( "Networks are equivalent after SAT.\n" );
    Abc_NtkDelete( pCnf );
}

/**Function*************************************************************

  Synopsis    [Verifies combinational equivalence by fraiging followed by SAT]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkSecFraig( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nSeconds, int nFrames, int fVerbose )
{
    Fraig_Params_t Params;
    Fraig_Man_t * pMan;
    Abc_Ntk_t * pMiter;
    Abc_Ntk_t * pFrames;
    int RetValue;

    // get the miter of the two networks
    pMiter = Abc_NtkMiter( pNtk1, pNtk2, 0, 0, 0, 0 );
    if ( pMiter == NULL )
    {
        printf( "Miter computation has failed.\n" );
        return 0;
    }
    RetValue = Abc_NtkMiterIsConstant( pMiter );
    if ( RetValue == 0 )
    {
        printf( "Networks are NOT EQUIVALENT after structural hashing.\n" );
        // report the error
        pMiter->pModel = Abc_NtkVerifyGetCleanModel( pMiter, nFrames );
        Abc_NtkVerifyReportErrorSeq( pNtk1, pNtk2, pMiter->pModel, nFrames );
        ABC_FREE( pMiter->pModel );
        Abc_NtkDelete( pMiter );
        return 0;
    }
    if ( RetValue == 1 )
    {
        Abc_NtkDelete( pMiter );
        printf( "Networks are equivalent after structural hashing.\n" );
        return 1;
    }

    // create the timeframes
    pFrames = Abc_NtkFrames( pMiter, nFrames, 1, 0 );
    Abc_NtkDelete( pMiter );
    if ( pFrames == NULL )
    {
        printf( "Frames computation has failed.\n" );
        return 0;
    }
    RetValue = Abc_NtkMiterIsConstant( pFrames );
    if ( RetValue == 0 )
    {
        printf( "Networks are NOT EQUIVALENT after framing.\n" );
        // report the error
        pFrames->pModel = Abc_NtkVerifyGetCleanModel( pFrames, 1 );
//        Abc_NtkVerifyReportErrorSeq( pNtk1, pNtk2, pFrames->pModel, nFrames );
        ABC_FREE( pFrames->pModel );
        Abc_NtkDelete( pFrames );
        return 0;
    }
    if ( RetValue == 1 )
    {
        Abc_NtkDelete( pFrames );
        printf( "Networks are equivalent after framing.\n" );
        return 1;
    }

    // convert the miter into a FRAIG
    Fraig_ParamsSetDefault( &Params );
    Params.fVerbose = fVerbose;
    Params.nSeconds = nSeconds;
//    Params.fFuncRed = 0;
//    Params.nPatsRand = 0;
//    Params.nPatsDyna = 0;
    pMan = (Fraig_Man_t *)Abc_NtkToFraig( pFrames, &Params, 0, 0 ); 
    Fraig_ManProveMiter( pMan );

    // analyze the result
    RetValue = Fraig_ManCheckMiter( pMan );
    // report the result
    if ( RetValue == -1 )
        printf( "Networks are undecided (SAT solver timed out on the final miter).\n" );
    else if ( RetValue == 1 )
        printf( "Networks are equivalent after fraiging.\n" );
    else if ( RetValue == 0 )
    {
        printf( "Networks are NOT EQUIVALENT after fraiging.\n" );
//        Abc_NtkVerifyReportErrorSeq( pNtk1, pNtk2, Fraig_ManReadModel(pMan), nFrames );
    }
    else assert( 0 );
    // delete the fraig manager
    Fraig_ManFree( pMan );
    // delete the miter
    Abc_NtkDelete( pFrames );
    return RetValue == 1;
}

/**Function*************************************************************

  Synopsis    [Returns a dummy pattern full of zeros.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Abc_NtkVerifyGetCleanModel( Abc_Ntk_t * pNtk, int nFrames )
{
    int * pModel = ABC_ALLOC( int, Abc_NtkCiNum(pNtk) * nFrames );
    memset( pModel, 0, sizeof(int) * Abc_NtkCiNum(pNtk) * nFrames );
    return pModel;
}

/**Function*************************************************************

  Synopsis    [Returns the PO values under the given input pattern.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
int * Abc_NtkVerifySimulatePattern( Abc_Ntk_t * pNtk, int * pModel )
{
    Abc_Obj_t * pNode;
    int * pValues, Value0, Value1, i;
    int fStrashed = 0;
    if ( !Abc_NtkIsStrash(pNtk) )
    {
        pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
        fStrashed = 1;
    }
/*
    printf( "Counter example: " );
    Abc_NtkForEachCi( pNtk, pNode, i )
        printf( " %d", pModel[i] );
    printf( "\n" );
*/
    // increment the trav ID
    Abc_NtkIncrementTravId( pNtk );
    // set the CI values
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)1;
    Abc_NtkForEachCi( pNtk, pNode, i )
        pNode->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)pModel[i];
    // simulate in the topological order
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Value0 = ((int)(ABC_PTRINT_T)Abc_ObjFanin0(pNode)->pCopy) ^ (int)Abc_ObjFaninC0(pNode);
        Value1 = ((int)(ABC_PTRINT_T)Abc_ObjFanin1(pNode)->pCopy) ^ (int)Abc_ObjFaninC1(pNode);
        pNode->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)(Value0 & Value1);
    }
    // fill the output values
    pValues = ABC_ALLOC( int, Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pNode, i )
        pValues[i] = ((int)(ABC_PTRINT_T)Abc_ObjFanin0(pNode)->pCopy) ^ (int)Abc_ObjFaninC0(pNode);
    if ( fStrashed )
        Abc_NtkDelete( pNtk );
    return pValues;
}


/**Function*************************************************************

  Synopsis    [Reports mismatch between the two networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkVerifyReportError( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int * pModel )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNode;
    int * pValues1, * pValues2;
    int nErrors, nPrinted, i, iNode = -1;

    assert( Abc_NtkCiNum(pNtk1) == Abc_NtkCiNum(pNtk2) );
    assert( Abc_NtkCoNum(pNtk1) == Abc_NtkCoNum(pNtk2) );
    // get the CO values under this model
    pValues1 = Abc_NtkVerifySimulatePattern( pNtk1, pModel );
    pValues2 = Abc_NtkVerifySimulatePattern( pNtk2, pModel );
    // count the mismatches
    nErrors = 0;
    for ( i = 0; i < Abc_NtkCoNum(pNtk1); i++ )
        nErrors += (int)( pValues1[i] != pValues2[i] );
    printf( "Verification failed for at least %d outputs: ", nErrors );
    // print the first 3 outputs
    nPrinted = 0;
    for ( i = 0; i < Abc_NtkCoNum(pNtk1); i++ )
        if ( pValues1[i] != pValues2[i] )
        {
            if ( iNode == -1 )
                iNode = i;
            printf( " %s", Abc_ObjName(Abc_NtkCo(pNtk1,i)) );
            if ( ++nPrinted == 3 )
                break;
        }
    if ( nPrinted != nErrors )
        printf( " ..." );
    printf( "\n" );
    // report mismatch for the first output
    if ( iNode >= 0 )
    {
        printf( "Output %s: Value in Network1 = %d. Value in Network2 = %d.\n", 
            Abc_ObjName(Abc_NtkCo(pNtk1,iNode)), pValues1[iNode], pValues2[iNode] );
        printf( "Input pattern: " );
        // collect PIs in the cone
        pNode = Abc_NtkCo(pNtk1,iNode);
        vNodes = Abc_NtkNodeSupport( pNtk1, &pNode, 1 );
        // set the PI numbers
        Abc_NtkForEachCi( pNtk1, pNode, i )
            pNode->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)i;
        // print the model
        pNode = (Abc_Obj_t *)Vec_PtrEntry( vNodes, 0 );
        if ( Abc_ObjIsCi(pNode) )
        {
            Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
            {
                assert( Abc_ObjIsCi(pNode) );
                printf( " %s=%d", Abc_ObjName(pNode), pModel[(int)(ABC_PTRINT_T)pNode->pCopy] );
            }
        }
        printf( "\n" );
        Vec_PtrFree( vNodes );
    }
    ABC_FREE( pValues1 );
    ABC_FREE( pValues2 );
}


/**Function*************************************************************

  Synopsis    [Computes the COs in the support of the PO in the given frame.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkGetSeqPoSupp( Abc_Ntk_t * pNtk, int iFrame, int iNumPo )
{
    Abc_Ntk_t * pFrames;
    Abc_Obj_t * pObj, * pNodePo;
    Vec_Ptr_t * vSupp;
    int i, k;
    // get the timeframes of the network
    pFrames = Abc_NtkFrames( pNtk, iFrame + 1, 0, 0 );
//Abc_NtkShowAig( pFrames );

    // get the PO of the timeframes
    pNodePo = Abc_NtkPo( pFrames, iFrame * Abc_NtkPoNum(pNtk) + iNumPo );
    // set the support
    vSupp   = Abc_NtkNodeSupport( pFrames, &pNodePo, 1 );
    // mark the support of the frames
    Abc_NtkForEachCi( pFrames, pObj, i )
        pObj->pCopy = NULL;
    Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)1;
    // mark the support of the network if the support of the timeframes is marked
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pCopy = NULL;
    Abc_NtkForEachLatch( pNtk, pObj, i )
        if ( Abc_NtkBox(pFrames, i)->pCopy )
            pObj->pCopy = (Abc_Obj_t *)1;
    Abc_NtkForEachPi( pNtk, pObj, i )
        for ( k = 0; k <= iFrame; k++ )
            if ( Abc_NtkPi(pFrames, k*Abc_NtkPiNum(pNtk) + i)->pCopy )
                pObj->pCopy = (Abc_Obj_t *)1;
    // free stuff
    Vec_PtrFree( vSupp );
    Abc_NtkDelete( pFrames );
}

/**Function*************************************************************

  Synopsis    [Reports mismatch between the two sequential networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkVerifyReportErrorSeq( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int * pModel, int nFrames )
{
    Vec_Ptr_t * vInfo1, * vInfo2;
    Abc_Obj_t * pObj, * pObjError, * pObj1, * pObj2;
    int ValueError1 = -1, ValueError2 = -1;
    unsigned * pPats1, * pPats2;
    int i, o, k, nErrors, iFrameError = -1, iNodePo = -1, nPrinted;
    int fRemove1 = 0, fRemove2 = 0;

    if ( !Abc_NtkIsStrash(pNtk1) )
        fRemove1 = 1, pNtk1 = Abc_NtkStrash( pNtk1, 0, 0, 0 );
    if ( !Abc_NtkIsStrash(pNtk2) )
        fRemove2 = 1, pNtk2 = Abc_NtkStrash( pNtk2, 0, 0, 0 );

    // simulate sequential circuits
    vInfo1 = Sim_SimulateSeqModel( pNtk1, nFrames, pModel );
    vInfo2 = Sim_SimulateSeqModel( pNtk2, nFrames, pModel );

    // look for a discrepancy in the PO values
    nErrors = 0;
    pObjError = NULL;
    for ( i = 0; i < nFrames; i++ )
    {
        if ( pObjError )
            break;
        Abc_NtkForEachPo( pNtk1, pObj1, o )
        {
            pObj2  = Abc_NtkPo( pNtk2, o );
            pPats1 = Sim_SimInfoGet(vInfo1, pObj1);
            pPats2 = Sim_SimInfoGet(vInfo2, pObj2);
            if ( pPats1[i] == pPats2[i] )
                continue;
            nErrors++;
            if ( pObjError == NULL )
            {
                pObjError   = pObj1;
                iFrameError = i;
                iNodePo     = o;
                ValueError1 = (pPats1[i] > 0);
                ValueError2 = (pPats2[i] > 0);
            }
        }
    }

    if ( pObjError == NULL )
    {
        printf( "No output mismatches detected.\n" );
        Sim_UtilInfoFree( vInfo1 );
        Sim_UtilInfoFree( vInfo2 );
        if ( fRemove1 ) Abc_NtkDelete( pNtk1 );
        if ( fRemove2 ) Abc_NtkDelete( pNtk2 );
        return;
    }

    printf( "Verification failed for at least %d output%s of frame %d: ", nErrors, (nErrors>1? "s":""), iFrameError+1 );
    // print the first 3 outputs
    nPrinted = 0;
    Abc_NtkForEachPo( pNtk1, pObj1, o )
    {
        pObj2 = Abc_NtkPo( pNtk2, o );
        pPats1 = Sim_SimInfoGet(vInfo1, pObj1);
        pPats2 = Sim_SimInfoGet(vInfo2, pObj2);
        if ( pPats1[iFrameError] == pPats2[iFrameError] )
            continue;
        printf( " %s", Abc_ObjName(pObj1) );
        if ( ++nPrinted == 3 )
            break;
    }
    if ( nPrinted != nErrors )
        printf( " ..." );
    printf( "\n" );

    // mark CIs of the networks in the cone of influence of this output
    Abc_NtkGetSeqPoSupp( pNtk1, iFrameError, iNodePo );
    Abc_NtkGetSeqPoSupp( pNtk2, iFrameError, iNodePo );

    // report mismatch for the first output
    printf( "Output %s: Value in Network1 = %d. Value in Network2 = %d.\n", 
        Abc_ObjName(pObjError), ValueError1, ValueError2 );

    printf( "The cone of influence of output %s in Network1:\n", Abc_ObjName(pObjError) );
    printf( "PIs: " );
    Abc_NtkForEachPi( pNtk1, pObj, i )
        if ( pObj->pCopy )
            printf( "%s ", Abc_ObjName(pObj) );
    printf( "\n" );
    printf( "Latches: " );
    Abc_NtkForEachLatch( pNtk1, pObj, i )
        if ( pObj->pCopy )
            printf( "%s ", Abc_ObjName(pObj) );
    printf( "\n" );

    printf( "The cone of influence of output %s in Network2:\n", Abc_ObjName(pObjError) );
    printf( "PIs: " );
    Abc_NtkForEachPi( pNtk2, pObj, i )
        if ( pObj->pCopy )
            printf( "%s ", Abc_ObjName(pObj) );
    printf( "\n" );
    printf( "Latches: " );
    Abc_NtkForEachLatch( pNtk2, pObj, i )
        if ( pObj->pCopy )
            printf( "%s ", Abc_ObjName(pObj) );
    printf( "\n" );

    // print the patterns
    for ( i = 0; i <= iFrameError; i++ )
    {
        printf( "Frame %d:  ", i+1 );

        printf( "PI(1):" );
        Abc_NtkForEachPi( pNtk1, pObj, k )
            if ( pObj->pCopy )
                printf( "%d", Sim_SimInfoGet(vInfo1, pObj)[i] > 0 );
        printf( " " );
        printf( "L(1):" );
        Abc_NtkForEachLatch( pNtk1, pObj, k )
            if ( pObj->pCopy )
                printf( "%d", Sim_SimInfoGet(vInfo1, pObj)[i] > 0 );
        printf( " " );
        printf( "%s(1):", Abc_ObjName(pObjError) );
        printf( "%d", Sim_SimInfoGet(vInfo1, pObjError)[i] > 0 );

        printf( "  " );

        printf( "PI(2):" );
        Abc_NtkForEachPi( pNtk2, pObj, k )
            if ( pObj->pCopy )
                printf( "%d", Sim_SimInfoGet(vInfo2, pObj)[i] > 0 );
        printf( " " );
        printf( "L(2):" );
        Abc_NtkForEachLatch( pNtk2, pObj, k )
            if ( pObj->pCopy )
                printf( "%d", Sim_SimInfoGet(vInfo2, pObj)[i] > 0 );
        printf( " " );
        printf( "%s(2):", Abc_ObjName(pObjError) );
        printf( "%d", Sim_SimInfoGet(vInfo2, pObjError)[i] > 0 );

        printf( "\n" );
    }
    Abc_NtkForEachCi( pNtk1, pObj, i )
        pObj->pCopy = NULL;
    Abc_NtkForEachCi( pNtk2, pObj, i )
        pObj->pCopy = NULL;

    Sim_UtilInfoFree( vInfo1 );
    Sim_UtilInfoFree( vInfo2 );
    if ( fRemove1 ) Abc_NtkDelete( pNtk1 );
    if ( fRemove2 ) Abc_NtkDelete( pNtk2 );
}

/**Function*************************************************************

  Synopsis    [Simulates buggy miter emailed by Mike.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkSimulteBuggyMiter( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    int * pModel1, * pModel2, * pResult1, * pResult2;
    char * vPiValues1 = "01001011100000000011010110101000000";
    char * vPiValues2 = "11001101011101011111110100100010001";

    assert( strlen(vPiValues1) == (unsigned)Abc_NtkPiNum(pNtk) );
    assert( 1 == Abc_NtkPoNum(pNtk) );

    pModel1 = ABC_ALLOC( int, Abc_NtkCiNum(pNtk) );
    Abc_NtkForEachPi( pNtk, pObj, i )
        pModel1[i] = vPiValues1[i] - '0';
    Abc_NtkForEachLatch( pNtk, pObj, i )
        pModel1[Abc_NtkPiNum(pNtk)+i] = ((int)(ABC_PTRINT_T)pObj->pData) - 1;

    pResult1 = Abc_NtkVerifySimulatePattern( pNtk, pModel1 );
    printf( "Value = %d\n", pResult1[0] );

    pModel2 = ABC_ALLOC( int, Abc_NtkCiNum(pNtk) );
    Abc_NtkForEachPi( pNtk, pObj, i )
        pModel2[i] = vPiValues2[i] - '0';
    Abc_NtkForEachLatch( pNtk, pObj, i )
        pModel2[Abc_NtkPiNum(pNtk)+i] = pResult1[Abc_NtkPoNum(pNtk)+i];

    pResult2 = Abc_NtkVerifySimulatePattern( pNtk, pModel2 );
    printf( "Value = %d\n", pResult2[0] );

    ABC_FREE( pModel1 );
    ABC_FREE( pModel2 );
    ABC_FREE( pResult1 );
    ABC_FREE( pResult2 );
}


/**Function*************************************************************

  Synopsis    [Returns the PO values under the given input pattern.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
int Abc_NtkIsTrueCex( Abc_Ntk_t * pNtk, Abc_Cex_t * pCex )
{
    extern Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
    Aig_Man_t * pMan;
    int status = 0, fStrashed = 0;
    if ( !Abc_NtkIsStrash(pNtk) )
    {
        pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
        fStrashed = 1;
    }
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan )
    {
        status = Saig_ManVerifyCex( pMan, pCex );
        Aig_ManStop( pMan );
    }
    if ( fStrashed )
        Abc_NtkDelete( pNtk );
    return status;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the number of PIs matches.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
int Abc_NtkIsValidCex( Abc_Ntk_t * pNtk, Abc_Cex_t * pCex )
{
    return Abc_NtkPiNum(pNtk) == pCex->nPis;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

