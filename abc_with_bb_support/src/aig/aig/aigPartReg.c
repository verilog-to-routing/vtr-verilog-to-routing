/**CFile****************************************************************

  FileName    [aigPartReg.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Register partitioning algorithm.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigPartReg.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"
//#include "fra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Aig_ManPre_t_         Aig_ManPre_t;

struct Aig_ManPre_t_
{
    // input data    
    Aig_Man_t *     pAig;            // seq AIG manager 
    Vec_Ptr_t *     vMatrix;         // register dependency
    int             nRegsMax;        // the max number of registers in the cluster
    // information about partitions
    Vec_Ptr_t *     vParts;          // the partitions
    char *          pfUsedRegs;      // the registers already included in the partitions
    // info about the current partition
    Vec_Int_t *     vRegs;           // registers of this partition
    Vec_Int_t *     vUniques;        // unique registers of this partition
    Vec_Int_t *     vFreeVars;       // free variables of this partition
    Vec_Flt_t *     vPartCost;       // costs of adding each variable
    char *          pfPartVars;      // input/output registers of the partition
};


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes partitioning of registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_ManPre_t * Aig_ManRegManStart( Aig_Man_t * pAig, int nPartSize )
{
    Aig_ManPre_t * p;
    p = ABC_ALLOC( Aig_ManPre_t, 1 );
    memset( p, 0, sizeof(Aig_ManPre_t) );
    p->pAig      = pAig;
    p->vMatrix   = Aig_ManSupportsRegisters( pAig );
    p->nRegsMax  = nPartSize;
    p->vParts    = Vec_PtrAlloc(256);
    p->vRegs     = Vec_IntAlloc(256);
    p->vUniques  = Vec_IntAlloc(256);
    p->vFreeVars = Vec_IntAlloc(256);
    p->vPartCost = Vec_FltAlloc(256);
    p->pfUsedRegs = ABC_ALLOC( char, Aig_ManRegNum(p->pAig) );
    memset( p->pfUsedRegs, 0, sizeof(char) * Aig_ManRegNum(p->pAig) );
    p->pfPartVars  = ABC_ALLOC( char, Aig_ManRegNum(p->pAig) );
    return p;
}

/**Function*************************************************************

  Synopsis    [Computes partitioning of registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManRegManStop( Aig_ManPre_t * p )
{
    Vec_VecFree( (Vec_Vec_t *)p->vMatrix );
    if ( p->vParts )
        Vec_VecFree( (Vec_Vec_t *)p->vParts );
    Vec_IntFree( p->vRegs );
    Vec_IntFree( p->vUniques );
    Vec_IntFree( p->vFreeVars );
    Vec_FltFree( p->vPartCost );
    ABC_FREE( p->pfUsedRegs );
    ABC_FREE( p->pfPartVars );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Determines what register to use as the seed.]

  Description [The register is selected as the one having the largest
  number of non-taken registers in its support.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManRegFindSeed( Aig_ManPre_t * p )
{
    Vec_Int_t * vRegs;
    int i, k, iReg;
    int iMax = -1; // Suppress "might be used uninitialized"
    int nRegsCur, nRegsMax = -1;
    for ( i = 0; i < Aig_ManRegNum(p->pAig); i++ )
    {
        if ( p->pfUsedRegs[i] )
            continue;
        nRegsCur = 0;
        vRegs = (Vec_Int_t *)Vec_PtrEntry( p->vMatrix, i );
        Vec_IntForEachEntry( vRegs, iReg, k )
            nRegsCur += !p->pfUsedRegs[iReg];
        if ( nRegsMax < nRegsCur )
        {
            nRegsMax = nRegsCur;
            iMax = i;
        }
    }
    return iMax;
}

/**Function*************************************************************

  Synopsis    [Computes the next register to be added to the set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManRegFindBestVar( Aig_ManPre_t * p )
{
    Vec_Int_t * vSupp;
    int nNewVars, nNewVarsBest = ABC_INFINITY;
    int iVarFree, iVarSupp, iVarBest = -1, i, k;
    // go through the free variables
    Vec_IntForEachEntry( p->vFreeVars, iVarFree, i )
    {
//        if ( p->pfUsedRegs[iVarFree] )
//            continue;
        // get support of this variable
        vSupp = (Vec_Int_t *)Vec_PtrEntry( p->vMatrix, iVarFree );
        // count the number of new vars
        nNewVars = 0;
        Vec_IntForEachEntry( vSupp, iVarSupp, k )
        {
            if ( p->pfPartVars[iVarSupp] )
                continue;
            nNewVars += 1 + 3 * p->pfUsedRegs[iVarSupp];
        }
        // quit if there is no new variables
        if ( nNewVars == 0 )
            return iVarFree;
        // compare the cost of this
        if ( nNewVarsBest > nNewVars )
        {
            nNewVarsBest = nNewVars;
            iVarBest = iVarFree;
        }
    }
    return iVarBest;
}

/**Function*************************************************************

  Synopsis    [Computes partitioning of registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManRegPartitionAdd( Aig_ManPre_t * p, int iReg )
{
    Vec_Int_t * vSupp;
    int RetValue, iVar, i;
    // make sure this is a new variable
//    assert( !p->pfUsedRegs[iReg] );
    if ( !p->pfUsedRegs[iReg] )
    {
        p->pfUsedRegs[iReg] = 1;
        Vec_IntPush( p->vUniques, iReg );
    }
    // remove it from the free variables
    if ( Vec_IntSize(p->vFreeVars) > 0 )
    {
        assert( p->pfPartVars[iReg] );
        RetValue = Vec_IntRemove( p->vFreeVars, iReg );
        assert( RetValue );
    }
    else
        assert( !p->pfPartVars[iReg] );
    // add it to the partition
    p->pfPartVars[iReg] = 1;
    Vec_IntPush( p->vRegs, iReg );
    // add new variables
    vSupp = (Vec_Int_t *)Vec_PtrEntry( p->vMatrix, iReg );
    Vec_IntForEachEntry( vSupp, iVar, i )
    {
        if ( p->pfPartVars[iVar] )
            continue;
        p->pfPartVars[iVar] = 1;
        Vec_IntPush( p->vFreeVars, iVar );
    }
    // add it to the cost
    Vec_FltPush( p->vPartCost, 1.0*Vec_IntSize(p->vFreeVars)/Vec_IntSize(p->vRegs) );
}

/**Function*************************************************************

  Synopsis    [Creates projection of 1-hot registers onto the given partition.]

  Description [Assumes that the relevant register outputs are labeled with
  the current traversal ID.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManRegProjectOnehots( Aig_Man_t * pAig, Aig_Man_t * pPart, Vec_Ptr_t * vOnehots, int fVerbose )
{
    Vec_Ptr_t * vOnehotsPart = NULL;
    Vec_Int_t * vGroup, * vGroupNew;
    Aig_Obj_t * pObj, * pObjNew;
    int nOffset, iReg, i, k;
    // set the PI numbers
    Aig_ManForEachCi( pPart, pObj, i )
        pObj->iData = i;
    // go through each group and check if registers are involved in this one
    nOffset = Aig_ManCiNum(pAig)-Aig_ManRegNum(pAig);
    Vec_PtrForEachEntry( Vec_Int_t *, vOnehots, vGroup, i )
    {
        vGroupNew = NULL;
        Vec_IntForEachEntry( vGroup, iReg, k )
        {
            pObj = Aig_ManCi( pAig, nOffset+iReg );
            if ( !Aig_ObjIsTravIdCurrent(pAig, pObj) )
                continue;
            if ( vGroupNew == NULL )
                vGroupNew = Vec_IntAlloc( Vec_IntSize(vGroup) );
            pObjNew = (Aig_Obj_t *)pObj->pData;
            Vec_IntPush( vGroupNew, pObjNew->iData );
        }
        if ( vGroupNew == NULL )
            continue;
        if ( Vec_IntSize(vGroupNew) > 1 )
        {
            if ( vOnehotsPart == NULL )
                vOnehotsPart = Vec_PtrAlloc( 100 );
            Vec_PtrPush( vOnehotsPart, vGroupNew );
        }
        else
            Vec_IntFree( vGroupNew );
    }
    // clear the PI numbers
    Aig_ManForEachCi( pPart, pObj, i )
        pObj->iData = 0;
    // print out
    if ( vOnehotsPart && fVerbose )
    {
        printf( "Partition contains %d groups of 1-hot registers: { ", Vec_PtrSize(vOnehotsPart) );
        Vec_PtrForEachEntry( Vec_Int_t *, vOnehotsPart, vGroup, k )
            printf( "%d ", Vec_IntSize(vGroup) );
        printf( "}\n" );
    }
    return vOnehotsPart;
}

/**Function*************************************************************

  Synopsis    [Computes partitioning of registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManRegCreatePart( Aig_Man_t * pAig, Vec_Int_t * vPart, int * pnCountPis, int * pnCountRegs, int ** ppMapBack )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj, * pObjNew;
    Vec_Ptr_t * vNodes;
    Vec_Ptr_t * vRoots;
    int nOffset, iOut, i;
    int nCountPis, nCountRegs;
    int * pMapBack;
    // collect roots
    vRoots = Vec_PtrAlloc( Vec_IntSize(vPart) );
    nOffset = Aig_ManCoNum(pAig)-Aig_ManRegNum(pAig);
    Vec_IntForEachEntry( vPart, iOut, i )
    {
        pObj = Aig_ManCo(pAig, nOffset+iOut);
        Vec_PtrPush( vRoots, Aig_ObjFanin0(pObj) );
    }
    // collect/mark nodes/PIs in the DFS order
    vNodes = Aig_ManDfsNodes( pAig, (Aig_Obj_t **)Vec_PtrArray(vRoots), Vec_PtrSize(vRoots) );
    Vec_PtrFree( vRoots );
    // unmark register outputs
    nOffset = Aig_ManCiNum(pAig)-Aig_ManRegNum(pAig);
    Vec_IntForEachEntry( vPart, iOut, i )
    {
        pObj = Aig_ManCi(pAig, nOffset+iOut);
        Aig_ObjSetTravIdPrevious( pAig, pObj );
    }
    // count pure PIs
    nCountPis = nCountRegs = 0;
    Aig_ManForEachPiSeq( pAig, pObj, i )
        nCountPis += Aig_ObjIsTravIdCurrent(pAig, pObj);
    // count outputs of other registers
    Aig_ManForEachLoSeq( pAig, pObj, i )
        nCountRegs += Aig_ObjIsTravIdCurrent(pAig, pObj); 
    if ( pnCountPis )
        *pnCountPis = nCountPis;
    if ( pnCountRegs )
        *pnCountRegs = nCountRegs;
    // create the new manager
    pNew = Aig_ManStart( Vec_PtrSize(vNodes) );
    Aig_ManConst1(pAig)->pData = Aig_ManConst1(pNew);
    // create the PIs
    Aig_ManForEachCi( pAig, pObj, i )
        if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
            pObj->pData = Aig_ObjCreateCi(pNew);
    // add variables for the register outputs
    // create fake POs to hold the register outputs
    nOffset = Aig_ManCiNum(pAig)-Aig_ManRegNum(pAig);
    Vec_IntForEachEntry( vPart, iOut, i )
    {
        pObj = Aig_ManCi(pAig, nOffset+iOut);
        pObj->pData = Aig_ObjCreateCi(pNew);
        Aig_ObjCreateCo( pNew, (Aig_Obj_t *)pObj->pData );
        Aig_ObjSetTravIdCurrent( pAig, pObj ); // added
    }
    // create the nodes
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        if ( Aig_ObjIsNode(pObj) )
            pObj->pData = Aig_And(pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // add real POs for the registers
    nOffset = Aig_ManCoNum(pAig)-Aig_ManRegNum(pAig);
    Vec_IntForEachEntry( vPart, iOut, i )
    {
        pObj = Aig_ManCo( pAig, nOffset+iOut );
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    }
    pNew->nRegs = Vec_IntSize(vPart);
    // create map
    if ( ppMapBack )
    {
        pMapBack = ABC_ALLOC( int, Aig_ManObjNumMax(pNew) );
        memset( pMapBack, 0xff, sizeof(int) * Aig_ManObjNumMax(pNew) );
        // map constant nodes
        pMapBack[0] = 0;
        // logic cones of register outputs
        Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        {
            pObjNew = Aig_Regular((Aig_Obj_t *)pObj->pData);
            pMapBack[pObjNew->Id] = pObj->Id;
        }
        // map register outputs
        nOffset = Aig_ManCiNum(pAig)-Aig_ManRegNum(pAig);
        Vec_IntForEachEntry( vPart, iOut, i )
        {
            pObj = Aig_ManCi(pAig, nOffset+iOut);
            pObjNew = (Aig_Obj_t *)pObj->pData;
            pMapBack[pObjNew->Id] = pObj->Id;
        }
        *ppMapBack = pMapBack;
    }
    Vec_PtrFree( vNodes );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Computes partitioning of registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManRegPartitionSmart( Aig_Man_t * pAig, int nPartSize )
{
    extern void Ioa_WriteAiger( Aig_Man_t * pMan, char * pFileName, int fWriteSymbols, int fCompact );

    Aig_ManPre_t * p;
    Vec_Ptr_t * vResult;
    int iSeed, iNext, i, k;
    // create the manager
    p = Aig_ManRegManStart( pAig, nPartSize );
    // add partitions as long as registers remain
    for ( i = 0; (iSeed = Aig_ManRegFindSeed(p)) >= 0; i++ )
    {
//printf( "Seed variable = %d.\n", iSeed );
        // clean the current partition information
        Vec_IntClear( p->vRegs );
        Vec_IntClear( p->vUniques );
        Vec_IntClear( p->vFreeVars );
        Vec_FltClear( p->vPartCost );
        memset( p->pfPartVars, 0, sizeof(char) * Aig_ManRegNum(p->pAig) );
        // add the register and its partition support
        Aig_ManRegPartitionAdd( p, iSeed );
        // select the best var to add
        for ( k = 0; Vec_IntSize(p->vRegs) < p->nRegsMax; k++ )
        {
            // get the next best variable
            iNext = Aig_ManRegFindBestVar( p );
            if ( iNext == -1 )
                break;
            // add the register to the support of the partition
            Aig_ManRegPartitionAdd( p, iNext );
            // report the result
//printf( "Part %3d  Reg %3d : Free = %4d. Total = %4d. Ratio = %6.2f. Unique = %4d.\n", i, k, 
//                Vec_IntSize(p->vFreeVars), Vec_IntSize(p->vRegs), 
//                1.0*Vec_IntSize(p->vFreeVars)/Vec_IntSize(p->vRegs), Vec_IntSize(p->vUniques) );
            // quit if there are not free variables
            if ( Vec_IntSize(p->vFreeVars) == 0 )
                break;
        }
        // add this partition to the set
        Vec_PtrPush( p->vParts, Vec_IntDup(p->vRegs) );        
printf( "Part %3d  SUMMARY:  Free = %4d. Total = %4d. Ratio = %6.2f. Unique = %4d.\n", i,
                Vec_IntSize(p->vFreeVars), Vec_IntSize(p->vRegs), 
                1.0*Vec_IntSize(p->vFreeVars)/Vec_IntSize(p->vRegs), Vec_IntSize(p->vUniques) );
//printf( "\n" ); 
    }
    vResult = p->vParts; p->vParts = NULL;
    Aig_ManRegManStop( p );
    return vResult;
}

/**Function*************************************************************

  Synopsis    [Computes partitioning of registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManRegPartitionSimple( Aig_Man_t * pAig, int nPartSize, int nOverSize )
{
    Vec_Ptr_t * vResult;
    Vec_Int_t * vPart;
    int i, Counter;
    if ( nOverSize >= nPartSize )
    {
        printf( "Overlap size (%d) is more or equal than the partition size (%d).\n", nOverSize, nPartSize );
        printf( "Adjusting it to be equal to half of the partition size.\n" );
        nOverSize = nPartSize/2;
    }
    assert( nOverSize < nPartSize );
    vResult = Vec_PtrAlloc( 100 );
    for ( Counter = 0; Counter < Aig_ManRegNum(pAig); Counter -= nOverSize )
    {
        vPart = Vec_IntAlloc( nPartSize );
        for ( i = 0; i < nPartSize; i++, Counter++ )
            if ( Counter < Aig_ManRegNum(pAig) )
                Vec_IntPush( vPart, Counter );
        if ( Vec_IntSize(vPart) <= nOverSize )
            Vec_IntFree(vPart);
        else
            Vec_PtrPush( vResult, vPart );
    }
    return vResult;
}


/**Function*************************************************************

  Synopsis    [Divides a large partition into several ones.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPartDivide( Vec_Ptr_t * vResult, Vec_Int_t * vDomain, int nPartSize, int nOverSize )
{
    Vec_Int_t * vPart;
    int i, Counter;
    assert( nPartSize && Vec_IntSize(vDomain) > nPartSize );
    if ( nOverSize >= nPartSize )
    {
        printf( "Overlap size (%d) is more or equal than the partition size (%d).\n", nOverSize, nPartSize );
        printf( "Adjusting it to be equal to half of the partition size.\n" );
        nOverSize = nPartSize/2;
    }
    assert( nOverSize < nPartSize );
    for ( Counter = 0; Counter < Vec_IntSize(vDomain); Counter -= nOverSize )
    {
        vPart = Vec_IntAlloc( nPartSize );
        for ( i = 0; i < nPartSize; i++, Counter++ )
            if ( Counter < Vec_IntSize(vDomain) )
                Vec_IntPush( vPart, Vec_IntEntry(vDomain, Counter) );
        if ( Vec_IntSize(vPart) <= nOverSize )
            Vec_IntFree(vPart);
        else
            Vec_PtrPush( vResult, vPart );
    }
}

/**Function*************************************************************

  Synopsis    [Computes partitioning of registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManRegPartitionTraverse_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vLos )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent( p, pObj );
    if ( Aig_ObjIsCi(pObj) )
    {
        if ( pObj->iData >= Aig_ManCiNum(p) - Aig_ManRegNum(p) )
        {
            Vec_PtrPush( vLos, pObj );
            printf( "%d ", pObj->iData - (Aig_ManCiNum(p) - Aig_ManRegNum(p)) );
        }
        return;
    }
    Aig_ManRegPartitionTraverse_rec( p, Aig_ObjFanin0(pObj), vLos );
    Aig_ManRegPartitionTraverse_rec( p, Aig_ObjFanin1(pObj), vLos );
}

/**Function*************************************************************

  Synopsis    [Computes partitioning of registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManRegPartitionTraverse( Aig_Man_t * p )
{
    Vec_Ptr_t * vLos;
    Aig_Obj_t * pObj;
    int i, nPrev, Counter;
    // mark the registers
    Aig_ManForEachCi( p, pObj, i )
       pObj->iData = i; 
    // collect registers
    vLos = Vec_PtrAlloc( Aig_ManRegNum(p) );
    nPrev = 0;
    Counter = 0;
    Aig_ManIncrementTravId( p );
    Aig_ManForEachLiSeq( p, pObj, i )
    {
        ++Counter;
        printf( "Latch %d: ", Counter );
        Aig_ManRegPartitionTraverse_rec( p, Aig_ObjFanin0(pObj), vLos );
printf( "%d=%d \n", Counter, Vec_PtrSize(vLos)-nPrev );
        nPrev = Vec_PtrSize(vLos);
    }
    printf( "Total collected = %d. Total regs = %d.\n", Vec_PtrSize(vLos), Aig_ManRegNum(p) );
    return vLos;
}

/**Function*************************************************************

  Synopsis    [Computes partitioning of registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManRegPartitionLinear( Aig_Man_t * pAig, int nPartSize )
{
    Vec_Ptr_t * vLos;
    vLos = Aig_ManRegPartitionTraverse( pAig );
    Vec_PtrFree( vLos );
    return NULL;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

