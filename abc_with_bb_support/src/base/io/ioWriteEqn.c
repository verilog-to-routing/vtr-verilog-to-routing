/**CFile****************************************************************

  FileName    [ioWriteEqn.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write equation representation of the network.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioWriteEqn.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Io_NtkWriteEqnOne( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteEqnCis( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteEqnCos( FILE * pFile, Abc_Ntk_t * pNtk );
static int Io_NtkWriteEqnCheck( Abc_Ntk_t * pNtk );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writes the logic network in the equation format.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteEqn( Abc_Ntk_t * pNtk, char * pFileName )
{
    FILE * pFile;

    assert( Abc_NtkIsAigNetlist(pNtk) );
    if ( Abc_NtkLatchNum(pNtk) > 0 )
        printf( "Warning: only combinational portion is being written.\n" );

    // check that the names are fine for the EQN format
    if ( !Io_NtkWriteEqnCheck(pNtk) )
        return;

    // start the output stream
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteEqn(): Cannot open the output file \"%s\".\n", pFileName );
        return;
    }
    fprintf( pFile, "# Equations for \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );

    // write the equations for the network
    Io_NtkWriteEqnOne( pFile, pNtk );
	fprintf( pFile, "\n" );
	fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Write one network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteEqnOne( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Vec_Vec_t * vLevels;
    ProgressBar * pProgress;
    Abc_Obj_t * pNode, * pFanin;
    int i, k;

    // write the PIs
    fprintf( pFile, "INORDER =" );
    Io_NtkWriteEqnCis( pFile, pNtk );
    fprintf( pFile, ";\n" );

    // write the POs
    fprintf( pFile, "OUTORDER =" );
    Io_NtkWriteEqnCos( pFile, pNtk );
    fprintf( pFile, ";\n" );

    // write each internal node
    vLevels = Vec_VecAlloc( 10 );
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        fprintf( pFile, "%s = ", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        // set the input names
        Abc_ObjForEachFanin( pNode, pFanin, k )
            Hop_IthVar((Hop_Man_t *)pNtk->pManFunc, k)->pData = Abc_ObjName(pFanin);
        // write the formula
        Hop_ObjPrintEqn( pFile, (Hop_Obj_t *)pNode->pData, vLevels, 0 );
        fprintf( pFile, ";\n" );
    }
    Extra_ProgressBarStop( pProgress );
    Vec_VecFree( vLevels );
}


/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteEqnCis( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pTerm, * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i;

    LineLength  = 9;
    NameCounter = 0;

    Abc_NtkForEachCi( pNtk, pTerm, i )
    {
        pNet = Abc_ObjFanout0(pTerm);
        // get the line length after this name is written
        AddedLength = strlen(Abc_ObjName(pNet)) + 1;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, " \n" );
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
void Io_NtkWriteEqnCos( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pTerm, * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i;

    LineLength  = 10;
    NameCounter = 0;

    Abc_NtkForEachCo( pNtk, pTerm, i )
    {
        pNet = Abc_ObjFanin0(pTerm);
        // get the line length after this name is written
        AddedLength = strlen(Abc_ObjName(pNet)) + 1;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, " \n" );
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

  Synopsis    [Make sure the network does not have offending names.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_NtkWriteEqnCheck( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    char * pName = NULL;
    int i, k, Length;
    int RetValue = 1;

    // make sure the network does not have proper names, such as "0" or "1" or containing parentheses
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        pName = Nm_ManFindNameById(pNtk->pManName, i);
        if ( pName == NULL )
            continue;
        Length = strlen(pName);
        if ( pName[0] == '0' || pName[0] == '1' )
        {
            RetValue = 0;
            break;
        }
        for ( k = 0; k < Length; k++ )
            if ( pName[k] == '(' || pName[k] == ')' || pName[k] == '!' || pName[k] == '*' || pName[k] == '+' )
            {
                RetValue = 0;
                break;
            }
        if ( k < Length )
            break;
    }
    if ( RetValue == 0 )
    {
        printf( "The network cannot be written in the EQN format because object %d has name \"%s\".\n", i, pName );
        printf( "Consider renaming the objects using command \"short_names\" and trying again.\n" );
    }
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

