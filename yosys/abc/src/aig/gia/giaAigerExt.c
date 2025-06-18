/**CFile****************************************************************

  FileName    [giaAigerExt.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Custom AIGER extensions.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaAigerExt.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/st/st.h"
#include "map/mio/mio.h"
#include "map/mio/mioInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Read/write equivalence classes information.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Rpr_t * Gia_AigerReadEquivClasses( unsigned char ** ppPos, int nSize )
{
    Gia_Rpr_t * pReprs;
    unsigned char * pStop;
    int i, Item, fProved, iRepr, iNode;
    pStop = *ppPos;
    pStop += Gia_AigerReadInt( *ppPos ); *ppPos += 4;
    pReprs = ABC_CALLOC( Gia_Rpr_t, nSize );
    for ( i = 0; i < nSize; i++ )
        pReprs[i].iRepr = GIA_VOID;
    iRepr = iNode = 0;
    while ( *ppPos < pStop )
    {
        Item = Gia_AigerReadUnsigned( ppPos );
        if ( Item & 1 )
        {
            iRepr += (Item >> 1);
            iNode = iRepr;
            continue;
        }
        Item >>= 1;
        fProved = (Item & 1);
        Item >>= 1;
        iNode += Item;
        pReprs[iNode].fProved = fProved;
        pReprs[iNode].iRepr = iRepr;
        assert( iRepr < iNode );
    }
    return pReprs;
}
unsigned char * Gia_WriteEquivClassesInt( Gia_Man_t * p, int * pEquivSize )
{
    unsigned char * pBuffer;
    int iRepr, iNode, iPrevRepr, iPrevNode, iLit, nItems, iPos;
    assert( p->pReprs && p->pNexts );
    // count the number of entries to be written
    nItems = 0;
    for ( iRepr = 1; iRepr < Gia_ManObjNum(p); iRepr++ )
    {
        nItems += Gia_ObjIsConst( p, iRepr );
        if ( !Gia_ObjIsHead(p, iRepr) )
            continue;
        Gia_ClassForEachObj( p, iRepr, iNode )
            nItems++;
    }
    pBuffer = ABC_ALLOC( unsigned char, sizeof(int) * (nItems + 10) );
    // write constant class
    iPos = Gia_AigerWriteUnsignedBuffer( pBuffer, 4, Abc_Var2Lit(0, 1) );
    iPrevNode = 0;
    for ( iNode = 1; iNode < Gia_ManObjNum(p); iNode++ )
        if ( Gia_ObjIsConst(p, iNode) )
        {
            iLit = Abc_Var2Lit( iNode - iPrevNode, Gia_ObjProved(p, iNode) );
            iPrevNode = iNode;
            iPos = Gia_AigerWriteUnsignedBuffer( pBuffer, iPos, Abc_Var2Lit(iLit, 0) );
        }
    // write non-constant classes
    iPrevRepr = 0;
    Gia_ManForEachClass( p, iRepr )
    {
        iPos = Gia_AigerWriteUnsignedBuffer( pBuffer, iPos, Abc_Var2Lit(iRepr - iPrevRepr, 1) );
        iPrevRepr = iPrevNode = iRepr;
        Gia_ClassForEachObj1( p, iRepr, iNode )
        {
            iLit = Abc_Var2Lit( iNode - iPrevNode, Gia_ObjProved(p, iNode) );
            iPrevNode = iNode;
            iPos = Gia_AigerWriteUnsignedBuffer( pBuffer, iPos, Abc_Var2Lit(iLit, 0) );
        }
    }
    Gia_AigerWriteInt( pBuffer, iPos );
    *pEquivSize = iPos;
    return pBuffer;
}
Vec_Str_t * Gia_WriteEquivClasses( Gia_Man_t * p )
{
    int nEquivSize;
    unsigned char * pBuffer = Gia_WriteEquivClassesInt( p, &nEquivSize );
    return Vec_StrAllocArray( (char *)pBuffer, nEquivSize );
}

/**Function*************************************************************

  Synopsis    [Read/write mapping information.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Gia_AigerReadDiffValue( unsigned char ** ppPos, int iPrev )
{
    int Item = Gia_AigerReadUnsigned( ppPos );
    if ( Item & 1 )
        return iPrev + (Item >> 1);
    return iPrev - (Item >> 1);
}
int * Gia_AigerReadMapping( unsigned char ** ppPos, int nSize )
{
    int * pMapping;
    unsigned char * pStop;
    int k, j, nFanins, nAlloc, iNode = 0, iOffset = nSize;
    pStop = *ppPos;
    pStop += Gia_AigerReadInt( *ppPos ); *ppPos += 4;
    nAlloc = nSize + pStop - *ppPos;
    pMapping = ABC_CALLOC( int, nAlloc );
    while ( *ppPos < pStop )
    {
        k = iOffset;
        pMapping[k++] = nFanins = Gia_AigerReadUnsigned( ppPos );
        for ( j = 0; j <= nFanins; j++ )
            pMapping[k++] = iNode = Gia_AigerReadDiffValue( ppPos, iNode );
        pMapping[iNode] = iOffset;
        iOffset = k;
    }
    assert( iOffset <= nAlloc );
    return pMapping;
}
static inline int Gia_AigerWriteDiffValue( unsigned char * pPos, int iPos, int iPrev, int iThis )
{
    if ( iPrev < iThis )
        return Gia_AigerWriteUnsignedBuffer( pPos, iPos, Abc_Var2Lit(iThis - iPrev, 1) );
    return Gia_AigerWriteUnsignedBuffer( pPos, iPos, Abc_Var2Lit(iPrev - iThis, 0) );
}
unsigned char * Gia_AigerWriteMappingInt( Gia_Man_t * p, int * pMapSize )
{
    unsigned char * pBuffer;
    int i, k, iPrev, iFan, nItems, iPos = 4;
    assert( Gia_ManHasMapping(p) );
    // count the number of entries to be written
    nItems = 0;
    Gia_ManForEachLut( p, i )
        nItems += 2 + Gia_ObjLutSize( p, i );
    pBuffer = ABC_ALLOC( unsigned char, sizeof(int) * (nItems + 1) );
    // write non-constant classes
    iPrev = 0;
    Gia_ManForEachLut( p, i )
    {
//printf( "\nSize = %d ", Gia_ObjLutSize(p, i) );
        iPos = Gia_AigerWriteUnsignedBuffer( pBuffer, iPos, Gia_ObjLutSize(p, i) );
        Gia_LutForEachFanin( p, i, iFan, k )
        {
//printf( "Fan = %d ", iFan );
            iPos = Gia_AigerWriteDiffValue( pBuffer, iPos, iPrev, iFan );
            iPrev = iFan;
        }
        iPos = Gia_AigerWriteDiffValue( pBuffer, iPos, iPrev, i );
        iPrev = i;
//printf( "Node = %d ", i );
    }
//printf( "\n" );
    Gia_AigerWriteInt( pBuffer, iPos );
    *pMapSize = iPos;
    return pBuffer;
}
Vec_Str_t * Gia_AigerWriteMapping( Gia_Man_t * p )
{
    int nMapSize;
    unsigned char * pBuffer = Gia_AigerWriteMappingInt( p, &nMapSize );
    return Vec_StrAllocArray( (char *)pBuffer, nMapSize );
}

/**Function*************************************************************

  Synopsis    [Read/write mapping information.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Gia_AigerReadMappingSimple( unsigned char ** ppPos, int nSize )
{
    int * pMapping = ABC_ALLOC( int, (size_t)nSize/4 );
    memcpy( pMapping, *ppPos, nSize );
    assert( nSize % 4 == 0 );
    return pMapping;
}
Vec_Str_t * Gia_AigerWriteMappingSimple( Gia_Man_t * p )
{
    unsigned char * pBuffer = ABC_ALLOC( unsigned char, 4*Vec_IntSize(p->vMapping) );
    memcpy( pBuffer, Vec_IntArray(p->vMapping), (size_t)4*Vec_IntSize(p->vMapping) );
    assert( Vec_IntSize(p->vMapping) >= Gia_ManObjNum(p) );
    return Vec_StrAllocArray( (char *)pBuffer, 4*Vec_IntSize(p->vMapping) );
}

/**Function*************************************************************

  Synopsis    [Read/write mapping information.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_AigerReadMappingDoc( unsigned char ** ppPos, int nObjs )
{
    int * pMapping, nLuts, LutSize, iRoot, nFanins, i, k, nOffset;
    nLuts    = Gia_AigerReadInt( *ppPos ); *ppPos += 4;
    LutSize  = Gia_AigerReadInt( *ppPos ); *ppPos += 4;
    pMapping = ABC_CALLOC( int, nObjs + (LutSize + 2) * nLuts );
    nOffset = nObjs;
    for ( i = 0; i < nLuts; i++ )
    {
        iRoot   = Gia_AigerReadInt( *ppPos ); *ppPos += 4;
        nFanins = Gia_AigerReadInt( *ppPos ); *ppPos += 4;
        pMapping[iRoot] = nOffset;
        // write one
        pMapping[ nOffset++ ] = nFanins; 
        for ( k = 0; k < nFanins; k++ )
        {
            pMapping[ nOffset++ ] = Gia_AigerReadInt( *ppPos ); *ppPos += 4;
        }
        pMapping[ nOffset++ ] = iRoot; 
    }
    return Vec_IntAllocArray( pMapping, nOffset );
}
Vec_Str_t * Gia_AigerWriteMappingDoc( Gia_Man_t * p )
{
    unsigned char * pBuffer;
    int i, k, iFan, nLuts = 0, LutSize = 0, nSize = 2, nSize2 = 0;
    Gia_ManForEachLut( p, i )
    {
        nLuts++;
        nSize += Gia_ObjLutSize(p, i) + 2;
        LutSize = Abc_MaxInt( LutSize, Gia_ObjLutSize(p, i) );
    }
    pBuffer = ABC_ALLOC( unsigned char, 4 * nSize );
    Gia_AigerWriteInt( pBuffer + 4 * nSize2++, nLuts );  
    Gia_AigerWriteInt( pBuffer + 4 * nSize2++, LutSize );
    Gia_ManForEachLut( p, i )
    {
        Gia_AigerWriteInt( pBuffer + 4 * nSize2++, i );
        Gia_AigerWriteInt( pBuffer + 4 * nSize2++, Gia_ObjLutSize(p, i) );
        Gia_LutForEachFanin( p, i, iFan, k )
            Gia_AigerWriteInt( pBuffer + 4 * nSize2++, iFan );
    }
    assert( nSize2 == nSize );
    return Vec_StrAllocArray( (char *)pBuffer, 4*nSize );
}

int Gia_AigerWriteCellMappingInstance( Gia_Man_t * p, unsigned char * pBuffer, int nSize2, int i )
{
    int k, iFan;
    if ( !Gia_ObjIsCellInv(p, i) ) {
        Gia_AigerWriteInt( pBuffer + nSize2, Gia_ObjCellId(p, i) ); nSize2 += 4;
        Gia_AigerWriteInt( pBuffer + nSize2, i ); nSize2 += 4;
        Gia_CellForEachFanin( p, i, iFan, k )
        {
            Gia_AigerWriteInt( pBuffer + nSize2, iFan );
            nSize2 += 4;
        }
    } else {
        Gia_AigerWriteInt( pBuffer + nSize2, 3 ); nSize2 += 4;
        Gia_AigerWriteInt( pBuffer + nSize2, i ); nSize2 += 4;
        Gia_AigerWriteInt( pBuffer + nSize2, Abc_LitNot(i) ); nSize2 += 4;
    }

    return nSize2;
}

Vec_Str_t * Gia_AigerWriteCellMappingDoc( Gia_Man_t * p )
{
    unsigned char * pBuffer;
    int i, nCells = 0, nInstances = 0, nSize = 8, nSize2 = 0;
    Mio_Cell2_t * pCells = Mio_CollectRootsNewDefault2( 6, &nCells, 0 );
    assert( pCells );

    for (int i = 0; i < nCells; i++)
    {
        Mio_Gate_t *pGate = (Mio_Gate_t *) pCells[i].pMioGate;
        Mio_Pin_t *pPin;
        nSize += strlen(Mio_GateReadName(pGate)) + 1;
        nSize += strlen(Mio_GateReadOutName(pGate)) + 1 + 4;
        Mio_GateForEachPin( pGate, pPin )
            nSize += strlen(Mio_PinReadName(pPin)) + 1;
    }

    Gia_ManForEachCell( p, i )
    {
        assert ( !Gia_ObjIsCellBuf(p, i) ); // not implemented
        nInstances++;
        if ( Gia_ObjIsCellInv(p, i) )
            nSize += 12;
        else
            nSize += Gia_ObjCellSize(p, i) * 4 + 8;
    }

    pBuffer = ABC_ALLOC( unsigned char, nSize );
    Gia_AigerWriteInt( pBuffer + nSize2, nCells ); nSize2 += 4;
    Gia_AigerWriteInt( pBuffer + nSize2, nInstances ); nSize2 += 4;

    for (int i = 0; i < nCells; i++)
    {
        int nPins = 0;
        Mio_Gate_t *pGate = (Mio_Gate_t *) pCells[i].pMioGate;
        Mio_Pin_t *pPin;

        strcpy((char *) pBuffer + nSize2, Mio_GateReadName(pGate));
        nSize2 += strlen(Mio_GateReadName(pGate)) + 1;
        strcpy((char *) pBuffer + nSize2, Mio_GateReadOutName(pGate));
        nSize2 += strlen(Mio_GateReadOutName(pGate)) + 1;

        Mio_GateForEachPin( pGate, pPin )
            nPins++;
        Gia_AigerWriteInt( pBuffer + nSize2, nPins ); nSize2 += 4;

        Mio_GateForEachPin( pGate, pPin )
        {
            strcpy((char *) pBuffer + nSize2, Mio_PinReadName(pPin));
            nSize2 += strlen(Mio_PinReadName(pPin)) + 1;
        }
    }

    Gia_ManForEachCell( p, i )
    {
        if ( Gia_ObjIsCellBuf(p, i) )
            continue;

        if ( Gia_ObjIsCellInv(p, i) && !Abc_LitIsCompl(i) ) {
            // swap the order so that the inverter is after the driver
            // of the inverter's input
            nSize2 = Gia_AigerWriteCellMappingInstance(p, pBuffer, nSize2, Abc_LitNot(i) );
            nSize2 = Gia_AigerWriteCellMappingInstance(p, pBuffer, nSize2, i );
            i += 1;
            continue;
        }

        nSize2 = Gia_AigerWriteCellMappingInstance(p, pBuffer, nSize2, i );
    }

    assert( nSize2 == nSize );
    ABC_FREE( pCells );
    return Vec_StrAllocArray( (char *)pBuffer, nSize );
}

/**Function*************************************************************

  Synopsis    [Read/write packing information.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_AigerReadPacking( unsigned char ** ppPos, int nSize )
{
    Vec_Int_t * vPacking = Vec_IntAlloc( nSize/4 );
    int i;
    assert( nSize % 4 == 0 );
    for ( i = 0; i < nSize/4; i++, *ppPos += 4 )
        Vec_IntPush( vPacking, Gia_AigerReadInt( *ppPos ) );
    return vPacking;
}
Vec_Str_t * Gia_WritePacking( Vec_Int_t * vPacking )
{
    unsigned char * pBuffer = ABC_ALLOC( unsigned char, 4*Vec_IntSize(vPacking) );
    int i, Entry, nSize = 0;
    Vec_IntForEachEntry( vPacking, Entry, i )
        Gia_AigerWriteInt( pBuffer + 4 * nSize++, Entry );
    return Vec_StrAllocArray( (char *)pBuffer, 4*Vec_IntSize(vPacking) );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

