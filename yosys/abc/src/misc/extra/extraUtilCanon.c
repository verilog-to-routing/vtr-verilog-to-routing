/**CFile****************************************************************

  FileName    [extraUtilMisc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [Computing canonical forms of Boolean functions using truth tables.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extraUtilMisc.c,v 1.0 2003/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "extra.h"

ABC_NAMESPACE_IMPL_START


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


static unsigned s_Truths3[256] = 
{
    0x00000000, 0x01010101, 0x01010101, 0x03030303, 0x01010101, 0x05050505, 0x06060606, 0x07070707,
    0x01010101, 0x06060606, 0x05050505, 0x07070707, 0x03030303, 0x07070707, 0x07070707, 0x0f0f0f0f,
    0x01010101, 0x11111111, 0x12121212, 0x13131313, 0x14141414, 0x15151515, 0x16161616, 0x17171717,
    0x18181818, 0x19191919, 0x1a1a1a1a, 0x1b1b1b1b, 0x1c1c1c1c, 0x1d1d1d1d, 0x1e1e1e1e, 0x1f1f1f1f,
    0x01010101, 0x12121212, 0x11111111, 0x13131313, 0x18181818, 0x1a1a1a1a, 0x19191919, 0x1b1b1b1b,
    0x14141414, 0x16161616, 0x15151515, 0x17171717, 0x1c1c1c1c, 0x1e1e1e1e, 0x1d1d1d1d, 0x1f1f1f1f,
    0x03030303, 0x13131313, 0x13131313, 0x33333333, 0x1c1c1c1c, 0x35353535, 0x36363636, 0x37373737,
    0x1c1c1c1c, 0x36363636, 0x35353535, 0x37373737, 0x3c3c3c3c, 0x3d3d3d3d, 0x3d3d3d3d, 0x3f3f3f3f,
    0x01010101, 0x14141414, 0x18181818, 0x1c1c1c1c, 0x11111111, 0x15151515, 0x19191919, 0x1d1d1d1d,
    0x12121212, 0x16161616, 0x1a1a1a1a, 0x1e1e1e1e, 0x13131313, 0x17171717, 0x1b1b1b1b, 0x1f1f1f1f,
    0x05050505, 0x15151515, 0x1a1a1a1a, 0x35353535, 0x15151515, 0x55555555, 0x56565656, 0x57575757,
    0x1a1a1a1a, 0x56565656, 0x5a5a5a5a, 0x5b5b5b5b, 0x35353535, 0x57575757, 0x5b5b5b5b, 0x5f5f5f5f,
    0x06060606, 0x16161616, 0x19191919, 0x36363636, 0x19191919, 0x56565656, 0x66666666, 0x67676767,
    0x16161616, 0x69696969, 0x56565656, 0x6b6b6b6b, 0x36363636, 0x6b6b6b6b, 0x67676767, 0x6f6f6f6f,
    0x07070707, 0x17171717, 0x1b1b1b1b, 0x37373737, 0x1d1d1d1d, 0x57575757, 0x67676767, 0x77777777,
    0x1e1e1e1e, 0x6b6b6b6b, 0x5b5b5b5b, 0x7b7b7b7b, 0x3d3d3d3d, 0x7d7d7d7d, 0x7e7e7e7e, 0x7f7f7f7f,
    0x01010101, 0x18181818, 0x14141414, 0x1c1c1c1c, 0x12121212, 0x1a1a1a1a, 0x16161616, 0x1e1e1e1e,
    0x11111111, 0x19191919, 0x15151515, 0x1d1d1d1d, 0x13131313, 0x1b1b1b1b, 0x17171717, 0x1f1f1f1f,
    0x06060606, 0x19191919, 0x16161616, 0x36363636, 0x16161616, 0x56565656, 0x69696969, 0x6b6b6b6b,
    0x19191919, 0x66666666, 0x56565656, 0x67676767, 0x36363636, 0x67676767, 0x6b6b6b6b, 0x6f6f6f6f,
    0x05050505, 0x1a1a1a1a, 0x15151515, 0x35353535, 0x1a1a1a1a, 0x5a5a5a5a, 0x56565656, 0x5b5b5b5b,
    0x15151515, 0x56565656, 0x55555555, 0x57575757, 0x35353535, 0x5b5b5b5b, 0x57575757, 0x5f5f5f5f,
    0x07070707, 0x1b1b1b1b, 0x17171717, 0x37373737, 0x1e1e1e1e, 0x5b5b5b5b, 0x6b6b6b6b, 0x7b7b7b7b,
    0x1d1d1d1d, 0x67676767, 0x57575757, 0x77777777, 0x3d3d3d3d, 0x7e7e7e7e, 0x7d7d7d7d, 0x7f7f7f7f,
    0x03030303, 0x1c1c1c1c, 0x1c1c1c1c, 0x3c3c3c3c, 0x13131313, 0x35353535, 0x36363636, 0x3d3d3d3d,
    0x13131313, 0x36363636, 0x35353535, 0x3d3d3d3d, 0x33333333, 0x37373737, 0x37373737, 0x3f3f3f3f,
    0x07070707, 0x1d1d1d1d, 0x1e1e1e1e, 0x3d3d3d3d, 0x17171717, 0x57575757, 0x6b6b6b6b, 0x7d7d7d7d,
    0x1b1b1b1b, 0x67676767, 0x5b5b5b5b, 0x7e7e7e7e, 0x37373737, 0x77777777, 0x7b7b7b7b, 0x7f7f7f7f,
    0x07070707, 0x1e1e1e1e, 0x1d1d1d1d, 0x3d3d3d3d, 0x1b1b1b1b, 0x5b5b5b5b, 0x67676767, 0x7e7e7e7e,
    0x17171717, 0x6b6b6b6b, 0x57575757, 0x7d7d7d7d, 0x37373737, 0x7b7b7b7b, 0x77777777, 0x7f7f7f7f,
    0x0f0f0f0f, 0x1f1f1f1f, 0x1f1f1f1f, 0x3f3f3f3f, 0x1f1f1f1f, 0x5f5f5f5f, 0x6f6f6f6f, 0x7f7f7f7f,
    0x1f1f1f1f, 0x6f6f6f6f, 0x5f5f5f5f, 0x7f7f7f7f, 0x3f3f3f3f, 0x7f7f7f7f, 0x7f7f7f7f, 0xffffffff
};

static char s_Phases3[256][9] = 
{
/*   0 */  {  8,   0, 1, 2, 3, 4, 5, 6, 7 }, 
/*   1 */  {  1,   0 }, 
/*   2 */  {  1,   1 }, 
/*   3 */  {  2,   0, 1 }, 
/*   4 */  {  1,   2 }, 
/*   5 */  {  2,   0, 2 }, 
/*   6 */  {  2,   0, 3 }, 
/*   7 */  {  1,   0 }, 
/*   8 */  {  1,   3 }, 
/*   9 */  {  2,   1, 2 }, 
/*  10 */  {  2,   1, 3 }, 
/*  11 */  {  1,   1 }, 
/*  12 */  {  2,   2, 3 }, 
/*  13 */  {  1,   2 }, 
/*  14 */  {  1,   3 }, 
/*  15 */  {  4,   0, 1, 2, 3 }, 
/*  16 */  {  1,   4 }, 
/*  17 */  {  2,   0, 4 }, 
/*  18 */  {  2,   0, 5 }, 
/*  19 */  {  1,   0 }, 
/*  20 */  {  2,   0, 6 }, 
/*  21 */  {  1,   0 }, 
/*  22 */  {  1,   0 }, 
/*  23 */  {  1,   0 }, 
/*  24 */  {  2,   0, 7 }, 
/*  25 */  {  1,   0 }, 
/*  26 */  {  1,   0 }, 
/*  27 */  {  1,   0 }, 
/*  28 */  {  1,   0 }, 
/*  29 */  {  1,   0 }, 
/*  30 */  {  1,   0 }, 
/*  31 */  {  1,   0 }, 
/*  32 */  {  1,   5 }, 
/*  33 */  {  2,   1, 4 }, 
/*  34 */  {  2,   1, 5 }, 
/*  35 */  {  1,   1 }, 
/*  36 */  {  2,   1, 6 }, 
/*  37 */  {  1,   1 }, 
/*  38 */  {  1,   1 }, 
/*  39 */  {  1,   1 }, 
/*  40 */  {  2,   1, 7 }, 
/*  41 */  {  1,   1 }, 
/*  42 */  {  1,   1 }, 
/*  43 */  {  1,   1 }, 
/*  44 */  {  1,   1 }, 
/*  45 */  {  1,   1 }, 
/*  46 */  {  1,   1 }, 
/*  47 */  {  1,   1 }, 
/*  48 */  {  2,   4, 5 }, 
/*  49 */  {  1,   4 }, 
/*  50 */  {  1,   5 }, 
/*  51 */  {  4,   0, 1, 4, 5 }, 
/*  52 */  {  1,   6 }, 
/*  53 */  {  1,   0 }, 
/*  54 */  {  1,   0 }, 
/*  55 */  {  1,   0 }, 
/*  56 */  {  1,   7 }, 
/*  57 */  {  1,   1 }, 
/*  58 */  {  1,   1 }, 
/*  59 */  {  1,   1 }, 
/*  60 */  {  4,   0, 1, 6, 7 }, 
/*  61 */  {  1,   0 }, 
/*  62 */  {  1,   1 }, 
/*  63 */  {  2,   0, 1 }, 
/*  64 */  {  1,   6 }, 
/*  65 */  {  2,   2, 4 }, 
/*  66 */  {  2,   2, 5 }, 
/*  67 */  {  1,   2 }, 
/*  68 */  {  2,   2, 6 }, 
/*  69 */  {  1,   2 }, 
/*  70 */  {  1,   2 }, 
/*  71 */  {  1,   2 }, 
/*  72 */  {  2,   2, 7 }, 
/*  73 */  {  1,   2 }, 
/*  74 */  {  1,   2 }, 
/*  75 */  {  1,   2 }, 
/*  76 */  {  1,   2 }, 
/*  77 */  {  1,   2 }, 
/*  78 */  {  1,   2 }, 
/*  79 */  {  1,   2 }, 
/*  80 */  {  2,   4, 6 }, 
/*  81 */  {  1,   4 }, 
/*  82 */  {  1,   5 }, 
/*  83 */  {  1,   4 }, 
/*  84 */  {  1,   6 }, 
/*  85 */  {  4,   0, 2, 4, 6 }, 
/*  86 */  {  1,   0 }, 
/*  87 */  {  1,   0 }, 
/*  88 */  {  1,   7 }, 
/*  89 */  {  1,   2 }, 
/*  90 */  {  4,   0, 2, 5, 7 }, 
/*  91 */  {  1,   0 }, 
/*  92 */  {  1,   6 }, 
/*  93 */  {  1,   2 }, 
/*  94 */  {  1,   2 }, 
/*  95 */  {  2,   0, 2 }, 
/*  96 */  {  2,   4, 7 }, 
/*  97 */  {  1,   4 }, 
/*  98 */  {  1,   5 }, 
/*  99 */  {  1,   4 }, 
/* 100 */  {  1,   6 }, 
/* 101 */  {  1,   4 }, 
/* 102 */  {  4,   0, 3, 4, 7 }, 
/* 103 */  {  1,   0 }, 
/* 104 */  {  1,   7 }, 
/* 105 */  {  4,   0, 3, 5, 6 }, 
/* 106 */  {  1,   7 }, 
/* 107 */  {  1,   0 }, 
/* 108 */  {  1,   7 }, 
/* 109 */  {  1,   3 }, 
/* 110 */  {  1,   3 }, 
/* 111 */  {  2,   0, 3 }, 
/* 112 */  {  1,   4 }, 
/* 113 */  {  1,   4 }, 
/* 114 */  {  1,   5 }, 
/* 115 */  {  1,   4 }, 
/* 116 */  {  1,   6 }, 
/* 117 */  {  1,   4 }, 
/* 118 */  {  1,   4 }, 
/* 119 */  {  2,   0, 4 }, 
/* 120 */  {  1,   7 }, 
/* 121 */  {  1,   5 }, 
/* 122 */  {  1,   5 }, 
/* 123 */  {  2,   0, 5 }, 
/* 124 */  {  1,   6 }, 
/* 125 */  {  2,   0, 6 }, 
/* 126 */  {  2,   0, 7 }, 
/* 127 */  {  1,   0 }, 
/* 128 */  {  1,   7 }, 
/* 129 */  {  2,   3, 4 }, 
/* 130 */  {  2,   3, 5 }, 
/* 131 */  {  1,   3 }, 
/* 132 */  {  2,   3, 6 }, 
/* 133 */  {  1,   3 }, 
/* 134 */  {  1,   3 }, 
/* 135 */  {  1,   3 }, 
/* 136 */  {  2,   3, 7 }, 
/* 137 */  {  1,   3 }, 
/* 138 */  {  1,   3 }, 
/* 139 */  {  1,   3 }, 
/* 140 */  {  1,   3 }, 
/* 141 */  {  1,   3 }, 
/* 142 */  {  1,   3 }, 
/* 143 */  {  1,   3 }, 
/* 144 */  {  2,   5, 6 }, 
/* 145 */  {  1,   4 }, 
/* 146 */  {  1,   5 }, 
/* 147 */  {  1,   5 }, 
/* 148 */  {  1,   6 }, 
/* 149 */  {  1,   6 }, 
/* 150 */  {  4,   1, 2, 4, 7 }, 
/* 151 */  {  1,   1 }, 
/* 152 */  {  1,   7 }, 
/* 153 */  {  4,   1, 2, 5, 6 }, 
/* 154 */  {  1,   5 }, 
/* 155 */  {  1,   1 }, 
/* 156 */  {  1,   6 }, 
/* 157 */  {  1,   2 }, 
/* 158 */  {  1,   2 }, 
/* 159 */  {  2,   1, 2 }, 
/* 160 */  {  2,   5, 7 }, 
/* 161 */  {  1,   4 }, 
/* 162 */  {  1,   5 }, 
/* 163 */  {  1,   5 }, 
/* 164 */  {  1,   6 }, 
/* 165 */  {  4,   1, 3, 4, 6 }, 
/* 166 */  {  1,   3 }, 
/* 167 */  {  1,   1 }, 
/* 168 */  {  1,   7 }, 
/* 169 */  {  1,   1 }, 
/* 170 */  {  4,   1, 3, 5, 7 }, 
/* 171 */  {  1,   1 }, 
/* 172 */  {  1,   7 }, 
/* 173 */  {  1,   3 }, 
/* 174 */  {  1,   3 }, 
/* 175 */  {  2,   1, 3 }, 
/* 176 */  {  1,   5 }, 
/* 177 */  {  1,   4 }, 
/* 178 */  {  1,   5 }, 
/* 179 */  {  1,   5 }, 
/* 180 */  {  1,   6 }, 
/* 181 */  {  1,   4 }, 
/* 182 */  {  1,   4 }, 
/* 183 */  {  2,   1, 4 }, 
/* 184 */  {  1,   7 }, 
/* 185 */  {  1,   5 }, 
/* 186 */  {  1,   5 }, 
/* 187 */  {  2,   1, 5 }, 
/* 188 */  {  1,   7 }, 
/* 189 */  {  2,   1, 6 }, 
/* 190 */  {  2,   1, 7 }, 
/* 191 */  {  1,   1 }, 
/* 192 */  {  2,   6, 7 }, 
/* 193 */  {  1,   4 }, 
/* 194 */  {  1,   5 }, 
/* 195 */  {  4,   2, 3, 4, 5 }, 
/* 196 */  {  1,   6 }, 
/* 197 */  {  1,   2 }, 
/* 198 */  {  1,   3 }, 
/* 199 */  {  1,   2 }, 
/* 200 */  {  1,   7 }, 
/* 201 */  {  1,   2 }, 
/* 202 */  {  1,   3 }, 
/* 203 */  {  1,   3 }, 
/* 204 */  {  4,   2, 3, 6, 7 }, 
/* 205 */  {  1,   2 }, 
/* 206 */  {  1,   3 }, 
/* 207 */  {  2,   2, 3 }, 
/* 208 */  {  1,   6 }, 
/* 209 */  {  1,   4 }, 
/* 210 */  {  1,   5 }, 
/* 211 */  {  1,   4 }, 
/* 212 */  {  1,   6 }, 
/* 213 */  {  1,   6 }, 
/* 214 */  {  1,   7 }, 
/* 215 */  {  2,   2, 4 }, 
/* 216 */  {  1,   7 }, 
/* 217 */  {  1,   6 }, 
/* 218 */  {  1,   7 }, 
/* 219 */  {  2,   2, 5 }, 
/* 220 */  {  1,   6 }, 
/* 221 */  {  2,   2, 6 }, 
/* 222 */  {  2,   2, 7 }, 
/* 223 */  {  1,   2 }, 
/* 224 */  {  1,   7 }, 
/* 225 */  {  1,   4 }, 
/* 226 */  {  1,   5 }, 
/* 227 */  {  1,   5 }, 
/* 228 */  {  1,   6 }, 
/* 229 */  {  1,   6 }, 
/* 230 */  {  1,   7 }, 
/* 231 */  {  2,   3, 4 }, 
/* 232 */  {  1,   7 }, 
/* 233 */  {  1,   6 }, 
/* 234 */  {  1,   7 }, 
/* 235 */  {  2,   3, 5 }, 
/* 236 */  {  1,   7 }, 
/* 237 */  {  2,   3, 6 }, 
/* 238 */  {  2,   3, 7 }, 
/* 239 */  {  1,   3 }, 
/* 240 */  {  4,   4, 5, 6, 7 }, 
/* 241 */  {  1,   4 }, 
/* 242 */  {  1,   5 }, 
/* 243 */  {  2,   4, 5 }, 
/* 244 */  {  1,   6 }, 
/* 245 */  {  2,   4, 6 }, 
/* 246 */  {  2,   4, 7 }, 
/* 247 */  {  1,   4 }, 
/* 248 */  {  1,   7 }, 
/* 249 */  {  2,   5, 6 }, 
/* 250 */  {  2,   5, 7 }, 
/* 251 */  {  1,   5 }, 
/* 252 */  {  2,   6, 7 }, 
/* 253 */  {  1,   6 }, 
/* 254 */  {  1,   7 }, 
/* 255 */  {  8,   0, 1, 2, 3, 4, 5, 6, 7 }
};


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static int Extra_TruthCanonN_rec( int nVars, unsigned char * pt, unsigned ** pptRes, char ** ppfRes, int Flag );

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Computes the N-canonical form of the Boolean function up to 6 inputs.]

  Description [The N-canonical form is defined as the truth table with 
  the minimum integer value. This function exhaustively enumerates 
  through the complete set of 2^N phase assignments.
  Returns pointers to the static storage to the truth table and phases. 
  This data should be used before the function is called again.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_TruthCanonFastN( int nVarsMax, int nVarsReal, unsigned * pt, unsigned ** pptRes, char ** ppfRes )
{
    static unsigned uTruthStore6[2];
    int RetValue;
    assert( nVarsMax <= 6 );
    assert( nVarsReal <= nVarsMax );
    RetValue = Extra_TruthCanonN_rec( nVarsReal <= 3? 3: nVarsReal, (unsigned char *)pt, pptRes, ppfRes, 0 );
    if ( nVarsMax == 6 && nVarsReal < nVarsMax )
    {
        uTruthStore6[0] = **pptRes;
        uTruthStore6[1] = **pptRes;
        *pptRes = uTruthStore6;
    }
    return RetValue;
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    [Recursive implementation of the above.]

  Description []
               
  SideEffects [This procedure has a bug, which shows on Solaris.
  Most likely has something to do with the casts, i.g *((unsigned *)pt0)]

  SeeAlso     []

***********************************************************************/
int Extra_TruthCanonN_rec( int nVars, unsigned char * pt, unsigned ** pptRes, char ** ppfRes, int Flag )
{
    static unsigned uTruthStore[7][2][2];
    static char uPhaseStore[7][2][64];

    unsigned char * pt0, * pt1;
    unsigned * ptRes0, * ptRes1, * ptRes;
    unsigned uInit0, uInit1, uTruth0, uTruth1, uTemp;
    char * pfRes0, * pfRes1, * pfRes;
    int nf0, nf1, nfRes, i, nVarsN;

    // table lookup for three vars
    if ( nVars == 3 )
    {
        *pptRes = &s_Truths3[*pt];
        *ppfRes = s_Phases3[*pt]+1;
        return s_Phases3[*pt][0];
    }

    // number of vars for the next call
    nVarsN = nVars-1;
    // truth table for the next call
    pt0 = pt;
    pt1 = pt + (1 << nVarsN) / 8;
    // 5-var truth tables for this call
//    uInit0 = *((unsigned *)pt0);
//    uInit1 = *((unsigned *)pt1);
    if ( nVarsN == 3 )
    {
        uInit0 = (pt0[0] << 24) | (pt0[0] << 16) | (pt0[0] << 8) | pt0[0];
        uInit1 = (pt1[0] << 24) | (pt1[0] << 16) | (pt1[0] << 8) | pt1[0];
    }
    else if ( nVarsN == 4 )
    {
        uInit0 = (pt0[1] << 24) | (pt0[0] << 16) | (pt0[1] << 8) | pt0[0];
        uInit1 = (pt1[1] << 24) | (pt1[0] << 16) | (pt1[1] << 8) | pt1[0];
    }
    else
    {
        uInit0 = (pt0[3] << 24) | (pt0[2] << 16) | (pt0[1] << 8) | pt0[0];
        uInit1 = (pt1[3] << 24) | (pt1[2] << 16) | (pt1[1] << 8) | pt1[0];
    }

    // storage for truth tables and phases
    ptRes  = uTruthStore[nVars][Flag];
    pfRes  = uPhaseStore[nVars][Flag];

    // solve trivial cases
    if ( uInit1 == 0 )
    {
        nf0 = Extra_TruthCanonN_rec( nVarsN, pt0, &ptRes0, &pfRes0, 0 );
        uTruth1 = uInit1;
        uTruth0 = *ptRes0;
        nfRes   = 0;
        for ( i = 0; i < nf0; i++ )
            pfRes[nfRes++] = pfRes0[i];
        goto finish;
    }
    if ( uInit0 == 0 )
    {
        nf1 = Extra_TruthCanonN_rec( nVarsN, pt1, &ptRes1, &pfRes1, 1 );
        uTruth1 = uInit0;
        uTruth0 = *ptRes1;
        nfRes   = 0;
        for ( i = 0; i < nf1; i++ )
            pfRes[nfRes++] = pfRes1[i] | (1<<nVarsN);
        goto finish;
    }

    if ( uInit1 == 0xFFFFFFFF )
    {
        nf0 = Extra_TruthCanonN_rec( nVarsN, pt0, &ptRes0, &pfRes0, 0 );
        uTruth1 = *ptRes0;
        uTruth0 = uInit1;
        nfRes   = 0;
        for ( i = 0; i < nf0; i++ )
            pfRes[nfRes++] = pfRes0[i] | (1<<nVarsN);
        goto finish;
    }
    if ( uInit0 == 0xFFFFFFFF )
    {
        nf1 = Extra_TruthCanonN_rec( nVarsN, pt1, &ptRes1, &pfRes1, 1 );
        uTruth1 = *ptRes1;
        uTruth0 = uInit0;
        nfRes   = 0;
        for ( i = 0; i < nf1; i++ )
            pfRes[nfRes++] = pfRes1[i];
        goto finish;
    }

    // solve the problem for cofactors
    nf0 = Extra_TruthCanonN_rec( nVarsN, pt0, &ptRes0, &pfRes0, 0 );
    nf1 = Extra_TruthCanonN_rec( nVarsN, pt1, &ptRes1, &pfRes1, 1 );

    // combine the result
    if ( *ptRes1 < *ptRes0 )
    {
        uTruth0 = 0xFFFFFFFF;
        nfRes   = 0;
        for ( i = 0; i < nf1; i++ )
        {
            uTemp = Extra_TruthPolarize( uInit0, pfRes1[i], nVarsN );
            if ( uTruth0 > uTemp )
            {
                nfRes          = 0;
                uTruth0        = uTemp;
                pfRes[nfRes++] = pfRes1[i];
            }
            else if ( uTruth0 == uTemp ) 
                pfRes[nfRes++] = pfRes1[i];
        }
        uTruth1 = *ptRes1;
    }
    else if ( *ptRes1 > *ptRes0 )
    {
        uTruth0 = 0xFFFFFFFF;
        nfRes   = 0;
        for ( i = 0; i < nf0; i++ )
        {
            uTemp = Extra_TruthPolarize( uInit1, pfRes0[i], nVarsN );
            if ( uTruth0 > uTemp )
            {
                nfRes          = 0;
                uTruth0        = uTemp;
                pfRes[nfRes++] = pfRes0[i] | (1<<nVarsN);
            }
            else if ( uTruth0 == uTemp ) 
                pfRes[nfRes++] = pfRes0[i] | (1<<nVarsN);
        }
        uTruth1 = *ptRes0;
    }
    else 
    {
        assert( nf0 == nf1 );
        nfRes = 0; 
        for ( i = 0; i < nf1; i++ )
            pfRes[nfRes++] = pfRes1[i];
        for ( i = 0; i < nf0; i++ )
            pfRes[nfRes++] = pfRes0[i] | (1<<nVarsN);
        uTruth0 = Extra_TruthPolarize( uInit0, pfRes1[0], nVarsN );
        uTruth1 = *ptRes0;
    }

finish :
    if ( nVarsN == 3 )
    {
        uTruth0 &= 0xFF;
        uTruth1 &= 0xFF;
        uTemp = (uTruth1 << 8) | uTruth0;
        *ptRes = (uTemp << 16) | uTemp;
    }
    else if ( nVarsN == 4 )
    {
        uTruth0 &= 0xFFFF;
        uTruth1 &= 0xFFFF;
        *ptRes = (uTruth1 << 16) | uTruth0;
    }
    else if ( nVarsN == 5 )
    {
        *(ptRes+0) = uTruth0;
        *(ptRes+1) = uTruth1;
    }

    *pptRes = ptRes;
    *ppfRes = pfRes;
    return nfRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_Var3Print()
{
    extern void Extra_Truth3VarN( unsigned ** puCanons, char *** puPhases, char ** ppCounters );

    unsigned * uCanons;
    char ** uPhases;
    char * pCounters;
    int i, k;

    Extra_Truth3VarN( &uCanons, &uPhases, &pCounters );

    for ( i = 0; i < 256; i++ )
    {
        if ( i % 8 == 0 )
            printf( "\n" );
        Extra_PrintHex( stdout, uCanons + i, 5 ); 
        printf( ", " );
    }
    printf( "\n" );

    for ( i = 0; i < 256; i++ )
    {
        printf( "%3d */  { %2d,   ", i, pCounters[i] );
        for ( k = 0; k < pCounters[i]; k++ )            
            printf( "%s%d", k? ", ":"", uPhases[i][k] );
        printf( " }\n" );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_Var3Test()
{
    extern void Extra_Truth3VarN( unsigned ** puCanons, char *** puPhases, char ** ppCounters );

    unsigned * uCanons;
    char ** uPhases;
    char * pCounters;
    int i;
    unsigned * ptRes;
    char * pfRes;
    unsigned uTruth;
    int Count;

    Extra_Truth3VarN( &uCanons, &uPhases, &pCounters );

    for ( i = 0; i < 256; i++ )
    {
        uTruth = i;
        Count =  Extra_TruthCanonFastN( 5, 3, &uTruth, &ptRes, &pfRes );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_Var4Test()
{
    extern void Extra_Truth4VarN( unsigned short ** puCanons, char *** puPhases, char ** ppCounters, int PhaseMax );

    unsigned short * uCanons;
    char ** uPhases;
    char * pCounters;
    int i;
    unsigned * ptRes;
    char * pfRes;
    unsigned uTruth;
    int Count;

    Extra_Truth4VarN( &uCanons, &uPhases, &pCounters, 16 );

    for ( i = 0; i < 256*256; i++ )
    {
        uTruth = i;
        Count =  Extra_TruthCanonFastN( 5, 4, &uTruth, &ptRes, &pfRes );
    }
}

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

