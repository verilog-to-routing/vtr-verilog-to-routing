/**CFile****************************************************************

  FileName    [cbaWriteBlif.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Verilog parser.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: cbaWriteBlif.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cba.h"
#include "cbaPrs.h"
#include "map/mio/mio.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writing parser state into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Prs_ManWriteBlifArray( FILE * pFile, Prs_Ntk_t * p, Vec_Int_t * vFanins )
{
    int i, NameId;
    Vec_IntForEachEntry( vFanins, NameId, i )
        fprintf( pFile, " %s", Prs_NtkStr(p, NameId) );
    fprintf( pFile, "\n" );
}
static void Prs_ManWriteBlifLines( FILE * pFile, Prs_Ntk_t * p )
{
    Vec_Int_t * vBox; 
    int i, k, FormId, ActId;
    Prs_NtkForEachBox( p, vBox, i )
    {
        int NtkId = Prs_BoxNtk(p, i);
        assert( Prs_BoxIONum(p, i) > 0 );
        assert( Vec_IntSize(vBox) % 2 == 0 );
        if ( NtkId == -1 ) // latch
        {
            fprintf( pFile, ".latch" );
            fprintf( pFile, " %s", Prs_NtkStr(p, Vec_IntEntry(vBox, 1)) );
            fprintf( pFile, " %s", Prs_NtkStr(p, Vec_IntEntry(vBox, 3)) );
            fprintf( pFile, " %c\n", '0' + Prs_BoxName(p, i) );
        }
        else if ( Prs_BoxIsNode(p, i) ) // node
        {
            fprintf( pFile, ".names" );
            Vec_IntForEachEntryDouble( vBox, FormId, ActId, k )
                fprintf( pFile, " %s", Prs_NtkStr(p, ActId) );
            fprintf( pFile, "\n%s", Prs_NtkSop(p, NtkId) );
        }
        else // box
        {
            fprintf( pFile, ".subckt" );
            fprintf( pFile, " %s", Prs_NtkStr(p, NtkId) );
            Vec_IntForEachEntryDouble( vBox, FormId, ActId, k )
                fprintf( pFile, " %s=%s", Prs_NtkStr(p, FormId), Prs_NtkStr(p, ActId) );
            fprintf( pFile, "\n" );
        }
    }
}
static void Prs_ManWriteBlifNtk( FILE * pFile, Prs_Ntk_t * p )
{
    // write header
    fprintf( pFile, ".model %s\n", Prs_NtkStr(p, p->iModuleName) );
    if ( Vec_IntSize(&p->vInouts) )
    fprintf( pFile, ".inouts" );
    if ( Vec_IntSize(&p->vInouts) )
    Prs_ManWriteBlifArray( pFile, p, &p->vInouts );
    fprintf( pFile, ".inputs" );
    Prs_ManWriteBlifArray( pFile, p, &p->vInputs );
    fprintf( pFile, ".outputs" );
    Prs_ManWriteBlifArray( pFile, p, &p->vOutputs );
    // write objects
    Prs_ManWriteBlifLines( pFile, p );
    fprintf( pFile, ".end\n\n" );
}
void Prs_ManWriteBlif( char * pFileName, Vec_Ptr_t * vPrs )
{
    Prs_Ntk_t * pNtk = Prs_ManRoot(vPrs);
    FILE * pFile = fopen( pFileName, "wb" ); int i;
    if ( pFile == NULL )
    {
        printf( "Cannot open output file \"%s\".\n", pFileName );
        return;
    }
    fprintf( pFile, "# Design \"%s\" written by ABC on %s\n\n", Prs_NtkStr(pNtk, pNtk->iModuleName), Extra_TimeStamp() );
    Vec_PtrForEachEntry( Prs_Ntk_t *, vPrs, pNtk, i )
        Prs_ManWriteBlifNtk( pFile, pNtk );
    fclose( pFile );
}


/**Function*************************************************************

  Synopsis    [Write elaborated design.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cba_ManWriteBlifLines( FILE * pFile, Cba_Ntk_t * p )
{
    int k, iObj, iFin, iFon;
    Cba_NtkForEachBox( p, iObj )
    {
        if ( Cba_ObjIsBoxUser(p, iObj) )
        {
            Cba_Ntk_t * pNtk = Cba_ObjNtk( p, iObj );
            fprintf( pFile, ".subckt" );
            fprintf( pFile, " %s", Cba_NtkName(pNtk) );
            Cba_ObjForEachFinFon( p, iObj, iFin, iFon, k )
                fprintf( pFile, " %s=%s", Cba_ObjNameStr(pNtk, Cba_NtkPi(pNtk, k)), Cba_FonNameStr(p, iFon) );
            Cba_ObjForEachFon( p, iObj, iFon, k )
                fprintf( pFile, " %s=%s", Cba_ObjNameStr(pNtk, Cba_NtkPo(pNtk, k)), Cba_FonNameStr(p, iFon) );
            fprintf( pFile, "\n" );
        }
        else if ( Cba_ObjIsGate(p, iObj) )
        {
            char * pGateName = Abc_NamStr(p->pDesign->pMods, Cba_ObjNtkId( p, iObj ));
            Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
            Mio_Gate_t * pGate = Mio_LibraryReadGateByName( pLib, pGateName, NULL );
            fprintf( pFile, ".gate %s", pGateName );
            Cba_ObjForEachFinFon( p, iObj, iFin, iFon, k )
                fprintf( pFile, " %s=%s", Mio_GateReadPinName(pGate, k), Cba_FonNameStr(p, iFon) );
            Cba_ObjForEachFon( p, iObj, iFon, k )
                fprintf( pFile, " %s=%s", Mio_GateReadOutName(pGate), Cba_FonNameStr(p, iFon) );
            fprintf( pFile, "\n" );
        }
        else
        {
            fprintf( pFile, ".names" );
            Cba_ObjForEachFinFon( p, iObj, iFin, iFon, k )
                fprintf( pFile, " %s", Cba_FonNameStr(p, iFon) );
            fprintf( pFile, " %s", Cba_FonNameStr(p, Cba_ObjFon0(p, iObj)) );
            fprintf( pFile, "\n%s",  Cba_NtkSop(p, Cba_ObjFunc(p, iObj)) );
        }
    }
}
void Cba_ManWriteBlifArray( FILE * pFile, Cba_Ntk_t * p, Vec_Int_t * vObjs )
{
    int iObj, i;
    Vec_IntForEachEntry( vObjs, iObj, i )
        fprintf( pFile, " %s", Cba_ObjNameStr(p, iObj) );
    fprintf( pFile, "\n" );
}
void Cba_ManWriteBlifNtk( FILE * pFile, Cba_Ntk_t * p )
{
    // write header
    fprintf( pFile, ".model %s\n", Cba_NtkName(p) );
    fprintf( pFile, ".inputs" );
    Cba_ManWriteBlifArray( pFile, p, &p->vInputs );
    fprintf( pFile, ".outputs" );
    Cba_ManWriteBlifArray( pFile, p, &p->vOutputs );
    // write objects
    Cba_ManWriteBlifLines( pFile, p );
    fprintf( pFile, ".end\n\n" );
}
void Cba_ManWriteBlif( char * pFileName, Cba_Man_t * p )
{
    FILE * pFile;
    Cba_Ntk_t * pNtk; 
    int i;
    // check the library
    if ( p->pMioLib && p->pMioLib != Abc_FrameReadLibGen() )
    {
        printf( "Genlib library used in the mapped design is not longer a current library.\n" );
        return;
    }
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open output file \"%s\".\n", pFileName );
        return;
    }
    fprintf( pFile, "# Design \"%s\" written via CBA package in ABC on %s\n\n", Cba_ManName(p), Extra_TimeStamp() );
//    Cba_ManAssignInternWordNames( p );
    Cba_ManForEachNtk( p, pNtk, i )
        Cba_ManWriteBlifNtk( pFile, pNtk );
    fclose( pFile );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

