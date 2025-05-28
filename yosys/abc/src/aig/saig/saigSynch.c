/**CFile****************************************************************

  FileName    [saigSynch.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Computation of synchronizing sequence.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigSynch.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

//        0  1  x
//       00 01 11
// 0  00 00 00 00
// 1  01 00 01 11
// x  11 00 11 11

static inline unsigned Saig_SynchNot( unsigned w )
{
    return w^((~(w&(w>>1)))&0x55555555);
}
static inline unsigned Saig_SynchAnd( unsigned u, unsigned w )
{
    return (u&w)|((((u&(u>>1)&w&~(w>>1))|(w&(w>>1)&u&~(u>>1)))&0x55555555)<<1);
}
static inline unsigned Saig_SynchRandomBinary() 
{ 
    return Aig_ManRandom(0) & 0x55555555;               
}
static inline unsigned Saig_SynchRandomTernary() 
{ 
    unsigned w = Aig_ManRandom(0);
    return w^((~w)&(w>>1)&0x55555555);        
}
static inline unsigned Saig_SynchTernary( int v ) 
{
    assert( v == 0 || v == 1 || v == 3 );
    return v? ((v==1)? 0x55555555 : 0xffffffff) : 0;
}


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Initializes registers to the ternary state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_SynchSetConstant1( Aig_Man_t * pAig, Vec_Ptr_t * vSimInfo, int nWords )
{
    Aig_Obj_t * pObj;
    unsigned * pSim;
    int w;
    pObj = Aig_ManConst1( pAig );
    pSim = (unsigned *)Vec_PtrEntry( vSimInfo, pObj->Id );
    for ( w = 0; w < nWords; w++ )
        pSim[w] = 0x55555555;
}

/**Function*************************************************************

  Synopsis    [Initializes registers to the ternary state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_SynchInitRegsTernary( Aig_Man_t * pAig, Vec_Ptr_t * vSimInfo, int nWords )
{
    Aig_Obj_t * pObj;
    unsigned * pSim;
    int i, w;
    Saig_ManForEachLo( pAig, pObj, i )
    {
        pSim = (unsigned *)Vec_PtrEntry( vSimInfo, pObj->Id );
        for ( w = 0; w < nWords; w++ )
            pSim[w] = 0xffffffff;
    }
}

/**Function*************************************************************

  Synopsis    [Initializes registers to the given binary state.]

  Description [The binary state is stored in pObj->fMarkA.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_SynchInitRegsBinary( Aig_Man_t * pAig, Vec_Ptr_t * vSimInfo, int nWords )
{
    Aig_Obj_t * pObj;
    unsigned * pSim;
    int i, w;
    Saig_ManForEachLo( pAig, pObj, i )
    {
        pSim = (unsigned *)Vec_PtrEntry( vSimInfo, pObj->Id );
        for ( w = 0; w < nWords; w++ )
            pSim[w] = Saig_SynchTernary( pObj->fMarkA );
    }
}

/**Function*************************************************************

  Synopsis    [Initializes random binary primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_SynchInitPisRandom( Aig_Man_t * pAig, Vec_Ptr_t * vSimInfo, int nWords )
{
    Aig_Obj_t * pObj;
    unsigned * pSim;
    int i, w;
    Saig_ManForEachPi( pAig, pObj, i )
    {
        pSim = (unsigned *)Vec_PtrEntry( vSimInfo, pObj->Id );
        for ( w = 0; w < nWords; w++ )
            pSim[w] = Saig_SynchRandomBinary();
    }
}

/**Function*************************************************************

  Synopsis    [Initializes random binary primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_SynchInitPisGiven( Aig_Man_t * pAig, Vec_Ptr_t * vSimInfo, int nWords, char * pValues )
{
    Aig_Obj_t * pObj;
    unsigned * pSim;
    int i, w;
    Saig_ManForEachPi( pAig, pObj, i )
    {
        pSim = (unsigned *)Vec_PtrEntry( vSimInfo, pObj->Id );
        for ( w = 0; w < nWords; w++ )
            pSim[w] = Saig_SynchTernary( pValues[i] );
    }
}

/**Function*************************************************************

  Synopsis    [Performs ternary simulation of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_SynchTernarySimulate( Aig_Man_t * pAig, Vec_Ptr_t * vSimInfo, int nWords )
{
    Aig_Obj_t * pObj;
    unsigned * pSim0, * pSim1, * pSim;
    int i, w;
    // simulate nodes
    Aig_ManForEachNode( pAig, pObj, i )
    {
        pSim  = (unsigned *)Vec_PtrEntry( vSimInfo, pObj->Id );
        pSim0 = (unsigned *)Vec_PtrEntry( vSimInfo, Aig_ObjFaninId0(pObj) );
        pSim1 = (unsigned *)Vec_PtrEntry( vSimInfo, Aig_ObjFaninId1(pObj) );
        if ( Aig_ObjFaninC0(pObj) && Aig_ObjFaninC1(pObj) )
        {
            for ( w = 0; w < nWords; w++ )
                pSim[w] = Saig_SynchAnd( Saig_SynchNot(pSim0[w]), Saig_SynchNot(pSim1[w]) );
        }
        else if ( !Aig_ObjFaninC0(pObj) && Aig_ObjFaninC1(pObj) )
        {
            for ( w = 0; w < nWords; w++ )
                pSim[w] = Saig_SynchAnd( pSim0[w], Saig_SynchNot(pSim1[w]) );
        }
        else if ( Aig_ObjFaninC0(pObj) && !Aig_ObjFaninC1(pObj) )
        {
            for ( w = 0; w < nWords; w++ )
                pSim[w] = Saig_SynchAnd( Saig_SynchNot(pSim0[w]), pSim1[w] );
        }
        else // if ( !Aig_ObjFaninC0(pObj) && !Aig_ObjFaninC1(pObj) )
        {
            for ( w = 0; w < nWords; w++ )
                pSim[w] = Saig_SynchAnd( pSim0[w], pSim1[w] );
        }
    }
    // transfer values to register inputs
    Saig_ManForEachLi( pAig, pObj, i )
    {
        pSim  = (unsigned *)Vec_PtrEntry( vSimInfo, pObj->Id );
        pSim0 = (unsigned *)Vec_PtrEntry( vSimInfo, Aig_ObjFaninId0(pObj) );
        if ( Aig_ObjFaninC0(pObj) )
        {
            for ( w = 0; w < nWords; w++ )
                pSim[w] = Saig_SynchNot( pSim0[w] );
        }
        else
        {
            for ( w = 0; w < nWords; w++ )
                pSim[w] = pSim0[w];
        }
    }
}

/**Function*************************************************************

  Synopsis    [Performs ternary simulation of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_SynchTernaryTransferState( Aig_Man_t * pAig, Vec_Ptr_t * vSimInfo, int nWords )
{
    Aig_Obj_t * pObjLi, * pObjLo;
    unsigned * pSim0, * pSim1;
    int i, w;
    Saig_ManForEachLiLo( pAig, pObjLi, pObjLo, i )
    {
        pSim0 = (unsigned *)Vec_PtrEntry( vSimInfo, pObjLi->Id );
        pSim1 = (unsigned *)Vec_PtrEntry( vSimInfo, pObjLo->Id );
        for ( w = 0; w < nWords; w++ )
            pSim1[w] = pSim0[w];
    }
}

/**Function*************************************************************

  Synopsis    [Returns the number of Xs in the smallest ternary pattern.]

  Description [Returns the number of this pattern.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_SynchCountX( Aig_Man_t * pAig, Vec_Ptr_t * vSimInfo, int nWords, int * piPat )
{
    Aig_Obj_t * pObj;
    unsigned * pSim;
    int * pCounters, i, w, b;
    int iPatBest, iTernMin;
    // count the number of ternary values in each pattern
    pCounters = ABC_CALLOC( int, nWords * 16 );
    Saig_ManForEachLi( pAig, pObj, i )
    {
        pSim = (unsigned *)Vec_PtrEntry( vSimInfo, pObj->Id );
        for ( w = 0; w < nWords; w++ )
            for ( b = 0; b < 16; b++ )
                if ( ((pSim[w] >> (b << 1)) & 3) == 3 )
                    pCounters[16 * w + b]++;
    }
    // get the best pattern
    iPatBest = -1;
    iTernMin = 1 + Saig_ManRegNum(pAig);
    for ( b = 0; b < 16 * nWords; b++ )
        if ( iTernMin > pCounters[b] )
        {
            iTernMin = pCounters[b];
            iPatBest = b;
            if ( iTernMin == 0 )
                break;
        }
    ABC_FREE( pCounters );
    *piPat = iPatBest;
    return iTernMin;
}

/**Function*************************************************************

  Synopsis    [Saves the best pattern found and initializes the registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_SynchSavePattern( Aig_Man_t * pAig, Vec_Ptr_t * vSimInfo, int nWords, int iPat, Vec_Str_t * vSequence )
{
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    unsigned * pSim;
    int Counter, Value, i, w;
    assert( iPat < 16 * nWords );
    Saig_ManForEachPi( pAig, pObj, i )
    {
        pSim = (unsigned *)Vec_PtrEntry( vSimInfo, pObj->Id );
        Value = (pSim[iPat>>4] >> ((iPat&0xf) << 1)) & 3;
        Vec_StrPush( vSequence, (char)Value );
//        printf( "%d ", Value );
    }
//    printf( "\n" );
    Counter = 0;
    Saig_ManForEachLiLo( pAig, pObjLi, pObjLo, i )
    {
        pSim = (unsigned *)Vec_PtrEntry( vSimInfo, pObjLi->Id );
        Value = (pSim[iPat>>4] >> ((iPat&0xf) << 1)) & 3;
        Counter += (Value == 3);
        // save patern in the same register
        pSim = (unsigned *)Vec_PtrEntry( vSimInfo, pObjLo->Id );
        for ( w = 0; w < nWords; w++ )
            pSim[w] = Saig_SynchTernary( Value );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Implement synchronizing sequence.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_SynchSequenceRun( Aig_Man_t * pAig, Vec_Ptr_t * vSimInfo, Vec_Str_t * vSequence, int fTernary )
{
    unsigned * pSim;
    Aig_Obj_t * pObj;
    int Counter, nIters, Value, i;
    assert( Vec_StrSize(vSequence) % Saig_ManPiNum(pAig) == 0 );
    nIters = Vec_StrSize(vSequence) / Saig_ManPiNum(pAig);
    Saig_SynchSetConstant1( pAig, vSimInfo, 1 );
    if ( fTernary )
        Saig_SynchInitRegsTernary( pAig, vSimInfo, 1 ); 
    else
        Saig_SynchInitRegsBinary( pAig, vSimInfo, 1 ); 
    for ( i = 0; i < nIters; i++ )
    {
        Saig_SynchInitPisGiven( pAig, vSimInfo, 1, Vec_StrArray(vSequence) + i * Saig_ManPiNum(pAig) );
        Saig_SynchTernarySimulate( pAig, vSimInfo, 1 );
        Saig_SynchTernaryTransferState( pAig, vSimInfo, 1 );
    }
    // save the resulting state in the registers
    Counter = 0;
    Saig_ManForEachLo( pAig, pObj, i )
    {
        pSim = (unsigned *)Vec_PtrEntry( vSimInfo, pObj->Id );
        Value = pSim[0] & 3;
        assert( Value != 2 );
        Counter += (Value == 3);
        pObj->fMarkA = Value;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Determines synchronizing sequence using ternary simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Saig_SynchSequence( Aig_Man_t * pAig, int nWords )
{
    int nStepsMax = 100;  // the maximum number of simulation steps
    int nTriesMax = 100;  // the maximum number of attempts at each step
    int fVerify   =   1;  // verify the resulting pattern
    Vec_Str_t * vSequence;
    Vec_Ptr_t * vSimInfo;
    int nTerPrev, nTerCur = 0, nTerCur2;
    int iPatBest, RetValue, s, t;
    assert( Saig_ManRegNum(pAig) > 0 );
    // reset random numbers
    Aig_ManRandom( 1 );
    // start the sequence
    vSequence = Vec_StrAlloc( 20 * Saig_ManRegNum(pAig) );
    // create sim info and init registers
    vSimInfo = Vec_PtrAllocSimInfo( Aig_ManObjNumMax(pAig), nWords );
    Saig_SynchSetConstant1( pAig, vSimInfo, nWords );
    // iterate over the timeframes
    nTerPrev = Saig_ManRegNum(pAig);
    Saig_SynchInitRegsTernary( pAig, vSimInfo, nWords );
    for ( s = 0; s < nStepsMax && nTerPrev > 0; s++ )
    {
        for ( t = 0; t < nTriesMax; t++ )
        {
            Saig_SynchInitPisRandom( pAig, vSimInfo, nWords );
            Saig_SynchTernarySimulate( pAig, vSimInfo, nWords );
            nTerCur = Saig_SynchCountX( pAig, vSimInfo, nWords, &iPatBest );
            if ( nTerCur < nTerPrev )
                break;
        }
        if ( t == nTriesMax )
            break;
        nTerCur2 = Saig_SynchSavePattern( pAig, vSimInfo, nWords, iPatBest, vSequence );
        assert( nTerCur == nTerCur2 );
        nTerPrev = nTerCur;
    }
    if ( nTerPrev > 0 )
    {
        printf( "Count not initialize %d registers.\n", nTerPrev );
        Vec_PtrFree( vSimInfo );
        Vec_StrFree( vSequence );
        return NULL;
    }
    // verify that the sequence is correct
    if ( fVerify )
    {
        RetValue = Saig_SynchSequenceRun( pAig, vSimInfo, vSequence, 1 );
        assert( RetValue == 0 );
        Aig_ManCleanMarkA( pAig );
    }
    Vec_PtrFree( vSimInfo );
    return vSequence;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG to have constant-0 initial state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManDupInitZero( Aig_Man_t * p )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj;
    int i;
    pNew = Aig_ManStart( Aig_ManObjNumMax(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
    Saig_ManForEachPi( p, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    Saig_ManForEachLo( p, pObj, i )
        pObj->pData = Aig_NotCond( Aig_ObjCreateCi( pNew ), pObj->fMarkA );
    Aig_ManForEachNode( p, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    Saig_ManForEachPo( p, pObj, i )
        pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    Saig_ManForEachLi( p, pObj, i )
        pObj->pData = Aig_ObjCreateCo( pNew, Aig_NotCond( Aig_ObjChild0Copy(pObj), pObj->fMarkA ) );
    Aig_ManSetRegNum( pNew, Saig_ManRegNum(p) );
    assert( Aig_ManNodeNum(pNew) == Aig_ManNodeNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Determines synchronizing sequence using ternary simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_SynchSequenceApply( Aig_Man_t * pAig, int nWords, int fVerbose )
{
    Aig_Man_t * pAigZero;
    Vec_Str_t * vSequence;
    Vec_Ptr_t * vSimInfo;
    int RetValue;
    abctime clk;

clk = Abc_Clock();
    // derive synchronization sequence
    vSequence = Saig_SynchSequence( pAig, nWords );
    if ( vSequence == NULL )
        printf( "Design 1: Synchronizing sequence is not found. " );
    else if ( fVerbose )
        printf( "Design 1: Synchronizing sequence of length %4d is found. ", Vec_StrSize(vSequence) / Saig_ManPiNum(pAig) );
    if ( fVerbose )
    {
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
    else
        printf( "\n" );
    if ( vSequence == NULL )
    {
        printf( "Quitting synchronization.\n" );
        return NULL;
    }

    // apply synchronization sequence
    vSimInfo = Vec_PtrAllocSimInfo( Aig_ManObjNumMax(pAig), 1 );
    RetValue = Saig_SynchSequenceRun( pAig, vSimInfo, vSequence, 1 );
    assert( RetValue == 0 );
    // duplicate 
    pAigZero = Saig_ManDupInitZero( pAig );
    // cleanup
    Vec_PtrFree( vSimInfo );
    Vec_StrFree( vSequence );
    Aig_ManCleanMarkA( pAig );
    return pAigZero;
}

/**Function*************************************************************

  Synopsis    [Creates SEC miter for two designs without initial state.]

  Description [The designs (pAig1 and pAig2) are assumed to have ternary 
  initial state. Determines synchronizing sequences using ternary simulation.
  Simulates the sequences on both designs to come up with equivalent binary 
  initial states. Create seq miter for the designs starting in these states.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_Synchronize( Aig_Man_t * pAig1, Aig_Man_t * pAig2, int nWords, int fVerbose )
{
    Aig_Man_t * pAig1z, * pAig2z, * pMiter;
    Vec_Str_t * vSeq1, * vSeq2;
    Vec_Ptr_t * vSimInfo;
    int RetValue;
    abctime clk;
/*
    {
        unsigned u = Saig_SynchRandomTernary();
        unsigned w = Saig_SynchRandomTernary();
        unsigned x = Saig_SynchNot( u );
        unsigned y = Saig_SynchNot( w );
        unsigned z = Saig_SynchAnd( x, y );

        Extra_PrintBinary( stdout, &u, 32 );  printf( "\n" );
        Extra_PrintBinary( stdout, &w, 32 );  printf( "\n" );  printf( "\n" );
        Extra_PrintBinary( stdout, &x, 32 );  printf( "\n" );
        Extra_PrintBinary( stdout, &y, 32 );  printf( "\n" );  printf( "\n" );
        Extra_PrintBinary( stdout, &z, 32 );  printf( "\n" );
    }
*/
    // report statistics
    if ( fVerbose )
    {
        printf( "Design 1: " );
        Aig_ManPrintStats( pAig1 );
        printf( "Design 2: " );
        Aig_ManPrintStats( pAig2 );
    }

    // synchronize the first design
    clk = Abc_Clock();
    vSeq1 = Saig_SynchSequence( pAig1, nWords );
    if ( vSeq1 == NULL )
        printf( "Design 1: Synchronizing sequence is not found. " );
    else if ( fVerbose )
        printf( "Design 1: Synchronizing sequence of length %4d is found. ", Vec_StrSize(vSeq1) / Saig_ManPiNum(pAig1) );
    if ( fVerbose )
    {
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
    else
        printf( "\n" );

    // synchronize the first design
    clk = Abc_Clock();
    vSeq2 = Saig_SynchSequence( pAig2, nWords );
    if ( vSeq2 == NULL )
        printf( "Design 2: Synchronizing sequence is not found. " );
    else if ( fVerbose )
        printf( "Design 2: Synchronizing sequence of length %4d is found. ", Vec_StrSize(vSeq2) / Saig_ManPiNum(pAig2) );
    if ( fVerbose )
    {
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
    else
        printf( "\n" );

    // quit if one of the designs cannot be synchronized
    if ( vSeq1 == NULL || vSeq2 == NULL )
    {
        printf( "Quitting synchronization.\n" );
        if ( vSeq1 ) Vec_StrFree( vSeq1 );
        if ( vSeq2 ) Vec_StrFree( vSeq2 );
        return NULL;
    }
    clk = Abc_Clock();
    vSimInfo = Vec_PtrAllocSimInfo( Abc_MaxInt( Aig_ManObjNumMax(pAig1), Aig_ManObjNumMax(pAig2) ), 1 );

    // process Design 1
    RetValue = Saig_SynchSequenceRun( pAig1, vSimInfo, vSeq1, 1 );
    assert( RetValue == 0 );
    RetValue = Saig_SynchSequenceRun( pAig1, vSimInfo, vSeq2, 0 );
    assert( RetValue == 0 );

    // process Design 2
    RetValue = Saig_SynchSequenceRun( pAig2, vSimInfo, vSeq2, 1 );
    assert( RetValue == 0 );

    // duplicate designs
    pAig1z = Saig_ManDupInitZero( pAig1 );
    pAig2z = Saig_ManDupInitZero( pAig2 );
    pMiter = Saig_ManCreateMiter( pAig1z, pAig2z, 0 );
    Aig_ManCleanup( pMiter );
    Aig_ManStop( pAig1z );
    Aig_ManStop( pAig2z );

    // cleanup
    Vec_PtrFree( vSimInfo );
    Vec_StrFree( vSeq1 );
    Vec_StrFree( vSeq2 );
    Aig_ManCleanMarkA( pAig1 );
    Aig_ManCleanMarkA( pAig2 );

    if ( fVerbose )
    {
        printf( "Miter of the synchronized designs is constructed.         " );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
    return pMiter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

