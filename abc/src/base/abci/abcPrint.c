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

#include <math.h>
#include "base/abc/abc.h"
#include "bool/dec/dec.h"
#include "base/main/main.h"
#include "map/mio/mio.h"
#include "aig/aig/aig.h"
#include "map/if/if.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

#ifdef WIN32
#include <windows.h>
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

//extern int s_TotalNodes = 0;
//extern int s_TotalChanges = 0;

abctime s_MappingTime = 0;
int s_MappingMem = 0;
//abctime s_ResubTime = 0;
abctime s_ResynTime = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [If the network is best, saves it in "best.blif" and returns 1.]

  Description [If the networks are incomparable, saves the new network, 
  returns its parameters in the internal parameter structure, and returns 1.
  If the new network is not a logic network, quits without saving and returns 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCompareAndSaveBest( Abc_Ntk_t * pNtk )
{
    extern void Io_Write( Abc_Ntk_t * pNtk, char * pFileName, Io_FileType_t FileType );
    static struct ParStruct {
        char * pName;  // name of the best saved network
        int    Depth;  // depth of the best saved network
        int    Flops;  // flops in the best saved network 
        int    Nodes;  // nodes in the best saved network
        int    Edges;  // edges in the best saved network
        int    nPis;   // the number of primary inputs
        int    nPos;   // the number of primary outputs
    } ParsNew, ParsBest = { 0 };
    char * pFileNameOut;
    // free storage for the name
    if ( pNtk == NULL )
    {
        ABC_FREE( ParsBest.pName );
        return 0;
    }
    // quit if not a logic network
    if ( !Abc_NtkIsLogic(pNtk) )
        return 0;
    // get the parameters
    ParsNew.Depth = Abc_NtkLevel( pNtk );
    ParsNew.Flops = Abc_NtkLatchNum( pNtk );
    ParsNew.Nodes = Abc_NtkNodeNum( pNtk );
    ParsNew.Edges = Abc_NtkGetTotalFanins( pNtk );
    ParsNew.nPis  = Abc_NtkPiNum( pNtk );
    ParsNew.nPos  = Abc_NtkPoNum( pNtk );
    // reset the parameters if the network has the same name
    if (  ParsBest.pName == NULL ||
          strcmp(ParsBest.pName, pNtk->pName) ||
          ParsBest.Depth >  ParsNew.Depth ||
         (ParsBest.Depth == ParsNew.Depth && ParsBest.Flops >  ParsNew.Flops) ||
         (ParsBest.Depth == ParsNew.Depth && ParsBest.Flops == ParsNew.Flops && ParsBest.Edges >  ParsNew.Edges) )
    {
        ABC_FREE( ParsBest.pName );
        ParsBest.pName = Extra_UtilStrsav( pNtk->pName );
        ParsBest.Depth = ParsNew.Depth;
        ParsBest.Flops = ParsNew.Flops;
        ParsBest.Nodes = ParsNew.Nodes;
        ParsBest.Edges = ParsNew.Edges;
        ParsBest.nPis  = ParsNew.nPis;
        ParsBest.nPos  = ParsNew.nPos;
        // writ the network
        if ( strcmp(pNtk->pSpec + strlen(pNtk->pSpec) - strlen("_best.blif"), "_best.blif") )
            pFileNameOut = Extra_FileNameGenericAppend( pNtk->pSpec, "_best.blif" );
        else
            pFileNameOut = pNtk->pSpec;
        Io_Write( pNtk, pFileNameOut, IO_FILE_BLIF );
        return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Collects memory usage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Abc_NtkMemory( Abc_Ntk_t * p )
{
    Abc_Obj_t * pObj; int i;
    double Memory = sizeof(Abc_Ntk_t);
    Memory += sizeof(Abc_Obj_t) * Abc_NtkObjNum(p);
    Memory += Vec_PtrMemory(p->vPis);
    Memory += Vec_PtrMemory(p->vPos);
    Memory += Vec_PtrMemory(p->vCis);
    Memory += Vec_PtrMemory(p->vCos);
    Memory += Vec_PtrMemory(p->vObjs);
    Memory += Vec_IntMemory(&p->vTravIds);
    Memory += Vec_IntMemory(p->vLevelsR);
    Abc_NtkForEachObj( p, pObj, i )
        Memory += sizeof(int) * (Vec_IntCap(&pObj->vFanins) + Vec_IntCap(&pObj->vFanouts));
    return Memory;
}

/**Function*************************************************************

  Synopsis    [Marks nodes for power-optimization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_NtkMfsTotalSwitching( Abc_Ntk_t * pNtk )
{
    extern Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
    extern Vec_Int_t * Saig_ManComputeSwitchProbs( Aig_Man_t * p, int nFrames, int nPref, int fProbOne );
    Vec_Int_t * vSwitching;
    float * pSwitching;
    Abc_Ntk_t * pNtkStr;
    Aig_Man_t * pAig;
    Aig_Obj_t * pObjAig;
    Abc_Obj_t * pObjAbc, * pObjAbc2;
    float Result = (float)0;
    int i;
    // strash the network
    pNtkStr = Abc_NtkStrash( pNtk, 0, 1, 0 );
    Abc_NtkForEachObj( pNtk, pObjAbc, i )
        if ( (pObjAbc->pTemp && Abc_ObjRegular((Abc_Obj_t *)pObjAbc->pTemp)->Type == ABC_FUNC_NONE) || (!Abc_ObjIsCi(pObjAbc) && !Abc_ObjIsNode(pObjAbc)) )
            pObjAbc->pTemp = NULL;
    // map network into an AIG
    pAig = Abc_NtkToDar( pNtkStr, 0, (int)(Abc_NtkLatchNum(pNtk) > 0) );
    vSwitching = Saig_ManComputeSwitchProbs( pAig, 48, 16, 0 );
    pSwitching = (float *)vSwitching->pArray;
    Abc_NtkForEachObj( pNtk, pObjAbc, i )
    {
        if ( (pObjAbc2 = Abc_ObjRegular((Abc_Obj_t *)pObjAbc->pTemp)) && (pObjAig = Aig_Regular((Aig_Obj_t *)pObjAbc2->pTemp)) )
        {
            Result += Abc_ObjFanoutNum(pObjAbc) * pSwitching[pObjAig->Id];
//            Abc_ObjPrint( stdout, pObjAbc );
//            printf( "%d = %.2f\n", i, Abc_ObjFanoutNum(pObjAbc) * pSwitching[pObjAig->Id] );
        }
    }
    Vec_IntFree( vSwitching );
    Aig_ManStop( pAig );
    Abc_NtkDelete( pNtkStr );
    return Result;
}

/**Function*************************************************************

  Synopsis    [Compute area using LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_NtkGetArea( Abc_Ntk_t * pNtk )
{
    If_LibLut_t * pLutLib;
    Abc_Obj_t * pObj;
    float Counter = 0.0;
    int i;
    assert( Abc_NtkIsLogic(pNtk) );
    // get the library
    pLutLib = (If_LibLut_t *)Abc_FrameReadLibLut();
    if ( pLutLib && pLutLib->LutMax >= Abc_NtkGetFaninMax(pNtk) )
    {
        Abc_NtkForEachNode( pNtk, pObj, i )
            Counter += pLutLib->pLutAreas[Abc_ObjFaninNum(pObj)];
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Print the vital stats of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintStats( Abc_Ntk_t * pNtk, int fFactored, int fSaveBest, int fDumpResult, int fUseLutLib, int fPrintMuxes, int fPower, int fGlitch, int fSkipBuf, int fSkipSmall, int fPrintMem )
{
    int nSingles = fSkipBuf ? Abc_NtkGetBufNum(pNtk) : 0;
    if ( fPrintMuxes && Abc_NtkIsStrash(pNtk) )
    {
        extern int Abc_NtkCountMuxes( Abc_Ntk_t * pNtk );
        int nXors = Abc_NtkGetExorNum(pNtk);
        int nMuxs = Abc_NtkCountMuxes(pNtk) - nXors;
        int nAnds = Abc_NtkNodeNum(pNtk) - (nMuxs + nXors) * 3 - nSingles;
        Abc_Print( 1, "XMA stats:  " );
        Abc_Print( 1,"Xor =%7d (%6.2f %%)  ", nXors, 300.0 * nXors / Abc_NtkNodeNum(pNtk) );
        Abc_Print( 1,"Mux =%7d (%6.2f %%)  ", nMuxs, 300.0 * nMuxs / Abc_NtkNodeNum(pNtk) );
        Abc_Print( 1,"And =%7d (%6.2f %%)  ",   nAnds, 100.0 * nAnds / Abc_NtkNodeNum(pNtk) );
        Abc_Print( 1,"Total =%7d",   nAnds + nXors + nMuxs );
        Abc_Print( 1,"\n" );
        return;
    }
    if ( fSaveBest )
        Abc_NtkCompareAndSaveBest( pNtk );
/*
    if ( fDumpResult )
    {
        char Buffer[1000] = {0};
        const char * pNameGen = pNtk->pSpec? Extra_FileNameGeneric( pNtk->pSpec ) : "nameless_";
        sprintf( Buffer, "%s_dump.blif", pNameGen );
        Io_Write( pNtk, Buffer, IO_FILE_BLIF );
        if ( pNtk->pSpec ) ABC_FREE( pNameGen );
    }
*/

//    if ( Abc_NtkIsStrash(pNtk) )
//        Abc_AigCountNext( pNtk->pManFunc );

#ifdef WIN32
    SetConsoleTextAttribute( GetStdHandle(STD_OUTPUT_HANDLE), 15 ); // bright
    Abc_Print( 1,"%-30s:", pNtk->pName );
    SetConsoleTextAttribute( GetStdHandle(STD_OUTPUT_HANDLE), 7 );  // normal
#else
    Abc_Print( 1,"%s%-30s:%s", "\033[1;37m", pNtk->pName, "\033[0m" );  // bright
#endif
    Abc_Print( 1," i/o =%5d/%5d", Abc_NtkPiNum(pNtk), Abc_NtkPoNum(pNtk) );
    if ( Abc_NtkConstrNum(pNtk) )
        Abc_Print( 1,"(c=%d)", Abc_NtkConstrNum(pNtk) );
    Abc_Print( 1,"  lat =%5d", Abc_NtkLatchNum(pNtk) );
    if ( pNtk->nBarBufs )
        Abc_Print( 1,"(b=%d)", pNtk->nBarBufs );
    if ( Abc_NtkIsNetlist(pNtk) )
    {
        Abc_Print( 1,"  net =%5d", Abc_NtkNetNum(pNtk) );
        Abc_Print( 1,"  nd =%5d",  fSkipSmall ? Abc_NtkGetLargeNodeNum(pNtk) : Abc_NtkNodeNum(pNtk) - nSingles );
        Abc_Print( 1,"  wbox =%3d", Abc_NtkWhiteboxNum(pNtk) );
        Abc_Print( 1,"  bbox =%3d", Abc_NtkBlackboxNum(pNtk) );
    }
    else if ( Abc_NtkIsStrash(pNtk) )
    {
        Abc_Print( 1,"  and =%7d", Abc_NtkNodeNum(pNtk) );
        if ( Abc_NtkGetChoiceNum(pNtk) )
            Abc_Print( 1," (choice = %d)", Abc_NtkGetChoiceNum(pNtk) );
    }
    else
    {
        Abc_Print( 1,"  nd =%6d", fSkipSmall ? Abc_NtkGetLargeNodeNum(pNtk) : Abc_NtkNodeNum(pNtk) - nSingles );
        Abc_Print( 1,"  edge =%7d", Abc_NtkGetTotalFanins(pNtk) - nSingles );
    }

    if ( Abc_NtkIsStrash(pNtk) || Abc_NtkIsNetlist(pNtk) )
    {
    }
    else if ( Abc_NtkHasSop(pNtk) )
    {

        Abc_Print( 1,"  cube =%6d",  Abc_NtkGetCubeNum(pNtk) - nSingles );
        if ( fFactored )
            Abc_Print( 1,"  lit(sop) =%6d",  Abc_NtkGetLitNum(pNtk) - nSingles );
        if ( fFactored )
            Abc_Print( 1,"  lit(fac) =%6d",  Abc_NtkGetLitFactNum(pNtk) - nSingles );
    }
    else if ( Abc_NtkHasAig(pNtk) )
        Abc_Print( 1,"  aig  =%6d",  Abc_NtkGetAigNodeNum(pNtk) - nSingles );
    else if ( Abc_NtkHasBdd(pNtk) )
        Abc_Print( 1,"  bdd  =%6d",  Abc_NtkGetBddNodeNum(pNtk) - nSingles );
    else if ( Abc_NtkHasMapping(pNtk) )
    {
        int fHasTimeMan = (int)(pNtk->pManTime != NULL);
        assert( pNtk->pManFunc == Abc_FrameReadLibGen() );
        Abc_Print( 1,"  area =%5.2f", Abc_NtkGetMappedArea(pNtk) );
        Abc_Print( 1,"  delay =%5.2f", Abc_NtkDelayTrace(pNtk, NULL, NULL, 0) );
        if ( !fHasTimeMan && pNtk->pManTime )
        {
            Abc_ManTimeStop( pNtk->pManTime );
            pNtk->pManTime = NULL;
        }
    }
    else if ( !Abc_NtkHasBlackbox(pNtk) )
    {
        assert( 0 );
    }

    if ( Abc_NtkIsStrash(pNtk) )
    {
        extern int Abc_NtkGetMultiRefNum( Abc_Ntk_t * pNtk );
        Abc_Print( 1,"  lev =%3d", Abc_AigLevel(pNtk) );
//        Abc_Print( 1,"  ff = %5d", Abc_NtkNodeNum(pNtk) + 2 * (Abc_NtkCoNum(pNtk)+Abc_NtkGetMultiRefNum(pNtk)) );
//        Abc_Print( 1,"  var = %5d", Abc_NtkCiNum(pNtk) + Abc_NtkCoNum(pNtk)+Abc_NtkGetMultiRefNum(pNtk) );
    }
    else
        Abc_Print( 1,"  lev = %d", Abc_NtkLevel(pNtk) );
    if ( pNtk->nBarBufs2 )
        Abc_Print( 1,"  buf = %d", pNtk->nBarBufs2 );
    if ( fUseLutLib && Abc_FrameReadLibLut() )
        Abc_Print( 1,"  delay =%5.2f", Abc_NtkDelayTraceLut(pNtk, 1) );
    if ( fUseLutLib && Abc_FrameReadLibLut() )
        Abc_Print( 1,"  area =%5.2f", Abc_NtkGetArea(pNtk) );
    if ( fPower )
        Abc_Print( 1,"  power =%7.2f", Abc_NtkMfsTotalSwitching(pNtk) );
    if ( fGlitch )
    {
        if ( Abc_NtkIsLogic(pNtk) && Abc_NtkGetFaninMax(pNtk) <= 6 )
            Abc_Print( 1,"  glitch =%7.2f %%", Abc_NtkMfsTotalGlitching(pNtk, 4000, 8, 0) );
        else
            printf( "\nCurrently computes glitching only for K-LUT networks with K <= 6." );
    }
    if ( fPrintMem )
        Abc_Print( 1,"  mem =%5.2f MB", Abc_NtkMemory(pNtk)/(1<<20) );
    Abc_Print( 1,"\n" );

    // print the statistic into a file
    if ( fDumpResult )
    {
        FILE * pTable = fopen( "abcstats.txt", "a+" );
        fprintf( pTable, "%s ",  pNtk->pName );
        fprintf( pTable, "%d ", Abc_NtkPiNum(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkPoNum(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkNodeNum(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkGetTotalFanins(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkLevel(pNtk) );
        fprintf( pTable, "\n" );
        fclose( pTable );
    }
/*
    {
        FILE * pTable;
        pTable = fopen( "ibm/seq_stats.txt", "a+" );
//        fprintf( pTable, "%s ",  pNtk->pName );
//        fprintf( pTable, "%d ", Abc_NtkPiNum(pNtk) );
//        fprintf( pTable, "%d ", Abc_NtkPoNum(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkNodeNum(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkLatchNum(pNtk) );
        fprintf( pTable, "%d ", Abc_NtkLevel(pNtk) );
        fprintf( pTable, "\n" );
        fclose( pTable );
    }
*/

/*
    // print the statistic into a file
    {
        FILE * pTable;
        pTable = fopen( "ucsb/stats.txt", "a+" );
//        fprintf( pTable, "%s ",  pNtk->pSpec );
        fprintf( pTable, "%d ",  Abc_NtkNodeNum(pNtk) );
//        fprintf( pTable, "%d ",  Abc_NtkLevel(pNtk) );
//        fprintf( pTable, "%.0f ", Abc_NtkGetMappedArea(pNtk) );
//        fprintf( pTable, "%.2f ", Abc_NtkDelayTrace(pNtk) );
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


    s_TotalNodes += Abc_NtkNodeNum(pNtk);
    printf( "Total nodes = %6d   %6.2f MB   Changes = %6d.\n", 
        s_TotalNodes, s_TotalNodes * 20.0 / (1<<20), s_TotalChanges );
*/

//    if ( Abc_NtkHasSop(pNtk) )
//        printf( "The total number of cube pairs = %d.\n", Abc_NtkGetCubePairNum(pNtk) );

    if ( 0 )
    {
        FILE * pTable = fopen( "stats.txt", "a+" );
        if ( Abc_NtkIsStrash(pNtk) )
        fprintf( pTable, "%s ", pNtk->pName );
        fprintf( pTable, "%d ", Abc_NtkNodeNum(pNtk) );
        fclose( pTable );
    }

    fflush( stdout );
    if ( pNtk->pExdc )
        Abc_NtkPrintStats( pNtk->pExdc, fFactored, fSaveBest, fDumpResult, fUseLutLib, fPrintMuxes, fPower, fGlitch, fSkipBuf, fSkipSmall, fPrintMem );
}

/**Function*************************************************************

  Synopsis    [Prints PIs/POs and LIs/LOs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintIo( FILE * pFile, Abc_Ntk_t * pNtk, int fPrintFlops )
{
    Abc_Obj_t * pObj;
    int i;

    fprintf( pFile, "Primary inputs (%d): ", Abc_NtkPiNum(pNtk) );
    Abc_NtkForEachPi( pNtk, pObj, i )
        fprintf( pFile, " %d=%s", i, Abc_ObjName(pObj) );
//        fprintf( pFile, " %s(%d)", Abc_ObjName(pObj), Abc_ObjFanoutNum(pObj) );
    fprintf( pFile, "\n" );

    fprintf( pFile, "Primary outputs (%d):", Abc_NtkPoNum(pNtk) );
    Abc_NtkForEachPo( pNtk, pObj, i )
        fprintf( pFile, " %d=%s", i, Abc_ObjName(pObj) );
    fprintf( pFile, "\n" );

    if ( !fPrintFlops )
        return;

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
            if ( Abc_LatchIsInit1(pLatch) == Abc_NodeIsConst1(Abc_ObjFanin0(Abc_ObjFanin0(pLatch))) )
                Counter2++;
        }
    }
//    fprintf( pFile, "%-15s:  ", pNtk->pName );
    fprintf( pFile, "Total latches = %5d. Init0 = %d. Init1 = %d. InitDC = %d. Const data = %d.\n",
        Abc_NtkLatchNum(pNtk), InitNums[1], InitNums[2], InitNums[3], Counter0 );
//    fprintf( pFile, "Const fanin = %3d. DC init = %3d. Matching init = %3d. ", Counter0, Counter1, Counter2 );
//    fprintf( pFile, "Self-feed latches = %2d.\n", -1 ); //Abc_NtkCountSelfFeedLatches(pNtk) );
}

/**Function*************************************************************

  Synopsis    [Prints the distribution of fanins/fanouts in the network.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFaninFanoutCounters( Abc_Ntk_t * pNtk, Vec_Int_t * vFan, Vec_Int_t * vFon, Vec_Int_t * vFanR, Vec_Int_t * vFonR )
{
    Abc_Obj_t * pNode;
    int i, nFanins, nFanouts;
    int nFaninsMax = 0, nFanoutsMax = 0;
    Abc_NtkForEachObj( pNtk, pNode, i )
    {
        nFaninsMax  = Abc_MaxInt( nFaninsMax,  Abc_ObjFaninNum(pNode) );
        nFanoutsMax = Abc_MaxInt( nFanoutsMax, Abc_ObjFanoutNum(pNode) );
    }
    Vec_IntFill( vFan, nFaninsMax + 1, 0 );
    Vec_IntFill( vFon, nFanoutsMax + 1, 0 );
    Vec_IntFill( vFanR, nFaninsMax + 1, 0 );
    Vec_IntFill( vFonR, nFanoutsMax + 1, 0 );
    Abc_NtkForEachObjReverse( pNtk, pNode, i )
    {
        nFanins  = Abc_ObjFaninNum( pNode );
        nFanouts = Abc_ObjFanoutNum( pNode );
        Vec_IntAddToEntry( vFan,  nFanins, 1 );
        Vec_IntAddToEntry( vFon, nFanouts, 1 );
        Vec_IntWriteEntry( vFanR, nFanins, i );
        Vec_IntWriteEntry( vFonR, nFanouts, i );
    }
}
void Abc_NtkInputOutputCounters( Abc_Ntk_t * pNtk, Vec_Int_t * vFan, Vec_Int_t * vFon, Vec_Int_t * vFanR, Vec_Int_t * vFonR )
{
    Abc_Obj_t * pNode;
    int i, nFanins, nFanouts;
    int nFaninsMax = 0, nFanoutsMax = 0;
    Abc_NtkForEachCi( pNtk, pNode, i )
        nFanoutsMax = Abc_MaxInt( nFanoutsMax, Abc_ObjFanoutNum(pNode) );
    Abc_NtkForEachCo( pNtk, pNode, i )
        nFaninsMax  = Abc_MaxInt( nFaninsMax,  Abc_ObjFaninNum(Abc_ObjFanin0(pNode)) );
    Vec_IntFill( vFan, nFaninsMax + 1, 0 );
    Vec_IntFill( vFon, nFanoutsMax + 1, 0 );
    Vec_IntFill( vFanR, nFaninsMax + 1, 0 );
    Vec_IntFill( vFonR, nFanoutsMax + 1, 0 );
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        nFanouts = Abc_ObjFanoutNum( pNode );
        Vec_IntAddToEntry( vFon, nFanouts, 1 );
        Vec_IntWriteEntry( vFonR, nFanouts, Abc_ObjId(pNode) );
    }
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        nFanins  = Abc_ObjFaninNum( Abc_ObjFanin0(pNode) );
        Vec_IntAddToEntry( vFan,  nFanins, 1 );
        Vec_IntWriteEntry( vFanR, nFanins, Abc_ObjId(pNode) );
    }
}
Vec_Int_t * Abc_NtkCollectCoSupps( Abc_Ntk_t * pNtk, int fVerbose )
{
    abctime clk = Abc_Clock();
    Abc_Obj_t * pNode; int i, k;
    Vec_Ptr_t * vNodes = Abc_NtkDfs( pNtk, 0 );
    Vec_Int_t * vFanin, * vFanout, * vTemp = Vec_IntAlloc( 0 );
    Vec_Int_t * vSuppsCo = Vec_IntAlloc( Abc_NtkCoNum(pNtk) );
    Vec_Wec_t * vSupps = Vec_WecStart( Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachCi( pNtk, pNode, i )
        Vec_IntPush( Vec_WecEntry(vSupps, Abc_ObjId(pNode)), i );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        vFanout = Vec_WecEntry(vSupps, Abc_ObjId(pNode));
        for ( k = 0; k < Abc_ObjFaninNum(pNode); k++ )
        {
            vFanin = Vec_WecEntry(vSupps, Abc_ObjFaninId(pNode, k));
            Vec_IntTwoMerge2( vFanout, vFanin, vTemp ); 
            ABC_SWAP( Vec_Int_t, *vFanout, *vTemp );
        }
    }
    Abc_NtkForEachCo( pNtk, pNode, i )
        Vec_IntPush( vSuppsCo, Vec_IntSize(Vec_WecEntry(vSupps, Abc_ObjFaninId0(pNode))) );
    Vec_WecFree( vSupps );
    Vec_PtrFree( vNodes );
    Vec_IntFree( vTemp );
    if ( fVerbose )
        Abc_PrintTime( 1, "Input  support computation", Abc_Clock() - clk );
    //Vec_IntPrint( vSuppsCo );
    return vSuppsCo;
}
Vec_Int_t * Abc_NtkCollectCiSupps( Abc_Ntk_t * pNtk, int fVerbose )
{
    abctime clk = Abc_Clock();
    Abc_Obj_t * pNode; int i, k;
    Vec_Ptr_t * vNodes = Abc_NtkDfs( pNtk, 0 );
    Vec_Int_t * vFanin, * vFanout, * vTemp = Vec_IntAlloc( 0 );
    Vec_Int_t * vSuppsCi = Vec_IntAlloc( Abc_NtkCiNum(pNtk) );
    Vec_Wec_t * vSupps = Vec_WecStart( Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        vFanout = Vec_WecEntry(vSupps, Abc_ObjId(pNode));
        vFanin = Vec_WecEntry(vSupps, Abc_ObjFaninId0(pNode));
        Vec_IntPush( vFanout, i );
        Vec_IntTwoMerge2( vFanin, vFanout, vTemp );
        ABC_SWAP( Vec_Int_t, *vFanin, *vTemp );
    }
    Vec_PtrForEachEntryReverse( Abc_Obj_t *, vNodes, pNode, i )
    {
        vFanout = Vec_WecEntry(vSupps, Abc_ObjId(pNode));
        for ( k = 0; k < Abc_ObjFaninNum(pNode); k++ )
        {
            vFanin = Vec_WecEntry(vSupps, Abc_ObjFaninId(pNode, k));
            Vec_IntTwoMerge2( vFanin, vFanout, vTemp );
            ABC_SWAP( Vec_Int_t, *vFanin, *vTemp );
        }
    }
    Abc_NtkForEachCi( pNtk, pNode, i )
        Vec_IntPush( vSuppsCi, Vec_IntSize(Vec_WecEntry(vSupps, Abc_ObjId(pNode))) );
    Vec_WecFree( vSupps );
    Vec_PtrFree( vNodes );
    Vec_IntFree( vTemp );
    if ( fVerbose )
        Abc_PrintTime( 1, "Output support computation", Abc_Clock() - clk );
    //Vec_IntPrint( vSuppsCi );
    return vSuppsCi;
}
void Abc_NtkInOutSupportCounters( Abc_Ntk_t * pNtk, Vec_Int_t * vFan, Vec_Int_t * vFon, Vec_Int_t * vFanR, Vec_Int_t * vFonR )
{
    Abc_Obj_t * pNode;
    Vec_Int_t * vSuppsCo = Abc_NtkCollectCoSupps( pNtk, 1 );
    Vec_Int_t * vSuppsCi = Abc_NtkCollectCiSupps( pNtk, 1 );
    int i, nFanins, nFanouts;
    int nFaninsMax  = Vec_IntFindMax( vSuppsCo );
    int nFanoutsMax = Vec_IntFindMax( vSuppsCi );
    Vec_IntFill( vFan, nFaninsMax + 1, 0 );
    Vec_IntFill( vFon, nFanoutsMax + 1, 0 );
    Vec_IntFill( vFanR, nFaninsMax + 1, 0 );
    Vec_IntFill( vFonR, nFanoutsMax + 1, 0 );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        nFanins  = Vec_IntEntry( vSuppsCo, i );
        Vec_IntAddToEntry( vFan,  nFanins, 1 );
        Vec_IntWriteEntry( vFanR, nFanins, Abc_ObjId(pNode) );
    }
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        nFanouts = Vec_IntEntry( vSuppsCi, i );
        Vec_IntAddToEntry( vFon, nFanouts, 1 );
        Vec_IntWriteEntry( vFonR, nFanouts, Abc_ObjId(pNode) );
    }
    Vec_IntFree( vSuppsCo );
    Vec_IntFree( vSuppsCi );
}

Vec_Int_t * Abc_NtkCollectCoCones( Abc_Ntk_t * pNtk, int fVerbose )
{
    abctime clk = Abc_Clock();
    Abc_Obj_t * pNode; int i, k;
    Vec_Ptr_t * vNodes = Abc_NtkDfs( pNtk, 0 );
    Vec_Int_t * vFanin, * vFanout, * vTemp = Vec_IntAlloc( 0 );
    Vec_Int_t * vSuppsCo = Vec_IntAlloc( Abc_NtkCoNum(pNtk) );
    Vec_Wec_t * vSupps = Vec_WecStart( Abc_NtkObjNumMax(pNtk) );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        vFanout = Vec_WecEntry(vSupps, Abc_ObjId(pNode));
        for ( k = 0; k < Abc_ObjFaninNum(pNode); k++ )
        {
            vFanin = Vec_WecEntry(vSupps, Abc_ObjFaninId(pNode, k));
            Vec_IntTwoMerge2( vFanout, vFanin, vTemp ); 
            ABC_SWAP( Vec_Int_t, *vFanout, *vTemp );
        }
        Vec_IntPush( vFanout, i );
    }
    Abc_NtkForEachCo( pNtk, pNode, i )
        Vec_IntPush( vSuppsCo, Vec_IntSize(Vec_WecEntry(vSupps, Abc_ObjFaninId0(pNode))) );
    Vec_WecFree( vSupps );
    Vec_PtrFree( vNodes );
    Vec_IntFree( vTemp );
    if ( fVerbose )
        Abc_PrintTime( 1, "Input  cone computation", Abc_Clock() - clk );
    //Vec_IntPrint( vSuppsCo );
    return vSuppsCo;
}
Vec_Int_t * Abc_NtkCollectCiCones( Abc_Ntk_t * pNtk, int fVerbose )
{
    abctime clk = Abc_Clock();
    Abc_Obj_t * pNode; int i, k;
    Vec_Ptr_t * vNodes = Abc_NtkDfs( pNtk, 0 );
    Vec_Int_t * vFanin, * vFanout, * vTemp = Vec_IntAlloc( 0 );
    Vec_Int_t * vSuppsCi = Vec_IntAlloc( Abc_NtkCiNum(pNtk) );
    Vec_Wec_t * vSupps = Vec_WecStart( Abc_NtkObjNumMax(pNtk) );
    Vec_PtrForEachEntryReverse( Abc_Obj_t *, vNodes, pNode, i )
    {
        vFanout = Vec_WecEntry(vSupps, Abc_ObjId(pNode));
        Vec_IntPush( vFanout, i );
        for ( k = 0; k < Abc_ObjFaninNum(pNode); k++ )
        {
            vFanin = Vec_WecEntry(vSupps, Abc_ObjFaninId(pNode, k));
            Vec_IntTwoMerge2( vFanin, vFanout, vTemp );
            ABC_SWAP( Vec_Int_t, *vFanin, *vTemp );
        }
    }
    Abc_NtkForEachCi( pNtk, pNode, i )
        Vec_IntPush( vSuppsCi, Vec_IntSize(Vec_WecEntry(vSupps, Abc_ObjId(pNode))) );
    Vec_WecFree( vSupps );
    Vec_PtrFree( vNodes );
    Vec_IntFree( vTemp );
    if ( fVerbose )
        Abc_PrintTime( 1, "Output cone computation", Abc_Clock() - clk );
    //Vec_IntPrint( vSuppsCi );
    return vSuppsCi;
}
void Abc_NtkInOutConeCounters( Abc_Ntk_t * pNtk, Vec_Int_t * vFan, Vec_Int_t * vFon, Vec_Int_t * vFanR, Vec_Int_t * vFonR )
{
    Abc_Obj_t * pNode;
    Vec_Int_t * vSuppsCo = Abc_NtkCollectCoCones( pNtk, 1 );
    Vec_Int_t * vSuppsCi = Abc_NtkCollectCiCones( pNtk, 1 );
    int i, nFanins, nFanouts;
    int nFaninsMax  = Vec_IntFindMax( vSuppsCo );
    int nFanoutsMax = Vec_IntFindMax( vSuppsCi );
    Vec_IntFill( vFan, nFaninsMax + 1, 0 );
    Vec_IntFill( vFon, nFanoutsMax + 1, 0 );
    Vec_IntFill( vFanR, nFaninsMax + 1, 0 );
    Vec_IntFill( vFonR, nFanoutsMax + 1, 0 );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        nFanins  = Vec_IntEntry( vSuppsCo, i );
        Vec_IntAddToEntry( vFan,  nFanins, 1 );
        Vec_IntWriteEntry( vFanR, nFanins, Abc_ObjId(pNode) );
    }
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        nFanouts = Vec_IntEntry( vSuppsCi, i );
        Vec_IntAddToEntry( vFon, nFanouts, 1 );
        Vec_IntWriteEntry( vFonR, nFanouts, Abc_ObjId(pNode) );
    }
    Vec_IntFree( vSuppsCo );
    Vec_IntFree( vSuppsCi );
}

void Abc_NtkPrintDistribInternal( FILE * pFile, Abc_Ntk_t * pNtk, char * pFanins, char * pFanouts, char * pNode, char * pFanin, char * pFanout, 
                                 Vec_Int_t * vFan, Vec_Int_t * vFon, Vec_Int_t * vFanR, Vec_Int_t * vFonR )
{
    int k, nSizeMax = Abc_MaxInt( Vec_IntSize(vFan), Vec_IntSize(vFon) );
    fprintf( pFile, "The distribution of %s and %s in the network:\n", pFanins, pFanouts );
    fprintf( pFile, "  Number   %s with %s  %s with %s          Repr1             Repr2\n", pNode, pFanin, pNode, pFanout );
    for ( k = 0; k < nSizeMax; k++ )
    {
        int EntryFan = k < Vec_IntSize(vFan) ? Vec_IntEntry(vFan, k) : 0;
        int EntryFon = k < Vec_IntSize(vFon) ? Vec_IntEntry(vFon, k) : 0;
        if ( EntryFan == 0 && EntryFon == 0 )
            continue;

        fprintf( pFile, "%5d : ", k );
        if ( EntryFan == 0 )
            fprintf( pFile, "              " );
        else
            fprintf( pFile, "%12d  ", EntryFan );
        fprintf( pFile, "    " );
        if ( EntryFon == 0 )
            fprintf( pFile, "              " );
        else
            fprintf( pFile, "%12d  ", EntryFon );

        fprintf( pFile, "        " );
        if ( EntryFan == 0 )
            fprintf( pFile, "              " );
        else
            fprintf( pFile, "%12s  ", Abc_ObjName(Abc_NtkObj(pNtk, Vec_IntEntry(vFanR, k))) );
        fprintf( pFile, "    " );
        if ( EntryFon == 0 )
            fprintf( pFile, "              " );
        else
            fprintf( pFile, "%12s  ", Abc_ObjName(Abc_NtkObj(pNtk, Vec_IntEntry(vFonR, k))) );
        fprintf( pFile, "\n" );
    }
}
void Abc_NtkPrintFanio( FILE * pFile, Abc_Ntk_t * pNtk, int fUseFanio, int fUsePio, int fUseSupp, int fUseCone )
{
    Vec_Int_t * vFan = Vec_IntAlloc( 0 );
    Vec_Int_t * vFon = Vec_IntAlloc( 0 );
    Vec_Int_t * vFanR = Vec_IntAlloc( 0 );
    Vec_Int_t * vFonR = Vec_IntAlloc( 0 );
    assert( fUseFanio + fUsePio + fUseSupp + fUseCone == 1 );
    if ( fUseFanio )
    {
        Abc_NtkFaninFanoutCounters( pNtk, vFan, vFon, vFanR, vFonR );
        Abc_NtkPrintDistribInternal( pFile, pNtk, "fanins", "fanouts", "Nodes", "fanin", "fanout", vFan, vFon, vFanR, vFonR ); 
    }
    else if ( fUsePio )
    {
        Abc_NtkInputOutputCounters( pNtk, vFan, vFon, vFanR, vFonR );
        Abc_NtkPrintDistribInternal( pFile, pNtk, "fanins", "fanouts", "I/O", "fanin", "fanout", vFan, vFon, vFanR, vFonR ); 
    }
    else if ( fUseSupp )
    {
        Abc_NtkInOutSupportCounters( pNtk, vFan, vFon, vFanR, vFonR );
        Abc_NtkPrintDistribInternal( pFile, pNtk, "input supports", "output supports", "I/O", "in-supp", "out-supp", vFan, vFon, vFanR, vFonR ); 
    }
    else if ( fUseCone )
    {
        Abc_NtkInOutConeCounters( pNtk, vFan, vFon, vFanR, vFonR );
        Abc_NtkPrintDistribInternal( pFile, pNtk, "input cones", "output cones", "I/O", "in-cone", "out-cone", vFan, vFon, vFanR, vFonR ); 
    }
    Vec_IntFree( vFan );
    Vec_IntFree( vFon );
    Vec_IntFree( vFanR );
    Vec_IntFree( vFonR );
}


/**Function*************************************************************

  Synopsis    [Prints the distribution of fanins/fanouts in the network.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintFanioNew( FILE * pFile, Abc_Ntk_t * pNtk, int fMffc )
{
    char Buffer[100];
    Abc_Obj_t * pNode;
    Vec_Int_t * vFanins, * vFanouts;
    int nFanins, nFanouts, nFaninsMax, nFanoutsMax, nFaninsAll, nFanoutsAll;
    int i, k, nSizeMax;

    // determine the largest fanin and fanout
    nFaninsMax = nFanoutsMax = 0;
    nFaninsAll = nFanoutsAll = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( fMffc && Abc_ObjFanoutNum(pNode) == 1 )
            continue;
        nFanins  = Abc_ObjFaninNum(pNode);
        if ( Abc_NtkIsNetlist(pNtk) )
            nFanouts = Abc_ObjFanoutNum( Abc_ObjFanout0(pNode) );
        else if ( fMffc )
            nFanouts = Abc_NodeMffcSize(pNode);
        else
            nFanouts = Abc_ObjFanoutNum(pNode);
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
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( fMffc && Abc_ObjFanoutNum(pNode) == 1 )
            continue;
        nFanins  = Abc_ObjFaninNum(pNode);
        if ( Abc_NtkIsNetlist(pNtk) )
            nFanouts = Abc_ObjFanoutNum( Abc_ObjFanout0(pNode) );
        else if ( fMffc )
            nFanouts = Abc_NodeMffcSize(pNode);
        else
            nFanouts = Abc_ObjFanoutNum(pNode);

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

    fprintf( pFile, "The distribution of fanins and fanouts in the network:\n" );
    fprintf( pFile, "         Number   Nodes with fanin  Nodes with fanout\n" );
    for ( k = 0; k < nSizeMax; k++ )
    {
        if ( vFanins->pArray[k] == 0 && vFanouts->pArray[k] == 0 )
            continue;
        if ( k < 10 )
            fprintf( pFile, "%15d : ", k );
        else
        {
            sprintf( Buffer, "%d - %d", (int)pow((double)10, k/10) * (k%10), (int)pow((double)10, k/10) * (k%10+1) - 1 );
            fprintf( pFile, "%15s : ", Buffer );
        }
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

    fprintf( pFile, "Fanins: Max = %d. Ave = %.2f.  Fanouts: Max = %d. Ave =  %.2f.\n",
        nFaninsMax,  1.0*nFaninsAll/Abc_NtkNodeNum(pNtk),
        nFanoutsMax, 1.0*nFanoutsAll/Abc_NtkNodeNum(pNtk)  );
/*
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        printf( "%d ", Abc_ObjFanoutNum(pNode) );
    }
    printf( "\n" );
*/
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
    extern void Abc_NodeMffcConeSuppPrint( Abc_Obj_t * pNode );
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( Abc_ObjFanoutNum(pNode) > 1 )
            Abc_NodeMffcConeSuppPrint( pNode );
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
    pGraph = Dec_Factor( (char *)pNode->pData );
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
void Abc_NtkPrintFactor( FILE * pFile, Abc_Ntk_t * pNtk, int fUseRealNames )
{
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsSopLogic(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
        Abc_NodePrintFactor( pFile, pNode, fUseRealNames );
}

/**Function*************************************************************

  Synopsis    [Prints the SOPs of one node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodePrintSop( FILE * pFile, Abc_Obj_t * pNode, int fUseRealNames )
{
    Vec_Ptr_t * vNamesIn = NULL;
    char * pCube, * pCur, * pSop; int nVars;
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
    pSop = (char *)pNode->pData;
    nVars = Abc_SopGetVarNum( pSop );
    if ( nVars == 0 )
    {
        fprintf( pFile, "%s = ", Abc_ObjName(pNode) );
        fprintf( pFile, "Constant %d", Abc_SopGetPhase(pSop) );
        return;
    }
    if ( !Abc_SopGetPhase(pSop) )
        fprintf( pFile, "!" );
    fprintf( pFile, "%s = ", Abc_ObjName(pNode) );
    if ( fUseRealNames )
        vNamesIn = Abc_NodeGetFaninNames(pNode);
    Abc_SopForEachCube( pSop, nVars, pCube )
    {
        if ( pCube != pSop ) 
            fprintf( pFile, " +" );
        if ( vNamesIn )
        {
            for ( pCur = pCube; *pCur != ' '; pCur++ )
                if ( *pCur != '-' )
                    fprintf( pFile, " %s%s", *pCur == '0' ? "!" : "",  (char *)Vec_PtrEntry(vNamesIn, pCur-pCube) );
        }
        else
        {
            for ( pCur = pCube; *pCur != ' '; pCur++ )
                if ( *pCur != '-' )
                    fprintf( pFile, " %s%c", *pCur == '0' ? "!" : "",  (char)('a' + pCur-pCube) );
        }
    }
    fprintf( pFile, "\n" );
    if ( vNamesIn )
        Abc_NodeFreeNames( vNamesIn );
}
void Abc_NtkPrintSop( FILE * pFile, Abc_Ntk_t * pNtk, int fUseRealNames )
{
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsSopLogic(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
        Abc_NodePrintSop( pFile, pNode, fUseRealNames );
}
 
/**Function*************************************************************

  Synopsis    [Prints the level stats of the PO node.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_NodeGetPrintName( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFan, * pFanout = NULL; int k, nPos = 0;
    if ( !Abc_ObjIsNode(pObj) ) 
        return Abc_ObjName(pObj);
    Abc_ObjForEachFanout( pObj, pFan, k ) {
        if ( Abc_ObjIsPo(pFan) )
            pFanout = pFan, nPos++;
    }
    return Abc_ObjName(nPos == 1 ? pFanout : pObj);
}
void Abc_NtkPrintLevel( FILE * pFile, Abc_Ntk_t * pNtk, int fProfile, int fListNodes, int fVerbose )
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
                    printf( " %s", Abc_NodeGetPrintName(pNode) );
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
        DelayMax   = Abc_NtkDelayTrace( pNtk, NULL, NULL, 0 );
        DelayDelta = DelayMax/nIntervals;
        // collect outputs by delay
        pLevelCounts = ABC_ALLOC( int, nIntervals );
        memset( pLevelCounts, 0, sizeof(int) * nIntervals );
        Abc_NtkForEachCo( pNtk, pNode, i )
        {
            if ( Abc_ObjIsNode(Abc_ObjFanin0(pNode)) && Abc_ObjFaninNum(Abc_ObjFanin0(pNode)) == 0 )
                DelayInt = 0;
            else
            {
                DelayCur  = Abc_NodeReadArrivalWorst( Abc_ObjFanin0(pNode) );
                DelayInt  = (int)(DelayCur / DelayDelta);
                if ( DelayInt >= nIntervals )
                    DelayInt = nIntervals - 1;
            }
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
        ABC_FREE( pLevelCounts );
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
        pLevelCounts = ABC_ALLOC( int, LevelMax + 1 );
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
        ABC_FREE( pLevelCounts );
        return;
    }
    assert( Abc_NtkIsStrash(pNtk) );

    if ( fVerbose )
    {
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
#ifdef ABC_USE_CUDD
    Vec_Ptr_t * vNamesIn;
    if ( fUseRealNames )
    {
        vNamesIn = Abc_NodeGetFaninNames(pNode);
        Extra_PrintKMap( stdout, (DdManager *)pNode->pNtk->pManFunc, (DdNode *)pNode->pData, Cudd_Not(pNode->pData),
            Abc_ObjFaninNum(pNode), NULL, 0, (char **)vNamesIn->pArray );
        Abc_NodeFreeNames( vNamesIn );
    }
    else
        Extra_PrintKMap( stdout, (DdManager *)pNode->pNtk->pManFunc, (DdNode *)pNode->pData, Cudd_Not(pNode->pData),
            Abc_ObjFaninNum(pNode), NULL, 0, NULL );
#endif
}

/**Function*************************************************************

  Synopsis    [Prints statistics about gates used in the network.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintGates( Abc_Ntk_t * pNtk, int fUseLibrary, int fUpdateProfile )
{
    Abc_Obj_t * pObj;
    int fHasBdds, i;
    int CountConst, CountBuf, CountInv, CountAnd, CountOr, CountOther, CounterTotal, TotalDiff = 0;
    char * pSop;

    if ( fUseLibrary && Abc_NtkHasMapping(pNtk) )
    {
        Mio_Gate_t ** ppGates;
        double Area, AreaTotal;
        int Counter, nGates, i, nGateNameLen;

        // clean value of all gates
        nGates = Mio_LibraryReadGateNum( (Mio_Library_t *)pNtk->pManFunc );
        ppGates = Mio_LibraryReadGateArray( (Mio_Library_t *)pNtk->pManFunc );
        for ( i = 0; i < nGates; i++ )
        {
            Mio_GateSetValue( ppGates[i], 0 );
            if ( fUpdateProfile )
                Mio_GateSetProfile2( ppGates[i], 0 );
        }

        // count the gates by name
        CounterTotal = 0;
        Abc_NtkForEachNodeNotBarBuf( pNtk, pObj, i )
        {
            if ( i == 0 ) continue;
            Mio_GateSetValue( (Mio_Gate_t *)pObj->pData, 1 + Mio_GateReadValue((Mio_Gate_t *)pObj->pData) );
            if ( fUpdateProfile )
                Mio_GateIncProfile2( (Mio_Gate_t *)pObj->pData );
            CounterTotal++;
            // assuming that twin gates follow each other
            if ( Abc_NtkFetchTwinNode(pObj) )
                i++;
        }

        // determine the longest gate name
        nGateNameLen = 5;
        for ( i = 0; i < nGates; i++ )
        {
            Counter = Mio_GateReadValue( ppGates[i] );
            if ( Counter == 0 )
                continue;
            nGateNameLen = Abc_MaxInt( nGateNameLen, strlen(Mio_GateReadName(ppGates[i])) );
        }

        // print the gates
        AreaTotal = Abc_NtkGetMappedArea(pNtk);
        for ( i = 0; i < nGates; i++ )
        {
            Counter = Mio_GateReadValue( ppGates[i] );
            if ( Counter == 0 && Mio_GateReadProfile(ppGates[i]) == 0 )
                continue;
            if ( Mio_GateReadPinNum(ppGates[i]) > 1 )
                TotalDiff += Abc_AbsInt( Mio_GateReadProfile(ppGates[i]) - Mio_GateReadProfile2(ppGates[i]) );
            Area = Counter * Mio_GateReadArea( ppGates[i] );
            printf( "%-*s   Fanin = %2d   Instance = %8d   Area = %10.2f   %6.2f %%   %8d  %8d   %s\n",
                nGateNameLen, Mio_GateReadName( ppGates[i] ),
                Mio_GateReadPinNum( ppGates[i] ),
                Counter, Area, 100.0 * Area / AreaTotal,
                Mio_GateReadProfile(ppGates[i]),
                Mio_GateReadProfile2(ppGates[i]),
                Mio_GateReadForm(ppGates[i]) );
        }
        printf( "%-*s                Instance = %8d   Area = %10.2f   %6.2f %%   AbsDiff = %d\n",
            nGateNameLen, "TOTAL",
            CounterTotal, AreaTotal, 100.0, TotalDiff );
        return;
    }

    if ( Abc_NtkIsAigLogic(pNtk) )
        return;

    // transform logic functions from BDD to SOP
    if ( (fHasBdds = Abc_NtkIsBddLogic(pNtk)) )
    {
        if ( !Abc_NtkBddToSop(pNtk, -1, ABC_INFINITY, 1) )
        {
            printf( "Abc_NtkPrintGates(): Converting to SOPs has failed.\n" );
            return;
        }
    }

    // get hold of the SOP of the node
    CountConst = CountBuf = CountInv = CountAnd = CountOr = CountOther = CounterTotal = 0;
    Abc_NtkForEachNodeNotBarBuf( pNtk, pObj, i )
    {
        if ( i == 0 ) continue;
        if ( Abc_NtkHasMapping(pNtk) )
            pSop = Mio_GateReadSop((Mio_Gate_t *)pObj->pData);
        else
            pSop = (char *)pObj->pData;
        // collect the stats
        if ( Abc_SopIsConst0(pSop) || Abc_SopIsConst1(pSop) )
            CountConst++;
        else if ( Abc_SopIsBuf(pSop) )
            CountBuf++;
        else if ( Abc_SopIsInv(pSop) )
            CountInv++;
        else if ( (!Abc_SopIsComplement(pSop) && Abc_SopIsAndType(pSop)) ||
                  ( Abc_SopIsComplement(pSop) && Abc_SopIsOrType(pSop)) )
            CountAnd++;
        else if ( ( Abc_SopIsComplement(pSop) && Abc_SopIsAndType(pSop)) ||
                  (!Abc_SopIsComplement(pSop) && Abc_SopIsOrType(pSop)) )
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
        Vec_PtrForEachEntry( Abc_Obj_t *, vNodes1, pNode1, m )
            pNode1->fMarkA = 1;
        // go through the second COs
        Abc_NtkForEachCo( pNtk, pObj2, k )
        {
            if ( i >= k )
                continue;
            vNodes2 = Abc_NtkDfsNodes( pNtk, &pObj2, 1 );
            // count the number of marked
            Counter = 0;
            Vec_PtrForEachEntry( Abc_Obj_t *, vNodes2, pNode2, n )
                Counter += pNode2->fMarkA;
            // print
            printf( "(%d,%d)=%d ", i, k, Counter );
            Vec_PtrFree( vNodes2 );
        }
        // unmark the nodes
        Vec_PtrForEachEntry( Abc_Obj_t *, vNodes1, pNode1, m )
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
int Abc_NtkCountPis( Vec_Ptr_t * vSupp )
{
    Abc_Obj_t * pObj;
    int i, Counter = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pObj, i )
        Counter += Abc_ObjIsPi(pObj);
    return Counter;
}
void Abc_NtkPrintStrSupports( Abc_Ntk_t * pNtk, int fMatrix )
{
    Vec_Ptr_t * vSupp, * vNodes;
    Abc_Obj_t * pObj;
    int i, k, nPis;
    printf( "Structural support info:\n" );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        vSupp  = Abc_NtkNodeSupport( pNtk, &pObj, 1 );
        vNodes = Abc_NtkDfsNodes( pNtk, &pObj, 1 );
        nPis   = Abc_NtkCountPis( vSupp );
        printf( "%5d  %20s :  Cone = %5d.  Supp = %5d. (PIs = %5d. FFs = %5d.)\n",
            i, Abc_ObjName(pObj), vNodes->nSize, vSupp->nSize, nPis, vSupp->nSize - nPis );
        Vec_PtrFree( vNodes );
        Vec_PtrFree( vSupp );
    }
    if ( !fMatrix )
    {
        Abc_NtkCleanMarkA( pNtk );
        return;
    }

    Abc_NtkForEachCi( pNtk, pObj, k )
        pObj->fMarkA = 0;

    printf( "Actual support info:\n" );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        vSupp  = Abc_NtkNodeSupport( pNtk, &pObj, 1 );
        Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pObj, k )
            pObj->fMarkA = 1;
        Vec_PtrFree( vSupp );

        Abc_NtkForEachCi( pNtk, pObj, k )
            printf( "%d", pObj->fMarkA );
        printf( "\n" );

        Abc_NtkForEachCi( pNtk, pObj, k )
            pObj->fMarkA = 0;
    }
    Abc_NtkCleanMarkA( pNtk );
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
        fprintf( pFile, " %s", (char*)pObj->pData );
    else if ( Abc_ObjIsNode(pObj) && Abc_NtkIsMappedLogic(pObj->pNtk) )
        fprintf( pFile, " %s\n", Mio_GateReadName((Mio_Gate_t *)pObj->pData) );
    else
        fprintf( pFile, "\n" );
}


/**Function*************************************************************

  Synopsis    [Checks the status of the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintMiter( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pChild, * pConst1 = Abc_AigConst1(pNtk);
    int i, iOut = -1;
    abctime Time = Abc_Clock();
    int nUnsat = 0;
    int nSat   = 0;
    int nUndec = 0;
    int nPis   = 0;
    Abc_NtkForEachPi( pNtk, pObj, i )
        nPis += (int)( Abc_ObjFanoutNum(pObj) > 0 );
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        pChild = Abc_ObjChild0(pObj);
        // check if the output is constant 0
        if ( pChild == Abc_ObjNot(pConst1) )
            nUnsat++;
        // check if the output is constant 1
        else if ( pChild == pConst1 )
        {
            nSat++;
            if ( iOut == -1 )
                iOut = i;
        }
        // check if the output is a primary input
        else if ( Abc_ObjIsPi(Abc_ObjRegular(pChild)) )
        {
            nSat++;
            if ( iOut == -1 )
                iOut = i;
        }
    // check if the output is 1 for the 0000 pattern
        else if ( Abc_ObjRegular(pChild)->fPhase != (unsigned)Abc_ObjIsComplement(pChild) )
        {
            nSat++;
            if ( iOut == -1 )
                iOut = i;
        }
        else
            nUndec++;
    }
    printf( "Miter:  I =%6d", nPis );
    printf( "  N =%7d", Abc_NtkNodeNum(pNtk) );
    printf( "  ? =%7d", nUndec );
    printf( "  U =%6d", nUnsat );
    printf( "  S =%6d", nSat );
    Time = Abc_Clock() - Time;
    printf(" %7.2f sec\n", (float)(Time)/(float)(CLOCKS_PER_SEC));
    if ( iOut >= 0 )
        printf( "The first satisfiable output is number %d (%s).\n", iOut, Abc_ObjName( Abc_NtkPo(pNtk, iOut) ) );
}

/**Function*************************************************************

  Synopsis    [Checks the status of the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintPoEquivs( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pDriver, * pRepr;  int i, iRepr;
    Vec_Int_t * vMap = Vec_IntStartFull( Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        pDriver = Abc_ObjFanin0(pObj);
        if ( Abc_NtkIsStrash(pNtk) && pDriver == Abc_AigConst1(pNtk) )
        {
            printf( "%s = Const%d\n", Abc_ObjName(pObj), !Abc_ObjFaninC0(pObj) );
            continue;
        }
        else if ( !Abc_NtkIsStrash(pNtk) && Abc_NodeIsConst(pDriver) )
        {
            printf( "%s = Const%d\n", Abc_ObjName(pObj), Abc_NodeIsConst1(pDriver) );
            continue;
        }
        iRepr = Vec_IntEntry( vMap, Abc_ObjId(pDriver) );
        if ( iRepr == -1 )
        {
            Vec_IntWriteEntry( vMap, Abc_ObjId(pDriver), i );
            continue;
        }
        pRepr = Abc_NtkCo(pNtk, iRepr);
        printf( "%s = %s%s\n", Abc_ObjName(pObj), Abc_ObjFaninC0(pRepr) == Abc_ObjFaninC0(pObj) ? "" : "!", Abc_ObjName(pRepr) );
    }
    Vec_IntFree( vMap );
}




typedef struct Gli_Man_t_ Gli_Man_t;

extern Gli_Man_t * Gli_ManAlloc( int nObjs, int nRegs, int nFanioPairs );
extern void        Gli_ManStop( Gli_Man_t * p );
extern int         Gli_ManCreateCi( Gli_Man_t * p, int nFanouts );
extern int         Gli_ManCreateCo( Gli_Man_t * p, int iFanin );
extern int         Gli_ManCreateNode( Gli_Man_t * p, Vec_Int_t * vFanins, int nFanouts, word * pGateTruth );

extern void        Gli_ManSwitchesAndGlitches( Gli_Man_t * p, int nPatterns, float PiTransProb, int fVerbose );
extern int         Gli_ObjNumSwitches( Gli_Man_t * p, int iNode );
extern int         Gli_ObjNumGlitches( Gli_Man_t * p, int iNode );

/**Function*************************************************************

  Synopsis    [Returns the percentable of increased power due to glitching.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_NtkMfsTotalGlitchingLut( Abc_Ntk_t * pNtk, int nPats, int Prob, int fVerbose )
{
    int nSwitches, nGlitches;
    Gli_Man_t * p;
    Vec_Ptr_t * vNodes;
    Vec_Int_t * vFanins, * vTruth;
    Abc_Obj_t * pObj, * pFanin;
    Vec_Wrd_t * vTruths; word * pTruth;
    unsigned * puTruth;
    int i, k;
    assert( Abc_NtkIsLogic(pNtk) );
    assert( Abc_NtkGetFaninMax(pNtk) <= 6 );
    if ( Abc_NtkGetFaninMax(pNtk) > 6 )
    {
        printf( "Abc_NtkMfsTotalGlitching() This procedure works only for mapped networks with LUTs size up to 6 inputs.\n" );
        return -1.0;
    }
    Abc_NtkToAig( pNtk );
    vNodes = Abc_NtkDfs( pNtk, 0 );
    vFanins = Vec_IntAlloc( 6 );
    vTruth = Vec_IntAlloc( 1 << 12 );
    vTruths = Vec_WrdStart( Abc_NtkObjNumMax(pNtk) );

    // derive network for glitch computation
    p = Gli_ManAlloc( Vec_PtrSize(vNodes) + Abc_NtkCiNum(pNtk) + Abc_NtkCoNum(pNtk),
        Abc_NtkLatchNum(pNtk), Abc_NtkGetTotalFanins(pNtk) + Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->iTemp = -1;
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->iTemp = Gli_ManCreateCi( p, Abc_ObjFanoutNum(pObj) );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        Vec_IntClear( vFanins );
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Vec_IntPush( vFanins, pFanin->iTemp );
        puTruth = Hop_ManConvertAigToTruth( (Hop_Man_t *)pNtk->pManFunc, (Hop_Obj_t *)pObj->pData, Abc_ObjFaninNum(pObj), vTruth, 0 );
        pTruth = Vec_WrdEntryP( vTruths, Abc_ObjId(pObj) );
        *pTruth = ((word)puTruth[Abc_ObjFaninNum(pObj) == 6] << 32) | (word)puTruth[0];
        pObj->iTemp = Gli_ManCreateNode( p, vFanins, Abc_ObjFanoutNum(pObj), pTruth );
    }
    Abc_NtkForEachCo( pNtk, pObj, i )
        Gli_ManCreateCo( p, Abc_ObjFanin0(pObj)->iTemp );

    // compute glitching
    Gli_ManSwitchesAndGlitches( p, 4000, 1.0/8.0, 0 );

    // compute the ratio
    nSwitches = nGlitches = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( pObj->iTemp >= 0 )
        {
            nSwitches += Abc_ObjFanoutNum(pObj) * Gli_ObjNumSwitches(p, pObj->iTemp);
            nGlitches += Abc_ObjFanoutNum(pObj) * Gli_ObjNumGlitches(p, pObj->iTemp);
        }

    Gli_ManStop( p );
    Vec_PtrFree( vNodes );
    Vec_IntFree( vTruth );
    Vec_IntFree( vFanins );
    Vec_WrdFree( vTruths );
    return nSwitches ? 100.0*(nGlitches-nSwitches)/nSwitches : 0.0;
}

/**Function*************************************************************

  Synopsis    [Returns the percentable of increased power due to glitching.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_NtkMfsTotalGlitching( Abc_Ntk_t * pNtk, int nPats, int Prob, int fVerbose )
{
    int nSwitches, nGlitches;
    Gli_Man_t * p;
    Vec_Ptr_t * vNodes;
    Vec_Int_t * vFanins;
    Abc_Obj_t * pObj, * pFanin;
    int i, k, nFaninMax = Abc_NtkGetFaninMax(pNtk);
    if ( !Abc_NtkIsMappedLogic(pNtk) )
        return Abc_NtkMfsTotalGlitchingLut( pNtk, nPats, Prob, fVerbose );
    assert( Abc_NtkIsMappedLogic(pNtk) );
    if ( nFaninMax > 16 )
    {
        printf( "Abc_NtkMfsTotalGlitching() This procedure works only for mapped networks with LUTs size up to 6 inputs.\n" );
        return -1.0;
    }
    vNodes = Abc_NtkDfs( pNtk, 0 );
    vFanins = Vec_IntAlloc( 6 );

    // derive network for glitch computation
    p = Gli_ManAlloc( Vec_PtrSize(vNodes) + Abc_NtkCiNum(pNtk) + Abc_NtkCoNum(pNtk),
        Abc_NtkLatchNum(pNtk), Abc_NtkGetTotalFanins(pNtk) + Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->iTemp = -1;
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->iTemp = Gli_ManCreateCi( p, Abc_ObjFanoutNum(pObj) );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        Vec_IntClear( vFanins );
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Vec_IntPush( vFanins, pFanin->iTemp );
        pObj->iTemp = Gli_ManCreateNode( p, vFanins, Abc_ObjFanoutNum(pObj), Mio_GateReadTruthP((Mio_Gate_t *)pObj->pData) );
    }
    Abc_NtkForEachCo( pNtk, pObj, i )
        Gli_ManCreateCo( p, Abc_ObjFanin0(pObj)->iTemp );

    // compute glitching
    Gli_ManSwitchesAndGlitches( p, nPats, 1.0/Prob, fVerbose );

    // compute the ratio
    nSwitches = nGlitches = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( pObj->iTemp >= 0 )
        {
            nSwitches += Abc_ObjFanoutNum(pObj) * Gli_ObjNumSwitches(p, pObj->iTemp);
            nGlitches += Abc_ObjFanoutNum(pObj) * Gli_ObjNumGlitches(p, pObj->iTemp);
        }

    Gli_ManStop( p );
    Vec_PtrFree( vNodes );
    Vec_IntFree( vFanins );
    return nSwitches ? 100.0*(nGlitches-nSwitches)/nSwitches : 0.0;
}

/**Function*************************************************************

  Synopsis    [Prints K-map of 6-var function represented by truth table.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_Show6VarFunc( word F0, word F1 )
{
    // order of cells in the Karnaugh map
//    int Cells[8] = { 0, 1, 3, 2, 6, 7, 5, 4 };
    int Cells[8] = { 0, 4, 6, 2, 3, 7, 5, 1 };
    // intermediate variables
    int s; // symbol counter
    int h; // horizontal coordinate;
    int v; // vertical coordinate;
    assert( (F0 & F1) == 0 );

    // output minterms above
    for ( s = 0; s < 4; s++ )
        printf( " " );
    printf( " " );
    for ( h = 0; h < 8; h++ )
    {
        for ( s = 0; s < 3; s++ )
            printf( "%d",  ((Cells[h] >> (2-s)) & 1) );
        printf( " " );
    }
    printf( "\n" );

    // output horizontal line above
    for ( s = 0; s < 4; s++ )
        printf( " " );
    printf( "+" );
    for ( h = 0; h < 8; h++ )
    {
        for ( s = 0; s < 3; s++ )
            printf( "-" );
        printf( "+" );
    }
    printf( "\n" );

    // output lines with function values
    for ( v = 0; v < 8; v++ )
    {
        for ( s = 0; s < 3; s++ )
            printf( "%d",  ((Cells[v] >> (2-s)) & 1) );
        printf( " |" );

        for ( h = 0; h < 8; h++ )
        {
            printf( " " );
            if ( ((F0 >> ((Cells[v]*8)+Cells[h])) & 1) )
                printf( "0" );
            else if ( ((F1 >> ((Cells[v]*8)+Cells[h])) & 1) )
                printf( "1" );
            else
                printf( " " );
            printf( " |" );
        }
        printf( "\n" );

        // output horizontal line above
        for ( s = 0; s < 4; s++ )
            printf( " " );
//        printf( "%c", v == 7 ? '+' : '|' );
        printf( "+" );
        for ( h = 0; h < 8; h++ )
        {
            for ( s = 0; s < 3; s++ )
                printf( "-" );
//            printf( "%c", v == 7 ? '+' : '|' );
            printf( "%c", (v == 7 || h == 7) ? '+' : '|' );
        }
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Prints K-map of 6-var function represented by truth table.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkShow6VarFunc( char * pF0, char * pF1 )
{
    word F0, F1;
    if ( strlen(pF0) != 16 )
    {
        printf( "Wrong length (%d) of 6-var truth table.\n", (int)strlen(pF0) );
        return;
    }
    if ( strlen(pF1) != 16 )
    {
        printf( "Wrong length (%d) of 6-var truth table.\n", (int)strlen(pF1) );
        return;
    }
    Extra_ReadHexadecimal( (unsigned *)&F0, pF0, 6 );
    Extra_ReadHexadecimal( (unsigned *)&F1, pF1, 6 );
    Abc_Show6VarFunc( F0, F1 );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
