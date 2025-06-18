#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

inline unsigned Checker::l2u (int lit) {
  CADICAL_assert (lit);
  CADICAL_assert (lit != INT_MIN);
  unsigned res = 2 * (abs (lit) - 1);
  if (lit < 0)
    res++;
  return res;
}

inline signed char Checker::val (int lit) {
  CADICAL_assert (lit);
  CADICAL_assert (lit != INT_MIN);
  CADICAL_assert (abs (lit) < size_vars);
  CADICAL_assert (vals[lit] == -vals[-lit]);
  return vals[lit];
}

signed char &Checker::mark (int lit) {
  const unsigned u = l2u (lit);
  CADICAL_assert (u < marks.size ());
  return marks[u];
}

inline CheckerWatcher &Checker::watcher (int lit) {
  const unsigned u = l2u (lit);
  CADICAL_assert (u < watchers.size ());
  return watchers[u];
}

/*------------------------------------------------------------------------*/

CheckerClause *Checker::new_clause () {
  const size_t size = simplified.size ();
  CADICAL_assert (size > 1), CADICAL_assert (size <= UINT_MAX);
  const size_t bytes = sizeof (CheckerClause) + (size - 2) * sizeof (int);
  CheckerClause *res = (CheckerClause *) new char[bytes];
  DeferDeleteArray<char> delete_res ((char *) res);
  res->next = 0;
  res->hash = last_hash;
  res->size = size;
  int *literals = res->literals, *p = literals;
  for (const auto &lit : simplified)
    *p++ = lit;
  num_clauses++;

  // First two literals are used as watches and should not be false.
  //
  for (unsigned i = 0; i < 2; i++) {
    int lit = literals[i];
    if (!val (lit))
      continue;
    for (unsigned j = i + 1; j < size; j++) {
      int other = literals[j];
      if (val (other))
        continue;
      swap (literals[i], literals[j]);
      break;
    }
  }
  CADICAL_assert (!val (literals[0]));
  CADICAL_assert (!val (literals[1]));
  watcher (literals[0]).push_back (CheckerWatch (literals[1], res));
  watcher (literals[1]).push_back (CheckerWatch (literals[0], res));

  delete_res.release ();
  return res;
}

void Checker::delete_clause (CheckerClause *c) {
  if (c->size) {
    CADICAL_assert (c->size > 1);
    CADICAL_assert (num_clauses);
    num_clauses--;
  } else {
    CADICAL_assert (num_garbage);
    num_garbage--;
  }
  delete[] (char *) c;
}

void Checker::enlarge_clauses () {
  CADICAL_assert (num_clauses == size_clauses);
  const uint64_t new_size_clauses = size_clauses ? 2 * size_clauses : 1;
  LOG ("CHECKER enlarging clauses of checker from %" PRIu64 " to %" PRIu64,
       (uint64_t) size_clauses, (uint64_t) new_size_clauses);
  CheckerClause **new_clauses;
  new_clauses = new CheckerClause *[new_size_clauses];
  clear_n (new_clauses, new_size_clauses);
  for (uint64_t i = 0; i < size_clauses; i++) {
    for (CheckerClause *c = clauses[i], *next; c; c = next) {
      next = c->next;
      const uint64_t h = reduce_hash (c->hash, new_size_clauses);
      c->next = new_clauses[h];
      new_clauses[h] = c;
    }
  }
  delete[] clauses;
  clauses = new_clauses;
  size_clauses = new_size_clauses;
}

bool Checker::clause_satisfied (CheckerClause *c) {
  for (unsigned i = 0; i < c->size; i++)
    if (val (c->literals[i]) > 0)
      return true;
  return false;
}

// The main reason why we have an explicit garbage collection phase is that
// removing clauses from watcher lists eagerly might lead to an accumulated
// quadratic algorithm.  Thus we delay removing garbage clauses from watcher
// lists until garbage collection (even though we remove garbage clauses on
// the fly during propagation too).  We also remove satisfied clauses.
//
void Checker::collect_garbage_clauses () {

  stats.collections++;

  for (size_t i = 0; i < size_clauses; i++) {
    CheckerClause **p = clauses + i, *c;
    while ((c = *p)) {
      if (clause_satisfied (c)) {
        c->size = 0; // mark as garbage
        *p = c->next;
        c->next = garbage;
        garbage = c;
        num_garbage++;
        CADICAL_assert (num_clauses);
        num_clauses--;
      } else
        p = &c->next;
    }
  }

  LOG ("CHECKER collecting %" PRIu64 " garbage clauses %.0f%%", num_garbage,
       percent (num_garbage, num_clauses));

  for (int lit = -size_vars + 1; lit < size_vars; lit++) {
    if (!lit)
      continue;
    CheckerWatcher &ws = watcher (lit);
    const auto end = ws.end ();
    auto j = ws.begin (), i = j;
    for (; i != end; i++) {
      CheckerWatch &w = *i;
      if (w.clause->size)
        *j++ = w;
    }
    if (j == ws.end ())
      continue;
    if (j == ws.begin ())
      erase_vector (ws);
    else
      ws.resize (j - ws.begin ());
  }

  for (CheckerClause *c = garbage, *next; c; c = next)
    next = c->next, delete_clause (c);

  CADICAL_assert (!num_garbage);
  garbage = 0;
}

/*------------------------------------------------------------------------*/

Checker::Checker (Internal *i)
    : internal (i), size_vars (0), vals (0), inconsistent (false),
      num_clauses (0), num_garbage (0), size_clauses (0), clauses (0),
      garbage (0), next_to_propagate (0), last_hash (0) {

  // Initialize random number table for hash function.
  //
  Random random (42);
  for (unsigned n = 0; n < num_nonces; n++) {
    uint64_t nonce = random.next ();
    if (!(nonce & 1))
      nonce++;
    CADICAL_assert (nonce), CADICAL_assert (nonce & 1);
    nonces[n] = nonce;
  }

  memset (&stats, 0, sizeof (stats)); // Initialize statistics.
}

void Checker::connect_internal (Internal *i) {
  internal = i;
  LOG ("CHECKER connected to internal");
}

Checker::~Checker () {
  LOG ("CHECKER delete");
  vals -= size_vars;
  delete[] vals;
  for (size_t i = 0; i < size_clauses; i++)
    for (CheckerClause *c = clauses[i], *next; c; c = next)
      next = c->next, delete_clause (c);
  for (CheckerClause *c = garbage, *next; c; c = next)
    next = c->next, delete_clause (c);
  delete[] clauses;
}

/*------------------------------------------------------------------------*/

// The simplicity for accessing 'vals' and 'watchers' directly through a
// signed integer literal, comes with the price of slightly more complex
// code in deleting and enlarging the checker data structures.

void Checker::enlarge_vars (int64_t idx) {

  CADICAL_assert (0 < idx), CADICAL_assert (idx <= INT_MAX);

  int64_t new_size_vars = size_vars ? 2 * size_vars : 2;
  while (idx >= new_size_vars)
    new_size_vars *= 2;
  LOG ("CHECKER enlarging variables of checker from %" PRId64 " to %" PRId64
       "",
       size_vars, new_size_vars);

  signed char *new_vals;
  new_vals = new signed char[2 * new_size_vars];
  clear_n (new_vals, 2 * new_size_vars);
  new_vals += new_size_vars;
  if (size_vars) // To make sanitizer happy (without '-O').
    memcpy ((void *) (new_vals - size_vars), (void *) (vals - size_vars),
            2 * size_vars);
  vals -= size_vars;
  delete[] vals;
  vals = new_vals;
  size_vars = new_size_vars;

  watchers.resize (2 * new_size_vars);
  marks.resize (2 * new_size_vars);

  CADICAL_assert (idx < new_size_vars);
}

inline void Checker::import_literal (int lit) {
  CADICAL_assert (lit);
  CADICAL_assert (lit != INT_MIN);
  int idx = abs (lit);
  if (idx >= size_vars)
    enlarge_vars (idx);
  simplified.push_back (lit);
  unsimplified.push_back (lit);
}

void Checker::import_clause (const vector<int> &c) {
  for (const auto &lit : c)
    import_literal (lit);
}

struct lit_smaller {
  bool operator() (int a, int b) const {
    int c = abs (a), d = abs (b);
    if (c < d)
      return true;
    if (c > d)
      return false;
    return a < b;
  }
};

bool Checker::tautological () {
  sort (simplified.begin (), simplified.end (), lit_smaller ());
  const auto end = simplified.end ();
  auto j = simplified.begin ();
  int prev = 0;
  for (auto i = j; i != end; i++) {
    int lit = *i;
    if (lit == prev)
      continue; // duplicated literal
    if (lit == -prev)
      return true; // tautological clause
    const signed char tmp = val (lit);
    if (tmp > 0)
      return true; // satisfied literal and clause
    *j++ = prev = lit;
  }
  simplified.resize (j - simplified.begin ());
  return false;
}

/*------------------------------------------------------------------------*/

uint64_t Checker::reduce_hash (uint64_t hash, uint64_t size) {
  CADICAL_assert (size > 0);
  unsigned shift = 32;
  uint64_t res = hash;
  while ((((uint64_t) 1) << shift) > size) {
    res ^= res >> shift;
    shift >>= 1;
  }
  res &= size - 1;
  CADICAL_assert (res < size);
  return res;
}

uint64_t Checker::compute_hash () {
  unsigned j = last_id % num_nonces;
  uint64_t tmp = nonces[j] * last_id;
  return last_hash = tmp;
}

CheckerClause **Checker::find () {
  stats.searches++;
  CheckerClause **res, *c;
  const uint64_t hash = compute_hash ();
  const unsigned size = simplified.size ();
  const uint64_t h = reduce_hash (hash, size_clauses);
  for (const auto &lit : simplified)
    mark (lit) = true;
  for (res = clauses + h; (c = *res); res = &c->next) {
    if (c->hash == hash && c->size == size) {
      bool found = true;
      const int *literals = c->literals;
      for (unsigned i = 0; found && i != size; i++)
        found = mark (literals[i]);
      if (found)
        break;
    }
    stats.collisions++;
  }
  for (const auto &lit : simplified)
    mark (lit) = false;
  return res;
}

void Checker::insert () {
  stats.insertions++;
  if (num_clauses == size_clauses)
    enlarge_clauses ();
  const uint64_t h = reduce_hash (compute_hash (), size_clauses);
  CheckerClause *c = new_clause ();
  c->next = clauses[h];
  clauses[h] = c;
}

/*------------------------------------------------------------------------*/

inline void Checker::assign (int lit) {
  CADICAL_assert (!val (lit));
  vals[lit] = 1;
  vals[-lit] = -1;
  trail.push_back (lit);
}

inline void Checker::assume (int lit) {
  signed char tmp = val (lit);
  if (tmp > 0)
    return;
  CADICAL_assert (!tmp);
  stats.assumptions++;
  assign (lit);
}

void Checker::backtrack (unsigned previously_propagated) {

  CADICAL_assert (previously_propagated <= trail.size ());

  while (trail.size () > previously_propagated) {
    int lit = trail.back ();
    CADICAL_assert (val (lit) > 0);
    CADICAL_assert (val (-lit) < 0);
    vals[lit] = vals[-lit] = 0;
    trail.pop_back ();
  }

  trail.resize (previously_propagated);
  next_to_propagate = previously_propagated;
  CADICAL_assert (trail.size () == next_to_propagate);
}

/*------------------------------------------------------------------------*/

// This is a standard propagation routine without using blocking literals
// nor without saving the last replacement position.

bool Checker::propagate () {
  bool res = true;
  while (res && next_to_propagate < trail.size ()) {
    int lit = trail[next_to_propagate++];
    stats.propagations++;
    CADICAL_assert (val (lit) > 0);
    CADICAL_assert (abs (lit) < size_vars);
    CheckerWatcher &ws = watcher (-lit);
    const auto end = ws.end ();
    auto j = ws.begin (), i = j;
    for (; res && i != end; i++) {
      CheckerWatch &w = *j++ = *i;
      const int blit = w.blit;
      CADICAL_assert (blit != -lit);
      const signed char blit_val = val (blit);
      if (blit_val > 0)
        continue;
      const unsigned size = w.size;
      if (size == 2) { // not precise since
        if (blit_val < 0)
          res = false; // clause might be garbage
        else
          assign (w.blit); // but still sound
      } else {
        CADICAL_assert (size > 2);
        CheckerClause *c = w.clause;
        if (!c->size) {
          j--;
          continue;
        } // skip garbage clauses
        CADICAL_assert (size == c->size);
        int *lits = c->literals;
        int other = lits[0] ^ lits[1] ^ (-lit);
        CADICAL_assert (other != -lit);
        signed char other_val = val (other);
        if (other_val > 0) {
          j[-1].blit = other;
          continue;
        }
        lits[0] = other, lits[1] = -lit;
        unsigned k;
        int replacement = 0;
        signed char replacement_val = -1;
        for (k = 2; k < size; k++)
          if ((replacement_val = val (replacement = lits[k])) >= 0)
            break;
        if (replacement_val >= 0) {
          watcher (replacement).push_back (CheckerWatch (-lit, c));
          swap (lits[1], lits[k]);
          j--;
        } else if (!other_val)
          assign (other);
        else
          res = false;
      }
    }
    while (i != end)
      *j++ = *i++;
    ws.resize (j - ws.begin ());
  }
  return res;
}

bool Checker::check () {
  stats.checks++;
  if (inconsistent)
    return true;
  unsigned previously_propagated = next_to_propagate;
  for (const auto &lit : simplified)
    assume (-lit);
  bool res = !propagate ();
  backtrack (previously_propagated);
  return res;
}

bool Checker::check_blocked () {
  for (const auto &lit : unsimplified) {
    mark (-lit) = true;
  }
  vector<int> not_blocked;
  for (size_t i = 0; i < size_clauses; i++) {
    for (CheckerClause *c = clauses[i], *next; c; c = next) {
      next = c->next;
      unsigned count = 0;
      int first;
      for (int *i = c->literals; i < c->literals + c->size; i++) {
        const int lit = *i;
        if (val (lit) > 0) {
          LOG (c->literals, c->size, "satisfied clause");
          count = 2;
          break;
        }
        if (mark (lit)) {
          count++;
          LOG (c->literals, c->size, "clause");
          first = lit;
        }
      }
      if (count == 1)
        not_blocked.push_back (first);
    }
  }
  for (const auto &lit : not_blocked) {
    mark (lit) = false;
  }
  bool blocked = false;
  for (const auto &lit : unsimplified) {
    if (mark (-lit))
      blocked = true;
    mark (-lit) = false;
  }
  return blocked;
}

/*------------------------------------------------------------------------*/

void Checker::add_clause (const char *type) {
#ifndef LOGGING
  (void) type;
#endif

  // If there are enough garbage clauses collect them first.
  if (num_garbage > 0.5 * max ((size_t) size_clauses, (size_t) size_vars))
    collect_garbage_clauses ();

  int unit = 0;
  for (const auto &lit : simplified) {
    const signed char tmp = val (lit);
    if (tmp < 0)
      continue;
    CADICAL_assert (!tmp);
    if (unit) {
      unit = INT_MIN;
      break;
    }
    unit = lit;
  }

  if (simplified.empty ()) {
    LOG ("CHECKER added empty %s clause", type);
    inconsistent = true;
  }
  if (!unit) {
    LOG ("CHECKER added and checked falsified %s clause", type);
    inconsistent = true;
  } else if (unit != INT_MIN) {
    LOG ("CHECKER added and checked %s unit clause %d", type, unit);
    assign (unit);
    stats.units++;
    if (!propagate ()) {
      LOG ("CHECKER inconsistent after propagating %s unit", type);
      inconsistent = true;
    }
  } else
    insert ();
}

void Checker::add_original_clause (int64_t id, bool, const vector<int> &c,
                                   bool) {
  if (inconsistent)
    return;
  START (checking);
  LOG (c, "CHECKER addition of original clause");
  stats.added++;
  stats.original++;
  import_clause (c);
  last_id = id;
  if (tautological ())
    LOG ("CHECKER ignoring satisfied original clause");
  else
    add_clause ("original");
  simplified.clear ();
  unsimplified.clear ();
  STOP (checking);
}

void Checker::add_derived_clause (int64_t id, bool, const vector<int> &c,
                                  const vector<int64_t> &) {
  if (inconsistent)
    return;
  START (checking);
  LOG (c, "CHECKER addition of derived clause");
  stats.added++;
  stats.derived++;
  import_clause (c);
  last_id = id;
  if (tautological ())
    LOG ("CHECKER ignoring satisfied derived clause");
  else if (!check () && !check_blocked ()) { // needed for ER proof support
    fatal_message_start ();
    fputs ("failed to check derived clause:\n", stderr);
    for (const auto &lit : unsimplified)
      fprintf (stderr, "%d ", lit);
    fputc ('0', stderr);
    fatal_message_end ();
  } else
    add_clause ("derived");
  simplified.clear ();
  unsimplified.clear ();
  STOP (checking);
}

/*------------------------------------------------------------------------*/

void Checker::delete_clause (int64_t id, bool, const vector<int> &c) {
  if (inconsistent)
    return;
  START (checking);
  LOG (c, "CHECKER checking deletion of clause");
  stats.deleted++;
  simplified.clear ();   // Can be non-empty if clause allocation fails.
  unsimplified.clear (); // Can be non-empty if clause allocation fails.
  import_clause (c);
  last_id = id;
  if (!tautological ()) {
    CheckerClause **p = find (), *d = *p;
    if (d) {
      CADICAL_assert (d->size > 1);
      // Remove from hash table, mark as garbage, connect to garbage list.
      num_garbage++;
      CADICAL_assert (num_clauses);
      num_clauses--;
      *p = d->next;
      d->next = garbage;
      garbage = d;
      d->size = 0;
    } else {
      fatal_message_start ();
      fputs ("deleted clause not in proof:\n", stderr);
      for (const auto &lit : unsimplified)
        fprintf (stderr, "%d ", lit);
      fputc ('0', stderr);
      fatal_message_end ();
    }
  }
  simplified.clear ();
  unsimplified.clear ();
  STOP (checking);
}

void Checker::add_assumption_clause (int64_t id, const vector<int> &c,
                                     const vector<int64_t> &chain) {
  add_derived_clause (id, true, c, chain);
  delete_clause (id, true, c);
}

/*------------------------------------------------------------------------*/

void Checker::dump () {
  int max_var = 0;
  for (uint64_t i = 0; i < size_clauses; i++)
    for (CheckerClause *c = clauses[i]; c; c = c->next)
      for (unsigned i = 0; i < c->size; i++)
        if (abs (c->literals[i]) > max_var)
          max_var = abs (c->literals[i]);
  printf ("p cnf %d %" PRIu64 "\n", max_var, num_clauses);
  for (uint64_t i = 0; i < size_clauses; i++)
    for (CheckerClause *c = clauses[i]; c; c = c->next) {
      for (unsigned i = 0; i < c->size; i++)
        printf ("%d ", c->literals[i]);
      printf ("0\n");
    }
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
