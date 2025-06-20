/**CFile****************************************************************

  FileName    [ioWriteBlifMv.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write BLIF-MV files.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioWriteBlifMv.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"
#include "base/main/main.h"
#include "map/mio/mio.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Io_NtkWriteBlifMv( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteBlifMvOne( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteBlifMvPis( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteBlifMvPos( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteBlifMvAsserts( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteBlifMvNodeFanins( FILE * pFile, Abc_Obj_t * pNode );
static void Io_NtkWriteBlifMvNode( FILE * pFile, Abc_Obj_t * pNode );
static void Io_NtkWriteBlifMvLatch( FILE * pFile, Abc_Obj_t * pLatch );
static void Io_NtkWriteBlifMvSubckt( FILE * pFile, Abc_Obj_t * pNode );
static void Io_NtkWriteBlifMvValues( FILE * pFile, Abc_Obj_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Write the network into a BLIF file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteBlifMv( Abc_Ntk_t * pNtk, char * FileName )
{
    FILE * pFile;
    Abc_Ntk_t * pNtkTemp;
    int i;
    assert( Abc_NtkIsNetlist(pNtk) );
    assert( Abc_NtkHasBlifMv(pNtk) );
    // start writing the file
    pFile = fopen( FileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteBlifMv(): Cannot open the output file.\n" );
        return;
    }
    fprintf( pFile, "# Benchmark \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );
    // write the master network
    Io_NtkWriteBlifMv( pFile, pNtk );
    // write the remaining networks
    if ( pNtk->pDesign )
    {
        Vec_PtrForEachEntry( Abc_Ntk_t *, pNtk->pDesign->vModules, pNtkTemp, i )
        {
            if ( pNtkTemp == pNtk )
                continue;
            fprintf( pFile, "\n\n" );
            Io_NtkWriteBlifMv( pFile, pNtkTemp );
        }
    }
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Write the network into a BLIF file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteBlifMv( FILE * pFile, Abc_Ntk_t * pNtk )
{
    assert( Abc_NtkIsNetlist(pNtk) );
    // write the model name
    fprintf( pFile, ".model %s\n", Abc_NtkName(pNtk) );
    // write the network
    Io_NtkWriteBlifMvOne( pFile, pNtk );
    // write EXDC network if it exists
    if ( Abc_NtkExdc(pNtk) )
        printf( "Io_NtkWriteBlifMv(): EXDC is not written.\n" );
    // finalize the file
    fprintf( pFile, ".end\n\n\n" );
}

/**Function*************************************************************

  Synopsis    [Write one network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteBlifMvOne( FILE * pFile, Abc_Ntk_t * pNtk )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pNode, * pTerm, * pLatch;
    int i;

    // write the PIs
    fprintf( pFile, ".inputs" );
    Io_NtkWriteBlifMvPis( pFile, pNtk );
    fprintf( pFile, "\n" );

    // write the POs
    fprintf( pFile, ".outputs" );
    Io_NtkWriteBlifMvPos( pFile, pNtk );
    fprintf( pFile, "\n" );

    // write the MV directives
    fprintf( pFile, "\n" );
    Abc_NtkForEachCi( pNtk, pTerm, i )
        if ( Abc_ObjMvVarNum(Abc_ObjFanout0(pTerm)) > 2 )
            fprintf( pFile, ".mv %s %d\n", Abc_ObjName(Abc_ObjFanout0(pTerm)), Abc_ObjMvVarNum(Abc_ObjFanout0(pTerm)) );
    Abc_NtkForEachCo( pNtk, pTerm, i )
        if ( Abc_ObjMvVarNum(Abc_ObjFanin0(pTerm)) > 2 )
            fprintf( pFile, ".mv %s %d\n", Abc_ObjName(Abc_ObjFanin0(pTerm)), Abc_ObjMvVarNum(Abc_ObjFanin0(pTerm)) );

    // write the blackbox
    if ( Abc_NtkHasBlackbox( pNtk ) )
    {
        fprintf( pFile, ".blackbox\n" );
        return;
    }

    // write the timing info
//    Io_WriteTimingInfo( pFile, pNtk );

    // write the latches
    if ( !Abc_NtkIsComb(pNtk) )
    {
        fprintf( pFile, "\n" );
        Abc_NtkForEachLatch( pNtk, pLatch, i )
            Io_NtkWriteBlifMvLatch( pFile, pLatch );
        fprintf( pFile, "\n" );
    }
/*
    // write the subcircuits
    assert( Abc_NtkWhiteboxNum(pNtk) == 0 );
    if ( Abc_NtkBlackboxNum(pNtk) > 0 )
    {
        fprintf( pFile, "\n" );
        Abc_NtkForEachBlackbox( pNtk, pNode, i )
            Io_NtkWriteBlifMvSubckt( pFile, pNode );
        fprintf( pFile, "\n" );
    }
*/
    if ( Abc_NtkBlackboxNum(pNtk) > 0 || Abc_NtkWhiteboxNum(pNtk) > 0 )
    {
        fprintf( pFile, "\n" );
        Abc_NtkForEachBox( pNtk, pNode, i )
        {
            if ( Abc_ObjIsLatch(pNode) )
                continue;
            Io_NtkWriteBlifMvSubckt( pFile, pNode );
        }
        fprintf( pFile, "\n" );
    }

    // write each internal node
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        Io_NtkWriteBlifMvNode( pFile, pNode );
    }
    Extra_ProgressBarStop( pProgress );
}


/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteBlifMvPis( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pTerm, * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i;

    LineLength  = 7;
    NameCounter = 0;

    Abc_NtkForEachPi( pNtk, pTerm, i )
    {
        pNet = Abc_ObjFanout0(pTerm);
        // get the line length after this name is written
        AddedLength = strlen(Abc_ObjName(pNet)) + 1;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, " \\\n" );
            // reset the line length
            LineLength  = 0;
            NameCounter = 0;
        }
        fprintf( pFile, " %s", Abc_ObjName(pNet) );
        LineLength += AddedLength;
        NameCounter++;
    }
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteBlifMvPos( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pTerm, * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i;

    LineLength  = 8;
    NameCounter = 0;

    Abc_NtkForEachPo( pNtk, pTerm, i )
    {
        pNet = Abc_ObjFanin0(pTerm);
        // get the line length after this name is written
        AddedLength = strlen(Abc_ObjName(pNet)) + 1;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, " \\\n" );
            // reset the line length
            LineLength  = 0;
            NameCounter = 0;
        }
        fprintf( pFile, " %s", Abc_ObjName(pNet) );
        LineLength += AddedLength;
        NameCounter++;
    }
}

/**Function*************************************************************

  Synopsis    [Write the latch into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteBlifMvLatch( FILE * pFile, Abc_Obj_t * pLatch )
{
    Abc_Obj_t * pNetLi, * pNetLo;
    int Reset;
    pNetLi = Abc_ObjFanin0( Abc_ObjFanin0(pLatch) );
    pNetLo = Abc_ObjFanout0( Abc_ObjFanout0(pLatch) );
    Reset  = (int)(ABC_PTRUINT_T)Abc_ObjData( pLatch );
    // write the latch line
    fprintf( pFile, ".latch" );
    fprintf( pFile, " %10s",    Abc_ObjName(pNetLi) );
    fprintf( pFile, " %10s",    Abc_ObjName(pNetLo) );
    fprintf( pFile, "\n" );
    // write the reset node
    fprintf( pFile, ".reset %s\n", Abc_ObjName(pNetLo) );
    fprintf( pFile, "%d\n", Reset-1 );
}

/**Function*************************************************************

  Synopsis    [Write the latch into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteBlifMvSubckt( FILE * pFile, Abc_Obj_t * pNode )
{
    Abc_Ntk_t * pModel = (Abc_Ntk_t *)pNode->pData;
    Abc_Obj_t * pTerm;
    int i;
    // write the MV directives
    fprintf( pFile, "\n" );
    Abc_ObjForEachFanin( pNode, pTerm, i )
        if ( Abc_ObjMvVarNum(pTerm) > 2 )
            fprintf( pFile, ".mv %s %d\n", Abc_ObjName(pTerm), Abc_ObjMvVarNum(pTerm) );
    Abc_ObjForEachFanout( pNode, pTerm, i )
        if ( Abc_ObjMvVarNum(pTerm) > 2 )
            fprintf( pFile, ".mv %s %d\n", Abc_ObjName(pTerm), Abc_ObjMvVarNum(pTerm) );
    // write the subcircuit
    fprintf( pFile, ".subckt %s %s", Abc_NtkName(pModel), Abc_ObjName(pNode) );
    // write pairs of the formal=actual names
    Abc_NtkForEachPi( pModel, pTerm, i )
    {
        fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanout0(pTerm)) );
        pTerm = Abc_ObjFanin( pNode, i );
        fprintf( pFile, "=%s", Abc_ObjName(Abc_ObjFanin0(pTerm)) );
    }
    Abc_NtkForEachPo( pModel, pTerm, i )
    {
        fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanin0(pTerm)) );
        pTerm = Abc_ObjFanout( pNode, i );
        fprintf( pFile, "=%s", Abc_ObjName(Abc_ObjFanout0(pTerm)) );
    }
    fprintf( pFile, "\n" );
}


/**Function*************************************************************

  Synopsis    [Write the node into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteBlifMvNode( FILE * pFile, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin;
    char * pCur;
    int nValues, iFanin, i;

    // write .mv directives for the fanins
    fprintf( pFile, "\n" );
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
//        nValues = atoi(pCur);
        nValues = Abc_ObjMvVarNum( pFanin );
        if ( nValues > 2 )
            fprintf( pFile, ".mv %s %d\n", Abc_ObjName(pFanin), nValues );
//        while ( *pCur++ != ' ' );
    }

    // write .mv directives for the node
//    nValues = atoi(pCur);
    nValues = Abc_ObjMvVarNum( Abc_ObjFanout0(pNode) );
    if ( nValues > 2 )
        fprintf( pFile, ".mv %s %d\n", Abc_ObjName(Abc_ObjFanout0(pNode)), nValues );
//    while ( *pCur++ != '\n' );

    // write the .names line
    fprintf( pFile, ".table" );
    Io_NtkWriteBlifMvNodeFanins( pFile, pNode );
    fprintf( pFile, "\n" );

    // write the cubes
    pCur = (char *)Abc_ObjData(pNode);
    if ( *pCur == 'd' )
    {
        fprintf( pFile, ".default " );
        pCur++;
    }
    // write the literals
    for ( ; *pCur; pCur++ )
    {
        fprintf( pFile, "%c", *pCur );
        if ( *pCur != '=' )
            continue;
        // get the number
        iFanin = atoi( pCur+1 );
        fprintf( pFile, "%s", Abc_ObjName(Abc_ObjFanin(pNode,iFanin)) );
        // scroll on to the next symbol
        while ( *pCur != ' ' && *pCur != '\n' )
            pCur++;
        pCur--;
    }
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteBlifMvNodeFanins( FILE * pFile, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    char * pName;
    int i;

    LineLength  = 6;
    NameCounter = 0;
    Abc_ObjForEachFanin( pNode, pNet, i )
    {
        // get the fanin name
        pName = Abc_ObjName(pNet);
        // get the line length after the fanin name is written
        AddedLength = strlen(pName) + 1;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, " \\\n" );
            // reset the line length
            LineLength  = 0;
            NameCounter = 0;
        }
        fprintf( pFile, " %s", pName );
        LineLength += AddedLength;
        NameCounter++;
    }

    // get the output name
    pName = Abc_ObjName(Abc_ObjFanout0(pNode));
    // get the line length after the output name is written
    AddedLength = strlen(pName) + 1;
    if ( NameCounter && LineLength + AddedLength > 75 )
    { // write the line extender
        fprintf( pFile, " \\\n" );
        // reset the line length
        LineLength  = 0;
        NameCounter = 0;
    }
    fprintf( pFile, " %s", pName );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

