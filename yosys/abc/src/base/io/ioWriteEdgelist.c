/**CFile****************************************************************

  FileName    [ioWriteEdgelist.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write Edgelist files for graph learning.]

  Author      [Cunxi Yu]
  
  Affiliation [University of Utah]

  Date        [Ver. 1.0 04/15/2023.]

  Revision    [$Id: more details at https://github.com/ycunxi/Verilog-to-PyG $]

***********************************************************************/

#include "ioAbc.h"
#include "base/main/main.h"
#include "map/mio/mio.h"
#include "bool/kit/kit.h"
#include "map/if/if.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Io_NtkEdgelistWrite( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches, int fBb2Wb, int fSeq , int fName);
static void Io_NtkEdgelistWriteOne( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches, int fBb2Wb, int fSeq , int fName);
static void Io_NtkEdgelistWritePis( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches );
static void Io_NtkEdgelistWritePos( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches );
static void Io_NtkEdgelistWriteSubckt( FILE * pFile, Abc_Obj_t * pNode );
static void Io_NtkEdgelistWriteAsserts( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkEdgelistWriteNodeFanins( FILE * pFile, Abc_Obj_t * pNode, int fName);
static int  Io_NtkEdgelistWriteNode( FILE * pFile, Abc_Obj_t * pNode, int Length, int fName);
static void Io_NtkEdgelistWriteLatch( FILE * pFile, Abc_Obj_t * pLatch , int fName);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Write the network into a Edgelist file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteEdgelist( Abc_Ntk_t * pNtk, char * FileName, int fWriteLatches, int fBb2Wb, int fSeq, int fName)
{
    FILE * pFile;  int i;
    // start writing the file
    pFile = fopen( FileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteEdgelist(): Cannot open the output file.\n" );
        return;
    }
    fprintf( stdout, "WriteEdgelist (Verilog-to-PyG @ https://github.com/ycunxi/Verilog-to-PyG) starts writing to %s.\n", FileName );
    fprintf( pFile, "# Benchmark Edgelist Dumping (beta) \"%s\" written by ABC on %s (more at https://github.com/ycunxi/Verilog-to-PyG)\n", pNtk->pName, Extra_TimeStamp() );
    // write the master network
    Io_NtkEdgelistWrite( pFile, pNtk, fWriteLatches, fBb2Wb, fSeq , fName);
    // make sure there is no logic hierarchy
//    assert( Abc_NtkWhiteboxNum(pNtk) == 0 );
    // write the hierarchy if present
    if ( Abc_NtkBlackboxNum(pNtk) > 0 || Abc_NtkWhiteboxNum(pNtk) > 0 )
    {
        Abc_Ntk_t * pNtkTemp;
        Vec_PtrForEachEntry( Abc_Ntk_t *, pNtk->pDesign->vModules, pNtkTemp, i )
        {
            if ( pNtkTemp == pNtk )
                continue;
            fprintf( pFile, "\n\n" );
            Io_NtkEdgelistWrite( pFile, pNtkTemp, fWriteLatches, fBb2Wb, fSeq, fName);
        }
    }
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Write the network into a Edgelist file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkEdgelistWrite_int( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches, int fBb2Wb, int fSeq , int fName)
{
    Abc_Ntk_t * pExdc;
    assert( Abc_NtkIsNetlist(pNtk) );
    // write the model name
    // fprintf( pFile, ".model %s\n", Abc_NtkName(pNtk) );
    // write the network
    Io_NtkEdgelistWriteOne( pFile, pNtk, fWriteLatches, fBb2Wb, fSeq , fName);
    // write EXDC network if it exists
    pExdc = Abc_NtkExdc( pNtk );
    if ( pExdc )
    {
        fprintf( pFile, "\n" );
        fprintf( pFile, ".exdc\n" );
        Io_NtkEdgelistWriteOne( pFile, pExdc, fWriteLatches, fBb2Wb, fSeq , fName);
    }
    // finalize the file
    //fprintf( pFile, ".end\n" );
}
void Io_NtkEdgelistWrite( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches, int fBb2Wb, int fSeq , int fName)
{
    if( Abc_NtkIsNetlist(pNtk) )
        Io_NtkEdgelistWrite_int( pFile, pNtk, fWriteLatches, fBb2Wb, fSeq , fName );
    else
    {
        Abc_Ntk_t * pNtkTemp = Abc_NtkToNetlist(pNtk);
        Io_NtkEdgelistWrite_int( pFile, pNtkTemp, fWriteLatches, fBb2Wb, fSeq , fName );
        Abc_NtkDelete( pNtkTemp );
    }
}
/**Function*************************************************************

  Synopsis    [Write one network.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
void Io_NtkEdgelistWriteConvertedBox( FILE * pFile, Abc_Ntk_t * pNtk, int fSeq )
{
    Abc_Obj_t * pObj;
    int i, v;
    if ( fSeq )
    {
        fprintf( pFile, ".attrib white box seq\n" );
    }
    else
    {
        fprintf( pFile, ".attrib white box comb\n" );
        fprintf( pFile, ".delay 1\n" );
    }
    Abc_NtkForEachPo( pNtk, pObj, i )
    { 
        // write the .names line
        //fprintf( pFile, ".names" );
        Io_NtkEdgelistWritePis( pFile, pNtk, 1 );
        if ( fSeq )
            fprintf( pFile, " %s_in\n", Abc_ObjName(Abc_ObjFanin0(pObj)) );
        else
            fprintf( pFile, " %s\n", Abc_ObjName(Abc_ObjFanin0(pObj)) );
        for ( v = 0; v < Abc_NtkPiNum(pNtk); v++ )
            fprintf( pFile, "1" );
        fprintf( pFile, " 1\n" );
        if ( fSeq )
            fprintf( pFile, " %s_in %s 1\n", Abc_ObjName(Abc_ObjFanin0(pObj)), Abc_ObjName(Abc_ObjFanin0(pObj)) );
            //fprintf( pFile, ".latch %s_in %s 1\n", Abc_ObjName(Abc_ObjFanin0(pObj)), Abc_ObjName(Abc_ObjFanin0(pObj)) );
    }
}

/**Function*************************************************************

  Synopsis    [Transforms truth table into an SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Io_NtkWriteEdgelistDeriveSop( Mem_Flex_t * pMem, word uTruth, int nVars, Vec_Int_t * vCover )
{
    char * pSop;
    int RetValue = Kit_TruthIsop( (unsigned *)&uTruth, nVars, vCover, 1 );
    assert( RetValue == 0 || RetValue == 1 );
    // check the case of constant cover
    if ( Vec_IntSize(vCover) == 0 || (Vec_IntSize(vCover) == 1 && Vec_IntEntry(vCover,0) == 0) )
    {
        char * pStr0 = " 0\n", * pStr1 = " 1\n";
        assert( RetValue == 0 );
        return Vec_IntSize(vCover) == 0 ? pStr0 : pStr1;
    }
    // derive the AIG for that tree
    pSop = Abc_SopCreateFromIsop( pMem, nVars, vCover );
    if ( RetValue )
        Abc_SopComplement( pSop );
    return pSop;
}


/**Function*************************************************************

  Synopsis    [Write one network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkEdgelistWriteOne( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches, int fBb2Wb, int fSeq , int fName)
{
    ProgressBar * pProgress;
    Abc_Obj_t * pNode, * pLatch;
    int i, Length;
    // write the PIs
    Io_NtkEdgelistWritePis( pFile, pNtk, fWriteLatches );
    // write the blackbox
    if ( Abc_NtkHasBlackbox( pNtk ) )
    {
        if ( fBb2Wb )
            Io_NtkEdgelistWriteConvertedBox( pFile, pNtk, fSeq );
        else
            fprintf( pFile, ".blackbox\n" );
        return;
    }

    // write the timing info
    Io_WriteTimingInfo( pFile, pNtk );

    // write the latches
    if ( fWriteLatches && !Abc_NtkIsComb(pNtk) )
    {
        //fprintf( pFile, "\n" );
        Abc_NtkForEachLatch( pNtk, pLatch, i )
            Io_NtkEdgelistWriteLatch( pFile, pLatch , fName);
        //fprintf( pFile, "\n" );
    }

    // write the subcircuits
//    assert( Abc_NtkWhiteboxNum(pNtk) == 0 );
    if ( Abc_NtkBlackboxNum(pNtk) > 0 || Abc_NtkWhiteboxNum(pNtk) > 0 )
    {
        fprintf( pFile, "\n" );
        Abc_NtkForEachBlackbox( pNtk, pNode, i )
            Io_NtkEdgelistWriteSubckt( pFile, pNode );
        fprintf( pFile, "\n" );
        Abc_NtkForEachWhitebox( pNtk, pNode, i )
            Io_NtkEdgelistWriteSubckt( pFile, pNode );
        fprintf( pFile, "\n" );
    }

    // write each internal node
    Length = Abc_NtkHasMapping(pNtk)? Mio_LibraryReadGateNameMax((Mio_Library_t *)pNtk->pManFunc) : 0;
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        if ( Io_NtkEdgelistWriteNode( pFile, pNode, Length, fName) ) // skip the next node
            i++;
    }
    // write the POs
    Io_NtkEdgelistWritePos( pFile, pNtk, fWriteLatches );

    Extra_ProgressBarStop( pProgress );
}


/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkEdgelistWritePis( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches )
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
                // fprintf( pFile, " \\\n" );
                // reset the line length
                LineLength  = 0;
                NameCounter = 0;
            }
            fprintf( pFile, "%d %d Pi 00\n", Abc_ObjId(pTerm), Abc_ObjId(pNet) );
            //fprintf( pFile, "%s %s Pi 00\n", Abc_ObjName(pTerm), Abc_ObjName(pNet) );
            //fprintf( pFile, " %s", Abc_ObjName(pNet) );
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
                // fprintf( pFile, " \\\n" );
                // reset the line length
                LineLength  = 0;
                NameCounter = 0;
            }
            fprintf( pFile, "%d %d Pi 00\n", Abc_ObjId(pTerm), Abc_ObjId(pNet) );
            //fprintf( pFile, "%s %s Pi\n", Abc_ObjName(pTerm), Abc_ObjName(pNet) );
            //fprintf( pFile, " %s", Abc_ObjName(pNet) );
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
void Io_NtkEdgelistWritePos( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches )
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
                //fprintf( pFile, " \\\n" );
                // reset the line length
                LineLength  = 0;
                NameCounter = 0;
            }
            fprintf( pFile, "%d %d Po 00\n",Abc_ObjId(pNet), Abc_ObjId(pTerm) );
            //fprintf( pFile, "%s %s Po 00\n", Abc_ObjName(pTerm), Abc_ObjName(pNet) );
            LineLength += AddedLength;
            NameCounter++;
        }
    }
    else
    {
        Abc_NtkForEachCo( pNtk, pTerm, i )
        {
            pNet = Abc_ObjFanin0(pTerm);
            // get the line length after this name is written
            AddedLength = strlen(Abc_ObjName(pNet)) + 1;
            if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
            { // write the line extender
                // fprintf( pFile, " \\\n" );
                // reset the line length
                LineLength  = 0;
                NameCounter = 0;
            }
            //fprintf( pFile, " %s", Abc_ObjName(pNet) );
            fprintf( pFile, "%d %d Po 00\n",Abc_ObjId(pNet), Abc_ObjId(pTerm) );
            //fprintf( pFile, "%s %s Po 00\n", Abc_ObjName(pTerm), Abc_ObjName(pNet) );
            LineLength += AddedLength;
            NameCounter++;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Write the latch into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkEdgelistWriteSubckt( FILE * pFile, Abc_Obj_t * pNode )
{
    Abc_Ntk_t * pModel = (Abc_Ntk_t *)pNode->pData;
    Abc_Obj_t * pTerm;
    int i;
    // write the subcircuit
//    fprintf( pFile, ".subckt %s %s", Abc_NtkName(pModel), Abc_ObjName(pNode) );
    fprintf( pFile, ".subckt %s", Abc_NtkName(pModel) );
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

  Synopsis    [Write the latch into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkEdgelistWriteLatch( FILE * pFile, Abc_Obj_t * pLatch , int fName)
{
    Abc_Obj_t * pNetLi, * pNetLo;
    int Reset;
    pNetLi = Abc_ObjFanin0( Abc_ObjFanin0(pLatch) );
    pNetLo = Abc_ObjFanout0( Abc_ObjFanout0(pLatch) );
    Reset  = (int)(ABC_PTRUINT_T)Abc_ObjData( pLatch );
    // write the latch line
    // fprintf( pFile, ".latch" );
    if (fName){
      fprintf( pFile, "%s", Abc_ObjName(pNetLi) );
      fprintf( pFile, " %s",    Abc_ObjName(pNetLo) );
    }
    else{
      fprintf( pFile, "%d", Abc_ObjId(pNetLi) );
      fprintf( pFile, " %d",    Abc_ObjId(pNetLo) );
    }

    fprintf( pFile, " REG %d\n",   Reset-1 );
    //fprintf( pFile, " {type:\"REG\" reset=\"%d\"}\n",   Reset-1 );
}


/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkEdgelistWriteNodeFanins( FILE * pFile, Abc_Obj_t * pNode , int fName)
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
        char * temp = NULL;
        int len;
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
        if (fName){
          fprintf( pFile, "%s", Abc_ObjName(pNet) );
        }
        else{
          fprintf( pFile, "%d", Abc_ObjId(pNet) );
        }
        LineLength += AddedLength;
        NameCounter++;

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
        //char temp[sizeof((char*)Abc_ObjData(pNode))];
        //strcpy(temp, (char*)Abc_ObjData(pNode));
        temp = Abc_UtilStrsav((char*)Abc_ObjData(pNode));
        len = strlen(temp);
        if (len > 0) {
            temp[len - 1] = '\0';
            temp[len - 2] = '\0';
            temp[len - 3] = '\0';
        }
        if (fName){
          if (Abc_ObjIsCi(pNode)) 
            fprintf( pFile, " %s Pi AIG %s\n", Abc_ObjName(Abc_ObjFanout0(pNode)), temp);
          else if (Abc_ObjIsCo(Abc_ObjFanout0(pNode)))
            fprintf( pFile, " %s AIG Po %s\n", Abc_ObjName(Abc_ObjFanout0(pNode)), temp);
          else
            fprintf( pFile, " %s AIG %s\n", Abc_ObjName(Abc_ObjFanout0(pNode)), temp);

        }
        else{
          if (Abc_ObjIsCi(pNode)) 
            fprintf( pFile, " %d Pi AIG %s\n", Abc_ObjId(Abc_ObjFanout0(pNode)), temp);
          else if (Abc_ObjIsCo(Abc_ObjFanout0(pNode)))
            fprintf( pFile, " %d AIG Po %s\n", Abc_ObjId(Abc_ObjFanout0(pNode)), temp);
          else
            fprintf( pFile, " %d AIG %s\n", Abc_ObjId(Abc_ObjFanout0(pNode)), temp);


        }
 
        //fprintf( pFile, " %d {type:\"AIG\" cube:\"%s\"}\n", Abc_ObjId(Abc_ObjFanout0(pNode)), temp);

        ABC_FREE( temp );
    }

    /*
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
    */
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkEdgelistWriteSubcktFanins( FILE * pFile, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    char * pName;
    int i;

    LineLength  = 6;
    NameCounter = 0;

    // get the output name
    pName = Abc_ObjName(Abc_ObjFanout0(pNode));
    // get the line length after the output name is written
    AddedLength = strlen(pName) + 1;
    fprintf( pFile, " m%d", Abc_ObjId(pNode) );

    // get the input names
    Abc_ObjForEachFanin( pNode, pNet, i )
    {
        // get the fanin name
        pName = Abc_ObjName(pNet);
        // get the line length after the fanin name is written
        AddedLength = strlen(pName) + 3;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, " \\\n" );
            // reset the line length
            LineLength  = 0;
            NameCounter = 0;
        }
        fprintf( pFile, " %c=%s", 'a'+i, pName );
        LineLength += AddedLength;
        NameCounter++;
    }

    // get the output name
    pName = Abc_ObjName(Abc_ObjFanout0(pNode));
    // get the line length after the output name is written
    AddedLength = strlen(pName) + 3;
    if ( NameCounter && LineLength + AddedLength > 75 )
    { // write the line extender
        fprintf( pFile, " \\\n" );
        // reset the line length
        LineLength  = 0;
        NameCounter = 0;
    }
    fprintf( pFile, " %c=%s", 'o', pName );
}


/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_NtkEdgelistWriteNodeGate( FILE * pFile, Abc_Obj_t * pNode, int Length, int fName)
{
    static int fReport = 0;
    Mio_Gate_t * pGate = (Mio_Gate_t *)pNode->pData;
    Mio_Pin_t * pGatePin;
    Abc_Obj_t * pNode2;
    int i;
    for ( pGatePin = Mio_GateReadPins(pGate), i = 0; pGatePin; pGatePin = Mio_PinReadNext(pGatePin), i++ ){
        if (fName)
          fprintf( pFile, "%s ", Abc_ObjName( Abc_ObjFanin(pNode,i) ) );
        else
          fprintf( pFile, "%d ", Abc_ObjId( Abc_ObjFanin(pNode,i) ) );
    }
        //fprintf( pFile, "%s=%s ", Mio_PinReadName(pGatePin), Abc_ObjName( Abc_ObjFanin(pNode,i) ) );
    assert ( i == Abc_ObjFaninNum(pNode) );
    if (fName)
      fprintf( pFile, "%s", Abc_ObjName( Abc_ObjFanout0(pNode) ) );
    else
      fprintf( pFile, "%d", Abc_ObjId( Abc_ObjFanout0(pNode) ) );

    //fprintf( pFile, "%s=%s", Mio_GateReadOutName(pGate), Abc_ObjName( Abc_ObjFanout0(pNode) ) );
    if ( Mio_GateReadTwin(pGate) == NULL ){
        //print gate type
        fprintf( pFile, " %s", Mio_GateReadName(pGate) );
        fprintf( pFile, " %lu", Mio_GateReadTruth(pGate) );
        //Vec_Int_t * Expr = Mio_GateReadExpr(pGate);
        //Vec_IntPrint(Expr);
        return 0;
    }
    pNode2 = Abc_NtkFetchTwinNode( pNode );
    if ( pNode2 == NULL )
    {
        if ( !fReport )
            fReport = 1, printf( "Warning: Missing second output of gate(s) \"%s\".\n", Mio_GateReadName(pGate) );
        return 0;
    }
    fprintf( pFile, " %s",  Abc_ObjName( Abc_ObjFanout0(pNode2) ) );
    //print gate type
    fprintf( pFile, " %s", Mio_GateReadName(pGate));
    fprintf( pFile, " %lu", Mio_GateReadTruth(pGate) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Write the node into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_NtkEdgelistWriteNode( FILE * pFile, Abc_Obj_t * pNode, int Length, int fName)
{
    int RetValue = 0;
    if ( Abc_NtkHasMapping(pNode->pNtk) )
    {
        // write the .gate line
        if ( Abc_ObjIsBarBuf(pNode) )
        {
            printf("ERROR: not implemented\n");
            return 0;
            fprintf( pFile, ".barbuf " );
            fprintf( pFile, "%s %s", Abc_ObjName(Abc_ObjFanin0(pNode)), Abc_ObjName(Abc_ObjFanout0(pNode)) );
            fprintf( pFile, "\n" );
        }
        else
        {
            //fprintf( pFile, ".gate" );
            RetValue = Io_NtkEdgelistWriteNodeGate( pFile, pNode, Length, fName);
            fprintf( pFile, "\n" );
        }
    }
    else
    {
        // write the .names line
        //fprintf( pFile, ".names" );
        Io_NtkEdgelistWriteNodeFanins( pFile, pNode, fName);
        // write the cubes
        // fprintf( pFile, " %s", (char*)Abc_ObjData(pNode) );
    }
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Write the node into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_NtkEdgelistWriteNodeSubckt( FILE * pFile, Abc_Obj_t * pNode, int Length )
{
    int RetValue = 0;
    fprintf( pFile, ".subckt" );
    Io_NtkEdgelistWriteSubcktFanins( pFile, pNode );
    fprintf( pFile, "\n" );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Write the node into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkEdgelistWriteNodeInt( FILE * pFile, Abc_Obj_t * pNode, Vec_Int_t * vCover )
{
    Abc_Obj_t * pNet;
    int i, nVars = Abc_ObjFaninNum(pNode);
    if ( nVars > 7 )
    {
        printf( "Node \"%s\" has more than 7 inputs. Writing Edgelist has failed.\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        return;
    }

    fprintf( pFile, "\n" );
    if ( nVars <= 4 )
    {
        Abc_ObjForEachFanin( pNode, pNet, i )
            fprintf( pFile, " %s", Abc_ObjName(pNet) );
        // get the output name
        fprintf( pFile, " %s ", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        // write the cubes
        fprintf( pFile, "%s\n", (char*)Abc_ObjData(pNode) );
    }
    else
    {
        extern int  If_Dec6PickBestMux( word t, word Cofs[2] );
        extern int  If_Dec7PickBestMux( word t[2], word c0r[2], word c1r[2] );
        extern word If_Dec6MinimumBase( word uTruth, int * pSupp, int nVarsAll, int * pnVars );
        extern void If_Dec7MinimumBase( word uTruth[2], int * pSupp, int nVarsAll, int * pnVars );
        extern word If_Dec6Perform( word t, int fDerive );
        extern word If_Dec7Perform( word t[2], int fDerive );

        char * pSop;
        word z, uTruth6 = 0, uTruth7[2], Cofs6[2], Cofs7[2][2];
        int c, iVar, nVarsMin[2], pVars[2][10];

        // collect variables
        Abc_ObjForEachFanin( pNode, pNet, i )
            pVars[0][i] = pVars[1][i] = i;

        // derive truth table
        if ( nVars == 7 )
        {
            Abc_SopToTruth7( (char*)Abc_ObjData(pNode), nVars, uTruth7 );
            iVar = If_Dec7PickBestMux( uTruth7, Cofs7[0], Cofs7[1] );
        }
        else
        {
            uTruth6 = Abc_SopToTruth( (char*)Abc_ObjData(pNode), nVars );
            iVar = If_Dec6PickBestMux( uTruth6, Cofs6 );
        }

        // perform MUX decomposition
        if ( iVar >= 0 )
        {
            if ( nVars == 7 )
            {
                If_Dec7MinimumBase( Cofs7[0], pVars[0], nVars, &nVarsMin[0] );
                If_Dec7MinimumBase( Cofs7[1], pVars[1], nVars, &nVarsMin[1] );
            }
            else
            {
                Cofs6[0] = If_Dec6MinimumBase( Cofs6[0], pVars[0], nVars, &nVarsMin[0] );
                Cofs6[1] = If_Dec6MinimumBase( Cofs6[1], pVars[1], nVars, &nVarsMin[1] );
            }
            assert( nVarsMin[0] < 5 );
            assert( nVarsMin[1] < 5 );
            // write MUX
            //fprintf( pFile, ".names" );
            fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanin(pNode,iVar)) );
            fprintf( pFile, " %s_cascade0", Abc_ObjName(Abc_ObjFanout0(pNode)) );
            fprintf( pFile, " %s_cascade1", Abc_ObjName(Abc_ObjFanout0(pNode)) );
            fprintf( pFile, " %s\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
            fprintf( pFile, "1-1 1\n01- 1\n" );
            // write cofactors
            for ( c = 0; c < 2; c++ )
            {
                pSop = Io_NtkWriteEdgelistDeriveSop( (Mem_Flex_t *)Abc_ObjNtk(pNode)->pManFunc, 
                    (word)(nVars == 7 ? Cofs7[c][0] : Cofs6[c]), nVarsMin[c], vCover );
                //fprintf( pFile, ".names" );
                for ( i = 0; i < nVarsMin[c]; i++ )
                    fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanin(pNode,pVars[c][i])) );
                fprintf( pFile, " %s_cascade%d\n", Abc_ObjName(Abc_ObjFanout0(pNode)), c );
                fprintf( pFile, "%s", pSop );
            }
            return;
        }
        assert( nVars == 6 || nVars == 7 );

        // try cascade decomposition
        if ( nVars == 7 )
        {
            z = If_Dec7Perform( uTruth7, 1 );
            //If_Dec7Verify( uTruth7, z );
        }
        else
        {
            z = If_Dec6Perform( uTruth6, 1 );
            //If_Dec6Verify( uTruth6, z );
        }
        if ( z == 0 )
        {
            printf( "Node \"%s\" is not decomposable. Writing Edgelist has failed.\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
            return;
        }

        // derive nodes
        for ( c = 1; c >= 0; c-- )
        {
            // collect fanins
            uTruth7[c]  = ((c ? z >> 32 : z) & 0xffff);
            uTruth7[c] |= (uTruth7[c] << 16);
            uTruth7[c] |= (uTruth7[c] << 32);
            for ( i = 0; i < 4; i++ )
                pVars[c][i] = (z >> (c*32+16+4*i)) & 7;

            // minimize truth table
            Cofs6[c] = If_Dec6MinimumBase( uTruth7[c], pVars[c], 4, &nVarsMin[c] );

            // write the nodes
            //fprintf( pFile, ".names" );
            for ( i = 0; i < nVarsMin[c]; i++ )
                if ( pVars[c][i] == 7 )
                    fprintf( pFile, " %s_cascade", Abc_ObjName(Abc_ObjFanout0(pNode)) );
                else
                    fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanin(pNode,pVars[c][i])) );
            fprintf( pFile, " %s%s\n", Abc_ObjName(Abc_ObjFanout0(pNode)), c? "" : "_cascade" );

            // write SOP
            pSop = Io_NtkWriteEdgelistDeriveSop( (Mem_Flex_t *)Abc_ObjNtk(pNode)->pManFunc, 
                (word)Cofs6[c], nVarsMin[c], vCover );
            fprintf( pFile, "%s", pSop );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Write the node into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkEdgelistWriteNodeIntStruct( FILE * pFile, Abc_Obj_t * pNode, Vec_Int_t * vCover, char * pStr )
{
    Abc_Obj_t * pNet;
    int nLeaves = Abc_ObjFaninNum(pNode);
    int i, nLutLeaf, nLutLeaf2, nLutRoot, Length;

    // quit if parameters are wrong
    Length = strlen(pStr);
    if ( Length != 2 && Length != 3 )
    {
        printf( "Wrong LUT struct (%s)\n", pStr );
        return;
    }
    for ( i = 0; i < Length; i++ )
        if ( pStr[i] - '0' < 3 || pStr[i] - '0' > 6 )
        {
            printf( "The LUT size (%d) should belong to {3,4,5,6}.\n", pStr[i] - '0' );
            return;
        }

    nLutLeaf  =                   pStr[0] - '0';
    nLutLeaf2 = ( Length == 3 ) ? pStr[1] - '0' : 0;
    nLutRoot  =                   pStr[Length-1] - '0';
    if ( nLeaves > nLutLeaf - 1 + (nLutLeaf2 ? nLutLeaf2 - 1 : 0) + nLutRoot )
    {
        printf( "The node size (%d) is too large for the LUT structure %s.\n", nLeaves, pStr );
        return;
    }

    // consider easy case
    fprintf( pFile, "\n" );
    if ( nLeaves <= Abc_MaxInt( nLutLeaf2, Abc_MaxInt(nLutLeaf, nLutRoot) ) )
    {
        // write the .names line
        ////fprintf( pFile, ".names" );
        Abc_ObjForEachFanin( pNode, pNet, i )
            fprintf( pFile, " %s", Abc_ObjName(pNet) );
        // get the output name
        fprintf( pFile, " %s ", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        // write the cubes
        fprintf( pFile, "%s\n", (char*)Abc_ObjData(pNode) );
        return;
    }
    else
    {
        extern int If_CluMinimumBase( word * t, int * pSupp, int nVarsAll, int * pnVars );

        static word TruthStore[16][1<<10] = {{0}}, * pTruths[16];
        word pCube[1<<10], pRes[1<<10], Func0, Func1, Func2;
        char pLut0[32], pLut1[32], pLut2[32] = {0}, * pSop;
//        int nVarsMin[3], pVars[3][20];

        if ( TruthStore[0][0] == 0 )
        {
            static word Truth6[6] = {
                ABC_CONST(0xAAAAAAAAAAAAAAAA),
                ABC_CONST(0xCCCCCCCCCCCCCCCC),
                ABC_CONST(0xF0F0F0F0F0F0F0F0),
                ABC_CONST(0xFF00FF00FF00FF00),
                ABC_CONST(0xFFFF0000FFFF0000),
                ABC_CONST(0xFFFFFFFF00000000)
            };
            int nVarsMax = 16;
            int nWordsMax = (1 << 10);
            int i, k;
            assert( nVarsMax <= 16 );
            for ( i = 0; i < nVarsMax; i++ )
                pTruths[i] = TruthStore[i];
            for ( i = 0; i < 6; i++ )
                for ( k = 0; k < nWordsMax; k++ )
                    pTruths[i][k] = Truth6[i];
            for ( i = 6; i < nVarsMax; i++ )
                for ( k = 0; k < nWordsMax; k++ )
                    pTruths[i][k] = ((k >> (i-6)) & 1) ? ~(word)0 : 0;
        }

        // collect variables
//        Abc_ObjForEachFanin( pNode, pNet, i )
//            pVars[0][i] = pVars[1][i] = pVars[2][i] = i;

        // derive truth table
        Abc_SopToTruthBig( (char*)Abc_ObjData(pNode), nLeaves, pTruths, pCube, pRes );
        if ( Kit_TruthIsConst0((unsigned *)pRes, nLeaves) || Kit_TruthIsConst1((unsigned *)pRes, nLeaves) )
        {
            fprintf( pFile, "%s\n const%d\n", Abc_ObjName(Abc_ObjFanout0(pNode)), Kit_TruthIsConst1((unsigned *)pRes, nLeaves) );
            return;
        }

//        Extra_PrintHex( stdout, (unsigned *)pRes, nLeaves );  printf( "    " );
//        Kit_DsdPrintFromTruth( (unsigned*)pRes, nLeaves );  printf( "\n" );

        // perform decomposition
        if ( Length == 2 )
        {
            if ( !If_CluCheckExt( NULL, pRes, nLeaves, nLutLeaf, nLutRoot, pLut0, pLut1, &Func0, &Func1 ) )
            {
                Extra_PrintHex( stdout, (unsigned *)pRes, nLeaves );  printf( "    " );
                Kit_DsdPrintFromTruth( (unsigned*)pRes, nLeaves );  printf( "\n" );
                printf( "Node \"%s\" is not decomposable. Writing BLIF has failed.\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
                return;
            }
        }
        else
        {
            if ( !If_CluCheckExt3( NULL, pRes, nLeaves, nLutLeaf, nLutLeaf2, nLutRoot, pLut0, pLut1, pLut2, &Func0, &Func1, &Func2 ) )
            {
                Extra_PrintHex( stdout, (unsigned *)pRes, nLeaves );  printf( "    " );
                Kit_DsdPrintFromTruth( (unsigned*)pRes, nLeaves );  printf( "\n" );
                printf( "Node \"%s\" is not decomposable. Writing BLIF has failed.\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
                return;
            }
        }

        // write leaf node
        // //fprintf( pFile, ".names" );
        for ( i = 0; i < pLut1[0]; i++ )
            fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanin(pNode,pLut1[2+i])) );
        fprintf( pFile, " %s_lut1\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        // write SOP
        pSop = Io_NtkWriteEdgelistDeriveSop( (Mem_Flex_t *)Abc_ObjNtk(pNode)->pManFunc, Func1, pLut1[0], vCover );
        fprintf( pFile, "%s", pSop );

        if ( Length == 3 && pLut2[0] > 0 )
        {
            // write leaf node
            // //fprintf( pFile, ".names" );
            for ( i = 0; i < pLut2[0]; i++ )
                if ( pLut2[2+i] == nLeaves )
                    fprintf( pFile, " %s_lut1", Abc_ObjName(Abc_ObjFanout0(pNode)) );
                else
                    fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanin(pNode,pLut2[2+i])) );
            fprintf( pFile, " %s_lut2\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
            // write SOP
            pSop = Io_NtkWriteEdgelistDeriveSop( (Mem_Flex_t *)Abc_ObjNtk(pNode)->pManFunc, Func2, pLut2[0], vCover );
            fprintf( pFile, "%s", pSop );
        }

        // write root node
        // //fprintf( pFile, ".names" );
        for ( i = 0; i < pLut0[0]; i++ )
            if ( pLut0[2+i] == nLeaves )
                fprintf( pFile, " %s_lut1", Abc_ObjName(Abc_ObjFanout0(pNode)) );
            else if ( pLut0[2+i] == nLeaves+1 )
                fprintf( pFile, " %s_lut2", Abc_ObjName(Abc_ObjFanout0(pNode)) );
            else
                fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanin(pNode,pLut0[2+i])) );
        fprintf( pFile, " %s\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        // write SOP
        pSop = Io_NtkWriteEdgelistDeriveSop( (Mem_Flex_t *)Abc_ObjNtk(pNode)->pManFunc, Func0, pLut0[0], vCover );
        fprintf( pFile, "%s", pSop );
    }
}

/**Function*************************************************************

  Synopsis    [Write the node into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkEdgelistWriteModelIntStruct( FILE * pFile, Abc_Obj_t * pNode, Vec_Int_t * vCover, char * pStr )
{
    Abc_Obj_t * pNet;
    int nLeaves = Abc_ObjFaninNum(pNode);
    int i, nLutLeaf, nLutLeaf2, nLutRoot, Length;

    // write the header
    /*
    fprintf( pFile, "\n" );
    fprintf( pFile, ".model m%d\n", Abc_ObjName(pNode) );
    fprintf( pFile, ".inputs" );
    for ( i = 0; i < Abc_ObjFaninNum(pNode); i++ )
        fprintf( pFile, " %c", 'a' + i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs o\n" );
    */
    // quit if parameters are wrong
    Length = strlen(pStr);
    if ( Length != 2 && Length != 3 )
    {
        printf( "Wrong LUT struct (%s)\n", pStr );
        return;
    }
    for ( i = 0; i < Length; i++ )
        if ( pStr[i] - '0' < 3 || pStr[i] - '0' > 6 )
        {
            printf( "The LUT size (%d) should belong to {3,4,5,6}.\n", pStr[i] - '0' );
            return;
        }

    nLutLeaf  =                   pStr[0] - '0';
    nLutLeaf2 = ( Length == 3 ) ? pStr[1] - '0' : 0;
    nLutRoot  =                   pStr[Length-1] - '0';
    if ( nLeaves > nLutLeaf - 1 + (nLutLeaf2 ? nLutLeaf2 - 1 : 0) + nLutRoot )
    {
        printf( "The node size (%d) is too large for the LUT structure %s.\n", nLeaves, pStr );
        return;
    }

    // consider easy case
    if ( nLeaves <= Abc_MaxInt( nLutLeaf2, Abc_MaxInt(nLutLeaf, nLutRoot) ) )
    {
        // write the .names line
        // //fprintf( pFile, ".names" );
        Abc_ObjForEachFanin( pNode, pNet, i )
            fprintf( pFile, "%c", 'a' + i );
        // get the output name
        fprintf( pFile, " %s", "o" );
        // write the cubes
        fprintf( pFile, " %s\n", (char*)Abc_ObjData(pNode) );
        //fprintf( pFile, ".end\n" );
        return;
    }
    else
    {
        extern int If_CluMinimumBase( word * t, int * pSupp, int nVarsAll, int * pnVars );

        static word TruthStore[16][1<<10] = {{0}}, * pTruths[16];
        word pCube[1<<10], pRes[1<<10], Func0, Func1, Func2;
        char pLut0[32], pLut1[32], pLut2[32] = {0}, * pSop;
//        int nVarsMin[3], pVars[3][20];

        if ( TruthStore[0][0] == 0 )
        {
            static word Truth6[6] = {
                ABC_CONST(0xAAAAAAAAAAAAAAAA),
                ABC_CONST(0xCCCCCCCCCCCCCCCC),
                ABC_CONST(0xF0F0F0F0F0F0F0F0),
                ABC_CONST(0xFF00FF00FF00FF00),
                ABC_CONST(0xFFFF0000FFFF0000),
                ABC_CONST(0xFFFFFFFF00000000)
            };
            int nVarsMax = 16;
            int nWordsMax = (1 << 10);
            int i, k;
            assert( nVarsMax <= 16 );
            for ( i = 0; i < nVarsMax; i++ )
                pTruths[i] = TruthStore[i];
            for ( i = 0; i < 6; i++ )
                for ( k = 0; k < nWordsMax; k++ )
                    pTruths[i][k] = Truth6[i];
            for ( i = 6; i < nVarsMax; i++ )
                for ( k = 0; k < nWordsMax; k++ )
                    pTruths[i][k] = ((k >> (i-6)) & 1) ? ~(word)0 : 0;
        }

        // collect variables
//        Abc_ObjForEachFanin( pNode, pNet, i )
//            pVars[0][i] = pVars[1][i] = pVars[2][i] = i;

        // derive truth table
        Abc_SopToTruthBig( (char*)Abc_ObjData(pNode), nLeaves, pTruths, pCube, pRes );
        if ( Kit_TruthIsConst0((unsigned *)pRes, nLeaves) || Kit_TruthIsConst1((unsigned *)pRes, nLeaves) )
        {
            fprintf( pFile, ".names %s\n %d\n", "o", Kit_TruthIsConst1((unsigned *)pRes, nLeaves) );
            fprintf( pFile, ".end\n" );
            return;
        }

//        Extra_PrintHex( stdout, (unsigned *)pRes, nLeaves );  printf( "    " );
//        Kit_DsdPrintFromTruth( (unsigned*)pRes, nLeaves );  printf( "\n" );

        // perform decomposition
        if ( Length == 2 )
        {
            if ( !If_CluCheckExt( NULL, pRes, nLeaves, nLutLeaf, nLutRoot, pLut0, pLut1, &Func0, &Func1 ) )
            {
                Extra_PrintHex( stdout, (unsigned *)pRes, nLeaves );  printf( "    " );
                Kit_DsdPrintFromTruth( (unsigned*)pRes, nLeaves );  printf( "\n" );
                printf( "Node \"%s\" is not decomposable. Writing BLIF has failed.\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
                return;
            }
        }
        else
        {
            if ( !If_CluCheckExt3( NULL, pRes, nLeaves, nLutLeaf, nLutLeaf2, nLutRoot, pLut0, pLut1, pLut2, &Func0, &Func1, &Func2 ) )
            {
                Extra_PrintHex( stdout, (unsigned *)pRes, nLeaves );  printf( "    " );
                Kit_DsdPrintFromTruth( (unsigned*)pRes, nLeaves );  printf( "\n" );
                printf( "Node \"%s\" is not decomposable. Writing BLIF has failed.\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
                return;
            }
        }

        // write leaf node
        // //fprintf( pFile, ".names" );
        for ( i = 0; i < pLut1[0]; i++ )
            fprintf( pFile, " %c", 'a' + pLut1[2+i] );
        fprintf( pFile, " lut1\n" );
        // write SOP
        pSop = Io_NtkWriteEdgelistDeriveSop( (Mem_Flex_t *)Abc_ObjNtk(pNode)->pManFunc, Func1, pLut1[0], vCover );
        fprintf( pFile, "%s", pSop );

        if ( Length == 3 && pLut2[0] > 0 )
        {
            // write leaf node
            // //fprintf( pFile, ".names" );
            for ( i = 0; i < pLut2[0]; i++ )
                if ( pLut2[2+i] == nLeaves )
                    fprintf( pFile, " lut1" );
                else
                    fprintf( pFile, " %c", 'a' + pLut2[2+i] );
            fprintf( pFile, " lut2\n" );
            // write SOP
            pSop = Io_NtkWriteEdgelistDeriveSop( (Mem_Flex_t *)Abc_ObjNtk(pNode)->pManFunc, Func2, pLut2[0], vCover );
            fprintf( pFile, "%s", pSop );
        }

        // write root node
        //fprintf( pFile, ".names" );
        for ( i = 0; i < pLut0[0]; i++ )
            if ( pLut0[2+i] == nLeaves )
                fprintf( pFile, " lut1" );
            else if ( pLut0[2+i] == nLeaves+1 )
                fprintf( pFile, " lut2" );
            else
                fprintf( pFile, " %c", 'a' + pLut0[2+i] );
        fprintf( pFile, " %s\n", "o" );
        // write SOP
        pSop = Io_NtkWriteEdgelistDeriveSop( (Mem_Flex_t *)Abc_ObjNtk(pNode)->pManFunc, Func0, pLut0[0], vCover );
        fprintf( pFile, "%s", pSop );
        fprintf( pFile, ".end\n" );
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

