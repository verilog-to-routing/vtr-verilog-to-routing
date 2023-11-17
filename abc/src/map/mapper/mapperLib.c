/**CFile****************************************************************

  FileName    [mapperLib.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapperLib.c,v 1.6 2005/01/23 06:59:44 alanmi Exp $]

***********************************************************************/
//#define _BSD_SOURCE

#ifndef WIN32
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <unistd.h>
#endif

#include "mapperInt.h"
#include "map/super/super.h"
#include "map/mapper/mapperInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads in the supergate library and prepares it for use.]

  Description [The supergates library comes in a .super file. This file
  contains descriptions of supergates along with some relevant information.
  This procedure reads the supergate file, canonicizes the supergates,
  and constructs an additional lookup table, which can be used to map
  truth tables of the cuts into the pair (phase, supergate). The phase
  indicates how the current truth table should be phase assigned to 
  match the canonical form of the supergate. The resulting phase is the
  bitwise EXOR of the phase needed to canonicize the supergate and the
  phase needed to transform the truth table into its canonical form.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_SuperLib_t * Map_SuperLibCreate( Mio_Library_t * pGenlib, Vec_Str_t * vStr, char * pFileName, char * pExcludeFile, int fAlgorithm, int fVerbose )
{
    Map_SuperLib_t * p;
    abctime clk;

    // start the supergate library
    p = ABC_ALLOC( Map_SuperLib_t, 1 );
    memset( p, 0, sizeof(Map_SuperLib_t) );
    p->pName     = Abc_UtilStrsav(pFileName);
    p->fVerbose  = fVerbose;
    p->mmSupers  = Extra_MmFixedStart( sizeof(Map_Super_t) );
    p->mmEntries = Extra_MmFixedStart( sizeof(Map_HashEntry_t) );
    p->mmForms   = Extra_MmFlexStart();
    Map_MappingSetupTruthTables( p->uTruths );

    // start the hash table
    p->tTableC = Map_SuperTableCreate( p );
    p->tTable  = Map_SuperTableCreate( p );

    // read the supergate library from file
clk = Abc_Clock();
    if ( vStr != NULL )
    {
        // read the supergate library from file
        int Status = Map_LibraryReadFileTreeStr( p, pGenlib, vStr, pFileName );
        if ( Status == 0 )
        {
            Map_SuperLibFree( p );
            return NULL;
        }
        // prepare the info about the library
        Status = Map_LibraryDeriveGateInfo( p, NULL );
        if ( Status == 0 )
        {
            Map_SuperLibFree( p );
            return NULL;
        }
        assert( p->nVarsMax > 0 );
    }
    else if ( fAlgorithm )
    {
        if ( !Map_LibraryReadTree( p, pGenlib, pFileName, pExcludeFile ) )
        {
            Map_SuperLibFree( p );
            return NULL;
        }
    }
    else
    {
        if ( pExcludeFile != 0 )
        {
            Map_SuperLibFree( p );
            printf ("Error: Exclude file support not present for old format. Stop.\n");
            return NULL;
        }
        if ( !Map_LibraryRead( p, pFileName ) )
        {
            Map_SuperLibFree( p );
            return NULL;
        }
    }
    assert( p->nVarsMax > 0 );

    // report the stats
    if ( fVerbose ) 
    {
        printf( "Loaded %d unique %d-input supergates from \"%s\".  ", 
            p->nSupersReal, p->nVarsMax, pFileName );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }

    // assign the interver parameters
    p->pGateInv        = Mio_LibraryReadInv( p->pGenlib );
    p->tDelayInv.Rise  = Mio_LibraryReadDelayInvRise( p->pGenlib );
    p->tDelayInv.Fall  = Mio_LibraryReadDelayInvFall( p->pGenlib );
    p->tDelayInv.Worst = MAP_MAX( p->tDelayInv.Rise, p->tDelayInv.Fall );
    p->AreaInv         = Mio_LibraryReadAreaInv( p->pGenlib );
    p->AreaBuf         = Mio_LibraryReadAreaBuf( p->pGenlib );

    // assign the interver supergate
    p->pSuperInv = (Map_Super_t *)Extra_MmFixedEntryFetch( p->mmSupers );
    memset( p->pSuperInv, 0, sizeof(Map_Super_t) );
    p->pSuperInv->Num         = -1;
    p->pSuperInv->nGates      =  1;
    p->pSuperInv->nFanins     =  1;
    p->pSuperInv->nFanLimit   = 10;
    p->pSuperInv->pFanins[0]  = p->ppSupers[0];
    p->pSuperInv->pRoot       = p->pGateInv;
    p->pSuperInv->Area        = p->AreaInv;
    p->pSuperInv->tDelayMax   = p->tDelayInv;
    p->pSuperInv->tDelaysR[0].Rise = MAP_NO_VAR;
    p->pSuperInv->tDelaysR[0].Fall = p->tDelayInv.Rise;
    p->pSuperInv->tDelaysF[0].Rise = p->tDelayInv.Fall;
    p->pSuperInv->tDelaysF[0].Fall = MAP_NO_VAR;
    return p;
}


/**Function*************************************************************

  Synopsis    [Deallocates the supergate library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_SuperLibFree( Map_SuperLib_t * p )
{
    if ( p == NULL ) return;
    if ( p->pGenlib )
    {
        if ( p->pGenlib != Abc_FrameReadLibGen() )
            Mio_LibraryDelete( p->pGenlib );
        p->pGenlib = NULL;
    }
    if ( p->tTableC )
        Map_SuperTableFree( p->tTableC );
    if ( p->tTable )
        Map_SuperTableFree( p->tTable );
    Extra_MmFixedStop( p->mmSupers );
    Extra_MmFixedStop( p->mmEntries );
    Extra_MmFlexStop( p->mmForms );
    ABC_FREE( p->ppSupers );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Derives the library from the genlib library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_SuperLibDeriveFromGenlib( Mio_Library_t * pLib, int fVerbose )
{
    Map_SuperLib_t * pLibSuper;
    Vec_Str_t * vStr;
    char * pFileName;
    if ( pLib == NULL )
        return 0;

    // compute supergates
    vStr = Super_PrecomputeStr( pLib, 5, 1, 100000000, 10000000, 10000000, 100, 1, 0 );
    if ( vStr == NULL )
        return 0;

    // create supergate library
    pFileName = Extra_FileNameGenericAppend( Mio_LibraryReadName(pLib), ".super" );
    pLibSuper = Map_SuperLibCreate( pLib, vStr, pFileName, NULL, 1, 0 );
    Vec_StrFree( vStr );

    // replace the library
    Map_SuperLibFree( (Map_SuperLib_t *)Abc_FrameReadLibSuper() );
    Abc_FrameSetLibSuper( pLibSuper );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Derives the library from the genlib library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_SuperLibDeriveFromGenlib2( Mio_Library_t * pLib, int fVerbose )
{
    Abc_Frame_t * pAbc = Abc_FrameGetGlobalFrame();
    char * pFileName;
    if ( pLib == NULL )
        return 0;
    // compute supergates
    pFileName = Extra_FileNameGenericAppend(Mio_LibraryReadName(pLib), ".super");
    Super_Precompute( pLib, 5, 1, 100000000, 10000000, 10000000, 100, 1, 0, pFileName );
    // assuming that it terminated successfully
    if ( Cmd_CommandExecute( pAbc, pFileName ) )
    {
        fprintf( stdout, "Cannot execute command \"read_super %s\".\n", pFileName );
        return 0;
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

