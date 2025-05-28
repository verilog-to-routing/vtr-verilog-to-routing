#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

void Internal::recompute_tier () {
  if (!opts.recomputetier)
    return;

  ++stats.tierecomputed;
  const int64_t delta =
      stats.tierecomputed >= 16 ? 1u << 16 : (1u << stats.tierecomputed);
  lim.recompute_tier = stats.conflicts + delta;
  LOG ("rescheduling in %zd at %zd (conflicts at %zd)", delta,
       lim.recompute_tier, stats.conflicts);
#ifndef CADICAL_NDEBUG
  uint64_t total_used = 0;
  for (auto u : stats.used[stable])
    total_used += u;
  CADICAL_assert (total_used == stats.bump_used[stable]);
#endif

  if (!stats.bump_used[stable]) {
    tier1[stable] = opts.reducetier1glue;
    tier2[stable] = opts.reducetier2glue;
    LOG ("tier1 limit = %d", tier1[stable]);
    LOG ("tier2 limit = %d", tier2[stable]);
    return;
  } else {
    uint64_t accumulated_tier1_limit =
        stats.bump_used[stable] * opts.tier1limit / 100;
    uint64_t accumulated_tier2_limit =
        stats.bump_used[stable] * opts.tier2limit / 100;
    uint64_t accumulated_used = 0;
    for (size_t glue = 0; glue < stats.used[stable].size (); ++glue) {
      const uint64_t u = stats.used[stable][glue];
      accumulated_used += u;
      if (accumulated_used <= accumulated_tier1_limit) {
        tier1[stable] = glue;
      }
      if (accumulated_used >= accumulated_tier2_limit) {
        tier2[stable] = glue;
        break;
      }
    }
  }

  LOG ("tier1 limit = %d in %s mode", tier1[stable],
       stable ? "stable" : "focused");
  LOG ("tier2 limit = %d in %s mode", tier2[stable],
       stable ? "stable" : "focused");
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
