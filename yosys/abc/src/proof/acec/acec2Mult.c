/**CFile****************************************************************

  FileName    [acec2Mult.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acec2Mult.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"
#include "misc/vec/vecMem.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START


static int s_nFuncTruths4a = 192; // 0xACC0 -- F(a=0)  <== useful
static unsigned s_FuncTruths4a[192] = {
0xACC0, 0xCAA0, 0xB8C0, 0xE288, 0xE488, 0xD8A0, 0xBC80, 0xE828, 0xE848, 0xDA80, 0xE680, 0xE860,
0x5CC0, 0x3AA0, 0x74C0, 0x2E88, 0x4E88, 0x72A0, 0x7C40, 0x28E8, 0x48E8, 0x7A20, 0x6E08, 0x60E8,
0xA330, 0xC550, 0x8B0C, 0xD144, 0xB122, 0x8D0A, 0x80BC, 0xD414, 0xB212, 0x80DA, 0x80E6, 0x8E06,
0x5330, 0x3550, 0x470C, 0x1D44, 0x1B22, 0x270A, 0x407C, 0x14D4, 0x12B2, 0x207A, 0x086E, 0x068E,
0xCA0C, 0xAC0A, 0xE230, 0xB822, 0xD844, 0xE450, 0xC0AC, 0xA0CA, 0xE320, 0xB282, 0xD484, 0xE540,
0xCB08, 0x8E82, 0xC0B8, 0x88E2, 0xD940, 0xD490, 0x8E84, 0xAD08, 0xB290, 0xB920, 0x88E4, 0xA0D8,
0xC50C, 0xA30A, 0xD130, 0x8B22, 0x8D44, 0xB150, 0xC05C, 0xA03A, 0xD310, 0x82B2, 0x84D4, 0xB510,
0xC704, 0x828E, 0xC074, 0x882E, 0x9D04, 0x90D4, 0x848E, 0xA702, 0x90B2, 0x9B02, 0x884E, 0xA072,
0xC5FC, 0xA3FA, 0xD1FC, 0x8BEE, 0x8DEE, 0xB1FA, 0xCF5C, 0xAF3A, 0xDF1C, 0x8EBE, 0x8EDE, 0xBF1A,
0xF734, 0xB2BE, 0xF374, 0xBB2E, 0xBF26, 0xB2F6, 0xD4DE, 0xF752, 0xD4F6, 0xDF46, 0xDD4E, 0xF572,
0xCAFC, 0xACFA, 0xE2FC, 0xB8EE, 0xD8EE, 0xE4FA, 0xCFAC, 0xAFCA, 0xEF2C, 0xBE8E, 0xDE8E, 0xEF4A,
0xFB38, 0xBEB2, 0xF3B8, 0xBBE2, 0xFB62, 0xF6B2, 0xDED4, 0xFD58, 0xF6D4, 0xFD64, 0xDDE4, 0xF5D8,
0x0CCA, 0x0AAC, 0x30E2, 0x22B8, 0x44D8, 0x50E4, 0x3E02, 0x2B28, 0x4D48, 0x5E04, 0x7610, 0x7160,
0xF33A, 0xF55C, 0xCF2E, 0xDD74, 0xBB72, 0xAF4E, 0xC2FE, 0xD7D4, 0xB7B2, 0xA4FE, 0x98FE, 0x9F8E,
0x033A, 0x055C, 0x032E, 0x1174, 0x1172, 0x054E, 0x023E, 0x1714, 0x1712, 0x045E, 0x1076, 0x1706,
0xFCCA, 0xFAAC, 0xFCE2, 0xEEB8, 0xEED8, 0xFAE4, 0xFEC2, 0xEBE8, 0xEDE8, 0xFEA4, 0xFE98, 0xF9E8
};


/*
static int s_nFuncTruths4 = 192; // 0xF335 -- F(a=1)
static unsigned s_FuncTruths4[192] = {
0x0CCA, 0x0AAC, 0x30E2, 0x22B8, 0x44D8, 0x50E4, 0x3E02, 0x2B28, 0x4D48, 0x5E04, 0x7610, 0x7160,
0xF33A, 0xF55C, 0xCF2E, 0xDD74, 0xBB72, 0xAF4E, 0xC2FE, 0xD7D4, 0xB7B2, 0xA4FE, 0x98FE, 0x9F8E,
0x033A, 0x055C, 0x032E, 0x1174, 0x1172, 0x054E, 0x023E, 0x1714, 0x1712, 0x045E, 0x1076, 0x1706,
0xFCCA, 0xFAAC, 0xFCE2, 0xEEB8, 0xEED8, 0xFAE4, 0xFEC2, 0xEBE8, 0xEDE8, 0xFEA4, 0xFE98, 0xF9E8,
0xC0AC, 0xA0CA, 0xC0B8, 0x88E2, 0x88E4, 0xA0D8, 0xCA0C, 0xAC0A, 0xCB08, 0x8E82, 0x8E84, 0xAD08,
0xE320, 0xB282, 0xE230, 0xB822, 0xB920, 0xB290, 0xD484, 0xE540, 0xD490, 0xD940, 0xD844, 0xE450,
0xC05C, 0xA03A, 0xC074, 0x882E, 0x884E, 0xA072, 0xC50C, 0xA30A, 0xC704, 0x828E, 0x848E, 0xA702,
0xD310, 0x82B2, 0xD130, 0x8B22, 0x9B02, 0x90B2, 0x84D4, 0xB510, 0x90D4, 0x9D04, 0x8D44, 0xB150,
0xCF5C, 0xAF3A, 0xF374, 0xBB2E, 0xDD4E, 0xF572, 0xC5FC, 0xA3FA, 0xF734, 0xB2BE, 0xD4DE, 0xF752,
0xDF1C, 0x8EBE, 0xD1FC, 0x8BEE, 0xDF46, 0xD4F6, 0x8EDE, 0xBF1A, 0xB2F6, 0xBF26, 0x8DEE, 0xB1FA,
0xCFAC, 0xAFCA, 0xF3B8, 0xBBE2, 0xDDE4, 0xF5D8, 0xCAFC, 0xACFA, 0xFB38, 0xBEB2, 0xDED4, 0xFD58,
0xEF2C, 0xBE8E, 0xE2FC, 0xB8EE, 0xFD64, 0xF6D4, 0xDE8E, 0xEF4A, 0xF6B2, 0xFB62, 0xD8EE, 0xE4FA,
0xACC0, 0xCAA0, 0xB8C0, 0xE288, 0xE488, 0xD8A0, 0xBC80, 0xE828, 0xE848, 0xDA80, 0xE680, 0xE860,
0x5CC0, 0x3AA0, 0x74C0, 0x2E88, 0x4E88, 0x72A0, 0x7C40, 0x28E8, 0x48E8, 0x7A20, 0x6E08, 0x60E8,
0xA330, 0xC550, 0x8B0C, 0xD144, 0xB122, 0x8D0A, 0x80BC, 0xD414, 0xB212, 0x80DA, 0x80E6, 0x8E06,
0x5330, 0x3550, 0x470C, 0x1D44, 0x1B22, 0x270A, 0x407C, 0x14D4, 0x12B2, 0x207A, 0x086E, 0x068E
};
*/

static int s_nFuncTruths4b = 48; // 0xF3C0 -- F(a=b)  <== useful
static unsigned s_FuncTruths4b[48] = {
0xF3C0, 0xF5A0, 0xCFC0, 0xDD88, 0xBB88, 0xAFA0, 0xFC30, 0xFA50, 0xCCF0, 0xD8D8, 0xB8B8, 0xAAF0,
0xF0CC, 0xE4E4, 0xFC0C, 0xEE44, 0xAACC, 0xACAC, 0xE2E2, 0xF0AA, 0xCACA, 0xCCAA, 0xEE22, 0xFA0A,
0x3F0C, 0x5F0A, 0x3F30, 0x7722, 0x7744, 0x5F50, 0x30FC, 0x50FA, 0x33F0, 0x7272, 0x7474, 0x55F0,
0x0FCC, 0x4E4E, 0x0CFC, 0x44EE, 0x55CC, 0x5C5C, 0x2E2E, 0x0FAA, 0x3A3A, 0x33AA, 0x22EE, 0x0AFA
};

/*
static int s_nFuncTruths4 = 12; // 0x35AC -- F(a!=b)
static unsigned s_FuncTruths4[12] = {
0x35AC, 0x53CA, 0x1DB8, 0x47E2, 0x27E4, 0x1BD8, 0x3A5C, 0x5C3A, 0x4E72, 0x2E74, 0x724E, 0x742E
};
*/

static int s_nFuncTruths4 = 384; // 0x35C0 -- F(b=0, c=0)  <== useful
static unsigned s_FuncTruths4[384] = {
0x35C0, 0x53A0, 0x1DC0, 0x4788, 0x2788, 0x1BA0, 0x3C50, 0x5A30, 0x1CD0, 0x4878, 0x2878, 0x1AB0,
0x34C4, 0x606C, 0x3C44, 0x660C, 0x268C, 0x286C, 0x606A, 0x52A2, 0x486A, 0x468A, 0x660A, 0x5A22,
0x3AC0, 0x5CA0, 0x2EC0, 0x7488, 0x7288, 0x4EA0, 0x3CA0, 0x5AC0, 0x2CE0, 0x7848, 0x7828, 0x4AE0,
0x38C8, 0x6C60, 0x3C88, 0x66C0, 0x62C8, 0x6C28, 0x6A60, 0x58A8, 0x6A48, 0x64A8, 0x66A0, 0x5A88,
0xC530, 0xA350, 0xD10C, 0x8B44, 0x8D22, 0xB10A, 0xC350, 0xA530, 0xD01C, 0x84B4, 0x82D2, 0xB01A,
0xC434, 0x909C, 0xC344, 0x990C, 0x8C26, 0x82C6, 0x909A, 0xA252, 0x84A6, 0x8A46, 0x990A, 0xA522,
0xCA30, 0xAC50, 0xE20C, 0xB844, 0xD822, 0xE40A, 0xC3A0, 0xA5C0, 0xE02C, 0xB484, 0xD282, 0xE04A,
0xC838, 0x9C90, 0xC388, 0x99C0, 0xC862, 0xC682, 0x9A90, 0xA858, 0xA684, 0xA864, 0x99A0, 0xA588,
0x530C, 0x350A, 0x4730, 0x1D22, 0x1B44, 0x2750, 0x503C, 0x305A, 0x4370, 0x12D2, 0x14B4, 0x2570,
0x434C, 0x06C6, 0x443C, 0x0C66, 0x194C, 0x149C, 0x06A6, 0x252A, 0x129A, 0x192A, 0x0A66, 0x225A,
0xA30C, 0xC50A, 0x8B30, 0xD122, 0xB144, 0x8D50, 0xA03C, 0xC05A, 0x83B0, 0xD212, 0xB414, 0x85D0,
0x838C, 0xC606, 0x883C, 0xC066, 0x91C4, 0x9C14, 0xA606, 0x858A, 0x9A12, 0x91A2, 0xA066, 0x885A,
0xA3FC, 0xC5FA, 0x8BFC, 0xD1EE, 0xB1EE, 0x8DFA, 0xAF3C, 0xCF5A, 0x8FBC, 0xDE1E, 0xBE1E, 0x8FDA,
0xB3BC, 0xF636, 0xBB3C, 0xF366, 0xB3E6, 0xBE36, 0xF656, 0xD5DA, 0xDE56, 0xD5E6, 0xF566, 0xDD5A,
0x53FC, 0x35FA, 0x47FC, 0x1DEE, 0x1BEE, 0x27FA, 0x5F3C, 0x3F5A, 0x4F7C, 0x1EDE, 0x1EBE, 0x2F7A,
0x737C, 0x36F6, 0x773C, 0x3F66, 0x3B6E, 0x36BE, 0x56F6, 0x757A, 0x56DE, 0x5D6E, 0x5F66, 0x775A,
0x3FCA, 0x5FAC, 0x3FE2, 0x77B8, 0x77D8, 0x5FE4, 0x3CFA, 0x5AFC, 0x3EF2, 0x7B78, 0x7D78, 0x5EF4,
0x3ECE, 0x6F6C, 0x3CEE, 0x66FC, 0x76DC, 0x7D6C, 0x6F6A, 0x5EAE, 0x7B6A, 0x76BA, 0x66FA, 0x5AEE,
0xC03A, 0xA05C, 0xC02E, 0x8874, 0x8872, 0xA04E, 0xC30A, 0xA50C, 0xC20E, 0x8784, 0x8782, 0xA40E,
0xC232, 0x9390, 0xC322, 0x9930, 0x9832, 0x9382, 0x9590, 0xA454, 0x9584, 0x9854, 0x9950, 0xA544,
0xCF3A, 0xAF5C, 0xF32E, 0xBB74, 0xDD72, 0xF54E, 0xC3FA, 0xA5FC, 0xF23E, 0xB7B4, 0xD7D2, 0xF45E,
0xCE3E, 0x9F9C, 0xC3EE, 0x99FC, 0xDC76, 0xD7C6, 0x9F9A, 0xAE5E, 0xB7A6, 0xBA76, 0x99FA, 0xA5EE,
0x30CA, 0x50AC, 0x0CE2, 0x44B8, 0x22D8, 0x0AE4, 0x3C0A, 0x5A0C, 0x0EC2, 0x4B48, 0x2D28, 0x0EA4,
0x32C2, 0x6360, 0x3C22, 0x6630, 0x3298, 0x3928, 0x6560, 0x54A4, 0x5948, 0x5498, 0x6650, 0x5A44,
0xF3AC, 0xF5CA, 0xCFB8, 0xDDE2, 0xBBE4, 0xAFD8, 0xFA3C, 0xFC5A, 0xCBF8, 0xDED2, 0xBEB4, 0xADF8,
0xE3EC, 0xF6C6, 0xEE3C, 0xFC66, 0xB9EC, 0xBE9C, 0xF6A6, 0xE5EA, 0xDE9A, 0xD9EA, 0xFA66, 0xEE5A,
0xF35C, 0xF53A, 0xCF74, 0xDD2E, 0xBB4E, 0xAF72, 0xF53C, 0xF35A, 0xC7F4, 0xD2DE, 0xB4BE, 0xA7F2,
0xD3DC, 0xC6F6, 0xDD3C, 0xCF66, 0x9BCE, 0x9CBE, 0xA6F6, 0xB5BA, 0x9ADE, 0x9DAE, 0xAF66, 0xBB5A,
0x035C, 0x053A, 0x0374, 0x112E, 0x114E, 0x0572, 0x053C, 0x035A, 0x0734, 0x121E, 0x141E, 0x0752,
0x131C, 0x0636, 0x113C, 0x0366, 0x1346, 0x1436, 0x0656, 0x151A, 0x1256, 0x1526, 0x0566, 0x115A,
0x03AC, 0x05CA, 0x03B8, 0x11E2, 0x11E4, 0x05D8, 0x0A3C, 0x0C5A, 0x0B38, 0x1E12, 0x1E14, 0x0D58,
0x232C, 0x3606, 0x223C, 0x3066, 0x3164, 0x3614, 0x5606, 0x454A, 0x5612, 0x5162, 0x5066, 0x445A
};

/*
static int s_nFuncTruths4 = 96; // 0xFD80 -- F(d=0)
static unsigned s_FuncTruths4[96] = {
0xFD80, 0xFB80, 0xEF80, 0xF8D0, 0xF8B0, 0xE8F0, 0xECC4, 0xE8CC, 0xEC8C, 0xE8AA, 0xEAA2, 0xEA8A,
0xFE40, 0xFE20, 0xFE08, 0xF4E0, 0xF2E0, 0xF0E8, 0xDCC8, 0xCCE8, 0xCEC8, 0xAAE8, 0xBAA8, 0xAEA8,
0xF720, 0xF740, 0xDF08, 0xDF40, 0xBF20, 0xBF08, 0xF270, 0xF470, 0xD0F8, 0xD4F0, 0xB2F0, 0xB0F8,
0xC4EC, 0xD4CC, 0xCE4C, 0xDC4C, 0x8CEC, 0x8ECC, 0xB2AA, 0xA2EA, 0x8EAA, 0x8AEA, 0xBA2A, 0xAE2A,
0xFB10, 0xFD10, 0xEF04, 0xFD04, 0xFB02, 0xEF02, 0xF1B0, 0xF1D0, 0xE0F4, 0xF0D4, 0xF0B2, 0xE0F2,
0xC8DC, 0xCCD4, 0xCD8C, 0xCDC4, 0xC8CE, 0xCC8E, 0xAAB2, 0xA8BA, 0xAA8E, 0xA8AE, 0xABA2, 0xAB8A,
0x7F02, 0x7F04, 0x7F10, 0x70F2, 0x70F4, 0x71F0, 0x4CCE, 0x4DCC, 0x4CDC, 0x2BAA, 0x2AAE, 0x2ABA,
0x40FE, 0x20FE, 0x08FE, 0x4F0E, 0x2F0E, 0x0F8E, 0x7332, 0x33B2, 0x3B32, 0x55D4, 0x7554, 0x5D54,
};
*/

/*
static int s_nFuncTruths4 = 48; // 0xD728 -- F(e=0)
static unsigned s_FuncTruths4[48] = {
0xD728, 0xB748, 0x9F60, 0xD278, 0xB478, 0x96F0, 0xC66C, 0x96CC, 0x9C6C, 0x96AA, 0xA66A, 0x9A6A,
0xEB14, 0xED12, 0xF906, 0xE1B4, 0xE1D2, 0xF096, 0xC99C, 0xCC96, 0xC9C6, 0xAA96, 0xA99A, 0xA9A6,
0x7D82, 0x7B84, 0x6F90, 0x78D2, 0x78B4, 0x69F0, 0x6CC6, 0x69CC, 0x6C9C, 0x69AA, 0x6AA6, 0x6A9A,
0x41BE, 0x21DE, 0x09F6, 0x4B1E, 0x2D1E, 0x0F96, 0x6336, 0x3396, 0x3936, 0x5596, 0x6556, 0x5956
};
*/


static int s_nFuncTruths5 = 960; // 0xF335ACC0
static unsigned s_FuncTruths5[960] = {
0xF335ACC0, 0xF553CAA0, 0xCF1DB8C0, 0xDD47E288, 0xBB27E488, 0xAF1BD8A0, 0xC1FDBC80, 0xD4D7E828,
0xB2B7E848, 0xA1FBDA80, 0x89EFE680, 0x8E9FE860, 0xF3AC35C0, 0xF5CA53A0, 0xCFB81DC0, 0xDDE24788,
0xBBE42788, 0xAFD81BA0, 0xC1BCFD80, 0xD4E8D728, 0xB2E8B748, 0xA1DAFB80, 0x89E6EF80, 0x8EE89F60,
0xFA3C3C50, 0xFC5A5A30, 0xCB1CF8D0, 0xDE48D278, 0xBE28B478, 0xAD1AF8B0, 0xCBF81CD0, 0xDED24878,
0xBEB42878, 0xADF81AB0, 0x8EE896F0, 0x8E96E8F0, 0xE334ECC4, 0xF660C66C, 0xEE3C3C44, 0xFC66660C,
0xB926EC8C, 0xBE289C6C, 0xE3EC34C4, 0xF6C6606C, 0xB296E8CC, 0xB2E896CC, 0xB9EC268C, 0xBE9C286C,
0xF660A66A, 0xE552EAA2, 0xDE489A6A, 0xD946EA8A, 0xFA66660A, 0xEE5A5A22, 0xD4E896AA, 0xD496E8AA,
0xF6A6606A, 0xE5EA52A2, 0xD9EA468A, 0xDE9A486A, 0xF33A5CC0, 0xF55C3AA0, 0xCF2E74C0, 0xDD742E88,
0xBB724E88, 0xAF4E72A0, 0xC2FE7C40, 0xD7D428E8, 0xB7B248E8, 0xA4FE7A20, 0x98FE6E08, 0x9F8E60E8,
0xF35C3AC0, 0xF53A5CA0, 0xCF742EC0, 0xDD2E7488, 0xBB4E7288, 0xAF724EA0, 0xC27CFE40, 0xD728D4E8,
0xB748B2E8, 0xA47AFE20, 0x986EFE08, 0x9F608EE8, 0xF53C3CA0, 0xF35A5AC0, 0xC72CF4E0, 0xD278DE48,
0xB478BE28, 0xA74AF2E0, 0xC7F42CE0, 0xD2DE7848, 0xB4BE7828, 0xA7F24AE0, 0x96F08EE8, 0x968EF0E8,
0xD338DCC8, 0xC66CF660, 0xDD3C3C88, 0xCF6666C0, 0x9B62CEC8, 0x9C6CBE28, 0xD3DC38C8, 0xC6F66C60,
0x96B2CCE8, 0x96CCB2E8, 0x9BCE62C8, 0x9CBE6C28, 0xA66AF660, 0xB558BAA8, 0x9A6ADE48, 0x9D64AEA8,
0xAF6666A0, 0xBB5A5A88, 0x96AAD4E8, 0x96D4AAE8, 0xA6F66A60, 0xB5BA58A8, 0x9DAE64A8, 0x9ADE6A48,
0xFCC5A330, 0xFAA3C550, 0xFCD18B0C, 0xEE8BD144, 0xEE8DB122, 0xFAB18D0A, 0xFDC180BC, 0xE8EBD414,
0xE8EDB212, 0xFBA180DA, 0xEF8980E6, 0xE8F98E06, 0xFCA3C530, 0xFAC5A350, 0xFC8BD10C, 0xEED18B44,
0xEEB18D22, 0xFA8DB10A, 0xFD80C1BC, 0xE8D4EB14, 0xE8B2ED12, 0xFB80A1DA, 0xEF8089E6, 0xE88EF906,
0xFAC3C350, 0xFCA5A530, 0xF8D0CB1C, 0xED84E1B4, 0xEB82E1D2, 0xF8B0AD1A, 0xF8CBD01C, 0xEDE184B4,
0xEBE182D2, 0xF8ADB01A, 0xE88EF096, 0xE8F08E96, 0xECC4E334, 0xF990C99C, 0xEEC3C344, 0xFC99990C,
0xEC8CB926, 0xEB82C9C6, 0xECE3C434, 0xF9C9909C, 0xE8CCB296, 0xE8B2CC96, 0xECB98C26, 0xEBC982C6,
0xF990A99A, 0xEAA2E552, 0xED84A9A6, 0xEA8AD946, 0xFA99990A, 0xEEA5A522, 0xE8D4AA96, 0xE8AAD496,
0xF9A9909A, 0xEAE5A252, 0xEAD98A46, 0xEDA984A6, 0xFCCA5330, 0xFAAC3550, 0xFCE2470C, 0xEEB81D44,
0xEED81B22, 0xFAE4270A, 0xFEC2407C, 0xEBE814D4, 0xEDE812B2, 0xFEA4207A, 0xFE98086E, 0xF9E8068E,
0xFC53CA30, 0xFA35AC50, 0xFC47E20C, 0xEE1DB844, 0xEE1BD822, 0xFA27E40A, 0xFE40C27C, 0xEB14E8D4,
0xED12E8B2, 0xFE20A47A, 0xFE08986E, 0xF906E88E, 0xF5C3C3A0, 0xF3A5A5C0, 0xF4E0C72C, 0xE1B4ED84,
0xE1D2EB82, 0xF2E0A74A, 0xF4C7E02C, 0xE1EDB484, 0xE1EBD282, 0xF2A7E04A, 0xF096E88E, 0xF0E8968E,
0xDCC8D338, 0xC99CF990, 0xDDC3C388, 0xCF9999C0, 0xCEC89B62, 0xC9C6EB82, 0xDCD3C838, 0xC9F99C90,
0xCCE896B2, 0xCC96E8B2, 0xCE9BC862, 0xC9EBC682, 0xA99AF990, 0xBAA8B558, 0xA9A6ED84, 0xAEA89D64,
0xAF9999A0, 0xBBA5A588, 0xAA96E8D4, 0xAAE896D4, 0xA9F99A90, 0xBAB5A858, 0xAE9DA864, 0xA9EDA684,
0x3F53CA0C, 0x5F35AC0A, 0x3F47E230, 0x771DB822, 0x771BD844, 0x5F27E450, 0x35F3C0AC, 0x53F5A0CA,
0x34F7E320, 0x717DB282, 0x717BD484, 0x52F7E540, 0x1CDFCB08, 0x4D7D8E82, 0x1DCFC0B8, 0x47DD88E2,
0x46DFD940, 0x4D6FD490, 0x2B7B8E84, 0x1ABFAD08, 0x2B6FB290, 0x26BFB920, 0x27BB88E4, 0x1BAFA0D8,
0x3FCA530C, 0x5FAC350A, 0x3FE24730, 0x77B81D22, 0x77D81B44, 0x5FE42750, 0x35C0F3AC, 0x53A0F5CA,
0x34E3F720, 0x71B27D82, 0x71D47B84, 0x52E5F740, 0x1CCBDF08, 0x4D8E7D82, 0x1DC0CFB8, 0x4788DDE2,
0x46D9DF40, 0x4DD46F90, 0x2B8E7B84, 0x1AADBF08, 0x2BB26F90, 0x26B9BF20, 0x2788BBE4, 0x1BA0AFD8,
0x3C50FA3C, 0x5A30FC5A, 0x3E43F270, 0x7B1278D2, 0x7D1478B4, 0x5E25F470, 0x3CFA503C, 0x5AFC305A,
0x3EF24370, 0x7B7812D2, 0x7D7814B4, 0x5EF42570, 0x1CD0CBF8, 0x4878DED2, 0x1CCBD0F8, 0x48DE78D2,
0x4DD469F0, 0x4D69D4F0, 0x2878BEB4, 0x1AB0ADF8, 0x2B69B2F0, 0x2BB269F0, 0x28BE78B4, 0x1AADB0F8,
0x3E43CE4C, 0x6F066CC6, 0x3C44EE3C, 0x660CFC66, 0x7619DC4C, 0x7D146C9C, 0x34E3C4EC, 0x60F66CC6,
0x34C4E3EC, 0x606CF6C6, 0x7169D4CC, 0x71D469CC, 0x3ECE434C, 0x6F6C06C6, 0x3CEE443C, 0x66FC0C66,
0x76DC194C, 0x7D6C149C, 0x2B698ECC, 0x2B8E69CC, 0x286CBE9C, 0x268CB9EC, 0x26B98CEC, 0x28BE6C9C,
0x6F066AA6, 0x5E25AE2A, 0x7B126A9A, 0x7619BA2A, 0x660AFA66, 0x5A22EE5A, 0x60F66AA6, 0x52E5A2EA,
0x71B269AA, 0x7169B2AA, 0x606AF6A6, 0x52A2E5EA, 0x4D8E69AA, 0x4D698EAA, 0x48DE6A9A, 0x46D98AEA,
0x468AD9EA, 0x486ADE9A, 0x6F6A06A6, 0x5EAE252A, 0x7B6A129A, 0x76BA192A, 0x66FA0A66, 0x5AEE225A,
0x3FA3C50C, 0x5FC5A30A, 0x3F8BD130, 0x77D18B22, 0x77B18D44, 0x5F8DB150, 0x3AF3C05C, 0x5CF5A03A,
0x38FBD310, 0x7D7182B2, 0x7B7184D4, 0x58FDB510, 0x2CEFC704, 0x7D4D828E, 0x2ECFC074, 0x74DD882E,
0x64FD9D04, 0x6F4D90D4, 0x7B2B848E, 0x4AEFA702, 0x6F2B90B2, 0x62FB9B02, 0x72BB884E, 0x4EAFA072,
0x3FC5A30C, 0x5FA3C50A, 0x3FD18B30, 0x778BD122, 0x778DB144, 0x5FB18D50, 0x3AC0F35C, 0x5CA0F53A,
0x38D3FB10, 0x7D8271B2, 0x7B8471D4, 0x58B5FD10, 0x2CC7EF04, 0x7D824D8E, 0x2EC0CF74, 0x7488DD2E,
0x649DFD04, 0x6F904DD4, 0x7B842B8E, 0x4AA7EF02, 0x6F902BB2, 0x629BFB02, 0x7288BB4E, 0x4EA0AF72,
0x3CA0F53C, 0x5AC0F35A, 0x3D83F1B0, 0x78D27B12, 0x78B47D14, 0x5B85F1D0, 0x3CF5A03C, 0x5AF3C05A,
0x3DF183B0, 0x787BD212, 0x787DB414, 0x5BF185D0, 0x2CE0C7F4, 0x7848D2DE, 0x2CC7E0F4, 0x78D248DE,
0x69F04DD4, 0x694DF0D4, 0x7828B4BE, 0x4AE0A7F2, 0x692BF0B2, 0x69F02BB2, 0x78B428BE, 0x4AA7E0F2,
0x3D83CD8C, 0x6CC66F06, 0x3C88DD3C, 0x66C0CF66, 0x6791CDC4, 0x6C9C7D14, 0x38D3C8DC, 0x6CC660F6,
0x38C8D3DC, 0x6C60C6F6, 0x6971CCD4, 0x69CC71D4, 0x3DCD838C, 0x6C6FC606, 0x3CDD883C, 0x66CFC066,
0x67CD91C4, 0x6C7D9C14, 0x692BCC8E, 0x69CC2B8E, 0x6C289CBE, 0x62C89BCE, 0x629BC8CE, 0x6C9C28BE,
0x6AA66F06, 0x5B85AB8A, 0x6A9A7B12, 0x6791ABA2, 0x66A0AF66, 0x5A88BB5A, 0x6AA660F6, 0x58B5A8BA,
0x69AA71B2, 0x6971AAB2, 0x6A60A6F6, 0x58A8B5BA, 0x69AA4D8E, 0x694DAA8E, 0x6A9A48DE, 0x649DA8AE,
0x64A89DAE, 0x6A489ADE, 0x6A6FA606, 0x5BAB858A, 0x6A7B9A12, 0x67AB91A2, 0x66AFA066, 0x5ABB885A,
0x30A3C5FC, 0x50C5A3FA, 0x0C8BD1FC, 0x44D18BEE, 0x22B18DEE, 0x0A8DB1FA, 0x3A03CF5C, 0x5C05AF3A,
0x08CBDF1C, 0x4D418EBE, 0x2B218EDE, 0x08ADBF1A, 0x20E3F734, 0x7141B2BE, 0x2E03F374, 0x7411BB2E,
0x20B9BF26, 0x2B09B2F6, 0x7121D4DE, 0x40E5F752, 0x4D09D4F6, 0x40D9DF46, 0x7211DD4E, 0x4E05F572,
0x30C5A3FC, 0x50A3C5FA, 0x0CD18BFC, 0x448BD1EE, 0x228DB1EE, 0x0AB18DFA, 0x3ACF035C, 0x5CAF053A,
0x08DFCB1C, 0x4D8E41BE, 0x2B8E21DE, 0x08BFAD1A, 0x20F7E334, 0x71B241BE, 0x2EF30374, 0x74BB112E,
0x20BFB926, 0x2BB209F6, 0x71D421DE, 0x40F7E552, 0x4DD409F6, 0x40DFD946, 0x72DD114E, 0x4EF50572,
0x3CAF053C, 0x5ACF035A, 0x0D8FC1BC, 0x48DE4B1E, 0x28BE2D1E, 0x0B8FA1DA, 0x3C05AF3C, 0x5A03CF5A,
0x0DC18FBC, 0x484BDE1E, 0x282DBE1E, 0x0BA18FDA, 0x2FE30734, 0x7B4B121E, 0x2F07E334, 0x7B124B1E,
0x2BB20F96, 0x2B0FB296, 0x7D2D141E, 0x4FE50752, 0x4D0FD496, 0x4DD40F96, 0x7D142D1E, 0x4F07E552,
0x31B3C1BC, 0x60F66336, 0x3CBB113C, 0x66F30366, 0x23B389E6, 0x28BE3936, 0x3B13CB1C, 0x6F066336,
0x3BCB131C, 0x6F630636, 0x2B338E96, 0x2B8E3396, 0x31C1B3BC, 0x6063F636, 0x3C11BB3C, 0x6603F366,
0x2389B3E6, 0x2839BE36, 0x7133D496, 0x71D43396, 0x7D391436, 0x73D91346, 0x7313D946, 0x7D143936,
0x60F66556, 0x51D5A1DA, 0x48DE5956, 0x45D589E6, 0x66F50566, 0x5ADD115A, 0x6F066556, 0x5D15AD1A,
0x4D8E5596, 0x4D558E96, 0x6F650656, 0x5DAD151A, 0x71B25596, 0x7155B296, 0x7B125956, 0x7515B926,
0x75B91526, 0x7B591256, 0x6065F656, 0x51A1D5DA, 0x4859DE56, 0x4589D5E6, 0x6605F566, 0x5A11DD5A,
0x3053CAFC, 0x5035ACFA, 0x0C47E2FC, 0x441DB8EE, 0x221BD8EE, 0x0A27E4FA, 0x3503CFAC, 0x5305AFCA,
0x04C7EF2C, 0x414DBE8E, 0x212BDE8E, 0x02A7EF4A, 0x10D3FB38, 0x4171BEB2, 0x1D03F3B8, 0x4711BBE2,
0x029BFB62, 0x092BF6B2, 0x2171DED4, 0x10B5FD58, 0x094DF6D4, 0x049DFD64, 0x2711DDE4, 0x1B05F5D8,
0x30CA53FC, 0x50AC35FA, 0x0CE247FC, 0x44B81DEE, 0x22D81BEE, 0x0AE427FA, 0x35CF03AC, 0x53AF05CA,
0x04EFC72C, 0x41BE4D8E, 0x21DE2B8E, 0x02EFA74A, 0x10FBD338, 0x41BE71B2, 0x1DF303B8, 0x47BB11E2,
0x02FB9B62, 0x09F62BB2, 0x21DE71D4, 0x10FDB558, 0x09F64DD4, 0x04FD9D64, 0x27DD11E4, 0x1BF505D8,
0x3C5F0A3C, 0x5A3F0C5A, 0x0E4FC27C, 0x4B1E48DE, 0x2D1E28BE, 0x0E2FA47A, 0x3C0A5F3C, 0x5A0C3F5A,
0x0EC24F7C, 0x4B481EDE, 0x2D281EBE, 0x0EA42F7A, 0x1FD30B38, 0x4B7B1E12, 0x1F0BD338, 0x4B1E7B12,
0x0F962BB2, 0x0F2B96B2, 0x2D7D1E14, 0x1FB50D58, 0x0F4D96D4, 0x0F964DD4, 0x2D1E7D14, 0x1F0DB558,
0x3273C27C, 0x633660F6, 0x3C77223C, 0x663F3066, 0x323B986E, 0x393628BE, 0x3723C72C, 0x63366F06,
0x37C7232C, 0x636F3606, 0x332B968E, 0x33962B8E, 0x32C2737C, 0x636036F6, 0x3C22773C, 0x66303F66,
0x32983B6E, 0x392836BE, 0x337196D4, 0x339671D4, 0x397D3614, 0x379D3164, 0x37319D64, 0x39367D14,
0x655660F6, 0x5475A47A, 0x595648DE, 0x545D986E, 0x665F5066, 0x5A77445A, 0x65566F06, 0x5745A74A,
0x55964D8E, 0x554D968E, 0x656F5606, 0x57A7454A, 0x559671B2, 0x557196B2, 0x59567B12, 0x57519B62,
0x579B5162, 0x597B5612, 0x656056F6, 0x54A4757A, 0x594856DE, 0x54985D6E, 0x66505F66, 0x5A44775A,
0x533F0CCA, 0x355F0AAC, 0x473F30E2, 0x1D7722B8, 0x1B7744D8, 0x275F50E4, 0x437F3E02, 0x17D72B28,
0x17B74D48, 0x257F5E04, 0x197F7610, 0x179F7160, 0x530C3FCA, 0x350A5FAC, 0x47303FE2, 0x1D2277B8,
0x1B4477D8, 0x27505FE4, 0x433E7F02, 0x172BD728, 0x174DB748, 0x255E7F04, 0x19767F10, 0x17719F60,
0x503C3CFA, 0x305A5AFC, 0x433E70F2, 0x127BD278, 0x147DB478, 0x255E70F4, 0x43703EF2, 0x12D27B78,
0x14B47D78, 0x25705EF4, 0x177196F0, 0x179671F0, 0x433E4CCE, 0x066FC66C, 0x443C3CEE, 0x0C6666FC,
0x19764CDC, 0x147D9C6C, 0x434C3ECE, 0x06C66F6C, 0x17964DCC, 0x174D96CC, 0x194C76DC, 0x149C7D6C,
0x066FA66A, 0x255E2AAE, 0x127B9A6A, 0x19762ABA, 0x0A6666FA, 0x225A5AEE, 0x172B96AA, 0x17962BAA,
0x06A66F6A, 0x252A5EAE, 0x192A76BA, 0x129A7B6A, 0x5CC0F33A, 0x3AA0F55C, 0x74C0CF2E, 0x2E88DD74,
0x4E88BB72, 0x72A0AF4E, 0x7C40C2FE, 0x28E8D7D4, 0x48E8B7B2, 0x7A20A4FE, 0x6E0898FE, 0x60E89F8E,
0x5CF3C03A, 0x3AF5A05C, 0x74CFC02E, 0x2EDD8874, 0x4EBB8872, 0x72AFA04E, 0x7CC240FE, 0x28D7E8D4,
0x48B7E8B2, 0x7AA420FE, 0x6E9808FE, 0x609FE88E, 0x5FC3C30A, 0x3FA5A50C, 0x7CC24F0E, 0x2D87ED84,
0x4B87EB82, 0x7AA42F0E, 0x7C4FC20E, 0x2DED8784, 0x4BEB8782, 0x7A2FA40E, 0x690FE88E, 0x69E80F8E,
0x7CC27332, 0x3993F990, 0x77C3C322, 0x3F999930, 0x6E983B32, 0x6393EB82, 0x7C73C232, 0x39F99390,
0x69E833B2, 0x6933E8B2, 0x6E3B9832, 0x63EB9382, 0x5995F990, 0x7AA47554, 0x6595ED84, 0x6E985D54,
0x5F999950, 0x77A5A544, 0x6955E8D4, 0x69E855D4, 0x59F99590, 0x7A75A454, 0x6E5D9854, 0x65ED9584,
0x5CCF033A, 0x3AAF055C, 0x74F3032E, 0x2EBB1174, 0x4EDD1172, 0x72F5054E, 0x7F43023E, 0x2BEB1714,
0x4DED1712, 0x7F25045E, 0x7F191076, 0x71F91706, 0x5C03CF3A, 0x3A05AF5C, 0x7403F32E, 0x2E11BB74,
0x4E11DD72, 0x7205F54E, 0x7F02433E, 0x2B17EB14, 0x4D17ED12, 0x7F04255E, 0x7F101976, 0x7117F906,
0x50C3C3FA, 0x30A5A5FC, 0x70F2433E, 0x21B7E1B4, 0x41D7E1D2, 0x70F4255E, 0x7043F23E, 0x21E1B7B4,
0x41E1D7D2, 0x7025F45E, 0x7117F096, 0x71F01796, 0x4CCE433E, 0x099FC99C, 0x44C3C3EE, 0x0C9999FC,
0x4CDC1976, 0x41D7C9C6, 0x4C43CE3E, 0x09C99F9C, 0x4DCC1796, 0x4D17CC96, 0x4C19DC76, 0x41C9D7C6,
0x099FA99A, 0x2AAE255E, 0x21B7A9A6, 0x2ABA1976, 0x0A9999FA, 0x22A5A5EE, 0x2B17AA96, 0x2BAA1796,
0x09A99F9A, 0x2A25AE5E, 0x2A19BA76, 0x21A9B7A6, 0x5330FCCA, 0x3550FAAC, 0x470CFCE2, 0x1D44EEB8,
0x1B22EED8, 0x270AFAE4, 0x407CFEC2, 0x14D4EBE8, 0x12B2EDE8, 0x207AFEA4, 0x086EFE98, 0x068EF9E8,
0x53FC30CA, 0x35FA50AC, 0x47FC0CE2, 0x1DEE44B8, 0x1BEE22D8, 0x27FA0AE4, 0x40FE7CC2, 0x14EBD4E8,
0x12EDB2E8, 0x20FE7AA4, 0x08FE6E98, 0x06F98EE8, 0x5F3C3C0A, 0x3F5A5A0C, 0x4F0E7CC2, 0x1E4BDE48,
0x1E2DBE28, 0x2F0E7AA4, 0x4F7C0EC2, 0x1EDE4B48, 0x1EBE2D28, 0x2F7A0EA4, 0x0F698EE8, 0x0F8E69E8,
0x73327CC2, 0x3663F660, 0x773C3C22, 0x3F666630, 0x3B326E98, 0x3639BE28, 0x737C32C2, 0x36F66360,
0x33B269E8, 0x3369B2E8, 0x3B6E3298, 0x36BE3928, 0x5665F660, 0x75547AA4, 0x5659DE48, 0x5D546E98,
0x5F666650, 0x775A5A44, 0x5569D4E8, 0x55D469E8, 0x56F66560, 0x757A54A4, 0x5D6E5498, 0x56DE5948
};

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define SDB_MAX_CUTSIZE    6
#define SDB_MAX_CUTNUM     51
#define SDB_MAX_TT_WORDS   ((SDB_MAX_CUTSIZE > 6) ? 1 << (SDB_MAX_CUTSIZE-6) : 1)

#define SDB_CUT_NO_LEAF   0xF

typedef struct Sdb_Cut_t_ Sdb_Cut_t; 
struct Sdb_Cut_t_
{
    word            Sign;                      // signature
    int             iFunc;                     // functionality
    int             Cost;                      // cut cost
    int             CostLev;                   // cut cost
    unsigned        nTreeLeaves  : 28;         // tree leaves
    unsigned        nLeaves      :  4;         // leaf count
    int             pLeaves[SDB_MAX_CUTSIZE];  // leaves
};

typedef struct Sdb_Sto_t_ Sdb_Sto_t; 
struct Sdb_Sto_t_
{
    int             nCutSize;
    int             nCutNum;
    int             fCutMin;
    int             fTruthMin;
    int             fVerbose;
    Gia_Man_t *     pGia;                      // user's AIG manager (will be modified by adding nodes)
    Vec_Int_t *     vRefs;                     // refs for each node
    Vec_Wec_t *     vCuts;                     // cuts for each node
    Vec_Mem_t *     vTtMem;                    // truth tables
    Sdb_Cut_t       pCuts[3][SDB_MAX_CUTNUM];  // temporary cuts
    Sdb_Cut_t *     ppCuts[SDB_MAX_CUTNUM];    // temporary cut pointers
    int             nCutsR;                    // the number of cuts
    int             Pivot;                     // current object
    int             iCutBest;                  // best-delay cut
    int             nCutsOver;                 // overflow cuts
    double          CutCount[4];               // cut counters
    abctime         clkStart;                  // starting time
};

static inline word * Sdb_CutTruth( Sdb_Sto_t * p, Sdb_Cut_t * pCut ) { return Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut->iFunc));           }

#define Sdb_ForEachCut( pList, pCut, i ) for ( i = 0, pCut = pList + 1; i < pList[0]; i++, pCut += pCut[0] + 2 )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Check correctness of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Sdb_CutGetSign( Sdb_Cut_t * pCut )
{
    word Sign = 0; int i; 
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        Sign |= ((word)1) << (pCut->pLeaves[i] & 0x3F);
    return Sign;
}
static inline int Sdb_CutCheck( Sdb_Cut_t * pBase, Sdb_Cut_t * pCut ) // check if pCut is contained in pBase
{
    int nSizeB = pBase->nLeaves;
    int nSizeC = pCut->nLeaves;
    int i, * pB = pBase->pLeaves;
    int k, * pC = pCut->pLeaves;
    for ( i = 0; i < nSizeC; i++ )
    {
        for ( k = 0; k < nSizeB; k++ )
            if ( pC[i] == pB[k] )
                break;
        if ( k == nSizeB )
            return 0;
    }
    return 1;
}
static inline int Sdb_CutSetCheckArray( Sdb_Cut_t ** ppCuts, int nCuts )
{
    Sdb_Cut_t * pCut0, * pCut1; 
    int i, k, m, n, Value;
    assert( nCuts > 0 );
    for ( i = 0; i < nCuts; i++ )
    {
        pCut0 = ppCuts[i];
        assert( pCut0->nLeaves <= SDB_MAX_CUTSIZE );
        assert( pCut0->Sign == Sdb_CutGetSign(pCut0) );
        // check duplicates
        for ( m = 0; m < (int)pCut0->nLeaves; m++ )
        for ( n = m + 1; n < (int)pCut0->nLeaves; n++ )
            assert( pCut0->pLeaves[m] < pCut0->pLeaves[n] );
        // check pairs
        for ( k = 0; k < nCuts; k++ )
        {
            pCut1 = ppCuts[k];
            if ( pCut0 == pCut1 )
                continue;
            // check containments
            Value = Sdb_CutCheck( pCut0, pCut1 );
            assert( Value == 0 );
        }
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Sdb_CutMergeOrder( Sdb_Cut_t * pCut0, Sdb_Cut_t * pCut1, Sdb_Cut_t * pCut, int nCutSize )
{ 
    int nSize0   = pCut0->nLeaves;
    int nSize1   = pCut1->nLeaves;
    int i, * pC0 = pCut0->pLeaves;
    int k, * pC1 = pCut1->pLeaves;
    int c, * pC  = pCut->pLeaves;
    // the case of the largest cut sizes
    if ( nSize0 == nCutSize && nSize1 == nCutSize )
    {
        for ( i = 0; i < nSize0; i++ )
        {
            if ( pC0[i] != pC1[i] )  return 0;
            pC[i] = pC0[i];
        }
        pCut->nLeaves = nCutSize;
        pCut->iFunc = -1;
        pCut->Sign = pCut0->Sign | pCut1->Sign;
        return 1;
    }
    // compare two cuts with different numbers
    i = k = c = 0;
    if ( nSize0 == 0 ) goto FlushCut1;
    if ( nSize1 == 0 ) goto FlushCut0;
    while ( 1 )
    {
        if ( c == nCutSize ) return 0;
        if ( pC0[i] < pC1[k] )
        {
            pC[c++] = pC0[i++];
            if ( i >= nSize0 ) goto FlushCut1;
        }
        else if ( pC0[i] > pC1[k] )
        {
            pC[c++] = pC1[k++];
            if ( k >= nSize1 ) goto FlushCut0;
        }
        else
        {
            pC[c++] = pC0[i++]; k++;
            if ( i >= nSize0 ) goto FlushCut1;
            if ( k >= nSize1 ) goto FlushCut0;
        }
    }

FlushCut0:
    if ( c + nSize0 > nCutSize + i ) return 0;
    while ( i < nSize0 )
        pC[c++] = pC0[i++];
    pCut->nLeaves = c;
    pCut->iFunc = -1;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;

FlushCut1:
    if ( c + nSize1 > nCutSize + k ) return 0;
    while ( k < nSize1 )
        pC[c++] = pC1[k++];
    pCut->nLeaves = c;
    pCut->iFunc = -1;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;
}
static inline int Sdb_CutMergeOrder2( Sdb_Cut_t * pCut0, Sdb_Cut_t * pCut1, Sdb_Cut_t * pCut, int nCutSize )
{ 
    int x0, i0 = 0, nSize0 = pCut0->nLeaves, * pC0 = pCut0->pLeaves;
    int x1, i1 = 0, nSize1 = pCut1->nLeaves, * pC1 = pCut1->pLeaves;
    int xMin, c = 0, * pC  = pCut->pLeaves;
    while ( 1 )
    {
        x0 = (i0 == nSize0) ? ABC_INFINITY : pC0[i0];
        x1 = (i1 == nSize1) ? ABC_INFINITY : pC1[i1];
        xMin = Abc_MinInt(x0, x1);
        if ( xMin == ABC_INFINITY ) break;
        if ( c == nCutSize ) return 0;
        pC[c++] = xMin;
        if (x0 == xMin) i0++;
        if (x1 == xMin) i1++;
    }
    pCut->nLeaves = c;
    pCut->iFunc = -1;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;
}
static inline int Sdb_CutSetCutIsContainedOrder( Sdb_Cut_t * pBase, Sdb_Cut_t * pCut ) // check if pCut is contained in pBase
{
    int i, nSizeB = pBase->nLeaves;
    int k, nSizeC = pCut->nLeaves;
    if ( nSizeB == nSizeC )
    {
        for ( i = 0; i < nSizeB; i++ )
            if ( pBase->pLeaves[i] != pCut->pLeaves[i] )
                return 0;
        return 1;
    }
    assert( nSizeB > nSizeC ); 
    if ( nSizeC == 0 )
        return 1;
    for ( i = k = 0; i < nSizeB; i++ )
    {
        if ( pBase->pLeaves[i] > pCut->pLeaves[k] )
            return 0;
        if ( pBase->pLeaves[i] == pCut->pLeaves[k] )
        {
            if ( ++k == nSizeC )
                return 1;
        }
    }
    return 0;
}
static inline int Sdb_CutSetLastCutIsContained( Sdb_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[i]->nLeaves <= pCuts[nCuts]->nLeaves && (pCuts[i]->Sign & pCuts[nCuts]->Sign) == pCuts[i]->Sign && Sdb_CutSetCutIsContainedOrder(pCuts[nCuts], pCuts[i]) )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Sdb_CutCompare( Sdb_Cut_t * pCut0, Sdb_Cut_t * pCut1 )
{
    if ( pCut0->nTreeLeaves < pCut1->nTreeLeaves )  return -1;
    if ( pCut0->nTreeLeaves > pCut1->nTreeLeaves )  return  1;
    if ( pCut0->nLeaves     < pCut1->nLeaves )      return -1;
    if ( pCut0->nLeaves     > pCut1->nLeaves )      return  1;
    return 0;
}
static inline int Sdb_CutSetLastCutContains( Sdb_Cut_t ** pCuts, int nCuts )
{
    int i, k, fChanges = 0;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[nCuts]->nLeaves < pCuts[i]->nLeaves && (pCuts[nCuts]->Sign & pCuts[i]->Sign) == pCuts[nCuts]->Sign && Sdb_CutSetCutIsContainedOrder(pCuts[i], pCuts[nCuts]) )
            pCuts[i]->nLeaves = SDB_CUT_NO_LEAF, fChanges = 1;
    if ( !fChanges )
        return nCuts;
    for ( i = k = 0; i <= nCuts; i++ )
    {
        if ( pCuts[i]->nLeaves == SDB_CUT_NO_LEAF )
            continue;
        if ( k < i )
            ABC_SWAP( Sdb_Cut_t *, pCuts[k], pCuts[i] );
        k++;
    }
    return k - 1;
}
static inline void Sdb_CutSetSortByCost( Sdb_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = nCuts; i > 0; i-- )
    {
        if ( Sdb_CutCompare(pCuts[i - 1], pCuts[i]) < 0 )//!= 1 )
            return;
        ABC_SWAP( Sdb_Cut_t *, pCuts[i - 1], pCuts[i] );
    }
}
static inline int Sdb_CutSetAddCut( Sdb_Cut_t ** pCuts, int nCuts, int nCutNum )
{
    if ( nCuts == 0 )
        return 1;
    nCuts = Sdb_CutSetLastCutContains(pCuts, nCuts);
    assert( nCuts >= 0 );
    Sdb_CutSetSortByCost( pCuts, nCuts );
    // add new cut if there is room
    return Abc_MinInt( nCuts + 1, nCutNum - 1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Sdb_CutComputeTruth6( Sdb_Sto_t * p, Sdb_Cut_t * pCut0, Sdb_Cut_t * pCut1, int fCompl0, int fCompl1, Sdb_Cut_t * pCutR, int fIsXor )
{
    int nOldSupp = pCutR->nLeaves, truthId, fCompl; word t;
    word t0 = *Sdb_CutTruth(p, pCut0);
    word t1 = *Sdb_CutTruth(p, pCut1);
    if ( Abc_LitIsCompl(pCut0->iFunc) ^ fCompl0 ) t0 = ~t0;
    if ( Abc_LitIsCompl(pCut1->iFunc) ^ fCompl1 ) t1 = ~t1;
    t0 = Abc_Tt6Expand( t0, pCut0->pLeaves, pCut0->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    t1 = Abc_Tt6Expand( t1, pCut1->pLeaves, pCut1->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    t =  fIsXor ? t0 ^ t1 : t0 & t1;
    if ( (fCompl = (int)(t & 1)) ) t = ~t;
    if ( p->fTruthMin )
        pCutR->nLeaves = Abc_Tt6MinBase( &t, pCutR->pLeaves, pCutR->nLeaves );
    assert( (int)(t & 1) == 0 );
    truthId        = Vec_MemHashInsert(p->vTtMem, &t);
    pCutR->iFunc   = Abc_Var2Lit( truthId, fCompl );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
}
static inline int Sdb_CutComputeTruth( Sdb_Sto_t * p, Sdb_Cut_t * pCut0, Sdb_Cut_t * pCut1, int fCompl0, int fCompl1, Sdb_Cut_t * pCutR, int fIsXor )
{
    if ( p->nCutSize <= 6 )
        return Sdb_CutComputeTruth6( p, pCut0, pCut1, fCompl0, fCompl1, pCutR, fIsXor );
    {
    word uTruth[SDB_MAX_TT_WORDS], uTruth0[SDB_MAX_TT_WORDS], uTruth1[SDB_MAX_TT_WORDS];
    int nOldSupp   = pCutR->nLeaves, truthId;
    int nCutSize   = p->nCutSize, fCompl;
    int nWords     = Abc_Truth6WordNum(nCutSize);
    word * pTruth0 = Sdb_CutTruth(p, pCut0);
    word * pTruth1 = Sdb_CutTruth(p, pCut1);
    Abc_TtCopy( uTruth0, pTruth0, nWords, Abc_LitIsCompl(pCut0->iFunc) ^ fCompl0 );
    Abc_TtCopy( uTruth1, pTruth1, nWords, Abc_LitIsCompl(pCut1->iFunc) ^ fCompl1 );
    Abc_TtExpand( uTruth0, nCutSize, pCut0->pLeaves, pCut0->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    Abc_TtExpand( uTruth1, nCutSize, pCut1->pLeaves, pCut1->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    if ( fIsXor )
        Abc_TtXor( uTruth, uTruth0, uTruth1, nWords, (fCompl = (int)((uTruth0[0] ^ uTruth1[0]) & 1)) );
    else
        Abc_TtAnd( uTruth, uTruth0, uTruth1, nWords, (fCompl = (int)((uTruth0[0] & uTruth1[0]) & 1)) );
    if ( p->fTruthMin )
        pCutR->nLeaves = Abc_TtMinBase( uTruth, pCutR->pLeaves, pCutR->nLeaves, nCutSize );
    assert( (uTruth[0] & 1) == 0 );
//Kit_DsdPrintFromTruth( uTruth, pCutR->nLeaves ), printf("\n" ), printf("\n" );
    truthId        = Vec_MemHashInsert(p->vTtMem, uTruth);
    pCutR->iFunc   = Abc_Var2Lit( truthId, fCompl );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Sdb_CutCountBits( word i )
{
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    i = ((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0F);
    return (i*(0x0101010101010101))>>56;
}
static inline void Sdb_CutAddUnit( Sdb_Sto_t * p, int iObj )
{
    Vec_Int_t * vThis = Vec_WecEntry( p->vCuts, iObj );
    if ( Vec_IntSize(vThis) == 0 )
        Vec_IntPush( vThis, 1 );
    else 
        Vec_IntAddToEntry( vThis, 0, 1 );
    Vec_IntPush( vThis, 1 );
    Vec_IntPush( vThis, iObj );
    Vec_IntPush( vThis, 2 );
}
static inline void Sdb_CutAddZero( Sdb_Sto_t * p, int iObj )
{
    Vec_Int_t * vThis = Vec_WecEntry( p->vCuts, iObj );
    assert( Vec_IntSize(vThis) == 0 );
    Vec_IntPush( vThis, 1 );
    Vec_IntPush( vThis, 0 );
    Vec_IntPush( vThis, 0 );
}
static inline int Sdb_CutTreeLeaves( Sdb_Sto_t * p, Sdb_Cut_t * pCut )
{
    int i, Cost = 0;
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        Cost += Vec_IntEntry( p->vRefs, pCut->pLeaves[i] ) == 1;
    return Cost;
}
static inline int Sdb_StoPrepareSet( Sdb_Sto_t * p, int iObj, int Index )
{
    Vec_Int_t * vThis = Vec_WecEntry( p->vCuts, iObj );
    int i, v, * pCut, * pList = Vec_IntArray( vThis );
    Sdb_ForEachCut( pList, pCut, i )
    {
        Sdb_Cut_t * pCutTemp = &p->pCuts[Index][i];
        pCutTemp->nLeaves = pCut[0];
        for ( v = 1; v <= pCut[0]; v++ )
            pCutTemp->pLeaves[v-1] = pCut[v];
        pCutTemp->iFunc = pCut[pCut[0]+1];
        pCutTemp->Sign = Sdb_CutGetSign( pCutTemp );
        pCutTemp->nTreeLeaves = Sdb_CutTreeLeaves( p, pCutTemp );
    }
    return pList[0];
}
static inline void Sdb_StoInitResult( Sdb_Sto_t * p )
{
    int i; 
    for ( i = 0; i < SDB_MAX_CUTNUM; i++ )
        p->ppCuts[i] = &p->pCuts[2][i];
}
static inline void Sdb_StoStoreResult( Sdb_Sto_t * p, int iObj, Sdb_Cut_t ** pCuts, int nCuts )
{
    int i, v;
    Vec_Int_t * vList = Vec_WecEntry( p->vCuts, iObj );
    Vec_IntPush( vList, nCuts );
    for ( i = 0; i < nCuts; i++ )
    {
        Vec_IntPush( vList, pCuts[i]->nLeaves );
        for ( v = 0; v < (int)pCuts[i]->nLeaves; v++ )
            Vec_IntPush( vList, pCuts[i]->pLeaves[v] );
        Vec_IntPush( vList, pCuts[i]->iFunc );
    }
}
static inline void Sdb_CutPrint( Sdb_Sto_t * p, int iObj, Sdb_Cut_t * pCut )
{
    int i, nDigits = Abc_Base10Log(Gia_ManObjNum(p->pGia)); 
    if ( pCut == NULL ) { printf( "No cut.\n" ); return; }
    printf( "%d  {", pCut->nLeaves );
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        printf( " %*d", nDigits, pCut->pLeaves[i] );
    for ( ; i < (int)p->nCutSize; i++ )
        printf( " %*s", nDigits, " " );
    printf( "  }  Cost = %3d  CostL = %3d  Tree = %d  ", 
        pCut->Cost, pCut->CostLev, pCut->nTreeLeaves );
    printf( "\n" );
}
void Sdb_StoMergeCuts( Sdb_Sto_t * p, int iObj )
{
    Gia_Obj_t * pObj = Gia_ManObj(p->pGia, iObj);
    int fIsXor       = Gia_ObjIsXor(pObj);
    int nCutSize     = p->nCutSize;
    int nCutNum      = p->nCutNum;
    int fComp0       = Gia_ObjFaninC0(pObj);
    int fComp1       = Gia_ObjFaninC1(pObj);
    int Fan0         = Gia_ObjFaninId0(pObj, iObj);
    int Fan1         = Gia_ObjFaninId1(pObj, iObj);
    int nCuts0       = Sdb_StoPrepareSet( p, Fan0, 0 );
    int nCuts1       = Sdb_StoPrepareSet( p, Fan1, 1 );
    int i, k, nCutsR = 0;
    Sdb_Cut_t * pCut0, * pCut1, ** pCutsR = p->ppCuts;
    assert( !Gia_ObjIsBuf(pObj) );
    assert( !Gia_ObjIsMux(p->pGia, pObj) );
    Sdb_StoInitResult( p );
    p->CutCount[0] += nCuts0 * nCuts1;
    for ( i = 0, pCut0 = p->pCuts[0]; i < nCuts0; i++, pCut0++ )
    for ( k = 0, pCut1 = p->pCuts[1]; k < nCuts1; k++, pCut1++ )
    {
        if ( (int)(pCut0->nLeaves + pCut1->nLeaves) > nCutSize && Sdb_CutCountBits(pCut0->Sign | pCut1->Sign) > nCutSize )
            continue;
        p->CutCount[1]++; 
        if ( !Sdb_CutMergeOrder(pCut0, pCut1, pCutsR[nCutsR], nCutSize) )
            continue;
        if ( Sdb_CutSetLastCutIsContained(pCutsR, nCutsR) )
            continue;
        p->CutCount[2]++;
        if ( p->fCutMin && Sdb_CutComputeTruth(p, pCut0, pCut1, fComp0, fComp1, pCutsR[nCutsR], fIsXor) )
            pCutsR[nCutsR]->Sign = Sdb_CutGetSign(pCutsR[nCutsR]);
        pCutsR[nCutsR]->nTreeLeaves = Sdb_CutTreeLeaves( p, pCutsR[nCutsR] );
        nCutsR = Sdb_CutSetAddCut( pCutsR, nCutsR, nCutNum );
    }
    p->CutCount[3] += nCutsR;
    p->nCutsOver += nCutsR == nCutNum-1;
    p->nCutsR = nCutsR;
    p->Pivot = iObj;
    // debug printout
    if ( 0 )
    {
        printf( "*** Obj = %4d  NumCuts = %4d\n", iObj, nCutsR );
        for ( i = 0; i < nCutsR; i++ )
            Sdb_CutPrint( p, iObj, pCutsR[i] );
        printf( "\n" );
    }
    // verify
    assert( nCutsR > 0 && nCutsR < nCutNum );
    assert( Sdb_CutSetCheckArray(pCutsR, nCutsR) );
    // store the cutset
    Sdb_StoStoreResult( p, iObj, pCutsR, nCutsR );
    if ( nCutsR > 1 || pCutsR[0]->nLeaves > 1 )
        Sdb_CutAddUnit( p, iObj );
}



/**Function*************************************************************

  Synopsis    [Cut comparison.]

  Description [Find out if there is a cut in vCuts such that pCut 
  has only one extra input. If so, return this input.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sdb_StoDiffExactlyOne( Vec_Wec_t * vCuts, int Limit, int * pCut )
{
    Vec_Int_t * vCut;  
    int i, k, iNew;
    // check if it is fully contained in any one
    Vec_WecForEachLevel( vCuts, vCut, i )
    {
        for ( k = 1; k <= pCut[0]; k++ )
            if ( Vec_IntFind(vCut, pCut[k]) == -1 )
                break;
        if ( k == pCut[0] + 1 )
            return -1;
    }
    // check if there is one different
    Vec_WecForEachLevel( vCuts, vCut, i )
    {
        if ( i == Limit )
            break;
        for ( iNew = -1, k = 1; k <= pCut[0]; k++ )
        {
            if ( Vec_IntFind(vCut, pCut[k]) >= 0 )
                continue;
            if ( iNew == -1 )
                iNew = pCut[k];
            else 
                break;
        }
        if ( k == pCut[0] + 1 && iNew != -1 )
            return iNew;
    }
    return -1;
}
int Sdb_StoDiffExactlyOne3( Vec_Wec_t * vCuts, int Limit, int * pCut, int * pCount )
{
    Vec_Int_t * vCut;  
    int i, iNewAll = -1, Count = 0;
    Vec_WecForEachLevel( vCuts, vCut, i )
    {
        int k, iNew = -1;
        if ( i == Limit )
            break;
        for ( k = 1; k <= pCut[0]; k++ )
        {
            if ( Vec_IntFind(vCut, pCut[k]) >= 0 )
                continue;
            if ( iNew == -1 )
                iNew = pCut[k];
            else 
                break;
        }
        if ( k == pCut[0] + 1 && iNew != -1 )
        {
            if ( iNewAll == -1 )
                iNewAll = iNew;
            if ( iNewAll == iNew )
                Count++;
        }
    }
    *pCount = Count;
    return iNewAll;
}
Vec_Int_t * Sdb_StoFindAll( Vec_Wec_t * vCuts )
{
    int i, k, Entry;
    Vec_Int_t * vCut, * vAll = Vec_IntAlloc( 100 );  
    Vec_WecForEachLevel( vCuts, vCut, i )
        Vec_IntForEachEntry( vCut, Entry, k )
            Vec_IntPushUnique( vAll, Entry );
    return vAll;
}
int Sdb_StoDiffExactlyOne2( Vec_Int_t * vAll, int * pCut )
{
    int k, iNew = -1;
    for ( k = 1; k <= pCut[0]; k++ )
    {
        if ( Vec_IntFind(vAll, pCut[k]) >= 0 )
            continue;
        if ( iNew == -1 )
            iNew = pCut[k];
        else 
            break;
    }
    if ( k == pCut[0] + 1 && iNew != -1 )
        return iNew;
    return -1;
}
Vec_Int_t * Sdb_StoFindInputs( Vec_Wec_t * vCuts, int Front )
{
    int fVerbose = 0;
    Vec_Int_t * vCut, * vCounts;
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    Vec_Int_t * vResA = Vec_IntAlloc( 100 );
    Vec_Int_t * vResB = Vec_IntAlloc( 100 );
    int i, k, Entry, Max = 0, Min, MinValue;
    // find MAX value
    Vec_WecForEachLevel( vCuts, vCut, i )
        Vec_IntForEachEntry( vCut, Entry, k )
            Max = Abc_MaxInt( Max, Entry );
    // count how many times each value appears
    vCounts = Vec_IntStart( Max + 1 );
    Vec_WecForEachLevel( vCuts, vCut, i )
        Vec_IntForEachEntry( vCut, Entry, k )
            Vec_IntAddToEntry( vCounts, Entry, 1 );
    Vec_IntWriteEntry( vCounts, 0, 0 );
    // print out
    if ( fVerbose )
    Vec_IntForEachEntry( vCounts, Entry, k )
        if ( Entry )
            printf( "%5d %5d\n", k, Entry );
    // collect first part
    MinValue = ABC_INFINITY;
    Vec_IntForEachEntry( vCounts, Entry, k )
        if ( Entry )
            MinValue = Abc_MinInt( MinValue, Entry );
    if ( MinValue == ABC_INFINITY )
        return vRes;
    Min = Vec_IntFind( vCounts, MinValue );
    Vec_IntPush( vResA, Min );
    Vec_IntWriteEntry( vCounts, Min, 0 );
    Vec_IntForEachEntry( vCounts, Entry, k )
        if ( Entry == MinValue || Entry == 2*MinValue )
        {
            Vec_IntPush( vResA, k );
            Vec_IntWriteEntry( vCounts, k, 0 );
        }
    // collect second parts
    MinValue = Vec_IntEntry( vCounts, Front );
    Min = Front;
    Vec_IntPush( vResB, Min );
    Vec_IntWriteEntry( vCounts, Min, 0 );
    Vec_IntForEachEntry( vCounts, Entry, k )
        if ( Entry == MinValue || Entry == 2*MinValue )
        {
            Vec_IntPush( vResB, k );
            Vec_IntWriteEntry( vCounts, k, 0 );
        }
    Vec_IntFree( vCounts );
    Vec_IntPrint( vResA );
    Vec_IntPrint( vResB );
    // here we need to order the inputs

    // append
    Vec_IntAppend( vRes, vResA );
    Vec_IntAppend( vRes, vResB );
    Vec_IntFree( vResA );
    Vec_IntFree( vResB );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Iterate through the cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sdb_StoIterCutsOne( Sdb_Sto_t * p, int iObj, int CutSize, int ** ppCut )
{
    int fVerbose = 0;
    Vec_Int_t * vThis = Vec_WecEntry( p->vCuts, iObj );
    int i, k, * pCut, * pList = Vec_IntArray( vThis );
    word * pTruth;
    Sdb_ForEachCut( pList, pCut, i )
    {
        if ( pCut[0] != CutSize )
            continue;
        if ( pCut[0] == 5 )
        {
            pTruth = Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut[pCut[0]+1]));
            for ( k = 0; k < s_nFuncTruths5; k++ )
                if ( s_FuncTruths5[k] == (unsigned)*pTruth )
                    break;
            if ( k < s_nFuncTruths5 )
            {
                if ( fVerbose )
                {
                    printf( "Object %d has %d-input cut:  ", iObj, pCut[0] );
                    for ( k = 1; k <= pCut[0]; k++ )
                        printf( "%d ", pCut[k] );
                    printf( "\n" );
                }
                *ppCut = pCut;
                return 1; // five-input
            }
        }
        if ( pCut[0] == 4 )
        {
            pTruth = Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut[pCut[0]+1]));
            for ( k = 0; k < s_nFuncTruths4; k++ )
                if ( s_FuncTruths4[k] == (0xFFFF & (unsigned)*pTruth) )
                    break;
            if ( k < s_nFuncTruths4 )
            {
                if ( fVerbose )
                {
                    printf( "Object %d has %d-input cut:  ", iObj, pCut[0] );
                    for ( k = 1; k <= pCut[0]; k++ )
                        printf( "%d ", pCut[k] );
                    printf( "\n" );
                }
                *ppCut = pCut;
                return 2; // front
            }
        }
        if ( pCut[0] == 4 )
        {
            pTruth = Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut[pCut[0]+1]));
            for ( k = 0; k < s_nFuncTruths4a; k++ )
                if ( s_FuncTruths4a[k] == (0xFFFF & (unsigned)*pTruth) )
                    break;
            if ( k < s_nFuncTruths4a )
            {
                if ( fVerbose )
                {
                    printf( "Object %d has %d-input cut:  ", iObj, pCut[0] );
                    for ( k = 1; k <= pCut[0]; k++ )
                        printf( "%d ", pCut[k] );
                    printf( "\n" );
                }
                *ppCut = pCut;
                return 3; // back unsigned
            }
        }
        if ( pCut[0] == 4 )
        {
            pTruth = Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut[pCut[0]+1]));
            for ( k = 0; k < s_nFuncTruths4b; k++ )
                if ( s_FuncTruths4b[k] == (0xFFFF & (unsigned)*pTruth) )
                    break;
            if ( k < s_nFuncTruths4b )
            {
                if ( fVerbose )
                {
                    printf( "Object %d has %d-input cut:  ", iObj, pCut[0] );
                    for ( k = 1; k <= pCut[0]; k++ )
                        printf( "%d ", pCut[k] );
                    printf( "\n" );
                }
                *ppCut = pCut;
                return 4; // back signed
            }
        }
    }
    return 0;
}
Vec_Int_t * Sdb_StoIterCuts( Sdb_Sto_t * p )
{
    Vec_Wec_t * v5Cuts = Vec_WecAlloc( 100 );
    Vec_Int_t * v5Cut, * vAll, * vRes;
    Gia_Obj_t * pObj;   
    int k, iObj, * pCut, Limit, iNew, iNewFront = -1, iNewBack = -1, iSigned = 0;
    Gia_ManForEachAnd( p->pGia, pObj, iObj )
    {
        int RetValue = Sdb_StoIterCutsOne( p, iObj, 5, &pCut );
        if ( RetValue == 0 )
            continue;
        assert( RetValue == 1 );
        assert( pCut[0] == 5 );
        v5Cut = Vec_WecPushLevel( v5Cuts );
        for ( k = 1; k <= pCut[0]; k++ )
            Vec_IntPush( v5Cut, pCut[k] );
    }
    Limit = Vec_WecSize( v5Cuts );
    printf( "Detected %d  5-cuts.\n", Vec_WecSize(v5Cuts) );
    vAll = Sdb_StoFindAll( v5Cuts );
    Gia_ManForEachAnd( p->pGia, pObj, iObj )
    {
        int RetValue = Sdb_StoIterCutsOne( p, iObj, 4, &pCut );
        if ( RetValue == 0 )
            continue;
        assert( RetValue >= 2 && RetValue <= 4 );
        assert( pCut[0] == 4 );
        // find cut, which differs in exactly one input
        iNew = Sdb_StoDiffExactlyOne( v5Cuts, Limit, pCut );
        if ( iNew == -1 )
            continue;
        if ( RetValue == 2 )
            iNewFront = iNew;
        else 
            iNewBack = iNew;
        if ( RetValue == 4 )
            iSigned = 1;
        // save in the second cut
        v5Cut = Vec_WecPushLevel( v5Cuts );
        Vec_IntPush( v5Cut, 0 );
        for ( k = 1; k <= pCut[0]; k++ )
            Vec_IntPush( v5Cut, pCut[k] );
    }
    Vec_IntFree( vAll );
    Vec_WecPrint( v5Cuts, 0 );

    if ( iNewFront )
        printf( "Front  = %d\n", iNewFront );
    if ( iNewBack )
        printf( "Back   = %d\n", iNewBack );
    if ( iSigned )
        printf( "Sign   = %d\n", iSigned );

    vRes = Sdb_StoFindInputs( v5Cuts, iNewFront );

    Vec_WecFree( v5Cuts );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Incremental cut computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sdb_Sto_t * Sdb_StoAlloc( Gia_Man_t * pGia, int nCutSize, int nCutNum, int fCutMin, int fTruthMin, int fVerbose )
{
    Sdb_Sto_t * p;
    assert( nCutSize < SDB_CUT_NO_LEAF );
    assert( nCutSize > 1 && nCutSize <= SDB_MAX_CUTSIZE );
    assert( nCutNum > 1 && nCutNum < SDB_MAX_CUTNUM );
    p = ABC_CALLOC( Sdb_Sto_t, 1 );
    p->clkStart  = Abc_Clock();
    p->nCutSize  = nCutSize;
    p->nCutNum   = nCutNum;
    p->fCutMin   = fCutMin;
    p->fTruthMin = fTruthMin;
    p->fVerbose  = fVerbose;
    p->pGia      = pGia;
    p->vRefs     = Vec_IntAlloc( Gia_ManObjNum(pGia) );
    p->vCuts     = Vec_WecStart( Gia_ManObjNum(pGia) );
    p->vTtMem    = fCutMin ? Vec_MemAllocForTT( nCutSize, 0 ) : NULL;
    return p;
}
void Sdb_StoFree( Sdb_Sto_t * p )
{
    Vec_IntFree( p->vRefs );
    Vec_WecFree( p->vCuts );
    if ( p->fCutMin )
        Vec_MemHashFree( p->vTtMem );
    if ( p->fCutMin )
        Vec_MemFree( p->vTtMem );
    ABC_FREE( p );
}
void Sdb_StoComputeCutsConst0( Sdb_Sto_t * p, int iObj )
{
    Sdb_CutAddZero( p, iObj );
}
void Sdb_StoComputeCutsCi( Sdb_Sto_t * p, int iObj )
{
    Sdb_CutAddUnit( p, iObj );
}
void Sdb_StoComputeCutsNode( Sdb_Sto_t * p, int iObj )
{
    Sdb_StoMergeCuts( p, iObj );
}
void Sdb_StoRefObj( Sdb_Sto_t * p, int iObj )
{
    Gia_Obj_t * pObj = Gia_ManObj(p->pGia, iObj);
    assert( iObj == Vec_IntSize(p->vRefs) );
    Vec_IntPush( p->vRefs, 0 );
    if ( Gia_ObjIsAnd(pObj) )
    {
        Vec_IntAddToEntry( p->vRefs, Gia_ObjFaninId0(pObj, iObj), 1 );
        Vec_IntAddToEntry( p->vRefs, Gia_ObjFaninId1(pObj, iObj), 1 );
    }
    else if ( Gia_ObjIsCo(pObj) )
        Vec_IntAddToEntry( p->vRefs, Gia_ObjFaninId0(pObj, iObj), 1 );
}
Vec_Int_t * Sdb_StoComputeCutsDetect( Gia_Man_t * pGia )
{
    Vec_Int_t * vRes = NULL;
    Sdb_Sto_t * p = Sdb_StoAlloc( pGia, 5, 20, 1, 0, 1 );
    Gia_Obj_t * pObj;   
    int i, iObj;
    // prepare references
    Gia_ManForEachObj( p->pGia, pObj, iObj )
        Sdb_StoRefObj( p, iObj );
    // compute cuts
    Sdb_StoComputeCutsConst0( p, 0 );
    Gia_ManForEachCiId( p->pGia, iObj, i )
        Sdb_StoComputeCutsCi( p, iObj );
    Gia_ManForEachAnd( p->pGia, pObj, iObj )
        Sdb_StoComputeCutsNode( p, iObj );
    if ( p->fVerbose )
    {
        printf( "Running cut computation with CutSize = %d  CutNum = %d:\n", p->nCutSize, p->nCutNum );
        printf( "CutPair = %.0f  ",         p->CutCount[0] );
        printf( "Merge = %.0f (%.2f %%)  ", p->CutCount[1], 100.0*p->CutCount[1]/p->CutCount[0] );
        printf( "Eval = %.0f (%.2f %%)  ",  p->CutCount[2], 100.0*p->CutCount[2]/p->CutCount[0] );
        printf( "Cut = %.0f (%.2f %%)  ",   p->CutCount[3], 100.0*p->CutCount[3]/p->CutCount[0] );
        printf( "Cut/Node = %.2f  ",        p->CutCount[3] / Gia_ManAndNum(p->pGia) );
        printf( "\n" );
        printf( "Over = %4d  ",             p->nCutsOver );
        Abc_PrintTime( 0, "Time", Abc_Clock() - p->clkStart );
    }
    vRes = Sdb_StoIterCuts( p );
    Sdb_StoFree( p );
    return vRes;
}
void Sdb_StoComputeCutsTest( Gia_Man_t * pGia )
{
    Vec_Int_t * vRes = Sdb_StoComputeCutsDetect( pGia );
    Vec_IntFree( vRes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

