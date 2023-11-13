/**CFile****************************************************************

  FileName    [wlnNdr.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Word-level network.]

  Synopsis    [Constructing WLN network from NDR data structure.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 23, 2018.]

  Revision    [$Id: wlnNdr.c,v 1.00 2018/09/23 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wln.h"
#include "aig/miniaig/ndr.h"

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
void * Wln_NtkToNdr( Wln_Ntk_t * p )
{
    Vec_Int_t * vFanins;
    int i, k, iObj, iFanin;
    // create a new module
    void * pDesign = Ndr_Create( 1 );
    int ModId = Ndr_AddModule( pDesign, 1 );
    // add primary inputs
    Wln_NtkForEachPi( p, iObj, i ) 
    {
        Ndr_AddObject( pDesign, ModId, ABC_OPER_CI, 0,   
            Wln_ObjRangeEnd(p, iObj), Wln_ObjRangeBeg(p, iObj), Wln_ObjIsSigned(p, iObj),   
            0, NULL,  1, &iObj,  NULL  ); // no fanins
    }
    // add internal nodes 
    vFanins = Vec_IntAlloc( 10 );
    Wln_NtkForEachObjInternal( p, iObj ) 
    {
        Vec_IntClear( vFanins );
        Wln_ObjForEachFanin( p, iObj, iFanin, k )
            Vec_IntPush( vFanins, iFanin );
        Ndr_AddObject( pDesign, ModId, Wln_ObjType(p, iObj), 0,   
            Wln_ObjRangeEnd(p, iObj), Wln_ObjRangeBeg(p, iObj), Wln_ObjIsSigned(p, iObj),   
            Vec_IntSize(vFanins), Vec_IntArray(vFanins), 1, &iObj,  
            Wln_ObjIsConst(p, iObj) ? Wln_ObjConstString(p, iObj) : NULL ); 
    }
    Vec_IntFree( vFanins );
    // add primary outputs
    Wln_NtkForEachPo( p, iObj, i ) 
    {
        Ndr_AddObject( pDesign, ModId, ABC_OPER_CO, 0,                   
            Wln_ObjRangeEnd(p, iObj), Wln_ObjRangeBeg(p, iObj), Wln_ObjIsSigned(p, iObj),   
            1, &iObj,  0, NULL,  NULL ); 
    }
    return pDesign;
}
void Wln_WriteNdr( Wln_Ntk_t * p, char * pFileName )
{
    void * pDesign = Wln_NtkToNdr( p );
    Ndr_Write( pFileName, pDesign );
    Ndr_Delete( pDesign );
    printf( "Dumped the current design into file \"%s\".\n", pFileName );
}
void Wln_NtkToNdrTest( Wln_Ntk_t * p )
{
    // transform
    void * pDesign = Wln_NtkToNdr( p );

    // collect names     
    char ** ppNames = ABC_ALLOC( char *, Wln_NtkObjNum(p) + 1 ); int i;
    Wln_NtkForEachObj( p, i ) 
        ppNames[i] = Abc_UtilStrsav(Wln_ObjName(p, i));

    // verify by writing Verilog
    Ndr_WriteVerilog( NULL, pDesign, ppNames, 0 );
    Ndr_Write( "test.ndr", pDesign );

    // cleanup
    Ndr_Delete( pDesign );
    Wln_NtkForEachObj( p, i ) 
        ABC_FREE( ppNames[i] );
    ABC_FREE( ppNames );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ndr_ObjGetRange( Ndr_Data_t * p, int Obj, int * End, int * Beg )
{
    int * pArray, nArray = Ndr_ObjReadArray( p, Obj, NDR_RANGE, &pArray );
    int Signed = 0; *End = *Beg = 0;
    if ( nArray == 0 )
        return 0;
    if ( nArray == 3 )
        Signed = 1;
    if ( nArray == 1 )
        *End = *Beg = pArray[0];
    else
        *End = pArray[0], *Beg = pArray[1];
    return Signed;
}
void Ndr_NtkPrintObjects( Wln_Ntk_t * pNtk )
{
    int k, iObj, iFanin;
    printf( "Node IDs and their fanins:\n" );
    Wln_NtkForEachObj( pNtk, iObj )
    {
        printf( "%5d = ", iObj );
        Wln_ObjForEachFanin( pNtk, iObj, iFanin, k )
            printf( "%5d ", iFanin );
        for (      ; k < 4; k++ )
            printf( "      " );
        printf( "    Name Id %d ", Wln_ObjNameId(pNtk, iObj) );
        if ( Wln_ObjIsPi(pNtk, iObj) )
            printf( "  pi  " );
        if ( Wln_ObjIsPo(pNtk, iObj) )
            printf( "  po  " );
        printf( "\n" );
    }
}
void Wln_NtkCheckIntegrity( void * pData )
{
    Ndr_Data_t * p = (Ndr_Data_t *)pData;  
    Vec_Int_t * vMap = Vec_IntAlloc( 100 );
    int Mod = 2, Obj;
    Ndr_ModForEachObj( p, Mod, Obj )
    {
        int NameId  = Ndr_ObjReadBody( p, Obj, NDR_OUTPUT );
        if ( NameId == -1 )
        {
            int Type = Ndr_ObjReadBody( p, Obj, NDR_OPERTYPE );
            if ( Type != ABC_OPER_CO )
                printf( "Internal object %d of type %s has no output name.\n", Obj, Abc_OperName(Type) );
            continue;
        }
        if ( Vec_IntGetEntry(vMap, NameId) > 0 )
            printf( "Output name %d is used more than once (obj %d and obj %d).\n", NameId, Vec_IntGetEntry(vMap, NameId), Obj );
        Vec_IntSetEntry( vMap, NameId, Obj );
    }
    Ndr_ModForEachObj( p, Mod, Obj )
    {
        int Type = Ndr_ObjReadBody( p, Obj, NDR_OPERTYPE );
        int i, * pArray, nArray  = Ndr_ObjReadArray( p, Obj, NDR_INPUT, &pArray );
        for ( i = 0; i < nArray; i++ )
            if ( Vec_IntGetEntry(vMap, pArray[i]) == 0 && !(Type == ABC_OPER_DFFRSE && (i >= 5 && i <= 7)) )
                printf( "Input name %d appearing as fanin %d of obj %d is not used as output name in any object.\n", pArray[i], i, Obj );
    }
    Vec_IntFree( vMap );
}
Wln_Ntk_t * Wln_NtkFromNdr( void * pData, int fDump )
{
    Ndr_Data_t * p = (Ndr_Data_t *)pData;  
    Vec_Int_t * vName2Obj, * vFanins = Vec_IntAlloc( 100 );
    Vec_Ptr_t * vConstStrings = Vec_PtrAlloc( 100 );
    int Mod = 2, i, k, iFanin, iObj, Obj, * pArray, nDigits, fFound, NameId, NameIdMax;
    Wln_Ntk_t * pTemp, * pNtk = Wln_NtkAlloc( "top", Ndr_DataObjNum(p, Mod) );
    Wln_NtkCheckIntegrity( pData );
    //pNtk->pSpec = Abc_UtilStrsav( pFileName );
    // construct network and save name IDs
    Wln_NtkCleanNameId( pNtk );
    Wln_NtkCleanInstId( pNtk );
    Ndr_ModForEachPi( p, Mod, Obj )
    {
        int End, Beg, Signed = Ndr_ObjGetRange(p, Obj, &End, &Beg);
        int iObj = Wln_ObjAlloc( pNtk, ABC_OPER_CI, Signed, End, Beg );
        int NameId = Ndr_ObjReadBody( p, Obj, NDR_OUTPUT );
        int InstId = Ndr_ObjReadBody( p, Obj, NDR_NAME );
        Wln_ObjSetNameId( pNtk, iObj, NameId );
        if ( InstId > 0 ) Wln_ObjSetInstId( pNtk, iObj, InstId );
    }
    Ndr_ModForEachNode( p, Mod, Obj )
    {
        int End, Beg, Signed = Ndr_ObjGetRange(p, Obj, &End, &Beg);
        int Type    = Ndr_ObjReadBody( p, Obj, NDR_OPERTYPE );
        int nArray  = Ndr_ObjReadArray( p, Obj, NDR_INPUT, &pArray );
        Vec_Int_t F = {nArray, nArray, pArray}, * vTemp = &F;
        int iObj    = Wln_ObjAlloc( pNtk, Type, Signed, End, Beg );
        int NameId  = Ndr_ObjReadBody( p, Obj, NDR_OUTPUT );
        int InstId  = Ndr_ObjReadBody( p, Obj, NDR_NAME );
        Vec_IntClear( vFanins );
        Vec_IntAppend( vFanins, vTemp );
        assert( Type != ABC_OPER_DFF );
        if ( Wln_ObjIsSlice(pNtk, iObj) )
            Wln_ObjSetSlice( pNtk, iObj, Hash_Int2ManInsert(pNtk->pRanges, End, Beg, 0) );
        else if ( Wln_ObjIsConst(pNtk, iObj) )
            Vec_PtrPush( vConstStrings, (char *)Ndr_ObjReadBodyP(p, Obj, NDR_FUNCTION) );
//        else if ( Type == ABC_OPER_BIT_MUX && Vec_IntSize(vFanins) == 3 )
//            ABC_SWAP( int, Wln_ObjFanins(pNtk, iObj)[1], Wln_ObjFanins(pNtk, iObj)[2] );
        Wln_ObjAddFanins( pNtk, iObj, vFanins );
        Wln_ObjSetNameId( pNtk, iObj, NameId );
        if ( InstId > 0 ) Wln_ObjSetInstId( pNtk, iObj, InstId );
        if ( Type == ABC_OPER_ARI_SMUL )
        {
            assert( Wln_ObjFaninNum(pNtk, iObj) == 2 );
            Wln_ObjSetSigned( pNtk, Wln_ObjFanin0(pNtk, iObj) );
            Wln_ObjSetSigned( pNtk, Wln_ObjFanin1(pNtk, iObj) );
        }
    }
    // mark primary outputs
    Ndr_ModForEachPo( p, Mod, Obj )
    {
        int End, Beg, Signed = Ndr_ObjGetRange(p, Obj, &End, &Beg);
        int nArray  = Ndr_ObjReadArray( p, Obj, NDR_INPUT, &pArray );
        int iObj    = Wln_ObjAlloc( pNtk, ABC_OPER_CO, Signed, End, Beg );
        int NameId  = Ndr_ObjReadBody( p, Obj, NDR_OUTPUT );
        int InstId  = Ndr_ObjReadBody( p, Obj, NDR_NAME );
        assert( nArray == 1 && NameId == -1 && InstId == -1 );
        Wln_ObjAddFanin( pNtk, iObj, pArray[0] );
    }
    Vec_IntFree( vFanins );
    // remove instance names if they are not given
    //Vec_IntPrint( &pNtk->vInstIds );
    if ( Vec_IntCountPositive(&pNtk->vInstIds) == 0 )
        Vec_IntErase( &pNtk->vInstIds );
    // map name IDs into object IDs
    vName2Obj = Vec_IntInvert( &pNtk->vNameIds, 0 );
    Wln_NtkForEachObj( pNtk, i )
        Wln_ObjForEachFanin( pNtk, i, iFanin, k )
            Wln_ObjSetFanin( pNtk, i, k, Vec_IntEntry(vName2Obj, iFanin) );
    Vec_IntFree(vName2Obj);
    // create fake object names
    NameIdMax = Vec_IntFindMax(&pNtk->vNameIds);
    nDigits = Abc_Base10Log( NameIdMax+1 );
    pNtk->pManName = Abc_NamStart( NameIdMax+1, 10 );
    for ( i = 1; i <= NameIdMax; i++ )
    {
        char pName[1000]; sprintf( pName, "s%0*d", (unsigned char)nDigits, i );
        NameId = Abc_NamStrFindOrAdd( pNtk->pManName, pName, &fFound );
        assert( !fFound && i == NameId );
    }
    // add const strings
    i = 0;
    Wln_NtkForEachObj( pNtk, iObj )
        if ( Wln_ObjIsConst(pNtk, iObj) )
            Wln_ObjSetConst( pNtk, iObj, Abc_NamStrFindOrAdd(pNtk->pManName, (char *)Vec_PtrEntry(vConstStrings, i++), NULL) );
    assert( i == Vec_PtrSize(vConstStrings) );
    Vec_PtrFree( vConstStrings );
    //Ndr_NtkPrintObjects( pNtk );
    Wln_WriteVer( pNtk, "temp_ndr.v" );
    printf( "Dumped design \"%s\" into file \"temp_ndr.v\".\n", pNtk->pName );
    if ( !Wln_NtkIsAcyclic(pNtk) )
    {
        Wln_NtkFree(pNtk);
        return NULL;
    }
    // derive topological order
    pNtk = Wln_NtkDupDfs( pTemp = pNtk );
    Wln_NtkFree( pTemp );
    //Ndr_NtkPrintObjects( pNtk );
    //pNtk->fMemPorts = 1;          // the network contains memory ports
    //pNtk->fEasyFfs = 1;           // the network contains simple flops
    return pNtk;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wln_Ntk_t * Wln_ReadNdr( char * pFileName )
{
    void * pData = Ndr_Read( pFileName );
    Wln_Ntk_t * pNtk = pData ? Wln_NtkFromNdr( pData, 0 ) : NULL;
    if ( pNtk ) return NULL;
    //char * ppNames[10] = { NULL, "a", "b", "c", "d", "e", "f", "g", "h", "i" };
    //Ndr_WriteVerilog( NULL, pData, ppNames, 0 );
    Ndr_Delete( pData );
    return pNtk;
}
void Wln_ReadNdrTest()
{
    Wln_Ntk_t * pNtk = Wln_ReadNdr( "D:\\temp\\brijesh\\for_alan_dff_warning\\work_fir_filter_fir_filter_proc_out.ndr" );
    //Wln_Ntk_t * pNtk = Wln_ReadNdr( "flop.ndr" );
    Wln_WriteVer( pNtk, "test__test.v" );
    Wln_NtkPrint( pNtk );
    Wln_NtkStaticFanoutTest( pNtk );
    Wln_NtkFree( pNtk );
}
void Wln_NtkRetimeTest( char * pFileName, int fIgnoreIO, int fSkipSimple, int fDump, int fVerbose )
{
    Vec_Int_t * vMoves;
    void * pData = Ndr_Read( pFileName );
    Wln_Ntk_t * pNtk = pData ? Wln_NtkFromNdr( pData, fDump ) : NULL;
    Ndr_Delete( pData );
    if ( pNtk == NULL ) 
    {
        printf( "Retiming network is not available.\n" );
        return;
    }
    Wln_NtkRetimeCreateDelayInfo( pNtk );
    vMoves = Wln_NtkRetime( pNtk, fIgnoreIO, fSkipSimple, fVerbose );
    //Vec_IntPrint( vMoves );
    Vec_IntFree( vMoves );
    Wln_NtkFree( pNtk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

