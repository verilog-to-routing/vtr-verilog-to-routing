/**CFile****************************************************************

  FileName    [aigUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Various procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigUtil.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Increments the current traversal ID of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManIncrementTravId( Aig_Man_t * p )
{
    if ( p->nTravIds >= (1<<30)-1 )
        Aig_ManCleanData( p );
    p->nTravIds++;
}

/**Function*************************************************************

  Synopsis    [Returns the time stamp.]

  Description [The file should be closed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Aig_TimeStamp()
{
    static char Buffer[100];
    char * TimeStamp;
    time_t ltime;
    // get the current time
    time( &ltime );
    TimeStamp = asctime( localtime( &ltime ) );
    TimeStamp[ strlen(TimeStamp) - 1 ] = 0;
    strcpy( Buffer, TimeStamp );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Make sure AIG has not gaps in the numeric order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManHasNoGaps( Aig_Man_t * p )
{
    return (int)(Aig_ManObjNum(p) == Aig_ManCiNum(p) + Aig_ManCoNum(p) + Aig_ManNodeNum(p) + 1);
}

/**Function*************************************************************

  Synopsis    [Collect the latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManLevels( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, LevelMax = 0;
    Aig_ManForEachCo( p, pObj, i )
        LevelMax = Abc_MaxInt( LevelMax, (int)Aig_ObjFanin0(pObj)->Level );
    return LevelMax;
}

/**Function*************************************************************

  Synopsis    [Reset reference counters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManResetRefs( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( p, pObj, i )
        pObj->nRefs = 0;
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( Aig_ObjFanin0(pObj) )
            Aig_ObjFanin0(pObj)->nRefs++;
        if ( Aig_ObjFanin1(pObj) )
            Aig_ObjFanin1(pObj)->nRefs++;
    }
}

/**Function*************************************************************

  Synopsis    [Cleans fMarkA.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCleanMarkA( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( p, pObj, i )
        pObj->fMarkA = 0;
}

/**Function*************************************************************

  Synopsis    [Cleans fMarkB.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCleanMarkB( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( p, pObj, i )
        pObj->fMarkB = 0;
}

/**Function*************************************************************

  Synopsis    [Cleans fMarkB.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCleanMarkAB( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( p, pObj, i )
        pObj->fMarkA = pObj->fMarkB = 0;
}

/**Function*************************************************************

  Synopsis    [Cleans the data pointers for the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCleanData( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( p, pObj, i )
        pObj->pData = NULL;
}

/**Function*************************************************************

  Synopsis    [Cleans the data pointers for the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCleanNext( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( p, pObj, i )
        pObj->pNext = NULL;
}

/**Function*************************************************************

  Synopsis    [Recursively cleans the data pointers in the cone of the node.]

  Description [Applicable to small AIGs only because no caching is performed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjCleanData_rec( Aig_Obj_t * pObj )
{
    assert( !Aig_IsComplement(pObj) );
    assert( !Aig_ObjIsCo(pObj) );
    if ( Aig_ObjIsAnd(pObj) )
    {
        Aig_ObjCleanData_rec( Aig_ObjFanin0(pObj) );
        Aig_ObjCleanData_rec( Aig_ObjFanin1(pObj) );
    }
    pObj->pData = NULL;
}


/**Function*************************************************************

  Synopsis    [Detects multi-input gate rooted at this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjCollectMulti_rec( Aig_Obj_t * pRoot, Aig_Obj_t * pObj, Vec_Ptr_t * vSuper )
{
    if ( pRoot != pObj && (Aig_IsComplement(pObj) || Aig_ObjIsCi(pObj) || Aig_ObjType(pRoot) != Aig_ObjType(pObj)) )
    {
        Vec_PtrPushUnique(vSuper, pObj);
        return;
    }
    Aig_ObjCollectMulti_rec( pRoot, Aig_ObjChild0(pObj), vSuper );
    Aig_ObjCollectMulti_rec( pRoot, Aig_ObjChild1(pObj), vSuper );
}

/**Function*************************************************************

  Synopsis    [Detects multi-input gate rooted at this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjCollectMulti( Aig_Obj_t * pRoot, Vec_Ptr_t * vSuper )
{
    assert( !Aig_IsComplement(pRoot) );
    Vec_PtrClear( vSuper );
    Aig_ObjCollectMulti_rec( pRoot, pRoot, vSuper );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is the root of MUX or EXOR/NEXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjIsMuxType( Aig_Obj_t * pNode )
{
    Aig_Obj_t * pNode0, * pNode1;
    // check that the node is regular
    assert( !Aig_IsComplement(pNode) );
    // if the node is not AND, this is not MUX
    if ( !Aig_ObjIsAnd(pNode) )
        return 0;
    // if the children are not complemented, this is not MUX
    if ( !Aig_ObjFaninC0(pNode) || !Aig_ObjFaninC1(pNode) )
        return 0;
    // get children
    pNode0 = Aig_ObjFanin0(pNode);
    pNode1 = Aig_ObjFanin1(pNode);
    // if the children are not ANDs, this is not MUX
    if ( !Aig_ObjIsAnd(pNode0) || !Aig_ObjIsAnd(pNode1) )
        return 0;
    // otherwise the node is MUX iff it has a pair of equal grandchildren
    return (Aig_ObjFanin0(pNode0) == Aig_ObjFanin0(pNode1) && (Aig_ObjFaninC0(pNode0) ^ Aig_ObjFaninC0(pNode1))) || 
           (Aig_ObjFanin0(pNode0) == Aig_ObjFanin1(pNode1) && (Aig_ObjFaninC0(pNode0) ^ Aig_ObjFaninC1(pNode1))) ||
           (Aig_ObjFanin1(pNode0) == Aig_ObjFanin0(pNode1) && (Aig_ObjFaninC1(pNode0) ^ Aig_ObjFaninC0(pNode1))) ||
           (Aig_ObjFanin1(pNode0) == Aig_ObjFanin1(pNode1) && (Aig_ObjFaninC1(pNode0) ^ Aig_ObjFaninC1(pNode1)));
}


/**Function*************************************************************

  Synopsis    [Recognizes what nodes are inputs of the EXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjRecognizeExor( Aig_Obj_t * pObj, Aig_Obj_t ** ppFan0, Aig_Obj_t ** ppFan1 )
{
    Aig_Obj_t * p0, * p1;
    assert( !Aig_IsComplement(pObj) );
    if ( !Aig_ObjIsNode(pObj) )
        return 0;
    if ( Aig_ObjIsExor(pObj) )
    {
        *ppFan0 = Aig_ObjChild0(pObj);
        *ppFan1 = Aig_ObjChild1(pObj);
        return 1;
    }
    assert( Aig_ObjIsAnd(pObj) );
    p0 = Aig_ObjChild0(pObj);
    p1 = Aig_ObjChild1(pObj);
    if ( !Aig_IsComplement(p0) || !Aig_IsComplement(p1) )
        return 0;
    p0 = Aig_Regular(p0);
    p1 = Aig_Regular(p1);
    if ( !Aig_ObjIsAnd(p0) || !Aig_ObjIsAnd(p1) )
        return 0;
    if ( Aig_ObjFanin0(p0) != Aig_ObjFanin0(p1) || Aig_ObjFanin1(p0) != Aig_ObjFanin1(p1) )
        return 0;
    if ( Aig_ObjFaninC0(p0) == Aig_ObjFaninC0(p1) || Aig_ObjFaninC1(p0) == Aig_ObjFaninC1(p1) )
        return 0;
    *ppFan0 = Aig_ObjChild0(p0);
    *ppFan1 = Aig_ObjChild1(p0);
    return 1;
}

/**Function*************************************************************

  Synopsis    [Recognizes what nodes are control and data inputs of a MUX.]

  Description [If the node is a MUX, returns the control variable C.
  Assigns nodes T and E to be the then and else variables of the MUX. 
  Node C is never complemented. Nodes T and E can be complemented.
  This function also recognizes EXOR/NEXOR gates as MUXes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_ObjRecognizeMux( Aig_Obj_t * pNode, Aig_Obj_t ** ppNodeT, Aig_Obj_t ** ppNodeE )
{
    Aig_Obj_t * pNode0, * pNode1;
    assert( !Aig_IsComplement(pNode) );
    assert( Aig_ObjIsMuxType(pNode) );
    // get children
    pNode0 = Aig_ObjFanin0(pNode);
    pNode1 = Aig_ObjFanin1(pNode);

    // find the control variable
    if ( Aig_ObjFanin1(pNode0) == Aig_ObjFanin1(pNode1) && (Aig_ObjFaninC1(pNode0) ^ Aig_ObjFaninC1(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p2) )
        if ( Aig_ObjFaninC1(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Aig_Not(Aig_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Aig_Not(Aig_ObjChild0(pNode0));//pNode1->p1);
            return Aig_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Aig_Not(Aig_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Aig_Not(Aig_ObjChild0(pNode1));//pNode2->p1);
            return Aig_ObjChild1(pNode0);//pNode1->p2;
        }
    }
    else if ( Aig_ObjFanin0(pNode0) == Aig_ObjFanin0(pNode1) && (Aig_ObjFaninC0(pNode0) ^ Aig_ObjFaninC0(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p1) )
        if ( Aig_ObjFaninC0(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Aig_Not(Aig_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Aig_Not(Aig_ObjChild1(pNode0));//pNode1->p2);
            return Aig_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Aig_Not(Aig_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Aig_Not(Aig_ObjChild1(pNode1));//pNode2->p2);
            return Aig_ObjChild0(pNode0);//pNode1->p1;
        }
    }
    else if ( Aig_ObjFanin0(pNode0) == Aig_ObjFanin1(pNode1) && (Aig_ObjFaninC0(pNode0) ^ Aig_ObjFaninC1(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p1) )
        if ( Aig_ObjFaninC0(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Aig_Not(Aig_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Aig_Not(Aig_ObjChild1(pNode0));//pNode1->p2);
            return Aig_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Aig_Not(Aig_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Aig_Not(Aig_ObjChild0(pNode1));//pNode2->p1);
            return Aig_ObjChild0(pNode0);//pNode1->p1;
        }
    }
    else if ( Aig_ObjFanin1(pNode0) == Aig_ObjFanin0(pNode1) && (Aig_ObjFaninC1(pNode0) ^ Aig_ObjFaninC0(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p2) )
        if ( Aig_ObjFaninC1(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Aig_Not(Aig_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Aig_Not(Aig_ObjChild0(pNode0));//pNode1->p1);
            return Aig_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Aig_Not(Aig_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Aig_Not(Aig_ObjChild1(pNode1));//pNode2->p2);
            return Aig_ObjChild1(pNode0);//pNode1->p2;
        }
    }
    assert( 0 ); // this is not MUX
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_ObjReal_rec( Aig_Obj_t * pObj )
{
    Aig_Obj_t * pObjNew, * pObjR = Aig_Regular(pObj);
    if ( !Aig_ObjIsBuf(pObjR) )
        return pObj;
    pObjNew = Aig_ObjReal_rec( Aig_ObjChild0(pObjR) );
    return Aig_NotCond( pObjNew, Aig_IsComplement(pObj) );
}

/**Function*************************************************************

  Synopsis    [Procedure used for sorting the nodes in increasing order of IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjCompareIdIncrease( Aig_Obj_t ** pp1, Aig_Obj_t ** pp2 )
{
    int Diff = Aig_ObjId(*pp1) - Aig_ObjId(*pp2);
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    return 0; 
}


/**Function*************************************************************

  Synopsis    [Prints Eqn formula for the AIG rooted at this node.]

  Description [The formula is in terms of PIs, which should have
  their names assigned in pObj->pData fields.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjPrintEqn( FILE * pFile, Aig_Obj_t * pObj, Vec_Vec_t * vLevels, int Level )
{
    Vec_Ptr_t * vSuper;
    Aig_Obj_t * pFanin;
    int fCompl, i;
    // store the complemented attribute
    fCompl = Aig_IsComplement(pObj);
    pObj = Aig_Regular(pObj);
    // constant case
    if ( Aig_ObjIsConst1(pObj) )
    {
        fprintf( pFile, "%d", !fCompl );
        return;
    }
    // PI case
    if ( Aig_ObjIsCi(pObj) )
    {
        fprintf( pFile, "%s%s", fCompl? "!" : "", (char*)pObj->pData );
        return;
    }
    // AND case
    Vec_VecExpand( vLevels, Level );
    vSuper = Vec_VecEntry(vLevels, Level);
    Aig_ObjCollectMulti( pObj, vSuper );
    fprintf( pFile, "%s", (Level==0? "" : "(") );
    Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pFanin, i )
    {
        Aig_ObjPrintEqn( pFile, Aig_NotCond(pFanin, fCompl), vLevels, Level+1 );
        if ( i < Vec_PtrSize(vSuper) - 1 )
            fprintf( pFile, " %s ", fCompl? "+" : "*" );
    }
    fprintf( pFile, "%s", (Level==0? "" : ")") );
    return;
}

/**Function*************************************************************

  Synopsis    [Prints Verilog formula for the AIG rooted at this node.]

  Description [The formula is in terms of PIs, which should have
  their names assigned in pObj->pData fields.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjPrintVerilog( FILE * pFile, Aig_Obj_t * pObj, Vec_Vec_t * vLevels, int Level )
{
    Vec_Ptr_t * vSuper;
    Aig_Obj_t * pFanin, * pFanin0, * pFanin1, * pFaninC;
    int fCompl, i;
    // store the complemented attribute
    fCompl = Aig_IsComplement(pObj);
    pObj = Aig_Regular(pObj);
    // constant case
    if ( Aig_ObjIsConst1(pObj) )
    {
        fprintf( pFile, "1\'b%d", !fCompl );
        return;
    }
    // PI case
    if ( Aig_ObjIsCi(pObj) )
    {
        fprintf( pFile, "%s%s", fCompl? "~" : "", (char*)pObj->pData );
        return;
    }
    // EXOR case
    if ( Aig_ObjIsExor(pObj) )
    {
        Vec_VecExpand( vLevels, Level );
        vSuper = Vec_VecEntry( vLevels, Level );
        Aig_ObjCollectMulti( pObj, vSuper );
        fprintf( pFile, "%s", (Level==0? "" : "(") );
        Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pFanin, i )
        {
            Aig_ObjPrintVerilog( pFile, Aig_NotCond(pFanin, (fCompl && i==0)), vLevels, Level+1 );
            if ( i < Vec_PtrSize(vSuper) - 1 )
                fprintf( pFile, " ^ " );
        }
        fprintf( pFile, "%s", (Level==0? "" : ")") );
        return;
    }
    // MUX case
    if ( Aig_ObjIsMuxType(pObj) )
    {
        if ( Aig_ObjRecognizeExor( pObj, &pFanin0, &pFanin1 ) )
        {
            fprintf( pFile, "%s", (Level==0? "" : "(") );
            Aig_ObjPrintVerilog( pFile, Aig_NotCond(pFanin0, fCompl), vLevels, Level+1 );
            fprintf( pFile, " ^ " );
            Aig_ObjPrintVerilog( pFile, pFanin1, vLevels, Level+1 );
            fprintf( pFile, "%s", (Level==0? "" : ")") );
        }
        else 
        {
            pFaninC = Aig_ObjRecognizeMux( pObj, &pFanin1, &pFanin0 );
            fprintf( pFile, "%s", (Level==0? "" : "(") );
            Aig_ObjPrintVerilog( pFile, pFaninC, vLevels, Level+1 );
            fprintf( pFile, " ? " );
            Aig_ObjPrintVerilog( pFile, Aig_NotCond(pFanin1, fCompl), vLevels, Level+1 );
            fprintf( pFile, " : " );
            Aig_ObjPrintVerilog( pFile, Aig_NotCond(pFanin0, fCompl), vLevels, Level+1 );
            fprintf( pFile, "%s", (Level==0? "" : ")") );
        }
        return;
    }
    // AND case
    Vec_VecExpand( vLevels, Level );
    vSuper = Vec_VecEntry(vLevels, Level);
    Aig_ObjCollectMulti( pObj, vSuper );
    fprintf( pFile, "%s", (Level==0? "" : "(") );
    Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pFanin, i )
    {
        Aig_ObjPrintVerilog( pFile, Aig_NotCond(pFanin, fCompl), vLevels, Level+1 );
        if ( i < Vec_PtrSize(vSuper) - 1 )
            fprintf( pFile, " %s ", fCompl? "|" : "&" );
    }
    fprintf( pFile, "%s", (Level==0? "" : ")") );
    return;
}


/**Function*************************************************************

  Synopsis    [Prints node in HAIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjPrintVerbose( Aig_Obj_t * pObj, int fHaig )
{
    assert( !Aig_IsComplement(pObj) );
    printf( "Node %d : ", pObj->Id );
    if ( Aig_ObjIsConst1(pObj) )
        printf( "constant 1" );
    else if ( Aig_ObjIsCi(pObj) )
        printf( "CI" );
    else if ( Aig_ObjIsCo(pObj) )
    {
        printf( "CO( " );
        printf( "%d%s )", 
            Aig_ObjFanin0(pObj)->Id, (Aig_ObjFaninC0(pObj)? "\'" : " ") );
    }
    else
        printf( "AND( %d%s, %d%s )", 
            Aig_ObjFanin0(pObj)->Id, (Aig_ObjFaninC0(pObj)? "\'" : " "), 
            Aig_ObjFanin1(pObj)->Id, (Aig_ObjFaninC1(pObj)? "\'" : " ") );
    printf( " (refs = %3d)", Aig_ObjRefs(pObj) );
}
void Aig_ObjPrintVerboseCone( Aig_Man_t * p, Aig_Obj_t * pRoot, int fHaig )
{
    extern Vec_Ptr_t * Aig_ManDfsArray( Aig_Man_t * p, Aig_Obj_t ** pNodes, int nNodes );
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    vNodes = Aig_ManDfsArray( p, &pRoot, 1 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        Aig_ObjPrintVerbose( pObj, fHaig ), printf( "\n" );
    printf( "\n" );
    Vec_PtrFree( vNodes );

}

/**Function*************************************************************

  Synopsis    [Prints node in HAIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPrintVerbose( Aig_Man_t * p, int fHaig )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    printf( "PIs: " );
    Aig_ManForEachCi( p, pObj, i )
        printf( " %p", pObj );
    printf( "\n" );
    vNodes = Aig_ManDfs( p, 0 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        Aig_ObjPrintVerbose( pObj, fHaig ), printf( "\n" );
    printf( "\n" );
    Vec_PtrFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Write speculative miter for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDump( Aig_Man_t * p )
{ 
    static int Counter = 0;
    char FileName[200];
    // dump the logic into a file
    sprintf( FileName, "aigbug\\%03d.blif", ++Counter );
    Aig_ManDumpBlif( p, FileName, NULL, NULL );
    printf( "Intermediate AIG with %d nodes was written into file \"%s\".\n", Aig_ManNodeNum(p), FileName );
}

/**Function*************************************************************

  Synopsis    [Writes the AIG into a BLIF file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDumpBlif( Aig_Man_t * p, char * pFileName, Vec_Ptr_t * vPiNames, Vec_Ptr_t * vPoNames )
{
    FILE * pFile;
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo, * pConst1 = NULL;
    int i, nDigits, Counter = 0;
    if ( Aig_ManCoNum(p) == 0 )
    {
        printf( "Aig_ManDumpBlif(): AIG manager does not have POs.\n" );
        return;
    }
    // check if constant is used
    Aig_ManForEachCo( p, pObj, i )
        if ( Aig_ObjIsConst1(Aig_ObjFanin0(pObj)) )
            pConst1 = Aig_ManConst1(p);
    // collect nodes in the DFS order
    vNodes = Aig_ManDfs( p, 1 );
    // assign IDs to objects
    Aig_ManConst1(p)->iData = Counter++;
    Aig_ManForEachCi( p, pObj, i )
        pObj->iData = Counter++;
    Aig_ManForEachCo( p, pObj, i )
        pObj->iData = Counter++;
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        pObj->iData = Counter++;
    nDigits = Abc_Base10Log( Counter );
    // write the file 
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# BLIF file written by procedure Aig_ManDumpBlif()\n" );
//    fprintf( pFile, "# http://www.eecs.berkeley.edu/~alanmi/abc/\n" );
    fprintf( pFile, ".model %s\n", p->pName );
    // write PIs
    fprintf( pFile, ".inputs" );
    Aig_ManForEachPiSeq( p, pObj, i )
        if ( vPiNames )
            fprintf( pFile, " %s", (char*)Vec_PtrEntry(vPiNames, i) );
        else
            fprintf( pFile, " n%0*d", nDigits, pObj->iData );
    fprintf( pFile, "\n" );
    // write POs
    fprintf( pFile, ".outputs" );
    Aig_ManForEachPoSeq( p, pObj, i )
        if ( vPoNames )
            fprintf( pFile, " %s", (char*)Vec_PtrEntry(vPoNames, i) );
        else
            fprintf( pFile, " n%0*d", nDigits, pObj->iData );
    fprintf( pFile, "\n" );
    // write latches
    if ( Aig_ManRegNum(p) )
    {
        fprintf( pFile, "\n" );
        Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
        {
            fprintf( pFile, ".latch" );
            if ( vPoNames )
                fprintf( pFile, " %s", (char*)Vec_PtrEntry(vPoNames, Aig_ManCoNum(p)-Aig_ManRegNum(p)+i) );
            else
                fprintf( pFile, " n%0*d", nDigits, pObjLi->iData );
            if ( vPiNames )
                fprintf( pFile, " %s", (char*)Vec_PtrEntry(vPiNames, Aig_ManCiNum(p)-Aig_ManRegNum(p)+i) );
            else
                fprintf( pFile, " n%0*d", nDigits, pObjLo->iData );
            fprintf( pFile, " 0\n" );
        }
        fprintf( pFile, "\n" );
    } 
    // write nodes
    if ( pConst1 )
        fprintf( pFile, ".names n%0*d\n 1\n", nDigits, pConst1->iData );
    Aig_ManSetCioIds( p );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        fprintf( pFile, ".names" );
        if ( vPiNames && Aig_ObjIsCi(Aig_ObjFanin0(pObj)) )
            fprintf( pFile, " %s", (char*)Vec_PtrEntry(vPiNames, Aig_ObjCioId(Aig_ObjFanin0(pObj))) );
        else
            fprintf( pFile, " n%0*d", nDigits, Aig_ObjFanin0(pObj)->iData );
        if ( vPiNames && Aig_ObjIsCi(Aig_ObjFanin1(pObj)) )
            fprintf( pFile, " %s", (char*)Vec_PtrEntry(vPiNames, Aig_ObjCioId(Aig_ObjFanin1(pObj))) );
        else
            fprintf( pFile, " n%0*d", nDigits, Aig_ObjFanin1(pObj)->iData );
        fprintf( pFile, " n%0*d\n", nDigits, pObj->iData );
        fprintf( pFile, "%d%d 1\n", !Aig_ObjFaninC0(pObj), !Aig_ObjFaninC1(pObj) );
    }
    // write POs
    Aig_ManForEachCo( p, pObj, i )
    {
        fprintf( pFile, ".names" );
        if ( vPiNames && Aig_ObjIsCi(Aig_ObjFanin0(pObj)) )
            fprintf( pFile, " %s", (char*)Vec_PtrEntry(vPiNames, Aig_ObjCioId(Aig_ObjFanin0(pObj))) );
        else
            fprintf( pFile, " n%0*d", nDigits, Aig_ObjFanin0(pObj)->iData );
        if ( vPoNames )
            fprintf( pFile, " %s\n", (char*)Vec_PtrEntry(vPoNames, Aig_ObjCioId(pObj)) );
        else
            fprintf( pFile, " n%0*d\n", nDigits, pObj->iData );
        fprintf( pFile, "%d 1\n", !Aig_ObjFaninC0(pObj) );
    }
    Aig_ManCleanCioIds( p );
    fprintf( pFile, ".end\n\n" );
    fclose( pFile );
    Vec_PtrFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Writes the AIG into a Verilog file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDumpVerilog( Aig_Man_t * p, char * pFileName )
{
    FILE * pFile;
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo, * pConst1 = NULL;
    int i, nDigits, Counter = 0;
    if ( Aig_ManCoNum(p) == 0 )
    {
        printf( "Aig_ManDumpBlif(): AIG manager does not have POs.\n" );
        return;
    }
    // check if constant is used
    Aig_ManForEachCo( p, pObj, i )
        if ( Aig_ObjIsConst1(Aig_ObjFanin0(pObj)) )
            pConst1 = Aig_ManConst1(p);
    // collect nodes in the DFS order
    vNodes = Aig_ManDfs( p, 1 );
    // assign IDs to objects
    Aig_ManConst1(p)->iData = Counter++;
    Aig_ManForEachCi( p, pObj, i )
        pObj->iData = Counter++;
    Aig_ManForEachCo( p, pObj, i )
        pObj->iData = Counter++;
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        pObj->iData = Counter++;
    nDigits = Abc_Base10Log( Counter );
    // write the file
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "// Verilog file written by procedure Aig_ManDumpVerilog()\n" );
//    fprintf( pFile, "// http://www.eecs.berkeley.edu/~alanmi/abc/\n" );
    if ( Aig_ManRegNum(p) )
        fprintf( pFile, "module %s ( clock", p->pName? p->pName: "test" );
    else
        fprintf( pFile, "module %s (", p->pName? p->pName: "test" );
    Aig_ManForEachPiSeq( p, pObj, i )
        fprintf( pFile, "%s n%0*d", ((Aig_ManRegNum(p) || i)? ",":""), nDigits, pObj->iData );
    Aig_ManForEachPoSeq( p, pObj, i )
        fprintf( pFile, ", n%0*d", nDigits, pObj->iData );
    fprintf( pFile, " );\n" );

    // write PIs
    if ( Aig_ManRegNum(p) )
        fprintf( pFile, "input clock;\n" );
    Aig_ManForEachPiSeq( p, pObj, i )
        fprintf( pFile, "input n%0*d;\n", nDigits, pObj->iData );
    // write POs
    Aig_ManForEachPoSeq( p, pObj, i )
        fprintf( pFile, "output n%0*d;\n", nDigits, pObj->iData );
    // write latches
    if ( Aig_ManRegNum(p) )
    {
    Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
        fprintf( pFile, "reg n%0*d;\n", nDigits, pObjLo->iData );
    Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
        fprintf( pFile, "wire n%0*d;\n", nDigits, pObjLi->iData );
    }
    // write nodes
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        fprintf( pFile, "wire n%0*d;\n", nDigits, pObj->iData );
    if ( pConst1 )
        fprintf( pFile, "wire n%0*d;\n", nDigits, pConst1->iData );
    // write nodes
    if ( pConst1 )
        fprintf( pFile, "assign n%0*d = 1\'b1;\n", nDigits, pConst1->iData );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        fprintf( pFile, "assign n%0*d = %sn%0*d & %sn%0*d;\n", 
            nDigits, pObj->iData,
            !Aig_ObjFaninC0(pObj) ? " " : "~", nDigits, Aig_ObjFanin0(pObj)->iData, 
            !Aig_ObjFaninC1(pObj) ? " " : "~", nDigits, Aig_ObjFanin1(pObj)->iData
            );
    }
    // write POs
    Aig_ManForEachPoSeq( p, pObj, i )
    {
        fprintf( pFile, "assign n%0*d = %sn%0*d;\n", 
            nDigits, pObj->iData,
            !Aig_ObjFaninC0(pObj) ? " " : "~", nDigits, Aig_ObjFanin0(pObj)->iData );
    }
    if ( Aig_ManRegNum(p) )
    {
        Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
        {
            fprintf( pFile, "assign n%0*d = %sn%0*d;\n", 
                nDigits, pObjLi->iData,
                !Aig_ObjFaninC0(pObjLi) ? " " : "~", nDigits, Aig_ObjFanin0(pObjLi)->iData );
        }
    }

    // write initial state
    if ( Aig_ManRegNum(p) )
    {
        Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
            fprintf( pFile, "always @ (posedge clock) begin n%0*d <= n%0*d; end\n", 
                 nDigits, pObjLo->iData,
                 nDigits, pObjLi->iData );
        Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
            fprintf( pFile, "initial begin n%0*d <= 1\'b0; end\n", 
                 nDigits, pObjLo->iData );
    }

    fprintf( pFile, "endmodule\n\n" );
    fclose( pFile );
    Vec_PtrFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Sets the PI/PO numbers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManSetCioIds( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachCi( p, pObj, i )
        pObj->CioId = i;
    Aig_ManForEachCo( p, pObj, i )
        pObj->CioId = i;
}

/**Function*************************************************************

  Synopsis    [Sets the PI/PO numbers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCleanCioIds( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachCi( p, pObj, i )
        pObj->pNext = NULL;
    Aig_ManForEachCo( p, pObj, i )
        pObj->pNext = NULL;
}

/**Function*************************************************************

  Synopsis    [Sets the PI/PO numbers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManChoiceNum( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    Aig_ManForEachNode( p, pObj, i )
        Counter += Aig_ObjIsChoice( p, pObj );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Prints the fanouts of the control register.]

  Description [Useful only for Intel MC benchmarks.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPrintControlFanouts( Aig_Man_t * p )
{
    Aig_Obj_t * pObj, * pFanin0, * pFanin1, * pCtrl;
    int i;

    pCtrl = Aig_ManCi( p, Aig_ManCiNum(p) - 1 );

    printf( "Control signal:\n" );
    Aig_ObjPrint( p, pCtrl ); printf( "\n\n" );

    Aig_ManForEachObj( p, pObj, i )
    {
        if ( !Aig_ObjIsNode(pObj) )
            continue;
        assert( pObj != pCtrl );
        pFanin0 = Aig_ObjFanin0(pObj);
        pFanin1 = Aig_ObjFanin1(pObj);
        if ( pFanin0 == pCtrl && Aig_ObjIsCi(pFanin1) )
        {
            Aig_ObjPrint( p, pObj ); printf( "\n" );
            Aig_ObjPrint( p, pFanin1 ); printf( "\n" );
            printf( "\n" );
        }
        if ( pFanin1 == pCtrl && Aig_ObjIsCi(pFanin0) )
        {
            Aig_ObjPrint( p, pObj ); printf( "\n" );
            Aig_ObjPrint( p, pFanin0 ); printf( "\n" );
            printf( "\n" );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Returns the composite name of the file.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Aig_FileNameGenericAppend( char * pBase, char * pSuffix )
{
    static char Buffer[1000];
    char * pDot;
    strcpy( Buffer, pBase );
    if ( (pDot = strrchr( Buffer, '.' )) )
        *pDot = 0;
    strcat( Buffer, pSuffix );
    if ( (pDot = strrchr( Buffer, '\\' )) || (pDot = strrchr( Buffer, '/' )) )
        return pDot+1;
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Creates a sequence of random numbers.]

  Description []
               
  SideEffects []

  SeeAlso     [http://en.wikipedia.org/wiki/LFSR]

***********************************************************************/
void Aig_ManRandomTest2()
{
    FILE * pFile;
    unsigned int lfsr = 1;
    unsigned int period = 0; 
    pFile = fopen( "rand.txt", "w" );
    do {
//        lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xd0000001u); // taps 32 31 29 1 
        lfsr = 1; // to prevent the warning
        ++period;
        fprintf( pFile, "%10d : %10d ", period, lfsr );
//        Extra_PrintBinary( pFile, &lfsr, 32 );
        fprintf( pFile, "\n" );
        if ( period == 20000 )
            break;
    } while(lfsr != 1u);
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Creates a sequence of random numbers.]

  Description []
               
  SideEffects []

  SeeAlso     [http://www.codeproject.com/KB/recipes/SimpleRNG.aspx]

***********************************************************************/
void Aig_ManRandomTest1()
{
    FILE * pFile;
    unsigned int lfsr;
    unsigned int period = 0; 
    pFile = fopen( "rand.txt", "w" );
    do {
        lfsr = Aig_ManRandom( 0 );
        ++period;
        fprintf( pFile, "%10d : %10d ", period, lfsr );
//        Extra_PrintBinary( pFile, &lfsr, 32 );
        fprintf( pFile, "\n" );
        if ( period == 20000 )
            break;
    } while(lfsr != 1u);
    fclose( pFile );
}

 
#define NUMBER1  3716960521u
#define NUMBER2  2174103536u

/**Function*************************************************************

  Synopsis    [Creates a sequence of random numbers.]

  Description []
               
  SideEffects []

  SeeAlso     [http://www.codeproject.com/KB/recipes/SimpleRNG.aspx]

***********************************************************************/
unsigned Aig_ManRandom( int fReset )
{
#ifdef _MSC_VER
    static unsigned int m_z = NUMBER1;
    static unsigned int m_w = NUMBER2;
#else
    static __thread unsigned int m_z = NUMBER1;
    static __thread unsigned int m_w = NUMBER2;
#endif    
    if ( fReset )
    {
        m_z = NUMBER1;
        m_w = NUMBER2;
    }
    m_z = 36969 * (m_z & 65535) + (m_z >> 16);
    m_w = 18000 * (m_w & 65535) + (m_w >> 16);
    return (m_z << 16) + m_w;
}

/**Function*************************************************************

  Synopsis    [Creates a sequence of random numbers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Aig_ManRandom64( int fReset )
{
    word Res = (word)Aig_ManRandom(fReset);
    return Res | ((word)Aig_ManRandom(0) << 32);
}


/**Function*************************************************************

  Synopsis    [Creates random info for the primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManRandomInfo( Vec_Ptr_t * vInfo, int iInputStart, int iWordStart, int iWordStop )
{
    unsigned * pInfo;
    int i, w;
    Vec_PtrForEachEntryStart( unsigned *, vInfo, pInfo, i, iInputStart )
        for ( w = iWordStart; w < iWordStop; w++ )
            pInfo[w] = Aig_ManRandom(0);
}

/**Function*************************************************************

  Synopsis    [Returns the result of merging the two vectors.]

  Description [Assumes that the vectors are sorted in the increasing order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_NodeUnionLists( Vec_Ptr_t * vArr1, Vec_Ptr_t * vArr2, Vec_Ptr_t * vArr )
{
    Aig_Obj_t ** pBeg  = (Aig_Obj_t **)vArr->pArray;
    Aig_Obj_t ** pBeg1 = (Aig_Obj_t **)vArr1->pArray;
    Aig_Obj_t ** pBeg2 = (Aig_Obj_t **)vArr2->pArray;
    Aig_Obj_t ** pEnd1 = (Aig_Obj_t **)vArr1->pArray + vArr1->nSize;
    Aig_Obj_t ** pEnd2 = (Aig_Obj_t **)vArr2->pArray + vArr2->nSize;
    Vec_PtrGrow( vArr, Vec_PtrSize(vArr1) + Vec_PtrSize(vArr2) );
    pBeg  = (Aig_Obj_t **)vArr->pArray;
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( (*pBeg1)->Id == (*pBeg2)->Id )
            *pBeg++ = *pBeg1++, pBeg2++;
        else if ( (*pBeg1)->Id < (*pBeg2)->Id )
            *pBeg++ = *pBeg1++;
        else 
            *pBeg++ = *pBeg2++;
    }
    while ( pBeg1 < pEnd1 )
        *pBeg++ = *pBeg1++;
    while ( pBeg2 < pEnd2 )
        *pBeg++ = *pBeg2++;
    vArr->nSize = pBeg - (Aig_Obj_t **)vArr->pArray;
    assert( vArr->nSize <= vArr->nCap );
    assert( vArr->nSize >= vArr1->nSize );
    assert( vArr->nSize >= vArr2->nSize );
}

/**Function*************************************************************

  Synopsis    [Returns the result of intersecting the two vectors.]

  Description [Assumes that the vectors are sorted in the increasing order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_NodeIntersectLists( Vec_Ptr_t * vArr1, Vec_Ptr_t * vArr2, Vec_Ptr_t * vArr )
{
    Aig_Obj_t ** pBeg  = (Aig_Obj_t **)vArr->pArray;
    Aig_Obj_t ** pBeg1 = (Aig_Obj_t **)vArr1->pArray;
    Aig_Obj_t ** pBeg2 = (Aig_Obj_t **)vArr2->pArray;
    Aig_Obj_t ** pEnd1 = (Aig_Obj_t **)vArr1->pArray + vArr1->nSize;
    Aig_Obj_t ** pEnd2 = (Aig_Obj_t **)vArr2->pArray + vArr2->nSize;
    Vec_PtrGrow( vArr, Abc_MaxInt( Vec_PtrSize(vArr1), Vec_PtrSize(vArr2) ) );
    pBeg  = (Aig_Obj_t **)vArr->pArray;
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( (*pBeg1)->Id == (*pBeg2)->Id )
            *pBeg++ = *pBeg1++, pBeg2++;
        else if ( (*pBeg1)->Id < (*pBeg2)->Id )
//            *pBeg++ = *pBeg1++;
            pBeg1++;
        else 
//            *pBeg++ = *pBeg2++;
            pBeg2++;
    }
//    while ( pBeg1 < pEnd1 )
//        *pBeg++ = *pBeg1++;
//    while ( pBeg2 < pEnd2 )
//        *pBeg++ = *pBeg2++;
    vArr->nSize = pBeg - (Aig_Obj_t **)vArr->pArray;
    assert( vArr->nSize <= vArr->nCap );
    assert( vArr->nSize <= vArr1->nSize );
    assert( vArr->nSize <= vArr2->nSize );
}

ABC_NAMESPACE_IMPL_END

#include "proof/fra/fra.h"
#include "aig/saig/saig.h"

ABC_NAMESPACE_IMPL_START


extern void Aig_ManCounterExampleValueStart( Aig_Man_t * pAig, Abc_Cex_t * pCex );
extern void Aig_ManCounterExampleValueStop( Aig_Man_t * pAig );
extern int Aig_ManCounterExampleValueLookup(  Aig_Man_t * pAig, int Id, int iFrame );

/**Function*************************************************************

  Synopsis    [Starts the process of retuning values for internal nodes.]

  Description [Should be called when pCex is available, before probing 
  any object for its value using Aig_ManCounterExampleValueLookup().]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCounterExampleValueStart( Aig_Man_t * pAig, Abc_Cex_t * pCex )
{
    Aig_Obj_t * pObj, * pObjRi, * pObjRo;
    int Val0, Val1, nObjs, i, k, iBit = 0;
    assert( Aig_ManRegNum(pAig) > 0 ); // makes sense only for sequential AIGs
    assert( pAig->pData2 == NULL );    // if this fail, there may be a memory leak
    // allocate memory to store simulation bits for internal nodes
    pAig->pData2 = ABC_CALLOC( unsigned, Abc_BitWordNum( (pCex->iFrame + 1) * Aig_ManObjNumMax(pAig) ) );
    // the register values in the counter-example should be zero
    Saig_ManForEachLo( pAig, pObj, k )
        assert( Abc_InfoHasBit(pCex->pData, iBit) == 0 ), iBit++;
    // iterate through the timeframes
    nObjs = Aig_ManObjNumMax(pAig);
    for ( i = 0; i <= pCex->iFrame; i++ )
    {
        // set constant 1 node
        Abc_InfoSetBit( (unsigned *)pAig->pData2, nObjs * i + 0 );
        // set primary inputs according to the counter-example
        Saig_ManForEachPi( pAig, pObj, k )
            if ( Abc_InfoHasBit(pCex->pData, iBit++) )
                Abc_InfoSetBit( (unsigned *)pAig->pData2, nObjs * i + Aig_ObjId(pObj) );
        // compute values for each node
        Aig_ManForEachNode( pAig, pObj, k )
        {
            Val0 = Abc_InfoHasBit( (unsigned *)pAig->pData2, nObjs * i + Aig_ObjFaninId0(pObj) );
            Val1 = Abc_InfoHasBit( (unsigned *)pAig->pData2, nObjs * i + Aig_ObjFaninId1(pObj) );
            if ( (Val0 ^ Aig_ObjFaninC0(pObj)) & (Val1 ^ Aig_ObjFaninC1(pObj)) )
                Abc_InfoSetBit( (unsigned *)pAig->pData2, nObjs * i + Aig_ObjId(pObj) );
        }
        // derive values for combinational outputs
        Aig_ManForEachCo( pAig, pObj, k )
        {
            Val0 = Abc_InfoHasBit( (unsigned *)pAig->pData2, nObjs * i + Aig_ObjFaninId0(pObj) );
            if ( Val0 ^ Aig_ObjFaninC0(pObj) )
                Abc_InfoSetBit( (unsigned *)pAig->pData2, nObjs * i + Aig_ObjId(pObj) );
        }
        if ( i == pCex->iFrame )
            continue;
        // transfer values to the register output of the next frame
        Saig_ManForEachLiLo( pAig, pObjRi, pObjRo, k )
            if ( Abc_InfoHasBit( (unsigned *)pAig->pData2, nObjs * i + Aig_ObjId(pObjRi) ) )
                Abc_InfoSetBit( (unsigned *)pAig->pData2, nObjs * (i+1) + Aig_ObjId(pObjRo) );
    }
    assert( iBit == pCex->nBits );
    // check that the counter-example is correct, that is, the corresponding output is asserted
    assert( Abc_InfoHasBit( (unsigned *)pAig->pData2, nObjs * pCex->iFrame + Aig_ObjId(Aig_ManCo(pAig, pCex->iPo)) ) );
}

/**Function*************************************************************

  Synopsis    [Stops the process of retuning values for internal nodes.]

  Description [Should be called when probing is no longer needed]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCounterExampleValueStop( Aig_Man_t * pAig )
{
    assert( pAig->pData2 != NULL );    // if this fail, we try to call this procedure more than once
    free( pAig->pData2 );
    pAig->pData2 = NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the value of the given object in the given timeframe.]

  Description [Should be called to probe the value of an object with 
  the given ID (iFrame is a 0-based number of a time frame - should not 
  exceed the number of timeframes in the original counter-example).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManCounterExampleValueLookup(  Aig_Man_t * pAig, int Id, int iFrame )
{
    assert( Id >= 0 && Id < Aig_ManObjNumMax(pAig) );
    return Abc_InfoHasBit( (unsigned *)pAig->pData2, Aig_ManObjNumMax(pAig) * iFrame + Id );
}

/**Function*************************************************************

  Synopsis    [Procedure to test the above code.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCounterExampleValueTest( Aig_Man_t * pAig, Abc_Cex_t * pCex )
{
    Aig_Obj_t * pObj = Aig_ManObj( pAig, Aig_ManObjNumMax(pAig)/2 );
    int iFrame = Abc_MaxInt( 0, pCex->iFrame - 1 );
    printf( "\nUsing counter-example, which asserts output %d in frame %d.\n", pCex->iPo, pCex->iFrame );
    Aig_ManCounterExampleValueStart( pAig, pCex );
    printf( "Value of object %d in frame %d is %d.\n", Aig_ObjId(pObj), iFrame,
        Aig_ManCounterExampleValueLookup(pAig, Aig_ObjId(pObj), iFrame) );
    Aig_ManCounterExampleValueStop( pAig );
}

/**Function*************************************************************

  Synopsis    [Handle the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManSetPhase( Aig_Man_t * pAig )
{
    Aig_Obj_t * pObj;
    int i;
    // set the PI simulation information
    Aig_ManConst1( pAig )->fPhase = 1;
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->fPhase = 0;
    // simulate internal nodes
    Aig_ManForEachNode( pAig, pObj, i )
        pObj->fPhase = ( Aig_ObjFanin0(pObj)->fPhase ^ Aig_ObjFaninC0(pObj) )
                     & ( Aig_ObjFanin1(pObj)->fPhase ^ Aig_ObjFaninC1(pObj) );
    // simulate PO nodes
    Aig_ManForEachCo( pAig, pObj, i )
        pObj->fPhase = Aig_ObjFanin0(pObj)->fPhase ^ Aig_ObjFaninC0(pObj);
}


/**Function*************************************************************

  Synopsis    [Collects muxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManMuxesCollect( Aig_Man_t * pAig )
{
    Vec_Ptr_t * vMuxes;
    Aig_Obj_t * pObj;
    int i;
    vMuxes = Vec_PtrAlloc( 100 );
    Aig_ManForEachNode( pAig, pObj, i )
        if ( Aig_ObjIsMuxType(pObj) )
            Vec_PtrPush( vMuxes, pObj );
    return vMuxes;
}

/**Function*************************************************************

  Synopsis    [Dereferences muxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManMuxesDeref( Aig_Man_t * pAig, Vec_Ptr_t * vMuxes )
{
    Aig_Obj_t * pObj, * pNodeT, * pNodeE, * pNodeC;
    int i;
    Vec_PtrForEachEntry( Aig_Obj_t *, vMuxes, pObj, i )
    {
        if ( Aig_ObjRecognizeExor( pObj, &pNodeT, &pNodeE ) )
        {
            pNodeT->nRefs--;
            pNodeE->nRefs--;
        }
        else
        {
            pNodeC = Aig_ObjRecognizeMux( pObj, &pNodeT, &pNodeE );
            pNodeC->nRefs--;
        }
    }
}

/**Function*************************************************************

  Synopsis    [References muxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManMuxesRef( Aig_Man_t * pAig, Vec_Ptr_t * vMuxes )
{
    Aig_Obj_t * pObj, * pNodeT, * pNodeE, * pNodeC;
    int i;
    Vec_PtrForEachEntry( Aig_Obj_t *, vMuxes, pObj, i )
    {
        if ( Aig_ObjRecognizeExor( pObj, &pNodeT, &pNodeE ) )
        {
            pNodeT->nRefs++;
            pNodeE->nRefs++;
        }
        else
        {
            pNodeC = Aig_ObjRecognizeMux( pObj, &pNodeT, &pNodeE );
            pNodeC->nRefs++;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Complements the constraint outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManInvertConstraints( Aig_Man_t * pAig )
{
    Aig_Obj_t * pObj;
    int i;
    if ( Aig_ManConstrNum(pAig) == 0 )
        return;
    Saig_ManForEachPo( pAig, pObj, i )
    {
        if ( i >= Saig_ManPoNum(pAig) - Aig_ManConstrNum(pAig) )
            Aig_ObjChild0Flip( pObj );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

