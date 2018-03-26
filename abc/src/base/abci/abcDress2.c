/**CFile****************************************************************

  FileName    [abcDressw.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Transfers names from one netlist to the other.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcDressw.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "aig/aig/aig.h"
#include "proof/dch/dch.h"

ABC_NAMESPACE_IMPL_START


/*
  Procedure Abc_NtkDressComputeEquivs() implemented in this file computes 
  equivalence classes of objects of the two networks (pNtk1 and pNtk2).

  It is possible that pNtk1 is the network before synthesis and pNtk2 is the
  network after synthesis.  The equiv classes of nodes from these networks 
  can be used to transfer the names from pNtk1 to pNtk2, or vice versa. 

  The above procedure returns the array (Vec_Ptr_t) of integer arrays (Vec_Int_t).
  Each of the integer arrays contains entries of one equivalence class.
  Each entry (EquivId) contains the following information: 
  (1) object ID, which is a number 'num', such that 0 <= 'num' < MaxId 
      where MaxId is the largest ID of nodes in a network
  (2) the polarity of the node, which is a binary number, 0 or 1, giving
      the node's value when pattern (000...0) is applied to the inputs
  (3) the number of the network, 0 or 1, which stands for pNtk1 and pNtk2, respectively
  The first array in the array of arrays is empty, or contains nodes that 
  are equivalent to a constant (if such nodes appear in the network).

  Given EquivID defined above, use the APIs below to get its components.
*/

// declarations to be added to the application code
extern int Abc_ObjEquivId2ObjId( int EquivId );
extern int Abc_ObjEquivId2Polar( int EquivId );
extern int Abc_ObjEquivId2NtkId( int EquivId );

// definition that may remain in this file
int Abc_ObjEquivId2ObjId( int EquivId ) { return  EquivId >> 2;      }
int Abc_ObjEquivId2Polar( int EquivId ) { return (EquivId >> 1) & 1; }
int Abc_ObjEquivId2NtkId( int EquivId ) { return  EquivId       & 1; }

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Aig_Man_t *  Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
extern void         Dch_ComputeEquivalences( Aig_Man_t * pAig, Dch_Pars_t * pPars );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the dual-output miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManCreateDualOutputMiter( Aig_Man_t * p1, Aig_Man_t * p2 )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj;
    int i;
    assert( Aig_ManCiNum(p1) == Aig_ManCiNum(p2) );
    assert( Aig_ManCoNum(p1) == Aig_ManCoNum(p2) );
    pNew = Aig_ManStart( Aig_ManObjNumMax(p1) + Aig_ManObjNumMax(p2) );
    // add first AIG
    Aig_ManConst1(p1)->pData = Aig_ManConst1(pNew);
    Aig_ManForEachCi( p1, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    Aig_ManForEachNode( p1, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // add second AIG
    Aig_ManConst1(p2)->pData = Aig_ManConst1(pNew);
    Aig_ManForEachCi( p2, pObj, i )
        pObj->pData = Aig_ManCi( pNew, i );
    Aig_ManForEachNode( p2, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // add the outputs
    for ( i = 0; i < Aig_ManCoNum(p1); i++ )
    {
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(Aig_ManCo(p1, i)) );
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(Aig_ManCo(p2, i)) );
    }
    Aig_ManCleanup( pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Sets polarity attribute of each object in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDressMapSetPolarity( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pAnd;
    int i;
    // each node refers to the the strash copy whose polarity is set
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( (pAnd = Abc_ObjRegular(pObj->pCopy)) && Abc_ObjType(pAnd) != ABC_OBJ_NONE ) // strashed object is present and legal
            pObj->fPhase = pAnd->fPhase ^ Abc_ObjIsComplement(pObj->pCopy);
    }
}

/**Function*************************************************************

  Synopsis    [Create mapping of node IDs of pNtk into equiv classes of pMiter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkDressMapClasses( Aig_Man_t * pMiter, Abc_Ntk_t * pNtk )
{
    Vec_Int_t * vId2Lit;
    Abc_Obj_t * pObj, * pAnd;
    Aig_Obj_t * pObjMan, * pObjMiter, * pObjRepr;
    int i;
    vId2Lit = Vec_IntAlloc( 0 );
    Vec_IntFill( vId2Lit, Abc_NtkObjNumMax(pNtk), -1 );
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        // get the pointer to the miter node corresponding to pObj
        if ( (pAnd = Abc_ObjRegular(pObj->pCopy)) && Abc_ObjType(pAnd) != ABC_OBJ_NONE &&          // strashed node is present and legal
             (pObjMan = Aig_Regular((Aig_Obj_t *)pAnd->pCopy)) && Aig_ObjType(pObjMan) != AIG_OBJ_NONE &&       // AIG node is present and legal
             (pObjMiter = Aig_Regular((Aig_Obj_t *)pObjMan->pData)) && Aig_ObjType(pObjMiter) != AIG_OBJ_NONE ) // miter node is present and legal
        {
            // get the representative of the miter node
            pObjRepr = Aig_ObjRepr( pMiter, pObjMiter );
            pObjRepr = pObjRepr? pObjRepr : pObjMiter;
            // map pObj (whose ID is i) into the repr node ID (i.e. equiv class)
            Vec_IntWriteEntry( vId2Lit, i, Aig_ObjId(pObjRepr) );
        }
    }
    return vId2Lit;
}

/**Function*************************************************************

  Synopsis    [Returns the vector of given equivalence class of objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_ObjDressClass( Vec_Ptr_t * vRes, Vec_Int_t * vClass2Num, int Class )
{
    int ClassNumber;
    assert( Class > 0 );
    ClassNumber = Vec_IntEntry( vClass2Num, Class );
    assert( ClassNumber != 0 );
    if ( ClassNumber > 0 )
        return (Vec_Int_t *)Vec_PtrEntry( vRes, ClassNumber ); // previous class
    // create new class
    Vec_IntWriteEntry( vClass2Num, Class, Vec_PtrSize(vRes) );
    Vec_PtrPush( vRes, Vec_IntAlloc(4) );
    return (Vec_Int_t *)Vec_PtrEntryLast( vRes ); 
}

/**Function*************************************************************

  Synopsis    [Returns the ID of a node in an equivalence class.]

  Description [The ID is composed of three parts: object ID, followed
  by one bit telling the phase of this node, followed by one bit
  telling the network to which this node belongs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjDressMakeId( Abc_Ntk_t * pNtk, int ObjId, int iNtk )
{
    return (ObjId << 2) | (Abc_NtkObj(pNtk,ObjId)->fPhase << 1) | iNtk;
}

/**Function*************************************************************

  Synopsis    [Computes equivalence classes of objects in pNtk1 and pNtk2.]

  Description [Internal procedure.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkDressMapIds( Aig_Man_t * pMiter, Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2 )
{
    Vec_Ptr_t * vRes;
    Vec_Int_t * vId2Lit1, * vId2Lit2, * vCounts0, * vCounts1, * vClassC, * vClass2Num;
    int i, Class;
    // start the classes
    vRes = Vec_PtrAlloc( 1000 );
    // set polarity of the nodes
    Abc_NtkDressMapSetPolarity( pNtk1 );
    Abc_NtkDressMapSetPolarity( pNtk2 );
    // create mapping of node IDs of pNtk1/pNtk2 into the IDs of equiv classes of pMiter
    vId2Lit1 = Abc_NtkDressMapClasses( pMiter, pNtk1 );
    vId2Lit2 = Abc_NtkDressMapClasses( pMiter, pNtk2 );
    // count the number of nodes in each equivalence class
    vCounts0 = Vec_IntStart( Aig_ManObjNumMax(pMiter) );
    Vec_IntForEachEntry( vId2Lit1, Class, i )
        if ( Class >= 0 )
            Vec_IntAddToEntry( vCounts0, Class, 1 );
    vCounts1 = Vec_IntStart( Aig_ManObjNumMax(pMiter) );
    Vec_IntForEachEntry( vId2Lit2, Class, i )
        if ( Class >= 0 )
            Vec_IntAddToEntry( vCounts1, Class, 1 );
    // get the costant class
    vClassC = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vId2Lit1, Class, i )
        if ( Class == 0 )
            Vec_IntPush( vClassC, Abc_ObjDressMakeId(pNtk1, i, 0) );
    Vec_IntForEachEntry( vId2Lit2, Class, i )
        if ( Class == 0 )
            Vec_IntPush( vClassC, Abc_ObjDressMakeId(pNtk2, i, 1) );
    Vec_PtrPush( vRes, vClassC );
    // map repr node IDs into class numbers
    vClass2Num = Vec_IntAlloc( 0 );
    Vec_IntFill( vClass2Num, Aig_ManObjNumMax(pMiter), -1 );
    // keep classes having at least one element from pNtk1 and one from pNtk2
    Vec_IntForEachEntry( vId2Lit1, Class, i )
        if ( Class > 0 && Vec_IntEntry(vCounts0, Class) && Vec_IntEntry(vCounts1, Class) )
            Vec_IntPush( Abc_ObjDressClass(vRes, vClass2Num, Class), Abc_ObjDressMakeId(pNtk1, i, 0) );
    Vec_IntForEachEntry( vId2Lit2, Class, i )
        if ( Class > 0 && Vec_IntEntry(vCounts0, Class) && Vec_IntEntry(vCounts1, Class) )
            Vec_IntPush( Abc_ObjDressClass(vRes, vClass2Num, Class), Abc_ObjDressMakeId(pNtk2, i, 1) );
    // package them accordingly
    Vec_IntFree( vClass2Num );
    Vec_IntFree( vCounts0 );
    Vec_IntFree( vCounts1 );
    Vec_IntFree( vId2Lit1 );
    Vec_IntFree( vId2Lit2 );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Computes equivalence classes of objects in pNtk1 and pNtk2.]

  Description [Returns the array (Vec_Ptr_t) of integer arrays (Vec_Int_t).
  Each of the integer arrays contains entries of one equivalence class.
  Each entry contains the following information: the network number (0/1),
  the polarity (0/1) and the object ID in the the network (0 <= num < MaxId)
  where MaxId is the largest number of an ID of an object in that network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkDressComputeEquivs( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nConflictLimit, int fVerbose )
{
    Dch_Pars_t Pars, * pPars = &Pars;
    Abc_Ntk_t * pAig1, * pAig2;
    Aig_Man_t * pMan1, * pMan2, * pMiter;
    Vec_Ptr_t * vRes;
    assert( !Abc_NtkIsStrash(pNtk1) );
    assert( !Abc_NtkIsStrash(pNtk2) );
    // convert network into AIG
    pAig1 = Abc_NtkStrash( pNtk1, 1, 1, 0 );
    pAig2 = Abc_NtkStrash( pNtk2, 1, 1, 0 );
    pMan1 = Abc_NtkToDar( pAig1, 0, 0 );
    pMan2 = Abc_NtkToDar( pAig2, 0, 0 );
    // derive the miter
    pMiter = Aig_ManCreateDualOutputMiter( pMan1, pMan2 );
    // set up parameters for SAT sweeping
    Dch_ManSetDefaultParams( pPars );
    pPars->nBTLimit = nConflictLimit;
    pPars->fVerbose = fVerbose;
    // perform SAT sweeping
    Dch_ComputeEquivalences( pMiter, pPars );
    // now, pMiter is annotated with the equivl class info
    // convert this info into the resulting array
    vRes = Abc_NtkDressMapIds( pMiter, pNtk1, pNtk2 );
    Aig_ManStop( pMiter );
    Aig_ManStop( pMan1 );
    Aig_ManStop( pMan2 );
    Abc_NtkDelete( pAig1 );
    Abc_NtkDelete( pAig2 );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Prints information about node equivalences.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDressPrintEquivs( Vec_Ptr_t * vRes )
{
    Vec_Int_t * vClass;
    int i, k, Entry;
    Vec_PtrForEachEntry( Vec_Int_t *, vRes, vClass, i )
    {
        printf( "Class %5d : ", i );
        printf( "Num =%5d    ", Vec_IntSize(vClass) );
        Vec_IntForEachEntry( vClass, Entry, k )
            printf( "%5d%c%d ", 
                Abc_ObjEquivId2ObjId(Entry), 
                Abc_ObjEquivId2Polar(Entry)? '-':'+', 
                Abc_ObjEquivId2NtkId(Entry) );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Prints information about node equivalences.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDressPrintStats( Vec_Ptr_t * vRes, int nNodes0, int nNodes1, abctime Time )
{
    Vec_Int_t * vClass;
    int i, k, Entry;
    int NegAll[2] = {0}, PosAll[2] = {0}, PairsAll = 0, PairsOne = 0;
    int Pos[2], Neg[2];
    // count the number of equivalences in each class
    Vec_PtrForEachEntry( Vec_Int_t *, vRes, vClass, i )
    {
        Pos[0] = Pos[1] = 0;
        Neg[0] = Neg[1] = 0;
        Vec_IntForEachEntry( vClass, Entry, k )
        {
            if ( Abc_ObjEquivId2NtkId(Entry) )
            {
                if ( Abc_ObjEquivId2Polar(Entry) )
                    Neg[1]++; // negative polarity in network 1
                else
                    Pos[1]++; // positive polarity in network 1
            }
            else
            {
                if ( Abc_ObjEquivId2Polar(Entry) )
                    Neg[0]++; // negative polarity in network 0
                else
                    Pos[0]++; // positive polarity in network 0
            }
        }
        PosAll[0] += Pos[0]; // total positive polarity in network 0
        PosAll[1] += Pos[1]; // total positive polarity in network 1
        NegAll[0] += Neg[0]; // total negative polarity in network 0
        NegAll[1] += Neg[1]; // total negative polarity in network 1

        // assuming that the name can be transferred to only one node
        PairsAll += Abc_MinInt(Neg[0] + Pos[0], Neg[1] + Pos[1]);
        PairsOne += Abc_MinInt(Neg[0], Neg[1]) + Abc_MinInt(Pos[0], Pos[1]);
    }
    printf( "Total number of equiv classes                = %7d.\n", Vec_PtrSize(vRes) );
    printf( "Participating nodes from both networks       = %7d.\n", NegAll[0]+PosAll[0]+NegAll[1]+PosAll[1] );
    printf( "Participating nodes from the first network   = %7d. (%7.2f %% of nodes)\n", NegAll[0]+PosAll[0], 100.0*(NegAll[0]+PosAll[0])/(nNodes0+1) );
    printf( "Participating nodes from the second network  = %7d. (%7.2f %% of nodes)\n", NegAll[1]+PosAll[1], 100.0*(NegAll[1]+PosAll[1])/(nNodes1+1) );
    printf( "Node pairs (any polarity)                    = %7d. (%7.2f %% of names can be moved)\n", PairsAll, 100.0*PairsAll/(nNodes0+1) );
    printf( "Node pairs (same polarity)                   = %7d. (%7.2f %% of names can be moved)\n", PairsOne, 100.0*PairsOne/(nNodes0+1) );
    ABC_PRT( "Total runtime", Time );
}

/**Function*************************************************************

  Synopsis    [Transfers IDs from pNtk1 to pNtk2 using equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDress2Transfer( Abc_Ntk_t * pNtk0, Abc_Ntk_t * pNtk1, Vec_Ptr_t * vRes, int fVerbose )
{
    Vec_Int_t * vClass;
    Abc_Obj_t * pObj0, * pObj1;
    int i, k, fComp0, fComp1, Entry;
    int CounterInv = 0, Counter = 0;
    char * pName;
    Vec_PtrForEachEntry( Vec_Int_t *, vRes, vClass, i )
    {
        pObj0 = pObj1 = NULL;
        fComp0 = fComp1 = 0;
        Vec_IntForEachEntry( vClass, Entry, k )
        {
            if ( Abc_ObjEquivId2NtkId(Entry) )
            {
                pObj1 = Abc_NtkObj( pNtk1, Abc_ObjEquivId2ObjId(Entry) );
                fComp1 = Abc_ObjEquivId2Polar(Entry);
            }
            else
            {
                pObj0 = Abc_NtkObj( pNtk0, Abc_ObjEquivId2ObjId(Entry) );
                fComp0 = Abc_ObjEquivId2Polar(Entry);
            }
        }
        if ( pObj0 == NULL || pObj1 == NULL )
            continue;
        // if the node already has a name, quit
        pName = Nm_ManFindNameById( pNtk0->pManName, pObj0->Id );
        if ( pName != NULL )
            continue;
        // if the other node has no name, quit
        pName = Nm_ManFindNameById( pNtk1->pManName, pObj1->Id );
        if ( pName == NULL )
            continue;
        // assign name
        if ( fComp0 ^ fComp1 )
        {
            Abc_ObjAssignName( pObj0, pName, "_inv" );
            CounterInv++;
        }
        else
        {
            Abc_ObjAssignName( pObj0, pName, NULL );
            Counter++;
        }
    }
    if ( fVerbose )
    {
        printf( "Total number of names assigned  = %5d. (Dir = %5d. Compl = %5d.)\n", 
            Counter + CounterInv, Counter, CounterInv );
    }
}

/**Function*************************************************************

  Synopsis    [Transfers names from pNtk1 to pNtk2.]

  Description [Internally calls new procedure for mapping node IDs of
  both networks into the shared equivalence classes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDress2( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nConflictLimit, int fVerbose )
{
    Vec_Ptr_t * vRes;
    abctime clk = Abc_Clock();
    vRes = Abc_NtkDressComputeEquivs( pNtk1, pNtk2, nConflictLimit, fVerbose );
//    Abc_NtkDressPrintEquivs( vRes );
    Abc_NtkDressPrintStats( vRes, Abc_NtkNodeNum(pNtk1), Abc_NtkNodeNum(pNtk1), Abc_Clock() - clk );
    Abc_NtkDress2Transfer( pNtk1, pNtk2, vRes, fVerbose );
    Vec_VecFree( (Vec_Vec_t *)vRes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

