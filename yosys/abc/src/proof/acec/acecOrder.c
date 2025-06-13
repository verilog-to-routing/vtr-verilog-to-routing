/**CFile****************************************************************

  FileName    [acecOrder.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Node ordering.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecOrder.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_PolynFindOrder( Gia_Man_t * pGia, Vec_Int_t * vFadds, Vec_Int_t * vHadds, int fVerbose, int fVeryVerbose )
{
    int fDumpLeftOver = 0;
    int i, iXor, iMaj, iAnd, Entry, Iter, fFound, fFoundAll = 1;
    Vec_Int_t * vRecord = Vec_IntAlloc( 100 ), * vLeft, * vRecord2;
    Vec_Int_t * vMap = Vec_IntStart( Gia_ManObjNum(pGia) );
    Gia_ManForEachCoDriverId( pGia, iAnd, i )
        Vec_IntWriteEntry( vMap, iAnd, 1 );

    // collect the last XOR
    Vec_IntFreeP( &pGia->vXors );
    pGia->vXors = Gia_PolynCollectLastXor( pGia, fVerbose );
    printf( "Collected %d topmost XORs\n", Vec_IntSize(pGia->vXors) );
    //Vec_IntPrint( p->vXors );
    Vec_IntForEachEntry( pGia->vXors, iAnd, i )
    {
        Gia_Obj_t * pAnd = Gia_ManObj( pGia, iAnd );
        assert( Vec_IntEntry(vMap, iAnd) );
        Vec_IntWriteEntry( vMap, iAnd, 0 );
        Vec_IntWriteEntry( vMap, Gia_ObjFaninId0(pAnd, iAnd), 1 );
        Vec_IntWriteEntry( vMap, Gia_ObjFaninId1(pAnd, iAnd), 1 );
        Vec_IntPush( vRecord, Abc_Var2Lit2(iAnd, 3) );
        if ( fVeryVerbose )
            printf( "Recognizing %d => XXXOR(%d %d)\n", iAnd, Gia_ObjFaninId0(pAnd, iAnd), Gia_ObjFaninId1(pAnd, iAnd) );
    }

    // detect FAs and HAs
    for ( Iter = 0; fFoundAll; Iter++ )
    {
        if ( fVeryVerbose )
            printf( "Iteration %d\n", Iter );

        // check if we can extract FADDs
        fFoundAll = 0;
        do {
            fFound = 0;
            for ( i = Vec_IntSize(vFadds)/5 - 1; i >= 0; i-- )
            {
                iXor = Vec_IntEntry(vFadds, 5*i+3);
                iMaj = Vec_IntEntry(vFadds, 5*i+4);
                if ( Vec_IntEntry(vMap, iXor) && Vec_IntEntry(vMap, iMaj) )
                {
                    Vec_IntWriteEntry( vMap, iXor, 0 );
                    Vec_IntWriteEntry( vMap, iMaj, 0 );
                    Vec_IntWriteEntry( vMap, Vec_IntEntry(vFadds, 5*i+0), 1 );
                    Vec_IntWriteEntry( vMap, Vec_IntEntry(vFadds, 5*i+1), 1 );
                    Vec_IntWriteEntry( vMap, Vec_IntEntry(vFadds, 5*i+2), 1 );
                    Vec_IntPush( vRecord, Abc_Var2Lit2(i, 2) );
                    fFound = 1;
                    fFoundAll = 1;
                    if ( fVeryVerbose )
                        printf( "Recognizing (%d %d) => FA(%d %d %d)\n", iXor, iMaj, Vec_IntEntry(vFadds, 5*i+0), Vec_IntEntry(vFadds, 5*i+1), Vec_IntEntry(vFadds, 5*i+2)  );
                }
            }
        } while ( fFound );
        // check if we can extract HADDs
        do {
            fFound = 0; 
            for ( i = Vec_IntSize(vHadds)/2 - 1; i >= 0; i-- )
            {
                iXor = Vec_IntEntry(vHadds, 2*i+0);
                iMaj = Vec_IntEntry(vHadds, 2*i+1);
                if ( Vec_IntEntry(vMap, iXor) && Vec_IntEntry(vMap, iMaj) )
                {
                    Gia_Obj_t * pAnd = Gia_ManObj( pGia, iMaj );
                    Vec_IntWriteEntry( vMap, iXor, 0 );
                    Vec_IntWriteEntry( vMap, iMaj, 0 );
                    Vec_IntWriteEntry( vMap, Gia_ObjFaninId0(pAnd, iMaj), 1 );
                    Vec_IntWriteEntry( vMap, Gia_ObjFaninId1(pAnd, iMaj), 1 );
                    Vec_IntPush( vRecord, Abc_Var2Lit2(i, 1) );
                    fFound = 1;
                    fFoundAll = 1;
                    if ( fVeryVerbose )
                        printf( "Recognizing (%d %d) => HA(%d %d)\n", iXor, iMaj, Gia_ObjFaninId0(pAnd, iMaj), Gia_ObjFaninId1(pAnd, iMaj) );
                }
            }
            break; // only one iter!
        } while ( fFound );
        if ( fFoundAll )
            continue;
/*
        // find the last one
        Vec_IntForEachEntryReverse( vMap, Entry, iAnd )
            if ( Entry && Gia_ObjIsAnd(Gia_ManObj(pGia, iAnd)) )//&& Vec_IntFind(vFadds, iAnd) == -1 )//&& Vec_IntFind(vHadds, iAnd) == -1 )
            {
                Gia_Obj_t * pFan0, * pFan1, * pAnd = Gia_ManObj( pGia, iAnd );
                if ( !Gia_ObjRecognizeExor(pAnd, &pFan0, &pFan1) )
                {
                    Vec_IntWriteEntry( vMap, iAnd, 0 );
                    Vec_IntWriteEntry( vMap, Gia_ObjFaninId0(pAnd, iAnd), 1 );
                    Vec_IntWriteEntry( vMap, Gia_ObjFaninId1(pAnd, iAnd), 1 );
                    Vec_IntPush( vRecord, Abc_Var2Lit2(iAnd, 0) );
                }
                else
                {
                    Vec_IntWriteEntry( vMap, iAnd, 0 );
                    Vec_IntWriteEntry( vMap, Gia_ObjId(pGia, pFan0), 1 );
                    Vec_IntWriteEntry( vMap, Gia_ObjId(pGia, pFan1), 1 );
                    Vec_IntPush( vRecord, Abc_Var2Lit2(iAnd, 0) );
                    Vec_IntPush( vRecord, Abc_Var2Lit2(Gia_ObjFaninId0(pAnd, iAnd), 0) );
                    Vec_IntPush( vRecord, Abc_Var2Lit2(Gia_ObjFaninId1(pAnd, iAnd), 0) );
                    printf( "Recognizing %d => XOR(%d %d)\n", iAnd, Gia_ObjId(pGia, pFan0), Gia_ObjId(pGia, pFan1) );
                }
                fFoundAll = 1;
                if ( fVeryVerbose )
                    printf( "Recognizing %d => AND(%d %d)\n", iAnd, Gia_ObjFaninId0(pAnd, iAnd), Gia_ObjFaninId1(pAnd, iAnd) );
                break;
            }
*/
    }
    //Vec_IntPrint( vMap );

    // collect remaining ones
    vLeft = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vMap, Entry, i )
        if ( Entry && Gia_ObjIsAnd(Gia_ManObj(pGia, i)) )
            Vec_IntPush( vLeft, i );
    Vec_IntFree( vMap );

    // collect them in the topo order
    vRecord2 = Vec_IntAlloc( 100 );
    Gia_ManIncrementTravId( pGia );
    Gia_ManCollectAnds( pGia, Vec_IntArray(vLeft), Vec_IntSize(vLeft), vRecord2, NULL );
    Vec_IntForEachEntry( vRecord2, iAnd, i )
        Vec_IntWriteEntry( vRecord2, i, Abc_Var2Lit2(iAnd, 0) );

    // dump remaining nodes as an AIG
    if ( fDumpLeftOver )
    {        
        Gia_Man_t * pNew;
        pNew = Gia_ManDupAndCones( pGia, Vec_IntArray(vLeft), Vec_IntSize(vLeft), 0 );
        Gia_AigerWrite( pNew, "leftover.aig", 0, 0, 0 );
        printf( "Leftover AIG with %d nodes is dumped into file \"%s\".\n", Gia_ManAndNum(pNew), "leftover.aig" );
        Gia_ManStop( pNew );
    }
    Vec_IntFree( vLeft );

    Vec_IntReverseOrder( vRecord );
    Vec_IntAppend( vRecord2, vRecord );
    Vec_IntFree( vRecord );
    return vRecord2;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_PolynReorder( Gia_Man_t * pGia, int fVerbose, int fVeryVerbose )
{
    Vec_Int_t * vFadds  = Gia_ManDetectFullAdders( pGia, fVeryVerbose, NULL );
    Vec_Int_t * vHadds  = Gia_ManDetectHalfAdders( pGia, fVeryVerbose );
    Vec_Int_t * vRecord = Gia_PolynFindOrder( pGia, vFadds, vHadds, fVerbose, fVeryVerbose );
    Vec_Int_t * vOrder  = Vec_IntAlloc( Gia_ManAndNum(pGia) );
    Vec_Int_t * vOrder2 = Vec_IntStart( Gia_ManObjNum(pGia) );
    int i, k, Entry;

    // collect nodes
    Gia_ManIncrementTravId( pGia );
    Vec_IntForEachEntry( vRecord, Entry, i )
    {
        int Node = Abc_Lit2Var2(Entry);
        int Attr = Abc_Lit2Att2(Entry);
        if ( Attr == 2 )
        {
            int * pFanins = Vec_IntEntryP( vFadds, 5*Node );
            for ( k = 3; k < 5; k++ )
                Gia_ManCollectAnds_rec( pGia, pFanins[k], vOrder );
        }
        else if ( Attr == 1 )
        {
            int * pFanins = Vec_IntEntryP( vHadds, 2*Node );
            for ( k = 0; k < 2; k++ )
                Gia_ManCollectAnds_rec( pGia, pFanins[k], vOrder );
        }
        else
            Gia_ManCollectAnds_rec( pGia, Node, vOrder );
        //printf( "Order = %d.  Node = %d.  Size = %d ", i, Node, Vec_IntSize(vOrder) );
    }
    //printf( "\n" );
    //assert( Vec_IntSize(vOrder) == Gia_ManAndNum(pGia) );

    // remap order
    Gia_ManForEachCiId( pGia, Entry, i )
        Vec_IntWriteEntry( vOrder2, Entry, 1+i );
    Vec_IntForEachEntry( vOrder, Entry, i )
        Vec_IntWriteEntry( vOrder2, Entry, 1+Gia_ManCiNum(pGia)+i );
    //Vec_IntPrint( vOrder );

    Vec_IntFree( vRecord );
    Vec_IntFree( vFadds );
    Vec_IntFree( vHadds );
    Vec_IntFree( vOrder );
    return vOrder2;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

