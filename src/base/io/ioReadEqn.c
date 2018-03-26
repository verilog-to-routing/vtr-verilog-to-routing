/**CFile****************************************************************

  FileName    [ioReadEqn.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to read equation format files.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioReadEqn.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Abc_Ntk_t * Io_ReadEqnNetwork( Extra_FileReader_t * p );
static void        Io_ReadEqnStrCompact( char * pStr );
static int         Io_ReadEqnStrFind( Vec_Ptr_t * vTokens, char * pName );
static void        Io_ReadEqnStrCutAt( char * pStr, char * pStop, int fUniqueOnly, Vec_Ptr_t * vTokens );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the network from a BENCH file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadEqn( char * pFileName, int fCheck )
{
    Extra_FileReader_t * p;
    Abc_Ntk_t * pNtk;

    // start the file
    p = Extra_FileReaderAlloc( pFileName, "#", ";", "=" );
    if ( p == NULL )
        return NULL;

    // read the network
    pNtk = Io_ReadEqnNetwork( p );
    Extra_FileReaderFree( p );
    if ( pNtk == NULL )
        return NULL;

    // make sure that everything is okay with the network structure
    if ( fCheck && !Abc_NtkCheckRead( pNtk ) )
    {
        printf( "Io_ReadEqn: The network check has failed.\n" );
        Abc_NtkDelete( pNtk );
        return NULL;
    }
    return pNtk;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadEqnNetwork( Extra_FileReader_t * p )
{
    ProgressBar * pProgress;
    Vec_Ptr_t * vTokens;
    Vec_Ptr_t * vVars;
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pNode;
    char * pNodeName, * pFormula, * pFormulaCopy, * pVarName;
    int iLine, i;
    
    // allocate the empty network
    pNtk = Abc_NtkAlloc( ABC_NTK_NETLIST, ABC_FUNC_AIG, 1 );
    // set the specs
    pNtk->pName = Extra_FileNameGeneric(Extra_FileReaderGetFileName(p));
    pNtk->pSpec = Extra_UtilStrsav(Extra_FileReaderGetFileName(p));

    // go through the lines of the file
    vVars  = Vec_PtrAlloc( 100 );
    pProgress = Extra_ProgressBarStart( stdout, Extra_FileReaderGetFileSize(p) );
    for ( iLine = 0; (vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p)); iLine++ )
    {
        Extra_ProgressBarUpdate( pProgress, Extra_FileReaderGetCurPosition(p), NULL );

        // check if the first token contains anything
        Io_ReadEqnStrCompact( (char *)vTokens->pArray[0] );
        if ( strlen((char *)vTokens->pArray[0]) == 0 )
            break;

        // if the number of tokens is different from two, error
        if ( vTokens->nSize != 2 )
        {
            printf( "%s: Wrong input file format.\n", Extra_FileReaderGetFileName(p) );
            Abc_NtkDelete( pNtk );
            return NULL;
        }

        // get the type of the line
        if ( strncmp( (char *)vTokens->pArray[0], "INORDER", 7 ) == 0 )
        {
            Io_ReadEqnStrCutAt( (char *)vTokens->pArray[1], " \n\r\t", 0, vVars );
            Vec_PtrForEachEntry( char *, vVars, pVarName, i )
                Io_ReadCreatePi( pNtk, pVarName );
        }
        else if ( strncmp( (char *)vTokens->pArray[0], "OUTORDER", 8 ) == 0 )
        {
            Io_ReadEqnStrCutAt( (char *)vTokens->pArray[1], " \n\r\t", 0, vVars );
            Vec_PtrForEachEntry( char *, vVars, pVarName, i )
                Io_ReadCreatePo( pNtk, pVarName );
        }
        else 
        {
            extern Hop_Obj_t * Parse_FormulaParserEqn( FILE * pOutput, char * pFormInit, Vec_Ptr_t * vVarNames, Hop_Man_t * pMan );

            // get hold of the node name and its formula
            pNodeName = (char *)vTokens->pArray[0];
            pFormula  = (char *)vTokens->pArray[1];
            // compact the formula 
            Io_ReadEqnStrCompact( pFormula );

            // consider the case of the constant node
            if ( pFormula[1] == 0 && (pFormula[0] == '0' || pFormula[0] == '1') )
            {
                pFormulaCopy = NULL;
                Vec_PtrClear( vVars );
            }
            else
            {
                // make a copy of formula for names
                pFormulaCopy = Extra_UtilStrsav( pFormula );
                // find the names of the fanins of this node
                Io_ReadEqnStrCutAt( pFormulaCopy, "!*+()", 1, vVars );
            }
            // create the node
            pNode = Io_ReadCreateNode( pNtk, pNodeName, (char **)Vec_PtrArray(vVars), Vec_PtrSize(vVars) );
            // derive the function
            pNode->pData = Parse_FormulaParserEqn( stdout, pFormula, vVars, (Hop_Man_t *)pNtk->pManFunc );
            // remove the cubes
            ABC_FREE( pFormulaCopy );
        }
    }
    Extra_ProgressBarStop( pProgress );
    Vec_PtrFree( vVars );
    Abc_NtkFinalizeRead( pNtk );
    return pNtk;
}



/**Function*************************************************************

  Synopsis    [Compacts the string by throwing away space-like chars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_ReadEqnStrCompact( char * pStr )
{
    char * pCur, * pNew;
    for ( pNew = pCur = pStr; *pCur; pCur++ )
        if ( !(*pCur == ' ' || *pCur == '\n' || *pCur == '\r' || *pCur == '\t') )
            *pNew++ = *pCur;
    *pNew = 0;
}

/**Function*************************************************************

  Synopsis    [Determines unique variables in the string.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadEqnStrFind( Vec_Ptr_t * vTokens, char * pName )
{
    char * pToken;
    int i;
    Vec_PtrForEachEntry( char *, vTokens, pToken, i )
        if ( strcmp( pToken, pName ) == 0 )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Cuts the string into pieces using stop chars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_ReadEqnStrCutAt( char * pStr, char * pStop, int fUniqueOnly, Vec_Ptr_t * vTokens )
{
    char * pToken;
    Vec_PtrClear( vTokens );
    for ( pToken = strtok( pStr, pStop ); pToken; pToken = strtok( NULL, pStop ) )
        if ( !fUniqueOnly || Io_ReadEqnStrFind( vTokens, pToken ) == -1 )
            Vec_PtrPush( vTokens, pToken );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_IMPL_END

