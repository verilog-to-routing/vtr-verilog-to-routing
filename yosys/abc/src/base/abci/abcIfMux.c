/**CFile****************************************************************

  FileName    [abcIf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Interface with the FPGA mapping package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: abcIf.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "map/if/if.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_NtkFuncCof0( int t, int v )
{
    static int s_Truth[3] = { 0xAA, 0xCC, 0xF0 };
    return 0xff & ((t & ~s_Truth[v]) | ((t & ~s_Truth[v]) << (1<<v)));
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_NtkFuncCof1( int t, int v )
{
    static int s_Truth[3] = { 0xAA, 0xCC, 0xF0 };
    return 0xff & ((t &  s_Truth[v]) | ((t &  s_Truth[v]) >> (1<<v)));
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_NtkFuncHasVar( int t, int v )
{
    static int s_Truth[3] = { 0xAA, 0xCC, 0xF0 };
    return ((t & s_Truth[v]) >> (1<<v)) != (t & ~s_Truth[v]);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_NtkFuncSuppSize( int t )
{
    return Abc_NtkFuncHasVar(t, 0) + Abc_NtkFuncHasVar(t, 1) + Abc_NtkFuncHasVar(t, 2);
}

/**Function*************************************************************

  Synopsis    [Precomputes MUXes and functions of less than 3 inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCutCostMuxPrecompute()
{
    int i, Value;
    int CounterM = 0;
    for ( i = 0; i < 256; i++ )
    {
        Value = 0;
        if ( Abc_NtkFuncSuppSize( i ) < 3 )
            Value = 1;
        else
        {
            if ( (Abc_NtkFuncSuppSize(Abc_NtkFuncCof0(i,0)) == 1 && Abc_NtkFuncSuppSize(Abc_NtkFuncCof1(i,0)) == 1) ||
                 (Abc_NtkFuncSuppSize(Abc_NtkFuncCof0(i,1)) == 1 && Abc_NtkFuncSuppSize(Abc_NtkFuncCof1(i,1)) == 1) ||
                 (Abc_NtkFuncSuppSize(Abc_NtkFuncCof0(i,2)) == 1 && Abc_NtkFuncSuppSize(Abc_NtkFuncCof1(i,2)) == 1) )
            {
                 Value = 1;
                 CounterM++;
            }
        }
        printf( "%d, // %3d  0x%02X\n", Value, i, i );
    }
    printf( "Total number of MUXes = %d.\n", CounterM );
}

/**Function*************************************************************

  Synopsis    [Procedure returning the cost of the cut.]

  Description [The number of MUXes needed to implement the function.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCutCostMux( If_Man_t * p, If_Cut_t * pCut )
{    
    static char uLookup[256] = {
        1, //   0  0x00
        0, //   1  0x01
        0, //   2  0x02
        1, //   3  0x03
        0, //   4  0x04
        1, //   5  0x05
        0, //   6  0x06
        0, //   7  0x07
        0, //   8  0x08
        0, //   9  0x09
        1, //  10  0x0A
        0, //  11  0x0B
        1, //  12  0x0C
        0, //  13  0x0D
        0, //  14  0x0E
        1, //  15  0x0F
        0, //  16  0x10
        1, //  17  0x11
        0, //  18  0x12
        0, //  19  0x13
        0, //  20  0x14
        0, //  21  0x15
        0, //  22  0x16
        0, //  23  0x17
        0, //  24  0x18
        0, //  25  0x19
        0, //  26  0x1A
        1, //  27  0x1B
        0, //  28  0x1C
        1, //  29  0x1D
        0, //  30  0x1E
        0, //  31  0x1F
        0, //  32  0x20
        0, //  33  0x21
        1, //  34  0x22
        0, //  35  0x23
        0, //  36  0x24
        0, //  37  0x25
        0, //  38  0x26
        1, //  39  0x27
        0, //  40  0x28
        0, //  41  0x29
        0, //  42  0x2A
        0, //  43  0x2B
        0, //  44  0x2C
        0, //  45  0x2D
        1, //  46  0x2E
        0, //  47  0x2F
        1, //  48  0x30
        0, //  49  0x31
        0, //  50  0x32
        1, //  51  0x33
        0, //  52  0x34
        1, //  53  0x35
        0, //  54  0x36
        0, //  55  0x37
        0, //  56  0x38
        0, //  57  0x39
        1, //  58  0x3A
        0, //  59  0x3B
        1, //  60  0x3C
        0, //  61  0x3D
        0, //  62  0x3E
        1, //  63  0x3F
        0, //  64  0x40
        0, //  65  0x41
        0, //  66  0x42
        0, //  67  0x43
        1, //  68  0x44
        0, //  69  0x45
        0, //  70  0x46
        1, //  71  0x47
        0, //  72  0x48
        0, //  73  0x49
        0, //  74  0x4A
        0, //  75  0x4B
        0, //  76  0x4C
        0, //  77  0x4D
        1, //  78  0x4E
        0, //  79  0x4F
        1, //  80  0x50
        0, //  81  0x51
        0, //  82  0x52
        1, //  83  0x53
        0, //  84  0x54
        1, //  85  0x55
        0, //  86  0x56
        0, //  87  0x57
        0, //  88  0x58
        0, //  89  0x59
        1, //  90  0x5A
        0, //  91  0x5B
        1, //  92  0x5C
        0, //  93  0x5D
        0, //  94  0x5E
        1, //  95  0x5F
        0, //  96  0x60
        0, //  97  0x61
        0, //  98  0x62
        0, //  99  0x63
        0, // 100  0x64
        0, // 101  0x65
        1, // 102  0x66
        0, // 103  0x67
        0, // 104  0x68
        0, // 105  0x69
        0, // 106  0x6A
        0, // 107  0x6B
        0, // 108  0x6C
        0, // 109  0x6D
        0, // 110  0x6E
        0, // 111  0x6F
        0, // 112  0x70
        0, // 113  0x71
        1, // 114  0x72
        0, // 115  0x73
        1, // 116  0x74
        0, // 117  0x75
        0, // 118  0x76
        1, // 119  0x77
        0, // 120  0x78
        0, // 121  0x79
        0, // 122  0x7A
        0, // 123  0x7B
        0, // 124  0x7C
        0, // 125  0x7D
        0, // 126  0x7E
        0, // 127  0x7F
        0, // 128  0x80
        0, // 129  0x81
        0, // 130  0x82
        0, // 131  0x83
        0, // 132  0x84
        0, // 133  0x85
        0, // 134  0x86
        0, // 135  0x87
        1, // 136  0x88
        0, // 137  0x89
        0, // 138  0x8A
        1, // 139  0x8B
        0, // 140  0x8C
        1, // 141  0x8D
        0, // 142  0x8E
        0, // 143  0x8F
        0, // 144  0x90
        0, // 145  0x91
        0, // 146  0x92
        0, // 147  0x93
        0, // 148  0x94
        0, // 149  0x95
        0, // 150  0x96
        0, // 151  0x97
        0, // 152  0x98
        1, // 153  0x99
        0, // 154  0x9A
        0, // 155  0x9B
        0, // 156  0x9C
        0, // 157  0x9D
        0, // 158  0x9E
        0, // 159  0x9F
        1, // 160  0xA0
        0, // 161  0xA1
        0, // 162  0xA2
        1, // 163  0xA3
        0, // 164  0xA4
        1, // 165  0xA5
        0, // 166  0xA6
        0, // 167  0xA7
        0, // 168  0xA8
        0, // 169  0xA9
        1, // 170  0xAA
        0, // 171  0xAB
        1, // 172  0xAC
        0, // 173  0xAD
        0, // 174  0xAE
        1, // 175  0xAF
        0, // 176  0xB0
        1, // 177  0xB1
        0, // 178  0xB2
        0, // 179  0xB3
        0, // 180  0xB4
        0, // 181  0xB5
        0, // 182  0xB6
        0, // 183  0xB7
        1, // 184  0xB8
        0, // 185  0xB9
        0, // 186  0xBA
        1, // 187  0xBB
        0, // 188  0xBC
        0, // 189  0xBD
        0, // 190  0xBE
        0, // 191  0xBF
        1, // 192  0xC0
        0, // 193  0xC1
        0, // 194  0xC2
        1, // 195  0xC3
        0, // 196  0xC4
        1, // 197  0xC5
        0, // 198  0xC6
        0, // 199  0xC7
        0, // 200  0xC8
        0, // 201  0xC9
        1, // 202  0xCA
        0, // 203  0xCB
        1, // 204  0xCC
        0, // 205  0xCD
        0, // 206  0xCE
        1, // 207  0xCF
        0, // 208  0xD0
        1, // 209  0xD1
        0, // 210  0xD2
        0, // 211  0xD3
        0, // 212  0xD4
        0, // 213  0xD5
        0, // 214  0xD6
        0, // 215  0xD7
        1, // 216  0xD8
        0, // 217  0xD9
        0, // 218  0xDA
        0, // 219  0xDB
        0, // 220  0xDC
        1, // 221  0xDD
        0, // 222  0xDE
        0, // 223  0xDF
        0, // 224  0xE0
        0, // 225  0xE1
        1, // 226  0xE2
        0, // 227  0xE3
        1, // 228  0xE4
        0, // 229  0xE5
        0, // 230  0xE6
        0, // 231  0xE7
        0, // 232  0xE8
        0, // 233  0xE9
        0, // 234  0xEA
        0, // 235  0xEB
        0, // 236  0xEC
        0, // 237  0xED
        1, // 238  0xEE
        0, // 239  0xEF
        1, // 240  0xF0
        0, // 241  0xF1
        0, // 242  0xF2
        1, // 243  0xF3
        0, // 244  0xF4
        1, // 245  0xF5
        0, // 246  0xF6
        0, // 247  0xF7
        0, // 248  0xF8
        0, // 249  0xF9
        1, // 250  0xFA
        0, // 251  0xFB
        1, // 252  0xFC
        0, // 253  0xFD
        0, // 254  0xFE
        1  // 255  0xFF    
    };
    if ( pCut->nLeaves < 3 )
        return 1;
    if ( pCut->nLeaves == 3 && uLookup[0xff & *If_CutTruth(p, pCut)] )
        return 1;
    return (1 << pCut->nLeaves) - 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

