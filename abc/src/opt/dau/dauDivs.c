/**CFile****************************************************************

  FileName    [dauDivs.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware unmapping.]

  Synopsis    [Divisor computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dauDivs.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dauInt.h"
#include "misc/util/utilTruth.h"

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
Vec_Int_t * Dau_DsdDivisors( word * pTruth, int nVars )
{
    word Copy[DAU_MAX_WORD];
    int nWords = Abc_TtWordNum(nVars);
    int nDigits = Abc_TtHexDigitNum(nVars);
    int i, j, k, Digit, Counter[5];

    printf( "     " );
    printf( " !a *!b"  );
    printf( " !a * b"  );
    printf( "  a *!b"  );
    printf( "  a * b"  );
    printf( "  a + b"  );
    printf( "\n" );

    for ( i = 0; i < nVars; i++ )
    for ( j = i+1; j < nVars; j++ )
    {
        Abc_TtCopy( Copy, pTruth, nWords, 0 );
        Abc_TtSwapVars( Copy, nVars, 0, i );
        Abc_TtSwapVars( Copy, nVars, 1, j );
        for ( k = 0; k < 5; k++ )
            Counter[k] = 0;
        for ( k = 0; k < nDigits; k++ )
        {
            Digit = Abc_TtGetHex( Copy, k );
            if ( Digit == 1 || Digit == 14 )
                Counter[0]++;
            else if ( Digit == 2 || Digit == 13 )
                Counter[1]++;
            else if ( Digit == 4 || Digit == 11 )
                Counter[2]++;
            else if ( Digit == 8 || Digit == 7 )
                Counter[3]++;
            else if ( Digit == 6 || Digit == 9 )
                Counter[4]++;
        }
        printf( "%c %c  ", 'a'+i, 'a'+j );
        for ( k = 0; k < 5; k++ )
            printf( "%7d", Counter[k] );
        printf( "\n" );
    }
    return NULL;
}
void Dau_DsdTest000()
{
//    char * pDsd = "!(!(abc)!(def))";
//    char * pDsd = "[(abc)(def)]";
    char * pDsd = "<<abc>d(ef)>";
    word t = Dau_Dsd6ToTruth( pDsd );
//    word t = 0xCA88CA88CA88CA88;
//    word t = 0x9ef7a8d9c7193a0f;
    int nVars = Abc_TtSupportSize( &t, 6 );
    return;
//    word t = 0xCACACACACACACACA;
    Dau_DsdDivisors( &t, nVars );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

