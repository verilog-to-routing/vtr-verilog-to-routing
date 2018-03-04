/**CFile****************************************************************

  FileName    [hopUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [Various procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: hopUtil.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "hop.h"

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
void Hop_ManIncrementTravId( Hop_Man_t * p )
{
    if ( p->nTravIds >= (1<<30)-1 )
        Hop_ManCleanData( p );
    p->nTravIds++;
}

/**Function*************************************************************

  Synopsis    [Cleans the data pointers for the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ManCleanData( Hop_Man_t * p )
{
    Hop_Obj_t * pObj;
    int i;
    p->nTravIds = 1;
    Hop_ManConst1(p)->pData = NULL;
    Hop_ManForEachPi( p, pObj, i )
        pObj->pData = NULL;
    Hop_ManForEachPo( p, pObj, i )
        pObj->pData = NULL;
    Hop_ManForEachNode( p, pObj, i )
        pObj->pData = NULL;
}

/**Function*************************************************************

  Synopsis    [Recursively cleans the data pointers in the cone of the node.]

  Description [Applicable to small AIGs only because no caching is performed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ObjCleanData_rec( Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    assert( !Hop_ObjIsPo(pObj) );
    if ( Hop_ObjIsAnd(pObj) )
    {
        Hop_ObjCleanData_rec( Hop_ObjFanin0(pObj) );
        Hop_ObjCleanData_rec( Hop_ObjFanin1(pObj) );
    }
    pObj->pData = NULL;
}

/**Function*************************************************************

  Synopsis    [Detects multi-input gate rooted at this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ObjCollectMulti_rec( Hop_Obj_t * pRoot, Hop_Obj_t * pObj, Vec_Ptr_t * vSuper )
{
    if ( pRoot != pObj && (Hop_IsComplement(pObj) || Hop_ObjIsPi(pObj) || Hop_ObjType(pRoot) != Hop_ObjType(pObj)) )
    {
        Vec_PtrPushUnique(vSuper, pObj);
        return;
    }
    Hop_ObjCollectMulti_rec( pRoot, Hop_ObjChild0(pObj), vSuper );
    Hop_ObjCollectMulti_rec( pRoot, Hop_ObjChild1(pObj), vSuper );
}

/**Function*************************************************************

  Synopsis    [Detects multi-input gate rooted at this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ObjCollectMulti( Hop_Obj_t * pRoot, Vec_Ptr_t * vSuper )
{
    assert( !Hop_IsComplement(pRoot) );
    Vec_PtrClear( vSuper );
    Hop_ObjCollectMulti_rec( pRoot, pRoot, vSuper );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is the root of MUX or EXOR/NEXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hop_ObjIsMuxType( Hop_Obj_t * pNode )
{
    Hop_Obj_t * pNode0, * pNode1;
    // check that the node is regular
    assert( !Hop_IsComplement(pNode) );
    // if the node is not AND, this is not MUX
    if ( !Hop_ObjIsAnd(pNode) )
        return 0;
    // if the children are not complemented, this is not MUX
    if ( !Hop_ObjFaninC0(pNode) || !Hop_ObjFaninC1(pNode) )
        return 0;
    // get children
    pNode0 = Hop_ObjFanin0(pNode);
    pNode1 = Hop_ObjFanin1(pNode);
    // if the children are not ANDs, this is not MUX
    if ( !Hop_ObjIsAnd(pNode0) || !Hop_ObjIsAnd(pNode1) )
        return 0;
    // otherwise the node is MUX iff it has a pair of equal grandchildren
    return (Hop_ObjFanin0(pNode0) == Hop_ObjFanin0(pNode1) && (Hop_ObjFaninC0(pNode0) ^ Hop_ObjFaninC0(pNode1))) || 
           (Hop_ObjFanin0(pNode0) == Hop_ObjFanin1(pNode1) && (Hop_ObjFaninC0(pNode0) ^ Hop_ObjFaninC1(pNode1))) ||
           (Hop_ObjFanin1(pNode0) == Hop_ObjFanin0(pNode1) && (Hop_ObjFaninC1(pNode0) ^ Hop_ObjFaninC0(pNode1))) ||
           (Hop_ObjFanin1(pNode0) == Hop_ObjFanin1(pNode1) && (Hop_ObjFaninC1(pNode0) ^ Hop_ObjFaninC1(pNode1)));
}


/**Function*************************************************************

  Synopsis    [Recognizes what nodes are inputs of the EXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hop_ObjRecognizeExor( Hop_Obj_t * pObj, Hop_Obj_t ** ppFan0, Hop_Obj_t ** ppFan1 )
{
    Hop_Obj_t * p0, * p1;
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) )
        return 0;
    if ( Hop_ObjIsExor(pObj) )
    {
        *ppFan0 = Hop_ObjChild0(pObj);
        *ppFan1 = Hop_ObjChild1(pObj);
        return 1;
    }
    assert( Hop_ObjIsAnd(pObj) );
    p0 = Hop_ObjChild0(pObj);
    p1 = Hop_ObjChild1(pObj);
    if ( !Hop_IsComplement(p0) || !Hop_IsComplement(p1) )
        return 0;
    p0 = Hop_Regular(p0);
    p1 = Hop_Regular(p1);
    if ( !Hop_ObjIsAnd(p0) || !Hop_ObjIsAnd(p1) )
        return 0;
    if ( Hop_ObjFanin0(p0) != Hop_ObjFanin0(p1) || Hop_ObjFanin1(p0) != Hop_ObjFanin1(p1) )
        return 0;
    if ( Hop_ObjFaninC0(p0) == Hop_ObjFaninC0(p1) || Hop_ObjFaninC1(p0) == Hop_ObjFaninC1(p1) )
        return 0;
    *ppFan0 = Hop_ObjChild0(p0);
    *ppFan1 = Hop_ObjChild1(p0);
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
Hop_Obj_t * Hop_ObjRecognizeMux( Hop_Obj_t * pNode, Hop_Obj_t ** ppNodeT, Hop_Obj_t ** ppNodeE )
{
    Hop_Obj_t * pNode0, * pNode1;
    assert( !Hop_IsComplement(pNode) );
    assert( Hop_ObjIsMuxType(pNode) );
    // get children
    pNode0 = Hop_ObjFanin0(pNode);
    pNode1 = Hop_ObjFanin1(pNode);

    // find the control variable
    if ( Hop_ObjFanin1(pNode0) == Hop_ObjFanin1(pNode1) && (Hop_ObjFaninC1(pNode0) ^ Hop_ObjFaninC1(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p2) )
        if ( Hop_ObjFaninC1(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Hop_Not(Hop_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Hop_Not(Hop_ObjChild0(pNode0));//pNode1->p1);
            return Hop_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Hop_Not(Hop_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Hop_Not(Hop_ObjChild0(pNode1));//pNode2->p1);
            return Hop_ObjChild1(pNode0);//pNode1->p2;
        }
    }
    else if ( Hop_ObjFanin0(pNode0) == Hop_ObjFanin0(pNode1) && (Hop_ObjFaninC0(pNode0) ^ Hop_ObjFaninC0(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p1) )
        if ( Hop_ObjFaninC0(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Hop_Not(Hop_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Hop_Not(Hop_ObjChild1(pNode0));//pNode1->p2);
            return Hop_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Hop_Not(Hop_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Hop_Not(Hop_ObjChild1(pNode1));//pNode2->p2);
            return Hop_ObjChild0(pNode0);//pNode1->p1;
        }
    }
    else if ( Hop_ObjFanin0(pNode0) == Hop_ObjFanin1(pNode1) && (Hop_ObjFaninC0(pNode0) ^ Hop_ObjFaninC1(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p1) )
        if ( Hop_ObjFaninC0(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Hop_Not(Hop_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Hop_Not(Hop_ObjChild1(pNode0));//pNode1->p2);
            return Hop_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Hop_Not(Hop_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Hop_Not(Hop_ObjChild0(pNode1));//pNode2->p1);
            return Hop_ObjChild0(pNode0);//pNode1->p1;
        }
    }
    else if ( Hop_ObjFanin1(pNode0) == Hop_ObjFanin0(pNode1) && (Hop_ObjFaninC1(pNode0) ^ Hop_ObjFaninC0(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p2) )
        if ( Hop_ObjFaninC1(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Hop_Not(Hop_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Hop_Not(Hop_ObjChild0(pNode0));//pNode1->p1);
            return Hop_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Hop_Not(Hop_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Hop_Not(Hop_ObjChild1(pNode1));//pNode2->p2);
            return Hop_ObjChild1(pNode0);//pNode1->p2;
        }
    }
    assert( 0 ); // this is not MUX
    return NULL;
}


/**Function*************************************************************

  Synopsis    [Prints Eqn formula for the AIG rooted at this node.]

  Description [The formula is in terms of PIs, which should have
  their names assigned in pObj->pData fields.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ObjPrintEqn( FILE * pFile, Hop_Obj_t * pObj, Vec_Vec_t * vLevels, int Level )
{
    Vec_Ptr_t * vSuper;
    Hop_Obj_t * pFanin;
    int fCompl, i;
    // store the complemented attribute
    fCompl = Hop_IsComplement(pObj);
    pObj = Hop_Regular(pObj);
    // constant case
    if ( Hop_ObjIsConst1(pObj) )
    {
        fprintf( pFile, "%d", !fCompl );
        return;
    }
    // PI case
    if ( Hop_ObjIsPi(pObj) )
    {
        fprintf( pFile, "%s%s", fCompl? "!" : "", (char*)pObj->pData );
        return;
    }
    // AND case
    Vec_VecExpand( vLevels, Level );
    vSuper = Vec_VecEntry(vLevels, Level);
    Hop_ObjCollectMulti( pObj, vSuper );
    fprintf( pFile, "%s", (Level==0? "" : "(") );
    Vec_PtrForEachEntry( Hop_Obj_t *, vSuper, pFanin, i )
    {
        Hop_ObjPrintEqn( pFile, Hop_NotCond(pFanin, fCompl), vLevels, Level+1 );
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
void Hop_ObjPrintVerilog( FILE * pFile, Hop_Obj_t * pObj, Vec_Vec_t * vLevels, int Level )
{
    Vec_Ptr_t * vSuper;
    Hop_Obj_t * pFanin, * pFanin0, * pFanin1, * pFaninC;
    int fCompl, i;
    // store the complemented attribute
    fCompl = Hop_IsComplement(pObj);
    pObj = Hop_Regular(pObj);
    // constant case
    if ( Hop_ObjIsConst1(pObj) )
    {
        fprintf( pFile, "1\'b%d", !fCompl );
        return;
    }
    // PI case
    if ( Hop_ObjIsPi(pObj) )
    {
        fprintf( pFile, "%s%s", fCompl? "~" : "", (char*)pObj->pData );
        return;
    }
    // EXOR case
    if ( Hop_ObjIsExor(pObj) )
    {
        Vec_VecExpand( vLevels, Level );
        vSuper = Vec_VecEntry( vLevels, Level );
        Hop_ObjCollectMulti( pObj, vSuper );
        fprintf( pFile, "%s", (Level==0? "" : "(") );
        Vec_PtrForEachEntry( Hop_Obj_t *, vSuper, pFanin, i )
        {
            Hop_ObjPrintVerilog( pFile, Hop_NotCond(pFanin, (fCompl && i==0)), vLevels, Level+1 );
            if ( i < Vec_PtrSize(vSuper) - 1 )
                fprintf( pFile, " ^ " );
        }
        fprintf( pFile, "%s", (Level==0? "" : ")") );
        return;
    }
    // MUX case
    if ( Hop_ObjIsMuxType(pObj) )
    {
        if ( Hop_ObjRecognizeExor( pObj, &pFanin0, &pFanin1 ) )
        {
            fprintf( pFile, "%s", (Level==0? "" : "(") );
            Hop_ObjPrintVerilog( pFile, Hop_NotCond(pFanin0, fCompl), vLevels, Level+1 );
            fprintf( pFile, " ^ " );
            Hop_ObjPrintVerilog( pFile, pFanin1, vLevels, Level+1 );
            fprintf( pFile, "%s", (Level==0? "" : ")") );
        }
        else 
        {
            pFaninC = Hop_ObjRecognizeMux( pObj, &pFanin1, &pFanin0 );
            fprintf( pFile, "%s", (Level==0? "" : "(") );
            Hop_ObjPrintVerilog( pFile, pFaninC, vLevels, Level+1 );
            fprintf( pFile, " ? " );
            Hop_ObjPrintVerilog( pFile, Hop_NotCond(pFanin1, fCompl), vLevels, Level+1 );
            fprintf( pFile, " : " );
            Hop_ObjPrintVerilog( pFile, Hop_NotCond(pFanin0, fCompl), vLevels, Level+1 );
            fprintf( pFile, "%s", (Level==0? "" : ")") );
        }
        return;
    }
    // AND case
    Vec_VecExpand( vLevels, Level );
    vSuper = Vec_VecEntry(vLevels, Level);
    Hop_ObjCollectMulti( pObj, vSuper );
    fprintf( pFile, "%s", (Level==0? "" : "(") );
    Vec_PtrForEachEntry( Hop_Obj_t *, vSuper, pFanin, i )
    {
        Hop_ObjPrintVerilog( pFile, Hop_NotCond(pFanin, fCompl), vLevels, Level+1 );
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
void Hop_ObjPrintVerbose( Hop_Obj_t * pObj, int fHaig )
{
    assert( !Hop_IsComplement(pObj) );
    printf( "Node %p : ", pObj );
    if ( Hop_ObjIsConst1(pObj) )
        printf( "constant 1" );
    else if ( Hop_ObjIsPi(pObj) )
        printf( "PI" );
    else
        printf( "AND( %p%s, %p%s )", 
            Hop_ObjFanin0(pObj), (Hop_ObjFaninC0(pObj)? "\'" : " "), 
            Hop_ObjFanin1(pObj), (Hop_ObjFaninC1(pObj)? "\'" : " ") );
    printf( " (refs = %3d)", Hop_ObjRefs(pObj) );
}

/**Function*************************************************************

  Synopsis    [Prints node in HAIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ManPrintVerbose( Hop_Man_t * p, int fHaig )
{
    Vec_Ptr_t * vNodes;
    Hop_Obj_t * pObj;
    int i;
    printf( "PIs: " );
    Hop_ManForEachPi( p, pObj, i )
        printf( " %p", pObj );
    printf( "\n" );
    vNodes = Hop_ManDfs( p );
    Vec_PtrForEachEntry( Hop_Obj_t *, vNodes, pObj, i )
        Hop_ObjPrintVerbose( pObj, fHaig ), printf( "\n" );
    printf( "\n" );
    Vec_PtrFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Writes the AIG into the BLIF file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ManDumpBlif( Hop_Man_t * p, char * pFileName )
{
    FILE * pFile;
    Vec_Ptr_t * vNodes;
    Hop_Obj_t * pObj, * pConst1 = NULL;
    int i, nDigits, Counter = 0;
    if ( Hop_ManPoNum(p) == 0 )
    {
        printf( "Hop_ManDumpBlif(): AIG manager does not have POs.\n" );
        return;
    }
    // collect nodes in the DFS order
    vNodes = Hop_ManDfs( p );
    // assign IDs to objects
    Hop_ManConst1(p)->pData = (void *)(ABC_PTRUINT_T)Counter++;
    Hop_ManForEachPi( p, pObj, i )
        pObj->pData = (void *)(ABC_PTRUINT_T)Counter++;
    Hop_ManForEachPo( p, pObj, i )
        pObj->pData = (void *)(ABC_PTRUINT_T)Counter++;
    Vec_PtrForEachEntry( Hop_Obj_t *, vNodes, pObj, i )
        pObj->pData = (void *)(ABC_PTRUINT_T)Counter++;
    nDigits = Hop_Base10Log( Counter );
    // write the file
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# BLIF file written by procedure Hop_ManDumpBlif() in ABC\n" );
    fprintf( pFile, "# http://www.eecs.berkeley.edu/~alanmi/abc/\n" );
    fprintf( pFile, ".model test\n" );
    // write PIs
    fprintf( pFile, ".inputs" );
    Hop_ManForEachPi( p, pObj, i )
        fprintf( pFile, " n%0*d", nDigits, (int)(ABC_PTRUINT_T)pObj->pData );
    fprintf( pFile, "\n" );
    // write POs
    fprintf( pFile, ".outputs" );
    Hop_ManForEachPo( p, pObj, i )
        fprintf( pFile, " n%0*d", nDigits, (int)(ABC_PTRUINT_T)pObj->pData );
    fprintf( pFile, "\n" );
    // write nodes
    Vec_PtrForEachEntry( Hop_Obj_t *, vNodes, pObj, i )
    {
        fprintf( pFile, ".names n%0*d n%0*d n%0*d\n", 
            nDigits, (int)(ABC_PTRUINT_T)Hop_ObjFanin0(pObj)->pData, 
            nDigits, (int)(ABC_PTRUINT_T)Hop_ObjFanin1(pObj)->pData, 
            nDigits, (int)(ABC_PTRUINT_T)pObj->pData );
        fprintf( pFile, "%d%d 1\n", !Hop_ObjFaninC0(pObj), !Hop_ObjFaninC1(pObj) );
    }
    // write POs
    Hop_ManForEachPo( p, pObj, i )
    {
        fprintf( pFile, ".names n%0*d n%0*d\n", 
            nDigits, (int)(ABC_PTRUINT_T)Hop_ObjFanin0(pObj)->pData, 
            nDigits, (int)(ABC_PTRUINT_T)pObj->pData );
        fprintf( pFile, "%d 1\n", !Hop_ObjFaninC0(pObj) );
        if ( Hop_ObjIsConst1(Hop_ObjFanin0(pObj)) )
            pConst1 = Hop_ManConst1(p);
    }
    if ( pConst1 )
        fprintf( pFile, ".names n%0*d\n 1\n", nDigits, (int)(ABC_PTRUINT_T)pConst1->pData );
    fprintf( pFile, ".end\n\n" );
    fclose( pFile );
    Vec_PtrFree( vNodes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

