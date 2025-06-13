/**CFile****************************************************************

  FileName    [acecMult.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Multiplier.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecMult.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"
#include "misc/extra/extra.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

unsigned s_Classes4a[96] = {
    0xD728, 0xB748, 0x9F60, 0xD278, 0xB478, 0x96F0, 0xC66C, 0x96CC, 0x9C6C, 0x96AA, 0xA66A, 0x9A6A,
    0x28D7, 0x48B7, 0x609F, 0x2D87, 0x4B87, 0x690F, 0x3993, 0x6933, 0x6393, 0x6955, 0x5995, 0x6595,
    0xEB14, 0xED12, 0xF906, 0xE1B4, 0xE1D2, 0xF096, 0xC99C, 0xCC96, 0xC9C6, 0xAA96, 0xA99A, 0xA9A6,
    0x14EB, 0x12ED, 0x06F9, 0x1E4B, 0x1E2D, 0x0F69, 0x3663, 0x3369, 0x3639, 0x5569, 0x5665, 0x5659,
    0x7D82, 0x7B84, 0x6F90, 0x78D2, 0x78B4, 0x69F0, 0x6CC6, 0x69CC, 0x6C9C, 0x69AA, 0x6AA6, 0x6A9A,
    0x827D, 0x847B, 0x906F, 0x872D, 0x874B, 0x960F, 0x9339, 0x9633, 0x9363, 0x9655, 0x9559, 0x9565,
    0xBE41, 0xDE21, 0xF609, 0xB4E1, 0xD2E1, 0xF069, 0x9CC9, 0xCC69, 0xC6C9, 0xAA69, 0x9AA9, 0xA6A9,
    0x41BE, 0x21DE, 0x09F6, 0x4B1E, 0x2D1E, 0x0F96, 0x6336, 0x3396, 0x3936, 0x5596, 0x6556, 0x5956
};

unsigned s_Classes4b[384] = {
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
    0x5C03, 0x3A05, 0x7403, 0x2E11, 0x4E11, 0x7205, 0x50C3, 0x30A5, 0x7043, 0x21E1, 0x41E1, 0x7025,
    0x4C43, 0x09C9, 0x44C3, 0x0C99, 0x4C19, 0x41C9, 0x09A9, 0x2A25, 0x21A9, 0x2A19, 0x0A99, 0x22A5,
    0xAC03, 0xCA05, 0xB803, 0xE211, 0xE411, 0xD805, 0xA0C3, 0xC0A5, 0xB083, 0xE121, 0xE141, 0xD085,
    0x8C83, 0xC909, 0x88C3, 0xC099, 0xC491, 0xC941, 0xA909, 0x8A85, 0xA921, 0xA291, 0xA099, 0x88A5,
    0xC035, 0xA053, 0xC01D, 0x8847, 0x8827, 0xA01B, 0xC305, 0xA503, 0xC10D, 0x8487, 0x8287, 0xA10B,
    0xC131, 0x9093, 0xC311, 0x9903, 0x8923, 0x8293, 0x9095, 0xA151, 0x8495, 0x8945, 0x9905, 0xA511,
    0xC03A, 0xA05C, 0xC02E, 0x8874, 0x8872, 0xA04E, 0xC30A, 0xA50C, 0xC20E, 0x8784, 0x8782, 0xA40E,
    0xC232, 0x9390, 0xC322, 0x9930, 0x9832, 0x9382, 0x9590, 0xA454, 0x9584, 0x9854, 0x9950, 0xA544,
    0x30C5, 0x50A3, 0x0CD1, 0x448B, 0x228D, 0x0AB1, 0x3C05, 0x5A03, 0x0DC1, 0x484B, 0x282D, 0x0BA1,
    0x31C1, 0x6063, 0x3C11, 0x6603, 0x2389, 0x2839, 0x6065, 0x51A1, 0x4859, 0x4589, 0x6605, 0x5A11,
    0x30CA, 0x50AC, 0x0CE2, 0x44B8, 0x22D8, 0x0AE4, 0x3C0A, 0x5A0C, 0x0EC2, 0x4B48, 0x2D28, 0x0EA4,
    0x32C2, 0x6360, 0x3C22, 0x6630, 0x3298, 0x3928, 0x6560, 0x54A4, 0x5948, 0x5498, 0x6650, 0x5A44,
    0x0C53, 0x0A35, 0x3047, 0x221D, 0x441B, 0x5027, 0x05C3, 0x03A5, 0x3407, 0x212D, 0x414B, 0x5207,
    0x1C13, 0x0939, 0x11C3, 0x0399, 0x4613, 0x4163, 0x0959, 0x1A15, 0x2165, 0x2615, 0x0599, 0x11A5,
    0x0CA3, 0x0AC5, 0x308B, 0x22D1, 0x44B1, 0x508D, 0x0AC3, 0x0CA5, 0x380B, 0x2D21, 0x4B41, 0x580D,
    0x2C23, 0x3909, 0x22C3, 0x3099, 0x6431, 0x6341, 0x5909, 0x4A45, 0x6521, 0x6251, 0x5099, 0x44A5,
    0x035C, 0x053A, 0x0374, 0x112E, 0x114E, 0x0572, 0x053C, 0x035A, 0x0734, 0x121E, 0x141E, 0x0752,
    0x131C, 0x0636, 0x113C, 0x0366, 0x1346, 0x1436, 0x0656, 0x151A, 0x1256, 0x1526, 0x0566, 0x115A,
    0x03AC, 0x05CA, 0x03B8, 0x11E2, 0x11E4, 0x05D8, 0x0A3C, 0x0C5A, 0x0B38, 0x1E12, 0x1E14, 0x0D58,
    0x232C, 0x3606, 0x223C, 0x3066, 0x3164, 0x3614, 0x5606, 0x454A, 0x5612, 0x5162, 0x5066, 0x445A
};

unsigned s_Classes4c[768] = {
    0x35C0, 0x53A0, 0x1DC0, 0x4788, 0x2788, 0x1BA0, 0x3C50, 0x5A30, 0x1CD0, 0x4878, 0x2878, 0x1AB0,
    0x34C4, 0x606C, 0x3C44, 0x660C, 0x268C, 0x286C, 0x606A, 0x52A2, 0x486A, 0x468A, 0x660A, 0x5A22,
    0xCA3F, 0xAC5F, 0xE23F, 0xB877, 0xD877, 0xE45F, 0xC3AF, 0xA5CF, 0xE32F, 0xB787, 0xD787, 0xE54F,
    0xCB3B, 0x9F93, 0xC3BB, 0x99F3, 0xD973, 0xD793, 0x9F95, 0xAD5D, 0xB795, 0xB975, 0x99F5, 0xA5DD,
    0x3AC0, 0x5CA0, 0x2EC0, 0x7488, 0x7288, 0x4EA0, 0x3CA0, 0x5AC0, 0x2CE0, 0x7848, 0x7828, 0x4AE0,
    0x38C8, 0x6C60, 0x3C88, 0x66C0, 0x62C8, 0x6C28, 0x6A60, 0x58A8, 0x6A48, 0x64A8, 0x66A0, 0x5A88,
    0xC53F, 0xA35F, 0xD13F, 0x8B77, 0x8D77, 0xB15F, 0xC35F, 0xA53F, 0xD31F, 0x87B7, 0x87D7, 0xB51F,
    0xC737, 0x939F, 0xC377, 0x993F, 0x9D37, 0x93D7, 0x959F, 0xA757, 0x95B7, 0x9B57, 0x995F, 0xA577,
    0xC530, 0xA350, 0xD10C, 0x8B44, 0x8D22, 0xB10A, 0xC350, 0xA530, 0xD01C, 0x84B4, 0x82D2, 0xB01A,
    0xC434, 0x909C, 0xC344, 0x990C, 0x8C26, 0x82C6, 0x909A, 0xA252, 0x84A6, 0x8A46, 0x990A, 0xA522,
    0x3ACF, 0x5CAF, 0x2EF3, 0x74BB, 0x72DD, 0x4EF5, 0x3CAF, 0x5ACF, 0x2FE3, 0x7B4B, 0x7D2D, 0x4FE5,
    0x3BCB, 0x6F63, 0x3CBB, 0x66F3, 0x73D9, 0x7D39, 0x6F65, 0x5DAD, 0x7B59, 0x75B9, 0x66F5, 0x5ADD,
    0xCA30, 0xAC50, 0xE20C, 0xB844, 0xD822, 0xE40A, 0xC3A0, 0xA5C0, 0xE02C, 0xB484, 0xD282, 0xE04A,
    0xC838, 0x9C90, 0xC388, 0x99C0, 0xC862, 0xC682, 0x9A90, 0xA858, 0xA684, 0xA864, 0x99A0, 0xA588,
    0x35CF, 0x53AF, 0x1DF3, 0x47BB, 0x27DD, 0x1BF5, 0x3C5F, 0x5A3F, 0x1FD3, 0x4B7B, 0x2D7D, 0x1FB5,
    0x37C7, 0x636F, 0x3C77, 0x663F, 0x379D, 0x397D, 0x656F, 0x57A7, 0x597B, 0x579B, 0x665F, 0x5A77,
    0x530C, 0x350A, 0x4730, 0x1D22, 0x1B44, 0x2750, 0x503C, 0x305A, 0x4370, 0x12D2, 0x14B4, 0x2570,
    0x434C, 0x06C6, 0x443C, 0x0C66, 0x194C, 0x149C, 0x06A6, 0x252A, 0x129A, 0x192A, 0x0A66, 0x225A,
    0xACF3, 0xCAF5, 0xB8CF, 0xE2DD, 0xE4BB, 0xD8AF, 0xAFC3, 0xCFA5, 0xBC8F, 0xED2D, 0xEB4B, 0xDA8F,
    0xBCB3, 0xF939, 0xBBC3, 0xF399, 0xE6B3, 0xEB63, 0xF959, 0xDAD5, 0xED65, 0xE6D5, 0xF599, 0xDDA5,
    0xA30C, 0xC50A, 0x8B30, 0xD122, 0xB144, 0x8D50, 0xA03C, 0xC05A, 0x83B0, 0xD212, 0xB414, 0x85D0,
    0x838C, 0xC606, 0x883C, 0xC066, 0x91C4, 0x9C14, 0xA606, 0x858A, 0x9A12, 0x91A2, 0xA066, 0x885A,
    0x5CF3, 0x3AF5, 0x74CF, 0x2EDD, 0x4EBB, 0x72AF, 0x5FC3, 0x3FA5, 0x7C4F, 0x2DED, 0x4BEB, 0x7A2F,
    0x7C73, 0x39F9, 0x77C3, 0x3F99, 0x6E3B, 0x63EB, 0x59F9, 0x7A75, 0x65ED, 0x6E5D, 0x5F99, 0x77A5,
    0x5C03, 0x3A05, 0x7403, 0x2E11, 0x4E11, 0x7205, 0x50C3, 0x30A5, 0x7043, 0x21E1, 0x41E1, 0x7025,
    0x4C43, 0x09C9, 0x44C3, 0x0C99, 0x4C19, 0x41C9, 0x09A9, 0x2A25, 0x21A9, 0x2A19, 0x0A99, 0x22A5,
    0xA3FC, 0xC5FA, 0x8BFC, 0xD1EE, 0xB1EE, 0x8DFA, 0xAF3C, 0xCF5A, 0x8FBC, 0xDE1E, 0xBE1E, 0x8FDA,
    0xB3BC, 0xF636, 0xBB3C, 0xF366, 0xB3E6, 0xBE36, 0xF656, 0xD5DA, 0xDE56, 0xD5E6, 0xF566, 0xDD5A,
    0xAC03, 0xCA05, 0xB803, 0xE211, 0xE411, 0xD805, 0xA0C3, 0xC0A5, 0xB083, 0xE121, 0xE141, 0xD085,
    0x8C83, 0xC909, 0x88C3, 0xC099, 0xC491, 0xC941, 0xA909, 0x8A85, 0xA921, 0xA291, 0xA099, 0x88A5,
    0x53FC, 0x35FA, 0x47FC, 0x1DEE, 0x1BEE, 0x27FA, 0x5F3C, 0x3F5A, 0x4F7C, 0x1EDE, 0x1EBE, 0x2F7A,
    0x737C, 0x36F6, 0x773C, 0x3F66, 0x3B6E, 0x36BE, 0x56F6, 0x757A, 0x56DE, 0x5D6E, 0x5F66, 0x775A,
    0xC035, 0xA053, 0xC01D, 0x8847, 0x8827, 0xA01B, 0xC305, 0xA503, 0xC10D, 0x8487, 0x8287, 0xA10B,
    0xC131, 0x9093, 0xC311, 0x9903, 0x8923, 0x8293, 0x9095, 0xA151, 0x8495, 0x8945, 0x9905, 0xA511,
    0x3FCA, 0x5FAC, 0x3FE2, 0x77B8, 0x77D8, 0x5FE4, 0x3CFA, 0x5AFC, 0x3EF2, 0x7B78, 0x7D78, 0x5EF4,
    0x3ECE, 0x6F6C, 0x3CEE, 0x66FC, 0x76DC, 0x7D6C, 0x6F6A, 0x5EAE, 0x7B6A, 0x76BA, 0x66FA, 0x5AEE,
    0xC03A, 0xA05C, 0xC02E, 0x8874, 0x8872, 0xA04E, 0xC30A, 0xA50C, 0xC20E, 0x8784, 0x8782, 0xA40E,
    0xC232, 0x9390, 0xC322, 0x9930, 0x9832, 0x9382, 0x9590, 0xA454, 0x9584, 0x9854, 0x9950, 0xA544,
    0x3FC5, 0x5FA3, 0x3FD1, 0x778B, 0x778D, 0x5FB1, 0x3CF5, 0x5AF3, 0x3DF1, 0x787B, 0x787D, 0x5BF1,
    0x3DCD, 0x6C6F, 0x3CDD, 0x66CF, 0x67CD, 0x6C7D, 0x6A6F, 0x5BAB, 0x6A7B, 0x67AB, 0x66AF, 0x5ABB,
    0x30C5, 0x50A3, 0x0CD1, 0x448B, 0x228D, 0x0AB1, 0x3C05, 0x5A03, 0x0DC1, 0x484B, 0x282D, 0x0BA1,
    0x31C1, 0x6063, 0x3C11, 0x6603, 0x2389, 0x2839, 0x6065, 0x51A1, 0x4859, 0x4589, 0x6605, 0x5A11,
    0xCF3A, 0xAF5C, 0xF32E, 0xBB74, 0xDD72, 0xF54E, 0xC3FA, 0xA5FC, 0xF23E, 0xB7B4, 0xD7D2, 0xF45E,
    0xCE3E, 0x9F9C, 0xC3EE, 0x99FC, 0xDC76, 0xD7C6, 0x9F9A, 0xAE5E, 0xB7A6, 0xBA76, 0x99FA, 0xA5EE,
    0x30CA, 0x50AC, 0x0CE2, 0x44B8, 0x22D8, 0x0AE4, 0x3C0A, 0x5A0C, 0x0EC2, 0x4B48, 0x2D28, 0x0EA4,
    0x32C2, 0x6360, 0x3C22, 0x6630, 0x3298, 0x3928, 0x6560, 0x54A4, 0x5948, 0x5498, 0x6650, 0x5A44,
    0xCF35, 0xAF53, 0xF31D, 0xBB47, 0xDD27, 0xF51B, 0xC3F5, 0xA5F3, 0xF13D, 0xB4B7, 0xD2D7, 0xF15B,
    0xCD3D, 0x9C9F, 0xC3DD, 0x99CF, 0xCD67, 0xC6D7, 0x9A9F, 0xAB5B, 0xA6B7, 0xAB67, 0x99AF, 0xA5BB,
    0x0C53, 0x0A35, 0x3047, 0x221D, 0x441B, 0x5027, 0x05C3, 0x03A5, 0x3407, 0x212D, 0x414B, 0x5207,
    0x1C13, 0x0939, 0x11C3, 0x0399, 0x4613, 0x4163, 0x0959, 0x1A15, 0x2165, 0x2615, 0x0599, 0x11A5,
    0xF3AC, 0xF5CA, 0xCFB8, 0xDDE2, 0xBBE4, 0xAFD8, 0xFA3C, 0xFC5A, 0xCBF8, 0xDED2, 0xBEB4, 0xADF8,
    0xE3EC, 0xF6C6, 0xEE3C, 0xFC66, 0xB9EC, 0xBE9C, 0xF6A6, 0xE5EA, 0xDE9A, 0xD9EA, 0xFA66, 0xEE5A,
    0x0CA3, 0x0AC5, 0x308B, 0x22D1, 0x44B1, 0x508D, 0x0AC3, 0x0CA5, 0x380B, 0x2D21, 0x4B41, 0x580D,
    0x2C23, 0x3909, 0x22C3, 0x3099, 0x6431, 0x6341, 0x5909, 0x4A45, 0x6521, 0x6251, 0x5099, 0x44A5,
    0xF35C, 0xF53A, 0xCF74, 0xDD2E, 0xBB4E, 0xAF72, 0xF53C, 0xF35A, 0xC7F4, 0xD2DE, 0xB4BE, 0xA7F2,
    0xD3DC, 0xC6F6, 0xDD3C, 0xCF66, 0x9BCE, 0x9CBE, 0xA6F6, 0xB5BA, 0x9ADE, 0x9DAE, 0xAF66, 0xBB5A,
    0x035C, 0x053A, 0x0374, 0x112E, 0x114E, 0x0572, 0x053C, 0x035A, 0x0734, 0x121E, 0x141E, 0x0752,
    0x131C, 0x0636, 0x113C, 0x0366, 0x1346, 0x1436, 0x0656, 0x151A, 0x1256, 0x1526, 0x0566, 0x115A,
    0xFCA3, 0xFAC5, 0xFC8B, 0xEED1, 0xEEB1, 0xFA8D, 0xFAC3, 0xFCA5, 0xF8CB, 0xEDE1, 0xEBE1, 0xF8AD,
    0xECE3, 0xF9C9, 0xEEC3, 0xFC99, 0xECB9, 0xEBC9, 0xF9A9, 0xEAE5, 0xEDA9, 0xEAD9, 0xFA99, 0xEEA5,
    0x03AC, 0x05CA, 0x03B8, 0x11E2, 0x11E4, 0x05D8, 0x0A3C, 0x0C5A, 0x0B38, 0x1E12, 0x1E14, 0x0D58,
    0x232C, 0x3606, 0x223C, 0x3066, 0x3164, 0x3614, 0x5606, 0x454A, 0x5612, 0x5162, 0x5066, 0x445A,
    0xFC53, 0xFA35, 0xFC47, 0xEE1D, 0xEE1B, 0xFA27, 0xF5C3, 0xF3A5, 0xF4C7, 0xE1ED, 0xE1EB, 0xF2A7,
    0xDCD3, 0xC9F9, 0xDDC3, 0xCF99, 0xCE9B, 0xC9EB, 0xA9F9, 0xBAB5, 0xA9ED, 0xAE9D, 0xAF99, 0xBBA5
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Computes NPN-canonical form using brute-force methods.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Extra_TruthCanonNPN3( word uTruth, int nVars, Vec_Wrd_t * vRes )
{
    int nMints  = (1 << nVars);
    int nPerms  = Extra_Factorial( nVars );
    int * pComp = Extra_GreyCodeSchedule( nVars );
    int * pPerm = Extra_PermSchedule( nVars );
    word tCur, tTemp1, tTemp2;
    word uTruthMin = ABC_CONST(0xFFFFFFFFFFFFFFFF);
    int i, p, c;
    Vec_WrdClear( vRes );
    for ( i = 0; i < 2; i++ )
    {
        tCur = i ? ~uTruth : uTruth;
        tTemp1 = tCur;
        for ( p = 0; p < nPerms; p++ )
        {
            tTemp2 = tCur;
            for ( c = 0; c < nMints; c++ )
            {
                if ( uTruthMin > tCur )
                    uTruthMin = tCur;
                Vec_WrdPushUnique( vRes, tCur );
                tCur = Abc_Tt6Flip( tCur, pComp[c] );
            }
            assert( tTemp2 == tCur );
            tCur = Abc_Tt6SwapAdjacent( tCur, pPerm[p] );
        }
        assert( tTemp1 == tCur );
    }
    ABC_FREE( pComp );
    ABC_FREE( pPerm );
    return uTruthMin;
}
void Acec_MultFuncTest6()
{
    Vec_Wrd_t * vRes = Vec_WrdAlloc( 10000 );
    int i; word Entry;

    word Truth = ABC_CONST(0xfedcba9876543210);
    word Canon = Extra_TruthCanonNPN3( Truth, 6, vRes );

    Extra_PrintHex( stdout, (unsigned*)&Truth, 6 );  printf( "\n" );
    Extra_PrintHex( stdout, (unsigned*)&Canon, 6 );  printf( "\n" );

    printf( "Members = %d.\n", Vec_WrdSize(vRes) );
    Vec_WrdForEachEntry( vRes, Entry, i )
    {
        Extra_PrintHex( stdout, (unsigned*)&Entry, 6 );  
        printf( ", " );
        if ( i % 8 == 7 )
            printf( "\n" );
    }

    Vec_WrdFree( vRes );
}

unsigned Extra_TruthCanonNPN2( unsigned uTruth, int nVars, Vec_Int_t * vRes )
{
    static int nVarsOld, nPerms;
    static char ** pPerms = NULL;

    unsigned uTruthMin, uTruthC, uPhase, uPerm;
    int nMints, k, i;

    if ( pPerms == NULL )
    {
        nPerms = Extra_Factorial( nVars );   
        pPerms = Extra_Permutations( nVars );
        nVarsOld = nVars;
    }
    else if ( nVarsOld != nVars )
    {
        ABC_FREE( pPerms );
        nPerms = Extra_Factorial( nVars );   
        pPerms = Extra_Permutations( nVars );
        nVarsOld = nVars;
    }

    nMints    = (1 << nVars);
    uTruthC   = (unsigned)( (~uTruth) & ((~((unsigned)0)) >> (32-nMints)) );
    uTruthMin = 0xFFFFFFFF;
    for ( i = 0; i < nMints; i++ )
    {
        uPhase = Extra_TruthPolarize( uTruth, i, nVars ); 
        for ( k = 0; k < nPerms; k++ )
        {
            uPerm = Extra_TruthPermute( uPhase, pPerms[k], nVars, 0 );
            if ( !(uPerm & 1) )
            Vec_IntPushUnique( vRes, uPerm );
            if ( uTruthMin > uPerm )
                uTruthMin = uPerm;
        }
        uPhase = Extra_TruthPolarize( uTruthC, i, nVars ); 
        for ( k = 0; k < nPerms; k++ )
        {
            uPerm = Extra_TruthPermute( uPhase, pPerms[k], nVars, 0 );
            if ( !(uPerm & 1) )
            Vec_IntPushUnique( vRes, uPerm );
            if ( uTruthMin > uPerm )
                uTruthMin = uPerm;
        }
    }
    return uTruthMin;
}

void Acec_MultFuncTest5()
{
    Vec_Int_t * vRes = Vec_IntAlloc( 1000 );
    int i, Entry;

    unsigned Truth = 0xF335ACC0;
    unsigned Canon = Extra_TruthCanonNPN2( Truth, 5, vRes );

    Extra_PrintHex( stdout, (unsigned*)&Truth, 5 );  printf( "\n" );
    Extra_PrintHex( stdout, (unsigned*)&Canon, 5 );  printf( "\n" );

    printf( "Members = %d.\n", Vec_IntSize(vRes) );
    Vec_IntForEachEntry( vRes, Entry, i )
    {
        Extra_PrintHex( stdout, (unsigned*)&Entry, 5 );  
        printf( ", " );
        if ( i % 8 == 7 )
            printf( "\n" );
    }

    Vec_IntFree( vRes );
}

void Acec_MultFuncTest4()
{
    Vec_Int_t * vRes = Vec_IntAlloc( 1000 );
    int i, Entry;

    unsigned Truth = 0xF3C0;
//    unsigned Truth = 0xF335;
//    unsigned Truth = 0xFD80;
    //unsigned Truth = 0xD728;
    //unsigned Truth = 0x35C0;
    //unsigned Truth = 0xACC0;
    unsigned Canon = Extra_TruthCanonNPN2( Truth, 4, vRes );

    Extra_PrintHex( stdout, (unsigned*)&Truth, 4 );  printf( "\n" );
    Extra_PrintHex( stdout, (unsigned*)&Canon, 4 );  printf( "\n" );

    printf( "Members = %d.\n", Vec_IntSize(vRes) );
    Vec_IntForEachEntry( vRes, Entry, i )
    {
        Extra_PrintHex( stdout, (unsigned*)&Entry, 4 );  
        printf( ", " );
        if ( i % 12 == 11 )
            printf( "\n" );
    }

    Vec_IntFree( vRes );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Acec_MultCollectInputs( Vec_Int_t * vPairs, Vec_Int_t * vRanks, int iObj )
{
    Vec_Int_t * vItems = Vec_IntAlloc( 100 );
    int k, iObj1, iObj2;
    // collect all those appearing with this one
    Vec_IntForEachEntryDouble( vPairs, iObj1, iObj2, k )
        if ( iObj == iObj1 )
            Vec_IntPushUnique( vItems, iObj2 );
        else if ( iObj == iObj2 )
            Vec_IntPushUnique( vItems, iObj1 );
    // sort items by rank cost
    Vec_IntSelectSortCost( Vec_IntArray(vItems), Vec_IntSize(vItems), vRanks );
    return vItems;
}
Vec_Int_t * Acec_MultDetectInputs1( Gia_Man_t * p, Vec_Wec_t * vLeafLits, Vec_Wec_t * vRootLits )
{
    Vec_Int_t * vInputs = Vec_IntAlloc( 100 );
    Vec_Int_t * vCounts = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vRanks  = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vPairs  = Vec_IntAlloc( 100 );
    Vec_Int_t * vItems  = Vec_IntAlloc( 100 );
    Vec_Int_t * vItems0;
    Vec_Int_t * vItems1;
    Vec_Int_t * vLevel;
    int i, k, iLit, iObj, Count;
    // count how many times each input appears
    Vec_WecForEachLevel( vLeafLits, vLevel, i )
        Vec_IntForEachEntry( vLevel, iLit, k )
        {
            iObj = Abc_Lit2Var(iLit);
            Vec_IntAddToEntry( vCounts, Gia_ObjFaninId0(Gia_ManObj(p, iObj), iObj), 1 );
            Vec_IntAddToEntry( vCounts, Gia_ObjFaninId1(Gia_ManObj(p, iObj), iObj), 1 );
/*
            printf( "Rank %2d : Leaf = %4d : (%2d, %2d)\n", i, iObj, 
                Gia_ObjFaninId0(Gia_ManObj(p, iObj), iObj), Gia_ObjFaninId1(Gia_ManObj(p, iObj), iObj) );
            if ( k == Vec_IntSize(vLevel) - 1 )
                printf( "\n" );
*/
        }
    // count ranks for each one
    Vec_WecForEachLevel( vLeafLits, vLevel, i )
        Vec_IntForEachEntry( vLevel, iLit, k )
        {
            iObj = Abc_Lit2Var(iLit);
            if ( Vec_IntEntry(vCounts, Gia_ObjFaninId0(Gia_ManObj(p, iObj), iObj)) < 2 )
            {
                printf( "Skipping %d.\n", iObj );
                continue;
            }
            if ( Vec_IntEntry(vCounts, Gia_ObjFaninId1(Gia_ManObj(p, iObj), iObj)) < 2 )
            {
                printf( "Skipping %d.\n", iObj );
                continue;
            }
            Vec_IntAddToEntry( vRanks, Gia_ObjFaninId0(Gia_ManObj(p, iObj), iObj), i );
            Vec_IntAddToEntry( vRanks, Gia_ObjFaninId1(Gia_ManObj(p, iObj), iObj), i );

            Vec_IntPushTwo( vPairs, Gia_ObjFaninId0(Gia_ManObj(p, iObj), iObj), Gia_ObjFaninId1(Gia_ManObj(p, iObj), iObj) );
        }

    // print statistics
    Vec_IntForEachEntry( vCounts, Count, i )
    {
        if ( !Count )
            continue;
        if ( !Vec_IntEntry(vRanks, i) )
            continue;
        Vec_IntPush( vItems, i );
        printf( "Obj = %3d  Occurs = %3d  Ranks = %3d\n", i, Count, Vec_IntEntry(vRanks, i) );
    }
    // sort items by rank cost
    Vec_IntSelectSortCost( Vec_IntArray(vItems), Vec_IntSize(vItems), vRanks );
    // collect all those appearing with the last one
    vItems0 = Acec_MultCollectInputs( vPairs, vRanks, Vec_IntEntryLast(vItems) );
    Vec_IntAppend( vInputs, vItems0 );
    // collect all those appearing with the last one
    vItems1 = Acec_MultCollectInputs( vPairs, vRanks, Vec_IntEntryLast(vItems0) );
    Vec_IntAppend( vInputs, vItems1 );

    Vec_IntPrint( vItems0 );
    Vec_IntPrint( vItems1 );

    Vec_IntFree( vCounts );
    Vec_IntFree( vRanks );
    Vec_IntFree( vPairs );
    Vec_IntFree( vItems );
    Vec_IntFree( vItems0 );
    Vec_IntFree( vItems1 );
    return vInputs;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Acec_MultDetectInputs( Gia_Man_t * p, Vec_Wec_t * vLeafLits, Vec_Wec_t * vRootLits )
{
    Vec_Int_t * vInputs = Vec_IntAlloc( 100 );
    Vec_Int_t * vSupp = Vec_IntAlloc( 100 );
    Vec_Wrd_t * vTemp = Vec_WrdStart( Gia_ManObjNum(p) );
    Vec_Int_t * vRanks  = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vCounts = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vLevel;
    int i, k, iLit, iObj, j, Entry;

    ABC_FREE( p->pRefs );
    Gia_ManCreateRefs( p );
    Gia_ManForEachCiId( p, iObj, i )
        printf( "%d=%d ", iObj, Gia_ObjRefNumId(p, iObj) );
    printf( "\n" );
    Gia_ManForEachAndId( p, iObj )
        if ( Gia_ObjRefNumId(p, iObj) >= 4 )
            printf( "%d=%d ", iObj, Gia_ObjRefNumId(p, iObj) );
    printf( "\n" );

    Vec_WecForEachLevel( vLeafLits, vLevel, i )
        Vec_IntForEachEntry( vLevel, iLit, k )
        {
            word Truth = Gia_ObjComputeTruth6Cis( p, iLit, vSupp, vTemp );
            if ( Vec_IntSize(vSupp) >= 0 )
            {
                printf( "Leaf = %4d : ", Abc_Lit2Var(iLit) );
                printf( "Rank = %2d  ", i );
                printf( "Supp = %2d  ", Vec_IntSize(vSupp) );
                Extra_PrintHex( stdout, (unsigned*)&Truth, Vec_IntSize(vSupp) );
                if ( Vec_IntSize(vSupp) == 4 ) printf( "    " );
                if ( Vec_IntSize(vSupp) == 3 ) printf( "      " );
                if ( Vec_IntSize(vSupp) <= 2 ) printf( "       " );
                printf( "  " );
                Vec_IntPrint( vSupp );
                /*
                if ( Truth == 0xF335ACC0F335ACC0 )
                {
                    int iObj = Abc_Lit2Var(iLit);
                    Gia_Man_t * pGia0 = Gia_ManDupAndCones( p, &iObj, 1, 1 );
                    Gia_ManShow( pGia0, NULL, 0, 0, 0 );
                    Gia_ManStop( pGia0 );
                }
                */
            }
            // support rank counts
            Vec_IntForEachEntry( vSupp, Entry, j )
            {
                Vec_IntAddToEntry( vRanks, Entry, i );
                Vec_IntAddToEntry( vCounts, Entry, 1 );
            }
            if ( k == Vec_IntSize(vLevel)-1 )
                printf( "\n" );
        }

    Vec_IntForEachEntry( vCounts, Entry, j )
        if ( Entry )
            printf( "%d=%d(%.2f) ", j, Entry, 1.0*Vec_IntEntry(vRanks, j)/Entry );
    printf( "\n" );

    Vec_IntFree( vSupp );
    Vec_WrdFree( vTemp );
    Vec_IntFree( vRanks );
    Vec_IntFree( vCounts );
    return vInputs;
}

/**Function*************************************************************

  Synopsis    [Mark nodes whose function is exactly that of a Booth PP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Bit_t * Acec_MultMarkPPs( Gia_Man_t * p )
{
    word Saved[32] = {
        ABC_CONST(0xF335ACC0F335ACC0),
        ABC_CONST(0x35C035C035C035C0),
        ABC_CONST(0xD728D728D728D728),
        ABC_CONST(0xFD80FD80FD80FD80),
        ABC_CONST(0xACC0ACC0ACC0ACC0),
        ABC_CONST(0x7878787878787878),
        ABC_CONST(0x2828282828282828),
        ABC_CONST(0xD0D0D0D0D0D0D0D0),
        ABC_CONST(0x8080808080808080),
        ABC_CONST(0x8888888888888888),
        ABC_CONST(0xAAAAAAAAAAAAAAAA),
        ABC_CONST(0x5555555555555555),

        ABC_CONST(0xD5A8D5A8D5A8D5A8),
        ABC_CONST(0x2A572A572A572A57),
        ABC_CONST(0xF3C0F3C0F3C0F3C0),
        ABC_CONST(0x5858585858585858),
        ABC_CONST(0xA7A7A7A7A7A7A7A7),
        ABC_CONST(0x2727272727272727),
        ABC_CONST(0xD8D8D8D8D8D8D8D8)
    };

    Vec_Bit_t * vRes = Vec_BitStart( Gia_ManObjNum(p) );
    Vec_Wrd_t * vTemp = Vec_WrdStart( Gia_ManObjNum(p) );
    Vec_Int_t * vSupp = Vec_IntAlloc( 100 );
    int i, iObj, nProds = 0;
    Gia_ManCleanMark0(p);
    Gia_ManForEachAndId( p, iObj )
    {
        word Truth = Gia_ObjComputeTruth6Cis( p, Abc_Var2Lit(iObj, 0), vSupp, vTemp );
        if ( Vec_IntSize(vSupp) > 6  )
            continue;
        vSupp->nSize = Abc_Tt6MinBase( &Truth, vSupp->pArray, vSupp->nSize );
        if ( Vec_IntSize(vSupp) > 5  )
            continue;
        for ( i = 0; i < 32 && Saved[i]; i++ )
        {
            if ( Truth == Saved[i] || Truth == ~Saved[i] )
            {
                Vec_BitWriteEntry( vRes, iObj, 1 );
                nProds++;
                break;
            }
        }
    }
    Gia_ManCleanMark0(p);
    printf( "Collected %d pps.\n", nProds );
    Vec_IntFree( vSupp );
    Vec_WrdFree( vTemp );
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_MultFindPPs_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vBold )
{
    Gia_Obj_t * pObj;
    pObj = Gia_ManObj( p, iObj );
    if ( pObj->fMark0 )
        return;
    pObj->fMark0 = 1;
    if ( !Gia_ObjIsAnd(pObj) )
        return;
    Acec_MultFindPPs_rec( p, Gia_ObjFaninId0(pObj, iObj), vBold );
    Acec_MultFindPPs_rec( p, Gia_ObjFaninId1(pObj, iObj), vBold );
    Vec_IntPush( vBold, iObj );
}
Vec_Int_t * Acec_MultFindPPs( Gia_Man_t * p )
{
    word Saved[32] = {
        ABC_CONST(0xF335ACC0F335ACC0),
        ABC_CONST(0x35C035C035C035C0),
        ABC_CONST(0xD728D728D728D728),
        ABC_CONST(0xFD80FD80FD80FD80),
        ABC_CONST(0xACC0ACC0ACC0ACC0),
        ABC_CONST(0x7878787878787878),
        ABC_CONST(0x2828282828282828),
        ABC_CONST(0xD0D0D0D0D0D0D0D0),
        ABC_CONST(0x8080808080808080),
        ABC_CONST(0x8888888888888888),
        ABC_CONST(0xAAAAAAAAAAAAAAAA),
        ABC_CONST(0x5555555555555555),

        ABC_CONST(0xD5A8D5A8D5A8D5A8),
        ABC_CONST(0x2A572A572A572A57),
        ABC_CONST(0xF3C0F3C0F3C0F3C0),
        ABC_CONST(0x5858585858585858),
        ABC_CONST(0xA7A7A7A7A7A7A7A7),
        ABC_CONST(0x2727272727272727),
        ABC_CONST(0xD8D8D8D8D8D8D8D8)
    };

    Vec_Int_t * vBold = Vec_IntAlloc( 100 );
    Vec_Int_t * vSupp = Vec_IntAlloc( 100 );
    Vec_Wrd_t * vTemp = Vec_WrdStart( Gia_ManObjNum(p) );
    int i, iObj, nProds = 0;
    Gia_ManCleanMark0(p);
    Gia_ManForEachAndId( p, iObj )
    {
        word Truth = Gia_ObjComputeTruth6Cis( p, Abc_Var2Lit(iObj, 0), vSupp, vTemp );
        if ( Vec_IntSize(vSupp) > 6  )
            continue;
        vSupp->nSize = Abc_Tt6MinBase( &Truth, vSupp->pArray, vSupp->nSize );
        if ( Vec_IntSize(vSupp) > 5  )
            continue;
        for ( i = 0; i < 32 && Saved[i]; i++ )
        {
            if ( Truth == Saved[i] || Truth == ~Saved[i] )
            {
                //printf( "*** Node %d is PP with support %d.\n", iObj, Vec_IntSize(vSupp) );
                Acec_MultFindPPs_rec( p, iObj, vBold );
                nProds++;
                break;
            }
        }
        /*
        if ( Saved[i] )
        {
            printf( "Obj = %4d  ", iObj );
            Extra_PrintHex( stdout, (unsigned*)&Truth, Vec_IntSize(vSupp) );
            if ( Vec_IntSize(vSupp) == 4 ) printf( "    " );
            if ( Vec_IntSize(vSupp) == 3 ) printf( "      " );
            if ( Vec_IntSize(vSupp) <= 2 ) printf( "       " );
            printf( "  " );
            Vec_IntPrint( vSupp );
        }
        */
    }
    Gia_ManCleanMark0(p);
    printf( "Collected %d pps and %d nodes.\n", nProds, Vec_IntSize(vBold) );

    Vec_IntFree( vSupp );
    Vec_WrdFree( vTemp );
    return vBold;
}
Vec_Bit_t * Acec_BoothFindPPG( Gia_Man_t * p )
{
    Vec_Bit_t * vIgnore = Vec_BitStart( Gia_ManObjNum(p) );
    Vec_Int_t * vMap = Acec_MultFindPPs( p );
    int i, Entry;
    Vec_IntForEachEntry( vMap, Entry, i )
        Vec_BitWriteEntry( vIgnore, Entry, 1 );
    Vec_IntFree( vMap );
    return vIgnore;
}
void Acec_MultFindPPsTest( Gia_Man_t * p )
{
    Vec_Int_t * vBold = Acec_MultFindPPs( p );
    Gia_ManShow( p, vBold, 1, 0, 0 );
    Vec_IntFree( vBold );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

