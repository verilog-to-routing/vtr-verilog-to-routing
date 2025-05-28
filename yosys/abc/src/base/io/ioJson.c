/**CFile****************************************************************

  FileName    [ioJson.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to read JSON.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioJson.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"
#include "misc/vec/vecWec.h"
#include "misc/util/utilNam.h"
#include "misc/extra/extra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int         Json_EntryIsName( int Fan )                     { return Abc_LitIsCompl(Fan);                                                       }
static inline char *      Json_EntryName( Abc_Nam_t * pStrs, int Fan )    { assert(Json_EntryIsName(Fan));  return Abc_NamStr( pStrs, Abc_Lit2Var(Fan) );     }
static inline Vec_Int_t * Json_EntryNode( Vec_Wec_t * vObjs, int Fan )    { assert(!Json_EntryIsName(Fan)); return Vec_WecEntry( vObjs, Abc_Lit2Var(Fan) );   }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writes JSON into a file.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Nnc_LayerType2Str( char * pStr )
{
    if ( !strcmp(pStr, "InputLayer") )
        return "input  ";
    if ( !strcmp(pStr, "Conv2D") )
        return "convo  ";
    if ( !strcmp(pStr, "BatchNormalization") )
        return "batch  ";
    if ( !strcmp(pStr, "Activation") )
        return "relu   ";
    if ( !strcmp(pStr, "Add") )
        return "eltwise";
    if ( !strcmp(pStr, "MaxPooling2D") )
        return "pool   ";
    if ( !strcmp(pStr, "GlobalAveragePooling2D") )
        return "pool   ";
    if ( !strcmp(pStr, "Dense") )
        return "fullcon";
    if ( !strcmp(pStr, "ZeroPadding2D") )
        return "pad";
//    if ( !strcmp(pStr, "InputLayer") )
//        return "softmax";
    return NULL;
}

void Json_Extract_rec( FILE * pFile, Abc_Nam_t * pStr, Vec_Wec_t * vObjs, Vec_Int_t * vArray, int fWrite, int * pCount )
{
    int i, Entry1, Entry2;
    if ( Vec_IntEntry(vArray, 0) ) // array
    {
        if ( Vec_IntSize(vArray) == 1 )
            return;
        if ( Vec_IntSize(vArray) == 2 && Json_EntryIsName(Vec_IntEntry(vArray,1)) )
        {
            if ( fWrite )
                fprintf( pFile, "%s", Json_EntryName(pStr, Vec_IntEntry(vArray,1)) );
            return;
        }
        else
        {
            Vec_IntForEachEntryStart( vArray, Entry1, i, 1 )
            {
                if ( Json_EntryIsName(Entry1) )
                {
                    int Digit = Json_EntryName(pStr, Entry1)[0];
                    if ( fWrite && Digit != '0' )
                        fprintf( pFile, "%s%s", Json_EntryName(pStr, Entry1), Digit >= '0' && Digit <= '9' ? "" : " " );
                }
                else
                    Json_Extract_rec( pFile, pStr, vObjs, Json_EntryNode(vObjs, Entry1), fWrite, pCount );
            }
            return;
        }
    }
    else // list of pairs
    {
        int fHaveConfig = 0;
        assert( Vec_IntSize(vArray) % 2 != 0 );
        Vec_IntForEachEntryDoubleStart( vArray, Entry1, Entry2, i, 1 )
        {
            char * pName1 = Json_EntryIsName(Entry1) ? Json_EntryName(pStr, Entry1) : NULL;
            char * pName2 = Json_EntryIsName(Entry2) ? Json_EntryName(pStr, Entry2) : NULL;
            char * pName3 = pName2 ? Nnc_LayerType2Str(pName2) : NULL;
            if ( pName1 == NULL )
                continue;
            if ( !strcmp(pName1, "class_name") )
            {
                if ( pName3 )
                    fprintf( pFile, "\n%3d : %-8s ", (*pCount)++, pName3 );
            }
            else if ( !strcmp(pName1, "name") )
            {
                if ( fHaveConfig )
                    fprintf( pFile, " N=%s  ", pName2 ? pName2 : "???" );
            }
            else if ( !strcmp(pName1, "kernel_size") )
            {
                fprintf( pFile, " K=" );
                Json_Extract_rec( pFile, pStr, vObjs, Json_EntryNode(vObjs, Entry2), 1, pCount );
            }
            else if ( !strcmp(pName1, "strides") )
            {
                fprintf( pFile, " S=" );
                Json_Extract_rec( pFile, pStr, vObjs, Json_EntryNode(vObjs, Entry2), 1, pCount );
            }
            else if ( !strcmp(pName1, "filters") )
                fprintf( pFile, " C=%s", pName2 );
            else if ( !strcmp(pName1, "inbound_nodes") )
                Json_Extract_rec( pFile, pStr, vObjs, Json_EntryNode(vObjs, Entry2), 1, pCount );
            else if ( !strcmp(pName1, "layers") )
                Json_Extract_rec( pFile, pStr, vObjs, Json_EntryNode(vObjs, Entry2), 1, pCount );
            else if ( !strcmp(pName1, "config") )
            {
                fHaveConfig = 1;
                Json_Extract_rec( pFile, pStr, vObjs, Json_EntryNode(vObjs, Entry2), 0, pCount );
            }
        }
    }
}
void Json_Extract( char * pFileName, Abc_Nam_t * pStr, Vec_Wec_t * vObjs )
{
    int Count = 0;
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return;
    }
    fprintf( pFile, "# Data extracted from JSON file:\n" );
    Json_Extract_rec( pFile, pStr, vObjs, Vec_WecEntry(vObjs, 0), 0, &Count );
    fprintf( pFile, "\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Parsing.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Json_CharIsSpace( char c )   
{ 
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ':');   
}
static inline char * Json_SkipSpaces( char * pCur )
{
    while ( Json_CharIsSpace(*pCur) )
        pCur++;    
    return pCur;
}
static inline char * Json_SkipNonSpaces( char * pCur )
{
    while ( !Json_CharIsSpace(*pCur) )
        pCur++;    
    return pCur;
}
static inline int Json_TokenCompare( char * pCur, char * pNext, char ** ppCur2, char ** ppNext2 )
{
//    int i;
    if ( *pCur == '\"' )
        pCur++;
    if ( *(pNext-1) == ',' )
        pNext--;
    if ( *(pNext-1) == '\"' )
        pNext--;
    *ppCur2 = pCur;
    *ppNext2 = pNext;
//    for ( i = 1; i < JSON_NUM_LINES; i++ )
//        if ( !strncmp( s_Types[i].pName, pCur, pNext - pCur ) )
//            return i;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Writes JSON into a file.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Json_Write_rec( FILE * pFile, Abc_Nam_t * pStr, Vec_Wec_t * vObjs, Vec_Int_t * vArray, int Level, int fAddComma, int fSpaces )
{
    int i, Entry1, Entry2, fComma;
    if ( Vec_IntEntry(vArray, 0) ) // array
    {
        if ( Vec_IntSize(vArray) == 1 )
            fprintf( pFile, "[]" );
        else if ( Vec_IntSize(vArray) == 2 && Json_EntryIsName(Vec_IntEntry(vArray,1)) )
            fprintf( pFile, "[ \"%s\" ]", Json_EntryName(pStr, Vec_IntEntry(vArray,1)) );
        else
        {
            if ( fSpaces )
                fprintf( pFile, "%*s", 3*(Level-1), "" );
            fprintf( pFile, "[\n" );
            Vec_IntForEachEntryStart( vArray, Entry1, i, 1 )
            {
                fComma = (i < Vec_IntSize(vArray) - 1);
                if ( Json_EntryIsName(Entry1) )
                    fprintf( pFile, "%*s\"%s\"%s\n", 3*Level, "", Json_EntryName(pStr, Entry1), fComma ? ",":"" );
                else
                    Json_Write_rec( pFile, pStr, vObjs, Json_EntryNode(vObjs, Entry1), Level+1, fComma, 1 );
            }
            fprintf( pFile, "%*s]", 3*(Level-1), "" );
        }
        fprintf( pFile, "%s\n", fAddComma ? ",":"" );
    }
    else // list of pairs
    {
        if ( fSpaces )
            fprintf( pFile, "%*s", 3*(Level-1), "" );
        fprintf( pFile, "{\n" );
        assert( Vec_IntSize(vArray) % 2 != 0 );
        Vec_IntForEachEntryDoubleStart( vArray, Entry1, Entry2, i, 1 )
        {
            fComma = (i < Vec_IntSize(vArray) - 3);
            if ( Json_EntryIsName(Entry1) )
                fprintf( pFile, "%*s\"%s\"", 3*Level, "", Json_EntryName(pStr, Entry1) );
            else
                Json_Write_rec( pFile, pStr, vObjs, Json_EntryNode(vObjs, Entry1), Level+1, 0, 1 );
            fprintf( pFile, " : " );
            if ( Json_EntryIsName(Entry2) )
                fprintf( pFile, "\"%s\"%s\n", Json_EntryName(pStr, Entry2), fComma ? ",":"" );
            else
                Json_Write_rec( pFile, pStr, vObjs, Json_EntryNode(vObjs, Entry2), Level+1, fComma, 0 );
        }
        fprintf( pFile, "%*s}%s\n", 3*(Level-1), "", fAddComma ? ",":"" );
    }
}
void Json_Write( char * pFileName, Abc_Nam_t * pStr, Vec_Wec_t * vObjs )
{
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return;
    }
    Json_Write_rec( pFile, pStr, vObjs, Vec_WecEntry(vObjs, 0), 1, 0, 1 );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Reads JSON from a file.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Json_ReadPreprocess( char * pIn, int nFileSize )
{
    char * pOut = ABC_ALLOC( char, 3*nFileSize ); int i, k = 0;
    for ( i = 0; i < nFileSize; i++ )
        if ( pIn[i] == '{' || pIn[i] == '}' || pIn[i] == '[' || pIn[i] == ']' )
        {
            pOut[k++] = ' ';
            pOut[k++] = pIn[i];
            pOut[k++] = ' ';
        }
        else
            pOut[k++] = pIn[i];
    pOut[k++] = '\0';
    return pOut;
}
Vec_Wec_t * Json_Read( char * pFileName, Abc_Nam_t ** ppStrs )
{
    Abc_Nam_t * pStrs; 
    Vec_Wec_t * vObjs; 
    Vec_Int_t * vStack, * vTemp;
    char * pContents, * pCur, * pNext, * pCur2, * pNext2;
    int nFileSize, RetValue, iToken;
    FILE * pFile;

    // read the file into the buffer
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return NULL;
    }
    nFileSize = Extra_FileSize( pFileName );
    pContents = pCur = ABC_ALLOC( char, nFileSize+1 );
    RetValue = fread( pContents, nFileSize, 1, pFile );
    pContents[nFileSize] = 0;
    fclose( pFile );

    pContents = Json_ReadPreprocess( pCur = pContents, nFileSize );
    nFileSize = strlen(pContents);
    ABC_FREE( pCur );
    pCur = pContents;

    // start data-structures
    vObjs  = Vec_WecAlloc( 1000 );
    vStack = Vec_IntAlloc( 100 );
    pStrs  = Abc_NamStart( 1000, 24 );
    //Json_AddTypes( pStrs );

    // read lines
    assert( Vec_WecSize(vObjs) == 0 );
    assert( Vec_IntSize(vStack) == 0 );
    while ( pCur < pContents + nFileSize )
    {
        pCur  = Json_SkipSpaces( pCur );
        if ( *pCur == '\0' )
            break;
        pNext = Json_SkipNonSpaces( pCur );
        if ( *pCur == '{' || *pCur == '[' )
        {
            // add fanin to node on the previous level
            if ( Vec_IntSize(vStack) > 0 )
                Vec_IntPush( Vec_WecEntry(vObjs, Vec_IntEntryLast(vStack)), Abc_Var2Lit(Vec_WecSize(vObjs), 0) );            
            // add node to the stack
            Vec_IntPush( vStack, Vec_WecSize(vObjs) );
            vTemp = Vec_WecPushLevel( vObjs );
            Vec_IntGrow( vTemp, 4 );
            // remember it as an array
            Vec_IntPush( vTemp, (int)(*pCur == '[') );
            pCur++;
            continue;
        }
        if ( *pCur == '}' || *pCur == ']' )
        {
            Vec_IntPop( vStack );
            pCur++;
            continue;
        }
        if ( *pCur == ',' || *pCur == ':' )
        {
            pCur++;
            continue;
        }
        iToken = Json_TokenCompare( pCur, pNext, &pCur2, &pNext2 );
        if ( iToken == 0 )
            iToken = Abc_NamStrFindOrAddLim( pStrs, pCur2, pNext2, NULL );
        Vec_IntPush( Vec_WecEntry(vObjs, Vec_IntEntryLast(vStack)), Abc_Var2Lit(iToken, 1) );
        pCur = pNext;
    }
    Vec_IntFree( vStack );
    ABC_FREE( pContents );
    *ppStrs = pStrs;
    return vObjs;
}

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Json_ReadTest( char * pFileName )
{
    Abc_Nam_t * pStrs;
    Vec_Wec_t * vObjs;
    vObjs = Json_Read( pFileName, &pStrs );
    if ( vObjs == NULL )
        return;
    Json_Write( "test.json", pStrs, vObjs );
    Abc_NamDeref( pStrs );
    Vec_WecFree( vObjs );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

