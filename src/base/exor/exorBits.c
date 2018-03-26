/**CFile****************************************************************

  FileName    [exorBits.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Exclusive sum-of-product minimization.]

  Synopsis    [Bit-level procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: exorBits.c,v 1.0 2005/06/20 00:00:00 alanmi Exp $]

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
///              EXOR-Oriented Bit String Manipulation               ///
///                                                                  ///
///  Ver. 1.0. Started - July 18, 2000. Last update - July 20, 2000  ///
///  Ver. 1.4. Started -  Aug 10, 2000. Last update -  Aug 10, 2000  ///
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

////////////////////////////////////////////////////////////////////////
///                       EXTERNAL VARIABLES                         ///
////////////////////////////////////////////////////////////////////////

// information about the cube cover
// the number of cubes is constantly updated when the cube cover is processed
// in this module, only the number of variables (nVarsIn) and integers (nWordsIn)
// is used, which do not change
extern cinfo g_CoverInfo;

////////////////////////////////////////////////////////////////////////
///                  FUNCTIONS OF THIS MODULE                        ///
////////////////////////////////////////////////////////////////////////

int GetDistance( Cube * pC1, Cube * pC2 );
// return the distance between two cubes
int GetDistancePlus( Cube * pC1, Cube * pC2 );

int FindDiffVars( int * pDiffVars, Cube * pC1, Cube * pC2 );
// determine different variables in cubes from pCubes[] and writes them into pDiffVars
// returns the number of different variables

void InsertVars( Cube * pC, int * pVars, int nVarsIn, int * pVarValues );

//inline int VarWord( int element );
//inline int VarBit( int element );
varvalue GetVar( Cube * pC, int Var );

void ExorVar( Cube * pC, int Var, varvalue Val );

////////////////////////////////////////////////////////////////////////
///                        STATIC VARIABLES                          ///
////////////////////////////////////////////////////////////////////////

// the bit count for the first 256 integer numbers
static unsigned char BitCount8[256] = {
    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

static int SparseNumbers[163] = { 
    0,1,4,5,16,17,20,21,64,65,68,69,80,81,84,85,256,257,260,
    261,272,273,276,277,320,321,324,325,336,337,340,1024,1025,
    1028,1029,1040,1041,1044,1045,1088,1089,1092,1093,1104,1105,
    1108,1280,1281,1284,1285,1296,1297,1300,1344,1345,1348,1360,
    4096,4097,4100,4101,4112,4113,4116,4117,4160,4161,4164,4165,
    4176,4177,4180,4352,4353,4356,4357,4368,4369,4372,4416,4417,
    4420,4432,5120,5121,5124,5125,5136,5137,5140,5184,5185,5188,
    5200,5376,5377,5380,5392,5440,16384,16385,16388,16389,16400,
    16401,16404,16405,16448,16449,16452,16453,16464,16465,16468,
    16640,16641,16644,16645,16656,16657,16660,16704,16705,16708,
    16720,17408,17409,17412,17413,17424,17425,17428,17472,17473,
    17476,17488,17664,17665,17668,17680,17728,20480,20481,20484,
    20485,20496,20497,20500,20544,20545,20548,20560,20736,20737,
    20740,20752,20800,21504,21505,21508,21520,21568,21760 
};

static unsigned char GroupLiterals[163][4] = {
    {0}, {0}, {1}, {0,1}, {2}, {0,2}, {1,2}, {0,1,2}, {3}, {0,3},
    {1,3}, {0,1,3}, {2,3}, {0,2,3}, {1,2,3}, {0,1,2,3}, {4}, {0,4},
    {1,4}, {0,1,4}, {2,4}, {0,2,4}, {1,2,4}, {0,1,2,4}, {3,4},
    {0,3,4}, {1,3,4}, {0,1,3,4}, {2,3,4}, {0,2,3,4}, {1,2,3,4}, {5},
    {0,5}, {1,5}, {0,1,5}, {2,5}, {0,2,5}, {1,2,5}, {0,1,2,5}, {3,5},
    {0,3,5}, {1,3,5}, {0,1,3,5}, {2,3,5}, {0,2,3,5}, {1,2,3,5},
    {4,5}, {0,4,5}, {1,4,5}, {0,1,4,5}, {2,4,5}, {0,2,4,5},
    {1,2,4,5}, {3,4,5}, {0,3,4,5}, {1,3,4,5}, {2,3,4,5}, {6}, {0,6},
    {1,6}, {0,1,6}, {2,6}, {0,2,6}, {1,2,6}, {0,1,2,6}, {3,6},
    {0,3,6}, {1,3,6}, {0,1,3,6}, {2,3,6}, {0,2,3,6}, {1,2,3,6},
    {4,6}, {0,4,6}, {1,4,6}, {0,1,4,6}, {2,4,6}, {0,2,4,6},
    {1,2,4,6}, {3,4,6}, {0,3,4,6}, {1,3,4,6}, {2,3,4,6}, {5,6},
    {0,5,6}, {1,5,6}, {0,1,5,6}, {2,5,6}, {0,2,5,6}, {1,2,5,6},
    {3,5,6}, {0,3,5,6}, {1,3,5,6}, {2,3,5,6}, {4,5,6}, {0,4,5,6},
    {1,4,5,6}, {2,4,5,6}, {3,4,5,6}, {7}, {0,7}, {1,7}, {0,1,7},
    {2,7}, {0,2,7}, {1,2,7}, {0,1,2,7}, {3,7}, {0,3,7}, {1,3,7},
    {0,1,3,7}, {2,3,7}, {0,2,3,7}, {1,2,3,7}, {4,7}, {0,4,7},
    {1,4,7}, {0,1,4,7}, {2,4,7}, {0,2,4,7}, {1,2,4,7}, {3,4,7},
    {0,3,4,7}, {1,3,4,7}, {2,3,4,7}, {5,7}, {0,5,7}, {1,5,7},
    {0,1,5,7}, {2,5,7}, {0,2,5,7}, {1,2,5,7}, {3,5,7}, {0,3,5,7},
    {1,3,5,7}, {2,3,5,7}, {4,5,7}, {0,4,5,7}, {1,4,5,7}, {2,4,5,7},
    {3,4,5,7}, {6,7}, {0,6,7}, {1,6,7}, {0,1,6,7}, {2,6,7},
    {0,2,6,7}, {1,2,6,7}, {3,6,7}, {0,3,6,7}, {1,3,6,7}, {2,3,6,7},
    {4,6,7}, {0,4,6,7}, {1,4,6,7}, {2,4,6,7}, {3,4,6,7}, {5,6,7},
    {0,5,6,7}, {1,5,6,7}, {2,5,6,7}, {3,5,6,7}, {4,5,6,7} 
};

// the bit count to 16-bit numbers
#define FULL16BITS  0x10000
#define MARKNUMBER  200

static unsigned char BitGroupNumbers[FULL16BITS];
unsigned char BitCount[FULL16BITS];

////////////////////////////////////////////////////////////////////////
///                      FUNCTION DEFINITIONS                        ///
////////////////////////////////////////////////////////////////////////

void PrepareBitSetModule()
// this function should be called before anything is done with the cube cover
{   
    // prepare bit count
    int i, k;
    int nLimit;
    
    nLimit = FULL16BITS;
    for ( i = 0; i < nLimit; i++ )
    {
        BitCount[i] = BitCount8[ i & 0xff ] + BitCount8[ i>>8 ];
        BitGroupNumbers[i] = MARKNUMBER;
    }
    // prepare bit groups
    for ( k = 0; k < 163; k++ )
        BitGroupNumbers[ SparseNumbers[k] ] = k;
/*
    // verify bit groups
    int n = 4368;
    char Buff[100];
    cout << "The number is " << n << endl;
    cout << "The binary is " << itoa(n,Buff,2) << endl;
    cout << "BitGroupNumbers[n] is " << (int)BitGroupNumbers[n] << endl;
    cout << "The group literals are ";
    for ( int g = 0; g < 4; g++ )
        cout << " " << (int)GroupLiterals[BitGroupNumbers[n]][g];
*/
}

////////////////////////////////////////////////////////////////////////
///                   INLINE FUNCTION DEFINITIONS                    ///
////////////////////////////////////////////////////////////////////////
/*
int VarWord( int element )
{
    return ( element >> LOGBPI );
}

int VarBit( int element )
{
    return ( element & BPIMASK );
}
*/

varvalue GetVar( Cube * pC, int Var )
// returns VAR_NEG if var is neg, VAR_POS if var is pos, VAR_ABS if var is absent
{
    int Bit = (Var<<1);
    int Value = ((pC->pCubeDataIn[VarWord(Bit)] >> VarBit(Bit)) & 3);
    assert( Value == VAR_NEG || Value == VAR_POS || Value == VAR_ABS );
    return (varvalue)Value;
}

void ExorVar( Cube * pC, int Var, varvalue Val )
// EXORs the value Val with the value of variable Var in the given cube
// ((cube[VAR_WORD((v)<<1)]) ^ ( (pol)<<VAR_BIT((v)<<1) ))
{
    int Bit = (Var<<1);
    pC->pCubeDataIn[VarWord(Bit)] ^= ( Val << VarBit(Bit) );
}

////////////////////////////////////////////////////////////////////////
///                      FUNCTION DEFINITIONS                        ///
////////////////////////////////////////////////////////////////////////

static int DiffVarCounter, cVars;
static drow Temp1, Temp2, Temp;
static drow LastNonZeroWord;
static int LastNonZeroWordNum;

int GetDistance( Cube * pC1, Cube * pC2 )
// finds and returns the distance between two cubes pC1 and pC2
{
    int i;
    DiffVarCounter = 0;

    for ( i = 0; i < g_CoverInfo.nWordsIn; i++ )
    {
        Temp1 = pC1->pCubeDataIn[i] ^ pC2->pCubeDataIn[i];
        Temp2 = (Temp1|(Temp1>>1)) & DIFFERENT;

        // count how many bits are one in this var difference
        DiffVarCounter  += BIT_COUNT(Temp2);
        if ( DiffVarCounter > 4 )
            return 5;
    }
    // check whether the output parts are different
    for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
        if ( pC1->pCubeDataOut[i] ^ pC2->pCubeDataOut[i] )
        {
            DiffVarCounter++;
            break;
        }
    return DiffVarCounter;
}

// place to put the number of the different variable and its value in the second cube
extern int s_DiffVarNum;
extern int s_DiffVarValueP_old;
extern int s_DiffVarValueP_new;
extern int s_DiffVarValueQ;

int GetDistancePlus( Cube * pC1, Cube * pC2 )
// finds and returns the distance between two cubes pC1 and pC2
// if the distance is 1, returns the number of diff variable in VarNum
{
    int i;

    DiffVarCounter = 0;
    LastNonZeroWordNum = -1;
    for ( i = 0; i < g_CoverInfo.nWordsIn; i++ )
    {
        Temp1 = pC1->pCubeDataIn[i] ^ pC2->pCubeDataIn[i];
        Temp2 = (Temp1|(Temp1>>1)) & DIFFERENT;

        // save the value of var difference, in case 
        // the distance is one and we need to return the var number
        if ( Temp2 )
        {
            LastNonZeroWordNum = i;
            LastNonZeroWord = Temp2;
        }

        // count how many bits are one in this var difference
        DiffVarCounter  += BIT_COUNT(Temp2);
        if ( DiffVarCounter > 4 )
            return 5;
    }
    // check whether the output parts are different
    for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
        if ( pC1->pCubeDataOut[i] ^ pC2->pCubeDataOut[i] )
        {
            DiffVarCounter++;
            break;
        }

    if ( DiffVarCounter == 1 )
    {
        if ( LastNonZeroWordNum == -1 ) // the output is the only different variable
            s_DiffVarNum = -1;
        else
        {
            Temp = (LastNonZeroWord>>2);
            for ( i = 0; Temp; Temp>>=2, i++ );
            s_DiffVarNum = LastNonZeroWordNum*BPI/2 + i;

            // save the old var value
            s_DiffVarValueP_old = GetVar( pC1, s_DiffVarNum );
            s_DiffVarValueQ = GetVar( pC2, s_DiffVarNum );

            // EXOR this value with the corresponding value in p cube           
            ExorVar( pC1, s_DiffVarNum, (varvalue)s_DiffVarValueQ );

            s_DiffVarValueP_new = GetVar( pC1, s_DiffVarNum );
        }
    }

    return DiffVarCounter;
}

int FindDiffVars( int * pDiffVars, Cube * pC1, Cube * pC2 )
// determine different variables in two cubes and 
// writes them into pDiffVars[]
// -1 is written into pDiffVars[0] if the cubes have different outputs
// returns the number of different variables (including the output)
{
    int i, v;
    DiffVarCounter = 0;
    // check whether the output parts of the cubes are different

    for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
        if ( pC1->pCubeDataOut[i] != pC2->pCubeDataOut[i] )
        { // they are different
            pDiffVars[0] = -1;
            DiffVarCounter = 1;
            break;
        }

    for ( i = 0; i < g_CoverInfo.nWordsIn; i++ )
    {

        Temp1 = pC1->pCubeDataIn[i] ^ pC2->pCubeDataIn[i];
        Temp2 = (Temp1|(Temp1>>1)) & DIFFERENT;

        // check the first part of this word
        Temp = Temp2 & 0xffff;
        cVars = BitCount[ Temp ];
        if ( cVars )
        {
            if ( cVars < 5 )
                for ( v = 0; v < cVars; v++ )
                {
                    assert( BitGroupNumbers[Temp] != MARKNUMBER );
                    pDiffVars[ DiffVarCounter++ ] = i*16 + GroupLiterals[ BitGroupNumbers[Temp] ][v];
                }
            else
                return 5;
        }
        if ( DiffVarCounter > 4 )
            return 5;

        // check the second part of this word
        Temp = Temp2 >> 16;
        cVars = BitCount[ Temp ];
        if ( cVars )
        {
            if ( cVars < 5 )
                for ( v = 0; v < cVars; v++ )
                {
                    assert( BitGroupNumbers[Temp] != MARKNUMBER );
                    pDiffVars[ DiffVarCounter++ ] = i*16 + 8 + GroupLiterals[ BitGroupNumbers[Temp] ][v];
                }
            else
                return 5;
        }
        if ( DiffVarCounter > 4 )
            return 5;
    }
    return DiffVarCounter;
}

void InsertVars( Cube * pC, int * pVars, int nVarsIn, int * pVarValues )
// corrects the given number of variables (nVarsIn) in pC->pCubeDataIn[]
// variable numbers are given in pVarNumbers[], their values are in pVarValues[]
// arrays pVarNumbers[] and pVarValues[] are provided by the user
{
    int GlobalBit;
    int LocalWord;
    int LocalBit;
    int i;
    assert( nVarsIn > 0 && nVarsIn <= g_CoverInfo.nVarsIn );
    for ( i = 0; i < nVarsIn; i++ )
    {
        assert( pVars[i] >= 0 && pVars[i] < g_CoverInfo.nVarsIn );
        assert( pVarValues[i] == VAR_NEG || pVarValues[i] == VAR_POS || pVarValues[i] == VAR_ABS );
        GlobalBit = (pVars[i]<<1);
        LocalWord = VarWord(GlobalBit);
        LocalBit  = VarBit(GlobalBit);

        // correct this variables
        pC->pCubeDataIn[LocalWord] = ((pC->pCubeDataIn[LocalWord]&(~(3<<LocalBit)))|(pVarValues[i]<<LocalBit));
    }
}

void InsertVarsWithoutClearing( Cube * pC, int * pVars, int nVarsIn, int * pVarValues, int Output )
// corrects the given number of variables (nVarsIn) in pC->pCubeDataIn[]
// variable numbers are given in pVarNumbers[], their values are in pVarValues[]
// arrays pVarNumbers[] and pVarValues[] are provided by the user
{
    int GlobalBit;
    int LocalWord;
    int LocalBit;
    int i;
    assert( nVarsIn > 0 && nVarsIn <= g_CoverInfo.nVarsIn );
    for ( i = 0; i < nVarsIn; i++ )
    {
        assert( pVars[i] >= 0 && pVars[i] < g_CoverInfo.nVarsIn );
        assert( pVarValues[i] == VAR_NEG || pVarValues[i] == VAR_POS || pVarValues[i] == VAR_ABS );
        GlobalBit = (pVars[i]<<1);
        LocalWord = VarWord(GlobalBit);
        LocalBit  = VarBit(GlobalBit);

        // correct this variables
        pC->pCubeDataIn[LocalWord] |= ( pVarValues[i] << LocalBit );
    }
    // insert the output bit
    pC->pCubeDataOut[VarWord(Output)] |= ( 1 << VarBit(Output) );
}

///////////////////////////////////////////////////////////////////
////////////              End of File             /////////////////
///////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
