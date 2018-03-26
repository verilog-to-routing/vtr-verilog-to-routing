/**CFile****************************************************************

  FileName    [ioWriteVerilog.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to output a special subset of Verilog.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioWriteVerilog.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"
#include "base/main/main.h"
#include "map/mio/mio.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Io_WriteVerilogInt( FILE * pFile, Abc_Ntk_t * pNtk, int fOnlyAnds );
static void Io_WriteVerilogPis( FILE * pFile, Abc_Ntk_t * pNtk, int Start );
static void Io_WriteVerilogPos( FILE * pFile, Abc_Ntk_t * pNtk, int Start );
static void Io_WriteVerilogWires( FILE * pFile, Abc_Ntk_t * pNtk, int Start );
static void Io_WriteVerilogRegs( FILE * pFile, Abc_Ntk_t * pNtk, int Start );
static void Io_WriteVerilogLatches( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_WriteVerilogObjects( FILE * pFile, Abc_Ntk_t * pNtk, int fOnlyAnds );
static int  Io_WriteVerilogWiresCount( Abc_Ntk_t * pNtk );
static char * Io_WriteVerilogGetName( char * pName );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Write verilog.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteVerilog( Abc_Ntk_t * pNtk, char * pFileName, int fOnlyAnds )
{
    Abc_Ntk_t * pNetlist;
    FILE * pFile;
    int i;
    // can only write nodes represented using local AIGs
    if ( !Abc_NtkIsAigNetlist(pNtk) && !Abc_NtkIsMappedNetlist(pNtk) )
    {
        printf( "Io_WriteVerilog(): Can produce Verilog for mapped or AIG netlists only.\n" );
        return;
    }
    // start the output stream
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteVerilog(): Cannot open the output file \"%s\".\n", pFileName );
        return;
    }

    // write the equations for the network
    fprintf( pFile, "// Benchmark \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );
    fprintf( pFile, "\n" );

    // write modules
    if ( pNtk->pDesign )
    {
        // write the network first
        Io_WriteVerilogInt( pFile, pNtk, fOnlyAnds );
        // write other things
        Vec_PtrForEachEntry( Abc_Ntk_t *, pNtk->pDesign->vModules, pNetlist, i )
        {
            assert( Abc_NtkIsNetlist(pNetlist) );
            if ( pNetlist == pNtk )
                continue;
            fprintf( pFile, "\n" );
            Io_WriteVerilogInt( pFile, pNetlist, fOnlyAnds );
        }
    }
    else
    {
        Io_WriteVerilogInt( pFile, pNtk, fOnlyAnds );
    }

    fprintf( pFile, "\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Writes verilog.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteVerilogInt( FILE * pFile, Abc_Ntk_t * pNtk, int fOnlyAnds )
{
    // write inputs and outputs
//    fprintf( pFile, "module %s ( gclk,\n   ", Abc_NtkName(pNtk) );
    fprintf( pFile, "module %s ( ", Io_WriteVerilogGetName(Abc_NtkName(pNtk)) );
    // add the clock signal if it does not exist
    if ( Abc_NtkLatchNum(pNtk) > 0 && Nm_ManFindIdByName(pNtk->pManName, "clock", ABC_OBJ_PI) == -1 )
        fprintf( pFile, "clock, " );
    // write other primary inputs
    fprintf( pFile, "\n   " );
    if ( Abc_NtkPiNum(pNtk) > 0  )
    {
        Io_WriteVerilogPis( pFile, pNtk, 3 );
        fprintf( pFile, ",\n   " );
    }
    if ( Abc_NtkPoNum(pNtk) > 0  )
        Io_WriteVerilogPos( pFile, pNtk, 3 );
    fprintf( pFile, "  );\n" );
    // add the clock signal if it does not exist
    if ( Abc_NtkLatchNum(pNtk) > 0 && Nm_ManFindIdByName(pNtk->pManName, "clock", ABC_OBJ_PI) == -1 )
        fprintf( pFile, "  input  clock;\n" );
    // write inputs, outputs, registers, and wires
    if ( Abc_NtkPiNum(pNtk) > 0  )
    {
//        fprintf( pFile, "  input gclk," );
        fprintf( pFile, "  input " );
        Io_WriteVerilogPis( pFile, pNtk, 10 );
        fprintf( pFile, ";\n" );
    }
    if ( Abc_NtkPoNum(pNtk) > 0  )
    {
        fprintf( pFile, "  output" );
        Io_WriteVerilogPos( pFile, pNtk, 5 );
        fprintf( pFile, ";\n" );
    }
    // if this is not a blackbox, write internal signals
    if ( !Abc_NtkHasBlackbox(pNtk) )
    {
        if ( Abc_NtkLatchNum(pNtk) > 0 )
        {
            fprintf( pFile, "  reg" );
            Io_WriteVerilogRegs( pFile, pNtk, 4 );
            fprintf( pFile, ";\n" );
        }
        if ( Io_WriteVerilogWiresCount(pNtk) > 0 )
        {
            fprintf( pFile, "  wire" );
            Io_WriteVerilogWires( pFile, pNtk, 4 );
            fprintf( pFile, ";\n" );
        }
        // write nodes
        Io_WriteVerilogObjects( pFile, pNtk, fOnlyAnds );        
        // write registers
        if ( Abc_NtkLatchNum(pNtk) > 0 )
            Io_WriteVerilogLatches( pFile, pNtk );
    }
    // finalize the file
    fprintf( pFile, "endmodule\n\n" );
} 

/**Function*************************************************************

  Synopsis    [Writes the primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteVerilogPis( FILE * pFile, Abc_Ntk_t * pNtk, int Start )
{
    Abc_Obj_t * pTerm, * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i;

    LineLength  = Start;
    NameCounter = 0;
    Abc_NtkForEachPi( pNtk, pTerm, i )
    {
        pNet = Abc_ObjFanout0(pTerm);
        // get the line length after this name is written
        AddedLength = strlen(Io_WriteVerilogGetName(Abc_ObjName(pNet))) + 2;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, "\n   " );
            // reset the line length
            LineLength  = 3;
            NameCounter = 0;
        }
        fprintf( pFile, " %s%s", Io_WriteVerilogGetName(Abc_ObjName(pNet)), (i==Abc_NtkPiNum(pNtk)-1)? "" : "," );
        LineLength += AddedLength;
        NameCounter++;
    }
} 

/**Function*************************************************************

  Synopsis    [Writes the primary outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteVerilogPos( FILE * pFile, Abc_Ntk_t * pNtk, int Start )
{
    Abc_Obj_t * pTerm, * pNet, * pSkip;
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i;
    int nskip;

    pSkip = 0;
    nskip = 0;

    LineLength  = Start;
    NameCounter = 0;
    Abc_NtkForEachPo( pNtk, pTerm, i )
    {
        pNet = Abc_ObjFanin0(pTerm);
        
        if ( Abc_ObjIsPi(Abc_ObjFanin0(pNet)) )
        {
            // Skip this output since it is a feedthrough -- the same
            // name will appear as an input and an output which other
            // tools reading verilog do not like.
            
            nskip++;
            pSkip = pNet;   // save an example of skipped net
            continue;
        }
        
        // get the line length after this name is written
        AddedLength = strlen(Io_WriteVerilogGetName(Abc_ObjName(pNet))) + 2;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, "\n   " );
            // reset the line length
            LineLength  = 3;
            NameCounter = 0;
        }
        fprintf( pFile, " %s%s", Io_WriteVerilogGetName(Abc_ObjName(pNet)), (i==Abc_NtkPoNum(pNtk)-1)? "" : "," );
        LineLength += AddedLength;
        NameCounter++;
    }

    if (nskip != 0)
    {
        assert (pSkip);
        printf( "Io_WriteVerilogPos(): Omitted %d feedthrough nets from output list of module (e.g. %s).\n", nskip, Abc_ObjName(pSkip) );
        return;
    }

}

/**Function*************************************************************

  Synopsis    [Writes the wires.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteVerilogWires( FILE * pFile, Abc_Ntk_t * pNtk, int Start )
{
    Abc_Obj_t * pObj, * pNet, * pBox, * pTerm;
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i, k, Counter, nNodes;

    // count the number of wires
    nNodes = Io_WriteVerilogWiresCount( pNtk );

    // write the wires
    Counter = 0;
    LineLength  = Start;
    NameCounter = 0;
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( i == 0 ) 
            continue;
        pNet = Abc_ObjFanout0(pObj);
        if ( Abc_ObjFanoutNum(pNet) > 0 && Abc_ObjIsCo(Abc_ObjFanout0(pNet)) )
            continue;
        Counter++;
        // get the line length after this name is written
        AddedLength = strlen(Io_WriteVerilogGetName(Abc_ObjName(pNet))) + 2;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, "\n   " );
            // reset the line length
            LineLength  = 3;
            NameCounter = 0;
        }
        fprintf( pFile, " %s%s", Io_WriteVerilogGetName(Abc_ObjName(pNet)), (Counter==nNodes)? "" : "," );
        LineLength += AddedLength;
        NameCounter++;
    }
    Abc_NtkForEachLatch( pNtk, pObj, i )
    {
        pNet = Abc_ObjFanin0(Abc_ObjFanin0(pObj));
        Counter++;
        // get the line length after this name is written
        AddedLength = strlen(Io_WriteVerilogGetName(Abc_ObjName(pNet))) + 2;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, "\n   " );
            // reset the line length
            LineLength  = 3;
            NameCounter = 0;
        }
        fprintf( pFile, " %s%s", Io_WriteVerilogGetName(Abc_ObjName(pNet)), (Counter==nNodes)? "" : "," );
        LineLength += AddedLength;
        NameCounter++;
    }
    Abc_NtkForEachBox( pNtk, pBox, i )
    {
        if ( Abc_ObjIsLatch(pBox) )
            continue;
        Abc_ObjForEachFanin( pBox, pTerm, k )
        {
            pNet = Abc_ObjFanin0(pTerm);
            Counter++;
            // get the line length after this name is written
            AddedLength = strlen(Io_WriteVerilogGetName(Abc_ObjName(pNet))) + 2;
            if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
            { // write the line extender
                fprintf( pFile, "\n   " );
                // reset the line length
                LineLength  = 3;
                NameCounter = 0;
            }
            fprintf( pFile, " %s%s", Io_WriteVerilogGetName(Abc_ObjName(pNet)), (Counter==nNodes)? "" : "," );
            LineLength += AddedLength;
            NameCounter++;
        }
        Abc_ObjForEachFanout( pBox, pTerm, k )
        {
            pNet = Abc_ObjFanout0(pTerm);
            if ( Abc_ObjFanoutNum(pNet) > 0 && Abc_ObjIsCo(Abc_ObjFanout0(pNet)) )
                continue;
            Counter++;
            // get the line length after this name is written
            AddedLength = strlen(Io_WriteVerilogGetName(Abc_ObjName(pNet))) + 2;
            if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
            { // write the line extender
                fprintf( pFile, "\n   " );
                // reset the line length
                LineLength  = 3;
                NameCounter = 0;
            }
            fprintf( pFile, " %s%s", Io_WriteVerilogGetName(Abc_ObjName(pNet)), (Counter==nNodes)? "" : "," );
            LineLength += AddedLength;
            NameCounter++;
        }
    }
    assert( Counter == nNodes );
}

/**Function*************************************************************

  Synopsis    [Writes the regs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteVerilogRegs( FILE * pFile, Abc_Ntk_t * pNtk, int Start )
{
    Abc_Obj_t * pLatch, * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i, Counter, nNodes;

    // count the number of latches
    nNodes = Abc_NtkLatchNum(pNtk);

    // write the wires
    Counter = 0;
    LineLength  = Start;
    NameCounter = 0;
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        pNet = Abc_ObjFanout0(Abc_ObjFanout0(pLatch));
        Counter++;
        // get the line length after this name is written
        AddedLength = strlen(Io_WriteVerilogGetName(Abc_ObjName(pNet))) + 2;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, "\n   " );
            // reset the line length
            LineLength  = 3;
            NameCounter = 0;
        }
        fprintf( pFile, " %s%s", Io_WriteVerilogGetName(Abc_ObjName(pNet)), (Counter==nNodes)? "" : "," );
        LineLength += AddedLength;
        NameCounter++;
    }
}

/**Function*************************************************************

  Synopsis    [Writes the latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteVerilogLatches( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pLatch;
    int i;
    if ( Abc_NtkLatchNum(pNtk) == 0 )
        return;
    // write the latches
//    fprintf( pFile, "  always @(posedge %s) begin\n", Io_WriteVerilogGetName(Abc_ObjFanout0(Abc_NtkPi(pNtk,0))) );
//    fprintf( pFile, "  always begin\n" );
    fprintf( pFile, "  always @ (posedge clock) begin\n" );
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        fprintf( pFile, "    %s", Io_WriteVerilogGetName(Abc_ObjName(Abc_ObjFanout0(Abc_ObjFanout0(pLatch)))) );
        fprintf( pFile, " <= %s;\n", Io_WriteVerilogGetName(Abc_ObjName(Abc_ObjFanin0(Abc_ObjFanin0(pLatch)))) );
    }
    fprintf( pFile, "  end\n" );
    // check if there are initial values
    Abc_NtkForEachLatch( pNtk, pLatch, i )
        if ( Abc_LatchInit(pLatch) == ABC_INIT_ZERO || Abc_LatchInit(pLatch) == ABC_INIT_ONE )
            break;
    if ( i == Abc_NtkLatchNum(pNtk) )
        return;
    // write the initial values
    fprintf( pFile, "  initial begin\n" );
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        if ( Abc_LatchInit(pLatch) == ABC_INIT_ZERO )
            fprintf( pFile, "    %s <= 1\'b0;\n", Io_WriteVerilogGetName(Abc_ObjName(Abc_ObjFanout0(Abc_ObjFanout0(pLatch)))) );
        else if ( Abc_LatchInit(pLatch) == ABC_INIT_ONE )
            fprintf( pFile, "    %s <= 1\'b1;\n", Io_WriteVerilogGetName(Abc_ObjName(Abc_ObjFanout0(Abc_ObjFanout0(pLatch)))) );
    }
    fprintf( pFile, "  end\n" );
}

/**Function*************************************************************

  Synopsis    [Writes the nodes and boxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteVerilogObjects( FILE * pFile, Abc_Ntk_t * pNtk, int fOnlyAnds )
{
    int fUseSimpleGateNames = 0;
    Vec_Vec_t * vLevels;
    Abc_Ntk_t * pNtkBox;
    Abc_Obj_t * pObj, * pTerm, * pFanin;
    Hop_Obj_t * pFunc;
    int i, k, Counter, nDigits, Length;

    // write boxes
    nDigits = Abc_Base10Log( Abc_NtkBoxNum(pNtk)-Abc_NtkLatchNum(pNtk) );
    Counter = 0;
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        if ( Abc_ObjIsLatch(pObj) )
            continue;
        pNtkBox = (Abc_Ntk_t *)pObj->pData;
        fprintf( pFile, "  %s box%0*d", pNtkBox->pName, nDigits, Counter++ );
        fprintf( pFile, "(" );
        Abc_NtkForEachPi( pNtkBox, pTerm, k )
        {
            fprintf( pFile, ".%s",   Io_WriteVerilogGetName(Abc_ObjName(Abc_ObjFanout0(pTerm))) );
            fprintf( pFile, "(%s), ", Io_WriteVerilogGetName(Abc_ObjName(Abc_ObjFanin0(Abc_ObjFanin(pObj,k)))) );
        }
        Abc_NtkForEachPo( pNtkBox, pTerm, k )
        {
            fprintf( pFile, ".%s",   Io_WriteVerilogGetName(Abc_ObjName(Abc_ObjFanin0(pTerm))) );
            fprintf( pFile, "(%s)%s", Io_WriteVerilogGetName(Abc_ObjName(Abc_ObjFanout0(Abc_ObjFanout(pObj,k)))), k==Abc_NtkPoNum(pNtkBox)-1? "":", " );
        }
        fprintf( pFile, ");\n" );
    }
    // write nodes
    if ( Abc_NtkHasMapping(pNtk) )
    {
        Length  = Mio_LibraryReadGateNameMax((Mio_Library_t *)pNtk->pManFunc);
        nDigits = Abc_Base10Log( Abc_NtkNodeNum(pNtk) );
        Counter = 0;
        Abc_NtkForEachNode( pNtk, pObj, k )
        {
            Mio_Gate_t * pGate = (Mio_Gate_t *)pObj->pData;
            Mio_Pin_t * pGatePin;
            if ( Abc_ObjFaninNum(pObj) == 0 && (!strcmp(Mio_GateReadName(pGate), "_const0_") || !strcmp(Mio_GateReadName(pGate), "_const1_")) )
            {
                fprintf( pFile, "  %-*s %s = 1\'b%d;\n", Length, "assign", Io_WriteVerilogGetName(Abc_ObjName( Abc_ObjFanout0(pObj) )), !strcmp(Mio_GateReadName(pGate), "_const1_") );
                continue;
            }
            // write the node
            if ( fUseSimpleGateNames )
            {
                fprintf( pFile, "%-*s ", Length, Mio_GateReadName(pGate) );
                fprintf( pFile, "( %s", Io_WriteVerilogGetName(Abc_ObjName( Abc_ObjFanout0(pObj) )) );
                for ( pGatePin = Mio_GateReadPins(pGate), i = 0; pGatePin; pGatePin = Mio_PinReadNext(pGatePin), i++ )
                    fprintf( pFile, ", %s", Io_WriteVerilogGetName(Abc_ObjName( Abc_ObjFanin(pObj,i) )) );
                assert ( i == Abc_ObjFaninNum(pObj) );
                fprintf( pFile, " );\n" );
            }
            else
            {
                fprintf( pFile, "  %-*s g%0*d", Length, Mio_GateReadName(pGate), nDigits, Counter++ );
                fprintf( pFile, "(" );
                for ( pGatePin = Mio_GateReadPins(pGate), i = 0; pGatePin; pGatePin = Mio_PinReadNext(pGatePin), i++ )
                {
                    fprintf( pFile, ".%s", Io_WriteVerilogGetName(Mio_PinReadName(pGatePin)) );
                    fprintf( pFile, "(%s), ", Io_WriteVerilogGetName(Abc_ObjName( Abc_ObjFanin(pObj,i) )) );
                }
                assert ( i == Abc_ObjFaninNum(pObj) );
                fprintf( pFile, ".%s", Io_WriteVerilogGetName(Mio_GateReadOutName(pGate)) );
                fprintf( pFile, "(%s)", Io_WriteVerilogGetName(Abc_ObjName( Abc_ObjFanout0(pObj) )) );
                fprintf( pFile, ");\n" );
            }
        }
    }
    else
    {
        vLevels = Vec_VecAlloc( 10 );
        Abc_NtkForEachNode( pNtk, pObj, i )
        {
            pFunc = (Hop_Obj_t *)pObj->pData;
            fprintf( pFile, "  assign %s = ", Io_WriteVerilogGetName(Abc_ObjName(Abc_ObjFanout0(pObj))) );
            // set the input names
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Hop_IthVar((Hop_Man_t *)pNtk->pManFunc, k)->pData = Extra_UtilStrsav(Io_WriteVerilogGetName(Abc_ObjName(pFanin)));
            // write the formula
            Hop_ObjPrintVerilog( pFile, pFunc, vLevels, 0, fOnlyAnds );
            fprintf( pFile, ";\n" );
            // clear the input names
            Abc_ObjForEachFanin( pObj, pFanin, k )
                ABC_FREE( Hop_IthVar((Hop_Man_t *)pNtk->pManFunc, k)->pData );
        }
        Vec_VecFree( vLevels );
    }
}

/**Function*************************************************************

  Synopsis    [Counts the number of wires.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteVerilogWiresCount( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pNet, * pBox;
    int i, k, nWires;
    nWires = Abc_NtkLatchNum(pNtk);
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( i == 0 ) 
            continue;
        pNet = Abc_ObjFanout0(pObj);
        if ( Abc_ObjFanoutNum(pNet) > 0 && Abc_ObjIsCo(Abc_ObjFanout0(pNet)) )
            continue;
        nWires++;
    }
    Abc_NtkForEachBox( pNtk, pBox, i )
    {
        if ( Abc_ObjIsLatch(pBox) )
            continue;
        nWires += Abc_ObjFaninNum(pBox);
        Abc_ObjForEachFanout( pBox, pObj, k )
        {
            pNet = Abc_ObjFanout0(pObj);
            if ( Abc_ObjFanoutNum(pNet) > 0 && Abc_ObjIsCo(Abc_ObjFanout0(pNet)) )
                continue;
            nWires++;
        }
    }
    return nWires;
}

/**Function*************************************************************

  Synopsis    [Prepares the name for writing the Verilog file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Io_WriteVerilogGetName( char * pName )
{
    static char Buffer[500];
    int Length, i;
    Length = strlen(pName);
    // consider the case of a signal having name "0" or "1"
    if ( !(Length == 1 && (pName[0] == '0' || pName[0] == '1')) )
    {
        for ( i = 0; i < Length; i++ )
            if ( !((pName[i] >= 'a' && pName[i] <= 'z') || 
                 (pName[i] >= 'A' && pName[i] <= 'Z') || 
                 (pName[i] >= '0' && pName[i] <= '9') || pName[i] == '_') )
                 break;
        if ( i == Length )
            return pName;
    }
    // create Verilog style name
    Buffer[0] = '\\';
    for ( i = 0; i < Length; i++ )
        Buffer[i+1] = pName[i];
    Buffer[Length+1] = ' ';
    Buffer[Length+2] = 0;
    return Buffer;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

