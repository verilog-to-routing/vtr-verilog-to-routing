/**CFile****************************************************************

  FileName    [wln.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Word-level network.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 23, 2018.]

  Revision    [$Id: wln.c,v 1.00 2018/09/23 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wln.h"

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
int Wln_ReadFindToken( char * pToken, Abc_Nam_t * p )
{
    char * pBuffer = Abc_UtilStrsavTwo( "\\", pToken );
    int RetValue = Abc_NamStrFindOrAdd( p, pBuffer, NULL );
    ABC_FREE( pBuffer );
    return RetValue;
}
void Wln_PrintGuidance( Vec_Wec_t * vGuide, Abc_Nam_t * p )
{
    Vec_Int_t * vLevel; int i, k, Obj; 
    Vec_WecForEachLevel( vGuide, vLevel, i )
    {
        Vec_IntForEachEntry( vLevel, Obj, k )
            printf( "%s ", Obj >= 0 ? Abc_NamStr(p, Obj) : "[unknown]" );
        printf( "\n" );
    }
}
Vec_Wec_t * Wln_ReadGuidance( char * pFileName, Abc_Nam_t * p )
{
    char * pBuffer = ABC_CALLOC( char, 10000 ), * pToken;
    Vec_Wec_t * vTokens = Vec_WecAlloc( 100 ); Vec_Int_t * vLevel;
    FILE * pFile = fopen( pFileName, "rb" );
    while ( fgets( pBuffer, 10000, pFile ) )
    {
        if ( pBuffer[0] == '#' )
            continue;
        vLevel = Vec_WecPushLevel( vTokens );
        pToken = strtok( pBuffer, " \t\r\n" );
        while ( pToken )
        {
            Vec_IntPush( vLevel, Vec_IntSize(vLevel) < 2 ? Abc_NamStrFindOrAdd(p, pToken, NULL) : Wln_ReadFindToken(pToken, p) );
            pToken = strtok( NULL, " \t\r\n" );
        }
        if ( Vec_IntSize(vLevel) % 4 == 3 ) // account for "property"
            Vec_IntPush( vLevel, -1 );
        assert( Vec_IntSize(vLevel) % 4 == 0 );
    }
    fclose( pFile );
    if ( Vec_WecSize(vTokens) == 0 )
        printf( "Guidance is empty.\n" );
    //Wln_PrintGuidance( vTokens, p );
    ABC_FREE( pBuffer );
    return vTokens;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

