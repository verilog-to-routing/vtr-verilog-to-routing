/**CFile****************************************************************

  FileName    [xsatCnfReader.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [xSAT - A SAT solver written in C.
               Read the license file for more info.]

  Synopsis    [CNF DIMACS file format parser.]

  Author      [Bruno Schmitt <boschmitt@inf.ufrgs.br>]

  Affiliation [UC Berkeley / UFRGS]

  Date        [Ver. 1.0. Started - November 10, 2016.]

  Revision    []

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include <ctype.h>

#include "misc/util/abc_global.h"
#include "misc/vec/vecInt.h"

#include "xsatSolver.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Read the file into the internal buffer.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * xSAT_FileRead( FILE * pFile )
{
    int nFileSize;
    char * pBuffer;
    int RetValue;
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );
    nFileSize = ftell( pFile );
    // move the file current reading position to the beginning
    rewind( pFile );
    // load the contents of the file into memory
    pBuffer = ABC_ALLOC( char, nFileSize + 3 );
    RetValue = fread( pBuffer, nFileSize, 1, pFile );
    // terminate the string with '\0'
    pBuffer[ nFileSize + 0] = '\n';
    pBuffer[ nFileSize + 1] = '\0';
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static void skipLine( char ** pIn )
{
    while ( 1 )
    {
        if (**pIn == 0)
            return;
        if (**pIn == '\n')
        {
            (*pIn)++;
            return;
        }
        (*pIn)++;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static int xSAT_ReadInt( char ** pIn )
{
    int val = 0;
    int neg = 0;

    for(; isspace(**pIn); (*pIn)++);
    if ( **pIn == '-' )
        neg = 1,
        (*pIn)++;
    else if ( **pIn == '+' )
        (*pIn)++;
    if ( !isdigit(**pIn) )
        fprintf(stderr, "PARSE ERROR! Unexpected char: %c\n", **pIn),
        exit(1);
    while ( isdigit(**pIn) )
        val = val*10 + (**pIn - '0'),
        (*pIn)++;
    return neg ? -val : val;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static void xSAT_ReadClause( char ** pIn, xSAT_Solver_t * p, Vec_Int_t * vLits )
{
    int token, var, sign;

    Vec_IntClear( vLits );
    while ( 1 )
    {
        token = xSAT_ReadInt( pIn );
        if ( token == 0 )
            break;
        var = abs(token) - 1;
        sign = (token > 0);
        Vec_IntPush( vLits, xSAT_Var2Lit( var, !sign ) );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static int xSAT_ParseDimacs( char * pText, xSAT_Solver_t ** pS )
{
    xSAT_Solver_t * p = NULL;
    Vec_Int_t * vLits = NULL;
    char * pIn = pText;
    int nVars, nClas;
    while ( 1 )
    {
        for(; isspace(*pIn); pIn++);
        if ( *pIn == 0 )
            break;
        else if ( *pIn == 'c' )
            skipLine( &pIn );
        else if ( *pIn == 'p' )
        {
            pIn++;
            for(; isspace(*pIn); pIn++);
            for(; !isspace(*pIn); pIn++);

            nVars = xSAT_ReadInt( &pIn );
            nClas = xSAT_ReadInt( &pIn );
            skipLine( &pIn );

            /* start the solver */
            p = xSAT_SolverCreate();
            /* allocate the vector */
            vLits = Vec_IntAlloc( nVars );
        }
        else
        {
            if ( p == NULL )
            {
                printf( "There is no parameter line.\n" );
                exit(1);
            }
            xSAT_ReadClause( &pIn, p, vLits );
            if ( !xSAT_SolverAddClause( p, vLits ) )
            {
                Vec_IntPrint(vLits);
                return 0;
            }
        }
    }
    Vec_IntFree( vLits );
    *pS = p;
    return xSAT_SolverSimplify( p );
}

/**Function*************************************************************

  Synopsis    [Starts the solver and reads the DIMAC file.]

  Description [Returns FALSE upon immediate conflict.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
int xSAT_SolverParseDimacs( FILE * pFile, xSAT_Solver_t ** p )
{
    char * pText;
    int  Value;
    pText = xSAT_FileRead( pFile );
    Value = xSAT_ParseDimacs( pText, p );
    ABC_FREE( pText );
    return Value;
}

ABC_NAMESPACE_IMPL_END

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
