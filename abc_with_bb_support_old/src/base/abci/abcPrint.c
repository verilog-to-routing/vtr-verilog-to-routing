/**CFile****************************************************************

  FileName    [abcPrint.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Printing statistics.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcPrint.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "dec.h"
#include "main.h"
#include "mio.h"
//#include "seq.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

//extern int s_TotalNodes = 0;
//extern int s_TotalChanges = 0;

int s_MappingTime = 0;
int s_MappingMem = 0;
int s_ResubTime = 0;
int s_ResynTime = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Print the vital stats of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintStats( FILE * pFile, Abc_Ntk_t * pNtk, int fFactored )
{
    int Num;

//    if ( Abc_NtkIsStrash(pNtk) )
//        Abc_AigCountNext( pNtk->pManFunc );

    fprintf( pFile, "%-13s:",       pNtk->pName );
    if ( Abc_NtkAssertNum(pNtk) )
        fprintf( pFile, " i/o/a = %4d/%4d/%4d", Abc_NtkPiNum(pNtk), Abc_NtkPoNum(pNtk), Abc_NtkAssertNum(pNtk) );
    else
        fprintf( pFile, " i/o = %4d/%4d", Abc_NtkPiNum(pNtk), Abc_NtkPoNum(pNtk) );
    fprintf( pFile, "  lat = %4d", Abc_NtkLatchNum(pNtk) );
    if ( Abc_NtkIsNetlist(pNtk) )
    {
        fprintf( pFile, "  net = %5d", Abc_NtkNetNum(pNtk) );
        fprintf( pFile, "  nd = %5d",  Abc_NtkNodeNum(pNtk) );
        fprintf( pFile, "  wbox = %3d", Abc_NtkWhiteboxNum(pNtk) );
        fprintf( pFile, "  bbox = %3d", Abc_NtkBlackboxNum(pNtk) );
    }
    else if ( Abc_NtkIsStrash(pNtk) )
    {        
        fprintf( pFile, "  and = %5d", Abc_NtkNodeNum(pNtk) );
        if ( Num = Abc_NtkGetChoiceNum(pNtk) )
            fprintf( pFile, " (choice = %d)", Num );
        if ( Num = Abc_NtkGetExorNum(pNtk) )
            fprintf( pFile, " (exor = %d)", Num );
//        if ( Num2 = Abc_NtkGetMuxNum(pNtk) )
//            fprintf( pFile, " (mux = %d)", Num2-Num );
//        if ( Num2 )
//            fprintf( pFile, " (other = %d)", Abc_NtkNodeNum(pNtk)-3*Num2 );
    }
    else 
    {
        fprintf( pFile, "  nd = %5d", Abc_NtkNodeNum(pNtk) );
        fprintf( pFile, "  net = %5d", Abc_NtkGetTotalFanins(pNtk) );
    }

    if ( Abc_NtkIsStrash(pNtk) || Abc_NtkIsNetlist(pNtk) )
    {
    }
    else if ( Abc_NtkHasSop(pNtk) )   
    {

        fprintf( pFile, "  cube = %5d",  Abc_NtkGetCubeNum(pNtk) );
//        fprintf( pFile, "  lit(sop) = %5d",  Abc_NtkGetLitNum(pNtk) );
        if ( fFactored )
            fprintf( pFile, "  lit(fac) = %5d",  Abc_NtkGetLitFactNum(pNtk) );
    }
    else if ( Abc_NtkHasAig(pNtk) )
        fprintf( pFile, "  aig  = %5d",  Abc_NtkGetAigNodeNum(pNtk) );
    else if ( Abc_NtkHasBdd(pNtk) )
        fprintf( pFile, "  bdd  = %5d",  Abc_NtkGetBddNodeNum(pNtk) );
    else if ( Abc_NtkHasMapping(pNtk) )
    {
        fprintf( pFile, "  area = %5.2f", Abc_NtkGetMappedArea(pNtk) );
        fprintf( pFile, "  delay = %5.2f", Abc_NtkDelayTrace(pNtk) );
    }
    else if ( !Abc_NtkHasBlackbox(pNtk) )
    {
        assert( 0 );
    }

    if ( Abc_NtkIsStrash(pNtk) )
        fprintf( pFile, "  lev = %3d", Abc_AigLevel(pNtk) );
    else 
        fprintf( pFile, "  lev = %3d", Abc_NtkLevel(pNtk) );

    fprintf( pFile, "\n" );

//    Abc_NtkCrossCut( pNtk );

    // print the statistic into a file
/*
    {
        FILE * pTable;
        pTable = fopen( "iscas/seqmap__stats.txt", "a+" );
        fprintf( pTable, "%s ",  pNtk->pName );
        fprintf( pTable, "%d ", Abc_NtkPiNum(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkPoNum(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkLatchNum(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkNodeNum(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkLevel(pNtk) );
        fprintf( pTable, "\n" );
        fclose( pTable );
    }
*/
/*
    // print the statistic into a file
    {
        FILE * pTable;
        pTable = fopen( "stats.txt", "a+" );
        fprintf( pTable, "%s ",  pNtk->pSpec );
        fprintf( pTable, "%.0f ", Abc_NtkGetMappedArea(pNtk) );
        fprintf( pTable, "%.2f ", Abc_NtkDelayTrace(pNtk) );
        fprintf( pTable, "\n" );
        fclose( pTable );
    }
*/

/*
    // print the statistic into a file
    {
        FILE * pTable;
        pTable = fopen( "x/stats_new.txt", "a+" );
        fprintf( pTable, "%s ",  pNtk->pName );
//        fprintf( pTable, "%d ", Abc_NtkPiNum(pNtk) );
//        fprintf( pTable, "%d ", Abc_NtkPoNum(pNtk) );
//        fprintf( pTable, "%d ", Abc_NtkLevel(pNtk) );
//        fprintf( pTable, "%d ", Abc_NtkNodeNum(pNtk) );
//        fprintf( pTable, "%d ", Abc_NtkGetTotalFanins(pNtk) );
//        fprintf( pTable, "%d ", Abc_NtkLatchNum(pNtk) );
//        fprintf( pTable, "%.2f ", (float)(s_MappingMem)/(float)(1<<20) );
        fprintf( pTable, "%.2f", (float)(s_MappingTime)/(float)(CLOCKS_PER_SEC) );
//        fprintf( pTable, "%.2f", (float)(s_ResynTime)/(float)(CLOCKS_PER_SEC) );
        fprintf( pTable, "\n" );
        fclose( pTable );

        s_ResynTime = 0;
    }
*/

/*
    // print the statistic into a file
    {
        static int Counter = 0;
        extern int timeRetime;
        FILE * pTable;
        Counter++;
        pTable = fopen( "a/ret__stats.txt", "a+" );
        fprintf( pTable, "%s ", pNtk->pName );
        fprintf( pTable, "%d ", Abc_NtkNodeNum(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkLatchNum(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkLevel(pNtk) );
        fprintf( pTable, "%.2f ", (float)(timeRetime)/(float)(CLOCKS_PER_SEC) );
        if ( Counter % 4 == 0 )
            fprintf( pTable, "\n" );
        fclose( pTable );
    }
*/

/*
    // print the statistic into a file
    {
        static int Counter = 0;
        extern int timeRetime;
        FILE * pTable;
        Counter++;
        pTable = fopen( "d/stats.txt", "a+" );
        fprintf( pTable, "%s ", pNtk->pName );
//        fprintf( pTable, "%d ", Abc_NtkPiNum(pNtk) );
//        fprintf( pTable, "%d ", Abc_NtkPoNum(pNtk) );
//        fprintf( pTable, "%d ", Abc_NtkLatchNum(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkNodeNum(pNtk) );
        fprintf( pTable, "%.2f ", (float)(timeRetime)/(float)(CLOCKS_PER_SEC) );
        fprintf( pTable, "\n" );
        fclose( pTable );
    }
*/

/*
    s_TotalNodes += Abc_NtkNodeNum(pNtk);
    printf( "Total nodes = %6d   %6.2f Mb   Changes = %6d.\n", 
        s_TotalNodes, s_TotalNodes * 20.0 / (1<<20), s_TotalChanges );
*/
}

/**Function*************************************************************

  Synopsis    [Prints PIs/POs and LIs/LOs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintIo( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;

    fprintf( pFile, "Primary inputs (%d): ", Abc_NtkPiNum(pNtk) );    
    Abc_NtkForEachPi( pNtk, pObj, i )
        fprintf( pFile, " %s", Abc_ObjName(pObj) );
//        fprintf( pFile, " %s(%d)", Abc_ObjName(pObj), Abc_ObjFanoutNum(pObj) );
    fprintf( pFile, "\n" );   

    fprintf( pFile, "Primary outputs (%d):", Abc_NtkPoNum(pNtk) );    
    Abc_NtkForEachPo( pNtk, pObj, i )
        fprintf( pFile, " %s", Abc_ObjName(pObj) );
    fprintf( pFile, "\n" );    

    fprintf( pFile, "Latches (%d):  ", Abc_NtkLatchNum(pNtk) );  
    Abc_NtkForEachLatch( pNtk, pObj, i )
        fprintf( pFile, " %s(%s=%s)", Abc_ObjName(pObj), 
            Abc_ObjName(Abc_ObjFanout0(pObj)), Abc_ObjName(Abc_ObjFanin0(pObj)) );
    fprintf( pFile, "\n" );   
}

/**Function*************************************************************

  Synopsis    [Prints statistics about latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintLatch( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pLatch, * pFanin;
    int i, Counter0, Counter1, Counter2;
    int InitNums[4], Init;

    assert( !Abc_NtkIsNetlist(pNtk) );
    if ( Abc_NtkLatchNum(pNtk) == 0 )
    {
        fprintf( pFile, "The network is combinational.\n" );
        return;
    }

    for ( i = 0; i < 4; i++ )    
        InitNums[i] = 0;
    Counter0 = Counter1 = Counter2 = 0;
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        Init = Abc_LatchInit( pLatch );
        assert( Init < 4 );
        InitNums[Init]++;

        pFanin = Abc_ObjFanin0(Abc_ObjFanin0(pLatch));
        if ( Abc_NtkIsLogic(pNtk) )
        {
            if ( !Abc_NodeIsConst(pFanin) )
                continue;
        }
        else if ( Abc_NtkIsStrash(pNtk) )
        {
            if ( !Abc_AigNodeIsConst(pFanin) )
                continue;
        }
        else
            assert( 0 );

        // the latch input is a constant node
        Counter0++;
        if ( Abc_LatchIsInitDc(pLatch) )
        {
            Counter1++;
            continue;
        }
        // count the number of cases when the constant is equal to the initial value
        if ( Abc_NtkIsStrash(pNtk) )
        {
            if ( Abc_LatchIsInit1(pLatch) == !Abc_ObjFaninC0(pLatch) )
                Counter2++;
        }
        else
        {
            if ( Abc_LatchIsInit1(pLatch) == Abc_NodeIsConst1(pLatch) )
                Counter2++;
        }
    }
    fprintf( pFile, "%-15s:  ", pNtk->pName );
    fprintf( pFile, "Latch = %6d. No = %4d. Zero = %4d. One = %4d. DC = %4d.\n", 
        Abc_NtkLatchNum(pNtk), InitNums[0], InitNums[1], InitNums[2], InitNums[3] );
    fprintf( pFile, "Const fanin = %3d. DC init = %3d. Matching init = %3d. ", Counter0, Counter1, Counter2 );
    fprintf( pFile, "Self-feed latches = %2d.\n", Abc_NtkCountSelfFeedLatches(pNtk) );
}

/**Function*************************************************************

  Synopsis    [Prints the distribution of fanins/fanouts in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintFanio( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, k, nFanins, nFanouts;
    Vec_Int_t * vFanins, * vFanouts;
    int nOldSize, nNewSize;

    vFanins  = Vec_IntAlloc( 0 );
    vFanouts = Vec_IntAlloc( 0 );
    Vec_IntFill( vFanins,  100, 0 );
    Vec_IntFill( vFanouts, 100, 0 );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        nFanins  = Abc_ObjFaninNum(pNode);
        if ( Abc_NtkIsNetlist(pNtk) )
            nFanouts = Abc_ObjFanoutNum( Abc_ObjFanout0(pNode) );
        else
            nFanouts = Abc_ObjFanoutNum(pNode);
//            nFanouts = Abc_NodeMffcSize(pNode);
        if ( nFanins > vFanins->nSize || nFanouts > vFanouts->nSize )
        {
            nOldSize = vFanins->nSize;
            nNewSize = ABC_MAX(nFanins, nFanouts) + 10;
            Vec_IntGrow( vFanins,  nNewSize  );
            Vec_IntGrow( vFanouts, nNewSize );
            for ( k = nOldSize; k < nNewSize; k++ )
            {
                Vec_IntPush( vFanins,  0  );
                Vec_IntPush( vFanouts, 0 );
            }
        }
        vFanins->pArray[nFanins]++;
        vFanouts->pArray[nFanouts]++;
    }
    fprintf( pFile, "The distribution of fanins and fanouts in the network:\n" );
    fprintf( pFile, "  Number   Nodes with fanin  Nodes with fanout\n" );
    for ( k = 0; k < vFanins->nSize; k++ )
    {
        if ( vFanins->pArray[k] == 0 && vFanouts->pArray[k] == 0 )
            continue;
        fprintf( pFile, "%5d : ", k );
        if ( vFanins->pArray[k] == 0 )
            fprintf( pFile, "              " );
        else
            fprintf( pFile, "%12d  ", vFanins->pArray[k] );
        fprintf( pFile, "    " );
        if ( vFanouts->pArray[k] == 0 )
            fprintf( pFile, "              " );
        else
            fprintf( pFile, "%12d  ", vFanouts->pArray[k] );
        fprintf( pFile, "\n" );
    }
    Vec_IntFree( vFanins );
    Vec_IntFree( vFanouts );
}

/**Function*************************************************************

  Synopsis    [Prints the fanins/fanouts of a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodePrintFanio( FILE * pFile, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNode2;
    int i;
    if ( Abc_ObjIsPo(pNode) )
        pNode = Abc_ObjFanin0(pNode);

    fprintf( pFile, "Node %s", Abc_ObjName(pNode) );    
    fprintf( pFile, "\n" ); 

    fprintf( pFile, "Fanins (%d): ", Abc_ObjFaninNum(pNode) );    
    Abc_ObjForEachFanin( pNode, pNode2, i )
        fprintf( pFile, " %s", Abc_ObjName(pNode2) );
    fprintf( pFile, "\n" ); 
    
    fprintf( pFile, "Fanouts (%d): ", Abc_ObjFaninNum(pNode) );    
    Abc_ObjForEachFanout( pNode, pNode2, i )
        fprintf( pFile, " %s", Abc_ObjName(pNode2) );
    fprintf( pFile, "\n" );   
}

/**Function*************************************************************

  Synopsis    [Prints the MFFCs of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintMffc( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i;
    extern void Abc_NodeMffsConeSuppPrint( Abc_Obj_t * pNode );
    Abc_NtkForEachNode( pNtk, pNode, i )
        Abc_NodeMffsConeSuppPrint( pNode );
}

/**Function*************************************************************

  Synopsis    [Prints the factored form of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintFactor( FILE * pFile, Abc_Ntk_t * pNtk, int fUseRealNames )
{
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsSopLogic(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
        Abc_NodePrintFactor( pFile, pNode, fUseRealNames );
}

/**Function*************************************************************

  Synopsis    [Prints the factored form of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodePrintFactor( FILE * pFile, Abc_Obj_t * pNode, int fUseRealNames )
{
    Dec_Graph_t * pGraph;
    Vec_Ptr_t * vNamesIn;
    if ( Abc_ObjIsCo(pNode) )
        pNode = Abc_ObjFanin0(pNode);
    if ( Abc_ObjIsPi(pNode) )
    {
        fprintf( pFile, "Skipping the PI node.\n" );
        return;
    }
    if ( Abc_ObjIsLatch(pNode) )
    {
        fprintf( pFile, "Skipping the latch.\n" );
        return;
    }
    assert( Abc_ObjIsNode(pNode) );
    pGraph = Dec_Factor( pNode->pData );
    if ( fUseRealNames )
    {
        vNamesIn = Abc_NodeGetFaninNames(pNode);
        Dec_GraphPrint( stdout, pGraph, (char **)vNamesIn->pArray, Abc_ObjName(pNode) );
        Abc_NodeFreeNames( vNamesIn );
    }
    else
        Dec_GraphPrint( stdout, pGraph, (char **)NULL, Abc_ObjName(pNode) );
    Dec_GraphFree( pGraph );
}


/**Function*************************************************************

  Synopsis    [Prints the level stats of the PO node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintLevel( FILE * pFile, Abc_Ntk_t * pNtk, int fProfile, int fListNodes )
{
    Abc_Obj_t * pNode;
    int i, k, Length;

    if ( fListNodes )
    {
        int nLevels;
        nLevels = Abc_NtkLevel(pNtk);
        printf( "Nodes by level:\n" );
        for ( i = 0; i <= nLevels; i++ )
        {
            printf( "%2d : ", i );
            Abc_NtkForEachNode( pNtk, pNode, k )
                if ( (int)pNode->Level == i )
                    printf( " %s", Abc_ObjName(pNode) );
            printf( "\n" );
        }
        return;
    }

    // print the delay profile
    if ( fProfile && Abc_NtkHasMapping(pNtk) )
    {
        int nIntervals = 12;
        float DelayMax, DelayCur, DelayDelta;
        int * pLevelCounts;
        int DelayInt, nOutsSum, nOutsTotal;

        // get the max delay and delta
        DelayMax   = Abc_NtkDelayTrace( pNtk );
        DelayDelta = DelayMax/nIntervals;
        // collect outputs by delay
        pLevelCounts = ALLOC( int, nIntervals );
        memset( pLevelCounts, 0, sizeof(int) * nIntervals );
        Abc_NtkForEachCo( pNtk, pNode, i )
        {
            DelayCur  = Abc_NodeReadArrival( Abc_ObjFanin0(pNode) )->Worst;
            DelayInt  = (int)(DelayCur / DelayDelta);
            if ( DelayInt >= nIntervals )
                DelayInt = nIntervals - 1;
            pLevelCounts[DelayInt]++;
        }

        nOutsSum   = 0;
        nOutsTotal = Abc_NtkCoNum(pNtk);
        for ( i = 0; i < nIntervals; i++ )
        {
            nOutsSum += pLevelCounts[i];
            printf( "[%8.2f - %8.2f] :   COs = %4d.   %5.1f %%\n", 
                DelayDelta * i, DelayDelta * (i+1), pLevelCounts[i], 100.0 * nOutsSum/nOutsTotal );
        }
        free( pLevelCounts );
        return;
    }
    else if ( fProfile )
    {
        int LevelMax, * pLevelCounts;
        int nOutsSum, nOutsTotal;

        if ( !Abc_NtkIsStrash(pNtk) )
            Abc_NtkLevel(pNtk);

        LevelMax = 0;
        Abc_NtkForEachCo( pNtk, pNode, i )
            if ( LevelMax < (int)Abc_ObjFanin0(pNode)->Level )
                LevelMax = Abc_ObjFanin0(pNode)->Level;
        pLevelCounts = ALLOC( int, LevelMax + 1 );
        memset( pLevelCounts, 0, sizeof(int) * (LevelMax + 1) );
        Abc_NtkForEachCo( pNtk, pNode, i )
            pLevelCounts[Abc_ObjFanin0(pNode)->Level]++;

        nOutsSum   = 0;
        nOutsTotal = Abc_NtkCoNum(pNtk);
        for ( i = 0; i <= LevelMax; i++ )
            if ( pLevelCounts[i] )
            {
                nOutsSum += pLevelCounts[i];
                printf( "Level = %4d.  COs = %4d.   %5.1f %%\n", i, pLevelCounts[i], 100.0 * nOutsSum/nOutsTotal );
            }
        free( pLevelCounts );
        return;
    }
    assert( Abc_NtkIsStrash(pNtk) );

    // find the longest name
    Length = 0;
    Abc_NtkForEachCo( pNtk, pNode, i )
        if ( Length < (int)strlen(Abc_ObjName(pNode)) )
            Length = strlen(Abc_ObjName(pNode));
    if ( Length < 5 )
        Length = 5;
    // print stats for each output
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        fprintf( pFile, "CO %4d :  %*s    ", i, Length, Abc_ObjName(pNode) ); 
        Abc_NodePrintLevel( pFile, pNode );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the factored form of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodePrintLevel( FILE * pFile, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pDriver;
    Vec_Ptr_t * vNodes;

    pDriver = Abc_ObjIsCo(pNode)? Abc_ObjFanin0(pNode) : pNode;
    if ( Abc_ObjIsPi(pDriver) )
    {
        fprintf( pFile, "Primary input.\n" );
        return;
    }
    if ( Abc_ObjIsLatch(pDriver) )
    {
        fprintf( pFile, "Latch.\n" );
        return;
    }
    if ( Abc_NodeIsConst(pDriver) )
    {
        fprintf( pFile, "Constant %d.\n", !Abc_ObjFaninC0(pNode) );
        return;
    }
    // print the level
    fprintf( pFile, "Level = %3d.  ", pDriver->Level );
    // print the size of MFFC
    fprintf( pFile, "Mffc = %5d.  ", Abc_NodeMffcSize(pDriver) );
    // print the size of the shole cone
    vNodes = Abc_NtkDfsNodes( pNode->pNtk, &pDriver, 1 );
    fprintf( pFile, "Cone = %5d.  ", Vec_PtrSize(vNodes) );
    Vec_PtrFree( vNodes );
    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints the factored form of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodePrintKMap( Abc_Obj_t * pNode, int fUseRealNames )
{
    Vec_Ptr_t * vNamesIn;
    if ( fUseRealNames )
    {
        vNamesIn = Abc_NodeGetFaninNames(pNode);
        Extra_PrintKMap( stdout, pNode->pNtk->pManFunc, pNode->pData, Cudd_Not(pNode->pData), 
            Abc_ObjFaninNum(pNode), NULL, 0, (char **)vNamesIn->pArray );
        Abc_NodeFreeNames( vNamesIn );
    }
    else
        Extra_PrintKMap( stdout, pNode->pNtk->pManFunc, pNode->pData, Cudd_Not(pNode->pData), 
            Abc_ObjFaninNum(pNode), NULL, 0, NULL );

}

/**Function*************************************************************

  Synopsis    [Prints statistics about gates used in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintGates( Abc_Ntk_t * pNtk, int fUseLibrary )
{
    Abc_Obj_t * pObj;
    int fHasBdds, i;
    int CountConst, CountBuf, CountInv, CountAnd, CountOr, CountOther, CounterTotal;
    char * pSop;

    if ( fUseLibrary && Abc_NtkHasMapping(pNtk) )
    {
        stmm_table * tTable;
        stmm_generator * gen;
        char * pName;
        int * pCounter, Counter;
        double Area, AreaTotal;

        // count the gates by name
        CounterTotal = 0;
        tTable = stmm_init_table(strcmp, stmm_strhash);
        Abc_NtkForEachNode( pNtk, pObj, i )
        {
            if ( i == 0 ) continue;
            if ( !stmm_find_or_add( tTable, Mio_GateReadName(pObj->pData), (char ***)&pCounter ) )
                *pCounter = 0;
            (*pCounter)++;
            CounterTotal++;
        }
        // print the gates
        AreaTotal = Abc_NtkGetMappedArea(pNtk);
        stmm_foreach_item( tTable, gen, (char **)&pName, (char **)&Counter )
        {
            Area = Counter * Mio_GateReadArea(Mio_LibraryReadGateByName(pNtk->pManFunc,pName));
            printf( "%-12s = %8d   %10.2f    %6.2f %%\n", pName, Counter, Area, 100.0 * Area / AreaTotal );
        }
        printf( "%-12s = %8d   %10.2f    %6.2f %%\n", "TOTAL", CounterTotal, AreaTotal, 100.0 );
        stmm_free_table( tTable );
        return;
    }

    if ( Abc_NtkIsAigLogic(pNtk) )
        return;

    // transform logic functions from BDD to SOP
    if ( fHasBdds = Abc_NtkIsBddLogic(pNtk) )
    {
        if ( !Abc_NtkBddToSop(pNtk, 0) )
        {
            printf( "Abc_NtkPrintGates(): Converting to SOPs has failed.\n" );
            return;
        }
    }

    // get hold of the SOP of the node
    CountConst = CountBuf = CountInv = CountAnd = CountOr = CountOther = CounterTotal = 0;
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( i == 0 ) continue;
        if ( Abc_NtkHasMapping(pNtk) )
            pSop = Mio_GateReadSop(pObj->pData);
        else
            pSop = pObj->pData;
        // collect the stats
        if ( Abc_SopIsConst0(pSop) || Abc_SopIsConst1(pSop) )
            CountConst++;
        else if ( Abc_SopIsBuf(pSop) )
            CountBuf++;
        else if ( Abc_SopIsInv(pSop) )
            CountInv++;
        else if ( !Abc_SopIsComplement(pSop) && Abc_SopIsAndType(pSop) ||  Abc_SopIsComplement(pSop) && Abc_SopIsOrType(pSop) )
            CountAnd++;
        else if (  Abc_SopIsComplement(pSop) && Abc_SopIsAndType(pSop) || !Abc_SopIsComplement(pSop) && Abc_SopIsOrType(pSop) )
            CountOr++;
        else
            CountOther++;
        CounterTotal++;
    }
    printf( "Const        = %8d    %6.2f %%\n", CountConst  ,  100.0 * CountConst   / CounterTotal );
    printf( "Buffer       = %8d    %6.2f %%\n", CountBuf    ,  100.0 * CountBuf     / CounterTotal );
    printf( "Inverter     = %8d    %6.2f %%\n", CountInv    ,  100.0 * CountInv     / CounterTotal );
    printf( "And          = %8d    %6.2f %%\n", CountAnd    ,  100.0 * CountAnd     / CounterTotal );
    printf( "Or           = %8d    %6.2f %%\n", CountOr     ,  100.0 * CountOr      / CounterTotal );
    printf( "Other        = %8d    %6.2f %%\n", CountOther  ,  100.0 * CountOther   / CounterTotal );
    printf( "TOTAL        = %8d    %6.2f %%\n", CounterTotal,  100.0 * CounterTotal / CounterTotal );

    // convert the network back into BDDs if this is how it was
    if ( fHasBdds )
        Abc_NtkSopToBdd(pNtk);
}

/**Function*************************************************************

  Synopsis    [Prints statistics about gates used in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintSharing( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes1, * vNodes2;
    Abc_Obj_t * pObj1, * pObj2, * pNode1, * pNode2;
    int i, k, m, n, Counter;

    // print the template
    printf( "Statistics about sharing of logic nodes among the CO pairs.\n" );
    printf( "(CO1,CO2)=NumShared : " );
    // go though the CO pairs
    Abc_NtkForEachCo( pNtk, pObj1, i )
    {
        vNodes1 = Abc_NtkDfsNodes( pNtk, &pObj1, 1 );
        // mark the nodes
        Vec_PtrForEachEntry( vNodes1, pNode1, m )
            pNode1->fMarkA = 1;
        // go through the second COs
        Abc_NtkForEachCo( pNtk, pObj2, k )
        {
            if ( i >= k )
                continue;
            vNodes2 = Abc_NtkDfsNodes( pNtk, &pObj2, 1 );
            // count the number of marked
            Counter = 0;
            Vec_PtrForEachEntry( vNodes2, pNode2, n )
                Counter += pNode2->fMarkA;
            // print
            printf( "(%d,%d)=%d ", i, k, Counter );
            Vec_PtrFree( vNodes2 );
        }
        // unmark the nodes
        Vec_PtrForEachEntry( vNodes1, pNode1, m )
            pNode1->fMarkA = 0;
        Vec_PtrFree( vNodes1 );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints info for each output cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintStrSupports( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vSupp, * vNodes;
    Abc_Obj_t * pObj;
    int i;
    printf( "Structural support info:\n" );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        vSupp  = Abc_NtkNodeSupport( pNtk, &pObj, 1 );
        vNodes = Abc_NtkDfsNodes( pNtk, &pObj, 1 );
        printf( "%20s :  Cone = %5d.  Supp = %5d.\n", 
            Abc_ObjName(pObj), vNodes->nSize, vSupp->nSize );
        Vec_PtrFree( vNodes );
        Vec_PtrFree( vSupp );
    }
}

/**Function*************************************************************

  Synopsis    [Prints information about the object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjPrint( FILE * pFile, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin;
    int i;
    fprintf( pFile, "Object %5d : ", pObj->Id );
    switch ( pObj->Type )
    {
        case ABC_OBJ_NONE: 
            fprintf( pFile, "NONE   " );  
            break;
        case ABC_OBJ_CONST1: 
            fprintf( pFile, "Const1 " );  
            break;
        case ABC_OBJ_PIO:    
            fprintf( pFile, "PIO    " );  
            break;
        case ABC_OBJ_PI:     
            fprintf( pFile, "PI     " );  
            break;
        case ABC_OBJ_PO:     
            fprintf( pFile, "PO     " );  
            break;
        case ABC_OBJ_BI:     
            fprintf( pFile, "BI     " );  
            break;
        case ABC_OBJ_BO:     
            fprintf( pFile, "BO     " );  
            break;
        case ABC_OBJ_ASSERT:     
            fprintf( pFile, "Assert " );  
            break;
        case ABC_OBJ_NET:  
            fprintf( pFile, "Net    " );  
            break;
        case ABC_OBJ_NODE: 
            fprintf( pFile, "Node   " );  
            break;
        case ABC_OBJ_LATCH:     
            fprintf( pFile, "Latch  " );  
            break;
        case ABC_OBJ_WHITEBOX: 
            fprintf( pFile, "Whitebox" );  
            break;
        case ABC_OBJ_BLACKBOX:     
            fprintf( pFile, "Blackbox" );  
            break;
        default:
            assert(0); 
            break;
    }
    // print the fanins
    fprintf( pFile, " Fanins ( " );
    Abc_ObjForEachFanin( pObj, pFanin, i )
        fprintf( pFile, "%d ", pFanin->Id );
    fprintf( pFile, ") " );
/*
    fprintf( pFile, " Fanouts ( " );
    Abc_ObjForEachFanout( pObj, pFanin, i )
        fprintf( pFile, "%d(%c) ", pFanin->Id, Abc_NodeIsTravIdCurrent(pFanin)? '+' : '-' );
    fprintf( pFile, ") " );
*/
    // print the logic function
    if ( Abc_ObjIsNode(pObj) && Abc_NtkIsSopLogic(pObj->pNtk) )
        fprintf( pFile, " %s", pObj->pData );
    else
        fprintf( pFile, "\n" );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


