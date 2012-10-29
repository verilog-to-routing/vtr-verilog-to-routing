/**CFile****************************************************************

  FileName    [ioWriteBlif.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write BLIF files.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioWriteBlif.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "io.h"
#include "main.h"
#include "mio.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Io_NtkWrite( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches );
static void Io_NtkWriteOne( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches );
static void Io_NtkWritePis( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches );
static void Io_NtkWritePos( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches );
static void Io_NtkWriteSubckt( FILE * pFile, Abc_Obj_t * pNode );
static void Io_NtkWriteAsserts( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteNodeGate( FILE * pFile, Abc_Obj_t * pNode, int Length );
static void Io_NtkWriteNodeFanins( FILE * pFile, Abc_Obj_t * pNode );
static void Io_NtkWriteNode( FILE * pFile, Abc_Obj_t * pNode, int Length );
static void Io_NtkWriteLatch( FILE * pFile, Abc_Obj_t * pLatch );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Write the network into a BLIF file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteBlifLogic( Abc_Ntk_t * pNtk, char * FileName, int fWriteLatches )
{
    Abc_Ntk_t * pNtkTemp;
    // derive the netlist
    pNtkTemp = Abc_NtkToNetlist(pNtk);
    if ( pNtkTemp == NULL )
    {
        fprintf( stdout, "Writing BLIF has failed.\n" );
        return;
    }
    Io_WriteBlif( pNtkTemp, FileName, fWriteLatches );
    Abc_NtkDelete( pNtkTemp );
}

/**Function*************************************************************

  Synopsis    [Write the network into a BLIF file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteBlif( Abc_Ntk_t * pNtk, char * FileName, int fWriteLatches )
{
    FILE * pFile;
    Abc_Ntk_t * pNtkTemp;
    int i;
    assert( Abc_NtkIsNetlist(pNtk) );
    // start writing the file
    pFile = fopen( FileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteBlif(): Cannot open the output file.\n" );
        return;
    }
    fprintf( pFile, "# Benchmark \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );
    // write the master network
    Io_NtkWrite( pFile, pNtk, fWriteLatches );
    // make sure there is no logic hierarchy
    assert( Abc_NtkWhiteboxNum(pNtk) == 0 );
    // write the hierarchy if present
    if ( Abc_NtkBlackboxNum(pNtk) > 0 )
    {
        Vec_PtrForEachEntry( pNtk->pDesign->vModules, pNtkTemp, i )
        {
            if ( pNtkTemp == pNtk )
                continue;
            fprintf( pFile, "\n\n" );
            Io_NtkWrite( pFile, pNtkTemp, fWriteLatches );
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
void Io_NtkWrite( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches )
{
    Abc_Ntk_t * pExdc;
    assert( Abc_NtkIsNetlist(pNtk) );
    // write the model name
    fprintf( pFile, ".model %s\n", Abc_NtkName(pNtk) );
    // write the network
    Io_NtkWriteOne( pFile, pNtk, fWriteLatches );
    // write EXDC network if it exists
    pExdc = Abc_NtkExdc( pNtk );
    if ( pExdc )
    {
        fprintf( pFile, "\n" );
        fprintf( pFile, ".exdc\n" );
        Io_NtkWriteOne( pFile, pExdc, fWriteLatches );
    }
    // finalize the file
    fprintf( pFile, ".end\n" );
}

/**Function*************************************************************

  Synopsis    [Write one network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteOne( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pNode, * pLatch;
    int i, Length;

    // write the PIs
    fprintf( pFile, ".inputs" );
    Io_NtkWritePis( pFile, pNtk, fWriteLatches );
    fprintf( pFile, "\n" );

    // write the POs
    fprintf( pFile, ".outputs" );
    Io_NtkWritePos( pFile, pNtk, fWriteLatches );
    fprintf( pFile, "\n" );

    // write the assertions
    if ( Abc_NtkAssertNum(pNtk) )
    {
        fprintf( pFile, ".asserts" );
        Io_NtkWriteAsserts( pFile, pNtk );
        fprintf( pFile, "\n" );
    }

    // write the blackbox
    if ( Abc_NtkHasBlackbox( pNtk ) )
    {
        fprintf( pFile, ".blackbox\n" );
        return;
    }

    // write the timing info
    Io_WriteTimingInfo( pFile, pNtk );

    // write the latches
    if ( fWriteLatches && !Abc_NtkIsComb(pNtk) )
    {
        fprintf( pFile, "\n" );
        Abc_NtkForEachLatch( pNtk, pLatch, i )
            Io_NtkWriteLatch( pFile, pLatch );
        fprintf( pFile, "\n" );
    }

    // write the subcircuits
    assert( Abc_NtkWhiteboxNum(pNtk) == 0 );
    if ( Abc_NtkBlackboxNum(pNtk) > 0 )
    {
        fprintf( pFile, "\n" );
        Abc_NtkForEachBlackbox( pNtk, pNode, i )
            Io_NtkWriteSubckt( pFile, pNode );
        fprintf( pFile, "\n" );
    }

    // write each internal node
    Length = Abc_NtkHasMapping(pNtk)? Mio_LibraryReadGateNameMax(pNtk->pManFunc) : 0;
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        Io_NtkWriteNode( pFile, pNode, Length );
    }
    Extra_ProgressBarStop( pProgress );
}


/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWritePis( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches )
{
    Abc_Obj_t * pTerm, * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i;

    LineLength  = 7;
    NameCounter = 0;

    if ( fWriteLatches )
    {
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
    else
    {
        Abc_NtkForEachCi( pNtk, pTerm, i )
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
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWritePos( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches )
{
    Abc_Obj_t * pTerm, * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i;

    LineLength  = 8;
    NameCounter = 0;

    if ( fWriteLatches )
    {
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
    else
    {
        Abc_NtkForEachCo( pNtk, pTerm, i )
        {
            if ( Abc_ObjIsAssert(pTerm) )
                continue;
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
}

/**Function*************************************************************

  Synopsis    [Writes the assertion list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteAsserts( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pTerm, * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i;

    LineLength  = 8;
    NameCounter = 0;

    Abc_NtkForEachAssert( pNtk, pTerm, i )
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

  Synopsis    [Write subcircuit info into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteSubckt( FILE * pFile, Abc_Obj_t * pNode )
{
    Abc_Ntk_t * pModel = pNode->pData;
    Abc_Obj_t * pTerm;
	char * str;
    int i;
	int LineLength;
    int AddedLength;
    
	// write the subcircuit
    fprintf( pFile, ".subckt %s", Abc_NtkName(pModel) );

	LineLength = 8 + strlen(Abc_NtkName(pModel)) + 1;

    // write pairs of the formal=actual names
    Abc_NtkForEachPi( pModel, pTerm, i )
    {
		str = Abc_ObjName(Abc_ObjFanout0(pTerm));
        fprintf( pFile, " %s", str );
		AddedLength = strlen(str) + 1;

        pTerm = Abc_ObjFanin( pNode, i );

		str = Abc_ObjName(Abc_ObjFanin0(pTerm));
        fprintf( pFile, "=%s", str );
		AddedLength += strlen(str) + 1;

		if ( LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { 
            fprintf( pFile, " \\\n" );
            LineLength  = 0;
        }
		else
		{
			LineLength += AddedLength;
		}
    }

    Abc_NtkForEachPo( pModel, pTerm, i )
    {
		str = Abc_ObjName(Abc_ObjFanin0(pTerm));
        fprintf( pFile, " %s", str );
		AddedLength = strlen(str) + 1;

        pTerm = Abc_ObjFanout( pNode, i );

		str = Abc_ObjName(Abc_ObjFanout0(pTerm));
        fprintf( pFile, "=%s", str );
		AddedLength += strlen(str) + 1;

		if ( LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { 
            fprintf( pFile, " \\\n" );
            LineLength  = 0;
        }
		else
		{
			LineLength += AddedLength;
		}
    }

	fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Write the latch into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteLatch( FILE * pFile, Abc_Obj_t * pLatch )
{
    Abc_Obj_t * pNetLi, * pNetLo;
	Abc_LatchInfo_t *pLatchInfo;
    int Reset;

	pLatchInfo = ((Abc_LatchInfo_t *)(pLatch->pData));

    pNetLi = Abc_ObjFanin0( Abc_ObjFanin0(pLatch) );
    pNetLo = Abc_ObjFanout0( Abc_ObjFanout0(pLatch) );
    Reset  = pLatchInfo->InitValue;
    // write the latch line
    fprintf( pFile, ".latch" );
    fprintf( pFile, " %10s",    Abc_ObjName(pNetLi) );
    fprintf( pFile, " %10s",    Abc_ObjName(pNetLo) );

	switch (pLatchInfo->LatchType)
    {
    case ABC_RISING_EDGE:
        fprintf( pFile, " re");
        break;
    case ABC_FALLING_EDGE:
        fprintf( pFile, " fe");
        break;
    case ABC_ACTIVE_HIGH:
        fprintf( pFile, " ah");
        break;
    case ABC_ACTIVE_LOW:
        fprintf( pFile, " al");
        break;
    default:
        fprintf( pFile, " as");
        break;
    }

    fprintf( pFile, " %10s",   ((Abc_LatchInfo_t *)pLatch->pData)->pClkName);
    fprintf( pFile, "  %d\n",   Reset-1 );
}


/**Function*************************************************************

  Synopsis    [Write the node into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteNode( FILE * pFile, Abc_Obj_t * pNode, int Length )
{
    if ( Abc_NtkHasMapping(pNode->pNtk) )
    {
        // write the .gate line
        fprintf( pFile, ".gate" );
        Io_NtkWriteNodeGate( pFile, pNode, Length );
        fprintf( pFile, "\n" );
    }
    else
    {
        // write the .names line
        fprintf( pFile, ".names" );
        Io_NtkWriteNodeFanins( pFile, pNode );
        fprintf( pFile, "\n" );
        // write the cubes
        fprintf( pFile, "%s", Abc_ObjData(pNode) );
    }
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteNodeGate( FILE * pFile, Abc_Obj_t * pNode, int Length )
{
    Mio_Gate_t * pGate = pNode->pData;
    Mio_Pin_t * pGatePin;
    int i;
    // write the node
    fprintf( pFile, " %-*s ", Length, Mio_GateReadName(pGate) );
    for ( pGatePin = Mio_GateReadPins(pGate), i = 0; pGatePin; pGatePin = Mio_PinReadNext(pGatePin), i++ )
        fprintf( pFile, "%s=%s ", Mio_PinReadName(pGatePin), Abc_ObjName( Abc_ObjFanin(pNode,i) ) );
    assert ( i == Abc_ObjFaninNum(pNode) );
    fprintf( pFile, "%s=%s", Mio_GateReadOutName(pGate), Abc_ObjName( Abc_ObjFanout0(pNode) ) );
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteNodeFanins( FILE * pFile, Abc_Obj_t * pNode )
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

/**Function*************************************************************

  Synopsis    [Writes the timing info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteTimingInfo( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    Abc_Time_t * pTime, * pTimeDef;
    int i;

    if ( pNtk->pManTime == NULL )
        return;

    pTimeDef = Abc_NtkReadDefaultArrival( pNtk );
    fprintf( pFile, ".default_input_arrival %g %g\n", pTimeDef->Rise, pTimeDef->Fall );
    Abc_NtkForEachPi( pNtk, pNode, i )
    {
        pTime = Abc_NodeReadArrival(pNode);
        if ( pTime->Rise == pTimeDef->Rise && pTime->Fall == pTimeDef->Fall )
            continue;
        fprintf( pFile, ".input_arrival %s %g %g\n", Abc_ObjName(pNode), pTime->Rise, pTime->Fall );
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


