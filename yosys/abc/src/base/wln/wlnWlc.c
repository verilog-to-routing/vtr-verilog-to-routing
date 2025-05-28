/**CFile****************************************************************

  FileName    [wlnWlc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Word-level network.]

  Synopsis    [Network transformation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 23, 2018.]

  Revision    [$Id: wlnWlc.c,v 1.00 2018/09/23 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wln.h"
#include "base/wlc/wlc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern int Ndr_TypeWlc2Ndr( int Type );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Wln_ConstFromBits( int * pBits, int nBits )
{
    char * pBuffer = ABC_ALLOC( char, nBits+100 ); int i, Len;
    sprintf( pBuffer, "%d\'b", nBits );
    Len = strlen(pBuffer);
    for ( i = nBits-1; i >= 0; i-- )
        pBuffer[Len++] = '0' + Abc_InfoHasBit((unsigned *)pBits, i);
    pBuffer[Len] = 0;
    return pBuffer;
}
char * Wln_ConstFromStr( char * pBits, int nBits )
{
    char * pBuffer = ABC_ALLOC( char, nBits+100 ); int i, Len;
    sprintf( pBuffer, "%d\'b", nBits );
    Len = strlen(pBuffer);
    for ( i = 0; i < nBits; i++ )
        pBuffer[Len++] = pBits[i];
    pBuffer[Len] = 0;
    return pBuffer;
}
int Wln_TrasformNameId( Wln_Ntk_t * pNew, Wlc_Ntk_t * p, Wlc_Obj_t * pObj )
{
    return Abc_NamStrFindOrAdd( pNew->pManName, Wlc_ObjName(p, Wlc_ObjId(p, pObj)), NULL );
}
Wln_Ntk_t * Wln_NtkFromWlc( Wlc_Ntk_t * p )
{
    Wlc_Obj_t * pObj; 
    char Buffer[1000];
    int i, j, n, Type, iFanin, iOutId, iBit = 0;
    Vec_Int_t * vFanins = Vec_IntAlloc( 10 );
    Vec_Int_t * vInits = Vec_IntAlloc( Wlc_NtkFfNum(p) );
    Wln_Ntk_t * pNew = Wln_NtkAlloc( p->pName, Wlc_NtkObjNum(p)+Wlc_NtkCoNum(p)+Wlc_NtkFfNum(p) );
    pNew->pManName = Abc_NamStart( Abc_NamObjNumMax(p->pManName), 10 );
    if ( p->pSpec ) pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->fSmtLib = p->fSmtLib;
    Wlc_NtkCleanCopy( p );
    Wln_NtkCleanNameId( pNew );
    // add primary inputs
    Wlc_NtkForEachPi( p, pObj, i ) 
    {
        iOutId = Wln_ObjAlloc( pNew, ABC_OPER_CI, pObj->Signed, pObj->End, pObj->Beg );
        Wln_ObjSetNameId( pNew, iOutId, Wln_TrasformNameId(pNew, p, pObj) );
        Wlc_ObjSetCopy( p, Wlc_ObjId(p, pObj), iOutId );
    }
    // create initial state of the flops
    Wlc_NtkForEachCi( p, pObj, i ) 
    {
        assert( i == Wlc_ObjCiId(pObj) );
        if ( pObj->Type == WLC_OBJ_PI )
            continue;
        for ( j = 0; j < Wlc_ObjRange(pObj); j++ )
            if ( p->pInits[iBit+j] == 'x' )
                break;
        // print flop init state
        if ( 1 )
        {
            printf( "Flop %3d init state: %d\'b", i-Wlc_NtkPiNum(p), Wlc_ObjRange(pObj) );
            if ( j == Wlc_ObjRange(pObj) )
            {
                int Count = 0;
                for ( n = 0; n < Wlc_ObjRange(pObj); n++ )
                    Count += p->pInits[iBit+n] == '0';
                if ( Count == Wlc_ObjRange(pObj) )
                    printf( "0" );
                else
                    for ( n = 0; n < Wlc_ObjRange(pObj); n++ )
                        printf( "%c", p->pInits[iBit+n] );
            }
            else
            {
                int Count = 0;
                for ( n = 0; n < Wlc_ObjRange(pObj); n++ )
                    Count += p->pInits[iBit+n] == 'x';
                printf( "x" );
                if ( Count != Wlc_ObjRange(pObj) )
                    printf( " (range %d)", Wlc_ObjRange(pObj) );
            }
            printf( "\n" );
        }
        Type = j == Wlc_ObjRange(pObj) ? ABC_OPER_CONST : ABC_OPER_CI; 
        iOutId = Wln_ObjAlloc( pNew, Type, pObj->Signed, pObj->End, pObj->Beg );
        if ( j == Wlc_ObjRange(pObj) ) // constant
        {
            char * pString = Wln_ConstFromStr(p->pInits + iBit, Wlc_ObjRange(pObj));
            Wln_ObjSetConst( pNew, iOutId, Abc_NamStrFindOrAdd(pNew->pManName, pString, NULL) );
            ABC_FREE( pString );
        }
        sprintf( Buffer, "ff_init_%d", Vec_IntSize(vInits) );
        Wln_ObjSetNameId( pNew, iOutId, Abc_NamStrFindOrAdd(pNew->pManName, Buffer, NULL) );
        Vec_IntPush( vInits, iOutId );
        iBit += Wlc_ObjRange(pObj);
    }
    assert( p->pInits == NULL || iBit == (int)strlen(p->pInits) );
    // add flop outputs
    Wlc_NtkForEachCi( p, pObj, i ) 
    {
        assert( i == Wlc_ObjCiId(pObj) );
        if ( pObj->Type == WLC_OBJ_PI )
            continue;
        iOutId = Wln_ObjAlloc( pNew, ABC_OPER_DFFRSE, pObj->Signed, pObj->End, pObj->Beg );
        Wln_ObjSetNameId( pNew, iOutId, Wln_TrasformNameId(pNew, p, pObj) );
        Wlc_ObjSetCopy( p, Wlc_ObjId(p, pObj), iOutId );
    }
    // add internal nodes 
    Wlc_NtkForEachObj( p, pObj, i ) 
    {
        if ( Wlc_ObjIsCi(pObj) || pObj->Type == 0 )
            continue;
        iOutId = Wln_ObjAlloc( pNew, Ndr_TypeWlc2Ndr(pObj->Type), pObj->Signed, pObj->End, pObj->Beg );
        Vec_IntClear( vFanins );
        Wlc_ObjForEachFanin( pObj, iFanin, n )
            Vec_IntPush( vFanins, Wlc_ObjCopy(p, iFanin) );
        Wln_ObjAddFanins( pNew, iOutId, vFanins );
        if ( pObj->Type == WLC_OBJ_BIT_SELECT )
            Wln_ObjSetSlice( pNew, iOutId, Hash_Int2ManInsert(pNew->pRanges, pObj->End, pObj->Beg, 0) );
        else if ( pObj->Type == WLC_OBJ_CONST )
        {
            char * pString = Wln_ConstFromBits(Wlc_ObjConstValue(pObj), Wlc_ObjRange(pObj));
            Wln_ObjSetConst( pNew, iOutId, Abc_NamStrFindOrAdd(pNew->pManName, pString, NULL) );
            ABC_FREE( pString );
        }
//        else if ( Type == ABC_OPER_BIT_MUX && Vec_IntSize(vFanins) == 3 )
//            ABC_SWAP( int, Wln_ObjFanins(p, iObj)[1], Wln_ObjFanins(p, iObj)[2] );
        Wln_ObjSetNameId( pNew, iOutId, Wln_TrasformNameId(pNew, p, pObj) );
        Wlc_ObjSetCopy( p, i, iOutId );
    }
    Wlc_NtkForEachPo( p, pObj, i ) 
    {
        iOutId = Wln_ObjAlloc( pNew, ABC_OPER_CO, pObj->Signed, pObj->End, pObj->Beg );
        Wln_ObjAddFanin( pNew, iOutId, Wlc_ObjCopy(p, Wlc_ObjId(p, pObj)) );
        //Wln_ObjSetNameId( pNew, iOutId, Wln_TrasformNameId(pNew, p, pObj) );
    }
    assert( Vec_IntSize(vInits) == Wlc_NtkCoNum(p) - Wlc_NtkPoNum(p) );
    Wlc_NtkForEachCo( p, pObj, i ) 
    {
        if ( i < Wlc_NtkPoNum(p) )
            continue;
        //char * pInNames[8] = {"d", "clk", "reset", "set", "enable", "async", "sre", "init"};
        Vec_IntClear( vFanins );
        Vec_IntPush( vFanins, Wlc_ObjCopy(p, Wlc_ObjFaninId0(pObj)) );
        for ( n = 0; n < 6; n++ )
            Vec_IntPush( vFanins, 0 );
        Vec_IntPush( vFanins, Vec_IntEntry(vInits, i-Wlc_NtkPoNum(p)) );
        Wln_ObjAddFanins( pNew, Vec_IntEntry(&pNew->vFfs, i-Wlc_NtkPoNum(p)), vFanins );
    }
    Vec_IntFree( vFanins );
    Vec_IntFree( vInits );
    return pNew;
}
void Wln_NtkFromWlcTest( Wlc_Ntk_t * p )
{
    Wln_Ntk_t * pNew = Wln_NtkFromWlc( p );
    Wln_WriteVer( pNew, "test_wlc2wln.v" );
    Wln_NtkFree( pNew );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

