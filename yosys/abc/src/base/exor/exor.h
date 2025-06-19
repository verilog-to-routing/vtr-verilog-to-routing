/**CFile****************************************************************

  FileName    [exor.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Exclusive sum-of-product minimization.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: exor.h,v 1.0 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                    Interface of EXORCISM - 4                     ///
///              An Exclusive Sum-of-Product Minimizer               ///
///               Alan Mishchenko  <alanmi@ee.pdx.edu>               ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                         Main Module                              ///
///                                                                  ///
///  Ver. 1.0. Started - July 15, 2000. Last update - July 20, 2000  ///
///  Ver. 1.4. Started - Aug 10, 2000.  Last update - Aug 10, 2000   ///
///  Ver. 1.7. Started - Sep 20, 2000.  Last update - Sep 23, 2000   ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///   This software was tested with the BDD package "CUDD", v.2.3.0  ///
///                          by Fabio Somenzi                        ///
///                  http://vlsi.colorado.edu/~fabio/                ///
////////////////////////////////////////////////////////////////////////

#ifndef __EXORMAIN_H__
#define __EXORMAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "misc/vec/vec.h"
#include "misc/vec/vecWec.h"

ABC_NAMESPACE_HEADER_START 

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

enum {
  // the number of bits per integer
  BPI = 32,
  BPIMASK = 31,
  LOGBPI = 5,

  // the maximum number of input variables
  MAXVARS = 1000,

  // the number of cubes that are allocated additionally
  ADDITIONAL_CUBES = 33,

  // the factor showing how many cube pairs will be allocated
  CUBE_PAIR_FACTOR = 20,
  // the following number of cube pairs are allocated:
  // nCubesAlloc/CUBE_PAIR_FACTOR

  DIFFERENT = 0x55555555,
};

extern unsigned char BitCount[];

static inline int BIT_COUNT(int w) { return BitCount[(w)&0xffff] + BitCount[(w)>>16]; }

static inline int VarWord(int element) { return element>>LOGBPI; }
static inline int VarBit(int element) { return element&BPIMASK; }

static inline float TICKS_TO_SECONDS(abctime time) { return (float)time/(float)CLOCKS_PER_SEC; }

////////////////////////////////////////////////////////////////////////
///                  CUBE COVER and CUBE typedefs                    ///
////////////////////////////////////////////////////////////////////////

typedef enum { MULTI_OUTPUT = 1, SINGLE_NODE, MULTI_NODE  } type;

// infomation about the cover
typedef struct cinfo_tag 
{
    int nVarsIn;        // number of input binary variables in the cubes
    int nVarsOut;       // number of output binary variables in the cubes
    int nWordsIn;       // number of input words used to represent the cover
    int nWordsOut;      // number of output words used to represent the cover
    int nCubesAlloc;    // the number of allocated cubes 
    int nCubesBefore;   // number of cubes before simplification
    int nCubesInUse;    // number of cubes after simplification
    int nCubesFree;     // number of free cubes
    int nLiteralsBefore;// number of literals before
    int nLiteralsAfter; // number of literals after
    int QCostBefore;    // q-cost before
    int QCostAfter;     // q-cost after
    int cIDs;           // the counter of cube IDs

    int Verbosity;      // verbosity level
    int Quality;        // quality
    int nCubesMax;      // maximum number of cubes in starting cover
    int fUseQCost;      // use q-cost instead of literal count

    abctime TimeRead;   // reading time
    abctime TimeStart;  // starting cover computation time
    abctime TimeMin;    // pure minimization time
} cinfo;

// representation of one cube (24 bytes + bit info)
typedef unsigned int  drow;
typedef unsigned char byte;
typedef struct cube 
{
    byte  fMark;        // the flag which is TRUE if the cubes is enabled
    byte  ID;           // (almost) unique ID of the cube
    short a;            // the number of literals
    short z;            // the number of 1's in the output part
    short q;            // user cost
    drow* pCubeDataIn;  // a pointer to the bit string representing literals
    drow* pCubeDataOut; // a pointer to the bit string representing literals
    struct cube* Prev;  // pointers to the previous/next cubes in the list/ring 
    struct cube* Next;
} Cube;


// preparation
extern void PrepareBitSetModule();
extern int WriteResultIntoFile( char * pFileName );

// iterative ExorLink
extern int IterativelyApplyExorLink2( char fDistEnable );   
extern int IterativelyApplyExorLink3( char fDistEnable );   
extern int IterativelyApplyExorLink4( char fDistEnable );   

// cube storage allocation/delocation
extern int AllocateCubeSets( int nVarsIn, int nVarsOut );
extern void DelocateCubeSets();

// adjacency queque allocation/delocation procedures
extern int AllocateQueques( int nPlaces );
extern void DelocateQueques();

extern int AllocateCover( int nCubes, int nWordsIn, int nWordsOut );
extern void DelocateCover();

extern void AddToFreeCubes( Cube* pC );
extern Cube* GetFreeCube();
// getting and returning free cubes to the heap

extern void InsertVarsWithoutClearing( Cube * pC, int * pVars, int nVarsIn, int * pVarValues, int Output );
extern int CheckForCloseCubes( Cube* p, int fAddCube );

extern int FindDiffVars( int *pDiffVars, Cube* pC1, Cube* pC2 );
// determines the variables that are different in cubes pC1 and pC2
// returns the number of variables

extern int ComputeQCost( Vec_Int_t * vCube );
extern int ComputeQCostBits( Cube * p );

extern int CountLiterals();
extern int CountQCost();

////////////////////////////////////////////////////////////////////////
///              VARVALUE and CUBEDIST enum typedefs                 ///
////////////////////////////////////////////////////////////////////////

// literal values
typedef enum { VAR_NEG = 1, VAR_POS, VAR_ABS  } varvalue;

// the flag in some function calls can take one of the follwing values
typedef enum { DIST2, DIST3, DIST4 } cubedist;

ABC_NAMESPACE_HEADER_END

#endif
