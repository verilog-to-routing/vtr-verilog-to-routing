/**CFile****************************************************************

  FileName    [rewire_rng.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Re-wiring.]

  Synopsis    []

  Author      [Jiun-Hao Chen]
  
  Affiliation [National Taiwan University]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rewire_rng.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rewire_rng.h"

ABC_NAMESPACE_IMPL_START

unsigned Random_Int(int fReset) {
    static unsigned int m_z = NUMBER1;
    static unsigned int m_w = NUMBER2;
    if (fReset) {
        m_z = NUMBER1;
        m_w = NUMBER2;
    } 
    m_z = 36969 * (m_z & 65535) + (m_z >> 16);
    m_w = 18000 * (m_w & 65535) + (m_w >> 16);
    return (m_z << 16) + m_w;
}

word Random_Word(int fReset) {
    return ((word)Random_Int(fReset) << 32) | ((word)Random_Int(fReset) << 0);
}

// This procedure should be called once with Seed > 0 to initialize the generator.
// After initialization, the generator should be always called with Seed == 0.
unsigned Random_Num(int Seed) {
    static unsigned RandMask = 0;
    if (Seed == 0)
        return RandMask ^ Random_Int(0);
    RandMask = Random_Int(1);
    for (int i = 0; i < Seed; i++)
        RandMask = Random_Int(0);
    return RandMask;
}

ABC_NAMESPACE_IMPL_END