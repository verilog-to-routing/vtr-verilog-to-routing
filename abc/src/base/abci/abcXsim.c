/**CFile****************************************************************

  FileName    [abcXsim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Using X-valued simulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcXsim.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "aig/gia/gia.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define XVS0   ABC_INIT_ZERO
#define XVS1   ABC_INIT_ONE
#define XVSX   ABC_INIT_DC

static inline void Abc_ObjSetXsim( Abc_Obj_t * pObj, int Value )  { pObj->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)Value;  }
static inline int  Abc_ObjGetXsim( Abc_Obj_t * pObj )             { return (int)(ABC_PTRINT_T)pObj->pCopy;           }
static inline int  Abc_XsimInv( int Value )   
{ 
    if ( Value == XVS0 )
        return XVS1;
    if ( Value == XVS1 )
        return XVS0;
    assert( Value == XVSX );       
    return XVSX;
}
static inline int  Abc_XsimAnd( int Value0, int Value1 )   
{ 
    if ( Value0 == XVS0 || Value1 == XVS0 )
        return XVS0;
    if ( Value0 == XVSX || Value1 == XVSX )
        return XVSX;
    assert( Value0 == XVS1 && Value1 == XVS1 );
    return XVS1;
}
static inline int  Abc_XsimRand2()   
{
//    return (rand() & 1) ? XVS1 : XVS0;
    return (Gia_ManRandom(0) & 1) ? XVS1 : XVS0;
}
static inline int  Abc_XsimRand3()   
{
    int RetValue;
    do { 
//        RetValue = rand() & 3; 
        RetValue = Gia_ManRandom(0) & 3; 
    } while ( RetValue == 0 );
    return RetValue;
}
static inline int  Abc_ObjGetXsimFanin0( Abc_Obj_t * pObj )       
{ 
    int RetValue;
    RetValue = Abc_ObjGetXsim(Abc_ObjFanin0(pObj));
    return Abc_ObjFaninC0(pObj)? Abc_XsimInv(RetValue) : RetValue;
}
static inline int  Abc_ObjGetXsimFanin1( Abc_Obj_t * pObj )       
{ 
    int RetValue;
    RetValue = Abc_ObjGetXsim(Abc_ObjFanin1(pObj));
    return Abc_ObjFaninC1(pObj)? Abc_XsimInv(RetValue) : RetValue;
}
static inline void Abc_XsimPrint( FILE * pFile, int Value )   
{ 
    if ( Value == XVS0 )
    {
        fprintf( pFile, "0" );
        return;
    }
    if ( Value == XVS1 )
    {
        fprintf( pFile, "1" );
        return;
    }
    assert( Value == XVSX );       
    fprintf( pFile, "x" );
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs X-valued simulation of the sequential network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkXValueSimulate( Abc_Ntk_t * pNtk, int nFrames, int fXInputs, int fXState, int fVerbose )
{
    Abc_Obj_t * pObj;
    int i, f;
    assert( Abc_NtkIsStrash(pNtk) );
//    srand( 0x12341234 );
    Gia_ManRandom( 1 );
    // start simulation
    Abc_ObjSetXsim( Abc_AigConst1(pNtk), XVS1 );
    if ( fXInputs )
    {
        Abc_NtkForEachPi( pNtk, pObj, i )
            Abc_ObjSetXsim( pObj, XVSX );
    }
    else
    {
        Abc_NtkForEachPi( pNtk, pObj, i )
            Abc_ObjSetXsim( pObj, Abc_XsimRand2() );
    }
    if ( fXState )
    {
        Abc_NtkForEachLatch( pNtk, pObj, i )
            Abc_ObjSetXsim( Abc_ObjFanout0(pObj), XVSX );
    }
    else
    {
        Abc_NtkForEachLatch( pNtk, pObj, i )
            Abc_ObjSetXsim( Abc_ObjFanout0(pObj), Abc_LatchInit(pObj) );
    }
    // simulate and print the result
    fprintf( stdout, "Frame : Inputs : Latches : Outputs\n" );
    for ( f = 0; f < nFrames; f++ )
    {
        Abc_AigForEachAnd( pNtk, pObj, i )
            Abc_ObjSetXsim( pObj, Abc_XsimAnd(Abc_ObjGetXsimFanin0(pObj), Abc_ObjGetXsimFanin1(pObj)) );
        Abc_NtkForEachCo( pNtk, pObj, i )
            Abc_ObjSetXsim( pObj, Abc_ObjGetXsimFanin0(pObj) );
        // print out
        fprintf( stdout, "%2d : ", f );
        Abc_NtkForEachPi( pNtk, pObj, i )
            Abc_XsimPrint( stdout, Abc_ObjGetXsim(pObj) );
        fprintf( stdout, " : " );
        Abc_NtkForEachLatch( pNtk, pObj, i )
        {
//            if ( Abc_ObjGetXsim(Abc_ObjFanout0(pObj)) != XVSX )
//                printf( " %s=", Abc_ObjName(pObj) );
            Abc_XsimPrint( stdout, Abc_ObjGetXsim(Abc_ObjFanout0(pObj)) );
        }
        fprintf( stdout, " : " );
        Abc_NtkForEachPo( pNtk, pObj, i )
            Abc_XsimPrint( stdout, Abc_ObjGetXsim(pObj) );
        fprintf( stdout, "\n" );
        // assign input values
        if ( fXInputs )
        {
            Abc_NtkForEachPi( pNtk, pObj, i )
                Abc_ObjSetXsim( pObj, XVSX );
        }
        else
        {
            Abc_NtkForEachPi( pNtk, pObj, i )
                Abc_ObjSetXsim( pObj, Abc_XsimRand2() );
        }
        // transfer the latch values
        Abc_NtkForEachLatch( pNtk, pObj, i )
            Abc_ObjSetXsim( Abc_ObjFanout0(pObj), Abc_ObjGetXsim(Abc_ObjFanin0(pObj)) );
    }
}

/**Function*************************************************************

  Synopsis    [Cycles the circuit to create a new initial state.]

  Description [Simulates the circuit with random (or ternary) input 
  for the given number of timeframes to get a better initial state.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCycleInitState( Abc_Ntk_t * pNtk, int nFrames, int fUseXval, int fVerbose )
{ 
    Abc_Obj_t * pObj;
    int i, f;
    assert( Abc_NtkIsStrash(pNtk) );
//    srand( 0x12341234 );
    Gia_ManRandom( 1 );
    // initialize the values
    Abc_ObjSetXsim( Abc_AigConst1(pNtk), XVS1 );
    Abc_NtkForEachLatch( pNtk, pObj, i )
        Abc_ObjSetXsim( Abc_ObjFanout0(pObj), Abc_LatchInit(pObj) );
    // simulate for the given number of timeframes
    for ( f = 0; f < nFrames; f++ )
    {
        Abc_NtkForEachPi( pNtk, pObj, i )
            Abc_ObjSetXsim( pObj, fUseXval? ABC_INIT_DC : Abc_XsimRand2() );
//            Abc_ObjSetXsim( pObj, ABC_INIT_ONE );
        Abc_AigForEachAnd( pNtk, pObj, i )
            Abc_ObjSetXsim( pObj, Abc_XsimAnd(Abc_ObjGetXsimFanin0(pObj), Abc_ObjGetXsimFanin1(pObj)) );
        Abc_NtkForEachCo( pNtk, pObj, i )
            Abc_ObjSetXsim( pObj, Abc_ObjGetXsimFanin0(pObj) );
        Abc_NtkForEachLatch( pNtk, pObj, i )
            Abc_ObjSetXsim( Abc_ObjFanout0(pObj), Abc_ObjGetXsim(Abc_ObjFanin0(pObj)) );
    }
    // set the final values
    Abc_NtkForEachLatch( pNtk, pObj, i )
    {
        pObj->pData = (void *)(ABC_PTRINT_T)Abc_ObjGetXsim(Abc_ObjFanout0(pObj));
//        printf( "%d", Abc_LatchIsInit1(pObj) );
    }
//    printf( "\n" );
}

///////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

