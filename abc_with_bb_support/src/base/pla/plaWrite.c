/**CFile****************************************************************

  FileName    [plaWrite.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SOP manager.]

  Synopsis    [Scalable SOP transformations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - March 18, 2015.]

  Revision    [$Id: plaWrite.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

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
Vec_Str_t * Pla_WritePlaInt( Pla_Man_t * p )
{
    Vec_Str_t * vOut = Vec_StrAlloc( 10000 );
    char * pLits = "-01?";
    word * pCubeIn, * pCubeOut; 
    int i, k, Lit; 
    // write comments
    Vec_StrPrintStr( vOut, "# SOP \"" );
    Vec_StrPrintStr( vOut, Pla_ManName(p) );
    Vec_StrPrintStr( vOut, "\" written via PLA package in ABC on " );
    Vec_StrPrintStr( vOut, Extra_TimeStamp() );
    Vec_StrPrintStr( vOut, "\n\n" );
    // write header
    if ( p->Type != PLA_FILE_FD )
    {
        if ( p->Type == PLA_FILE_F )
            Vec_StrPrintStr( vOut, ".type f\n" );
        else if ( p->Type == PLA_FILE_FR )
            Vec_StrPrintStr( vOut, ".type fr\n" );
        else if ( p->Type == PLA_FILE_FDR )
            Vec_StrPrintStr( vOut, ".type fdr\n" );
        else if ( p->Type == PLA_FILE_NONE )
            Vec_StrPrintStr( vOut, ".type ???\n" );
    }
    Vec_StrPrintStr( vOut, ".i " );
    Vec_StrPrintNum( vOut, p->nIns );
    Vec_StrPrintStr( vOut, "\n.o " );
    Vec_StrPrintNum( vOut, p->nOuts );
    Vec_StrPrintStr( vOut, "\n.p " );
    Vec_StrPrintNum( vOut, Pla_ManCubeNum(p) );
    Vec_StrPrintStr( vOut, "\n" );
    // write cube
    Pla_ForEachCubeInOut( p, pCubeIn, pCubeOut, i )
    {
        Pla_CubeForEachLit( p->nIns, pCubeIn, Lit, k )
            Vec_StrPush( vOut, pLits[Lit] );
        Vec_StrPush( vOut, ' ' );
        Pla_CubeForEachLit( p->nOuts, pCubeOut, Lit, k )
            Vec_StrPush( vOut, pLits[Lit] );
        Vec_StrPush( vOut, '\n' );
    }
    Vec_StrPrintStr( vOut, ".e\n\n\0" );
    return vOut;
}   
void Pla_WritePla( Pla_Man_t * p, char * pFileName )
{
    Vec_Str_t * vOut = Pla_WritePlaInt( p );
    if ( Vec_StrSize(vOut) > 0 )
    {
        FILE * pFile = fopen( pFileName, "wb" );
        if ( pFile == NULL )
            printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        else
        {
            fwrite( Vec_StrArray(vOut), 1, Vec_StrSize(vOut), pFile );
            fclose( pFile );
        }
    }
    Vec_StrFreeP( &vOut );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

