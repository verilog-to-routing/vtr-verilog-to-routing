/**CFile****************************************************************

  FileName    [sbd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sbd.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sbdInt.h"
#include "misc/vec/vecHsh.h"

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
int Sbd_CountConfigVars( Vec_Int_t * vSet, int nVars, int Degree )
{
    int i, k, Entry = 0, Entry2, Count = 0, Below;
    int Prev = Vec_IntEntry( vSet, 0 );
    Vec_IntForEachEntryStart( vSet, Entry, i, 1 )
    {
        assert( Degree*Prev >= Entry );
        if ( Degree*Prev == Entry )
        {
            Prev = Entry;
            continue;
        }
        Below = nVars; 
        Vec_IntForEachEntryStart( vSet, Entry2, k, i )
            Below += Entry2;
        Count += Below * (Degree*Prev - 1);
        Prev = Entry;
    }
    Count += nVars * Degree*Prev;
    return Vec_IntSum(vSet) < nVars - 1 ? 0 : Count;
}
void Sbd_CountTopos()
{
    int nInputs  =  9;
    int nNodes   = 10;
    int Degree   =  3;
    int fVerbose =  1;
    Hsh_VecMan_t * p = Hsh_VecManStart( 10000 ); // hash table for arrays
    Vec_Int_t * vSet = Vec_IntAlloc( 100 );
    int i, k, e, Start, Stop;
    printf( "Counting topologies for %d inputs and %d degree-%d nodes.\n", nInputs, nNodes, Degree );
    Start = Hsh_VecManAdd( p, vSet );
    for ( i = 1; i <= nNodes; i++ )
    {
        Stop = Hsh_VecSize( p );
        for ( e = Start; e < Stop; e++ )
        {
            Vec_Int_t * vTemp = Hsh_VecReadEntry( p, e );
            Vec_IntClear( vSet );
            Vec_IntAppend( vSet, vTemp );
            for ( k = 0; k < Vec_IntSize(vSet); k++ )
            {
                // skip if the number of entries on this level is equal to the number of fanins on the previous level
                if ( k ? (Vec_IntEntry(vSet, k) == Degree*Vec_IntEntry(vSet, k-1)) : (Vec_IntEntry(vSet, 0) > 0) )
                    continue;
                Vec_IntAddToEntry( vSet, k, 1 );
                Hsh_VecManAdd( p, vSet );
                Vec_IntAddToEntry( vSet, k, -1 );
            }
            Vec_IntPush( vSet, 1 );
            Hsh_VecManAdd( p, vSet );
        }
        printf( "Nodes = %2d : This = %8d  All = %8d\n", i, Hsh_VecSize(p) - Stop, Hsh_VecSize(p) );
        if ( fVerbose )
        {
            for ( e = Stop; e < Hsh_VecSize(p); e++ )
            {
                Vec_Int_t * vTemp = Hsh_VecReadEntry( p, e );
                printf( "Params = %3d.  ", Sbd_CountConfigVars(vTemp, nInputs, Degree) );
                Vec_IntPrint( vTemp );
            }
            printf( "\n" );
        }
        Start = Stop;
    }
    Vec_IntFree( vSet );
    Hsh_VecManStop( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

