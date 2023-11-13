/**CFile****************************************************************

  FileName    [giaMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Package manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <ctype.h>

#include "gia.h"
#include "misc/tim/tim.h"
#include "proof/abs/abs.h"
#include "opt/dar/dar.h"
#include "misc/extra/extra.h"

#ifdef WIN32
#include <windows.h>
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void Gia_ManDfsSlacksPrint( Gia_Man_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManStart( int nObjsMax )
{
    Gia_Man_t * p;
    assert( nObjsMax > 0 );
    p = ABC_CALLOC( Gia_Man_t, 1 );
    p->nObjsAlloc = nObjsMax;
    p->pObjs = ABC_CALLOC( Gia_Obj_t, nObjsMax );
    p->pObjs->iDiff0 = p->pObjs->iDiff1 = GIA_NONE;
    p->nObjs = 1;
    p->vCis  = Vec_IntAlloc( nObjsMax / 20 );
    p->vCos  = Vec_IntAlloc( nObjsMax / 20 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManStop( Gia_Man_t * p )
{
    if ( p->vSeqModelVec )
        Vec_PtrFreeFree( p->vSeqModelVec );
    Gia_ManStaticFanoutStop( p );
    Tim_ManStopP( (Tim_Man_t **)&p->pManTime );
    assert( p->pManTime == NULL );
    Vec_PtrFreeFree( p->vNamesIn );
    Vec_PtrFreeFree( p->vNamesOut );
    Vec_PtrFreeFree( p->vNamesNode );
    Vec_IntFreeP( &p->vSwitching );
    Vec_IntFreeP( &p->vSuper );
    Vec_IntFreeP( &p->vStore );
    Vec_IntFreeP( &p->vClassNew );
    Vec_IntFreeP( &p->vClassOld );
    Vec_IntFreeP( &p->vPats );
    Vec_BitFreeP( &p->vPolars );
    Vec_WrdFreeP( &p->vSims );
    Vec_WrdFreeP( &p->vSimsT );
    Vec_WrdFreeP( &p->vSimsPi );
    Vec_WrdFreeP( &p->vSimsPo );
    Vec_IntFreeP( &p->vTimeStamps );
    Vec_FltFreeP( &p->vTiming );
    Vec_VecFreeP( &p->vClockDoms );
    Vec_IntFreeP( &p->vCofVars );
    Vec_IntFreeP( &p->vIdsOrig );
    Vec_IntFreeP( &p->vIdsEquiv );
    Vec_IntFreeP( &p->vLutConfigs );
    Vec_IntFreeP( &p->vEdgeDelay );
    Vec_IntFreeP( &p->vEdgeDelayR );
    Vec_IntFreeP( &p->vEdge1 );
    Vec_IntFreeP( &p->vEdge2 );
    Vec_IntFreeP( &p->vUserPiIds );
    Vec_IntFreeP( &p->vUserPoIds );
    Vec_IntFreeP( &p->vUserFfIds );
    Vec_IntFreeP( &p->vFlopClasses );
    Vec_IntFreeP( &p->vGateClasses );
    Vec_IntFreeP( &p->vObjClasses );
    Vec_IntFreeP( &p->vInitClasses );
    Vec_IntFreeP( &p->vRegClasses );
    Vec_IntFreeP( &p->vRegInits );
    Vec_IntFreeP( &p->vDoms );
    Vec_IntFreeP( &p->vBarBufs );
    Vec_IntFreeP( &p->vXors );
    Vec_IntFreeP( &p->vLevels );
    Vec_IntFreeP( &p->vTruths );
    Vec_IntErase( &p->vCopies );
    Vec_IntErase( &p->vCopies2 );
    Vec_IntFreeP( &p->vVar2Obj );
    Vec_IntErase( &p->vCopiesTwo );
    Vec_IntErase( &p->vSuppVars );
    Vec_IntErase( &p->vVarMap );
    Vec_WrdFreeP( &p->vSuppWords );
    Vec_IntFreeP( &p->vTtNums );
    Vec_IntFreeP( &p->vTtNodes );
    Vec_WrdFreeP( &p->vTtMemory );
    Vec_PtrFreeP( &p->vTtInputs );
    Vec_IntFreeP( &p->vMapping );
    Vec_WecFreeP( &p->vMapping2 );
    Vec_WecFreeP( &p->vFanouts2 );
    Vec_IntFreeP( &p->vCellMapping );
    Vec_IntFreeP( &p->vPacking );
    Vec_IntFreeP( &p->vConfigs );
    ABC_FREE( p->pCellStr );
    Vec_FltFreeP( &p->vInArrs );
    Vec_FltFreeP( &p->vOutReqs );
    Vec_IntFreeP( &p->vCiArrs );
    Vec_IntFreeP( &p->vCoReqs );
    Vec_IntFreeP( &p->vCoArrs );
    Vec_IntFreeP( &p->vCoAttrs );
    Vec_IntFreeP( &p->vWeights );
    Gia_ManStopP( &p->pAigExtra );
    Vec_IntFree( p->vCis );
    Vec_IntFree( p->vCos );
    Vec_IntErase( &p->vHash );
    Vec_IntErase( &p->vHTable );
    Vec_IntErase( &p->vRefs );
    Vec_StrFreeP( &p->vStopsF );
    Vec_StrFreeP( &p->vStopsB );    
    ABC_FREE( p->pData2 );
    ABC_FREE( p->pTravIds );
    ABC_FREE( p->pPlacement );
    ABC_FREE( p->pSwitching );
    ABC_FREE( p->pCexSeq );
    ABC_FREE( p->pCexComb );
    ABC_FREE( p->pIso );
//    ABC_FREE( p->pMapping );
    ABC_FREE( p->pFanData );
    ABC_FREE( p->pReprsOld );
    ABC_FREE( p->pReprs );
    ABC_FREE( p->pNexts );
    ABC_FREE( p->pSibls );
    ABC_FREE( p->pRefs );
    ABC_FREE( p->pLutRefs );
    ABC_FREE( p->pMuxes );
    ABC_FREE( p->pObjs );
    ABC_FREE( p->pSpec );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns memory used in megabytes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Gia_ManMemory( Gia_Man_t * p )
{
    double Memory = sizeof(Gia_Man_t);
    Memory += sizeof(Gia_Obj_t) * Gia_ManObjNum(p);
    Memory += sizeof(int) * Gia_ManCiNum(p);
    Memory += sizeof(int) * Gia_ManCoNum(p);
    Memory += sizeof(int) * Vec_IntSize(&p->vHTable);
    Memory += sizeof(int) * Gia_ManObjNum(p) * (p->pRefs != NULL);
    Memory += Vec_IntMemory( p->vLevels );
    Memory += Vec_IntMemory( p->vCellMapping );
    Memory += Vec_IntMemory( &p->vCopies );
    Memory += Vec_FltMemory( p->vInArrs );
    Memory += Vec_FltMemory( p->vOutReqs );
    Memory += Vec_PtrMemory( p->vNamesIn );
    Memory += Vec_PtrMemory( p->vNamesOut );
    Memory += Vec_PtrMemory( p->vNamesNode );
    return Memory;
}

/**Function*************************************************************

  Synopsis    [Stops the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManStopP( Gia_Man_t ** p )
{
    if ( *p == NULL )
        return;
    Gia_ManStop( *p );
    *p = NULL;
}

/**Function*************************************************************

  Synopsis    [Prints stats for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintClasses_old( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    if ( p->vFlopClasses == NULL )
        return;
    Gia_ManForEachRo( p, pObj, i )
        Abc_Print( 1, "%d", Vec_IntEntry(p->vFlopClasses, i) );
    Abc_Print( 1, "\n" );

    {
        Gia_Man_t * pTemp;
        pTemp = Gia_ManDupFlopClass( p, 1 );
        Gia_AigerWrite( pTemp, "dom1.aig", 0, 0, 0 );
        Gia_ManStop( pTemp );
        pTemp = Gia_ManDupFlopClass( p, 2 );
        Gia_AigerWrite( pTemp, "dom2.aig", 0, 0, 0 );
        Gia_ManStop( pTemp );
    }
}

/**Function*************************************************************

  Synopsis    [Prints stats for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintPlacement( Gia_Man_t * p )
{
    int i, nFixed = 0, nUndef = 0;
    if ( p->pPlacement == NULL )
        return;
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
    {
        nFixed += p->pPlacement[i].fFixed;
        nUndef += p->pPlacement[i].fUndef;
    }
    Abc_Print( 1, "Placement:  Objects = %8d.  Fixed = %8d.  Undef = %8d.\n", Gia_ManObjNum(p), nFixed, nUndef );
}


/**Function*************************************************************

  Synopsis    [Duplicates AIG for unrolling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintTents_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vObjs )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    Vec_IntPush( vObjs, Gia_ObjId(p, pObj) );
    if ( Gia_ObjIsCi(pObj) )
        return;
    Gia_ManPrintTents_rec( p, Gia_ObjFanin0(pObj), vObjs );
    if ( Gia_ObjIsAnd(pObj) )
        Gia_ManPrintTents_rec( p, Gia_ObjFanin1(pObj), vObjs );
}
void Gia_ManPrintTents( Gia_Man_t * p )
{
    Vec_Int_t * vObjs;
    Gia_Obj_t * pObj;
    int t, i, iObjId, nSizePrev, nSizeCurr;
    assert( Gia_ManPoNum(p) > 0 );
    vObjs = Vec_IntAlloc( 100 );
    // save constant class
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    Vec_IntPush( vObjs, 0 );
    // create starting root
    nSizePrev = Vec_IntSize(vObjs);
    Gia_ManForEachPo( p, pObj, i )
        Gia_ManPrintTents_rec( p, pObj, vObjs );
    // build tents
    Abc_Print( 1, "Tents:  " );
    for ( t = 1; nSizePrev < Vec_IntSize(vObjs); t++ )
    {
        int nPis = 0;
        nSizeCurr = Vec_IntSize(vObjs);
        Vec_IntForEachEntryStartStop( vObjs, iObjId, i, nSizePrev, nSizeCurr )
        {
            nPis += Gia_ObjIsPi(p, Gia_ManObj(p, iObjId));
            if ( Gia_ObjIsRo(p, Gia_ManObj(p, iObjId)) )
                Gia_ManPrintTents_rec( p, Gia_ObjRoToRi(p, Gia_ManObj(p, iObjId)), vObjs );
        }
        Abc_Print( 1, "%d=%d(%d)  ", t, nSizeCurr - nSizePrev, nPis );
        nSizePrev = nSizeCurr;
    }
    Abc_Print( 1, " Unused=%d\n", Gia_ManObjNum(p) - Vec_IntSize(vObjs) );
    Vec_IntFree( vObjs );
    // the remaining objects are PIs without fanout
//    Gia_ManForEachObj( p, pObj, i )
//        if ( !Gia_ObjIsTravIdCurrent(p, pObj) )
//            Gia_ObjPrint( p, pObj );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintInitClasses( Vec_Int_t * vInits )
{
    int i, Value;
    int Counts[6] = {0};
    Vec_IntForEachEntry( vInits, Value, i )
        Counts[Value]++;
    for ( i = 0; i < 6; i++ )
        if ( Counts[i] )
            printf( "%d = %d  ", i, Counts[i] );
    printf( "  " );
    printf( "B = %d  ", Counts[0] + Counts[1] );
    printf( "X = %d  ", Counts[2] + Counts[3] );
    printf( "Q = %d\n", Counts[4] + Counts[5] );
    Vec_IntForEachEntry( vInits, Value, i )
    {
        Counts[Value]++;
        if ( Value == 0 )
            printf( "0" );
        else if ( Value == 1 )
            printf( "1" );
        else if ( Value == 2 )
            printf( "2" );
        else if ( Value == 3 )
            printf( "3" );
        else if ( Value == 4 )
            printf( "4" );
        else if ( Value == 5 )
            printf( "5" );
        else assert( 0 );
    }
    printf( "\n" );
    
}

/**Function*************************************************************

  Synopsis    [Prints stats for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintChoiceStats( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, nEquivs = 0, nChoices = 0;
    Gia_ManMarkFanoutDrivers( p );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( !Gia_ObjSibl(p, i) )
            continue;
        nEquivs++;
        if ( pObj->fMark0 )
            nChoices++;
        assert( !Gia_ObjSiblObj(p, i)->fMark0 );
        assert( Gia_ObjIsAnd(Gia_ObjSiblObj(p, i)) );
    }
    Abc_Print( 1, "Choice stats: Equivs =%7d. Choices =%7d.\n", nEquivs, nChoices );
    Gia_ManCleanMark0( p );
}


/**Function*************************************************************

  Synopsis    [Prints stats for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManPrintEdges( Gia_Man_t * p )
{
    printf( "Edges (Q=2)    :                " );
    printf( "edge =%8d  ", (Vec_IntCountPositive(p->vEdge1) + Vec_IntCountPositive(p->vEdge2))/2 );
    printf( "lev =%5.1f",  0.1*Gia_ManEvalEdgeDelay(p) );
    printf( "\n" );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Prints stats for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
void Gia_ManLogAigStats( Gia_Man_t * p, char * pDumpFile )
{
    FILE * pTable = fopen( pDumpFile, "wb" );
    fprintf( pTable, "Name = %24s     ", p->pName );
    fprintf( pTable, "In = %6d   ",      Gia_ManCiNum(p) );
    fprintf( pTable, "Out = %6d   ",     Gia_ManCoNum(p) );
    fprintf( pTable, "And = %8d   ",     Gia_ManAndNum(p) );
    fprintf( pTable, "Lev = %6d",        Gia_ManLevelNum(p) );
    fprintf( pTable, "\n" );
    fclose( pTable );
}
*/
void Gia_ManLogAigStats( Gia_Man_t * p, char * pDumpFile )
{
    FILE * pTable = fopen( pDumpFile, "wb" );
    fprintf( pTable, "{\n" );
    fprintf( pTable, "    \"name\" : \"%s\",\n", p->pName );
    fprintf( pTable, "    \"input\" : %d,\n",    Gia_ManCiNum(p) );
    fprintf( pTable, "    \"output\" : %d,\n",   Gia_ManCoNum(p) );
    fprintf( pTable, "    \"and\" : %d,\n",      Gia_ManAndNum(p) );
    fprintf( pTable, "    \"level\" : %d\n",     Gia_ManLevelNum(p) );
    fprintf( pTable, "}\n" );
    fclose( pTable );
}

/**Function*************************************************************

  Synopsis    [Prints stats for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintStats( Gia_Man_t * p, Gps_Par_t * pPars )
{
    extern float Gia_ManLevelAve( Gia_Man_t * p );
    int fHaveLevels = p->vLevels != NULL;
    if ( pPars && pPars->fMiter )
    {
        Gia_ManPrintStatsMiter( p, 0 );
        return;
    }
    if ( pPars && pPars->fNoColor )
    {
        if ( p->pName )
            Abc_Print( 1, "%-8s : ", p->pName );
    }
    else
    {
#ifdef WIN32
    SetConsoleTextAttribute( GetStdHandle(STD_OUTPUT_HANDLE), 15 ); // bright
    if ( p->pName )
        Abc_Print( 1, "%-8s : ", p->pName );
    SetConsoleTextAttribute( GetStdHandle(STD_OUTPUT_HANDLE), 7 );  // normal
#else
    if ( p->pName )
        Abc_Print( 1, "%s%-8s%s : ", "\033[1;37m", p->pName, "\033[0m" );  // bright
#endif
    }
    Abc_Print( 1, "i/o =%7d/%7d", 
        Gia_ManPiNum(p) - Gia_ManBoxCiNum(p) - Gia_ManRegBoxNum(p), 
        Gia_ManPoNum(p) - Gia_ManBoxCoNum(p) - Gia_ManRegBoxNum(p) );
    if ( Gia_ManConstrNum(p) )
        Abc_Print( 1, "(c=%d)", Gia_ManConstrNum(p) );
    if ( Gia_ManRegNum(p) )
        Abc_Print( 1, "  ff =%7d", Gia_ManRegNum(p) );
    if ( Gia_ManRegBoxNum(p) )
        Abc_Print( 1, "  boxff =%d(%d)", Gia_ManRegBoxNum(p), Gia_ManClockDomainNum(p) );
    if ( pPars && pPars->fNoColor )
    {
        Abc_Print( 1, "  %s =%8d", p->pMuxes? "nod" : "and", Gia_ManAndNum(p) );
        Abc_Print( 1, "  lev =%5d", Gia_ManLevelNum(p) ); 
        Abc_Print( 1, " (%.2f)", Gia_ManLevelAve(p) ); 
    }
    else
    {
#ifdef WIN32
    {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute( hConsole, 11 ); // blue
    Abc_Print( 1, "  %s =%8d", p->pMuxes? "nod" : "and", Gia_ManAndNum(p) );
    SetConsoleTextAttribute( hConsole, 13 ); // magenta
    Abc_Print( 1, "  lev =%5d", Gia_ManLevelNum(p) ); 
    Abc_Print( 1, " (%7.2f)", Gia_ManLevelAve(p) ); 
    SetConsoleTextAttribute( hConsole, 7 ); // normal
    }
#else
    Abc_Print( 1, "  %s%s =%8d%s",  "\033[1;36m", p->pMuxes? "nod" : "and", Gia_ManAndNum(p), "\033[0m" ); // blue
    Abc_Print( 1, "  %slev =%5d%s", "\033[1;35m", Gia_ManLevelNum(p), "\033[0m" ); // magenta
    Abc_Print( 1, " %s(%.2f)%s",    "\033[1;35m", Gia_ManLevelAve(p), "\033[0m" ); 
#endif
    }
    if ( !fHaveLevels ) 
        Vec_IntFreeP( &p->vLevels );
    if ( pPars && pPars->fCut )
        Abc_Print( 1, "  cut = %d(%d)", Gia_ManCrossCut(p, 0), Gia_ManCrossCut(p, 1) );
    Abc_Print( 1, "  mem =%5.2f MB", Gia_ManMemory(p)/(1<<20) );
    if ( Gia_ManHasChoices(p) )
        Abc_Print( 1, "  ch =%5d", Gia_ManChoiceNum(p) );
    if ( p->pManTime )
        Abc_Print( 1, "  box = %d", Gia_ManNonRegBoxNum(p) );
    if ( p->pManTime )
        Abc_Print( 1, "  bb = %d", Gia_ManBlackBoxNum(p) );
    if ( Gia_ManBufNum(p) )
        Abc_Print( 1, "  buf = %d", Gia_ManBufNum(p) );
    if ( Gia_ManXorNum(p) && p->pMuxes == NULL )
        Abc_Print( 1, "  xor = %d", Gia_ManXorNum(p) );
    if ( pPars && pPars->fMuxXor )
        printf( "\nXOR/MUX " ), Gia_ManPrintMuxStats( p );
    if ( pPars && pPars->fSwitch )
    {
        static int nPiPo = 0;
        static float PrevSwiTotal = 0;
        float SwiTotal = Gia_ManComputeSwitching( p, 48, 16, 0 );
        Abc_Print( 1, "  power =%8.1f", SwiTotal );
        if ( PrevSwiTotal > 0 && nPiPo == Gia_ManCiNum(p) + Gia_ManCoNum(p) )
            Abc_Print( 1, " %6.2f %%", 100.0*(PrevSwiTotal-SwiTotal)/PrevSwiTotal );
        else if ( PrevSwiTotal == 0 || nPiPo != Gia_ManCiNum(p) + Gia_ManCoNum(p) )
            PrevSwiTotal = SwiTotal, nPiPo = Gia_ManCiNum(p) + Gia_ManCoNum(p);
    }
//    Abc_Print( 1, "obj =%5d  ", Gia_ManObjNum(p) );
    Abc_Print( 1, "\n" );

//    Gia_ManSatExperiment( p );
    if ( p->pReprs && p->pNexts )
        Gia_ManEquivPrintClasses( p, 0, 0.0 );
    if ( Gia_ManHasMapping(p) && (pPars == NULL || !pPars->fSkipMap) )
        Gia_ManPrintMappingStats( p, pPars ? pPars->pDumpFile : NULL );
    else if ( pPars && pPars->pDumpFile )
        Gia_ManLogAigStats( p, pPars->pDumpFile );
    if ( pPars && pPars->fNpn && Gia_ManHasMapping(p) )
        Gia_ManPrintNpnClasses( p );
    if ( p->vPacking )
        Gia_ManPrintPackingStats( p );
    if ( p->vEdge1 )
        Gia_ManPrintEdges( p );
    if ( pPars && pPars->fLutProf && Gia_ManHasMapping(p) )
        Gia_ManPrintLutStats( p );
    if ( p->pPlacement )
        Gia_ManPrintPlacement( p );
//    if ( p->pManTime )
//        Tim_ManPrintStats( (Tim_Man_t *)p->pManTime, p->nAnd2Delay );
    Gia_ManPrintFlopClasses( p );
    Gia_ManPrintGateClasses( p );
    Gia_ManPrintObjClasses( p );
//    if ( p->vRegClasses )
//    {
//        printf( "The design has %d flops with the following class info: ", Vec_IntSize(p->vRegClasses) );
//        Vec_IntPrint( p->vRegClasses );
//    }
    if ( p->vInitClasses )
        Gia_ManPrintInitClasses( p->vInitClasses );
    // check integrity of boxes
    Gia_ManCheckIntegrityWithBoxes( p );
/*
    if ( Gia_ManRegBoxNum(p) )
    {
        int i, Limit = Vec_IntFindMax(p->vRegClasses);
        for ( i = 1; i <= Limit; i++ )
            printf( "%d ", Vec_IntCountEntry(p->vRegClasses, i) );
        printf( "\n" );
    }
*/
    if ( pPars && pPars->fTents )
    {
/*
        int k, Entry, Prev = 1;
        Vec_Int_t * vLimit = Vec_IntAlloc( 1000 );
        Gia_Man_t * pNew = Gia_ManUnrollDup( p, vLimit );
        Abc_Print( 1, "Tents:  " );
        Vec_IntForEachEntryStart( vLimit, Entry, k, 1 )
            Abc_Print( 1, "%d=%d  ", k, Entry-Prev ), Prev = Entry;
        Abc_Print( 1, " Unused=%d.", Gia_ManObjNum(p) - Gia_ManObjNum(pNew) );
        Abc_Print( 1, "\n" );
        Vec_IntFree( vLimit );
        Gia_ManStop( pNew );
*/
        Gia_ManPrintTents( p );
    }
    if ( pPars && pPars->fSlacks )
        Gia_ManDfsSlacksPrint( p );
}

/**Function*************************************************************

  Synopsis    [Prints stats for the AIG.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintStatsShort( Gia_Man_t * p )
{
    Abc_Print( 1, "i/o =%7d/%7d  ", Gia_ManPiNum(p), Gia_ManPoNum(p) );
    Abc_Print( 1, "ff =%7d  ", Gia_ManRegNum(p) );
    Abc_Print( 1, "and =%8d  ", Gia_ManAndNum(p) );
    Abc_Print( 1, "lev =%5d  ", Gia_ManLevelNum(p) );
//    Abc_Print( 1, "mem =%5.2f MB", 12.0*Gia_ManObjNum(p)/(1<<20) );
    Abc_Print( 1, "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints stats for the AIG.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintMiterStatus( Gia_Man_t * p )
{
    Gia_Obj_t * pObj, * pChild;
    int i, nSat = 0, nUnsat = 0, nUndec = 0, iOut = -1;
    Gia_ManForEachPo( p, pObj, i )
    {
        pChild = Gia_ObjChild0(pObj);
        // check if the output is constant 0
        if ( pChild == Gia_ManConst0(p) )
            nUnsat++;
        // check if the output is constant 1
        else if ( pChild == Gia_ManConst1(p) )
        {
            nSat++;
            if ( iOut == -1 )
                iOut = i;
        }
        // check if the output is a primary input
        else if ( Gia_ObjIsPi(p, Gia_Regular(pChild)) )
        {
            nSat++;
            if ( iOut == -1 )
                iOut = i;
        }
/*
        // check if the output is 1 for the 0000 pattern
        else if ( Gia_Regular(pChild)->fPhase != (unsigned)Gia_IsComplement(pChild) )
        {
            nSat++;
            if ( iOut == -1 )
                iOut = i;
        }
*/
        else
            nUndec++;
    }
    Abc_Print( 1, "Outputs = %7d.  Unsat = %7d.  Sat = %7d.  Undec = %7d.\n",
        Gia_ManPoNum(p), nUnsat, nSat, nUndec );
}

/**Function*************************************************************

  Synopsis    [Statistics of the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintStatsMiter( Gia_Man_t * p, int fVerbose )
{
    Gia_Obj_t * pObj;
    Vec_Flt_t * vProb;
    int i, iObjId;
    Gia_ManLevelNum( p );
    Gia_ManCreateRefs( p );
    vProb = Gia_ManPrintOutputProb( p );
    printf( "Statistics for each outputs of the miter:\n" );
    Gia_ManForEachPo( p, pObj, i )
    {
        iObjId = Gia_ObjId(p, pObj);
        printf( "%4d : ", i );
        printf( "Level = %5d  ",  Gia_ObjLevelId(p, iObjId) );
        printf( "Supp = %5d  ",   Gia_ManSuppSize(p, &iObjId, 1) );
        printf( "Cone = %5d  ",   Gia_ManConeSize(p, &iObjId, 1) );
        printf( "Mffc = %5d  ",   Gia_NodeMffcSize(p, Gia_ObjFanin0(pObj)) );
        printf( "Prob = %8.4f  ", Vec_FltEntry(vProb, iObjId) );
        printf( "\n" );
    }
    Vec_FltFree( vProb );
}

/**Function*************************************************************

  Synopsis    [Prints stats for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSetRegNum( Gia_Man_t * p, int nRegs )
{
    assert( p->nRegs == 0 );
    p->nRegs = nRegs;
}


/**Function*************************************************************

  Synopsis    [Reports the reduction of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManReportImprovement( Gia_Man_t * p, Gia_Man_t * pNew )
{
    Abc_Print( 1, "REG: Beg = %5d. End = %5d. (R =%5.1f %%)  ",
        Gia_ManRegNum(p), Gia_ManRegNum(pNew),
        Gia_ManRegNum(p)? 100.0*(Gia_ManRegNum(p)-Gia_ManRegNum(pNew))/Gia_ManRegNum(p) : 0.0 );
    Abc_Print( 1, "AND: Beg = %6d. End = %6d. (R =%5.1f %%)",
        Gia_ManAndNum(p), Gia_ManAndNum(pNew),
        Gia_ManAndNum(p)? 100.0*(Gia_ManAndNum(p)-Gia_ManAndNum(pNew))/Gia_ManAndNum(p) : 0.0 );
    Abc_Print( 1, "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints NPN class statistics.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintNpnClasses( Gia_Man_t * p )
{
    extern char ** Kit_DsdNpn4ClassNames();
    char ** pNames = Kit_DsdNpn4ClassNames();
    Vec_Int_t * vLeaves, * vTruth, * vVisited;
    int * pLutClass, ClassCounts[222] = {0};
    int i, k, iFan, Class, OtherClasses, OtherClasses2, nTotal, Counter, Counter2;
    unsigned * pTruth; int nLutSize = 0;
    assert( Gia_ManHasMapping(p) );
    assert(  Gia_ManLutSizeMax( p ) <= 4 );
    vLeaves   = Vec_IntAlloc( 100 );
    vVisited  = Vec_IntAlloc( 100 );
    vTruth    = Vec_IntAlloc( (1<<16) );
    pLutClass = ABC_CALLOC( int, Gia_ManObjNum(p) );
    Gia_ManCleanTruth( p );
    Gia_ManForEachLut( p, i )
    {
        nLutSize = Abc_MaxInt( nLutSize, Gia_ObjLutSize(p,i) );
        if ( Gia_ObjLutSize(p,i) > 4 )
            continue;
        Vec_IntClear( vLeaves );
        Gia_LutForEachFanin( p, i, iFan, k )
            Vec_IntPush( vLeaves, iFan );
        for ( ; k < 4; k++ )
            Vec_IntPush( vLeaves, 0 );
        pTruth = Gia_ManConvertAigToTruth( p, Gia_ManObj(p, i), vLeaves, vTruth, vVisited );
        Class = Dar_LibReturnClass( *pTruth );
        ClassCounts[ Class ]++;
        pLutClass[i] = Class;
    }
    Vec_IntFree( vLeaves );
    Vec_IntFree( vTruth );
    Vec_IntFree( vVisited );
    Vec_IntFreeP( &p->vTruths );
    nTotal = 0;
    for ( i = 0; i < 222; i++ )
        nTotal += ClassCounts[i];
    Abc_Print( 1, "NPN CLASS STATISTICS (for %d LUT4 present in the current mapping):\n", nTotal );
    OtherClasses = 0;
    for ( i = k = 0; i < 222; i++ )
    {
        if ( ClassCounts[i] == 0 )
            continue;
//        if ( 100.0 * ClassCounts[i] / (nTotal+1) < 0.1 ) // do not show anything below 0.1 percent
//            continue;
        OtherClasses += ClassCounts[i];
        Abc_Print( 1, "%3d: Class %3d :  Count = %6d   (%7.2f %%)   %s\n", 
            ++k, i, ClassCounts[i], 100.0 * ClassCounts[i] / (nTotal+1), pNames[i] );
    }
    OtherClasses = nTotal - OtherClasses;
    Abc_Print( 1, "Other     :  Count = %6d   (%7.2f %%)\n", 
        OtherClasses, 100.0 * OtherClasses / (nTotal+1) );
    // count the number of LUTs that have MUX function and two fanins with MUX functions
    OtherClasses = OtherClasses2 = 0;
    ABC_FREE( p->pRefs );
    Gia_ManSetRefsMapped( p );
    Gia_ManForEachLut( p, i )
    {
        if ( pLutClass[i] != 109 )
            continue;
        Counter = Counter2 = 0;
        Gia_LutForEachFanin( p, i, iFan, k )
        {
            Counter  += (pLutClass[iFan] == 109);
            Counter2 += (pLutClass[iFan] == 109) && (Gia_ObjRefNumId(p, iFan) == 1);
        }
        OtherClasses  += (Counter > 1);
        OtherClasses2 += (Counter2 > 1);
//            Abc_Print( 1, "%d -- ", pLutClass[i] );
//            Gia_LutForEachFanin( p, i, iFan, k )
//                Abc_Print( 1, "%d ", pLutClass[iFan] );
//            Abc_Print( 1, "\n" );
    }
    ABC_FREE( p->pRefs );
    Abc_Print( 1, "Approximate number of 4:1 MUX structures: All = %6d  (%7.2f %%)  MFFC = %6d  (%7.2f %%)\n", 
        OtherClasses,  100.0 * OtherClasses  / (nTotal+1),
        OtherClasses2, 100.0 * OtherClasses2 / (nTotal+1) );
    // print information about LUT pairs
    if ( nLutSize <= 4 )
    {
        int nTopPairs = 100, nTopShow = 30;
        int i, j, k, iFan, * pVec = NULL;
        Vec_Int_t * vPairs = Vec_IntAlloc( 3 * nTopPairs );
        Gia_ManForEachLut( p, j ) {
            Gia_LutForEachFanin( p, j, iFan, k ) {
                int Num1 = pLutClass[iFan];
                int Num2 = pLutClass[j];
                assert( Vec_IntSize(vPairs) % 3 == 0 );
                for ( i = 0; i < Vec_IntSize(vPairs); i += 3 )
                    if ( Vec_IntEntry(vPairs, i+0) == Num1 && Vec_IntEntry(vPairs, i+1) == Num2 )
                        break;
                if ( i == Vec_IntSize(vPairs) ) {
                    if ( Vec_IntSize(vPairs) < 3*nTopPairs ) {
                        Vec_IntPush( vPairs, Num1 );
                        Vec_IntPush( vPairs, Num2 );
                        Vec_IntPush( vPairs, 1 );
                    }
                    continue;
                }
                // found this pair
                assert( Vec_IntEntry(vPairs, i+0) == Num1 );
                assert( Vec_IntEntry(vPairs, i+1) == Num2 );                
                Vec_IntAddToEntry( vPairs, i+2, 1 );
                // sort 
                pVec = Vec_IntArray( vPairs );
                while ( i > 0 && pVec[i+2] > pVec[i-1] ) {
                    ABC_SWAP( int, pVec[i+0], pVec[i-3] )
                    ABC_SWAP( int, pVec[i+1], pVec[i-2] )
                    ABC_SWAP( int, pVec[i+2], pVec[i-1] )
                    i -= 3;
                }
                while ( i < Vec_IntSize(vPairs) - 3 && pVec[i+2] < pVec[i+5] ) {
                    ABC_SWAP( int, pVec[i+0], pVec[i+3] )
                    ABC_SWAP( int, pVec[i+1], pVec[i+4] )
                    ABC_SWAP( int, pVec[i+2], pVec[i+5] )
                    i += 3;
                    assert( 0 );
                }                
            }
        }
        pVec = Vec_IntArray( vPairs );
        nTopShow = Abc_MinInt( nTopShow, Vec_IntSize(vPairs)/3 );
        for ( i = 0; i < 3*nTopShow; i += 3 )
            printf( "%3d : (%3d %3d) x %3d\n", i/3, pVec[i+0], pVec[i+1], pVec[i+2] );
        Vec_IntFree( vPairs );
    }    
    ABC_FREE( pLutClass );
}


/**Function*************************************************************

  Synopsis    [Collects internal nodes and boxes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDfsCollect_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vObjs )
{
    if ( Gia_ObjIsTravIdCurrent( p, pObj ) )
        return;
    Gia_ObjSetTravIdCurrent( p, pObj );
    if ( Gia_ObjIsCi(pObj) )
    {
        Tim_Man_t * pManTime = (Tim_Man_t *)p->pManTime;
        if ( pManTime )
        {
            int i, iFirst, nTerms, iBox;
            iBox = Tim_ManBoxForCi( pManTime, Gia_ObjCioId(pObj) );
            if ( iBox >= 0 ) // pObj is a box input
            {
                // mark box outputs
                iFirst = Tim_ManBoxOutputFirst( pManTime, iBox );
                nTerms = Tim_ManBoxOutputNum( pManTime, iBox );
                for ( i = 0; i < nTerms; i++ )
                {
                    pObj = Gia_ManCi( p, iFirst + i );
                    Gia_ObjSetTravIdCurrent( p, pObj );
                }
                // traverse box inputs
                iFirst = Tim_ManBoxInputFirst( pManTime, iBox );
                nTerms = Tim_ManBoxInputNum( pManTime, iBox );
                for ( i = 0; i < nTerms; i++ )
                {
                    pObj = Gia_ManCo( p, iFirst + i );
                    Gia_ManDfsCollect_rec( p, pObj, vObjs );
                }
                // save the box
                Vec_IntPush( vObjs, -iBox-1 );
            }
        }
        return;
    }
    else if ( Gia_ObjIsCo(pObj) )
    {
        Gia_ManDfsCollect_rec( p, Gia_ObjFanin0(pObj), vObjs );
    }
    else if ( Gia_ObjIsAnd(pObj) )
    { 
        int iFan, k, iObj = Gia_ObjId(p, pObj);
        if ( Gia_ManHasMapping(p) )
        {
            assert( Gia_ObjIsLut(p, iObj) );
            Gia_LutForEachFanin( p, iObj, iFan, k )
                Gia_ManDfsCollect_rec( p, Gia_ManObj(p, iFan), vObjs );
        }
        else
        {
            Gia_ManDfsCollect_rec( p, Gia_ObjFanin0(pObj), vObjs );
            Gia_ManDfsCollect_rec( p, Gia_ObjFanin1(pObj), vObjs );
        }
        // save the object
        Vec_IntPush( vObjs, iObj );
    }
    else if ( !Gia_ObjIsConst0(pObj) )
        assert( 0 );
}
Vec_Int_t * Gia_ManDfsCollect( Gia_Man_t * p )
{
    Vec_Int_t * vObjs = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_Obj_t * pObj; int i;
    Gia_ManIncrementTravId( p );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManDfsCollect_rec( p, pObj, vObjs );
    Gia_ManForEachCi( p, pObj, i )
        Gia_ManDfsCollect_rec( p, pObj, vObjs );
    return vObjs;
} 

/**Function*************************************************************

  Synopsis    [Compute arrival/required times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManDfsArrivals( Gia_Man_t * p, Vec_Int_t * vObjs )
{
    Tim_Man_t * pManTime = (Tim_Man_t *)p->pManTime;
    Vec_Int_t * vTimes = Vec_IntStartFull( Gia_ManObjNum(p) );
    Gia_Obj_t * pObj; int j, Entry, k, iFan;
    Vec_IntWriteEntry( vTimes, 0, 0 );
    if ( pManTime ) 
    {
        Tim_ManIncrementTravId( pManTime );
        Gia_ManForEachCi( p, pObj, j )
            if ( j < Tim_ManPiNum(pManTime) )
            {
                float arrTime = Tim_ManGetCiArrival( pManTime, j );
                Vec_IntWriteEntry( vTimes, Gia_ObjId(p, pObj), (int)arrTime );
            }
    }
    else
    {
        Gia_ManForEachCi( p, pObj, j )
            Vec_IntWriteEntry( vTimes, Gia_ObjId(p, pObj), 0 );
    }
    Vec_IntForEachEntry( vObjs, Entry, j )
    {
        if ( Entry < 0 ) // box
        {
            int Time0, iFirst, nTerms, iBox = -Entry-1;
            assert( iBox >= 0 );
            // set arrivals for box inputs
            iFirst = Tim_ManBoxInputFirst( pManTime, iBox );
            nTerms = Tim_ManBoxInputNum( pManTime, iBox );
            for ( k = 0; k < nTerms; k++ )
            {
                pObj  = Gia_ManCo( p, iFirst + k );
                Time0 = Vec_IntEntry( vTimes, Gia_ObjFaninId0p(p, pObj) );
                assert( Time0 >= 0 );
                Tim_ManSetCoArrival( pManTime, Gia_ObjCioId(pObj), Time0 );
            }
            // derive arrivals for box outputs
            iFirst = Tim_ManBoxOutputFirst( pManTime, iBox );
            nTerms = Tim_ManBoxOutputNum( pManTime, iBox );
            for ( k = 0; k < nTerms; k++ )
            {
                pObj  = Gia_ManCi( p, iFirst + k );
                Time0 = Tim_ManGetCiArrival( pManTime, Gia_ObjCioId(pObj) );
                assert( Time0 >= 0 );
                Vec_IntWriteEntry( vTimes, Gia_ObjId(p, pObj), Time0 );
            }
        }
        else if ( Entry > 0 ) // node
        {
            int Time0, Time1, TimeMax = 0;
            if ( Gia_ManHasMapping(p) )
            {
                assert( Gia_ObjIsLut(p, Entry) );
                Gia_LutForEachFanin( p, Entry, iFan, k )
                {
                    Time0 = Vec_IntEntry( vTimes, iFan );
                    assert( Time0 >= 0 );
                    TimeMax = Abc_MaxInt( TimeMax, Time0 );
                }
            }
            else
            {
                pObj  = Gia_ManObj( p, Entry );
                Time0 = Vec_IntEntry( vTimes, Gia_ObjFaninId0(pObj, Entry) );
                Time1 = Vec_IntEntry( vTimes, Gia_ObjFaninId1(pObj, Entry) );
                assert( Time0 >= 0 && Time1 >= 0 );
                TimeMax = Abc_MaxInt( Time0, Time1 );
            }
            Vec_IntWriteEntry( vTimes, Entry, TimeMax + 10 );
        }
        else assert( 0 );
    }
    return vTimes;
}
static inline void Gia_ManDfsUpdateRequired( Vec_Int_t * vTimes, int iObj, int Req )
{
    int *pTime = Vec_IntEntryP( vTimes, iObj );
    if (*pTime == -1 || *pTime > Req)
        *pTime = Req;
}
Vec_Int_t * Gia_ManDfsRequireds( Gia_Man_t * p, Vec_Int_t * vObjs, int ReqTime )
{
    Tim_Man_t * pManTime = (Tim_Man_t *)p->pManTime;
    Vec_Int_t * vTimes = Vec_IntStartFull( Gia_ManObjNum(p) );
    Gia_Obj_t * pObj; 
    int j, Entry, k, iFan, Req;
    Vec_IntWriteEntry( vTimes, 0, 0 );
    if ( pManTime ) 
    {
        int nCoLimit = Gia_ManCoNum(p) - Tim_ManPoNum(pManTime);
        Tim_ManIncrementTravId( pManTime );
        //Tim_ManInitPoRequiredAll( pManTime, (float)ReqTime );
        Gia_ManForEachCo( p, pObj, j )
            if ( j >= nCoLimit )
            {
                Tim_ManSetCoRequired( pManTime, j, ReqTime );
                Gia_ManDfsUpdateRequired( vTimes, Gia_ObjFaninId0p(p, pObj), ReqTime );
            }
    }
    else
    {
        Gia_ManForEachCo( p, pObj, j )
            Gia_ManDfsUpdateRequired( vTimes, Gia_ObjFaninId0p(p, pObj), ReqTime );
    }
    Vec_IntForEachEntryReverse( vObjs, Entry, j )
    {
        if ( Entry < 0 ) // box
        {
            int iFirst, nTerms, iBox = -Entry-1;
            assert( iBox >= 0 );
            // set requireds for box outputs
            iFirst = Tim_ManBoxOutputFirst( pManTime, iBox );
            nTerms = Tim_ManBoxOutputNum( pManTime, iBox );
            for ( k = 0; k < nTerms; k++ )
            {
                pObj = Gia_ManCi( p, iFirst + k );
                Req  = Vec_IntEntry( vTimes, Gia_ObjId(p, pObj) );
                Req  = Req == -1 ? ReqTime : Req; // dangling box output
                assert( Req >= 0 );
                Tim_ManSetCiRequired( pManTime, Gia_ObjCioId(pObj), Req );
            }
            // derive requireds for box inputs
            iFirst = Tim_ManBoxInputFirst( pManTime, iBox );
            nTerms = Tim_ManBoxInputNum( pManTime, iBox );
            for ( k = 0; k < nTerms; k++ )
            {
                pObj = Gia_ManCo( p, iFirst + k );
                Req  = Tim_ManGetCoRequired( pManTime, Gia_ObjCioId(pObj) );
                assert( Req >= 0 );
                Gia_ManDfsUpdateRequired( vTimes, Gia_ObjFaninId0p(p, pObj), Req );
            }
        }
        else if ( Entry > 0 ) // node
        {
            Req = Vec_IntEntry(vTimes, Entry) - 10;
            assert( Req >= 0 );
            if ( Gia_ManHasMapping(p) )
            {
                assert( Gia_ObjIsLut(p, Entry) );
                Gia_LutForEachFanin( p, Entry, iFan, k )
                    Gia_ManDfsUpdateRequired( vTimes, iFan, Req );
            }
            else
            {
                pObj  = Gia_ManObj( p, Entry );
                Gia_ManDfsUpdateRequired( vTimes, Gia_ObjFaninId0(pObj, Entry), Req );
                Gia_ManDfsUpdateRequired( vTimes, Gia_ObjFaninId1(pObj, Entry), Req );
            }
        }
        else assert( 0 );
    }
    return vTimes;
}
Vec_Int_t * Gia_ManDfsSlacks( Gia_Man_t * p )
{
    Vec_Int_t * vSlack = Vec_IntStartFull( Gia_ManObjNum(p) );
    Vec_Int_t * vObjs  = Gia_ManDfsCollect( p );
    if ( Vec_IntSize(vObjs) > 0 )
    {
        Vec_Int_t * vArrs  = Gia_ManDfsArrivals( p, vObjs );
        int Required       = Vec_IntFindMax( vArrs );
        Vec_Int_t * vReqs  = Gia_ManDfsRequireds( p, vObjs, Required );
        int i, Arr, Req, Arrivals = ABC_INFINITY;
        Vec_IntForEachEntry( vReqs, Req, i )
            if ( Req != -1 )
                Arrivals = Abc_MinInt( Arrivals, Req );
        //if ( Arrivals != 0 )
        //    printf( "\nGlobal timing check has failed.\n\n" );
        //assert( Arrivals == 0 );
        Vec_IntForEachEntryTwo( vArrs, vReqs, Arr, Req, i )
        {
            if ( !Gia_ObjIsAnd(Gia_ManObj(p, i)) )
                continue;
            if ( Gia_ManHasMapping(p) && !Gia_ObjIsLut(p, i) )
                continue;
            assert( Arr <= Req );
            Vec_IntWriteEntry( vSlack, i, Req - Arr );
        }
        Vec_IntFree( vArrs );
        Vec_IntFree( vReqs );
    }
    Vec_IntFree( vObjs );
    return vSlack;
}
void Gia_ManDfsSlacksPrint( Gia_Man_t * p )
{
    Vec_Int_t * vCounts, * vSlacks = Gia_ManDfsSlacks( p );
    int i, Entry, nRange, nTotal;
    if ( Vec_IntSize(vSlacks) == 0 )
    {
        printf( "Network contains no internal objects.\n" );
        Vec_IntFree( vSlacks );
        return;
    }
    // compute slacks
    Vec_IntForEachEntry( vSlacks, Entry, i )
        if ( Entry != -1 )
            Vec_IntWriteEntry( vSlacks, i, Entry/10 );
    nRange = Vec_IntFindMax( vSlacks );
    // count items
    vCounts = Vec_IntStart( nRange + 1 );
    Vec_IntForEachEntry( vSlacks, Entry, i )
        if ( Entry != -1 )
            Vec_IntAddToEntry( vCounts, Entry, 1 );
    // print slack ranges
    nTotal = Vec_IntSum( vCounts );
    assert( nTotal > 0 );
    Vec_IntForEachEntry( vCounts, Entry, i )
    {
        printf( "Slack range %3d = ", i );
        printf( "[%4d, %4d)   ", 10*i, 10*(i+1) );
        printf( "Nodes = %5d  ", Entry );
        printf( "(%6.2f %%) ", 100.0*Entry/nTotal );
        printf( "\n" );
    }
    Vec_IntFree( vSlacks );
    Vec_IntFree( vCounts );
}


/**Function*************************************************************

  Synopsis    [Dump interface module]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManWriteNamesInter( FILE * pFile, char c, int n, int Start, int Skip, int nRegs )
{
    int Length = Start, i, fFirst = 1; 
    char pName[100];
    for ( i = 0; i < n-nRegs; i++ )
    {
        sprintf( pName, "%c[%d]", c, i );
        Length += strlen(pName) + 2;
        if ( Length > 60 )
        {
            fprintf( pFile, ",\n    " );
            Length = Skip;
            fFirst = 1;
        }
        fprintf( pFile, "%s%s", fFirst ? "":", ", pName );
        fFirst = 0;
    }
    for ( i = n-nRegs; i < n; i++ )
    {
        sprintf( pName, "%c%c[%d]", c, c, i );
        Length += strlen(pName) + 2;
        if ( Length > 60 )
        {
            fprintf( pFile, ",\n    " );
            Length = Skip;
            fFirst = 1;
        }
        fprintf( pFile, "%s%s", fFirst ? "":", ", pName );
        fFirst = 0;
    }}
void Gia_ManDumpModuleName( FILE * pFile, char * pName )
{
    int i;
    for ( i = 0; i < (int)strlen(pName); i++ )
    if ( isalpha(pName[i]) || isdigit(pName[i]) )
        fprintf( pFile, "%c", pName[i] );
    else
        fprintf( pFile, "_" );
}
void Gia_ManDumpInterface2( Gia_Man_t * p, FILE * pFile )
{
    int fPrintClk = 0;
    fprintf( pFile, "module " );
    Gia_ManDumpModuleName( pFile, p->pName );
    fprintf( pFile, "_wrapper" );
    fprintf( pFile, " (%s i, o );\n\n", fPrintClk && Gia_ManRegNum(p) ? " clk," : "" );
    if ( fPrintClk && Gia_ManRegNum(p) )
    fprintf( pFile, "  input clk;\n" );
    fprintf( pFile, "  input  [%d:0] i;\n",   Gia_ManPiNum(p)-1 );
    fprintf( pFile, "  output [%d:0] o;\n\n", Gia_ManPoNum(p)-1 );

    if ( Gia_ManRegNum(p) ) {
        fprintf( pFile, "  wire [%d:%d] ii;\n",   Gia_ManCiNum(p)-1, Gia_ManPiNum(p) );
        fprintf( pFile, "  wire [%d:%d] oo;\n\n", Gia_ManCoNum(p)-1, Gia_ManPoNum(p) );    
        fprintf( pFile, "  always @ (posedge %s)\n    ii <= oo;\n\n", fPrintClk ? "clk" : "i[0]" );
    }

    fprintf( pFile, "  " );
    Gia_ManDumpModuleName( pFile, p->pName );
    fprintf( pFile, " " );
    Gia_ManDumpModuleName( pFile, p->pName );
    fprintf( pFile, "_inst" );

    fprintf( pFile, " (\n    " );
    Gia_ManWriteNamesInter( pFile, 'i', Gia_ManCiNum(p), 4, 4, Gia_ManRegNum(p) );
    fprintf( pFile, ",\n    " );
    Gia_ManWriteNamesInter( pFile, 'o', Gia_ManCoNum(p), 4, 4, Gia_ManRegNum(p) );
    fprintf( pFile, "\n  );\n\n" );

    fprintf( pFile, "endmodule\n\n" );
}

/**Function*************************************************************

  Synopsis    [Compute arrival/required times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Bit_t * Gia_ManGenUsed( Gia_Man_t * p, int fBuf )
{
    Gia_Obj_t * pObj; int i;
    Vec_Bit_t * vUsed = Vec_BitStart( Gia_ManObjNum(p) );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( fBuf )
            Vec_BitWriteEntry( vUsed, i, 1 );
        if ( Gia_ObjFaninC0(pObj) ^ fBuf )
            Vec_BitWriteEntry( vUsed, Gia_ObjFaninId0(pObj, i), 1 );
        if ( Gia_ObjFaninC1(pObj) ^ fBuf )
            Vec_BitWriteEntry( vUsed, Gia_ObjFaninId1(pObj, i), 1 );
    }
    Gia_ManForEachCo( p, pObj, i )
        if ( Gia_ObjFaninC0(pObj) ^ fBuf )
            Vec_BitWriteEntry( vUsed, Gia_ObjFaninId0p(p, pObj), 1 );
    Vec_BitWriteEntry( vUsed, 0, 0 ); // clean zero
    return vUsed;
}
int Gia_ManNameIsLegalInVerilog( char * pName )
{
    // identifier ::= simple_identifier | escaped_identifier
    // simple_identifier ::= [a-zA-Z_][a-zA-Z0-9_$]
    // escaped_identifier ::= \ {Any_ASCII_character_except_white_space} white_space
    // white_space ::= space | tab | newline
    assert( pName != NULL && *pName != '\0' );
    if ( *pName == '\\' )
        return 1;
    if ( (*pName < 'a' || *pName > 'z') && (*pName < 'A' || *pName > 'Z') && *pName != '_' )
        return 0;
    while ( *(++pName) )
        if ( (*pName < 'a' || *pName > 'z') && (*pName < 'A' || *pName > 'Z') && (*pName < '0' || *pName > '9') && *pName != '_' && *pName != '$' ) 
            return 0;
    return 1;
}
char * Gia_ObjGetDumpName( Vec_Ptr_t * vNames, char c, int i, int d )
{
    static char pBuffer[10000];
    if ( vNames )
    {
        char * pName = (char *)Vec_PtrEntry(vNames, i);
        if ( Gia_ManNameIsLegalInVerilog(pName) )
            sprintf( pBuffer, "%s", pName );
        else
            sprintf( pBuffer, "\\%s ", pName );
    }
    else
        sprintf( pBuffer, "%c%0*d%c", c, d, i, c );
    return pBuffer;
}
void Gia_ManWriteNames( FILE * pFile, char c, int n, Vec_Ptr_t * vNames, int Start, int Skip, Vec_Bit_t * vObjs )
{
    int Digits = Abc_Base10Log( n );
    int Length = Start, i, fFirst = 1; 
    char * pName;
    for ( i = 0; i < n; i++ )
    {
        if ( vObjs && !Vec_BitEntry(vObjs, i) )
            continue;
        pName = Gia_ObjGetDumpName( vNames, c, i, Digits );
        Length += strlen(pName) + 2;
        if ( Length > 60 )
        {
            fprintf( pFile, ",\n    " );
            Length = Skip;
            fFirst = 1;
        }
        fprintf( pFile, "%s%s", fFirst ? "":", ", pName );
        fFirst = 0;
    }
}
void Gia_ManDumpVerilog( Gia_Man_t * p, char * pFileName, Vec_Int_t * vObjs, int fVerBufs, int fInter, int fInterComb, int fAssign )
{
    if ( fInterComb ) 
    {
        if ( fAssign ) {
            extern void Gia_ManDumpInterfaceAssign( Gia_Man_t * p, char * pFileName );
            Gia_ManDumpInterfaceAssign( p, pFileName );
        }
        else {
            extern void Gia_ManDumpInterface( Gia_Man_t * p, char * pFileName );
            Gia_ManDumpInterface( p, pFileName );
        }
    }
    else
    {
        if ( fAssign ) {
            extern void Gia_ManDumpVerilogNoInterAssign( Gia_Man_t * p, char * pFileName, Vec_Int_t * vObjs, int fVerBufs, int fInter );
            Gia_ManDumpVerilogNoInterAssign( p, pFileName, vObjs, fVerBufs, fInter );
        }
        else {
            extern void Gia_ManDumpVerilogNoInter( Gia_Man_t * p, char * pFileName, Vec_Int_t * vObjs, int fVerBufs, int fInter );
            Gia_ManDumpVerilogNoInter( p, pFileName, vObjs, fVerBufs, fInter );
        }
    }
}
void Gia_ManDumpVerilogNoInter( Gia_Man_t * p, char * pFileName, Vec_Int_t * vObjs, int fVerBufs, int fInter )
{
    Gia_Obj_t * pObj;
    Vec_Bit_t * vInvs, * vUsed;
    int nDigits  = Abc_Base10Log( Gia_ManObjNum(p) );
    int nDigitsI = Abc_Base10Log( Gia_ManPiNum(p) );
    int nDigitsO = Abc_Base10Log( Gia_ManPoNum(p) );
    int i, k, iObj, nRegs = Gia_ManRegNum(p);
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open output file \"%s\".\n", pFileName );
        return;
    }

    if ( fInter || nRegs ) 
        Gia_ManDumpInterface2( p, pFile );
    //Gia_ManSetRegNum( p, 0 );
    p->nRegs = 0;

    vInvs = Gia_ManGenUsed( p, 0 );
    vUsed = Gia_ManGenUsed( p, 1 );

    //fprintf( pFile, "// This Verilog file is written by ABC on %s\n\n", Extra_TimeStamp() );

    fprintf( pFile, "module " );
    Gia_ManDumpModuleName( pFile, p->pName );

    if ( fVerBufs )
    {
        fprintf( pFile, " (\n    " );
        Gia_ManWriteNames( pFile, 'a', Gia_ManPiNum(p), NULL, 4, 4, NULL );
        fprintf( pFile, ",\n    " );

        Gia_ManWriteNames( pFile, 'y', Gia_ManPoNum(p), NULL, 4, 4, NULL );
        fprintf( pFile, "\n  );\n\n" );

        fprintf( pFile, "  input " );
        Gia_ManWriteNames( pFile, 'a', Gia_ManPiNum(p), NULL, 8, 4, NULL );
        fprintf( pFile, ";\n\n" );

        fprintf( pFile, "  output " );
        Gia_ManWriteNames( pFile, 'y', Gia_ManPoNum(p), NULL, 9, 4, NULL );
        fprintf( pFile, ";\n\n" );

        fprintf( pFile, "  wire " );
        Gia_ManWriteNames( pFile, 'x', Gia_ManPiNum(p), p->vNamesIn, 8, 4, NULL );
        fprintf( pFile, ";\n\n" );

        fprintf( pFile, "  wire " );
        Gia_ManWriteNames( pFile, 'z', Gia_ManPoNum(p), p->vNamesOut, 9, 4, NULL );
        fprintf( pFile, ";\n\n" );

        Gia_ManForEachPi( p, pObj, i )
        {
            fprintf( pFile, "  buf ( %s,", Gia_ObjGetDumpName(p->vNamesIn, 'x', i, nDigitsI) );
            fprintf( pFile, " %s );\n", Gia_ObjGetDumpName(NULL, 'a', i, nDigitsI) );
        }
        fprintf( pFile, "\n" );

        Gia_ManForEachPo( p, pObj, i )
        {
            fprintf( pFile, "  buf ( %s,", Gia_ObjGetDumpName(NULL, 'y', i, nDigitsO) );
            fprintf( pFile, " %s );\n", Gia_ObjGetDumpName(p->vNamesOut, 'z', i, nDigitsO) );
        }
        fprintf( pFile, "\n" );
    }
    else
    {
        fprintf( pFile, " (\n    " );
        Gia_ManWriteNames( pFile, 'x', Gia_ManPiNum(p), p->vNamesIn, 4, 4, NULL );
        fprintf( pFile, ",\n    " );

        Gia_ManWriteNames( pFile, 'z', Gia_ManPoNum(p), p->vNamesOut, 4, 4, NULL );
        fprintf( pFile, "\n  );\n\n" );

        fprintf( pFile, "  input " );
        Gia_ManWriteNames( pFile, 'x', Gia_ManPiNum(p), p->vNamesIn, 8, 4, NULL );
        fprintf( pFile, ";\n\n" );

        fprintf( pFile, "  output " );
        Gia_ManWriteNames( pFile, 'z', Gia_ManPoNum(p), p->vNamesOut, 9, 4, NULL );
        fprintf( pFile, ";\n\n" );
    }

    if ( Vec_BitCount(vUsed) )
    {
        fprintf( pFile, "  wire " );
        Gia_ManWriteNames( pFile, 'n', Gia_ManObjNum(p), NULL, 7, 4, vUsed );
        fprintf( pFile, ";\n\n" );
    }

    if ( Vec_BitCount(vInvs) )
    {
        fprintf( pFile, "  wire " );
        Gia_ManWriteNames( pFile, 'i', Gia_ManObjNum(p), NULL, 7, 4, vInvs );
        fprintf( pFile, ";\n\n" );
    }

    if ( vObjs )
    {
        fprintf( pFile, "  wire " );
        Vec_IntForEachEntry( vObjs, iObj, i )
            fprintf( pFile, " t_%d%s", i, i==Vec_IntSize(vObjs)-1 ? "" : "," ); 
        fprintf( pFile, ";\n\n" );
        Vec_IntForEachEntry( vObjs, iObj, i )
        {
            fprintf( pFile, "  buf ( %s,", Gia_ObjGetDumpName(NULL, 'n', iObj, nDigits) );
            fprintf( pFile, " t_%d );\n", i );
        }
        fprintf( pFile, "\n" );
    }

    // input inverters
    Gia_ManForEachPi( p, pObj, i )
    {
        if ( Vec_BitEntry(vUsed, Gia_ObjId(p, pObj)) )
        {
            fprintf( pFile, "  buf ( %s,", Gia_ObjGetDumpName(NULL, 'n', Gia_ObjId(p, pObj), nDigits) );
            fprintf( pFile, " %s );\n",   Gia_ObjGetDumpName(p->vNamesIn, 'x', i, nDigitsI) );
        }
        if ( Vec_BitEntry(vInvs, Gia_ObjId(p, pObj)) )
        {
            fprintf( pFile, "  not ( %s,", Gia_ObjGetDumpName(NULL, 'i', Gia_ObjId(p, pObj), nDigits) );
            fprintf( pFile, " %s );\n",   Gia_ObjGetDumpName(p->vNamesIn, 'x', i, nDigitsI) );
        }
    }

    // internal nodes and their inverters
    fprintf( pFile, "\n" );
    Gia_ManForEachAnd( p, pObj, i )
    {
        int fSkip = 0;
        if ( vObjs )
        {
            Vec_IntForEachEntry( vObjs, iObj, k )
                if ( iObj == i )
                    break;
            if ( k < Vec_IntSize(vObjs) )
                fSkip = 1;
        }
        if ( !fSkip )
        {
            fprintf( pFile, "  and ( %s,", Gia_ObjGetDumpName(NULL, 'n', i, nDigits) );
            fprintf( pFile, " %s,",       Gia_ObjGetDumpName(NULL, (char)(Gia_ObjFaninC0(pObj)? 'i':'n'), Gia_ObjFaninId0(pObj, i), nDigits) );
            fprintf( pFile, " %s );\n",   Gia_ObjGetDumpName(NULL, (char)(Gia_ObjFaninC1(pObj)? 'i':'n'), Gia_ObjFaninId1(pObj, i), nDigits) );
        }
        if ( Vec_BitEntry(vInvs, i) )
        {
            fprintf( pFile, "  not ( %s,", Gia_ObjGetDumpName(NULL, 'i', i, nDigits) );
            fprintf( pFile, " %s );\n",   Gia_ObjGetDumpName(NULL, 'n', i, nDigits) );
        }
    }
    
    // output drivers
    fprintf( pFile, "\n" );
    Gia_ManForEachPo( p, pObj, i )
    {
/*
        fprintf( pFile, "  assign %s = ", Gia_ObjGetDumpName(p->vNamesOut, 'z', i, nDigits2) );
        if ( Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) )
            fprintf( pFile, "1\'b%d;\n", Gia_ObjFaninC0(pObj) );
        else 
            fprintf( pFile, "%s;\n", Gia_ObjGetDumpName(NULL, (char)(Gia_ObjFaninC0(pObj)? 'i':'n'), Gia_ObjFaninId0p(p, pObj), nDigits) );
*/
        fprintf( pFile, "  buf ( %s, ", Gia_ObjGetDumpName(p->vNamesOut, 'z', i, nDigitsO) );
        if ( Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) )
            fprintf( pFile, "1\'b%d );\n", Gia_ObjFaninC0(pObj) );
        else 
            fprintf( pFile, "%s );\n", Gia_ObjGetDumpName(NULL, (char)(Gia_ObjFaninC0(pObj)? 'i':'n'), Gia_ObjFaninId0p(p, pObj), nDigits) );
    }

    fprintf( pFile, "\nendmodule\n\n" );
    fclose( pFile );

    Vec_BitFree( vInvs );
    Vec_BitFree( vUsed );

    Gia_ManSetRegNum( p, nRegs );
}
void Gia_ManDumpVerilogNoInterAssign( Gia_Man_t * p, char * pFileName, Vec_Int_t * vObjs, int fVerBufs, int fInter )
{
    Gia_Obj_t * pObj;
    Vec_Bit_t * vInvs, * vUsed;
    int nDigits  = Abc_Base10Log( Gia_ManObjNum(p) );
    int nDigitsI = Abc_Base10Log( Gia_ManPiNum(p) );
    int nDigitsO = Abc_Base10Log( Gia_ManPoNum(p) );
    int i, k, iObj, nRegs = Gia_ManRegNum(p);
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open output file \"%s\".\n", pFileName );
        return;
    }

    if ( fInter || nRegs ) 
        Gia_ManDumpInterface2( p, pFile );
    //Gia_ManSetRegNum( p, 0 );
    p->nRegs = 0;

    vInvs = Gia_ManGenUsed( p, 0 );
    vUsed = Gia_ManGenUsed( p, 1 );

    //fprintf( pFile, "// This Verilog file is written by ABC on %s\n\n", Extra_TimeStamp() );

    fprintf( pFile, "module " );
    Gia_ManDumpModuleName( pFile, p->pName );

    if ( fVerBufs )
    {
        fprintf( pFile, " (\n    " );
        Gia_ManWriteNames( pFile, 'a', Gia_ManPiNum(p), NULL, 4, 4, NULL );
        fprintf( pFile, ",\n    " );

        Gia_ManWriteNames( pFile, 'y', Gia_ManPoNum(p), NULL, 4, 4, NULL );
        fprintf( pFile, "\n  );\n\n" );

        fprintf( pFile, "  input " );
        Gia_ManWriteNames( pFile, 'a', Gia_ManPiNum(p), NULL, 8, 4, NULL );
        fprintf( pFile, ";\n\n" );

        fprintf( pFile, "  output " );
        Gia_ManWriteNames( pFile, 'y', Gia_ManPoNum(p), NULL, 9, 4, NULL );
        fprintf( pFile, ";\n\n" );

        fprintf( pFile, "  wire " );
        Gia_ManWriteNames( pFile, 'x', Gia_ManPiNum(p), p->vNamesIn, 8, 4, NULL );
        fprintf( pFile, ";\n\n" );

        fprintf( pFile, "  wire " );
        Gia_ManWriteNames( pFile, 'z', Gia_ManPoNum(p), p->vNamesOut, 9, 4, NULL );
        fprintf( pFile, ";\n\n" );

        Gia_ManForEachPi( p, pObj, i )
        {
            fprintf( pFile, "  assign %s =", Gia_ObjGetDumpName(p->vNamesIn, 'x', i, nDigitsI) );
            fprintf( pFile, " %s;\n", Gia_ObjGetDumpName(NULL, 'a', i, nDigitsI) );
        }
        fprintf( pFile, "\n" );

        Gia_ManForEachPo( p, pObj, i )
        {
            fprintf( pFile, "  assign %s =", Gia_ObjGetDumpName(NULL, 'y', i, nDigitsO) );
            fprintf( pFile, " %s;\n", Gia_ObjGetDumpName(p->vNamesOut, 'z', i, nDigitsO) );
        }
        fprintf( pFile, "\n" );
    }
    else
    {
        fprintf( pFile, " (\n    " );
        Gia_ManWriteNames( pFile, 'x', Gia_ManPiNum(p), p->vNamesIn, 4, 4, NULL );
        fprintf( pFile, ",\n    " );

        Gia_ManWriteNames( pFile, 'z', Gia_ManPoNum(p), p->vNamesOut, 4, 4, NULL );
        fprintf( pFile, "\n  );\n\n" );

        fprintf( pFile, "  input " );
        Gia_ManWriteNames( pFile, 'x', Gia_ManPiNum(p), p->vNamesIn, 8, 4, NULL );
        fprintf( pFile, ";\n\n" );

        fprintf( pFile, "  output " );
        Gia_ManWriteNames( pFile, 'z', Gia_ManPoNum(p), p->vNamesOut, 9, 4, NULL );
        fprintf( pFile, ";\n\n" );
    }

    if ( Vec_BitCount(vUsed) )
    {
        fprintf( pFile, "  wire " );
        Gia_ManWriteNames( pFile, 'n', Gia_ManObjNum(p), NULL, 7, 4, vUsed );
        fprintf( pFile, ";\n\n" );
    }

    if ( Vec_BitCount(vInvs) )
    {
        fprintf( pFile, "  wire " );
        Gia_ManWriteNames( pFile, 'i', Gia_ManObjNum(p), NULL, 7, 4, vInvs );
        fprintf( pFile, ";\n\n" );
    }

    if ( vObjs )
    {
        fprintf( pFile, "  wire " );
        Vec_IntForEachEntry( vObjs, iObj, i )
            fprintf( pFile, " t_%d%s", i, i==Vec_IntSize(vObjs)-1 ? "" : "," ); 
        fprintf( pFile, ";\n\n" );
        Vec_IntForEachEntry( vObjs, iObj, i )
        {
            fprintf( pFile, "  assign %s =", Gia_ObjGetDumpName(NULL, 'n', iObj, nDigits) );
            fprintf( pFile, " t_%d;\n", i );
        }
        fprintf( pFile, "\n" );
    }

    // input inverters
    Gia_ManForEachPi( p, pObj, i )
    {
        if ( Vec_BitEntry(vUsed, Gia_ObjId(p, pObj)) )
        {
            fprintf( pFile, "  assign %s =", Gia_ObjGetDumpName(NULL, 'n', Gia_ObjId(p, pObj), nDigits) );
            fprintf( pFile, " %s;\n",   Gia_ObjGetDumpName(p->vNamesIn, 'x', i, nDigitsI) );
        }
        if ( Vec_BitEntry(vInvs, Gia_ObjId(p, pObj)) )
        {
            fprintf( pFile, "  assign %s =", Gia_ObjGetDumpName(NULL, 'i', Gia_ObjId(p, pObj), nDigits) );
            fprintf( pFile, " ~%s;\n",   Gia_ObjGetDumpName(p->vNamesIn, 'x', i, nDigitsI) );
        }
    }

    // internal nodes and their inverters
    fprintf( pFile, "\n" );
    Gia_ManForEachAnd( p, pObj, i )
    {
        int fSkip = 0;
        if ( vObjs )
        {
            Vec_IntForEachEntry( vObjs, iObj, k )
                if ( iObj == i )
                    break;
            if ( k < Vec_IntSize(vObjs) )
                fSkip = 1;
        }
        if ( !fSkip )
        {
            fprintf( pFile, "  assign %s =", Gia_ObjGetDumpName(NULL, 'n', i, nDigits) );
            fprintf( pFile, " %s &",       Gia_ObjGetDumpName(NULL, (char)(Gia_ObjFaninC0(pObj)? 'i':'n'), Gia_ObjFaninId0(pObj, i), nDigits) );
            fprintf( pFile, " %s;\n",   Gia_ObjGetDumpName(NULL, (char)(Gia_ObjFaninC1(pObj)? 'i':'n'), Gia_ObjFaninId1(pObj, i), nDigits) );
        }
        if ( Vec_BitEntry(vInvs, i) )
        {
            fprintf( pFile, "  assign %s =", Gia_ObjGetDumpName(NULL, 'i', i, nDigits) );
            fprintf( pFile, " ~%s;\n",   Gia_ObjGetDumpName(NULL, 'n', i, nDigits) );
        }
    }
    
    // output drivers
    fprintf( pFile, "\n" );
    Gia_ManForEachPo( p, pObj, i )
    {
        fprintf( pFile, "  assign %s = ", Gia_ObjGetDumpName(p->vNamesOut, 'z', i, nDigitsO) );
        if ( Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) )
            fprintf( pFile, "1\'b%d;\n", Gia_ObjFaninC0(pObj) );
        else 
            fprintf( pFile, "%s;\n", Gia_ObjGetDumpName(NULL, (char)(Gia_ObjFaninC0(pObj)? 'i':'n'), Gia_ObjFaninId0p(p, pObj), nDigits) );
    }

    fprintf( pFile, "\nendmodule\n\n" );
    fclose( pFile );

    Vec_BitFree( vInvs );
    Vec_BitFree( vUsed );

    Gia_ManSetRegNum( p, nRegs );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDumpInterface( Gia_Man_t * p, char * pFileName )
{
    Gia_Obj_t * pObj;
    Vec_Bit_t * vInvs, * vUsed;
    int nDigits  = Abc_Base10Log( Gia_ManObjNum(p) );
    int nDigitsI = Abc_Base10Log( Gia_ManPiNum(p) );
    int nDigitsO = Abc_Base10Log( Gia_ManPoNum(p) );
    int i;

    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open output file \"%s\".\n", pFileName );
        return;
    }

    vInvs = Gia_ManGenUsed( p, 0 );
    vUsed = Gia_ManGenUsed( p, 1 );

    fprintf( pFile, "module " );
    Gia_ManDumpModuleName( pFile, p->pName );
    fprintf( pFile, "_wrapper" );
    fprintf( pFile, " ( _i_, _o_ );\n\n" );
    fprintf( pFile, "  input  [%d:0] _i_;\n",   Gia_ManCiNum(p)-1 );
    fprintf( pFile, "  output [%d:0] _o_;\n\n", Gia_ManCoNum(p)-1 );

    fprintf( pFile, "  wire " );
    Gia_ManWriteNames( pFile, 'x', Gia_ManPiNum(p), p->vNamesIn, 8, 4, NULL );
    fprintf( pFile, ";\n\n" );

    fprintf( pFile, "  wire " );
    Gia_ManWriteNames( pFile, 'z', Gia_ManPoNum(p), p->vNamesOut, 9, 4, NULL );
    fprintf( pFile, ";\n\n" );

    fprintf( pFile, "  assign { " );
    Gia_ManWriteNames( pFile, 'x', Gia_ManCiNum(p), p->vNamesIn, 8, 4, NULL );
    fprintf( pFile, " } = _i_;\n\n" );

    fprintf( pFile, "  assign _o_ = { " );
    Gia_ManWriteNames( pFile, 'z', Gia_ManCoNum(p), p->vNamesOut, 9, 4, NULL );
    fprintf( pFile, " };\n\n" );

    if ( Vec_BitCount(vUsed) )
    {
        fprintf( pFile, "  wire " );
        Gia_ManWriteNames( pFile, 'n', Gia_ManObjNum(p), NULL, 7, 4, vUsed );
        fprintf( pFile, ";\n\n" );
    }

    if ( Vec_BitCount(vInvs) )
    {
        fprintf( pFile, "  wire " );
        Gia_ManWriteNames( pFile, 'i', Gia_ManObjNum(p), NULL, 7, 4, vInvs );
        fprintf( pFile, ";\n\n" );
    }

    // input inverters
    Gia_ManForEachCi( p, pObj, i )
    {
        if ( Vec_BitEntry(vUsed, Gia_ObjId(p, pObj)) )
        {
            fprintf( pFile, "  buf ( %s,", Gia_ObjGetDumpName(NULL, 'n', Gia_ObjId(p, pObj), nDigits) );
            fprintf( pFile, " %s );\n",   Gia_ObjGetDumpName(p->vNamesIn, 'x', i, nDigitsI) );
        }
        if ( Vec_BitEntry(vInvs, Gia_ObjId(p, pObj)) )
        {
            fprintf( pFile, "  not ( %s,", Gia_ObjGetDumpName(NULL, 'i', Gia_ObjId(p, pObj), nDigits) );
            fprintf( pFile, " %s );\n",   Gia_ObjGetDumpName(p->vNamesIn, 'x', i, nDigitsI) );
        }
    }

    // internal nodes and their inverters
    fprintf( pFile, "\n" );
    Gia_ManForEachAnd( p, pObj, i )
    {
        fprintf( pFile, "  and ( %s,", Gia_ObjGetDumpName(NULL, 'n', i, nDigits) );
        fprintf( pFile, " %s,",       Gia_ObjGetDumpName(NULL, (char)(Gia_ObjFaninC0(pObj)? 'i':'n'), Gia_ObjFaninId0(pObj, i), nDigits) );
        fprintf( pFile, " %s );\n",   Gia_ObjGetDumpName(NULL, (char)(Gia_ObjFaninC1(pObj)? 'i':'n'), Gia_ObjFaninId1(pObj, i), nDigits) );
        if ( Vec_BitEntry(vInvs, i) )
        {
            fprintf( pFile, "  not ( %s,", Gia_ObjGetDumpName(NULL, 'i', i, nDigits) );
            fprintf( pFile, " %s );\n",   Gia_ObjGetDumpName(NULL, 'n', i, nDigits) );
        }
    }
    
    // output drivers
    fprintf( pFile, "\n" );
    Gia_ManForEachCo( p, pObj, i )
    {
        fprintf( pFile, "  buf ( %s, ", Gia_ObjGetDumpName(p->vNamesOut, 'z', i, nDigitsO) );
        if ( Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) )
            fprintf( pFile, "1\'b%d );\n", Gia_ObjFaninC0(pObj) );
        else 
            fprintf( pFile, "%s );\n", Gia_ObjGetDumpName(NULL, (char)(Gia_ObjFaninC0(pObj)? 'i':'n'), Gia_ObjFaninId0p(p, pObj), nDigits) );
    }

    fprintf( pFile, "\nendmodule\n\n" );
    fclose( pFile );

    Vec_BitFree( vInvs );
    Vec_BitFree( vUsed );
}
void Gia_ManDumpInterfaceAssign( Gia_Man_t * p, char * pFileName )
{
    Gia_Obj_t * pObj;
    Vec_Bit_t * vInvs, * vUsed;
    int nDigits  = Abc_Base10Log( Gia_ManObjNum(p) );
    int nDigitsI = Abc_Base10Log( Gia_ManPiNum(p) );
    int nDigitsO = Abc_Base10Log( Gia_ManPoNum(p) );
    int i;

    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open output file \"%s\".\n", pFileName );
        return;
    }

    vInvs = Gia_ManGenUsed( p, 0 );
    vUsed = Gia_ManGenUsed( p, 1 );

    fprintf( pFile, "module " );
    Gia_ManDumpModuleName( pFile, p->pName );
    fprintf( pFile, "_wrapper" );
    fprintf( pFile, " ( _i_, _o_ );\n\n" );
    fprintf( pFile, "  input  [%d:0] _i_;\n",   Gia_ManCiNum(p)-1 );
    fprintf( pFile, "  output [%d:0] _o_;\n\n", Gia_ManCoNum(p)-1 );

    fprintf( pFile, "  wire " );
    Gia_ManWriteNames( pFile, 'x', Gia_ManPiNum(p), p->vNamesIn, 8, 4, NULL );
    fprintf( pFile, ";\n\n" );

    fprintf( pFile, "  wire " );
    Gia_ManWriteNames( pFile, 'z', Gia_ManPoNum(p), p->vNamesOut, 9, 4, NULL );
    fprintf( pFile, ";\n\n" );

    fprintf( pFile, "  assign { " );
    Gia_ManWriteNames( pFile, 'x', Gia_ManCiNum(p), p->vNamesIn, 8, 4, NULL );
    fprintf( pFile, " } = _i_;\n\n" );

    fprintf( pFile, "  assign _o_ = { " );
    Gia_ManWriteNames( pFile, 'z', Gia_ManCoNum(p), p->vNamesOut, 9, 4, NULL );
    fprintf( pFile, " };\n\n" );

    if ( Vec_BitCount(vUsed) )
    {
        fprintf( pFile, "  wire " );
        Gia_ManWriteNames( pFile, 'n', Gia_ManObjNum(p), NULL, 7, 4, vUsed );
        fprintf( pFile, ";\n\n" );
    }

    if ( Vec_BitCount(vInvs) )
    {
        fprintf( pFile, "  wire " );
        Gia_ManWriteNames( pFile, 'i', Gia_ManObjNum(p), NULL, 7, 4, vInvs );
        fprintf( pFile, ";\n\n" );
    }

    // input inverters
    Gia_ManForEachCi( p, pObj, i )
    {
        if ( Vec_BitEntry(vUsed, Gia_ObjId(p, pObj)) )
        {
            fprintf( pFile, "  assign %s =", Gia_ObjGetDumpName(NULL, 'n', Gia_ObjId(p, pObj), nDigits) );
            fprintf( pFile, " %s;\n",   Gia_ObjGetDumpName(p->vNamesIn, 'x', i, nDigitsI) );
        }
        if ( Vec_BitEntry(vInvs, Gia_ObjId(p, pObj)) )
        {
            fprintf( pFile, "  assign %s =", Gia_ObjGetDumpName(NULL, 'i', Gia_ObjId(p, pObj), nDigits) );
            fprintf( pFile, " ~%s;\n",   Gia_ObjGetDumpName(p->vNamesIn, 'x', i, nDigitsI) );
        }
    }

    // internal nodes and their inverters
    fprintf( pFile, "\n" );
    Gia_ManForEachAnd( p, pObj, i )
    {
        fprintf( pFile, "  assign %s =", Gia_ObjGetDumpName(NULL, 'n', i, nDigits) );
        fprintf( pFile, " %s &",       Gia_ObjGetDumpName(NULL, (char)(Gia_ObjFaninC0(pObj)? 'i':'n'), Gia_ObjFaninId0(pObj, i), nDigits) );
        fprintf( pFile, " %s;\n",   Gia_ObjGetDumpName(NULL, (char)(Gia_ObjFaninC1(pObj)? 'i':'n'), Gia_ObjFaninId1(pObj, i), nDigits) );
        if ( Vec_BitEntry(vInvs, i) )
        {
            fprintf( pFile, "  assign %s =", Gia_ObjGetDumpName(NULL, 'i', i, nDigits) );
            fprintf( pFile, " ~%s;\n",   Gia_ObjGetDumpName(NULL, 'n', i, nDigits) );
        }
    }
    
    // output drivers
    fprintf( pFile, "\n" );
    Gia_ManForEachCo( p, pObj, i )
    {
        fprintf( pFile, "  assign %s = ", Gia_ObjGetDumpName(p->vNamesOut, 'z', i, nDigitsO) );
        if ( Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) )
            fprintf( pFile, "1\'b%d;\n", Gia_ObjFaninC0(pObj) );
        else 
            fprintf( pFile, "%s;\n", Gia_ObjGetDumpName(NULL, (char)(Gia_ObjFaninC0(pObj)? 'i':'n'), Gia_ObjFaninId0p(p, pObj), nDigits) );
    }

    fprintf( pFile, "\nendmodule\n\n" );
    fclose( pFile );

    Vec_BitFree( vInvs );
    Vec_BitFree( vUsed );
}


/**Function*************************************************************

  Synopsis    [Generate hierarchical design.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_FreeMany( Gia_Man_t ** pGias, int nGias )
{
    int i;
    for ( i = 0; i < nGias; i++ )
        Gia_ManStopP( &pGias[i] );
}
void Gia_GenSandwich( char ** pFNames, int nFNames, char * pFileName )
{
    FILE * pFile = NULL;
    Gia_Man_t * pGias[16] = {0};
    int i, k;
    assert( nFNames <= 16 );
    for ( i = 0; i < nFNames; i++ ) 
    {
        FILE * pFile = fopen( pFNames[i], "rb" );
        if ( pFile == NULL ) {
            printf( "Cannot open input file \"%s\".\n", pFNames[i] );
            Gia_FreeMany( pGias, nFNames );
            return;
        }
        fclose( pFile );
        pGias[i] = Gia_AigerRead( pFNames[i], 0, 0, 0 );
        if ( pGias[i] == NULL ) {
            printf( "Failed to read an AIG from file \"%s\".\n", pFNames[i] );
            Gia_FreeMany( pGias, nFNames );
            return;
        }
    }
    for ( i = 0; i < nFNames-1; i++ ) 
        if ( Gia_ManPoNum(pGias[i]) < Gia_ManPiNum(pGias[i+1]) ) {
            printf( "AIG in file \"%s\" has fewer outputs than inputs of AIG in file \"%s\".\n", pFNames[i], pFNames[i+1] );
            Gia_FreeMany( pGias, nFNames );
            return;
        }
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open output file \"%s\".\n", pFileName );
        Gia_FreeMany( pGias, nFNames );        
        return;
    }
    fprintf( pFile, "\n" );
    for ( i = 0; i < nFNames; i++ ) 
        fprintf( pFile, "`include \"%s\"\n", Extra_FileNameGenericAppend(pGias[i]->pSpec, ".v") );
    fprintf( pFile, "\n" );
    fprintf( pFile, "module sandwich ( in, out );\n" );
    fprintf( pFile, "  input  [%3d:0] in;\n",  Gia_ManPiNum(pGias[0])-1 );
    fprintf( pFile, "  output [%3d:0] out;\n", Gia_ManPoNum(pGias[nFNames-1])-1 );
    fprintf( pFile, "  wire [%3d:0] tmp0 = in;\n", Gia_ManPiNum(pGias[0])-1 );
    for ( i = 0; i < nFNames; i++ ) {
        fprintf( pFile, "  wire [%3d:0] tmp%d;  ", Gia_ManPoNum(pGias[i])-1, i+1 );
        Gia_ManDumpModuleName( pFile, pGias[i]->pName );
        fprintf( pFile, "_wrapper" );
        for ( k = strlen(pGias[i]->pName); k < 24; k++ )
            fprintf( pFile, " " );
        fprintf( pFile, " i%d ( tmp%d, tmp%d );\n", i+1, i, i+1 );
    }    
    fprintf( pFile, "  assign out = tmp%d;\n", nFNames );
    fprintf( pFile, "endmodule\n" );
    fclose( pFile );
    for ( i = 0; i < nFNames; i++ ) {
        Gia_ManDumpVerilog( pGias[i], Extra_FileNameGenericAppend(pGias[i]->pSpec, ".v"), NULL, 0, 0, 1, 0 );
        printf( "Dumped Verilog file \"%s\"\n", Extra_FileNameGenericAppend(pGias[i]->pSpec, ".v") );        
    }
    Gia_FreeMany( pGias, nFNames );
    printf( "Dumped hierarchical design into file \"%s\"\n", pFileName );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
