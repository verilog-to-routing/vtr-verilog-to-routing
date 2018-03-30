/**CFile****************************************************************

  FileName    [abcDebug.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Automated debugging procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcDebug.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/io/ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Abc_NtkCountFaninsTotal( Abc_Ntk_t * pNtk );
static Abc_Ntk_t * Abc_NtkAutoDebugModify( Abc_Ntk_t * pNtk, int ObjNum, int fConst1 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Takes a network and a procedure to test.]

  Description [The network demonstrates the bug in the procedure.
  Procedure should return 1 if the bug is demonstrated.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkAutoDebug( Abc_Ntk_t * pNtk, int (*pFuncError) (Abc_Ntk_t *) )
{
    Abc_Ntk_t * pNtkMod;
    char * pFileName = "bug_found.blif";
    int i, nSteps, nIter, ModNum, RandNum = 1;
    abctime clk, clkTotal = Abc_Clock();
    assert( Abc_NtkIsLogic(pNtk) );
    srand( 0x123123 );
    // create internal copy of the network
    pNtk = Abc_NtkDup(pNtk);
    if ( !(*pFuncError)( pNtk ) )
    {
        printf( "The original network does not cause the bug. Quitting.\n" );
        Abc_NtkDelete( pNtk );
        return;
    }
    // perform incremental modifications
    for ( nIter = 0; ; nIter++ )
    {
        clk = Abc_Clock();
        // count how many ways of modifying the network exists
        nSteps = 2 * Abc_NtkCountFaninsTotal(pNtk);
        // try modifying the network as many times
        RandNum ^= rand();
        for ( i = 0; i < nSteps; i++ )
        {
            // get the shifted number of bug
            ModNum = (i + RandNum) % nSteps;
            // get the modified network
            pNtkMod = Abc_NtkAutoDebugModify( pNtk, ModNum/2, ModNum%2 );
            // write the network
            Io_WriteBlifLogic( pNtk, "bug_temp.blif", 1 );   
            // check if the bug is still there
            if ( (*pFuncError)( pNtkMod ) ) // bug is still there
            {
                Abc_NtkDelete( pNtk );
                pNtk = pNtkMod;
                break;
            }
            else // no bug
                Abc_NtkDelete( pNtkMod );
        }
        printf( "Iter %6d : Latches = %6d. Nodes = %6d. Steps = %6d. Error step = %3d.  ", 
            nIter, Abc_NtkLatchNum(pNtk), Abc_NtkNodeNum(pNtk), nSteps, i );
        ABC_PRT( "Time", Abc_Clock() - clk );
        if ( i == nSteps ) // could not modify it while preserving the bug
            break;
    }
    // write out the final network
    Io_WriteBlifLogic( pNtk, pFileName, 1 );
    printf( "Final network written into file \"%s\". ", pFileName );
    ABC_PRT( "Total time", Abc_Clock() - clkTotal );
    Abc_NtkDelete( pNtk );
}

/**Function*************************************************************

  Synopsis    [Counts the total number of fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCountFaninsTotal( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pFanin;
    int i, k, Counter = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            if ( !Abc_ObjIsNode(pObj) && !Abc_ObjIsPo(pObj) )
                continue;
            if ( Abc_ObjIsPo(pObj) && Abc_NtkPoNum(pNtk) == 1 )
                continue;
            if ( Abc_ObjIsNode(pObj) && Abc_NodeIsConst(pFanin) )
                continue;
            Counter++;
        }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns the node and fanin to be modified.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkFindGivenFanin( Abc_Ntk_t * pNtk, int Step, Abc_Obj_t ** ppObj, Abc_Obj_t ** ppFanin )
{
    Abc_Obj_t * pObj, * pFanin;
    int i, k, Counter = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            if ( !Abc_ObjIsNode(pObj) && !Abc_ObjIsPo(pObj) )
                continue;
            if ( Abc_ObjIsPo(pObj) && Abc_NtkPoNum(pNtk) == 1 )
                continue;
            if ( Abc_ObjIsNode(pObj) && Abc_NodeIsConst(pFanin) )
                continue;
            if ( Counter++ == Step )
            {
                *ppObj   = pObj;
                *ppFanin = pFanin;
                return 1;
            }
        }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Perform modification with the given number.]

  Description [Modification consists of replacing the node by a constant.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkAutoDebugModify( Abc_Ntk_t * pNtkInit, int Step, int fConst1 )
{
    extern void Abc_NtkCycleInitStateSop( Abc_Ntk_t * pNtk, int nFrames, int fVerbose );
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pObj, * pFanin, * pConst;
    // copy the network
    pNtk = Abc_NtkDup( pNtkInit );
    assert( Abc_NtkNodeNum(pNtk) == Abc_NtkNodeNum(pNtkInit) );
    // find the object number
    Abc_NtkFindGivenFanin( pNtk, Step, &pObj, &pFanin );
    // consider special case 
    if ( Abc_ObjIsPo(pObj) && Abc_NodeIsConst(pFanin) )
    {
        Abc_NtkDeleteAll_rec( pObj );
        return pNtk;
    }
    // plug in a constant node
    pConst = fConst1? Abc_NtkCreateNodeConst1(pNtk) : Abc_NtkCreateNodeConst0(pNtk);
    Abc_ObjTransferFanout( pFanin, pConst );
    Abc_NtkDeleteAll_rec( pFanin );

    Abc_NtkSweep( pNtk, 0 );
    Abc_NtkCleanupSeq( pNtk, 0, 0, 0 );
    Abc_NtkToSop( pNtk, -1, ABC_INFINITY );
    Abc_NtkCycleInitStateSop( pNtk, 50, 0 );
    return pNtk;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

