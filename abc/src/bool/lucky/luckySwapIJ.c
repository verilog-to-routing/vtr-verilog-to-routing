/**CFile****************************************************************

  FileName    [luckySwapIJ.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Semi-canonical form computation package.]

  Synopsis    [just for support of swap_ij() function]

  Author      [Jake]

  Date        [Started - September 2012]

***********************************************************************/

#include "luckyInt.h"


ABC_NAMESPACE_IMPL_START


void swap_ij_case1( word* f,int totalVars, int i, int j)
{
    int e,wordsNumber,n,shift;
    word maskArray[45]=
    {   ABC_CONST(0x9999999999999999), ABC_CONST(0x2222222222222222), ABC_CONST(0x4444444444444444) ,ABC_CONST(0xA5A5A5A5A5A5A5A5), ABC_CONST(0x0A0A0A0A0A0A0A0A), ABC_CONST(0x5050505050505050),
        ABC_CONST(0xAA55AA55AA55AA55), ABC_CONST(0x00AA00AA00AA00AA), ABC_CONST(0x5500550055005500) ,ABC_CONST(0xAAAA5555AAAA5555), ABC_CONST(0x0000AAAA0000AAAA), ABC_CONST(0x5555000055550000),
        ABC_CONST(0xAAAAAAAA55555555), ABC_CONST(0x00000000AAAAAAAA), ABC_CONST(0x5555555500000000) ,ABC_CONST(0xC3C3C3C3C3C3C3C3), ABC_CONST(0x0C0C0C0C0C0C0C0C), ABC_CONST(0x3030303030303030),
        ABC_CONST(0xCC33CC33CC33CC33), ABC_CONST(0x00CC00CC00CC00CC), ABC_CONST(0x3300330033003300) ,ABC_CONST(0xCCCC3333CCCC3333), ABC_CONST(0x0000CCCC0000CCCC), ABC_CONST(0x3333000033330000),
        ABC_CONST(0xCCCCCCCC33333333), ABC_CONST(0x00000000CCCCCCCC), ABC_CONST(0x3333333300000000) ,ABC_CONST(0xF00FF00FF00FF00F), ABC_CONST(0x00F000F000F000F0), ABC_CONST(0x0F000F000F000F00),
        ABC_CONST(0xF0F00F0FF0F00F0F), ABC_CONST(0x0000F0F00000F0F0), ABC_CONST(0x0F0F00000F0F0000) ,ABC_CONST(0xF0F0F0F00F0F0F0F), ABC_CONST(0x00000000F0F0F0F0), ABC_CONST(0x0F0F0F0F00000000),
        ABC_CONST(0xFF0000FFFF0000FF), ABC_CONST(0x0000FF000000FF00), ABC_CONST(0x00FF000000FF0000) ,ABC_CONST(0xFF00FF0000FF00FF), ABC_CONST(0x00000000FF00FF00), ABC_CONST(0x00FF00FF00000000),
        ABC_CONST(0xFFFF00000000FFFF), ABC_CONST(0x00000000FFFF0000), ABC_CONST(0x0000FFFF00000000)
    };
    e = 3*((9*i - i*i -2)/2 + j);          //  Exact formula for index in maskArray
    wordsNumber = Kit_TruthWordNum_64bit(totalVars);
    shift = (1<<j)-(1<<i);
    for(n = 0; n < wordsNumber; n++)
        f[n] = (f[n]&maskArray[e])+((f[n]&(maskArray[e+1]))<< shift)+((f[n]&(maskArray[e+2]))>> shift);
}
//  "width" - how many "Words" in a row have "1s" (or "0s")in position "i"
//   wi - width of i
//   wj - width of j
//   wwi = 2*wi; wwj = 2*wj;

void swap_ij_case2( word* f,int totalVars, int i, int j)
{
    word mask[] = { ABC_CONST(0xAAAAAAAAAAAAAAAA), ABC_CONST(0xCCCCCCCCCCCCCCCC), ABC_CONST(0xF0F0F0F0F0F0F0F0),
                    ABC_CONST(0xFF00FF00FF00FF00), ABC_CONST(0xFFFF0000FFFF0000), ABC_CONST(0xFFFFFFFF00000000) };
    word temp;
    int x,y,wj;
    int WORDS_IN_TT = Kit_TruthWordNum_64bit(totalVars);
    //  int forShift = ((Word)1)<<i;
    int forShift = (1<<i);
    wj = 1 << (j - 6);
    x = 0;
    y = wj;
    for(y=wj; y<WORDS_IN_TT;y+=2*wj)
        for(x=y-wj; x < y; x++)
        {
            temp = f[x+wj];
            f[x+wj] = ((f[x+wj])&(mask[i])) + (((f[x]) & (mask[i])) >> forShift);
            f[x] = ((f[x])&(~mask[i])) + ((temp&(~mask[i])) << forShift);
        }
}

void swap_ij_case3( word* f,int totalVars, int i, int j)
{
    int x,y,wwi,wwj,shift;
    int WORDS_IN_TT;
    int SizeOfBlock;
    word* temp;
    wwi = 1 << (i - 5);
    wwj = 1 << (j - 5);
    shift = (wwj - wwi)/2;
    WORDS_IN_TT = Kit_TruthWordNum_64bit(totalVars);
    SizeOfBlock = sizeof(word)*wwi/2;
    temp = (word *)malloc(SizeOfBlock);
    for(y=wwj/2; y<WORDS_IN_TT; y+=wwj)
        for(x=y-shift; x<y; x+=wwi)
        {
            memcpy(temp,&f[x],SizeOfBlock);
            memcpy(&f[x],&f[x+shift],SizeOfBlock);
            memcpy(&f[x+shift],temp,SizeOfBlock);
        }
}
void swap_ij( word* f,int totalVars, int varI, int varJ)
{
    if (varI == varJ)
        return;
    else if(varI>varJ)
        swap_ij( f,totalVars,varJ,varI);
    else if((varI <= 4) && (varJ <= 5))
        swap_ij_case1(f,totalVars, varI, varJ);
    else if((varI <= 5) && (varJ > 5))
        swap_ij_case2(f,totalVars, varI, varJ);
    else if((varI > 5) && (varJ > 5))
        swap_ij_case3(f,totalVars,varI,varJ);
}

ABC_NAMESPACE_IMPL_END
