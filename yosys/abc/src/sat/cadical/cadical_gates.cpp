#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// As in our original SATeLite published at SAT'05 we are trying to find
// gates in order to restrict the number of resolutions that need to be
// tried.  If there is such a gate, we only need to consider resolvents
// among gate and one non-gate clauses.  Resolvents between definitions will
// be tautological anyhow and resolvents among non-gates can actually be
// shown to be redundant too.

/*------------------------------------------------------------------------*/

// The next function returns a non-zero if the clause 'c', which is assumed
// to contain the literal 'first', after removing falsified literals is a
// binary clause.  Then the actual second literal is returned.

int Internal::second_literal_in_binary_clause (Eliminator &eliminator,
                                               Clause *c, int first) {
  CADICAL_assert (!c->garbage);
  int second = 0;
  for (const auto &lit : *c) {
    if (lit == first)
      continue;
    const signed char tmp = val (lit);
    if (tmp < 0)
      continue;
    if (tmp > 0) {
      mark_garbage (c);
      elim_update_removed_clause (eliminator, c);
      return 0;
    }
    if (second) {
      second = INT_MIN;
      break;
    }
    second = lit;
  }
  if (!second)
    return 0;
  if (second == INT_MIN)
    return 0;
  CADICAL_assert (active (second));
#ifdef LOGGING
  if (c->size == 2)
    LOG (c, "found binary");
  else
    LOG (c, "found actual binary %d %d", first, second);
#endif
  return second;
}

/*------------------------------------------------------------------------*/

// need a copy from above that does not care about garbage

int Internal::second_literal_in_binary_clause_lrat (Clause *c, int first) {
  if (c->garbage)
    return 0;
  int second = 0;
  for (const auto &lit : *c) {
    if (lit == first)
      continue;
    const signed char tmp = val (lit);
    if (tmp < 0)
      continue;
    if (tmp > 0)
      return 0;
    if (!tmp) {
      if (second) {
        second = INT_MIN;
        break;
      }
      second = lit;
    }
  }
  if (!second)
    return 0;
  if (second == INT_MIN)
    return 0;
  return second;
}

// I needed to find the second clause for hyper unary resolution to build
// LRAT this is not efficient but I could not find a better way then just
// finding the corresponding clause in all possible clauses
//
Clause *Internal::find_binary_clause (int first, int second) {
  int best = first;
  int other = second;
  if (occs (first).size () > occs (second).size ()) {
    best = second;
    other = first;
  }
  for (auto c : occs (best))
    if (second_literal_in_binary_clause_lrat (c, best) == other)
      return c;
  return 0;
}

/*------------------------------------------------------------------------*/

// Mark all other literals in binary clauses with 'first'.  During this
// marking we might also detect hyper unary resolvents producing a unit.
// If such a unit is found we propagate it and return immediately.

void Internal::mark_binary_literals (Eliminator &eliminator, int first) {

  if (unsat)
    return;
  if (val (first))
    return;
  if (!eliminator.gates.empty ())
    return;

  CADICAL_assert (!marked (first));
  CADICAL_assert (eliminator.marked.empty ());

  const Occs &os = occs (first);
  for (const auto &c : os) {
    if (c->garbage)
      continue;
    const int second =
        second_literal_in_binary_clause (eliminator, c, first);
    if (!second)
      continue;
    const int tmp = marked (second);
    if (tmp < 0) {
      // had a bug where units could occur multiple times here
      // solved with flags
      LOG ("found binary resolved unit %d", first);
      if (lrat) {
        Clause *d = find_binary_clause (first, -second);
        CADICAL_assert (d);
        for (auto &lit : *d) {
          if (lit == first || lit == -second)
            continue;
          CADICAL_assert (val (lit) < 0);
          Flags &f = flags (lit);
          if (f.seen)
            continue;
          analyzed.push_back (lit);
          f.seen = true;
          int64_t id = unit_id (-lit);
          lrat_chain.push_back (id);
          // LOG ("gates added id %" PRId64, id);
        }
        for (auto &lit : *c) {
          if (lit == first || lit == second)
            continue;
          CADICAL_assert (val (lit) < 0);
          Flags &f = flags (lit);
          if (f.seen)
            continue;
          analyzed.push_back (lit);
          f.seen = true;
          int64_t id = unit_id (-lit);
          lrat_chain.push_back (id);
          // LOG ("gates added id %" PRId64, id);
        }
        lrat_chain.push_back (c->id);
        lrat_chain.push_back (d->id);
        // LOG ("gates added id %" PRId64, c->id);
        // LOG ("gates added id %" PRId64, d->id);
        clear_analyzed_literals ();
      }
      assign_unit (first);
      elim_propagate (eliminator, first);
      return;
    }
    if (tmp > 0) {
      LOG (c, "duplicated actual binary clause");
      elim_update_removed_clause (eliminator, c);
      mark_garbage (c);
      continue;
    }
    eliminator.marked.push_back (second);
    mark (second);
    LOG ("marked second literal %d in binary clause %d %d", second, first,
         second);
  }
}

// Unmark all literals saved on the 'marked' stack.

void Internal::unmark_binary_literals (Eliminator &eliminator) {
  LOG ("unmarking %zd literals", eliminator.marked.size ());
  for (const auto &lit : eliminator.marked)
    unmark (lit);
  eliminator.marked.clear ();
}

/*------------------------------------------------------------------------*/

// Find equivalence for 'pivot'.  Requires that all other literals in binary
// clauses with 'pivot' are marked (through 'mark_binary_literals');

void Internal::find_equivalence (Eliminator &eliminator, int pivot) {

  if (!opts.elimequivs)
    return;

  CADICAL_assert (opts.elimsubst);

  if (unsat)
    return;
  if (val (pivot))
    return;
  if (!eliminator.gates.empty ())
    return;

  mark_binary_literals (eliminator, pivot);
  if (unsat || val (pivot))
    goto DONE;

  for (const auto &c : occs (-pivot)) {

    if (c->garbage)
      continue;

    const int second =
        second_literal_in_binary_clause (eliminator, c, -pivot);
    if (!second)
      continue;
    const int tmp = marked (second);
    if (tmp > 0) {
      LOG ("found binary resolved unit %d", second);
      // did not find a bug where units could occur multiple times here
      // still solved potential issues with flags
      if (lrat) {
        Clause *d = find_binary_clause (pivot, second);
        CADICAL_assert (d);
        for (auto &lit : *d) {
          if (lit == pivot || lit == second)
            continue;
          CADICAL_assert (val (lit) < 0);
          Flags &f = flags (lit);
          if (f.seen)
            continue;
          analyzed.push_back (lit);
          f.seen = true;
          int64_t id = unit_id (-lit);
          lrat_chain.push_back (id);
          // LOG ("gates added id %" PRId64, id);
        }
        for (auto &lit : *c) {
          if (lit == -pivot || lit == second)
            continue;
          CADICAL_assert (val (lit) < 0);
          Flags &f = flags (lit);
          if (f.seen)
            continue;
          analyzed.push_back (lit);
          f.seen = true;
          int64_t id = unit_id (-lit);
          lrat_chain.push_back (id);
          // LOG ("gates added id %" PRId64, id);
        }
        lrat_chain.push_back (c->id);
        lrat_chain.push_back (d->id);
        clear_analyzed_literals ();
        // LOG ("gates added id %" PRId64, c->id);
        // LOG ("gates added id %" PRId64, d->id);
      }
      assign_unit (second);
      elim_propagate (eliminator, second);
      if (val (pivot))
        break;
      if (unsat)
        break;
    }
    if (tmp >= 0)
      continue;

    LOG ("found equivalence %d = %d", pivot, -second);
    stats.elimequivs++;
    stats.elimgates++;

    LOG (c, "first gate clause");
    CADICAL_assert (!c->gate);
    c->gate = true;
    eliminator.gates.push_back (c);

    Clause *d = 0;
    const Occs &ps = occs (pivot);
    for (const auto &e : ps) {
      if (e->garbage)
        continue;
      const int other =
          second_literal_in_binary_clause (eliminator, e, pivot);
      if (other == -second) {
        d = e;
        break;
      }
    }
    CADICAL_assert (d);

    LOG (d, "second gate clause");
    CADICAL_assert (!d->gate);
    d->gate = true;
    eliminator.gates.push_back (d);
    eliminator.gatetype = EQUI;

    break;
  }

DONE:
  unmark_binary_literals (eliminator);
}

/*------------------------------------------------------------------------*/

// Find and gates for 'pivot' with a long clause, in which the pivot occurs
// positively.  Requires that all other literals in binary clauses with
// 'pivot' are marked (through 'mark_binary_literals');

void Internal::find_and_gate (Eliminator &eliminator, int pivot) {

  if (!opts.elimands)
    return;

  CADICAL_assert (opts.elimsubst);

  if (unsat)
    return;
  if (val (pivot))
    return;
  if (!eliminator.gates.empty ())
    return;

  mark_binary_literals (eliminator, pivot);
  if (unsat || val (pivot))
    goto DONE;

  for (const auto &c : occs (-pivot)) {

    if (c->garbage)
      continue;
    if (c->size < 3)
      continue;

    bool all_literals_marked = true;
    unsigned arity = 0;
    int satisfied = 0;

    for (const auto &lit : *c) {
      if (lit == -pivot)
        continue;
      CADICAL_assert (lit != pivot);
      signed char tmp = val (lit);
      if (tmp < 0)
        continue;
      if (tmp > 0) {
        satisfied = lit;
        break;
      }
      tmp = marked (lit);
      if (tmp < 0) {
        arity++;
        continue;
      }
      all_literals_marked = false;
      break;
    }

    if (!all_literals_marked)
      continue;

    if (satisfied) {
      LOG (c, "satisfied by %d candidate base clause", satisfied);
      mark_garbage (c);
      continue;
    }

#ifdef LOGGING
    if (opts.log) {
      Logger::print_log_prefix (this);
      tout.magenta ();
      printf ("found arity %u AND gate %d = ", arity, -pivot);
      bool first = true;
      for (const auto &lit : *c) {
        if (lit == -pivot)
          continue;
        CADICAL_assert (lit != pivot);
        if (!first)
          fputs (" & ", stdout);
        printf ("%d", -lit);
        first = false;
      }
      fputc ('\n', stdout);
      tout.normal ();
      fflush (stdout);
    }
#endif
    stats.elimands++;
    stats.elimgates++;
    eliminator.gatetype = AND;

    (void) arity;
    CADICAL_assert (!c->gate);
    c->gate = true;
    eliminator.gates.push_back (c);
    for (const auto &lit : *c) {
      if (lit == -pivot)
        continue;
      CADICAL_assert (lit != pivot);
      signed char tmp = val (lit);
      if (tmp < 0)
        continue;
      CADICAL_assert (!tmp);
      CADICAL_assert (marked (lit) < 0);
      marks[vidx (lit)] *= 2;
    }

    unsigned count = 0;
    for (const auto &d : occs (pivot)) {
      if (d->garbage)
        continue;
      const int other =
          second_literal_in_binary_clause (eliminator, d, pivot);
      if (!other)
        continue;
      const int tmp = marked (other);
      if (tmp != 2)
        continue;
      LOG (d, "AND gate binary side clause");
      CADICAL_assert (!d->gate);
      d->gate = true;
      eliminator.gates.push_back (d);
      count++;
    }
    CADICAL_assert (count >= arity);
    (void) count;

    break;
  }

DONE:
  unmark_binary_literals (eliminator);
}

/*------------------------------------------------------------------------*/

// Find and extract ternary clauses.

bool Internal::get_ternary_clause (Clause *d, int &a, int &b, int &c) {
  if (d->garbage)
    return false;
  if (d->size < 3)
    return false;
  int found = 0;
  a = b = c = 0;
  for (const auto &lit : *d) {
    if (val (lit))
      continue;
    if (++found == 1)
      a = lit;
    else if (found == 2)
      b = lit;
    else if (found == 3)
      c = lit;
    else
      return false;
  }
  return found == 3;
}

// This function checks whether 'd' exists as ternary clause.

bool Internal::match_ternary_clause (Clause *d, int a, int b, int c) {
  if (d->garbage)
    return false;
  int found = 0;
  for (const auto &lit : *d) {
    if (val (lit))
      continue;
    if (a != lit && b != lit && c != lit)
      return false;
    found++;
  }
  return found == 3;
}

Clause *Internal::find_ternary_clause (int a, int b, int c) {
  if (occs (b).size () > occs (c).size ())
    swap (b, c);
  if (occs (a).size () > occs (b).size ())
    swap (a, b);
  for (auto d : occs (a))
    if (match_ternary_clause (d, a, b, c))
      return d;
  return 0;
}

/*------------------------------------------------------------------------*/

// Find if-then-else gate.

void Internal::find_if_then_else (Eliminator &eliminator, int pivot) {

  if (!opts.elimites)
    return;

  CADICAL_assert (opts.elimsubst);

  if (unsat)
    return;
  if (val (pivot))
    return;
  if (!eliminator.gates.empty ())
    return;

  const Occs &os = occs (pivot);
  const auto end = os.end ();
  for (auto i = os.begin (); i != end; i++) {
    Clause *di = *i;
    int ai, bi, ci;
    if (!get_ternary_clause (di, ai, bi, ci))
      continue;
    if (bi == pivot)
      swap (ai, bi);
    if (ci == pivot)
      swap (ai, ci);
    CADICAL_assert (ai == pivot);
    for (auto j = i + 1; j != end; j++) {
      Clause *dj = *j;
      int aj, bj, cj;
      if (!get_ternary_clause (dj, aj, bj, cj))
        continue;
      if (bj == pivot)
        swap (aj, bj);
      if (cj == pivot)
        swap (aj, cj);
      CADICAL_assert (aj == pivot);
      if (abs (bi) == abs (cj))
        swap (bj, cj);
      if (abs (ci) == abs (cj))
        continue;
      if (bi != -bj)
        continue;
      Clause *d1 = find_ternary_clause (-pivot, bi, -ci);
      if (!d1)
        continue;
      Clause *d2 = find_ternary_clause (-pivot, bj, -cj);
      if (!d2)
        continue;
      LOG (di, "1st if-then-else");
      LOG (dj, "2nd if-then-else");
      LOG (d1, "3rd if-then-else");
      LOG (d2, "4th if-then-else");
      LOG ("found ITE gate %d == (%d ? %d : %d)", pivot, -bi, -ci, -cj);
      CADICAL_assert (!di->gate);
      CADICAL_assert (!dj->gate);
      CADICAL_assert (!d1->gate);
      CADICAL_assert (!d2->gate);
      di->gate = true;
      dj->gate = true;
      d1->gate = true;
      d2->gate = true;
      eliminator.gates.push_back (di);
      eliminator.gates.push_back (dj);
      eliminator.gates.push_back (d1);
      eliminator.gates.push_back (d2);
      stats.elimgates++;
      stats.elimites++;
      eliminator.gatetype = ITE;
      return;
    }
  }
}

/*------------------------------------------------------------------------*/

// Find and extract clause.

bool Internal::get_clause (Clause *c, vector<int> &l) {
  if (c->garbage)
    return false;
  l.clear ();
  for (const auto &lit : *c) {
    if (val (lit) < 0)
      continue;
    if (val (lit) > 0) {
      l.clear ();
      return false;
    }
    l.push_back (lit);
  }
  return true;
}

// Check whether 'c' contains only the literals in 'l'.

bool Internal::is_clause (Clause *c, const vector<int> &lits) {
  if (c->garbage)
    return false;
  int size = lits.size ();
  if (c->size < size)
    return false;
  int found = 0;
  for (const auto &lit : *c) {
    if (val (lit) < 0)
      continue;
    if (val (lit) > 0)
      return false;
    const auto it = find (lits.begin (), lits.end (), lit);
    if (it == lits.end ())
      return false;
    if (++found > size)
      return false;
  }
  return found == size;
}

Clause *Internal::find_clause (const vector<int> &lits) {
  int best = 0;
  size_t len = 0;
  for (const auto &lit : lits) {
    size_t l = occs (lit).size ();
    if (best && l >= len)
      continue;
    len = l, best = lit;
  }
  for (auto c : occs (best))
    if (is_clause (c, lits))
      return c;
  return 0;
}

void Internal::find_xor_gate (Eliminator &eliminator, int pivot) {

  if (!opts.elimxors)
    return;

  CADICAL_assert (opts.elimsubst);

  if (unsat)
    return;
  if (val (pivot))
    return;
  if (!eliminator.gates.empty ())
    return;

  vector<int> lits;

  for (auto d : occs (pivot)) {

    if (!get_clause (d, lits))
      continue;

    const int size = lits.size (); // clause size
    const int arity = size - 1;    // arity of XOR

    if (size < 3)
      continue;
    if (arity > opts.elimxorlim)
      continue;

    CADICAL_assert (eliminator.gates.empty ());

    unsigned needed = (1u << arity) - 1; // additional clauses
    unsigned signs = 0;                  // literals to negate

    do {
      const unsigned prev = signs;
      while (parity (++signs))
        ;
      for (int j = 0; j < size; j++) {
        const unsigned bit = 1u << j;
        int lit = lits[j];
        if ((prev & bit) != (signs & bit))
          lits[j] = lit = -lit;
      }
      Clause *e = find_clause (lits);
      if (!e)
        break;
      eliminator.gates.push_back (e);
    } while (--needed);

    if (needed) {
      eliminator.gates.clear ();
      continue;
    }

    eliminator.gates.push_back (d);
    CADICAL_assert (eliminator.gates.size () == (1u << arity));

#ifdef LOGGING
    if (opts.log) {
      Logger::print_log_prefix (this);
      tout.magenta ();
      printf ("found arity %u XOR gate %d = ", arity, -pivot);
      bool first = true;
      for (const auto &lit : *d) {
        if (lit == pivot)
          continue;
        CADICAL_assert (lit != -pivot);
        if (!first)
          fputs (" ^ ", stdout);
        printf ("%d", lit);
        first = false;
      }
      fputc ('\n', stdout);
      tout.normal ();
      fflush (stdout);
    }
#endif
    stats.elimgates++;
    stats.elimxors++;
    const auto end = eliminator.gates.end ();
    auto j = eliminator.gates.begin ();
    for (auto i = j; i != end; i++) {
      Clause *e = *i;
      if (e->gate)
        continue;
      e->gate = true;
      LOG (e, "contributing");
      *j++ = e;
    }
    eliminator.gates.resize (j - eliminator.gates.begin ());
    eliminator.gatetype = XOR;
    break;
  }
}

/*------------------------------------------------------------------------*/

// Find a gate for 'pivot'.  If such a gate is found, the gate clauses are
// marked and pushed on the stack of gates.  Further hyper unary resolution
// might detect units, which are propagated.  This might assign the pivot or
// even produce the empty clause.

void Internal::find_gate_clauses (Eliminator &eliminator, int pivot) {
  if (!opts.elimsubst)
    return;

  if (unsat)
    return;
  if (val (pivot))
    return;

  CADICAL_assert (eliminator.gates.empty ());

  find_equivalence (eliminator, pivot);
  find_and_gate (eliminator, pivot);
  find_and_gate (eliminator, -pivot);
  find_if_then_else (eliminator, pivot);
  find_xor_gate (eliminator, pivot);
  find_definition (eliminator, pivot);
}

void Internal::unmark_gate_clauses (Eliminator &eliminator) {
  LOG ("unmarking %zd gate clauses", eliminator.gates.size ());
  for (const auto &c : eliminator.gates) {
    CADICAL_assert (c->gate);
    c->gate = false;
  }
  eliminator.gates.clear ();
  eliminator.definition_unit = 0;
}

/*------------------------------------------------------------------------*/

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
