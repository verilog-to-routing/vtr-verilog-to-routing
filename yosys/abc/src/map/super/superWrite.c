/**CFile****************************************************************

  FileName    [superWrite.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Pre-computation of supergates.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: superWrite.c,v 1.1 2004/04/03 01:36:45 alanmi Exp $]

***********************************************************************/

#include "superInt.h"

ABC_NAMESPACE_IMPL_START


/*
    One record in the supergate library file consists of:

    <gate_number> <truth_table> <delay_max> <pin-to-pin-delays> <area> <gate_formula>

    <gate_number>       is a zero-based integer
    <truth_table>       is a string of 2^n bits representing the value of the function for each minterm
    <delay_max>         is the maximum delay of the gate
    <pin-to-pin-delays> is the array of n double values
    <area>              is a floating point value
    <gate_formula>      is the string representing the gate in the following format:
        GATENAME1( GATENAME2( a, c ), GATENAME3( a, d ), ... )
        The gate names (GATENAME1, etc) are the names as they appear in the .genlib library.
        The primary inputs of the gates are denoted by lowercase chars 'a', 'b', etc.
        The parentheses are mandatory for each gate, except for the wire. 
        The wire name can be omitted, so that "a" can be used instead of "**wire**( a )".
        The spaces are optional in any position of this string.


    The supergates are generated exhaustively from all gate combinations that
    have the max delay lower than the delay given by the user, or until the specified time
    limit is reached.

    The supergates are stored in supergate classes by their functionality. 
    Among the gates with the equivalent functionaly only those are dropped, which are
    dominated by at least one other gate in the class in terms of both delay and area.
    For the definition of gate dominance see pliGenCheckDominance().
*/


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


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

