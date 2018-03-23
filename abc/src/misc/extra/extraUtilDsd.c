/**CFile****************************************************************

  FileName    [extraUtilDsd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [File management utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extraUtilDsd.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "extra.h"
#include "misc/vec/vec.h"
#include "misc/vec/vecHsh.h"
#include "misc/util/utilTruth.h"
#include "bool/rsb/rsb.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Sdm_Dsd_t_ Sdm_Dsd_t;
struct Sdm_Dsd_t_
{
    int              nVars;                    // support size
    int              nAnds;                    // the number of AND gates
    int              nClauses;                 // the number of CNF clauses
    word             uTruth;                   // truth table
    char *           pStr;                     // description 
};

#define DSD_CLASS_NUM 595

static Sdm_Dsd_t s_DsdClass6[DSD_CLASS_NUM] = { 
    { 0,  0,  1, ABC_CONST(0x0000000000000000), "0" },   //    0
    { 1,  0,  2, ABC_CONST(0xAAAAAAAAAAAAAAAA), "a" },   //    1
    { 2,  1,  3, ABC_CONST(0x8888888888888888), "(ab)" },   //    2
    { 2,  3,  4, ABC_CONST(0x6666666666666666), "[ab]" },   //    3
    { 3,  2,  4, ABC_CONST(0x8080808080808080), "(abc)" },   //    4
    { 3,  2,  4, ABC_CONST(0x7070707070707070), "(!(ab)c)" },   //    5
    { 3,  4,  6, ABC_CONST(0x7878787878787878), "[(ab)c]" },   //    6
    { 3,  4,  5, ABC_CONST(0x6060606060606060), "([ab]c)" },   //    7
    { 3,  6,  8, ABC_CONST(0x9696969696969696), "[abc]" },   //    8
    { 3,  3,  4, ABC_CONST(0xCACACACACACACACA), "<abc>" },   //    9
    { 4,  3,  5, ABC_CONST(0x8000800080008000), "(abcd)" },   //   10
    { 4,  3,  5, ABC_CONST(0x7F007F007F007F00), "(!(abc)d)" },   //   11
    { 4,  5,  8, ABC_CONST(0x7F807F807F807F80), "[(abc)d]" },   //   12
    { 4,  3,  5, ABC_CONST(0x7000700070007000), "(!(ab)cd)" },   //   13
    { 4,  3,  5, ABC_CONST(0x8F008F008F008F00), "(!(!(ab)c)d)" },   //   14
    { 4,  5,  8, ABC_CONST(0x8F708F708F708F70), "[(!(ab)c)d]" },   //   15
    { 4,  5,  7, ABC_CONST(0x7800780078007800), "([(ab)c]d)" },   //   16
    { 4,  7, 12, ABC_CONST(0x8778877887788778), "[(ab)cd]" },   //   17
    { 4,  5,  6, ABC_CONST(0x6000600060006000), "([ab]cd)" },   //   18
    { 4,  5,  6, ABC_CONST(0x9F009F009F009F00), "(!([ab]c)d)" },   //   19
    { 4,  7, 10, ABC_CONST(0x9F609F609F609F60), "[([ab]c)d]" },   //   20
    { 4,  7,  9, ABC_CONST(0x9600960096009600), "([abc]d)" },   //   21
    { 4,  9, 16, ABC_CONST(0x6996699669966996), "[abcd]" },   //   22
    { 4,  4,  5, ABC_CONST(0xCA00CA00CA00CA00), "(<abc>d)" },   //   23
    { 4,  6,  8, ABC_CONST(0x35CA35CA35CA35CA), "[<abc>d]" },   //   24
    { 4,  3,  6, ABC_CONST(0x0777077707770777), "(!(ab)!(cd))" },   //   25
    { 4,  5,  9, ABC_CONST(0x7888788878887888), "[(ab)(cd)]" },   //   26
    { 4,  5,  7, ABC_CONST(0x0666066606660666), "([ab]!(cd))" },   //   27
    { 4,  7,  8, ABC_CONST(0x0660066006600660), "([ab][cd])" },   //   28
    { 4,  4,  6, ABC_CONST(0xCAAACAAACAAACAAA), "<ab(cd)>" },   //   29
    { 4,  6,  8, ABC_CONST(0xACCAACCAACCAACCA), "<ab[cd]>" },   //   30
    { 4,  4,  5, ABC_CONST(0xF088F088F088F088), "<(ab)cd>" },   //   31
    { 4,  6,  6, ABC_CONST(0xF066F066F066F066), "<[ab]cd>" },   //   32
    { 5,  4,  6, ABC_CONST(0x8000000080000000), "(abcde)" },   //   33
    { 5,  4,  6, ABC_CONST(0x7FFF00007FFF0000), "(!(abcd)e)" },   //   34
    { 5,  6, 10, ABC_CONST(0x7FFF80007FFF8000), "[(abcd)e]" },   //   35
    { 5,  4,  6, ABC_CONST(0x7F0000007F000000), "(!(abc)de)" },   //   36
    { 5,  4,  6, ABC_CONST(0x80FF000080FF0000), "(!(!(abc)d)e)" },   //   37
    { 5,  6, 10, ABC_CONST(0x80FF7F0080FF7F00), "[(!(abc)d)e]" },   //   38
    { 5,  6,  9, ABC_CONST(0x7F8000007F800000), "([(abc)d]e)" },   //   39
    { 5,  8, 16, ABC_CONST(0x807F7F80807F7F80), "[(abc)de]" },   //   40
    { 5,  4,  6, ABC_CONST(0x7000000070000000), "(!(ab)cde)" },   //   41
    { 5,  4,  6, ABC_CONST(0x8FFF00008FFF0000), "(!(!(ab)cd)e)" },   //   42
    { 5,  6, 10, ABC_CONST(0x8FFF70008FFF7000), "[(!(ab)cd)e]" },   //   43
    { 5,  4,  6, ABC_CONST(0x8F0000008F000000), "(!(!(ab)c)de)" },   //   44
    { 5,  4,  6, ABC_CONST(0x70FF000070FF0000), "(!(!(!(ab)c)d)e)" },   //   45
    { 5,  6, 10, ABC_CONST(0x70FF8F0070FF8F00), "[(!(!(ab)c)d)e]" },   //   46
    { 5,  6,  9, ABC_CONST(0x8F7000008F700000), "([(!(ab)c)d]e)" },   //   47
    { 5,  8, 16, ABC_CONST(0x708F8F70708F8F70), "[(!(ab)c)de]" },   //   48
    { 5,  6,  8, ABC_CONST(0x7800000078000000), "([(ab)c]de)" },   //   49
    { 5,  6,  8, ABC_CONST(0x87FF000087FF0000), "(!([(ab)c]d)e)" },   //   50
    { 5,  8, 14, ABC_CONST(0x87FF780087FF7800), "[([(ab)c]d)e]" },   //   51
    { 5,  8, 13, ABC_CONST(0x8778000087780000), "([(ab)cd]e)" },   //   52
    { 5, 10, 24, ABC_CONST(0x7887877878878778), "[(ab)cde]" },   //   53
    { 5,  6,  7, ABC_CONST(0x6000000060000000), "([ab]cde)" },   //   54
    { 5,  6,  7, ABC_CONST(0x9FFF00009FFF0000), "(!([ab]cd)e)" },   //   55
    { 5,  8, 12, ABC_CONST(0x9FFF60009FFF6000), "[([ab]cd)e]" },   //   56
    { 5,  6,  7, ABC_CONST(0x9F0000009F000000), "(!([ab]c)de)" },   //   57
    { 5,  6,  7, ABC_CONST(0x60FF000060FF0000), "(!(!([ab]c)d)e)" },   //   58
    { 5,  8, 12, ABC_CONST(0x60FF9F0060FF9F00), "[(!([ab]c)d)e]" },   //   59
    { 5,  8, 11, ABC_CONST(0x9F6000009F600000), "([([ab]c)d]e)" },   //   60
    { 5, 10, 20, ABC_CONST(0x609F9F60609F9F60), "[([ab]c)de]" },   //   61
    { 5,  8, 10, ABC_CONST(0x9600000096000000), "([abc]de)" },   //   62
    { 5,  8, 10, ABC_CONST(0x69FF000069FF0000), "(!([abc]d)e)" },   //   63
    { 5, 10, 18, ABC_CONST(0x69FF960069FF9600), "[([abc]d)e]" },   //   64
    { 5, 10, 17, ABC_CONST(0x6996000069960000), "([abcd]e)" },   //   65
    { 5, 12, 32, ABC_CONST(0x9669699696696996), "[abcde]" },   //   66
    { 5,  5,  6, ABC_CONST(0xCA000000CA000000), "(<abc>de)" },   //   67
    { 5,  5,  6, ABC_CONST(0x35FF000035FF0000), "(!(<abc>d)e)" },   //   68
    { 5,  7, 10, ABC_CONST(0x35FFCA0035FFCA00), "[(<abc>d)e]" },   //   69
    { 5,  7,  9, ABC_CONST(0x35CA000035CA0000), "([<abc>d]e)" },   //   70
    { 5,  9, 16, ABC_CONST(0xCA3535CACA3535CA), "[<abc>de]" },   //   71
    { 5,  4,  7, ABC_CONST(0x0777000007770000), "(!(ab)!(cd)e)" },   //   72
    { 5,  4,  7, ABC_CONST(0xF8880000F8880000), "(!(!(ab)!(cd))e)" },   //   73
    { 5,  6, 12, ABC_CONST(0xF8880777F8880777), "[(!(ab)!(cd))e]" },   //   74
    { 5,  6, 10, ABC_CONST(0x7888000078880000), "([(ab)(cd)]e)" },   //   75
    { 5,  6, 10, ABC_CONST(0x8777000087770000), "(![(ab)(cd)]e)" },   //   76
    { 5,  8, 18, ABC_CONST(0x8777788887777888), "[(ab)(cd)e]" },   //   77
    { 5,  6,  8, ABC_CONST(0x0666000006660000), "([ab]!(cd)e)" },   //   78
    { 5,  6,  8, ABC_CONST(0xF9990000F9990000), "(!([ab]!(cd))e)" },   //   79
    { 5,  8, 14, ABC_CONST(0xF9990666F9990666), "[([ab]!(cd))e]" },   //   80
    { 5,  8,  9, ABC_CONST(0x0660000006600000), "([ab][cd]e)" },   //   81
    { 5,  8,  9, ABC_CONST(0xF99F0000F99F0000), "(!([ab][cd])e)" },   //   82
    { 5, 10, 16, ABC_CONST(0xF99F0660F99F0660), "[([ab][cd])e]" },   //   83
    { 5,  5,  7, ABC_CONST(0xCAAA0000CAAA0000), "(<ab(cd)>e)" },   //   84
    { 5,  7, 12, ABC_CONST(0x3555CAAA3555CAAA), "[<ab(cd)>e]" },   //   85
    { 5,  7,  9, ABC_CONST(0xACCA0000ACCA0000), "(<ab[cd]>e)" },   //   86
    { 5,  9, 16, ABC_CONST(0x5335ACCA5335ACCA), "[<ab[cd]>e]" },   //   87
    { 5,  5,  6, ABC_CONST(0xF0880000F0880000), "(<(ab)cd>e)" },   //   88
    { 5,  5,  6, ABC_CONST(0x0F7700000F770000), "(!<(ab)cd>e)" },   //   89
    { 5,  7, 10, ABC_CONST(0x0F77F0880F77F088), "[<(ab)cd>e]" },   //   90
    { 5,  7,  7, ABC_CONST(0xF0660000F0660000), "(<[ab]cd>e)" },   //   91
    { 5,  9, 12, ABC_CONST(0x0F99F0660F99F066), "[<[ab]cd>e]" },   //   92
    { 5,  4,  8, ABC_CONST(0x007F7F7F007F7F7F), "(!(abc)!(de))" },   //   93
    { 5,  6, 12, ABC_CONST(0x7F8080807F808080), "[(abc)(de)]" },   //   94
    { 5,  4,  7, ABC_CONST(0x008F8F8F008F8F8F), "(!(!(ab)c)!(de))" },   //   95
    { 5,  6, 12, ABC_CONST(0x8F7070708F707070), "[(!(ab)c)(de)]" },   //   96
    { 5,  6, 10, ABC_CONST(0x0078787800787878), "([(ab)c]!(de))" },   //   97
    { 5,  6,  9, ABC_CONST(0x009F9F9F009F9F9F), "(!([ab]c)!(de))" },   //   98
    { 5,  8, 15, ABC_CONST(0x9F6060609F606060), "[([ab]c)(de)]" },   //   99
    { 5,  8, 13, ABC_CONST(0x0096969600969696), "([abc]!(de))" },   //  100
    { 5,  5,  7, ABC_CONST(0x00CACACA00CACACA), "(<abc>!(de))" },   //  101
    { 5,  7, 12, ABC_CONST(0x35CACACA35CACACA), "[<abc>(de)]" },   //  102
    { 5,  6,  9, ABC_CONST(0x007F7F00007F7F00), "(!(abc)[de])" },   //  103
    { 5,  6,  8, ABC_CONST(0x008F8F00008F8F00), "(!(!(ab)c)[de])" },   //  104
    { 5,  8, 11, ABC_CONST(0x0078780000787800), "([(ab)c][de])" },   //  105
    { 5,  8, 10, ABC_CONST(0x009F9F00009F9F00), "(!([ab]c)[de])" },   //  106
    { 5, 10, 14, ABC_CONST(0x0096960000969600), "([abc][de])" },   //  107
    { 5,  7,  8, ABC_CONST(0x00CACA0000CACA00), "(<abc>[de])" },   //  108
    { 5,  5,  8, ABC_CONST(0xCAAAAAAACAAAAAAA), "<ab(cde)>" },   //  109
    { 5,  5,  8, ABC_CONST(0xACCCAAAAACCCAAAA), "<ab(!(cd)e)>" },   //  110
    { 5,  7, 12, ABC_CONST(0xACCCCAAAACCCCAAA), "<ab[(cd)e]>" },   //  111
    { 5,  7, 10, ABC_CONST(0xACCAAAAAACCAAAAA), "<ab([cd]e)>" },   //  112
    { 5,  9, 16, ABC_CONST(0xCAACACCACAACACCA), "<ab[cde]>" },   //  113
    { 5,  6,  8, ABC_CONST(0xCCAACACACCAACACA), "<ab<cde>>" },   //  114
    { 5,  5,  7, ABC_CONST(0xC0AAAAAAC0AAAAAA), "<a(bc)(de)>" },   //  115
    { 5,  7,  8, ABC_CONST(0x3CAAAAAA3CAAAAAA), "<a[bc](de)>" },   //  116
    { 5,  5,  8, ABC_CONST(0xF0888888F0888888), "<(ab)c(de)>" },   //  117
    { 5,  7, 10, ABC_CONST(0x88F0F08888F0F088), "<(ab)c[de]>" },   //  118
    { 5,  7, 10, ABC_CONST(0xF0666666F0666666), "<[ab]c(de)>" },   //  119
    { 5,  9, 12, ABC_CONST(0x66F0F06666F0F066), "<[ab]c[de]>" },   //  120
    { 5,  5,  6, ABC_CONST(0xF0008888F0008888), "<(ab)(cd)e>" },   //  121
    { 5,  5,  6, ABC_CONST(0xF0007777F0007777), "<!(ab)(cd)e>" },   //  122
    { 5,  7,  7, ABC_CONST(0xF0006666F0006666), "<[ab](cd)e>" },   //  123
    { 5,  9,  8, ABC_CONST(0x0FF066660FF06666), "<[ab][cd]e>" },   //  124
    { 5,  5,  6, ABC_CONST(0xFF008080FF008080), "<(abc)de>" },   //  125
    { 5,  5,  6, ABC_CONST(0xFF007070FF007070), "<(!(ab)c)de>" },   //  126
    { 5,  7,  8, ABC_CONST(0xFF007878FF007878), "<[(ab)c]de>" },   //  127
    { 5,  7,  7, ABC_CONST(0xFF006060FF006060), "<([ab]c)de>" },   //  128
    { 5,  9, 10, ABC_CONST(0xFF009696FF009696), "<[abc]de>" },   //  129
    { 5,  6,  6, ABC_CONST(0xFF00CACAFF00CACA), "<<abc>de>" },   //  130
    { 6,  5,  7, ABC_CONST(0x8000000000000000), "(abcdef)" },   //  131
    { 6,  5,  7, ABC_CONST(0x7FFFFFFF00000000), "(!(abcde)f)" },   //  132
    { 6,  7, 12, ABC_CONST(0x7FFFFFFF80000000), "[(abcde)f]" },   //  133
    { 6,  5,  7, ABC_CONST(0x7FFF000000000000), "(!(abcd)ef)" },   //  134
    { 6,  5,  7, ABC_CONST(0x8000FFFF00000000), "(!(!(abcd)e)f)" },   //  135
    { 6,  7, 12, ABC_CONST(0x8000FFFF7FFF0000), "[(!(abcd)e)f]" },   //  136
    { 6,  7, 11, ABC_CONST(0x7FFF800000000000), "([(abcd)e]f)" },   //  137
    { 6,  9, 20, ABC_CONST(0x80007FFF7FFF8000), "[(abcd)ef]" },   //  138
    { 6,  5,  7, ABC_CONST(0x7F00000000000000), "(!(abc)def)" },   //  139
    { 6,  5,  7, ABC_CONST(0x80FFFFFF00000000), "(!(!(abc)de)f)" },   //  140
    { 6,  7, 12, ABC_CONST(0x80FFFFFF7F000000), "[(!(abc)de)f]" },   //  141
    { 6,  5,  7, ABC_CONST(0x80FF000000000000), "(!(!(abc)d)ef)" },   //  142
    { 6,  5,  7, ABC_CONST(0x7F00FFFF00000000), "(!(!(!(abc)d)e)f)" },   //  143
    { 6,  7, 12, ABC_CONST(0x7F00FFFF80FF0000), "[(!(!(abc)d)e)f]" },   //  144
    { 6,  7, 11, ABC_CONST(0x80FF7F0000000000), "([(!(abc)d)e]f)" },   //  145
    { 6,  9, 20, ABC_CONST(0x7F0080FF80FF7F00), "[(!(abc)d)ef]" },   //  146
    { 6,  7, 10, ABC_CONST(0x7F80000000000000), "([(abc)d]ef)" },   //  147
    { 6,  7, 10, ABC_CONST(0x807FFFFF00000000), "(!([(abc)d]e)f)" },   //  148
    { 6,  9, 18, ABC_CONST(0x807FFFFF7F800000), "[([(abc)d]e)f]" },   //  149
    { 6,  9, 17, ABC_CONST(0x807F7F8000000000), "([(abc)de]f)" },   //  150
    { 6, 11, 32, ABC_CONST(0x7F80807F807F7F80), "[(abc)def]" },   //  151
    { 6,  5,  7, ABC_CONST(0x7000000000000000), "(!(ab)cdef)" },   //  152
    { 6,  5,  7, ABC_CONST(0x8FFFFFFF00000000), "(!(!(ab)cde)f)" },   //  153
    { 6,  7, 12, ABC_CONST(0x8FFFFFFF70000000), "[(!(ab)cde)f]" },   //  154
    { 6,  5,  7, ABC_CONST(0x8FFF000000000000), "(!(!(ab)cd)ef)" },   //  155
    { 6,  5,  7, ABC_CONST(0x7000FFFF00000000), "(!(!(!(ab)cd)e)f)" },   //  156
    { 6,  7, 12, ABC_CONST(0x7000FFFF8FFF0000), "[(!(!(ab)cd)e)f]" },   //  157
    { 6,  7, 11, ABC_CONST(0x8FFF700000000000), "([(!(ab)cd)e]f)" },   //  158
    { 6,  9, 20, ABC_CONST(0x70008FFF8FFF7000), "[(!(ab)cd)ef]" },   //  159
    { 6,  5,  7, ABC_CONST(0x8F00000000000000), "(!(!(ab)c)def)" },   //  160
    { 6,  5,  7, ABC_CONST(0x70FFFFFF00000000), "(!(!(!(ab)c)de)f)" },   //  161
    { 6,  7, 12, ABC_CONST(0x70FFFFFF8F000000), "[(!(!(ab)c)de)f]" },   //  162
    { 6,  5,  7, ABC_CONST(0x70FF000000000000), "(!(!(!(ab)c)d)ef)" },   //  163
    { 6,  5,  7, ABC_CONST(0x8F00FFFF00000000), "(!(!(!(!(ab)c)d)e)f)" },   //  164
    { 6,  7, 12, ABC_CONST(0x8F00FFFF70FF0000), "[(!(!(!(ab)c)d)e)f]" },   //  165
    { 6,  7, 11, ABC_CONST(0x70FF8F0000000000), "([(!(!(ab)c)d)e]f)" },   //  166
    { 6,  9, 20, ABC_CONST(0x8F0070FF70FF8F00), "[(!(!(ab)c)d)ef]" },   //  167
    { 6,  7, 10, ABC_CONST(0x8F70000000000000), "([(!(ab)c)d]ef)" },   //  168
    { 6,  7, 10, ABC_CONST(0x708FFFFF00000000), "(!([(!(ab)c)d]e)f)" },   //  169
    { 6,  9, 18, ABC_CONST(0x708FFFFF8F700000), "[([(!(ab)c)d]e)f]" },   //  170
    { 6,  9, 17, ABC_CONST(0x708F8F7000000000), "([(!(ab)c)de]f)" },   //  171
    { 6, 11, 32, ABC_CONST(0x8F70708F708F8F70), "[(!(ab)c)def]" },   //  172
    { 6,  7,  9, ABC_CONST(0x7800000000000000), "([(ab)c]def)" },   //  173
    { 6,  7,  9, ABC_CONST(0x87FFFFFF00000000), "(!([(ab)c]de)f)" },   //  174
    { 6,  9, 16, ABC_CONST(0x87FFFFFF78000000), "[([(ab)c]de)f]" },   //  175
    { 6,  7,  9, ABC_CONST(0x87FF000000000000), "(!([(ab)c]d)ef)" },   //  176
    { 6,  7,  9, ABC_CONST(0x7800FFFF00000000), "(!(!([(ab)c]d)e)f)" },   //  177
    { 6,  9, 16, ABC_CONST(0x7800FFFF87FF0000), "[(!([(ab)c]d)e)f]" },   //  178
    { 6,  9, 15, ABC_CONST(0x87FF780000000000), "([([(ab)c]d)e]f)" },   //  179
    { 6, 11, 28, ABC_CONST(0x780087FF87FF7800), "[([(ab)c]d)ef]" },   //  180
    { 6,  9, 14, ABC_CONST(0x8778000000000000), "([(ab)cd]ef)" },   //  181
    { 6,  9, 14, ABC_CONST(0x7887FFFF00000000), "(!([(ab)cd]e)f)" },   //  182
    { 6, 11, 26, ABC_CONST(0x7887FFFF87780000), "[([(ab)cd]e)f]" },   //  183
    { 6, 11, 25, ABC_CONST(0x7887877800000000), "([(ab)cde]f)" },   //  184
    { 6, 13, 48, ABC_CONST(0x8778788778878778), "[(ab)cdef]" },   //  185
    { 6,  7,  8, ABC_CONST(0x6000000000000000), "([ab]cdef)" },   //  186
    { 6,  7,  8, ABC_CONST(0x9FFFFFFF00000000), "(!([ab]cde)f)" },   //  187
    { 6,  9, 14, ABC_CONST(0x9FFFFFFF60000000), "[([ab]cde)f]" },   //  188
    { 6,  7,  8, ABC_CONST(0x9FFF000000000000), "(!([ab]cd)ef)" },   //  189
    { 6,  7,  8, ABC_CONST(0x6000FFFF00000000), "(!(!([ab]cd)e)f)" },   //  190
    { 6,  9, 14, ABC_CONST(0x6000FFFF9FFF0000), "[(!([ab]cd)e)f]" },   //  191
    { 6,  9, 13, ABC_CONST(0x9FFF600000000000), "([([ab]cd)e]f)" },   //  192
    { 6, 11, 24, ABC_CONST(0x60009FFF9FFF6000), "[([ab]cd)ef]" },   //  193
    { 6,  7,  8, ABC_CONST(0x9F00000000000000), "(!([ab]c)def)" },   //  194
    { 6,  7,  8, ABC_CONST(0x60FFFFFF00000000), "(!(!([ab]c)de)f)" },   //  195
    { 6,  9, 14, ABC_CONST(0x60FFFFFF9F000000), "[(!([ab]c)de)f]" },   //  196
    { 6,  7,  8, ABC_CONST(0x60FF000000000000), "(!(!([ab]c)d)ef)" },   //  197
    { 6,  7,  8, ABC_CONST(0x9F00FFFF00000000), "(!(!(!([ab]c)d)e)f)" },   //  198
    { 6,  9, 14, ABC_CONST(0x9F00FFFF60FF0000), "[(!(!([ab]c)d)e)f]" },   //  199
    { 6,  9, 13, ABC_CONST(0x60FF9F0000000000), "([(!([ab]c)d)e]f)" },   //  200
    { 6, 11, 24, ABC_CONST(0x9F0060FF60FF9F00), "[(!([ab]c)d)ef]" },   //  201
    { 6,  9, 12, ABC_CONST(0x9F60000000000000), "([([ab]c)d]ef)" },   //  202
    { 6,  9, 12, ABC_CONST(0x609FFFFF00000000), "(!([([ab]c)d]e)f)" },   //  203
    { 6, 11, 22, ABC_CONST(0x609FFFFF9F600000), "[([([ab]c)d]e)f]" },   //  204
    { 6, 11, 21, ABC_CONST(0x609F9F6000000000), "([([ab]c)de]f)" },   //  205
    { 6, 13, 40, ABC_CONST(0x9F60609F609F9F60), "[([ab]c)def]" },   //  206
    { 6,  9, 11, ABC_CONST(0x9600000000000000), "([abc]def)" },   //  207
    { 6,  9, 11, ABC_CONST(0x69FFFFFF00000000), "(!([abc]de)f)" },   //  208
    { 6, 11, 20, ABC_CONST(0x69FFFFFF96000000), "[([abc]de)f]" },   //  209
    { 6,  9, 11, ABC_CONST(0x69FF000000000000), "(!([abc]d)ef)" },   //  210
    { 6,  9, 11, ABC_CONST(0x9600FFFF00000000), "(!(!([abc]d)e)f)" },   //  211
    { 6, 11, 20, ABC_CONST(0x9600FFFF69FF0000), "[(!([abc]d)e)f]" },   //  212
    { 6, 11, 19, ABC_CONST(0x69FF960000000000), "([([abc]d)e]f)" },   //  213
    { 6, 13, 36, ABC_CONST(0x960069FF69FF9600), "[([abc]d)ef]" },   //  214
    { 6, 11, 18, ABC_CONST(0x6996000000000000), "([abcd]ef)" },   //  215
    { 6, 11, 18, ABC_CONST(0x9669FFFF00000000), "(!([abcd]e)f)" },   //  216
    { 6, 13, 34, ABC_CONST(0x9669FFFF69960000), "[([abcd]e)f]" },   //  217
    { 6, 13, 33, ABC_CONST(0x9669699600000000), "([abcde]f)" },   //  218
    { 6, 15, 64, ABC_CONST(0x6996966996696996), "[abcdef]" },   //  219
    { 6,  6,  7, ABC_CONST(0xCA00000000000000), "(<abc>def)" },   //  220
    { 6,  6,  7, ABC_CONST(0x35FFFFFF00000000), "(!(<abc>de)f)" },   //  221
    { 6,  8, 12, ABC_CONST(0x35FFFFFFCA000000), "[(<abc>de)f]" },   //  222
    { 6,  6,  7, ABC_CONST(0x35FF000000000000), "(!(<abc>d)ef)" },   //  223
    { 6,  6,  7, ABC_CONST(0xCA00FFFF00000000), "(!(!(<abc>d)e)f)" },   //  224
    { 6,  8, 12, ABC_CONST(0xCA00FFFF35FF0000), "[(!(<abc>d)e)f]" },   //  225
    { 6,  8, 11, ABC_CONST(0x35FFCA0000000000), "([(<abc>d)e]f)" },   //  226
    { 6, 10, 20, ABC_CONST(0xCA0035FF35FFCA00), "[(<abc>d)ef]" },   //  227
    { 6,  8, 10, ABC_CONST(0x35CA000000000000), "([<abc>d]ef)" },   //  228
    { 6,  8, 10, ABC_CONST(0xCA35FFFF00000000), "(!([<abc>d]e)f)" },   //  229
    { 6, 10, 18, ABC_CONST(0xCA35FFFF35CA0000), "[([<abc>d]e)f]" },   //  230
    { 6, 10, 17, ABC_CONST(0xCA3535CA00000000), "([<abc>de]f)" },   //  231
    { 6, 12, 32, ABC_CONST(0x35CACA35CA3535CA), "[<abc>def]" },   //  232
    { 6,  5,  8, ABC_CONST(0x0777000000000000), "(!(ab)!(cd)ef)" },   //  233
    { 6,  5,  8, ABC_CONST(0xF888FFFF00000000), "(!(!(ab)!(cd)e)f)" },   //  234
    { 6,  7, 14, ABC_CONST(0xF888FFFF07770000), "[(!(ab)!(cd)e)f]" },   //  235
    { 6,  5,  8, ABC_CONST(0xF888000000000000), "(!(!(ab)!(cd))ef)" },   //  236
    { 6,  5,  8, ABC_CONST(0x0777FFFF00000000), "(!(!(!(ab)!(cd))e)f)" },   //  237
    { 6,  7, 14, ABC_CONST(0x0777FFFFF8880000), "[(!(!(ab)!(cd))e)f]" },   //  238
    { 6,  7, 13, ABC_CONST(0xF888077700000000), "([(!(ab)!(cd))e]f)" },   //  239
    { 6,  9, 24, ABC_CONST(0x0777F888F8880777), "[(!(ab)!(cd))ef]" },   //  240
    { 6,  7, 11, ABC_CONST(0x7888000000000000), "([(ab)(cd)]ef)" },   //  241
    { 6,  7, 11, ABC_CONST(0x8777FFFF00000000), "(!([(ab)(cd)]e)f)" },   //  242
    { 6,  9, 20, ABC_CONST(0x8777FFFF78880000), "[([(ab)(cd)]e)f]" },   //  243
    { 6,  7, 11, ABC_CONST(0x8777000000000000), "(![(ab)(cd)]ef)" },   //  244
    { 6,  7, 11, ABC_CONST(0x7888FFFF00000000), "(!(![(ab)(cd)]e)f)" },   //  245
    { 6,  9, 20, ABC_CONST(0x7888FFFF87770000), "[(![(ab)(cd)]e)f]" },   //  246
    { 6,  9, 19, ABC_CONST(0x8777788800000000), "([(ab)(cd)e]f)" },   //  247
    { 6, 11, 36, ABC_CONST(0x7888877787777888), "[(ab)(cd)ef]" },   //  248
    { 6,  7,  9, ABC_CONST(0x0666000000000000), "([ab]!(cd)ef)" },   //  249
    { 6,  7,  9, ABC_CONST(0xF999FFFF00000000), "(!([ab]!(cd)e)f)" },   //  250
    { 6,  9, 16, ABC_CONST(0xF999FFFF06660000), "[([ab]!(cd)e)f]" },   //  251
    { 6,  7,  9, ABC_CONST(0xF999000000000000), "(!([ab]!(cd))ef)" },   //  252
    { 6,  7,  9, ABC_CONST(0x0666FFFF00000000), "(!(!([ab]!(cd))e)f)" },   //  253
    { 6,  9, 16, ABC_CONST(0x0666FFFFF9990000), "[(!([ab]!(cd))e)f]" },   //  254
    { 6,  9, 15, ABC_CONST(0xF999066600000000), "([([ab]!(cd))e]f)" },   //  255
    { 6, 11, 28, ABC_CONST(0x0666F999F9990666), "[([ab]!(cd))ef]" },   //  256
    { 6,  9, 10, ABC_CONST(0x0660000000000000), "([ab][cd]ef)" },   //  257
    { 6,  9, 10, ABC_CONST(0xF99FFFFF00000000), "(!([ab][cd]e)f)" },   //  258
    { 6, 11, 18, ABC_CONST(0xF99FFFFF06600000), "[([ab][cd]e)f]" },   //  259
    { 6,  9, 10, ABC_CONST(0xF99F000000000000), "(!([ab][cd])ef)" },   //  260
    { 6,  9, 10, ABC_CONST(0x0660FFFF00000000), "(!(!([ab][cd])e)f)" },   //  261
    { 6, 11, 18, ABC_CONST(0x0660FFFFF99F0000), "[(!([ab][cd])e)f]" },   //  262
    { 6, 11, 17, ABC_CONST(0xF99F066000000000), "([([ab][cd])e]f)" },   //  263
    { 6, 13, 32, ABC_CONST(0x0660F99FF99F0660), "[([ab][cd])ef]" },   //  264
    { 6,  6,  8, ABC_CONST(0xCAAA000000000000), "(<ab(cd)>ef)" },   //  265
    { 6,  6,  8, ABC_CONST(0x3555FFFF00000000), "(!(<ab(cd)>e)f)" },   //  266
    { 6,  8, 14, ABC_CONST(0x3555FFFFCAAA0000), "[(<ab(cd)>e)f]" },   //  267
    { 6,  8, 13, ABC_CONST(0x3555CAAA00000000), "([<ab(cd)>e]f)" },   //  268
    { 6, 10, 24, ABC_CONST(0xCAAA35553555CAAA), "[<ab(cd)>ef]" },   //  269
    { 6,  8, 10, ABC_CONST(0xACCA000000000000), "(<ab[cd]>ef)" },   //  270
    { 6,  8, 10, ABC_CONST(0x5335FFFF00000000), "(!(<ab[cd]>e)f)" },   //  271
    { 6, 10, 18, ABC_CONST(0x5335FFFFACCA0000), "[(<ab[cd]>e)f]" },   //  272
    { 6, 10, 17, ABC_CONST(0x5335ACCA00000000), "([<ab[cd]>e]f)" },   //  273
    { 6, 12, 32, ABC_CONST(0xACCA53355335ACCA), "[<ab[cd]>ef]" },   //  274
    { 6,  6,  7, ABC_CONST(0xF088000000000000), "(<(ab)cd>ef)" },   //  275
    { 6,  6,  7, ABC_CONST(0x0F77FFFF00000000), "(!(<(ab)cd>e)f)" },   //  276
    { 6,  8, 12, ABC_CONST(0x0F77FFFFF0880000), "[(<(ab)cd>e)f]" },   //  277
    { 6,  6,  7, ABC_CONST(0x0F77000000000000), "(!<(ab)cd>ef)" },   //  278
    { 6,  6,  7, ABC_CONST(0xF088FFFF00000000), "(!(!<(ab)cd>e)f)" },   //  279
    { 6,  8, 12, ABC_CONST(0xF088FFFF0F770000), "[(!<(ab)cd>e)f]" },   //  280
    { 6,  8, 11, ABC_CONST(0x0F77F08800000000), "([<(ab)cd>e]f)" },   //  281
    { 6, 10, 20, ABC_CONST(0xF0880F770F77F088), "[<(ab)cd>ef]" },   //  282
    { 6,  8,  8, ABC_CONST(0xF066000000000000), "(<[ab]cd>ef)" },   //  283
    { 6,  8,  8, ABC_CONST(0x0F99FFFF00000000), "(!(<[ab]cd>e)f)" },   //  284
    { 6, 10, 14, ABC_CONST(0x0F99FFFFF0660000), "[(<[ab]cd>e)f]" },   //  285
    { 6, 10, 13, ABC_CONST(0x0F99F06600000000), "([<[ab]cd>e]f)" },   //  286
    { 6, 12, 24, ABC_CONST(0xF0660F990F99F066), "[<[ab]cd>ef]" },   //  287
    { 6,  5,  9, ABC_CONST(0x007F7F7F00000000), "(!(abc)!(de)f)" },   //  288
    { 6,  5,  9, ABC_CONST(0xFF80808000000000), "(!(!(abc)!(de))f)" },   //  289
    { 6,  7, 16, ABC_CONST(0xFF808080007F7F7F), "[(!(abc)!(de))f]" },   //  290
    { 6,  7, 13, ABC_CONST(0x7F80808000000000), "([(abc)(de)]f)" },   //  291
    { 6,  7, 13, ABC_CONST(0x807F7F7F00000000), "(![(abc)(de)]f)" },   //  292
    { 6,  9, 24, ABC_CONST(0x807F7F7F7F808080), "[(abc)(de)f]" },   //  293
    { 6,  5,  8, ABC_CONST(0x008F8F8F00000000), "(!(!(ab)c)!(de)f)" },   //  294
    { 6,  5,  8, ABC_CONST(0xFF70707000000000), "(!(!(!(ab)c)!(de))f)" },   //  295
    { 6,  7, 14, ABC_CONST(0xFF707070008F8F8F), "[(!(!(ab)c)!(de))f]" },   //  296
    { 6,  7, 13, ABC_CONST(0x8F70707000000000), "([(!(ab)c)(de)]f)" },   //  297
    { 6,  7, 13, ABC_CONST(0x708F8F8F00000000), "(![(!(ab)c)(de)]f)" },   //  298
    { 6,  9, 24, ABC_CONST(0x708F8F8F8F707070), "[(!(ab)c)(de)f]" },   //  299
    { 6,  7, 11, ABC_CONST(0x0078787800000000), "([(ab)c]!(de)f)" },   //  300
    { 6,  7, 11, ABC_CONST(0xFF87878700000000), "(!([(ab)c]!(de))f)" },   //  301
    { 6,  9, 20, ABC_CONST(0xFF87878700787878), "[([(ab)c]!(de))f]" },   //  302
    { 6,  7, 10, ABC_CONST(0x009F9F9F00000000), "(!([ab]c)!(de)f)" },   //  303
    { 6,  7, 10, ABC_CONST(0xFF60606000000000), "(!(!([ab]c)!(de))f)" },   //  304
    { 6,  9, 18, ABC_CONST(0xFF606060009F9F9F), "[(!([ab]c)!(de))f]" },   //  305
    { 6,  9, 16, ABC_CONST(0x9F60606000000000), "([([ab]c)(de)]f)" },   //  306
    { 6,  9, 16, ABC_CONST(0x609F9F9F00000000), "(![([ab]c)(de)]f)" },   //  307
    { 6, 11, 30, ABC_CONST(0x609F9F9F9F606060), "[([ab]c)(de)f]" },   //  308
    { 6,  9, 14, ABC_CONST(0x0096969600000000), "([abc]!(de)f)" },   //  309
    { 6,  9, 14, ABC_CONST(0xFF69696900000000), "(!([abc]!(de))f)" },   //  310
    { 6, 11, 26, ABC_CONST(0xFF69696900969696), "[([abc]!(de))f]" },   //  311
    { 6,  6,  8, ABC_CONST(0x00CACACA00000000), "(<abc>!(de)f)" },   //  312
    { 6,  6,  8, ABC_CONST(0xFF35353500000000), "(!(<abc>!(de))f)" },   //  313
    { 6,  8, 14, ABC_CONST(0xFF35353500CACACA), "[(<abc>!(de))f]" },   //  314
    { 6,  8, 13, ABC_CONST(0x35CACACA00000000), "([<abc>(de)]f)" },   //  315
    { 6, 10, 24, ABC_CONST(0xCA35353535CACACA), "[<abc>(de)f]" },   //  316
    { 6,  7, 10, ABC_CONST(0x007F7F0000000000), "(!(abc)[de]f)" },   //  317
    { 6,  7, 10, ABC_CONST(0xFF8080FF00000000), "(!(!(abc)[de])f)" },   //  318
    { 6,  9, 18, ABC_CONST(0xFF8080FF007F7F00), "[(!(abc)[de])f]" },   //  319
    { 6,  7,  9, ABC_CONST(0x008F8F0000000000), "(!(!(ab)c)[de]f)" },   //  320
    { 6,  7,  9, ABC_CONST(0xFF7070FF00000000), "(!(!(!(ab)c)[de])f)" },   //  321
    { 6,  9, 16, ABC_CONST(0xFF7070FF008F8F00), "[(!(!(ab)c)[de])f]" },   //  322
    { 6,  9, 12, ABC_CONST(0x0078780000000000), "([(ab)c][de]f)" },   //  323
    { 6,  9, 12, ABC_CONST(0xFF8787FF00000000), "(!([(ab)c][de])f)" },   //  324
    { 6, 11, 22, ABC_CONST(0xFF8787FF00787800), "[([(ab)c][de])f]" },   //  325
    { 6,  9, 11, ABC_CONST(0x009F9F0000000000), "(!([ab]c)[de]f)" },   //  326
    { 6,  9, 11, ABC_CONST(0xFF6060FF00000000), "(!(!([ab]c)[de])f)" },   //  327
    { 6, 11, 20, ABC_CONST(0xFF6060FF009F9F00), "[(!([ab]c)[de])f]" },   //  328
    { 6, 11, 15, ABC_CONST(0x0096960000000000), "([abc][de]f)" },   //  329
    { 6, 11, 15, ABC_CONST(0xFF6969FF00000000), "(!([abc][de])f)" },   //  330
    { 6, 13, 28, ABC_CONST(0xFF6969FF00969600), "[([abc][de])f]" },   //  331
    { 6,  8,  9, ABC_CONST(0x00CACA0000000000), "(<abc>[de]f)" },   //  332
    { 6,  8,  9, ABC_CONST(0xFF3535FF00000000), "(!(<abc>[de])f)" },   //  333
    { 6, 10, 16, ABC_CONST(0xFF3535FF00CACA00), "[(<abc>[de])f]" },   //  334
    { 6,  6,  9, ABC_CONST(0xCAAAAAAA00000000), "(<ab(cde)>f)" },   //  335
    { 6,  8, 16, ABC_CONST(0x35555555CAAAAAAA), "[<ab(cde)>f]" },   //  336
    { 6,  6,  9, ABC_CONST(0xACCCAAAA00000000), "(<ab(!(cd)e)>f)" },   //  337
    { 6,  8, 16, ABC_CONST(0x53335555ACCCAAAA), "[<ab(!(cd)e)>f]" },   //  338
    { 6,  8, 13, ABC_CONST(0xACCCCAAA00000000), "(<ab[(cd)e]>f)" },   //  339
    { 6, 10, 24, ABC_CONST(0x53333555ACCCCAAA), "[<ab[(cd)e]>f]" },   //  340
    { 6,  8, 11, ABC_CONST(0xACCAAAAA00000000), "(<ab([cd]e)>f)" },   //  341
    { 6, 10, 20, ABC_CONST(0x53355555ACCAAAAA), "[<ab([cd]e)>f]" },   //  342
    { 6, 10, 17, ABC_CONST(0xCAACACCA00000000), "(<ab[cde]>f)" },   //  343
    { 6, 12, 32, ABC_CONST(0x35535335CAACACCA), "[<ab[cde]>f]" },   //  344
    { 6,  7,  9, ABC_CONST(0xCCAACACA00000000), "(<ab<cde>>f)" },   //  345
    { 6,  9, 16, ABC_CONST(0x33553535CCAACACA), "[<ab<cde>>f]" },   //  346
    { 6,  6,  8, ABC_CONST(0xC0AAAAAA00000000), "(<a(bc)(de)>f)" },   //  347
    { 6,  6,  8, ABC_CONST(0x3F55555500000000), "(!<a(bc)(de)>f)" },   //  348
    { 6,  8, 14, ABC_CONST(0x3F555555C0AAAAAA), "[<a(bc)(de)>f]" },   //  349
    { 6,  8,  9, ABC_CONST(0x3CAAAAAA00000000), "(<a[bc](de)>f)" },   //  350
    { 6, 10, 16, ABC_CONST(0xC35555553CAAAAAA), "[<a[bc](de)>f]" },   //  351
    { 6,  6,  9, ABC_CONST(0xF088888800000000), "(<(ab)c(de)>f)" },   //  352
    { 6,  6,  9, ABC_CONST(0x0F77777700000000), "(!<(ab)c(de)>f)" },   //  353
    { 6,  8, 16, ABC_CONST(0x0F777777F0888888), "[<(ab)c(de)>f]" },   //  354
    { 6,  8, 11, ABC_CONST(0x88F0F08800000000), "(<(ab)c[de]>f)" },   //  355
    { 6,  8, 11, ABC_CONST(0x770F0F7700000000), "(!<(ab)c[de]>f)" },   //  356
    { 6, 10, 20, ABC_CONST(0x770F0F7788F0F088), "[<(ab)c[de]>f]" },   //  357
    { 6,  8, 11, ABC_CONST(0xF066666600000000), "(<[ab]c(de)>f)" },   //  358
    { 6, 10, 20, ABC_CONST(0x0F999999F0666666), "[<[ab]c(de)>f]" },   //  359
    { 6, 10, 13, ABC_CONST(0x66F0F06600000000), "(<[ab]c[de]>f)" },   //  360
    { 6, 12, 24, ABC_CONST(0x990F0F9966F0F066), "[<[ab]c[de]>f]" },   //  361
    { 6,  6,  7, ABC_CONST(0xF000888800000000), "(<(ab)(cd)e>f)" },   //  362
    { 6,  6,  7, ABC_CONST(0x0FFF777700000000), "(!<(ab)(cd)e>f)" },   //  363
    { 6,  8, 12, ABC_CONST(0x0FFF7777F0008888), "[<(ab)(cd)e>f]" },   //  364
    { 6,  6,  7, ABC_CONST(0xF000777700000000), "(<!(ab)(cd)e>f)" },   //  365
    { 6,  8, 12, ABC_CONST(0x0FFF8888F0007777), "[<!(ab)(cd)e>f]" },   //  366
    { 6,  8,  8, ABC_CONST(0xF000666600000000), "(<[ab](cd)e>f)" },   //  367
    { 6,  8,  8, ABC_CONST(0x0FFF999900000000), "(!<[ab](cd)e>f)" },   //  368
    { 6, 10, 14, ABC_CONST(0x0FFF9999F0006666), "[<[ab](cd)e>f]" },   //  369
    { 6, 10,  9, ABC_CONST(0x0FF0666600000000), "(<[ab][cd]e>f)" },   //  370
    { 6, 12, 16, ABC_CONST(0xF00F99990FF06666), "[<[ab][cd]e>f]" },   //  371
    { 6,  6,  7, ABC_CONST(0xFF00808000000000), "(<(abc)de>f)" },   //  372
    { 6,  6,  7, ABC_CONST(0x00FF7F7F00000000), "(!<(abc)de>f)" },   //  373
    { 6,  8, 12, ABC_CONST(0x00FF7F7FFF008080), "[<(abc)de>f]" },   //  374
    { 6,  6,  7, ABC_CONST(0xFF00707000000000), "(<(!(ab)c)de>f)" },   //  375
    { 6,  6,  7, ABC_CONST(0x00FF8F8F00000000), "(!<(!(ab)c)de>f)" },   //  376
    { 6,  8, 12, ABC_CONST(0x00FF8F8FFF007070), "[<(!(ab)c)de>f]" },   //  377
    { 6,  8,  9, ABC_CONST(0xFF00787800000000), "(<[(ab)c]de>f)" },   //  378
    { 6, 10, 16, ABC_CONST(0x00FF8787FF007878), "[<[(ab)c]de>f]" },   //  379
    { 6,  8,  8, ABC_CONST(0xFF00606000000000), "(<([ab]c)de>f)" },   //  380
    { 6,  8,  8, ABC_CONST(0x00FF9F9F00000000), "(!<([ab]c)de>f)" },   //  381
    { 6, 10, 14, ABC_CONST(0x00FF9F9FFF006060), "[<([ab]c)de>f]" },   //  382
    { 6, 10, 11, ABC_CONST(0xFF00969600000000), "(<[abc]de>f)" },   //  383
    { 6, 12, 20, ABC_CONST(0x00FF6969FF009696), "[<[abc]de>f]" },   //  384
    { 6,  7,  7, ABC_CONST(0xFF00CACA00000000), "(<<abc>de>f)" },   //  385
    { 6,  9, 12, ABC_CONST(0x00FF3535FF00CACA), "[<<abc>de>f]" },   //  386
    { 6,  5, 10, ABC_CONST(0x00007FFF7FFF7FFF), "(!(abcd)!(ef))" },   //  387
    { 6,  7, 15, ABC_CONST(0x7FFF800080008000), "[(abcd)(ef)]" },   //  388
    { 6,  5,  8, ABC_CONST(0x000080FF80FF80FF), "(!(!(abc)d)!(ef))" },   //  389
    { 6,  7, 15, ABC_CONST(0x80FF7F007F007F00), "[(!(abc)d)(ef)]" },   //  390
    { 6,  7, 13, ABC_CONST(0x00007F807F807F80), "([(abc)d]!(ef))" },   //  391
    { 6,  5,  9, ABC_CONST(0x00008FFF8FFF8FFF), "(!(!(ab)cd)!(ef))" },   //  392
    { 6,  7, 15, ABC_CONST(0x8FFF700070007000), "[(!(ab)cd)(ef)]" },   //  393
    { 6,  5,  9, ABC_CONST(0x000070FF70FF70FF), "(!(!(!(ab)c)d)!(ef))" },   //  394
    { 6,  7, 15, ABC_CONST(0x70FF8F008F008F00), "[(!(!(ab)c)d)(ef)]" },   //  395
    { 6,  7, 13, ABC_CONST(0x00008F708F708F70), "([(!(ab)c)d]!(ef))" },   //  396
    { 6,  7, 12, ABC_CONST(0x000087FF87FF87FF), "(!([(ab)c]d)!(ef))" },   //  397
    { 6,  9, 21, ABC_CONST(0x87FF780078007800), "[([(ab)c]d)(ef)]" },   //  398
    { 6,  9, 19, ABC_CONST(0x0000877887788778), "([(ab)cd]!(ef))" },   //  399
    { 6,  7, 11, ABC_CONST(0x00009FFF9FFF9FFF), "(!([ab]cd)!(ef))" },   //  400
    { 6,  9, 18, ABC_CONST(0x9FFF600060006000), "[([ab]cd)(ef)]" },   //  401
    { 6,  7, 10, ABC_CONST(0x000060FF60FF60FF), "(!(!([ab]c)d)!(ef))" },   //  402
    { 6,  9, 18, ABC_CONST(0x60FF9F009F009F00), "[(!([ab]c)d)(ef)]" },   //  403
    { 6,  9, 16, ABC_CONST(0x00009F609F609F60), "([([ab]c)d]!(ef))" },   //  404
    { 6,  9, 15, ABC_CONST(0x000069FF69FF69FF), "(!([abc]d)!(ef))" },   //  405
    { 6, 11, 27, ABC_CONST(0x69FF960096009600), "[([abc]d)(ef)]" },   //  406
    { 6, 11, 25, ABC_CONST(0x0000699669966996), "([abcd]!(ef))" },   //  407
    { 6,  6,  9, ABC_CONST(0x000035FF35FF35FF), "(!(<abc>d)!(ef))" },   //  408
    { 6,  8, 15, ABC_CONST(0x35FFCA00CA00CA00), "[(<abc>d)(ef)]" },   //  409
    { 6,  8, 13, ABC_CONST(0x000035CA35CA35CA), "([<abc>d]!(ef))" },   //  410
    { 6,  5, 11, ABC_CONST(0x0000077707770777), "(!(ab)!(cd)!(ef))" },   //  411
    { 6,  5,  9, ABC_CONST(0x0000F888F888F888), "(!(!(ab)!(cd))!(ef))" },   //  412
    { 6,  7, 18, ABC_CONST(0xF888077707770777), "[(!(ab)!(cd))(ef)]" },   //  413
    { 6,  7, 14, ABC_CONST(0x0000788878887888), "([(ab)(cd)]!(ef))" },   //  414
    { 6,  7, 15, ABC_CONST(0x0000877787778777), "(![(ab)(cd)]!(ef))" },   //  415
    { 6,  9, 27, ABC_CONST(0x8777788878887888), "[(ab)(cd)(ef)]" },   //  416
    { 6,  7, 12, ABC_CONST(0x0000066606660666), "([ab]!(cd)!(ef))" },   //  417
    { 6,  7, 11, ABC_CONST(0x0000F999F999F999), "(!([ab]!(cd))!(ef))" },   //  418
    { 6,  9, 21, ABC_CONST(0xF999066606660666), "[([ab]!(cd))(ef)]" },   //  419
    { 6,  9, 13, ABC_CONST(0x0000066006600660), "([ab][cd]!(ef))" },   //  420
    { 6,  9, 13, ABC_CONST(0x0000F99FF99FF99F), "(!([ab][cd])!(ef))" },   //  421
    { 6, 11, 24, ABC_CONST(0xF99F066006600660), "[([ab][cd])(ef)]" },   //  422
    { 6,  6, 10, ABC_CONST(0x0000CAAACAAACAAA), "(<ab(cd)>!(ef))" },   //  423
    { 6,  8, 18, ABC_CONST(0x3555CAAACAAACAAA), "[<ab(cd)>(ef)]" },   //  424
    { 6,  8, 13, ABC_CONST(0x0000ACCAACCAACCA), "(<ab[cd]>!(ef))" },   //  425
    { 6, 10, 24, ABC_CONST(0x5335ACCAACCAACCA), "[<ab[cd]>(ef)]" },   //  426
    { 6,  6,  8, ABC_CONST(0x0000F088F088F088), "(<(ab)cd>!(ef))" },   //  427
    { 6,  6,  9, ABC_CONST(0x00000F770F770F77), "(!<(ab)cd>!(ef))" },   //  428
    { 6,  8, 15, ABC_CONST(0x0F77F088F088F088), "[<(ab)cd>(ef)]" },   //  429
    { 6,  8, 10, ABC_CONST(0x0000F066F066F066), "(<[ab]cd>!(ef))" },   //  430
    { 6, 10, 18, ABC_CONST(0x0F99F066F066F066), "[<[ab]cd>(ef)]" },   //  431
    { 6,  7, 11, ABC_CONST(0x00007FFF7FFF0000), "(!(abcd)[ef])" },   //  432
    { 6,  7,  9, ABC_CONST(0x000080FF80FF0000), "(!(!(abc)d)[ef])" },   //  433
    { 6,  9, 14, ABC_CONST(0x00007F807F800000), "([(abc)d][ef])" },   //  434
    { 6,  7, 10, ABC_CONST(0x00008FFF8FFF0000), "(!(!(ab)cd)[ef])" },   //  435
    { 6,  7, 10, ABC_CONST(0x000070FF70FF0000), "(!(!(!(ab)c)d)[ef])" },   //  436
    { 6,  9, 14, ABC_CONST(0x00008F708F700000), "([(!(ab)c)d][ef])" },   //  437
    { 6,  9, 13, ABC_CONST(0x000087FF87FF0000), "(!([(ab)c]d)[ef])" },   //  438
    { 6, 11, 20, ABC_CONST(0x0000877887780000), "([(ab)cd][ef])" },   //  439
    { 6,  9, 12, ABC_CONST(0x00009FFF9FFF0000), "(!([ab]cd)[ef])" },   //  440
    { 6,  9, 11, ABC_CONST(0x000060FF60FF0000), "(!(!([ab]c)d)[ef])" },   //  441
    { 6, 11, 17, ABC_CONST(0x00009F609F600000), "([([ab]c)d][ef])" },   //  442
    { 6, 11, 16, ABC_CONST(0x000069FF69FF0000), "(!([abc]d)[ef])" },   //  443
    { 6, 13, 26, ABC_CONST(0x0000699669960000), "([abcd][ef])" },   //  444
    { 6,  8, 10, ABC_CONST(0x000035FF35FF0000), "(!(<abc>d)[ef])" },   //  445
    { 6, 10, 14, ABC_CONST(0x000035CA35CA0000), "([<abc>d][ef])" },   //  446
    { 6,  7, 10, ABC_CONST(0x0000F888F8880000), "(!(!(ab)!(cd))[ef])" },   //  447
    { 6,  9, 15, ABC_CONST(0x0000788878880000), "([(ab)(cd)][ef])" },   //  448
    { 6,  9, 16, ABC_CONST(0x0000877787770000), "(![(ab)(cd)][ef])" },   //  449
    { 6,  9, 12, ABC_CONST(0x0000F999F9990000), "(!([ab]!(cd))[ef])" },   //  450
    { 6, 11, 14, ABC_CONST(0x0000066006600000), "([ab][cd][ef])" },   //  451
    { 6, 11, 14, ABC_CONST(0x0000F99FF99F0000), "(!([ab][cd])[ef])" },   //  452
    { 6,  8, 11, ABC_CONST(0x0000CAAACAAA0000), "(<ab(cd)>[ef])" },   //  453
    { 6, 10, 14, ABC_CONST(0x0000ACCAACCA0000), "(<ab[cd]>[ef])" },   //  454
    { 6,  8,  9, ABC_CONST(0x0000F088F0880000), "(<(ab)cd>[ef])" },   //  455
    { 6,  8, 10, ABC_CONST(0x00000F770F770000), "(!<(ab)cd>[ef])" },   //  456
    { 6, 10, 11, ABC_CONST(0x0000F066F0660000), "(<[ab]cd>[ef])" },   //  457
    { 6,  5, 11, ABC_CONST(0x007F7F7F7F7F7F7F), "(!(abc)!(def))" },   //  458
    { 6,  7, 16, ABC_CONST(0x7F80808080808080), "[(abc)(def)]" },   //  459
    { 6,  5,  9, ABC_CONST(0x008F8F8F8F8F8F8F), "(!(!(ab)c)!(def))" },   //  460
    { 6,  7, 16, ABC_CONST(0x8F70707070707070), "[(!(ab)c)(def)]" },   //  461
    { 6,  7, 13, ABC_CONST(0x0078787878787878), "([(ab)c]!(def))" },   //  462
    { 6,  7, 12, ABC_CONST(0x009F9F9F9F9F9F9F), "(!([ab]c)!(def))" },   //  463
    { 6,  9, 20, ABC_CONST(0x9F60606060606060), "[([ab]c)(def)]" },   //  464
    { 6,  9, 17, ABC_CONST(0x0096969696969696), "([abc]!(def))" },   //  465
    { 6,  6,  9, ABC_CONST(0x00CACACACACACACA), "(<abc>!(def))" },   //  466
    { 6,  8, 16, ABC_CONST(0x35CACACACACACACA), "[<abc>(def)]" },   //  467
    { 6,  5,  8, ABC_CONST(0x8F0000008F8F8F8F), "(!(!(ab)c)!(!(de)f))" },   //  468
    { 6,  7, 16, ABC_CONST(0x708F8F8F70707070), "[(!(ab)c)(!(de)f)]" },   //  469
    { 6,  7, 11, ABC_CONST(0x7800000078787878), "([(ab)c]!(!(de)f))" },   //  470
    { 6,  7, 10, ABC_CONST(0x9F0000009F9F9F9F), "(!([ab]c)!(!(de)f))" },   //  471
    { 6,  9, 20, ABC_CONST(0x609F9F9F60606060), "[([ab]c)(!(de)f)]" },   //  472
    { 6,  9, 14, ABC_CONST(0x9600000096969696), "([abc]!(!(de)f))" },   //  473
    { 6,  6,  8, ABC_CONST(0xCA000000CACACACA), "(<abc>!(!(de)f))" },   //  474
    { 6,  8, 16, ABC_CONST(0xCA353535CACACACA), "[<abc>(!(de)f)]" },   //  475
    { 6,  9, 15, ABC_CONST(0x0078787878000000), "([(ab)c][(de)f])" },   //  476
    { 6,  9, 14, ABC_CONST(0x009F9F9F9F000000), "(!([ab]c)[(de)f])" },   //  477
    { 6, 11, 19, ABC_CONST(0x0096969696000000), "([abc][(de)f])" },   //  478
    { 6,  8, 11, ABC_CONST(0x00CACACACA000000), "(<abc>[(de)f])" },   //  479
    { 6,  9, 13, ABC_CONST(0x9F00009F9F9F9F9F), "(!([ab]c)!([de]f))" },   //  480
    { 6, 11, 25, ABC_CONST(0x609F9F6060606060), "[([ab]c)([de]f)]" },   //  481
    { 6, 11, 18, ABC_CONST(0x9600009696969696), "([abc]!([de]f))" },   //  482
    { 6,  8, 10, ABC_CONST(0xCA0000CACACACACA), "(<abc>!([de]f))" },   //  483
    { 6, 10, 20, ABC_CONST(0xCA3535CACACACACA), "[<abc>([de]f)]" },   //  484
    { 6, 13, 24, ABC_CONST(0x9600009600969600), "([abc][def])" },   //  485
    { 6, 10, 14, ABC_CONST(0xCA0000CA00CACA00), "(<abc>[def])" },   //  486
    { 6,  7,  8, ABC_CONST(0xCACA0000CA00CA00), "(<abc><def>)" },   //  487
    { 6,  9, 16, ABC_CONST(0x3535CACA35CA35CA), "[<abc><def>]" },   //  488
    { 6,  6, 10, ABC_CONST(0xCAAAAAAAAAAAAAAA), "<ab(cdef)>" },   //  489
    { 6,  6, 10, ABC_CONST(0xACCCCCCCAAAAAAAA), "<ab(!(cde)f)>" },   //  490
    { 6,  8, 16, ABC_CONST(0xACCCCCCCCAAAAAAA), "<ab[(cde)f]>" },   //  491
    { 6,  6, 10, ABC_CONST(0xACCCAAAAAAAAAAAA), "<ab(!(cd)ef)>" },   //  492
    { 6,  6, 10, ABC_CONST(0xCAAACCCCAAAAAAAA), "<ab(!(!(cd)e)f)>" },   //  493
    { 6,  8, 16, ABC_CONST(0xCAAACCCCACCCAAAA), "<ab[(!(cd)e)f]>" },   //  494
    { 6,  8, 14, ABC_CONST(0xACCCCAAAAAAAAAAA), "<ab([(cd)e]f)>" },   //  495
    { 6, 10, 24, ABC_CONST(0xCAAAACCCACCCCAAA), "<ab[(cd)ef]>" },   //  496
    { 6,  8, 12, ABC_CONST(0xACCAAAAAAAAAAAAA), "<ab([cd]ef)>" },   //  497
    { 6,  8, 12, ABC_CONST(0xCAACCCCCAAAAAAAA), "<ab(!([cd]e)f)>" },   //  498
    { 6, 10, 20, ABC_CONST(0xCAACCCCCACCAAAAA), "<ab[([cd]e)f]>" },   //  499
    { 6, 10, 18, ABC_CONST(0xCAACACCAAAAAAAAA), "<ab([cde]f)>" },   //  500
    { 6, 12, 32, ABC_CONST(0xACCACAACCAACACCA), "<ab[cdef]>" },   //  501
    { 6,  7, 10, ABC_CONST(0xCCAACACAAAAAAAAA), "<ab(<cde>f)>" },   //  502
    { 6,  9, 16, ABC_CONST(0xAACCACACCCAACACA), "<ab[<cde>f]>" },   //  503
    { 6,  6, 12, ABC_CONST(0xAAAAACCCACCCACCC), "<ab(!(cd)!(ef))>" },   //  504
    { 6,  8, 18, ABC_CONST(0xACCCCAAACAAACAAA), "<ab[(cd)(ef)]>" },   //  505
    { 6,  8, 14, ABC_CONST(0xAAAAACCAACCAACCA), "<ab([cd]!(ef))>" },   //  506
    { 6, 10, 16, ABC_CONST(0xAAAAACCAACCAAAAA), "<ab([cd][ef])>" },   //  507
    { 6,  7, 12, ABC_CONST(0xCCAACACACACACACA), "<ab<cd(ef)>>" },   //  508
    { 6,  9, 16, ABC_CONST(0xCACACCAACCAACACA), "<ab<cd[ef]>>" },   //  509
    { 6,  7, 10, ABC_CONST(0xCCCCAAAACAAACAAA), "<ab<(cd)ef>>" },   //  510
    { 6,  9, 12, ABC_CONST(0xCCCCAAAAACCAACCA), "<ab<[cd]ef>>" },   //  511
    { 6,  6,  9, ABC_CONST(0xC0AAAAAAAAAAAAAA), "<a(bc)(def)>" },   //  512
    { 6,  6, 10, ABC_CONST(0xAAC0C0C0AAAAAAAA), "<a(bc)(!(de)f)>" },   //  513
    { 6,  8, 12, ABC_CONST(0xAAC0C0AAAAAAAAAA), "<a(bc)([de]f)>" },   //  514
    { 6,  8, 10, ABC_CONST(0x3CAAAAAAAAAAAAAA), "<a[bc](def)>" },   //  515
    { 6,  8, 12, ABC_CONST(0xAA3C3C3CAAAAAAAA), "<a[bc](!(de)f)>" },   //  516
    { 6, 10, 14, ABC_CONST(0xAA3C3CAAAAAAAAAA), "<a[bc]([de]f)>" },   //  517
    { 6,  6,  8, ABC_CONST(0xC000AAAAAAAAAAAA), "<a(bcd)(ef)>" },   //  518
    { 6,  6,  8, ABC_CONST(0x3F00AAAAAAAAAAAA), "<a(!(bc)d)(ef)>" },   //  519
    { 6,  8, 10, ABC_CONST(0x3FC0AAAAAAAAAAAA), "<a[(bc)d](ef)>" },   //  520
    { 6,  8,  9, ABC_CONST(0x3C00AAAAAAAAAAAA), "<a([bc]d)(ef)>" },   //  521
    { 6, 10, 12, ABC_CONST(0xC33CAAAAAAAAAAAA), "<a[bcd](ef)>" },   //  522
    { 6,  7,  8, ABC_CONST(0xF0CCAAAAAAAAAAAA), "<a<bcd>(ef)>" },   //  523
    { 6,  6, 11, ABC_CONST(0xF088888888888888), "<(ab)c(def)>" },   //  524
    { 6,  6, 10, ABC_CONST(0x88F0F0F088888888), "<(ab)c(!(de)f)>" },   //  525
    { 6,  8, 15, ABC_CONST(0x88F0F0F0F0888888), "<(ab)c[(de)f]>" },   //  526
    { 6,  8, 13, ABC_CONST(0x88F0F08888888888), "<(ab)c([de]f)>" },   //  527
    { 6, 10, 20, ABC_CONST(0xF08888F088F0F088), "<(ab)c[def]>" },   //  528
    { 6,  7, 10, ABC_CONST(0xF0F08888F088F088), "<(ab)c<def>>" },   //  529
    { 6,  8, 14, ABC_CONST(0xF066666666666666), "<[ab]c(def)>" },   //  530
    { 6,  8, 12, ABC_CONST(0x66F0F0F066666666), "<[ab]c(!(de)f)>" },   //  531
    { 6, 10, 18, ABC_CONST(0x66F0F0F0F0666666), "<[ab]c[(de)f]>" },   //  532
    { 6, 10, 16, ABC_CONST(0x66F0F06666666666), "<[ab]c([de]f)>" },   //  533
    { 6, 12, 24, ABC_CONST(0xF06666F066F0F066), "<[ab]c[def]>" },   //  534
    { 6,  9, 12, ABC_CONST(0xF0F06666F066F066), "<[ab]c<def>>" },   //  535
    { 6,  6,  9, ABC_CONST(0xF000888888888888), "<(ab)(cd)(ef)>" },   //  536
    { 6,  6,  9, ABC_CONST(0xF000777777777777), "<!(ab)(cd)(ef)>" },   //  537
    { 6,  8, 12, ABC_CONST(0x8888F000F0008888), "<(ab)(cd)[ef]>" },   //  538
    { 6,  8, 12, ABC_CONST(0x7777F000F0007777), "<!(ab)(cd)[ef]>" },   //  539
    { 6,  8, 10, ABC_CONST(0x0FF0888888888888), "<(ab)[cd](ef)>" },   //  540
    { 6,  8, 11, ABC_CONST(0xF000666666666666), "<[ab](cd)(ef)>" },   //  541
    { 6, 10, 14, ABC_CONST(0x6666F000F0006666), "<[ab](cd)[ef]>" },   //  542
    { 6, 10, 12, ABC_CONST(0x0FF0666666666666), "<[ab][cd](ef)>" },   //  543
    { 6, 12, 16, ABC_CONST(0x66660FF00FF06666), "<[ab][cd][ef]>" },   //  544
    { 6,  6, 10, ABC_CONST(0xFF00808080808080), "<(abc)d(ef)>" },   //  545
    { 6,  8, 12, ABC_CONST(0x8080FF00FF008080), "<(abc)d[ef]>" },   //  546
    { 6,  6, 10, ABC_CONST(0xFF00707070707070), "<(!(ab)c)d(ef)>" },   //  547
    { 6,  8, 12, ABC_CONST(0x7070FF00FF007070), "<(!(ab)c)d[ef]>" },   //  548
    { 6,  8, 14, ABC_CONST(0xFF00787878787878), "<[(ab)c]d(ef)>" },   //  549
    { 6, 10, 16, ABC_CONST(0x7878FF00FF007878), "<[(ab)c]d[ef]>" },   //  550
    { 6,  8, 12, ABC_CONST(0xFF00606060606060), "<([ab]c)d(ef)>" },   //  551
    { 6, 10, 14, ABC_CONST(0x6060FF00FF006060), "<([ab]c)d[ef]>" },   //  552
    { 6, 10, 18, ABC_CONST(0xFF00969696969696), "<[abc]d(ef)>" },   //  553
    { 6, 12, 20, ABC_CONST(0x9696FF00FF009696), "<[abc]d[ef]>" },   //  554
    { 6,  7, 10, ABC_CONST(0xFF00CACACACACACA), "<<abc>d(ef)>" },   //  555
    { 6,  9, 12, ABC_CONST(0xCACAFF00FF00CACA), "<<abc>d[ef]>" },   //  556
    { 6,  6,  7, ABC_CONST(0xFF00000080808080), "<(abc)(de)f>" },   //  557
    { 6,  6,  7, ABC_CONST(0xFF0000007F7F7F7F), "<!(abc)(de)f>" },   //  558
    { 6,  8,  8, ABC_CONST(0x00FFFF0080808080), "<(abc)[de]f>" },   //  559
    { 6,  6,  7, ABC_CONST(0xFF00000070707070), "<(!(ab)c)(de)f>" },   //  560
    { 6,  6,  7, ABC_CONST(0xFF0000008F8F8F8F), "<!(!(ab)c)(de)f>" },   //  561
    { 6,  8,  8, ABC_CONST(0x00FFFF0070707070), "<(!(ab)c)[de]f>" },   //  562
    { 6,  8,  9, ABC_CONST(0xFF00000078787878), "<[(ab)c](de)f>" },   //  563
    { 6, 10, 10, ABC_CONST(0x00FFFF0078787878), "<[(ab)c][de]f>" },   //  564
    { 6,  8,  8, ABC_CONST(0xFF00000060606060), "<([ab]c)(de)f>" },   //  565
    { 6,  8,  8, ABC_CONST(0xFF0000009F9F9F9F), "<!([ab]c)(de)f>" },   //  566
    { 6, 10,  9, ABC_CONST(0x00FFFF0060606060), "<([ab]c)[de]f>" },   //  567
    { 6, 10, 11, ABC_CONST(0xFF00000096969696), "<[abc](de)f>" },   //  568
    { 6, 12, 12, ABC_CONST(0x00FFFF0096969696), "<[abc][de]f>" },   //  569
    { 6,  7,  7, ABC_CONST(0xFF000000CACACACA), "<<abc>(de)f>" },   //  570
    { 6,  9,  8, ABC_CONST(0x00FFFF00CACACACA), "<<abc>[de]f>" },   //  571
    { 6,  6,  7, ABC_CONST(0xFFFF000080008000), "<(abcd)ef>" },   //  572
    { 6,  6,  7, ABC_CONST(0xFFFF00007F007F00), "<(!(abc)d)ef>" },   //  573
    { 6,  8, 10, ABC_CONST(0xFFFF00007F807F80), "<[(abc)d]ef>" },   //  574
    { 6,  6,  7, ABC_CONST(0xFFFF000070007000), "<(!(ab)cd)ef>" },   //  575
    { 6,  6,  7, ABC_CONST(0xFFFF00008F008F00), "<(!(!(ab)c)d)ef>" },   //  576
    { 6,  8, 10, ABC_CONST(0xFFFF00008F708F70), "<[(!(ab)c)d]ef>" },   //  577
    { 6,  8,  9, ABC_CONST(0xFFFF000078007800), "<([(ab)c]d)ef>" },   //  578
    { 6, 10, 14, ABC_CONST(0xFFFF000087788778), "<[(ab)cd]ef>" },   //  579
    { 6,  8,  8, ABC_CONST(0xFFFF000060006000), "<([ab]cd)ef>" },   //  580
    { 6,  8,  8, ABC_CONST(0xFFFF00009F009F00), "<(!([ab]c)d)ef>" },   //  581
    { 6, 10, 12, ABC_CONST(0xFFFF00009F609F60), "<[([ab]c)d]ef>" },   //  582
    { 6, 10, 11, ABC_CONST(0xFFFF000096009600), "<([abc]d)ef>" },   //  583
    { 6, 12, 18, ABC_CONST(0xFFFF000069966996), "<[abcd]ef>" },   //  584
    { 6,  7,  7, ABC_CONST(0xFFFF0000CA00CA00), "<(<abc>d)ef>" },   //  585
    { 6,  9, 10, ABC_CONST(0xFFFF000035CA35CA), "<[<abc>d]ef>" },   //  586
    { 6,  6,  8, ABC_CONST(0xFFFF000007770777), "<(!(ab)!(cd))ef>" },   //  587
    { 6,  8, 11, ABC_CONST(0xFFFF000078887888), "<[(ab)(cd)]ef>" },   //  588
    { 6,  8,  9, ABC_CONST(0xFFFF000006660666), "<([ab]!(cd))ef>" },   //  589
    { 6, 10, 10, ABC_CONST(0xFFFF000006600660), "<([ab][cd])ef>" },   //  590
    { 6,  7,  8, ABC_CONST(0xFFFF0000CAAACAAA), "<<ab(cd)>ef>" },   //  591
    { 6,  9, 10, ABC_CONST(0xFFFF0000ACCAACCA), "<<ab[cd]>ef>" },   //  592
    { 6,  7,  7, ABC_CONST(0xFFFF0000F088F088), "<<(ab)cd>ef>" },   //  593
    { 6,  9,  8, ABC_CONST(0xFFFF0000F066F066), "<<[ab]cd>ef>" }    //  594
};

struct Sdm_Man_t_
{
    Sdm_Dsd_t *      pDsd6;                    // NPN class information
    Hsh_IntMan_t *   pHash;                    // maps DSD functions into NPN classes
    Vec_Int_t *      vConfgRes;                // configurations
    Vec_Wrd_t *      vPerm6;                   // permutations of DSD classes
    Vec_Int_t *      vMap2Perm;                // maps number into its permutation
    char             Perm6[720][6];            // permutations
    int              nCountDsd[DSD_CLASS_NUM];
    int              nNonDsd;
    int              nAllDsd;
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sdm_ManPrintDsdStats( Sdm_Man_t * p, int fVerbose )
{
    int i, Absent = 0;
    for ( i = 0; i < DSD_CLASS_NUM; i++ )
    {
        if ( p->nCountDsd[i] == 0 )
        {
            Absent++;
            continue;
        }
        if ( fVerbose )
        {
            printf( "%5d  :  ", i );
            printf( "%-20s   ", p->pDsd6[i].pStr );
            printf( "%8d ",     p->nCountDsd[i] );
            printf( "\n" );
        }
    }
    printf( "Unused classes = %d (%.2f %%).  ", Absent, 100.0 * Absent / DSD_CLASS_NUM );
    printf( "Non-DSD cuts = %d (%.2f %%).  ",   p->nNonDsd, 100.0 * p->nNonDsd / Abc_MaxInt(1, p->nAllDsd) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hsh_IntMan_t * Sdm_ManBuildHashTable( Vec_Int_t ** pvConfgRes ) 
{
    FILE * pFile;
    char * pFileName = "dsdfuncs6.dat";
    int RetValue, size = Extra_FileSize( pFileName ) / 12;  // 2866420
    Vec_Wrd_t * vTruthRes = Vec_WrdAlloc( size );
    Vec_Int_t * vConfgRes = Vec_IntAlloc( size );
    Hsh_IntMan_t * pHash;

    pFile = fopen( pFileName, "rb" );
    RetValue = fread( Vec_WrdArray(vTruthRes), sizeof(word), size, pFile );
    RetValue = fread( Vec_IntArray(vConfgRes), sizeof(int), size, pFile );
    vTruthRes->nSize = size;
    vConfgRes->nSize = size;
    // create hash table
    pHash = Hsh_WrdManHashArrayStart( vTruthRes, 1 );
    // cleanup
    if ( pvConfgRes )
        *pvConfgRes = vConfgRes;
    else
        Vec_IntFree( vConfgRes );
    Vec_WrdFree( vTruthRes );
//    Hsh_IntManStop( pHash );
    return pHash;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sdm_ManPrecomputePerms( Sdm_Man_t * p )
{
    int nVars = 6;
    // 0(1:1) 1(2:1) 2(4:2) 3(10:6) 4(33:23) 5(131:98) 6(595:464)
    int nClasses[7] = { 1, 2, 4, 10, 33, 131, 595 };
    int nPerms = Extra_Factorial( nVars );
//    int nSwaps = (1 << nVars);
    int * pComp, * pPerm;
    int i, k, x, One, OneCopy, Num;
    Vec_Int_t * vVars;
    abctime clk = Abc_Clock();
    assert( p->pDsd6 == NULL );
    p->pDsd6 = s_DsdClass6;
    // precompute schedules
    pComp = Extra_GreyCodeSchedule( nVars );
    pPerm = Extra_PermSchedule( nVars );
    // map numbers into perms
    p->vMap2Perm = Vec_IntStartFull( (1<<(3*nVars)) );
    // store permutations
    One = 0;
    for ( x = 0; x < nVars; x++ )
    {
        p->Perm6[0][x] = (char)x;
        One |= (x << (3*x));
    }
//    Vec_IntWriteEntry( p->vMap2Perm, One, 0 );
    OneCopy = One;
    for ( k = 0; k < nPerms; k++ )
    {
        if ( k > 0 )
        for ( x = 0; x < nVars; x++ )
            p->Perm6[k][x] = p->Perm6[k-1][x];
        ABC_SWAP( char, p->Perm6[k][pPerm[k]], p->Perm6[k][pPerm[k]+1] );

        Num = ( (One >> (3*(pPerm[k]  ))) ^ (One >> (3*(pPerm[k]+1))) ) & 7;
        One ^=  (Num << (3*(pPerm[k]  )));
        One ^=  (Num << (3*(pPerm[k]+1)));

        Vec_IntWriteEntry( p->vMap2Perm, One, k );
        
//        Sdm_ManPrintPerm( One );
//        for ( x = 0; x < nVars; x++ )
//            printf( "%d ", p->Perm6[k][x] );
//        printf( "\n" );
    }
    assert( OneCopy == One );
    // fill in the gaps
    vVars = Vec_IntAlloc( 6 );
    Vec_IntForEachEntry( p->vMap2Perm, Num, i )
    {
        // mark used variables
        int Count = 0;
        One = i;
        Vec_IntFill( vVars, 6, 0 );
        for ( k = 0; k < nVars; k++ )
        {
            int iVar = ((One >> (3*k)) & 7);
            if ( iVar >= nVars && iVar < 7 )
                break;
            if ( iVar != 7 )
            {
                if ( Vec_IntEntry( vVars, iVar ) == 1 )
                    break;
                Vec_IntWriteEntry( vVars, iVar, 1 );
                Count++;
            }
        }
        // skip ones with dups and complete
        if ( k < nVars || Count == nVars )
            continue;
        // find unused variables
        for ( x = k = 0; k < 6; k++ )
            if ( Vec_IntEntry(vVars, k) == 0 )
                Vec_IntWriteEntry( vVars, x++, k );
        Vec_IntShrink( vVars, x );
        // fill in used variables
        x = 0;
        for ( k = 0; k < nVars; k++ )
        {
            int iVar = ((One >> (3*k)) & 7);
            if ( iVar == 7 )
                One ^= ((Vec_IntEntry(vVars, x++) ^ 7) << (3*k));
        }
        assert( x == Vec_IntSize(vVars) );
        // save this one
        assert( Vec_IntEntry( p->vMap2Perm, One ) != -1 );
        Vec_IntWriteEntry( p->vMap2Perm, i, Vec_IntEntry(p->vMap2Perm, One) );
/*
        // mapping
        Sdm_ManPrintPerm( i );
        printf( "->  " );
        Sdm_ManPrintPerm( One );
        printf( "\n" );
*/
    }
    Vec_IntFree( vVars );

    // store permuted truth tables
    assert( p->vPerm6 == NULL );
    p->vPerm6 = Vec_WrdAlloc( nPerms * DSD_CLASS_NUM );
    for ( i = 0; i < nClasses[nVars]; i++ )
    {
        word uTruth = s_DsdClass6[i].uTruth;
        for ( k = 0; k < nPerms; k++ )
        {
            uTruth = Abc_Tt6SwapAdjacent( uTruth, pPerm[k] );
            Vec_WrdPush( p->vPerm6, uTruth );
        }
        assert( uTruth == s_DsdClass6[i].uTruth );
    }
    ABC_FREE( pPerm );
    ABC_FREE( pComp );
    // build hash table
    p->pHash = Sdm_ManBuildHashTable( &p->vConfgRes );
    Abc_PrintTime( 1, "Setting up DSD information", Abc_Clock() - clk );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sdm_ManPrintPerm( unsigned s )
{
    int i;
    for ( i = 0; i < 6; i++ )
        printf( "%d ", (s >> (3*i)) & 7 );
    printf( "  " );
}

/**Function*************************************************************

  Synopsis    [Checks hash table for DSD class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sdm_ManCheckDsd6( Sdm_Man_t * p, word t )
{
    int fCompl, Entry, Config;
    if ( (fCompl = (t & 1)) )
        t = ~t;
    Entry = *Hsh_IntManLookup( p->pHash, (unsigned *)&t );
    if ( Entry == -1 )
        return -1;
    Config = Vec_IntEntry( p->vConfgRes, Entry );
    if ( fCompl )
        Config ^= (1 << 16);
    return Config;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sdm_ManComputeFunc( Sdm_Man_t * p, int iDsdLit0, int iDsdLit1, int * pCut, int uMask, int fXor )
{
//    int fVerbose = 0;
    int i, Config, iClass, fCompl, Res;
    int PermMask = uMask & 0x3FFFF;
    int ComplMask = uMask >> 18;
    word Truth0, Truth1p, t0, t1, t;
    p->nAllDsd++;

    assert( uMask > 1 );
    assert( iDsdLit0 < DSD_CLASS_NUM * 2 );
    assert( iDsdLit1 < DSD_CLASS_NUM * 2 );
    Truth0  = p->pDsd6[Abc_Lit2Var(iDsdLit0)].uTruth;
    Truth1p = Vec_WrdEntry( p->vPerm6, Abc_Lit2Var(iDsdLit1) * 720 + Vec_IntEntry(p->vMap2Perm, PermMask ) );
    if ( ComplMask )
        for ( i = 0; i < 6; i++ )
            if ( (ComplMask >> i) & 1 )
                Truth1p = Abc_Tt6Flip( Truth1p, i );            
    t0 = Abc_LitIsCompl(iDsdLit0) ? ~Truth0  : Truth0;
    t1 = Abc_LitIsCompl(iDsdLit1) ? ~Truth1p : Truth1p;
    t = fXor ? t0 ^ t1 : t0 & t1;
/*
if ( fVerbose )
{
Sdm_ManPrintPerm( PermMask );                      printf( "\n" );
Extra_PrintBinary( stdout, &ComplMask, 6 );        printf( "\n" );
Kit_DsdPrintFromTruth( (unsigned *)&Truth0, 6 );   printf( "\n" );
Kit_DsdPrintFromTruth( (unsigned *)&Truth1p, 6 );  printf( "\n" );
Kit_DsdPrintFromTruth( (unsigned *)&t, 6 );        printf( "\n" );
}
*/
    // find configuration
    Config = Sdm_ManCheckDsd6( p, t );
    if ( Config == -1 )
    {
        p->nNonDsd++;
        return -1;
    }

    // get the class
    iClass = Config >> 17;
    fCompl = (Config >> 16) & 1;
    Config &= 0xFFFF;

    // set the function
    Res = Abc_Var2Lit( iClass, fCompl );

    // update cut
    assert( (Config >> 6) < 720 );
    if ( pCut )
    {
        int pLeavesNew[6] = { -1, -1, -1, -1, -1, -1 };
        assert( pCut[0] <= 6 );
        for ( i = 0; i < pCut[0]; i++ )
            pLeavesNew[(int)(p->Perm6[Config >> 6][i])] = Abc_LitNotCond( pCut[i+1], (Config >> i) & 1 );
        pCut[0] = p->pDsd6[iClass].nVars;
        for ( i = 0; i < pCut[0]; i++ )
            assert( pLeavesNew[i] != -1 );
        for ( i = 0; i < pCut[0]; i++ )
            pCut[i+1] = pLeavesNew[i];
    }
    assert( iClass < DSD_CLASS_NUM );
    p->nCountDsd[iClass]++;
    return Res;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sdm_ManReadDsdVarNum( Sdm_Man_t * p, int iDsd )
{
    return p->pDsd6[iDsd].nVars;
}
int Sdm_ManReadDsdAndNum( Sdm_Man_t * p, int iDsd )
{
    return p->pDsd6[iDsd].nAnds;
}
int Sdm_ManReadDsdClauseNum( Sdm_Man_t * p, int iDsd )
{
    return p->pDsd6[iDsd].nClauses;
}
word Sdm_ManReadDsdTruth( Sdm_Man_t * p, int iDsd )
{
    return p->pDsd6[iDsd].uTruth;
}
char * Sdm_ManReadDsdStr( Sdm_Man_t * p, int iDsd )
{
    return p->pDsd6[iDsd].pStr;
}
void Sdm_ManReadCnfCosts( Sdm_Man_t * p, int * pCosts, int nCosts )
{
    int i;
    assert( nCosts == DSD_CLASS_NUM );
    pCosts[0] = pCosts[1] = 0;
    for ( i = 2; i < DSD_CLASS_NUM; i++ )
        pCosts[i] = Sdm_ManReadDsdClauseNum( p, i );
}


/**Function*************************************************************

  Synopsis    [Manager manipulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sdm_Man_t * Sdm_ManAlloc()
{
    Sdm_Man_t * p;
    p = ABC_CALLOC( Sdm_Man_t, 1 );
    Sdm_ManPrecomputePerms( p );
    return p;
}
void Sdm_ManFree( Sdm_Man_t * p )
{
    Vec_WrdFree( p->vPerm6 );
    Vec_IntFree( p->vMap2Perm );
    Vec_IntFree( p->vConfgRes );
    Vec_IntFree( p->pHash->vData );
    Hsh_IntManStop( p->pHash );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Sdm_Man_t * s_SdmMan = NULL;
Sdm_Man_t * Sdm_ManRead()
{
    if ( s_SdmMan == NULL )
        s_SdmMan = Sdm_ManAlloc();
    memset( s_SdmMan->nCountDsd, 0, sizeof(int) * DSD_CLASS_NUM );
    return s_SdmMan;
}
void Sdm_ManQuit()
{
    if ( s_SdmMan != NULL )
        Sdm_ManFree( s_SdmMan );
    s_SdmMan = NULL;
}
int Sdm_ManCanRead()
{
    char * pFileName = "dsdfuncs6.dat";
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
        return 0;
    fclose( pFile );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sdm_ManTest()
{
    Sdm_Man_t * p;
    int iDsd0 = 4;
    int iDsd1 = 6;
    int iDsd;
    int uMask = 0x3FFFF ^ ((7 ^ 0) << 6) ^ ((7 ^ 1) << 9);
    int pCut[7] = { 4, 10, 20, 30, 40 };
//    Sdm_ManPrintPerm( uMask );
    p = Sdm_ManAlloc();
    iDsd = Sdm_ManComputeFunc( p, iDsd0, iDsd1, pCut, uMask, 0 );
    Sdm_ManFree( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
void Sdm_ManCompareCnfSizes()
{
    Vec_Int_t * vMemory;
    word uTruth;
    int i, nSop0, nSop1, nVars, nCla, RetValue;
    vMemory = Vec_IntAlloc( 1 << 16 );
    for ( i = 1; i < DSD_CLASS_NUM; i++ )
    {
        uTruth = Sdm_ManReadDsdTruth( s_SdmMan, i );
        nVars = Sdm_ManReadDsdVarNum( s_SdmMan, i );
        nCla = Sdm_ManReadDsdClauseNum( s_SdmMan, i );

        RetValue = Kit_TruthIsop( &uTruth, nVars, vMemory, 0 );
        nSop0 = Vec_IntSize(vMemory);

        uTruth = ~uTruth;
        RetValue = Kit_TruthIsop( &uTruth, nVars, vMemory, 0 );
        nSop1 = Vec_IntSize(vMemory);

        if ( nSop0 + nSop1 != nCla )
            printf( "Class %4d : %d + %d != %d\n", i, nSop0, nSop1, nCla );
        else
            printf( "Class %4d : %d + %d == %d\n", i, nSop0, nSop1, nCla );
    }
    Vec_IntFree( vMemory );
}
*/


/**Function*************************************************************

  Synopsis    [Collect DSD divisors of the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sdm_ManDivCollect_rec( word t, Vec_Wrd_t ** pvDivs )
{
    int i, Config, Counter = 0;
    // check if function is decomposable
    Config = Sdm_ManCheckDsd6( s_SdmMan, t );
    if ( Config != -1 && (Config >> 17) < 2 )
        return;
    for ( i = 0; i < 6; i++ )
    {
        if ( !Abc_Tt6HasVar( t, i ) )
            continue;
        Sdm_ManDivCollect_rec( Abc_Tt6Cofactor0(t, i), pvDivs );
        Sdm_ManDivCollect_rec( Abc_Tt6Cofactor1(t, i), pvDivs );
        Counter++;
    }
    if ( Config != -1 && Counter >= 2 && Counter <= 4 )
    {
        Vec_WrdPush( pvDivs[Counter], (t & 1) ? ~t : t );
//        Kit_DsdPrintFromTruth( (unsigned *)&t, 6 ); printf( "\n" );
    }
}
void Sdm_ManDivTest()
{
//    word u, t0, t1, t = ABC_CONST(0xB0F0BBFFB0F0BAFE);
//    word u, t0, t1, t = ABC_CONST(0x3F1F3F13FFDFFFD3);
    word u, t0, t1, t = ABC_CONST(0x3F3FFFFF37003700);
    Rsb_Man_t * pManRsb = Rsb_ManAlloc( 6, 200, 3, 1 );
    Vec_Wrd_t * pvDivs[7] = { NULL };
    Vec_Wrd_t * vDivs = Vec_WrdAlloc( 100 );
    int i, RetValue;

    // collect divisors
    for ( i = 2; i <= 4; i++ )
        pvDivs[i] = Vec_WrdAlloc( 100 );
    Sdm_ManDivCollect_rec( t, pvDivs );
    for ( i = 2; i <= 4; i++ )
        Vec_WrdUniqify( pvDivs[i] );

    // prepare the set
    vDivs = Vec_WrdAlloc( 100 );
    for ( i = 0; i < 6; i++ )
        Vec_WrdPush( vDivs, s_Truths6[i] );
    for ( i = 2; i <= 4; i++ )
        Vec_WrdAppend( vDivs, pvDivs[i] );
    for ( i = 2; i <= 4; i++ )
        Vec_WrdFree( pvDivs[i] );

    Vec_WrdForEachEntry( vDivs, u, i )
    {
        printf( "%2d : ", i );
//        Kit_DsdPrintFromTruth( (unsigned *)&u, 6 ); 
        printf( "\n" );
    }

    RetValue = Rsb_ManPerformResub6( pManRsb, 6, t, vDivs, &t0, &t1, 1 );
    if ( RetValue )
    {
//        Kit_DsdPrintFromTruth( (unsigned *)&t0, 6 ); printf( "\n" );
//        Kit_DsdPrintFromTruth( (unsigned *)&t1, 6 ); printf( "\n" );
        printf( "Decomposition exits.\n" );
    }


    Vec_WrdFree( vDivs );
    Rsb_ManFree( pManRsb );
}


/**Function*************************************************************

  Synopsis    [Generation of node test.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
#include "bool/kit/kit.h"
void Sdm_ManNodeGenTest()
{
    extern Kit_Graph_t * Kit_TruthToGraph( unsigned * pTruth, int nVars, Vec_Int_t * vMemory );
    Sdm_Man_t * p = s_SdmMan;
    Vec_Int_t * vCover;
    Kit_Graph_t * pGraph;
    int i;
    vCover = Vec_IntAlloc( 1 << 16 );
    for ( i = 2; i < DSD_CLASS_NUM; i++ )
    {
        pGraph = Kit_TruthToGraph( (unsigned *)&p->pDsd6[i].uTruth, p->pDsd6[i].nVars, vCover );
        printf( "%d %s %d %d   ", i, p->pDsd6[i].pStr, Kit_GraphNodeNum(pGraph), p->pDsd6[i].nAnds );
    }
    printf( "\n" );
}
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

