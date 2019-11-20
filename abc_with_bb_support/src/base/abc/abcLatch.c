/**CFile****************************************************************

  FileName    [abcLatch.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Procedures working with latches.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcLatch.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks if latches form self-loop.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkLatchIsSelfFeed_rec( Abc_Obj_t * pLatch, Abc_Obj_t * pLatchRoot )
{
    Abc_Obj_t * pFanin;
    assert( Abc_ObjIsLatch(pLatch) );
    if ( pLatch == pLatchRoot )
        return 1;
    pFanin = Abc_ObjFanin0(Abc_ObjFanin0(pLatch));
    if ( !Abc_ObjIsBo(pFanin) || !Abc_ObjIsLatch(Abc_ObjFanin0(pFanin)) )
        return 0;
    return Abc_NtkLatchIsSelfFeed_rec( Abc_ObjFanin0(pFanin), pLatch );
}

/**Function*************************************************************

  Synopsis    [Checks if latches form self-loop.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkLatchIsSelfFeed( Abc_Obj_t * pLatch )
{
    Abc_Obj_t * pFanin;
    assert( Abc_ObjIsLatch(pLatch) );
    pFanin = Abc_ObjFanin0(Abc_ObjFanin0(pLatch));
    if ( !Abc_ObjIsBo(pFanin) || !Abc_ObjIsLatch(Abc_ObjFanin0(pFanin)) )
        return 0;
    return Abc_NtkLatchIsSelfFeed_rec( Abc_ObjFanin0(pFanin), pLatch );
}

/**Function*************************************************************

  Synopsis    [Checks if latches form self-loop.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCountSelfFeedLatches( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pLatch;
    int i, Counter;
    Counter = 0;
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
//        if ( Abc_NtkLatchIsSelfFeed(pLatch) && Abc_ObjFanoutNum(pLatch) > 1 )
//            printf( "Fanouts = %d.\n", Abc_ObjFanoutNum(pLatch) );
        Counter += Abc_NtkLatchIsSelfFeed( pLatch );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Replaces self-feeding latches by latches with constant inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRemoveSelfFeedLatches( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pLatch, * pConst1;
    int i, Counter;
    Counter = 0;
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        if ( Abc_NtkLatchIsSelfFeed( pLatch ) )
        {
            if ( Abc_NtkIsStrash(pNtk) )
                pConst1 = Abc_AigConst1(pNtk);
            else
                pConst1 = Abc_NtkCreateNodeConst1(pNtk);
            Abc_ObjPatchFanin( Abc_ObjFanin0(pLatch), Abc_ObjFanin0(Abc_ObjFanin0(pLatch)), pConst1 );
            Counter++;
        }
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Pipelines the network with latches.]

  Description []
               
  SideEffects [Does not check the names of the added latches!!!]

  SeeAlso     []

***********************************************************************/
void Abc_NtkLatchPipe( Abc_Ntk_t * pNtk, int nLatches )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pLatch, * pFanin, * pFanout;
    int i, k, nTotal, nDigits;
    if ( nLatches < 1 )
        return;
    nTotal = nLatches * Abc_NtkPiNum(pNtk);
    nDigits = Extra_Base10Log( nTotal );
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        // remember current fanins of the PI
        Abc_NodeCollectFanouts( pObj, vNodes );
        // create the latches
        for ( pFanin = pObj, k = 0; k < nLatches; k++, pFanin = pLatch )
        {
            pLatch = Abc_NtkCreateLatch( pNtk );
            Abc_ObjAddFanin( pLatch, pFanin );
            Abc_LatchSetInitDc( pLatch );
            // create the name of the new latch
            Abc_ObjAssignName( pLatch, Abc_ObjNameDummy("LL", i*nLatches + k, nDigits), NULL );
        }
        // patch the PI fanouts
        Vec_PtrForEachEntry( vNodes, pFanout, k )
            Abc_ObjPatchFanin( pFanout, pObj, pFanin );
    }
    Vec_PtrFree( vNodes );
    Abc_NtkLogicMakeSimpleCos( pNtk, 0 );
}

/**Function*************************************************************

  Synopsis    [Strashes one logic node using its SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkCollectLatchValues( Abc_Ntk_t * pNtk )
{
    Vec_Int_t * vValues;
    Abc_Obj_t * pLatch;
    int i;
    vValues = Vec_IntAlloc( Abc_NtkLatchNum(pNtk) );
    Abc_NtkForEachLatch( pNtk, pLatch, i )
        Vec_IntPush( vValues, Abc_LatchIsInit1(pLatch) );
    return vValues;
}

/**Function*************************************************************

  Synopsis    [Strashes one logic node using its SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkInsertLatchValues( Abc_Ntk_t * pNtk, Vec_Int_t * vValues )
{
    Abc_Obj_t * pLatch;
    int i;
    Abc_NtkForEachLatch( pNtk, pLatch, i )
        pLatch->pData = (void *)(vValues? (Vec_IntEntry(vValues,i)? ABC_INIT_ONE : ABC_INIT_ZERO) : ABC_INIT_DC);
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


