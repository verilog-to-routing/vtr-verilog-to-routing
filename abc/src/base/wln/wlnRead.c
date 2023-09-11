/**CFile****************************************************************

  FileName    [wln.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Word-level network.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 23, 2018.]

  Revision    [$Id: wln.c,v 1.00 2018/09/23 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wln.h"
#include "proof/cec/cec.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAX_LINE 1000000

#define MAX_MAP       32
#define CELL_NUM       8
#define WIRE_NUM       5
#define TEMP_NUM       5
#define CONST_SHIFT   99

//typedef struct Rtl_Lib_t_  Rtl_Lib_t;
struct Rtl_Lib_t_ 
{
    char *                pSpec;     // input file name
    Vec_Ptr_t *           vNtks;     // modules
    Abc_Nam_t *           pManName;  // object names
    Vec_Int_t             vConsts;   // constants
    Vec_Int_t             vSlices;   // selections
    Vec_Int_t             vConcats;  // concatenations
    FILE *                pFile;     // temp file
    Vec_Int_t *           vTokens;   // temp tokens
    int                   pMap[MAX_MAP];  // temp map
    Vec_Int_t *           vMap;      // mapping NameId into wires
    Vec_Int_t *           vDirects;  // direct equivalences
    Vec_Int_t *           vInverses; // inverse equivalences
    Vec_Int_t             vAttrTemp; // temp
    Vec_Int_t             vTemp[TEMP_NUM];  // temp
};

typedef struct Rtl_Ntk_t_  Rtl_Ntk_t;
struct Rtl_Ntk_t_ 
{
    int                   NameId;    // model name
    int                   nInputs;   // word-level inputs
    int                   nOutputs;  // word-level outputs
    Vec_Int_t             vWires;    // wires (name{upto,signed,in,out}+width+offset+number)
    Vec_Int_t             vCells;    // instances ([0]type+[1]name+[2]mod+[3]ins+[4]nattr+[5]nparams+[6]nconns+[7]mark+(attr+params+conns))
    Vec_Int_t             vConns;    // connection pairs
    Vec_Int_t             vStore;    // storage for cells
    Vec_Int_t             vAttrs;    // attributes
    Rtl_Lib_t *           pLib;      // parent
    Vec_Int_t             vOrder;    // topological order
    Vec_Int_t             vLits;     // bit-level view
    Vec_Int_t             vDrivers;  // bit-level view
    Vec_Int_t             vBitTemp;  // storage for bits
    Vec_Int_t             vBitTemp2; // storage for bits
    Gia_Man_t *           pGia;      // derived by bit-blasting
    int                   Slice0;    // first slice
    int                   Slice1;    // last slice
    int                   iCopy;     // place in array
    int                   fRoot;     // denote root network
};

static inline int         Rtl_LibNtkNum( Rtl_Lib_t * pLib )                { return Vec_PtrSize(pLib->vNtks);                  }
static inline Rtl_Ntk_t * Rtl_LibNtk( Rtl_Lib_t * pLib, int i )            { return (Rtl_Ntk_t *)Vec_PtrEntry(pLib->vNtks, i); }
static inline Rtl_Ntk_t * Rtl_LibTop( Rtl_Lib_t * pLib )                   { return Rtl_LibNtk( pLib, Rtl_LibNtkNum(pLib)-1 ); }
static inline char *      Rtl_LibStr( Rtl_Lib_t * pLib, int h )            { return Abc_NamStr(pLib->pManName, h);             }
static inline int         Rtl_LibStrId( Rtl_Lib_t * pLib, char * s )       { return Abc_NamStrFind(pLib->pManName, s);         }

static inline Rtl_Ntk_t * Rtl_NtkModule( Rtl_Ntk_t * p, int i )            { return Rtl_LibNtk( p->pLib, i );                  }

static inline int         Rtl_NtkStrId( Rtl_Ntk_t * p, char * s )          { return Abc_NamStrFind(p->pLib->pManName, s);      }
static inline char *      Rtl_NtkStr( Rtl_Ntk_t * p, int h )               { return Abc_NamStr(p->pLib->pManName, h);          }
static inline char *      Rtl_NtkName( Rtl_Ntk_t * p )                     { return Rtl_NtkStr(p, p->NameId);                  }

static inline FILE *      Rtl_NtkFile( Rtl_Ntk_t * p )                     { return p->pLib->pFile;                            }
static inline int         Rtl_NtkTokId( Rtl_Ntk_t * p, int i )             { return i < Vec_IntSize(p->pLib->vTokens) ? Vec_IntEntry(p->pLib->vTokens, i) : -1;                  }
static inline char *      Rtl_NtkTokStr( Rtl_Ntk_t * p, int i )            { return i < Vec_IntSize(p->pLib->vTokens) ? Rtl_NtkStr(p, Vec_IntEntry(p->pLib->vTokens, i)) : NULL; }
static inline int         Rtl_NtkTokCheck( Rtl_Ntk_t * p, int i, int Tok ) { return i == p->pLib->pMap[Tok];                                    }
static inline int         Rtl_NtkPosCheck( Rtl_Ntk_t * p, int i, int Tok ) { return Vec_IntEntry(p->pLib->vTokens, i) == p->pLib->pMap[Tok];    }

static inline int         Rtl_NtkInputNum( Rtl_Ntk_t * p )                 { return p->nInputs;                                }
static inline int         Rtl_NtkOutputNum( Rtl_Ntk_t * p )                { return p->nOutputs;                               }
static inline int         Rtl_NtkAttrNum( Rtl_Ntk_t * p )                  { return Vec_IntSize(&p->vAttrs)/2;                 }
static inline int         Rtl_NtkWireNum( Rtl_Ntk_t * p )                  { return Vec_IntSize(&p->vWires)/WIRE_NUM;          }
static inline int         Rtl_NtkCellNum( Rtl_Ntk_t * p )                  { return Vec_IntSize(&p->vCells);                   }
static inline int         Rtl_NtkConNum( Rtl_Ntk_t * p )                   { return Vec_IntSize(&p->vConns)/2;                 }
static inline int         Rtl_NtkObjNum( Rtl_Ntk_t * p )                   { return p->nInputs + p->nOutputs + Rtl_NtkCellNum(p) + Rtl_NtkConNum(p); }

static inline int *       Rtl_NtkWire( Rtl_Ntk_t * p, int i )              { return Vec_IntEntryP(&p->vWires, WIRE_NUM*i);                  }
static inline int *       Rtl_NtkCell( Rtl_Ntk_t * p, int i )              { return Vec_IntEntryP(&p->vStore, Vec_IntEntry(&p->vCells, i)); }
static inline int *       Rtl_NtkCon( Rtl_Ntk_t * p, int i )               { return Vec_IntEntryP(&p->vConns, 2*i);                         }

static inline int         Rtl_WireName( Rtl_Ntk_t * p, int i )             { return Vec_IntEntry(&p->vWires, WIRE_NUM*i) >> 4; }
static inline char *      Rtl_WireNameStr( Rtl_Ntk_t * p, int i )          { return Rtl_NtkStr(p, Rtl_WireName(p, i));         }
static inline int         Rtl_WireFirst( Rtl_Ntk_t * p, int i )            { return Vec_IntEntry(&p->vWires, WIRE_NUM*i);      }
static inline int         Rtl_WireWidth( Rtl_Ntk_t * p, int i )            { return Vec_IntEntry(&p->vWires, WIRE_NUM*i+1);    }
static inline int         Rtl_WireOffset( Rtl_Ntk_t * p, int i )           { return Vec_IntEntry(&p->vWires, WIRE_NUM*i+2);    }
static inline int         Rtl_WireNumber( Rtl_Ntk_t * p, int i )           { return Vec_IntEntry(&p->vWires, WIRE_NUM*i+3);    }
static inline int         Rtl_WireBitStart( Rtl_Ntk_t * p, int i )         { return Vec_IntEntry(&p->vWires, WIRE_NUM*i+4);    }
static inline int         Rtl_WireMapNameToId( Rtl_Ntk_t * p, int i )      { return Vec_IntEntry(p->pLib->vMap, i);            }

static inline int         Rtl_CellType( int * pCell )                      { return pCell[0];                                  }
static inline int         Rtl_CellName( int * pCell )                      { return pCell[1];                                  }
static inline int         Rtl_CellModule( int * pCell )                    { return pCell[2];                                  }
static inline int         Rtl_CellInputNum( int * pCell )                  { return pCell[3];                                  }
static inline int         Rtl_CellOutputNum( int * pCell )                 { return pCell[6]-pCell[3];                         }
static inline int         Rtl_CellAttrNum( int * pCell )                   { return pCell[4];                                  }
static inline int         Rtl_CellParamNum( int * pCell )                  { return pCell[5];                                  }
static inline int         Rtl_CellConNum( int * pCell )                    { return pCell[6];                                  }
static inline int         Rtl_CellMark( int * pCell )                      { return pCell[7];                                  }
static inline Rtl_Ntk_t * Rtl_CellNtk( Rtl_Ntk_t * p, int * pCell )        { return Rtl_CellModule(pCell) >= ABC_INFINITY ? Rtl_NtkModule(p, Rtl_CellModule(pCell)-ABC_INFINITY) : NULL; }

static inline char *      Rtl_CellTypeStr( Rtl_Ntk_t * p, int * pCell )    { return Rtl_NtkStr(p, Rtl_CellType(pCell));        }
static inline char *      Rtl_CellNameStr( Rtl_Ntk_t * p, int * pCell )    { return Rtl_NtkStr(p, Rtl_CellName(pCell));        }

static inline int         Rtl_SigIsNone( int s )                           { return (s & 0x3) == 0;                            }
static inline int         Rtl_SigIsConst( int s )                          { return (s & 0x3) == 1;                            }
static inline int         Rtl_SigIsSlice( int s )                          { return (s & 0x3) == 2;                            }
static inline int         Rtl_SigIsConcat( int s )                         { return (s & 0x3) == 3;                            }

#define Rtl_NtkForEachAttr( p, Par, Val, i ) \
    for ( i = 0; i < Rtl_NtkAttrNum(p) && (Par = Vec_IntEntry(&p->vAttrs, 2*i)) && (Val = Vec_IntEntry(&p->vAttrs, 2*i+1)); i++ )
#define Rtl_NtkForEachWire( p, pWire, i ) \
    for ( i = 0; i < Rtl_NtkWireNum(p) && (pWire = Vec_IntEntryP(&p->vWires, WIRE_NUM*i)); i++ )
#define Rtl_NtkForEachCell( p, pCell, i ) \
    for ( i = 0; i < Rtl_NtkCellNum(p) && (pCell = Rtl_NtkCell(p, i)); i++ )
#define Rtl_NtkForEachCon( p, pCon, i ) \
    for ( i = 0; i < Rtl_NtkConNum(p) && (pCon = Vec_IntEntryP(&p->vConns, 2*i)); i++ )

#define Rtl_CellForEachAttr( p, pCell, Par, Val, i ) \
    for ( i = 0; i < pCell[4] && (Par = pCell[CELL_NUM+2*i]) && (Val = pCell[CELL_NUM+2*i+1]); i++ )
#define Rtl_CellForEachParam( p, pCell, Par, Val, i ) \
    for ( i = 0; i < pCell[5] && (Par = pCell[CELL_NUM+2*(pCell[4]+i)]) && (Val = pCell[CELL_NUM+2*(pCell[4]+i)+1]); i++ )
#define Rtl_CellForEachConnect( p, pCell, Par, Val, i ) \
    for ( i = 0; i < pCell[6] && (Par = pCell[CELL_NUM+2*(pCell[4]+pCell[5]+i)]) && (Val = pCell[CELL_NUM+2*(pCell[4]+pCell[5]+i)+1]); i++ )

#define Rtl_CellForEachInput( p, pCell, Par, Val, i ) \
    Rtl_CellForEachConnect( p, pCell, Par, Val, i ) if ( i >= Rtl_CellInputNum(pCell) ) continue; else
#define Rtl_CellForEachOutput( p, pCell, Par, Val, i ) \
    Rtl_CellForEachConnect( p, pCell, Par, Val, i ) if ( i <  Rtl_CellInputNum(pCell) ) continue; else

extern Gia_Man_t * Cec4_ManSimulateTest3( Gia_Man_t * p, int nBTLimit, int fVerbose );
extern int         Abc_NtkFromGiaCollapse( Gia_Man_t * pGia );
extern int         Wln_ReadFindToken( char * pToken, Abc_Nam_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Rtl_Ntk_t * Rtl_NtkAlloc( Rtl_Lib_t * pLib )
{
    Rtl_Ntk_t * p = ABC_CALLOC( Rtl_Ntk_t, 1 );
    Vec_IntGrow( &p->vWires, 4 );
    Vec_IntGrow( &p->vCells, 4 );
    Vec_IntGrow( &p->vConns, 4 );
    Vec_IntGrow( &p->vStore, 8 );
    Vec_IntGrow( &p->vAttrs, 8 );
    Vec_PtrPush( pLib->vNtks, (void *)p );
    p->pLib = pLib;
    return p;
}
void Rtl_NtkFree( Rtl_Ntk_t * p )
{
    Gia_ManStopP( &p->pGia );
    ABC_FREE( p->vWires.pArray );
    ABC_FREE( p->vCells.pArray );
    ABC_FREE( p->vConns.pArray );
    ABC_FREE( p->vStore.pArray );
    ABC_FREE( p->vAttrs.pArray );
    ABC_FREE( p->vOrder.pArray );
    ABC_FREE( p->vLits.pArray );
    ABC_FREE( p->vDrivers.pArray );
    ABC_FREE( p->vBitTemp.pArray );
    ABC_FREE( p->vBitTemp2.pArray );
    ABC_FREE( p );
}
void Rtl_NtkCountPio( Rtl_Ntk_t * p, int Counts[4] )
{
    int i, * pWire;
    Rtl_NtkForEachWire( p, pWire, i )
    {
        if ( pWire[0] & 1 ) // PI
            Counts[0]++, Counts[1] += pWire[1];
        if ( pWire[0] & 2 ) // PO
            Counts[2]++, Counts[3] += pWire[1];
    }
    assert( p->nInputs  == Counts[0] );
    assert( p->nOutputs == Counts[2] );
}
void Rtl_NtkPrintOpers( Rtl_Ntk_t * p )
{
    int i, * pCell, nBlack = 0, nUser = 0, Counts[ABC_OPER_LAST] = {0};
    if ( Rtl_NtkCellNum(p) == 0 )
        return;
    Rtl_NtkForEachCell( p, pCell, i )
        if ( Rtl_CellModule(pCell) < ABC_OPER_LAST )
            Counts[Rtl_CellModule(pCell)]++;
        else if ( Rtl_CellModule(pCell) == ABC_OPER_LAST-1 )
            nBlack++;
        else
            nUser++;
    printf( "There are %d instances in this network:\n", Rtl_NtkCellNum(p) );
    if ( nBlack )
        printf( "  %s (%d)", "blackbox", nBlack );
    if ( nUser )
        printf( "  %s (%d)", "user", nUser );
    for ( i = 0; i < ABC_OPER_LAST; i++ )
        if ( Counts[i] )
            printf( "  %s (%d)", Abc_OperName(i), Counts[i] );
    printf( "\n" );
}
void Rtl_NtkPrintStats( Rtl_Ntk_t * p, int nNameSymbs )
{
    int Counts[4] = {0};     Rtl_NtkCountPio( p, Counts );
    printf( "%*s : ",        nNameSymbs, Rtl_NtkName(p) );
    printf( "PI = %5d (%5d)  ", Counts[0], Counts[1] );
    printf( "PO = %5d (%5d)  ", Counts[2], Counts[3] );
    printf( "Wire = %6d   ", Rtl_NtkWireNum(p) );
    printf( "Cell = %6d   ", Rtl_NtkCellNum(p) );
    printf( "Con = %6d",     Rtl_NtkConNum(p) );
    printf( "\n" );
    //Rtl_NtkPrintOpers( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtl_NtkPrintHieStats( Rtl_Ntk_t * p, int nOffset )
{
    Vec_Int_t * vFound = Vec_IntAlloc( 100 );
    int i, * pCell;
    for ( i = 0; i < 5*(nOffset-1); i++ )
        printf( " " );
    if ( nOffset )
        printf( "|--> " );
    printf( "%s\n", Rtl_NtkName(p) );
    Rtl_NtkForEachCell( p, pCell, i )
        if ( Rtl_CellModule(pCell) >= ABC_INFINITY )
        {
            Rtl_Ntk_t * pModel = Rtl_NtkModule( p, Rtl_CellModule(pCell)-ABC_INFINITY );
            assert( pCell[6] == pModel->nInputs+pModel->nOutputs );
            if ( Vec_IntFind(vFound, pModel->NameId) >= 0 )
                continue;
            Vec_IntPush( vFound, pModel->NameId );
            Rtl_NtkPrintHieStats( pModel, nOffset+1 );
        }
    Vec_IntFree( vFound );
}
void Rtl_LibPrintHieStats( Rtl_Lib_t * p )
{
    Rtl_Ntk_t * pNtk; int i;
    printf( "Hierarchy found in \"%s\":\n", p->pSpec );
    Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
    {
        printf( "\n" );
        printf( "MODULE %d: ", i );
        Rtl_NtkPrintHieStats( pNtk, 0 );  
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Rtl_Lib_t * Rtl_LibAlloc()
{
    Rtl_Lib_t * p = ABC_CALLOC( Rtl_Lib_t, 1 );
    p->vNtks = Vec_PtrAlloc( 100 );
    Vec_IntGrow( &p->vConsts,  1000 );
    Vec_IntGrow( &p->vSlices,  1000 );
    Vec_IntGrow( &p->vConcats, 1000 );
    return p;
}
void Rtl_LibFree( Rtl_Lib_t * p )
{
    Rtl_Ntk_t * pNtk; int i;
    Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
        Rtl_NtkFree( pNtk );    
    ABC_FREE( p->vConsts.pArray );
    ABC_FREE( p->vSlices.pArray );
    ABC_FREE( p->vConcats.pArray );
    ABC_FREE( p->vAttrTemp.pArray );
    for ( i = 0; i < TEMP_NUM; i++ )
        ABC_FREE( p->vTemp[i].pArray );
    Vec_IntFreeP( &p->vMap );
    Vec_IntFreeP( &p->vDirects );
    Vec_IntFreeP( &p->vInverses );
    Vec_IntFreeP( &p->vTokens );
    Abc_NamStop( p->pManName );
    Vec_PtrFree( p->vNtks );
    ABC_FREE( p->pSpec );
    ABC_FREE( p );
}
int Rtl_LibFindModule( Rtl_Lib_t * p, int NameId )
{
    Rtl_Ntk_t * pNtk; int i;
    Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
        if ( pNtk->NameId == NameId )
            return i;
    return -1;
}
int Rtl_LibFindModule2( Rtl_Lib_t * p, int NameId, int iNtk0 )
{
    char * pName = Rtl_LibStr( p, NameId );
    Rtl_Ntk_t * pNtk0 = Rtl_LibNtk( p, iNtk0 );
    Rtl_Ntk_t * pNtk; int i;
    int Counts0[4] = {0};  Rtl_NtkCountPio( pNtk0, Counts0 );
    Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
        if ( strstr(Rtl_NtkName(pNtk), pName+1) )
        {
            int Counts[4] = {0};  Rtl_NtkCountPio( pNtk, Counts );
            if ( Counts[1] == Counts0[1] && Counts[3] == Counts0[3] )
                return i;
        }
    return -1;
}
int Rtl_LibFindTwoModules( Rtl_Lib_t * p, int Name1, int Name2 )
{
    int iNtk1 = Rtl_LibFindModule( p, Name1 );
    if ( Name2 == -1 )
        return (iNtk1 << 16) | iNtk1;
    else if ( iNtk1 == -1 )
        return -1;
    else
    {
        int Counts1[4] = {0}, Counts2[4] = {0};
        int iNtk2 = Rtl_LibFindModule( p, Name2 );
        if ( iNtk2 == -1 )
            return -1;
        else
        {
            Rtl_Ntk_t * pNtk1 = Rtl_LibNtk( p, iNtk1 );
            Rtl_Ntk_t * pNtk2 = Rtl_LibNtk( p, iNtk2 );
            Rtl_NtkCountPio( pNtk1, Counts1 );
            Rtl_NtkCountPio( pNtk2, Counts2 );
            if ( Counts1[1] != Counts2[1] || Counts1[3] != Counts2[3] )
                iNtk1 = Rtl_LibFindModule2( p, Name1, iNtk2 );
            return (iNtk1 << 16) | iNtk2;
        }
    }
}
void Rtl_LibPrintStats( Rtl_Lib_t * p )
{
    Rtl_Ntk_t * pNtk; int i, nSymbs = 0;
    printf( "Modules found in \"%s\":\n", p->pSpec );
    Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
        nSymbs = Abc_MaxInt( nSymbs, strlen(Rtl_NtkName(pNtk)) );
    Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
        Rtl_NtkPrintStats( pNtk, nSymbs + 2 );  
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
typedef enum {
    RTL_NONE = 0,  // 0:  unused
    RTL_MODULE,    // 1:  "module"
    RTL_END,       // 2:  "end"
    RTL_INPUT,     // 3:  "input"
    RTL_OUTPUT,    // 4:  "output"
    RTL_INOUT,     // 5:  "inout"
    RTL_UPTO,      // 6:  "upto"
    RTL_SIGNED,    // 7:  "signed"
    RTL_OFFSET,    // 8:  "offset"
    RTL_PARAMETER, // 9:  "parameter"
    RTL_WIRE,      // 10: "wire"
    RTL_CONNECT,   // 11: "connect"
    RTL_CELL,      // 12: "cell"
    RTL_WIDTH,     // 13: "width"
    RTL_ATTRIBUTE, // 14: "attribute"
    RTL_UNUSED     // 15: unused
} Rtl_Type_t; 

static inline char * Rtl_Num2Name( int i )
{
    if ( i == 1  )  return "module";
    if ( i == 2  )  return "end";
    if ( i == 3  )  return "input";
    if ( i == 4  )  return "output";
    if ( i == 5  )  return "inout";
    if ( i == 6  )  return "upto";
    if ( i == 7  )  return "signed";
    if ( i == 8  )  return "offset";
    if ( i == 9  )  return "parameter";
    if ( i == 10 )  return "wire";
    if ( i == 11 )  return "connect";
    if ( i == 12 )  return "cell";
    if ( i == 13 )  return "width";
    if ( i == 14 )  return "attribute";
    return NULL;
}

static inline void Rtl_LibDeriveMap( Rtl_Lib_t * p )
{
    int i;
    p->pMap[0] = -1;
    for ( i = 1; i < RTL_UNUSED; i++ )
        p->pMap[i] = Abc_NamStrFind( p->pManName, Rtl_Num2Name(i) );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rtl_LibReadType( char * pType )
{
    if ( !strcmp(pType, "$not") )         return ABC_OPER_BIT_INV;       // Y = ~A       $not            
    if ( !strcmp(pType, "$pos") )         return ABC_OPER_BIT_BUF;       // Y = +A       $pos            
    if ( !strcmp(pType, "$neg") )         return ABC_OPER_ARI_MIN;       // Y = -A       $neg            
    if ( !strcmp(pType, "$reduce_and") )  return ABC_OPER_RED_AND;       // Y = &A       $reduce_and     
    if ( !strcmp(pType, "$reduce_or") )   return ABC_OPER_RED_OR;        // Y = |A       $reduce_or      
    if ( !strcmp(pType, "$reduce_xor") )  return ABC_OPER_RED_XOR;       // Y = ^A       $reduce_xor     
    if ( !strcmp(pType, "$reduce_xnor") ) return ABC_OPER_RED_NXOR;      // Y = ~^A      $reduce_xnor   
    if ( !strcmp(pType, "$reduce_bool") ) return ABC_OPER_RED_OR;        // Y = |A       $reduce_bool    
    if ( !strcmp(pType, "$logic_not") )   return ABC_OPER_LOGIC_NOT;     // Y = !A       $logic_not      

    if ( !strcmp(pType, "$and") )         return ABC_OPER_BIT_AND;       // Y = A & B    $and         
    if ( !strcmp(pType, "$or") )          return ABC_OPER_BIT_OR;        // Y = A | B    $or          
    if ( !strcmp(pType, "$xor") )         return ABC_OPER_BIT_XOR;       // Y = A ^ B    $xor         
    if ( !strcmp(pType, "$xnor") )        return ABC_OPER_BIT_NXOR;      // Y = A ~^ B   $xnor    
    
    if ( !strcmp(pType, "$shl") )         return ABC_OPER_SHIFT_L;       // Y = A << B   $shl        
    if ( !strcmp(pType, "$shr") )         return ABC_OPER_SHIFT_R;       // Y = A >> B   $shr        
    if ( !strcmp(pType, "$sshl") )        return ABC_OPER_SHIFT_LA;      // Y = A <<< B  $sshl      
    if ( !strcmp(pType, "$sshr") )        return ABC_OPER_SHIFT_RA;      // Y = A >>> B  $sshr      

    if ( !strcmp(pType, "$shiftx") )      return ABC_OPER_SHIFT_R;       // Y = A << B   $shl     <== temporary   

    if ( !strcmp(pType, "$logic_and") )   return ABC_OPER_LOGIC_AND;     // Y = A && B   $logic_and  
    if ( !strcmp(pType, "$logic_or") )    return ABC_OPER_LOGIC_OR;      // Y = A || B   $logic_or  
    
    if ( !strcmp(pType, "$lt") )          return ABC_OPER_COMP_LESS;     // Y = A < B    $lt          
    if ( !strcmp(pType, "$le") )          return ABC_OPER_COMP_LESSEQU;  // Y = A <= B   $le         
    if ( !strcmp(pType, "$ge") )          return ABC_OPER_COMP_MOREEQU;  // Y = A >= B   $ge           
    if ( !strcmp(pType, "$gt") )          return ABC_OPER_COMP_MORE;     // Y = A > B    $gt        
    if ( !strcmp(pType, "$eq") )          return ABC_OPER_COMP_EQU;      // Y = A == B   $eq         
    if ( !strcmp(pType, "$ne") )          return ABC_OPER_COMP_NOTEQU;   // Y = A != B   $ne         
    if ( !strcmp(pType, "$eqx") )         return ABC_OPER_COMP_EQU;      // Y = A === B  $eqx       
    if ( !strcmp(pType, "$nex") )         return ABC_OPER_COMP_NOTEQU;   // Y = A !== B  $nex       
    
    if ( !strcmp(pType, "$add") )         return ABC_OPER_ARI_ADD;       // Y = A + B    $add         
    if ( !strcmp(pType, "$sub") )         return ABC_OPER_ARI_SUB;       // Y = A - B    $sub         
    if ( !strcmp(pType, "$mul") )         return ABC_OPER_ARI_MUL;       // Y = A * B    $mul         
    if ( !strcmp(pType, "$div") )         return ABC_OPER_ARI_DIV;       // Y = A / B    $div         
    if ( !strcmp(pType, "$mod") )         return ABC_OPER_ARI_MOD;       // Y = A % B    $mod         
    if ( !strcmp(pType, "$pow") )         return ABC_OPER_ARI_POW;       // Y = A ** B   $pow        

    if ( !strcmp(pType, "$modfoor") )     return ABC_OPER_NONE;          // [N/A] $modfoor         
    if ( !strcmp(pType, "$divfloor") )    return ABC_OPER_NONE;          // [N/A] $divfloor        

    if ( !strcmp(pType, "$mux") )         return ABC_OPER_SEL_NMUX;      // $mux                   
    if ( !strcmp(pType, "$pmux") )        return ABC_OPER_SEL_SEL;       // $pmux                  
                                                               
    if ( !strcmp(pType, "$dff") )         return ABC_OPER_DFF;
    if ( !strcmp(pType, "$adff") )        return ABC_OPER_DFF;
    if ( !strcmp(pType, "$sdff") )        return ABC_OPER_DFF;
    assert( 0 );                                               
    return -1;                                                 
}                                                                         
int Rtl_NtkReadType( Rtl_Ntk_t * p, int Type )                                                                         
{                                                                         
    extern int Rtl_LibFindModule( Rtl_Lib_t * p, int NameId );
    char * pType = Rtl_NtkStr( p, Type );                   
    if ( pType[0] == '$' && strncmp(pType,"$paramod",strlen("$paramod")) )
        return Rtl_LibReadType( pType );                    
    return ABC_INFINITY + Rtl_LibFindModule( p->pLib, Type );
}                                                           
                                                            
/**Function*************************************************************
                                                            
  Synopsis    [There is no need to normalize ranges in Yosys.]
                                                            
  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rtl_NtkRangeWires( Rtl_Ntk_t * p )
{
    int i, * pWire, nBits = 0;
    Rtl_NtkForEachWire( p, pWire, i )
    {
        //printf( "%s -> %d\n", Rtl_WireNameStr(p, i), nBits );
        pWire[4] = nBits, nBits += Rtl_WireWidth(p, i);
    }
    return nBits;
}
void Rtl_NtkMapWires( Rtl_Ntk_t * p, int fUnmap )
{
    int i, Value;
    assert( Vec_IntSize(p->pLib->vMap) == Abc_NamObjNumMax(p->pLib->pManName) );
    for ( i = 0; i < Rtl_NtkWireNum(p); i++ )
    {
        int NameId = Rtl_WireName( p, i );
        assert( Vec_IntEntry(p->pLib->vMap, NameId) == (fUnmap ? i : -1) );
        Vec_IntWriteEntry( p->pLib->vMap, NameId, fUnmap ? -1 : i );
    }
    if ( fUnmap )
        Vec_IntForEachEntry( p->pLib->vMap, Value, i )
            assert( Value == -1 );
}
void Rtl_NtkNormRanges( Rtl_Ntk_t * p )
{
    int i, * pWire;
    Rtl_NtkMapWires( p, 0 );
    for ( i = p->Slice0; i < p->Slice1; i += 3 )
    {
        int NameId = Vec_IntEntry( &p->pLib->vSlices, i );
        int Left   = Vec_IntEntry( &p->pLib->vSlices, i+1 );
        int Right  = Vec_IntEntry( &p->pLib->vSlices, i+2 );
        int Wire   = Rtl_WireMapNameToId( p, NameId );
        int Offset = Rtl_WireOffset( p, Wire );
        int First  = Rtl_WireFirst( p, Wire );
        assert( First >> 4 == NameId );
        if ( Offset )
        {
            Left  -= Offset;
            Right -= Offset;
        }
        if ( First & 8 ) // upto
        {
            Vec_IntWriteEntry( &p->pLib->vSlices, i+1, Right );
            Vec_IntWriteEntry( &p->pLib->vSlices, i+2, Left );
        }
    }
    Rtl_NtkForEachWire( p, pWire, i )
    {
        Vec_IntWriteEntry( &p->vWires, WIRE_NUM*i+0, Rtl_WireFirst(p, i) & ~0x8 ); // upto
        Vec_IntWriteEntry( &p->vWires, WIRE_NUM*i+2, 0 ); // offset
    }
    Rtl_NtkMapWires( p, 1 );
}
void Rtl_LibNormRanges( Rtl_Lib_t * pLib )
{
    Rtl_Ntk_t * p; int i;
    if ( pLib->vMap == NULL )
        pLib->vMap = Vec_IntStartFull( Abc_NamObjNumMax(pLib->pManName) );
    Vec_PtrForEachEntry( Rtl_Ntk_t *, pLib->vNtks, p, i )
        Rtl_NtkNormRanges( p );    
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Rlt_NtkFindIOPerm( Rtl_Ntk_t * p )
{
    Vec_Int_t * vCost = Vec_IntAlloc( 100 );
    int i, * pWire, * pPerm = NULL, Count = 0;
    Rtl_NtkForEachWire( p, pWire, i )
    {
        int First  = Rtl_WireFirst( p, i );
        int Number = Rtl_WireNumber( p, i );
        int fIsPi  = (int)((First & 1) > 0);
        int fIsPo  = (int)((First & 2) > 0);
        assert( (fIsPi || fIsPo) == (Number > 0) );
        if ( fIsPi || fIsPo )
            Vec_IntPush( vCost, fIsPo*ABC_INFINITY + Number );
        else
            Vec_IntPush( vCost, 2*ABC_INFINITY + Count++ );
    }
    pPerm = Abc_MergeSortCost( Vec_IntArray(vCost), Vec_IntSize(vCost) );
    Vec_IntFree( vCost );
    return pPerm;
}
void Rtl_NtkOrderWires( Rtl_Ntk_t * p )
{
    Vec_Int_t * vTemp = Vec_IntAlloc( Vec_IntSize(&p->vWires) );
    int i, k, * pWire, * pPerm = Rlt_NtkFindIOPerm( p );
    Rtl_NtkForEachWire( p, pWire, i )
    {
        pWire = Vec_IntEntryP( &p->vWires, WIRE_NUM*pPerm[i] );
        for ( k = 0; k < WIRE_NUM; k++ )
            Vec_IntPush( vTemp, pWire[k] );
    }
    ABC_FREE( pPerm );
    assert( Vec_IntSize(&p->vWires) == Vec_IntSize(vTemp) );
    ABC_SWAP( Vec_Int_t, p->vWires, *vTemp ); 
    Vec_IntFree( vTemp );
}
void Rtl_LibUpdateInstances( Rtl_Ntk_t * p )
{
    Vec_Int_t * vMap  = p->pLib->vMap;
    Vec_Int_t * vTemp = &p->pLib->vTemp[2];
    int i, k, Par, Val, * pCell, Value;
    Rtl_NtkForEachCell( p, pCell, i )
        if ( Rtl_CellModule(pCell) >= ABC_INFINITY )
        {
            Rtl_Ntk_t * pModel = Rtl_NtkModule( p, Rtl_CellModule(pCell)-ABC_INFINITY );
            assert( pCell[6] == pModel->nInputs+pModel->nOutputs );
            Rtl_CellForEachConnect( p, pCell, Par, Val, k )
                Vec_IntWriteEntry( vMap, Par >> 2, k );
            Vec_IntClear( vTemp );
            for ( k = 0; k < pCell[6]; k++ )
            {
                int Perm = Vec_IntEntry( vMap, Rtl_WireName(pModel, k) );
                int Par = pCell[CELL_NUM+2*(pCell[4]+pCell[5]+Perm)];
                int Val = pCell[CELL_NUM+2*(pCell[4]+pCell[5]+Perm)+1];
                assert( (Par >> 2) == Rtl_WireName(pModel, k) );
                Vec_IntWriteEntry( vMap, Par >> 2, -1 );
                Vec_IntPushTwo( vTemp, Par, Val );
                assert( Perm >= 0 );
            }
            memcpy( pCell+CELL_NUM+2*(pCell[4]+pCell[5]), Vec_IntArray(vTemp), sizeof(int)*Vec_IntSize(vTemp) );
        }
    Vec_IntForEachEntry( p->pLib->vMap, Value, i )
        assert( Value == -1 );
}
void Rtl_LibOrderWires( Rtl_Lib_t * pLib )
{
    Rtl_Ntk_t * p; int i;
    if ( pLib->vMap == NULL )
        pLib->vMap = Vec_IntStartFull( Abc_NamObjNumMax(pLib->pManName) );
    Vec_PtrForEachEntry( Rtl_Ntk_t *, pLib->vNtks, p, i )
        Rtl_NtkOrderWires( p );    
    Vec_PtrForEachEntry( Rtl_Ntk_t *, pLib->vNtks, p, i )
        Rtl_LibUpdateInstances( p );    
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
extern int Rtl_NtkCountSignalRange( Rtl_Ntk_t * p, int Sig );

int Rtl_NtkCountWireRange( Rtl_Ntk_t * p, int NameId )
{
    int Wire  = Rtl_WireMapNameToId( p, NameId );
    int Width = Rtl_WireWidth( p, Wire );
    return Width;
}
int Rtl_NtkCountSliceRange( Rtl_Ntk_t * p, int * pSlice )
{
    return pSlice[1] - pSlice[2] + 1;
}
int Rtl_NtkCountConcatRange( Rtl_Ntk_t * p, int * pConcat )
{
    int i, nBits = 0;
    for ( i = 1; i <= pConcat[0]; i++ )
        nBits += Rtl_NtkCountSignalRange( p, pConcat[i] );
    return nBits;
}
int Rtl_NtkCountSignalRange( Rtl_Ntk_t * p, int Sig )
{
    if ( Rtl_SigIsNone(Sig) )
        return Rtl_NtkCountWireRange( p, Sig >> 2 );
    if ( Rtl_SigIsSlice(Sig) )
        return Rtl_NtkCountSliceRange( p, Vec_IntEntryP(&p->pLib->vSlices, Sig >> 2) );
    if ( Rtl_SigIsConcat(Sig) )
        return Rtl_NtkCountConcatRange( p, Vec_IntEntryP(&p->pLib->vConcats, Sig >> 2) );
    if ( Rtl_SigIsConst(Sig) )
        assert( 0 );
    return ABC_INFINITY;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
extern int Rtl_NtkCheckSignalRange( Rtl_Ntk_t * p, int Sig );

int Rtl_NtkCheckWireRange( Rtl_Ntk_t * p, int NameId, int Left, int Right )
{
    int Wire  = Rtl_WireMapNameToId( p, NameId );
    int First = Rtl_WireBitStart( p, Wire );
    int Width = Rtl_WireWidth( p, Wire ), i;
    Left  = Left  == -1 ? Width-1 :  Left;
    Right = Right == -1 ? 0       : Right;
    assert ( Right <= Left && Right >= 0 );
    for ( i = Right; i <= Left; i++ )
        if ( Vec_IntEntry(&p->vLits, First+i) == -1 )
            return 0;
    return 1;
}
int Rtl_NtkCheckSliceRange( Rtl_Ntk_t * p, int * pSlice )
{
    return Rtl_NtkCheckWireRange( p, pSlice[0], pSlice[1], pSlice[2] );
}
int Rtl_NtkCheckConcatRange( Rtl_Ntk_t * p, int * pConcat )
{
    int i;
    for ( i = 1; i <= pConcat[0]; i++ )
        if ( !Rtl_NtkCheckSignalRange( p, pConcat[i] ) )
            return 0;
    return 1;
}
int Rtl_NtkCheckSignalRange( Rtl_Ntk_t * p, int Sig )
{
    if ( Rtl_SigIsNone(Sig) )
        return Rtl_NtkCheckWireRange( p, Sig >> 2, -1, -1 );
    else if ( Rtl_SigIsConst(Sig) )
        return 1;
    else if ( Rtl_SigIsSlice(Sig) )
        return Rtl_NtkCheckSliceRange( p, Vec_IntEntryP(&p->pLib->vSlices, Sig >> 2) );
    else if ( Rtl_SigIsConcat(Sig) )
        return Rtl_NtkCheckConcatRange( p, Vec_IntEntryP(&p->pLib->vConcats, Sig >> 2) );
    else assert( 0 );
    return -1;
}


extern void Rtl_NtkSetSignalRange( Rtl_Ntk_t * p, int Sig, int Value );

void Rtl_NtkSetWireRange( Rtl_Ntk_t * p, int NameId, int Left, int Right, int Value )
{
    //char * pName = Rtl_NtkStr( p, NameId );
    int Wire  = Rtl_WireMapNameToId( p, NameId );
    int First = Rtl_WireBitStart( p, Wire );
    int Width = Rtl_WireWidth( p, Wire ), i;
    Left  = Left  == -1 ? Width-1 :  Left;
    Right = Right == -1 ? 0       : Right;
    assert ( Right <= Left && Right >= 0 );
    for ( i = Right; i <= Left; i++ )
    {
        assert( Vec_IntEntry(&p->vLits, First+i) == -1 );
        Vec_IntWriteEntry(&p->vLits, First+i, Value );
    }
    //printf( "Finished setting wire %s\n", Rtl_NtkStr(p, NameId) );
}
void Rtl_NtkSetSliceRange( Rtl_Ntk_t * p, int * pSlice, int Value )
{
    Rtl_NtkSetWireRange( p, pSlice[0], pSlice[1], pSlice[2], Value );
}
void Rtl_NtkSetConcatRange( Rtl_Ntk_t * p, int * pConcat, int Value )
{
    int i;
    for ( i = 1; i <= pConcat[0]; i++ )
        Rtl_NtkSetSignalRange( p, pConcat[i], Value );
}
void Rtl_NtkSetSignalRange( Rtl_Ntk_t * p, int Sig, int Value )
{
    if ( Rtl_SigIsNone(Sig) )
        Rtl_NtkSetWireRange( p, Sig >> 2, -1, -1, Value );
    else if ( Rtl_SigIsSlice(Sig) )
        Rtl_NtkSetSliceRange( p, Vec_IntEntryP(&p->pLib->vSlices, Sig >> 2), Value );
    else if ( Rtl_SigIsConcat(Sig) )
        Rtl_NtkSetConcatRange( p, Vec_IntEntryP(&p->pLib->vConcats, Sig >> 2), Value );
    else if ( Rtl_SigIsConst(Sig) )
        assert( 0 );
}


void Rtl_NtkInitInputs( Rtl_Ntk_t * p )
{
    int b, i;
    for ( i = 0; i < p->nInputs; i++ )
    {
        int First = Rtl_WireBitStart( p, i );
        int Width = Rtl_WireWidth( p, i );
        for ( b = 0; b < Width; b++ )
        {
            assert( Vec_IntEntry(&p->vLits, First+b) == -1 );
            Vec_IntWriteEntry( &p->vLits, First+b, Vec_IntSize(&p->vOrder) );
        }
        Vec_IntPush( &p->vOrder, i );
        //printf( "Finished setting input %s\n", Rtl_WireNameStr(p, i) );
    }
}
Vec_Int_t * Rtl_NtkCollectOutputs( Rtl_Ntk_t * p )
{
    //char * pNtkName = Rtl_NtkName(p);
    int b, i;
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    for ( i = 0; i < p->nOutputs; i++ )
    {
        //char * pName = Rtl_WireNameStr(p, p->nInputs + i);
        int First = Rtl_WireBitStart( p, p->nInputs + i );
        int Width = Rtl_WireWidth( p, p->nInputs + i );
        for ( b = 0; b < Width; b++ )
        {
            assert( Vec_IntEntry(&p->vLits, First+b) != -1 );
            Vec_IntPush( vRes, Vec_IntEntry(&p->vLits, First+b) );
        }
    }
    return vRes;
}
int Rtl_NtkReviewCells( Rtl_Ntk_t * p )
{
    int i, k, Par, Val, * pCell, RetValue = 0;
    Rtl_NtkForEachCell( p, pCell, i )
    {
        if ( pCell[7] )
            continue;
        Rtl_CellForEachInput( p, pCell, Par, Val, k )
            if ( !Rtl_NtkCheckSignalRange( p, Val ) )
                break;
        if ( k < Rtl_CellInputNum(pCell) )
            continue;
        Rtl_CellForEachOutput( p, pCell, Par, Val, k )
            Rtl_NtkSetSignalRange( p, Val, Vec_IntSize(&p->vOrder) );
        Vec_IntPush( &p->vOrder, p->nInputs + i );
        pCell[7] = 1;
        RetValue = 1;
        //printf( "Setting cell %s as propagated.\n", Rtl_CellNameStr(p, pCell) );
    }
    return RetValue;
}
int Rtl_NtkReviewConnections( Rtl_Ntk_t * p )
{
    int i, * pCon, RetValue = 0;
    Rtl_NtkForEachCon( p, pCon, i )
    {
        int Status0 = Rtl_NtkCheckSignalRange( p, pCon[0] );
        int Status1 = Rtl_NtkCheckSignalRange( p, pCon[1] );
        if ( Status0 == Status1 )
            continue;
        if ( !Status0 && Status1 )
            ABC_SWAP( int, pCon[0], pCon[1] )
        Rtl_NtkSetSignalRange( p, pCon[1], Vec_IntSize(&p->vOrder) );
        Vec_IntPush( &p->vOrder, p->nInputs + Rtl_NtkCellNum(p) + i );
        RetValue = 1;
    }
    return RetValue;
}
void Rtl_NtkPrintCellOrder( Rtl_Ntk_t * p )
{
    int i, iCell;
    Vec_IntForEachEntry( &p->vOrder, iCell, i )
    {
        printf( "%4d :  ", i );
        printf( "Cell %4d  ", iCell );
        if ( iCell < p->nInputs )
            printf( "Type  Input " );
        else if ( iCell < p->nInputs + Rtl_NtkCellNum(p) )
        {
            int * pCell = Rtl_NtkCell( p, iCell - p->nInputs );
            printf( "Type  %4d  ", Rtl_CellType(pCell) );
            printf( "%16s ",       Rtl_CellTypeStr(p, pCell) );
            printf( "%16s ",       Rtl_CellNameStr(p, pCell) );
        }
        else
            printf( "Type  Connection " );
        printf( "\n" );
    }
}
void Rtl_NtkPrintUnusedCells( Rtl_Ntk_t * p )
{
    int i, * pCell;
    printf( "\n*** Printing unused cells:\n" );
    Rtl_NtkForEachCell( p, pCell, i )
    {
        if ( pCell[7] )
            continue;
        printf( "Unused cell %s           %s\n", Rtl_CellTypeStr(p, pCell), Rtl_CellNameStr(p, pCell) );
    }
    printf( "\n" );
}
void Rtl_NtkOrderCells( Rtl_Ntk_t * p )
{
    Vec_Int_t * vRes;
    int nBits = Rtl_NtkRangeWires( p );
    Vec_IntFill( &p->vLits, nBits, -1 );

    Vec_IntClear( &p->vOrder );
    Vec_IntGrow( &p->vOrder, Rtl_NtkObjNum(p) );
    Rtl_NtkInitInputs( p );

    Rtl_NtkMapWires( p, 0 );
//Vec_IntPrint(&p->vLits);

    Rtl_NtkReviewConnections( p );
    while ( Rtl_NtkReviewCells(p) | Rtl_NtkReviewConnections(p) );
    Rtl_NtkMapWires( p, 1 );

    vRes = Rtl_NtkCollectOutputs( p );
    Vec_IntFree( vRes );

    //Rtl_NtkPrintCellOrder( p );
}
void Rtl_LibOrderCells( Rtl_Lib_t * pLib )
{
    Rtl_Ntk_t * p; int i;
    if ( pLib->vMap == NULL )
        pLib->vMap = Vec_IntStartFull( Abc_NamObjNumMax(pLib->pManName) );
    assert( Vec_IntSize(pLib->vMap) == Abc_NamObjNumMax(pLib->pManName) );
    Vec_PtrForEachEntry( Rtl_Ntk_t *, pLib->vNtks, p, i )
        Rtl_NtkOrderCells( p );    
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtl_TokenUnspace( char * p )
{
    int i, Length = strlen(p), Quote = 0;
    for ( i = 0; i < Length; i++ )
        if ( p[i] == '\"' )
            Quote ^= 1;
        else if ( Quote && p[i] == ' ' )
            p[i] = '\"';
}
void Rtl_TokenRespace( char * p )
{
    int i, Length = strlen(p);
    assert( p[0] == '\"' && p[Length-1] == '\"' );
    for ( i = 1; i < Length-1; i++ )
        if ( p[i] == '\"' )
            p[i] = ' ';
}
Vec_Int_t * Rtl_NtkReadFile( char * pFileName, Abc_Nam_t * p )
{
    Vec_Int_t * vTokens;
    char * pTemp, * pBuffer; 
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return NULL;
    }
    pBuffer = ABC_ALLOC( char, MAX_LINE );
    Abc_NamStrFindOrAdd( p, "module", NULL );
    assert( Abc_NamObjNumMax(p) == 2 );
    vTokens = Vec_IntAlloc( 1000 );
    while ( fgets( pBuffer, MAX_LINE, pFile ) != NULL )
    {
        if ( pBuffer[0] == '#' )
            continue;
        Rtl_TokenUnspace( pBuffer );
        pTemp = strtok( pBuffer, " \t\r\n" );
        if ( pTemp == NULL )
            continue;
        while ( pTemp )
        {
            if ( *pTemp == '\"' )  Rtl_TokenRespace( pTemp );
            Vec_IntPush( vTokens, Abc_NamStrFindOrAdd(p, pTemp, NULL) );
            pTemp = strtok( NULL, " \t\r\n" );
        }
        Vec_IntPush( vTokens, -1 );
    }
    ABC_FREE( pBuffer );
    fclose( pFile );
    return vTokens;
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
extern void Rtl_NtkPrintSig( Rtl_Ntk_t * p, int Sig );

void Rtl_NtkPrintConst( Rtl_Ntk_t * p, int * pConst )
{
    int i;
    if ( pConst[0] == -1 )
    {
        fprintf( Rtl_NtkFile(p), " %d", pConst[1] );
        return;
    }
    fprintf( Rtl_NtkFile(p), " %d\'", pConst[0] );
    for ( i = pConst[0] - 1; i >= 0; i-- )
        fprintf( Rtl_NtkFile(p), "%d", Abc_InfoHasBit((unsigned *)pConst+1,i) );
}
void Rtl_NtkPrintSlice( Rtl_Ntk_t * p, int * pSlice )
{
    fprintf( Rtl_NtkFile(p), " %s", Rtl_NtkStr(p, pSlice[0]) );
    if ( pSlice[1] == pSlice[2] )
        fprintf( Rtl_NtkFile(p), " [%d]", pSlice[1] );
    else
        fprintf( Rtl_NtkFile(p), " [%d:%d]", pSlice[1], pSlice[2] );
}
void Rtl_NtkPrintConcat( Rtl_Ntk_t * p, int * pConcat )
{
    int i;
    fprintf( Rtl_NtkFile(p), " {" );
    for ( i = 1; i <= pConcat[0]; i++ )
        Rtl_NtkPrintSig( p, pConcat[i] );
    fprintf( Rtl_NtkFile(p), " }" );
}
void Rtl_NtkPrintSig( Rtl_Ntk_t * p, int Sig )
{
    if ( Rtl_SigIsNone(Sig) )
        fprintf( Rtl_NtkFile(p), " %s", Rtl_NtkStr(p, Sig >> 2) );
    else if ( Rtl_SigIsConst(Sig) )
        Rtl_NtkPrintConst( p, Vec_IntEntryP(&p->pLib->vConsts, Sig >> 2) );
    else if ( Rtl_SigIsSlice(Sig) )
        Rtl_NtkPrintSlice( p, Vec_IntEntryP(&p->pLib->vSlices, Sig >> 2) );
    else if ( Rtl_SigIsConcat(Sig) )
        Rtl_NtkPrintConcat( p, Vec_IntEntryP(&p->pLib->vConcats, Sig >> 2) );
    else assert( 0 );
}
void Rtl_NtkPrintWire( Rtl_Ntk_t * p, int * pWire )
{
    fprintf( Rtl_NtkFile(p), "  wire" );
    if ( pWire[1] != 1 )  fprintf( Rtl_NtkFile(p), " width %d",  pWire[1] );
    if ( pWire[2] != 0 )  fprintf( Rtl_NtkFile(p), " offset %d", pWire[2] );
    if ( pWire[0] & 8 )   fprintf( Rtl_NtkFile(p), " upto" );
    if ( pWire[0] & 1 )   fprintf( Rtl_NtkFile(p), " input %d",  pWire[3] );
    if ( pWire[0] & 2 )   fprintf( Rtl_NtkFile(p), " output %d", pWire[3] );
    if ( pWire[0] & 4 )   fprintf( Rtl_NtkFile(p), " signed" );
    fprintf( Rtl_NtkFile(p), " %s\n", Rtl_NtkStr(p, pWire[0] >> 4) );
}
void Rtl_NtkPrintCell( Rtl_Ntk_t * p, int * pCell )
{
    int i, Par, Val;
    Rtl_CellForEachAttr( p, pCell, Par, Val, i )  {
        fprintf( Rtl_NtkFile(p), "  attribute %s %s\n", Rtl_NtkStr(p, Par), Rtl_NtkStr(p, Val) );  }
        fprintf( Rtl_NtkFile(p), "  cell %s %s\n", Rtl_NtkStr(p, Rtl_CellType(pCell)), Rtl_NtkStr(p, pCell[1]) );
    Rtl_CellForEachParam( p, pCell, Par, Val, i )
        fprintf( Rtl_NtkFile(p), "    parameter" ), Rtl_NtkPrintSig(p, Par), Rtl_NtkPrintSig(p, Val), printf( "\n" );
    Rtl_CellForEachConnect( p, pCell, Par, Val, i ) {
        fprintf( Rtl_NtkFile(p), "    connect" ), Rtl_NtkPrintSig(p, Par), Rtl_NtkPrintSig(p, Val), printf( "\n" ); }
        fprintf( Rtl_NtkFile(p), "  end\n" );
}
void Rtl_NtkPrintConnection( Rtl_Ntk_t * p, int * pCon )
{
    fprintf( Rtl_NtkFile(p), "  connect" );
    Rtl_NtkPrintSig( p, pCon[0] );
    Rtl_NtkPrintSig( p, pCon[1] );
    fprintf( Rtl_NtkFile(p), "\n" );
}
void Rtl_NtkPrint( Rtl_Ntk_t * p )
{
    int i, Par, Val, * pWire, * pCell, * pCon;
    fprintf( Rtl_NtkFile(p), "\n" );
    Rtl_NtkForEachAttr( p, Par, Val, i )
        fprintf( Rtl_NtkFile(p), "attribute %s %s\n", Rtl_NtkStr(p, Par), Rtl_NtkStr(p, Val) );
    fprintf( Rtl_NtkFile(p), "module %s\n", Rtl_NtkName(p) );
    Rtl_NtkForEachWire( p, pWire, i )
        Rtl_NtkPrintWire( p, pWire );
    Rtl_NtkForEachCell( p, pCell, i )
        Rtl_NtkPrintCell( p, pCell );
    Rtl_NtkForEachCon( p, pCon, i )
        Rtl_NtkPrintConnection( p, pCon );
    fprintf( Rtl_NtkFile(p), "end\n" );
}
void Rtl_LibPrint( char * pFileName, Rtl_Lib_t * p )
{
    p->pFile = pFileName ? fopen( pFileName, "wb" ) : stdout;
    if ( p->pFile == NULL )
    {
        printf( "Cannot open output file \"%s\".\n", pFileName );
        return;
    }
    else
    {
        Rtl_Ntk_t * pNtk; int i;
        fprintf( p->pFile, "\n" );
        fprintf( p->pFile, "# Generated by ABC on %s\n", Extra_TimeStamp() );
        Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
            Rtl_NtkPrint( pNtk );    
        if ( p->pFile != stdout )
            fclose( p->pFile );
        p->pFile = NULL;
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
extern int Rtl_NtkReadSig( Rtl_Ntk_t * p, int * pPos );

int Rtl_NtkReadConst( Rtl_Ntk_t * p, char * pConst )
{
    Vec_Int_t * vConst = &p->pLib->vConsts;
    int RetVal = Vec_IntSize( vConst );
    int Width  = atoi( pConst );
    assert( pConst[0] >= '0' && pConst[0] <= '9' );
    if ( strstr(pConst, "\'") )
    {
        int Length = strlen(pConst);
        int nWords = (Width + 31) / 32;
        int i, * pArray;
        Vec_IntPush( vConst, Width );
        Vec_IntFillExtra( vConst, Vec_IntSize(vConst) + nWords, 0 );
        pArray = Vec_IntEntryP( vConst, RetVal + 1 );
        for ( i = Length-1; i >= Length-Width; i-- )
            if ( pConst[i] == '1' )
                Abc_InfoSetBit( (unsigned *)pArray, Length-1-i );
    }
    else
    {
        Vec_IntPush( vConst, -1 );
        Vec_IntPush( vConst, Width );
    }
    return (RetVal << 2) | 1;
}
int Rtl_NtkReadSlice( Rtl_Ntk_t * p, char * pSlice, int NameId )
{
    Vec_Int_t * vSlice = &p->pLib->vSlices;
    int RetVal  = Vec_IntSize( vSlice );
    int Left    = atoi( pSlice+1 );
    char * pTwo = strstr( pSlice, ":" );
    int Right   = pTwo ? atoi( pTwo+1 ) : Left;
    assert( pSlice[0] == '[' && pSlice[strlen(pSlice)-1] == ']' );
    Vec_IntPush( vSlice, NameId );
    Vec_IntPush( vSlice, Left   );
    Vec_IntPush( vSlice, Right  );
    return (RetVal << 2) | 2;
}
int Rtl_NtkReadConcat( Rtl_Ntk_t * p, int * pPos )
{
    Vec_Int_t * vConcat = &p->pLib->vConcats;
    int RetVal = Vec_IntSize( vConcat ); char * pTok;
    Vec_IntPush( vConcat, ABC_INFINITY );
    do {
        int Sig = Rtl_NtkReadSig( p, pPos );
        Vec_IntPush( vConcat, Sig );
        pTok = Rtl_NtkTokStr( p, *pPos );
    } 
    while ( pTok[0] != '}' );
    Vec_IntWriteEntry( vConcat, RetVal, Vec_IntSize(vConcat) - RetVal - 1 );
    assert( pTok[0] == '}' );
    (*pPos)++;
    return (RetVal << 2) | 3;
}
int Rtl_NtkReadSig( Rtl_Ntk_t * p, int * pPos )
{
    int NameId  = Rtl_NtkTokId( p, *pPos );
    char * pSig = Rtl_NtkTokStr( p, (*pPos)++ );
    if ( pSig[0] >= '0' && pSig[0] <= '9' )
        return Rtl_NtkReadConst( p, pSig );
    if ( pSig[0] == '{' )
        return Rtl_NtkReadConcat( p, pPos );
    else 
    {
        char * pNext = Rtl_NtkTokStr( p, *pPos );
        if ( pNext && pNext[0] == '[' )
        {
            (*pPos)++;
            return Rtl_NtkReadSlice( p, pNext, NameId );
        }
        else
            return NameId << 2;
    }
}
int Rtl_NtkReadWire( Rtl_Ntk_t * p, int iPos )
{
    int i, Entry, Prev = -1;
    int Width = 1, Upto = 0, Offset = 0, Out = 0, In = 0, Number = 0, Signed = 0;
    assert( Rtl_NtkPosCheck(p, iPos-1, RTL_WIRE) );
    Vec_IntClear( &p->pLib->vAttrTemp );
    Vec_IntForEachEntryStart( p->pLib->vTokens, Entry, i, iPos )
    {
        //char * pTok = Rtl_NtkTokStr(p, i);
        if ( Entry == -1 )
            break;
        else if ( Rtl_NtkTokCheck(p, Entry, RTL_WIDTH) )
            Width = atoi( Rtl_NtkTokStr(p, ++i) );
        else if ( Rtl_NtkTokCheck(p, Entry, RTL_OFFSET) )
            Offset = atoi( Rtl_NtkTokStr(p, ++i) );
        else if ( Rtl_NtkTokCheck(p, Entry, RTL_INPUT) )
            Number = atoi( Rtl_NtkTokStr(p, ++i) ), In = 1, p->nInputs++;
        else if ( Rtl_NtkTokCheck(p, Entry, RTL_OUTPUT) )
            Number = atoi( Rtl_NtkTokStr(p, ++i) ), Out = 1, p->nOutputs++;
        else if ( Rtl_NtkTokCheck(p, Entry, RTL_SIGNED) )
            Signed = 1;
        else if ( Rtl_NtkTokCheck(p, Entry, RTL_UPTO) )
            Upto = 1;
        Prev = Entry;
    }
    // add WIRE_NUM=5 entries
    Vec_IntPush( &p->vWires, (Prev << 4) | (Upto << 3) | (Signed << 2) | (Out << 1) | (In << 0) );
    Vec_IntPush( &p->vWires, Width  );
    Vec_IntPush( &p->vWires, Offset );
    Vec_IntPush( &p->vWires, Number );
    Vec_IntPush( &p->vWires, -1 );
    assert( Rtl_NtkPosCheck(p, i, RTL_NONE) );
    return i;
}
int Rtl_NtkReadAttribute( Rtl_Ntk_t * p, int iPos )
{
//char * pTok1 = Rtl_NtkTokStr(p, iPos-1);
//char * pTok2 = Rtl_NtkTokStr(p, iPos);
//char * pTok3 = Rtl_NtkTokStr(p, iPos+1);
    assert( Rtl_NtkPosCheck(p, iPos-1, RTL_ATTRIBUTE) );
    Vec_IntPush( &p->pLib->vAttrTemp, Rtl_NtkTokId(p, iPos++) );
    Vec_IntPush( &p->pLib->vAttrTemp, Rtl_NtkTokId(p, iPos++) );
    assert( Rtl_NtkPosCheck(p, iPos, RTL_NONE) );
    return iPos;
}
int Rtl_NtkReadAttribute2( Rtl_Lib_t * p, int iPos )
{
//char * pTok1 = Abc_NamStr(p->pManName, Vec_IntEntry(p->vTokens, iPos-1));
//char * pTok2 = Abc_NamStr(p->pManName, Vec_IntEntry(p->vTokens, iPos)  );
//char * pTok3 = Abc_NamStr(p->pManName, Vec_IntEntry(p->vTokens, iPos+1));
    assert( Vec_IntEntry(p->vTokens, iPos-1) == p->pMap[RTL_ATTRIBUTE] );
    Vec_IntPush( &p->vAttrTemp, Vec_IntEntry(p->vTokens, iPos++) );
    Vec_IntPush( &p->vAttrTemp, Vec_IntEntry(p->vTokens, iPos++) );
    assert( Vec_IntEntry(p->vTokens, iPos) == p->pMap[RTL_NONE] );
    return iPos;
}
int Rtl_NtkReadConnect( Rtl_Ntk_t * p, int iPos )
{
//char * pTok1 = Rtl_NtkTokStr(p, iPos-1);
//char * pTok2 = Rtl_NtkTokStr(p, iPos);
//char * pTok3 = Rtl_NtkTokStr(p, iPos+1);
    assert( Rtl_NtkPosCheck(p, iPos-1, RTL_CONNECT) );
    Vec_IntPush( &p->vConns, Rtl_NtkReadSig(p, &iPos) );
    Vec_IntPush( &p->vConns, Rtl_NtkReadSig(p, &iPos) );
    assert( Rtl_NtkPosCheck(p, iPos, RTL_NONE) );
    return iPos;
}
int Rtl_NtkReadCell( Rtl_Ntk_t * p, int iPos )
{
    Vec_Int_t * vAttrs = &p->pLib->vAttrTemp;
    int iPosPars, iPosCons, Par, Val, i, Entry;
    assert( Rtl_NtkPosCheck(p, iPos-1, RTL_CELL) );
    Vec_IntPush( &p->vCells, Vec_IntSize(&p->vStore) );
    Vec_IntPush( &p->vStore, Rtl_NtkTokId(p, iPos++) ); // 0
    Vec_IntPush( &p->vStore, Rtl_NtkTokId(p, iPos++) ); // 1
    Vec_IntPush( &p->vStore, -1 );
    Vec_IntPush( &p->vStore, -1 );
    assert( Vec_IntSize(vAttrs) % 2 == 0 );
    Vec_IntPush( &p->vStore, Vec_IntSize(vAttrs)/2 );
    iPosPars = Vec_IntSize(&p->vStore);
    Vec_IntPush( &p->vStore, 0 );  // 5
    iPosCons = Vec_IntSize(&p->vStore);
    Vec_IntPush( &p->vStore, 0 );  // 6
    Vec_IntPush( &p->vStore, 0 );  // 7
    assert( Vec_IntSize(&p->vStore) == Vec_IntEntryLast(&p->vCells)+CELL_NUM );
    Vec_IntAppend( &p->vStore, vAttrs );
    Vec_IntClear( vAttrs );
    Vec_IntForEachEntryStart( p->pLib->vTokens, Entry, i, iPos )
    {
        if ( Rtl_NtkTokCheck(p, Entry, RTL_END) )
            break;
        if ( Rtl_NtkTokCheck(p, Entry, RTL_PARAMETER) || Rtl_NtkTokCheck(p, Entry, RTL_CONNECT) )
        {
            int iPosCount = Rtl_NtkTokCheck(p, Entry, RTL_PARAMETER) ? iPosPars : iPosCons;
            Vec_IntAddToEntry( &p->vStore, iPosCount, 1 );
            i++;
            Par = Rtl_NtkReadSig(p, &i);
            Val = Rtl_NtkReadSig(p, &i);            
            Vec_IntPushTwo( &p->vStore, Par, Val );
        }
        assert( Rtl_NtkPosCheck(p, i, RTL_NONE) );
    }
    assert( Rtl_NtkPosCheck(p, i, RTL_END) );
    i++;
    assert( Rtl_NtkPosCheck(p, i, RTL_NONE) );
    return i;
}
int Wln_ReadMatchEnd( Rtl_Ntk_t * p, int Mod )
{
    int i, Entry, Count = 0;
    Vec_IntForEachEntryStart( p->pLib->vTokens, Entry, i, Mod )
        if ( Rtl_NtkTokCheck(p, Entry, RTL_CELL) )
            Count++;
        else if ( Rtl_NtkTokCheck(p, Entry, RTL_END) )
        {
            if ( Count == 0 )
                return i;
            Count--;
        }
    assert( 0 );
    return -1;
}
int Rtl_NtkReadNtk( Rtl_Lib_t * pLib, int Mod )
{
    Rtl_Ntk_t * p = Rtl_NtkAlloc( pLib );
    Vec_Int_t * vAttrs = &p->pLib->vAttrTemp;
    int End = Wln_ReadMatchEnd( p, Mod ), i, Entry;
    assert( Rtl_NtkPosCheck(p, Mod-1, RTL_MODULE) );
    assert( Rtl_NtkPosCheck(p, End, RTL_END)    );
    p->NameId = Rtl_NtkTokId( p, Mod );
    p->Slice0 = Vec_IntSize( &pLib->vSlices );
    Vec_IntAppend( &p->vAttrs, vAttrs );
    Vec_IntClear( vAttrs );
    Vec_IntForEachEntryStartStop( pLib->vTokens, Entry, i, Mod, End )
    {
        if ( Rtl_NtkTokCheck(p, Entry, RTL_WIRE) )
            i = Rtl_NtkReadWire( p, i+1 );
        else if ( Rtl_NtkTokCheck(p, Entry, RTL_ATTRIBUTE) )
            i = Rtl_NtkReadAttribute( p, i+1 );
        else if ( Rtl_NtkTokCheck(p, Entry, RTL_CELL) )
            i = Rtl_NtkReadCell( p, i+1 );
        else if ( Rtl_NtkTokCheck(p, Entry, RTL_CONNECT) )
            i = Rtl_NtkReadConnect( p, i+1 );
    }
    p->Slice1 = Vec_IntSize( &pLib->vSlices );
    assert( Vec_IntSize(&p->vWires) % WIRE_NUM == 0 );
    return End;
}
void Rtl_NtkReportUndefs( Rtl_Ntk_t * p )
{
    Vec_Int_t * vNames, * vCounts;
    int i, iName, * pCell;
    vNames  = Vec_IntAlloc( 10 );
    vCounts = Vec_IntAlloc( 10 );
    Rtl_NtkForEachCell( p, pCell, i )
        if ( Rtl_CellModule(pCell) == ABC_INFINITY-1 ) 
        {
            iName = Vec_IntFind(vNames, Rtl_CellType(pCell));
            if ( iName == -1 )
            {
                iName = Vec_IntSize(vNames);
                Vec_IntPush( vNames, Rtl_CellType(pCell) );
                Vec_IntPush( vCounts, 0 );
            }
            Vec_IntAddToEntry( vCounts, iName, 1 );
        }
    Vec_IntForEachEntry( vNames, iName, i )
        printf( "  %s (%d)", Rtl_NtkStr(p, iName), Vec_IntEntry(vCounts, i) );
    printf( "\n" );
    Vec_IntFree( vNames );
    Vec_IntFree( vCounts );
}
int Rtl_NtkSetParents( Rtl_Ntk_t * p )
{
    int i, * pCell, nUndef = 0;
    Rtl_NtkForEachCell( p, pCell, i )
    {
        pCell[2] = Rtl_NtkReadType( p, Rtl_CellType(pCell) );
        if ( Rtl_CellModule(pCell) == ABC_INFINITY-1 ) 
            nUndef++;
        else
            pCell[3] = Rtl_CellModule(pCell) < ABC_INFINITY ? pCell[6]-1 : Rtl_NtkModule(p, Rtl_CellModule(pCell)-ABC_INFINITY)->nInputs;
    }
    if ( !nUndef )
        return 0;
    printf( "Module \"%s\" has %d blackbox instances: ", Rtl_NtkName(p), nUndef );
    Rtl_NtkReportUndefs( p );
    return nUndef;
}
void Rtl_LibSetParents( Rtl_Lib_t * p )
{
    Rtl_Ntk_t * pNtk; int i;
    Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
        Rtl_NtkSetParents( pNtk );  
}
void Rtl_LibReorderModules_rec( Rtl_Ntk_t * p, Vec_Ptr_t * vNew )
{
    int i, * pCell;
    Rtl_NtkForEachCell( p, pCell, i )
    {
        Rtl_Ntk_t * pMod = Rtl_CellNtk( p, pCell );
        if ( pMod && pMod->iCopy == -1 )
            Rtl_LibReorderModules_rec( pMod, vNew );
    }
    assert( p->iCopy == -1 );
    p->iCopy = Vec_PtrSize(vNew);
    Vec_PtrPush( vNew, p );
}
int Rtl_LibCountInsts( Rtl_Lib_t * p, Rtl_Ntk_t * pOne )
{
    Rtl_Ntk_t * pNtk; int n, i, * pCell, Count = 0;
    Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, n )
        Rtl_NtkForEachCell( pNtk, pCell, i )
        {
            Rtl_Ntk_t * pMod = Rtl_CellNtk( pNtk, pCell );
            if ( pMod && pMod == pOne ) 
                Count++;
        }
    return Count;
}
void Rtl_NtkUpdateBoxes( Rtl_Ntk_t * p )
{
    int i, * pCell;
    Rtl_NtkForEachCell( p, pCell, i )
    {
        Rtl_Ntk_t * pMod = Rtl_CellNtk( p, pCell );
        if ( pMod && pMod->iCopy >= 0 ) 
            pCell[2] = ABC_INFINITY + pMod->iCopy;
    }
}
void Rtl_LibUpdateBoxes( Rtl_Lib_t * p )
{
    Rtl_Ntk_t * pNtk; int i;
    Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
        Rtl_NtkUpdateBoxes( pNtk );  
}
void Rtl_LibReorderModules( Rtl_Lib_t * p )
{
    Vec_Ptr_t * vNew = Vec_PtrAlloc( Vec_PtrSize(p->vNtks) );
    Rtl_Ntk_t * pNtk; int i;
    Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
        pNtk->iCopy = -1;
    Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
        if ( pNtk->iCopy == -1 )
            Rtl_LibReorderModules_rec( pNtk, vNew );
    assert( Vec_PtrSize(p->vNtks) == Vec_PtrSize(vNew) );
    Rtl_LibUpdateBoxes( p );
    Vec_PtrClear( p->vNtks );
    Vec_PtrAppend( p->vNtks, vNew );
    Vec_PtrFree( vNew );
}
Rtl_Lib_t * Rtl_LibReadFile( char * pFileName, char * pFileSpec )
{
    Rtl_Lib_t * p = Rtl_LibAlloc(); int i, Entry;
    p->pSpec      = Abc_UtilStrsav( pFileSpec );
    p->pManName   = Abc_NamStart( 1000, 50 );
    p->vTokens    = Rtl_NtkReadFile( pFileName, p->pManName );
    Rtl_LibDeriveMap( p );
    Vec_IntClear( &p->vAttrTemp );
    Vec_IntForEachEntry( p->vTokens, Entry, i )
        if ( Entry == p->pMap[RTL_MODULE] )
            i = Rtl_NtkReadNtk( p, i+1 );
        else if ( Entry == p->pMap[RTL_ATTRIBUTE] )
            i = Rtl_NtkReadAttribute2( p, i+1 );
    Rtl_LibSetParents( p );
    Rtl_LibReorderModules( p );
    Rtl_LibOrderWires( p );
    return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
extern int Rtl_NtkMapSignalRange( Rtl_Ntk_t * p, int Sig, int iCell, int iBit );

int Rtl_NtkMapWireRange( Rtl_Ntk_t * p, int NameId, int Left, int Right, int iCell, int iBit )
{
    //char * pName = Rtl_NtkStr( p, NameId );
    int Wire  = Rtl_WireMapNameToId( p, NameId );
    int First = Rtl_WireBitStart( p, Wire );
    int Width = Rtl_WireWidth( p, Wire ), i;
    Left  = Left  == -1 ? Width-1 :  Left;
    Right = Right == -1 ? 0       : Right;
    assert ( Right >= 0 && Right <= Left );
    for ( i = Right; i <= Left; i++ )
    {
        assert( Vec_IntEntry(&p->vDrivers, 2*(First+i)) == -4 );
        Vec_IntWriteEntry(&p->vDrivers, 2*(First+i)+0, iCell );
        Vec_IntWriteEntry(&p->vDrivers, 2*(First+i)+1, iBit + (i - Right) );
    }
    return Left - Right + 1;
}
int Rtl_NtkMapSliceRange( Rtl_Ntk_t * p, int * pSlice, int iCell, int iBit )
{
    return Rtl_NtkMapWireRange( p, pSlice[0], pSlice[1], pSlice[2], iCell, iBit );
}
int Rtl_NtkMapConcatRange( Rtl_Ntk_t * p, int * pConcat, int iCell, int iBit )
{
    int i, k = 0;
    for ( i = 1; i <= pConcat[0]; i++ )
        k += Rtl_NtkMapSignalRange( p, pConcat[i], iCell, iBit+k );
    return k;
}
int Rtl_NtkMapSignalRange( Rtl_Ntk_t * p, int Sig, int iCell, int iBit )
{
    int nBits = ABC_INFINITY;
    if ( Rtl_SigIsNone(Sig) )
        nBits = Rtl_NtkMapWireRange( p, Sig >> 2, -1, -1, iCell, iBit );
    if ( Rtl_SigIsSlice(Sig) )
        nBits = Rtl_NtkMapSliceRange( p, Vec_IntEntryP(&p->pLib->vSlices, Sig >> 2), iCell, iBit );
    if ( Rtl_SigIsConcat(Sig) )
        nBits = Rtl_NtkMapConcatRange( p, Vec_IntEntryP(&p->pLib->vConcats, Sig >> 2), iCell, iBit );
    if ( Rtl_SigIsConst(Sig) )
        assert( 0 );
    return nBits;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
extern void Rtl_NtkCollectSignalInfo( Rtl_Ntk_t * p, int Sig );

void Rtl_NtkCollectWireInfo( Rtl_Ntk_t * p, int NameId, int Left, int Right )
{
    int Wire  = Rtl_WireMapNameToId( p, NameId );
    int First = Rtl_WireBitStart( p, Wire );
    int Width = Rtl_WireWidth( p, Wire ), i;
    Left  = Left  == -1 ? Width-1 :  Left;
    Right = Right == -1 ? 0       : Right;
    assert ( Right >= 0 && Right <= Left );
    for ( i = Right; i <= Left; i++ )
        Vec_IntPush( &p->vBitTemp, First+i );
}
void Rtl_NtkCollectConstInfo( Rtl_Ntk_t * p, int * pConst )
{
    int i, nLimit = pConst[0];
    if ( nLimit == -1 )
        nLimit = 32;
    for ( i = 0; i < nLimit; i++ )
        Vec_IntPush( &p->vBitTemp, Abc_InfoHasBit((unsigned *)pConst+1,i)-CONST_SHIFT );
}
void Rtl_NtkCollectSliceInfo( Rtl_Ntk_t * p, int * pSlice )
{
    Rtl_NtkCollectWireInfo( p, pSlice[0], pSlice[1], pSlice[2] );
}
void Rtl_NtkCollectConcatInfo( Rtl_Ntk_t * p, int * pConcat )
{
    int i;
    for ( i = pConcat[0]; i >= 1; i-- )
        Rtl_NtkCollectSignalInfo( p, pConcat[i] );
}
void Rtl_NtkCollectSignalInfo( Rtl_Ntk_t * p, int Sig )
{
    if ( Rtl_SigIsNone(Sig) )
        Rtl_NtkCollectWireInfo( p, Sig >> 2, -1, -1 );
    else if ( Rtl_SigIsConst(Sig) )
        Rtl_NtkCollectConstInfo( p, Vec_IntEntryP(&p->pLib->vConsts, Sig >> 2) );
    else if ( Rtl_SigIsSlice(Sig) )
        Rtl_NtkCollectSliceInfo( p, Vec_IntEntryP(&p->pLib->vSlices, Sig >> 2) );
    else if ( Rtl_SigIsConcat(Sig) )
        Rtl_NtkCollectConcatInfo( p, Vec_IntEntryP(&p->pLib->vConcats, Sig >> 2) );
    else assert( 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
extern void Rtl_NtkCollectSignalRange( Rtl_Ntk_t * p, int Sig );

void Rtl_NtkCollectWireRange( Rtl_Ntk_t * p, int NameId, int Left, int Right )
{
    int Wire  = Rtl_WireMapNameToId( p, NameId );
    int First = Rtl_WireBitStart( p, Wire );
    int Width = Rtl_WireWidth( p, Wire ), i;
    Left  = Left  == -1 ? Width-1 :  Left;
    Right = Right == -1 ? 0       : Right;
    assert ( Right >= 0 && Right <= Left );
    for ( i = Right; i <= Left; i++ )
    {
        assert( Vec_IntEntry(&p->vLits, First+i) != -1 );
        Vec_IntPush( &p->vBitTemp, Vec_IntEntry(&p->vLits, First+i) );
    }
}
void Rtl_NtkCollectConstRange( Rtl_Ntk_t * p, int * pConst )
{
    int i, nLimit = pConst[0];
    if ( nLimit == -1 )
        nLimit = 32;
    for ( i = 0; i < nLimit; i++ )
        Vec_IntPush( &p->vBitTemp, Abc_InfoHasBit((unsigned *)pConst+1,i) );
}
void Rtl_NtkCollectSliceRange( Rtl_Ntk_t * p, int * pSlice )
{
    Rtl_NtkCollectWireRange( p, pSlice[0], pSlice[1], pSlice[2] );
}
void Rtl_NtkCollectConcatRange( Rtl_Ntk_t * p, int * pConcat )
{
    int i;
    for ( i = pConcat[0]; i >= 1; i-- )
        Rtl_NtkCollectSignalRange( p, pConcat[i] );
}
void Rtl_NtkCollectSignalRange( Rtl_Ntk_t * p, int Sig )
{
    if ( Rtl_SigIsNone(Sig) )
        Rtl_NtkCollectWireRange( p, Sig >> 2, -1, -1 );
    else if ( Rtl_SigIsConst(Sig) )
        Rtl_NtkCollectConstRange( p, Vec_IntEntryP(&p->pLib->vConsts, Sig >> 2) );
    else if ( Rtl_SigIsSlice(Sig) )
        Rtl_NtkCollectSliceRange( p, Vec_IntEntryP(&p->pLib->vSlices, Sig >> 2) );
    else if ( Rtl_SigIsConcat(Sig) )
        Rtl_NtkCollectConcatRange( p, Vec_IntEntryP(&p->pLib->vConcats, Sig >> 2) );
    else assert( 0 );
}


extern int Rtl_NtkInsertSignalRange( Rtl_Ntk_t * p, int Sig, int * pLits, int nLits );

int Rtl_NtkInsertWireRange( Rtl_Ntk_t * p, int NameId, int Left, int Right, int * pLits, int nLits )
{
    //char * pName = Rtl_NtkStr( p, NameId );
    int Wire  = Rtl_WireMapNameToId( p, NameId );
    int First = Rtl_WireBitStart( p, Wire );
    int Width = Rtl_WireWidth( p, Wire ), i, k = 0;
    Left  = Left  == -1 ? Width-1 :  Left;
    Right = Right == -1 ? 0       : Right;
    assert ( Right >= 0 && Right <= Left );
    for ( i = Right; i <= Left; i++ )
    {
        assert( Vec_IntEntry(&p->vLits, First+i) == -1 );
        Vec_IntWriteEntry(&p->vLits, First+i, pLits[k++] );
    }
    assert( k <= nLits );
    return k;
}
int Rtl_NtkInsertSliceRange( Rtl_Ntk_t * p, int * pSlice, int * pLits, int nLits )
{
    return Rtl_NtkInsertWireRange( p, pSlice[0], pSlice[1], pSlice[2], pLits, nLits );
}
int Rtl_NtkInsertConcatRange( Rtl_Ntk_t * p, int * pConcat, int * pLits, int nLits )
{
    int i, k = 0;
    for ( i = 1; i <= pConcat[0]; i++ )
        k += Rtl_NtkInsertSignalRange( p, pConcat[i], pLits+k, nLits-k );
    assert( k <= nLits );
    return k;
}
int Rtl_NtkInsertSignalRange( Rtl_Ntk_t * p, int Sig, int * pLits, int nLits )
{
    int nBits = ABC_INFINITY;
    if ( Rtl_SigIsNone(Sig) )
        nBits = Rtl_NtkInsertWireRange( p, Sig >> 2, -1, -1, pLits, nLits );
    if ( Rtl_SigIsSlice(Sig) )
        nBits = Rtl_NtkInsertSliceRange( p, Vec_IntEntryP(&p->pLib->vSlices, Sig >> 2), pLits, nLits );
    if ( Rtl_SigIsConcat(Sig) )
        nBits = Rtl_NtkInsertConcatRange( p, Vec_IntEntryP(&p->pLib->vConcats, Sig >> 2), pLits, nLits );
    if ( Rtl_SigIsConst(Sig) )
        assert( 0 );
    assert( nBits == nLits );
    return nBits;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Rtl_NtkRevPermInput( Rtl_Ntk_t * p )
{
    Vec_Int_t * vNew = Vec_IntAlloc( 100 ); int b, i, Count = 0;
    for ( i = 0; i < p->nInputs; i++ )
    {
        int Width = Rtl_WireWidth( p, i );
        for ( b = 0; b < Width; b++ )
            Vec_IntPush( vNew, Count + Width-1-b );
        Count += Width;
    }
    return vNew;
}
Vec_Int_t * Rtl_NtkRevPermOutput( Rtl_Ntk_t * p )
{
    Vec_Int_t * vNew = Vec_IntAlloc( 100 );  int b, i, Count = 0;
    for ( i = 0; i < p->nOutputs; i++ )
    {
        int Width = Rtl_WireWidth( p, p->nInputs + i );
        for ( b = 0; b < Width; b++ )
            Vec_IntPush( vNew, Count + Width-1-b );
        Count += Width;
    }
    return vNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtl_NtkBlastInputs( Gia_Man_t * pNew, Rtl_Ntk_t * p )
{
    int b, i;
    for ( i = 0; i < p->nInputs; i++ )
    {
        int First = Rtl_WireBitStart( p, i );
        int Width = Rtl_WireWidth( p, i );
        for ( b = 0; b < Width; b++ )
        {
            assert( Vec_IntEntry(&p->vLits, First+b) == -1 );
            Vec_IntWriteEntry( &p->vLits, First+b, Gia_ManAppendCi(pNew) );
        }
    }
}
void Rtl_NtkBlastOutputs( Gia_Man_t * pNew, Rtl_Ntk_t * p )
{
    int b, i;
    for ( i = 0; i < p->nOutputs; i++ )
    {
        int First = Rtl_WireBitStart( p, p->nInputs + i );
        int Width = Rtl_WireWidth( p, p->nInputs + i );
        for ( b = 0; b < Width; b++ )
        {
            assert( Vec_IntEntry(&p->vLits, First+b) != -1 );
            Gia_ManAppendCo( pNew, Vec_IntEntry(&p->vLits, First+b) );
        }
    }
}
void Rtl_NtkBlastConnect( Gia_Man_t * pNew, Rtl_Ntk_t * p, int * pCon )
{
    int nBits;
    Vec_IntClear( &p->vBitTemp );
    Rtl_NtkCollectSignalRange( p, pCon[0] );
    nBits = Rtl_NtkInsertSignalRange( p, pCon[1], Vec_IntArray(&p->vBitTemp), Vec_IntSize(&p->vBitTemp) );
    assert( nBits == Vec_IntSize(&p->vBitTemp) );
    //printf( "Finished blasting connection (Value = %d).\n", Vec_IntEntry(&p->vBitTemp, 0) );
}
void Rtl_NtkBlastHierarchy( Gia_Man_t * pNew, Rtl_Ntk_t * p, int * pCell )
{
    extern void Rtl_NtkPrintBufs( Rtl_Ntk_t * p, Vec_Int_t * vBufs );
    extern Gia_Man_t * Rtl_NtkBlast( Rtl_Ntk_t * p );
    extern void Gia_ManDupRebuild( Gia_Man_t * pNew, Gia_Man_t * p, Vec_Int_t * vLits, int fBufs );
    extern int Gia_ManFindFirst( Rtl_Ntk_t * p, int * pnOuts );
    Rtl_Ntk_t * pModel = Rtl_NtkModule( p, Rtl_CellModule(pCell)-ABC_INFINITY );
    int nIns = 0, nOuts = 0, nOuts1, iFirst1 = Gia_ManFindFirst( pModel, &nOuts1 );
    int k, Par, Val, iThis = -1, nBits = 0;
    //int fFound = 0;
    int fFound = p->pLib->vInverses && (iThis = Vec_IntFind(p->pLib->vInverses, pModel->NameId)) >= 0;
    //int iThat = fFound ? Vec_IntEntry( p->pLib->vInverses, iThis ^ 1 ) : -1;
    Vec_IntClear( &p->vBitTemp );
    Rtl_CellForEachInput( p, pCell, Par, Val, k )
        Rtl_NtkCollectSignalRange( p, Val );
    assert( pModel->pGia );
    if ( fFound )
    {
        nIns = nOuts1;
        Vec_IntForEachEntry( &p->vBitTemp, Val, k )
            Vec_IntWriteEntry( &p->vBitTemp, k, (k >= iFirst1 && k < iFirst1 + nOuts1) ? Gia_ManAppendBuf(pNew, Val) : Val );
        Vec_IntPush( pNew->vBarBufs, (nIns << 16) | Abc_Var2Lit(pModel->NameId, 0) );
    }
    else if ( pModel->fRoot )
    {
        nIns = Vec_IntSize(&p->vBitTemp);
        Vec_IntForEachEntry( &p->vBitTemp, Val, k )
            //Vec_IntWriteEntry( &p->vBitTemp, k, (k >= iFirst1 && k < iFirst1 + nOuts1) ? Gia_ManAppendBuf(pNew, Val) : Val );
            Vec_IntWriteEntry( &p->vBitTemp, k, Gia_ManAppendBuf(pNew, Val) );
        Vec_IntPush( pNew->vBarBufs, (nIns << 16) | Abc_Var2Lit(pModel->NameId, 0) );
    }
    if ( fFound || pModel->fRoot )
        Gia_ManDupRebuild( pNew, pModel->pGia, &p->vBitTemp, 0 );
    else
    {
        Gia_ManDupRebuild( pNew, pModel->pGia, &p->vBitTemp, 1 );
        Vec_IntAppend( pNew->vBarBufs, pModel->pGia->vBarBufs );
    }
    if ( pModel->fRoot || fFound )
    {
        nOuts = Vec_IntSize(&p->vBitTemp);
        Vec_IntForEachEntry( &p->vBitTemp, Val, k )
            Vec_IntWriteEntry( &p->vBitTemp, k, Gia_ManAppendBuf(pNew, Val) );
        Vec_IntPush( pNew->vBarBufs, (nOuts << 16) | Abc_Var2Lit(pModel->NameId, 1) );
        printf( "Added %d input buffers and %d output buffers for module %s.\n", nIns, nOuts, Rtl_NtkName(pModel) );
    }
    Rtl_CellForEachOutput( p, pCell, Par, Val, k )
        nBits += Rtl_NtkInsertSignalRange( p, Val, Vec_IntArray(&p->vBitTemp)+nBits, Vec_IntSize(&p->vBitTemp)-nBits );
    assert( nBits == Vec_IntSize(&p->vBitTemp) );
}

int Rtl_NtkCellParamValue( Rtl_Ntk_t * p, int * pCell, char * pParam )
{
    int ParamId = Rtl_NtkStrId( p, pParam );
    int i, Par, Val, ValOut = ABC_INFINITY, * pConst;
//    p->pLib->pFile = stdout;
//    Rtl_CellForEachParam( p, pCell, Par, Val, i )
//        fprintf( Rtl_NtkFile(p), "    parameter" ), Rtl_NtkPrintSig(p, Par), Rtl_NtkPrintSig(p, Val), printf( "\n" );
    Rtl_CellForEachParam( p, pCell, Par, Val, i )
        if ( (Par >> 2) == ParamId )
        {
            assert( Rtl_SigIsConst(Val) );
            pConst = Vec_IntEntryP( &p->pLib->vConsts, Val >> 2 );
            assert( pConst[0] < 32 );
            ValOut = pConst[1];
        }
    return ValOut;
}
void Rtl_NtkBlastOperator( Gia_Man_t * pNew, Rtl_Ntk_t * p, int * pCell )
{
    extern void Rtl_NtkBlastNode( Gia_Man_t * pNew, int Type, int nIns, Vec_Int_t * vDatas, int nRange, int fSign0, int fSign1 );
    Vec_Int_t * vRes = &p->pLib->vTemp[3];
    int i, Par, Val, ValOut = -1, nBits = 0, nRange = -1;
    int fSign0 = Rtl_NtkCellParamValue( p, pCell, "\\A_SIGNED" );
    int fSign1 = Rtl_NtkCellParamValue( p, pCell, "\\B_SIGNED" );
    Rtl_CellForEachOutput( p, pCell, Par, ValOut, i )
        nRange = Rtl_NtkCountSignalRange( p, ValOut );
    assert( nRange > 0 );
    for ( i = 0; i < TEMP_NUM; i++ )
        Vec_IntClear( &p->pLib->vTemp[i] );
    //printf( "Starting blasting cell %s.\n", Rtl_CellNameStr(p, pCell) );
    Rtl_CellForEachInput( p, pCell, Par, Val, i )
    {
        Vec_IntClear( &p->vBitTemp );
        Rtl_NtkCollectSignalRange( p, Val );
        Vec_IntAppend( &p->pLib->vTemp[i], &p->vBitTemp );
    }
    Rtl_NtkBlastNode( pNew, Rtl_CellModule(pCell), Rtl_CellInputNum(pCell), p->pLib->vTemp, nRange, fSign0, fSign1 );
    assert( Vec_IntSize(vRes) > 0 );
    nBits = Rtl_NtkInsertSignalRange( p, ValOut, Vec_IntArray(vRes)+nBits, Vec_IntSize(vRes)-nBits );
    assert( nBits == Vec_IntSize(vRes) );
    //printf( "Finished blasting cell %s (Value = %d).\n", Rtl_CellNameStr(p, pCell), Vec_IntEntry(vRes, 0) );
}
char * Rtl_ShortenName( char * pName, int nSize )
{
    static char Buffer[1000];
    if ( (int)strlen(pName) <= nSize )
        return pName;
    Buffer[0] = 0;
    strcat( Buffer, pName );
    //Buffer[nSize-4] = ' ';
    Buffer[nSize-3] = '.';
    Buffer[nSize-2] = '.';
    Buffer[nSize-1] = '.';
    Buffer[nSize-0] = 0;
    return Buffer;
}
void Rtl_NtkPrintBufOne( Rtl_Lib_t * p, int Lit )
{
    printf( "%s (%c%d)  ", Rtl_LibStr(p, Abc_Lit2Var(Lit&0xFFFF)), Abc_LitIsCompl(Lit)? 'o' : 'i', Lit >> 16 );
}
void Rtl_NtkPrintBufs( Rtl_Ntk_t * p, Vec_Int_t * vBufs )
{
    int i, Lit;
    if ( Vec_IntSize(vBufs) )
        printf( "Found %d buffers (%d groups):  ", p->pGia->nBufs, Vec_IntSize(vBufs) );
    Vec_IntForEachEntry( vBufs, Lit, i )
        Rtl_NtkPrintBufOne( p->pLib, Lit );
    if ( Vec_IntSize(vBufs) )
        printf( "\n" );
}
Gia_Man_t * Rtl_NtkBlast( Rtl_Ntk_t * p )
{
    int fDump = 0;
    Gia_Man_t * pTemp, * pNew = Gia_ManStart( 1000 );
    int i, iObj, * pCell, nBits = Rtl_NtkRangeWires( p );
    Vec_IntFill( &p->vLits, nBits, -1 );
    Rtl_NtkMapWires( p, 0 );
    Rtl_NtkBlastInputs( pNew, p );
    Gia_ManHashAlloc( pNew );
    Vec_IntForEachEntry( &p->vOrder, iObj, i )
    {
        iObj -= Rtl_NtkInputNum(p);
        if ( iObj < 0 )
            continue;
        if ( iObj >= Rtl_NtkCellNum(p) )
        {
            Rtl_NtkBlastConnect( pNew, p, Rtl_NtkCon(p, iObj - Rtl_NtkCellNum(p)) );
            continue;
        }
        pCell = Rtl_NtkCell(p, iObj);
        if ( Rtl_CellModule(pCell) >= ABC_INFINITY )
            Rtl_NtkBlastHierarchy( pNew, p, pCell );
        else if ( Rtl_CellModule(pCell) < ABC_OPER_LAST )
            Rtl_NtkBlastOperator( pNew, p, pCell );
        else
            printf( "Cannot blast black box %s in module %s.\n", Rtl_NtkStr(p, Rtl_CellType(pCell)), Rtl_NtkName(p) );
    }
    Gia_ManHashStop( pNew );
    Rtl_NtkBlastOutputs( pNew, p );
    Rtl_NtkMapWires( p, 1 );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    if ( fDump )
    {
        char Buffer[100]; static int counter = 0;
        sprintf( Buffer, "old%02d.aig", counter++ );
        Gia_AigerWrite( pNew, Buffer, 0, 0, 0 );
        printf( "Dumped \"%s\" with AIG for module %-20s : ", Buffer, Rtl_ShortenName(Rtl_NtkName(p), 20) );
    }
    else
        printf( "Derived AIG for module %-20s : ", Rtl_ShortenName(Rtl_NtkName(p), 20) );
    Gia_ManPrintStats( pNew, NULL );
    return pNew;
}
void Rtl_LibBlast( Rtl_Lib_t * pLib )
{
    Rtl_Ntk_t * p; int i;
    Vec_PtrForEachEntry( Rtl_Ntk_t *, pLib->vNtks, p, i )
        if ( p->pGia == NULL )
            p->pGia = Rtl_NtkBlast( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// -4 unassigned
// -3 other bit
// -2 constant
// -1 primary input
// 0+ cell
int Rtl_NtkBlastCons( Rtl_Ntk_t * p )
{
    int c, i, iBit0, iBit1, * pCon, * pDri0, * pDri1, nChanges = 0;
    Rtl_NtkForEachCon( p, pCon, c )
    {
        Vec_IntClear( &p->vBitTemp );
        Rtl_NtkCollectSignalInfo( p, pCon[1] );
        Vec_IntClearAppend( &p->vBitTemp2, &p->vBitTemp );

        Vec_IntClear( &p->vBitTemp );
        Rtl_NtkCollectSignalInfo( p, pCon[0] );
        assert( Vec_IntSize(&p->vBitTemp2) == Vec_IntSize(&p->vBitTemp) );

        Vec_IntForEachEntryTwo( &p->vBitTemp, &p->vBitTemp2, iBit0, iBit1, i )
        {
            pDri0 = iBit0 >= 0 ? Vec_IntEntryP(&p->vDrivers, 2*iBit0) : NULL;
            pDri1 = iBit1 >= 0 ? Vec_IntEntryP(&p->vDrivers, 2*iBit1) : NULL;
            assert( iBit0 >= 0 || iBit1 >= 0 );
            if ( iBit0 < 0 )
            {
                if ( pDri1[0] == -4 )
                {
                    assert( pDri1[1] == -4 );
                    pDri1[0] = -2;
                    pDri1[1] = iBit0+CONST_SHIFT;
                    nChanges++;
                }
                continue;
            }
            if ( iBit1 < 0 )
            {
                if ( pDri0[0] == -4 )
                {
                    assert( pDri0[1] == -4 );
                    pDri0[0] = -2;
                    pDri0[1] = iBit1+CONST_SHIFT;
                    nChanges++;
                }
                continue;
            }
            if ( pDri0[0] == -4 && pDri1[0] != -4 )
            {
                assert( pDri0[1] == -4 );
                pDri0[0] = -3;
                pDri0[1] = iBit1;
                nChanges++;
                continue;
            }
            if ( pDri1[0] == -4 && pDri0[0] != -4 )
            {
                assert( pDri1[1] == -4 );
                pDri1[0] = -3;
                pDri1[1] = iBit0;
                nChanges++;
                continue;
            }
        }
    }
    //printf( "Changes %d\n", nChanges );
    return nChanges;
}
void Rtl_NtkBlastMap( Rtl_Ntk_t * p, int nBits )
{
    int i, k, Par, Val, * pCell, iBit = 0;
    Vec_IntFill( &p->vDrivers, 2*nBits, -4 );
    for ( i = 0; i < p->nInputs; i++ )
    {
        int First = Rtl_WireBitStart( p, i );
        int Width = Rtl_WireWidth( p, i );
        for ( k = 0; k < Width; k++ )
        {
            assert( Vec_IntEntry(&p->vDrivers, 2*(First+k)) == -4 );
            Vec_IntWriteEntry(&p->vDrivers, 2*(First+k)+0, -1 );
            Vec_IntWriteEntry(&p->vDrivers, 2*(First+k)+1, iBit++ );
        }
    }
    Rtl_NtkForEachCell( p, pCell, i )
    {
        int iBit = 0;
        Rtl_CellForEachOutput( p, pCell, Par, Val, k )
            iBit += Rtl_NtkMapSignalRange( p, Val, i, iBit );
    }
    for ( i = 0; i < 100; i++ )
        if ( !Rtl_NtkBlastCons(p) )
            break;
    if ( i == 100 )
        printf( "Mapping connections did not succeed after %d iterations.\n", i );
//    else
//        printf( "Mapping connections converged after %d iterations.\n", i );
}
int Rtl_NtkCollectOrComputeBit( Rtl_Ntk_t * p, int iBit )
{
    extern void Rtl_NtkBlast2_rec( Rtl_Ntk_t * p, int iBit, int * pDriver );
    if ( Vec_IntEntry(&p->vLits, iBit) == -1 )
    {
        int * pDriver = Vec_IntEntryP(&p->vDrivers, 2*iBit);
        assert( pDriver[0] != -4 );
        Rtl_NtkBlast2_rec( p, iBit, pDriver );
    }
    assert( Vec_IntEntry(&p->vLits, iBit) >= 0 );
    return Vec_IntEntry(&p->vLits, iBit);
}
int Rtl_NtkBlast2Spec( Rtl_Ntk_t * p, int * pCell, int iInput )
{
    int i, Par, Val, pLits[3] = {-1, -1, -1}, iBit;
    Rtl_CellForEachInput( p, pCell, Par, Val, i )
    {
        Vec_Int_t * vTemp;
        Vec_IntClear( &p->vBitTemp );
        Rtl_NtkCollectSignalInfo( p, Val );
        vTemp = Vec_IntDup( &p->vBitTemp );
        iBit = Vec_IntEntry( vTemp, i==2 ? 0 : iInput );
        if ( iBit >= 0 )
            pLits[i] = Rtl_NtkCollectOrComputeBit( p, iBit );
        else
            pLits[i] = iBit+CONST_SHIFT;
        assert( pLits[i] >= 0 );
        Vec_IntFree( vTemp );
    }
    return Gia_ManHashMux(p->pGia, pLits[2], pLits[1], pLits[0]);
}
void Rtl_NtkBlastPrepareInputs( Rtl_Ntk_t * p, int * pCell )
{
    int i, k, Par, Val, iBit;
    Rtl_CellForEachInput( p, pCell, Par, Val, i )
    {
        Vec_Int_t * vTemp;
        Vec_IntClear( &p->vBitTemp );
        Rtl_NtkCollectSignalInfo( p, Val );
        vTemp = Vec_IntDup( &p->vBitTemp );
        Vec_IntForEachEntry( vTemp, iBit, k )
            if ( iBit >= 0 )
                Rtl_NtkCollectOrComputeBit( p, iBit );
        Vec_IntFree( vTemp );
    }
}
void Rtl_NtkBlast2_rec( Rtl_Ntk_t * p, int iBit, int * pDriver )
{
    //char * pName = Rtl_NtkName(p);
    assert( pDriver[0] != -1 );
    if ( pDriver[0] == -3 )
    {
        int * pDriver1 = Vec_IntEntryP( &p->vDrivers, 2*pDriver[1] );
        if ( Vec_IntEntry(&p->vLits, pDriver[1]) == -1 )
            Rtl_NtkBlast2_rec( p, pDriver[1], pDriver1 );
        assert( Vec_IntEntry(&p->vLits, pDriver[1]) >= 0 );
        Vec_IntWriteEntry( &p->vLits, iBit, Vec_IntEntry(&p->vLits, pDriver[1]) );
        return;
    }
    if ( pDriver[0] == -2 )
    {
        Vec_IntWriteEntry( &p->vLits, iBit, pDriver[1] );
        return;
    }
    else
    {
        int * pCell = Rtl_NtkCell(p, pDriver[0]);
        assert( pDriver[0] >= 0 );
        if ( Rtl_CellModule(pCell) == ABC_OPER_SEL_NMUX ) // special case
        {
            int iLit = Rtl_NtkBlast2Spec( p, pCell, pDriver[1] );
            Vec_IntWriteEntry( &p->vLits, iBit, iLit );
            return;
        }
        Rtl_NtkBlastPrepareInputs( p, pCell );
        if ( Rtl_CellModule(pCell) >= ABC_INFINITY )
            Rtl_NtkBlastHierarchy( p->pGia, p, pCell );
        else if ( Rtl_CellModule(pCell) < ABC_OPER_LAST )
            Rtl_NtkBlastOperator( p->pGia, p, pCell );
        else
            printf( "Cannot blast black box %s in module %s.\n", Rtl_NtkStr(p, Rtl_CellType(pCell)), Rtl_NtkName(p) );
    }
}
Gia_Man_t * Rtl_NtkBlast2( Rtl_Ntk_t * p )
{
    int fDump = 0;
    Gia_Man_t * pTemp;
    int i, b, nBits = Rtl_NtkRangeWires( p );
    Vec_IntFill( &p->vLits, nBits, -1 );
printf( "Blasting %s...\r", Rtl_NtkName(p) );
    Rtl_NtkMapWires( p, 0 );
    Rtl_NtkBlastMap( p, nBits );
    assert( p->pGia == NULL );
    p->pGia = Gia_ManStart( 1000 );
    p->pGia->vBarBufs = Vec_IntAlloc( 1000 );
    Rtl_NtkBlastInputs( p->pGia, p );
    Gia_ManHashAlloc( p->pGia );
    for ( i = 0; i < p->nOutputs; i++ )
    {
        int First = Rtl_WireBitStart( p, p->nInputs + i );
        int Width = Rtl_WireWidth( p, p->nInputs + i );
        for ( b = 0; b < Width; b++ )
            Rtl_NtkCollectOrComputeBit( p, First+b );
    }
    Gia_ManHashStop( p->pGia );
    Rtl_NtkBlastOutputs( p->pGia, p );
    Rtl_NtkMapWires( p, 1 );
    p->pGia = Gia_ManCleanup( pTemp = p->pGia );
    ABC_SWAP( Vec_Int_t *, p->pGia->vBarBufs, pTemp->vBarBufs );
    Gia_ManStop( pTemp );
//    if ( p->fRoot )
//        Rtl_NtkPrintBufs( p, p->pGia->vBarBufs );
    if ( fDump )
    {
        char Buffer[100]; static int counter = 0;
        sprintf( Buffer, "old%02d.aig", counter++ );
        Gia_AigerWrite( p->pGia, Buffer, 0, 0, 0 );
        printf( "Dumped \"%s\" with AIG for module %-20s : ", Buffer, Rtl_ShortenName(Rtl_NtkName(p), 20) );
    }
    else
        printf( "Derived AIG for module %-20s : ", Rtl_ShortenName(Rtl_NtkName(p), 20) );
    Gia_ManPrintStats( p->pGia, NULL );
    return p->pGia;
}
void Rtl_LibMark_rec( Rtl_Ntk_t * pNtk )
{
    int i, * pCell;
    if ( pNtk->iCopy == -1 )
        return;
    Rtl_NtkForEachCell( pNtk, pCell, i )
    {
        Rtl_Ntk_t * pMod = Rtl_CellNtk( pNtk, pCell );
        if ( pMod )
            Rtl_LibMark_rec( pMod );
    }
    assert( pNtk->iCopy == -2 );
    pNtk->iCopy = -1;
}
void Rtl_LibBlast2( Rtl_Lib_t * pLib, Vec_Int_t * vRoots, int fInv )
{
    Rtl_Ntk_t * pNtk; int i, iNtk;
    Vec_PtrForEachEntry( Rtl_Ntk_t *, pLib->vNtks, pNtk, i )
        pNtk->iCopy = -1;
    if ( vRoots )
    {
        Vec_PtrForEachEntry( Rtl_Ntk_t *, pLib->vNtks, pNtk, i )
            pNtk->iCopy = -2;//, pNtk->fRoot = 0;
        Vec_IntForEachEntry( vRoots, iNtk, i )
        {
            Rtl_Ntk_t * pNtk = Rtl_LibNtk(pLib, iNtk);
            //pNtk->fRoot = fInv;
            Rtl_LibMark_rec( pNtk );
        }
    }
    Vec_PtrForEachEntry( Rtl_Ntk_t *, pLib->vNtks, pNtk, i )
        if ( pNtk->iCopy == -1 && pNtk->pGia == NULL )
            pNtk->pGia = Rtl_NtkBlast2( pNtk );
//    Vec_PtrForEachEntry( Rtl_Ntk_t *, pLib->vNtks, pNtk, i )
//        if ( pNtk->iCopy == -2 )
//            printf( "Skipping network \"%s\" during bit-blasting.\n", Rtl_NtkName(pNtk) );
    Vec_PtrForEachEntry( Rtl_Ntk_t *, pLib->vNtks, pNtk, i )
        pNtk->iCopy = -1;
}
void Rtl_LibBlastClean( Rtl_Lib_t * p )
{
    Rtl_Ntk_t * pNtk; int i;
    Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
        Gia_ManStopP( &pNtk->pGia );
}
void Rtl_LibSetReplace( Rtl_Lib_t * p, Vec_Wec_t * vGuide )
{
    Vec_Int_t * vLevel; int i, iNtk1, iNtk2; 
    Rtl_Ntk_t * pNtk, * pNtk1, * pNtk2;
    Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
        pNtk->iCopy = -1;
    Vec_WecForEachLevel( vGuide, vLevel, i )
    {
        int Type  = Vec_IntEntry( vLevel, 1 );
        int Name1 = Vec_IntEntry( vLevel, 2 );
        int Name2 = Vec_IntEntry( vLevel, 3 );
        int iNtk  = Rtl_LibFindTwoModules( p, Name1, Name2 );
        if ( iNtk == -1 )
        {
            printf( "Cannot find networks \"%s\" and \"%s\" in the design.\n", Rtl_LibStr(p, Name1), Rtl_LibStr(p, Name2) ); 
            break;
        }
        if ( Type != Rtl_LibStrId(p, "equal") )
            continue;
        iNtk1 = iNtk >> 16;
        iNtk2 = iNtk & 0xFFFF;
        pNtk1 = Rtl_LibNtk(p, iNtk1);
        pNtk2 = Rtl_LibNtk(p, iNtk2);
        pNtk1->iCopy = iNtk2;
        if ( iNtk1 == iNtk2 )
            printf( "Preparing to prove \"%s\".\n", Rtl_NtkName(pNtk1) );
        else
            printf( "Preparing to replace \"%s\" by \"%s\".\n", Rtl_NtkName(pNtk1), Rtl_NtkName(pNtk2) );
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtl_LibPreprocess( Rtl_Lib_t * pLib )
{
    abctime clk = Abc_Clock(); 
    Rtl_Ntk_t * p1 = NULL, * p2 = NULL, * p;
    int i, k, Status, fFound = 0;
    printf( "Performing preprocessing for verification.\n" );
    // find similar modules
    Vec_PtrForEachEntry( Rtl_Ntk_t *, pLib->vNtks, p1, i )
    Vec_PtrForEachEntry( Rtl_Ntk_t *, pLib->vNtks, p2, k )
    {
        if ( i >= k )  
            continue;
        if ( Gia_ManCiNum(p1->pGia) != Gia_ManCiNum(p2->pGia) || 
             Gia_ManCoNum(p1->pGia) != Gia_ManCoNum(p2->pGia) )
            continue;
        // two similar modules
        Status = Cec_ManVerifyTwo( p1->pGia, p2->pGia, 0 );
        if ( Status != 1 )
            continue;
        printf( "Proved equivalent modules: %s == %s\n", Rtl_NtkName(p1), Rtl_NtkName(p2) );
        // inline
        if ( Gia_ManAndNum(p1->pGia) > Gia_ManAndNum(p2->pGia) )
            ABC_SWAP( Gia_Man_t *, p1->pGia, p2->pGia );
        assert( Gia_ManAndNum(p1->pGia) <= Gia_ManAndNum(p2->pGia) );
        Gia_ManStopP( &p2->pGia );
        p2->pGia = Gia_ManDup( p1->pGia );
        fFound = 1;
        goto finish;
    }
finish:
    if ( fFound == 0 )
        printf( "Preprocessing not succeded.\n" );
    Abc_PrintTime( 1, "Preprocessing time", Abc_Clock() - clk );
    // blast AIGs again
    Vec_PtrForEachEntry( Rtl_Ntk_t *, pLib->vNtks, p, i )
        if ( p != p1 && p != p2 )
            Gia_ManStopP( &p->pGia );
    //Rtl_LibBlast( pLib );
    Rtl_LibBlast2( pLib, NULL, 0 );
}
void Rtl_LibSolve( Rtl_Lib_t * pLib, void * pNtk )
{
    extern Gia_Man_t * Gia_ManReduceBuffers( Rtl_Lib_t * pLib, Gia_Man_t * p );
    abctime clk = Abc_Clock(); int Status;
    Rtl_Ntk_t * pTop  = pNtk ? (Rtl_Ntk_t *)pNtk : Rtl_LibTop( pLib );
    Gia_Man_t * pGia2 = Gia_ManReduceBuffers( pLib, pTop->pGia );
    Gia_Man_t * pSwp  = Cec4_ManSimulateTest3( pGia2, 1000000, 0 );
    int RetValue = Gia_ManAndNum(pSwp);
    char * pFileName = "miter_to_solve.aig";
    printf( "Dumped the miter into file \"%s\".\n", pFileName );
    Gia_AigerWrite( pGia2, pFileName, 0, 0, 0 );
    Gia_ManStop( pSwp );
    Gia_ManStop( pGia2 );
    if ( RetValue == 0 )
        printf( "Verification problem solved after SAT sweeping!  " );
    else
    {
        Gia_Man_t * pCopy = Gia_ManDup( pTop->pGia );
        Gia_ManInvertPos( pCopy );
        Gia_ManAppendCo( pCopy, 0 );
        Status = Cec_ManVerifySimple( pCopy );
        Gia_ManStop( pCopy );
        if ( Status == 1 )
            printf( "Verification problem solved after CEC!  " );
        else
            printf( "Verification problem is NOT solved (miter has %d nodes)!  ", RetValue );
    }
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wln_SolveEqual( Rtl_Lib_t * p, int iNtk1, int iNtk2 )
{
    abctime clk = Abc_Clock(); 
    Rtl_Ntk_t * pNtk1 = Rtl_LibNtk( p, iNtk1 );
    Rtl_Ntk_t * pNtk2 = Rtl_LibNtk( p, iNtk2 );
    printf( "\nProving equivalence of \"%s\" and \"%s\"...\n", Rtl_NtkName(pNtk1), Rtl_NtkName(pNtk2) );
    if ( Gia_ManCiNum(pNtk1->pGia) != Gia_ManCiNum(pNtk2->pGia) || 
         Gia_ManCoNum(pNtk1->pGia) != Gia_ManCoNum(pNtk2->pGia) )
    {
        printf( "The number of inputs/outputs does not match.\n" );
    }
    else if ( 1 )
    {
        Gia_Man_t * pGia = Gia_ManMiter( pNtk1->pGia, pNtk2->pGia, 0, 0, 0, 0, 0 );
        if ( Abc_NtkFromGiaCollapse( pGia ) )
            Abc_Print( 1, "Networks are equivalent after collapsing.  " );
        else
        {
            Gia_Man_t * pNew = Cec4_ManSimulateTest3( pGia, 10000000, 0 );
            //printf( "Miter %d -> %d\n",  Gia_ManAndNum(pGia),  Gia_ManAndNum(pNew) );
            if ( Gia_ManAndNum(pNew) == 0 )
                Abc_Print( 1, "Networks are equivalent.  " );
            else
                Abc_Print( 1, "Networks are UNDECIDED.  " );
            Gia_ManStopP( &pNew );
            Gia_ManStopP( &pGia );
        }
    }
    else
    {
        int Status = Cec_ManVerifyTwo( pNtk1->pGia, pNtk2->pGia, 0 );
        if ( Status == 1 )
            printf( "The networks are equivalent.  " );
        else
            printf( "The networks are NOT equivalent.  " );
    }
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}
int Gia_ManFindFirst( Rtl_Ntk_t * p, int * pnOuts )
{
    int i, * pWire, Counts[4] = {0}, nBits = 0;
    assert( p->nOutputs == 1 );
    Rtl_NtkForEachWire( p, pWire, i )
    {
        if ( pWire[0] & 1 ) // PI
            Counts[0]++, Counts[1] += pWire[1];
        if ( pWire[0] & 2 ) // PO
            Counts[2]++, Counts[3] += pWire[1];
    }
    assert( p->nInputs  == Counts[0] );
    assert( p->nOutputs == Counts[2] );
    *pnOuts = Counts[3];
    Rtl_NtkForEachWire( p, pWire, i )
    {
        if ( pWire[0] & 1 ) // PI
        {
            if ( pWire[1] == Counts[3] )
                return nBits;
            nBits += pWire[1];
        }
    }
    return -1;
}
Gia_Man_t * Gia_ManMoveSharedFirst( Gia_Man_t * pGia, int iFirst, int nBits )
{
    Vec_Int_t * vPiPerm = Vec_IntAlloc( Gia_ManPiNum(pGia) );
    Gia_Man_t * pTemp; int i, n;
    for ( n = 0; n < 2; n++ )
        for ( i = 0; i < Gia_ManPiNum(pGia); i++ )
            if ( n == (i >= iFirst && i < iFirst + nBits) )
                Vec_IntPush( vPiPerm, i );
    pTemp = Gia_ManDupPerm( pGia, vPiPerm );
    if ( pGia->vBarBufs )
        pTemp->vBarBufs = Vec_IntDup( pGia->vBarBufs );
    Vec_IntFree( vPiPerm );
    return pTemp;
}
Vec_Int_t * Gia_ManCollectBufs( Gia_Man_t * p, int iFirst, int nBufs )
{
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    Gia_Obj_t * pObj; int i, iBuf = 0;
    assert( iFirst >= 0 && iFirst + nBufs < p->nBufs );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Gia_ObjIsBuf(pObj) && iBuf >= iFirst && iBuf < iFirst + nBufs )
            Vec_IntPush( vRes, i );
        iBuf += Gia_ObjIsBuf(pObj);
    }
    assert( iBuf == p->nBufs );
    return vRes;
}
Gia_Man_t * Gia_ManReduceBuffers( Rtl_Lib_t * pLib, Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Vec_Int_t * vMap = Vec_IntStartFull( Gia_ManObjNum(p) );
    Vec_Int_t * vOne = Gia_ManCollectBufs( p, 0,       64 );
    Vec_Int_t * vTwo = Gia_ManCollectBufs( p, 1280-64, 64 );
    //Vec_Int_t * vOne = Gia_ManCollectBufs( p, 0,      1280/2 );
    //Vec_Int_t * vTwo = Gia_ManCollectBufs( p, 1280/2, 1280/2 );
    int i, One, Two;
    printf( "Reducing %d buffers... Size(vOne) = %d. Size(vTwo) = %d. \n", p->nBufs, Vec_IntSize(vOne), Vec_IntSize(vTwo) );
    assert( p->nBufs == 1280 );
    Vec_IntForEachEntryTwo( vOne, vTwo, One, Two, i )
        Vec_IntWriteEntry( vMap, Two, One );
    Vec_IntFree( vOne );
    Vec_IntFree( vTwo );
Gia_ManPrintStats( p, NULL );
    //pNew = Gia_ManDupNoBuf( p );
    pNew = Gia_ManDupMap( p, vMap );
Gia_ManPrintStats( pNew, NULL );
    Vec_IntFree( vMap );
    //Rtl_NtkPrintBufs( pNtk1, pGia->vBarBufs );
    //printf( "Found %d buffers.\n", p->nBufs );
    return pNew;
}
void Wln_SolveInverse( Rtl_Lib_t * p, int iNtk1, int iNtk2 )
{
    abctime clk = Abc_Clock(); 
    Rtl_Ntk_t * pNtk1 = Rtl_LibNtk( p, iNtk1 );
    Rtl_Ntk_t * pNtk2 = Rtl_LibNtk( p, iNtk2 );
    int Res = printf( "\nProving inverse equivalence of \"%s\" and \"%s\".\n", Rtl_NtkName(pNtk1), Rtl_NtkName(pNtk2) );
    int nOuts1, iFirst1 = Gia_ManFindFirst( pNtk1, &nOuts1 );
    int nOuts2, iFirst2 = Gia_ManFindFirst( pNtk2, &nOuts2 );
    Gia_Man_t * pGia1 = Gia_ManMoveSharedFirst( pNtk1->pGia, iFirst1, nOuts1 );
    Gia_Man_t * pGia2 = Gia_ManMoveSharedFirst( pNtk2->pGia, iFirst2, nOuts2 );
    if ( 1 )
    {
        char * pFileName  = "inv_miter.aig";
        Gia_Man_t * pGia  = Gia_ManMiterInverse( pGia1, pGia2, 0, 0 );
        //Gia_Man_t * pGia2 = Gia_ManReduceBuffers( p, pGia );
        Gia_Man_t * pGia2 = Gia_ManDupNoBuf( pGia );
        printf( "Dumping inverse miter into file \"%s\".\n", pFileName );
        Gia_AigerWrite( pGia2, pFileName, 0, 0, 0 );
        printf( "Dumped the miter into file \"%s\".\n", pFileName );
        if ( Abc_NtkFromGiaCollapse( pGia2 ) )
            Abc_Print( 1, "Networks are equivalent after collapsing.  " );
        else
        {
            Gia_Man_t * pNew  = Cec4_ManSimulateTest3( pGia2, 10000000, 0 );
            Rtl_NtkPrintBufs( pNtk1, pGia->vBarBufs );
            //printf( "Miter %d -> %d\n",  Gia_ManAndNum(pGia),  Gia_ManAndNum(pNew) );
            if ( Gia_ManAndNum(pNew) == 0 )
                Abc_Print( 1, "Networks are equivalent.  " );
            else
                Abc_Print( 1, "Networks are UNDECIDED.  " );
            Gia_ManStopP( &pNew );
        }
        Gia_ManStopP( &pGia2 );
        Gia_ManStopP( &pGia );
    }
    else
    {
        int Status = Cec_ManVerifyTwoInv( pGia1, pGia2, 0 );
        if ( Status == 1 )
            printf( "The networks are equivalent.  " );
        else
            printf( "The networks are NOT equivalent.  " );
    }
    Res = 0;
    Gia_ManStopP( &pGia1 );
    Gia_ManStopP( &pGia2 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}
void Wln_SolveProperty( Rtl_Lib_t * p, int iNtk )
{
    Rtl_Ntk_t * pNtk = Rtl_LibNtk( p, iNtk );
    printf( "\nProving property \"%s\".\n", Rtl_NtkName(pNtk) );
    Rtl_NtkPrintBufs( pNtk, pNtk->pGia->vBarBufs );
    Rtl_LibSolve( p, pNtk );
}
Vec_Int_t * Wln_ReadNtkRoots( Rtl_Lib_t * p, Vec_Wec_t * vGuide ) 
{
    Vec_Int_t * vLevel; int i; 
    Vec_Int_t * vRoots = Vec_IntAlloc( 100 ); 
    Vec_WecForEachLevel( vGuide, vLevel, i )
    {
        int Name1 = Vec_IntEntry( vLevel, 2 );
        int Name2 = Vec_IntEntry( vLevel, 3 );
        int iNtk  = Rtl_LibFindTwoModules( p, Name1, Name2 );
        if ( iNtk == -1 )
        {
            printf( "Cannot find networks \"%s\" and \"%s\" in the design.\n", Rtl_LibStr(p, Name1), Rtl_LibStr(p, Name2) ); 
            break;
        }
/*
        else
        {
            Rtl_Ntk_t * pNtk1 = Rtl_LibNtk( p, iNtk >> 16 );
            Rtl_Ntk_t * pNtk2 = Rtl_LibNtk( p, iNtk & 0xFFFF );
            printf( "Matching \"%s\" and \"%s\".\n", Rtl_NtkName(pNtk1), Rtl_NtkName(pNtk2) );
        }
*/
        Vec_IntPushTwo( vRoots, iNtk >> 16, iNtk & 0xFFFF );
    }
    return vRoots;
}
void Wln_SolveWithGuidance( char * pFileName, Rtl_Lib_t * p )
{
    extern Vec_Wec_t * Wln_ReadGuidance( char * pFileName, Abc_Nam_t * p );
    Vec_Wec_t * vGuide = Wln_ReadGuidance( pFileName, p->pManName );
    Vec_Int_t * vRoots, * vLevel; int i, iNtk1, iNtk2, fInv = 0;
    Vec_WecForEachLevel( vGuide, vLevel, i )
        if ( Vec_IntEntry( vLevel, 1 ) == Rtl_LibStrId(p, "inverse") )
            fInv = 1;
    Vec_IntFillExtra( p->vMap, Abc_NamObjNumMax(p->pManName), -1 );
    Rtl_LibSetReplace( p, vGuide );
    Rtl_LibUpdateBoxes( p );
    Rtl_LibReorderModules( p );
    vRoots = Wln_ReadNtkRoots( p, vGuide );
    Rtl_LibBlast2( p, vRoots, fInv );
    Vec_WecForEachLevel( vGuide, vLevel, i )
    {
        int Prove = Vec_IntEntry( vLevel, 0 );
        int Type  = Vec_IntEntry( vLevel, 1 );
        int Name1 = Vec_IntEntry( vLevel, 2 );
        int Name2 = Vec_IntEntry( vLevel, 3 );
        int iNtk  = Rtl_LibFindTwoModules( p, Name1, Name2 );
        if ( iNtk == -1 )
        {
            printf( "Cannot find networks \"%s\" and \"%s\" in the design.\n", Rtl_LibStr(p, Name1), Rtl_LibStr(p, Name2) ); 
            break;
        }
        iNtk1 = iNtk >> 16;
        iNtk2 = iNtk & 0xFFFF;
        if ( Prove != Rtl_LibStrId(p, "prove") )
            printf( "Unknown task in line %d.\n", i );
        //else if ( iNtk1 == -1 )
        //    printf( "Network %s cannot be found in this design.\n", Rtl_LibStr(p, Name1) );
        else
        {
            if ( Type == Rtl_LibStrId(p, "equal") )
                Wln_SolveEqual( p, iNtk1, iNtk2 );
            else if ( Type == Rtl_LibStrId(p, "inverse") )
                Wln_SolveInverse( p, iNtk1, iNtk2 );
            else if ( Type == Rtl_LibStrId(p, "property") )
                Wln_SolveProperty( p, iNtk1 );
            continue;
        }
        break;
    }
    Rtl_LibBlastClean( p );
    Vec_WecFree( vGuide );
    Vec_IntFree( vRoots );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

static inline void Gia_ManPatchBufDriver( Gia_Man_t * p, int iObj, int iLit0 )  
{
    Gia_Obj_t * pObj  = Gia_ManObj( p, iObj );
    assert( iObj > Abc_Lit2Var(iLit0) );
    pObj->iDiff0  = pObj->iDiff1  = iObj - Abc_Lit2Var(iLit0);
    pObj->fCompl0 = pObj->fCompl1 = Abc_LitIsCompl(iLit0);
}

Gia_Man_t * Rtl_ReduceInverse( Rtl_Lib_t * pLib, Gia_Man_t * p )
{
    int fVerbose = 1;
    Gia_Man_t * pNew   = NULL;
    Vec_Wec_t * vBufs  = Vec_WecStart( Vec_IntSize(p->vBarBufs) );
    Vec_Int_t * vPairs = Vec_IntAlloc( 10 );
    Vec_Int_t * vTypes = Vec_IntAlloc( p->nBufs );
    Vec_Int_t * vMap = Vec_IntStartFull( Gia_ManObjNum(p) );
    Gia_Obj_t * pObj; int i, k = 0, Entry, Buf0, Buf1, fChange = 1;
    Vec_IntForEachEntry( p->vBarBufs, Entry, i )
        Vec_IntFillExtra( vTypes, Vec_IntSize(vTypes) + (Entry >> 16), i );
    assert( Vec_IntSize(vTypes) == p->nBufs );
    Gia_ManForEachAnd( p, pObj, i )
        if ( Gia_ObjIsBuf(pObj) )
        {
            Vec_WecPush( vBufs, Vec_IntEntry(vTypes, k), i );
            Vec_IntWriteEntry( vMap, i, Vec_IntEntry(vTypes, k++) );
        }
    assert( k == p->nBufs );
    Gia_ManForEachAnd( p, pObj, i )
        if ( Gia_ObjIsBuf(pObj) && Gia_ObjIsBuf(Gia_ObjFanin0(pObj)) )
            Vec_IntPushUnique( vPairs, (Vec_IntEntry(vMap, Gia_ObjFaninId0(pObj, i)) << 16) | (Vec_IntEntry(vMap, i) & 0xFFFF) );
    if ( fVerbose )
    {
        printf( "Connected boundaries:\n" );
        Vec_IntForEachEntry( vPairs, Entry, i )
        {
            printf( "%2d -> %2d : ", Entry >> 16, Entry & 0xFFFF );
            Rtl_NtkPrintBufOne( pLib, Vec_IntEntry(p->vBarBufs, Entry >> 16) );
            printf( " -> "  );
            Rtl_NtkPrintBufOne( pLib, Vec_IntEntry(p->vBarBufs, Entry & 0xFFFF) );
            printf( "\n" );
        }
    }
    while ( fChange )
    {
        int Entry1, Entry2, j;
        fChange = 0;
        Vec_IntForEachEntryDouble( vPairs, Entry1, Entry2, j )
            if ( (Entry1 & 0xFFFF) + 1 == (Entry2 >> 16) ) 
            {
                Vec_IntWriteEntry( vPairs, j, ((Entry1 >> 16) << 16) | (Entry2 & 0xFFFF) );
                Vec_IntDrop( vPairs, j+1 );  
                fChange = 1;
                break;
            }
    }
//    printf( "Before:\n" );
//    Vec_IntForEachEntry( vPairs, Entry, i )
//        printf( "%d %d\n", Entry >> 16, Entry & 0xFFFF );
    Vec_IntForEachEntry( vPairs, Entry, i )
        Vec_IntWriteEntry( vPairs, i, (((Entry >> 16) - 1) << 16) | ((Entry & 0xFFFF) + 1) );
//    printf( "After:\n" );
//    Vec_IntForEachEntry( vPairs, Entry, i )
//        printf( "%d %d\n", Entry >> 16, Entry & 0xFFFF );
    if ( fVerbose )
    {
        printf( "Transformed boundaries:\n" );
        Vec_IntForEachEntry( vPairs, Entry, i )
        {
            printf( "%2d -> %2d : ", Entry >> 16, Entry & 0xFFFF );
            Rtl_NtkPrintBufOne( pLib, Vec_IntEntry(p->vBarBufs, Entry >> 16) );
            printf( " -> "  );
            Rtl_NtkPrintBufOne( pLib, Vec_IntEntry(p->vBarBufs, Entry & 0xFFFF) );
            printf( "\n" );
        }
    }
    Vec_IntForEachEntry( vPairs, Entry, i )
    {
        Vec_Int_t * vLevel0 = Vec_WecEntry( vBufs, Entry >> 16    );
        Vec_Int_t * vLevel1 = Vec_WecEntry( vBufs, Entry & 0xFFFF );
        Vec_IntForEachEntryTwo( vLevel0, vLevel1, Buf0, Buf1, k )
            Gia_ManPatchBufDriver( p, Buf1, Gia_ObjFaninLit0(Gia_ManObj(p, Buf0), Buf0) );
    }
    pNew = Gia_ManRehash( p, 0 );
    Vec_IntFree( vPairs );
    Vec_IntFree( vTypes );
    Vec_IntFree( vMap );
    Vec_WecFree( vBufs );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupPermIO( Gia_Man_t * p, Vec_Int_t * vPermI, Vec_Int_t * vPermO )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    assert( Vec_IntSize(vPermI) == Gia_ManCiNum(p) );
    assert( Vec_IntSize(vPermO) == Gia_ManCoNum(p) );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav(p->pName);
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        Gia_ManCi(p, Vec_IntEntry(vPermI, i))->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Gia_ObjIsBuf(pObj) )
            pObj->Value = Gia_ManAppendBuf( pNew, Gia_ObjFanin0Copy(pObj) );
        else
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        assert( Abc_Lit2Var(pObj->Value) == i );
    }
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(Gia_ManCo(p, Vec_IntEntry(vPermO, i))) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rtl_LibReturnNtk( Rtl_Lib_t * p, char * pModule )
{
    int NameId = Wln_ReadFindToken( pModule, p->pManName );
    int iNtk   = NameId ? Rtl_LibFindModule( p, NameId )  : -1;
    if ( iNtk == -1 )
    {
        printf( "Cannot find module \"%s\" in the current design.\n", pModule );
        return -1;
    }
    return iNtk;
}
Gia_Man_t * Rtl_LibCollapse( Rtl_Lib_t * p, char * pTopModule, int fRev, int fVerbose )
{
    Gia_Man_t * pGia = NULL;
    int NameId = Wln_ReadFindToken( pTopModule, p->pManName );
    int iNtk   = NameId ? Rtl_LibFindModule( p, NameId ) : -1;
    if ( iNtk == -1 )
    {
        printf( "Cannot find top module \"%s\".\n", pTopModule );
        return NULL;
    }
    else
    {
        abctime clk = Abc_Clock(); 
        Rtl_Ntk_t * pTop = Rtl_LibNtk(p, iNtk);
        Vec_Int_t * vRoots = Vec_IntAlloc( 1 );
        Vec_IntPush( vRoots, iNtk );
        Rtl_LibBlast2( p, vRoots, 1 );
        pGia = Gia_ManDup( pTop->pGia );
        if ( fRev )
        {
            Gia_Man_t * pTemp;
            Vec_Int_t * vPermI = Rtl_NtkRevPermInput( pTop );
            Vec_Int_t * vPermO = Rtl_NtkRevPermOutput( pTop );
            pGia = Gia_ManDupPermIO( pTemp = pGia, vPermI, vPermO );
            Vec_IntFree( vPermI );
            Vec_IntFree( vPermO );
            Gia_ManStop( pTemp );
        }
        //Gia_AigerWrite( pGia, "temp_miter.aig", 0, 0, 0 );
        if ( pTop->pGia->vBarBufs )
            pGia->vBarBufs = Vec_IntDup( pTop->pGia->vBarBufs );
        printf( "Derived global AIG for the top module \"%s\".  ", Rtl_NtkStr(pTop, NameId) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        Rtl_NtkPrintBufs( pTop, pGia->vBarBufs );
        Rtl_LibBlastClean( p );
        Vec_IntFree( vRoots );
        if ( p->vInverses )
        {
            Gia_Man_t * pTemp;
            pGia = Rtl_ReduceInverse( p, pTemp = pGia );
            Gia_ManStop( pTemp );
        }
    }
    return pGia;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wln_LibGraftOne( Rtl_Lib_t * p, char ** pModules, int nModules, int fInv, int fVerbose )
{
    if ( nModules == 0 )
    {
        Rtl_Ntk_t * pNtk; int i;
        Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
            pNtk->iCopy = -1;
        Vec_IntFreeP( &p->vInverses );
        if ( p->vDirects )
        {
            int iName1, iName2;
            Vec_IntForEachEntryDouble( p->vDirects, iName1, iName2, i )
            {
                int iNtk1 = Rtl_LibFindModule(p, iName1);
                int iNtk2 = Rtl_LibFindModule(p, iName2);
                //Rtl_Ntk_t * pNtk1 = Rtl_LibNtk( p, iNtk1 );
                Rtl_Ntk_t * pNtk2 = Rtl_LibNtk( p, iNtk2 );
                pNtk2->iCopy = iNtk1;
            }
            Rtl_LibUpdateBoxes( p );
            Rtl_LibReorderModules( p );
            Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
                pNtk->iCopy = -1;
            Vec_IntFreeP( &p->vDirects );
        }
    }
    else
    {
        int Name1 = Wln_ReadFindToken( pModules[0], p->pManName );
        int Name2 = Wln_ReadFindToken( pModules[1], p->pManName );
        int iNtk  = Rtl_LibFindTwoModules( p, Name1, Name2 ); 
        if ( iNtk == -1 )
        {
            printf( "Cannot find networks \"%s\" and \"%s\" in the design.\n", Rtl_LibStr(p, Name1), Rtl_LibStr(p, Name2) ); 
            return;
        }
        else
        {
            int iNtk1 = iNtk >> 16;
            int iNtk2 = iNtk & 0xFFFF;
            Rtl_Ntk_t * pNtk1 = Rtl_LibNtk(p, iNtk1);
            Rtl_Ntk_t * pNtk2 = Rtl_LibNtk(p, iNtk2);
            assert( iNtk1 != iNtk2 );
            if ( fInv )
            {
                printf( "Setting \"%s\" (appearing %d times) and \"%s\" (appearing %d times) as inverse-equivalent.\n", 
                    Rtl_NtkName(pNtk1), Rtl_LibCountInsts(p, pNtk1), Rtl_NtkName(pNtk2), Rtl_LibCountInsts(p, pNtk2) );
                if ( p->vInverses == NULL )
                    p->vInverses = Vec_IntAlloc( 10 );
                Vec_IntPushTwo( p->vInverses, pNtk1->NameId, pNtk2->NameId );
            }
            else
            {
                printf( "Replacing \"%s\" (appearing %d times) by \"%s\" (appearing %d times).\n", 
                    Rtl_NtkName(pNtk1), Rtl_LibCountInsts(p, pNtk1), Rtl_NtkName(pNtk2), Rtl_LibCountInsts(p, pNtk2) );
                pNtk1->iCopy = iNtk2;
            //    Rtl_LibSetReplace( p, vGuide );
                Rtl_LibUpdateBoxes( p );
                Rtl_LibReorderModules( p );
                if ( p->vDirects == NULL )
                    p->vDirects = Vec_IntAlloc( 10 );
                Vec_IntPushTwo( p->vDirects, pNtk1->NameId, pNtk2->NameId );
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wln_LibMarkHierarchy( Rtl_Lib_t * p, char ** ppModule, int nModules, int fVerbose )
{
    Rtl_Ntk_t * pNtk; int i;
    if ( nModules == 0 ) // clean labels
        Vec_PtrForEachEntry( Rtl_Ntk_t *, p->vNtks, pNtk, i )
            pNtk->fRoot = 0;
    for ( i = 0; i < nModules; i++ )
    {
        int iNtk = Rtl_LibReturnNtk( p, ppModule[i] );
        if ( iNtk == -1 )
            continue;
        pNtk = Rtl_LibNtk( p, iNtk );
        pNtk->fRoot = 1;
        printf( "Marking module \"%s\" (appearing %d times in the hierarchy).\n", ppModule[i], Rtl_LibCountInsts(p, pNtk) );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

