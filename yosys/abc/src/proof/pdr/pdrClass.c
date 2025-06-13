/**CFile****************************************************************

  FileName    [pdrClass.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Property driven reachability.]

  Synopsis    [Equivalence classes of register outputs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 20, 2010.]

  Revision    [$Id: pdrClass.c,v 1.00 2010/11/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "pdrInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs duplication with the variable map.]

  Description [Var map contains -1 if const0 and <reg_num> otherwise.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Pdr_ManRehashWithMap( Aig_Man_t * pAig, Vec_Int_t * vMap )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj;
    int i, iReg;
    assert( Vec_IntSize(vMap) == Aig_ManRegNum(pAig) );
    // start the fraig package
    pFrames = Aig_ManStart( Aig_ManObjNumMax(pAig) );
    pFrames->pName = Abc_UtilStrsav( pAig->pName );
    pFrames->pSpec = Abc_UtilStrsav( pAig->pSpec );
    // create CI mapping
    Aig_ManCleanData( pAig );
    Aig_ManConst1(pAig)->pData = Aig_ManConst1(pFrames);
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi(pFrames);
    Saig_ManForEachLo( pAig, pObj, i )
    {
        iReg = Vec_IntEntry(vMap, i);
        if ( iReg == -1 )
            pObj->pData = Aig_ManConst0(pFrames);
        else
            pObj->pData = Saig_ManLo(pAig, iReg)->pData;
    }
    // add internal nodes of this frame
    Aig_ManForEachNode( pAig, pObj, i )
        pObj->pData = Aig_And( pFrames, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // add output nodes
    Aig_ManForEachCo( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCo( pFrames, Aig_ObjChild0Copy(pObj) );
    // finish off
    Aig_ManCleanup( pFrames );
    Aig_ManSetRegNum( pFrames, Aig_ManRegNum(pAig) );
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [Creates mapping of registers.]

  Description [Var map contains -1 if const0 and <reg_num> otherwise.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Pdr_ManCreateMap( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    Vec_Int_t * vMap;
    int * pLit2Id, Lit, i;
    pLit2Id = ABC_ALLOC( int, Aig_ManObjNumMax(p) * 2 );
    for ( i = 0; i < Aig_ManObjNumMax(p) * 2; i++ )
        pLit2Id[i] = -1;
    vMap = Vec_IntAlloc( Aig_ManRegNum(p) );
    Saig_ManForEachLi( p, pObj, i )
    {
        if ( Aig_ObjChild0(pObj) == Aig_ManConst0(p) )
        {
            Vec_IntPush( vMap, -1 );
            continue;
        }
        Lit = 2 * Aig_ObjFaninId0(pObj) + Aig_ObjFaninC0(pObj);
        if ( pLit2Id[Lit] < 0 ) // the first time
            pLit2Id[Lit] = i;
        Vec_IntPush( vMap, pLit2Id[Lit] );
    }
    ABC_FREE( pLit2Id );
    return vMap;
}

/**Function*************************************************************

  Synopsis    [Counts reduced registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pdr_ManCountMap( Vec_Int_t * vMap )
{
    int i, Entry, Counter = 0;
    Vec_IntForEachEntry( vMap, Entry, i )
        if ( Entry != i )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts reduced registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pdr_ManPrintMap( Vec_Int_t * vMap )
{
    Vec_Int_t * vMarks;
    int f, i, iClass, Entry, Counter = 0;
    Abc_Print( 1, "    Consts: " );
    Vec_IntForEachEntry( vMap, Entry, i )
        if ( Entry == -1 )
            Abc_Print( 1, "%d ", i );
    Abc_Print( 1, "\n" );
    vMarks = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vMap, iClass, f )
    {
        if ( iClass == -1 )
            continue;
        if ( iClass == f )
            continue;
        // check previous classes
        if ( Vec_IntFind( vMarks, iClass ) >= 0 )
            continue;
        Vec_IntPush( vMarks, iClass );
        // print class
        Abc_Print( 1, "    Class %d : ", iClass );
        Vec_IntForEachEntry( vMap, Entry, i )
            if ( Entry == iClass )
                Abc_Print( 1, "%d ", i );
        Abc_Print( 1, "\n" );
    }
    Vec_IntFree( vMarks );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pdr_ManEquivClasses( Aig_Man_t * pAig )
{
    Vec_Int_t * vMap;
    Aig_Man_t * pTemp;
    int f, nFrames = 100;
    assert( Saig_ManRegNum(pAig) > 0 );
    // start the map
    vMap = Vec_IntAlloc( 0 );
    Vec_IntFill( vMap, Aig_ManRegNum(pAig), -1 );
    // iterate and print changes
    for ( f = 0; f < nFrames; f++ )
    {
        // implement variable map
        pTemp = Pdr_ManRehashWithMap( pAig, vMap );
        // report the result
        Abc_Print( 1, "F =%4d : Total = %6d. Nodes = %6d. RedRegs = %6d. Prop = %s\n", 
            f+1, Aig_ManNodeNum(pAig), Aig_ManNodeNum(pTemp), Pdr_ManCountMap(vMap), 
            Aig_ObjChild0(Aig_ManCo(pTemp,0)) == Aig_ManConst0(pTemp) ? "proof" : "unknown" );
        // recreate the map
        Pdr_ManPrintMap( vMap );
        Vec_IntFree( vMap );
        vMap = Pdr_ManCreateMap( pTemp );
        Aig_ManStop( pTemp );
        if ( Pdr_ManCountMap(vMap) == 0 )
            break;
    }
    Vec_IntFree( vMap );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

