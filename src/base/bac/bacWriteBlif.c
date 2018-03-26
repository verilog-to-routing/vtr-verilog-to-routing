/**CFile****************************************************************

  FileName    [bacWriteBlif.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Verilog parser.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: bacWriteBlif.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bac.h"
#include "bacPrs.h"
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
static void Psr_ManWriteBlifArray( FILE * pFile, Psr_Ntk_t * p, Vec_Int_t * vFanins )
{
    int i, NameId;
    Vec_IntForEachEntry( vFanins, NameId, i )
        fprintf( pFile, " %s", Psr_NtkStr(p, NameId) );
    fprintf( pFile, "\n" );
}
static void Psr_ManWriteBlifLines( FILE * pFile, Psr_Ntk_t * p )
{
    Vec_Int_t * vBox; 
    int i, k, FormId, ActId;
    Psr_NtkForEachBox( p, vBox, i )
    {
        int NtkId = Psr_BoxNtk(p, i);
        assert( Psr_BoxIONum(p, i) > 0 );
        assert( Vec_IntSize(vBox) % 2 == 0 );
        if ( NtkId == -1 ) // latch
        {
            fprintf( pFile, ".latch" );
            fprintf( pFile, " %s", Psr_NtkStr(p, Vec_IntEntry(vBox, 1)) );
            fprintf( pFile, " %s", Psr_NtkStr(p, Vec_IntEntry(vBox, 3)) );
            fprintf( pFile, " %c\n", '0' + Psr_BoxName(p, i) );
        }
        else if ( Psr_BoxIsNode(p, i) ) // node
        {
            fprintf( pFile, ".names" );
            Vec_IntForEachEntryDouble( vBox, FormId, ActId, k )
                fprintf( pFile, " %s", Psr_NtkStr(p, ActId) );
            fprintf( pFile, "\n%s", Psr_NtkStr(p, NtkId) );
        }
        else // box
        {
            fprintf( pFile, ".subckt" );
            fprintf( pFile, " %s", Psr_NtkStr(p, NtkId) );
            Vec_IntForEachEntryDouble( vBox, FormId, ActId, k )
                fprintf( pFile, " %s=%s", Psr_NtkStr(p, FormId), Psr_NtkStr(p, ActId) );
            fprintf( pFile, "\n" );
        }
    }
}
static void Psr_ManWriteBlifNtk( FILE * pFile, Psr_Ntk_t * p )
{
    // write header
    fprintf( pFile, ".model %s\n", Psr_NtkStr(p, p->iModuleName) );
    if ( Vec_IntSize(&p->vInouts) )
    fprintf( pFile, ".inouts" );
    if ( Vec_IntSize(&p->vInouts) )
    Psr_ManWriteBlifArray( pFile, p, &p->vInouts );
    fprintf( pFile, ".inputs" );
    Psr_ManWriteBlifArray( pFile, p, &p->vInputs );
    fprintf( pFile, ".outputs" );
    Psr_ManWriteBlifArray( pFile, p, &p->vOutputs );
    // write objects
    Psr_ManWriteBlifLines( pFile, p );
    fprintf( pFile, ".end\n\n" );
}
void Psr_ManWriteBlif( char * pFileName, Vec_Ptr_t * vPrs )
{
    Psr_Ntk_t * pNtk = Psr_ManRoot(vPrs);
    FILE * pFile = fopen( pFileName, "wb" ); int i;
    if ( pFile == NULL )
    {
        printf( "Cannot open output file \"%s\".\n", pFileName );
        return;
    }
    fprintf( pFile, "# Design \"%s\" written by ABC on %s\n\n", Psr_NtkStr(pNtk, pNtk->iModuleName), Extra_TimeStamp() );
    Vec_PtrForEachEntry( Psr_Ntk_t *, vPrs, pNtk, i )
        Psr_ManWriteBlifNtk( pFile, pNtk );
    fclose( pFile );
}



/**Function*************************************************************

  Synopsis    [Write elaborated design.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bac_ManWriteBlifGate( FILE * pFile, Bac_Ntk_t * p, Mio_Gate_t * pGate, Vec_Int_t * vFanins, int iObj )
{
    int iFanin, i;
    Vec_IntForEachEntry( vFanins, iFanin, i )
        fprintf( pFile, " %s=%s", Mio_GateReadPinName(pGate, i), Bac_ObjNameStr(p, iFanin) );
    fprintf( pFile, " %s=%s", Mio_GateReadOutName(pGate), Bac_ObjNameStr(p, iObj) );
    fprintf( pFile, "\n" );
}
void Bac_ManWriteBlifArray( FILE * pFile, Bac_Ntk_t * p, Vec_Int_t * vFanins, int iObj )
{
    int iFanin, i;
    Vec_IntForEachEntry( vFanins, iFanin, i )
        fprintf( pFile, " %s", Bac_ObjNameStr(p, iFanin) );
    if ( iObj >= 0 )
        fprintf( pFile, " %s", Bac_ObjNameStr(p, iObj) );
    fprintf( pFile, "\n" );
}
void Bac_ManWriteBlifArray2( FILE * pFile, Bac_Ntk_t * p, int iObj )
{
    int iTerm, i;
    Bac_Ntk_t * pModel = Bac_BoxNtk( p, iObj );
    Bac_NtkForEachPi( pModel, iTerm, i )
        fprintf( pFile, " %s=%s", Bac_ObjNameStr(pModel, iTerm), Bac_ObjNameStr(p, Bac_BoxBi(p, iObj, i)) );
    Bac_NtkForEachPo( pModel, iTerm, i )
        fprintf( pFile, " %s=%s", Bac_ObjNameStr(pModel, iTerm), Bac_ObjNameStr(p, Bac_BoxBo(p, iObj, i)) );
    fprintf( pFile, "\n" );
}
void Bac_ManWriteBlifLines( FILE * pFile, Bac_Ntk_t * p )
{
    int i, k, iTerm;
    Bac_NtkForEachBox( p, i )
    {
        if ( Bac_ObjIsBoxUser(p, i) )
        {
            fprintf( pFile, ".subckt" );
            fprintf( pFile, " %s", Bac_NtkName(Bac_BoxNtk(p, i)) );
            Bac_ManWriteBlifArray2( pFile, p, i );
        }
        else if ( Bac_ObjIsGate(p, i) )
        {
            char * pGateName = Abc_NamStr(p->pDesign->pMods, Bac_BoxNtkId(p, i));
            Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
            Mio_Gate_t * pGate = Mio_LibraryReadGateByName( pLib, pGateName, NULL );
            fprintf( pFile, ".gate %s", pGateName );
            Bac_BoxForEachBi( p, i, iTerm, k )
                fprintf( pFile, " %s=%s", Mio_GateReadPinName(pGate, k), Bac_ObjNameStr(p, iTerm) );
            Bac_BoxForEachBo( p, i, iTerm, k )
                fprintf( pFile, " %s=%s", Mio_GateReadOutName(pGate), Bac_ObjNameStr(p, iTerm) );
            fprintf( pFile, "\n" );
        }
        else
        {
            fprintf( pFile, ".names" );
            Bac_BoxForEachBi( p, i, iTerm, k )
                fprintf( pFile, " %s", Bac_ObjNameStr(p, Bac_ObjFanin(p, iTerm)) );
            Bac_BoxForEachBo( p, i, iTerm, k )
                fprintf( pFile, " %s", Bac_ObjNameStr(p, iTerm) );
            fprintf( pFile, "\n%s",  Ptr_TypeToSop(Bac_ObjType(p, i)) );
        }
    }
}
void Bac_ManWriteBlifNtk( FILE * pFile, Bac_Ntk_t * p )
{
    assert( Vec_IntSize(&p->vFanin) == Bac_NtkObjNum(p) );
    // write header
    fprintf( pFile, ".model %s\n", Bac_NtkName(p) );
    fprintf( pFile, ".inputs" );
    Bac_ManWriteBlifArray( pFile, p, &p->vInputs, -1 );
    fprintf( pFile, ".outputs" );
    Bac_ManWriteBlifArray( pFile, p, &p->vOutputs, -1 );
    // write objects
    Bac_ManWriteBlifLines( pFile, p );
    fprintf( pFile, ".end\n\n" );
}
void Bac_ManWriteBlif( char * pFileName, Bac_Man_t * p )
{
    FILE * pFile;
    Bac_Ntk_t * pNtk; 
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
    fprintf( pFile, "# Design \"%s\" written via CBA package in ABC on %s\n\n", Bac_ManName(p), Extra_TimeStamp() );
    Bac_ManAssignInternWordNames( p );
    Bac_ManForEachNtk( p, pNtk, i )
        Bac_ManWriteBlifNtk( pFile, pNtk );
    fclose( pFile );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

