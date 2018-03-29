/**CFile****************************************************************

  FileName    [ Fxch.c ]

  PackageName [ Fast eXtract with Cube Hashing (FXCH) ]

  Synopsis    [ The entrance into the fast extract module. ]

  Author      [ Bruno Schmitt - boschmitt at inf.ufrgs.br ]

  Affiliation [ UFRGS ]

  Date        [ Ver. 1.0. Started - March 6, 2016. ]

  Revision    []

***********************************************************************/
#include "Fxch.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxch_CubesGruping(Fxch_Man_t* pFxchMan)
{
    Vec_Int_t* vCube;
    int iCube, nOutputs, SizeOutputID;
    Hsh_VecMan_t* pCubeHash;

    /* Identify the number of Outputs and create the translation table */
    pFxchMan->vTranslation = Vec_IntAlloc( 32 );
    Vec_WecForEachLevel( pFxchMan->vCubes, vCube, iCube )
    {
        int Id = Vec_IntEntry( vCube, 0 );
        int iTranslation = Vec_IntFind( pFxchMan->vTranslation, Id );

        if ( iTranslation == -1 )
            Vec_IntPush( pFxchMan->vTranslation, Id );
    }
    nOutputs = Vec_IntSize( pFxchMan->vTranslation );

    /* Size of the OutputID in number o ints */
    SizeOutputID = ( nOutputs >> 5 ) + ( ( nOutputs & 31 ) > 0 );

    /* Initialize needed structures */
    pFxchMan->vOutputID = Vec_IntAlloc( 4096 );
    pFxchMan->pTempOutputID = ABC_CALLOC( int, SizeOutputID );
    pFxchMan->nSizeOutputID = SizeOutputID;

    pCubeHash =  Hsh_VecManStart( 1024 );

    /* Identify equal cubes */
    Vec_WecForEachLevel( pFxchMan->vCubes, vCube, iCube )
    {
        int Id = Vec_IntEntry( vCube, 0 );
        int iTranslation = Vec_IntFind( pFxchMan->vTranslation, Id );
        int i, iCubeNoID, Temp, * pEntry;
        Vec_IntWriteEntry( vCube, 0, 0 ); // Clear ID, Outputs will be identified by it later

        iCubeNoID = Hsh_VecManAdd( pCubeHash, vCube );
        Temp = ( 1 << ( iTranslation & 31 ) );
        if ( iCubeNoID == Vec_IntSize( pFxchMan->vOutputID ) / SizeOutputID )
        {
            for ( i = 0; i < SizeOutputID; i++ )
                pFxchMan->pTempOutputID[i] = 0;

            pFxchMan->pTempOutputID[ iTranslation >> 5 ] = Temp;
            Vec_IntPushArray( pFxchMan->vOutputID, pFxchMan->pTempOutputID, SizeOutputID ); 
        }
        else
        {
            Vec_IntClear( vCube );
            pEntry = Vec_IntEntryP( pFxchMan->vOutputID, ( iCubeNoID * SizeOutputID ) + ( iTranslation >> 5 ) );
            *pEntry |= Temp;
        }
    }

    Hsh_VecManStop( pCubeHash );
    Vec_WecRemoveEmpty( pFxchMan->vCubes );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxch_CubesUnGruping(Fxch_Man_t* pFxchMan)
{
    int iCube;
    int i, j;
    Vec_Int_t* vCube;
    Vec_Int_t* vNewCube;

    assert( Vec_WecSize( pFxchMan->vCubes ) == ( Vec_IntSize( pFxchMan->vOutputID ) / pFxchMan->nSizeOutputID ) );
    Vec_WecForEachLevel( pFxchMan->vCubes, vCube, iCube )
    {
        int * pOutputID, nOnes;
        if ( Vec_IntSize( vCube ) == 0 || Vec_IntEntry( vCube, 0 ) != 0 )
            continue;

        pOutputID = Vec_IntEntryP( pFxchMan->vOutputID, iCube * pFxchMan->nSizeOutputID );
        nOnes = 0;

        for ( i = 0; i < pFxchMan->nSizeOutputID; i++ )
            nOnes += Fxch_CountOnes( (unsigned int) pOutputID[i] );

        for ( i = 0; i < pFxchMan->nSizeOutputID && nOnes; i++ )
            for ( j = 0; j < 32 && nOnes; j++ )
                if ( pOutputID[i] & ( 1 << j ) )
                {
                    if ( nOnes == 1 )
                        Vec_IntWriteEntry( vCube, 0, Vec_IntEntry( pFxchMan->vTranslation, ( i << 5 ) | j ) );
                    else
                    {
                        vNewCube = Vec_WecPushLevel( pFxchMan->vCubes );
                        Vec_IntAppend( vNewCube, vCube );
                        Vec_IntWriteEntry( vNewCube, 0, Vec_IntEntry( pFxchMan->vTranslation, (i << 5 ) | j ) );
                    }
                    nOnes -= 1;
                }
    }

    Vec_IntFree( pFxchMan->vTranslation );
    Vec_IntFree( pFxchMan->vOutputID );
    ABC_FREE( pFxchMan->pTempOutputID );
    return;
}

/**Function*************************************************************

  Synopsis    [ Performs fast extract with cube hashing on a set
                of covers. ]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxch_FastExtract( Vec_Wec_t* vCubes,
                      int ObjIdMax,
                      int nMaxDivExt,
                      int fVerbose,
                      int fVeryVerbose )
{
    abctime TempTime;
    Fxch_Man_t* pFxchMan = Fxch_ManAlloc( vCubes );
    int i;

    TempTime = Abc_Clock();
    Fxch_CubesGruping( pFxchMan );
    Fxch_ManMapLiteralsIntoCubes( pFxchMan, ObjIdMax );
    Fxch_ManGenerateLitHashKeys( pFxchMan );
    Fxch_ManComputeLevel( pFxchMan );
    Fxch_ManSCHashTablesInit( pFxchMan );
    Fxch_ManDivCreate( pFxchMan );
    pFxchMan->timeInit = Abc_Clock() - TempTime;

    if ( fVeryVerbose )
        Fxch_ManPrintDivs( pFxchMan );

    if ( fVerbose )
        Fxch_ManPrintStats( pFxchMan );

    TempTime = Abc_Clock();
    
    for ( i = 0; (!nMaxDivExt || i < nMaxDivExt) && Vec_QueTopPriority( pFxchMan->vDivPrio ) > 0.0; i++ )
    {
        int iDiv = Vec_QuePop( pFxchMan->vDivPrio );

        if ( fVeryVerbose )
            Fxch_DivPrint( pFxchMan, iDiv );

        Fxch_ManUpdate( pFxchMan, iDiv );
    }
   
    pFxchMan->timeExt = Abc_Clock() - TempTime;

    if ( fVerbose )
    {
        Fxch_ManPrintStats( pFxchMan );
        Abc_PrintTime( 1, "\n[FXCH] Elapsed Time", pFxchMan->timeInit + pFxchMan->timeExt );
        Abc_PrintTime( 1, "[FXCH]    +-> Init", pFxchMan->timeInit );
        Abc_PrintTime( 1, "[FXCH]    +-> Extr", pFxchMan->timeExt );
    }

    Fxch_CubesUnGruping( pFxchMan );
    Fxch_ManSCHashTablesFree( pFxchMan );
    Fxch_ManFree( pFxchMan );

    Vec_WecRemoveEmpty( vCubes );
    Vec_WecSortByFirstInt( vCubes, 0 );

    return 1;
}

/**Function*************************************************************

  Synopsis    [ Retrives the necessary information for the fast extract
                with cube hashing. ]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkFxchPerform( Abc_Ntk_t* pNtk,
                        int nMaxDivExt,
                        int fVerbose,
                        int fVeryVerbose )
{
    Vec_Wec_t* vCubes;

    assert( Abc_NtkIsSopLogic( pNtk ) );

    if ( !Abc_NtkFxCheck( pNtk ) )
    {
        printf( "Abc_NtkFxchPerform(): Nodes have duplicated fanins. FXCH is not performed.\n" );
        return 0;
    }

    vCubes = Abc_NtkFxRetrieve( pNtk );
    if ( Fxch_FastExtract( vCubes, Abc_NtkObjNumMax( pNtk ), nMaxDivExt, fVerbose, fVeryVerbose ) > 0 )
    {
        Abc_NtkFxInsert( pNtk, vCubes );
        Vec_WecFree( vCubes );

        if ( !Abc_NtkCheck( pNtk ) )
            printf( "Abc_NtkFxchPerform(): The network check has failed.\n" );

        return 1;
    }
    else
        printf( "Warning: The network has not been changed by \"fxch\".\n" );

    Vec_WecFree( vCubes );

    return 0;
}
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
