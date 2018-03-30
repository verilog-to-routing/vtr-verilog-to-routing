/**CFile****************************************************************

  FileName    [ndr.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Format for word-level design representation.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: ndr.h,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__base__ndr__ndr_h
#define ABC__base__ndr__ndr_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "abcOper.h"

ABC_NAMESPACE_HEADER_START 

#ifdef _WIN32
#define inline __inline
#endif

/*
    For the lack of a better name, this format is called New Data Representation (NDR).

    NDR is based on the following principles:
    - complex data is composed of individual records
    - a record has one of several known types (module, name, range, fanins, etc)
    - a record can be atomic, for example, a name or an operator type
    - a record can be composed of other records (for example, a module is composed of objects, etc)
    - the stored data should be easy to write into and read from a file, or pass around as a memory buffer
    - the format should be simple, easy to use, low-memory, and extensible
    - new record types can be added by the user as needed

    The implementation is based on the following ideas:
    - a record is composed of two parts (the header followed by the body)
    - the header contains two items (the record type and the body size, measured in terms of 4-byte integers)
    - the body contains as many entries as stated in the record size
    - if a record is composed of other records, its body contains these records

    As an example, consider a name. It can be a module name, an object name, or a net name.
    A record storing one name has a header {NDR_NAME, 1} containing record type (NDR_NAME) and size (1),
    The body of the record is composed of one unsigned integer representing the name (say, 357).  
    So the complete record looks as follows:  { <header>, <body> } = { {NDR_NAME, 1}, {357} }.

    As another example, consider a two-input AND-gate.  In this case, the recent is composed 
    of a header {NDR_OBJECT, 4}  containing record type (NDR_OBJECT) and the body size (4), followed 
    by an array of records creating the AND-gate:  (a) name, (b) operation type, (c) fanins.   
    The complete record  looks as follows: {  {NDR_OBJECT, 5}, {{{NDR_NAME, 1}, 357}, {{NDR_OPERTYPE, 1}, ABC_OPER_LOGIC_AND}, 
    {{NDR_INPUT, 2}, {<id_fanin1>, <id_fanin2>}}} }.   Please note that only body entries are counted towards size. 
    In the case of one name, there is only one body entry.  In the case of the AND-gate, there are 4 body entries 
    (name ID, gate type, first fanin, second fanin).

    Headers and bodies of all objects are stored differently.  Headers are stored in an array of unsigned chars, 
    while bodies are stored in the array of 4-byte unsigned integers.  This is important for memory efficiency. 
    However, the user does not see these details.

    To estimate memory usage, we can assume that each header takes 1 byte and each body entry contains 4 bytes.   
    A name takes 5 bytes, and an AND-gate takes 1 * NumHeaders + 4 * NumBodyEntries = 1 * 4 + 4 * 4 = 20 bytes.  
    Not bad.  The same as memory usage in a well-designed AIG package with structural hashing. 

    Comments:
    - it is assumed that all port names, net names, and instance names are hashed into 1-based integer numbers called name IDs
    - nets are not explicitly represented but their name ID are used to establish connectivity between the objects
    - primary input and primary output objects have to be explicitly created (as shown in the example below)
    - object inputs are name IDs of the driving nets; object outputs are name IDs of the driven nets
    - objects can be added to a module in any order
    - if the ordering of inputs/outputs/flops of a module is not provided as a separate record,
      their ordering is determined by the order of their appearance of their records in the body of the module
    - if range limits and signedness are all 0, it is assumed that it is a Boolean object
    - if left limit and right limit of a range are equal, it is assumed that the range contains one bit
    - instances of known operators can have types defined by Wlc_ObjType_t below
    - instances of user modules have type equal to the name ID of the module plus 1000
    - initial states of the flops are given as char-strings containing 0, 1, and 'x' 
      (for example, "4'b10XX" is an init state of a 4-bit flop with bit-level init states const1, const0, unknown, unknown)
    - word-level constants are represented as char-strings given in the same way as they would appear in a Verilog file 
      (for example, the 16-bit constant 10 is represented as a string "4'b1010". This string contains  8 bytes, 
      including the char '\0' to denote the end of the string. It will take 2 unsigned ints, therefore 
      its record will look as follows { {NDR_FUNCTION, 2}, {"4'b1010"} }, but the user does not see these details.  
      The user only gives  "4'b1010" as an argument (char * pFunction) to the above procedure Ndr_AddObject(). 
*/

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// record types
typedef enum {
    NDR_NONE = 0,          // 0:  unused
    NDR_DESIGN,            // 1:  design (or library of modules)
    NDR_MODULE,            // 2:  one module
    NDR_OBJECT,            // 3:  object
    NDR_INPUT,             // 4:  input
    NDR_OUTPUT,            // 5:  output
    NDR_OPERTYPE,          // 6:  operator type (buffer, shifter, adder, etc)
    NDR_NAME,              // 7:  name
    NDR_RANGE,             // 8:  bit range
    NDR_FUNCTION,          // 9:  specified for some operators (PLAs, etc)
    NDR_TARGET,            // 10: target
    NDR_UNKNOWN            // 11: unknown
} Ndr_RecordType_t; 


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// this is an internal procedure, which is not seen by the user
typedef struct Ndr_Data_t_  Ndr_Data_t;
struct Ndr_Data_t_ 
{
    int                     nSize;
    int                     nCap;
    unsigned char *         pHead;
    unsigned int *          pBody;
};

static inline int           Ndr_DataType( Ndr_Data_t * p, int i )                { assert( p->pHead[i] ); return (int)p->pHead[i];                }
static inline int           Ndr_DataSize( Ndr_Data_t * p, int i )                { return Ndr_DataType(p, i) > NDR_OBJECT ? 1 : p->pBody[i];      }
static inline int           Ndr_DataEntry( Ndr_Data_t * p, int i )               { return (int)p->pBody[i];                                       }
static inline int *         Ndr_DataEntryP( Ndr_Data_t * p, int i )              { return (int *)p->pBody + i;                                    }
static inline int           Ndr_DataEnd( Ndr_Data_t * p, int i )                 { return i + p->pBody[i];                                        }
static inline void          Ndr_DataAddTo( Ndr_Data_t * p, int i, int Add )      { assert(Ndr_DataType(p, i) <= NDR_OBJECT); p->pBody[i] += Add;  } 
static inline void          Ndr_DataPush( Ndr_Data_t * p, int Type, int Entry )  { p->pHead[p->nSize] = Type; p->pBody[p->nSize++] = Entry;       }

////////////////////////////////////////////////////////////////////////
///                          ITERATORS                               ///
////////////////////////////////////////////////////////////////////////

// iterates over modules in the design
#define Ndr_DesForEachMod( p, Mod )                                   \
    for ( Mod = 1; Mod < Ndr_DataEntry(p, 0); Mod += Ndr_DataSize(p, Mod) ) if (Ndr_DataType(p, Mod) != NDR_MODULE) {} else

// iterates over objects in a module
#define Ndr_ModForEachObj( p, Mod, Obj )                              \
    for ( Obj = Mod + 1; Obj < Ndr_DataEnd(p, Mod); Obj += Ndr_DataSize(p, Obj) ) if (Ndr_DataType(p, Obj) != NDR_OBJECT) {} else

// iterates over records in an object
#define Ndr_ObjForEachEntry( p, Obj, Ent )                            \
    for ( Ent = Obj + 1; Ent < Ndr_DataEnd(p, Obj); Ent += Ndr_DataSize(p, Ent) )

// iterates over primary inputs of a module
#define Ndr_ModForEachPi( p, Mod, Obj )                               \
    Ndr_ModForEachObj( p, Mod, Obj ) if ( !Ndr_ObjIsType(p, Obj, ABC_OPER_CI) ) {} else

// iteraots over primary outputs of a module
#define Ndr_ModForEachPo( p, Mod, Obj )                               \
    Ndr_ModForEachObj( p, Mod, Obj ) if ( !Ndr_ObjIsType(p, Obj, ABC_OPER_CO) ) {} else

// iterates over internal nodes of a module
#define Ndr_ModForEachNode( p, Mod, Obj )                             \
    Ndr_ModForEachObj( p, Mod, Obj ) if ( Ndr_ObjIsType(p, Obj, ABC_OPER_CI) || Ndr_ObjIsType(p, Obj, ABC_OPER_CO) ) {} else

// iterates over target signals of a module
#define Ndr_ModForEachTarget( p, Mod, Obj )                           \
    for ( Obj = Mod + 1; Obj < Ndr_DataEnd(p, Mod); Obj += Ndr_DataSize(p, Obj) ) if (Ndr_DataType(p, Obj) != NDR_TARGET) {} else

////////////////////////////////////////////////////////////////////////
///                    INTERNAL PROCEDURES                           ///
////////////////////////////////////////////////////////////////////////


static inline void Ndr_DataResize( Ndr_Data_t * p, int Add )
{
    if ( p->nSize + Add <= p->nCap )
        return;
    p->nCap  = Abc_MaxInt( 2 * p->nCap, p->nSize + Add );
    p->pHead = (unsigned char*)realloc( p->pHead,   p->nCap );
    p->pBody = (unsigned int *)realloc( p->pBody, 4*p->nCap );
}
static inline void Ndr_DataPushRange( Ndr_Data_t * p, int RangeLeft, int RangeRight, int fSignedness )
{ 
    if ( fSignedness )
    {
        Ndr_DataPush( p, NDR_RANGE, RangeLeft );
        Ndr_DataPush( p, NDR_RANGE, RangeRight );
        Ndr_DataPush( p, NDR_RANGE, fSignedness );
        return;
    }
    if ( !RangeLeft && !RangeRight )
        return;
    if ( RangeLeft == RangeRight )
        Ndr_DataPush( p, NDR_RANGE, RangeLeft );
    else
    {
        Ndr_DataPush( p, NDR_RANGE, RangeLeft );
        Ndr_DataPush( p, NDR_RANGE, RangeRight );
    }
}
static inline void Ndr_DataPushArray( Ndr_Data_t * p, int Type, int nArray, int * pArray )
{ 
    if ( !nArray )
        return;
    assert( nArray > 0 );
    Ndr_DataResize( p, nArray );
    memset( p->pHead + p->nSize, Type, nArray );
    memcpy( p->pBody + p->nSize, pArray, 4*nArray );
    p->nSize += nArray;
}
static inline void Ndr_DataPushString( Ndr_Data_t * p, int Type, char * pFunc )
{ 
    if ( !pFunc )
        return;
    Ndr_DataPushArray( p, Type, ((int)strlen(pFunc) + 4) / 4, (int *)pFunc );
}

////////////////////////////////////////////////////////////////////////
///                     VERILOG WRITING                              ///
////////////////////////////////////////////////////////////////////////

static inline int Ndr_ObjReadEntry( Ndr_Data_t * p, int Obj, int Type )
{
    int Ent;
    Ndr_ObjForEachEntry( p, Obj, Ent )
        if ( Ndr_DataType(p, Ent) == Type )
            return Ndr_DataEntry(p, Ent);
    return -1;
}
static inline int Ndr_ObjReadArray( Ndr_Data_t * p, int Obj, int Type, int ** ppStart )
{
    int Ent, Counter = 0; *ppStart = NULL;
    Ndr_ObjForEachEntry( p, Obj, Ent )
        if ( Ndr_DataType(p, Ent) == Type )
        {
            Counter++;
            if ( *ppStart == NULL )
                *ppStart = (int *)p->pBody + Ent;
        }
        else if ( *ppStart )
            return Counter;
    return Counter;
} 
static inline int Ndr_ObjIsType( Ndr_Data_t * p, int Obj, int Type )
{
    int Ent;
    Ndr_ObjForEachEntry( p, Obj, Ent )
        if ( Ndr_DataType(p, Ent) == NDR_OPERTYPE )
            return (int)(Ndr_DataEntry(p, Ent) == Type);
    return -1;
}
static inline int Ndr_ObjReadBody( Ndr_Data_t * p, int Obj, int Type )
{
    int Ent;
    Ndr_ObjForEachEntry( p, Obj, Ent )
        if ( Ndr_DataType(p, Ent) == Type )
            return Ndr_DataEntry(p, Ent);
    return -1;
}
static inline int * Ndr_ObjReadBodyP( Ndr_Data_t * p, int Obj, int Type )
{
    int Ent;
    Ndr_ObjForEachEntry( p, Obj, Ent )
        if ( Ndr_DataType(p, Ent) == Type )
            return Ndr_DataEntryP(p, Ent);
    return NULL;
}
static inline void Ndr_ObjWriteRange( Ndr_Data_t * p, int Obj, FILE * pFile, int fSkipBin )
{
    int * pArray, nArray = Ndr_ObjReadArray( p, Obj, NDR_RANGE, &pArray );
    if ( (nArray == 0 || nArray == 1) && fSkipBin )
        return;
    if ( nArray == 3 && fSkipBin )
        fprintf( pFile, "signed " ); 
    else if ( nArray == 1 )
    {
        if ( fSkipBin )
            fprintf( pFile, "[%d:%d]", pArray[0], pArray[0] );
        else
            fprintf( pFile, "[%d]", pArray[0] );
    }
    else if ( nArray == 0 )
    {
        if ( fSkipBin )
            fprintf( pFile, "[%d:%d]", 0, 0 );
        else
            fprintf( pFile, "[%d]", 0 );
    }
    else
        fprintf( pFile, "[%d:%d]", pArray[0], pArray[1] );
}
static inline char * Ndr_ObjReadOutName( Ndr_Data_t * p, int Obj, char ** pNames )
{
    return pNames[Ndr_ObjReadBody(p, Obj, NDR_OUTPUT)];
}
static inline char * Ndr_ObjReadInName( Ndr_Data_t * p, int Obj, char ** pNames )
{
    return pNames[Ndr_ObjReadBody(p, Obj, NDR_INPUT)];
}

static inline int Ndr_DataCiNum( Ndr_Data_t * p, int Mod )
{ 
    int Obj, Count = 0;
    Ndr_ModForEachPi( p, Mod, Obj )
        Count++;
    return Count;
}
static inline int Ndr_DataCoNum( Ndr_Data_t * p, int Mod )
{ 
    int Obj, Count = 0;
    Ndr_ModForEachPo( p, Mod, Obj )
        Count++;
    return Count;
}
static inline int Ndr_DataObjNum( Ndr_Data_t * p, int Mod )
{ 
    int Obj, Count = 0;
    Ndr_ModForEachObj( p, Mod, Obj )
        Count++;
    return Count;
}

// to write signal names, this procedure takes a mapping of name IDs into actual char-strings (pNames)
static inline void Ndr_WriteVerilogModule( FILE * pFile, void * pDesign, int Mod, char ** pNames )
{
    Ndr_Data_t * p = (Ndr_Data_t *)pDesign; 
    int Obj, nArray, * pArray, fFirst = 1;

    fprintf( pFile, "\nmodule %s (\n  ", pNames[Ndr_ObjReadEntry(p, Mod, NDR_NAME)] );

    Ndr_ModForEachPi( p, Mod, Obj )
        fprintf( pFile, "%s, ", Ndr_ObjReadOutName(p, Obj, pNames) );

    fprintf( pFile, "\n  " );

    Ndr_ModForEachPo( p, Mod, Obj )
        fprintf( pFile, "%s%s", fFirst ? "":", ", Ndr_ObjReadInName(p, Obj, pNames) ), fFirst = 0;

    fprintf( pFile, "\n);\n\n" );

    Ndr_ModForEachPi( p, Mod, Obj )
    {
        fprintf( pFile, "  input " );
        Ndr_ObjWriteRange( p, Obj, pFile, 1 );
        fprintf( pFile, " %s;\n", Ndr_ObjReadOutName(p, Obj, pNames) );
    }

    Ndr_ModForEachPo( p, Mod, Obj )
    {
        fprintf( pFile, "  output " );
        Ndr_ObjWriteRange( p, Obj, pFile, 1 );
        fprintf( pFile, " %s;\n", Ndr_ObjReadInName(p, Obj, pNames) );
    }

    fprintf( pFile, "\n" );

    Ndr_ModForEachNode( p, Mod, Obj )
    {
        fprintf( pFile, "  wire " );
        Ndr_ObjWriteRange( p, Obj, pFile, 1 );
        fprintf( pFile, " %s;\n", Ndr_ObjReadOutName(p, Obj, pNames) );
    }

    fprintf( pFile, "\n" );

    Ndr_ModForEachNode( p, Mod, Obj )
    {
        int i, Type = Ndr_ObjReadBody(p, Obj, NDR_OPERTYPE);
        if ( Type >= 256 )
        {
            fprintf( pFile, "  %s ", pNames[Ndr_ObjReadEntry(p, Type-256, NDR_NAME)] );
            if ( Ndr_ObjReadBody(p, Obj, NDR_NAME) )
                fprintf( pFile, "%s ", pNames[Ndr_ObjReadBody(p, Obj, NDR_NAME)] );
            fprintf( pFile, "( " );
            nArray = Ndr_ObjReadArray( p, Obj, NDR_INPUT, &pArray );
            for ( i = 0; i < nArray; i++ )
                fprintf( pFile, "%s%s ", pNames[pArray[i]], i==nArray-1 ? "":"," );
            fprintf( pFile, ");\n" );
            continue;
        }
        fprintf( pFile, "  assign %s = ", Ndr_ObjReadOutName(p, Obj, pNames) );
        nArray = Ndr_ObjReadArray( p, Obj, NDR_INPUT, &pArray );
        if ( nArray == 0 )
            fprintf( pFile, "%s;\n", (char *)Ndr_ObjReadBodyP(p, Obj, NDR_FUNCTION) );
        else if ( nArray == 1 && Ndr_ObjReadBody(p, Obj, NDR_OPERTYPE) == ABC_OPER_BIT_BUF )
            fprintf( pFile, "%s;\n", pNames[pArray[0]] );
        else if ( Type == ABC_OPER_SLICE )
            fprintf( pFile, "%s", pNames[pArray[0]] ),
            Ndr_ObjWriteRange( p, Obj, pFile, 0 ),
            fprintf( pFile, ";\n" );
        else if ( Type == ABC_OPER_CONCAT )
        {
            fprintf( pFile, "{" );
            for ( i = 0; i < nArray; i++ )
                fprintf( pFile, "%s%s", pNames[pArray[i]], i==nArray-1 ? "":", " );
            fprintf( pFile, "};\n" );
        }
        else if ( nArray == 1 )
            fprintf( pFile, "%s %s;\n", Abc_OperName(Ndr_ObjReadBody(p, Obj, NDR_OPERTYPE)), pNames[pArray[0]] );
        else if ( nArray == 2 )
            fprintf( pFile, "%s %s %s;\n", pNames[pArray[0]], Abc_OperName(Ndr_ObjReadBody(p, Obj, NDR_OPERTYPE)), pNames[pArray[1]] );
        else if ( nArray == 3 && Type == ABC_OPER_ARI_ADD )
            fprintf( pFile, "%s + %s + %s;\n", pNames[pArray[0]], pNames[pArray[1]], pNames[pArray[2]] );
        else if ( Type == ABC_OPER_BIT_MUX )
            fprintf( pFile, "%s ? %s : %s;\n", pNames[pArray[0]], pNames[pArray[1]], pNames[pArray[2]] );
        else
            fprintf( pFile, "<cannot write operation %s>;\n", Abc_OperName(Ndr_ObjReadBody(p, Obj, NDR_OPERTYPE)) );
    }

    fprintf( pFile, "\nendmodule\n\n" );
}

// to write signal names, this procedure takes a mapping of name IDs into actual char-strings (pNames)
static inline void Ndr_WriteVerilog( char * pFileName, void * pDesign, char ** pNames )
{
    Ndr_Data_t * p = (Ndr_Data_t *)pDesign; int Mod;

    FILE * pFile = pFileName ? fopen( pFileName, "wb" ) : stdout;
    if ( pFile == NULL ) { printf( "Cannot open file \"%s\" for writing.\n", pFileName ); return; }

    Ndr_DesForEachMod( p, Mod )
        Ndr_WriteVerilogModule( pFile, p, Mod, pNames );
    
    if ( pFileName ) fclose( pFile );
}


////////////////////////////////////////////////////////////////////////
///                     EXTERNAL PROCEDURES                          ///
////////////////////////////////////////////////////////////////////////

// creating a new module (returns pointer to the memory buffer storing the module info)
static inline void * Ndr_Create( int Name )
{
    Ndr_Data_t * p = ABC_ALLOC( Ndr_Data_t, 1 );
    p->nSize = 0;
    p->nCap  = 16;
    p->pHead = ABC_ALLOC( unsigned char, p->nCap );
    p->pBody = ABC_ALLOC( unsigned int, p->nCap * 4 );
    Ndr_DataPush( p, NDR_DESIGN, 0 );
    Ndr_DataPush( p, NDR_NAME, Name );
    Ndr_DataAddTo( p, 0, p->nSize );
    assert( p->nSize == 2 );
    assert( Name );
    return p;
}

// creating a new module in an already started design 
// returns module ID to be used when adding objects to the module
static inline int Ndr_AddModule( void * pDesign, int Name )
{
    Ndr_Data_t * p = (Ndr_Data_t *)pDesign;
    int Mod = p->nSize;  
    Ndr_DataResize( p, 6 );
    Ndr_DataPush( p, NDR_MODULE, 0 );
    Ndr_DataPush( p, NDR_NAME, Name );
    Ndr_DataAddTo( p, Mod, p->nSize - Mod );
    Ndr_DataAddTo( p, 0, p->nSize - Mod );
    assert( (int)p->pBody[0] == p->nSize );
    return Mod + 256;
}

// adding a new object (input/output/flop/intenal node) to an already started module
// this procedure takes the design, the module ID, and the parameters of the boject
// (please note that all objects should be added to a given module before starting a new module)
static inline void Ndr_AddObject( void * pDesign, int ModuleId,
                                  int ObjType, int InstName, 
                                  int RangeLeft, int RangeRight, int fSignedness, 
                                  int nInputs, int * pInputs, 
                                  int nOutputs, int * pOutputs, 
                                  char * pFunction )
{
    Ndr_Data_t * p = (Ndr_Data_t *)pDesign;
    int Mod = ModuleId - 256;  
    int Obj = p->nSize;  assert( ObjType != 0 );
    Ndr_DataResize( p, 6 );
    Ndr_DataPush( p, NDR_OBJECT, 0 );
    Ndr_DataPush( p, NDR_OPERTYPE, ObjType );
    Ndr_DataPushRange( p, RangeLeft, RangeRight, fSignedness );
    if ( InstName )
        Ndr_DataPush( p, NDR_NAME, InstName );
    Ndr_DataPushArray( p, NDR_INPUT, nInputs, pInputs );
    Ndr_DataPushArray( p, NDR_OUTPUT, nOutputs, pOutputs );
    Ndr_DataPushString( p, NDR_FUNCTION, pFunction );
    Ndr_DataAddTo( p, Obj, p->nSize - Obj );
    Ndr_DataAddTo( p, Mod, p->nSize - Obj );
    Ndr_DataAddTo( p, 0, p->nSize - Obj );
    assert( (int)p->pBody[0] == p->nSize );
}

// deallocate the memory buffer
static inline void Ndr_Delete( void * pDesign )
{
    Ndr_Data_t * p = (Ndr_Data_t *)pDesign;
    if ( !p ) return;
    free( p->pHead );
    free( p->pBody );
    free( p );
}


////////////////////////////////////////////////////////////////////////
///                  FILE READING AND WRITING                        ///
////////////////////////////////////////////////////////////////////////

// file reading/writing
static inline void * Ndr_Read( char * pFileName )
{
    Ndr_Data_t * p; int nFileSize, RetValue;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL ) { printf( "Cannot open file \"%s\" for reading.\n", pFileName ); return NULL; }
    // check file size
    fseek( pFile, 0, SEEK_END );
    nFileSize = ftell( pFile ); 
    assert( nFileSize % 5 == 0 );
    rewind( pFile );
    // create structure
    p = ABC_ALLOC( Ndr_Data_t, 1 );
    p->nSize = p->nCap = nFileSize / 5;
    p->pHead = ABC_ALLOC( unsigned char, p->nCap );
    p->pBody = ABC_ALLOC( unsigned int, p->nCap * 4 );
    RetValue = (int)fread( p->pBody, 4, p->nCap, pFile );
    RetValue = (int)fread( p->pHead, 1, p->nCap, pFile );
    assert( p->nSize == (int)p->pBody[0] );
    fclose( pFile );
    //printf( "Read the design from file \"%s\".\n", pFileName );
    return p;
}
static inline void Ndr_Write( char * pFileName, void * pDesign )
{
    Ndr_Data_t * p = (Ndr_Data_t *)pDesign; int RetValue;
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL ) { printf( "Cannot open file \"%s\" for writing.\n", pFileName ); return; }
    RetValue = (int)fwrite( p->pBody, 4, p->pBody[0], pFile );
    RetValue = (int)fwrite( p->pHead, 1, p->pBody[0], pFile );
    fclose( pFile );
    //printf( "Dumped the design into file \"%s\".\n", pFileName );
}


////////////////////////////////////////////////////////////////////////
///                     TESTING PROCEDURE                            ///
////////////////////////////////////////////////////////////////////////

// This testing procedure creates and writes into a Verilog file 
// for the following design composed of one module

// module add10 ( input [3:0] a, output [3:0] s );
//   wire [3:0] const10 = 4'b1010;
//   assign s = a + const10;
// endmodule

static inline void Ndr_ModuleTest()
{
    // name IDs
    int NameIdA = 2;
    int NameIdS = 3;
    int NameIdC = 4;
    // array of fanins of node s
    int Fanins[2] = { NameIdA, NameIdC };
    // map name IDs into char strings
    char * ppNames[5] = { NULL, "add10", "a", "s", "const10" };

    // create a new module
    void * pDesign = Ndr_Create( 1 );

    int ModuleID = Ndr_AddModule( pDesign, 1 );

    // add objects to the modele
    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_CI,       0,   3, 0, 0,   0, NULL,      1, &NameIdA,   NULL      ); // no fanins
    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_CONST,    0,   3, 0, 0,   0, NULL,      1, &NameIdC,   "4'b1010" ); // no fanins
    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_ARI_ADD,  0,   3, 0, 0,   2, Fanins,    1, &NameIdS,   NULL      ); // fanins are a and const10 
    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_CO,       0,   3, 0, 0,   1, &NameIdS,  0, NULL,       NULL      ); // fanin is a

    // write Verilog for verification
    Ndr_WriteVerilog( NULL, pDesign, ppNames );
    Ndr_Write( "add4.ndr", pDesign );
    Ndr_Delete( pDesign );
}



// This testing procedure creates and writes into a Verilog file 
// for the following design composed of one adder divided into two

// module add8 ( input [7:0] a, input [7:0] b, output [7:0] s, output co );
//   wire [3:0] a0 = a[3:0];
//   wire [3:0] b0 = b[3:0];

//   wire [7:4] a1 = a[7:4];
//   wire [7:4] b1 = b[7:4];

//   wire [4:0] r0 = a0 + b0;
//   wire [3:0] s0 = r0[3:0];
//   wire rco = r0[4];

//   wire [4:0] r1 = a1 + b1 + rco;
//   wire [3:0] s1 = r1[3:0];
//   assign co = r1[4];

//   assign s = {s1, s0};
// endmodule

static inline void Ndr_ModuleTestAdder()
{
    // map name IDs into char strings
    char * ppNames[20] = {  NULL, 
                           "a", "b", "s", "co",          // 1,  2,  3,  4
                           "a0",  "a1",  "b0",  "b1",    // 5,  6,  7,  8
                           "r0", "s0", "rco",            // 9,  10, 11
                           "r1", "s1", "add8"            // 12, 13, 14
                         };
    // fanins 
    int FaninA        =  1;
    int FaninB        =  2;
    int FaninS        =  3;
    int FaninCO       =  4;

    int FaninA0       =  5;
    int FaninA1       =  6;
    int FaninB0       =  7;
    int FaninB1       =  8;

    int FaninR0       =  9;
    int FaninS0       = 10;
    int FaninRCO      = 11;

    int FaninR1       = 12;
    int FaninS1       = 13;

    int Fanins1[2]    = { FaninA0, FaninB0 };
    int Fanins2[3]    = { FaninA1, FaninB1, FaninRCO };
    int Fanins3[4]    = { FaninS1, FaninS0 };

    // create a new module
    void * pDesign = Ndr_Create( 14 );

    int ModuleID = Ndr_AddModule( pDesign, 14 );

    // add objects to the modele
    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_CI,       0,   7, 0, 0,   0, NULL,      1, &FaninA,   NULL  );  // no fanins
    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_CI,       0,   7, 0, 0,   0, NULL,      1, &FaninB,   NULL  );  // no fanins

    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_SLICE,    0,   3, 0, 0,   1, &FaninA,   1, &FaninA0,   NULL );  // wire [3:0] a0 = a[3:0];
    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_SLICE,    0,   3, 0, 0,   1, &FaninB,   1, &FaninB0,   NULL );  // wire [3:0] b0 = a[3:0];

    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_SLICE,    0,   7, 4, 0,   1, &FaninA,   1, &FaninA1,   NULL );  // wire [7:4] a1 = a[7:4];
    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_SLICE,    0,   7, 4, 0,   1, &FaninB,   1, &FaninB1,   NULL );  // wire [7:4] b1 = b[7:4];

    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_ARI_ADD,  0,   4, 0, 0,   2, Fanins1,   1, &FaninR0,   NULL );  // wire [4:0] r0 = a0 + b0;
    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_SLICE,    0,   3, 0, 0,   1, &FaninR0,  1, &FaninS0,   NULL );  // wire [3:0] s0 = r0[3:0];
    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_SLICE,    0,   4, 4, 0,   1, &FaninR0,  1, &FaninRCO,  NULL );  // wire rco = r0[4];

    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_ARI_ADD,  0,   4, 0, 0,   3, Fanins2,   1, &FaninR1,   NULL );  // wire [4:0] r1 = a1 + b1 + rco;
    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_SLICE,    0,   3, 0, 0,   1, &FaninR1,  1, &FaninS1,   NULL );  // wire [3:0] s1 = r1[3:0];
    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_SLICE,    0,   4, 4, 0,   1, &FaninR1,  1, &FaninCO,   NULL );  // assign co = r1[4];

    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_CONCAT,   0,   7, 0, 0,   2, Fanins3,   1, &FaninS,    NULL );  // s = {s1, s0};

    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_CO,       0,   7, 0, 0,   1, &FaninS,   0, NULL,       NULL ); 
    Ndr_AddObject( pDesign, ModuleID, ABC_OPER_CO,       0,   0, 0, 0,   1, &FaninCO,  0, NULL,       NULL ); 

    // write Verilog for verification
    Ndr_WriteVerilog( NULL, pDesign, ppNames );
    Ndr_Write( "add8.ndr", pDesign );
    Ndr_Delete( pDesign );

}

// This testing procedure creates and writes into a Verilog file 
// for the following hierarchical design composed of three modules

// module mux21w ( input sel, input [3:0] d1, input [3:0] d0, output [3:0] out );
//   assign out = sel ? d1 : d0;
// endmodule

// module mux41w ( input [1:0] sel, input [15:0] d, output [3:0] out );
//   wire [3:0] t0, t1;
//   wire [3:0] d0 = d[3:0];
//   wire [3:0] d1 = d[7:4];
//   wire [3:0] d2 = d[11:8];
//   wire [3:0] d3 = d[15:12];
//   wire sel0 = sel[0];
//   wire sel1 = sel[1];
//   mux21w i0 ( sel0, d1, d0, t0 );
//   mux21w i1 ( sel0, d3, d2, t1 );
//   mux21w i2 ( sel1, t1, t0, out );
// endmodule

static inline void Ndr_ModuleTestHierarchy()
{
    // map name IDs into char strings
    char * ppNames[20] = {  NULL, 
                           "mux21w", "mux41w",     // 1,  2
                           "sel",  "d",  "out",    // 3,  4,  5
                           "d0", "d1", "d2", "d3", // 6,  7,  8,  9
                           "sel0", "sel1",         // 10, 11,
                           "t0", "t1",             // 12, 13
                           "i0", "i1", "i2"        // 14, 15, 16
                         };
    // fanins 
    int FaninSel      =  3;
    int FaninSel0     = 10;
    int FaninSel1     = 11;
    int FaninD        =  4;
    int FaninD0       =  6;
    int FaninD1       =  7;
    int FaninD2       =  8;
    int FaninD3       =  9;
    int FaninT0       = 12;
    int FaninT1       = 13;
    int FaninOut      =  5;
    int Fanins1[3]    = {  FaninSel,  FaninD1, FaninD0 };
    int Fanins3[3][3] = { {FaninSel0, FaninD1, FaninD0 },  
                          {FaninSel0, FaninD3, FaninD2 },  
                          {FaninSel1, FaninT1, FaninT0 } };

    // create a new module
    void * pDesign = Ndr_Create( 2 );

    int Module21, Module41;

    Module21 = Ndr_AddModule( pDesign, 1 );
    
    Ndr_AddObject( pDesign, Module21, ABC_OPER_CI,        0,   0, 0, 0,   0, NULL,        1, &FaninSel,  NULL ); 
    Ndr_AddObject( pDesign, Module21, ABC_OPER_CI,        0,   3, 0, 0,   0, NULL,        1, &FaninD1,   NULL ); 
    Ndr_AddObject( pDesign, Module21, ABC_OPER_CI,        0,   3, 0, 0,   0, NULL,        1, &FaninD0,   NULL ); 
    Ndr_AddObject( pDesign, Module21, ABC_OPER_BIT_MUX,   0,   3, 0, 0,   3, Fanins1,     1, &FaninOut,  NULL ); 
    Ndr_AddObject( pDesign, Module21, ABC_OPER_CO,        0,   3, 0, 0,   1, &FaninOut,   0, NULL,       NULL ); 

    Module41 = Ndr_AddModule( pDesign, 2 );

    Ndr_AddObject( pDesign, Module41, ABC_OPER_CI,        0,   1, 0, 0,   0, NULL,        1, &FaninSel,  NULL ); 
    Ndr_AddObject( pDesign, Module41, ABC_OPER_CI,        0,   15,0, 0,   0, NULL,        1, &FaninD,    NULL ); 

    Ndr_AddObject( pDesign, Module41, ABC_OPER_SLICE,     0,   3, 0, 0,   1, &FaninD,     1, &FaninD0,   NULL ); 
    Ndr_AddObject( pDesign, Module41, ABC_OPER_SLICE,     0,   7, 4, 0,   1, &FaninD,     1, &FaninD1,   NULL ); 
    Ndr_AddObject( pDesign, Module41, ABC_OPER_SLICE,     0,   11,8, 0,   1, &FaninD,     1, &FaninD2,   NULL ); 
    Ndr_AddObject( pDesign, Module41, ABC_OPER_SLICE,     0,   15,12,0,   1, &FaninD,     1, &FaninD3,   NULL ); 

    Ndr_AddObject( pDesign, Module41, ABC_OPER_SLICE,     0,   0, 0, 0,   1, &FaninSel,   1, &FaninSel0, NULL ); 
    Ndr_AddObject( pDesign, Module41, ABC_OPER_SLICE,     0,   1, 1, 0,   1, &FaninSel,   1, &FaninSel1, NULL ); 

    Ndr_AddObject( pDesign, Module41, Module21,          14,   3, 0, 0,   3, Fanins3[0],  1, &FaninT0,   NULL ); 
    Ndr_AddObject( pDesign, Module41, Module21,          15,   3, 0, 0,   3, Fanins3[1],  1, &FaninT1,   NULL ); 
    Ndr_AddObject( pDesign, Module41, Module21,          16,   3, 0, 0,   3, Fanins3[2],  1, &FaninOut,  NULL ); 
    Ndr_AddObject( pDesign, Module41, ABC_OPER_CO,        0,   3, 0, 0,   1, &FaninOut,   0, NULL,       NULL ); 

    // write Verilog for verification
    Ndr_WriteVerilog( NULL, pDesign, ppNames );
    Ndr_Write( "mux41w.ndr", pDesign );
    Ndr_Delete( pDesign );
}


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

