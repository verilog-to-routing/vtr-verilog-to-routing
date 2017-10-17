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

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    [Returns the next prime >= p.]

  Description [Copied from CUDD, for stand-aloneness.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
unsigned int Aig_PrimeCudd( unsigned int p )
{
    int i,pn;

    p--;
    do {
        p++;
        if (p&1) {
	    pn = 1;
	    i = 3;
	    while ((unsigned) (i * i) <= p) {
		if (p % i == 0) {
		    pn = 0;
		    break;
		}
		i += 2;
	    }
	} else {
	    pn = 0;
	}
    } while (!pn);
    return(p);

} /* end of Cudd_Prime */

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

  Synopsis    [Collect the latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManLevels( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, LevelMax = 0;
    Aig_ManForEachPo( p, pObj, i )
        LevelMax = AIG_MAX( LevelMax, (int)Aig_ObjFanin0(pObj)->Level );
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

  Synopsis    [Cleans MarkB.]

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

  Synopsis    [Cleans MarkB.]

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

  Synopsis    [Recursively cleans the data pointers in the cone of the node.]

  Description [Applicable to small AIGs only because no caching is performed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjCleanData_rec( Aig_Obj_t * pObj )
{
    assert( !Aig_IsComplement(pObj) );
    assert( !Aig_ObjIsPo(pObj) );
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
    if ( pRoot != pObj && (Aig_IsComplement(pObj) || Aig_ObjIsPi(pObj) || Aig_ObjType(pRoot) != Aig_ObjType(pObj)) )
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
    if ( Aig_ObjIsPi(pObj) )
    {
        fprintf( pFile, "%s%s", fCompl? "!" : "", pObj->pData );
        return;
    }
    // AND case
    Vec_VecExpand( vLevels, Level );
    vSuper = Vec_VecEntry(vLevels, Level);
    Aig_ObjCollectMulti( pObj, vSuper );
    fprintf( pFile, "%s", (Level==0? "" : "(") );
    Vec_PtrForEachEntry( vSuper, pFanin, i )
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
    if ( Aig_ObjIsPi(pObj) )
    {
        fprintf( pFile, "%s%s", fCompl? "~" : "", pObj->pData );
        return;
    }
    // EXOR case
    if ( Aig_ObjIsExor(pObj) )
    {
        Vec_VecExpand( vLevels, Level );
        vSuper = Vec_VecEntry( vLevels, Level );
        Aig_ObjCollectMulti( pObj, vSuper );
        fprintf( pFile, "%s", (Level==0? "" : "(") );
        Vec_PtrForEachEntry( vSuper, pFanin, i )
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
    Vec_PtrForEachEntry( vSuper, pFanin, i )
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
    printf( "Node %p : ", pObj );
    if ( Aig_ObjIsConst1(pObj) )
        printf( "constant 1" );
    else if ( Aig_ObjIsPi(pObj) )
        printf( "PI" );
    else
        printf( "AND( %p%s, %p%s )", 
            Aig_ObjFanin0(pObj), (Aig_ObjFaninC0(pObj)? "\'" : " "), 
            Aig_ObjFanin1(pObj), (Aig_ObjFaninC1(pObj)? "\'" : " ") );
    printf( " (refs = %3d)", Aig_ObjRefs(pObj) );
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
    Aig_ManForEachPi( p, pObj, i )
        printf( " %p", pObj );
    printf( "\n" );
    vNodes = Aig_ManDfs( p );
    Vec_PtrForEachEntry( vNodes, pObj, i )
        Aig_ObjPrintVerbose( pObj, fHaig ), printf( "\n" );
    printf( "\n" );
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
    char FileName[20];
    // dump the logic into a file
    sprintf( FileName, "aigbug\\%03d.blif", ++Counter );
    Aig_ManDumpBlif( p, FileName );
    printf( "Intermediate AIG with %d nodes was written into file \"%s\".\n", Aig_ManNodeNum(p), FileName );
}

/**Function*************************************************************

  Synopsis    [Writes the AIG into the BLIF file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDumpBlif( Aig_Man_t * p, char * pFileName )
{
    FILE * pFile;
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj, * pConst1 = NULL;
    int i, nDigits, Counter = 0;
    if ( Aig_ManPoNum(p) == 0 )
    {
        printf( "Aig_ManDumpBlif(): AIG manager does not have POs.\n" );
        return;
    }
    // collect nodes in the DFS order
    vNodes = Aig_ManDfs( p );
    // assign IDs to objects
    Aig_ManConst1(p)->pData = (void *)Counter++;
    Aig_ManForEachPi( p, pObj, i )
        pObj->pData = (void *)Counter++;
    Aig_ManForEachPo( p, pObj, i )
        pObj->pData = (void *)Counter++;
    Vec_PtrForEachEntry( vNodes, pObj, i )
        pObj->pData = (void *)Counter++;
    nDigits = Extra_Base10Log( Counter );
    // write the file
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# BLIF file written by procedure Aig_ManDumpBlif() in ABC\n" );
    fprintf( pFile, "# http://www.eecs.berkeley.edu/~alanmi/abc/\n" );
    fprintf( pFile, ".model test\n" );
    // write PIs
    fprintf( pFile, ".inputs" );
    Aig_ManForEachPi( p, pObj, i )
        fprintf( pFile, " n%0*d", nDigits, (int)pObj->pData );
    fprintf( pFile, "\n" );
    // write POs
    fprintf( pFile, ".outputs" );
    Aig_ManForEachPo( p, pObj, i )
        fprintf( pFile, " n%0*d", nDigits, (int)pObj->pData );
    fprintf( pFile, "\n" );
    // write nodes
    Vec_PtrForEachEntry( vNodes, pObj, i )
    {
        fprintf( pFile, ".names n%0*d n%0*d n%0*d\n", 
            nDigits, (int)Aig_ObjFanin0(pObj)->pData, 
            nDigits, (int)Aig_ObjFanin1(pObj)->pData, 
            nDigits, (int)pObj->pData );
        fprintf( pFile, "%d%d 1\n", !Aig_ObjFaninC0(pObj), !Aig_ObjFaninC1(pObj) );
    }
    // write POs
    Aig_ManForEachPo( p, pObj, i )
    {
        if ( (int)pObj->pData == 1359 )
        {
            int x = 0;
        }
        fprintf( pFile, ".names n%0*d n%0*d\n", 
            nDigits, (int)Aig_ObjFanin0(pObj)->pData, 
            nDigits, (int)pObj->pData );
        fprintf( pFile, "%d 1\n", !Aig_ObjFaninC0(pObj) );
        if ( Aig_ObjIsConst1(Aig_ObjFanin0(pObj)) )
            pConst1 = Aig_ManConst1(p);
    }
    if ( pConst1 )
        fprintf( pFile, ".names n%0*d\n 1\n", nDigits, (int)pConst1->pData );
    fprintf( pFile, ".end\n\n" );
    fclose( pFile );
    Vec_PtrFree( vNodes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


