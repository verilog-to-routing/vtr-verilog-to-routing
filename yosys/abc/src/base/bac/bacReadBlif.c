/**CFile****************************************************************

  FileName    [bacReadBlif.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [BLIF parser.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: bacReadBlif.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bac.h"
#include "bacPrs.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// BLIF keywords
typedef enum { 
    PRS_BLIF_NONE = 0, // 0:   unused
    PRS_BLIF_MODEL,    // 1:   .model
    PRS_BLIF_INOUTS,   // 2:   .inouts
    PRS_BLIF_INPUTS,   // 3:   .inputs
    PRS_BLIF_OUTPUTS,  // 4:   .outputs
    PRS_BLIF_NAMES,    // 5:   .names
    PRS_BLIF_SUBCKT,   // 6:   .subckt
    PRS_BLIF_GATE,     // 7:   .gate
    PRS_BLIF_LATCH,    // 8:   .latch
    PRS_BLIF_SHORT,    // 9:   .short
    PRS_BLIF_END,      // 10:  .end
    PRS_BLIF_UNKNOWN   // 11:  unknown
} Bac_BlifType_t;

static const char * s_BlifTypes[PRS_BLIF_UNKNOWN+1] = {
    NULL,              // 0:   unused
    ".model",          // 1:   .model   
    ".inouts",         // 2:   .inputs
    ".inputs",         // 3:   .inputs
    ".outputs",        // 4:   .outputs
    ".names",          // 5:   .names
    ".subckt",         // 6:   .subckt
    ".gate",           // 7:   .gate
    ".latch",          // 8:   .latch
    ".short",          // 9:   .short
    ".end",            // 10:  .end
    NULL               // 11:  unknown
};

static inline void Psr_NtkAddBlifDirectives( Psr_Man_t * p )
{
    int i;
    for ( i = 1; s_BlifTypes[i]; i++ )
        Abc_NamStrFindOrAdd( p->pStrs, (char *)s_BlifTypes[i], NULL );
    assert( Abc_NamObjNumMax(p->pStrs) == i );
}


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reading characters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int  Psr_CharIsSpace( char c )                { return c == ' ' || c == '\t' || c == '\r';              }
static inline int  Psr_CharIsStop( char c )                 { return c == '#' || c == '\\' || c == '\n' || c == '=';  }
static inline int  Psr_CharIsLit( char c )                  { return c == '0' || c == '1'  || c == '-';               }

static inline int  Psr_ManIsSpace( Psr_Man_t * p )          { return Psr_CharIsSpace(*p->pCur);                       }
static inline int  Psr_ManIsStop( Psr_Man_t * p )           { return Psr_CharIsStop(*p->pCur);                        }
static inline int  Psr_ManIsLit( Psr_Man_t * p )            { return Psr_CharIsLit(*p->pCur);                         }

static inline int  Psr_ManIsChar( Psr_Man_t * p, char c )   { return *p->pCur == c;                                   }
static inline int  Psr_ManIsChar2( Psr_Man_t * p, char c )  { return *p->pCur++ == c;                                 }

static inline void Psr_ManSkip( Psr_Man_t * p )             { p->pCur++;                                              }
static inline char Psr_ManSkip2( Psr_Man_t * p )            { return *p->pCur++;                                      }


/**Function*************************************************************

  Synopsis    [Reading names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Psr_ManSkipToChar( Psr_Man_t * p, char c )  
{ 
    while ( !Psr_ManIsChar(p, c) ) 
        Psr_ManSkip(p);
}
static inline void Psr_ManSkipSpaces( Psr_Man_t * p )
{
    while ( 1 )
    {
        while ( Psr_ManIsSpace(p) )
            Psr_ManSkip(p);
        if ( Psr_ManIsChar(p, '\\') )
        {
            Psr_ManSkipToChar( p, '\n' );
            Psr_ManSkip(p);
            continue;
        }
        if ( Psr_ManIsChar(p, '#') )  
            Psr_ManSkipToChar( p, '\n' );
        break;
    }
    assert( !Psr_ManIsSpace(p) );
}
static inline int Psr_ManReadName( Psr_Man_t * p )
{
    char * pStart;
    Psr_ManSkipSpaces( p );
    if ( Psr_ManIsChar(p, '\n') )
        return 0;
    pStart = p->pCur;
    while ( !Psr_ManIsSpace(p) && !Psr_ManIsStop(p) )
        Psr_ManSkip(p);
    if ( pStart == p->pCur )
        return 0;
    return Abc_NamStrFindOrAddLim( p->pStrs, pStart, p->pCur, NULL );
}
static inline int Psr_ManReadList( Psr_Man_t * p, Vec_Int_t * vOrder, int Type )
{
    int iToken;
    Vec_IntClear( &p->vTemp );
    while ( (iToken = Psr_ManReadName(p)) )
    {
        Vec_IntPush( &p->vTemp, iToken );
        Vec_IntPush( vOrder, Abc_Var2Lit2(iToken, Type) );
    }
    if ( Vec_IntSize(&p->vTemp) == 0 )                return Psr_ManErrorSet(p, "Signal list is empty.", 1);
    return 0;
}
static inline int Psr_ManReadList2( Psr_Man_t * p )
{
    int iToken;
    Vec_IntClear( &p->vTemp );
    while ( (iToken = Psr_ManReadName(p)) )
        Vec_IntPushTwo( &p->vTemp, 0, iToken );
    if ( Vec_IntSize(&p->vTemp) == 0 )                return Psr_ManErrorSet(p, "Signal list is empty.", 1);
    return 0;
}
static inline int Psr_ManReadList3( Psr_Man_t * p )
{
    Vec_IntClear( &p->vTemp );
    while ( !Psr_ManIsChar(p, '\n') )
    {
        int iToken = Psr_ManReadName(p);
        if ( iToken == 0 )              return Psr_ManErrorSet(p, "Cannot read formal name.", 1);
        Vec_IntPush( &p->vTemp, iToken );
        Psr_ManSkipSpaces( p );
        if ( !Psr_ManIsChar2(p, '=') )  return Psr_ManErrorSet(p, "Cannot find symbol \"=\".", 1);
        iToken = Psr_ManReadName(p);
        if ( iToken == 0 )              return Psr_ManErrorSet(p, "Cannot read actual name.", 1);
        Vec_IntPush( &p->vTemp, iToken );
        Psr_ManSkipSpaces( p );
    }
    if ( Vec_IntSize(&p->vTemp) == 0 )  return Psr_ManErrorSet(p, "Cannot read a list of formal/actual names.", 1);
    if ( Vec_IntSize(&p->vTemp) % 2  )  return Psr_ManErrorSet(p, "The number of formal/actual names is not even.", 1);
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Psr_ManReadCube( Psr_Man_t * p )
{
    assert( Psr_ManIsLit(p) );
    while ( Psr_ManIsLit(p) )
        Vec_StrPush( &p->vCover, Psr_ManSkip2(p) );
    Psr_ManSkipSpaces( p );
    if ( Psr_ManIsChar(p, '\n') )
    {
        if ( Vec_StrSize(&p->vCover) != 1 )           return Psr_ManErrorSet(p, "Cannot read cube.", 1);
        // fix single literal cube by adding space
        Vec_StrPush( &p->vCover, Vec_StrEntry(&p->vCover,0) );
        Vec_StrWriteEntry( &p->vCover, 0, ' ' );
        Vec_StrPush( &p->vCover, '\n' );
        return 0;
    }
    if ( !Psr_ManIsLit(p) )                           return Psr_ManErrorSet(p, "Cannot read output literal.", 1);
    Vec_StrPush( &p->vCover, ' ' );
    Vec_StrPush( &p->vCover, Psr_ManSkip2(p) );
    Vec_StrPush( &p->vCover, '\n' );
    Psr_ManSkipSpaces( p );
    if ( !Psr_ManIsChar(p, '\n') )                    return Psr_ManErrorSet(p, "Cannot read end of cube.", 1);
    return 0;
}
static inline void Psr_ManSaveCover( Psr_Man_t * p )
{
    int iToken;
    if ( Vec_StrSize(&p->vCover) == 0 )
        p->pNtk->fHasC0s = 1;
    else if ( Vec_StrSize(&p->vCover) == 2 )
    {
        if ( Vec_StrEntryLast(&p->vCover) == '0' )
            p->pNtk->fHasC0s = 1;
        else if ( Vec_StrEntryLast(&p->vCover) == '1' )
            p->pNtk->fHasC1s = 1;
        else assert( 0 );
    }
    assert( Vec_StrSize(&p->vCover) > 0 );
    Vec_StrPush( &p->vCover, '\0' );
    //iToken = Abc_NamStrFindOrAdd( p->pStrs, Vec_StrArray(&p->vCover), NULL );
    iToken = Ptr_SopToType( Vec_StrArray(&p->vCover) );
    Vec_StrClear( &p->vCover );
    // set the cover to the module of this box
    assert( Psr_BoxNtk(p->pNtk, Psr_NtkBoxNum(p->pNtk)-1) == 1 ); // default const 0
    Psr_BoxSetNtk( p->pNtk, Psr_NtkBoxNum(p->pNtk)-1, iToken );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Psr_ManReadInouts( Psr_Man_t * p )
{
    if ( Psr_ManReadList(p, &p->pNtk->vOrder, 3) )    return 1;
    Vec_IntAppend( &p->pNtk->vInouts, &p->vTemp );
    return 0;
}
static inline int Psr_ManReadInputs( Psr_Man_t * p )
{
    if ( Psr_ManReadList(p, &p->pNtk->vOrder, 1) )    return 1;
    Vec_IntAppend( &p->pNtk->vInputs, &p->vTemp );
    return 0;
}
static inline int Psr_ManReadOutputs( Psr_Man_t * p )
{
    if ( Psr_ManReadList(p, &p->pNtk->vOrder, 2) )    return 1;
    Vec_IntAppend( &p->pNtk->vOutputs, &p->vTemp );
    return 0;
}
static inline int Psr_ManReadNode( Psr_Man_t * p )
{
    if ( Psr_ManReadList2(p) )   return 1;
    // save results
    Psr_NtkAddBox( p->pNtk, 1, 0, &p->vTemp ); // default const 0 function
    return 0;
}
static inline int Psr_ManReadBox( Psr_Man_t * p, int fGate )
{
    int iToken = Psr_ManReadName(p);
    if ( iToken == 0 )           return Psr_ManErrorSet(p, "Cannot read model name.", 1);
    if ( Psr_ManReadList3(p) )   return 1;
    // save results
    Psr_NtkAddBox( p->pNtk, iToken, 0, &p->vTemp );
    if ( fGate ) p->pNtk->fMapped = 1;
    return 0;
}
static inline int Psr_ManReadLatch( Psr_Man_t * p )
{
    int iToken = Psr_ManReadName(p);
    Vec_IntClear( &p->vTemp );
    if ( iToken == 0 )                 return Psr_ManErrorSet(p, "Cannot read latch input.", 1);
    Vec_IntWriteEntry( &p->vTemp, 1, iToken );
    iToken = Psr_ManReadName(p);
    if ( iToken == 0 )                 return Psr_ManErrorSet(p, "Cannot read latch output.", 1);
    Vec_IntWriteEntry( &p->vTemp, 0, iToken );
    Psr_ManSkipSpaces( p );
    if ( Psr_ManIsChar(p, '0') )
        iToken = 0;
    else if ( Psr_ManIsChar(p, '1') )
        iToken = 1;
    else 
        iToken = 2;
    Psr_ManSkipToChar( p, '\n' );
    // save results
    Psr_NtkAddBox( p->pNtk, -1, iToken, &p->vTemp ); // -1 stands for latch
    return 0;
}
static inline int Psr_ManReadShort( Psr_Man_t * p )
{
    int iToken = Psr_ManReadName(p);
    Vec_IntClear( &p->vTemp );
    if ( iToken == 0 )                 return Psr_ManErrorSet(p, "Cannot read .short input.", 1);
    Vec_IntWriteEntry( &p->vTemp, 1, iToken );
    iToken = Psr_ManReadName(p);
    if ( iToken == 0 )                 return Psr_ManErrorSet(p, "Cannot read .short output.", 1);
    Vec_IntWriteEntry( &p->vTemp, 0, iToken );
    Psr_ManSkipSpaces( p );
    if ( !Psr_ManIsChar(p, '\n') )     return Psr_ManErrorSet(p, "Trailing symbols on .short line.", 1);
    // save results
    iToken = Abc_NamStrFindOrAdd( p->pStrs, "1 1\n", NULL );
    Psr_NtkAddBox( p->pNtk, iToken, 0, &p->vTemp );
    return 0;
}
static inline int Psr_ManReadModel( Psr_Man_t * p )
{
    int iToken;
    if ( p->pNtk != NULL )                         return Psr_ManErrorSet(p, "Parsing previous model is unfinished.", 1);
    iToken = Psr_ManReadName(p);
    if ( iToken == 0 )                             return Psr_ManErrorSet(p, "Cannot read model name.", 1);
    Psr_ManInitializeNtk( p, iToken, 0 );
    Psr_ManSkipSpaces( p );
    if ( !Psr_ManIsChar(p, '\n') )                 return Psr_ManErrorSet(p, "Trailing symbols on .model line.", 1);
    return 0;
}
static inline int Psr_ManReadEnd( Psr_Man_t * p )
{
    if ( p->pNtk == 0 )                            return Psr_ManErrorSet(p, "Directive .end without .model.", 1);
    //printf( "Saving model \"%s\".\n", Abc_NamStr(p->pStrs, p->iModuleName) );
    Psr_ManFinalizeNtk( p );
    Psr_ManSkipSpaces( p );
    if ( !Psr_ManIsChar(p, '\n') )                 return Psr_ManErrorSet(p, "Trailing symbols on .end line.", 1);
    return 0;
}

static inline int Psr_ManReadDirective( Psr_Man_t * p )
{
    int iToken;
    if ( !Psr_ManIsChar(p, '.') )
        return Psr_ManReadCube( p );
    if ( Vec_StrSize(&p->vCover) > 0 ) // SOP was specified for the previous node
        Psr_ManSaveCover( p );
    iToken = Psr_ManReadName( p );  
    if ( iToken == PRS_BLIF_MODEL )
        return Psr_ManReadModel( p );
    if ( iToken == PRS_BLIF_INOUTS )
        return Psr_ManReadInouts( p );
    if ( iToken == PRS_BLIF_INPUTS )
        return Psr_ManReadInputs( p );
    if ( iToken == PRS_BLIF_OUTPUTS )
        return Psr_ManReadOutputs( p );
    if ( iToken == PRS_BLIF_NAMES )
        return Psr_ManReadNode( p );
    if ( iToken == PRS_BLIF_SUBCKT )
        return Psr_ManReadBox( p, 0 );
    if ( iToken == PRS_BLIF_GATE )
        return Psr_ManReadBox( p, 1 );
    if ( iToken == PRS_BLIF_LATCH )
        return Psr_ManReadLatch( p );
    if ( iToken == PRS_BLIF_SHORT )
        return Psr_ManReadShort( p );
    if ( iToken == PRS_BLIF_END )
        return Psr_ManReadEnd( p );
    printf( "Cannot read directive \"%s\".\n", Abc_NamStr(p->pStrs, iToken) );
    return 1;
}
static inline int Psr_ManReadLines( Psr_Man_t * p )
{
    while ( p->pCur[1] != '\0' )
    {
        assert( Psr_ManIsChar(p, '\n') );
        Psr_ManSkip(p);
        Psr_ManSkipSpaces( p );
        if ( Psr_ManIsChar(p, '\n') )
            continue;
        if ( Psr_ManReadDirective(p) )   
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
Vec_Ptr_t * Psr_ManReadBlif( char * pFileName )
{
    Vec_Ptr_t * vPrs = NULL;
    Psr_Man_t * p = Psr_ManAlloc( pFileName );
    if ( p == NULL )
        return NULL;
    Psr_NtkAddBlifDirectives( p );
    Psr_ManReadLines( p );
    if ( Psr_ManErrorPrint(p) )
        ABC_SWAP( Vec_Ptr_t *, vPrs, p->vNtks );
    Psr_ManFree( p );
    return vPrs;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Psr_ManReadBlifTest()
{
    abctime clk = Abc_Clock();
    extern void Psr_ManWriteBlif( char * pFileName, Vec_Ptr_t * vPrs );
//    Vec_Ptr_t * vPrs = Psr_ManReadBlif( "aga/ray/ray_hie_oper.blif" );
    Vec_Ptr_t * vPrs = Psr_ManReadBlif( "c/hie/dump/1/netlist_1_out8.blif" );
    if ( !vPrs ) return;
    printf( "Finished reading %d networks. ", Vec_PtrSize(vPrs) );
    printf( "NameIDs = %d. ", Abc_NamObjNumMax(Psr_ManNameMan(vPrs)) );
    printf( "Memory = %.2f MB. ", 1.0*Psr_ManMemory(vPrs)/(1<<20) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
//    Abc_NamPrint( p->pStrs );
    Psr_ManWriteBlif( "c/hie/dump/1/netlist_1_out8_out.blif", vPrs );
    Psr_ManVecFree( vPrs );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

