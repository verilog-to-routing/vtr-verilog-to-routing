/**CFile****************************************************************

  FileName    [main.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The main package.]

  Synopsis    [Here everything starts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: main.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "mainInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Initialization procedure for the library project.]

  Description [Note that when Abc_Start() is run in a static library
  project, it does not load the resource file by default. As a result, 
  ABC is not set up the same way, as when it is run on a command line. 
  For example, some error messages while parsing files will not be 
  produced, and intermediate networks will not be checked for consistancy. 
  One possibility is to load the resource file after Abc_Start() as follows:
  Abc_UtilsSource(  Abc_FrameGetGlobalFrame() );]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_Start()
{
    Abc_Frame_t * pAbc;
    // added to detect memory leaks:
#if defined(_DEBUG) && defined(_MSC_VER) 
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
    // start the glocal frame
    pAbc = Abc_FrameGetGlobalFrame();
    // source the resource file
//    Abc_UtilsSource( pAbc );
}

/**Function*************************************************************

  Synopsis    [Deallocation procedure for the library project.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_Stop()
{
    Abc_Frame_t * pAbc;
    pAbc = Abc_FrameGetGlobalFrame();
    // perform uninitializations
    Abc_FrameEnd( pAbc );
    // stop the framework
    Abc_FrameDeallocate( pAbc );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

