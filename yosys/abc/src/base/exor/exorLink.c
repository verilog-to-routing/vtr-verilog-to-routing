/**CFile****************************************************************

  FileName    [exorLink.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Exclusive sum-of-product minimization.]

  Synopsis    [Cube iterators.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: exorLink.c,v 1.0 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                  Implementation of EXORCISM - 4                  ///
///              An Exclusive Sum-of-Product Minimizer               ///
///                                                                  ///
///               Alan Mishchenko  <alanmi@ee.pdx.edu>               ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                 Generation of ExorLinked Cubes                   ///
///                                                                  ///
///  Ver. 1.0. Started - July 26, 2000. Last update - July 29, 2000  ///
///  Ver. 1.4. Started -  Aug 10, 2000. Last update -  Aug 12, 2000  ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///   This software was tested with the BDD package "CUDD", v.2.3.0  ///
///                          by Fabio Somenzi                        ///
///                  http://vlsi.colorado.edu/~fabio/                ///
////////////////////////////////////////////////////////////////////////

#include "exor.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

#define LARGE_NUM 1000000

////////////////////////////////////////////////////////////////////////
///                 EXTERNAL FUNCTION DECLARATIONS                   ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTIONS OF THIS MODULE                      ///
////////////////////////////////////////////////////////////////////////

int ExorLinkCubeIteratorStart( Cube** pGroup, Cube* pC1, Cube* pC2, cubedist Dist );
// this function starts the Exor-Link iterator, which iterates
// through the cube groups starting from the group with min literals
// returns 1 on success, returns 0 if the cubes have wrong distance

int ExorLinkCubeIteratorNext( Cube** pGroup );
// give the next group in the decreasing order of sum of literals
// returns 1 on success, returns 0 if there are no more groups

int ExorLinkCubeIteratorPick( Cube** pGroup, int g );
// gives the group #g in the order in which the groups were given
// during iteration
// returns 1 on success, returns 0 if something g is too large

void ExorLinkCubeIteratorCleanUp( int fTakeLastGroup );
// removes the cubes from the store back into the list of free cubes
// if fTakeLastGroup is 0, removes all cubes
// if fTakeLastGroup is 1, does not store the last group

////////////////////////////////////////////////////////////////////////
///                       EXTERNAL VARIABLES                         ///
////////////////////////////////////////////////////////////////////////

// information about the cube cover before
extern cinfo g_CoverInfo;
// new IDs are assigned only when it is known that the cubes are useful
// this is done in ExorLinkCubeIteratorCleanUp();

// the head of the list of free cubes
extern Cube* g_CubesFree;

extern byte BitCount[];

////////////////////////////////////////////////////////////////////////
///                         EXORLINK INFO                            ///
////////////////////////////////////////////////////////////////////////

const int s_ELMax = 4;

// ExorLink-2: there are 4 cubes,  2 literals each, combined into 2 groups
// ExorLink-3: there are 12 cubes, 3 literals each, combined into 6 groups
// ExorLink-4: there are 32 cubes, 4 literals each, combined into 24 groups
// ExorLink-5: there are 80 cubes, 5 literals each, combined into 120 groups
// Exorlink-n: there are n*2^(n-1) cubes, n literals each, combined into n! groups
const int s_ELnCubes[4]  = { 4, 12, 32, 80 };
const int s_ELnGroups[4] = { 2,  6, 24, 120 };

// value sets of cubes X{a0}Y{b0}Z{c0}U{d0} and X{a1}Y{b1}Z{c1}U{d1}
// used to represent the ExorLink cube generation rules
enum { vs0, vs1, vsX };
// vs0 = 0, // the value set of the first cube 
// vs1 = 1, // the value set of the second cube
// vsX = 2  // EXOR of the value sets of the first and second cubes

// representation of ExorLinked cubes
static int s_ELCubeRules[3][32][4]  = {
{ // ExorLink-2 Cube Generating Rules
                       // | 0 | 1 | - sections
                       // |-------|
    {vsX,vs0}, // cube 0  |   |   |
    {vsX,vs1}, // cube 1  |   | 0 |
    {vs0,vsX}, // cube 2  |   |   |
    {vs1,vsX}  // cube 3  | 0 |   |
},
{ // ExorLink-3 Cube Generating Rules
                            // | 0 | 1 | 2 | - sections
                            // |-----------|
    {vsX,vs0,vs0}, // cube 0   |   |   |   |
    {vsX,vs0,vs1}, // cube 1   |   |   | 0 |
    {vsX,vs1,vs0}, // cube 2   |   | 0 |   |
    {vsX,vs1,vs1}, // cube 3   |   | 1 | 1 |

    {vs0,vsX,vs0}, // cube 4   |   |   |   |
    {vs0,vsX,vs1}, // cube 5   |   |   | 2 |
    {vs1,vsX,vs0}, // cube 6   | 0 |   |   |
    {vs1,vsX,vs1}, // cube 7   | 1 |   | 3 |

    {vs0,vs0,vsX}, // cube 8   |   |   |   |
    {vs0,vs1,vsX}, // cube 9   |   | 2 |   |
    {vs1,vs0,vsX}, // cube 10  | 2 |   |   |
    {vs1,vs1,vsX}  // cube 11  | 3 | 3 |   |
},
{ // ExorLink-4 Rules Generating Rules
                                // | 0 | 1 | 2 | 4 | - sections
                                // |---------------|
    {vsX,vs0,vs0,vs0}, // cube 0   |   |   |   |   |
    {vsX,vs0,vs0,vs1}, // cube 1   |   |   |   | 0 |
    {vsX,vs0,vs1,vs0}, // cube 2   |   |   | 0 |   |
    {vsX,vs0,vs1,vs1}, // cube 3   |   |   | 1 | 1 |
    {vsX,vs1,vs0,vs0}, // cube 4   |   | 0 |   |   |
    {vsX,vs1,vs0,vs1}, // cube 5   |   | 1 |   | 2 |
    {vsX,vs1,vs1,vs0}, // cube 6   |   | 2 | 2 |   |
    {vsX,vs1,vs1,vs1}, // cube 7   |   | 3 | 3 | 3 |

    {vs0,vsX,vs0,vs0}, // cube 8   |   |   |   |   |
    {vs0,vsX,vs0,vs1}, // cube 9   |   |   |   | 4 |
    {vs0,vsX,vs1,vs0}, // cube 10  |   |   | 4 |   |
    {vs0,vsX,vs1,vs1}, // cube 11  |   |   | 5 | 5 |
    {vs1,vsX,vs0,vs0}, // cube 12  | 0 |   |   |   |
    {vs1,vsX,vs0,vs1}, // cube 13  | 1 |   |   | 6 |
    {vs1,vsX,vs1,vs0}, // cube 14  | 2 |   | 6 |   |
    {vs1,vsX,vs1,vs1}, // cube 15  | 3 |   | 7 | 7 |

    {vs0,vs0,vsX,vs0}, // cube 16  |   |   |   |   |
    {vs0,vs0,vsX,vs1}, // cube 17  |   |   |   | 8 |
    {vs0,vs1,vsX,vs0}, // cube 18  |   | 4 |   |   |
    {vs0,vs1,vsX,vs1}, // cube 19  |   | 5 |   | 9 |
    {vs1,vs0,vsX,vs0}, // cube 20  | 4 |   |   |   |
    {vs1,vs0,vsX,vs1}, // cube 21  | 5 |   |   | 10|
    {vs1,vs1,vsX,vs0}, // cube 22  | 6 | 6 |   |   |
    {vs1,vs1,vsX,vs1}, // cube 23  | 7 | 7 |   | 11|

    {vs0,vs0,vs0,vsX}, // cube 24  |   |   |   |   |
    {vs0,vs0,vs1,vsX}, // cube 25  |   |   | 8 |   |
    {vs0,vs1,vs0,vsX}, // cube 26  |   | 8 |   |   |
    {vs0,vs1,vs1,vsX}, // cube 27  |   | 9 | 9 |   |
    {vs1,vs0,vs0,vsX}, // cube 28  | 8 |   |   |   |
    {vs1,vs0,vs1,vsX}, // cube 29  | 9 |   | 10|   |
    {vs1,vs1,vs0,vsX}, // cube 30  | 10| 10|   |   |
    {vs1,vs1,vs1,vsX}  // cube 31  | 11| 11| 11|   |
}
};

// these cubes are combined into groups
static int s_ELGroupRules[3][24][4] = { 
{ // ExorLink-2 Group Forming Rules
    {0,3},  // group 0 - section 0
    {2,1}   // group 1 - section 1
},
{ // ExorLink-3 Group Forming Rules
    {0,6,11}, // group 0 - section 0 
    {0,7,10}, // group 1
    {4,2,11}, // group 2 - section 1 
    {4,3,9},  // group 3
    {8,1,7},  // group 4 - section 2 
    {8,3,5}   // group 5
},
{ // ExorLink-4 Group Forming Rules 
// section 0: (0-12)(1-13)(2-14)(3-15)(4-20)(5-21)(6-22)(7-23)(8-28)(9-29)(10-30)(11-31)
    {0,12,22,31},  // group 0       // {0,6,11}, // group 0  - section 0
    {0,12,23,30},  // group 1       // {0,7,10}, // group 1
    {0,20,14,31},  // group 2       // {4,2,11}, // group 2
    {0,20,15,29},  // group 3       // {4,3,9},  // group 3
    {0,28,13,23},  // group 4       // {8,1,7},  // group 4
    {0,28,15,21},  // group 5       // {8,3,5}   // group 5
// section 1: (0-4)(1-5)(2-6)(3-7)(4-18)(5-19)(6-22)(7-23)(8-26)(9-27)(10-30)(11-31)
    {8,4,22,31},   // group 6
    {8,4,23,30},   // group 7
    {8,18,6,31},   // group 8
    {8,18,7,27},   // group 9
    {8,26,5,23},   // group 10
    {8,26,7,19},   // group 11
// section 2: (0-2)(1-3)(2-6)(3-7)(4-10)(5-11)(6-14)(7-15)(8-25)(9-27)(10-29)(11-31)
    {16,2,14,31},  // group 12
    {16,2,15,29},  // group 13
    {16,10,6,31},  // group 14
    {16,10,7,27},  // group 15
    {16,25,3,15},  // group 16
    {16,25,7,11},  // group 17
// section 3: (0-1)(1-3)(2-5)(3-7)(4-9)(5-11)(6-13)(7-15)(8-17)(9-19)(10-21)(11-23)
    {24,1,13,23},  // group 18
    {24,1,15,21},  // group 19
    {24,9, 5,23},  // group 20
    {24,9, 7,19},  // group 21
    {24,17,3,15},  // group 22
    {24,17,7,11}   // group 23
}
};

// it is assumed that if literals in the first cube, second cube 
// and their EXOR are 0 or 1 (as opposed to -), they are written
// into a mask, which is used to count the number of literals in 
// the cube groups cubes
//
// below is the set of masks selecting literals belonging
// to the given cube of the group

static drow s_CubeLitMasks[3][32] = { 
{  // ExorLink-2 Literal Counting Masks
//                      v3   v2   v1   v0
//                     -xBA -xBA -xBA -xBA  
//                     -------------------
    0x14,  // cube 0  <0000 0000 0001 0100> {vsX,vs0}
    0x24,  // cube 1  <0000 0000 0010 0100> {vsX,vs1}
    0x41,  // cube 2  <0000 0000 0100 0001> {vs0,vsX}
    0x42,  // cube 3  <0000 0000 0100 0010> {vs1,vsX}
},
{  // ExorLink-3 Literal Counting Masks
    0x114, // cube 0  <0000 0001 0001 0100> {vsX,vs0,vs0}
    0x214, // cube 1  <0000 0010 0001 0100> {vsX,vs0,vs1}
    0x124, // cube 2  <0000 0001 0010 0100> {vsX,vs1,vs0}
    0x224, // cube 3  <0000 0010 0010 0100> {vsX,vs1,vs1}
    0x141, // cube 4  <0000 0001 0100 0001> {vs0,vsX,vs0}
    0x241, // cube 5  <0000 0010 0100 0001> {vs0,vsX,vs1}
    0x142, // cube 6  <0000 0001 0100 0010> {vs1,vsX,vs0}
    0x242, // cube 7  <0000 0010 0100 0010> {vs1,vsX,vs1}
    0x411, // cube 8  <0000 0100 0001 0001> {vs0,vs0,vsX}
    0x421, // cube 9  <0000 0100 0010 0001> {vs0,vs1,vsX}
    0x412, // cube 10 <0000 0100 0001 0010> {vs1,vs0,vsX}
    0x422, // cube 11 <0000 0100 0010 0010> {vs1,vs1,vsX}
},
{  // ExorLink-4 Literal Counting Masks
    0x1114, // cube 0   <0001 0001 0001 0100> {vsX,vs0,vs0,vs0}
    0x2114, // cube 1   <0010 0001 0001 0100> {vsX,vs0,vs0,vs1}
    0x1214, // cube 2   <0001 0010 0001 0100> {vsX,vs0,vs1,vs0}
    0x2214, // cube 3   <0010 0010 0001 0100> {vsX,vs0,vs1,vs1}
    0x1124, // cube 4   <0001 0001 0010 0100> {vsX,vs1,vs0,vs0}
    0x2124, // cube 5   <0010 0001 0010 0100> {vsX,vs1,vs0,vs1}
    0x1224, // cube 6   <0001 0010 0010 0100> {vsX,vs1,vs1,vs0}
    0x2224, // cube 7   <0010 0010 0010 0100> {vsX,vs1,vs1,vs1}
    0x1141, // cube 8   <0001 0001 0100 0001> {vs0,vsX,vs0,vs0}
    0x2141, // cube 9   <0010 0001 0100 0001> {vs0,vsX,vs0,vs1}
    0x1241, // cube 10  <0001 0010 0100 0001> {vs0,vsX,vs1,vs0}
    0x2241, // cube 11  <0010 0010 0100 0001> {vs0,vsX,vs1,vs1}
    0x1142, // cube 12  <0001 0001 0100 0010> {vs1,vsX,vs0,vs0}
    0x2142, // cube 13  <0010 0001 0100 0010> {vs1,vsX,vs0,vs1}
    0x1242, // cube 14  <0001 0010 0100 0010> {vs1,vsX,vs1,vs0}
    0x2242, // cube 15  <0010 0010 0100 0010> {vs1,vsX,vs1,vs1}
    0x1411, // cube 16  <0001 0100 0001 0001> {vs0,vs0,vsX,vs0}
    0x2411, // cube 17  <0010 0100 0001 0001> {vs0,vs0,vsX,vs1}
    0x1421, // cube 18  <0001 0100 0010 0001> {vs0,vs1,vsX,vs0}
    0x2421, // cube 19  <0010 0100 0010 0001> {vs0,vs1,vsX,vs1}
    0x1412, // cube 20  <0001 0100 0001 0010> {vs1,vs0,vsX,vs0}
    0x2412, // cube 21  <0010 0100 0001 0010> {vs1,vs0,vsX,vs1}
    0x1422, // cube 22  <0001 0100 0010 0010> {vs1,vs1,vsX,vs0}
    0x2422, // cube 23  <0010 0100 0010 0010> {vs1,vs1,vsX,vs1}
    0x4111, // cube 24  <0100 0001 0001 0001> {vs0,vs0,vs0,vsX}
    0x4211, // cube 25  <0100 0010 0001 0001> {vs0,vs0,vs1,vsX}
    0x4121, // cube 26  <0100 0001 0010 0001> {vs0,vs1,vs0,vsX}
    0x4221, // cube 27  <0100 0010 0010 0001> {vs0,vs1,vs1,vsX}
    0x4112, // cube 28  <0100 0001 0001 0010> {vs1,vs0,vs0,vsX}
    0x4212, // cube 29  <0100 0010 0001 0010> {vs1,vs0,vs1,vsX}
    0x4122, // cube 30  <0100 0001 0010 0010> {vs1,vs1,vs0,vsX}
    0x4222, // cube 31  <0100 0010 0010 0010> {vs1,vs1,vs1,vsX}
}
};

static drow s_BitMasks[32] = 
{
    0x00000001,0x00000002,0x00000004,0x00000008,
    0x00000010,0x00000020,0x00000040,0x00000080,
    0x00000100,0x00000200,0x00000400,0x00000800,
    0x00001000,0x00002000,0x00004000,0x00008000,
    0x00010000,0x00020000,0x00040000,0x00080000,
    0x00100000,0x00200000,0x00400000,0x00800000,
    0x01000000,0x02000000,0x04000000,0x08000000,
    0x10000000,0x20000000,0x40000000,0x80000000
};

////////////////////////////////////////////////////////////////////////
///                        STATIC VARIABLES                          ///
////////////////////////////////////////////////////////////////////////

// this flag is TRUE as long as the storage is allocated
static int fWorking;

// set these flags to have minimum literal groups generated first
static int fMinLitGroupsFirst[4] = { 0 /*dist2*/, 0 /*dist3*/, 0 /*dist4*/};

static int nDist;
static int nCubes;
static int nCubesInGroup;
static int nGroups;
static Cube *pCA, *pCB;

// storage for variable numbers that are different in the cubes
static int DiffVars[5];
static int* pDiffVars;
static int nDifferentVars;

// storage for the bits and words of different input variables
static int nDiffVarsIn;
static int DiffVarWords[5];
static int DiffVarBits[5];

// literal mask used to count the number of literals in the cubes
static drow MaskLiterals;
// the base for counting literals
static int StartingLiterals;
// the number of literals in each cube
static int CubeLiterals[32];
static int BitShift;
static int DiffVarValues[4][3];
static int Value;

// the sorted array of groups in the increasing order of costs
static int GroupCosts[32];
static int GroupCostBest;
static int GroupCostBestNum;

static int CubeNum;
static int NewZ;
static drow Temp;

// the cubes currently created
static Cube* ELCubes[32];

// the bit string with 1's corresponding to cubes in ELCubes[] 
// that constitute the last group
static drow LastGroup;

static int  GroupOrder[24];
static drow VisitedGroups;
static int  nVisitedGroups;

//int RemainderBits = (nVars*2)%(sizeof(drow)*8);
//int TotalWords    = (nVars*2)/(sizeof(drow)*8) + (RemainderBits > 0);
static drow DammyBitData[(MAXVARS*2)/(sizeof(drow)*8)+(MAXVARS*2)%(sizeof(drow)*8)];

////////////////////////////////////////////////////////////////////////
///                       FUNCTION DEFINTIONS                        ///
////////////////////////////////////////////////////////////////////////

// IDEA! if we already used a cube to count distances and it did not improve
// there is no need to try it again with other group
// (this idea works only for ExorLink-2 and -3)

int ExorLinkCubeIteratorStart( Cube** pGroup, Cube* pC1, Cube* pC2, cubedist Dist )
// this function starts the Exor-Link iterator, which iterates
// through the cube groups starting from the group with min literals
// returns 1 on success, returns 0 if the cubes have wrong distance
{
    int i, c;

    // check that everything is okey
    assert( pC1 != NULL );
    assert( pC2 != NULL );
    assert( !fWorking );

    nDist = Dist;
    nCubes = Dist + 2;
    nCubesInGroup = s_ELnCubes[nDist];
    nGroups = s_ELnGroups[Dist];
    pCA = pC1;
    pCB = pC2;
    // find what variables are different in these two cubes
    // FindDiffVars returns DiffVars[0] < 0, if the output is different
    nDifferentVars = FindDiffVars( DiffVars, pCA, pCB );
    if ( nCubes != nDifferentVars )
    {
//      cout << "ExorLinkCubeIterator(): Distance mismatch";
//      cout << " nCubes = " << nCubes << " nDiffVars = " << nDifferentVars << endl;
        fWorking = 0;
        return 0;
    }

    // copy the input variable cube data into DammyBitData[]
    for ( i = 0; i < g_CoverInfo.nWordsIn; i++ )
        DammyBitData[i] = pCA->pCubeDataIn[i];

    // find the number of different input variables
    nDiffVarsIn = ( DiffVars[0] >= 0 )? nCubes: nCubes-1;
    // assign the pointer to the place where the number of diff input vars is stored
    pDiffVars   = ( DiffVars[0] >= 0 )? DiffVars: DiffVars+1;
    // find the bit offsets and remove different variables
    for ( i = 0; i < nDiffVarsIn; i++ )
    {
        DiffVarWords[i] = ((2*pDiffVars[i]) >> LOGBPI) ;
        DiffVarBits[i]  = ((2*pDiffVars[i]) & BPIMASK);
        // clear this position
        DammyBitData[ DiffVarWords[i] ] &= ~( 3 << DiffVarBits[i] );
    }

    // extract the values from the cubes and create the mask of literals
    MaskLiterals = 0;
    // initialize the base for literal counts
    StartingLiterals = pCA->a;
    for ( i = 0, BitShift = 0; i < nDiffVarsIn; i++, BitShift++ )
    {
        DiffVarValues[i][0] = ( pCA->pCubeDataIn[DiffVarWords[i]] >> DiffVarBits[i] ) & 3;
        if ( DiffVarValues[i][0] != VAR_ABS )
        {
            MaskLiterals |= ( 1 << BitShift );
            // update the base for literal counts
            StartingLiterals--;
        }
        BitShift++;

        DiffVarValues[i][1] = ( pCB->pCubeDataIn[DiffVarWords[i]] >> DiffVarBits[i] ) & 3;
        if ( DiffVarValues[i][1] != VAR_ABS )
            MaskLiterals |= ( 1 << BitShift );
        BitShift++;

        DiffVarValues[i][2] = DiffVarValues[i][0] ^ DiffVarValues[i][1];
        if ( DiffVarValues[i][2] != VAR_ABS )
            MaskLiterals |= ( 1 << BitShift );
        BitShift++;
    }

    // count the number of additional literals in each cube of the group
    for ( i = 0; i < nCubesInGroup; i++ )
        CubeLiterals[i] = BitCount[ MaskLiterals & s_CubeLitMasks[Dist][i] ];

    // compute the costs of all groups
    for ( i = 0; i < nGroups; i++ )
        // go over all cubes in the group
        for ( GroupCosts[i] = 0, c = 0; c < nCubes; c++ )
            GroupCosts[i] += CubeLiterals[ s_ELGroupRules[Dist][i][c] ];

    // find the best cost group
    if ( fMinLitGroupsFirst[Dist] ) 
    { // find the minimum cost group
        GroupCostBest = LARGE_NUM;
        for ( i = 0; i < nGroups; i++ )
            if ( GroupCostBest > GroupCosts[i] )
            {
                GroupCostBest = GroupCosts[i];
                GroupCostBestNum = i;
            }
    }
    else 
    { // find the maximum cost group
        GroupCostBest = -1;
        for ( i = 0; i < nGroups; i++ )
            if ( GroupCostBest < GroupCosts[i] )
            {
                GroupCostBest = GroupCosts[i];
                GroupCostBestNum = i;
            }
    }

    // create the cubes with min number of literals needed for the group
    LastGroup = 0;
    for ( c = 0; c < nCubes; c++ )
    {
        CubeNum = s_ELGroupRules[Dist][GroupCostBestNum][c];
        LastGroup |= s_BitMasks[CubeNum];

        // bring a cube from the free cube list
        ELCubes[CubeNum] = GetFreeCube();

        // copy the input bit data into the cube 
        for ( i = 0; i < g_CoverInfo.nWordsIn; i++ )
            ELCubes[CubeNum]->pCubeDataIn[i] = DammyBitData[i];

        // copy the output bit data into the cube
        NewZ = 0;
        if ( DiffVars[0] >= 0 ) // the output is not involved in ExorLink
        {
            for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
                ELCubes[CubeNum]->pCubeDataOut[i] = pCA->pCubeDataOut[i];
            NewZ = pCA->z;
        }
        else // the output is involved
        { // determine where the output information comes from
            Value = s_ELCubeRules[Dist][CubeNum][nDiffVarsIn];
            if ( Value == vs0 )
                for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
                {
                    Temp = pCA->pCubeDataOut[i];
                    ELCubes[CubeNum]->pCubeDataOut[i] = Temp;
                    NewZ += BIT_COUNT(Temp);
                }
            else if ( Value == vs1 )
                for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
                {
                    Temp = pCB->pCubeDataOut[i];
                    ELCubes[CubeNum]->pCubeDataOut[i] = Temp;
                    NewZ += BIT_COUNT(Temp);
                }
            else if ( Value == vsX )
                for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
                {
                    Temp = pCA->pCubeDataOut[i] ^ pCB->pCubeDataOut[i];
                    ELCubes[CubeNum]->pCubeDataOut[i] = Temp;
                    NewZ += BIT_COUNT(Temp);
                }
        }

        // set the variables that should be there
        for ( i = 0; i < nDiffVarsIn; i++ )
        {
            Value = DiffVarValues[i][ s_ELCubeRules[Dist][CubeNum][i] ];
            ELCubes[CubeNum]->pCubeDataIn[ DiffVarWords[i] ] |= ( Value << DiffVarBits[i] );
        }

        // set the number of literals
        ELCubes[CubeNum]->a = StartingLiterals + CubeLiterals[CubeNum];
        ELCubes[CubeNum]->z = NewZ;
        ELCubes[CubeNum]->q = ComputeQCostBits( ELCubes[CubeNum] );

        // assign the ID
        ELCubes[CubeNum]->ID = g_CoverInfo.cIDs++;
        // skip through zero-ID
        if ( g_CoverInfo.cIDs == 256 )
            g_CoverInfo.cIDs = 1;

        // prepare the return array
        pGroup[c] = ELCubes[CubeNum];
    }

    // mark this group as visited 
    VisitedGroups |= s_BitMasks[ GroupCostBestNum ];
    // set the first visited group number
    GroupOrder[0] = GroupCostBestNum;
    // increment the counter of visited groups
    nVisitedGroups = 1;
    fWorking = 1;
    return 1;
}

int ExorLinkCubeIteratorNext( Cube** pGroup )
// give the next group in the decreasing order of sum of literals
// returns 1 on success, returns 0 if there are no more groups
{
    int i, c;

    // check that everything is okey
    assert( fWorking );

    if ( nVisitedGroups == nGroups ) 
    // we have iterated through all groups
        return 0;

    // find the min/max cost group
    if ( fMinLitGroupsFirst[nDist] ) 
//  if ( nCubes == 4 ) 
    { // find the minimum cost
        // go through all groups
        GroupCostBest = LARGE_NUM;
        for ( i = 0; i < nGroups; i++ )
            if ( !(VisitedGroups & s_BitMasks[i]) && GroupCostBest > GroupCosts[i] )
            {
                GroupCostBest = GroupCosts[i];
                GroupCostBestNum = i;
            }
        assert( GroupCostBest != LARGE_NUM );
    }
    else 
    { // find the maximum cost
        // go through all groups
        GroupCostBest = -1;
        for ( i = 0; i < nGroups; i++ )
            if ( !(VisitedGroups & s_BitMasks[i]) && GroupCostBest < GroupCosts[i] )
            {
                GroupCostBest = GroupCosts[i];
                GroupCostBestNum = i;
            }
        assert( GroupCostBest != -1 );
    }

    // create the cubes needed for the group, if they are not created already
    LastGroup = 0;
    for ( c = 0; c < nCubes; c++ )
    {
        CubeNum = s_ELGroupRules[nDist][GroupCostBestNum][c];
        LastGroup |= s_BitMasks[CubeNum];

        if ( ELCubes[CubeNum] == NULL ) // this cube does not exist
        {
            // bring a cube from the free cube list
            ELCubes[CubeNum] = GetFreeCube();

            // copy the input bit data into the cube 
            for ( i = 0; i < g_CoverInfo.nWordsIn; i++ )
                ELCubes[CubeNum]->pCubeDataIn[i] = DammyBitData[i];

            // copy the output bit data into the cube
            NewZ = 0;
            if ( DiffVars[0] >= 0 ) // the output is not involved in ExorLink
            {
                for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
                    ELCubes[CubeNum]->pCubeDataOut[i] = pCA->pCubeDataOut[i];
                NewZ = pCA->z;
            }
            else // the output is involved
            { // determine where the output information comes from
                Value = s_ELCubeRules[nDist][CubeNum][nDiffVarsIn];
                if ( Value == vs0 )
                    for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
                    {
                        Temp = pCA->pCubeDataOut[i];
                        ELCubes[CubeNum]->pCubeDataOut[i] = Temp;
                        NewZ += BIT_COUNT(Temp);
                    }
                else if ( Value == vs1 )
                    for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
                    {
                        Temp = pCB->pCubeDataOut[i];
                        ELCubes[CubeNum]->pCubeDataOut[i] = Temp;
                        NewZ += BIT_COUNT(Temp);
                    }
                else if ( Value == vsX )
                    for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
                    {
                        Temp = pCA->pCubeDataOut[i] ^ pCB->pCubeDataOut[i];
                        ELCubes[CubeNum]->pCubeDataOut[i] = Temp;
                        NewZ += BIT_COUNT(Temp);
                    }
            }

            // set the variables that should be there
            for ( i = 0; i < nDiffVarsIn; i++ )
            {
                Value = DiffVarValues[i][ s_ELCubeRules[nDist][CubeNum][i] ];
                ELCubes[CubeNum]->pCubeDataIn[ DiffVarWords[i] ] |= ( Value << DiffVarBits[i] );
            }

            // set the number of literals and output ones
            ELCubes[CubeNum]->a = StartingLiterals + CubeLiterals[CubeNum];
            ELCubes[CubeNum]->z = NewZ;
            ELCubes[CubeNum]->q = ComputeQCostBits( ELCubes[CubeNum] );
            assert( NewZ != 255 );

            // assign the ID
            ELCubes[CubeNum]->ID = g_CoverInfo.cIDs++;
            // skip through zero-ID
            if ( g_CoverInfo.cIDs == 256 )
                g_CoverInfo.cIDs = 1;

        }
        // prepare the return array
        pGroup[c] = ELCubes[CubeNum];
    }

    // mark this group as visited 
    VisitedGroups |= s_BitMasks[ GroupCostBestNum ];
    // set the next visited group number and
    // increment the counter of visited groups
    GroupOrder[ nVisitedGroups++ ] = GroupCostBestNum;
    return 1;
}

int ExorLinkCubeIteratorPick( Cube** pGroup, int g )
// gives the group #g in the order in which the groups were given
// during iteration
// returns 1 on success, returns 0 if something is wrong (g is too large)
{
    int GroupNum, c;

    assert( fWorking );
    assert( g >= 0 && g < nGroups );
    assert( VisitedGroups & s_BitMasks[g] );

    GroupNum = GroupOrder[g];
    // form the group
    LastGroup = 0;
    for ( c = 0; c < nCubes; c++ )
    {
        CubeNum = s_ELGroupRules[nDist][GroupNum][c];

        // remember this group as the last one
        LastGroup |= s_BitMasks[CubeNum];

        assert( ELCubes[CubeNum] != NULL ); // this cube should exist
        // prepare the return array
        pGroup[c] = ELCubes[CubeNum];
    }
    return 1;
}

void ExorLinkCubeIteratorCleanUp( int fTakeLastGroup )
// removes the cubes from the store back into the list of free cubes
// if fTakeLastGroup is 0, removes all cubes
// if fTakeLastGroup is 1, does not store the last group
{
    int c;
    assert( fWorking );

    // put cubes back
    // set the cube pointers to zero
    if ( fTakeLastGroup == 0 )
        for ( c = 0; c < nCubesInGroup; c++ )
        {
            ELCubes[c]->fMark = 0;
            AddToFreeCubes( ELCubes[c] );
            ELCubes[c] = NULL;
        }
    else
        for ( c = 0; c < nCubesInGroup; c++ )
        if ( ELCubes[c] )
        {
            ELCubes[c]->fMark = 0;
            if ( (LastGroup & s_BitMasks[c]) == 0 ) // does not belong to the last group
                AddToFreeCubes( ELCubes[c] );
            ELCubes[c] = NULL;
        }

    // set the cube groups to zero
    VisitedGroups = 0;
    // shut down the iterator
    fWorking = 0;
}

    
///////////////////////////////////////////////////////////////////
////////////              End of File             /////////////////
///////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
