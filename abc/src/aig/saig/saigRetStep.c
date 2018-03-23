/**CFile****************************************************************

  FileName    [saigRetStep.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Implementation of retiming steps.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigRetStep.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs one retiming step forward.]

  Description [Returns the pointer to the register output after retiming.]
               
  SideEffects [Remember to run Aig_ManSetCioIds() in advance.]

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Saig_ManRetimeNodeFwd( Aig_Man_t * p, Aig_Obj_t * pObj, int fMakeBug )
{
    Aig_Obj_t * pFanin0, * pFanin1;
    Aig_Obj_t * pInput0, * pInput1;
    Aig_Obj_t * pObjNew, * pObjLi, * pObjLo;
    int fCompl;

    assert( Saig_ManRegNum(p) > 0 );
    assert( Aig_ObjIsNode(pObj) );

    // get the fanins
    pFanin0 = Aig_ObjFanin0(pObj);
    pFanin1 = Aig_ObjFanin1(pObj);
    // skip of they are not primary inputs
    if ( !Aig_ObjIsCi(pFanin0) || !Aig_ObjIsCi(pFanin1) )
        return NULL;

    // skip of they are not register outputs
    if ( !Saig_ObjIsLo(p, pFanin0) || !Saig_ObjIsLo(p, pFanin1) )
        return NULL;
    assert( Aig_ObjCioId(pFanin0) > 0 );
    assert( Aig_ObjCioId(pFanin1) > 0 );

    // skip latch guns
    if ( !Aig_ObjIsTravIdCurrent(p, pFanin0) && !Aig_ObjIsTravIdCurrent(p, pFanin1) )
        return NULL;

    // get the inputs of these registers
    pInput0 = Saig_ManLi( p, Aig_ObjCioId(pFanin0) - Saig_ManPiNum(p) );
    pInput1 = Saig_ManLi( p, Aig_ObjCioId(pFanin1) - Saig_ManPiNum(p) );
    pInput0 = Aig_ObjChild0( pInput0 );
    pInput1 = Aig_ObjChild0( pInput1 );
    pInput0 = Aig_NotCond( pInput0, Aig_ObjFaninC0(pObj) );
    pInput1 = Aig_NotCond( pInput1, Aig_ObjFaninC1(pObj) );
    // get the condition when the register should be complemetned
    fCompl = Aig_ObjFaninC0(pObj) && Aig_ObjFaninC1(pObj);

    if ( fMakeBug )
    {
        printf( "Introducing bug during retiming.\n" );
        pInput1 = Aig_Not( pInput1 );
    }

    // create new node
    pObjNew = Aig_And( p, pInput0, pInput1 );

    // create new register input
    pObjLi = Aig_ObjCreateCo( p, Aig_NotCond(pObjNew, fCompl) );
    pObjLi->CioId = Aig_ManCoNum(p) - 1;

    // create new register output
    pObjLo = Aig_ObjCreateCi( p );
    pObjLo->CioId = Aig_ManCiNum(p) - 1;
    p->nRegs++;

    // make sure the register is retimable.
    Aig_ObjSetTravIdCurrent(p, pObjLo);

//printf( "Reg = %4d. Reg = %4d. Compl = %d. Phase = %d.\n", 
//       pFanin0->PioNum, pFanin1->PioNum, Aig_IsComplement(pObjNew), fCompl );

    // return register output
    return Aig_NotCond( pObjLo, fCompl );
}

/**Function*************************************************************

  Synopsis    [Performs one retiming step backward.]

  Description [Returns the pointer to node after retiming.]
               
  SideEffects [Remember to run Aig_ManSetCioIds() in advance.]

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Saig_ManRetimeNodeBwd( Aig_Man_t * p, Aig_Obj_t * pObjLo )
{
    Aig_Obj_t * pFanin0, * pFanin1;
    Aig_Obj_t * pLo0New, * pLo1New;
    Aig_Obj_t * pLi0New, * pLi1New;
    Aig_Obj_t * pObj, * pObjNew, * pObjLi;
    int fCompl0, fCompl1;

    assert( Saig_ManRegNum(p) > 0 );
    assert( Aig_ObjCioId(pObjLo) > 0 );
    assert( Saig_ObjIsLo(p, pObjLo) );

    // get the corresponding latch input
    pObjLi = Saig_ManLi( p, Aig_ObjCioId(pObjLo) - Saig_ManPiNum(p) );

    // get the node
    pObj = Aig_ObjFanin0(pObjLi);
    if ( !Aig_ObjIsNode(pObj) )
        return NULL;

    // get the fanins
    pFanin0 = Aig_ObjFanin0(pObj);
    pFanin1 = Aig_ObjFanin1(pObj);

    // get the complemented attributes of the fanins
    fCompl0 = Aig_ObjFaninC0(pObj) ^ Aig_ObjFaninC0(pObjLi);
    fCompl1 = Aig_ObjFaninC1(pObj) ^ Aig_ObjFaninC0(pObjLi);

    // create latch inputs
    pLi0New = Aig_ObjCreateCo( p, Aig_NotCond(pFanin0, fCompl0) );
    pLi0New->CioId = Aig_ManCoNum(p) - 1;
    pLi1New = Aig_ObjCreateCo( p, Aig_NotCond(pFanin1, fCompl1) );
    pLi1New->CioId = Aig_ManCoNum(p) - 1;

    // create latch outputs
    pLo0New = Aig_ObjCreateCi(p);
    pLo0New->CioId = Aig_ManCiNum(p) - 1;
    pLo1New = Aig_ObjCreateCi(p);
    pLo1New->CioId = Aig_ManCiNum(p) - 1;
    pLo0New = Aig_NotCond( pLo0New, fCompl0 );
    pLo1New = Aig_NotCond( pLo1New, fCompl1 );
    p->nRegs += 2;

    // create node
    pObjNew = Aig_And( p, pLo0New, pLo1New );
//    assert( pObjNew->fPhase == 0 );
    return pObjNew;
}

/**Function*************************************************************

  Synopsis    [Performs the given number of retiming steps.]

  Description [Returns the pointer to node after retiming.]
               
  SideEffects [Remember to run Aig_ManSetCioIds() in advance.]

  SeeAlso     []

***********************************************************************/
int Saig_ManRetimeSteps( Aig_Man_t * p, int nSteps, int fForward, int fAddBugs )
{
    Aig_Obj_t * pObj, * pObjNew;
    int RetValue, s, i;
    Aig_ManSetCioIds( p );
    Aig_ManFanoutStart( p );
    p->fCreatePios = 1;
    if ( fForward )
    {
        Saig_ManMarkAutonomous( p );
        for ( s = 0; s < nSteps; s++ )
        {
            Aig_ManForEachNode( p, pObj, i )
            {
                pObjNew = Saig_ManRetimeNodeFwd( p, pObj, fAddBugs && (s == 10) );
//                pObjNew = Saig_ManRetimeNodeFwd( p, pObj, 0 );
                if ( pObjNew == NULL )
                    continue;
                Aig_ObjReplace( p, pObj, pObjNew, 0 );
                break;
            }
            if ( i == Vec_PtrSize(p->vObjs) )
                break;
        }
    }
    else
    {
        for ( s = 0; s < nSteps; s++ )
        {
            Saig_ManForEachLo( p, pObj, i )
            {
                pObjNew = Saig_ManRetimeNodeBwd( p, pObj );
                if ( pObjNew == NULL )
                    continue;
                Aig_ObjReplace( p, pObj, pObjNew, 0 );
                break;
            }
            if ( i == Vec_PtrSize(p->vObjs) )
                break;
        }
    }
    p->fCreatePios = 0;
    Aig_ManFanoutStop( p );
    RetValue = Aig_ManCleanup( p );
    assert( RetValue == 0 );
    Aig_ManSetRegNum( p, p->nRegs ); 
    return s;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

