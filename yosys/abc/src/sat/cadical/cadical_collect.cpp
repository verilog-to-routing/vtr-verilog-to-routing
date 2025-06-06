#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Returns the positive number '1' ( > 0) if the given clause is root level
// satisfied or the negative number '-1' ( < 0) if it is not root level
// satisfied but contains a root level falsified literal. Otherwise, if it
// contains neither a satisfied nor falsified literal, then '0' is returned.

int Internal::clause_contains_fixed_literal (Clause *c) {
  int satisfied = 0, falsified = 0;
  for (const auto &lit : *c) {
    const int tmp = fixed (lit);
    if (tmp > 0) {
      LOG (c, "root level satisfied literal %d in", lit);
      satisfied++;
    }
    if (tmp < 0) {
      LOG (c, "root level falsified literal %d in", lit);
      falsified++;
    }
  }
  if (satisfied)
    return 1;
  else if (falsified)
    return -1;
  else
    return 0;
}

// Assume that the clause is not root level satisfied but contains a literal
// set to false (root level falsified literal), so it can be shrunken.  The
// clause data is not actually reallocated at this point to avoid dealing
// with issues of special policies for watching binary clauses or whether a
// clause is extended or not. Only its size field is adjusted accordingly
// after flushing out root level falsified literals.

void Internal::remove_falsified_literals (Clause *c) {
  const const_literal_iterator end = c->end ();
  const_literal_iterator i;
  int num_non_false = 0;
  for (i = c->begin (); num_non_false < 2 && i != end; i++)
    if (fixed (*i) >= 0)
      num_non_false++;
  if (num_non_false < 2)
    return;
  if (proof) {
    // Flush changes the clause id, external forgettables need to be
    // marked here (or the new id could be used instead of old one)
    if (opts.check && is_external_forgettable (c->id))
      mark_garbage_external_forgettable (c->id);
    proof->flush_clause (c);
  }
  literal_iterator j = c->begin ();
  for (i = j; i != end; i++) {
    const int lit = *j++ = *i, tmp = fixed (lit);
    CADICAL_assert (tmp <= 0);
    if (tmp >= 0)
      continue;
    LOG ("flushing %d", lit);
    j--;
  }
  stats.collected += shrink_clause (c, j - c->begin ());
}

// If there are new units (fixed variables) since the last garbage
// collection we go over all clauses, mark satisfied ones as garbage and
// flush falsified literals.  Otherwise if no new units have been generated
// since the last garbage collection just skip this step.

void Internal::mark_satisfied_clauses_as_garbage () {

  if (last.collect.fixed >= stats.all.fixed)
    return;
  last.collect.fixed = stats.all.fixed;

  LOG ("marking satisfied clauses and removing falsified literals");

  for (const auto &c : clauses) {
    if (c->garbage)
      continue;
    const int tmp = clause_contains_fixed_literal (c);
    if (tmp > 0)
      mark_garbage (c);
    else if (tmp < 0)
      remove_falsified_literals (c);
  }
}

/*------------------------------------------------------------------------*/

// Reason clauses can not be collected.
//
// We protect reasons before and release protection after garbage collection
// (actually within garbage collection).
//
// For 'reduce' we still need to make sure that all clauses which should not
// be removed are marked as such and thus we need to call it before marking
// clauses to be flushed.

void Internal::protect_reasons () {
  LOG ("protecting reason clauses of all assigned variables on trail");
  CADICAL_assert (!protected_reasons);
#ifdef LOGGING
  size_t count = 0;
#endif
  for (const auto &lit : trail) {
    if (!active (lit))
      continue;
    CADICAL_assert (val (lit));
    Var &v = var (lit);
    CADICAL_assert (v.level > 0);
    Clause *reason = v.reason;
    if (!reason)
      continue;
    if (reason == external_reason)
      continue;
    LOG (reason, "protecting assigned %d reason %p", lit, (void *) reason);
    CADICAL_assert (!reason->reason);
    reason->reason = true;
#ifdef LOGGING
    count++;
#endif
  }
  LOG ("protected %zd reason clauses referenced on trail", count);
  protected_reasons = true;
}

/*------------------------------------------------------------------------*/

// After garbage collection we reset the 'reason' flag of the reasons
// of assigned literals on the trail.

void Internal::unprotect_reasons () {
  LOG ("unprotecting reasons clauses of all assigned variables on trail");
  CADICAL_assert (protected_reasons);
#ifdef LOGGING
  size_t count = 0;
#endif
  for (const auto &lit : trail) {
    if (!active (lit))
      continue;
    CADICAL_assert (val (lit));
    Var &v = var (lit);
    CADICAL_assert (v.level > 0);
    Clause *reason = v.reason;
    if (!reason)
      continue;
    if (reason == external_reason)
      continue;
    LOG (reason, "unprotecting assigned %d reason %p", lit,
         (void *) reason);
    CADICAL_assert (reason->reason);
    reason->reason = false;
#ifdef LOGGING
    count++;
#endif
  }
  LOG ("unprotected %zd reason clauses referenced on trail", count);
  protected_reasons = false;
}

/*------------------------------------------------------------------------*/

// Update occurrence lists before deleting garbage clauses in the context of
// preprocessing, e.g., during bounded variable elimination 'elim'.  The
// result is the number of remaining clauses, which in this context means
// the number of non-garbage clauses.

size_t Internal::flush_occs (int lit) {
  Occs &os = occs (lit);
  const const_occs_iterator end = os.end ();
  occs_iterator j = os.begin ();
  const_occs_iterator i;
  size_t res = 0;
  Clause *c;
  for (i = j; i != end; i++) {
    c = *i;
    if (c->collect ())
      continue;
    *j++ = c->moved ? c->copy : c;
    // CADICAL_assert (!c->redundant); // -> not true in sweeping
    res++;
  }
  os.resize (j - os.begin ());
  shrink_occs (os);
  return res;
}

// Update watch lists before deleting garbage clauses in the context of
// 'reduce' where we watch and no occurrence lists.  We have to protect
// reason clauses not be collected and thus we have this additional check
// hidden in 'Clause.collect', which for the root level context of
// preprocessing is actually redundant.

inline void Internal::flush_watches (int lit, Watches &saved) {
  CADICAL_assert (saved.empty ());
  Watches &ws = watches (lit);
  const const_watch_iterator end = ws.end ();
  watch_iterator j = ws.begin ();
  const_watch_iterator i;
  for (i = j; i != end; i++) {
    Watch w = *i;
    Clause *c = w.clause;
    if (c->collect ())
      continue;
    if (c->moved)
      c = w.clause = c->copy;
    w.size = c->size;
    const int new_blit_pos = (c->literals[0] == lit);
    LOG (c, "clause in flush_watch starting from %d", lit);
    CADICAL_assert (c->literals[!new_blit_pos] == lit); /*FW1*/
    w.blit = c->literals[new_blit_pos];
    if (w.binary ())
      *j++ = w;
    else
      saved.push_back (w);
  }
  ws.resize (j - ws.begin ());
  for (const auto &w : saved)
    ws.push_back (w);
  saved.clear ();
  shrink_vector (ws);
}

void Internal::flush_all_occs_and_watches () {
  if (occurring ())
    for (auto idx : vars)
      flush_occs (idx), flush_occs (-idx);

  if (watching ()) {
    Watches tmp;
    for (auto idx : vars)
      flush_watches (idx, tmp), flush_watches (-idx, tmp);
  }
}

/*------------------------------------------------------------------------*/

void Internal::update_reason_references () {
  LOG ("update assigned reason references");
#ifdef LOGGING
  size_t count = 0;
#endif
  for (auto &lit : trail) {
    if (!active (lit))
      continue;
    Var &v = var (lit);
    Clause *c = v.reason;
    if (!c)
      continue;
    if (c == external_reason)
      continue;
    LOG (c, "updating assigned %d reason", lit);
    CADICAL_assert (c->reason);
    CADICAL_assert (c->moved);
    Clause *d = c->copy;
    v.reason = d;
#ifdef LOGGING
    count++;
#endif
  }
  LOG ("updated %zd assigned reason references", count);
}

/*------------------------------------------------------------------------*/

// This is a simple garbage collector which does not move clauses.  It needs
// less space than the arena based clause allocator, but is not as cache
// efficient, since the copying garbage collector can put clauses together
// which are likely accessed after each other.

void Internal::delete_garbage_clauses () {

  flush_all_occs_and_watches ();

  LOG ("deleting garbage clauses");
#ifndef CADICAL_QUIET
  int64_t collected_bytes = 0, collected_clauses = 0;
#endif
  const auto end = clauses.end ();
  auto j = clauses.begin (), i = j;
  while (i != end) {
    Clause *c = *j++ = *i++;
    if (!c->collect ())
      continue;
#ifndef CADICAL_QUIET
    collected_bytes += c->bytes ();
    collected_clauses++;
#endif
    delete_clause (c);
    j--;
  }
  clauses.resize (j - clauses.begin ());
  shrink_vector (clauses);

  PHASE ("collect", stats.collections,
         "collected %" PRId64 " bytes of %" PRId64 " garbage clauses",
         collected_bytes, collected_clauses);
}

/*------------------------------------------------------------------------*/

// This is the start of the copying garbage collector using the arena.  At
// the core is the following function, which copies a clause to the 'to'
// space of the arena.  Be careful if this clause is a reason of an
// assignment.  In that case update the reason reference.
//
void Internal::copy_clause (Clause *c) {
  LOG (c, "moving");
  CADICAL_assert (!c->moved);
  char *p = (char *) c;
  char *q = arena.copy (p, c->bytes ());
  c->copy = (Clause *) q;
  c->moved = true;
  LOG ("copied clause[%" PRId64 "] from %p to %p", c->id, (void *) c,
       (void *) c->copy);
}

// This is the moving garbage collector.

void Internal::copy_non_garbage_clauses () {

  size_t collected_clauses = 0, collected_bytes = 0;
  size_t moved_clauses = 0, moved_bytes = 0;

  // First determine 'moved_bytes' and 'collected_bytes'.
  //
  for (const auto &c : clauses)
    if (!c->collect ())
      moved_bytes += c->bytes (), moved_clauses++;
    else
      collected_bytes += c->bytes (), collected_clauses++;

  PHASE ("collect", stats.collections,
         "moving %zd bytes %.0f%% of %zd non garbage clauses", moved_bytes,
         percent (moved_bytes, collected_bytes + moved_bytes),
         moved_clauses);
  (void) moved_clauses, (void) collected_clauses, (void) collected_bytes;
  // Prepare 'to' space of size 'moved_bytes'.
  //
  arena.prepare (moved_bytes);

  // Keep clauses in arena in the same order.
  //
  if (opts.arenacompact)
    for (const auto &c : clauses)
      if (!c->collect () && arena.contains (c))
        copy_clause (c);

  if (opts.arenatype == 1 || !watching ()) {

    // Localize according to current clause order.

    // If the option 'opts.arenatype == 1' is set, then this means the
    // solver uses the original order of clauses.  If there are no watches,
    // we can not use the watched based copying policies below.  This
    // happens if garbage collection is triggered during bounded variable
    // elimination.

    // Copy clauses according to the order of calling 'copy_clause', which
    // in essence just gives a compacting garbage collector, since their
    // relative order is kept, and actually already gives the largest
    // benefit due to better cache locality.

    for (const auto &c : clauses)
      if (!c->moved && !c->collect ())
        copy_clause (c);

  } else if (opts.arenatype == 2) {

    // Localize according to (original) variable order.

    // This is almost the version used by MiniSAT and descendants.
    // Our version uses saved phases too.

    for (int sign = -1; sign <= 1; sign += 2)
      for (auto idx : vars)
        for (const auto &w : watches (sign * likely_phase (idx)))
          if (!w.clause->moved && !w.clause->collect ())
            copy_clause (w.clause);

  } else {

    // Localize according to decision queue order.

    // This is the default for search. It allocates clauses in the order of
    // the decision queue and also uses saved phases.  It seems faster than
    // the MiniSAT version and thus we keep 'opts.arenatype == 3'.

    CADICAL_assert (opts.arenatype == 3);

    for (int sign = -1; sign <= 1; sign += 2)
      for (int idx = queue.last; idx; idx = link (idx).prev)
        for (const auto &w : watches (sign * likely_phase (idx)))
          if (!w.clause->moved && !w.clause->collect ())
            copy_clause (w.clause);
  }

  // Do not forget to move clauses which are not watched, which happened in
  // a rare situation, and now is only left as defensive code.
  //
  for (const auto &c : clauses)
    if (!c->collect () && !c->moved)
      copy_clause (c);

  flush_all_occs_and_watches ();
  update_reason_references ();

  // Replace and flush clause references in 'clauses'.
  //
  const auto end = clauses.end ();
  auto j = clauses.begin (), i = j;
  for (; i != end; i++) {
    Clause *c = *i;
    if (c->collect ())
      delete_clause (c);
    else
      CADICAL_assert (c->moved), *j++ = c->copy, deallocate_clause (c);
  }
  clauses.resize (j - clauses.begin ());
  if (clauses.size () < clauses.capacity () / 2)
    shrink_vector (clauses);

  if (opts.arenasort)
    rsort (clauses.begin (), clauses.end (), pointer_rank ());

  // Release 'from' space completely and then swap 'to' with 'from'.
  //
  arena.swap ();

  PHASE ("collect", stats.collections,
         "collected %zd bytes %.0f%% of %zd garbage clauses",
         collected_bytes,
         percent (collected_bytes, collected_bytes + moved_bytes),
         collected_clauses);
}

/*------------------------------------------------------------------------*/

// Maintaining clause statistics is complex and error prone but necessary
// for proper scheduling of garbage collection, particularly during bounded
// variable elimination.  With this function we can check whether these
// statistics are updated correctly.

void Internal::check_clause_stats () {
#ifndef CADICAL_NDEBUG
  int64_t irredundant = 0, redundant = 0, total = 0, irrlits = 0;
  for (const auto &c : clauses) {
    if (c->garbage)
      continue;
    if (c->redundant)
      redundant++;
    else
      irredundant++;
    if (!c->redundant)
      irrlits += c->size;
    total++;
  }
  CADICAL_assert (stats.current.irredundant == irredundant);
  CADICAL_assert (stats.current.redundant == redundant);
  CADICAL_assert (stats.current.total == total);
  CADICAL_assert (stats.irrlits == irrlits);
#endif
}

/*------------------------------------------------------------------------*/

// only delete binary clauses from watch list that are already mark as
// deleted.
void Internal::remove_garbage_binaries () {
  if (unsat)
    return;
  START (collect);

  if (!protected_reasons)
    protect_reasons ();
  int backtrack_level = level + 1;
  Watches saved;
  for (auto v : vars) {
    for (auto lit : {-v, v}) {
      CADICAL_assert (saved.empty ());
      Watches &ws = watches (lit);
      const const_watch_iterator end = ws.end ();
      watch_iterator j = ws.begin ();
      const_watch_iterator i;
      for (i = j; i != end; i++) {
        Watch w = *i;
        *j++ = w;
        Clause *c = w.clause;
        COVER (!w.binary () && c->size == 2);
        if (!w.binary ())
          continue;
        if (c->reason && c->garbage) {
          COVER (true);
          CADICAL_assert (c->size == 2);
          backtrack_level =
              min (backtrack_level, var (c->literals[0]).level);
          LOG ("need to backtrack to before level %d", backtrack_level);
          --j;
          continue;
        }
        if (!c->collect ())
          continue;
        LOG (c, "removing from watch list");
        --j;
      }
      ws.resize (j - ws.begin ());
      shrink_vector (ws);
    }
  }
  delete_garbage_clauses ();
  unprotect_reasons ();
  if (backtrack_level - 1 < level)
    backtrack (backtrack_level - 1);
  STOP (collect);
}

/*------------------------------------------------------------------------*/

bool Internal::arenaing () { return opts.arena && (stats.collections > 1); }

void Internal::garbage_collection () {
  if (unsat)
    return;
  START (collect);
  report ('G', 1);
  stats.collections++;
  mark_satisfied_clauses_as_garbage ();
  if (!protected_reasons)
    protect_reasons ();
  if (arenaing ())
    copy_non_garbage_clauses ();
  else
    delete_garbage_clauses ();
  check_clause_stats ();
  check_var_stats ();
  unprotect_reasons ();
  report ('C', 1);
  STOP (collect);
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
