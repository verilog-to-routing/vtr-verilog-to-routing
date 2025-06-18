/**CFile****************************************************************

  FileName    [xsatUtils.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [xSAT - A SAT solver written in C.
               Read the license file for more info.]

  Synopsis    [Utility functions used in xSAT]

  Author      [Bruno Schmitt <boschmitt@inf.ufrgs.br>]

  Affiliation [UC Berkeley / UFRGS]

  Date        [Ver. 1.0. Started - November 10, 2016.]

  Revision    []

***********************************************************************/
#ifndef ABC__sat__xSAT__xsatUtils_h
#define ABC__sat__xSAT__xsatUtils_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include "misc/util/abc_global.h"

ABC_NAMESPACE_HEADER_START

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_UtilSelectSort( void** pArray, int nSize, int(* CompFnct )( const void *, const void * ) )
{
    int i, j, iBest;
    void* pTmp;

    for ( i = 0; i < ( nSize - 1 ); i++ )
    {
        iBest = i;
        for ( j = i + 1; j < nSize; j++ )
        {
            if ( CompFnct( pArray[j], pArray[iBest] ) )
                iBest = j;
        }
        pTmp = pArray[i];
        pArray[i] = pArray[iBest];
        pArray[iBest] = pTmp;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static void xSAT_UtilSort( void** pArray, int nSize, int(* CompFnct )( const void *, const void *) )
{
    if ( nSize <= 15 )
        xSAT_UtilSelectSort( pArray, nSize, CompFnct );
    else
    {
        void* pPivot = pArray[nSize / 2];
        void* pTmp;
        int i = -1;
        int j = nSize;

        for(;;)
        {
            do i++; while( CompFnct( pArray[i], pPivot ) );
            do j--; while( CompFnct( pPivot, pArray[j] ) );

            if ( i >= j )
                break;

            pTmp = pArray[i];
            pArray[i] = pArray[j];
            pArray[j] = pTmp;
        }

        xSAT_UtilSort( pArray, i, CompFnct );
        xSAT_UtilSort( pArray + i, ( nSize - i ), CompFnct );
    }
}

ABC_NAMESPACE_HEADER_END

#endif
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
