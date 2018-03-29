/**CFile****************************************************************

  FileName    [aigIso.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Graph isomorphism package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigIso.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"

ABC_NAMESPACE_IMPL_START

/*
#define ISO_MASK 0x3FF
static int s_1kPrimes[ISO_MASK+1] = 
{
    901403,984877,908741,966307,924437,965639,918787,931067,982621,917669,981473,936407,990487,926077,922897,970861,
    942317,961747,979717,978947,940157,987821,981221,917713,983083,992231,928253,961187,991817,927643,923129,934291,
    998071,967567,961087,988661,910031,930481,904489,974167,941351,959911,963811,921463,900161,934489,905629,930653,
    901819,909457,939871,924083,915113,937969,928457,946291,973787,912869,994093,959279,905803,995219,949903,911011,
    986707,995053,930583,955511,928307,930889,968729,911507,949043,939359,961679,918041,937681,909091,963913,923539,
    929587,953347,917573,913037,995387,976483,986239,946949,922489,917887,957553,931529,929813,949567,941683,905161,
    928819,932417,900089,935903,926587,914467,967361,944833,945881,941741,915949,903407,904157,971863,993893,963607,
    918943,912463,980957,962963,968089,904513,963763,907363,904097,904093,991343,918347,986983,986659,935819,903569,
    929171,913933,999749,923123,961531,935861,915053,994853,943511,969923,927191,968333,949391,950959,968311,991409,
    911681,987101,904027,975259,907399,946223,907259,900409,957221,901063,974657,912337,979001,970147,982301,968213,
    923959,964219,935443,950161,989251,936127,985679,958159,930077,971899,944857,956083,914293,941981,909481,909047,
    960527,958183,970687,914827,949051,928159,933551,964423,914041,915869,929953,901367,914219,975551,912391,917809,
    991499,904781,949153,959887,961957,970943,947741,941263,984541,951437,984301,947423,905761,964913,971357,927709,
    916441,941933,956993,988243,921197,905453,922081,950813,946331,998561,929023,937421,956231,907651,977897,905491,
    960173,931837,955217,911951,990643,971021,949439,988453,996781,951497,906011,944309,911293,917123,983803,928097,
    977747,928703,949957,919189,925513,923953,904997,986351,930689,902009,912007,906757,955793,926803,906809,962743,
    911917,909329,949021,974651,959083,945367,905137,948377,931757,945409,920279,915007,960121,920609,946163,946391,
    928903,932951,944329,901529,959809,918469,978643,911159,982573,965411,962233,911269,953273,974437,907589,992269,
    929399,980431,905693,968267,970481,911089,950557,913799,920407,974489,909863,918529,975277,929323,971549,969181,
    972787,964267,939971,943763,940483,971501,921637,945341,955211,920701,978349,969041,929861,904103,908539,995369,
    995567,917471,908879,993821,947783,954599,978463,914519,942869,947263,988343,914657,956987,903641,943343,991063,
    985403,926327,982829,958439,942017,960353,944987,934793,948971,999331,990767,915199,912211,946459,997019,965059,
    924907,983233,943273,945359,919613,933883,928927,942763,994087,996211,918971,924871,938491,957139,918839,914629,
    974329,900577,952823,941641,900461,946997,983123,935149,923693,908419,995651,912871,987067,920201,913921,929209,
    962509,974599,972001,920273,922099,951781,958549,909971,975133,937207,929941,961397,980677,923579,980081,942199,
    940319,942979,912349,942691,986989,947711,972343,932663,937877,940369,919571,927187,981439,932353,952313,915947,
    915851,974041,989381,921029,997013,999199,914801,918751,997327,992843,982133,932051,964861,903979,937463,916781,
    944389,986719,958369,961451,917767,954367,949853,934939,958807,975797,949699,957097,980773,969989,934907,909281,
    904679,909833,991741,946769,908381,932447,957889,981697,905701,919033,999023,993541,912953,911719,934603,925019,
    989341,912269,917789,981049,959149,989909,960521,952183,922627,936253,910957,972047,945037,940399,928313,928471,
    962459,959947,927541,917333,926899,911837,985631,955127,922729,911171,900511,926251,918209,943477,955277,959773,
    971039,917353,955313,930301,990799,957731,917519,938507,988111,911657,999721,968917,934537,903073,921703,966227,
    904661,998213,954307,931309,909331,933643,910099,958627,914533,902903,950149,972721,915157,969037,988219,944137,
    976411,952873,964787,970927,968963,920741,975187,966817,982909,975281,931907,959267,980711,924617,975691,962309,
    976307,932209,989921,907969,947927,932207,945397,948929,904903,938563,961691,977671,963173,927149,951061,966547,
    937661,983597,948139,948041,982759,941093,993703,910097,902347,990307,978217,996763,904919,924641,902299,929549,
    977323,975071,932917,996293,925579,925843,915487,917443,999541,943043,919109,959879,912173,986339,939193,939599,
    927077,977183,966521,959471,991943,985951,942187,932557,904297,972337,931751,964097,942341,966221,929113,960131,
    906427,970133,996511,925637,971651,983443,981703,933613,939749,929029,958043,961511,957241,901079,950479,975493,
    985799,909401,945601,911077,978359,948151,950333,968879,978727,961151,957823,950393,960293,915683,971513,915659,
    943841,902477,916837,911161,958487,963691,949607,935707,987607,901613,972557,938947,931949,919021,982217,914737,
    913753,971279,981683,915631,907807,970421,983173,916099,984587,912049,981391,947747,966233,932101,991733,969757,
    904283,996601,979807,974419,964693,931537,917251,967961,910093,989321,988129,997307,963427,999221,962447,991171,
    993137,914339,964973,908617,968567,920497,980719,949649,912239,907367,995623,906779,914327,918131,983113,962993,
    977849,914941,932681,905713,932579,923977,965507,916469,984119,931981,998423,984407,993841,901273,910799,939847,
    997153,971429,994927,912631,931657,968377,927833,920149,978041,947449,993233,927743,939737,975017,961861,984539,
    938857,977437,950921,963659,923917,932983,922331,982393,983579,935537,914357,973051,904531,962077,990281,989231,
    910643,948281,961141,911839,947413,923653,982801,903883,931943,930617,928679,962119,969977,926921,999773,954181,
    963019,973411,918139,959719,918823,941471,931883,925273,918173,949453,946993,945457,959561,968857,935603,978283,
    978269,947389,931267,902599,961189,947621,920039,964049,947603,913259,997811,943843,978277,972119,929431,918257,
    991663,954043,910883,948797,929197,985057,990023,960961,967139,923227,923371,963499,961601,971591,976501,989959,
    908731,951331,989887,925307,909299,949159,913447,969797,959449,976957,906617,901213,922667,953731,960199,960049,
    985447,942061,955613,965443,947417,988271,945887,976369,919823,971353,962537,929963,920473,974177,903649,955777,
    963877,973537,929627,994013,940801,985709,995341,936319,904681,945817,996617,953191,952859,934889,949513,965407,
    988357,946801,970391,953521,905413,976187,968419,940669,908591,976439,915731,945473,948517,939181,935183,978067,
    907663,967511,968111,981599,913907,933761,994933,980557,952073,906557,935621,914351,967903,949129,957917,971821,
    925937,926179,955729,966871,960737,968521,949997,956999,961273,962683,990377,908851,932231,929749,932149,966971,
    922079,978149,938453,958313,995381,906259,969503,922321,918947,972443,916411,935021,944429,928643,952199,918157,
    917783,998497,944777,917771,936731,999953,975157,908471,989557,914189,933787,933157,938953,922931,986569,964363,
    906473,963419,941467,946079,973561,957431,952429,965267,978473,924109,979529,991901,988583,918259,961991,978037,
    938033,949967,986071,986333,974143,986131,963163,940553,950933,936587,923407,950357,926741,959099,914891,976231,
    949387,949441,943213,915353,983153,975739,934243,969359,926557,969863,961097,934463,957547,916501,904901,928231,
    903673,974359,932219,916933,996019,934399,955813,938089,907693,918223,969421,940903,940703,913027,959323,940993,
    937823,906691,930841,923701,933259,911959,915601,960251,985399,914359,930827,950251,975379,903037,905783,971237
};
*/

/*
#define ISO_MASK 0x7F
static int s_1kPrimes[ISO_MASK+1] = { 
    1009, 1049, 1093, 1151, 1201, 1249, 1297, 1361, 1427, 1459, 
    1499, 1559, 1607, 1657, 1709, 1759, 1823, 1877, 1933, 1997, 
    2039, 2089, 2141, 2213, 2269, 2311, 2371, 2411, 2467, 2543, 
    2609, 2663, 2699, 2741, 2797, 2851, 2909, 2969, 3037, 3089, 
    3169, 3221, 3299, 3331, 3389, 3461, 3517, 3557, 3613, 3671, 
    3719, 3779, 3847, 3907, 3943, 4013, 4073, 4129, 4201, 4243, 
    4289, 4363, 4441, 4493, 4549, 4621, 4663, 4729, 4793, 4871, 
    4933, 4973, 5021, 5087, 5153, 5227, 5281, 5351, 5417, 5471, 
    5519, 5573, 5651, 5693, 5749, 5821, 5861, 5923, 6011, 6073, 
    6131, 6199, 6257, 6301, 6353, 6397, 6481, 6563, 6619, 6689, 
    6737, 6803, 6863, 6917, 6977, 7027, 7109, 7187, 7237, 7309, 
    7393, 7477, 7523, 7561, 7607, 7681, 7727, 7817, 7877, 7933, 
    8011, 8039, 8059, 8081, 8093, 8111, 8123, 8147
};
*/

/*
#define ISO_MASK 0x7
static int s_1kPrimes[ISO_MASK+1] = { 
    12582917, 25165843, 50331653, 100663319, 201326611, 402653189, 805306457, 1610612741
};
*/

#define ISO_MASK 0x3FF
static unsigned int s_1kPrimes[ISO_MASK+1] = 
//#define ISO_MASK 0xFF
//static int s_1kPrimes[0x3FF+1] = 
{
    0x38c19891,0xde37b0ed,0xdebcd025,0xe19b7bbe,0x7e7ebd0e,0xaeed03a1,0x811230dc,0x10bfece0,
    0xb3b23fb1,0x74176098,0xc34ec7c5,0x6bef8939,0xc40be5e3,0x2ab51a09,0xafc17cea,0x0dccc7a2,
    0xdf7db34d,0x1009c96f,0x93fd7494,0x54385b33,0x6f36eed8,0xa1953f82,0xfbd1144a,0xde533a46,
    0x23aa1cad,0x9a18943c,0xb65000d8,0x867e9974,0xe7880035,0xf9931ad4,0xcfca1e45,0x6b5aec96,
    0xe9c1a119,0xfa4968c5,0x94cf93da,0xe8c9eac4,0x95884242,0x1bba52c7,0x9232c321,0x9cec8658,
    0x984b6ad9,0x18a6eed3,0x950353e2,0x6222f6eb,0xdfbedd47,0xef0f9023,0xac932a26,0x590eaf55,
    0x97d0a034,0xdc36cd2e,0x22736b37,0xdc9066b0,0x2eb2f98b,0x5d9c7baf,0x85747c9e,0x8aca1055,
    0x50d66b74,0x2f01ae9e,0xa1a80123,0x3e1ce2dc,0xebedbc57,0x4e68bc34,0x855ee0cf,0x17275120,
    0x2ae7f2df,0xf71039eb,0x7c283eec,0x70cd1137,0x7cf651f3,0xa87bfa7a,0x14d87f02,0xe82e197d,
    0x8d8a5ebe,0x1e6a15dc,0x197d49db,0x5bab9c89,0x4b55dea7,0x55dede49,0x9a6a8080,0xe5e51035,
    0xe148d658,0x8a17eb3b,0xe22e4b38,0xe5be2a9a,0xbe938cbb,0x3b981069,0x7f9c0c8e,0xf756df10,
    0x8fa783f7,0x252062ce,0x3dc46b4b,0xf70f6432,0x3f378276,0x44b137a1,0x2bf74b77,0x04892ed6,
    0xfd318de1,0xd58c235e,0x94c6d25b,0x7aa5f218,0x35c9e921,0x5732fbbb,0x06026481,0xf584a44f,
    0x946e1b5f,0x8463d5b2,0x4ebca7b2,0x54887b15,0x08d1e804,0x5b22067d,0x794580f6,0xb351ea43,
    0xbce555b9,0x19ae2194,0xd32f1396,0x6fc1a7f1,0x1fd8a867,0x3a89fdb0,0xea49c61c,0x25f8a879,
    0xde1e6437,0x7c74afca,0x8ba63e50,0xb1572074,0xe4655092,0xdb6f8b1c,0xc2955f3c,0x327f85ba,
    0x60a17021,0x95bd261d,0xdea94f28,0x04528b65,0xbe0109cc,0x26dd5688,0x6ab2729d,0xc4f029ce,
    0xacf7a0be,0x4c912f55,0x34c06e65,0x4fbb938e,0x1533fb5f,0x03da06bd,0x48262889,0xc2523d7d,
    0x28a71d57,0x89f9713a,0xf574c551,0x7a99deb5,0x52834d91,0x5a6f4484,0xc67ba946,0x13ae698f,
    0x3e390f34,0x34fc9593,0x894c7932,0x6cf414a3,0xdb7928ab,0x13a3b8a3,0x4b381c1d,0xa10b54cb,
    0x55359d9d,0x35a3422a,0x58d1b551,0x0fd4de20,0x199eb3f4,0x167e09e2,0x3ee6a956,0x5371a7fa,
    0xd424efda,0x74f521c5,0xcb899ff6,0x4a42e4f4,0x747917b6,0x4b08df0b,0x090c7a39,0x11e909e4,
    0x258e2e32,0xd9fad92d,0x48fe5f69,0x0545cde6,0x55937b37,0x9b4ae4e4,0x1332b40e,0xc3792351,
    0xaff982ef,0x4dba132a,0x38b81ef1,0x28e641bf,0x227208c1,0xec4bbe37,0xc4e1821c,0x512c9d09,
    0xdaef1257,0xb63e7784,0x043e04d7,0x9c2cea47,0x45a0e59a,0x281315ca,0x849f0aac,0xa4071ed3,
    0x0ef707b3,0xfe8dac02,0x12173864,0x471f6d46,0x24a53c0a,0x35ab9265,0xbbf77406,0xa2144e79,
    0xb39a884a,0x0baf5b6d,0xcccee3dd,0x12c77584,0x2907325b,0xfd1adcd2,0xd16ee972,0x345ad6c1,
    0x315ebe66,0xc7ad2b8d,0x99e82c8d,0xe52da8c8,0xba50f1d3,0x66689cd8,0x2e8e9138,0x43e15e74,
    0xf1ced14d,0x188ec52a,0xe0ef3cbb,0xa958aedc,0x4107a1bc,0x5a9e7a3e,0x3bde939f,0xb5b28d5a,
    0x596fe848,0xe85ad00c,0x0b6b3aae,0x44503086,0x25b5695c,0xc0c31dcd,0x5ee617f0,0x74d40c3a,
    0xd2cb2b9f,0x1e19f5fa,0x81e24faf,0xa01ed68f,0xcee172fc,0x7fdf2e4d,0x002f4774,0x664f82dd,
    0xc569c39a,0xa2d4dcbe,0xaadea306,0xa4c947bf,0xa413e4e3,0x81fb5486,0x8a404970,0x752c980c,
    0x98d1d881,0x5c932c1e,0xeee65dfb,0x37592cdd,0x0fd4e65b,0xad1d383f,0x62a1452f,0x8872f68d,
    0xb58c919b,0x345c8ee3,0xb583a6d6,0x43d72cb3,0x77aaa0aa,0xeb508242,0xf2db64f8,0x86294328,
    0x82211731,0x1239a9d5,0x673ba5de,0xaf4af007,0x44203b19,0x2399d955,0xa175cd12,0x595928a7,
    0x6918928b,0xde3126bb,0x6c99835c,0x63ba1fa2,0xdebbdff0,0x3d02e541,0xd6f7aac6,0xe80b4cd0,
    0xd0fa29f1,0x804cac5e,0x2c226798,0x462f624c,0xad05b377,0x22924fcd,0xfbea205c,0x1b47586d,
    0x214d04ab,0xe093e487,0x10607ada,0x42b261cc,0x1a85e0f6,0xb851bfc3,0x77d5591c,0xda13f344,
    0xc39c4c00,0xe60d75fc,0x7edae36a,0x3e4ac3ec,0x8bc38db4,0xe848dce9,0xb2407d4d,0x0d79c61e,
    0x1e6c293a,0x7bc30986,0xdf18cb8f,0x23003172,0x6948e3fa,0x9b7e4f09,0x14b3b339,0x9c8078c2,
    0x9a47c29f,0x85bb45ec,0x9ca35a93,0xd7db5227,0x1d9b0a10,0xb7fbbfe9,0x05b72426,0x6f30b2fa,
    0x9fb44078,0xedffd3f8,0x1b02b7bc,0x43e3cfd3,0x0d44293e,0x25c8d552,0xedd3f85d,0x6f921c8c,
    0x953cca0c,0x9c975b70,0xc6bd0b53,0x4f204f3e,0xa3cc69cc,0xceec390b,0x34905626,0x82ad5d41,
    0xe46589a5,0x7989841d,0x045d7d9f,0xe49b7b2f,0x46baf101,0x996f92de,0x427566c8,0x918a1ee1,
    0xf4baa589,0x6bdff7c7,0x3c6936ea,0xe85bfb70,0x5d96ea26,0x6d5a8991,0x7f0a528d,0x852f0634,
    0x2ec04501,0x5ca15c35,0xd8695e7a,0x456876c7,0x52e97b83,0x34b4c5ed,0x54d73fbb,0x44a6be01,
    0xf8019155,0x33d55a31,0x3fe51c99,0xe1cb94fd,0x8c39cd60,0xd585efba,0x2765579b,0xb8f7ed12,
    0xbb04b2cd,0xd8859981,0xd966988d,0xa68bfeda,0x73110705,0x38d6aec0,0x613bc275,0xc7283c2d,
    0xe051d0b1,0x32b8c2ee,0x0e73fb9e,0x7ab05c25,0x6ff652b9,0x45aeabc6,0x6be1a891,0x5b43531b,
    0xcd711248,0x2b777d40,0x46956d16,0x474355a8,0xe872d6c6,0xe4158d96,0xabe03036,0x5b4fd9a4,
    0xeceba1db,0xaac9713f,0xe953153b,0xf44a9229,0x460cba72,0xfd13fdf6,0x8bbf82ae,0xaf55778f,
    0xa447a5b2,0x98b647b3,0x5f351c57,0x69d0bb71,0xf14d2706,0x78b1a3af,0x7206c73f,0x3f5cd4a6,
    0x5c0e4515,0xdb46a663,0x10c3a9b0,0x8eda7748,0x52bb44c9,0x3df62689,0xc83e2732,0xf36c99af,
    0x7ec7a94c,0x5c823718,0x6586e58e,0x4b726e75,0xcfe03a05,0x34eb2f4b,0xf4eec9cf,0xb38d6d73,
    0x71fafa9e,0x0371a29a,0xc405f83b,0x889f49c2,0xd1443000,0x469665bf,0x609ed65d,0x67c8f9ba,
    0x9d2f6055,0xb91b8eb1,0x96c809fe,0x2d6ab0f5,0xc16d4f04,0x590171ab,0x73d67526,0xf724e44c,
    0x6aef02b7,0x6489a325,0x4458401e,0x22d94ad7,0x05e5be57,0x5634fad8,0x951fcf70,0x4baad7f0,
    0x40c1090d,0xedc28abd,0x343cc6e4,0x4ff7c030,0x0734a641,0x2635a90e,0x2e000c84,0x4b617a70,
    0x872e9c9e,0x3dceeb42,0xd0fcc398,0x9cc9b9c8,0x2de02f0c,0xaf0e2614,0xa60253aa,0xe0517492,
    0xa7bde4b4,0x3bb66d7d,0x7f215e82,0xf259de66,0xe17380fe,0xdbc718b4,0x66abcc29,0xf0826e1f,
    0x08f60995,0xce81b933,0xa832c0e9,0x37aed7d4,0x8a75c261,0x916627b4,0xd486a04b,0x64fd0fde,
    0x1261328a,0xe037772f,0xb5b71117,0x55d04bd8,0x8f6c1c7b,0xb9f5fcdd,0x5918f756,0x25c90099,
    0x2e8787db,0x58e14e38,0x0d397852,0x32c8e33b,0x5ae2b795,0x3a7b3ff7,0x5eebf893,0x1aeee816,
    0xc2ef31d0,0x1d86e615,0x183f1de3,0xb89d46c4,0x525ebbf6,0xfe0198ca,0x4986cc4a,0x2a75701e,
    0x382158b1,0x192ee88f,0x3e512912,0xcd571c3d,0xdf694fe8,0xec8ead1d,0x83719ac3,0x3f654079,
    0xf6a623c5,0x33e1fc6e,0xe7f7c426,0x5bff0f6c,0x698a9bb1,0xec2a29ba,0x75358b45,0x40c6ffef,
    0x6605bb55,0x53a8c97a,0x7bba1569,0x499bc51b,0x5849c89a,0xe6ddb267,0x8659c719,0x14a05548,
    0xeec648a9,0x618af87a,0x62214521,0x7f36e610,0x152efeeb,0xc2b0f0ed,0x1d657588,0xa5fcec4b,
    0xf872f109,0x46903038,0x04b57b97,0xe5d51b14,0x06c264ec,0x68aa8d14,0xa4e1bed8,0xdae169c2,
    0xeb90debd,0xe8c11a7a,0xcafce013,0x63820cee,0x948c23e5,0xc1d42ea3,0x8256c951,0x9b587773,
    0x2fa8380c,0x30255e09,0x1a809cdc,0xe1446068,0x2714621d,0xb3347d64,0x1f4cbf3d,0xd068bc26,
    0x2c422740,0x06c4a3ad,0x5dc9d63c,0x4724bf48,0x28e34add,0x27d5221d,0xe349c7e2,0x6119e0a5,
    0x4ae7d29f,0x53a7912d,0xfc5db779,0x7d28d357,0xfd80036d,0x06bcc597,0x36d70a8a,0x37738cb7,
    0xf11e6272,0xfdd5d153,0x5dc666dc,0x6b5a415d,0x1073b415,0x36f30d9a,0x807daf7b,0x387f6823,
    0x8970fe00,0xee560be5,0xea8c0bad,0xfac2b422,0xc845861d,0xa181a2ee,0x29c4dffd,0x4d441bb2,
    0x7a64cf93,0x0c33e6ac,0x0a35d034,0x1067d26d,0x8c7da0cc,0x2d6e2d5a,0x9932c25a,0x5fca4e2c,
    0x2c82fd71,0x41730b70,0x244bdbb9,0x96514307,0xc6a32a6b,0xc4c256a7,0x38517fd8,0x541aa859,
    0x0752afe3,0x741e22f9,0xa2565483,0x7588b0b9,0xdd404e42,0x4d86c49d,0x6fa93fc1,0x163bd200,
    0x745d0d31,0x8d3dd20e,0xebdc64db,0x9315c149,0x39db3299,0xb0c22004,0xa4c0295b,0x8b3573eb,
    0xd92a40a3,0x73d6c379,0x67673309,0xdaff1d7f,0x42fcfeb8,0xd57c11a4,0x402066ef,0x9d1134e0,
    0x9f417081,0x10f49e00,0x7e7ee855,0x314e6d25,0x602bdbe6,0xa4be4045,0xac511dc4,0x33d6bda8,
    0x2f2bc412,0x4b9c0b6c,0x98aaab06,0x7f0a5801,0xfbf1f16d,0x058f54ae,0x4fd97724,0x348cb50b,
    0xef6f659f,0x0cd8b184,0x1d71a666,0xae3c87dd,0x7bd56793,0xe0f8f6a8,0x90429c55,0x8a3cc4d0,
    0x49957b70,0x3baf3912,0x755efebb,0xa5eca17f,0x486065a1,0x1dffcefb,0xd914b3a0,0x1ced93c1,
    0xa4262dcd,0xc12a4adc,0x08f6de4e,0x4c204faf,0xca1815de,0xa4af836f,0x91d5e44d,0xd2a7caa4,
    0x68a9a3fe,0x844f8dac,0x3fc36c67,0x8be23937,0x69879d94,0x5d0dbecb,0x1f0f59a4,0x94721142,
    0xfca6064a,0x6d28aa48,0x804cd04e,0x4a3906de,0x8e352509,0x302326d9,0xed4937ed,0x4a570e63,
    0xcaa57efb,0x64bd4878,0x3419334a,0x712e5f6b,0x9fa9d687,0x06f8645f,0x620ca96f,0xdc5d6cce,
    0x392f3257,0x52140f06,0xc4b3bda4,0xe8c7eba7,0x066bd754,0xc5941f26,0xe6dfd573,0x328dd14d,
    0xb1cb4ba7,0x1d37a9b8,0x96a195a5,0x970e535a,0x290351b8,0x570000f6,0xe14ae341,0x35ede6a6,
    0x9a02f032,0xaf2ebb58,0x146be492,0x670b3e4b,0x72fa6cfd,0xa243af97,0xbbd5fc21,0xcb8852a2,
    0x5d5b4a42,0xeefff0ac,0xa59ad1b6,0x3ec55544,0x48d64f69,0x9065d3d0,0xdd09485b,0xdd63bd09,
    0x605e811d,0xc4b4ed7d,0xb0b58558,0x0644400b,0x12222346,0x086f146a,0xad6dee36,0x5488d1a3,
    0x0c93bc0c,0x18555d92,0x9f4427bf,0xa590a66a,0x3a630fda,0xf1681c2e,0x948a16fb,0x16fe3338,
    0xc9832357,0xd1e8e6b5,0xd9cfe034,0x05b22f26,0x27233c6e,0x355890e1,0xfbe6eaf3,0x0dcd8e8f,
    0x00b5df46,0xd97730ac,0xc6dfd8ff,0x0cb1840a,0x41e9d249,0xbb471b4e,0x480b8f63,0x1ffe8871,
    0x17b11821,0x1709e440,0xcefb3668,0xa4954ddd,0xf03ef8b5,0x6b3e633c,0xe5813096,0x3697c9a6,
    0x7800f52f,0x73a7aa39,0x59ac23b7,0xb4663166,0x9ca9d6f8,0x2d441072,0x38cef3d3,0x39a3faf6,
    0x89299fb9,0xd558295f,0xcf79c633,0x232dd96e,0xadb2955b,0xe962cbb9,0xab7c0061,0x1027c329,
    0xb4b43e07,0x25240a7a,0x98ea4825,0xdbf2edbd,0x8be15d26,0x879f3cd9,0xa4138089,0xa32dcb06,
    0x602af961,0x4b13f451,0x1c87d0d5,0xc3bb141b,0x9ebf55a1,0xef030a9a,0x8d199b93,0xdabcbb56,
    0xf412f80f,0x302e90ad,0xc4d9878b,0xc392f650,0x4fd3a614,0x0a96ddc4,0xcd1878c7,0x9ddd3ae1,
    0xdaf46458,0xba7c8656,0xf667948f,0xc37e3c23,0x04a577c6,0xbe615f1e,0xcc97406c,0xd497f16f,
    0x79586586,0xd2057d14,0x1bb92028,0xab888e5e,0x26bef100,0xf46b3671,0xf21f1acc,0x67f288c8,
    0x39c722df,0x61d21eaf,0x9c5853a0,0x63b693c7,0x1ea53c0a,0x95bc0a85,0xa7372f2d,0x3ef6a6b3,
    0x82c9c4ac,0x4dea10ee,0xdfcb543d,0xd412f427,0x53e27f2c,0x875d8422,0x5367a7d8,0x41acf3fa,
    0xbce47234,0x8056fb9a,0x4e9a4c48,0xe4a45945,0x2cfee3ae,0xb4554b10,0x5e37a915,0x591b1963,
    0x4fa255c1,0xe01c897b,0x504e6208,0x7c7368eb,0x13808fd7,0x02ac0816,0x30305d2c,0x6c4bbdb7,
    0xa48a9599,0x57466059,0x4c6ebfc7,0x8587ccdf,0x6ff0abf0,0x5b6b63fe,0x31d9ec64,0x63458abd,
    0x21245905,0xccdb28fc,0xac828acb,0xe5e82bea,0xa7d76141,0xa699038e,0xcaba7e06,0x2710253f,
    0x2ff9c94d,0x26e48a2c,0xd713ec5e,0x869f2ed4,0x25bcd712,0xcb3e918f,0x615c3a5a,0x9fb43903,
    0x37900eb9,0x4f682db0,0x35a80dc6,0x4eb27c65,0x502735ab,0xb163b4c8,0x604649a8,0xb23a6cd3,
    0x9f715091,0x2e6fbb51,0x2ec9144b,0x272cbe65,0x90a0a453,0x420e1503,0x2d00d338,0x4aa96675,
    0xd025b61c,0xab02d9d7,0x2afe2a37,0xf8129b9b,0x4db04c54,0x654a5c06,0x3213ff51,0x4e09d0b1,
    0x333369a3,0xae27310a,0x91d076d0,0xa96ebcd0,0xefde54f4,0x021c309a,0xd506f53d,0xa5635251,
    0x2f23916e,0x1fe86bb1,0xcbc62160,0x2147c8cc,0xdeb3e47c,0x028e3987,0x8061de42,0x39be931b,
    0x2b7e54c5,0xe64d2f96,0x4069522d,0x4aa66857,0x83b62634,0x4ba72095,0x9aade2a9,0xf1223cd9,
    0x91cbddf0,0xec5d237f,0x593f3280,0x0b924439,0x446f4063,0xc66f8d8c,0x8b7128ba,0xb597f451,
    0xc8925236,0x1720f235,0x7cd2e9a0,0x4c130339,0x8a665a52,0x5bef2733,0xba35d9bc,0x6d26644c,
    0x385cdce1,0x509e4280,0x12aa9ed7,0xf5314d21,0xbe249d4a,0xf32e9753,0x91821cf9,0x01d63491,
    0x49afa237,0x80e0bc27,0x844d796b,0xeff5ccb7,0x46303091,0x743484b4,0x77de1ab7,0x5ab00bea,
    0x6316cd81,0x8ded07f4,0x3845a3a5,0x206625c4,0x8c123c6f,0xc80a971e,0xd4d4fa3f,0x5eba911d,
    0xee168406,0x61cdcbad,0x981a44cd,0x718d030f,0xf653e92e,0xd5b77859,0x11e9e5d9,0xf6fe6ff3,
    0x5239f010,0xe289b21b,0x0b52832b,0xca700c62,0xee7a5e15,0x8543ce2c,0x94a703cc,0x0b844d34,
    0xf70638e5,0xfa286206,0xf8778906,0x1419e883,0xdb0fc46b,0xbeb74261,0xc6957b62,0x8352d2a8,
    0x460586ce,0x90b28336,0xc9107ea8,0x3590403b,0x259a4279,0x6a1a7bbe,0x0f3b76e1,0x4872a716,
    0xa5bfff13,0x8b30be72,0xe5a68957,0x17dbbc52,0x33a40187,0x7074220c,0xd8221b92,0x40ec7448,
    0x7dbbcdfc,0xd5a9bb83,0xb4c0d25c,0xa0040390,0x6fb429dc,0xb8cede12,0x87d193bd,0x55c6e004
};

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define ISO_NUM_INTS 3

typedef struct Iso_Obj_t_ Iso_Obj_t;
struct Iso_Obj_t_
{
    // hashing entries (related to the parameter ISO_NUM_INTS!)
    unsigned      Level     : 30;
    unsigned      nFinNeg   :  2;
    unsigned      FaninSig;
    unsigned      FanoutSig;
    // other data
    int           iNext;          // hash table entry
    int           iClass;         // next one in class
    int           Id;             // unique ID
};

typedef struct Iso_Man_t_ Iso_Man_t;
struct Iso_Man_t_
{
    Aig_Man_t *   pAig;           // user's AIG manager
    Iso_Obj_t *   pObjs;          // isomorphism objects
    int           nObjIds;        // counter of object IDs
    int           nClasses;       // total number of classes
    int           nEntries;       // total number of entries
    int           nSingles;       // total number of singletons
    int           nObjs;          // total objects
    int           nBins;          // hash table size
    int *         pBins;          // hash table 
    Vec_Ptr_t *   vSingles;       // singletons 
    Vec_Ptr_t *   vClasses;       // other classes
    Vec_Ptr_t *   vTemp1;         // other classes
    Vec_Ptr_t *   vTemp2;         // other classes
    abctime       timeHash;
    abctime       timeFout;
    abctime       timeSort;
    abctime       timeOther;
    abctime       timeTotal;
};

static inline Iso_Obj_t *  Iso_ManObj( Iso_Man_t * p, int i )            { assert( i >= 0 && i < p->nObjs ); return i ? p->pObjs + i : NULL;                }
static inline int          Iso_ObjId( Iso_Man_t * p, Iso_Obj_t * pObj )  { assert( pObj > p->pObjs && pObj < p->pObjs + p->nObjs ); return pObj - p->pObjs; }
static inline Aig_Obj_t *  Iso_AigObj( Iso_Man_t * p, Iso_Obj_t * q )    { return Aig_ManObj( p->pAig, Iso_ObjId(p, q) );                                   }

#define Iso_ManForEachObj( p, pObj, i )   \
    for ( i = 1; (i < p->nObjs) && ((pObj) = Iso_ManObj(p, i)); i++ ) if ( pIso->Level == -1 ) {} else

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

//extern void Iso_ReadPrimes( char * pFileName );
//Iso_ReadPrimes( "primes.txt" );

/**Function*************************************************************

  Synopsis    [Read primes from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Iso_ReadPrimes( char * pFileName )
{
    FILE * pFile;
    int Nums[10000];
    int i, j, Temp, nSize = 0;
    // read the numbers
    pFile = fopen( pFileName, "rb" );
    while ( fscanf( pFile, "%d", Nums + nSize++ ) == 1 );
    fclose( pFile );
    assert( nSize >= (1<<10) );
    // randomly permute
    srand( 111 );
    for ( i = 0; i < nSize; i++ )
    {
        j = rand() % nSize;
        Temp = Nums[i];
        Nums[i] = Nums[j];
        Nums[j] = Temp;
    }
    // write out
    for ( i = 0; i < 64; i++ )
    {
        printf( "    " );
        for ( j = 0; j < 16; j++ )
            printf( "%d,", Nums[i*16+j] );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Read primes from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Iso_FindNumbers()
{
    unsigned Nums[1024];
    unsigned char * pNums = (unsigned char *)Nums;
    int i, j;
    srand( 111 );
    for ( i = 0; i < 1024 * 4; i++ )
        pNums[i] = (unsigned char)rand();
    // write out
    for ( i = 0; i < 128; i++ )
    {
        printf( "    " );
        for ( j = 0; j < 8; j++ )
            printf( "0x%08x,", Nums[i*8+j] );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Iso_ManObjCount_rec( Aig_Man_t * p, Aig_Obj_t * pObj, int * pnNodes, int * pnEdges )
{
    if ( Aig_ObjIsCi(pObj) )
        return;
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    Iso_ManObjCount_rec( p, Aig_ObjFanin0(pObj), pnNodes, pnEdges );
    Iso_ManObjCount_rec( p, Aig_ObjFanin1(pObj), pnNodes, pnEdges );
    (*pnEdges) += Aig_ObjFaninC0(pObj) + Aig_ObjFaninC1(pObj);
    (*pnNodes)++;
}
void Iso_ManObjCount( Aig_Man_t * p, Aig_Obj_t * pObj, int * pnNodes, int * pnEdges )
{
    assert( Aig_ObjIsNode(pObj) );
    *pnNodes = *pnEdges = 0;
    Aig_ManIncrementTravId( p );
    Iso_ManObjCount_rec( p, pObj, pnNodes, pnEdges );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Iso_Man_t * Iso_ManStart( Aig_Man_t * pAig )
{
    Iso_Man_t * p;
    p = ABC_CALLOC( Iso_Man_t, 1 );
    p->pAig     = pAig;
    p->nObjs    = Aig_ManObjNumMax( pAig );
    p->pObjs    = ABC_CALLOC( Iso_Obj_t, p->nObjs );
    p->nBins    = Abc_PrimeCudd( p->nObjs );
    p->pBins    = ABC_CALLOC( int, p->nBins );    
    p->vSingles = Vec_PtrAlloc( 1000 );
    p->vClasses = Vec_PtrAlloc( 1000 );
    p->vTemp1   = Vec_PtrAlloc( 1000 );
    p->vTemp2   = Vec_PtrAlloc( 1000 );
    p->nObjIds  = 1;
    return p;
}
void Iso_ManStop( Iso_Man_t * p, int fVerbose )
{
    if ( fVerbose )
    {
        p->timeOther = p->timeTotal - p->timeHash - p->timeFout;
        ABC_PRTP( "Building ", p->timeFout,               p->timeTotal );
        ABC_PRTP( "Hashing  ", p->timeHash-p->timeSort,   p->timeTotal );
        ABC_PRTP( "Sorting  ", p->timeSort,               p->timeTotal );
        ABC_PRTP( "Other    ", p->timeOther,              p->timeTotal );
        ABC_PRTP( "TOTAL    ", p->timeTotal,              p->timeTotal );
    }
    Vec_PtrFree( p->vTemp1 );
    Vec_PtrFree( p->vTemp2 );
    Vec_PtrFree( p->vClasses );
    Vec_PtrFree( p->vSingles );
    ABC_FREE( p->pBins );
    ABC_FREE( p->pObjs );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Compares two objects by their signature.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Iso_ObjCompare( Iso_Obj_t ** pp1, Iso_Obj_t ** pp2 )
{
    return -memcmp( *pp1, *pp2, sizeof(int) * ISO_NUM_INTS );
}

/**Function*************************************************************

  Synopsis    [Compares two objects by their ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Iso_ObjCompareByData( Aig_Obj_t ** pp1, Aig_Obj_t ** pp2 )
{
    Aig_Obj_t * pIso1 = *pp1;
    Aig_Obj_t * pIso2 = *pp2;
    assert( Aig_ObjIsCi(pIso1) || Aig_ObjIsCo(pIso1) );
    assert( Aig_ObjIsCi(pIso2) || Aig_ObjIsCo(pIso2) );
    return pIso1->iData - pIso2->iData;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Iso_ObjHash( Iso_Obj_t * pIso, int nBins )
{
    static unsigned BigPrimes[8] = {12582917, 25165843, 50331653, 100663319, 201326611, 402653189, 805306457, 1610612741};
    unsigned * pArray = (unsigned *)pIso;
    unsigned i, Value = 0;
    assert( ISO_NUM_INTS < 8 );
    for ( i = 0; i < ISO_NUM_INTS; i++ )
        Value ^= BigPrimes[i] * pArray[i];
    return Value % nBins;
}
static inline int Iso_ObjHashAdd( Iso_Man_t * p, Iso_Obj_t * pIso )
{
    Iso_Obj_t * pThis;
    int * pPlace = p->pBins + Iso_ObjHash( pIso, p->nBins );
    p->nEntries++;
    for ( pThis = Iso_ManObj(p, *pPlace); 
          pThis; pPlace = &pThis->iNext, 
          pThis = Iso_ManObj(p, *pPlace) )
        if ( Iso_ObjCompare( &pThis, &pIso ) == 0 ) // equal signatures
        {
            if ( pThis->iClass == 0 )
            {
                p->nClasses++;
                p->nSingles--;
            }
            // add to the list
            pIso->iClass = pThis->iClass;
            pThis->iClass = Iso_ObjId( p, pIso );
            return 1;
        }
    // create new list
    *pPlace = Iso_ObjId( p, pIso );
    p->nSingles++;
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Iso_ManCollectClasses( Iso_Man_t * p )
{
    Iso_Obj_t * pIso;
    int i;
    abctime clk = Abc_Clock();
    Vec_PtrClear( p->vSingles );
    Vec_PtrClear( p->vClasses );
    for ( i = 0; i < p->nBins; i++ )
    {
        for ( pIso = Iso_ManObj(p, p->pBins[i]); pIso; pIso = Iso_ManObj(p, pIso->iNext) )
        {
            assert( pIso->Id == 0 );
            if ( pIso->iClass )
                Vec_PtrPush( p->vClasses, pIso );
            else 
                Vec_PtrPush( p->vSingles, pIso );
        }
    }
    clk = Abc_Clock();
    Vec_PtrSort( p->vSingles, (int (*)(void))Iso_ObjCompare );
    Vec_PtrSort( p->vClasses, (int (*)(void))Iso_ObjCompare );
    p->timeSort += Abc_Clock() - clk;
    assert( Vec_PtrSize(p->vSingles) == p->nSingles );
    assert( Vec_PtrSize(p->vClasses) == p->nClasses );
    // assign IDs to singletons
    Vec_PtrForEachEntry( Iso_Obj_t *, p->vSingles, pIso, i )
        if ( pIso->Id == 0 )
            pIso->Id = p->nObjIds++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Iso_Man_t * Iso_ManCreate( Aig_Man_t * pAig )
{
    int fUseXor = 0;
    Iso_Man_t * p;
    Iso_Obj_t * pIso, * pIsoF;
    Aig_Obj_t * pObj, * pObjLi;
    int i;
    p = Iso_ManStart( pAig );

    // create TFI signatures
    Aig_ManForEachObj( pAig, pObj, i )
    {
        if ( Aig_ObjIsCo(pObj) )
            continue;
        pIso = p->pObjs + i;
        pIso->Level = pObj->Level;
//        pIso->nFinNeg = Aig_ObjFaninC0(pObj) + Aig_ObjFaninC1(pObj);

        assert( pIso->FaninSig == 0 );
        assert( pIso->FanoutSig == 0 );
        if ( fUseXor )
        {
            if ( Aig_ObjIsNode(pObj) )
            {
                pIsoF = p->pObjs + Aig_ObjFaninId0(pObj);
                pIso->FaninSig ^= pIsoF->FaninSig;
                pIso->FaninSig ^= s_1kPrimes[Abc_Var2Lit(pIso->Level, Aig_ObjFaninC0(pObj)) & ISO_MASK];

                pIsoF = p->pObjs + Aig_ObjFaninId1(pObj);
                pIso->FaninSig ^= pIsoF->FaninSig;
                pIso->FaninSig ^= s_1kPrimes[Abc_Var2Lit(pIso->Level, Aig_ObjFaninC1(pObj)) & ISO_MASK];
            }
        }
        else
        {
            if ( Aig_ObjIsNode(pObj) )
            {
                pIsoF = p->pObjs + Aig_ObjFaninId0(pObj);
                pIso->FaninSig += pIsoF->FaninSig;
                pIso->FaninSig += pIso->Level * s_1kPrimes[Abc_Var2Lit(pIso->Level, Aig_ObjFaninC0(pObj)) & ISO_MASK];

                pIsoF = p->pObjs + Aig_ObjFaninId1(pObj);
                pIso->FaninSig += pIsoF->FaninSig;
                pIso->FaninSig += pIso->Level * s_1kPrimes[Abc_Var2Lit(pIso->Level, Aig_ObjFaninC1(pObj)) & ISO_MASK];
            }
        }
    }

    // create TFO signatures
    Aig_ManForEachObjReverse( pAig, pObj, i )
    {
        if ( Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj) )
            continue;
        pIso = p->pObjs + i;
        if ( fUseXor )
        {
            if ( Aig_ObjIsNode(pObj) )
            {
                pIsoF = p->pObjs + Aig_ObjFaninId0(pObj);
                pIsoF->FanoutSig ^= pIso->FanoutSig;
                pIsoF->FanoutSig ^= s_1kPrimes[Abc_Var2Lit(pIso->Level, Aig_ObjFaninC0(pObj)) & ISO_MASK];

                pIsoF = p->pObjs + Aig_ObjFaninId1(pObj);
                pIsoF->FanoutSig ^= pIso->FanoutSig;
                pIsoF->FanoutSig ^= s_1kPrimes[Abc_Var2Lit(pIso->Level, Aig_ObjFaninC1(pObj)) & ISO_MASK];
            }
            else if ( Aig_ObjIsCo(pObj) )
            {
                pIsoF = p->pObjs + Aig_ObjFaninId0(pObj);
                pIsoF->FanoutSig ^= pIso->FanoutSig;
                pIsoF->FanoutSig ^= s_1kPrimes[Abc_Var2Lit(pIso->Level, Aig_ObjFaninC0(pObj)) & ISO_MASK];
            }
        }
        else
        {
            if ( Aig_ObjIsNode(pObj) )
            {
                pIsoF = p->pObjs + Aig_ObjFaninId0(pObj);
                pIsoF->FanoutSig += pIso->FanoutSig;
                pIsoF->FanoutSig += pIso->Level * s_1kPrimes[Abc_Var2Lit(pIso->Level, Aig_ObjFaninC0(pObj)) & ISO_MASK];

                pIsoF = p->pObjs + Aig_ObjFaninId1(pObj);
                pIsoF->FanoutSig += pIso->FanoutSig;
                pIsoF->FanoutSig += pIso->Level * s_1kPrimes[Abc_Var2Lit(pIso->Level, Aig_ObjFaninC1(pObj)) & ISO_MASK];
            }
            else if ( Aig_ObjIsCo(pObj) )
            {
                pIsoF = p->pObjs + Aig_ObjFaninId0(pObj);
                pIsoF->FanoutSig += pIso->FanoutSig;
                pIsoF->FanoutSig += pIso->Level * s_1kPrimes[Abc_Var2Lit(pIso->Level, Aig_ObjFaninC0(pObj)) & ISO_MASK];
            }
        }
    }

    // consider flops
    Aig_ManForEachLiLoSeq( p->pAig, pObjLi, pObj, i )
    {
        if ( Aig_ObjFaninId0(pObjLi) == 0 ) // ignore constant!
            continue;
        pIso  = Iso_ManObj( p, Aig_ObjId(pObj) );
        pIsoF = Iso_ManObj( p, Aig_ObjFaninId0(pObjLi) );

        assert( pIso->FaninSig == 0 );
        pIso->FaninSig = pIsoF->FaninSig;

//        assert( pIsoF->FanoutSig == 0 );
        pIsoF->FanoutSig += pIso->FanoutSig;
    }
/*
    Aig_ManForEachObj( pAig, pObj, i )
    {
        pIso = p->pObjs + i;
        Aig_ObjPrint( pAig, pObj );
        printf( "Lev = %4d.  Pos = %4d.  FaninSig = %10d.  FanoutSig = %10d.\n", 
            pIso->Level, pIso->nFinNeg, pIso->FaninSig, pIso->FanoutSig );
    }
*/
    // add to the hash table
    Aig_ManForEachObj( pAig, pObj, i )
    {
        if ( !Aig_ObjIsCi(pObj) && !Aig_ObjIsNode(pObj) )
            continue;
        pIso = p->pObjs + i;
        Iso_ObjHashAdd( p, pIso );
    }
    // derive classes for the first time
    Iso_ManCollectClasses( p );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates adjacency lists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Iso_ManAssignAdjacency( Iso_Man_t * p )
{
    int fUseXor = 0;
    Iso_Obj_t * pIso, * pIsoF;
    Aig_Obj_t * pObj, * pObjLi;
    int i;

    // create TFI signatures
    Aig_ManForEachObj( p->pAig, pObj, i )
    {
        pIso = p->pObjs + i;
        pIso->FaninSig = 0;
        pIso->FanoutSig = 0;

        if ( Aig_ObjIsCo(pObj) )
            continue;
        if ( fUseXor )
        {
            if ( Aig_ObjIsNode(pObj) )
            {
                pIsoF = p->pObjs + Aig_ObjFaninId0(pObj);
                pIso->FaninSig ^= pIsoF->FaninSig;
                if ( pIsoF->Id )
                    pIso->FaninSig ^= s_1kPrimes[Abc_Var2Lit(pIsoF->Id, Aig_ObjFaninC0(pObj)) & ISO_MASK];

                pIsoF = p->pObjs + Aig_ObjFaninId1(pObj);
                pIso->FaninSig ^= pIsoF->FaninSig;
                if ( pIsoF->Id )
                    pIso->FaninSig ^= s_1kPrimes[Abc_Var2Lit(pIsoF->Id, Aig_ObjFaninC1(pObj)) & ISO_MASK];
            }
        }
        else
        {
            if ( Aig_ObjIsNode(pObj) )
            {
                pIsoF = p->pObjs + Aig_ObjFaninId0(pObj);
                pIso->FaninSig += pIsoF->FaninSig;
                if ( pIsoF->Id )
                    pIso->FaninSig += pIsoF->Id * s_1kPrimes[Abc_Var2Lit(pIsoF->Id, Aig_ObjFaninC0(pObj)) & ISO_MASK];

                pIsoF = p->pObjs + Aig_ObjFaninId1(pObj);
                pIso->FaninSig += pIsoF->FaninSig;
                if ( pIsoF->Id )
                    pIso->FaninSig += pIsoF->Id * s_1kPrimes[Abc_Var2Lit(pIsoF->Id, Aig_ObjFaninC1(pObj)) & ISO_MASK];
            }
        }
    }
    // create TFO signatures
    Aig_ManForEachObjReverse( p->pAig, pObj, i )
    {
        if ( Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj) )
            continue;
        pIso = p->pObjs + i;
        assert( !Aig_ObjIsCo(pObj) || pIso->Id == 0 );
        if ( fUseXor )
        {
            if ( Aig_ObjIsNode(pObj) )
            {
                pIsoF = p->pObjs + Aig_ObjFaninId0(pObj);
                pIsoF->FanoutSig ^= pIso->FanoutSig;
                if ( pIso->Id )
                    pIsoF->FanoutSig ^= s_1kPrimes[Abc_Var2Lit(pIso->Id, Aig_ObjFaninC0(pObj)) & ISO_MASK];

                pIsoF = p->pObjs + Aig_ObjFaninId1(pObj);
                pIsoF->FanoutSig ^= pIso->FanoutSig;
                if ( pIso->Id )
                    pIsoF->FanoutSig ^= s_1kPrimes[Abc_Var2Lit(pIso->Id, Aig_ObjFaninC1(pObj)) & ISO_MASK];
            }
            else if ( Aig_ObjIsCo(pObj) )
            {
                pIsoF = p->pObjs + Aig_ObjFaninId0(pObj);
                pIsoF->FanoutSig ^= pIso->FanoutSig;
                if ( pIso->Id )
                    pIsoF->FanoutSig ^= s_1kPrimes[Abc_Var2Lit(pIso->Id, Aig_ObjFaninC0(pObj)) & ISO_MASK];
            }
        }
        else
        {
            if ( Aig_ObjIsNode(pObj) )
            {
                pIsoF = p->pObjs + Aig_ObjFaninId0(pObj);
                pIsoF->FanoutSig += pIso->FanoutSig;
                if ( pIso->Id )
                    pIsoF->FanoutSig += pIso->Id * s_1kPrimes[Abc_Var2Lit(pIso->Id, Aig_ObjFaninC0(pObj)) & ISO_MASK];

                pIsoF = p->pObjs + Aig_ObjFaninId1(pObj);
                pIsoF->FanoutSig += pIso->FanoutSig;
                if ( pIso->Id )
                    pIsoF->FanoutSig += pIso->Id * s_1kPrimes[Abc_Var2Lit(pIso->Id, Aig_ObjFaninC1(pObj)) & ISO_MASK];
            }
            else if ( Aig_ObjIsCo(pObj) )
            {
                pIsoF = p->pObjs + Aig_ObjFaninId0(pObj);
                pIsoF->FanoutSig += pIso->FanoutSig;
                if ( pIso->Id )
                    pIsoF->FanoutSig += pIso->Id * s_1kPrimes[Abc_Var2Lit(pIso->Id, Aig_ObjFaninC0(pObj)) & ISO_MASK];
            }
        }
    }

    // consider flops
    Aig_ManForEachLiLoSeq( p->pAig, pObjLi, pObj, i )
    {
        if ( Aig_ObjFaninId0(pObjLi) == 0 ) // ignore constant!
            continue;
        pIso  = Iso_ManObj( p, Aig_ObjId(pObj) );
        pIsoF = Iso_ManObj( p, Aig_ObjFaninId0(pObjLi) );
        assert( pIso->FaninSig == 0 );
//        assert( pIsoF->FanoutSig == 0 );

        if ( fUseXor )
        {
            pIso->FaninSig = pIsoF->FaninSig;
            if ( pIsoF->Id )
                pIso->FaninSig ^= s_1kPrimes[Abc_Var2Lit(pIsoF->Id, Aig_ObjFaninC0(pObjLi)) & ISO_MASK];

            pIsoF->FanoutSig += pIso->FanoutSig;
            if ( pIso->Id )
                pIsoF->FanoutSig ^= s_1kPrimes[Abc_Var2Lit(pIso->Id, Aig_ObjFaninC0(pObjLi)) & ISO_MASK];
        }
        else
        {
            pIso->FaninSig = pIsoF->FaninSig;
            if ( pIsoF->Id )
                pIso->FaninSig += pIsoF->Id * s_1kPrimes[Abc_Var2Lit(pIsoF->Id, Aig_ObjFaninC0(pObjLi)) & ISO_MASK];

            pIsoF->FanoutSig += pIso->FanoutSig;
            if ( pIso->Id )
                pIsoF->FanoutSig += pIso->Id * s_1kPrimes[Abc_Var2Lit(pIso->Id, Aig_ObjFaninC0(pObjLi)) & ISO_MASK];
        }
    }
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Iso_ManPrintClasseSizes( Iso_Man_t * p )
{
    Iso_Obj_t * pIso, * pTemp;
    int i, Counter;
    Vec_PtrForEachEntry( Iso_Obj_t *, p->vClasses, pIso, i )
    {
        Counter = 0;
        for ( pTemp = pIso; pTemp; pTemp = Iso_ManObj(p, pTemp->iClass) )
            Counter++;
        printf( "%d ", Counter );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Iso_ManPrintClasses( Iso_Man_t * p, int fVerbose, int fVeryVerbose )
{
    int fOnlyCis = 0;
    Iso_Obj_t * pIso, * pTemp;
    int i;

    // count unique objects
    if ( fVerbose )
        printf( "Total objects =%7d.  Entries =%7d.  Classes =%7d.  Singles =%7d.\n", 
            p->nObjs, p->nEntries, p->nClasses, p->nSingles );

    if ( !fVeryVerbose )
        return;

    printf( "Non-trivial classes:\n" );
    Vec_PtrForEachEntry( Iso_Obj_t *, p->vClasses, pIso, i )
    {
        if ( fOnlyCis && pIso->Level > 0 )
            continue;

        printf( "%5d : {", i );
        for ( pTemp = pIso; pTemp; pTemp = Iso_ManObj(p, pTemp->iClass) )
        {
            if ( fOnlyCis )
                printf( " %d", Aig_ObjCioId( Iso_AigObj(p, pTemp) ) );
            else
            {
                Aig_Obj_t * pObj = Iso_AigObj(p, pTemp);
                if ( Aig_ObjIsNode(pObj) )
                    printf( " %d{%s%d(%d),%s%d(%d)}", Iso_ObjId(p, pTemp), 
                        Aig_ObjFaninC0(pObj)? "-": "+", Aig_ObjFaninId0(pObj), Aig_ObjLevel(Aig_ObjFanin0(pObj)), 
                        Aig_ObjFaninC1(pObj)? "-": "+", Aig_ObjFaninId1(pObj), Aig_ObjLevel(Aig_ObjFanin1(pObj)) );
                else
                    printf( " %d", Iso_ObjId(p, pTemp) );
            }
            printf( "(%d)", pTemp->Level );
        }        
        printf( " }\n" );
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Iso_ManRehashClassNodes( Iso_Man_t * p )
{
    Iso_Obj_t * pIso, * pTemp;
    int i;
    // collect nodes
    Vec_PtrClear( p->vTemp1 );
    Vec_PtrClear( p->vTemp2 );
    Vec_PtrForEachEntry( Iso_Obj_t *, p->vClasses, pIso, i )
    {
        for ( pTemp = pIso; pTemp; pTemp = Iso_ManObj(p, pTemp->iClass) )
            if ( pTemp->Id == 0 )
                Vec_PtrPush( p->vTemp1, pTemp );
            else
                Vec_PtrPush( p->vTemp2, pTemp );
    }
    // clean and add nodes
    p->nClasses = 0;       // total number of classes
    p->nEntries = 0;       // total number of entries
    p->nSingles = 0;       // total number of singletons
    memset( p->pBins, 0, sizeof(int) * p->nBins );
    Vec_PtrForEachEntry( Iso_Obj_t *, p->vTemp1, pTemp, i )
    {
        assert( pTemp->Id == 0 );
        pTemp->iClass = pTemp->iNext = 0;
        Iso_ObjHashAdd( p, pTemp );
    }
    Vec_PtrForEachEntry( Iso_Obj_t *, p->vTemp2, pTemp, i )
    {
        assert( pTemp->Id != 0 );
        pTemp->iClass = pTemp->iNext = 0;
    }
    // collect new classes
    Iso_ManCollectClasses( p );
}

/**Function*************************************************************

  Synopsis    [Find nodes with the min number of edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Iso_Obj_t * Iso_ManFindBestObj( Iso_Man_t * p, Iso_Obj_t * pIso )
{
    Iso_Obj_t * pTemp, * pBest = NULL;
    int nNodesBest = -1, nNodes;
    int nEdgesBest = -1, nEdges;
    assert( pIso->Id == 0 );
    if ( pIso->Level == 0 )
        return pIso;
    for ( pTemp = pIso; pTemp; pTemp = Iso_ManObj(p, pTemp->iClass) )
    {
        assert( pTemp->Id == 0 );
        Iso_ManObjCount( p->pAig, Iso_AigObj(p, pTemp), &nNodes, &nEdges );
//        printf( "%d,%d ", nNodes, nEdges );
        if ( nNodesBest < nNodes || (nNodesBest == nNodes && nEdgesBest < nEdges) )
        {
            nNodesBest = nNodes;
            nEdgesBest = nEdges;
            pBest = pTemp;
        }
    }
//    printf( "\n" );
    return pBest;
}

/**Function*************************************************************

  Synopsis    [Find nodes with the min number of edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Iso_ManBreakTies( Iso_Man_t * p, int fVerbose )
{
    int fUseOneBest = 0;
    Iso_Obj_t * pIso, * pTemp;
    int i, LevelStart = 0;
    pIso = (Iso_Obj_t *)Vec_PtrEntry( p->vClasses, 0 );
    LevelStart = pIso->Level;
    if ( fVerbose )
        printf( "Best level %d\n", LevelStart ); 
    Vec_PtrForEachEntry( Iso_Obj_t *, p->vClasses, pIso, i )
    {
        if ( (int)pIso->Level < LevelStart )
            break;
        if ( !fUseOneBest )
        {
            for ( pTemp = pIso; pTemp; pTemp = Iso_ManObj(p, pTemp->iClass) )
            {
                assert( pTemp->Id ==  0 );
                pTemp->Id = p->nObjIds++;
            }
            continue;
        }
        if ( pIso->Level == 0 )//&& pIso->nFoutPos + pIso->nFoutNeg == 0 )
        {
            for ( pTemp = pIso; pTemp; pTemp = Iso_ManObj(p, pTemp->iClass) )
                pTemp->Id = p->nObjIds++;
            continue;
        }
        pIso = Iso_ManFindBestObj( p, pIso );
        pIso->Id = p->nObjIds++;
    }
}

/**Function*************************************************************

  Synopsis    [Finalizes unification of combinational outputs.]

  Description [Assigns IDs to the unclassified CIs in the order of obj IDs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Iso_ManFinalize( Iso_Man_t * p )
{
    Vec_Int_t * vRes;
    Aig_Obj_t * pObj;
    int i;
    assert( p->nClasses == 0 );
    assert( Vec_PtrSize(p->vClasses) == 0 );
    // set canonical numbers
    Aig_ManForEachObj( p->pAig, pObj, i )
    {
        if ( !Aig_ObjIsCi(pObj) && !Aig_ObjIsNode(pObj) )
        {
            pObj->iData = -1;
            continue;
        }
        pObj->iData = Iso_ManObj(p, Aig_ObjId(pObj))->Id;
        assert( pObj->iData > 0 );
    }
    Aig_ManConst1(p->pAig)->iData = 0;
    // assign unique IDs to the CIs
    Vec_PtrClear( p->vTemp1 );
    Vec_PtrClear( p->vTemp2 );
    Aig_ManForEachCi( p->pAig, pObj, i )
    {
        assert( pObj->iData > 0 );
        if ( Aig_ObjCioId(pObj) >= Aig_ManCiNum(p->pAig) - Aig_ManRegNum(p->pAig) ) // flop
            Vec_PtrPush( p->vTemp2, pObj );
        else // PI
            Vec_PtrPush( p->vTemp1, pObj );
    }
    // sort CIs by their IDs
    Vec_PtrSort( p->vTemp1, (int (*)(void))Iso_ObjCompareByData );
    Vec_PtrSort( p->vTemp2, (int (*)(void))Iso_ObjCompareByData );
    // create the result
    vRes = Vec_IntAlloc( Aig_ManCiNum(p->pAig) );
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vTemp1, pObj, i )
        Vec_IntPush( vRes, Aig_ObjCioId(pObj) );
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vTemp2, pObj, i )
        Vec_IntPush( vRes, Aig_ObjCioId(pObj) );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Find nodes with the min number of edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Iso_ManDumpOneClass( Iso_Man_t * p )
{
    Vec_Ptr_t * vNodes = Vec_PtrAlloc( 100 );
    Iso_Obj_t * pIso, * pTemp;
    Aig_Man_t * pNew = NULL;
    assert( p->nClasses > 0 );
    pIso = (Iso_Obj_t *)Vec_PtrEntry( p->vClasses, 0 );
    assert( pIso->Id == 0 );
    for ( pTemp = pIso; pTemp; pTemp = Iso_ManObj(p, pTemp->iClass) )
    {
        assert( pTemp->Id == 0 );
        Vec_PtrPush( vNodes, Iso_AigObj(p, pTemp) );
    }
    pNew = Aig_ManDupNodes( p->pAig, vNodes );
    Vec_PtrFree( vNodes );
    Aig_ManShow( pNew, 0, NULL ); 
    Aig_ManStopP( &pNew );
}

/**Function*************************************************************

  Synopsis    [Finds canonical permutation of CIs and assigns unique IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManFindIsoPerm( Aig_Man_t * pAig, int fVerbose )
{
    int fVeryVerbose = 0;
    Vec_Int_t * vRes;
    Iso_Man_t * p;
    abctime clk = Abc_Clock(), clk2 = Abc_Clock();
    p = Iso_ManCreate( pAig );
    p->timeFout += Abc_Clock() - clk;
    Iso_ManPrintClasses( p, fVerbose, fVeryVerbose );
    while ( p->nClasses )
    {
        // assign adjacency to classes
        clk = Abc_Clock();
        Iso_ManAssignAdjacency( p );
        p->timeFout += Abc_Clock() - clk;
        // rehash the class nodes
        clk = Abc_Clock();
        Iso_ManRehashClassNodes( p );
        p->timeHash += Abc_Clock() - clk;
        Iso_ManPrintClasses( p, fVerbose, fVeryVerbose );
        // force refinement
        while ( p->nSingles == 0 && p->nClasses )
        {
//            Iso_ManPrintClasseSizes( p );
            // assign IDs to the topmost level of classes
            Iso_ManBreakTies( p, fVerbose );
            // assign adjacency to classes
            clk = Abc_Clock();
            Iso_ManAssignAdjacency( p );
            p->timeFout += Abc_Clock() - clk;
            // rehash the class nodes
            clk = Abc_Clock();
            Iso_ManRehashClassNodes( p );
            p->timeHash += Abc_Clock() - clk;
            Iso_ManPrintClasses( p, fVerbose, fVeryVerbose );
        }
    }
    p->timeTotal = Abc_Clock() - clk2;
//    printf( "IDs assigned = %d.  Objects = %d.\n", p->nObjIds, 1+Aig_ManCiNum(p->pAig)+Aig_ManNodeNum(p->pAig) );
    assert( p->nObjIds == 1+Aig_ManCiNum(p->pAig)+Aig_ManNodeNum(p->pAig) );
//    if ( p->nClasses )
//        Iso_ManDumpOneClass( p );
    vRes = Iso_ManFinalize( p );
    Iso_ManStop( p, fVerbose );
    return vRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

