/**CFile****************************************************************

  FileName    [msatRead.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [The reader of the CNF formula in DIMACS format.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: msatRead.c,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/

#include "msatInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static char * Msat_FileRead( FILE * pFile );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Read the file into the internal buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Msat_FileRead( FILE * pFile )
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
static void Msat_ReadWhitespace( char ** pIn ) 
{
    while ((**pIn >= 9 && **pIn <= 13) || **pIn == 32)
        (*pIn)++; 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Msat_ReadNotWhitespace( char ** pIn ) 
{
    while ( !((**pIn >= 9 && **pIn <= 13) || **pIn == 32) )
        (*pIn)++; 
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
static int Msat_ReadInt( char ** pIn ) 
{
    int     val = 0;
    int     neg = 0;

    Msat_ReadWhitespace( pIn );
    if ( **pIn == '-' ) 
        neg = 1, 
        (*pIn)++;
    else if ( **pIn == '+' ) 
        (*pIn)++;
    if ( **pIn < '0' || **pIn > '9' ) 
        fprintf(stderr, "PARSE ERROR! Unexpected char: %c\n", **pIn), 
        exit(1);
    while ( **pIn >= '0' && **pIn <= '9' )
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
static void Msat_ReadClause( char ** pIn, Msat_Solver_t * p, Msat_IntVec_t * pLits ) 
{
    int nVars = Msat_SolverReadVarNum( p );
    int parsed_lit, var, sign;

    Msat_IntVecClear( pLits );
    while ( 1 )
    {
        parsed_lit = Msat_ReadInt(pIn);
        if ( parsed_lit == 0 ) 
            break;
        var = abs(parsed_lit) - 1;
        sign = (parsed_lit > 0);
        if ( var >= nVars )
        {
            printf( "Variable %d is larger than the number of allocated variables (%d).\n", var+1, nVars );
            exit(1);
        }
        Msat_IntVecPush( pLits, MSAT_VAR2LIT(var, !sign) );
    }
}
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int  Msat_ReadDimacs( char * pText, Msat_Solver_t ** pS, int  fVerbose ) 
{
    Msat_Solver_t * p = NULL; // Suppress "might be used uninitialized"
    Msat_IntVec_t * pLits = NULL; // Suppress "might be used uninitialized"
    char * pIn = pText;
    int nVars, nClas;
    while ( 1 )
    {
        Msat_ReadWhitespace( &pIn );
        if ( *pIn == 0 )
            break;
        else if ( *pIn == 'c' )
            skipLine( &pIn );
        else if ( *pIn == 'p' )
        {
            pIn++;
            Msat_ReadWhitespace( &pIn );
            Msat_ReadNotWhitespace( &pIn );

            nVars = Msat_ReadInt( &pIn );
            nClas = Msat_ReadInt( &pIn );
            skipLine( &pIn );
            // start the solver
            p = Msat_SolverAlloc( nVars, 1, 1, 1, 1, 0 ); 
            Msat_SolverClean( p, nVars );
            Msat_SolverSetVerbosity( p, fVerbose );
            // allocate the vector
            pLits = Msat_IntVecAlloc( nVars );
        }
        else
        {
            if ( p == NULL )
            {
                printf( "There is no parameter line.\n" );
                exit(1);
            }
            Msat_ReadClause( &pIn, p, pLits );
            if ( !Msat_SolverAddClause( p, pLits ) )
                return 0;
        }
    }
    Msat_IntVecFree( pLits );
    *pS = p;
    return Msat_SolverSimplifyDB( p );
}

/**Function*************************************************************

  Synopsis    [Starts the solver and reads the DIMAC file.]

  Description [Returns FALSE upon immediate conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Msat_SolverParseDimacs( FILE * pFile, Msat_Solver_t ** p, int fVerbose )
{
    char * pText;
    int  Value;
    pText = Msat_FileRead( pFile );
    Value = Msat_ReadDimacs( pText, p, fVerbose );
    ABC_FREE( pText );
    return Value;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

