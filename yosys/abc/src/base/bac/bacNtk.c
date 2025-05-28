/**CFile****************************************************************

  FileName    [bacNtk.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Netlist manipulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: bacNtk.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bac.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Bac_Pair_t_ Bac_Pair_t;
struct Bac_Pair_t_
{
    Bac_ObjType_t Type;
    char *        pName;
    char *        pSymb;
};
static const char * s_Pref = "ABC_";
static Bac_Pair_t s_Types[BAC_BOX_UNKNOWN] =
{ 
    { BAC_OBJ_NONE,     "NONE",    NULL    },
    { BAC_OBJ_PI,       "PI",      NULL    },
    { BAC_OBJ_PO,       "PO",      NULL    },
    { BAC_OBJ_BI,       "BI",      NULL    },
    { BAC_OBJ_BO,       "BO",      NULL    },
    { BAC_OBJ_BOX,      "BOX",     NULL    },
    
    { BAC_BOX_CF,       "CF",      "o"     },
    { BAC_BOX_CT,       "CT",      "o"     }, 
    { BAC_BOX_CX,       "CX",      "o"     }, 
    { BAC_BOX_CZ,       "CZ",      "o"     },
    { BAC_BOX_BUF,      "BUF",     "ao"    },
    { BAC_BOX_INV,      "INV",     "ao"    },
    { BAC_BOX_AND,      "AND",     "abo"   },
    { BAC_BOX_NAND,     "NAND",    "abo"   },
    { BAC_BOX_OR,       "OR",      "abo"   },
    { BAC_BOX_NOR,      "NOR",     "abo"   },
    { BAC_BOX_XOR,      "XOR",     "abo"   },
    { BAC_BOX_XNOR,     "XNOR",    "abo"   },
    { BAC_BOX_SHARP,    "SHARP",   "abo"   },
    { BAC_BOX_SHARPL,   "SHARPL",  "abo"   },
    { BAC_BOX_MUX,      "MUX",     "cabo"  },
    { BAC_BOX_MAJ,      "MAJ",     "abco"  },
    
    { BAC_BOX_RAND,     "RAND",    "ao"    },
    { BAC_BOX_RNAND,    "RNAND",   "ao"    },
    { BAC_BOX_ROR,      "ROR",     "ao"    },
    { BAC_BOX_RNOR,     "RNOR",    "ao"    },
    { BAC_BOX_RXOR,     "RXOR",    "ao"    },
    { BAC_BOX_RXNOR,    "RXNOR",   "ao"    },
    
    { BAC_BOX_LAND,     "LAND",    "abo"   },
    { BAC_BOX_LNAND,    "LNAND",   "abo"   },
    { BAC_BOX_LOR,      "LOR",     "abo"   },
    { BAC_BOX_LNOR,     "LNOR",    "abo"   },
    { BAC_BOX_LXOR,     "LXOR",    "abo"   },
    { BAC_BOX_LXNOR,    "LXNOR",   "abo"   },
    
    { BAC_BOX_NMUX,     "NMUX",    "abo"   },
    { BAC_BOX_SEL,      "SEL",     "abo"   },
    { BAC_BOX_PSEL,     "PSEL",    "iabo"  },
    { BAC_BOX_ENC,      "ENC",     "ao"    },
    { BAC_BOX_PENC,     "PENC",    "ao"    },
    { BAC_BOX_DEC,      "DEC",     "ao"    },
    { BAC_BOX_EDEC,     "EDEC",    "abo"   },
    
    { BAC_BOX_ADD,      "ADD",     "iabso" },
    { BAC_BOX_SUB,      "SUB",     "abo"   },
    { BAC_BOX_MUL,      "MUL",     "abo"   },
    { BAC_BOX_DIV,      "DIV",     "abo"   },
    { BAC_BOX_MOD,      "MOD",     "abo"   },
    { BAC_BOX_REM,      "REM",     "abo"   },
    { BAC_BOX_POW,      "POW",     "abo"   },
    { BAC_BOX_MIN,      "MIN",     "ao"    },
    { BAC_BOX_ABS,      "ABS",     "ao"    },

    { BAC_BOX_LTHAN,    "LTHAN",   "iabo"  },
    { BAC_BOX_LETHAN,   "LETHAN",  "abo"   },
    { BAC_BOX_METHAN,   "METHAN",  "abo"   },
    { BAC_BOX_MTHAN,    "MTHAN",   "abo"   },
    { BAC_BOX_EQU,      "EQU",     "abo"   },
    { BAC_BOX_NEQU,     "NEQU",    "abo"   },
    
    { BAC_BOX_SHIL,     "SHIL",    "abo"   },
    { BAC_BOX_SHIR,     "SHIR",    "abo"   },
    { BAC_BOX_ROTL,     "ROTL",    "abo"   },
    { BAC_BOX_ROTR,     "ROTR",    "abo"   },

    { BAC_BOX_GATE,     "GATE",    "io"    },
    { BAC_BOX_LUT,      "LUT",     "io"    },
    { BAC_BOX_ASSIGN,   "ASSIGN",  "abo"   },
    
    { BAC_BOX_TRI,      "TRI",     "abo"   },
    { BAC_BOX_RAM,      "RAM",     "eadro" },
    { BAC_BOX_RAMR,     "RAMR",    "eamo"  },
    { BAC_BOX_RAMW,     "RAMW",    "eado"  },
    { BAC_BOX_RAMWC,    "RAMWC",   "ceado" },
    { BAC_BOX_RAMBOX,   "RAMBOX",  "io"    },
    
    { BAC_BOX_LATCH,    "LATCH",   "dvsgq" },
    { BAC_BOX_LATCHRS,  "LATCHRS", "dsrgq" },
    { BAC_BOX_DFF,      "DFF",     "dvscq" },
    { BAC_BOX_DFFRS,    "DFFRS",   "dsrcq" }
}; 
static inline int Bac_GetTypeId( Bac_ObjType_t Type )
{
    int i;
    for ( i = 1; i < BAC_BOX_UNKNOWN; i++ )
        if ( s_Types[i].Type == Type )
            return i;
    return -1;
}
void Bac_ManSetupTypes( char ** pNames, char ** pSymbs )
{
    int Type;
    for ( Type = 1; Type < BAC_BOX_UNKNOWN; Type++ )
    {
        int Id = Bac_GetTypeId( (Bac_ObjType_t)Type );
        pNames[Type] = s_Types[Id].pName;
        pSymbs[Type] = s_Types[Id].pSymb;
    }
}

char * Bac_NtkGenerateName( Bac_Ntk_t * p, Bac_ObjType_t Type, Vec_Int_t * vBits )
{
    static char Buffer[100]; 
    char * pTemp; int i, Bits;
    char * pName = Bac_ManPrimName( p->pDesign, Type );
    char * pSymb = Bac_ManPrimSymb( p->pDesign, Type );
    assert( Vec_IntSize(vBits) == (int)strlen(pSymb) );
    sprintf( Buffer, "%s%s_", s_Pref, pName );
    pTemp = Buffer + strlen(Buffer);
    Vec_IntForEachEntry( vBits, Bits, i )
    {
        sprintf( pTemp, "%c%d", pSymb[i], Bits );
        pTemp += strlen(pTemp);
    }
    //Vec_IntPrint( vBits );
    //printf( "%s\n", Buffer );
    return Buffer;
}

Bac_ObjType_t Bac_NameToType( char * pName )
{
    int i;
    if ( strncmp(pName, s_Pref, strlen(s_Pref)) )
        return BAC_OBJ_NONE;
    pName += strlen(s_Pref);
    for ( i = 1; i < BAC_BOX_UNKNOWN; i++ )
        if ( !strncmp(pName, s_Types[i].pName, strlen(s_Types[i].pName)) )
            return s_Types[i].Type;
    return BAC_OBJ_NONE;
}
Vec_Int_t * Bac_NameToRanges( char * pName )
{
    static Vec_Int_t Bits, * vBits = &Bits;
    static int pArray[10];
    char * pTemp; 
    int Num = 0, Count = 0;
    // initialize array
    vBits->pArray = pArray;
    vBits->nSize = 0;
    vBits->nCap = 10;
    // check the name
    assert( !strncmp(pName, s_Pref, strlen(s_Pref)) );
    for ( pTemp = pName; *pTemp && !Bac_CharIsDigit(*pTemp); pTemp++ );
    assert( Bac_CharIsDigit(*pTemp) );
    for ( ; *pTemp; pTemp++ )
    {
        if ( Bac_CharIsDigit(*pTemp) )
            Num = 10 * Num + *pTemp - '0';
        else
            Vec_IntPush( vBits, Num ), Count += Num, Num = 0;
    }
    assert( Num > 0 );
    Vec_IntPush( vBits, Num );  Count += Num;
    assert( Vec_IntSize(vBits) <= 10 );
    return vBits;
}


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Replaces fanin iOld by iNew in all fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bac_NtkUpdateFanout( Bac_Ntk_t * p, int iOld, int iNew )
{
    int iCo;
    assert( Bac_ObjIsCi(p, iOld) );
    assert( Bac_ObjIsCi(p, iNew) );
    Bac_ObjForEachFanout( p, iOld, iCo )
    {
        assert( Bac_ObjFanin(p, iCo) == iOld );
        Bac_ObjCleanFanin( p, iCo );
        Bac_ObjSetFanin( p, iCo, iNew );
    }
    Bac_ObjSetFanout( p, iNew, Bac_ObjFanout(p, iOld) );
    Bac_ObjSetFanout( p, iOld, 0 );
}

/**Function*************************************************************

  Synopsis    [Derives fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bac_NtkDeriveFanout( Bac_Ntk_t * p )
{
    int iCi, iCo;
    assert( !Bac_NtkHasFanouts(p) );
    Bac_NtkStartFanouts( p );
    Bac_NtkForEachCo( p, iCo )
    {
        assert( !Bac_ObjNextFanout(p, iCo) );
        iCi = Bac_ObjFanin(p, iCo);
        if ( Bac_ObjFanout(p, iCi) )
            Bac_ObjSetNextFanout( p, Bac_ObjFanout(p, iCi), iCo );
        Bac_ObjSetFanout( p, iCi, iCo );
    }
    Bac_NtkForEachCo( p, iCo )
        if ( !Bac_ObjNextFanout(p, iCo) )
            Bac_ObjSetFanout( p, Bac_ObjFanin(p, iCo), iCo );
}
void Bac_ManDeriveFanout( Bac_Man_t * p )
{
    Bac_Ntk_t * pNtk; int i;
    Bac_ManForEachNtk( p, pNtk, i )
        Bac_NtkDeriveFanout( pNtk );
}

/**Function*************************************************************

  Synopsis    [Assigns word-level names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bac_ManAssignInternTwo( Bac_Ntk_t * p, int iNum, int nDigits, char * pPref, Vec_Int_t * vMap )
{
    char Buffer[16]; int i, NameId = 0;
    for ( i = 0; !NameId || Vec_IntEntry(vMap, NameId); i++ )
    {
        if ( i == 0 )
            sprintf( Buffer, "%s%0*d", pPref, nDigits, iNum );
        else
            sprintf( Buffer, "%s%0*d_%d", pPref, nDigits, iNum, i );
        NameId = Abc_NamStrFindOrAdd( p->pDesign->pStrs, Buffer, NULL );
    }
    Vec_IntWriteEntry( vMap, NameId, 1 );
    return NameId;
}
int Bac_ManAssignCountNames( Bac_Ntk_t * p )
{
    int i, iObj, iBox, Count = 0;
    Bac_NtkForEachPiMain( p, iObj, i )
        if ( !Bac_ObjNameInt(p, iObj) )
            Count++;
    Bac_NtkForEachBox( p, iBox )
        Bac_BoxForEachBoMain( p, iBox, iObj, i )
            if ( !Bac_ObjNameInt(p, iObj) )
                Count++;
    return Count;
}
void Bac_ManAssignInternWordNamesNtk( Bac_Ntk_t * p, Vec_Int_t * vMap )
{
    int k, iObj, iTerm, iName = -1, iBit = -1;
    int nDigits, nPis = 0, nPos = 0, nNames = 1;
    // start names
    if ( !Bac_NtkHasNames(p) )
        Bac_NtkStartNames(p);
    nDigits = Abc_Base10Log( Bac_ManAssignCountNames(p) );
    // populate map with the currently used names
    Bac_NtkForEachCi( p, iObj )
        if ( Bac_ObjNameInt(p, iObj) )
            Vec_IntWriteEntry( vMap, Bac_ObjNameId(p, iObj), 1 );
    Bac_NtkForEachBox( p, iObj )
        if ( Bac_ObjNameInt(p, iObj) )
            Vec_IntWriteEntry( vMap, Bac_ObjNameId(p, iObj), 1 );
    // assign CI names
    Bac_NtkForEachCi( p, iObj )
    {
        if ( Bac_ObjNameInt(p, iObj) )
        {
            iName = -1;
            iBit = -1;
            continue;
        }
        if ( Bac_ObjBit(p, iObj) )
        {
            assert( iBit > 0 );
            Bac_ObjSetName( p, iObj, Abc_Var2Lit2(iBit++, BAC_NAME_INDEX) );
        }
        else
        {
            //int Type = Bac_ObjType(p, iObj);
            int Range = Bac_ObjIsPi(p, iObj) ? Bac_ObjPiRange(p, iObj) : Bac_BoxBoRange(p, iObj);
            iName = Bac_ManAssignInternTwo( p, nNames++, nDigits, (char*)(Bac_ObjIsPi(p, iObj) ? "i":"n"), vMap );
            if ( Range == 1 )
                Bac_ObjSetName( p, iObj, Abc_Var2Lit2(iName, BAC_NAME_BIN) );
            else
                Bac_ObjSetName( p, iObj, Abc_Var2Lit2(iName, BAC_NAME_WORD) );
            iBit = 1;
        }
    }
    // transfer names to the interface
    if ( Bac_NtkInfoNum(p) )
    {
        for ( k = 0; k < Bac_NtkInfoNum(p); k++ )
        {
            //char * pName = Bac_NtkName(p);
            if ( Bac_NtkInfoType(p, k) == 1 ) // PI
            {
                iObj = Bac_NtkPi(p, nPis);
                assert( !Bac_ObjBit(p, iObj) );
                assert( Bac_ObjNameType(p, iObj) <= BAC_NAME_WORD );
                Bac_NtkSetInfoName( p, k, Abc_Var2Lit2(Bac_ObjNameId(p, iObj), 1) );
                nPis += Bac_NtkInfoRange(p, k);
            }
            else if ( Bac_NtkInfoType(p, k) == 2 ) // PO
            {
                iObj = Bac_NtkPo(p, nPos);
                assert( !Bac_ObjBit(p, iObj) );
                iObj = Bac_ObjFanin(p, iObj);
                assert( Bac_ObjNameType(p, iObj) <= BAC_NAME_WORD );
                Bac_NtkSetInfoName( p, k, Abc_Var2Lit2(Bac_ObjNameId(p, iObj), 2) );
                nPos += Bac_NtkInfoRange(p, k);
            }
            else assert( 0 );
        }
        assert( nPis == Bac_NtkPiNum(p) );
        assert( nPos == Bac_NtkPoNum(p) );
    }
    // assign instance names
    nDigits = Abc_Base10Log( Bac_NtkObjNum(p) );
    Bac_NtkForEachBox( p, iObj )
        if ( !Bac_ObjNameInt(p, iObj) )
        {
            iName = Bac_ManAssignInternTwo( p, iObj, nDigits, "g", vMap );
            Bac_ObjSetName( p, iObj, Abc_Var2Lit2(iName, BAC_NAME_BIN) );
        }
    // unmark all names
    Bac_NtkForEachPi( p, iObj, k )
        if ( Bac_ObjNameType(p, iObj) <= BAC_NAME_WORD )
            Vec_IntWriteEntry( vMap, Bac_ObjNameId(p, iObj), 0 );
    Bac_NtkForEachBox( p, iObj )
    {
        Vec_IntWriteEntry( vMap, Bac_ObjNameId(p, iObj), 0 );
        Bac_BoxForEachBo( p, iObj, iTerm, k )
            if ( Bac_ObjNameType(p, iTerm) <= BAC_NAME_WORD )
                Vec_IntWriteEntry( vMap, Bac_ObjNameId(p, iTerm), 0 );
    }
//    printf( "Generated %d word-level names.\n", nNames-1 );
}
void Bac_ManAssignInternWordNames( Bac_Man_t * p )
{
    Vec_Int_t * vMap = Vec_IntStart( 2*Bac_ManObjNum(p) );
    Bac_Ntk_t * pNtk; int i;
    Bac_ManForEachNtk( p, pNtk, i )
        Bac_ManAssignInternWordNamesNtk( pNtk, vMap );
    assert( Vec_IntCountEntry(vMap, 0) == Vec_IntSize(vMap) );
    Vec_IntFree( vMap );
}


/**Function*************************************************************

  Synopsis    [Count number of objects after collapsing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bac_ManClpObjNum_rec( Bac_Ntk_t * p )
{
    int i, Counter = 0;
    if ( p->Count >= 0 )
        return p->Count;
    Bac_NtkForEachBox( p, i )
        Counter += Bac_ObjIsBoxUser(p, i) ? Bac_ManClpObjNum_rec( Bac_BoxNtk(p, i) ) + 3*Bac_BoxBoNum(p, i) : Bac_BoxSize(p, i);
    return (p->Count = Counter);
}
int Bac_ManClpObjNum( Bac_Man_t * p )
{
    Bac_Ntk_t * pNtk; int i;
    Bac_ManForEachNtk( p, pNtk, i )
        pNtk->Count = -1;
    return Bac_NtkPioNum( Bac_ManRoot(p) ) + Bac_ManClpObjNum_rec( Bac_ManRoot(p) );
}

/**Function*************************************************************

  Synopsis    [Collects boxes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bac_NtkDfs_rec( Bac_Ntk_t * p, int iObj, Vec_Int_t * vBoxes )
{
    int k, iFanin;
    if ( Bac_ObjIsBo(p, iObj) == 1 )
    {
        Bac_NtkDfs_rec( p, Bac_ObjFanin(p, iObj), vBoxes );
        return;
    }
    assert( Bac_ObjIsPi(p, iObj) || Bac_ObjIsBox(p, iObj) );
    if ( Bac_ObjCopy(p, iObj) > 0 ) // visited
        return;
    Bac_ObjSetCopy( p, iObj, 1 );
    Bac_BoxForEachFanin( p, iObj, iFanin, k )
        Bac_NtkDfs_rec( p, iFanin, vBoxes );
    Vec_IntPush( vBoxes, iObj );
}
Vec_Int_t * Bac_NtkDfs( Bac_Ntk_t * p )
{
    int i, iObj;
    Vec_Int_t * vBoxes = Vec_IntAlloc( Bac_NtkBoxNum(p) );
    Bac_NtkStartCopies( p ); // -1 = not visited; 1 = finished
    Bac_NtkForEachPi( p, iObj, i )
        Bac_ObjSetCopy( p, iObj, 1 );
    Bac_NtkForEachPo( p, iObj, i )
        Bac_NtkDfs_rec( p, Bac_ObjFanin(p, iObj), vBoxes );
    return vBoxes;
}

/**Function*************************************************************

  Synopsis    [Collects user boxes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bac_NtkDfsUserBoxes_rec( Bac_Ntk_t * p, int iObj, Vec_Int_t * vBoxes )
{
    int k, iFanin;
    assert( Bac_ObjIsBoxUser(p, iObj) );
    if ( Bac_ObjCopy(p, iObj) == 1 ) // visited
        return 1;
    if ( Bac_ObjCopy(p, iObj) == 0 ) // loop
        return 0;
    Bac_ObjSetCopy( p, iObj, 0 );
    Bac_BoxForEachFanin( p, iObj, iFanin, k )
        if ( Bac_ObjIsBo(p, iFanin) && Bac_ObjIsBoxUser(p, Bac_ObjFanin(p, iFanin)) )
            if ( !Bac_NtkDfsUserBoxes_rec( p, Bac_ObjFanin(p, iFanin), vBoxes ) )
                return 0;
    Vec_IntPush( vBoxes, iObj );
    Bac_ObjSetCopy( p, iObj, 1 );
    return 1;
}
int Bac_NtkDfsUserBoxes( Bac_Ntk_t * p )
{
    int iObj;
    Bac_NtkStartCopies( p ); // -1 = not visited; 0 = on the path; 1 = finished
    Vec_IntClear( &p->vArray );
    Bac_NtkForEachBoxUser( p, iObj )
        if ( !Bac_NtkDfsUserBoxes_rec( p, iObj, &p->vArray ) )
        {
            printf( "Cyclic dependency of user boxes is detected.\n" );
            return 0;
        }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bac_NtkCollapse_rec( Bac_Ntk_t * pNew, Bac_Ntk_t * p, Vec_Int_t * vSigs )
{
    int i, iObj, iObjNew, iTerm;
    Bac_NtkStartCopies( p );
    // set PI copies
    assert( Vec_IntSize(vSigs) == Bac_NtkPiNum(p) );
    Bac_NtkForEachPi( p, iObj, i )
        Bac_ObjSetCopy( p, iObj, Vec_IntEntry(vSigs, i) );
    // duplicate internal objects and create buffers for hierarchy instances
    Bac_NtkForEachBox( p, iObj )
        if ( Bac_ObjIsBoxPrim( p, iObj ) )
            Bac_BoxDup( pNew, p, iObj );
        else
        {
            Bac_BoxForEachBo( p, iObj, iTerm, i )
            {
                iObjNew = Bac_ObjAlloc( pNew, BAC_OBJ_BI,  -1 );
                iObjNew = Bac_ObjAlloc( pNew, BAC_BOX_BUF, -1 ); // buffer
                iObjNew = Bac_ObjAlloc( pNew, BAC_OBJ_BO,  -1 );
                Bac_ObjSetCopy( p, iTerm, iObjNew );
            }
        }
    // duplicate user modules and connect objects
    Bac_NtkForEachBox( p, iObj )
        if ( Bac_ObjIsBoxPrim( p, iObj ) )
        {
            Bac_BoxForEachBi( p, iObj, iTerm, i )
                Bac_ObjSetFanin( pNew, Bac_ObjCopy(p, iTerm), Bac_ObjCopy(p, Bac_ObjFanin(p, iTerm)) );
        }
        else
        {
            Vec_IntClear( vSigs );
            Bac_BoxForEachBi( p, iObj, iTerm, i )
                Vec_IntPush( vSigs, Bac_ObjCopy(p, Bac_ObjFanin(p, iTerm)) );
            Bac_NtkCollapse_rec( pNew, Bac_BoxNtk(p, iObj), vSigs );
            assert( Vec_IntSize(vSigs) == Bac_BoxBoNum(p, iObj) );
            Bac_BoxForEachBo( p, iObj, iTerm, i )
                Bac_ObjSetFanin( pNew, Bac_ObjCopy(p, iTerm)-2, Vec_IntEntry(vSigs, i) );
        }
    // collect POs
    Vec_IntClear( vSigs );
    Bac_NtkForEachPo( p, iObj, i )
        Vec_IntPush( vSigs, Bac_ObjCopy(p, Bac_ObjFanin(p, iObj)) );
}
Bac_Man_t * Bac_ManCollapse( Bac_Man_t * p )
{
    int i, iObj;
    Vec_Int_t * vSigs = Vec_IntAlloc( 1000 );
    Bac_Man_t * pNew = Bac_ManStart( p, 1 );
    Bac_Ntk_t * pRoot = Bac_ManRoot( p );
    Bac_Ntk_t * pRootNew = Bac_ManRoot( pNew );
    Bac_NtkAlloc( pRootNew, Bac_NtkNameId(pRoot), Bac_NtkPiNum(pRoot), Bac_NtkPoNum(pRoot), Bac_ManClpObjNum(p) );
    if ( Vec_IntSize(&pRoot->vInfo) )
        Vec_IntAppend( &pRootNew->vInfo, &pRoot->vInfo );
    Bac_NtkForEachPi( pRoot, iObj, i )
        Vec_IntPush( vSigs, Bac_ObjAlloc(pRootNew, BAC_OBJ_PI, -1) );
    Bac_NtkCollapse_rec( pRootNew, pRoot, vSigs );
    assert( Vec_IntSize(vSigs) == Bac_NtkPoNum(pRoot) );
    Bac_NtkForEachPo( pRoot, iObj, i )
        Bac_ObjAlloc( pRootNew, BAC_OBJ_PO, Vec_IntEntry(vSigs, i) );
    assert( Bac_NtkObjNum(pRootNew) == Bac_NtkObjNumAlloc(pRootNew) );
    Vec_IntFree( vSigs );
    // transfer PI/PO names
    if ( Bac_NtkHasNames(pRoot) )
    {
        Bac_NtkStartNames( pRootNew );
        Bac_NtkForEachPi( pRoot, iObj, i )
            Bac_ObjSetName( pRootNew, Bac_NtkPi(pRootNew, i), Bac_ObjName(pRoot, iObj) );
        Bac_NtkForEachPoDriver( pRoot, iObj, i )
            if ( !Bac_ObjIsPi(pRoot, iObj) )
                Bac_ObjSetName( pRootNew, Bac_ObjCopy(pRoot, iObj), Bac_ObjName(pRoot, iObj) );
    }
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
