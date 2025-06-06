/**CFile****************************************************************

  FileName    [giaIiff.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Boolean matching.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaIiff.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/st/st.h"
#include "map/mio/mio.h"

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
int Gia_ManDeriveMatches( Vec_Ptr_t ** pvNames, Vec_Wrd_t ** pvTruths, Vec_Int_t ** pvTt2Match4, Vec_Int_t ** pvConfigs, Vec_Mem_t * pvTtMem2[3], Vec_Int_t * pvTt2Match2[3] )
{
    return 0;
}
Gia_Man_t * Gia_ManIiffTest( char * pFileName, Gia_Man_t * pGia, int nLutSize, int nNumCuts, int fUseGates, int fUseCells, int fUseLuts, int fVerbose )
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

