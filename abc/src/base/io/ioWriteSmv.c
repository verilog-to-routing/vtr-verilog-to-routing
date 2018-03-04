/**CFile****************************************************************

  FileName    [ioWriteSmv.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write the network in SMV format.]

  Author      [Satrajit Chatterjee]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioWriteSmv.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Io_WriteSmvCheckNames( Abc_Ntk_t * pNtk );

static int Io_WriteSmvOne( FILE * pFile, Abc_Ntk_t * pNtk );
static int Io_WriteSmvOneNode( FILE * pFile, Abc_Obj_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// This returns a pointer to a static area, so be careful in using results
// of this function i.e. don't call this twice in the same printf call.
//
// This function replaces '|' with '_' I think abc introduces '|' when
// flattening hierarchy. The '|' is interpreted as a or function by nusmv
// which is unfortunate. This probably should be fixed elsewhere.
static char *cleanUNSAFE( const char *s )
{
    char *t;
    static char buffer[1024];
    assert (strlen(s) < 1024);
    strcpy(buffer, s);
    for (t = buffer; *t != 0; ++t) *t = (*t == '|') ? '_' : *t;
    return buffer;
}

static int hasPrefix(const char *needle, const char *haystack)
{
    return (strncmp(haystack, needle, strlen(needle)) == 0);
}

/**Function*************************************************************

  Synopsis    [Writes the network in SMV format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteSmv( Abc_Ntk_t * pNtk, char * pFileName )
{
    Abc_Ntk_t * pExdc;
    FILE * pFile;
    assert( Abc_NtkIsSopNetlist(pNtk) );
    if ( !Io_WriteSmvCheckNames(pNtk) )
    {
        fprintf( stdout, "Io_WriteSmv(): Signal names in this benchmark contain parentheses making them impossible to reproduce in the SMV format. Use \"short_names\".\n" );
        return 0;
    }
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteSmv(): Cannot open the output file.\n" );
        return 0;
    }
    fprintf( pFile, "-- benchmark \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );
    // write the network
    Io_WriteSmvOne( pFile, pNtk );
    // write EXDC network if it exists
    pExdc = Abc_NtkExdc( pNtk );
    if ( pExdc )
        printf( "Io_WriteSmv: EXDC is not written (warning).\n" );
    // finalize the file
    fclose( pFile );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Writes the network in SMV format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteSmvOne( FILE * pFile, Abc_Ntk_t * pNtk )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pNode;
    int i;

    // write the PIs/POs/latches
    fprintf( pFile, "MODULE main\n");   // nusmv needs top module to be main
    fprintf ( pFile, "\n" );

    fprintf( pFile, "VAR  -- inputs\n");
    Abc_NtkForEachPi( pNtk, pNode, i )
        fprintf( pFile, "    %s : boolean;\n", 
                cleanUNSAFE(Abc_ObjName(Abc_ObjFanout0(pNode))) );
    fprintf ( pFile, "\n" );

    fprintf( pFile, "VAR  -- state variables\n");
    Abc_NtkForEachLatch( pNtk, pNode, i )
        fprintf( pFile, "    %s : boolean;\n", 
                cleanUNSAFE(Abc_ObjName(Abc_ObjFanout0(Abc_ObjFanout0(pNode)))) ); 
    fprintf ( pFile, "\n" );

    // No outputs needed for NuSMV: 
    // TODO: Add sepcs by recognizing assume_.* and assert_.*
    //
    // Abc_NtkForEachPo( pNtk, pNode, i )
    //    fprintf( pFile, "OUTPUT(%s)\n", Abc_ObjName(Abc_ObjFanin0(pNode)) );
    
    // write internal nodes
    fprintf( pFile, "DEFINE\n");
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        Io_WriteSmvOneNode( pFile, pNode );
    }
    Extra_ProgressBarStop( pProgress );
    fprintf ( pFile, "\n" );

    fprintf( pFile, "ASSIGN\n");
    Abc_NtkForEachLatch( pNtk, pNode, i )
    {
        int Reset = (int)(ABC_PTRUINT_T)Abc_ObjData( pNode );
        assert (Reset >= 1);
        assert (Reset <= 3);

        if (Reset != 3)
        {
            fprintf( pFile, "    init(%s) := %d;\n", 
                cleanUNSAFE(Abc_ObjName(Abc_ObjFanout0(Abc_ObjFanout0(pNode)))),
                Reset - 1); 
        }
        fprintf( pFile, "    next(%s) := ", 
                cleanUNSAFE(Abc_ObjName(Abc_ObjFanout0(Abc_ObjFanout0(pNode)))) ); 
        fprintf( pFile, "%s;\n", 
                cleanUNSAFE(Abc_ObjName(Abc_ObjFanin0(Abc_ObjFanin0(pNode)))) );
    }
      
    fprintf ( pFile, "\n" );
    Abc_NtkForEachPo( pNtk, pNode, i )
    {
        const char *n = cleanUNSAFE(Abc_ObjName(Abc_ObjFanin0(pNode)));
        // fprintf( pFile, "-- output %s;\n", n );
        if (hasPrefix("assume_fair_", n))
        {
            fprintf( pFile, "FAIRNESS %s;\n", n );
        }
        else if (hasPrefix("Assert_", n) || 
                hasPrefix("assert_safety_", n))
        {
            fprintf( pFile, "INVARSPEC %s;\n", n );
        }
        else if (hasPrefix("assert_fair_", n))
        {
            fprintf( pFile, "LTLSPEC G F %s;\n", n );
        }
    }

    return 1;
}

/**Function*************************************************************

  Synopsis    [Writes the network in SMV format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteSmvOneNode( FILE * pFile, Abc_Obj_t * pNode )
{
    int nFanins;

    assert( Abc_ObjIsNode(pNode) );
    nFanins = Abc_ObjFaninNum(pNode);
    if ( nFanins == 0 )
    {   // write the constant 1 node
        assert( Abc_NodeIsConst1(pNode) );
        fprintf( pFile, "    %s", cleanUNSAFE(Abc_ObjName(Abc_ObjFanout0(pNode)) ) );
        fprintf( pFile, " := 1;\n" );
    }
    else if ( nFanins == 1 )
    {   // write the interver/buffer
        if ( Abc_NodeIsBuf(pNode) )
        {
            fprintf( pFile, "    %s := ", cleanUNSAFE(Abc_ObjName(Abc_ObjFanout0(pNode))) );
            fprintf( pFile, "%s;\n",      cleanUNSAFE(Abc_ObjName(Abc_ObjFanin0(pNode))) );
        }
        else
        {
            fprintf( pFile, "    %s := !",  cleanUNSAFE(Abc_ObjName(Abc_ObjFanout0(pNode))) );
            fprintf( pFile, "%s;\n",       cleanUNSAFE(Abc_ObjName(Abc_ObjFanin0(pNode))) );
        }
    }
    else
    {   // write the AND gate
        fprintf( pFile, "    %s", cleanUNSAFE(Abc_ObjName(Abc_ObjFanout0(pNode))) );
        fprintf( pFile, " := %s & ", cleanUNSAFE(Abc_ObjName(Abc_ObjFanin0(pNode))) );
        fprintf( pFile, "%s;\n", cleanUNSAFE(Abc_ObjName(Abc_ObjFanin1(pNode))) );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the names cannot be written into the bench file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteSmvCheckNames( Abc_Ntk_t * pNtk )
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

