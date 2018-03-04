/**CFile****************************************************************

  FileName    [mvc.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Data structure for MV cube/cover manipulation.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvc.h,v 1.10 2003/05/02 23:23:59 wjiang Exp $]

***********************************************************************/

#ifndef ABC__misc__mvc__mvc_h
#define ABC__misc__mvc__mvc_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "misc/extra/extra.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// this is the only part of Mvc package, which should be modified
// when compiling the package for other platforms

// these parameters can be computed but setting them manually makes it faster
#define BITS_PER_WORD         32                      // sizeof(Mvc_CubeWord_t) * 8 
#define BITS_PER_WORD_MINUS   31                      // the same minus 1
#define BITS_PER_WORD_LOG     5                       // log2(sizeof(Mvc_CubeWord_t) * 8)
#define BITS_DISJOINT         ((Mvc_CubeWord_t)0x55555555)  // the mask of the type "01010101"
#define BITS_FULL             ((Mvc_CubeWord_t)0xffffffff)  // the mask of the type "11111111"

// uncomment this macro to switch to standard memory management
//#define USE_SYSTEM_MEMORY_MANAGEMENT 

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// cube/list/cover/data
typedef unsigned int               Mvc_CubeWord_t;
typedef struct MvcCubeStruct       Mvc_Cube_t;
typedef struct MvcListStruct       Mvc_List_t;
typedef struct MvcCoverStruct      Mvc_Cover_t;
typedef struct MvcDataStruct       Mvc_Data_t;
typedef struct MvcManagerStruct    Mvc_Manager_t;

// the cube data structure
struct MvcCubeStruct
{
    Mvc_Cube_t *    pNext;        // the next cube in the linked list
    unsigned        iLast   : 24; // the index of the last word
    unsigned        nUnused :  6; // the number of unused bits in the last word
    unsigned        fPrime  :  1; // marks the prime cube
    unsigned        fEssen  :  1; // marks the essential cube
    unsigned        nOnes;        // the number of 1's in the bit data
    Mvc_CubeWord_t  pData[1];     // the first Mvc_CubeWord_t filled with bit data
};

// the single-linked list of cubes in the cover
struct MvcListStruct
{
    Mvc_Cube_t *    pHead;        // the first cube in the list
    Mvc_Cube_t *    pTail;        // the last cube in the list
    int             nItems;       // the number of cubes in the list
};

// the cover data structure
struct MvcCoverStruct
{
    int             nWords;       // the number of machine words
    int             nUnused;      // the number of unused bits in the last word
    int             nBits;        // the number of used data bits in the cube
    Mvc_List_t      lCubes;       // the single-linked list of cubes
    Mvc_Cube_t **   pCubes;       // the array of cubes (for sorting)
    int             nCubesAlloc;  // the size of allocated storage
    int *           pLits;        // the counter of lit occurrances in cubes
    Mvc_Cube_t *    pMask;        // the multipurpose mask
    Mvc_Manager_t * pMem;         // the memory manager
};

// data structure to store information about MV variables
struct MvcDataStruct
{
    Mvc_Manager_t * pMan;         // the memory manager
//    Vm_VarMap_t *   pVm;          // the MV variable data
    int             nBinVars;     // the number of binary variables
    Mvc_Cube_t *    pMaskBin;     // the mask to select the binary bits only
    Mvc_Cube_t **   ppMasks;      // the mask to select each MV variable
    Mvc_Cube_t *    ppTemp[3];    // the temporary cubes
};

// the manager of covers and cubes (as of today, only managing memory)
struct MvcManagerStruct
{
    Extra_MmFixed_t * pManC;        // the manager for covers
    Extra_MmFixed_t * pMan1;        // the manager for 1-word cubes
    Extra_MmFixed_t * pMan2;        // the manager for 2-word cubes
    Extra_MmFixed_t * pMan4;        // the manager for 3-word cubes
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

// reading data from the header of the cube
#define Mvc_CubeReadNext(Cube)       ((Cube)->pNext)
#define Mvc_CubeReadNextP(Cube)      (&(Cube)->pNext)
#define Mvc_CubeReadLast(Cube)       ((Cube)->iLast)
#define Mvc_CubeReadSize(Cube)       ((Cube)->nOnes)
// setting data to the header of the cube
#define Mvc_CubeSetNext(Cube,Next)   ((Cube)->pNext = (Next))
#define Mvc_CubeSetLast(Cube,Last)   ((Cube)->iLast = (Last))
#define Mvc_CubeSetSize(Cube,Size)   ((Cube)->nOnes = (Size))
// checking the number of words

#define Mvc_Cube1Words(Cube)         ((Cube)->iLast == 0) 
#define Mvc_Cube2Words(Cube)         ((Cube)->iLast == 1) 
#define Mvc_CubeNWords(Cube)         ((Cube)->iLast >  1) 
// getting one data bit
#define Mvc_CubeWhichWord(Bit)       ((Bit) >> BITS_PER_WORD_LOG)
#define Mvc_CubeWhichBit(Bit)        ((Bit) &  BITS_PER_WORD_MINUS)
// accessing individual bits
#define Mvc_CubeBitValue(Cube, Bit) (((Cube)->pData[Mvc_CubeWhichWord(Bit)] &   (((Mvc_CubeWord_t)1)<<(Mvc_CubeWhichBit(Bit)))) > 0)
#define Mvc_CubeBitInsert(Cube, Bit) ((Cube)->pData[Mvc_CubeWhichWord(Bit)] |=  (((Mvc_CubeWord_t)1)<<(Mvc_CubeWhichBit(Bit))))
#define Mvc_CubeBitRemove(Cube, Bit) ((Cube)->pData[Mvc_CubeWhichWord(Bit)] &= ~(((Mvc_CubeWord_t)1)<<(Mvc_CubeWhichBit(Bit))))
// accessing values of the binary variables
#define Mvc_CubeVarValue(Cube, Var) (((Cube)->pData[Mvc_CubeWhichWord(2*(Var))] >> (Mvc_CubeWhichBit(2*(Var)))) & ((Mvc_CubeWord_t)3))

// various macros

// cleaning the data bits of the cube
#define Mvc_Cube1BitClean( Cube )\
          ((Cube)->pData[0] = 0)
#define Mvc_Cube2BitClean( Cube )\
         (((Cube)->pData[0] = 0),\
          ((Cube)->pData[1] = 0))
#define Mvc_CubeNBitClean( Cube  )\
{\
    int _i_;\
    for( _i_ = (Cube)->iLast; _i_ >= 0; _i_--)\
           (Cube)->pData[_i_] = 0;\
}

// cleaning the unused part of the lat word
#define Mvc_CubeBitCleanUnused( Cube )\
    ((Cube)->pData[(Cube)->iLast] &= (BITS_FULL >> (Cube)->nUnused))

// filling the used data bits with 1's
#define Mvc_Cube1BitFill( Cube )\
           (Cube)->pData[0] = (BITS_FULL >> (Cube)->nUnused);
#define Mvc_Cube2BitFill( Cube )\
         (((Cube)->pData[0] =  BITS_FULL),\
          ((Cube)->pData[1] = (BITS_FULL >> (Cube)->nUnused))) 
#define Mvc_CubeNBitFill( Cube )\
{\
    int _i_;\
    (Cube)->pData[(Cube)->iLast] = (BITS_FULL >> (Cube)->nUnused);\
    for( _i_ = (Cube)->iLast - 1; _i_ >= 0; _i_-- )\
        (Cube)->pData[_i_] =  BITS_FULL;\
}

// complementing the data bits
#define Mvc_Cube1BitNot( Cube )\
          ((Cube)->pData[0] ^= (BITS_FULL >> (Cube)->nUnused))
#define Mvc_Cube2BitNot( Cube )\
         (((Cube)->pData[0] ^=  BITS_FULL),\
          ((Cube)->pData[1] ^= (BITS_FULL >> (Cube)->nUnused)))
#define Mvc_CubeNBitNot( Cube )\
{\
    int _i_;\
    (Cube)->pData[(Cube)->iLast] ^= (BITS_FULL >> (Cube)->nUnused);\
    for( _i_ = (Cube)->iLast - 1; _i_ >= 0; _i_-- )\
        (Cube)->pData[_i_] ^=  BITS_FULL;\
}

#define Mvc_Cube1BitCopy( Cube1, Cube2 )\
         (((Cube1)->pData[0]) = ((Cube2)->pData[0]))
#define Mvc_Cube2BitCopy( Cube1, Cube2 )\
        ((((Cube1)->pData[0]) = ((Cube2)->pData[0])),\
         (((Cube1)->pData[1])=  ((Cube2)->pData[1])))
#define Mvc_CubeNBitCopy( Cube1, Cube2 )\
{\
    int _i_;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
        ((Cube1)->pData[_i_]) = ((Cube2)->pData[_i_]);\
}

#define Mvc_Cube1BitOr( CubeR, Cube1, Cube2 )\
         (((CubeR)->pData[0]) = ((Cube1)->pData[0] | (Cube2)->pData[0]))
#define Mvc_Cube2BitOr( CubeR, Cube1, Cube2 )\
        ((((CubeR)->pData[0]) = ((Cube1)->pData[0] | (Cube2)->pData[0])),\
         (((CubeR)->pData[1]) = ((Cube1)->pData[1] | (Cube2)->pData[1])))
#define Mvc_CubeNBitOr( CubeR, Cube1, Cube2 )\
{\
    int _i_;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
        (((CubeR)->pData[_i_]) = ((Cube1)->pData[_i_] | (Cube2)->pData[_i_]));\
}

#define Mvc_Cube1BitExor( CubeR, Cube1, Cube2 )\
         (((CubeR)->pData[0]) = ((Cube1)->pData[0] ^ (Cube2)->pData[0]))
#define Mvc_Cube2BitExor( CubeR, Cube1, Cube2 )\
        ((((CubeR)->pData[0]) = ((Cube1)->pData[0] ^ (Cube2)->pData[0])),\
         (((CubeR)->pData[1]) = ((Cube1)->pData[1] ^ (Cube2)->pData[1])))
#define Mvc_CubeNBitExor( CubeR, Cube1, Cube2 )\
{\
    int _i_;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
        (((CubeR)->pData[_i_]) = ((Cube1)->pData[_i_] ^ (Cube2)->pData[_i_]));\
}

#define Mvc_Cube1BitAnd( CubeR, Cube1, Cube2 )\
         (((CubeR)->pData[0]) = ((Cube1)->pData[0] & (Cube2)->pData[0]))
#define Mvc_Cube2BitAnd( CubeR, Cube1, Cube2 )\
        ((((CubeR)->pData[0]) = ((Cube1)->pData[0] & (Cube2)->pData[0])),\
         (((CubeR)->pData[1]) = ((Cube1)->pData[1] & (Cube2)->pData[1])))
#define Mvc_CubeNBitAnd( CubeR, Cube1, Cube2 )\
{\
    int _i_;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
        (((CubeR)->pData[_i_]) = ((Cube1)->pData[_i_] & (Cube2)->pData[_i_]));\
} 

#define Mvc_Cube1BitSharp( CubeR, Cube1, Cube2 )\
         (((CubeR)->pData[0]) = ((Cube1)->pData[0] & ~((Cube2)->pData[0])))
#define Mvc_Cube2BitSharp( CubeR, Cube1, Cube2 )\
        ((((CubeR)->pData[0]) = ((Cube1)->pData[0] & ~((Cube2)->pData[0]))),\
         (((CubeR)->pData[1]) = ((Cube1)->pData[1] & ~((Cube2)->pData[1]))))
#define Mvc_CubeNBitSharp( CubeR, Cube1, Cube2 )\
{\
    int _i_;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
        (((CubeR)->pData[_i_]) = ((Cube1)->pData[_i_] & ~(Cube2)->pData[_i_]));\
}  

#define Mvc_Cube1BitEmpty( Res, Cube )\
  (Res = ((Cube)->pData[0] == 0))
#define Mvc_Cube2BitEmpty( Res, Cube )\
  (Res = ((Cube)->pData[0] == 0 && (Cube)->pData[1] == 0))
#define Mvc_CubeNBitEmpty( Res, Cube )\
{\
    int _i_; Res = 1;\
    for (_i_ = (Cube)->iLast; _i_ >= 0; _i_--)\
        if ( (Cube)->pData[_i_] )\
           { Res = 0; break; }\
}

#define Mvc_Cube1BitEqual( Res, Cube1, Cube2 )\
  (Res = (((Cube1)->pData[0]) == ((Cube2)->pData[0])))
#define Mvc_Cube2BitEqual( Res, Cube1, Cube2 )\
 (Res = ((((Cube1)->pData[0]) == ((Cube2)->pData[0])) &&\
         (((Cube1)->pData[1]) == ((Cube2)->pData[1]))))
#define Mvc_CubeNBitEqual( Res, Cube1, Cube2 )\
{\
    int _i_; Res = 1;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
         if (((Cube1)->pData[_i_]) != ((Cube2)->pData[_i_]))\
              { Res = 0; break; }\
}

#define Mvc_Cube1BitLess( Res, Cube1, Cube2 )\
  (Res = (((Cube1)->pData[0]) <  ((Cube2)->pData[0])))
#define Mvc_Cube2BitLess( Res, Cube1, Cube2 )\
 (Res = ((((Cube1)->pData[0]) <  ((Cube2)->pData[0])) ||\
        ((((Cube1)->pData[0]) == ((Cube2)->pData[0])) && (((Cube1)->pData[1]) <  ((Cube2)->pData[1])))))
#define Mvc_CubeNBitLess( Res, Cube1, Cube2 )\
{\
    int _i_; Res = 1;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
         if (((Cube1)->pData[_i_]) >= ((Cube2)->pData[_i_]))\
             { Res = 0; break; }\
}        

#define Mvc_Cube1BitMore( Res, Cube1, Cube2 )\
  (Res = (((Cube1)->pData[0]) >  ((Cube2)->pData[0])))
#define Mvc_Cube2BitMore( Res, Cube1, Cube2 )\
 (Res = ((((Cube1)->pData[0]) >  ((Cube2)->pData[0])) ||\
        ((((Cube1)->pData[0]) == ((Cube2)->pData[0])) && (((Cube1)->pData[1]) >  ((Cube2)->pData[1])))))
#define Mvc_CubeNBitMore( Res, Cube1, Cube2 )\
{\
    int _i_; Res = 1;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
        if (((Cube1)->pData[_i_]) <= ((Cube2)->pData[_i_]))\
             { Res = 0; break; }\
}         

#define Mvc_Cube1BitNotImpl( Res, Cube1, Cube2 )\
  (Res = (((Cube1)->pData[0])  & ~((Cube2)->pData[0])))
#define Mvc_Cube2BitNotImpl( Res, Cube1, Cube2 )\
 (Res = ((((Cube1)->pData[0]) & ~((Cube2)->pData[0])) ||\
         (((Cube1)->pData[1]) & ~((Cube2)->pData[1]))))
#define Mvc_CubeNBitNotImpl( Res, Cube1, Cube2 )\
{\
    int _i_; Res = 0;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
        if (((Cube1)->pData[_i_]) & ~((Cube2)->pData[_i_]))\
            { Res = 1; break; }\
}         

#define Mvc_Cube1BitDisjoint( Res, Cube1, Cube2 )\
  (Res = ((((Cube1)->pData[0]) &  ((Cube2)->pData[0])) == 0 ))
#define Mvc_Cube2BitDisjoint( Res, Cube1, Cube2 )\
 (Res = (((((Cube1)->pData[0]) &  ((Cube2)->pData[0])) == 0 ) &&\
         ((((Cube1)->pData[1]) &  ((Cube2)->pData[1])) == 0 )))
#define Mvc_CubeNBitDisjoint( Res, Cube1, Cube2 )\
{\
    int _i_; Res = 1;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
        if (((Cube1)->pData[_i_]) & ((Cube2)->pData[_i_]))\
             { Res = 0; break; }\
}             

#define Mvc_Cube1BitEqualUnderMask( Res, Cube1, Cube2, Mask )\
  (Res = ((((Cube1)->pData[0]) & ((Mask)->pData[0])) == (((Cube2)->pData[0]) & ((Mask)->pData[0]))))
#define Mvc_Cube2BitEqualUnderMask( Res, Cube1, Cube2, Mask )\
 (Res = (((((Cube1)->pData[0]) & ((Mask)->pData[0])) == (((Cube2)->pData[0]) & ((Mask)->pData[0])))  &&\
         ((((Cube1)->pData[1]) & ((Mask)->pData[1])) == (((Cube2)->pData[1]) & ((Mask)->pData[1])))))
#define Mvc_CubeNBitEqualUnderMask( Res, Cube1, Cube2, Mask )\
{\
    int _i_; Res = 1;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
        if ((((Cube1)->pData[_i_]) & ((Mask)->pData[_i_])) != (((Cube2)->pData[_i_]) & ((Mask)->pData[_i_])))\
            { Res = 0; break; }\
}               

#define Mvc_Cube1BitEqualOutsideMask( Res, Cube1, Cube2, Mask )\
  (Res = ((((Cube1)->pData[0]) | ((Mask)->pData[0])) == (((Cube2)->pData[0]) | ((Mask)->pData[0]))))
#define Mvc_Cube2BitEqualOutsideMask( Res, Cube1, Cube2, Mask )\
 (Res = (((((Cube1)->pData[0]) | ((Mask)->pData[0])) == (((Cube2)->pData[0]) | ((Mask)->pData[0])))  &&\
         ((((Cube1)->pData[1]) | ((Mask)->pData[1])) == (((Cube2)->pData[1]) | ((Mask)->pData[1])))))
#define Mvc_CubeNBitEqualOutsideMask( Res, Cube1, Cube2, Mask )\
{\
    int _i_; Res = 1;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
        if ((((Cube1)->pData[_i_]) | ((Mask)->pData[_i_])) != (((Cube2)->pData[_i_]) | ((Mask)->pData[_i_])))\
            { Res = 0; break; }\
}              

#define Mvc_Cube1BitIntersectUnderMask( Res, Cube1, Cube2, Mask)\
  (Res = ((((Cube1)->pData[0]) & ((Cube2)->pData[0]) & ((Mask)->pData[0])) > 0))
#define Mvc_Cube2BitIntersectUnderMask( Res, Cube1, Cube2, Mask)\
 (Res = (((((Cube1)->pData[0]) & ((Cube2)->pData[0]) & ((Mask)->pData[0])) > 0) ||\
         ((((Cube1)->pData[1]) & ((Cube2)->pData[1]) & ((Mask)->pData[1])) > 0)))
#define Mvc_CubeNBitIntersectUnderMask( Res, Cube1, Cube2, Mask)\
{\
    int _i_; Res = 0;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
        if (((Cube1)->pData[_i_]) & ((Cube2)->pData[_i_]) & ((Mask)->pData[_i_]))\
           { Res = 1; break; }\
}       

#define Mvc_Cube1BitNotImplUnderMask( Res, Cube1, Cube2, Mask )\
  (Res = (((Mask)->pData[0]) & ((Cube1)->pData[0])  & ~((Cube2)->pData[0])))
#define Mvc_Cube2BitNotImplUnderMask( Res, Cube1, Cube2, Mask )\
 (Res = ((((Mask)->pData[0]) & ((Cube1)->pData[0]) & ~((Cube2)->pData[0])) ||\
         (((Mask)->pData[1]) & ((Cube1)->pData[1]) & ~((Cube2)->pData[1]))))
#define Mvc_CubeNBitNotImplUnderMask( Res, Cube1, Cube2, Mask )\
{\
    int _i_; Res = 0;\
    for (_i_ = (Cube1)->iLast; _i_ >= 0; _i_--)\
        if (((Mask)->pData[_i_]) & ((Cube1)->pData[_i_]) & ~((Cube2)->pData[_i_]))\
            { Res = 1; break; }\
}         

// the following macros make no assumption about the cube's bitset size
#define Mvc_CubeBitClean( Cube )\
    if ( Mvc_Cube1Words(Cube) )      { Mvc_Cube1BitClean( Cube ); }\
    else if ( Mvc_Cube2Words(Cube) ) { Mvc_Cube2BitClean( Cube ); }\
    else                             { Mvc_CubeNBitClean( Cube ); }
#define Mvc_CubeBitFill( Cube )\
    if ( Mvc_Cube1Words(Cube) )      { Mvc_Cube1BitFill( Cube ); }\
    else if ( Mvc_Cube2Words(Cube) ) { Mvc_Cube2BitFill( Cube ); }\
    else                             { Mvc_CubeNBitFill( Cube ); }
#define Mvc_CubeBitNot( Cube )\
    if ( Mvc_Cube1Words(Cube) )      { Mvc_Cube1BitNot( Cube ); }\
    else if ( Mvc_Cube2Words(Cube) ) { Mvc_Cube2BitNot( Cube ); }\
    else                             { Mvc_CubeNBitNot( Cube ); }
#define Mvc_CubeBitCopy( Cube1, Cube2 )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitCopy( Cube1, Cube2 ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitCopy( Cube1, Cube2 ); }\
    else                             { Mvc_CubeNBitCopy( Cube1, Cube2 ); }
#define Mvc_CubeBitOr( CubeR, Cube1, Cube2 )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitOr( CubeR, Cube1, Cube2 ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitOr( CubeR, Cube1, Cube2 ); }\
    else                             { Mvc_CubeNBitOr( CubeR, Cube1, Cube2 ); }
#define Mvc_CubeBitExor( CubeR, Cube1, Cube2 )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitExor( CubeR, Cube1, Cube2 ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitExor( CubeR, Cube1, Cube2 ); }\
    else                             { Mvc_CubeNBitExor( CubeR, Cube1, Cube2 ); }
#define Mvc_CubeBitAnd( CubeR, Cube1, Cube2 )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitAnd( CubeR, Cube1, Cube2 ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitAnd( CubeR, Cube1, Cube2 ); }\
    else                             { Mvc_CubeNBitAnd( CubeR, Cube1, Cube2 ); }
#define Mvc_CubeBitSharp( CubeR, Cube1, Cube2 )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitSharp( CubeR, Cube1, Cube2 ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitSharp( CubeR, Cube1, Cube2 ); }\
    else                             { Mvc_CubeNBitSharp( CubeR, Cube1, Cube2 ); }
#define Mvc_CubeBitEmpty( Res, Cube )\
    if ( Mvc_Cube1Words(Cube) )      { Mvc_Cube1BitEmpty( Res, Cube ); }\
    else if ( Mvc_Cube2Words(Cube) ) { Mvc_Cube2BitEmpty( Res, Cube ); }\
    else                             { Mvc_CubeNBitEmpty( Res, Cube ); }
#define Mvc_CubeBitEqual( Res, Cube1, Cube2 )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitEqual( Res, Cube1, Cube2 ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitEqual( Res, Cube1, Cube2 ); }\
    else                             { Mvc_CubeNBitEqual( Res, Cube1, Cube2 ); }
#define Mvc_CubeBitLess( Res, Cube1, Cube2 )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitLess( Res, Cube1, Cube2 ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitLess( Res, Cube1, Cube2 ); }\
    else                             { Mvc_CubeNBitLess( Res, Cube1, Cube2 ); }
#define Mvc_CubeBitMore( Res, Cube1, Cube2 )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitMore( Res, Cube1, Cube2 ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitMore( Res, Cube1, Cube2 ); }\
    else                             { Mvc_CubeNBitMore( Res, Cube1, Cube2 ); }
#define Mvc_CubeBitNotImpl( Res, Cube1, Cube2 )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitNotImpl( Res, Cube1, Cube2 ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitNotImpl( Res, Cube1, Cube2 ); }\
    else                             { Mvc_CubeNBitNotImpl( Res, Cube1, Cube2 ); }
#define Mvc_CubeBitDisjoint( Res, Cube1, Cube2 )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitDisjoint( Res, Cube1, Cube2 ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitDisjoint( Res, Cube1, Cube2 ); }\
    else                             { Mvc_CubeNBitDisjoint( Res, Cube1, Cube2 ); }
#define Mvc_CubeBitEqualUnderMask( Res, Cube1, Cube2, Mask )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitEqualUnderMask( Res, Cube1, Cube2, Mask ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitEqualUnderMask( Res, Cube1, Cube2, Mask ); }\
    else                             { Mvc_CubeNBitEqualUnderMask( Res, Cube1, Cube2, Mask ); }
#define Mvc_CubeBitEqualOutsideMask( Res, Cube1, Cube2, Mask )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitEqualOutsideMask( Res, Cube1, Cube2, Mask ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitEqualOutsideMask( Res, Cube1, Cube2, Mask ); }\
    else                             { Mvc_CubeNBitEqualOutsideMask( Res, Cube1, Cube2, Mask ); }
#define Mvc_CubeBitIntersectUnderMask( Res, Cube1, Cube2, Mask )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitIntersectUnderMask( Res, Cube1, Cube2, Mask ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitIntersectUnderMask( Res, Cube1, Cube2, Mask ); }\
    else                             { Mvc_CubeNBitIntersectUnderMask( Res, Cube1, Cube2, Mask ); }
#define Mvc_CubeBitNotImplUnderMask( Res, Cube1, Cube2, Mask )\
    if ( Mvc_Cube1Words(Cube1) )     { Mvc_Cube1BitNotImplUnderMask( Res, Cube1, Cube2, Mask ); }\
    else if ( Mvc_Cube2Words(Cube1) ){ Mvc_Cube2BitNotImplUnderMask( Res, Cube1, Cube2, Mask ); }\
    else                             { Mvc_CubeNBitNotImplUnderMask( Res, Cube1, Cube2, Mask ); }


// managing linked lists
#define Mvc_ListAddCubeHead( pList, pCube )\
    {\
        if ( pList->pHead == NULL )\
        {\
            Mvc_CubeSetNext( pCube, NULL );\
            pList->pHead = pCube;\
            pList->pTail = pCube;\
        }\
        else\
        {\
            Mvc_CubeSetNext( pCube, pList->pHead );\
            pList->pHead = pCube;\
        }\
        pList->nItems++;\
    }
#define Mvc_ListAddCubeTail( pList, pCube )\
    {\
        if ( pList->pHead == NULL )\
            pList->pHead = pCube;\
        else\
            Mvc_CubeSetNext( pList->pTail, pCube );\
        pList->pTail    = pCube;\
        Mvc_CubeSetNext( pCube, NULL );\
        pList->nItems++;\
    }
#define Mvc_ListDeleteCube( pList, pPrev, pCube )\
{\
    if ( pPrev == NULL )\
        pList->pHead = pCube->pNext;\
    else\
        pPrev->pNext = pCube->pNext;\
    if ( pList->pTail == pCube )\
    {\
        assert( pCube->pNext == NULL );\
        pList->pTail = pPrev;\
    }\
    pList->nItems--;\
}

// managing linked lists inside the cover
#define Mvc_CoverAddCubeHead( pCover, pCube )\
{\
    Mvc_List_t * pList = &pCover->lCubes;\
    Mvc_ListAddCubeHead( pList, pCube );\
}
#define Mvc_CoverAddCubeTail( pCover, pCube )\
{\
    Mvc_List_t * pList = &pCover->lCubes;\
    Mvc_ListAddCubeTail( pList, pCube );\
}
#define Mvc_CoverDeleteCube( pCover, pPrev, pCube )\
{\
    Mvc_List_t * pList = &pCover->lCubes;\
    Mvc_ListDeleteCube( pList, pPrev, pCube );\
}






// iterator through the cubes in the cube list
#define Mvc_ListForEachCube( List, Cube )\
    for ( Cube = List->pHead;\
          Cube;\
          Cube = Cube->pNext )
#define Mvc_ListForEachCubeSafe( List, Cube, Cube2 )\
    for ( Cube = List->pHead, Cube2 = (Cube? Cube->pNext: NULL);\
          Cube;\
          Cube = Cube2, Cube2 = (Cube? Cube->pNext: NULL) )

// iterator through cubes in the cover
#define Mvc_CoverForEachCube( Cover, Cube )\
    for ( Cube = (Cover)->lCubes.pHead;\
          Cube;\
          Cube = Cube->pNext )
#define Mvc_CoverForEachCubeWithIndex( Cover, Cube, Index )\
    for ( Index = 0, Cube = (Cover)->lCubes.pHead;\
          Cube;\
          Index++, Cube = Cube->pNext )
#define Mvc_CoverForEachCubeSafe( Cover, Cube, Cube2 )\
    for ( Cube = (Cover)->lCubes.pHead, Cube2 = (Cube? Cube->pNext: NULL);\
          Cube;\
          Cube = Cube2, Cube2 = (Cube? Cube->pNext: NULL) )

// iterator which starts from the given cube
#define Mvc_CoverForEachCubeStart( Start, Cube )\
    for ( Cube = Start;\
          Cube;\
          Cube = Cube->pNext )
#define Mvc_CoverForEachCubeStartSafe( Start, Cube, Cube2 )\
    for ( Cube = Start, Cube2 = (Cube? Cube->pNext: NULL);\
          Cube;\
          Cube = Cube2, Cube2 = (Cube? Cube->pNext: NULL) )


// iterator through literals of the cube
#define Mvc_CubeForEachBit( Cover, Cube, iBit, Value )\
    for ( iBit = 0;\
          iBit < Cover->nBits && ((Value = Mvc_CubeBitValue(Cube,iBit)), 1);\
          iBit++ )
// iterator through values of binary variables
#define Mvc_CubeForEachVarValue( Cover, Cube, iVar, Value )\
    for ( iVar = 0;\
          iVar < Cover->nBits/2 && (Value = Mvc_CubeVarValue(Cube,iVar));\
          iVar++ )


// macros which work with memory
// MEM_ALLOC: allocate the given number (Size) of items of type (Type)
// MEM_FREE:  deallocate the pointer (Pointer) to the given number (Size) of items of type (Type)
#define MEM_ALLOC( Manager, Type, Size )          ((Type *)ABC_ALLOC( char, (Size) * sizeof(Type) ))
#define MEM_FREE( Manager, Type, Size, Pointer )  if ( Pointer ) { ABC_FREE(Pointer); Pointer = NULL; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== mvcApi.c ====================================================*/
extern int              Mvc_CoverReadWordNum( Mvc_Cover_t * pCover );
extern int              Mvc_CoverReadBitNum( Mvc_Cover_t * pCover );
extern int              Mvc_CoverReadCubeNum( Mvc_Cover_t * pCover );
extern Mvc_Cube_t *     Mvc_CoverReadCubeHead( Mvc_Cover_t * pCover );
extern Mvc_Cube_t *     Mvc_CoverReadCubeTail( Mvc_Cover_t * pCover );
extern Mvc_List_t *     Mvc_CoverReadCubeList( Mvc_Cover_t * pCover );
extern int              Mvc_ListReadCubeNum( Mvc_List_t * pList );
extern Mvc_Cube_t *     Mvc_ListReadCubeHead( Mvc_List_t * pList );
extern Mvc_Cube_t *     Mvc_ListReadCubeTail( Mvc_List_t * pList );
extern void             Mvc_CoverSetCubeNum( Mvc_Cover_t * pCover,int nItems );
extern void             Mvc_CoverSetCubeHead( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );
extern void             Mvc_CoverSetCubeTail( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );
extern void             Mvc_CoverSetCubeList( Mvc_Cover_t * pCover, Mvc_List_t * pList );
extern int              Mvc_CoverIsEmpty( Mvc_Cover_t * pCover );
extern int              Mvc_CoverIsTautology( Mvc_Cover_t * pCover );
extern int              Mvc_CoverIsBinaryBuffer( Mvc_Cover_t * pCover );
extern void             Mvc_CoverMakeEmpty( Mvc_Cover_t * pCover );
extern void             Mvc_CoverMakeTautology( Mvc_Cover_t * pCover );
extern Mvc_Cover_t *    Mvc_CoverCreateEmpty( Mvc_Cover_t * pCover );
extern Mvc_Cover_t *    Mvc_CoverCreateTautology( Mvc_Cover_t * pCover );
/*=== mvcCover.c ====================================================*/
extern Mvc_Cover_t *    Mvc_CoverAlloc( Mvc_Manager_t * pMem, int nBits );
extern Mvc_Cover_t *    Mvc_CoverCreateConst( Mvc_Manager_t * pMem, int nBits, int  Phase );
extern Mvc_Cover_t *    Mvc_CoverClone( Mvc_Cover_t * pCover );
extern Mvc_Cover_t *    Mvc_CoverDup( Mvc_Cover_t * pCover );
extern void             Mvc_CoverFree( Mvc_Cover_t * pCover );
extern void             Mvc_CoverAllocateMask( Mvc_Cover_t * pCover );
extern void             Mvc_CoverAllocateArrayLits( Mvc_Cover_t * pCover );
extern void             Mvc_CoverAllocateArrayCubes( Mvc_Cover_t * pCover );
extern void             Mvc_CoverDeallocateMask( Mvc_Cover_t * pCover );
extern void             Mvc_CoverDeallocateArrayLits( Mvc_Cover_t * pCover );
/*=== mvcCube.c ====================================================*/
extern Mvc_Cube_t *     Mvc_CubeAlloc( Mvc_Cover_t * pCover );
extern Mvc_Cube_t *     Mvc_CubeDup( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );
extern void             Mvc_CubeFree( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );
extern void             Mvc_CubeBitRemoveDcs( Mvc_Cube_t * pCube );
/*=== mvcCompare.c ====================================================*/
extern int              Mvc_CubeCompareInt( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask );
extern int              Mvc_CubeCompareSizeAndInt( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask );
extern int              Mvc_CubeCompareIntUnderMask( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask );
extern int              Mvc_CubeCompareIntOutsideMask( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask );
extern int              Mvc_CubeCompareIntOutsideAndUnderMask( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask );
/*=== mvcDd.c ====================================================*/
/*
extern DdNode *         Mvc_CoverConvertToBdd( DdManager * dd, Mvc_Cover_t * pCover );
extern DdNode *         Mvc_CoverConvertToZdd( DdManager * dd, Mvc_Cover_t * pCover );
extern DdNode *         Mvc_CoverConvertToZdd2( DdManager * dd, Mvc_Cover_t * pCover );
extern DdNode *         Mvc_CubeConvertToBdd( DdManager * dd, Mvc_Cube_t * pCube );
extern DdNode *         Mvc_CubeConvertToZdd( DdManager * dd, Mvc_Cube_t * pCube );
extern DdNode *         Mvc_CubeConvertToZdd2( DdManager * dd, Mvc_Cube_t * pCube );
*/
/*=== mvcDivisor.c ====================================================*/
extern Mvc_Cover_t *    Mvc_CoverDivisor( Mvc_Cover_t * pCover );
/*=== mvcDivide.c ====================================================*/
extern void             Mvc_CoverDivide( Mvc_Cover_t * pCover, Mvc_Cover_t * pDiv, Mvc_Cover_t ** ppQuo, Mvc_Cover_t ** ppRem );
extern void             Mvc_CoverDivideInternal( Mvc_Cover_t * pCover, Mvc_Cover_t * pDiv, Mvc_Cover_t ** ppQuo, Mvc_Cover_t ** ppRem );
extern void             Mvc_CoverDivideByLiteral( Mvc_Cover_t * pCover, Mvc_Cover_t * pDiv, Mvc_Cover_t ** ppQuo, Mvc_Cover_t ** ppRem );
extern void             Mvc_CoverDivideByCube( Mvc_Cover_t * pCover, Mvc_Cover_t * pDiv, Mvc_Cover_t ** ppQuo, Mvc_Cover_t ** ppRem );
extern void             Mvc_CoverDivideByLiteralQuo( Mvc_Cover_t * pCover, int iLit );
/*=== mvcList.c ====================================================*/
// these functions are available as macros
extern void             Mvc_ListAddCubeHead_( Mvc_List_t * pList, Mvc_Cube_t * pCube );
extern void             Mvc_ListAddCubeTail_( Mvc_List_t * pList, Mvc_Cube_t * pCube );
extern void             Mvc_ListDeleteCube_( Mvc_List_t * pList, Mvc_Cube_t * pPrev, Mvc_Cube_t * pCube );
extern void             Mvc_CoverAddCubeHead_( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );
extern void             Mvc_CoverAddCubeTail_( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );
extern void             Mvc_CoverDeleteCube_( Mvc_Cover_t * pCover, Mvc_Cube_t * pPrev, Mvc_Cube_t * pCube );
extern void             Mvc_CoverAddDupCubeHead( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );
extern void             Mvc_CoverAddDupCubeTail( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );
// other functions
extern void             Mvc_CoverAddLiteralsOfCube( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );
extern void             Mvc_CoverDeleteLiteralsOfCube( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );
extern void             Mvc_CoverList2Array( Mvc_Cover_t * pCover );
extern void             Mvc_CoverArray2List( Mvc_Cover_t * pCover );
extern Mvc_Cube_t *     Mvc_ListGetTailFromHead( Mvc_Cube_t * pHead );
/*=== mvcPrint.c ====================================================*/
extern void             Mvc_CoverPrint( Mvc_Cover_t * pCover );
extern void             Mvc_CubePrint( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );
extern void             Mvc_CoverPrintMv( Mvc_Data_t * pData, Mvc_Cover_t * pCover );
extern void             Mvc_CubePrintMv( Mvc_Data_t * pData, Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );
/*=== mvcSort.c ====================================================*/
extern void             Mvc_CoverSort( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask, int (* pCompareFunc)(Mvc_Cube_t *, Mvc_Cube_t *, Mvc_Cube_t *) );
/*=== mvcUtils.c ====================================================*/
extern void             Mvc_CoverSupport( Mvc_Cover_t * pCover, Mvc_Cube_t * pSupp );
extern int              Mvc_CoverSupportSizeBinary( Mvc_Cover_t * pCover );
extern int              Mvc_CoverSupportVarBelongs( Mvc_Cover_t * pCover, int iVar );
extern void             Mvc_CoverCommonCube( Mvc_Cover_t * pCover, Mvc_Cube_t * pComCube );
extern int              Mvc_CoverIsCubeFree( Mvc_Cover_t * pCover );
extern void             Mvc_CoverMakeCubeFree( Mvc_Cover_t * pCover );
extern Mvc_Cover_t *    Mvc_CoverCommonCubeCover( Mvc_Cover_t * pCover );
extern int              Mvc_CoverCheckSuppContainment( Mvc_Cover_t * pCover1, Mvc_Cover_t * pCover2 );
extern int              Mvc_CoverSetCubeSizes( Mvc_Cover_t * pCover );
extern int              Mvc_CoverGetCubeSize( Mvc_Cube_t * pCube );
extern int              Mvc_CoverCountCubePairDiffs( Mvc_Cover_t * pCover, unsigned char pDiffs[] );
extern Mvc_Cover_t *    Mvc_CoverRemap( Mvc_Cover_t * pCover, int * pVarsRem, int nVarsRem );
extern void             Mvc_CoverInverse( Mvc_Cover_t * pCover );
extern Mvc_Cover_t *    Mvc_CoverRemoveDontCareLits( Mvc_Cover_t * pCover );
extern Mvc_Cover_t *    Mvc_CoverCofactor( Mvc_Cover_t * pCover, int iValue, int iValueOther );
extern Mvc_Cover_t *    Mvc_CoverFlipVar( Mvc_Cover_t * pCover, int iValue0, int iValue1 );
extern Mvc_Cover_t *    Mvc_CoverUnivQuantify( Mvc_Cover_t * p, int iValueA0, int iValueA1, int iValueB0, int iValueB1 );
extern Mvc_Cover_t **   Mvc_CoverCofactors( Mvc_Data_t * pData, Mvc_Cover_t * pCover, int iVar );
extern int              Mvr_CoverCountLitsWithValue( Mvc_Data_t * pData, Mvc_Cover_t * pCover, int iVar, int iValue );
//extern Mvc_Cover_t *    Mvc_CoverCreateExpanded( Mvc_Cover_t * pCover, Vm_VarMap_t * pVmNew );
extern Mvc_Cover_t *    Mvc_CoverTranspose( Mvc_Cover_t * pCover );
extern int              Mvc_UtilsCheckUnusedZeros( Mvc_Cover_t * pCover );
/*=== mvcLits.c ====================================================*/
extern int              Mvc_CoverAnyLiteral( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask );
extern int              Mvc_CoverBestLiteral( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask );
extern int              Mvc_CoverWorstLiteral( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask );
extern Mvc_Cover_t *    Mvc_CoverBestLiteralCover( Mvc_Cover_t * pCover, Mvc_Cover_t * pSimple );
extern int              Mvc_CoverFirstCubeFirstLit( Mvc_Cover_t * pCover );
extern int              Mvc_CoverCountLiterals( Mvc_Cover_t * pCover );
extern int              Mvc_CoverIsOneLiteral( Mvc_Cover_t * pCover );
/*=== mvcOpAlg.c ====================================================*/
extern Mvc_Cover_t *    Mvc_CoverAlgebraicMultiply( Mvc_Cover_t * pCover1, Mvc_Cover_t * pCover2 );
extern Mvc_Cover_t *    Mvc_CoverAlgebraicSubtract( Mvc_Cover_t * pCover1, Mvc_Cover_t * pCover2 );
extern int              Mvc_CoverAlgebraicEqual( Mvc_Cover_t * pCover1, Mvc_Cover_t * pCover2 );
/*=== mvcOpBool.c ====================================================*/
extern Mvc_Cover_t *    Mvc_CoverBooleanOr( Mvc_Cover_t * pCover1, Mvc_Cover_t * pCover2 );
extern Mvc_Cover_t *    Mvc_CoverBooleanAnd( Mvc_Data_t * p, Mvc_Cover_t * pCover1, Mvc_Cover_t * pCover2 );
extern int              Mvc_CoverBooleanEqual( Mvc_Data_t * p, Mvc_Cover_t * pCover1, Mvc_Cover_t * pCover2 );

/*=== mvcContain.c ====================================================*/
extern int              Mvc_CoverContain( Mvc_Cover_t * pCover );
/*=== mvcTau.c ====================================================*/
extern int              Mvc_CoverTautology( Mvc_Data_t * p, Mvc_Cover_t * pCover );
/*=== mvcCompl.c ====================================================*/
extern Mvc_Cover_t *    Mvc_CoverComplement( Mvc_Data_t * p, Mvc_Cover_t * pCover );
/*=== mvcSharp.c ====================================================*/
extern Mvc_Cover_t *    Mvc_CoverSharp( Mvc_Data_t * p, Mvc_Cover_t * pA, Mvc_Cover_t * pB );
extern int              Mvc_CoverDist0Cubes( Mvc_Data_t * pData, Mvc_Cube_t * pA, Mvc_Cube_t * pB );
extern void             Mvc_CoverIntersectCubes( Mvc_Data_t * pData, Mvc_Cover_t * pC1, Mvc_Cover_t * pC2 );
extern int              Mvc_CoverIsIntersecting( Mvc_Data_t * pData, Mvc_Cover_t * pC1, Mvc_Cover_t * pC2 );
extern void             Mvc_CoverAppendCubes( Mvc_Cover_t * pC1, Mvc_Cover_t * pC2 );
extern void             Mvc_CoverCopyAndAppendCubes( Mvc_Cover_t * pC1, Mvc_Cover_t * pC2 );
extern void             Mvc_CoverRemoveCubes( Mvc_Cover_t * pC );

/*=== mvcReshape.c ====================================================*/
extern void             Mvc_CoverMinimizeByReshape( Mvc_Data_t * pData, Mvc_Cover_t * pCover );

/*=== mvcMerge.c ====================================================*/
extern void             Mvc_CoverDist1Merge( Mvc_Data_t * p, Mvc_Cover_t * pCover );

/*=== mvcData.c ====================================================*/
//extern Mvc_Data_t *     Mvc_CoverDataAlloc( Vm_VarMap_t * pVm, Mvc_Cover_t * pCover );
//extern void             Mvc_CoverDataFree( Mvc_Data_t * p, Mvc_Cover_t * pCover );

/*=== mvcMan.c ====================================================*/
extern void             Mvc_ManagerFree( Mvc_Manager_t * p );
extern Mvc_Manager_t *  Mvc_ManagerStart();
extern Mvc_Manager_t *  Mvc_ManagerAllocCover();
extern Mvc_Manager_t *  Mvc_ManagerAllocCube( int nWords );
extern Mvc_Manager_t *  Mvc_ManagerFreeCover( Mvc_Cover_t * pCover );
extern Mvc_Manager_t *  Mvc_ManagerFreeCube( Mvc_Cover_t * pCube, int nWords );



ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

