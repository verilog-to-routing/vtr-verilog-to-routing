/**CFile****************************************************************

  FileName    [bacPrsBuild.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Parse tree to netlist transformation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: bacPrsBuild.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bac.h"
#include "bacPrs.h"
#include "map/mio/mio.h"
#include "base/main/main.h"

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
int Psr_ManIsMapped( Psr_Ntk_t * pNtk )
{
    Vec_Int_t * vSigs; int iBox;
    Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
    if ( pLib == NULL )
        return 0;
    Psr_NtkForEachBox( pNtk, vSigs, iBox )
        if ( !Psr_BoxIsNode(pNtk, iBox) )
        {
            int NtkId = Psr_BoxNtk( pNtk, iBox );
            if ( Mio_LibraryReadGateByName(pLib, Psr_NtkStr(pNtk, NtkId), NULL) )
                return 1;
        }
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Psr_NtkCountObjects( Psr_Ntk_t * pNtk )
{
    Vec_Int_t * vFanins; 
    int i, Count = Psr_NtkObjNum(pNtk);
    Psr_NtkForEachBox( pNtk, vFanins, i )
        Count += Psr_BoxIONum(pNtk, i);
    return Count;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// replaces NameIds of formal names by their index in the box model
void Psr_ManRemapOne( Vec_Int_t * vSigs, Psr_Ntk_t * pNtkBox, Vec_Int_t * vMap )
{
    int i, NameId;
    // map formal names into I/O indexes
    Psr_NtkForEachPi( pNtkBox, NameId, i )
    {
        assert( Vec_IntEntry(vMap, NameId) == -1 );
        Vec_IntWriteEntry( vMap, NameId, i + 1 ); // +1 to keep 1st form input non-zero
    }
    Psr_NtkForEachPo( pNtkBox, NameId, i )
    {
        assert( Vec_IntEntry(vMap, NameId) == -1 );
        Vec_IntWriteEntry( vMap, NameId, Psr_NtkPiNum(pNtkBox) + i + 1 ); // +1 to keep 1st form input non-zero
    }
    // remap box
    assert( Vec_IntSize(vSigs) % 2 == 0 );
    Vec_IntForEachEntry( vSigs, NameId, i )
    {
        assert( Vec_IntEntry(vMap, NameId) != -1 );
        Vec_IntWriteEntry( vSigs, i++, Vec_IntEntry(vMap, NameId) );
    }
    // unmap formal inputs
    Psr_NtkForEachPi( pNtkBox, NameId, i )
        Vec_IntWriteEntry( vMap, NameId, -1 );
    Psr_NtkForEachPo( pNtkBox, NameId, i )
        Vec_IntWriteEntry( vMap, NameId, -1 );
}
void Psr_ManRemapGate( Vec_Int_t * vSigs )
{
    int i, FormId;
    Vec_IntForEachEntry( vSigs, FormId, i )
        Vec_IntWriteEntry( vSigs, i, i/2 + 1 ), i++;
}
void Psr_ManRemapBoxes( Bac_Man_t * pNew, Vec_Ptr_t * vDes, Psr_Ntk_t * pNtk, Vec_Int_t * vMap )
{
    Vec_Int_t * vSigs; int iBox;
    Psr_NtkForEachBox( pNtk, vSigs, iBox )
        if ( !Psr_BoxIsNode(pNtk, iBox) )
        {
            int NtkId = Psr_BoxNtk( pNtk, iBox );
            int NtkIdNew = Bac_ManNtkFindId( pNew, Psr_NtkStr(pNtk, NtkId) );
            assert( NtkIdNew > 0 );
            Psr_BoxSetNtk( pNtk, iBox, NtkIdNew );
            if ( NtkIdNew <= Bac_ManNtkNum(pNew) )
                Psr_ManRemapOne( vSigs, Psr_ManNtk(vDes, NtkIdNew-1), vMap );
            //else
            //    Psr_ManRemapGate( vSigs );
        }
}
void Psr_ManCleanMap( Psr_Ntk_t * pNtk, Vec_Int_t * vMap )
{
    Vec_Int_t * vSigs; 
    int i, k, NameId, Sig;
    Psr_NtkForEachPi( pNtk, NameId, i )
        Vec_IntWriteEntry( vMap, NameId, -1 );
    Psr_NtkForEachBox( pNtk, vSigs, i )
        Vec_IntForEachEntryDouble( vSigs, NameId, Sig, k )
            Vec_IntWriteEntry( vMap, Psr_NtkSigName(pNtk, Sig), -1 );
    Psr_NtkForEachPo( pNtk, NameId, i )
        Vec_IntWriteEntry( vMap, NameId, -1 );
}
// create maps of NameId and boxes
void Psr_ManBuildNtk( Bac_Ntk_t * pNew, Vec_Ptr_t * vDes, Psr_Ntk_t * pNtk, Vec_Int_t * vMap, Vec_Int_t * vBoxes )
{
    Psr_Ntk_t * pNtkBox; Vec_Int_t * vSigs; int iBox;
    int i, Index, NameId, iObj, iConst0, iTerm;
    int iNonDriven = -1, nNonDriven = 0;
    assert( Psr_NtkPioNum(pNtk) == 0 );
    Psr_ManRemapBoxes( pNew->pDesign, vDes, pNtk, vMap );
    Bac_NtkStartNames( pNew );
    // create primary inputs 
    Psr_NtkForEachPi( pNtk, NameId, i )
    {
        if ( Vec_IntEntry(vMap, NameId) != -1 )
            printf( "Primary inputs %d and %d have the same name.\n", Vec_IntEntry(vMap, NameId), i );
        iObj = Bac_ObjAlloc( pNew, BAC_OBJ_PI, -1 );
        Bac_ObjSetName( pNew, iObj, Abc_Var2Lit2(NameId, BAC_NAME_BIN) );
        Vec_IntWriteEntry( vMap, NameId, iObj );
    }
    // create box outputs
    Vec_IntClear( vBoxes );
    Psr_NtkForEachBox( pNtk, vSigs, iBox )
        if ( !Psr_BoxIsNode(pNtk, iBox) )
        {
            pNtkBox = Psr_ManNtk( vDes, Psr_BoxNtk(pNtk, iBox)-1 );
            if ( pNtkBox == NULL )
            {
                iObj = Bac_BoxAlloc( pNew, BAC_BOX_GATE, Vec_IntSize(vSigs)/2-1, 1, Psr_BoxNtk(pNtk, iBox) );
                Bac_ObjSetName( pNew, iObj, Abc_Var2Lit2(Psr_BoxName(pNtk, iBox), BAC_NAME_BIN) );
                // consider box output 
                NameId = Vec_IntEntryLast( vSigs );
                NameId = Psr_NtkSigName( pNtk, NameId );
                if ( Vec_IntEntry(vMap, NameId) != -1 )
                    printf( "Box output name %d is already driven.\n", NameId );
                iTerm = Bac_BoxBo( pNew, iObj, 0 );
                Bac_ObjSetName( pNew, iTerm, Abc_Var2Lit2(NameId, BAC_NAME_BIN) );
                Vec_IntWriteEntry( vMap, NameId, iTerm );
            }
            else
            {
                iObj = Bac_BoxAlloc( pNew, BAC_OBJ_BOX, Psr_NtkPiNum(pNtkBox), Psr_NtkPoNum(pNtkBox), Psr_BoxNtk(pNtk, iBox) );
                Bac_ObjSetName( pNew, iObj, Abc_Var2Lit2(Psr_BoxName(pNtk, iBox), BAC_NAME_BIN) );
                Bac_NtkSetHost( Bac_ManNtk(pNew->pDesign, Psr_BoxNtk(pNtk, iBox)), Bac_NtkId(pNew), iObj );
                Vec_IntForEachEntry( vSigs, Index, i )
                {
                    i++;
                    if ( --Index < Psr_NtkPiNum(pNtkBox) )
                        continue;
                    assert( Index - Psr_NtkPiNum(pNtkBox) < Psr_NtkPoNum(pNtkBox) );
                    // consider box output 
                    NameId = Vec_IntEntry( vSigs, i );
                    NameId = Psr_NtkSigName( pNtk, NameId );
                    if ( Vec_IntEntry(vMap, NameId) != -1 )
                        printf( "Box output name %d is already driven.\n", NameId );
                    iTerm = Bac_BoxBo( pNew, iObj, Index - Psr_NtkPiNum(pNtkBox) );
                    Bac_ObjSetName( pNew, iTerm, Abc_Var2Lit2(NameId, BAC_NAME_BIN) );
                    Vec_IntWriteEntry( vMap, NameId, iTerm );
                }
            }
            // remember box
            Vec_IntPush( vBoxes, iObj );
        }
        else
        {
            iObj = Bac_BoxAlloc( pNew, (Bac_ObjType_t)Psr_BoxNtk(pNtk, iBox), Psr_BoxIONum(pNtk, iBox)-1, 1, -1 );
            // consider box output 
            NameId = Vec_IntEntryLast( vSigs );
            NameId = Psr_NtkSigName( pNtk, NameId );
            if ( Vec_IntEntry(vMap, NameId) != -1 )
                printf( "Node output name %d is already driven.\n", NameId );
            iTerm = Bac_BoxBo( pNew, iObj, 0 );
            Bac_ObjSetName( pNew, iTerm, Abc_Var2Lit2(NameId, BAC_NAME_BIN) );
            Vec_IntWriteEntry( vMap, NameId, iTerm );
            // remember box
            Vec_IntPush( vBoxes, iObj );
        }
    // add fanins for box inputs
    Psr_NtkForEachBox( pNtk, vSigs, iBox )
        if ( !Psr_BoxIsNode(pNtk, iBox) )
        {
            pNtkBox = Psr_ManNtk( vDes, Psr_BoxNtk(pNtk, iBox)-1 );
            iObj = Vec_IntEntry( vBoxes, iBox );
            if ( pNtkBox == NULL )
            {
                Vec_IntForEachEntryStop( vSigs, Index, i, Vec_IntSize(vSigs)-2 )
                {
                    i++;
                    NameId = Vec_IntEntry( vSigs, i );
                    NameId = Psr_NtkSigName( pNtk, NameId );
                    iTerm = Bac_BoxBi( pNew, iObj, i/2 );
                    if ( Vec_IntEntry(vMap, NameId) == -1 )
                    {
                        iConst0 = Bac_BoxAlloc( pNew, BAC_BOX_CF, 0, 1, -1 );
                        Vec_IntWriteEntry( vMap, NameId, iConst0+1 );
                        if ( iNonDriven == -1 )
                            iNonDriven = NameId;
                        nNonDriven++;
                    }
                    Bac_ObjSetFanin( pNew, iTerm, Vec_IntEntry(vMap, NameId) );
                }
            }
            else
            {
                Vec_IntForEachEntry( vSigs, Index, i )
                {
                    i++;
                    if ( --Index >= Psr_NtkPiNum(pNtkBox) )
                        continue;
                    NameId = Vec_IntEntry( vSigs, i );
                    NameId = Psr_NtkSigName( pNtk, NameId );
                    iTerm = Bac_BoxBi( pNew, iObj, Index );
                    if ( Vec_IntEntry(vMap, NameId) == -1 )
                    {
                        iConst0 = Bac_BoxAlloc( pNew, BAC_BOX_CF, 0, 1, -1 );
                        Vec_IntWriteEntry( vMap, NameId, iConst0+1 );
                        if ( iNonDriven == -1 )
                            iNonDriven = NameId;
                        nNonDriven++;
                    }
                    Bac_ObjSetFanin( pNew, iTerm, Vec_IntEntry(vMap, NameId) );
                }
            }
        }
        else
        {
            iObj = Vec_IntEntry( vBoxes, iBox );
            Vec_IntForEachEntryStop( vSigs, Index, i, Vec_IntSize(vSigs)-2 )
            {
                NameId = Vec_IntEntry( vSigs, ++i );
                NameId = Psr_NtkSigName( pNtk, NameId );
                iTerm = Bac_BoxBi( pNew, iObj, i/2 );
                if ( Vec_IntEntry(vMap, NameId) == -1 )
                {
                    iConst0 = Bac_BoxAlloc( pNew, BAC_BOX_CF, 0, 1, -1 );
                    Vec_IntWriteEntry( vMap, NameId, iConst0+1 );
                    if ( iNonDriven == -1 )
                        iNonDriven = NameId;
                    nNonDriven++;
                }
                Bac_ObjSetFanin( pNew, iTerm, Vec_IntEntry(vMap, NameId) );
            }
        }
    // add fanins for primary outputs
    Psr_NtkForEachPo( pNtk, NameId, i )
        if ( Vec_IntEntry(vMap, NameId) == -1 )
        {
            iConst0 = Bac_BoxAlloc( pNew, BAC_BOX_CF, 0, 1, -1 );
            Vec_IntWriteEntry( vMap, NameId, iConst0+1 );
            if ( iNonDriven == -1 )
                iNonDriven = NameId;
            nNonDriven++;
        }
    Psr_NtkForEachPo( pNtk, NameId, i )
        iObj = Bac_ObjAlloc( pNew, BAC_OBJ_PO, Vec_IntEntry(vMap, NameId) );
    if ( nNonDriven )
        printf( "Module %s has %d non-driven nets (for example, %s).\n", Psr_NtkName(pNtk), nNonDriven, Psr_NtkStr(pNtk, iNonDriven) );
    Psr_ManCleanMap( pNtk, vMap );
    // setup info
    Vec_IntForEachEntry( &pNtk->vOrder, NameId, i )
        Bac_NtkAddInfo( pNew, NameId, -1, -1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bac_Man_t * Psr_ManBuildCba( char * pFileName, Vec_Ptr_t * vDes )
{
    Psr_Ntk_t * pNtk = Psr_ManRoot( vDes );  int i;
    Bac_Man_t * pNew = Bac_ManAlloc( pFileName, Vec_PtrSize(vDes) );
    Vec_Int_t * vMap = Vec_IntStartFull( Abc_NamObjNumMax(pNtk->pStrs) + 1 );
    Vec_Int_t * vTmp = Vec_IntAlloc( Psr_NtkBoxNum(pNtk) );
    Abc_NamDeref( pNew->pStrs );
    pNew->pStrs = Abc_NamRef( pNtk->pStrs );  
    Vec_PtrForEachEntry( Psr_Ntk_t *, vDes, pNtk, i )
        Bac_NtkAlloc( Bac_ManNtk(pNew, i+1), Psr_NtkId(pNtk), Psr_NtkPiNum(pNtk), Psr_NtkPoNum(pNtk), Psr_NtkCountObjects(pNtk) );
    if ( (pNtk->fMapped || (pNtk->fSlices && Psr_ManIsMapped(pNtk))) && !Bac_NtkBuildLibrary(pNew) )
        Bac_ManFree(pNew), pNew = NULL;
    else 
        Vec_PtrForEachEntry( Psr_Ntk_t *, vDes, pNtk, i )
            Psr_ManBuildNtk( Bac_ManNtk(pNew, i+1), vDes, pNtk, vMap, vTmp );
    assert( Vec_IntCountEntry(vMap, -1) == Vec_IntSize(vMap) );
    Vec_IntFree( vMap );
    Vec_IntFree( vTmp );
//    Vec_StrPrint( &Bac_ManNtk(pNew, 1)->vType, 1 );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

