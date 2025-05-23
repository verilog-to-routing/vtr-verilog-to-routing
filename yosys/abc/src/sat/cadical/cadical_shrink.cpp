#include "global.h"

#include "internal.hpp"
#include "reap.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

void Internal::reset_shrinkable () {
#ifdef LOGGING
  size_t reset = 0;
#endif
  for (const auto &lit : shrinkable) {
    LOG ("resetting lit %i", lit);
    Flags &f = flags (lit);
    CADICAL_assert (f.shrinkable);
    f.shrinkable = false;
#ifdef LOGGING
    ++reset;
#endif
  }
  LOG ("resetting %zu shrinkable variables", reset);
}

void Internal::mark_shrinkable_as_removable (
    int blevel, std::vector<int>::size_type minimized_start) {
#ifdef LOGGING
  size_t marked = 0, reset = 0;
#endif
#ifndef CADICAL_NDEBUG
  unsigned kept = 0, minireset = 0;
  for (; minimized_start < minimized.size (); ++minimized_start) {
    const int lit = minimized[minimized_start];
    Flags &f = flags (lit);
    const Var &v = var (lit);
    if (v.level == blevel) {
      CADICAL_assert (!f.poison);
      ++minireset;
    } else
      ++kept;
  }
  (void) kept;
  (void) minireset;
#else
  (void) blevel;
  (void) minimized_start;
#endif

  for (const int lit : shrinkable) {
    Flags &f = flags (lit);
    CADICAL_assert (f.shrinkable);
    CADICAL_assert (!f.poison);
    f.shrinkable = false;
#ifdef LOGGING
    ++reset;
#endif
    if (f.removable)
      continue;
    f.removable = true;
    minimized.push_back (lit);
#ifdef LOGGING
    ++marked;
#endif
  }
  LOG ("resetting %zu shrinkable variables", reset);
  LOG ("marked %zu removable variables", marked);
}

int inline Internal::shrink_literal (int lit, int blevel,
                                     unsigned max_trail) {
  CADICAL_assert (val (lit) < 0);

  Flags &f = flags (lit);
  Var &v = var (lit);
  CADICAL_assert (v.level <= blevel);

  if (!v.level) {
    LOG ("skipping root level assigned %d", (lit));
    return 0;
  }

  if (v.reason == external_reason) {
    CADICAL_assert (!opts.exteagerreasons);
    v.reason = learn_external_reason_clause (-lit, 0, true);
    if (!v.reason) {
      CADICAL_assert (!v.level);
      return 0;
    }
  }
  CADICAL_assert (v.reason != external_reason);
  if (f.shrinkable) {
    LOG ("skipping already shrinkable literal %d", (lit));
    return 0;
  }

  if (v.level < blevel) {
    if (f.removable) {
      LOG ("skipping removable thus shrinkable %d", (lit));
      return 0;
    }
    const bool always_minimize_on_lower_blevel = (opts.shrink > 2);
    if (always_minimize_on_lower_blevel && minimize_literal (-lit, 1)) {
      LOG ("minimized thus shrinkable %d", (lit));
      return 0;
    }
    LOG ("literal %d on lower blevel %u < %u not removable/shrinkable",
         (lit), v.level, blevel);
    return -1;
  }

  LOG ("marking %d as shrinkable", lit);
  f.shrinkable = true;
  f.poison = false;
  shrinkable.push_back (lit);
  if (opts.shrinkreap) {
    CADICAL_assert (max_trail < trail.size ());
    const unsigned dist = max_trail - v.trail;
    reap.push (dist);
  }
  return 1;
}

unsigned Internal::shrunken_block_uip (
    int uip, int blevel, std::vector<int>::reverse_iterator &rbegin_block,
    std::vector<int>::reverse_iterator &rend_block,
    std::vector<int>::size_type minimized_start, const int uip0) {
  CADICAL_assert (clause[0] == uip0);

  LOG ("UIP on level %u, uip: %i (replacing by %i)", blevel, uip, uip0);
  CADICAL_assert (rend_block > rbegin_block);
  CADICAL_assert (rend_block < clause.rend ());
  unsigned block_shrunken = 0;
  *rbegin_block = -uip;
  Var &v = var (-uip);
  Level &l = control[v.level];
  l.seen.trail = v.trail;
  l.seen.count = 1;

  Flags &f = flags (-uip);
  if (!f.seen) {
    analyzed.push_back (-uip);
    f.seen = true;
  }

  flags (-uip).keep = true;
  for (auto p = rbegin_block + 1; p != rend_block; ++p) {
    const int lit = *p;
    if (lit == -uip0)
      continue;
    *p = uip0;
    // if (lit == -uip) continue;
    ++block_shrunken;
    CADICAL_assert (clause[0] == uip0);
  }
  mark_shrinkable_as_removable (blevel, minimized_start);
  CADICAL_assert (clause[0] == uip0);
  return block_shrunken;
}

void inline Internal::shrunken_block_no_uip (
    const std::vector<int>::reverse_iterator &rbegin_block,
    const std::vector<int>::reverse_iterator &rend_block,
    unsigned &block_minimized, const int uip0) {
  STOP (shrink);
  START (minimize);
  CADICAL_assert (rend_block > rbegin_block);
  LOG ("no UIP found, now minimizing");
  for (auto p = rbegin_block; p != rend_block; ++p) {
    CADICAL_assert (p != clause.rend () - 1);
    const int lit = *p;
    if (opts.minimize && minimize_literal (-lit)) {
      CADICAL_assert (!flags (lit).keep);
      ++block_minimized;
      *p = uip0;
    } else {
      flags (lit).keep = true;
      CADICAL_assert (flags (lit).keep);
    }
  }
  STOP (minimize);
  START (shrink);
}

void Internal::push_literals_of_block (
    const std::vector<int>::reverse_iterator &rbegin_block,
    const std::vector<int>::reverse_iterator &rend_block, int blevel,
    unsigned max_trail) {
  CADICAL_assert (rbegin_block < rend_block);
  for (auto p = rbegin_block; p != rend_block; ++p) {
    CADICAL_assert (p != clause.rend () - 1);
    CADICAL_assert (!flags (*p).keep);
    const int lit = *p;
    LOG ("pushing lit %i of blevel %i", lit, var (lit).level);
#ifndef CADICAL_NDEBUG
    int tmp =
#endif
        shrink_literal (lit, blevel, max_trail);
    CADICAL_assert (tmp > 0);
  }
}

unsigned inline Internal::shrink_next (int blevel, unsigned &open,
                                       unsigned &max_trail) {
  const auto &t = &trail;
  if (opts.shrinkreap) {
    CADICAL_assert (!reap.empty ());
    const unsigned dist = reap.pop ();
    --open;
    CADICAL_assert (dist <= max_trail);
    const unsigned pos = max_trail - dist;
    CADICAL_assert (pos < t->size ());
    const int uip = (*t)[pos];
    CADICAL_assert (val (uip) > 0);
    LOG ("trying to shrink literal %d at trail[%u] and level %d", uip, pos,
         blevel);
    return uip;
  } else {
    int uip;
#ifndef CADICAL_NDEBUG
    unsigned init_max_trail = max_trail;
#endif
    do {
      CADICAL_assert (max_trail <= init_max_trail);
      uip = (*t)[max_trail--];
    } while (!flags (uip).shrinkable);
    --open;
    LOG ("open is now %d, uip = %d, level %d", open, uip, blevel);
    return uip;
  }
  (void) blevel;
}

unsigned inline Internal::shrink_along_reason (int uip, int blevel,
                                               bool resolve_large_clauses,
                                               bool &failed_ptr,
                                               unsigned max_trail) {
  LOG ("shrinking along the reason of lit %i", uip);
  unsigned open = 0;
#ifndef CADICAL_NDEBUG
  const Flags &f = flags (uip);
#endif
  const Var &v = var (uip);

  CADICAL_assert (f.shrinkable);
  CADICAL_assert (v.level == blevel);
  CADICAL_assert (v.reason);

  if (opts.minimizeticks)
    stats.ticks.search[stable]++;

  if (resolve_large_clauses || v.reason->size == 2) {
    const Clause &c = *v.reason;
    LOG (v.reason, "resolving with reason");
    for (int lit : c) {
      if (lit == uip)
        continue;
      CADICAL_assert (val (lit) < 0);
      int tmp = shrink_literal (lit, blevel, max_trail);
      if (tmp < 0) {
        failed_ptr = true;
        break;
      }
      if (tmp > 0) {
        ++open;
      }
    }
  } else {
    failed_ptr = true;
  }
  return open;
}

unsigned
Internal::shrink_block (std::vector<int>::reverse_iterator &rbegin_lits,
                        std::vector<int>::reverse_iterator &rend_block,
                        int blevel, unsigned &open,
                        unsigned &block_minimized, const int uip0,
                        unsigned max_trail) {
  CADICAL_assert (shrinkable.empty ());
  CADICAL_assert (blevel <= this->level);
  CADICAL_assert (open < clause.size ());
  CADICAL_assert (rbegin_lits >= clause.rbegin ());
  CADICAL_assert (rend_block < clause.rend ());
  CADICAL_assert (rbegin_lits < rend_block);
  CADICAL_assert (opts.shrink);

#ifdef LOGGING

  LOG ("trying to shrink %u literals on level %u", open, blevel);

  const auto &t = &trail;

  LOG ("maximum trail position %zd on level %u", t->size (), blevel);
  if (opts.shrinkreap)
    LOG ("shrinking up to %u", max_trail);
#endif

  const bool resolve_large_clauses = (opts.shrink > 1);
  bool failed = false;
  unsigned block_shrunken = 0;
  std::vector<int>::size_type minimized_start = minimized.size ();
  int uip = uip0;
  unsigned max_trail2 = max_trail;

  if (!failed) {
    push_literals_of_block (rbegin_lits, rend_block, blevel, max_trail);
    CADICAL_assert (!opts.shrinkreap || reap.size () == open);

    CADICAL_assert (open > 0);
    while (!failed) {
      CADICAL_assert (!opts.shrinkreap || reap.size () == open);
      uip = shrink_next (blevel, open, max_trail);
      if (open == 0) {
        break;
      }
      open += shrink_along_reason (uip, blevel, resolve_large_clauses,
                                   failed, max_trail2);
      CADICAL_assert (open >= 1);
    }

    if (!failed)
      LOG ("shrinking found UIP %i on level %i (open: %d)", uip, blevel,
           open);
    else
      LOG ("shrinking failed on level %i", blevel);
  }

  if (failed)
    reset_shrinkable (), shrunken_block_no_uip (rbegin_lits, rend_block,
                                                block_minimized, uip0);
  else
    block_shrunken = shrunken_block_uip (uip, blevel, rbegin_lits,
                                         rend_block, minimized_start, uip0);

  if (opts.shrinkreap)
    reap.clear ();
  shrinkable.clear ();
  return block_shrunken;
}

// Smaller level and trail.  Comparing literals on their level is necessary
// for chronological backtracking, since trail order might in this case not
// respect level order.

struct shrink_trail_negative_rank {
  Internal *internal;
  shrink_trail_negative_rank (Internal *s) : internal (s) {}
  typedef uint64_t Type;
  Type operator() (int a) {
    Var &v = internal->var (a);
    uint64_t res = v.level;
    res <<= 32;
    res |= v.trail;
    return ~res;
  }
};

struct shrink_trail_larger {
  Internal *internal;
  shrink_trail_larger (Internal *s) : internal (s) {}
  bool operator() (const int &a, const int &b) const {
    return shrink_trail_negative_rank (internal) (a) <
           shrink_trail_negative_rank (internal) (b);
  }
};

// Finds the beginning of the block (rend_block, non-included) ending at
// rend_block (included). Then tries to shrinks and minimizes literals  the
// block
std::vector<int>::reverse_iterator Internal::minimize_and_shrink_block (
    std::vector<int>::reverse_iterator &rbegin_block,
    unsigned &total_shrunken, unsigned &total_minimized, const int uip0)

{
  LOG ("shrinking block");
  CADICAL_assert (rbegin_block < clause.rend () - 1);
  int blevel;
  unsigned open = 0;
  unsigned max_trail;

  // find begining of block;
  std::vector<int>::reverse_iterator rend_block;
  {
    CADICAL_assert (rbegin_block <= clause.rend ());
    const int lit = *rbegin_block;
    const int idx = vidx (lit);
    blevel = vtab[idx].level;
    max_trail = vtab[idx].trail;
    LOG ("Block at level %i (first lit: %i)", blevel, lit);

    rend_block = rbegin_block;
    bool finished;
    do {
      CADICAL_assert (rend_block < clause.rend () - 1);
      const int lit = *(++rend_block);
      const int idx = vidx (lit);
      finished = (blevel != vtab[idx].level);
      if (!finished && (unsigned) vtab[idx].trail > max_trail)
        max_trail = vtab[idx].trail;
      ++open;
      LOG (
          "testing if lit %i is on the same level (of lit: %i, global: %i)",
          lit, vtab[idx].level, blevel);

    } while (!finished);
  }
  CADICAL_assert (open > 0);
  CADICAL_assert (open < clause.size ());
  CADICAL_assert (rbegin_block < clause.rend ());
  CADICAL_assert (rend_block < clause.rend ());

  unsigned block_shrunken = 0, block_minimized = 0;
  if (open < 2) {
    flags (*rbegin_block).keep = true;
    minimized.push_back (*rbegin_block);
  } else
    block_shrunken = shrink_block (rbegin_block, rend_block, blevel, open,
                                   block_minimized, uip0, max_trail);

  LOG ("shrunken %u literals on level %u (including %u minimized)",
       block_shrunken, blevel, block_minimized);

  total_shrunken += block_shrunken;
  total_minimized += block_minimized;

  return rend_block;
}

void Internal::shrink_and_minimize_clause () {
  CADICAL_assert (opts.minimize || opts.shrink > 0);
  LOG (clause, "shrink first UIP clause");

  START (shrink);
  external->check_learned_clause (); // check 1st UIP learned clause first
  MSORT (opts.radixsortlim, clause.begin (), clause.end (),
         shrink_trail_negative_rank (this), shrink_trail_larger (this));
  unsigned total_shrunken = 0;
  unsigned total_minimized = 0;

  LOG (clause, "shrink first UIP clause (CADICAL_asserting lit: %i)", clause[0]);

  auto rend_lits = clause.rend () - 1;
  auto rend_block = clause.rbegin ();
  const int uip0 = clause[0];

  // for direct LRAT we remember how the clause used to look
  vector<int> old_clause_lrat;
  CADICAL_assert (minimize_chain.empty ());
  if (lrat)
    for (auto &i : clause)
      old_clause_lrat.push_back (i);

  while (rend_block != rend_lits) {
    rend_block = minimize_and_shrink_block (rend_block, total_shrunken,
                                            total_minimized, uip0);
  }

  LOG (clause,
       "post shrink pass (with uips, not removed) first UIP clause");
  LOG (old_clause_lrat, "(used for lratdirect) before shrink: clause");
#if defined(LOGGING) || !defined(CADICAL_NDEBUG)
  const unsigned old_size = clause.size ();
#endif
  std::vector<int> stack;
  {
    std::vector<int>::size_type i = 1;
    for (std::vector<int>::size_type j = 1; j < clause.size (); ++j) {
      CADICAL_assert (i <= j);
      clause[i] = clause[j];
      if (lrat) {
        CADICAL_assert (j < old_clause_lrat.size ());
        CADICAL_assert (mini_chain.empty ());
        if (clause[j] != old_clause_lrat[j]) {
          calculate_minimize_chain (-old_clause_lrat[j], stack);
          for (auto p : mini_chain) {
            minimize_chain.push_back (p);
          }
          mini_chain.clear ();
        }
      }
      if (clause[j] == uip0) {
        continue;
      }
      CADICAL_assert (flags (clause[i]).keep);
      ++i;
      LOG ("keeping literal %i", clause[j]);
    }
    clause.resize (i);
  }
  CADICAL_assert (old_size ==
          (unsigned) clause.size () + total_shrunken + total_minimized);
  LOG (clause, "after shrinking first UIP clause");
  LOG ("clause shrunken by %zd literals (including %u minimized)",
       old_size - clause.size (), total_minimized);

  stats.shrunken += total_shrunken;
  stats.minishrunken += total_minimized;
  STOP (shrink);

  START (minimize);
  clear_minimized_literals ();
  for (auto p = minimize_chain.rbegin (); p != minimize_chain.rend ();
       p++) {
    lrat_chain.push_back (*p);
  }
  minimize_chain.clear ();
  STOP (minimize);
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
