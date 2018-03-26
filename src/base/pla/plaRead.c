/**CFile****************************************************************

  FileName    [plaRead.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SOP manager.]

  Synopsis    [Scalable SOP transformations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - March 18, 2015.]

  Revision    [$Id: plaRead.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "pla.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Pla_ReadFile( char * pFileName, char ** ppLimit )
{
    char * pBuffer;
    int nFileSize, RetValue;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open input file.\n" );
        return NULL;
    }
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    // move the file current reading position to the beginning
    rewind( pFile ); 
    // load the contents of the file into memory
    pBuffer = ABC_ALLOC( char, nFileSize + 16 );
    pBuffer[0] = '\n';
    RetValue = fread( pBuffer+1, nFileSize, 1, pFile );
    fclose( pFile );
    // terminate the string with '\0'
    pBuffer[nFileSize + 1] = '\n';
    pBuffer[nFileSize + 2] = '\0';
    *ppLimit = pBuffer + nFileSize + 3;
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pla_ReadPlaRemoveComments( char * pBuffer, char * pLimit )
{
    char * pTemp; 
    for ( pTemp = pBuffer; pTemp < pLimit; pTemp++ )
        if ( *pTemp == '#' )
            while ( *pTemp && *pTemp != '\n' )
                *pTemp++ = ' ';
}
int Pla_ReadPlaHeader( char * pBuffer, char * pLimit, int * pnIns, int * pnOuts, int * pnCubes, int * pType )
{
    char * pTemp; 
    *pType = PLA_FILE_FD;
    *pnIns = *pnOuts = *pnCubes = -1;
    for ( pTemp = pBuffer; pTemp < pLimit; pTemp++ )
    {
        if ( *pTemp != '.' )
            continue;
        if ( !strncmp(pTemp, ".i ", 3) )
            *pnIns = atoi( pTemp + 3 );
        else if ( !strncmp(pTemp, ".o ", 3) )
            *pnOuts = atoi( pTemp + 3 );
        else if ( !strncmp(pTemp, ".p ", 3) )
            *pnCubes = atoi( pTemp + 3 );
        else if ( !strncmp(pTemp, ".e ", 3) )
            break;
        else if ( !strncmp(pTemp, ".type ", 6) )
        {
            char Buffer[100];
            *pType = PLA_FILE_NONE;
            sscanf( pTemp+6, "%s", Buffer );
            if ( !strcmp(Buffer, "f") )
                *pType = PLA_FILE_F;
            else if ( !strcmp(Buffer, "fr") )
                *pType = PLA_FILE_FR;
            else if ( !strcmp(Buffer, "fd") )
                *pType = PLA_FILE_FD;
            else if ( !strcmp(Buffer, "fdr") )
                *pType = PLA_FILE_FDR;
        }
    }
    if ( *pnIns <= 0 )
        printf( "The number of inputs (.i) should be positive.\n" );
    if ( *pnOuts <= 0 )
        printf( "The number of outputs (.o) should be positive.\n" );
    return *pnIns > 0 && *pnOuts > 0;
}
Vec_Str_t * Pla_ReadPlaBody( char * pBuffer, char * pLimit, Pla_File_t Type )
{
    char * pTemp;
    Vec_Str_t * vLits;
    vLits = Vec_StrAlloc( 10000 );
    for ( pTemp = pBuffer; pTemp < pLimit; pTemp++ )
    {
        if ( *pTemp == '.' )
            while ( *pTemp && *pTemp != '\n' )
                pTemp++;
        if ( *pTemp == '0' )
            Vec_StrPush( vLits, (char)PLA_LIT_ZERO );
        else if ( *pTemp == '1' )
            Vec_StrPush( vLits, (char)PLA_LIT_ONE );
        else if ( *pTemp == '-' || *pTemp == '2' )
            Vec_StrPush( vLits, (char)PLA_LIT_DASH );
        else if ( *pTemp == '~' ) // no meaning
        {
            if ( Type == PLA_FILE_F || Type == PLA_FILE_FD )
                Vec_StrPush( vLits, (char)PLA_LIT_ZERO );
            else if ( Type == PLA_FILE_FR )
                Vec_StrPush( vLits, (char)PLA_LIT_DASH );
            else if ( Type == PLA_FILE_FDR )
                Vec_StrPush( vLits, (char)PLA_LIT_FULL );
            else assert( 0 );
        }
    }
    return vLits;
}
void Pla_ReadAddBody( Pla_Man_t * p, Vec_Str_t * vLits )
{
    word * pCubeIn, * pCubeOut; 
    int i, k, Lit, Count = 0; 
    int nCubesReal = Vec_StrSize(vLits) / (p->nIns + p->nOuts);
    assert( Vec_StrSize(vLits) % (p->nIns + p->nOuts) == 0 );
    if ( nCubesReal != Pla_ManCubeNum(p) )
    {
        printf( "Warning: Declared number of cubes (%d) differs from the actual (%d).\n", 
            Pla_ManCubeNum(p), nCubesReal );
        if ( nCubesReal < Pla_ManCubeNum(p) )
            Vec_IntShrink( &p->vCubes, nCubesReal );
        else
        {
            assert( nCubesReal > Pla_ManCubeNum(p) );
            Vec_IntFillNatural( &p->vCubes, nCubesReal );
            Vec_WrdFillExtra( &p->vInBits,  nCubesReal * p->nInWords,  0 );
            Vec_WrdFillExtra( &p->vOutBits, nCubesReal * p->nOutWords, 0 );
        }
    }
    Pla_ForEachCubeInOut( p, pCubeIn, pCubeOut, i )
    {
        Pla_CubeForEachLit( p->nIns, pCubeIn, Lit, k )
            Pla_CubeSetLit( pCubeIn, k, (Pla_Lit_t)Vec_StrEntry(vLits, Count++) );
        Pla_CubeForEachLit( p->nOuts, pCubeOut, Lit, k )
            Pla_CubeSetLit( pCubeOut, k, (Pla_Lit_t)Vec_StrEntry(vLits, Count++) );
    }
    assert( Count == Vec_StrSize(vLits) );
}
Pla_Man_t * Pla_ReadPla( char * pFileName )
{   
    Pla_Man_t * p;
    Vec_Str_t * vLits;
    int nIns, nOuts, nCubes, Type;
    char * pBuffer, * pLimit;   
    pBuffer = Pla_ReadFile( pFileName, &pLimit );
    if ( pBuffer == NULL )
        return NULL;
    Pla_ReadPlaRemoveComments( pBuffer, pLimit );
    if ( Pla_ReadPlaHeader( pBuffer, pLimit, &nIns, &nOuts, &nCubes, &Type ) )
    {
        vLits = Pla_ReadPlaBody( pBuffer, pLimit, (Pla_File_t)Type );
        if ( Vec_StrSize(vLits) % (nIns + nOuts) == 0 )
        {
            if ( nCubes == -1 )
                nCubes = Vec_StrSize(vLits) / (nIns + nOuts);
            p = Pla_ManAlloc( pFileName, nIns, nOuts, nCubes );
            p->Type = (Pla_File_t)Type;
            Pla_ReadAddBody( p, vLits );
            Vec_StrFree( vLits );
            ABC_FREE( pBuffer );
            return p;
        }
        printf( "Literal count is incorrect (in = %d; out = %d; lit = %d).\n", nIns, nOuts, Vec_StrSize(vLits) );
        Vec_StrFree( vLits );
    }
    ABC_FREE( pBuffer );
    return NULL;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

