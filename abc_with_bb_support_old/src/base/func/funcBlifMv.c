/**CFile****************************************************************

  FileName    [funcBlifMv.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Implementation of BLIF-MV representation of the nodes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: funcBlifMv.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"

/* 
    The BLIF-MV tables are represented using char * strings.
    For example, the representation of the table 

       .table c d0 d1 x
       .default 0
       0 - - =d0
       1 - 1 1

    is the string: "2 2 2 2\n0\n0 - - =1\n1 - 1 1\n" where '\n' is a single char.
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
Hop_Obj_t * Abc_ConvertBlifMvToAig( Hop_Man_t * pMan, char * pSop )
{
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


