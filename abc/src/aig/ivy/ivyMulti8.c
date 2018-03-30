/**CFile****************************************************************

  FileName    [ivyMulti.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [Constructing multi-input AND/EXOR gates.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyMulti.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Ivy_Eval_t_ Ivy_Eval_t;
struct Ivy_Eval_t_
{
    unsigned Mask   :  5;  // the mask of covered nodes
    unsigned Weight :  3;  // the number of covered nodes
    unsigned Cost   :  4;  // the number of overlapping nodes
    unsigned Level  : 12;  // the level of this node
    unsigned Fan0   :  4;  // the first fanin
    unsigned Fan1   :  4;  // the second fanin
};

static Ivy_Obj_t * Ivy_MultiBuild_rec( Ivy_Eval_t * pEvals, int iNum, Ivy_Obj_t ** pArgs, int nArgs, Ivy_Type_t Type );
static void        Ivy_MultiSort( Ivy_Obj_t ** pArgs, int nArgs );
static int         Ivy_MultiPushUniqueOrderByLevel( Ivy_Obj_t ** pArray, int nArgs, Ivy_Obj_t * pNode );
static Ivy_Obj_t * Ivy_MultiEval( Ivy_Obj_t ** pArgs, int nArgs, Ivy_Type_t Type );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Constructs the well-balanced tree of gates.]

  Description [Disregards levels and possible logic sharing.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_Multi_rec( Ivy_Obj_t ** ppObjs, int nObjs, Ivy_Type_t Type )
{
    Ivy_Obj_t * pObj1, * pObj2;
    if ( nObjs == 1 )
        return ppObjs[0];
    pObj1 = Ivy_Multi_rec( ppObjs,           nObjs/2,         Type );
    pObj2 = Ivy_Multi_rec( ppObjs + nObjs/2, nObjs - nObjs/2, Type );
    return Ivy_Oper( pObj1, pObj2, Type );
}

/**Function*************************************************************

  Synopsis    [Constructs a balanced tree while taking sharing into account.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_Multi( Ivy_Obj_t ** pArgsInit, int nArgs, Ivy_Type_t Type )
{
    static char NumBits[32] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5};
    static Ivy_Eval_t pEvals[15+15*14/2];
    static Ivy_Obj_t * pArgs[16];
    Ivy_Eval_t * pEva, * pEvaBest;
    int nArgsNew, nEvals, i, k;
    Ivy_Obj_t * pTemp;

    // consider the case of one argument
    assert( nArgs > 0 );
    if ( nArgs == 1 )
        return pArgsInit[0];
    // consider the case of two arguments
    if ( nArgs == 2 )
        return Ivy_Oper( pArgsInit[0], pArgsInit[1], Type );

//Ivy_MultiEval( pArgsInit, nArgs, Type ); printf( "\n" );

    // set the initial ones
    for ( i = 0; i < nArgs; i++ )
    {
        pArgs[i] = pArgsInit[i];
        pEva = pEvals + i;
        pEva->Mask   = (1 << i);
        pEva->Weight = 1;
        pEva->Cost   = 0; 
        pEva->Level  = Ivy_Regular(pArgs[i])->Level; 
        pEva->Fan0   = 0; 
        pEva->Fan1   = 0; 
    }

    // find the available nodes
    pEvaBest = pEvals;
    nArgsNew = nArgs;
    for ( i = 1; i < nArgsNew; i++ )
    for ( k = 0; k < i; k++ )
        if ( pTemp = Ivy_TableLookup(Ivy_ObjCreateGhost(pArgs[k], pArgs[i], Type, IVY_INIT_NONE)) )
        {
            pEva = pEvals + nArgsNew;
            pEva->Mask   = pEvals[k].Mask | pEvals[i].Mask;
            pEva->Weight = NumBits[pEva->Mask];
            pEva->Cost   = pEvals[k].Cost + pEvals[i].Cost + NumBits[pEvals[k].Mask & pEvals[i].Mask]; 
            pEva->Level  = 1 + IVY_MAX(pEvals[k].Level, pEvals[i].Level); 
            pEva->Fan0   = k; 
            pEva->Fan1   = i; 
//            assert( pEva->Level == (unsigned)Ivy_ObjLevel(pTemp) );
            // compare
            if ( pEvaBest->Weight < pEva->Weight ||
                 pEvaBest->Weight == pEva->Weight && pEvaBest->Cost > pEva->Cost ||
                 pEvaBest->Weight == pEva->Weight && pEvaBest->Cost == pEva->Cost && pEvaBest->Level > pEva->Level )
                 pEvaBest = pEva;
            // save the argument
            pArgs[nArgsNew++] = pTemp;
            if ( nArgsNew == 15 )
                goto Outside;
        }
Outside:

//    printf( "Best = %d.\n", pEvaBest - pEvals );

    // the case of no common nodes
    if ( nArgsNew == nArgs )
    {
        Ivy_MultiSort( pArgs, nArgs );
        return Ivy_MultiBalance_rec( pArgs, nArgs, Type );
    }
    // the case of one common node
    if ( nArgsNew == nArgs + 1 )
    {
        assert( pEvaBest - pEvals == nArgs );
        k = 0;
        for ( i = 0; i < nArgs; i++ )
            if ( i != (int)pEvaBest->Fan0 && i != (int)pEvaBest->Fan1 )
                pArgs[k++] = pArgs[i];
        pArgs[k++] = pArgs[nArgs];
        assert( k == nArgs - 1 );
        nArgs = k;
        Ivy_MultiSort( pArgs, nArgs );
        return Ivy_MultiBalance_rec( pArgs, nArgs, Type );
    }
    // the case when there is a node that covers everything
    if ( (int)pEvaBest->Mask == ((1 << nArgs) - 1) )
        return Ivy_MultiBuild_rec( pEvals, pEvaBest - pEvals, pArgs, nArgsNew, Type ); 

    // evaluate node pairs
    nEvals = nArgsNew;
    for ( i = 1; i < nArgsNew; i++ )
    for ( k = 0; k < i; k++ )
    {
        pEva = pEvals + nEvals;
        pEva->Mask   = pEvals[k].Mask | pEvals[i].Mask;
        pEva->Weight = NumBits[pEva->Mask];
        pEva->Cost   = pEvals[k].Cost + pEvals[i].Cost + NumBits[pEvals[k].Mask & pEvals[i].Mask]; 
        pEva->Level  = 1 + IVY_MAX(pEvals[k].Level, pEvals[i].Level); 
        pEva->Fan0   = k; 
        pEva->Fan1   = i; 
        // compare
        if ( pEvaBest->Weight < pEva->Weight ||
             pEvaBest->Weight == pEva->Weight && pEvaBest->Cost > pEva->Cost ||
             pEvaBest->Weight == pEva->Weight && pEvaBest->Cost == pEva->Cost && pEvaBest->Level > pEva->Level )
             pEvaBest = pEva;
        // save the argument
        nEvals++;
    }
    assert( pEvaBest - pEvals >= nArgsNew );

//    printf( "Used (%d, %d).\n", pEvaBest->Fan0, pEvaBest->Fan1 );

    // get the best implementation
    pTemp = Ivy_MultiBuild_rec( pEvals, pEvaBest - pEvals, pArgs, nArgsNew, Type );

    // collect those not covered by EvaBest
    k = 0;
    for ( i = 0; i < nArgs; i++ )
        if ( (pEvaBest->Mask & (1 << i)) == 0 )
            pArgs[k++] = pArgs[i];
    pArgs[k++] = pTemp;
    assert( k == nArgs - (int)pEvaBest->Weight + 1 );
    nArgs = k;
    Ivy_MultiSort( pArgs, nArgs );
    return Ivy_MultiBalance_rec( pArgs, nArgs, Type );
}

/**Function*************************************************************

  Synopsis    [Implements multi-input AND/EXOR operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_MultiBuild_rec( Ivy_Eval_t * pEvals, int iNum, Ivy_Obj_t ** pArgs, int nArgs, Ivy_Type_t Type )
{
    Ivy_Obj_t * pNode0, * pNode1;
    if ( iNum < nArgs )
        return pArgs[iNum];
    pNode0 = Ivy_MultiBuild_rec( pEvals, pEvals[iNum].Fan0, pArgs, nArgs, Type );
    pNode1 = Ivy_MultiBuild_rec( pEvals, pEvals[iNum].Fan1, pArgs, nArgs, Type );
    return Ivy_Oper( pNode0, pNode1, Type );
}

/**Function*************************************************************

  Synopsis    [Selection-sorts the nodes in the decreasing over of level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_MultiSort( Ivy_Obj_t ** pArgs, int nArgs )
{
    Ivy_Obj_t * pTemp;
    int i, j, iBest;

    for ( i = 0; i < nArgs-1; i++ )
    {
        iBest = i;
        for ( j = i+1; j < nArgs; j++ )
            if ( Ivy_Regular(pArgs[j])->Level > Ivy_Regular(pArgs[iBest])->Level )
                iBest = j;
        pTemp = pArgs[i]; 
        pArgs[i] = pArgs[iBest]; 
        pArgs[iBest] = pTemp;
    }
}

/**Function*************************************************************

  Synopsis    [Inserts a new node in the order by levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_MultiPushUniqueOrderByLevel( Ivy_Obj_t ** pArray, int nArgs, Ivy_Obj_t * pNode )
{
    Ivy_Obj_t * pNode1, * pNode2;
    int i;
    // try to find the node in the array
    for ( i = 0; i < nArgs; i++ )
        if ( pArray[i] == pNode )
            return nArgs;
    // put the node last
    pArray[nArgs++] = pNode;
    // find the place to put the new node
    for ( i = nArgs-1; i > 0; i-- )
    {
        pNode1 = pArray[i  ];
        pNode2 = pArray[i-1];
        if ( Ivy_Regular(pNode1)->Level <= Ivy_Regular(pNode2)->Level )
            break;
        pArray[i  ] = pNode2;
        pArray[i-1] = pNode1;
    }
    return nArgs;
}

/**Function*************************************************************

  Synopsis    [Balances the array recursively.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_MultiBalance_rec( Ivy_Obj_t ** pArgs, int nArgs, Ivy_Type_t Type )
{
    Ivy_Obj_t * pNodeNew;
    // consider the case of one argument
    assert( nArgs > 0 );
    if ( nArgs == 1 )
        return pArgs[0];
    // consider the case of two arguments
    if ( nArgs == 2 )
        return Ivy_Oper( pArgs[0], pArgs[1], Type );
    // get the last two nodes
    pNodeNew = Ivy_Oper( pArgs[nArgs-1], pArgs[nArgs-2], Type );
    // add the new node
    nArgs = Ivy_MultiPushUniqueOrderByLevel( pArgs, nArgs - 2, pNodeNew );
    return Ivy_MultiBalance_rec( pArgs, nArgs, Type );
}

/**Function*************************************************************

  Synopsis    [Implements multi-input AND/EXOR operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_MultiEval( Ivy_Obj_t ** pArgs, int nArgs, Ivy_Type_t Type )
{
    Ivy_Obj_t * pTemp;
    int i, k;
    int nArgsOld = nArgs;
    for ( i = 0; i < nArgs; i++ )
        printf( "%d[%d] ", i, Ivy_Regular(pArgs[i])->Level );
    for ( i = 1; i < nArgs; i++ )
        for ( k = 0; k < i; k++ )
        {
            pTemp = Ivy_TableLookup(Ivy_ObjCreateGhost(pArgs[k], pArgs[i], Type, IVY_INIT_NONE));
            if ( pTemp != NULL )
            {
                printf( "%d[%d]=(%d,%d) ", nArgs, Ivy_Regular(pTemp)->Level, k, i );
                pArgs[nArgs++] = pTemp;
            }
        }
    printf( "     ((%d/%d))    ", nArgsOld, nArgs-nArgsOld );
    return NULL;
}



/**Function*************************************************************

  Synopsis    [Old code.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_Multi1( Ivy_Obj_t ** pArgs, int nArgs, Ivy_Type_t Type )
{
    Ivy_Obj_t * pArgsRef[5], * pTemp;
    int i, k, m, nArgsNew, Counter = 0;
 
    
//Ivy_MultiEval( pArgs, nArgs, Type );  printf( "\n" );


    assert( Type == IVY_AND || Type == IVY_EXOR );
    assert( nArgs > 0 );
    if ( nArgs == 1 )
        return pArgs[0];

    // find the nodes with more than one fanout
    nArgsNew = 0;
    for ( i = 0; i < nArgs; i++ )
        if ( Ivy_ObjRefs( Ivy_Regular(pArgs[i]) ) > 0 )
            pArgsRef[nArgsNew++] = pArgs[i];

    // go through pairs
    if ( nArgsNew >= 2 )
    for ( i = 0; i < nArgsNew; i++ )
    for ( k = i + 1; k < nArgsNew; k++ )
        if ( pTemp = Ivy_TableLookup(Ivy_ObjCreateGhost(pArgsRef[i], pArgsRef[k], Type, IVY_INIT_NONE)) )
            Counter++;
//    printf( "%d", Counter );
            
    // go through pairs
    if ( nArgsNew >= 2 )
    for ( i = 0; i < nArgsNew; i++ )
    for ( k = i + 1; k < nArgsNew; k++ )
        if ( pTemp = Ivy_TableLookup(Ivy_ObjCreateGhost(pArgsRef[i], pArgsRef[k], Type, IVY_INIT_NONE)) )
        {
            nArgsNew = 0;
            for ( m = 0; m < nArgs; m++ )
                if ( pArgs[m] != pArgsRef[i] && pArgs[m] != pArgsRef[k] )
                    pArgs[nArgsNew++] = pArgs[m];
            pArgs[nArgsNew++] = pTemp;
            assert( nArgsNew == nArgs - 1 );
            return Ivy_Multi1( pArgs, nArgsNew, Type );
        }
    return Ivy_Multi_rec( pArgs, nArgs, Type );
}

/**Function*************************************************************

  Synopsis    [Old code.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_Multi2( Ivy_Obj_t ** pArgs, int nArgs, Ivy_Type_t Type )
{
    assert( Type == IVY_AND || Type == IVY_EXOR );
    assert( nArgs > 0 );
    return Ivy_Multi_rec( pArgs, nArgs, Type );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

