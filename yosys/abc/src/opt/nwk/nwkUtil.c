/**CFile****************************************************************

  FileName    [nwkUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Logic network representation.]

  Synopsis    [Various utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nwkUtil.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <math.h>
#include "nwk.h"
#include "bool/kit/kit.h"

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
void Nwk_ManIncrementTravId( Nwk_Man_t * pNtk )
{
    Nwk_Obj_t * pObj;
    int i;
    if ( pNtk->nTravIds >= (1<<26)-1 )
    {
        pNtk->nTravIds = 0;
        Nwk_ManForEachObj( pNtk, pObj, i )
            pObj->TravId = 0;
    }
    pNtk->nTravIds++;
}

/**Function*************************************************************

  Synopsis    [Reads the maximum number of fanins of a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManGetFaninMax( Nwk_Man_t * pNtk )
{
    Nwk_Obj_t * pNode;
    int i, nFaninsMax = 0;
    Nwk_ManForEachNode( pNtk, pNode, i )
    {
        if ( nFaninsMax < Nwk_ObjFaninNum(pNode) )
            nFaninsMax = Nwk_ObjFaninNum(pNode);
    }
    return nFaninsMax;
}

/**Function*************************************************************

  Synopsis    [Reads the total number of all fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManGetTotalFanins( Nwk_Man_t * pNtk )
{
    Nwk_Obj_t * pNode;
    int i, nFanins = 0;
    Nwk_ManForEachNode( pNtk, pNode, i )
        nFanins += Nwk_ObjFaninNum(pNode);
    return nFanins;
}


/**Function*************************************************************

  Synopsis    [Returns the number of true PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManPiNum( Nwk_Man_t * pNtk )
{
    Nwk_Obj_t * pNode;
    int i, Counter = 0;
    Nwk_ManForEachCi( pNtk, pNode, i )
        Counter += Nwk_ObjIsPi( pNode );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns the number of true POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManPoNum( Nwk_Man_t * pNtk )
{
    Nwk_Obj_t * pNode;
    int i, Counter = 0;
    Nwk_ManForEachCo( pNtk, pNode, i )
        Counter += Nwk_ObjIsPo( pNode );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Reads the number of AIG nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManGetAigNodeNum( Nwk_Man_t * pNtk )
{
    Nwk_Obj_t * pNode;
    int i, nNodes = 0;
    Nwk_ManForEachNode( pNtk, pNode, i )
    {
        if ( pNode->pFunc == NULL )
        {
            printf( "Nwk_ManGetAigNodeNum(): Local AIG of node %d is not assigned.\n", pNode->Id );
            continue;
        }
        if ( Nwk_ObjFaninNum(pNode) < 2 )
            continue;
        nNodes += Hop_DagSize( pNode->pFunc );
    }
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Procedure used for sorting the nodes in increasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_NodeCompareLevelsIncrease( Nwk_Obj_t ** pp1, Nwk_Obj_t ** pp2 )
{
    int Diff = (*pp1)->Level - (*pp2)->Level;
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Procedure used for sorting the nodes in decreasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_NodeCompareLevelsDecrease( Nwk_Obj_t ** pp1, Nwk_Obj_t ** pp2 )
{
    int Diff = (*pp1)->Level - (*pp2)->Level;
    if ( Diff > 0 )
        return -1;
    if ( Diff < 0 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Prints the objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ObjPrint( Nwk_Obj_t * pObj )
{
    Nwk_Obj_t * pNext;
    int i;
    printf( "ObjId = %5d.  ", pObj->Id );
    if ( Nwk_ObjIsPi(pObj) )
        printf( "PI" );
    if ( Nwk_ObjIsPo(pObj) )
        printf( "PO" );
    if ( Nwk_ObjIsNode(pObj) )
        printf( "Node" );
    printf( "   Fanins = " );
    Nwk_ObjForEachFanin( pObj, pNext, i )
        printf( "%d ", pNext->Id );
    printf( "   Fanouts = " );
    Nwk_ObjForEachFanout( pObj, pNext, i )
        printf( "%d ", pNext->Id );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Dumps the BLIF file for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManDumpBlif( Nwk_Man_t * pNtk, char * pFileName, Vec_Ptr_t * vPiNames, Vec_Ptr_t * vPoNames )
{
    FILE * pFile;
    Vec_Ptr_t * vNodes;
    Vec_Int_t * vTruth;
    Vec_Int_t * vCover;
    Nwk_Obj_t * pObj, * pFanin;
    Aig_MmFlex_t * pMem;
    char * pSop = NULL;
    unsigned * pTruth;
    int i, k, nDigits;
    if ( Nwk_ManPoNum(pNtk) == 0 )
    {
        printf( "Nwk_ManDumpBlif(): Network does not have POs.\n" );
        return;
    }
    // collect nodes in the DFS order
    nDigits = Abc_Base10Log( Nwk_ManObjNumMax(pNtk) );
    // write the file 
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# BLIF file written by procedure Nwk_ManDumpBlif()\n" );
//    fprintf( pFile, "# http://www.eecs.berkeley.edu/~alanmi/abc/\n" );
    fprintf( pFile, ".model %s\n", pNtk->pName );
    // write PIs
    fprintf( pFile, ".inputs" );
    Nwk_ManForEachCi( pNtk, pObj, i )
        if ( vPiNames )
            fprintf( pFile, " %s", (char*)Vec_PtrEntry(vPiNames, i) );
        else
            fprintf( pFile, " n%0*d", nDigits, pObj->Id );
    fprintf( pFile, "\n" );
    // write POs
    fprintf( pFile, ".outputs" );
    Nwk_ManForEachCo( pNtk, pObj, i )
        if ( vPoNames )
            fprintf( pFile, " %s", (char*)Vec_PtrEntry(vPoNames, i) );
        else
            fprintf( pFile, " n%0*d", nDigits, pObj->Id );
    fprintf( pFile, "\n" );
    // write nodes
    pMem = Aig_MmFlexStart();
    vTruth = Vec_IntAlloc( 1 << 16 );
    vCover = Vec_IntAlloc( 1 << 16 );
    vNodes = Nwk_ManDfs( pNtk );
    Vec_PtrForEachEntry( Nwk_Obj_t *, vNodes, pObj, i )
    {
        if ( !Nwk_ObjIsNode(pObj) )
            continue;
        // derive SOP for the AIG
        pTruth = Hop_ManConvertAigToTruth( pNtk->pManHop, Hop_Regular(pObj->pFunc), Nwk_ObjFaninNum(pObj), vTruth, 0 );
        if ( Hop_IsComplement(pObj->pFunc) )
            Kit_TruthNot( pTruth, pTruth, Nwk_ObjFaninNum(pObj) );
        pSop = Kit_PlaFromTruth( pMem, pTruth, Nwk_ObjFaninNum(pObj), vCover );
        // write the node
        fprintf( pFile, ".names" );
        if ( !Kit_TruthIsConst0(pTruth, Nwk_ObjFaninNum(pObj)) && !Kit_TruthIsConst1(pTruth, Nwk_ObjFaninNum(pObj)) )
        {
            Nwk_ObjForEachFanin( pObj, pFanin, k )
                if ( vPiNames && Nwk_ObjIsPi(pFanin) )
                    fprintf( pFile, " %s", (char*)Vec_PtrEntry(vPiNames, Nwk_ObjPioNum(pFanin)) );
                else
                    fprintf( pFile, " n%0*d", nDigits, pFanin->Id );
        }
        fprintf( pFile, " n%0*d\n", nDigits, pObj->Id );
        // write the function
        fprintf( pFile, "%s", pSop );
    }
    Vec_IntFree( vCover );
    Vec_IntFree( vTruth );
    Vec_PtrFree( vNodes );
    Aig_MmFlexStop( pMem, 0 );
    // write POs
    Nwk_ManForEachCo( pNtk, pObj, i )
    {
        fprintf( pFile, ".names" );
        if ( vPiNames && Nwk_ObjIsPi(Nwk_ObjFanin0(pObj)) )
            fprintf( pFile, " %s", (char*)Vec_PtrEntry(vPiNames, Nwk_ObjPioNum(Nwk_ObjFanin0(pObj))) );
        else
            fprintf( pFile, " n%0*d", nDigits, Nwk_ObjFanin0(pObj)->Id );
        if ( vPoNames )
            fprintf( pFile, " %s\n", (char*)Vec_PtrEntry(vPoNames, Nwk_ObjPioNum(pObj)) );
        else
            fprintf( pFile, " n%0*d\n", nDigits, pObj->Id );
        fprintf( pFile, "%d 1\n", !pObj->fInvert );
    }
    fprintf( pFile, ".end\n\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Prints the distribution of fanins/fanouts in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManPrintFanioNew( Nwk_Man_t * pNtk )
{
    char Buffer[100];
    Nwk_Obj_t * pNode;
    Vec_Int_t * vFanins, * vFanouts;
    int nFanins, nFanouts, nFaninsMax, nFanoutsMax, nFaninsAll, nFanoutsAll;
    int i, k, nSizeMax;

    // determine the largest fanin and fanout
    nFaninsMax = nFanoutsMax = 0;
    nFaninsAll = nFanoutsAll = 0;
    Nwk_ManForEachNode( pNtk, pNode, i )
    {
        nFanins  = Nwk_ObjFaninNum(pNode);
        nFanouts = Nwk_ObjFanoutNum(pNode);
        nFaninsAll  += nFanins;
        nFanoutsAll += nFanouts;
        nFaninsMax   = Abc_MaxInt( nFaninsMax, nFanins );
        nFanoutsMax  = Abc_MaxInt( nFanoutsMax, nFanouts );
    }

    // allocate storage for fanin/fanout numbers
    nSizeMax = Abc_MaxInt( 10 * (Abc_Base10Log(nFaninsMax) + 1), 10 * (Abc_Base10Log(nFanoutsMax) + 1) );
    vFanins  = Vec_IntStart( nSizeMax );
    vFanouts = Vec_IntStart( nSizeMax );

    // count the number of fanins and fanouts
    Nwk_ManForEachNode( pNtk, pNode, i )
    {
        nFanins  = Nwk_ObjFaninNum(pNode);
        nFanouts = Nwk_ObjFanoutNum(pNode);
//        nFanouts = Nwk_NodeMffcSize(pNode);

        if ( nFanins < 10 )
            Vec_IntAddToEntry( vFanins, nFanins, 1 );
        else if ( nFanins < 100 )
            Vec_IntAddToEntry( vFanins, 10 + nFanins/10, 1 );
        else if ( nFanins < 1000 )
            Vec_IntAddToEntry( vFanins, 20 + nFanins/100, 1 );
        else if ( nFanins < 10000 )
            Vec_IntAddToEntry( vFanins, 30 + nFanins/1000, 1 );
        else if ( nFanins < 100000 )
            Vec_IntAddToEntry( vFanins, 40 + nFanins/10000, 1 );
        else if ( nFanins < 1000000 )
            Vec_IntAddToEntry( vFanins, 50 + nFanins/100000, 1 );
        else if ( nFanins < 10000000 )
            Vec_IntAddToEntry( vFanins, 60 + nFanins/1000000, 1 );

        if ( nFanouts < 10 )
            Vec_IntAddToEntry( vFanouts, nFanouts, 1 );
        else if ( nFanouts < 100 )
            Vec_IntAddToEntry( vFanouts, 10 + nFanouts/10, 1 );
        else if ( nFanouts < 1000 )
            Vec_IntAddToEntry( vFanouts, 20 + nFanouts/100, 1 );
        else if ( nFanouts < 10000 )
            Vec_IntAddToEntry( vFanouts, 30 + nFanouts/1000, 1 );
        else if ( nFanouts < 100000 )
            Vec_IntAddToEntry( vFanouts, 40 + nFanouts/10000, 1 );
        else if ( nFanouts < 1000000 )
            Vec_IntAddToEntry( vFanouts, 50 + nFanouts/100000, 1 );
        else if ( nFanouts < 10000000 )
            Vec_IntAddToEntry( vFanouts, 60 + nFanouts/1000000, 1 );
    }

    printf( "The distribution of fanins and fanouts in the network:\n" );
    printf( "         Number   Nodes with fanin  Nodes with fanout\n" );
    for ( k = 0; k < nSizeMax; k++ )
    {
        if ( vFanins->pArray[k] == 0 && vFanouts->pArray[k] == 0 )
            continue;
        if ( k < 10 )
            printf( "%15d : ", k );
        else
        {
            sprintf( Buffer, "%d - %d", (int)pow((double)10, k/10) * (k%10), (int)pow((double)10, k/10) * (k%10+1) - 1 ); 
            printf( "%15s : ", Buffer );
        }
        if ( vFanins->pArray[k] == 0 )
            printf( "              " );
        else
            printf( "%12d  ", vFanins->pArray[k] );
        printf( "    " );
        if ( vFanouts->pArray[k] == 0 )
            printf( "              " );
        else
            printf( "%12d  ", vFanouts->pArray[k] );
        printf( "\n" );
    }
    Vec_IntFree( vFanins );
    Vec_IntFree( vFanouts );

    printf( "Fanins: Max = %d. Ave = %.2f.  Fanouts: Max = %d. Ave =  %.2f.\n", 
        nFaninsMax,  1.0*nFaninsAll/Nwk_ManNodeNum(pNtk), 
        nFanoutsMax, 1.0*nFanoutsAll/Nwk_ManNodeNum(pNtk)  );
}

/**Function*************************************************************

  Synopsis    [Cleans the temporary marks of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManCleanMarks( Nwk_Man_t * pMan )
{
    Nwk_Obj_t * pObj;
    int i;
    Nwk_ManForEachObj( pMan, pObj, i )
        pObj->MarkA = pObj->MarkB = 0;
}

/**Function*************************************************************

  Synopsis    [Minimizes the support of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManMinimumBaseNode( Nwk_Obj_t * pObj, Vec_Int_t * vTruth, int fVerbose )
{
    unsigned * pTruth;
    Nwk_Obj_t * pFanin, * pObjNew;
    Nwk_Man_t * pNtk = pObj->pMan;
    int uSupp, nSuppSize, k, Counter = 0;
    pTruth = Hop_ManConvertAigToTruth( pNtk->pManHop, Hop_Regular(pObj->pFunc), Nwk_ObjFaninNum(pObj), vTruth, 0 );
    nSuppSize = Kit_TruthSupportSize(pTruth, Nwk_ObjFaninNum(pObj));
    if ( nSuppSize == Nwk_ObjFaninNum(pObj) )
        return 0;
    Counter++;
    uSupp = Kit_TruthSupport( pTruth, Nwk_ObjFaninNum(pObj) );
    // create new node with the given support
    pObjNew = Nwk_ManCreateNode( pNtk, nSuppSize, Nwk_ObjFanoutNum(pObj) );
    Nwk_ObjForEachFanin( pObj, pFanin, k )
        if ( uSupp & (1 << k) )
            Nwk_ObjAddFanin( pObjNew, pFanin );
    pObjNew->pFunc = Hop_Remap( pNtk->pManHop, pObj->pFunc, uSupp, Nwk_ObjFaninNum(pObj) );
    if ( fVerbose )
        printf( "Reducing node %d fanins from %d to %d.\n", 
            pObj->Id, Nwk_ObjFaninNum(pObj), Nwk_ObjFaninNum(pObjNew) );
    Nwk_ObjReplace( pObj, pObjNew );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Minimizes the support of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManMinimumBaseInt( Nwk_Man_t * pNtk, int fVerbose )
{
    Vec_Int_t * vTruth;
    Nwk_Obj_t * pObj;
    int i, Counter = 0;
    vTruth = Vec_IntAlloc( 1 << 16 );
    Nwk_ManForEachNode( pNtk, pObj, i )
        Counter += Nwk_ManMinimumBaseNode( pObj, vTruth, fVerbose );
    if ( fVerbose && Counter )
        printf( "Support minimization reduced support of %d nodes.\n", Counter );
    Vec_IntFree( vTruth );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Minimizes the support of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManMinimumBaseRec( Nwk_Man_t * pNtk, int fVerbose )
{
    int i;
    abctime clk = Abc_Clock();
    for ( i = 0; Nwk_ManMinimumBaseInt( pNtk, fVerbose ); i++ );
    ABC_PRT( "Minbase", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    [Minimizes the support of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManMinimumBase( Nwk_Man_t * pNtk, int fVerbose )
{
    Vec_Int_t * vTruth;
    Nwk_Obj_t * pObj;
    int i, Counter = 0;
    vTruth = Vec_IntAlloc( 1 << 16 );
    Nwk_ManForEachNode( pNtk, pObj, i )
        Counter += Nwk_ManMinimumBaseNode( pObj, vTruth, fVerbose );
    if ( fVerbose && Counter )
        printf( "Support minimization reduced support of %d nodes.\n", Counter );
    Vec_IntFree( vTruth );
}

/**Function*************************************************************

  Synopsis    [Minimizes the support of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManRemoveDupFaninsNode( Nwk_Obj_t * pObj, int iFan0, int iFan1, Vec_Int_t * vTruth )
{
    Hop_Man_t * pManHop = pObj->pMan->pManHop;
//    Nwk_Obj_t * pFanin0 = pObj->pFanio[iFan0];
//    Nwk_Obj_t * pFanin1 = pObj->pFanio[iFan1];
    assert( pObj->pFanio[iFan0] == pObj->pFanio[iFan1] );
    pObj->pFunc = Hop_Compose( pManHop, pObj->pFunc, Hop_IthVar(pManHop,iFan0), iFan1 );
    Nwk_ManMinimumBaseNode( pObj, vTruth, 0 );
}

/**Function*************************************************************

  Synopsis    [Minimizes the support of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManRemoveDupFanins( Nwk_Man_t * pNtk, int fVerbose )
{
    Vec_Int_t * vTruth;
    Nwk_Obj_t * pObj;
    int i, k, m, fFound;
    // check if the nodes have duplicated fanins
    vTruth = Vec_IntAlloc( 1 << 16 );
    Nwk_ManForEachNode( pNtk, pObj, i )
    {
        fFound = 0;
        for ( k = 0; k < pObj->nFanins; k++ )
        {
            for ( m = k + 1; m < pObj->nFanins; m++ )
                if ( pObj->pFanio[k] == pObj->pFanio[m] )
                {
                    if ( fVerbose )
                        printf( "Removing duplicated fanins of node %d (fanins %d and %d).\n", 
                            pObj->Id, pObj->pFanio[k]->Id, pObj->pFanio[m]->Id );
                    Nwk_ManRemoveDupFaninsNode( pObj, k, m, vTruth );
                    fFound = 1;
                    break;
                }
            if ( fFound )
                break;
        }
    }
    Vec_IntFree( vTruth );
//    Nwk_ManMinimumBase( pNtk, fVerbose );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

