/**CFile****************************************************************

  FileName    [acecSt.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecSt.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"
#include "misc/vec/vecWec.h"
#include "misc/extra/extra.h"
#include "aig/aig/aig.h"
#include "opt/dar/dar.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

int Npn3Table[256][2] = {
    {0x00,  0}, // 0x00 =   0
    {0x01,  1}, // 0x01 =   1
    {0x01,  1}, // 0x02 =   2
    {0x03,  2}, // 0x03 =   3
    {0x01,  1}, // 0x04 =   4
    {0x03,  2}, // 0x05 =   5
    {0x06,  3}, // 0x06 =   6
    {0x07,  4}, // 0x07 =   7
    {0x01,  1}, // 0x08 =   8
    {0x06,  3}, // 0x09 =   9
    {0x03,  2}, // 0x0A =  10
    {0x07,  4}, // 0x0B =  11
    {0x03,  2}, // 0x0C =  12
    {0x07,  4}, // 0x0D =  13
    {0x07,  4}, // 0x0E =  14
    {0x0F,  5}, // 0x0F =  15
    {0x01,  1}, // 0x10 =  16
    {0x03,  2}, // 0x11 =  17
    {0x06,  3}, // 0x12 =  18
    {0x07,  4}, // 0x13 =  19
    {0x06,  3}, // 0x14 =  20
    {0x07,  4}, // 0x15 =  21
    {0x16,  6}, // 0x16 =  22
    {0x17,  7}, // 0x17 =  23
    {0x18,  8}, // 0x18 =  24
    {0x19,  9}, // 0x19 =  25
    {0x19,  9}, // 0x1A =  26
    {0x1B, 10}, // 0x1B =  27
    {0x19,  9}, // 0x1C =  28
    {0x1B, 10}, // 0x1D =  29
    {0x1E, 11}, // 0x1E =  30
    {0x07,  4}, // 0x1F =  31
    {0x01,  1}, // 0x20 =  32
    {0x06,  3}, // 0x21 =  33
    {0x03,  2}, // 0x22 =  34
    {0x07,  4}, // 0x23 =  35
    {0x18,  8}, // 0x24 =  36
    {0x19,  9}, // 0x25 =  37
    {0x19,  9}, // 0x26 =  38
    {0x1B, 10}, // 0x27 =  39
    {0x06,  3}, // 0x28 =  40
    {0x16,  6}, // 0x29 =  41
    {0x07,  4}, // 0x2A =  42
    {0x17,  7}, // 0x2B =  43
    {0x19,  9}, // 0x2C =  44
    {0x1E, 11}, // 0x2D =  45
    {0x1B, 10}, // 0x2E =  46
    {0x07,  4}, // 0x2F =  47
    {0x03,  2}, // 0x30 =  48
    {0x07,  4}, // 0x31 =  49
    {0x07,  4}, // 0x32 =  50
    {0x0F,  5}, // 0x33 =  51
    {0x19,  9}, // 0x34 =  52
    {0x1B, 10}, // 0x35 =  53
    {0x1E, 11}, // 0x36 =  54
    {0x07,  4}, // 0x37 =  55
    {0x19,  9}, // 0x38 =  56
    {0x1E, 11}, // 0x39 =  57
    {0x1B, 10}, // 0x3A =  58
    {0x07,  4}, // 0x3B =  59
    {0x3C, 12}, // 0x3C =  60
    {0x19,  9}, // 0x3D =  61
    {0x19,  9}, // 0x3E =  62
    {0x03,  2}, // 0x3F =  63
    {0x01,  1}, // 0x40 =  64
    {0x06,  3}, // 0x41 =  65
    {0x18,  8}, // 0x42 =  66
    {0x19,  9}, // 0x43 =  67
    {0x03,  2}, // 0x44 =  68
    {0x07,  4}, // 0x45 =  69
    {0x19,  9}, // 0x46 =  70
    {0x1B, 10}, // 0x47 =  71
    {0x06,  3}, // 0x48 =  72
    {0x16,  6}, // 0x49 =  73
    {0x19,  9}, // 0x4A =  74
    {0x1E, 11}, // 0x4B =  75
    {0x07,  4}, // 0x4C =  76
    {0x17,  7}, // 0x4D =  77
    {0x1B, 10}, // 0x4E =  78
    {0x07,  4}, // 0x4F =  79
    {0x03,  2}, // 0x50 =  80
    {0x07,  4}, // 0x51 =  81
    {0x19,  9}, // 0x52 =  82
    {0x1B, 10}, // 0x53 =  83
    {0x07,  4}, // 0x54 =  84
    {0x0F,  5}, // 0x55 =  85
    {0x1E, 11}, // 0x56 =  86
    {0x07,  4}, // 0x57 =  87
    {0x19,  9}, // 0x58 =  88
    {0x1E, 11}, // 0x59 =  89
    {0x3C, 12}, // 0x5A =  90
    {0x19,  9}, // 0x5B =  91
    {0x1B, 10}, // 0x5C =  92
    {0x07,  4}, // 0x5D =  93
    {0x19,  9}, // 0x5E =  94
    {0x03,  2}, // 0x5F =  95
    {0x06,  3}, // 0x60 =  96
    {0x16,  6}, // 0x61 =  97
    {0x19,  9}, // 0x62 =  98
    {0x1E, 11}, // 0x63 =  99
    {0x19,  9}, // 0x64 = 100
    {0x1E, 11}, // 0x65 = 101
    {0x3C, 12}, // 0x66 = 102
    {0x19,  9}, // 0x67 = 103
    {0x16,  6}, // 0x68 = 104
    {0x69, 13}, // 0x69 = 105
    {0x1E, 11}, // 0x6A = 106
    {0x16,  6}, // 0x6B = 107
    {0x1E, 11}, // 0x6C = 108
    {0x16,  6}, // 0x6D = 109
    {0x19,  9}, // 0x6E = 110
    {0x06,  3}, // 0x6F = 111
    {0x07,  4}, // 0x70 = 112
    {0x17,  7}, // 0x71 = 113
    {0x1B, 10}, // 0x72 = 114
    {0x07,  4}, // 0x73 = 115
    {0x1B, 10}, // 0x74 = 116
    {0x07,  4}, // 0x75 = 117
    {0x19,  9}, // 0x76 = 118
    {0x03,  2}, // 0x77 = 119
    {0x1E, 11}, // 0x78 = 120
    {0x16,  6}, // 0x79 = 121
    {0x19,  9}, // 0x7A = 122
    {0x06,  3}, // 0x7B = 123
    {0x19,  9}, // 0x7C = 124
    {0x06,  3}, // 0x7D = 125
    {0x18,  8}, // 0x7E = 126
    {0x01,  1}, // 0x7F = 127
    {0x01,  1}, // 0x80 = 128
    {0x18,  8}, // 0x81 = 129
    {0x06,  3}, // 0x82 = 130
    {0x19,  9}, // 0x83 = 131
    {0x06,  3}, // 0x84 = 132
    {0x19,  9}, // 0x85 = 133
    {0x16,  6}, // 0x86 = 134
    {0x1E, 11}, // 0x87 = 135
    {0x03,  2}, // 0x88 = 136
    {0x19,  9}, // 0x89 = 137
    {0x07,  4}, // 0x8A = 138
    {0x1B, 10}, // 0x8B = 139
    {0x07,  4}, // 0x8C = 140
    {0x1B, 10}, // 0x8D = 141
    {0x17,  7}, // 0x8E = 142
    {0x07,  4}, // 0x8F = 143
    {0x06,  3}, // 0x90 = 144
    {0x19,  9}, // 0x91 = 145
    {0x16,  6}, // 0x92 = 146
    {0x1E, 11}, // 0x93 = 147
    {0x16,  6}, // 0x94 = 148
    {0x1E, 11}, // 0x95 = 149
    {0x69, 13}, // 0x96 = 150
    {0x16,  6}, // 0x97 = 151
    {0x19,  9}, // 0x98 = 152
    {0x3C, 12}, // 0x99 = 153
    {0x1E, 11}, // 0x9A = 154
    {0x19,  9}, // 0x9B = 155
    {0x1E, 11}, // 0x9C = 156
    {0x19,  9}, // 0x9D = 157
    {0x16,  6}, // 0x9E = 158
    {0x06,  3}, // 0x9F = 159
    {0x03,  2}, // 0xA0 = 160
    {0x19,  9}, // 0xA1 = 161
    {0x07,  4}, // 0xA2 = 162
    {0x1B, 10}, // 0xA3 = 163
    {0x19,  9}, // 0xA4 = 164
    {0x3C, 12}, // 0xA5 = 165
    {0x1E, 11}, // 0xA6 = 166
    {0x19,  9}, // 0xA7 = 167
    {0x07,  4}, // 0xA8 = 168
    {0x1E, 11}, // 0xA9 = 169
    {0x0F,  5}, // 0xAA = 170
    {0x07,  4}, // 0xAB = 171
    {0x1B, 10}, // 0xAC = 172
    {0x19,  9}, // 0xAD = 173
    {0x07,  4}, // 0xAE = 174
    {0x03,  2}, // 0xAF = 175
    {0x07,  4}, // 0xB0 = 176
    {0x1B, 10}, // 0xB1 = 177
    {0x17,  7}, // 0xB2 = 178
    {0x07,  4}, // 0xB3 = 179
    {0x1E, 11}, // 0xB4 = 180
    {0x19,  9}, // 0xB5 = 181
    {0x16,  6}, // 0xB6 = 182
    {0x06,  3}, // 0xB7 = 183
    {0x1B, 10}, // 0xB8 = 184
    {0x19,  9}, // 0xB9 = 185
    {0x07,  4}, // 0xBA = 186
    {0x03,  2}, // 0xBB = 187
    {0x19,  9}, // 0xBC = 188
    {0x18,  8}, // 0xBD = 189
    {0x06,  3}, // 0xBE = 190
    {0x01,  1}, // 0xBF = 191
    {0x03,  2}, // 0xC0 = 192
    {0x19,  9}, // 0xC1 = 193
    {0x19,  9}, // 0xC2 = 194
    {0x3C, 12}, // 0xC3 = 195
    {0x07,  4}, // 0xC4 = 196
    {0x1B, 10}, // 0xC5 = 197
    {0x1E, 11}, // 0xC6 = 198
    {0x19,  9}, // 0xC7 = 199
    {0x07,  4}, // 0xC8 = 200
    {0x1E, 11}, // 0xC9 = 201
    {0x1B, 10}, // 0xCA = 202
    {0x19,  9}, // 0xCB = 203
    {0x0F,  5}, // 0xCC = 204
    {0x07,  4}, // 0xCD = 205
    {0x07,  4}, // 0xCE = 206
    {0x03,  2}, // 0xCF = 207
    {0x07,  4}, // 0xD0 = 208
    {0x1B, 10}, // 0xD1 = 209
    {0x1E, 11}, // 0xD2 = 210
    {0x19,  9}, // 0xD3 = 211
    {0x17,  7}, // 0xD4 = 212
    {0x07,  4}, // 0xD5 = 213
    {0x16,  6}, // 0xD6 = 214
    {0x06,  3}, // 0xD7 = 215
    {0x1B, 10}, // 0xD8 = 216
    {0x19,  9}, // 0xD9 = 217
    {0x19,  9}, // 0xDA = 218
    {0x18,  8}, // 0xDB = 219
    {0x07,  4}, // 0xDC = 220
    {0x03,  2}, // 0xDD = 221
    {0x06,  3}, // 0xDE = 222
    {0x01,  1}, // 0xDF = 223
    {0x07,  4}, // 0xE0 = 224
    {0x1E, 11}, // 0xE1 = 225
    {0x1B, 10}, // 0xE2 = 226
    {0x19,  9}, // 0xE3 = 227
    {0x1B, 10}, // 0xE4 = 228
    {0x19,  9}, // 0xE5 = 229
    {0x19,  9}, // 0xE6 = 230
    {0x18,  8}, // 0xE7 = 231
    {0x17,  7}, // 0xE8 = 232
    {0x16,  6}, // 0xE9 = 233
    {0x07,  4}, // 0xEA = 234
    {0x06,  3}, // 0xEB = 235
    {0x07,  4}, // 0xEC = 236
    {0x06,  3}, // 0xED = 237
    {0x03,  2}, // 0xEE = 238
    {0x01,  1}, // 0xEF = 239
    {0x0F,  5}, // 0xF0 = 240
    {0x07,  4}, // 0xF1 = 241
    {0x07,  4}, // 0xF2 = 242
    {0x03,  2}, // 0xF3 = 243
    {0x07,  4}, // 0xF4 = 244
    {0x03,  2}, // 0xF5 = 245
    {0x06,  3}, // 0xF6 = 246
    {0x01,  1}, // 0xF7 = 247
    {0x07,  4}, // 0xF8 = 248
    {0x06,  3}, // 0xF9 = 249
    {0x03,  2}, // 0xFA = 250
    {0x01,  1}, // 0xFB = 251
    {0x03,  2}, // 0xFC = 252
    {0x01,  1}, // 0xFD = 253
    {0x01,  1}, // 0xFE = 254
    {0x00,  0}  // 0xFF = 255
};



////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collect non-XOR inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_GenerateNpnTable()
{
    int Table[256], Classes[16], Map[256];
    int i, k, nClasses = 0;
    for ( i = 0; i < 256; i++ )
        Table[i] = Extra_TruthCanonNPN( i, 3 );
    for ( i = 0; i < 256; i++ )
    {
        printf( "{" );
        Extra_PrintHex( stdout, (unsigned *)&Table[i], 3 );
        printf( ", " );
        // find the class
        for ( k = 0; k < nClasses; k++ )
            if ( Table[i] == Classes[k] )
                break;
        if ( k == nClasses )
            Classes[nClasses++] = Table[i];
        Map[i] = k;
        // print
        printf( "%2d}, // ", k );
        Extra_PrintHex( stdout, (unsigned *)&i, 3 );
        printf( " = %3d\n", i );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_StatsCollect( Gia_Man_t * p, int fVerbose )
{
    extern int Dar_LibReturnClass( unsigned uTruth );
    extern char ** Kit_DsdNpn4ClassNames();
    char ** pNames = Kit_DsdNpn4ClassNames();

    int Map[256] = {0};
    Vec_Wrd_t * vTruths = Vec_WrdStart( Gia_ManObjNum(p) );
    Vec_Wrd_t * vTemp = Vec_WrdStart( Gia_ManObjNum(p) );
    word Truth, TruthF;
    int iTruth, iTruthF;
    int iFan, iLut, i, k, Class;
    assert( Gia_ManHasMapping(p) );
    assert( Gia_ManLutSizeMax(p) < 4 );
    // collect truth tables
    Gia_ManForEachLut( p, iLut )
    {
        Truth = Gia_ObjComputeTruthTable6Lut( p, iLut, vTemp );
        Vec_WrdWriteEntry( vTruths, iLut, Truth );
    }
    Vec_WrdFree( vTemp );
    // collect pairs
    Gia_ManForEachLut( p, iLut )
    {
        Truth = Vec_WrdEntry( vTruths, iLut ) & 0xFF;
        iTruth = Npn3Table[Truth][1];
        assert( iTruth < 15 );
        Gia_LutForEachFanin( p, iLut, iFan, k )
        {
            TruthF = Vec_WrdEntry( vTruths, iFan ) & 0xFF;
            iTruthF = Npn3Table[TruthF][1];
            assert( iTruthF < 15 );
            Map[(iTruthF << 4) + iTruth]++;
        }
    }
    Gia_ManForEachCoDriverId( p, iFan, k )
    {
        TruthF = Vec_WrdEntry( vTruths, iFan );
        iTruthF = Npn3Table[TruthF & 0xFF][1];
        assert( iTruthF < 15 );
        Map[(iTruthF << 4)]++;
    }
    Vec_WrdFree( vTruths );
    // print statistics
    printf( "fi / fo" );
    for ( i = 0; i < 14; i++ )
        printf( "%6d ", i );
    printf( "\n" );
    for ( i = 0; i < 14; i++ )
    {
        printf( "%6d ", i );
        for ( k = 0; k < 14; k++ )
            if ( Map[(i << 4) | k] )
                printf( "%6d ", Map[(i << 4) | k] );
            else
                printf( "%6s ", "." );
        printf( "\n" );
    }
    // print class formulas
    printf( "\nClasses:\n" );
    for ( i = 0; i < 14; i++ )
    {
        for ( k = 0; k < 256; k++ )
            if ( Npn3Table[k][1] == i )
                break;
        assert( k < 256 );
        Class = Dar_LibReturnClass( (Npn3Table[k][0] << 8) | Npn3Table[k][0] );
        printf( "%2d : %s\n", i, pNames[Class] );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

