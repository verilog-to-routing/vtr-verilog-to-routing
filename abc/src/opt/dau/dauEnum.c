/**CFile****************************************************************

  FileName    [dauEnum.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware unmapping.]

  Synopsis    [Enumeration of decompositions.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dauEnum.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dauInt.h"

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
char * Dau_EnumLift( char * pName, int Shift )
{
    static char pBuffer[64];
    char * pTemp;
    for ( pTemp = pBuffer; *pName; pTemp++, pName++ )
        *pTemp = (*pName >= 'a' && *pName <= 'z') ? *pName + Shift : *pName;
    *pTemp = 0;
    return pBuffer;
}
char * Dau_EnumLift2( char * pName, int Shift )
{
    static char pBuffer[64];
    char * pTemp;
    for ( pTemp = pBuffer; *pName; pTemp++, pName++ )
        *pTemp = (*pName >= 'a' && *pName <= 'z') ? *pName + Shift : *pName;
    *pTemp = 0;
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_EnumCombineTwo( Vec_Ptr_t * vOne, int fStar, int fXor, char * pName1, char * pName2, int Shift2, int fCompl1, int fCompl2 )
{
    static char pBuffer[256];
    pName2 = Dau_EnumLift( pName2, Shift2 );
    sprintf( pBuffer, "%s%c%s%s%s%s%c", 
        fStar?"*":"", 
        fXor?'[':'(', 
        fCompl1?"!":"", pName1[0] == '*' ? pName1 + 1 : pName1, 
        fCompl2?"!":"", pName2[0] == '*' ? pName2 + 1 : pName2, 
        fXor?']':')' );
//    printf( "%s ", pBuffer );
    Vec_PtrPush( vOne, Abc_UtilStrsav(pBuffer) );
}
void Dau_EnumCombineThree( Vec_Ptr_t * vOne, int fStar, char * pNameC, char * pName1, char * pName2, int Shift1, int Shift2, int fComplC, int fCompl1, int fCompl2 )
{
    static char pBuffer[256];
    pName1 = Dau_EnumLift( pName1, Shift1 );
    pName2 = Dau_EnumLift2( pName2, Shift2 );
    sprintf( pBuffer, "%s%c%s%s%s%s%s%s%c", 
        fStar?"*":"", 
        '<', 
        fComplC?"!":"", pNameC[0] == '*' ? pNameC + 1 : pNameC, 
        fCompl1?"!":"", pName1[0] == '*' ? pName1 + 1 : pName1, 
        fCompl2?"!":"", pName2[0] == '*' ? pName2 + 1 : pName2, 
        '>' );
//    printf( "%s ", pBuffer );
    Vec_PtrPush( vOne, Abc_UtilStrsav(pBuffer) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_EnumTestDump( Vec_Ptr_t * vSets, char * pFileName )
{ 
    FILE * pFile;
    Vec_Ptr_t * vOne;
    char * pName;
    int v, k;
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
        return;
    Vec_PtrForEachEntry( Vec_Ptr_t *, vSets, vOne, v )
    {
        fprintf( pFile, "VARIABLE NUMBER %d:\n", v );
        Vec_PtrForEachEntry( char *, vOne, pName, k )
            fprintf( pFile, "%s\n", pName );
    }
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_EnumTest()
{
    int v, k, nVarMax = 10;
    Vec_Ptr_t * vSets;
    Vec_Ptr_t * vOne;
    char * pName;
    // 0 vars
    vSets = Vec_PtrAlloc( 16 );
    Vec_PtrPush( vSets, Vec_PtrAlloc(0) );
    // 1 vars
    vOne = Vec_PtrAlloc( 1 );
    Vec_PtrPush( vOne, Abc_UtilStrsav("*a") );
    Vec_PtrPush( vSets, vOne );
    // 2+ vars
    for ( v = 2; v <= nVarMax; v++ )
    {
        Vec_Ptr_t * vSetI, * vSetJ, * vSetK;
        char * pNameI, * pNameJ, * pNameK;
        int i, j, k, i1, j1, k1;
        vOne = Vec_PtrAlloc( 100 );
        for ( i = 1; i < v; i++ )
        for ( j = i; j < v; j++ )
        {
            if ( i + j != v )
                continue;
            vSetI = (Vec_Ptr_t *)Vec_PtrEntry( vSets, i );
            vSetJ = (Vec_Ptr_t *)Vec_PtrEntry( vSets, j );
            Vec_PtrForEachEntry( char *, vSetI, pNameI, i1 )
            Vec_PtrForEachEntry( char *, vSetJ, pNameJ, j1 )
            {
                // AND(a,b)
                Dau_EnumCombineTwo( vOne, 0, 0, pNameI, pNameJ, i, 0, 0 );
                // AND(!a,b)
                if ( pNameI[0] != '*' )
                Dau_EnumCombineTwo( vOne, 0, 0, pNameI, pNameJ, i, 1, 0 );
                // AND(a,!b)
                if ( pNameJ[0] != '*' && !(i == j && i1 == j1) )
                Dau_EnumCombineTwo( vOne, 0, 0, pNameI, pNameJ, i, 0, 1 );
                // AND(!a,!b)
                if ( pNameI[0] != '*' && pNameJ[0] != '*' )
                Dau_EnumCombineTwo( vOne, 0, 0, pNameI, pNameJ, i, 1, 1 );
                // XOR(a,b)
                Dau_EnumCombineTwo( vOne, pNameI[0] == '*' || pNameJ[0] == '*', 1, pNameI, pNameJ, i, 0, 0 );
            }
        }
        for ( k = 1; k < v; k++ )
        for ( i = 1; i < v; i++ )
        for ( j = i; j < v; j++ )
        {
            if ( k + i + j != v )
                continue;
            vSetK = (Vec_Ptr_t *)Vec_PtrEntry( vSets, k );
            vSetI = (Vec_Ptr_t *)Vec_PtrEntry( vSets, i );
            vSetJ = (Vec_Ptr_t *)Vec_PtrEntry( vSets, j );
            Vec_PtrForEachEntry( char *, vSetK, pNameK, k1 )
            Vec_PtrForEachEntry( char *, vSetI, pNameI, i1 )
            Vec_PtrForEachEntry( char *, vSetJ, pNameJ, j1 )
            {
                int fStar = pNameI[0] == '*' && pNameJ[0] == '*';

                // MUX(c,a,b)
                Dau_EnumCombineThree( vOne, fStar, pNameK, pNameI, pNameJ, k, k+i, 0, 0, 0 );
                // MUX(c,!a,b)
                if ( pNameI[0] != '*' )
                Dau_EnumCombineThree( vOne, fStar, pNameK, pNameI, pNameJ, k, k+i, 0, 1, 0 );
                // MUX(c,a,!b)
                if ( pNameJ[0] != '*' && !(i == j && i1 == j1) )
                Dau_EnumCombineThree( vOne, fStar, pNameK, pNameI, pNameJ, k, k+i, 0, 0, 1 );

                if ( pNameK[0] != '*' && !(i == j && i1 == j1) )
                {
                    // MUX(!c,a,b)
                    Dau_EnumCombineThree( vOne, fStar, pNameK, pNameI, pNameJ, k, k+i, 1, 0, 0 );
                    // MUX(!c,!a,b)
                    if ( pNameI[0] != '*' )
                    Dau_EnumCombineThree( vOne, fStar, pNameK, pNameI, pNameJ, k, k+i, 1, 1, 0 );
                    // MUX(!c,a,!b)
                    if ( pNameJ[0] != '*' )
                    Dau_EnumCombineThree( vOne, fStar, pNameK, pNameI, pNameJ, k, k+i, 1, 0, 1 );
                }
            }
        }
        Vec_PtrPush( vSets, vOne );
    }
    Dau_EnumTestDump( vSets, "_npn/npn/dsd10.txt" );

    Vec_PtrForEachEntry( Vec_Ptr_t *, vSets, vOne, v )
    {
        printf( "VARIABLE NUMBER %d:\n", v );
        Vec_PtrForEachEntry( char *, vOne, pName, k )
            printf( "%s\n", pName );
        if ( v == 4 )
            break;
    }
    Vec_PtrForEachEntry( Vec_Ptr_t *, vSets, vOne, v )
    {
        printf( "%d=%d ", v, Vec_PtrSize(vOne) );
        Vec_PtrFreeFree( vOne );
    }
    Vec_PtrFree( vSets );
    printf( "\n" );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

