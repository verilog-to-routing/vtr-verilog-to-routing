/**CFile****************************************************************

  FileName    [ioWriteBench.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write the network in BENCH format.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioWriteBench.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Io_WriteBenchCheckNames( Abc_Ntk_t * pNtk );

static int Io_WriteBenchOne( FILE * pFile, Abc_Ntk_t * pNtk );
static int Io_WriteBenchOneNode( FILE * pFile, Abc_Obj_t * pNode );

static int Io_WriteBenchLutOne( FILE * pFile, Abc_Ntk_t * pNtk );
static int Io_WriteBenchLutOneNode( FILE * pFile, Abc_Obj_t * pNode, Vec_Int_t * vTruth );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writes the network in BENCH format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteBench( Abc_Ntk_t * pNtk, const char * pFileName )
{
    Abc_Ntk_t * pExdc;
    FILE * pFile;
    assert( Abc_NtkIsSopNetlist(pNtk) );
    if ( !Io_WriteBenchCheckNames(pNtk) )
    {
        fprintf( stdout, "Io_WriteBench(): Signal names in this benchmark contain parentheses making them impossible to reproduce in the BENCH format. Use \"short_names\".\n" );
        return 0;
    }
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteBench(): Cannot open the output file.\n" );
        return 0;
    }
    fprintf( pFile, "# Benchmark \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );
    // write the network
    Io_WriteBenchOne( pFile, pNtk );
    // write EXDC network if it exists
    pExdc = Abc_NtkExdc( pNtk );
    if ( pExdc )
        printf( "Io_WriteBench: EXDC is not written (warning).\n" );
    // finalize the file
    fclose( pFile );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Writes the network in BENCH format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteBenchOne( FILE * pFile, Abc_Ntk_t * pNtk )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pNode;
    int i;

    // write the PIs/POs/latches
    Abc_NtkForEachPi( pNtk, pNode, i )
        fprintf( pFile, "INPUT(%s)\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
    Abc_NtkForEachPo( pNtk, pNode, i )
        fprintf( pFile, "OUTPUT(%s)\n", Abc_ObjName(Abc_ObjFanin0(pNode)) );
    Abc_NtkForEachLatch( pNtk, pNode, i )
        fprintf( pFile, "%-11s = DFF(%s)\n", 
            Abc_ObjName(Abc_ObjFanout0(Abc_ObjFanout0(pNode))), Abc_ObjName(Abc_ObjFanin0(Abc_ObjFanin0(pNode))) );

    // write internal nodes
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        Io_WriteBenchOneNode( pFile, pNode );
    }
    Extra_ProgressBarStop( pProgress );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Writes the network in BENCH format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteBenchOneNode( FILE * pFile, Abc_Obj_t * pNode )
{
    int nFanins;

    assert( Abc_ObjIsNode(pNode) );
    nFanins = Abc_ObjFaninNum(pNode);
    if ( nFanins == 0 )
    {   // write the constant 1 node
        assert( Abc_NodeIsConst1(pNode) );
        fprintf( pFile, "%-11s",          Abc_ObjName(Abc_ObjFanout0(pNode)) );
        fprintf( pFile, " = vdd\n" );
    }
    else if ( nFanins == 1 )
    {   // write the interver/buffer
        if ( Abc_NodeIsBuf(pNode) )
        {
            fprintf( pFile, "%-11s = BUFF(",  Abc_ObjName(Abc_ObjFanout0(pNode)) );
            fprintf( pFile, "%s)\n",          Abc_ObjName(Abc_ObjFanin0(pNode)) );
        }
        else
        {
            fprintf( pFile, "%-11s = NOT(",   Abc_ObjName(Abc_ObjFanout0(pNode)) );
            fprintf( pFile, "%s)\n",          Abc_ObjName(Abc_ObjFanin0(pNode)) );
        }
    }
    else
    {   // write the AND gate
        fprintf( pFile, "%-11s",       Abc_ObjName(Abc_ObjFanout0(pNode)) );
        fprintf( pFile, " = AND(%s, ", Abc_ObjName(Abc_ObjFanin0(pNode)) );
        fprintf( pFile, "%s)\n",       Abc_ObjName(Abc_ObjFanin1(pNode)) );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Writes the network in BENCH format with LUTs and DFFRSE.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteBenchLut( Abc_Ntk_t * pNtk, char * pFileName )
{
    Abc_Ntk_t * pExdc;
    FILE * pFile;
    assert( Abc_NtkIsAigNetlist(pNtk) );
    if ( !Io_WriteBenchCheckNames(pNtk) )
    {
        fprintf( stdout, "Io_WriteBenchLut(): Signal names in this benchmark contain parentheses making them impossible to reproduce in the BENCH format. Use \"short_names\".\n" );
        return 0;
    }
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteBench(): Cannot open the output file.\n" );
        return 0;
    }
    fprintf( pFile, "# Benchmark \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );
    // write the network
    Io_WriteBenchLutOne( pFile, pNtk );
    // write EXDC network if it exists
    pExdc = Abc_NtkExdc( pNtk );
    if ( pExdc )
        printf( "Io_WriteBench: EXDC is not written (warning).\n" );
    // finalize the file
    fclose( pFile );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Writes the network in BENCH format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteBenchLutOne( FILE * pFile, Abc_Ntk_t * pNtk )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pNode;
    Vec_Int_t * vMemory;
    int i;

    // write the PIs/POs/latches
    Abc_NtkForEachPi( pNtk, pNode, i )
        fprintf( pFile, "INPUT(%s)\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
    Abc_NtkForEachPo( pNtk, pNode, i )
        fprintf( pFile, "OUTPUT(%s)\n", Abc_ObjName(Abc_ObjFanin0(pNode)) );
    Abc_NtkForEachLatch( pNtk, pNode, i )
        fprintf( pFile, "%-11s = DFFRSE( %s, gnd, gnd, gnd, gnd )\n", 
            Abc_ObjName(Abc_ObjFanout0(Abc_ObjFanout0(pNode))), Abc_ObjName(Abc_ObjFanin0(Abc_ObjFanin0(pNode))) );
//Abc_NtkLevel(pNtk);
    // write internal nodes
    vMemory = Vec_IntAlloc( 10000 );
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        Io_WriteBenchLutOneNode( pFile, pNode, vMemory );
    }
    Extra_ProgressBarStop( pProgress );
    Vec_IntFree( vMemory );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Writes the network in BENCH format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteBenchLutOneNode( FILE * pFile, Abc_Obj_t * pNode, Vec_Int_t * vTruth )
{
    Abc_Obj_t * pFanin;
    unsigned * pTruth;
    int i, nFanins;
    assert( Abc_ObjIsNode(pNode) );
    nFanins = Abc_ObjFaninNum(pNode);
    assert( nFanins <= 15 );
    // compute the truth table
    pTruth = Hop_ManConvertAigToTruth( (Hop_Man_t *)pNode->pNtk->pManFunc, Hop_Regular((Hop_Obj_t *)pNode->pData), nFanins, vTruth, 0 );
    if ( Hop_IsComplement((Hop_Obj_t *)pNode->pData) )
        Extra_TruthNot( pTruth, pTruth, nFanins );
    // consider simple cases
    if ( Extra_TruthIsConst0(pTruth, nFanins) )
    {
        fprintf( pFile, "%-11s = gnd\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        return 1;
    }
    if ( Extra_TruthIsConst1(pTruth, nFanins) )
    {
        fprintf( pFile, "%-11s = vdd\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        return 1;
    }
    if ( nFanins == 1 )
    {
        fprintf( pFile, "%-11s = LUT 0x%d ( %s )\n",  
            Abc_ObjName(Abc_ObjFanout0(pNode)), 
            Abc_NodeIsBuf(pNode)? 2 : 1,
            Abc_ObjName(Abc_ObjFanin0(pNode)) );
        return 1;
    }
    // write it in the hexadecimal form
    fprintf( pFile, "%-11s = LUT 0x",  Abc_ObjName(Abc_ObjFanout0(pNode)) );
    Extra_PrintHexadecimal( pFile, pTruth, nFanins );
/*
    {
extern void Kit_DsdTest( unsigned * pTruth, int nVars );
Abc_ObjForEachFanin( pNode, pFanin, i )
printf( "%c%d ", 'a'+i, Abc_ObjFanin0(pFanin)->Level );
printf( "\n" );
Kit_DsdTest( pTruth, nFanins );
    }
    if ( pNode->Id % 1000 == 0 )
    {
        int x = 0;
    }
*/
    // write the fanins
    fprintf( pFile, " (" );
    Abc_ObjForEachFanin( pNode, pFanin, i )
        fprintf( pFile, " %s%s", Abc_ObjName(pFanin), ((i==nFanins-1)? "" : ",") );
    fprintf( pFile, " )\n" );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if the names cannot be written into the bench file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteBenchCheckNames( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    char * pName;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        for ( pName = Nm_ManFindNameById(pNtk->pManName, i); pName && *pName; pName++ )
            if ( *pName == '(' || *pName == ')' )
                return 0;
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

