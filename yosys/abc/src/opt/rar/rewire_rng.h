/**CFile****************************************************************

  FileName    [rewire_rng.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Re-wiring.]

  Synopsis    []

  Author      [Jiun-Hao Chen]
  
  Affiliation [National Taiwan University]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rewire_rng.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef RAR_RNG_H
#define RAR_RNG_H

/*************************************************************
                 random number generation
**************************************************************/

#include "base/abc/abc.h"

// Creates a sequence of random numbers.
// http://www.codeproject.com/KB/recipes/SimpleRNG.aspx

ABC_NAMESPACE_HEADER_START

#define NUMBER1 3716960521u
#define NUMBER2 2174103536u

unsigned Random_Int(int fReset);

word Random_Word(int fReset);

// This procedure should be called once with Seed > 0 to initialize the generator.
// After initialization, the generator should be always called with Seed == 0.
unsigned Random_Num(int Seed);

ABC_NAMESPACE_HEADER_END

#endif // RAR_RNG_H