/**CFile****************************************************************

  FileName    [wlcParse.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Parses several flavors of word-level Verilog.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcParse.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"
#include "misc/vec/vecWec.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// parser
typedef struct Smt_Prs_t_ Smt_Prs_t;
struct Smt_Prs_t_
{
    // input data
    char *       pName;       // file name
    char *       pBuffer;     // file contents
    char *       pLimit;      // end of file
    char *       pCur;        // current position
    Abc_Nam_t *  pStrs;       // string manager
    // network structure
    Vec_Int_t    vStack;      // current node on each level
    //Vec_Wec_t    vDepth;      // objects on each level
    Vec_Wec_t    vObjs;       // objects
    int          NameCount;
    int          nDigits;
    Vec_Int_t    vTempFans; 
    // error handling
    char ErrorStr[1000];     
};

//#define SMT_GLO_SUFFIX "_glb"
#define SMT_GLO_SUFFIX ""

// parser name types
typedef enum { 
    SMT_PRS_NONE = 0, 
    SMT_PRS_SET_OPTION,    
    SMT_PRS_SET_LOGIC,    
    SMT_PRS_SET_INFO,    
    SMT_PRS_DEFINE_FUN,
    SMT_PRS_DECLARE_FUN,   
    SMT_PRS_ASSERT,   
    SMT_PRS_LET,   
    SMT_PRS_CHECK_SAT,   
    SMT_PRS_GET_VALUE,   
    SMT_PRS_EXIT,
    SMT_PRS_END
} Smt_LineType_t; 

typedef struct Smt_Pair_t_ Smt_Pair_t;
struct Smt_Pair_t_
{
    Smt_LineType_t Type;
    char *         pName;
};
static Smt_Pair_t s_Types[SMT_PRS_END] =
{
    { SMT_PRS_NONE,         NULL         },
    { SMT_PRS_SET_OPTION,  "set-option"  },
    { SMT_PRS_SET_LOGIC,   "set-logic"   },
    { SMT_PRS_SET_INFO,    "set-info"    },
    { SMT_PRS_DEFINE_FUN,  "define-fun"  },
    { SMT_PRS_DECLARE_FUN, "declare-fun" },
    { SMT_PRS_ASSERT,      "assert"      },
    { SMT_PRS_LET,         "let"         },
    { SMT_PRS_CHECK_SAT,   "check-sat"   },
    { SMT_PRS_GET_VALUE,   "get-value"   },
    { SMT_PRS_EXIT,        "exit"        }
};
static inline char * Smt_GetTypeName( Smt_LineType_t Type )
{
    int i;
    for ( i = 1; i < SMT_PRS_END; i++ )
        if ( s_Types[i].Type == Type )
            return s_Types[i].pName;
    return NULL;
}
static inline void Smt_AddTypes( Abc_Nam_t * p )
{
    int Type;
    for ( Type = 1; Type < SMT_PRS_END; Type++ )
        Abc_NamStrFindOrAdd( p, Smt_GetTypeName((Smt_LineType_t)Type), NULL );
    assert( Abc_NamObjNumMax(p) == SMT_PRS_END );
}

static inline int         Smt_EntryIsName( int Fan )                          { return Abc_LitIsCompl(Fan);                                                         }
static inline int         Smt_EntryIsType( int Fan, Smt_LineType_t Type )     { assert(Smt_EntryIsName(Fan));  return Abc_Lit2Var(Fan) == Type;                     }
static inline char *      Smt_EntryName( Smt_Prs_t * p, int Fan )             { assert(Smt_EntryIsName(Fan));  return Abc_NamStr( p->pStrs, Abc_Lit2Var(Fan) );     }
static inline Vec_Int_t * Smt_EntryNode( Smt_Prs_t * p, int Fan )             { assert(!Smt_EntryIsName(Fan)); return Vec_WecEntry( &p->vObjs, Abc_Lit2Var(Fan) );  }

static inline int         Smt_VecEntryIsType( Vec_Int_t * vFans, int i, Smt_LineType_t Type ) { return i < Vec_IntSize(vFans) && Smt_EntryIsName(Vec_IntEntry(vFans, i)) && Smt_EntryIsType(Vec_IntEntry(vFans, i), Type); }
static inline char *      Smt_VecEntryName( Smt_Prs_t * p, Vec_Int_t * vFans, int i )         { return Smt_EntryIsName(Vec_IntEntry(vFans, i)) ? Smt_EntryName(p, Vec_IntEntry(vFans, i)) : NULL;                          }
static inline Vec_Int_t * Smt_VecEntryNode( Smt_Prs_t * p, Vec_Int_t * vFans, int i )         { return Smt_EntryIsName(Vec_IntEntry(vFans, i)) ? NULL : Smt_EntryNode(p, Vec_IntEntry(vFans, i));                          }

#define Smt_ManForEachDir( p, Type, vVec, i )                                                       \
    for ( i = 0; (i < Vec_WecSize(&p->vObjs)) && (((vVec) = Vec_WecEntry(&p->vObjs, i)), 1); i++ )  \
        if ( !Smt_VecEntryIsType(vVec, 0, Type) ) {} else

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Smt_StrToType( char * pName, int * pfSigned )
{
    int Type = 0; *pfSigned = 0;
    if ( !strcmp(pName, "ite") )
        Type = WLC_OBJ_MUX;           // 08: multiplexer
    else if ( !strcmp(pName, "bvlshr") )
        Type = WLC_OBJ_SHIFT_R;       // 09: shift right
    else if ( !strcmp(pName, "bvashr") )
        Type = WLC_OBJ_SHIFT_RA , *pfSigned = 1;      // 10: shift right (arithmetic)
    else if ( !strcmp(pName, "bvshl") )
        Type = WLC_OBJ_SHIFT_L;       // 11: shift left
//    else if ( !strcmp(pName, "") )
//        Type = WLC_OBJ_SHIFT_LA;    // 12: shift left (arithmetic)
    else if ( !strcmp(pName, "rotate_right") )
        Type = WLC_OBJ_ROTATE_R;      // 13: rotate right
    else if ( !strcmp(pName, "rotate_left") )
        Type = WLC_OBJ_ROTATE_L;      // 14: rotate left
    else if ( !strcmp(pName, "bvnot") )
        Type = WLC_OBJ_BIT_NOT;       // 15: bitwise NOT
    else if ( !strcmp(pName, "bvand") )
        Type = WLC_OBJ_BIT_AND;       // 16: bitwise AND
    else if ( !strcmp(pName, "bvor") )
        Type = WLC_OBJ_BIT_OR;        // 17: bitwise OR
    else if ( !strcmp(pName, "bvxor") )
        Type = WLC_OBJ_BIT_XOR;       // 18: bitwise XOR
    else if ( !strcmp(pName, "bvnand") )
        Type = WLC_OBJ_BIT_NAND;      // 16: bitwise NAND
    else if ( !strcmp(pName, "bvnor") )
        Type = WLC_OBJ_BIT_NOR;       // 17: bitwise NOR
    else if ( !strcmp(pName, "bvxnor") )
        Type = WLC_OBJ_BIT_NXOR;      // 18: bitwise NXOR
    else if ( !strcmp(pName, "extract") )
        Type = WLC_OBJ_BIT_SELECT;    // 19: bit selection
    else if ( !strcmp(pName, "concat") )
        Type = WLC_OBJ_BIT_CONCAT;    // 20: bit concatenation
    else if ( !strcmp(pName, "zero_extend") )
        Type = WLC_OBJ_BIT_ZEROPAD;   // 21: zero padding
    else if ( !strcmp(pName, "sign_extend") )
        Type = WLC_OBJ_BIT_SIGNEXT;   // 22: sign extension
    else if ( !strcmp(pName, "not") )
        Type = WLC_OBJ_LOGIC_NOT;     // 23: logic NOT
    else if ( !strcmp(pName, "=>") )
        Type = WLC_OBJ_LOGIC_IMPL;    // 24: logic AND
    else if ( !strcmp(pName, "and") )
        Type = WLC_OBJ_LOGIC_AND;     // 24: logic AND
    else if ( !strcmp(pName, "or") )
        Type = WLC_OBJ_LOGIC_OR;      // 25: logic OR
    else if ( !strcmp(pName, "xor") )
        Type = WLC_OBJ_LOGIC_XOR;     // 26: logic OR
    else if ( !strcmp(pName, "bvcomp") || !strcmp(pName, "=") )
        Type = WLC_OBJ_COMP_EQU;      // 27: compare equal
    else if ( !strcmp(pName, "distinct") )
        Type = WLC_OBJ_COMP_NOTEQU;   // 28: compare not equal
    else if ( !strcmp(pName, "bvult") )
        Type = WLC_OBJ_COMP_LESS;     // 29: compare less
    else if ( !strcmp(pName, "bvugt") )
        Type = WLC_OBJ_COMP_MORE;     // 30: compare more
    else if ( !strcmp(pName, "bvule") )
        Type = WLC_OBJ_COMP_LESSEQU;  // 31: compare less or equal
    else if ( !strcmp(pName, "bvuge") )
        Type = WLC_OBJ_COMP_MOREEQU;  // 32: compare more or equal
    else if ( !strcmp(pName, "bvslt") )
        Type = WLC_OBJ_COMP_LESS, *pfSigned = 1;     // 29: compare less
    else if ( !strcmp(pName, "bvsgt") )
        Type = WLC_OBJ_COMP_MORE, *pfSigned = 1;     // 30: compare more
    else if ( !strcmp(pName, "bvsle") )
        Type = WLC_OBJ_COMP_LESSEQU, *pfSigned = 1;  // 31: compare less or equal
    else if ( !strcmp(pName, "bvsge") )
        Type = WLC_OBJ_COMP_MOREEQU, *pfSigned = 1;  // 32: compare more or equal
    else if ( !strcmp(pName, "bvredand") )
        Type = WLC_OBJ_REDUCT_AND;    // 33: reduction AND
    else if ( !strcmp(pName, "bvredor") )
        Type = WLC_OBJ_REDUCT_OR;     // 34: reduction OR
    else if ( !strcmp(pName, "bvredxor") )
        Type = WLC_OBJ_REDUCT_XOR;    // 35: reduction XOR
    else if ( !strcmp(pName, "bvadd") )
        Type = WLC_OBJ_ARI_ADD;       // 36: arithmetic addition
    else if ( !strcmp(pName, "bvsub") )
        Type = WLC_OBJ_ARI_SUB;       // 37: arithmetic subtraction
    else if ( !strcmp(pName, "bvmul") )
        Type = WLC_OBJ_ARI_MULTI;     // 38: arithmetic multiplier
    else if ( !strcmp(pName, "bvudiv") )
        Type = WLC_OBJ_ARI_DIVIDE;    // 39: arithmetic division
    else if ( !strcmp(pName, "bvurem") )
        Type = WLC_OBJ_ARI_REM;       // 40: arithmetic remainder
    else if ( !strcmp(pName, "bvsdiv") )
        Type = WLC_OBJ_ARI_DIVIDE, *pfSigned = 1;    // 39: arithmetic division
    else if ( !strcmp(pName, "bvsrem") )
        Type = WLC_OBJ_ARI_REM, *pfSigned = 1;       // 40: arithmetic remainder
    else if ( !strcmp(pName, "bvsmod") )
        Type = WLC_OBJ_ARI_MODULUS, *pfSigned = 1;   // 40: arithmetic modulus
    else if ( !strcmp(pName, "=") )
        Type = WLC_OBJ_COMP_EQU;   // 40: arithmetic modulus
//    else if ( !strcmp(pName, "") )
//        Type = WLC_OBJ_ARI_POWER;     // 41: arithmetic power
    else if ( !strcmp(pName, "bvneg") )
        Type = WLC_OBJ_ARI_MINUS;       // 42: arithmetic minus
//    else if ( !strcmp(pName, "") )
//        Type = WLC_OBJ_TABLE;         // 43: bit table
    else
    {
        printf( "The following operations is currently not supported (%s)\n", pName );
        fflush( stdout );
    }
    return Type;
}
static inline int Smt_PrsReadType( Smt_Prs_t * p, int iSig, int * pfSigned, int * Value1, int * Value2 )
{
    if ( Smt_EntryIsName(iSig) )
        return Smt_StrToType( Smt_EntryName(p, iSig), pfSigned );
    else
    {
        Vec_Int_t * vFans = Smt_EntryNode( p, iSig );
        char * pStr = Smt_VecEntryName( p, vFans, 0 );  int Type;
        assert( Vec_IntSize(vFans) >= 3 );
        assert( !strcmp(pStr, "_") ); // special op
        *Value1 = *Value2 = -1;
        assert( pStr[0] != 'b' || pStr[1] != 'v' );
        // read type
        Type = Smt_StrToType( Smt_VecEntryName(p, vFans, 1), pfSigned );
        if ( Type == 0 )
            return 0;
        *Value1 = atoi( Smt_VecEntryName(p, vFans, 2) );
        if ( Vec_IntSize(vFans) > 3 )
            *Value2 = atoi( Smt_VecEntryName(p, vFans, 3) );
        return Type;
    }
}

static inline int Smt_StrType( char * str ) { return Smt_StrToType(str, NULL); }

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Smt_PrsCreateNodeOld( Wlc_Ntk_t * pNtk, int Type, int fSigned, int Range, Vec_Int_t * vFanins, char * pName )
{
    char Buffer[100];
    int NameId, fFound;
    int iObj = Wlc_ObjAlloc( pNtk, Type, fSigned, Range-1, 0 );
    assert( Type > 0 );
    assert( Range >= 0 );
    assert( fSigned >= 0 );
    
    // add node's name
    if ( pName == NULL )
    {
        sprintf( Buffer, "_n%d_", iObj );
        pName = Buffer;
    }
    NameId = Abc_NamStrFindOrAdd( pNtk->pManName, pName, &fFound );
    assert( !fFound );
    assert( iObj == NameId );
    
    Wlc_ObjAddFanins( pNtk, Wlc_NtkObj(pNtk, iObj), vFanins );
    if ( fSigned )
    {
        Wlc_NtkObj(pNtk, iObj)->Signed = fSigned;
//        if ( Vec_IntSize(vFanins) > 0 )
//            Wlc_NtkObj(pNtk, Vec_IntEntry(vFanins, 0))->Signed = fSigned;
//        if ( Vec_IntSize(vFanins) > 1 )
//            Wlc_NtkObj(pNtk, Vec_IntEntry(vFanins, 1))->Signed = fSigned;
    }

    return iObj;
}
static inline int Smt_PrsCreateNode( Wlc_Ntk_t * pNtk, int Type, int fSigned, int Range, Vec_Int_t * vFanins, char * pName )
{
    char Buffer[100];
    char * pNameFanin;
    int NameId, fFound, old_size, new_size;
    int iObj, iFanin0, iFanin1; 
    Vec_Int_t * v2Fanins = Vec_IntStartFull(2);
    
    assert( Type > 0 );
    assert( Range >= 0 );
    assert( fSigned >= 0 );
    
    //if (Vec_IntSize(vFanins)<=2 || Type == WLC_OBJ_BIT_CONCAT || Type == WLC_OBJ_MUX )
    // explicitely secify allowed multi operators
    if (Vec_IntSize(vFanins)<=2 || 
        !( Type == WLC_OBJ_BIT_AND ||       // 16:`` bitwise AND
          Type == WLC_OBJ_BIT_OR ||        // 17: bitwise OR
          Type == WLC_OBJ_BIT_XOR ||       // 18: bitwise XOR
          Type == WLC_OBJ_BIT_NAND ||      // 19: bitwise AND
          Type == WLC_OBJ_BIT_NOR ||       // 20: bitwise OR
          Type == WLC_OBJ_BIT_NXOR ||      // 21: bitwise NXOR

          Type == WLC_OBJ_LOGIC_IMPL ||    // 27: logic implication
          Type == WLC_OBJ_LOGIC_AND ||     // 28: logic AND
          Type == WLC_OBJ_LOGIC_OR ||      // 29: logic OR
          Type == WLC_OBJ_LOGIC_XOR ||     // 30: logic XOR
          Type == WLC_OBJ_COMP_EQU ||      // 31: compare equal
//          Type == WLC_OBJ_COMP_NOTEQU ||   // 32: compare not equal -- bug fix
          Type == WLC_OBJ_COMP_LESS ||     // 33: compare less
          Type == WLC_OBJ_COMP_MORE ||     // 34: compare more
          Type == WLC_OBJ_COMP_LESSEQU ||  // 35: compare less or equal
          Type == WLC_OBJ_COMP_MOREEQU || // 36: compare more or equal

          Type == WLC_OBJ_ARI_ADD ||       // 43: arithmetic addition
          Type == WLC_OBJ_ARI_SUB ||       // 44: arithmetic subtraction
          Type == WLC_OBJ_ARI_MULTI ||     // 45: arithmetic multiplier
          Type == WLC_OBJ_ARI_DIVIDE       // 46: arithmetic division 
        ))
        goto FINISHED_WITH_FANINS;

    // we will be creating nodes backwards. this way we can pop from vFanins easier
    while (Vec_IntSize(vFanins)>2)
    {
        // getting 2 fanins at a time
        old_size = Vec_IntSize(vFanins);
        iFanin0 = Vec_IntPop(vFanins);
        iFanin1 = Vec_IntPop(vFanins);
        
        Vec_IntWriteEntry(v2Fanins,0,iFanin0);
        Vec_IntWriteEntry(v2Fanins,1,iFanin1);
        
        iObj = Wlc_ObjAlloc( pNtk, Type, fSigned, Range-1, 0 );
        sprintf( Buffer, "_n%d_", iObj );
        pNameFanin = Buffer;
        NameId = Abc_NamStrFindOrAdd( pNtk->pManName, pNameFanin, &fFound );
        
        assert( !fFound );
        assert( iObj == NameId );
        
        Wlc_ObjAddFanins( pNtk, Wlc_NtkObj(pNtk, iObj), v2Fanins );
        
        // pushing the new node
        Vec_IntPush(vFanins, iObj);
        new_size = Vec_IntSize(vFanins);
        assert(new_size<old_size);
    }
    
FINISHED_WITH_FANINS:

    Vec_IntFree(v2Fanins);
    
    //added to deal with long shifts create extra bit select (ROTATE as well ??)
    // this is a temporary hack
    // basically we keep only 32 bits. 
    // bit[31] will be the copy of original MSB (sign bit, just in case) UPDATE: assume it is unsigned first????
    // bit[31] will be the reduction or of any bits from [31] to Range
    if (Type == WLC_OBJ_SHIFT_R || Type == WLC_OBJ_SHIFT_RA || Type == WLC_OBJ_SHIFT_L)
    {
        int range1, iObj1, iObj2, iObj3;
        assert(Vec_IntSize(vFanins)>=2);
        iFanin1 = Vec_IntEntry(vFanins,1);
        range1 = Wlc_ObjRange( Wlc_NtkObj(pNtk, iFanin1) );
        if (range1>32)
        {
            Vec_Int_t * newFanins = Vec_IntAlloc(10);
            Vec_IntPush(newFanins,iFanin1);
            Vec_IntPushTwo( newFanins, 30, 0 );
            
            iObj1 = Wlc_ObjAlloc( pNtk, WLC_OBJ_BIT_SELECT, 0, 30, 0 );
            sprintf( Buffer, "_n%d_", iObj1 );
            pNameFanin = Buffer;
            NameId = Abc_NamStrFindOrAdd( pNtk->pManName, pNameFanin, &fFound );
        
            assert( !fFound );
            assert( iObj1 == NameId );
        
            Wlc_ObjAddFanins( pNtk, Wlc_NtkObj(pNtk, iObj1), newFanins );
            
            //printf("obj1: %d\n",iObj1);
            
            // bit select of larger bits
            Vec_IntPop(newFanins);
            Vec_IntPop(newFanins);
            Vec_IntPushTwo( newFanins, range1-1, 31 );
            iObj2 = Wlc_ObjAlloc( pNtk, WLC_OBJ_BIT_SELECT, 0, range1-1, 31 );
            sprintf( Buffer, "_n%d_", iObj2 );
            pNameFanin = Buffer;
            NameId = Abc_NamStrFindOrAdd( pNtk->pManName, pNameFanin, &fFound );
        
            assert( !fFound );
            assert( iObj2 == NameId );
        
            Wlc_ObjAddFanins( pNtk, Wlc_NtkObj(pNtk, iObj2), newFanins );
            //printf("obj2: %d\n",iObj2);
            
            // reduction or
            Vec_IntPop( newFanins );
            Vec_IntPop( newFanins );
            Vec_IntWriteEntry( newFanins, 0, iObj2 );
            iObj3 = Wlc_ObjAlloc( pNtk, WLC_OBJ_REDUCT_OR, 0, 0, 0 );
            sprintf( Buffer, "_n%d_", iObj3 );
            pNameFanin = Buffer;
            NameId = Abc_NamStrFindOrAdd( pNtk->pManName, pNameFanin, &fFound );
        
            assert( !fFound );
            assert( iObj3 == NameId );
        
            Wlc_ObjAddFanins( pNtk, Wlc_NtkObj(pNtk, iObj3), newFanins );
            //printf("obj3: %d\n",iObj3);
        
            // concat all together
            Vec_IntWriteEntry( newFanins, 0, iObj3 );
            Vec_IntPush( newFanins, iObj1 );
            iObj = Wlc_ObjAlloc( pNtk, WLC_OBJ_BIT_CONCAT, 0, 31, 0 );
            sprintf( Buffer, "_n%d_", iObj );
            pNameFanin = Buffer;
            NameId = Abc_NamStrFindOrAdd( pNtk->pManName, pNameFanin, &fFound );
        
            assert( !fFound );
            assert( iObj == NameId );
        
            Wlc_ObjAddFanins( pNtk, Wlc_NtkObj(pNtk, iObj), newFanins );
            //printf("obj: %d\n",iObj);
        
            // pushing the new node
            Vec_IntWriteEntry(vFanins, 1, iObj);
            Vec_IntFree(newFanins);
        }
    }
    
    iObj = Wlc_ObjAlloc( pNtk, Type, fSigned, Range-1, 0 );
    
    // add node's name
    if ( pName == NULL )
    {
        sprintf( Buffer, "_n%d_", iObj );
        pName = Buffer;
    }
    NameId = Abc_NamStrFindOrAdd( pNtk->pManName, pName, &fFound );
    assert( !fFound );
    assert( iObj == NameId );
    
    Wlc_ObjAddFanins( pNtk, Wlc_NtkObj(pNtk, iObj), vFanins );
    if ( fSigned )
    {
        Wlc_NtkObj(pNtk, iObj)->Signed = fSigned;
//        if ( Vec_IntSize(vFanins) > 0 )
//            Wlc_NtkObj(pNtk, Vec_IntEntry(vFanins, 0))->Signed = fSigned;
//        if ( Vec_IntSize(vFanins) > 1 )
//            Wlc_NtkObj(pNtk, Vec_IntEntry(vFanins, 1))->Signed = fSigned;
    }
        
    return iObj;
}

static inline char * Smt_GetHexFromDecimalString(char * pStr)
{
    int i,k=0, nDigits = strlen(pStr);
    int digit, carry = 0;
    int metNonZeroBit = 0;
    
    Vec_Int_t * decimal = Vec_IntAlloc(nDigits);
    Vec_Int_t * rev;
    int nBits;
    char * hex;
    
    for (i=0;i<nDigits;i++)
        Vec_IntPush(decimal,pStr[i]-'0');

    // firstly fillin the reversed vector
    rev = Vec_IntAlloc(10);
    while(k<nDigits)
    {
        digit = Vec_IntEntry(decimal,k);
        if (!digit && !carry)
        {
            k++;
            if (k>=nDigits)
            {
                if (!metNonZeroBit)
                    break;
                else
                {
                    Vec_IntPush(rev,carry);
                    carry = 0;
                    k = 0;
                    metNonZeroBit = 0;
                }
            }
            continue;
        }
        
        if (!metNonZeroBit)
            metNonZeroBit = 1;

        digit = carry*10 + digit;
        carry = digit%2;
        digit = digit / 2;
        Vec_IntWriteEntry(decimal,k,digit);

        k++;
        if (k>=nDigits)
        {
            Vec_IntPush(rev,carry);   
            carry = 0; 
            k = 0;
            metNonZeroBit = 0;
        }
    }
    
    Vec_IntFree(decimal);
    
    if (!Vec_IntSize(rev))
        Vec_IntPush(rev,0);

    while (Vec_IntSize(rev)%4)
        Vec_IntPush(rev,0);
    
    nBits = Vec_IntSize(rev);
    hex = (char *)malloc((nBits/4+1)*sizeof(char));
 
    for (k=0;k<nBits/4;k++)
    {   
        int number = Vec_IntEntry(rev,k*4) + 2*Vec_IntEntry(rev,k*4+1) + 4*Vec_IntEntry(rev,k*4+2) + 8*Vec_IntEntry(rev,k*4+3);
        char letter;

        switch(number) {
            case 0: letter = '0'; break;
            case 1: letter = '1'; break;
            case 2: letter = '2'; break;
            case 3: letter = '3'; break;
            case 4: letter = '4'; break;
            case 5: letter = '5'; break;
            case 6: letter = '6'; break;
            case 7: letter = '7'; break;
            case 8: letter = '8'; break;
            case 9: letter = '9'; break;
            case 10: letter = 'a'; break;
            case 11: letter = 'b'; break;
            case 12: letter = 'c'; break;
            case 13: letter = 'd'; break;
            case 14: letter = 'e'; break;
            case 15: letter = 'f'; break;
            default: assert(0);
        }
        hex[nBits/4-1-k] = letter;
        //if (k<Vec_IntSize(rev))
        //    Vec_IntPush(vFanins,Vec_IntEntry(rev,k));
        //else
        //    Vec_IntPush(vFanins,0);
    }
    hex[nBits/4] = '\0';
    Vec_IntFree(rev);

    return hex;
}           
    
static inline int Smt_PrsBuildConstant( Wlc_Ntk_t * pNtk, char * pStr, int nBits, char * pName )
{
    int i, nDigits, iObj;
    Vec_Int_t * vFanins = Vec_IntAlloc( 10 );
    if ( pStr[0] != '#' ) // decimal
    {
        if ( pStr[0] >= '0' && pStr[0] <= '9' )
        {
            // added: sanity check for large constants
            /*
            Vec_Int_t * temp = Vec_IntAlloc(10);
            int fullBits = -1;
            Smt_GetBinaryFromDecimalString(pStr,temp,&fullBits);
            Vec_IntFree(temp);
            assert(fullBits < 32);*/

            char * pHex = Smt_GetHexFromDecimalString(pStr);
            
            if ( nBits == -1 )
                nBits = strlen(pHex)*4;
            
            //printf("nbits: %d\n",nBits);
            
            Vec_IntFill( vFanins, Abc_BitWordNum(nBits), 0 );
            nDigits = Abc_TtReadHexNumber( (word *)Vec_IntArray(vFanins), pHex );
            ABC_FREE( pHex );
            
            /*
            int w, nWords, Number = atoi( pStr );
            if ( nBits == -1 )
            {
                nBits = Abc_Base2Log( Number+1 );
                assert( nBits < 32 );
            }
            nWords = Abc_BitWordNum( nBits );
            for ( w = 0; w < nWords; w++ )
                Vec_IntPush( vFanins, w ? 0 : Number );
            */
        }
        else
        {
            int fFound, iObj = Abc_NamStrFindOrAdd( pNtk->pManName, pStr, &fFound );
            assert( fFound );
            Vec_IntFree( vFanins );
            return iObj;
        }
    }
    else if ( pStr[1] == 'b' ) // binary
    {
        if ( nBits == -1 )
            nBits = strlen(pStr+2);
        Vec_IntFill( vFanins, Abc_BitWordNum(nBits), 0 );
        for ( i = 0; i < nBits; i++ )
            if ( pStr[2+i] == '1' )
                Abc_InfoSetBit( (unsigned *)Vec_IntArray(vFanins), nBits-1-i );
            else if ( pStr[2+i] != '0' )
            {
                Vec_IntFree( vFanins );
                return 0;
            }
    }
    else if ( pStr[1] == 'x' ) // hexadecimal
    {
        if ( nBits == -1 )
            nBits = strlen(pStr+2)*4;
        Vec_IntFill( vFanins, Abc_BitWordNum(nBits), 0 );
        nDigits = Abc_TtReadHexNumber( (word *)Vec_IntArray(vFanins), pStr+2 );
        if ( nDigits != (nBits + 3)/4 )
        {
            Vec_IntFree( vFanins );
            return 0;
        }
    }
    else 
    {
        Vec_IntFree( vFanins );
        return 0;
    }
    // create constant node
    iObj = Smt_PrsCreateNode( pNtk, WLC_OBJ_CONST, 0, nBits, vFanins, pName );
    Vec_IntFree( vFanins );
    return iObj;
}
int Smt_PrsBuildNode( Wlc_Ntk_t * pNtk, Smt_Prs_t * p, int iNode, int RangeOut, char * pName )
{
    if ( Smt_EntryIsName(iNode) ) // name or constant
    {
        char * pStr = Abc_NamStr(p->pStrs, Abc_Lit2Var(iNode));
        if ( (pStr[0] >= '0' && pStr[0] <= '9') || pStr[0] == '#' )
        { 
            // (_ BitVec 8) #x19
            return Smt_PrsBuildConstant( pNtk, pStr, RangeOut, pName );
        }
        else
        {
            // s3087
            int fFound, iObj = Abc_NamStrFindOrAdd( pNtk->pManName, pStr, &fFound );
            assert( fFound );
            return iObj;
        }
    }
    else // node
    {
        Vec_Int_t * vFans = Smt_EntryNode( p, iNode );
        char * pStr0 = Smt_VecEntryName( p, vFans, 0 );
        char * pStr1 = Smt_VecEntryName( p, vFans, 1 );
        if ( pStr0 && pStr1 && pStr0[0] == '_' && pStr1[0] == 'b' && pStr1[1] == 'v' )
        {
            // (_ bv1 32)
            char * pStr2 = Smt_VecEntryName( p, vFans, 2 );
            assert( Vec_IntSize(vFans) == 3 );
            return Smt_PrsBuildConstant( pNtk, pStr1+2, atoi(pStr2), pName );
        }
        else if ( pStr0 && pStr0[0] == '=' )
        {
            assert( Vec_IntSize(vFans) == 3 );
            iNode = Vec_IntEntry(vFans, 2);
            assert( Smt_EntryIsName(iNode) );
            pStr0 = Smt_EntryName(p, iNode);
            // check the last one is "#b1"
            if ( !strcmp("#b1", pStr0) )
            {
                iNode = Vec_IntEntry(vFans, 1);
                return Smt_PrsBuildNode( pNtk, p, iNode, -1, pName );
            }
            else
            {
                Vec_Int_t * vFanins = Vec_IntAlloc( 2 );
                // get the constant
                int iObj, iOper, iConst = Smt_PrsBuildConstant( pNtk, pStr0, -1, NULL );
                // check the middle one is an operator
                iNode = Vec_IntEntry(vFans, 1);
                iOper = Smt_PrsBuildNode( pNtk, p, iNode, -1, pName );
                // build comparator
                Vec_IntPushTwo( vFanins, iOper, iConst );
                iObj = Smt_PrsCreateNode( pNtk, WLC_OBJ_COMP_EQU, 0, 1, vFanins, pName );
                Vec_IntFree( vFanins );
                return iObj;
            }
        }
        else
        {
            int i, Fan, NameId, iFanin, fSigned, Range, Value1 = -1, Value2 = -1;
            int Type = Smt_PrsReadType( p, Vec_IntEntry(vFans, 0), &fSigned, &Value1, &Value2 );
            // collect fanins
            Vec_Int_t * vFanins = Vec_IntAlloc( 100 );
            Vec_IntForEachEntryStart( vFans, Fan, i, 1 )
            {
                iFanin = Smt_PrsBuildNode( pNtk, p, Fan, -1, NULL );
                if ( iFanin == 0 )
                {
                    Vec_IntFree( vFanins );
                    return 0;
                }
                Vec_IntPush( vFanins, iFanin );
            }
            // update specialized nodes
            assert( Type != WLC_OBJ_BIT_SIGNEXT && Type != WLC_OBJ_BIT_ZEROPAD );
            if ( Type == WLC_OBJ_BIT_SELECT )
            {
                assert( Value1 >= 0 && Value2 >= 0 && Value1 >= Value2 );
                Vec_IntPushTwo( vFanins, Value1, Value2 );
            }
            else if ( Type == WLC_OBJ_ROTATE_R || Type == WLC_OBJ_ROTATE_L )
            {
                char Buffer[10];
                assert( Value1 >= 0 );
                sprintf( Buffer, "%d", Value1 ); 
                NameId = Smt_PrsBuildConstant( pNtk, Buffer, -1, NULL );
                Vec_IntPush( vFanins, NameId );
            }
            // find range
            Range = 0;
            if ( Type >= WLC_OBJ_LOGIC_NOT && Type <= WLC_OBJ_REDUCT_XOR )
                Range = 1;
            else if ( Type == WLC_OBJ_BIT_SELECT )
                Range = Value1 - Value2 + 1;
            else if ( Type == WLC_OBJ_BIT_CONCAT )
            {
                Vec_IntForEachEntry( vFanins, NameId, i )
                    Range += Wlc_ObjRange( Wlc_NtkObj(pNtk, NameId) );
            }
            else if ( Type == WLC_OBJ_MUX )
            {
                int * pArray = Vec_IntArray(vFanins);
                assert( Vec_IntSize(vFanins) == 3 );
                ABC_SWAP( int, pArray[1], pArray[2] );
                NameId = Vec_IntEntry(vFanins, 1);
                Range = Wlc_ObjRange( Wlc_NtkObj(pNtk, NameId) );
            }
            else // to determine range, look at the first argument
            {
                NameId = Vec_IntEntry(vFanins, 0);
                Range = Wlc_ObjRange( Wlc_NtkObj(pNtk, NameId) );
            }
            // create node
            assert( Range > 0 );
            NameId = Smt_PrsCreateNode( pNtk, Type, fSigned, Range, vFanins, pName );
            Vec_IntFree( vFanins );
            return NameId;
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wlc_Ntk_t * Smt_PrsBuild( Smt_Prs_t * p )
{
    Wlc_Ntk_t * pNtk;
    Vec_Int_t * vFans, * vFans2, * vFans3; 
    Vec_Int_t * vAsserts = Vec_IntAlloc(100);
    char * pName, * pRange, * pValue;
    int i, k, Fan, Fan3, iObj, Status, Range, NameId, nBits = 0;
    // issue warnings about unknown dirs
    vFans = Vec_WecEntry( &p->vObjs, 0 );
    Vec_IntForEachEntry( vFans, Fan, i )
    {
        assert( !Smt_EntryIsName(Fan) );
        vFans2 = Smt_VecEntryNode( p, vFans, i );
        Fan = Vec_IntEntry(vFans2, 0);
        assert( Smt_EntryIsName(Fan) );
        if ( Abc_Lit2Var(Fan) >= SMT_PRS_END )
            printf( "Ignoring directive \"%s\".\n", Smt_EntryName(p, Fan) );
    }
    // start network and create primary inputs
    pNtk = Wlc_NtkAlloc( p->pName, 1000 );
    pNtk->pManName = Abc_NamStart( 1000, 24 );
    pNtk->fSmtLib = 1;
    Smt_ManForEachDir( p, SMT_PRS_DECLARE_FUN, vFans, i )
    {
        assert( Vec_IntSize(vFans) == 4 );
        assert( Smt_VecEntryIsType(vFans, 0, SMT_PRS_DECLARE_FUN) );
        // get name
        Fan = Vec_IntEntry(vFans, 1);
        assert( Smt_EntryIsName(Fan) );
        pName = Smt_EntryName(p, Fan);
        // skip ()
        Fan = Vec_IntEntry(vFans, 2);
        assert( !Smt_EntryIsName(Fan) );
        // check type (Bool or BitVec)
        Fan = Vec_IntEntry(vFans, 3);
        if ( Smt_EntryIsName(Fan) ) 
        {
            //(declare-fun s1 () Bool)
            assert( !strcmp("Bool", Smt_VecEntryName(p, vFans, 3)) );
            Range = 1;
        }
        else
        {
            // (declare-fun s1 () (_ BitVec 64))
            // get range
            Fan = Vec_IntEntry(vFans, 3);
            assert( !Smt_EntryIsName(Fan) );
            vFans2 = Smt_EntryNode(p, Fan);
            assert( Vec_IntSize(vFans2) == 3 );
            assert( !strcmp("_", Smt_VecEntryName(p, vFans2, 0)) );
            assert( !strcmp("BitVec", Smt_VecEntryName(p, vFans2, 1)) );
            Fan = Vec_IntEntry(vFans2, 2);
            assert( Smt_EntryIsName(Fan) );
            pRange = Smt_EntryName(p, Fan);
            Range = atoi(pRange);
        }
        // create node
        iObj = Wlc_ObjAlloc( pNtk, WLC_OBJ_PI, 0, Range-1, 0 );
        NameId = Abc_NamStrFindOrAdd( pNtk->pManName, pName, NULL );
        assert( iObj == NameId );
        // save values
        Vec_IntPush( &pNtk->vValues, NameId );
        Vec_IntPush( &pNtk->vValues, nBits );
        Vec_IntPush( &pNtk->vValues, Range );
        nBits += Range;
    }
    // create constants
    Smt_ManForEachDir( p, SMT_PRS_DEFINE_FUN, vFans, i )
    {
        assert( Vec_IntSize(vFans) == 5 );
        assert( Smt_VecEntryIsType(vFans, 0, SMT_PRS_DEFINE_FUN) );
        // get name
        Fan = Vec_IntEntry(vFans, 1);
        assert( Smt_EntryIsName(Fan) );
        pName = Smt_EntryName(p, Fan);
        // skip ()
        Fan = Vec_IntEntry(vFans, 2);
        assert( !Smt_EntryIsName(Fan) );
        // check type (Bool or BitVec)
        Fan = Vec_IntEntry(vFans, 3);
        if ( Smt_EntryIsName(Fan) ) 
        {
            // (define-fun s_2 () Bool false)
            assert( !strcmp("Bool", Smt_VecEntryName(p, vFans, 3)) );
            Range = 1;
            pValue = Smt_VecEntryName(p, vFans, 4);
            if ( !strcmp("false", pValue) )
                pValue = "#b0";
            else if ( !strcmp("true", pValue) )
                pValue = "#b1";
            else assert( 0 );
            Status = Smt_PrsBuildConstant( pNtk, pValue, Range, pName );
        }
        else
        {
            // (define-fun s702 () (_ BitVec 4) #xe)
            // (define-fun s1 () (_ BitVec 8) (bvneg #x7f))
            // get range
            Fan = Vec_IntEntry(vFans, 3);
            assert( !Smt_EntryIsName(Fan) );
            vFans2 = Smt_VecEntryNode(p, vFans, 3);
            assert( Vec_IntSize(vFans2) == 3 );
            assert( !strcmp("_", Smt_VecEntryName(p, vFans2, 0)) );
            assert( !strcmp("BitVec", Smt_VecEntryName(p, vFans2, 1)) );
            // get range
            Fan = Vec_IntEntry(vFans2, 2);
            assert( Smt_EntryIsName(Fan) );
            pRange = Smt_EntryName(p, Fan);
            Range = atoi(pRange);
            // get constant
            Fan = Vec_IntEntry(vFans, 4);
            Status = Smt_PrsBuildNode( pNtk, p, Fan, Range, pName );
        }        
        if ( !Status )
        {
            Wlc_NtkFree( pNtk ); pNtk = NULL;
            goto finish;
        }
    }
    // process let-statements
    Smt_ManForEachDir( p, SMT_PRS_LET, vFans, i )
    {
        // let ((s35550 (bvor s48 s35549)))
        assert( Vec_IntSize(vFans) == 3 );
        assert( Smt_VecEntryIsType(vFans, 0, SMT_PRS_LET) );
        // get parts
        Fan = Vec_IntEntry(vFans, 1);
        assert( !Smt_EntryIsName(Fan) );
        vFans2 = Smt_EntryNode(p, Fan);
        if ( Smt_VecEntryIsType(vFans2, 0, SMT_PRS_LET) )
            continue;
        // iterate through the parts
        Vec_IntForEachEntry( vFans2, Fan, k )
        {
            // s35550 (bvor s48 s35549)
            Fan = Vec_IntEntry(vFans2, 0);
            assert( !Smt_EntryIsName(Fan) );
            vFans3 = Smt_EntryNode(p, Fan);
            // get the name
            Fan3 = Vec_IntEntry(vFans3, 0);
            assert( Smt_EntryIsName(Fan3) );
            pName = Smt_EntryName(p, Fan3);
            // get function
            Fan3 = Vec_IntEntry(vFans3, 1);
            assert( !Smt_EntryIsName(Fan3) );
            Status = Smt_PrsBuildNode( pNtk, p, Fan3, -1, pName );
            if ( !Status )
            {
                Wlc_NtkFree( pNtk ); pNtk = NULL;
                goto finish;
            }
        }
    }
    // process assert-statements
    Vec_IntClear( vAsserts );
    Smt_ManForEachDir( p, SMT_PRS_ASSERT, vFans, i )
    {
        //(assert ; no quantifiers
        //   (let ((s2 (bvsge s0 s1)))
        //   (not s2)))
        //(assert (not (= s0 #x00)))
        assert( Vec_IntSize(vFans) == 2 );
        assert( Smt_VecEntryIsType(vFans, 0, SMT_PRS_ASSERT) );
        // get second directive
        Fan = Vec_IntEntry(vFans, 1);
        if ( !Smt_EntryIsName(Fan) )
        {
            // find the final let-statement
            vFans2 = Smt_VecEntryNode(p, vFans, 1);
            while ( Smt_VecEntryIsType(vFans2, 0, SMT_PRS_LET) )
            {
                Fan = Vec_IntEntry(vFans2, 2);
                if ( Smt_EntryIsName(Fan) )
                    break;
                vFans2 = Smt_VecEntryNode(p, vFans2, 2);
            }
        }
        // find name or create node
        iObj = Smt_PrsBuildNode( pNtk, p, Fan, -1, NULL );
        if ( !iObj )
        {
            Wlc_NtkFree( pNtk ); pNtk = NULL;
            goto finish;
        }
        Vec_IntPush( vAsserts, iObj );
    }
    // build AND of asserts
    if ( Vec_IntSize(vAsserts) == 1 )
        iObj = Smt_PrsCreateNode( pNtk, WLC_OBJ_BUF, 0, 1, vAsserts, "miter_output" );
    else
    {
        iObj = Smt_PrsCreateNode( pNtk, WLC_OBJ_BIT_CONCAT, 0, Vec_IntSize(vAsserts), vAsserts, NULL );
        Vec_IntFill( vAsserts, 1, iObj );
        iObj = Smt_PrsCreateNode( pNtk, WLC_OBJ_REDUCT_AND, 0, 1, vAsserts, "miter_output" );
    }
    Wlc_ObjSetCo( pNtk, Wlc_NtkObj(pNtk, iObj), 0 );
    // create nameIDs
    vFans = Vec_IntStartNatural( Wlc_NtkObjNumMax(pNtk) );
    Vec_IntAppend( &pNtk->vNameIds, vFans );
    Vec_IntFree( vFans );
    //Wlc_NtkReport( pNtk, NULL );
finish:
    // cleanup
    Vec_IntFree(vAsserts);
    return pNtk;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Smt_PrsGenName( Smt_Prs_t * p )
{
    static char Buffer[16];
    sprintf( Buffer, "_%0*X_", p->nDigits, ++p->NameCount );
    return Buffer;
}
int Smt_PrsBuild2_rec( Wlc_Ntk_t * pNtk, Smt_Prs_t * p, int iNode, int iObjPrev, char * pName )
{
    char suffix[100];
    sprintf(suffix,"_as%d",pNtk->nAssert);

    //char * prepStr = Abc_NamStr(p->pStrs, Abc_Lit2Var(iNode));
    //printf("prestr: %s\n",prepStr);
    
    //printf("inode: %d %d\n",iNode,Smt_EntryIsName(iNode));
    
    if ( Smt_EntryIsName(iNode) )
    {
        char * pStr = Abc_NamStr(p->pStrs, Abc_Lit2Var(iNode));
        // handle built-in constants
        if ( !strcmp(pStr, "false") )
            pStr = "#b0";
        else if ( !strcmp(pStr, "true") )
            pStr = "#b1";
        if ( pStr[0] == '#' )
            return Smt_PrsBuildConstant( pNtk, pStr, -1, pName ? pName : Smt_PrsGenName(p) );
        else
        {
            int fFound, iObj;
            // look either for global DECLARE-FUN variable or local LET
            char * pStr_glb = (char *)malloc(strlen(pStr) + 4 +1); //glb
            char * pStr_loc = (char *)malloc(strlen(pStr) + strlen(suffix) +1);
            strcpy(pStr_glb,pStr);
            strcat(pStr_glb,SMT_GLO_SUFFIX);
            strcpy(pStr_loc,pStr);
            strcat(pStr_loc,suffix);
            
            fFound =  Abc_NamStrFind( pNtk->pManName, pStr_glb );
            
            if (fFound)
                pStr = pStr_glb;
            else
            {
                assert( Abc_NamStrFind( pNtk->pManName, pStr_loc ));
                pStr = pStr_loc;
            }
            // FIXME: delete memory of pStr 
            
            iObj = Abc_NamStrFindOrAdd( pNtk->pManName, pStr, &fFound );
            assert( fFound );
            // create buffer if the name of the fanin has different name
            if ( pName && strcmp(Wlc_ObjName(pNtk, iObj), pName) )
            {
                Vec_IntFill( &p->vTempFans, 1, iObj );
                iObj = Smt_PrsCreateNode( pNtk, WLC_OBJ_BUF, 0, Wlc_ObjRange(Wlc_NtkObj(pNtk, iObj)), &p->vTempFans, pName );
            }
            ABC_FREE( pStr_glb );
            ABC_FREE( pStr_loc );
            return iObj;
        }
    }
    else
    {
        Vec_Int_t * vRoots, * vRoots1, * vFans3; 
        int iObj, iRoot0, iRoot1, iRoot2, Fan, Fan3, k;
        assert( !Smt_EntryIsName(iNode) );
        vRoots = Smt_EntryNode( p, iNode );
        iRoot0 = Vec_IntEntry( vRoots, 0 );
        if ( Smt_EntryIsName(iRoot0) )
        {
            char * pName2, * pStr0 = Abc_NamStr(p->pStrs, Abc_Lit2Var(iRoot0));
            if ( Abc_Lit2Var(iRoot0) == SMT_PRS_LET || Abc_Lit2Var(iRoot0) == SMT_PRS_DEFINE_FUN) //added define-fun is similar to let
            {
                // let ((s35550 (bvor s48 s35549)))
                assert( Vec_IntSize(vRoots) == 3 );
                assert( Smt_VecEntryIsType(vRoots, 0, SMT_PRS_LET) );
                // get parts
                iRoot1 = Vec_IntEntry(vRoots, 1);
                assert( !Smt_EntryIsName(iRoot1) );
                vRoots1 = Smt_EntryNode(p, iRoot1);
                // iterate through the parts
                Vec_IntForEachEntry( vRoots1, Fan, k )
                {
                    char * temp;
                    // s35550 (bvor s48 s35549)
                    assert( !Smt_EntryIsName(Fan) );
                    vFans3 = Smt_EntryNode(p, Fan);
                    assert( Vec_IntSize(vFans3) == 2 );
                    // get the name
                    Fan3 = Vec_IntEntry(vFans3, 0);
                    assert( Smt_EntryIsName(Fan3) );
                    pName2 = Smt_EntryName(p, Fan3);
                    // create a local name with suffix
                    if ( Abc_Lit2Var(iRoot0) == SMT_PRS_LET )
                    {   
                        temp = (char *)malloc(strlen(pName2) + strlen(suffix) + 1);
                        strcpy(temp, pName2);
                        strcat(temp,suffix);
                    }
                    else 
                    {   temp = (char *)malloc(strlen(pName2) + 4 + 1);
                        strcpy(temp, pName2);
                        strcat(temp,SMT_GLO_SUFFIX);
                    }
                    // FIXME: need to delete memory of pName2
                    pName2 = temp;
                    // get function
                    Fan3 = Vec_IntEntry(vFans3, 1);
                    //assert( !Smt_EntryIsName(Fan3) );
                    // solve the problem
                    iObj = Smt_PrsBuild2_rec( pNtk, p, Fan3, -1, pName2 ); // NULL ); //pName2 );
                    ABC_FREE( temp );
                    if ( iObj == 0 )
                        return 0;
                    // create buffer
                    //Vec_IntFill( &p->vTempFans, 1, iObj );
                    //Smt_PrsCreateNode( pNtk, WLC_OBJ_BUF, 0, Wlc_ObjRange(Wlc_NtkObj(pNtk, iObj)), &p->vTempFans, pName2 );
                }
                // process the final part of let
                iRoot2 = Vec_IntEntry(vRoots, 2);
                return Smt_PrsBuild2_rec( pNtk, p, iRoot2, -1, pName );
            }
            else if ( pStr0[0] == '_' )
            {
                char * pStr1 = Smt_VecEntryName( p, vRoots, 1 );
                if ( pStr1[0] == 'b' && pStr1[1] == 'v' )
                {
                    // (_ bv1 32)
                    char * pStr2 = Smt_VecEntryName( p, vRoots, 2 );
                    assert( Vec_IntSize(vRoots) == 3 );
                    return Smt_PrsBuildConstant( pNtk, pStr1+2, atoi(pStr2), pName ? pName : Smt_PrsGenName(p) );
                }
                else
                {
                    int Type1, fSigned = 0, Range = -1;
                    assert( iObjPrev != -1 );
                    Type1 = Smt_StrToType( pStr1, &fSigned );
                    if ( Type1 == 0 )
                        return 0;
                    // find out this branch
                    Vec_IntFill( &p->vTempFans, 1, iObjPrev );
                    if ( Type1 == WLC_OBJ_BIT_SIGNEXT || Type1 == WLC_OBJ_BIT_ZEROPAD || Type1 == WLC_OBJ_ROTATE_R || Type1 == WLC_OBJ_ROTATE_L )
                    {
                        int iRoot2 = Vec_IntEntry(vRoots, 2);
                        char * pStr2 = Abc_NamStr(p->pStrs, Abc_Lit2Var(iRoot2));
                        int Num = atoi( pStr2 );
                        //fSigned = (Type1 == WLC_OBJ_BIT_SIGNEXT);
                        if ( Type1 == WLC_OBJ_ROTATE_R || Type1 == WLC_OBJ_ROTATE_L )
                        {
                            int iConst = Smt_PrsBuildConstant( pNtk, pStr2, -1, Smt_PrsGenName(p) );
                            Vec_IntClear( &p->vTempFans );
                            Vec_IntPushTwo( &p->vTempFans, iObjPrev, iConst );
                            Range = Wlc_ObjRange( Wlc_NtkObj(pNtk, iObjPrev) );
                        }
                        else
                            Range = Num + Wlc_ObjRange( Wlc_NtkObj(pNtk, iObjPrev) );
                    }
                    else if ( Type1 == WLC_OBJ_BIT_SELECT )
                    {
                        int iRoot2 = Vec_IntEntry( vRoots, 2 );
                        int iRoot3 = Vec_IntEntry( vRoots, 3 );
                        char * pStr2 = Abc_NamStr(p->pStrs, Abc_Lit2Var(iRoot2));
                        char * pStr3 = Abc_NamStr(p->pStrs, Abc_Lit2Var(iRoot3));
                        int Num1 = atoi( pStr2 );
                        int Num2 = atoi( pStr3 );
                        assert( Num1 >= Num2 );
                        Range = Num1 - Num2 + 1;
                        Vec_IntPushTwo( &p->vTempFans, Num1, Num2 );
                    }
                    else assert( 0 );
                    iObj = Smt_PrsCreateNode( pNtk, Type1, fSigned, Range, &p->vTempFans, pName ? pName : Smt_PrsGenName(p) );
                    return iObj;
                }
            }
            else
            {
                Vec_Int_t * vFanins;
                int i, Fan, fSigned = 0, Range, Type0;
                int iObj = Abc_NamStrFind( pNtk->pManName, pStr0 );
                if ( iObj )
                    return iObj;
                Type0 = Smt_StrToType( pStr0, &fSigned );
                if ( Type0 == 0 )
                    return 0;
                assert( Type0 != WLC_OBJ_BIT_SIGNEXT && Type0 != WLC_OBJ_BIT_ZEROPAD && Type0 != WLC_OBJ_BIT_SELECT && Type0 != WLC_OBJ_ROTATE_R && Type0 != WLC_OBJ_ROTATE_L );

                // collect fanins
                vFanins = Vec_IntAlloc( 100 );
                Vec_IntForEachEntryStart( vRoots, Fan, i, 1 )
                {
                    iObj = Smt_PrsBuild2_rec( pNtk, p, Fan, -1, NULL );
                    if ( iObj == 0 )
                    {
                        Vec_IntFree( vFanins );
                        return 0;
                    }
                    Vec_IntPush( vFanins, iObj );
                }
                // find range
                Range = 0;
                if ( Type0 >= WLC_OBJ_LOGIC_NOT && Type0 <= WLC_OBJ_REDUCT_XOR )
                    Range = 1;
                else if ( Type0 == WLC_OBJ_BIT_CONCAT )
                {
                    Vec_IntForEachEntry( vFanins, Fan, i )
                        Range += Wlc_ObjRange( Wlc_NtkObj(pNtk, Fan) );
                }
                else if ( Type0 == WLC_OBJ_MUX )
                {
                    int * pArray = Vec_IntArray(vFanins);
                    assert( Vec_IntSize(vFanins) == 3 );
                    ABC_SWAP( int, pArray[1], pArray[2] );
                    iObj = Vec_IntEntry(vFanins, 1);
                    Range = Wlc_ObjRange( Wlc_NtkObj(pNtk, iObj) );
                }
                else // to determine range, look at the first argument
                {
                    iObj = Vec_IntEntry(vFanins, 0);
                    Range = Wlc_ObjRange( Wlc_NtkObj(pNtk, iObj) );
                }
                // create node
                assert( Range > 0 );
                iObj = Smt_PrsCreateNode( pNtk, Type0, fSigned, Range, vFanins, pName ? pName : Smt_PrsGenName(p) );
                Vec_IntFree( vFanins );
                return iObj;
            }
        }
        else if ( Vec_IntSize(vRoots) == 2 ) // could be   ((_ extract 48 16) (bvmul ?v_5 ?v_12))
        {
            int iObjPrev = Smt_PrsBuild2_rec( pNtk, p, Vec_IntEntry(vRoots, 1), -1, NULL );
            if ( iObjPrev == 0 )
                return 0;
            return Smt_PrsBuild2_rec( pNtk, p, Vec_IntEntry(vRoots, 0), iObjPrev, pName );
        }
        else assert( 0 ); // return Smt_PrsBuild2_rec( pNtk, p, iRoot0 );
        return 0;
    }
}
Wlc_Ntk_t * Smt_PrsBuild2( Smt_Prs_t * p )
{
    Wlc_Ntk_t * pNtk;
    Vec_Int_t * vFansRoot, * vFans, * vFans2; 
    Vec_Int_t * vAsserts = Vec_IntAlloc(100);
    int i, Root, Fan, iObj, NameId, Range, nBits = 0;
    char * pName, * pRange;
    // start network and create primary inputs
    pNtk = Wlc_NtkAlloc( p->pName, 1000 );
    pNtk->pManName = Abc_NamStart( 1000, 24 );
    pNtk->fSmtLib = 1;
    // collect top-level asserts
    vFansRoot = Vec_WecEntry( &p->vObjs, 0 );
    Vec_IntForEachEntry( vFansRoot, Root, i )
    {
        assert( !Smt_EntryIsName(Root) );
        vFans = Smt_VecEntryNode( p, vFansRoot, i );
        Fan = Vec_IntEntry(vFans, 0);
        assert( Smt_EntryIsName(Fan) );
        // create variables
        if ( Abc_Lit2Var(Fan) == SMT_PRS_DECLARE_FUN )
        {
            char * pName_glb;
            assert( Vec_IntSize(vFans) == 4 );
            assert( Smt_VecEntryIsType(vFans, 0, SMT_PRS_DECLARE_FUN) );
            // get name
            Fan = Vec_IntEntry(vFans, 1);
            assert( Smt_EntryIsName(Fan) );
            pName = Smt_EntryName(p, Fan);
            // added: giving a global suffix 
            pName_glb = (char *) malloc(strlen(pName) + 4 + 1);
            strcpy(pName_glb,pName);
            strcat(pName_glb,SMT_GLO_SUFFIX);
            // FIXME: delete memory of pName
            pName = pName_glb;
            // skip ()
            Fan = Vec_IntEntry(vFans, 2);
            assert( !Smt_EntryIsName(Fan) );
            // check type (Bool or BitVec)
            Fan = Vec_IntEntry(vFans, 3);
            if ( Smt_EntryIsName(Fan) ) 
            {   //(declare-fun s1 () Bool)
                assert( !strcmp("Bool", Smt_VecEntryName(p, vFans, 3)) );
                Range = 1;
            }
            else
            {   // (declare-fun s1 () (_ BitVec 64))
                Fan = Vec_IntEntry(vFans, 3);
                assert( !Smt_EntryIsName(Fan) );
                vFans2 = Smt_EntryNode(p, Fan);
                assert( Vec_IntSize(vFans2) == 3 );
                assert( !strcmp("_", Smt_VecEntryName(p, vFans2, 0)) );
                assert( !strcmp("BitVec", Smt_VecEntryName(p, vFans2, 1)) );
                Fan = Vec_IntEntry(vFans2, 2);
                assert( Smt_EntryIsName(Fan) );
                pRange = Smt_EntryName(p, Fan);
                Range = atoi(pRange);
            }
            // create node
            iObj = Wlc_ObjAlloc( pNtk, WLC_OBJ_PI, 0, Range-1, 0 );
            NameId = Abc_NamStrFindOrAdd( pNtk->pManName, pName, NULL );
            assert( iObj == NameId );
            // save values
            Vec_IntPush( &pNtk->vValues, NameId );
            Vec_IntPush( &pNtk->vValues, nBits );
            Vec_IntPush( &pNtk->vValues, Range );
            nBits += Range;
            ABC_FREE( pName_glb );
        }
        // create constants
        /*
        else if ( Abc_Lit2Var(Fan) == SMT_PRS_DEFINE_FUN ) // added: we parse DEFINE_FUN in LET
        {
            assert( Vec_IntSize(vFans) == 5 );
            assert( Smt_VecEntryIsType(vFans, 0, SMT_PRS_DEFINE_FUN) );
            // get name
            Fan = Vec_IntEntry(vFans, 1);
            assert( Smt_EntryIsName(Fan) );
            pName = Smt_EntryName(p, Fan);
            
            // added: giving a global suffix 
            char * pName_glb = (char *) malloc(strlen(pName) + 4 + 1);
            strcpy(pName_glb,pName);
            strcat(pName_glb,SMT_GLO_SUFFIX);
            // FIXME: delete memory of pName
            pName = pName_glb;
            
            // skip ()
            Fan = Vec_IntEntry(vFans, 2);
            assert( !Smt_EntryIsName(Fan) );
            // check type (Bool or BitVec)
            Fan = Vec_IntEntry(vFans, 3);
            if ( Smt_EntryIsName(Fan) ) 
            {
                // (define-fun s_2 () Bool false)
                assert( !strcmp("Bool", Smt_VecEntryName(p, vFans, 3)) );
                Range = 1;
                pValue = Smt_VecEntryName(p, vFans, 4);
                
                //printf("value: %s\n",pValue);
                
                if ( !strcmp("false", pValue) )
                    pValue = "#b0";
                else if ( !strcmp("true", pValue) )
                    pValue = "#b1";
                else assert( 0 );
                Status = Smt_PrsBuildConstant( pNtk, pValue, Range, pName );    
            }
            else
            {
                // (define-fun s702 () (_ BitVec 4) #xe)
                // (define-fun s1 () (_ BitVec 8) (bvneg #x7f))
                // get range
                Fan = Vec_IntEntry(vFans, 3);
                
                assert( !Smt_EntryIsName(Fan) );
                vFans2 = Smt_VecEntryNode(p, vFans, 3);
                assert( Vec_IntSize(vFans2) == 3 );
                assert( !strcmp("_", Smt_VecEntryName(p, vFans2, 0)) );
                assert( !strcmp("BitVec", Smt_VecEntryName(p, vFans2, 1)) );
                // get range
                Fan = Vec_IntEntry(vFans2, 2);
                
                assert( Smt_EntryIsName(Fan) );
                pRange = Smt_EntryName(p, Fan);
                Range = atoi(pRange);
                
                // added: can parse functions too
                Vec_Int_t * vFans3 = Smt_VecEntryNode(p, vFans, 4);
                Fan = Vec_IntEntry(vFans3, 0);
                
                // get constant
                //Fan = Vec_IntEntry(vFans, 4);
                
                //printf("fan3: %s\n",Fan);
                //printf("fan0: %s\n",Smt_VecEntryName(p, vFans3, 0));
                //printf("fan1: %s\n",Smt_VecEntryName(p, vFans3, 1));
                //printf("fan2: %s\n",Smt_VecEntryName(p, vFans3, 2));
                //printf("fan3: %s\n",Smt_VecEntryName(p, vFans3, 3));
                
                Status = Smt_PrsBuildNode( pNtk, p, Fan, Range, pName );
            }        
            if ( !Status )
            {
                Wlc_NtkFree( pNtk ); pNtk = NULL;
                goto finish;
            }
        }
        */
        // added: new way to parse define-fun
        // create constants
        else if ( Abc_Lit2Var(Fan) == SMT_PRS_DEFINE_FUN ) 
        {
            char * pName_glb;
            // (define-fun def_16001 () Bool (or def_15999 def_16000))
            // (define-fun def_15990 () (_ BitVec 24) (concat def_15988 def_15989))
            assert( Smt_VecEntryIsType(vFans, 0, SMT_PRS_DEFINE_FUN) );
            assert( Vec_IntSize(vFans) == 5 ); // const or definition
            
            // get name
            Fan = Vec_IntEntry(vFans, 1);
            assert( Smt_EntryIsName(Fan) );
            pName = Smt_EntryName(p, Fan);
            // added: giving a global suffix 
            pName_glb = (char *) malloc(strlen(pName) + 4 + 1);
            strcpy(pName_glb,pName);
            strcat(pName_glb,SMT_GLO_SUFFIX);
            // FIXME: delete memory of pName
            pName = pName_glb;
            
            //get range
            Fan = Vec_IntEntry(vFans, 3);
            if ( Smt_EntryIsName(Fan) ) 
            {
                // (define-fun s_2 () Bool false)
                assert( !strcmp("Bool", Smt_VecEntryName(p, vFans, 3)) );
                Range = 1;  
            }
            else
            {
                // (define-fun s702 () (_ BitVec 4) #xe)
                // (define-fun s1 () (_ BitVec 8) (bvneg #x7f))
                assert( !Smt_EntryIsName(Fan) );
                vFans2 = Smt_VecEntryNode(p, vFans, 3);
                assert( Vec_IntSize(vFans2) == 3 );
                assert( !strcmp("_", Smt_VecEntryName(p, vFans2, 0)) );
                assert( !strcmp("BitVec", Smt_VecEntryName(p, vFans2, 1)) );
                // get range
                Fan = Vec_IntEntry(vFans2, 2);
                assert( Smt_EntryIsName(Fan) );
                pRange = Smt_EntryName(p, Fan);
                Range = atoi(pRange);
            }
        
            iObj = Smt_PrsBuild2_rec( pNtk, p, Vec_IntEntry(vFans, 4), Range, pName );
            assert( iObj );
            ABC_FREE( pName_glb );
        }
        
        // collect assertion outputs
        else if ( Abc_Lit2Var(Fan) == SMT_PRS_ASSERT )
        {
            //(assert ; no quantifiers
            //   (let ((s2 (bvsge s0 s1)))
            //   (not s2)))
            //(assert (not (= s0 #x00)))
            assert( Vec_IntSize(vFans) == 2 );
            assert( Smt_VecEntryIsType(vFans, 0, SMT_PRS_ASSERT) );
            pNtk->nAssert++; // added
            iObj = Smt_PrsBuild2_rec( pNtk, p, Vec_IntEntry(vFans, 1), -1, NULL );
            if ( iObj == 0 )
            {
                Wlc_NtkFree( pNtk ); pNtk = NULL;
                goto finish;
            }
            Vec_IntPush( vAsserts, iObj );
        }
        // issue warnings about unknown dirs
        else if ( Abc_Lit2Var(Fan) >= SMT_PRS_END )
            printf( "Ignoring directive \"%s\".\n", Smt_EntryName(p, Fan) );
    }
    // build AND of asserts
    if ( Vec_IntSize(vAsserts) == 1 )
        iObj = Smt_PrsCreateNode( pNtk, WLC_OBJ_BUF, 0, 1, vAsserts, "miter" );
    // added: 0 asserts
    else if ( Vec_IntSize(vAsserts) == 0 )
        iObj = Smt_PrsBuildConstant( pNtk, "#b1", 1, "miter" );
    else
    {
        iObj = Smt_PrsCreateNode( pNtk, WLC_OBJ_BIT_CONCAT, 0, Vec_IntSize(vAsserts), vAsserts, NULL );
        Vec_IntFill( vAsserts, 1, iObj );
        iObj = Smt_PrsCreateNode( pNtk, WLC_OBJ_REDUCT_AND, 0, 1, vAsserts, "miter" );
    }
    Wlc_ObjSetCo( pNtk, Wlc_NtkObj(pNtk, iObj), 0 );
    // create nameIDs
    vFans = Vec_IntStartNatural( Wlc_NtkObjNumMax(pNtk) );
    Vec_IntAppend( &pNtk->vNameIds, vFans );
    Vec_IntFree( vFans );
    //Wlc_NtkReport( pNtk, NULL );
finish:
    // cleanup
    Vec_IntFree(vAsserts);
    return pNtk;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// create error message
static inline int Smt_PrsErrorSet( Smt_Prs_t * p, char * pError, int Value )
{
    assert( !p->ErrorStr[0] );
    sprintf( p->ErrorStr, "%s", pError );
    return Value;
}
// print error message
static inline int Smt_PrsErrorPrint( Smt_Prs_t * p )
{
    char * pThis; int iLine = 0;
    if ( !p->ErrorStr[0] ) return 1;
    for ( pThis = p->pBuffer; pThis < p->pCur; pThis++ )
        iLine += (int)(*pThis == '\n');
    printf( "Line %d: %s\n", iLine, p->ErrorStr );
    return 0;
}
static inline char * Smt_PrsLoadFile( char * pFileName, char ** ppLimit )
{
    char * pBuffer;
    int nFileSize, RetValue;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open input file.\n" );
        return NULL;
    }
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    // move the file current reading position to the beginning
    rewind( pFile ); 
    // load the contents of the file into memory
    pBuffer = ABC_ALLOC( char, nFileSize + 16 );
    pBuffer[0] = '\n';
    RetValue = fread( pBuffer+1, nFileSize, 1, pFile );
    fclose( pFile );
    // terminate the string with '\0'
    pBuffer[nFileSize + 1] = '\n';
    pBuffer[nFileSize + 2] = '\0';
    *ppLimit = pBuffer + nFileSize + 2;
    return pBuffer;
}
static inline int Smt_PrsRemoveComments( char * pBuffer, char * pLimit )
{
    char * pTemp; int nCount1 = 0, nCount2 = 0, fHaveBar = 0;
    int backslash = 0;
    for ( pTemp = pBuffer; pTemp < pLimit; pTemp++ )
    {
        if ( *pTemp == '(' )
            { if ( !fHaveBar ) nCount1++; }
        else if ( *pTemp == ')' )
            { if ( !fHaveBar ) nCount2++; }
        else if ( *pTemp == '|' )
            fHaveBar ^= 1;
        else if ( *pTemp == ';' && !fHaveBar )
            while ( *pTemp && *pTemp != '\n' )
                *pTemp++ = ' ';
        // added: hack to remove quotes 
        else if ( *pTemp == '\"' && *(pTemp-1) != '\\' && !fHaveBar )
        {
            *pTemp++ = ' ';
            while ( *pTemp && (*pTemp != '\"' || backslash))
            {
                if (*pTemp == '\\')
                    backslash = 1;
                else 
                    backslash = 0;
                *pTemp++ = ' ';
            }
            // remove the last quote symbol
            *pTemp = ' ';
        }
    }
    
    if ( nCount1 != nCount2 )
        printf( "The input SMTLIB file has different number of opening and closing parentheses (%d and %d).\n", nCount1, nCount2 );
    else if ( nCount1 == 0 )
        printf( "The input SMTLIB file has no opening or closing parentheses.\n" );
    return nCount1 == nCount2 ? nCount1 : 0;
}
static inline Smt_Prs_t * Smt_PrsAlloc( char * pFileName, char * pBuffer, char * pLimit, int nObjs )
{
    Smt_Prs_t * p;
    if ( nObjs == 0 )
        return NULL;
    p = ABC_CALLOC( Smt_Prs_t, 1 );
    p->pName   = pFileName;
    p->pBuffer = pBuffer;
    p->pLimit  = pLimit;
    p->pCur    = pBuffer;
    p->pStrs   = Abc_NamStart( 1000, 24 );
    Smt_AddTypes( p->pStrs );
    Vec_IntGrow( &p->vStack, 100 );
    //Vec_WecGrow( &p->vDepth, 100 );
    Vec_WecGrow( &p->vObjs, nObjs+1 );
    return p;
}
static inline void Smt_PrsFree( Smt_Prs_t * p )
{
    if ( p->pStrs )
        Abc_NamDeref( p->pStrs );
    Vec_IntErase( &p->vStack );
    Vec_IntErase( &p->vTempFans );
    //Vec_WecErase( &p->vDepth );
    Vec_WecErase( &p->vObjs );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Smt_PrsIsSpace( char c )
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}
static inline void Smt_PrsSkipSpaces( Smt_Prs_t * p )
{
    while ( Smt_PrsIsSpace(*p->pCur) )
        p->pCur++;
}
static inline void Smt_PrsSkipNonSpaces( Smt_Prs_t * p )
{
    while ( p->pCur < p->pLimit && !Smt_PrsIsSpace(*p->pCur) && *p->pCur != '(' && *p->pCur != ')' )
        p->pCur++;
}
void Smt_PrsReadLines( Smt_Prs_t * p )
{
    int fFirstTime = 1;
    assert( Vec_IntSize(&p->vStack) == 0 );
    //assert( Vec_WecSize(&p->vDepth) == 0 );
    assert( Vec_WecSize(&p->vObjs) == 0 );
    // add top node at level 0
    //Vec_WecPushLevel( &p->vDepth );
    //Vec_WecPush( &p->vDepth, Vec_IntSize(&p->vStack), Vec_WecSize(&p->vObjs) );
    // add top node
    Vec_IntPush( &p->vStack, Vec_WecSize(&p->vObjs) );
    Vec_WecPushLevel( &p->vObjs );
    // add other nodes
    for ( p->pCur = p->pBuffer; p->pCur < p->pLimit; p->pCur++ )
    {
        Smt_PrsSkipSpaces( p );
        if ( fFirstTime && *p->pCur == '|' )
        {
            fFirstTime = 0;
            *p->pCur = ' ';
            while ( *p->pCur && *p->pCur != '|' )
                *p->pCur++ = ' ';
            if ( *p->pCur == '|' )
                *p->pCur = ' ';
            continue;
        }
        if ( *p->pCur == '(' )
        {
            // add new node at this depth
            //assert( Vec_WecSize(&p->vDepth) >= Vec_IntSize(&p->vStack) );
            //if ( Vec_WecSize(&p->vDepth) == Vec_IntSize(&p->vStack) )
            //    Vec_WecPushLevel(&p->vDepth);
            //Vec_WecPush( &p->vDepth, Vec_IntSize(&p->vStack), Vec_WecSize(&p->vObjs) );
            // add fanin to node on the previous level
            Vec_IntPush( Vec_WecEntry(&p->vObjs, Vec_IntEntryLast(&p->vStack)), Abc_Var2Lit(Vec_WecSize(&p->vObjs), 0) );            
            // add node to the stack
            Vec_IntPush( &p->vStack, Vec_WecSize(&p->vObjs) );
            Vec_WecPushLevel( &p->vObjs );
        }
        else if ( *p->pCur == ')' )
        {
            Vec_IntPop( &p->vStack );
        }
        else  // token
        {
            char * pStart = p->pCur; 
            Smt_PrsSkipNonSpaces( p );
            if ( p->pCur < p->pLimit )
            {
                // remove strange characters (this can lead to name clashes)
                int iToken;
                /* commented out for SMT comp
                char * pTemp;
                if ( *pStart == '?' )
                    *pStart = '_';
                for ( pTemp = pStart; pTemp < p->pCur; pTemp++ )
                    if ( *pTemp == '.' )
                        *pTemp = '_';*/
                // create and save token for this string
                iToken = Abc_NamStrFindOrAddLim( p->pStrs, pStart, p->pCur--, NULL );
                Vec_IntPush( Vec_WecEntry(&p->vObjs, Vec_IntEntryLast(&p->vStack)), Abc_Var2Lit(iToken, 1) );
            }
        }
    }
    assert( Vec_IntSize(&p->vStack) == 1 );
    assert( Vec_WecSize(&p->vObjs) == Vec_WecCap(&p->vObjs) );
    p->nDigits = Abc_Base16Log( Vec_WecSize(&p->vObjs) );
}
void Smt_PrsPrintParser_rec( Smt_Prs_t * p, int iObj, int Depth )
{
    Vec_Int_t * vFans; int i, Fan;
    printf( "%*s(\n", Depth, "" );
    vFans = Vec_WecEntry( &p->vObjs, iObj );
    Vec_IntForEachEntry( vFans, Fan, i )
    {
        if ( Abc_LitIsCompl(Fan) )
        {
            printf( "%*s", Depth+2, "" );
            printf( "%s\n", Abc_NamStr(p->pStrs, Abc_Lit2Var(Fan)) );
        }
        else
            Smt_PrsPrintParser_rec( p, Abc_Lit2Var(Fan), Depth+4 );
    }
    printf( "%*s)\n", Depth, "" );
}
void Smt_PrsPrintParser( Smt_Prs_t * p )
{
//    Vec_WecPrint( &p->vDepth, 0 );
    Smt_PrsPrintParser_rec( p, 0, 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wlc_Ntk_t * Wlc_ReadSmtBuffer( char * pFileName, char * pBuffer, char * pLimit, int fOldParser, int fPrintTree )
{
    Wlc_Ntk_t * pNtk = NULL;
    int nCount = Smt_PrsRemoveComments( pBuffer, pLimit );
    Smt_Prs_t * p = Smt_PrsAlloc( pFileName, pBuffer, pLimit, nCount );
    if ( p == NULL )
        return NULL;
    Smt_PrsReadLines( p );
    if ( fPrintTree )
        Smt_PrsPrintParser( p );
    if ( Smt_PrsErrorPrint(p) )
        pNtk = fOldParser ? Smt_PrsBuild(p) : Smt_PrsBuild2(p);
    Smt_PrsFree( p );
    return pNtk;
}
Wlc_Ntk_t * Wlc_ReadSmt( char * pFileName, int fOldParser, int fPrintTree )
{
    Wlc_Ntk_t * pNtk = NULL;
    char * pBuffer, * pLimit; 
    pBuffer = Smt_PrsLoadFile( pFileName, &pLimit );
    if ( pBuffer == NULL )
        return NULL;
    pNtk = Wlc_ReadSmtBuffer( pFileName, pBuffer, pLimit, fOldParser, fPrintTree );
    ABC_FREE( pBuffer );
    return pNtk;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
