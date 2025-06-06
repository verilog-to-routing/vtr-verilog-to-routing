/**CFile****************************************************************

  FileName    [rewire_map.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Re-wiring.]

  Synopsis    []

  Author      [Jiun-Hao Chen]
  
  Affiliation [National Taiwan University]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rewire_map.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef REWIRE_MAP_H
#define REWIRE_MAP_H

#include "base/abc/abc.h"
#include "aig/gia/giaAig.h"
#include "map/amap/amap.h"
#include "map/mio/mio.h"
#include "aig/miniaig/miniaig.h"

ABC_NAMESPACE_HEADER_START

Abc_Ntk_t *Gia_ManRewirePut(Gia_Man_t *pGia);
Abc_Ntk_t *Abc_ManRewireMapAmap(Abc_Ntk_t *pNtk);
Abc_Ntk_t *Gia_ManRewireMapNf(Gia_Man_t *pGia);
Abc_Ntk_t *Gia_ManRewireMapSimap(Gia_Man_t *pGia, int nBound, int nBTLimit, int nTimeout);
Vec_Int_t *Abc_ManRewireNtkWriteMiniMapping(Abc_Ntk_t *pNtk);
Abc_Ntk_t *Abc_ManRewireNtkFromMiniMapping(int *vMapping);
Mini_Aig_t *Abc_ManRewireMiniAigFromNtk(Abc_Ntk_t *pNtk);

ABC_NAMESPACE_HEADER_END

#endif // REWIRE_MAP_H