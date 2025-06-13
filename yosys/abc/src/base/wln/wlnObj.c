/**CFile****************************************************************

  FileName    [wlnObj.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Word-level network.]

  Synopsis    [Object construction procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 23, 2018.]

  Revision    [$Id: wlnObj.c,v 1.00 2018/09/23 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wln.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creating objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Wln_ObjName( Wln_Ntk_t * p, int iObj )
{
    static char Buffer[100];
    if ( Wln_NtkHasNameId(p) && Wln_ObjNameId(p, iObj) )
        return Abc_NamStr( p->pManName, Wln_ObjNameId(p, iObj) );
    sprintf( Buffer, "n%d", iObj );
    return Buffer;
}                          
char * Wln_ObjConstString( Wln_Ntk_t * p, int iObj )
{
    assert( Wln_ObjIsConst(p, iObj) );
    return Abc_NamStr( p->pManName, Wln_ObjFanin0(p, iObj) );
}                          
void Wln_ObjUpdateType( Wln_Ntk_t * p, int iObj, int Type )
{
    assert( Wln_ObjIsNone(p, iObj) );
    p->nObjs[Wln_ObjType(p, iObj)]--;
    Vec_IntWriteEntry( &p->vTypes, iObj, Type );
    p->nObjs[Wln_ObjType(p, iObj)]++;
}
void Wln_ObjSetConst( Wln_Ntk_t * p, int iObj, int NameId )
{
    assert( Wln_ObjIsConst(p, iObj) );
    Wln_ObjSetFanin( p, iObj, 0, NameId );
}
void Wln_ObjSetSlice( Wln_Ntk_t * p, int iObj, int SliceId )
{
    assert( Wln_ObjIsSlice(p, iObj) );
    Wln_ObjSetFanin( p, iObj, 1, SliceId );
}
void Wln_ObjAddFanin( Wln_Ntk_t * p, int iObj, int i )        
{ 
    Wln_Vec_t * pVec = p->vFanins + iObj;
    if ( Wln_ObjFaninNum(p, iObj) < 2 )
        pVec->Array[pVec->nSize++] = i;
    else if ( Wln_ObjFaninNum(p, iObj) == 2 )
    {
        int * pArray = ABC_ALLOC( int, 4 );
        pArray[0] = pVec->Array[0];
        pArray[1] = pVec->Array[1];
        pArray[2] = i;
        pVec->pArray[0] = pArray;
        pVec->nSize     = 3;
        pVec->nCap      = 4;
    }
    else 
    {
        if ( pVec->nSize == pVec->nCap )
            pVec->pArray[0] = ABC_REALLOC( int, pVec->pArray[0], (pVec->nCap = 2*pVec->nSize) ); 
        assert( pVec->nSize < pVec->nCap );
        pVec->pArray[0][pVec->nSize++] = i;
    }
}
int Wln_ObjAddFanins( Wln_Ntk_t * p, int iObj, Vec_Int_t * vFanins )
{
    int i, iFanin;
    Vec_IntForEachEntry( vFanins, iFanin, i )
        Wln_ObjAddFanin( p, iObj, iFanin );
    return iObj;
}
int Wln_ObjAlloc( Wln_Ntk_t * p, int Type, int Signed, int End, int Beg )
{
    int iObj = Vec_IntSize(&p->vTypes);
    if ( iObj == Vec_IntCap(&p->vTypes) )
    {
        p->vFanins = ABC_REALLOC( Wln_Vec_t, p->vFanins, 2 * iObj );
        memset( p->vFanins + iObj, 0, sizeof(Wln_Vec_t) * iObj );
        Vec_IntGrow( &p->vTypes, 2 * iObj );
    }
    assert( iObj == Vec_StrSize(&p->vSigns) );
    assert( iObj == Vec_IntSize(&p->vRanges) );
    Vec_IntPush( &p->vTypes, Type );
    Vec_StrPush( &p->vSigns, (char)Signed );
    Vec_IntPush( &p->vRanges, Hash_Int2ManInsert(p->pRanges, End, Beg, 0) );
    if ( Wln_ObjIsCi(p, iObj) ) Wln_ObjSetFanin( p, iObj, 1, Vec_IntSize(&p->vCis) ), Vec_IntPush( &p->vCis, iObj );
    if ( Wln_ObjIsCo(p, iObj) ) Wln_ObjSetFanin( p, iObj, 1, Vec_IntSize(&p->vCos) ), Vec_IntPush( &p->vCos, iObj );
    if ( Wln_ObjIsFf(p, iObj) ) Vec_IntPush( &p->vFfs, iObj );
    p->nObjs[Type]++;
    return iObj;
}
int Wln_ObjClone( Wln_Ntk_t * pNew, Wln_Ntk_t * p, int iObj )
{
    return Wln_ObjAlloc( pNew, Wln_ObjType(p, iObj), Wln_ObjIsSigned(p, iObj), Wln_ObjRangeEnd(p, iObj), Wln_ObjRangeBeg(p, iObj) );
}
int Wln_ObjCreateCo( Wln_Ntk_t * p, int iFanin )
{
    int iCo = Wln_ObjClone( p, p, iFanin );
    Wln_ObjUpdateType( p, iCo, ABC_OPER_CO );
    Wln_ObjAddFanin( p, iCo, iFanin );
    return iCo;
}
void Wln_ObjPrint( Wln_Ntk_t * p, int iObj )
{
    int k, iFanin, Type = Wln_ObjType(p, iObj);
    printf( "Obj %6d : Type = %6s  Fanins = %d : ", iObj, Abc_OperName(Type), Wln_ObjFaninNum(p, iObj) );
    Wln_ObjForEachFanin( p, iObj, iFanin, k )
        printf( "%5d ", iFanin );
    printf( "\n" );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

