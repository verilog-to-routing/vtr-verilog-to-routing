/**CFile****************************************************************

  FileName    [ifLibBox.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Box library.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifLibBox.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"
#include "misc/extra/extra.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define If_LibBoxForEachBox( p, pBox, i )     \
    Vec_PtrForEachEntry( If_Box_t *, p->vBoxes, pBox, i ) if ( pBox == NULL ) {} else

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Box_t * If_BoxStart( char * pName, int Id, int nPis, int nPos, int fSeq, int fBlack, int fOuter )
{
    If_Box_t * p;
    p = ABC_CALLOC( If_Box_t, 1 );
    p->pName   = pName; // consumes memory
    p->Id      = Id;
    p->fSeq    = (char)fSeq;
    p->fBlack  = (char)fBlack;
    p->fOuter  = (char)fOuter;
    p->nPis    = nPis;
    p->nPos    = nPos;
    p->pDelays = ABC_CALLOC( int, nPis * nPos );
    return p;
}
If_Box_t * If_BoxDup( If_Box_t * p )
{
    If_Box_t * pNew = NULL;
    return pNew;
}
void If_BoxFree( If_Box_t * p )
{
    ABC_FREE( p->pDelays );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_LibBox_t * If_LibBoxStart()
{
    If_LibBox_t * p;
    p = ABC_CALLOC( If_LibBox_t, 1 );
    p->vBoxes = Vec_PtrAlloc( 100 );
    return p;
}
If_LibBox_t * If_LibBoxDup( If_Box_t * p )
{
    If_LibBox_t * pNew = NULL;
    return pNew;
}
void If_LibBoxFree( If_LibBox_t * p )
{
    If_Box_t * pBox;
    int i;
    if ( p == NULL )
        return;
    If_LibBoxForEachBox( p, pBox, i )
        If_BoxFree( pBox );
    Vec_PtrFree( p->vBoxes );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Box_t * If_LibBoxReadBox( If_LibBox_t * p, int Id )
{
    return (If_Box_t *)Vec_PtrEntry( p->vBoxes, Id );
}
If_Box_t * If_LibBoxFindBox( If_LibBox_t * p, char * pName )
{
    If_Box_t * pBox;
    int i;
    if ( p == NULL )
        return NULL;
    If_LibBoxForEachBox( p, pBox, i )
        if ( !strcmp(pBox->pName, pName) )
            return pBox;
    return NULL;
}
void If_LibBoxAdd( If_LibBox_t * p, If_Box_t * pBox )
{
    if ( pBox->Id >= Vec_PtrSize(p->vBoxes) )
        Vec_PtrFillExtra( p->vBoxes, 2 * pBox->Id + 10, NULL );
    assert( Vec_PtrEntry( p->vBoxes, pBox->Id ) == NULL );
    Vec_PtrWriteEntry( p->vBoxes, pBox->Id, pBox );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_LibBox_t * If_LibBoxRead2( char * pFileName )
{
    int nSize = 100000;
    char * pBuffer;
    FILE * pFile;
    If_LibBox_t * p = NULL;
    If_Box_t * pBox = NULL;
    char * pToken, * pName;
    int fSeq, fBlack, fOuter;
    int i, Id, nPis, nPos;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\".\n", pFileName );
        return NULL;
    }
    // read lines
    nPis = nPos = 0;
    pBuffer = ABC_ALLOC( char, nSize );
    while ( fgets( pBuffer, nSize, pFile ) )
    {
        pToken = strtok( pBuffer, " \n\r\t" );
        if ( pToken == NULL )
            continue;
        if ( pToken[0] == '.' )
        {
            if ( !strcmp(pToken, ".box") )
            {
                // save ID
                pToken = strtok( NULL, " \n\r\t" );
                Id     = atoi( pToken );
                // save name
                pToken = strtok( NULL, " \n\r\t" );
                pName  = Abc_UtilStrsav(pToken);
                // save PIs
                pToken = strtok( NULL, " \n\r\t" );
                nPis   = atoi( pToken );
                // save POs
                pToken = strtok( NULL, " \n\r\t" );
                nPos   = atoi( pToken );
                // save attributes
                fSeq = fBlack = fOuter = 0;
                pToken = strtok( NULL, " \n\r\t" );
                while ( pToken )
                {
                    if ( !strcmp(pToken, "seq") )
                        fSeq = 1;
                    else if ( !strcmp(pToken, "black") )
                        fBlack = 1;
                    else if ( !strcmp(pToken, "outer") )
                        fOuter = 1;
                    else assert( !strcmp(pToken, "comb") || !strcmp(pToken, "white") || !strcmp(pToken, "inner") );
                    pToken = strtok( NULL, " \n\r\t" );
                }
                // create library
                if ( p == NULL )
                    p = If_LibBoxStart();
                // create box
                pBox = If_BoxStart( pName, Id, nPis, nPos, fSeq, fBlack, fOuter );
                If_LibBoxAdd( p, pBox );
            }
            continue;
        }
        // read the table
        assert( nPis > 0 && nPos > 0 );
        for ( i = 0; i < nPis * nPos; i++ )
        {
            while ( pToken == NULL )
            {
                if ( fgets( pBuffer, nSize, pFile ) == NULL )
                { printf( "The table does not have enough entries.\n" ); fflush(stdout); assert( 0 ); }
                pToken = strtok( pBuffer, " \n\r\t" );
            }
            pBox->pDelays[i] = (pToken[0] == '-') ? -1 : atoi(pToken);
            pToken = strtok( NULL, " \n\r\t" );
        }
        pBox = NULL;
    }
    ABC_FREE( pBuffer );
    fclose( pFile );
    return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * If_LibBoxGetToken( FILE * pFile )
{
    static char pBuffer[1000];
    int c; char * pTemp = pBuffer;
    while ( (c = fgetc(pFile)) != EOF )
    {
        if ( c == '#' )
        {
            while ( (c = fgetc(pFile)) != EOF )
                if ( c == '\n' )
                    break;
        }
        if ( c == ' ' || c == '\t' || c == '\n' || c == '\r' )
        {
            if ( pTemp > pBuffer )
                break;
            continue;
        }
        *pTemp++ = c;
    }
    *pTemp = 0;
    return pTemp > pBuffer ? pBuffer : NULL;
}
If_LibBox_t * If_LibBoxRead( char * pFileName )
{
    FILE * pFile;
    If_LibBox_t * p;
    If_Box_t * pBox;
    char * pToken, * pName;
    int i, Id, fBlack, nPis, nPos;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\".\n", pFileName );
        return NULL;
    }
    // get the library name
    pToken = If_LibBoxGetToken( pFile );
    if ( pToken == NULL )
    {
        fclose( pFile );
        printf( "Cannot read library name from file \"%s\".\n", pFileName );
        return NULL;
    }
    if ( pToken[0] == '.' )    
    {
        fclose( pFile );
        printf( "Wrong box format. Please try \"read_box -e\".\n" );
        return NULL;
    }
       
    // create library
    p = If_LibBoxStart();
    while ( pToken )
    {
        // save name
        pName  = Abc_UtilStrsav(pToken);
        // save ID
        pToken = If_LibBoxGetToken( pFile );
        Id     = atoi( pToken );
        // save white/black
        pToken = If_LibBoxGetToken( pFile );
        fBlack = !atoi( pToken );
        // save PIs
        pToken = If_LibBoxGetToken( pFile );
        nPis   = atoi( pToken );
        // save POs
        pToken = If_LibBoxGetToken( pFile );
        nPos   = atoi( pToken );
        // create box
        pBox   = If_BoxStart( pName, Id, nPis, nPos, 0, fBlack, 0 );
        If_LibBoxAdd( p, pBox );
        // read the table
        for ( i = 0; i < nPis * nPos; i++ )
        {
            pToken = If_LibBoxGetToken( pFile );
            pBox->pDelays[i] = (pToken[0] == '-') ? -ABC_INFINITY : atoi(pToken);
        }
        // extract next name
        pToken = If_LibBoxGetToken( pFile );
    }
    fclose( pFile );
    return p;
}
void If_LibBoxPrint( FILE * pFile, If_LibBox_t * p )
{
    If_Box_t * pBox;
    int i, j, k;
    fprintf( pFile, "# Box library written by ABC on %s.\n", Extra_TimeStamp() );
    fprintf( pFile, "# <Name> <ID> <Type> <I> <O>\n" );
    If_LibBoxForEachBox( p, pBox, i )
    {
        fprintf( pFile, "%s %d %d %d %d\n", pBox->pName, pBox->Id, !pBox->fBlack, pBox->nPis, pBox->nPos );
        for ( j = 0; j < pBox->nPos; j++, printf("\n") )
            for ( k = 0; k < pBox->nPis; k++ )
                if ( pBox->pDelays[j * pBox->nPis + k] == -ABC_INFINITY )
                    fprintf( pFile, "    - " );
                else
                    fprintf( pFile, "%5d ", pBox->pDelays[j * pBox->nPis + k] );
    }
}
void If_LibBoxWrite( char * pFileName, If_LibBox_t * p )
{
    FILE * pFile;
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\".\n", pFileName );
        return;
    }
    If_LibBoxPrint( pFile, p );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_LibBoxLoad( char * pFileName )
{
    FILE * pFile;
    If_LibBox_t * pLib;
    char * pFileNameOther;
    // check if library can be read
    pFileNameOther = Extra_FileNameGenericAppend( pFileName, ".cdl" );
    pFile = fopen( pFileNameOther, "r" );
    if ( pFile == NULL )
        return 0;
    fclose( pFile );
    // read library
    pLib = If_LibBoxRead2( pFileNameOther );
    // replace the current library
    If_LibBoxFree( (If_LibBox_t *)Abc_FrameReadLibBox() );
    Abc_FrameSetLibBox( pLib );
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

