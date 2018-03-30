/**CFile****************************************************************

  FileName    [bacBac.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Verilog parser.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: bacBac.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bac.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Read CBA.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int BacManReadBacLine( Vec_Str_t * vOut, int * pPos, char * pBuffer, char * pLimit )
{
    char c;
    while ( (c = Vec_StrEntry(vOut, (*pPos)++)) != '\n' && pBuffer < pLimit )
        *pBuffer++ = c;
    *pBuffer = 0;
    return pBuffer < pLimit;
}
int BacManReadBacNameAndNums( char * pBuffer, int * Num1, int * Num2, int * Num3, int * Num4 )
{
    *Num1 = *Num2 = *Num3 = *Num4 = -1;
    // read name
    while ( *pBuffer && *pBuffer != ' ' )
        pBuffer++;
    if ( !*pBuffer )
        return 0;
    assert( *pBuffer == ' ' );
    *pBuffer = 0;
    // read Num1
    *Num1 = atoi(++pBuffer);
    while ( *pBuffer && *pBuffer != ' ' )
        pBuffer++;
    if ( !*pBuffer )
        return 0;
    // read Num2
    assert( *pBuffer == ' ' );
    *Num2 = atoi(++pBuffer);
    while ( *pBuffer && *pBuffer != ' ' )
        pBuffer++;
    if ( !*pBuffer )
        return 1;
    // read Num3
    assert( *pBuffer == ' ' );
    *Num3 = atoi(++pBuffer);
    while ( *pBuffer && *pBuffer != ' ' )
        pBuffer++;
    if ( !*pBuffer )
        return 1;
    // read Num4
    assert( *pBuffer == ' ' );
    *Num4 = atoi(++pBuffer);
    return 1;
}
void Bac_ManReadBacVecStr( Vec_Str_t * vOut, int * pPos, Vec_Str_t * p, int nSize )
{
    memcpy( Vec_StrArray(p), Vec_StrArray(vOut) + *pPos, nSize );
    *pPos += nSize;
    p->nSize = nSize;
    assert( Vec_StrSize(p) == Vec_StrCap(p) );
}
void Bac_ManReadBacVecInt( Vec_Str_t * vOut, int * pPos, Vec_Int_t * p, int nSize )
{
    memcpy( Vec_IntArray(p), Vec_StrArray(vOut) + *pPos, nSize );
    *pPos += nSize;
    p->nSize = nSize / 4;
    assert( Vec_IntSize(p) == Vec_IntCap(p) );
}
void Bac_ManReadBacNtk( Vec_Str_t * vOut, int * pPos, Bac_Ntk_t * pNtk )
{
    int i, Type;
    //char * pName; int iObj, NameId;
    Bac_ManReadBacVecStr( vOut, pPos, &pNtk->vType,      Bac_NtkObjNumAlloc(pNtk) );
    Bac_ManReadBacVecInt( vOut, pPos, &pNtk->vFanin, 4 * Bac_NtkObjNumAlloc(pNtk) );
    Bac_ManReadBacVecInt( vOut, pPos, &pNtk->vInfo, 12 * Bac_NtkInfoNumAlloc(pNtk) );
    Bac_NtkForEachObjType( pNtk, Type, i )
    {
        if ( Type == BAC_OBJ_PI )
            Vec_IntPush( &pNtk->vInputs, i );
        if ( Type == BAC_OBJ_PO )
            Vec_IntPush( &pNtk->vOutputs, i );
    }
    assert( Bac_NtkPiNum(pNtk)  == Bac_NtkPiNumAlloc(pNtk) );
    assert( Bac_NtkPoNum(pNtk)  == Bac_NtkPoNumAlloc(pNtk) );
    assert( Bac_NtkObjNum(pNtk) == Bac_NtkObjNumAlloc(pNtk) );
    assert( Bac_NtkInfoNum(pNtk) == Bac_NtkInfoNumAlloc(pNtk) );
/*
    // read input/output/box names
    Bac_NtkForEachPiMain( pNtk, iObj, i )
    {
        pName = Vec_StrEntryP( vOut, Pos );     
        NameId = Abc_NamStrFindOrAdd( p->pStrs, pName, NULL );
        Pos += strlen(pName) + 1;
    }
    Bac_NtkForEachPoMain( pNtk, iObj, i )
    {
        pName = Vec_StrEntryP( vOut, Pos );     
        NameId = Abc_NamStrFindOrAdd( p->pStrs, pName, NULL );
        Pos += strlen(pName) + 1;
    }
    Bac_NtkForEachBox( pNtk, iObj )
    {
        pName = Vec_StrEntryP( vOut, Pos );     
        NameId = Abc_NamStrFindOrAdd( p->pStrs, pName, NULL );
        Pos += strlen(pName) + 1;
    }
*/
}
Bac_Man_t * Bac_ManReadBacInt( Vec_Str_t * vOut )
{
    Bac_Man_t * p;
    Bac_Ntk_t * pNtk;
    char Buffer[1000] = "#"; 
    int i, NameId, Pos = 0, nNtks, Num1, Num2, Num3, Num4;
    while ( Buffer[0] == '#' )
        if ( !BacManReadBacLine(vOut, &Pos, Buffer, Buffer+1000) )
            return NULL;
    if ( !BacManReadBacNameAndNums(Buffer, &nNtks, &Num2, &Num3, &Num4) )
        return NULL;
    // start manager
    assert( nNtks > 0 );
    p = Bac_ManAlloc( Buffer, nNtks );
    // start networks
    Bac_ManForEachNtk( p, pNtk, i )
    {
        if ( !BacManReadBacLine(vOut, &Pos, Buffer, Buffer+1000) )
        {
            Bac_ManFree( p );
            return NULL;
        }
        if ( !BacManReadBacNameAndNums(Buffer, &Num1, &Num2, &Num3, &Num4) )
        {
            Bac_ManFree( p );
            return NULL;
        }
        assert( Num1 >= 0 && Num2 >= 0 && Num3 >= 0 );
        NameId = Abc_NamStrFindOrAdd( p->pStrs, Buffer, NULL );
        Bac_NtkAlloc( pNtk, NameId, Num1, Num2, Num3 );
        Vec_IntFill( &pNtk->vInfo, 3 * Num4, -1 );
    }
    // read networks
    Bac_ManForEachNtk( p, pNtk, i )
        Bac_ManReadBacNtk( vOut, &Pos, pNtk );
    assert( Bac_ManNtkNum(p) == nNtks );
    assert( Pos == Vec_StrSize(vOut) );
    return p;
}
Bac_Man_t * Bac_ManReadBac( char * pFileName )
{
    Bac_Man_t * p;
    FILE * pFile;
    Vec_Str_t * vOut;
    int nFileSize;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return NULL;
    }
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    rewind( pFile ); 
    // load the contents
    vOut = Vec_StrAlloc( nFileSize );
    vOut->nSize = vOut->nCap;
    assert( nFileSize == Vec_StrSize(vOut) );
    nFileSize = fread( Vec_StrArray(vOut), 1, Vec_StrSize(vOut), pFile );
    assert( nFileSize == Vec_StrSize(vOut) );
    fclose( pFile );
    // read the networks
    p = Bac_ManReadBacInt( vOut );
    if ( p != NULL )
    {
        ABC_FREE( p->pSpec );
        p->pSpec = Abc_UtilStrsav( pFileName );
    }
    Vec_StrFree( vOut );
    return p;
}

/**Function*************************************************************

  Synopsis    [Write CBA.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bac_ManWriteBacNtk( Vec_Str_t * vOut, Bac_Ntk_t * pNtk )
{
    //char * pName; int iObj, NameId;
    Vec_StrPushBuffer( vOut, (char *)Vec_StrArray(&pNtk->vType),       Bac_NtkObjNum(pNtk) );
    Vec_StrPushBuffer( vOut, (char *)Vec_IntArray(&pNtk->vFanin),  4 * Bac_NtkObjNum(pNtk) );
    Vec_StrPushBuffer( vOut, (char *)Vec_IntArray(&pNtk->vInfo),  12 * Bac_NtkInfoNum(pNtk) );
/*
    // write input/output/box names
    Bac_NtkForEachPiMain( pNtk, iObj, i )
    {
        pName = Bac_ObjNameStr( pNtk, iObj );
        Vec_StrPrintStr( vOut, pName );
        Vec_StrPush( vOut, '\0' );
    }
    Bac_NtkForEachPoMain( pNtk, iObj, i )
    {
        pName = Bac_ObjNameStr( pNtk, iObj );
        Vec_StrPrintStr( vOut, pName );
        Vec_StrPush( vOut, '\0' );
    }
    Bac_NtkForEachBox( pNtk, iObj )
    {
        pName = Bac_ObjNameStr( pNtk, iObj );  
        Vec_StrPrintStr( vOut, pName );
        Vec_StrPush( vOut, '\0' );
    }
*/
}
void Bac_ManWriteBacInt( Vec_Str_t * vOut, Bac_Man_t * p )
{
    char Buffer[1000];
    Bac_Ntk_t * pNtk; int i;
    sprintf( Buffer, "# Design \"%s\" written by ABC on %s\n", Bac_ManName(p), Extra_TimeStamp() );
    Vec_StrPrintStr( vOut, Buffer );
    // write short info
    sprintf( Buffer, "%s %d \n", Bac_ManName(p), Bac_ManNtkNum(p) );
    Vec_StrPrintStr( vOut, Buffer );
    Bac_ManForEachNtk( p, pNtk, i )
    {
        sprintf( Buffer, "%s %d %d %d %d \n", Bac_NtkName(pNtk), 
            Bac_NtkPiNum(pNtk), Bac_NtkPoNum(pNtk), Bac_NtkObjNum(pNtk), Bac_NtkInfoNum(pNtk) );
        Vec_StrPrintStr( vOut, Buffer );
    }
    Bac_ManForEachNtk( p, pNtk, i )
        Bac_ManWriteBacNtk( vOut, pNtk );
}
void Bac_ManWriteBac( char * pFileName, Bac_Man_t * p )
{
    Vec_Str_t * vOut;
    assert( p->pMioLib == NULL );
    vOut = Vec_StrAlloc( 10000 );
    Bac_ManWriteBacInt( vOut, p );
    if ( Vec_StrSize(vOut) > 0 )
    {
        FILE * pFile = fopen( pFileName, "wb" );
        if ( pFile == NULL )
            printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        else
        {
            fwrite( Vec_StrArray(vOut), 1, Vec_StrSize(vOut), pFile );
            fclose( pFile );
        }
    }
    Vec_StrFree( vOut );    
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

