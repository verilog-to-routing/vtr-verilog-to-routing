/**CFile****************************************************************

  FileName    [abcPart.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Output partitioning package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcPart.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prepare supports.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkPartitionCollectSupps( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vSupp, * vSupports;
    Vec_Int_t * vSuppI;
    Abc_Obj_t * pObj, * pTemp;
    int i, k;
    // set the PI numbers
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pCopy = (void *)i;
    // save hte CI numbers
    vSupports = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        vSupp = Abc_NtkNodeSupport( pNtk, &pObj, 1 );
        vSuppI = (Vec_Int_t *)vSupp;
        Vec_PtrForEachEntry( vSupp, pTemp, k )
            Vec_IntWriteEntry( vSuppI, k, (int)pTemp->pCopy );
        Vec_IntSort( vSuppI, 0 );
        // append the number of this output
        Vec_IntPush( vSuppI, i );
        // save the support in the vector
        Vec_PtrPush( vSupports, vSuppI );
    }
    // sort supports by size
    Vec_VecSort( (Vec_Vec_t *)vSupports, 1 );
    return vSupports;
}

/**Function*************************************************************

  Synopsis    [Start char-bases support representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_NtkSuppCharStart( Vec_Int_t * vOne, int nPis )
{
    char * pBuffer;
    int i, Entry;
    pBuffer = ALLOC( char, nPis );
    memset( pBuffer, 0, sizeof(char) * nPis );
    Vec_IntForEachEntry( vOne, Entry, i )
    {
        assert( Entry < nPis );
        pBuffer[Entry] = 1;
    }
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    [Add to char-bases support representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkSuppCharAdd( char * pBuffer, Vec_Int_t * vOne, int nPis )
{
    int i, Entry;
    Vec_IntForEachEntry( vOne, Entry, i )
    {
        assert( Entry < nPis );
        pBuffer[Entry] = 1;
    }
}

/**Function*************************************************************

  Synopsis    [Find the common variables using char-bases support representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkSuppCharCommon( char * pBuffer, Vec_Int_t * vOne )
{
    int i, Entry, nCommon = 0;
    Vec_IntForEachEntry( vOne, Entry, i )
        nCommon += pBuffer[Entry];
    return nCommon;
}

/**Function*************************************************************

  Synopsis    [Find the best partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkPartitionSmartFindPart( Vec_Ptr_t * vPartSuppsAll, Vec_Ptr_t * vPartsAll, Vec_Ptr_t * vPartSuppsChar, int nPartSizeLimit, Vec_Int_t * vOne )
{
/*
    Vec_Int_t * vPartSupp, * vPart;
    double Attract, Repulse, Cost, CostBest;
    int i, nCommon, iBest;
    iBest = -1;
    CostBest = 0.0;
    Vec_PtrForEachEntry( vPartSuppsAll, vPartSupp, i )
    {
        vPart = Vec_PtrEntry( vPartsAll, i );
        if ( nPartSizeLimit > 0 && Vec_IntSize(vPart) >= nPartSizeLimit )
            continue;
        nCommon = Vec_IntTwoCountCommon( vPartSupp, vOne );
        if ( nCommon == 0 )
            continue;
        if ( nCommon == Vec_IntSize(vOne) )
            return i;
        Attract = 1.0 * nCommon / Vec_IntSize(vOne);
        if ( Vec_IntSize(vPartSupp) < 100 )
            Repulse = 1.0;
        else
            Repulse = log10( Vec_IntSize(vPartSupp) / 10.0 );
        Cost = pow( Attract, pow(Repulse, 5.0) );
        if ( CostBest < Cost )
        {
            CostBest = Cost;
            iBest = i;
        }
    }
    if ( CostBest < 0.6 )
        return -1;
    return iBest;
*/

    Vec_Int_t * vPartSupp, * vPart;
    int Attract, Repulse, Value, ValueBest;
    int i, nCommon, iBest;
//    int nCommon2;
    iBest = -1;
    ValueBest = 0;
    Vec_PtrForEachEntry( vPartSuppsAll, vPartSupp, i )
    {
        vPart = Vec_PtrEntry( vPartsAll, i );
        if ( nPartSizeLimit > 0 && Vec_IntSize(vPart) >= nPartSizeLimit )
            continue;
//        nCommon2 = Vec_IntTwoCountCommon( vPartSupp, vOne );
        nCommon = Abc_NtkSuppCharCommon( Vec_PtrEntry(vPartSuppsChar, i), vOne );
//        assert( nCommon2 == nCommon );
        if ( nCommon == 0 )
            continue;
        if ( nCommon == Vec_IntSize(vOne) )
            return i;
        Attract = 1000 * nCommon / Vec_IntSize(vOne);
        if ( Vec_IntSize(vPartSupp) < 100 )
            Repulse = 1;
        else
            Repulse = 1+Extra_Base2Log(Vec_IntSize(vPartSupp)-100);
        Value = Attract/Repulse;
        if ( ValueBest < Value )
        {
            ValueBest = Value;
            iBest = i;
        }
    }
    if ( ValueBest < 75 )
        return -1;
    return iBest;
}

/**Function*************************************************************

  Synopsis    [Perform the smart partitioning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPartitionPrint( Abc_Ntk_t * pNtk, Vec_Ptr_t * vPartsAll, Vec_Ptr_t * vPartSuppsAll )
{
    Vec_Int_t * vOne;
    int i, nOutputs, Counter;

    Counter = 0;
    Vec_PtrForEachEntry( vPartSuppsAll, vOne, i )
    {
        nOutputs = Vec_IntSize(Vec_PtrEntry(vPartsAll, i));
        printf( "%d=(%d,%d) ", i, Vec_IntSize(vOne), nOutputs );
        Counter += nOutputs;
        if ( i == Vec_PtrSize(vPartsAll) - 1 )
            break;
    }
    assert( Counter == Abc_NtkCoNum(pNtk) );
    printf( "\nTotal = %d. Outputs = %d.\n", Counter, Abc_NtkCoNum(pNtk) );
}

/**Function*************************************************************

  Synopsis    [Perform the smart partitioning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPartitionCompact( Vec_Ptr_t * vPartsAll, Vec_Ptr_t * vPartSuppsAll, int nPartSizeLimit )
{
    Vec_Int_t * vOne, * vPart, * vPartSupp, * vTemp;
    int i, iPart;

    if ( nPartSizeLimit == 0 )
        nPartSizeLimit = 200;

    // pack smaller partitions into larger blocks
    iPart = 0;
    vPart = vPartSupp = NULL;
    Vec_PtrForEachEntry( vPartSuppsAll, vOne, i )
    {
        if ( Vec_IntSize(vOne) < nPartSizeLimit )
        {
            if ( vPartSupp == NULL )
            {
                assert( vPart == NULL );
                vPartSupp = Vec_IntDup(vOne);
                vPart = Vec_PtrEntry(vPartsAll, i);
            }
            else
            {
                vPartSupp = Vec_IntTwoMerge( vTemp = vPartSupp, vOne );
                Vec_IntFree( vTemp );
                vPart = Vec_IntTwoMerge( vTemp = vPart, Vec_PtrEntry(vPartsAll, i) );
                Vec_IntFree( vTemp );
                Vec_IntFree( Vec_PtrEntry(vPartsAll, i) );
            }
            if ( Vec_IntSize(vPartSupp) < nPartSizeLimit )
                continue;
        }
        else
            vPart = Vec_PtrEntry(vPartsAll, i);
        // add the partition 
        Vec_PtrWriteEntry( vPartsAll, iPart, vPart );  
        vPart = NULL;
        if ( vPartSupp ) 
        {
            Vec_IntFree( Vec_PtrEntry(vPartSuppsAll, iPart) );
            Vec_PtrWriteEntry( vPartSuppsAll, iPart, vPartSupp );  
            vPartSupp = NULL;
        }
        iPart++;
    }
    // add the last one
    if ( vPart )
    {
        Vec_PtrWriteEntry( vPartsAll, iPart, vPart );  
        vPart = NULL;

        assert( vPartSupp != NULL );
        Vec_IntFree( Vec_PtrEntry(vPartSuppsAll, iPart) );
        Vec_PtrWriteEntry( vPartSuppsAll, iPart, vPartSupp );  
        vPartSupp = NULL;
        iPart++;
    }
    Vec_PtrShrink( vPartsAll, iPart );
    Vec_PtrShrink( vPartsAll, iPart );
}

/**Function*************************************************************

  Synopsis    [Perform the smart partitioning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Abc_NtkPartitionSmart( Abc_Ntk_t * pNtk, int nPartSizeLimit, int fVerbose )
{
    Vec_Ptr_t * vPartSuppsChar;
    Vec_Ptr_t * vSupps, * vPartsAll, * vPartsAll2, * vPartSuppsAll, * vPartPtr;
    Vec_Int_t * vOne, * vPart, * vPartSupp, * vTemp;
    int i, iPart, iOut, clk, clk2, timeFind = 0;

    // compute the supports for all outputs
clk = clock();
    vSupps = Abc_NtkPartitionCollectSupps( pNtk );
if ( fVerbose )
{
PRT( "Supps", clock() - clk );
}
    // start char-based support representation
    vPartSuppsChar = Vec_PtrAlloc( 1000 );

    // create partitions
clk = clock();
    vPartsAll = Vec_PtrAlloc( 256 );
    vPartSuppsAll = Vec_PtrAlloc( 256 );
    Vec_PtrForEachEntry( vSupps, vOne, i )
    {
        // get the output number
        iOut = Vec_IntPop(vOne);
        // find closely matching part
clk2 = clock();
        iPart = Abc_NtkPartitionSmartFindPart( vPartSuppsAll, vPartsAll, vPartSuppsChar, nPartSizeLimit, vOne );
timeFind += clock() - clk2;
//printf( "%4d->%4d  ", i, iPart );
        if ( iPart == -1 )
        {
            // create new partition
            vPart = Vec_IntAlloc( 32 );
            Vec_IntPush( vPart, iOut );
            // create new partition support
            vPartSupp = Vec_IntDup( vOne );
            // add this partition and its support
            Vec_PtrPush( vPartsAll, vPart );
            Vec_PtrPush( vPartSuppsAll, vPartSupp );

            Vec_PtrPush( vPartSuppsChar, Abc_NtkSuppCharStart(vOne, Abc_NtkPiNum(pNtk)) );
        }
        else
        {
            // add output to this partition
            vPart = Vec_PtrEntry( vPartsAll, iPart );
            Vec_IntPush( vPart, iOut );
            // merge supports
            vPartSupp = Vec_PtrEntry( vPartSuppsAll, iPart );
            vPartSupp = Vec_IntTwoMerge( vTemp = vPartSupp, vOne );
            Vec_IntFree( vTemp );
            // reinsert new support
            Vec_PtrWriteEntry( vPartSuppsAll, iPart, vPartSupp );

            Abc_NtkSuppCharAdd( Vec_PtrEntry(vPartSuppsChar, iPart), vOne, Abc_NtkPiNum(pNtk) );
        }
    }

    // stop char-based support representation
    Vec_PtrForEachEntry( vPartSuppsChar, vTemp, i )
        free( vTemp );
    Vec_PtrFree( vPartSuppsChar );

//printf( "\n" );
if ( fVerbose )
{
PRT( "Parts", clock() - clk );
//PRT( "Find ", timeFind );
}

clk = clock();
    // remember number of supports
    Vec_PtrForEachEntry( vPartSuppsAll, vOne, i )
        Vec_IntPush( vOne, i );
    // sort the supports in the decreasing order
    Vec_VecSort( (Vec_Vec_t *)vPartSuppsAll, 1 );
    // reproduce partitions
    vPartsAll2 = Vec_PtrAlloc( 256 );
    Vec_PtrForEachEntry( vPartSuppsAll, vOne, i )
        Vec_PtrPush( vPartsAll2, Vec_PtrEntry(vPartsAll, Vec_IntPop(vOne)) );
    Vec_PtrFree( vPartsAll );
    vPartsAll = vPartsAll2;

    // compact small partitions
//    Abc_NtkPartitionPrint( pNtk, vPartsAll, vPartSuppsAll );
    Abc_NtkPartitionCompact( vPartsAll, vPartSuppsAll, nPartSizeLimit );
    if ( fVerbose )
//    Abc_NtkPartitionPrint( pNtk, vPartsAll, vPartSuppsAll );
    printf( "Created %d partitions.\n", Vec_PtrSize(vPartsAll) );

if ( fVerbose )
{
PRT( "Comps", clock() - clk );
}

    // cleanup
    Vec_VecFree( (Vec_Vec_t *)vSupps );
    Vec_VecFree( (Vec_Vec_t *)vPartSuppsAll );

    // converts from intergers to nodes
    Vec_PtrForEachEntry( vPartsAll, vPart, iPart )
    {
        vPartPtr = Vec_PtrAlloc( Vec_IntSize(vPart) );
        Vec_IntForEachEntry( vPart, iOut, i )
            Vec_PtrPush( vPartPtr, Abc_NtkCo(pNtk, iOut) );
        Vec_IntFree( vPart );
        Vec_PtrWriteEntry( vPartsAll, iPart, vPartPtr );
    }
    return (Vec_Vec_t *)vPartsAll;
}

/**Function*************************************************************

  Synopsis    [Perform the naive partitioning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Abc_NtkPartitionNaive( Abc_Ntk_t * pNtk, int nPartSize )
{
    Vec_Vec_t * vParts;
    Abc_Obj_t * pObj;
    int nParts, i;
    nParts = (Abc_NtkCoNum(pNtk) / nPartSize) + ((Abc_NtkCoNum(pNtk) % nPartSize) > 0);
    vParts = Vec_VecStart( nParts );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Vec_VecPush( vParts, i / nPartSize, pObj );
    return vParts;
}

/**Function*************************************************************

  Synopsis    [Returns representative of the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkPartStitchFindRepr_rec( Vec_Ptr_t * vEquiv, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pRepr;
    pRepr = Vec_PtrEntry( vEquiv, pObj->Id );
    if ( pRepr == NULL || pRepr == pObj )
        return pObj;
    return Abc_NtkPartStitchFindRepr_rec( vEquiv, pRepr );
}

/**Function*************************************************************

  Synopsis    [Returns the representative of the fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Abc_Obj_t * Abc_NtkPartStitchCopy0( Vec_Ptr_t * vEquiv, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFan = Abc_ObjFanin0( pObj );
    Abc_Obj_t * pRepr = Abc_NtkPartStitchFindRepr_rec( vEquiv, pFan );
    return Abc_ObjNotCond( pRepr->pCopy, pRepr->fPhase ^ pFan->fPhase ^ Abc_ObjFaninC1(pObj) );
}
static inline Abc_Obj_t * Abc_NtkPartStitchCopy1( Vec_Ptr_t * vEquiv, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFan = Abc_ObjFanin1( pObj );
    Abc_Obj_t * pRepr = Abc_NtkPartStitchFindRepr_rec( vEquiv, pFan );
    return Abc_ObjNotCond( pRepr->pCopy, pRepr->fPhase ^ pFan->fPhase ^ Abc_ObjFaninC1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Stitches together several networks with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkPartStitchChoices_old( Abc_Ntk_t * pNtk, Vec_Ptr_t * vParts )
{
    Vec_Ptr_t * vNodes, * vEquiv;
    Abc_Ntk_t * pNtkNew, * pNtkNew2, * pNtkTemp;
    Abc_Obj_t * pObj, * pFanin, * pRepr0, * pRepr1, * pRepr;
    int i, k, iNodeId;

    // start a new network similar to the original one
    assert( Abc_NtkIsStrash(pNtk) );
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);

    // annotate parts to point to the new network
    vEquiv = Vec_PtrStart( Abc_NtkObjNumMax(pNtk) + 1 );
    Vec_PtrForEachEntry( vParts, pNtkTemp, i )
    {
        assert( Abc_NtkIsStrash(pNtkTemp) );
        Abc_NtkCleanCopy( pNtkTemp );

        // map the CI nodes
        Abc_AigConst1(pNtkTemp)->pCopy = Abc_AigConst1(pNtkNew);
        Abc_NtkForEachCi( pNtkTemp, pObj, k )
        {
            iNodeId = Nm_ManFindIdByNameTwoTypes( pNtkNew->pManName, Abc_ObjName(pObj), ABC_OBJ_PI, ABC_OBJ_BO );
            if ( iNodeId == -1 )
            {
                printf( "Cannot find CI node %s in the original network.\n", Abc_ObjName(pObj) );
                return NULL;
            }
            pObj->pCopy = Abc_NtkObj( pNtkNew, iNodeId );
        }

        // add the internal nodes while saving representatives
        vNodes = Abc_AigDfs( pNtkTemp, 1, 0 );
        Vec_PtrForEachEntry( vNodes, pObj, k )
        {
            pObj->pCopy = Abc_AigAnd( pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
            assert( !Abc_ObjIsComplement(pObj->pCopy) );
            if ( !Abc_AigNodeIsChoice(pObj) )
                continue;
            // find the earliest representative of the choice node
            pRepr0 = NULL;
            for ( pFanin = pObj; pFanin; pFanin = pFanin->pData )
            {
                pRepr1 = Abc_NtkPartStitchFindRepr_rec( vEquiv, pFanin->pCopy );
                if ( pRepr0 == NULL || pRepr0->Id > pRepr1->Id )
                    pRepr0 = pRepr1;
            }
            // set this representative for the representives of all choices
            for ( pFanin = pObj; pFanin; pFanin = pFanin->pData )
            {
                pRepr1 = Abc_NtkPartStitchFindRepr_rec( vEquiv, pFanin->pCopy );
                Vec_PtrWriteEntry( vEquiv, pRepr1->Id, pRepr0 );
            }
        }
        Vec_PtrFree( vNodes );

        // map the CO nodes
        Abc_NtkForEachCo( pNtkTemp, pObj, k )
        {
            iNodeId = Nm_ManFindIdByNameTwoTypes( pNtkNew->pManName, Abc_ObjName(pObj), ABC_OBJ_PO, ABC_OBJ_BI );
            if ( iNodeId == -1 )
            {
                printf( "Cannot find CO node %s in the original network.\n", Abc_ObjName(pObj) );
                return NULL;
            }
            pObj->pCopy = Abc_NtkObj( pNtkNew, iNodeId );
            Abc_ObjAddFanin( pObj->pCopy, Abc_ObjChild0Copy(pObj) );
        }
    }

    // reconstruct the AIG
    pNtkNew2 = Abc_NtkStartFrom( pNtkNew, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // duplicate the name and the spec
    pNtkNew2->pName = Extra_UtilStrsav(pNtkNew->pName);
    pNtkNew2->pSpec = Extra_UtilStrsav(pNtkNew->pSpec);
    // duplicate internal nodes
    Abc_AigForEachAnd( pNtkNew, pObj, i )
    {
        pRepr0 = Abc_NtkPartStitchCopy0( vEquiv, pObj );
        pRepr1 = Abc_NtkPartStitchCopy1( vEquiv, pObj );
        pObj->pCopy = Abc_AigAnd( pNtkNew2->pManFunc, pRepr0, pRepr1 );
        assert( !Abc_ObjIsComplement(pObj->pCopy) );
        // add the choice if applicable
        pRepr = Abc_NtkPartStitchFindRepr_rec( vEquiv, pObj );
        if ( pObj != pRepr )
        {
            assert( pObj->Id > pRepr->Id );
            if ( pObj->pCopy != pRepr->pCopy )
            {
                assert( pObj->pCopy->Id > pRepr->pCopy->Id );
                pObj->pCopy->pData = pRepr->pCopy->pData;
                pRepr->pCopy->pData = pObj->pCopy;
            }
        }
    }
    // connect the COs
    Abc_NtkForEachCo( pNtkNew, pObj, k )
        Abc_ObjAddFanin( pObj->pCopy, Abc_NtkPartStitchCopy0(vEquiv,pObj) );

    // replace the network
    Abc_NtkDelete( pNtkNew );
    pNtkNew = pNtkNew2;

    // check correctness of the new network
    Vec_PtrFree( vEquiv );
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkPartStitchChoices: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Hop_Obj_t * Hop_ObjChild0Next( Abc_Obj_t * pObj ) { return Hop_NotCond( (Hop_Obj_t *)Abc_ObjFanin0(pObj)->pNext, Abc_ObjFaninC0(pObj) ); }
static inline Hop_Obj_t * Hop_ObjChild1Next( Abc_Obj_t * pObj ) { return Hop_NotCond( (Hop_Obj_t *)Abc_ObjFanin1(pObj)->pNext, Abc_ObjFaninC1(pObj) ); }


/**Function*************************************************************

  Synopsis    [Stitches together several networks with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Man_t * Abc_NtkPartStartHop( Abc_Ntk_t * pNtk )
{
    Hop_Man_t * pMan;
    Abc_Obj_t * pObj;
    int i;
    // start the HOP package
    pMan = Hop_ManStart();
    pMan->vObjs = Vec_PtrAlloc( Abc_NtkObjNumMax(pNtk) + 1  );
    Vec_PtrPush( pMan->vObjs, Hop_ManConst1(pMan) );
    // map constant node and PIs
    Abc_AigConst1(pNtk)->pNext = (Abc_Obj_t *)Hop_ManConst1(pMan);
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pNext = (Abc_Obj_t *)Hop_ObjCreatePi(pMan);
    // map the internal nodes
    Abc_AigForEachAnd( pNtk, pObj, i )
    {
        pObj->pNext = (Abc_Obj_t *)Hop_And( pMan, Hop_ObjChild0Next(pObj), Hop_ObjChild1Next(pObj) );
        assert( !Abc_ObjIsComplement(pObj->pNext) );
    }
    // set the choice nodes
    Abc_AigForEachAnd( pNtk, pObj, i )
    {
        if ( pObj->pCopy )
            ((Hop_Obj_t *)pObj->pNext)->pData = pObj->pCopy->pNext;
    }
    // transfer the POs
    Abc_NtkForEachCo( pNtk, pObj, i )
        Hop_ObjCreatePo( pMan, Hop_ObjChild0Next(pObj) );
    // check the new manager
    if ( !Hop_ManCheck(pMan) )
        printf( "Abc_NtkPartStartHop: HOP manager check has failed.\n" );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Stitches together several networks with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkPartStitchChoices( Abc_Ntk_t * pNtk, Vec_Ptr_t * vParts )
{
    extern Abc_Ntk_t * Abc_NtkHopRemoveLoops( Abc_Ntk_t * pNtk, Hop_Man_t * pMan );

    Hop_Man_t * pMan;
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkNew, * pNtkTemp;
    Abc_Obj_t * pObj, * pFanin;
    int i, k, iNodeId;

    // start a new network similar to the original one
    assert( Abc_NtkIsStrash(pNtk) );
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );

    // annotate parts to point to the new network
    Vec_PtrForEachEntry( vParts, pNtkTemp, i )
    {
        assert( Abc_NtkIsStrash(pNtkTemp) );
        Abc_NtkCleanCopy( pNtkTemp );

        // map the CI nodes
        Abc_AigConst1(pNtkTemp)->pCopy = Abc_AigConst1(pNtkNew);
        Abc_NtkForEachCi( pNtkTemp, pObj, k )
        {
            iNodeId = Nm_ManFindIdByNameTwoTypes( pNtkNew->pManName, Abc_ObjName(pObj), ABC_OBJ_PI, ABC_OBJ_BO );
            if ( iNodeId == -1 )
            {
                printf( "Cannot find CI node %s in the original network.\n", Abc_ObjName(pObj) );
                return NULL;
            }
            pObj->pCopy = Abc_NtkObj( pNtkNew, iNodeId );
        }

        // add the internal nodes while saving representatives
        vNodes = Abc_AigDfs( pNtkTemp, 1, 0 );
        Vec_PtrForEachEntry( vNodes, pObj, k )
        {
            pObj->pCopy = Abc_AigAnd( pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
            assert( !Abc_ObjIsComplement(pObj->pCopy) );
            if ( Abc_AigNodeIsChoice(pObj) )
                for ( pFanin = pObj->pData; pFanin; pFanin = pFanin->pData )
                    pFanin->pCopy->pCopy = pObj->pCopy;
        }
        Vec_PtrFree( vNodes );

        // map the CO nodes
        Abc_NtkForEachCo( pNtkTemp, pObj, k )
        {
            iNodeId = Nm_ManFindIdByNameTwoTypes( pNtkNew->pManName, Abc_ObjName(pObj), ABC_OBJ_PO, ABC_OBJ_BI );
            if ( iNodeId == -1 )
            {
                printf( "Cannot find CO node %s in the original network.\n", Abc_ObjName(pObj) );
                return NULL;
            }
            pObj->pCopy = Abc_NtkObj( pNtkNew, iNodeId );
            Abc_ObjAddFanin( pObj->pCopy, Abc_ObjChild0Copy(pObj) );
        }
    }

    // transform into the HOP manager
    pMan = Abc_NtkPartStartHop( pNtkNew );
    pNtkNew = Abc_NtkHopRemoveLoops( pNtkTemp = pNtkNew, pMan );
    Abc_NtkDelete( pNtkTemp );

    // check correctness of the new network
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkPartStitchChoices: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Stitches together several networks with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFraigPartitioned( Abc_Ntk_t * pNtk, void * pParams )
{
    extern int Cmd_CommandExecute( void * pAbc, char * sCommand );
    extern void * Abc_FrameGetGlobalFrame();

    Vec_Vec_t * vParts;
    Vec_Ptr_t * vFraigs, * vOne;
    Abc_Ntk_t * pNtkAig, * pNtkFraig;
    int i;

    // perform partitioning
    assert( Abc_NtkIsStrash(pNtk) );
//    vParts = Abc_NtkPartitionNaive( pNtk, 20 );
    vParts = Abc_NtkPartitionSmart( pNtk, 0, 1 );

    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "unset progressbar" );

    // fraig each partition
    vFraigs = Vec_PtrAlloc( Vec_VecSize(vParts) );
    Vec_VecForEachLevel( vParts, vOne, i )
    {
        pNtkAig = Abc_NtkCreateConeArray( pNtk, vOne, 0 );
        pNtkFraig = Abc_NtkFraig( pNtkAig, pParams, 0, 0 );
        Vec_PtrPush( vFraigs, pNtkFraig );
        Abc_NtkDelete( pNtkAig );

        printf( "Finished part %d (out of %d)\r", i+1, Vec_VecSize(vParts) );
    }
    Vec_VecFree( vParts );

    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "set progressbar" );

    // derive the final network
    pNtkFraig = Abc_NtkPartStitchChoices( pNtk, vFraigs );
    Vec_PtrForEachEntry( vFraigs, pNtkAig, i )
    {
//        Abc_NtkPrintStats( stdout, pNtkAig, 0 );
        Abc_NtkDelete( pNtkAig );
    }
    Vec_PtrFree( vFraigs );

    return pNtkFraig;
}

/**Function*************************************************************

  Synopsis    [Stitches together several networks with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFraigPartitionedTime( Abc_Ntk_t * pNtk, void * pParams )
{
    extern int Cmd_CommandExecute( void * pAbc, char * sCommand );
    extern void * Abc_FrameGetGlobalFrame();

    Vec_Vec_t * vParts;
    Vec_Ptr_t * vFraigs, * vOne;
    Abc_Ntk_t * pNtkAig, * pNtkFraig;
    int i;
    int clk = clock();

    // perform partitioning
    assert( Abc_NtkIsStrash(pNtk) );
//    vParts = Abc_NtkPartitionNaive( pNtk, 20 );
    vParts = Abc_NtkPartitionSmart( pNtk, 0, 0 );

    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "unset progressbar" );

    // fraig each partition
    vFraigs = Vec_PtrAlloc( Vec_VecSize(vParts) );
    Vec_VecForEachLevel( vParts, vOne, i )
    {
        pNtkAig = Abc_NtkCreateConeArray( pNtk, vOne, 0 );
        pNtkFraig = Abc_NtkFraig( pNtkAig, pParams, 0, 0 );
        Vec_PtrPush( vFraigs, pNtkFraig );
        Abc_NtkDelete( pNtkAig );

        printf( "Finished part %d (out of %d)\r", i+1, Vec_VecSize(vParts) );
    }
    Vec_VecFree( vParts );

    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "set progressbar" );

    // derive the final network
    Vec_PtrForEachEntry( vFraigs, pNtkAig, i )
        Abc_NtkDelete( pNtkAig );
    Vec_PtrFree( vFraigs );

    PRT( "Partitioned fraiging time", clock() - clk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


