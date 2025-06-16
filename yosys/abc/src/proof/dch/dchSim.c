/**CFile****************************************************************

  FileName    [dchSim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Choice computation for tech-mapping.]

  Synopsis    [Performs random simulation at the beginning.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: dchSim.c,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dchInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline unsigned * Dch_ObjSim( Vec_Ptr_t * vSims, Aig_Obj_t * pObj )
{ 
    return (unsigned *)Vec_PtrEntry( vSims, pObj->Id ); 
}
static inline unsigned Dch_ObjRandomSim()    
{ 
    return Aig_ManRandom(0);               
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns 1 if the node appears to be constant 1 candidate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dch_NodeIsConstCex( void * p, Aig_Obj_t * pObj )
{
    return pObj->fPhase == pObj->fMarkB;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the nodes appear equal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dch_NodesAreEqualCex( void * p, Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 )
{
    return (pObj0->fPhase == pObj1->fPhase) == (pObj0->fMarkB == pObj1->fMarkB);
}

/**Function*************************************************************

  Synopsis    [Computes hash value of the node using its simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Dch_NodeHash( void * p, Aig_Obj_t * pObj )
{
    Vec_Ptr_t * vSims = (Vec_Ptr_t *)p;
    static int s_FPrimes[128] = { 
        1009, 1049, 1093, 1151, 1201, 1249, 1297, 1361, 1427, 1459, 
        1499, 1559, 1607, 1657, 1709, 1759, 1823, 1877, 1933, 1997, 
        2039, 2089, 2141, 2213, 2269, 2311, 2371, 2411, 2467, 2543, 
        2609, 2663, 2699, 2741, 2797, 2851, 2909, 2969, 3037, 3089, 
        3169, 3221, 3299, 3331, 3389, 3461, 3517, 3557, 3613, 3671, 
        3719, 3779, 3847, 3907, 3943, 4013, 4073, 4129, 4201, 4243, 
        4289, 4363, 4441, 4493, 4549, 4621, 4663, 4729, 4793, 4871, 
        4933, 4973, 5021, 5087, 5153, 5227, 5281, 5351, 5417, 5471, 
        5519, 5573, 5651, 5693, 5749, 5821, 5861, 5923, 6011, 6073, 
        6131, 6199, 6257, 6301, 6353, 6397, 6481, 6563, 6619, 6689, 
        6737, 6803, 6863, 6917, 6977, 7027, 7109, 7187, 7237, 7309, 
        7393, 7477, 7523, 7561, 7607, 7681, 7727, 7817, 7877, 7933, 
        8011, 8039, 8059, 8081, 8093, 8111, 8123, 8147
    };
    unsigned * pSim;
    unsigned uHash;
    int k, nWords;
    nWords = (unsigned *)Vec_PtrEntry(vSims, 1) - (unsigned *)Vec_PtrEntry(vSims, 0);
    uHash = 0;
    pSim  = Dch_ObjSim( vSims, pObj );
    if ( pObj->fPhase )
    {
        for ( k = 0; k < nWords; k++ )
            uHash ^= ~pSim[k] * s_FPrimes[k & 0x7F];
    }
    else
    {
        for ( k = 0; k < nWords; k++ )
            uHash ^= pSim[k] * s_FPrimes[k & 0x7F];
    }
    return uHash;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dch_NodeIsConst( void * p, Aig_Obj_t * pObj )
{
    Vec_Ptr_t * vSims = (Vec_Ptr_t *)p;
    unsigned * pSim;
    int k, nWords;
    nWords = (unsigned *)Vec_PtrEntry(vSims, 1) - (unsigned *)Vec_PtrEntry(vSims, 0);
    pSim  = Dch_ObjSim( vSims, pObj );
    if ( pObj->fPhase )
    {
        for ( k = 0; k < nWords; k++ )
            if ( ~pSim[k] )
                return 0;
    }
    else
    {
        for ( k = 0; k < nWords; k++ )
            if ( pSim[k] )
                return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation infos are equal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dch_NodesAreEqual( void * p, Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 )
{
    Vec_Ptr_t * vSims = (Vec_Ptr_t *)p;
    unsigned * pSim0, * pSim1;
    int k, nWords;
    nWords = (unsigned *)Vec_PtrEntry(vSims, 1) - (unsigned *)Vec_PtrEntry(vSims, 0);
    pSim0 = Dch_ObjSim( vSims, pObj0 );
    pSim1 = Dch_ObjSim( vSims, pObj1 );
    if ( pObj0->fPhase != pObj1->fPhase )
    {
        for ( k = 0; k < nWords; k++ )
            if ( pSim0[k] != ~pSim1[k] )
                return 0;
    }
    else
    {
        for ( k = 0; k < nWords; k++ )
            if ( pSim0[k] != pSim1[k] )
                return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Perform random simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dch_PerformRandomSimulation( Aig_Man_t * pAig, Vec_Ptr_t * vSims )
{
    unsigned * pSim, * pSim0, * pSim1;
    Aig_Obj_t * pObj;
    int i, k, nWords;
    nWords = (unsigned *)Vec_PtrEntry(vSims, 1) - (unsigned *)Vec_PtrEntry(vSims, 0);

    // assign const 1 sim info
    pObj = Aig_ManConst1(pAig);
    pSim = Dch_ObjSim( vSims, pObj );
    memset( pSim, 0xff, sizeof(unsigned) * nWords );

    // assign primary input random sim info
    Aig_ManForEachCi( pAig, pObj, i )
    {
        pSim = Dch_ObjSim( vSims, pObj );
        for ( k = 0; k < nWords; k++ )
            pSim[k] = Dch_ObjRandomSim();
        pSim[0] <<= 1;
    }

    // simulate AIG in the topological order
    Aig_ManForEachNode( pAig, pObj, i )
    {
        pSim0 = Dch_ObjSim( vSims, Aig_ObjFanin0(pObj) ); 
        pSim1 = Dch_ObjSim( vSims, Aig_ObjFanin1(pObj) ); 
        pSim  = Dch_ObjSim( vSims, pObj );

        if ( Aig_ObjFaninC0(pObj) && Aig_ObjFaninC1(pObj) ) // both are compls
        {
            for ( k = 0; k < nWords; k++ )
                pSim[k] = ~pSim0[k] & ~pSim1[k];
        }
        else if ( Aig_ObjFaninC0(pObj) && !Aig_ObjFaninC1(pObj) ) // first one is compl
        {
            for ( k = 0; k < nWords; k++ )
                pSim[k] = ~pSim0[k] & pSim1[k];
        }
        else if ( !Aig_ObjFaninC0(pObj) && Aig_ObjFaninC1(pObj) ) // second one is compl
        {
            for ( k = 0; k < nWords; k++ )
                pSim[k] = pSim0[k] & ~pSim1[k];
        }
        else // if ( Aig_ObjFaninC0(pObj) && Aig_ObjFaninC1(pObj) ) // none is compl
        {
            for ( k = 0; k < nWords; k++ )
                pSim[k] = pSim0[k] & pSim1[k];
        }
    }
    // get simulation information for primary outputs
}

/**Function*************************************************************

  Synopsis    [Derives candidate equivalence classes of AIG nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dch_Cla_t * Dch_CreateCandEquivClasses( Aig_Man_t * pAig, int nWords, int fVerbose )
{
    Dch_Cla_t * pClasses;
    Vec_Ptr_t * vSims;
    int i;
    // allocate simulation information
    vSims = Vec_PtrAllocSimInfo( Aig_ManObjNumMax(pAig), nWords );
    // run random simulation from the primary inputs
    Dch_PerformRandomSimulation( pAig, vSims );
    // start storage for equivalence classes
    pClasses = Dch_ClassesStart( pAig );
    Dch_ClassesSetData( pClasses, vSims, Dch_NodeHash, Dch_NodeIsConst, Dch_NodesAreEqual );
    // hash nodes by sim info
    Dch_ClassesPrepare( pClasses, 0, 0 );
    // iterate random simulation
    for ( i = 0; i < 7; i++ )
    {
        Dch_PerformRandomSimulation( pAig, vSims );
        Dch_ClassesRefine( pClasses );
    }
    // clean up and return
    Vec_PtrFree( vSims );
    // prepare class refinement procedures
    Dch_ClassesSetData( pClasses, NULL, NULL, Dch_NodeIsConstCex, Dch_NodesAreEqualCex );
    return pClasses;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

