#include "tiers.h"
#include "internal.h"
#include "logging.h"
#include "print.h"

ABC_NAMESPACE_IMPL_START

static void compute_tier_limits (kissat *solver, bool stable,
                                 unsigned *tier1_ptr, unsigned *tier2_ptr) {
  statistics *statistics = &solver->statistics_;
  uint64_t *used_stats = statistics->used[stable].glue;
  uint64_t total_used = 0;
  for (unsigned glue = 0; glue <= MAX_GLUE_USED; glue++)
    total_used += used_stats[glue];
  int tier1 = -1, tier2 = -1;
  if (total_used) {
    uint64_t accumulated_tier1_limit = total_used * TIER1RELATIVE;
    uint64_t accumulated_tier2_limit = total_used * TIER2RELATIVE;
    uint64_t accumulated_used = 0;
    unsigned glue;
    for (glue = 0; glue <= MAX_GLUE_USED; glue++) {
      uint64_t glue_used = used_stats[glue];
      accumulated_used += glue_used;
      if (accumulated_used >= accumulated_tier1_limit) {
        tier1 = glue;
        break;
      }
    }
    if (accumulated_used < accumulated_tier2_limit) {
      for (glue = tier1 + 1; glue <= MAX_GLUE_USED; glue++) {
        uint64_t glue_used = used_stats[glue];
        accumulated_used += glue_used;
        if (accumulated_used >= accumulated_tier2_limit) {
          tier2 = glue;
          break;
        }
      }
    }
  }
  if (tier1 < 0) {
    tier1 = GET_OPTION (tier1);
    tier2 = MAX (GET_OPTION (tier2), tier1);
  } else if (tier2 < 0)
    tier2 = tier1;
  KISSAT_assert (0 <= tier1);
  KISSAT_assert (0 <= tier2);
  *tier1_ptr = tier1;
  *tier2_ptr = tier2;
  LOG ("%s tier1 limit %u", stable ? "stable" : "focused", tier1);
  LOG ("%s tier2 limit %u", stable ? "stable" : "focused", tier2);
}

void kissat_compute_and_set_tier_limits (struct kissat *solver) {
  bool stable = solver->stable;
  unsigned tier1, tier2;
  compute_tier_limits (solver, stable, &tier1, &tier2);
  solver->tier1[stable] = tier1;
  solver->tier2[stable] = tier2;
  kissat_phase (solver, "retiered", GET (retiered),
                "recomputed %s tier1 limit %u and tier2 limit %u "
                "after %" PRIu64 " conflicts",
                stable ? "stable" : "focused", tier1, tier2, CONFLICTS);
}

static unsigned decimal_digits (uint64_t i) {
  unsigned res = 1;
  uint64_t limit = 10;
  for (;;) {
    if (i < limit)
      return res;
    limit *= 10;
    res++;
  }
}

void kissat_print_tier_usage_statistics (kissat *solver, bool stable) {
  unsigned tier1, tier2;
  compute_tier_limits (solver, stable, &tier1, &tier2);
  statistics *statistics = &solver->statistics_;
  uint64_t *used_stats = statistics->used[stable].glue;
  uint64_t total_used = 0;
  for (unsigned glue = 0; glue <= MAX_GLUE_USED; glue++)
    total_used += used_stats[glue];
  const char *mode = stable ? "stable" : "focused";
  KISSAT_assert (tier1 <= tier2);
  unsigned span = tier2 - tier1 + 1;
  const unsigned max_printed = 5;
  KISSAT_assert (max_printed & 1), KISSAT_assert (max_printed / 2 > 0);
  unsigned prefix, suffix;
  if (span > max_printed) {
    prefix = tier1 + max_printed / 2 - 1;
    suffix = tier2 - max_printed / 2 + 1;
  } else
    prefix = UINT_MAX, suffix = 0;
  uint64_t accumulated_middle = 0;
  int glue_digits = 1, clauses_digits = 1;
  for (unsigned glue = 0; glue <= MAX_GLUE_USED; glue++) {
    if (glue < tier1)
      continue;
    uint64_t used = used_stats[glue];
    int tmp_glue = 0, tmp_clauses = 0;
    if (glue <= prefix || suffix <= glue) {
      tmp_glue = decimal_digits (glue);
      tmp_clauses = decimal_digits (used);
    } else {
      accumulated_middle += used;
      if (glue + 1 == suffix) {
        tmp_glue = decimal_digits (prefix + 1) + decimal_digits (glue) + 1;
        tmp_clauses = decimal_digits (accumulated_middle);
      }
    }
    if (tmp_glue > glue_digits)
      glue_digits = tmp_glue;
    if (tmp_clauses > clauses_digits)
      clauses_digits = tmp_clauses;
    if (glue == tier2)
      break;
  }
  char fmt[32];
  sprintf (fmt, "%%%d" PRIu64, clauses_digits);
  accumulated_middle = 0;
  uint64_t accumulated = 0;
  for (unsigned glue = 0; glue <= MAX_GLUE_USED; glue++) {
    uint64_t used = used_stats[glue];
    accumulated += used;
    if (glue < tier1)
      continue;
    if (glue <= prefix || suffix <= glue + 1) {
      fputs (solver->prefix, stdout);
      fputs (mode, stdout);
      fputs (" glue ", stdout);
    }
    if (glue <= prefix || suffix <= glue) {
      int len = printf ("%u", glue);
      while (len > 0 && len < glue_digits)
        fputc (' ', stdout), len++;
      fputs (" used ", stdout);
      printf (fmt, used);
      printf (" clauses %5.2f%% accumulated %5.2f%%",
              kissat_percent (used, total_used),
              kissat_percent (accumulated, total_used));
      if (glue == tier1)
        fputs (" tier1", stdout);
      if (glue == tier2)
        fputs (" tier2", stdout);
      fputc ('\n', stdout);
    } else {
      accumulated_middle += used;
      if (glue + 1 == suffix) {
        int len = printf ("%u-%u", prefix + 1, suffix - 1);
        while (len > 0 && len < glue_digits)
          fputc (' ', stdout), len++;
        fputs (" used ", stdout);
        printf (fmt, accumulated_middle);
        printf (" clauses %5.2f%% accumulated %5.2f%%\n",
                kissat_percent (accumulated_middle, total_used),
                kissat_percent (accumulated, total_used));
      }
    }
    if (glue == tier2)
      break;
  }
}

ABC_NAMESPACE_IMPL_END
