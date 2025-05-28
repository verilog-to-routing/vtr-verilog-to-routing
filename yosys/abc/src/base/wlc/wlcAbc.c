/**CFile****************************************************************

  FileName    [wlcAbc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Parses several flavors of word-level Verilog.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcAbc.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"
#include "base/abc/abc.h"

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
void Wlc_NtkPrintInputInfo( Wlc_Ntk_t * pNtk )
{
    Wlc_Obj_t * pObj;
    int i, k, nRange, nBeg, nEnd, nBits = 0;
    FILE * output;

    output = fopen("abc_blast_input.info","w");

    Wlc_NtkForEachCi( pNtk, pObj, i )
    {
        nRange = Wlc_ObjRange(pObj);
        nBeg = pObj->Beg;
        nEnd = pObj->End;

        for ( k = 0; k < nRange; k++ )
        {
            int index = nEnd > nBeg ? nBeg + k : nEnd + k;
            char c = pObj->Type != WLC_OBJ_FO ? 'i' : pNtk->pInits[nBits + k];
            fprintf(output,"%s[%d] : %c \n", Wlc_ObjName(pNtk, Wlc_ObjId(pNtk, pObj)), index , c );
        }
        if (pObj->Type == WLC_OBJ_FO)
            nBits += nRange;
    }

    Wlc_NtkForEachPo( pNtk, pObj, i )
    {
        nRange = Wlc_ObjRange(pObj);
        nBeg = pObj->Beg;
        nEnd = pObj->End;

        for ( k = 0; k < nRange; k++ )
        {
            int index = nEnd > nBeg ? nBeg + k : nEnd + k;
            fprintf(output,"%s[%d] : o \n", Wlc_ObjName(pNtk, Wlc_ObjId(pNtk, pObj)), index);
        }
    }

    fclose(output);
    return;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkPrintInvStats( Wlc_Ntk_t * pNtk, Vec_Int_t * vCounts, int fVerbose )
{
    Wlc_Obj_t * pObj;
    int i, k, nNum, nRange, nBits = 0;
    Wlc_NtkForEachCi( pNtk, pObj, i )
    {
        if ( pObj->Type != WLC_OBJ_FO )
            continue;
        nRange = Wlc_ObjRange(pObj);
        for ( k = 0; k < nRange; k++ )
        {
            nNum = Vec_IntEntry(vCounts, nBits + k);
            if ( nNum )
                break;
        }
        if ( k == nRange )
        {
            nBits += nRange;
            continue;
        }
        printf( "%s[%d:%d] : ", Wlc_ObjName(pNtk, Wlc_ObjId(pNtk, pObj)), pObj->End, pObj->Beg );
        for ( k = 0; k < nRange; k++ )
        {
            nNum = Vec_IntEntry( vCounts, nBits + k );
            if ( nNum == 0 )
                continue;
            printf( "  [%d] -> %d", k, nNum );
        }
        printf( "\n");
        nBits += nRange;
    }
    //printf( "%d %d\n", Vec_IntSize(vCounts), nBits );
    assert( Vec_IntSize(vCounts) == nBits );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Wlc_NtkGetInv( Wlc_Ntk_t * pNtk, Vec_Int_t * vInv, Vec_Ptr_t * vNamesIn )
{
    extern Vec_Int_t * Pdr_InvCounts( Vec_Int_t * vInv );
    extern Vec_Str_t * Pdr_InvPrintStr( Vec_Int_t * vInv, Vec_Int_t * vCounts );

    Vec_Int_t * vCounts = Pdr_InvCounts( vInv );
    Vec_Str_t * vSop = Pdr_InvPrintStr( vInv, vCounts );

    Wlc_Obj_t * pObj;
    int i, k, nNum, nRange, nBits = 0;
    Abc_Ntk_t * pMainNtk = NULL;
    Abc_Obj_t * pMainObj, * pMainTemp;
    char Buffer[5000];
    // start the network
    pMainNtk = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    // duplicate the name and the spec
    pMainNtk->pName = Extra_UtilStrsav(pNtk ? pNtk->pName : "inv");
    // create primary inputs
    if ( pNtk == NULL )
    {
        int Entry, nInputs = Abc_SopGetVarNum( Vec_StrArray(vSop) );
        Vec_IntForEachEntry( vCounts, Entry, i )
        {
            if ( Entry == 0 )
                continue;
            pMainObj = Abc_NtkCreatePi( pMainNtk );
            // If vNamesIn is given, take names from there for as many entries as possible
            // otherwise generate names from counter
            if (vNamesIn != NULL && i < Vec_PtrSize(vNamesIn)) {
                Abc_ObjAssignName( pMainObj, (char *)Vec_PtrEntry(vNamesIn, i), NULL );
            }
            else
            {
                sprintf( Buffer, "pi%d", i );
                Abc_ObjAssignName( pMainObj, Buffer, NULL );
            }
        }
        if ( Abc_NtkPiNum(pMainNtk) != nInputs )
        {
            printf( "Mismatch between number of inputs and the number of literals in the invariant.\n" );
            Abc_NtkDelete( pMainNtk );
            return NULL;
        }
    }
    else
    {
        Wlc_NtkForEachCi( pNtk, pObj, i )
        {
            if ( pObj->Type != WLC_OBJ_FO )
                continue;
            nRange = Wlc_ObjRange(pObj);
            for ( k = 0; k < nRange; k++ )
            {
                nNum = Vec_IntEntry(vCounts, nBits + k);
                if ( nNum )
                    break;
            }
            if ( k == nRange )
            {
                nBits += nRange;
                continue;
            }
            //printf( "%s[%d:%d] : ", Wlc_ObjName(pNtk, Wlc_ObjId(pNtk, pObj)), pObj->End, pObj->Beg );
            for ( k = 0; k < nRange; k++ )
            {
                nNum = Vec_IntEntry( vCounts, nBits + k );
                if ( nNum == 0 )
                    continue;
                //printf( "  [%d] -> %d", k, nNum );
                pMainObj = Abc_NtkCreatePi( pMainNtk );
                sprintf( Buffer, "%s[%d]", Wlc_ObjName(pNtk, Wlc_ObjId(pNtk, pObj)), k );
                Abc_ObjAssignName( pMainObj, Buffer, NULL );

            }
            //printf( "\n");
            nBits += nRange;
        }
    }
    //printf( "%d %d\n", Vec_IntSize(vCounts), nBits );
    assert( pNtk == NULL || Vec_IntSize(vCounts) == nBits );
    // create node
    pMainObj = Abc_NtkCreateNode( pMainNtk );
    Abc_NtkForEachPi( pMainNtk, pMainTemp, i )
        Abc_ObjAddFanin( pMainObj, pMainTemp );
    pMainObj->pData = Abc_SopRegister( (Mem_Flex_t *)pMainNtk->pManFunc, Vec_StrArray(vSop) );
    Vec_IntFree( vCounts );
    Vec_StrFree( vSop );
    // create PO
    pMainTemp = Abc_NtkCreatePo( pMainNtk );
    Abc_ObjAddFanin( pMainTemp, pMainObj );
    Abc_ObjAssignName( pMainTemp, "inv", NULL );
    return pMainNtk;
}

/**Function*************************************************************

  Synopsis    [Translate current network into an invariant.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Wlc_NtkGetPut( Abc_Ntk_t * pNtk, Gia_Man_t * pGia )
{
    int nRegs = Gia_ManRegNum(pGia);
    Vec_Int_t * vRes = NULL;
    if ( Abc_NtkPoNum(pNtk) != 1 )
        printf( "The number of outputs is other than 1.\n" );
    else if ( Abc_NtkNodeNum(pNtk) != 1 )
        printf( "The number of internal nodes is other than 1.\n" );
    else
    {
        Abc_Nam_t * pNames = NULL;
        Abc_Obj_t * pFanin, * pNode = Abc_ObjFanin0( Abc_NtkCo(pNtk, 0) );
        char * pName, * pCube, * pSop = (char *)pNode->pData;
        Vec_Int_t * vFanins = Vec_IntAlloc( Abc_ObjFaninNum(pNode) );
        int i, k, Value, nLits, Counter = 0;
        if ( pGia->vNamesIn )
        {
            // hash the names
            pNames = Abc_NamStart( 100, 16 );
            Vec_PtrForEachEntry( char *, pGia->vNamesIn, pName, i )
            {
                Value = Abc_NamStrFindOrAdd( pNames, pName, NULL );
                assert( Value == i+1 );
                //printf( "%s(%d) ", pName, i );
            }
            //printf( "\n" );
        }
        Abc_ObjForEachFanin( pNode, pFanin, i )
        {
            assert( Abc_ObjIsCi(pFanin) );
            pName = Abc_ObjName(pFanin);
            if ( pNames )
            {
                Value = Abc_NamStrFind(pNames, pName) - 1 - Gia_ManPiNum(pGia);
                if ( Value < 0 )
                {
                    if ( Counter++ == 0 )
                        printf( "Cannot read input name \"%s\" of fanin %d.\n", pName, i );
                    Value = i;
                }
            }
            else
            {
                for ( k = (int)strlen(pName)-1; k >= 0; k-- )
                    if ( pName[k] < '0' || pName[k] > '9' )
                        break;
                if ( k == (int)strlen(pName)-1 )
                {
                    if ( Counter++ == 0 )
                        printf( "Cannot read input name \"%s\" of fanin %d.\n", pName, i );
                    Value = i;
                }
                else 
                    Value = atoi(pName + k + 1);
            }
            Vec_IntPush( vFanins, Value );
        }
        if ( Counter )
            printf( "Cannot read names for %d inputs of the invariant.\n", Counter );
        if ( pNames )
            Abc_NamStop( pNames );
        assert( Vec_IntSize(vFanins) == Abc_ObjFaninNum(pNode) );
        vRes = Vec_IntAlloc( 1000 );
        Vec_IntPush( vRes, Abc_SopGetCubeNum(pSop) );
        Abc_SopForEachCube( pSop, Abc_ObjFaninNum(pNode), pCube )
        {
            nLits = 0;
            Abc_CubeForEachVar( pCube, Value, k )
                if ( Value != '-' )
                    nLits++;
            Vec_IntPush( vRes, nLits );
            Abc_CubeForEachVar( pCube, Value, k )
                if ( Value != '-' )
                    Vec_IntPush( vRes, Abc_Var2Lit(Vec_IntEntry(vFanins, k), (int)Value == '0') );
        }
        Vec_IntPush( vRes, nRegs );
        Vec_IntFree( vFanins );
    }
    return vRes;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

