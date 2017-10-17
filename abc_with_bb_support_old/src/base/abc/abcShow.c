/**CFile****************************************************************

  FileName    [abcShow.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Visualization procedures using DOT software and GSView.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcShow.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifdef WIN32
#include <process.h> 
#endif

#include "abc.h"
#include "main.h"
#include "io.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void Abc_ShowFile( char * FileNameDot );
static void Abc_ShowGetFileName( char * pName, char * pBuffer );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Visualizes BDD of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeShowBdd( Abc_Obj_t * pNode )
{
    FILE * pFile;
    Vec_Ptr_t * vNamesIn;
    char FileNameDot[200];
    char * pNameOut;

    assert( Abc_NtkIsBddLogic(pNode->pNtk) );
    // create the file name
    Abc_ShowGetFileName( Abc_ObjName(pNode), FileNameDot );
    // check that the file can be opened
    if ( (pFile = fopen( FileNameDot, "w" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        return;
    }

    // set the node names 
    vNamesIn = Abc_NodeGetFaninNames( pNode );
    pNameOut = Abc_ObjName(pNode);
    Cudd_DumpDot( pNode->pNtk->pManFunc, 1, (DdNode **)&pNode->pData, (char **)vNamesIn->pArray, &pNameOut, pFile );
    Abc_NodeFreeNames( vNamesIn );
    Abc_NtkCleanCopy( pNode->pNtk );
    fclose( pFile );

    // visualize the file 
    Abc_ShowFile( FileNameDot );
}

/**Function*************************************************************

  Synopsis    [Visualizes a reconvergence driven cut at the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeShowCut( Abc_Obj_t * pNode, int nNodeSizeMax, int nConeSizeMax )
{
    FILE * pFile;
    char FileNameDot[200];
    Abc_ManCut_t * p;
    Vec_Ptr_t * vCutSmall;
    Vec_Ptr_t * vCutLarge;
    Vec_Ptr_t * vInside;
    Vec_Ptr_t * vNodesTfo;
    Abc_Obj_t * pTemp;
    int i;

    assert( Abc_NtkIsStrash(pNode->pNtk) );

    // start the cut computation manager
    p = Abc_NtkManCutStart( nNodeSizeMax, nConeSizeMax, 2, ABC_INFINITY );
    // get the recovergence driven cut
    vCutSmall = Abc_NodeFindCut( p, pNode, 1 );
    // get the containing cut
    vCutLarge = Abc_NtkManCutReadCutLarge( p );
    // get the array for the inside nodes
    vInside = Abc_NtkManCutReadVisited( p );
    // get the inside nodes of the containing cone
    Abc_NodeConeCollect( &pNode, 1, vCutLarge, vInside, 1 );

    // add the nodes in the TFO 
    vNodesTfo = Abc_NodeCollectTfoCands( p, pNode, vCutSmall, ABC_INFINITY );
    Vec_PtrForEachEntry( vNodesTfo, pTemp, i )
        Vec_PtrPushUnique( vInside, pTemp );

    // create the file name
    Abc_ShowGetFileName( Abc_ObjName(pNode), FileNameDot );
    // check that the file can be opened
    if ( (pFile = fopen( FileNameDot, "w" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        return;
    }
    // add the root node to the cone (for visualization)
    Vec_PtrPush( vCutSmall, pNode );
    // write the DOT file
    Io_WriteDotNtk( pNode->pNtk, vInside, vCutSmall, FileNameDot, 0, 0 );
    // stop the cut computation manager
    Abc_NtkManCutStop( p );

    // visualize the file 
    Abc_ShowFile( FileNameDot );
}

/**Function*************************************************************

  Synopsis    [Visualizes AIG with choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkShow( Abc_Ntk_t * pNtk, int fGateNames, int fSeq, int fUseReverse )
{
    FILE * pFile;
    Abc_Obj_t * pNode;
    Vec_Ptr_t * vNodes;
    char FileNameDot[200];
    int i;

    assert( Abc_NtkIsStrash(pNtk) || Abc_NtkIsLogic(pNtk) );
    if ( Abc_NtkIsStrash(pNtk) && Abc_NtkGetChoiceNum(pNtk) )
    {
        printf( "Temporarily visualization of AIGs with choice nodes is disabled.\n" );
        return;
    }
    // convert to logic SOP
    if ( Abc_NtkIsLogic(pNtk) )
        Abc_NtkToSop( pNtk, 0 );
    // create the file name
    Abc_ShowGetFileName( pNtk->pName, FileNameDot );
    // check that the file can be opened
    if ( (pFile = fopen( FileNameDot, "w" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        return;
    }
    fclose( pFile );

    // collect all nodes in the network
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NtkForEachObj( pNtk, pNode, i )
        Vec_PtrPush( vNodes, pNode );
    // write the DOT file
    if ( fSeq )
        Io_WriteDotSeq( pNtk, vNodes, NULL, FileNameDot, fGateNames, fUseReverse );
    else
        Io_WriteDotNtk( pNtk, vNodes, NULL, FileNameDot, fGateNames, fUseReverse );
    Vec_PtrFree( vNodes );

    // visualize the file 
    Abc_ShowFile( FileNameDot );
}


/**Function*************************************************************

  Synopsis    [Shows the given DOT file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ShowFile( char * FileNameDot )
{
    FILE * pFile;
    char * FileGeneric;
    char FileNamePs[200];
    char CommandDot[1000];
    char * pDotName;
    char * pDotNameWin = "dot.exe";
    char * pDotNameUnix = "dot";
    char * pGsNameWin = "gsview32.exe";
    char * pGsNameUnix = "gv";
    int RetValue;

    // get DOT names from the resource file
    if ( Abc_FrameReadFlag("dotwin") )
        pDotNameWin = Abc_FrameReadFlag("dotwin");
    if ( Abc_FrameReadFlag("dotunix") )
        pDotNameUnix = Abc_FrameReadFlag("dotunix");

#ifdef WIN32
    pDotName = pDotNameWin;
#else
    pDotName = pDotNameUnix;
#endif

    // check if the input DOT file is okay
    if ( (pFile = fopen( FileNameDot, "r" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        return;
    }
    fclose( pFile );

    // create the PostScript file name
    FileGeneric = Extra_FileNameGeneric( FileNameDot );
    sprintf( FileNamePs,  "%s.ps",  FileGeneric ); 
    free( FileGeneric );

    // generate the PostScript file using DOT
    sprintf( CommandDot,  "%s -Tps -o %s %s", pDotName, FileNamePs, FileNameDot ); 
    RetValue = system( CommandDot );
    if ( RetValue == -1 )
    {
        fprintf( stdout, "Command \"%s\" did not succeed.\n", CommandDot );
        return;
    }
    // check that the input PostScript file is okay
    if ( (pFile = fopen( FileNamePs, "r" )) == NULL )
    {
        fprintf( stdout, "Cannot open intermediate file \"%s\".\n", FileNamePs );
        return;
    }
    fclose( pFile ); 


    // get GSVIEW names from the resource file
    if ( Abc_FrameReadFlag("gsviewwin") )
        pGsNameWin = Abc_FrameReadFlag("gsviewwin");
    if ( Abc_FrameReadFlag("gsviewunix") )
        pGsNameUnix = Abc_FrameReadFlag("gsviewunix");

    // spawn the viewer
#ifdef WIN32
    _unlink( FileNameDot );
    if ( _spawnl( _P_NOWAIT, pGsNameWin, pGsNameWin, FileNamePs, NULL ) == -1 )
        if ( _spawnl( _P_NOWAIT, "C:\\Program Files\\Ghostgum\\gsview\\gsview32.exe", 
            "C:\\Program Files\\Ghostgum\\gsview\\gsview32.exe", FileNamePs, NULL ) == -1 )
        {
            fprintf( stdout, "Cannot find \"%s\".\n", pGsNameWin );
            return;
        }
#else
    {
        char CommandPs[1000];
        unlink( FileNameDot );
        sprintf( CommandPs,  "%s %s &", pGsNameUnix, FileNamePs ); 
        if ( system( CommandPs ) == -1 )
        {
            fprintf( stdout, "Cannot execute \"%s\".\n", CommandPs );
            return;
        }
    }
#endif
}

/**Function*************************************************************

  Synopsis    [Derives the DOT file name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ShowGetFileName( char * pName, char * pBuffer )
{
    char * pCur;
    // creat the file name
    sprintf( pBuffer, "%s.dot", pName );
    // get rid of not-alpha-numeric characters
    for ( pCur = pBuffer; *pCur; pCur++ )
        if ( !((*pCur >= '0' && *pCur <= '9') || (*pCur >= 'a' && *pCur <= 'z') || 
               (*pCur >= 'A' && *pCur <= 'Z') || (*pCur == '.')) )
            *pCur = '_';
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


