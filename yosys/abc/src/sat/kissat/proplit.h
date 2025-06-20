static inline void kissat_watch_large_delayed (kissat *solver,
                                               watches *all_watches,
                                               unsigneds *delayed) {
  KISSAT_assert (all_watches == solver->watches);
  KISSAT_assert (delayed == &solver->delayed);
  const unsigned *const end_delayed = END_STACK (*delayed);
  unsigned const *d = BEGIN_STACK (*delayed);
  while (d != end_delayed) {
    const unsigned lit = *d++;
    KISSAT_assert (d != end_delayed);
    const watch watch = {.raw = *d++};
    KISSAT_assert (!watch.type.binary);
    KISSAT_assert (lit < LITS);
    watches *const lit_watches = all_watches + lit;
    KISSAT_assert (d != end_delayed);
    const reference ref = *d++;
    const unsigned blocking = watch.blocking.lit;
    LOGREF3 (ref, "watching %s blocking %s in", LOGLIT (lit),
             LOGLIT (blocking));
    kissat_push_blocking_watch (solver, lit_watches, blocking, ref);
  }
  CLEAR_STACK (*delayed);
}

static inline void
kissat_delay_watching_large (kissat *solver, unsigneds *const delayed,
                             unsigned lit, unsigned other, reference ref) {
  const watch watch = kissat_blocking_watch (other);
  PUSH_STACK (*delayed, lit);
  PUSH_STACK (*delayed, watch.raw);
  PUSH_STACK (*delayed, ref);
}

static inline clause *PROPAGATE_LITERAL (kissat *solver,
#if defined(PROBING_PROPAGATION)
                                         const clause *const ignore,
#endif
                                         const unsigned lit) {
  KISSAT_assert (solver->watching);
  LOG (PROPAGATION_TYPE " propagating %s", LOGLIT (lit));
  KISSAT_assert (VALUE (lit) > 0);
  KISSAT_assert (EMPTY_STACK (solver->delayed));

  watches *const all_watches = solver->watches;
  ward *const arena = BEGIN_STACK (solver->arena);
  assigned *const assigned = solver->assigned;
  value *const values = solver->values;

  const unsigned not_lit = NOT (lit);

  KISSAT_assert (not_lit < LITS);
  watches *watches = all_watches + not_lit;

  watch *const begin_watches = BEGIN_WATCHES (*watches);
  const watch *const end_watches = END_WATCHES (*watches);

  watch *q = begin_watches;
  const watch *p = q;

  unsigneds *const delayed = &solver->delayed;
  KISSAT_assert (EMPTY_STACK (*delayed));

  const size_t size_watches = SIZE_WATCHES (*watches);
  uint64_t ticks = 1 + kissat_cache_lines (size_watches, sizeof (watch));
  const unsigned idx = IDX (lit);
  struct assigned *const a = assigned + idx;
  const bool probing = solver->probing;
  const unsigned level = a->level;
  clause *res = 0;

  while (p != end_watches) {
    const watch head = *q++ = *p++;
    const unsigned blocking = head.blocking.lit;
    KISSAT_assert (VALID_INTERNAL_LITERAL (blocking));
    const value blocking_value = values[blocking];
    const bool binary = head.type.binary;
    watch tail;
    if (!binary)
      tail = *q++ = *p++;
    if (blocking_value > 0)
      continue;
    if (binary) {
      if (blocking_value < 0) {
        res = kissat_binary_conflict (solver, not_lit, blocking);
#ifndef CONTINUE_PROPAGATING_AFTER_CONFLICT
        break;
#endif
      } else {
        KISSAT_assert (!blocking_value);
        kissat_fast_binary_assign (solver, probing, level, values, assigned,
                                   blocking, not_lit);
        ticks++;
      }
    } else {
      const reference ref = tail.raw;
      KISSAT_assert (ref < SIZE_STACK (solver->arena));
      clause *const c = (clause *) (arena + ref);
      ticks++;
      if (c->garbage) {
        q -= 2;
        continue;
      }
      unsigned *const lits = BEGIN_LITS (c);
      const unsigned other = lits[0] ^ lits[1] ^ not_lit;
      KISSAT_assert (lits[0] != lits[1]);
      KISSAT_assert (VALID_INTERNAL_LITERAL (other));
      KISSAT_assert (not_lit != other);
      KISSAT_assert (lit != other);
      const value other_value = values[other];
      if (other_value > 0) {
        q[-2].blocking.lit = other;
        continue;
      }
      const unsigned *const end_lits = lits + c->size;
      unsigned *const searched = lits + c->searched;
      KISSAT_assert (c->lits + 2 <= searched);
      KISSAT_assert (searched < end_lits);
      unsigned *r, replacement = INVALID_LIT;
      value replacement_value = -1;
      for (r = searched; r != end_lits; r++) {
        replacement = *r;
        KISSAT_assert (VALID_INTERNAL_LITERAL (replacement));
        replacement_value = values[replacement];
        if (replacement_value >= 0)
          break;
      }
      if (replacement_value < 0) {
        for (r = lits + 2; r != searched; r++) {
          replacement = *r;
          KISSAT_assert (VALID_INTERNAL_LITERAL (replacement));
          replacement_value = values[replacement];
          if (replacement_value >= 0)
            break;
        }
      }

      if (replacement_value >= 0) {
        c->searched = r - lits;
        KISSAT_assert (replacement != INVALID_LIT);
        LOGREF3 (ref, "unwatching %s in", LOGLIT (not_lit));
        q -= 2;
        lits[0] = other;
        lits[1] = replacement;
        KISSAT_assert (lits[0] != lits[1]);
        *r = not_lit;
        kissat_delay_watching_large (solver, delayed, replacement, other,
                                     ref);
        ticks++;
      } else if (other_value) {
        KISSAT_assert (replacement_value < 0);
        KISSAT_assert (blocking_value < 0);
        KISSAT_assert (other_value < 0);
#if defined(PROBING_PROPAGATION)
        if (c == ignore) {
          LOGREF (ref, "conflicting but ignored");
          continue;
        }
#endif
        LOGREF (ref, "conflicting");
        res = c;
#ifndef CONTINUE_PROPAGATING_AFTER_CONFLICT
        break;
#endif
      } else {
        KISSAT_assert (replacement_value < 0);
#if defined(PROBING_PROPAGATION)
        if (c == ignore) {
          LOGREF (ref, "forcing %s but ignored", LOGLIT (other));
          continue;
        }
#endif
        kissat_fast_assign_reference (solver, values, assigned, other, ref,
                                      c);
        ticks++;
      }
    }
  }
  solver->ticks += ticks;

  while (p != end_watches)
    *q++ = *p++;
  SET_END_OF_WATCHES (*watches, q);

  kissat_watch_large_delayed (solver, all_watches, delayed);

  return res;
}

static inline void kissat_update_conflicts_and_trail (kissat *solver,
                                                      clause *conflict,
                                                      bool flush) {
  if (conflict) {
#ifndef PROBING_PROPAGATION
    INC (conflicts);
#endif
    if (!solver->level) {
      LOG (PROPAGATION_TYPE " propagation on root-level failed");
      solver->inconsistent = true;
      CHECK_AND_ADD_EMPTY ();
      ADD_EMPTY_TO_PROOF ();
    }
  } else if (flush && !solver->level && solver->unflushed)
    kissat_flush_trail (solver);
}
