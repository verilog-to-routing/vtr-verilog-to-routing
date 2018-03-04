/**CFile****************************************************************

 FileName    [extraBddThresh.c]

 SystemName  [ABC: Logic synthesis and verification system.]

 PackageName [extra]

 Synopsis    [Dealing with threshold functions.]

 Author      [Augusto Neutzling, Jody Matos, and Alan Mishchenko (UC Berkeley).]

 Affiliation [Federal University of Rio Grande do Sul, Brazil]

 Date        [Ver. 1.0. Started - October 7, 2014.]

 Revision    [$Id: extraBddThresh.c,v 1.0 2014/10/07 00:00:00 alanmi Exp $]

 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "base/main/main.h"
#include "misc/extra/extra.h"
#include "bdd/cudd/cudd.h"
#include "bool/kit/kit.h"

#include "misc/vec/vec.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

 Synopsis    [Checks thresholdness of the function.]

 Description []

 SideEffects []

 SeeAlso     []

 ***********************************************************************/
void Extra_ThreshPrintChow(int Chow0, int * pChow, int nVars) {
	int i;
	for (i = 0; i < nVars; i++)
		printf("%d ", pChow[i]);
	printf("  %d\n", Chow0);
}
int Extra_ThreshComputeChow(word * t, int nVars, int * pChow) {
	int i, k, Chow0 = 0, nMints = (1 << nVars);
	memset(pChow, 0, sizeof(int) * nVars);
	// compute Chow coefs
	for (i = 0; i < nMints; i++)
		if (Abc_TtGetBit(t, i))
			for (Chow0++, k = 0; k < nVars; k++)
				if ((i >> k) & 1)
					pChow[k]++;
	// compute modified Chow coefs
	for (k = 0; k < nVars; k++)
		pChow[k] = 2 * pChow[k] - Chow0;
	return Chow0 - (1 << (nVars - 1));
}
void Extra_ThreshSortByChow(word * t, int nVars, int * pChow) {
	int i, nWords = Abc_TtWordNum(nVars);
	//sort the variables by Chow in ascending order
	while (1) {
		int fChange = 0;
		for (i = 0; i < nVars - 1; i++) {
			if (pChow[i] >= pChow[i + 1])
				continue;
			ABC_SWAP(int, pChow[i], pChow[i + 1]);
			Abc_TtSwapAdjacent(t, nWords, i);
			fChange = 1;
		}
		if (!fChange)
			return;
	}
}
void Extra_ThreshSortByChowInverted(word * t, int nVars, int * pChow) {
	int i, nWords = Abc_TtWordNum(nVars);
	//sort the variables by Chow in descending order
	while (1) {
		int fChange = 0;
		for (i = 0; i < nVars - 1; i++) {
			if (pChow[i] <= pChow[i + 1])
				continue;
			ABC_SWAP(int, pChow[i], pChow[i + 1]);
			Abc_TtSwapAdjacent(t, nWords, i);
			fChange = 1;
		}
		if (!fChange)
			return;
	}
}
int Extra_ThreshInitializeChow(int nVars, int * pChow) {
	int i = 0, Aux[16], nChows = 0;
	//group the variables which have the same Chow
	for (i = 0; i < nVars; i++) {
		if (i == 0 || (pChow[i] == pChow[i - 1]))
			Aux[i] = nChows;
		else {
			nChows++;
			Aux[i] = nChows;
		}
	}
	for (i = 0; i < nVars; i++)
		pChow[i] = Aux[i];
	nChows++;
	return nChows;

}
static inline int Extra_ThreshWeightedSum(int * pW, int nVars, int m) {
	int i, Cost = 0;
	for (i = 0; i < nVars; i++)
		if ((m >> i) & 1)
			Cost += pW[i];
	return Cost;
}
static inline int Extra_ThreshCubeWeightedSum1(int * pWofChow, int * pChow,
		char * pIsop, int nVars, int j) {
	int k, Cost = 0;
	for (k = j; k < j + nVars; k++)
		if (pIsop[k] == '1')
			Cost += pWofChow[pChow[k - j]];
	return Cost;
}
static inline int Extra_ThreshCubeWeightedSum2(int * pWofChow, int * pChow,
		char * pIsop, int nVars, int j) {
	int k, Cost = 0;
	for (k = j; k < j + nVars; k++)
		if (pIsop[k] == '-')
			Cost += pWofChow[pChow[k - j]];
	return Cost;
}

static inline int Extra_ThreshCubeWeightedSum3(int * pWofChow, int nChows,
		unsigned long ** pGreaters, int j) {
	int i, Cost = 0;
	for (i = 0; i < nChows; i++)
		Cost += pWofChow[i] * pGreaters[j][i];
	return Cost;
}
static inline int Extra_ThreshCubeWeightedSum4(int * pWofChow, int nChows,
		unsigned long ** pSmallers, int j) {
	int i, Cost = 0;
	for (i = 0; i < nChows; i++)
		Cost += pWofChow[i] * pSmallers[j][i];
	return Cost;
}
int Extra_ThreshSelectWeights3(word * t, int nVars, int * pW) {
	int m, Lmin, Lmax, nMints = (1 << nVars);
	assert(nVars == 3);
	for (pW[2] = 1; pW[2] <= nVars; pW[2]++)
		for (pW[1] = pW[2]; pW[1] <= nVars; pW[1]++)
			for (pW[0] = pW[1]; pW[0] <= nVars; pW[0]++) {
				Lmin = 10000;
				Lmax = 0;
				for (m = 0; m < nMints; m++) {
					if (Abc_TtGetBit(t, m))
						Lmin = Abc_MinInt(Lmin,
								Extra_ThreshWeightedSum(pW, nVars, m));
					else
						Lmax = Abc_MaxInt(Lmax,
								Extra_ThreshWeightedSum(pW, nVars, m));
					if (Lmax >= Lmin)
						break;
//            printf( "%c%d ", Abc_TtGetBit(t, m) ? '+' : '-', Extra_ThreshWeightedSum(pW, nVars, m) );
				}
//        printf( "  -%d +%d\n", Lmax, Lmin );
				if (m < nMints)
					continue;
				assert(Lmax < Lmin);
				return Lmin;
			}
	return 0;
}
int Extra_ThreshSelectWeights4(word * t, int nVars, int * pW) {
	int m, Lmin, Lmax, nMints = (1 << nVars);
	assert(nVars == 4);
	for (pW[3] = 1; pW[3] <= nVars; pW[3]++)
		for (pW[2] = pW[3]; pW[2] <= nVars; pW[2]++)
			for (pW[1] = pW[2]; pW[1] <= nVars; pW[1]++)
				for (pW[0] = pW[1]; pW[0] <= nVars; pW[0]++) {
					Lmin = 10000;
					Lmax = 0;
					for (m = 0; m < nMints; m++) {
						if (Abc_TtGetBit(t, m))
							Lmin = Abc_MinInt(Lmin,
									Extra_ThreshWeightedSum(pW, nVars, m));
						else
							Lmax = Abc_MaxInt(Lmax,
									Extra_ThreshWeightedSum(pW, nVars, m));
						if (Lmax >= Lmin)
							break;
					}
					if (m < nMints)
						continue;
					assert(Lmax < Lmin);
					return Lmin;
				}
	return 0;
}
int Extra_ThreshSelectWeights5(word * t, int nVars, int * pW) {
	int m, Lmin, Lmax, nMints = (1 << nVars), Limit = nVars + 0;
	assert(nVars == 5);
	for (pW[4] = 1; pW[4] <= Limit; pW[4]++)
		for (pW[3] = pW[4]; pW[3] <= Limit; pW[3]++)
			for (pW[2] = pW[3]; pW[2] <= Limit; pW[2]++)
				for (pW[1] = pW[2]; pW[1] <= Limit; pW[1]++)
					for (pW[0] = pW[1]; pW[0] <= Limit; pW[0]++) {
						Lmin = 10000;
						Lmax = 0;
						for (m = 0; m < nMints; m++) {
							if (Abc_TtGetBit(t, m))
								Lmin = Abc_MinInt(Lmin,
										Extra_ThreshWeightedSum(pW, nVars, m));
							else
								Lmax = Abc_MaxInt(Lmax,
										Extra_ThreshWeightedSum(pW, nVars, m));
							if (Lmax >= Lmin)
								break;
						}
						if (m < nMints)
							continue;
						assert(Lmax < Lmin);
						return Lmin;
					}
	return 0;
}
int Extra_ThreshSelectWeights6(word * t, int nVars, int * pW) {
	int m, Lmin, Lmax, nMints = (1 << nVars), Limit = nVars + 3;
	assert(nVars == 6);
	for (pW[5] = 1; pW[5] <= Limit; pW[5]++)
		for (pW[4] = pW[5]; pW[4] <= Limit; pW[4]++)
			for (pW[3] = pW[4]; pW[3] <= Limit; pW[3]++)
				for (pW[2] = pW[3]; pW[2] <= Limit; pW[2]++)
					for (pW[1] = pW[2]; pW[1] <= Limit; pW[1]++)
						for (pW[0] = pW[1]; pW[0] <= Limit; pW[0]++) {
							Lmin = 10000;
							Lmax = 0;
							for (m = 0; m < nMints; m++) {
								if (Abc_TtGetBit(t, m))
									Lmin = Abc_MinInt(Lmin,
											Extra_ThreshWeightedSum(pW, nVars,
													m));
								else
									Lmax = Abc_MaxInt(Lmax,
											Extra_ThreshWeightedSum(pW, nVars,
													m));
								if (Lmax >= Lmin)
									break;
							}
							if (m < nMints)
								continue;
							assert(Lmax < Lmin);
							return Lmin;
						}
	return 0;
}
int Extra_ThreshSelectWeights7(word * t, int nVars, int * pW) {
	int m, Lmin, Lmax, nMints = (1 << nVars), Limit = nVars + 6;
	assert(nVars == 7);
	for (pW[6] = 1; pW[6] <= Limit; pW[6]++)
		for (pW[5] = pW[6]; pW[5] <= Limit; pW[5]++)
			for (pW[4] = pW[5]; pW[4] <= Limit; pW[4]++)
				for (pW[3] = pW[4]; pW[3] <= Limit; pW[3]++)
					for (pW[2] = pW[3]; pW[2] <= Limit; pW[2]++)
						for (pW[1] = pW[2]; pW[1] <= Limit; pW[1]++)
							for (pW[0] = pW[1]; pW[0] <= Limit; pW[0]++) {
								Lmin = 10000;
								Lmax = 0;
								for (m = 0; m < nMints; m++) {
									if (Abc_TtGetBit(t, m))
										Lmin = Abc_MinInt(Lmin,
												Extra_ThreshWeightedSum(pW,
														nVars, m));
									else
										Lmax = Abc_MaxInt(Lmax,
												Extra_ThreshWeightedSum(pW,
														nVars, m));
									if (Lmax >= Lmin)
										break;
								}
								if (m < nMints)
									continue;
								assert(Lmax < Lmin);
								return Lmin;
							}
	return 0;
}
int Extra_ThreshSelectWeights8(word * t, int nVars, int * pW) {
	int m, Lmin, Lmax, nMints = (1 << nVars), Limit = nVars + 1; // <<-- incomplete detection to save runtime!
	assert(nVars == 8);
	for (pW[7] = 1; pW[7] <= Limit; pW[7]++)
		for (pW[6] = pW[7]; pW[6] <= Limit; pW[6]++)
			for (pW[5] = pW[6]; pW[5] <= Limit; pW[5]++)
				for (pW[4] = pW[5]; pW[4] <= Limit; pW[4]++)
					for (pW[3] = pW[4]; pW[3] <= Limit; pW[3]++)
						for (pW[2] = pW[3]; pW[2] <= Limit; pW[2]++)
							for (pW[1] = pW[2]; pW[1] <= Limit; pW[1]++)
								for (pW[0] = pW[1]; pW[0] <= Limit; pW[0]++) {
									Lmin = 10000;
									Lmax = 0;
									for (m = 0; m < nMints; m++) {
										if (Abc_TtGetBit(t, m))
											Lmin = Abc_MinInt(Lmin,
													Extra_ThreshWeightedSum(pW,
															nVars, m));
										else
											Lmax = Abc_MaxInt(Lmax,
													Extra_ThreshWeightedSum(pW,
															nVars, m));
										if (Lmax >= Lmin)
											break;
									}
									if (m < nMints)
										continue;
									assert(Lmax < Lmin);
									return Lmin;
								}
	return 0;
}
int Extra_ThreshSelectWeights(word * t, int nVars, int * pW) {
	if (nVars <= 2)
		return (t[0] & 0xF) != 6 && (t[0] & 0xF) != 9;
	if (nVars == 3)
		return Extra_ThreshSelectWeights3(t, nVars, pW);
	if (nVars == 4)
		return Extra_ThreshSelectWeights4(t, nVars, pW);
	if (nVars == 5)
		return Extra_ThreshSelectWeights5(t, nVars, pW);
	if (nVars == 6)
		return Extra_ThreshSelectWeights6(t, nVars, pW);
	if (nVars == 7)
		return Extra_ThreshSelectWeights7(t, nVars, pW);
	if (nVars == 8)
		return Extra_ThreshSelectWeights8(t, nVars, pW);
	return 0;
}
void Extra_ThreshIncrementWeights(int nChows, int * pWofChow, int i) {
	int k;
	for (k = i; k < nChows; k++) {
		pWofChow[k]++;
	}
}
void Extra_ThreshDecrementWeights(int nChows, int * pWofChow, int i) {
	int k;
	for (k = i; k < nChows; k++) {
		pWofChow[k]--;
	}
}
void Extra_ThreshPrintInequalities(unsigned long ** pGreaters,
		unsigned long ** pSmallers, int nChows, int nInequalities) {
	int i = 0, k = 0;
	for (k = 0; k < nInequalities; k++) {
		printf("\n Inequality [%d] = ", k);
		for (i = 0; i < nChows; i++) {
			printf("%ld ", pGreaters[k][i]);
		}
		printf(" > ");

		for (i = 0; i < nChows; i++) {
			printf("%ld ", pSmallers[k][i]);
		}
	}
}
void Extra_ThreshCreateInequalities(char * pIsop, char * pIsopFneg, int nVars,
		int * pWofChow, int * pChow, int nChows, int nInequalities,
		unsigned long ** pGreaters, unsigned long ** pSmallers) {
	int i = 0, j = 0, k = 0, m = 0;

	int nCubesIsop = strlen(pIsop) / (nVars + 3);
	int nCubesIsopFneg = strlen(pIsopFneg) / (nVars + 3);

	for (k = 0; k < nCubesIsop * nCubesIsopFneg; k++)
		for (i = 0; i < nChows; i++) {
			pGreaters[k][i] = 0;
			pSmallers[k][i] = 0;
		}
	m = 0;
	for (i = 0; i < (int)strlen(pIsop); i += (nVars + 3))
		for (j = 0; j < nCubesIsopFneg; j++) {
			for (k = 0; k < nVars; k++)
				if (pIsop[i + k] == '1')
					pGreaters[m][pChow[k]] = pGreaters[m][(pChow[k])] + 1;

			m++;
		}
	m = 0;
	for (i = 0; i < nCubesIsop; i++)
		for (j = 0; j < (int)strlen(pIsopFneg); j += (nVars + 3)) {
			for (k = 0; k < nVars; k++)
				if (pIsopFneg[j + k] == '-')
					pSmallers[m][pChow[k]] = pSmallers[m][pChow[k]] + 1;
			m++;
		}
//		Extra_ThreshPrintInequalities( pGreaters, pSmallers, nChows, nInequalities);
//		printf( "\nInequalities was Created \n");
}

void Extra_ThreshSimplifyInequalities(int nInequalities, int nChows,
		unsigned long ** pGreaters, unsigned long ** pSmallers) {
	int i = 0, k = 0;

	for (k = 0; k < nInequalities; k++) {
		for (i = 0; i < nChows; i++) {
			if ((pGreaters[k][i]) == (pSmallers[k][i])) {
				pGreaters[k][i] = 0;
				pSmallers[k][i] = 0;
			} else if ((pGreaters[k][i]) > (pSmallers[k][i])) {
				pGreaters[k][i] = pGreaters[k][i] - pSmallers[k][i];
				pSmallers[k][i] = 0;
			} else {
				pSmallers[k][i] = pSmallers[k][i] - pGreaters[k][i];
				;
				pGreaters[k][i] = 0;
			}
		}
	}
//		Extra_ThreshPrintInequalities( pGreaters, pSmallers, nChows, nInequalities);
//		printf( "\nInequalities was Siplified \n");
}
int Extra_ThreshAssignWeights(word * t, char * pIsop, char * pIsopFneg,
		int nVars, int * pW, int * pChow, int nChows, int Wmin) {

	int i = 0, j = 0, Lmin = 1000, Lmax = 0, Limit = nVars * 2, delta = 0,
			deltaOld = -1000, fIncremented = 0;
	//******************************
	int * pWofChow = ABC_ALLOC( int, nChows );
	int nCubesIsop = strlen(pIsop) / (nVars + 3);
	int nCubesIsopFneg = strlen(pIsopFneg) / (nVars + 3);
	int nInequalities = nCubesIsop * nCubesIsopFneg;
	unsigned long **pGreaters;
	unsigned long **pSmallers;

	pGreaters = (unsigned long **)malloc(nCubesIsop * nCubesIsopFneg * sizeof *pGreaters);
	for (i = 0; i < nCubesIsop * nCubesIsopFneg; i++) {
		pGreaters[i] = (unsigned long *)malloc(nChows * sizeof *pGreaters[i]);
	}
	pSmallers = (unsigned long **)malloc(nCubesIsop * nCubesIsopFneg * sizeof *pSmallers);
	for (i = 0; i < nCubesIsop * nCubesIsopFneg; i++) {
		pSmallers[i] = (unsigned long *)malloc(nChows * sizeof *pSmallers[i]);
	}

	//******************************
	Extra_ThreshCreateInequalities(pIsop, pIsopFneg, nVars, pWofChow, pChow,
			nChows, nInequalities, pGreaters, pSmallers);
	Extra_ThreshSimplifyInequalities(nInequalities, nChows, pGreaters,
			pSmallers);

	//initializes the weights
	pWofChow[0] = Wmin;
	for (i = 1; i < nChows; i++) {
		pWofChow[i] = pWofChow[i - 1] + 1;
	}
	i = 0;

	//assign the weights respecting the inequalities
	while (i < nChows && pWofChow[nChows - 1] <= Limit) {
		Lmin = 1000;
		Lmax = 0;

		while (j < nInequalities) {
			if (pGreaters[j][i] != 0) {
				Lmin = Extra_ThreshCubeWeightedSum3(pWofChow, nChows, pGreaters,
						j);
				Lmax = Extra_ThreshCubeWeightedSum4(pWofChow, nChows, pSmallers,
						j);
				delta = Lmin - Lmax;

				if (delta > 0) {
					if (fIncremented == 1) {
						j = 0;
						fIncremented = 0;
						deltaOld = -1000;
					} else
						j++;
					continue;
				}
				if (delta > deltaOld) {
					Extra_ThreshIncrementWeights(nChows, pWofChow, i);
					deltaOld = delta;
					fIncremented = 1;
				} else if (fIncremented == 1) {
					Extra_ThreshDecrementWeights(nChows, pWofChow, i);
					j++;
					deltaOld = -1000;
					fIncremented = 0;
					continue;
				} else
					j++;
			} else
				j++;

		}
		i++;
		j = 0;
	}

	//******************************
	for (i = 0; i < nCubesIsop * nCubesIsopFneg; i++) {
		free(pGreaters[i]);
	}
	free(pGreaters);
	for (i = 0; i < nCubesIsop * nCubesIsopFneg; i++) {
		free(pSmallers[i]);
	}
	free(pSmallers);
	//******************************

	i = 0;
	Lmin = 1000;
	Lmax = 0;

	//check the assigned weights in the original system
	for (j = 0; j < (int)strlen(pIsop); j += (nVars + 3))
		Lmin = Abc_MinInt(Lmin,
				Extra_ThreshCubeWeightedSum1(pWofChow, pChow, pIsop, nVars, j));
	for (j = 0; j < (int)strlen(pIsopFneg); j += (nVars + 3))
		Lmax = Abc_MaxInt(Lmax,
				Extra_ThreshCubeWeightedSum2(pWofChow, pChow, pIsopFneg, nVars,
						j));

	for (i = 0; i < nVars; i++) {
		pW[i] = pWofChow[pChow[i]];
	}

    ABC_FREE( pWofChow );
	if (Lmin > Lmax)
		return Lmin;
	else
		return 0;
}
void Extra_ThreshPrintWeights(int T, int * pW, int nVars) {
	int i;

	if (T == 0)
		fprintf( stdout, "\nHeuristic method: is not TLF\n\n");
	else {
		fprintf( stdout, "\nHeuristic method: Weights and threshold value:\n");
		for (i = 0; i < nVars; i++)
			printf("%d ", pW[i]);
		printf("  %d\n", T);
	}
}
/**Function*************************************************************


 Synopsis    [Checks thresholdness of the function.]

 Description []

 SideEffects []


 SeeAlso     []

 ***********************************************************************/
int Extra_ThreshCheck(word * t, int nVars, int * pW) {
	int Chow0, Chow[16];
	if (!Abc_TtIsUnate(t, nVars))
		return 0;
	Abc_TtMakePosUnate(t, nVars);
	Chow0 = Extra_ThreshComputeChow(t, nVars, Chow);
	Extra_ThreshSortByChow(t, nVars, Chow);
	return Extra_ThreshSelectWeights(t, nVars, pW);
}
/**Function*************************************************************


 Synopsis    [Checks thresholdness of the function by using a heuristic method.]

 Description []

 SideEffects []


 SeeAlso     []

 ***********************************************************************/
int Extra_ThreshHeuristic(word * t, int nVars, int * pW) {

	extern char * Abc_ConvertBddToSop( Mem_Flex_t * pMan, DdManager * dd, DdNode * bFuncOn, DdNode * bFuncOnDc, int nFanins, int fAllPrimes, Vec_Str_t * vCube, int fMode );
	int Chow0, Chow[16], nChows, i, T = 0;
    DdManager * dd;
    Vec_Str_t * vCube;
    DdNode * ddNode, * ddNodeFneg;
    char * pIsop, * pIsopFneg;
	if (nVars <= 1)
		return 1;
	if (!Abc_TtIsUnate(t, nVars))
		return 0;
	Abc_TtMakePosUnate(t, nVars);
	Chow0 = Extra_ThreshComputeChow(t, nVars, Chow);
	Extra_ThreshSortByChowInverted(t, nVars, Chow);
	nChows = Extra_ThreshInitializeChow(nVars, Chow);

	dd = (DdManager *) Abc_FrameReadManDd();
	vCube = Vec_StrAlloc(nVars);
	for (i = 0; i < nVars; i++)
		Cudd_bddIthVar(dd, i);
	ddNode = Kit_TruthToBdd(dd, (unsigned *) t, nVars, 0);
	Cudd_Ref(ddNode);
	pIsop = Abc_ConvertBddToSop( NULL, dd, ddNode, ddNode, nVars, 1,
			vCube, 1);

	Abc_TtNot(t, Abc_TruthWordNum(nVars));
	ddNodeFneg = Kit_TruthToBdd(dd, (unsigned *) t, nVars, 0);
	Cudd_Ref(ddNodeFneg);

	pIsopFneg = Abc_ConvertBddToSop( NULL, dd, ddNodeFneg, ddNodeFneg,
			nVars, 1, vCube, 1);

	Cudd_RecursiveDeref(dd, ddNode);
	Cudd_RecursiveDeref(dd, ddNodeFneg);

	T = Extra_ThreshAssignWeights(t, pIsop, pIsopFneg, nVars, pW, Chow, nChows,
			1);

	for (i = 2; (i < 4) && (T == 0) && (nVars >= 6); i++)
		T = Extra_ThreshAssignWeights(t, pIsop, pIsopFneg, nVars, pW, Chow,
				nChows, i);

	free(pIsop);
	free(pIsopFneg);
	Vec_StrFree(vCube);

	return T;
}

/**Function*************************************************************

 Synopsis    [Checks unateness of a function.]

 Description []

 SideEffects []

 SeeAlso     []

 ***********************************************************************/
void Extra_ThreshCheckTest() {
	int nVars = 6;
	int T, Chow0, Chow[16], Weights[16];
//    word t =  s_Truths6[0] & s_Truths6[1] & s_Truths6[2] & s_Truths6[3] & s_Truths6[4];
//    word t =  (s_Truths6[0] & s_Truths6[1]) | (s_Truths6[0] & s_Truths6[2] & s_Truths6[3]) | (s_Truths6[0] & s_Truths6[2] & s_Truths6[4]);
//	word t = (s_Truths6[2] & s_Truths6[1])
//			| (s_Truths6[2] & s_Truths6[0] & s_Truths6[3])
//			| (s_Truths6[2] & s_Truths6[0] & ~s_Truths6[4]);
	word t = (s_Truths6[0] & s_Truths6[1] & s_Truths6[2])| (s_Truths6[0] & s_Truths6[1] & s_Truths6[3]) | (s_Truths6[0] & s_Truths6[1] & s_Truths6[4] & s_Truths6[5]) | (s_Truths6[0] & s_Truths6[2] & s_Truths6[3] & s_Truths6[4] & s_Truths6[5]);
//    word t =  (s_Truths6[0] & s_Truths6[1]) | (s_Truths6[0] & s_Truths6[2] & s_Truths6[3]) | (s_Truths6[0] & s_Truths6[2] & s_Truths6[4]) | (s_Truths6[1] & s_Truths6[2] & s_Truths6[3]);
//    word t =  (s_Truths6[0] & s_Truths6[1]) | (s_Truths6[0] & s_Truths6[2]) | (s_Truths6[0] & s_Truths6[3] & s_Truths6[4] & s_Truths6[5]) | 
//        (s_Truths6[1] & s_Truths6[2] & s_Truths6[3]) | (s_Truths6[1] & s_Truths6[2] & s_Truths6[4]) | (s_Truths6[1] & s_Truths6[2] & s_Truths6[5]);
	int i;
	assert(nVars <= 8);
	for (i = 0; i < nVars; i++)
		printf("%d %d %d\n", i, Abc_TtPosVar(&t, nVars, i),
				Abc_TtNegVar(&t, nVars, i));
//    word t = s_Truths6[0] & s_Truths6[1] & s_Truths6[2];
	Chow0 = Extra_ThreshComputeChow(&t, nVars, Chow);
	if ((T = Extra_ThreshCheck(&t, nVars, Weights)))
		Extra_ThreshPrintChow(T, Weights, nVars);
	else
		printf("No threshold\n");
}
void Extra_ThreshHeuristicTest() {
	int nVars = 6;
	int T, Weights[16];

	//	word t = 19983902376700760000;
	word t = (s_Truths6[0] & s_Truths6[1] & s_Truths6[2])| (s_Truths6[0] & s_Truths6[1] & s_Truths6[3]) | (s_Truths6[0] & s_Truths6[1] & s_Truths6[4] & s_Truths6[5]) | (s_Truths6[0] & s_Truths6[2] & s_Truths6[3] & s_Truths6[4] & s_Truths6[5]);
	word * pT = &t;
	T = Extra_ThreshHeuristic(pT, nVars, Weights);
	Extra_ThreshPrintWeights(T, Weights, nVars);

}
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

