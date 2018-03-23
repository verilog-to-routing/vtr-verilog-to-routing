/**CFile****************************************************************

  FileName    [satChecker.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT sat_solver.]

  Synopsis    [Resolution proof checker.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: satChecker.c,v 1.4 2005/09/16 22:55:03 casem Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"

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
void Sat_PrintClause( Vec_Vec_t * vClauses, int Clause )
{
    Vec_Int_t * vClause;
    int i, Entry;
    printf( "Clause %d:  {", Clause );
    vClause = Vec_VecEntry( vClauses, Clause );
    Vec_IntForEachEntry( vClause, Entry, i )
        printf( " %d", Entry );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_ProofResolve( Vec_Vec_t * vClauses, int Result, int Clause1, int Clause2 )
{
    Vec_Int_t * vResult  = Vec_VecEntry( vClauses, Result );
    Vec_Int_t * vClause1 = Vec_VecEntry( vClauses, Clause1 );
    Vec_Int_t * vClause2 = Vec_VecEntry( vClauses, Clause2 );
    int Entry1, Entry2, ResVar;
    int i, j, Counter = 0;

    Vec_IntForEachEntry( vClause1, Entry1, i )
    Vec_IntForEachEntry( vClause2, Entry2, j )
    if ( Entry1 == -Entry2 )
    {
        ResVar = Entry1;
        Counter++;
    }
    if ( Counter != 1 )
    {
        printf( "Error: Clause %d = Resolve(%d, %d): The number of pivot vars is %d.\n", 
            Result, Clause1, Clause2, Counter );
        Sat_PrintClause( vClauses, Clause1 );
        Sat_PrintClause( vClauses, Clause2 );
        return 0;
    }
    // create new clause
    assert( Vec_IntSize(vResult) == 0 );
    Vec_IntForEachEntry( vClause1, Entry1, i )
        if ( Entry1 != ResVar && Entry1 != -ResVar )
            Vec_IntPushUnique( vResult, Entry1 );
    assert( Vec_IntSize(vResult) + 1 == Vec_IntSize(vClause1) );
    Vec_IntForEachEntry( vClause2, Entry2, i )
        if ( Entry2 != ResVar && Entry2 != -ResVar )
            Vec_IntPushUnique( vResult, Entry2 );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_ProofChecker( char * pFileName )
{
    FILE * pFile;
    Vec_Vec_t * vClauses;
    int c, i, Num, RetValue, Counter, Counter2, Clause1, Clause2;
    int RetValue;
    // open the file
    pFile = fopen( pFileName, "r" );
    if ( pFile == NULL )
        return;
    // count the number of clauses
    Counter = Counter2 = 0;
    while ( (c = fgetc(pFile)) != EOF )
    {
        Counter += (c == '\n');
        Counter2 += (c == '*');
    }
    vClauses = Vec_VecStart( Counter+1 );
    printf( "The proof contains %d roots and %d resolution steps.\n", Counter-Counter2, Counter2 );
    // read the clauses
    rewind( pFile );
    for ( i = 1 ; ; i++ )
    {
        RetValue = RetValue = fscanf( pFile, "%d", &Num );
        if ( RetValue != 1 )
            break;
        assert( Num == i );
        while ( (c = fgetc( pFile )) == ' ' );
        if ( c == '*' )
        {
            RetValue = fscanf( pFile, "%d %d", &Clause1, &Clause2 );
            assert( RetValue == 2 );
            RetValue = fscanf( pFile, "%d", &Num );
            assert( RetValue == 1 );
            assert( Num == 0 );
            if ( !Sat_ProofResolve( vClauses, i, Clause1, Clause2 ) )
            {
                printf( "Error detected in the resolution proof.\n" );
                Vec_VecFree( vClauses );
                fclose( pFile );
                return;
            }
        }
        else
        {
            ungetc( c, pFile );
            while ( 1 )
            {
                RetValue = fscanf( pFile, "%d", &Num );
                assert( RetValue == 1 );
                if ( Num == 0 )
                    break;
                Vec_VecPush( vClauses, i, (void *)Num );
            }
            RetValue = fscanf( pFile, "%d", &Num );
            assert( RetValue == 1 );
            assert( Num == 0 );
        }
    }
    assert( i-1 == Counter );
    if ( Vec_IntSize( Vec_VecEntry(vClauses, Counter) ) != 0 )
        printf( "The last clause is not empty.\n" );
    else
        printf( "The empty clause is derived.\n" );
    Vec_VecFree( vClauses );
    fclose( pFile );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

