#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

bool Internal::flip (int lit) {

  // Do not try to flip inactive literals except for unused variables.

  if (!active (lit) && !flags (lit).unused ())
    return false;

  /*
  if (flags (lit).unused ()) {
    CADICAL_assert (lit <= max_var);
    mark_active (lit);
    set_val (lit, 1);
    return true;
  }
  */

  // TODO: Unused case is not handled yet.
  // if (flags (lit).unused ()) return false;

  // Need to reestablish proper watching invariants as if there are no
  // blocking literals as flipping in principle does not work with them.

  if (propergated < trail.size ())
    propergate ();

  LOG ("trying to flip %d", lit);

  const int idx = vidx (lit);
  const signed char original_value = vals[idx];
  CADICAL_assert (original_value);
  lit = original_value < 0 ? -idx : idx;
  CADICAL_assert (val (lit) > 0);

  // Here we go over all the clauses in which 'lit' is watched by 'lit' and
  // check whether assigning 'lit' to false would break watching invariants
  // or even make the clause false.  We also try to find replacement
  // watches in case this fixes the watching invariant.  This code is very
  // similar to propagation of a literal in 'Internal::propagate'.

  bool res = true;

  Watches &ws = watches (lit);
  const const_watch_iterator eow = ws.end ();
  watch_iterator bow = ws.begin ();

  // We first go over binary watches/clauses first as this is cheaper and
  // has higher chance of failure and we can not use blocking literals.

  for (const_watch_iterator i = bow; i != eow; i++) {
    const Watch w = *i;
    if (!w.binary ())
      continue;
    const signed char b = val (w.blit);
    if (b > 0)
      continue;
    CADICAL_assert (b < 0);
    res = false;
    break;
  }

  if (res) {
    const_watch_iterator i = bow;
    watch_iterator j = bow;

    while (i != eow) {

      const Watch w = *j++ = *i++;

      if (w.binary ())
        continue;

      if (w.clause->garbage) {
        j--;
        continue;
      }

      literal_iterator lits = w.clause->begin ();

      const int other = lits[0] ^ lits[1] ^ lit;
      const signed char u = val (other);
      if (u > 0)
        continue;

      const int size = w.clause->size;
      const literal_iterator middle = lits + w.clause->pos;
      const const_literal_iterator end = lits + size;
      literal_iterator k = middle;

      int r = 0;
      signed char v = -1;
      while (k != end && (v = val (r = *k)) < 0)
        k++;
      if (v < 0) {
        k = lits + 2;
        CADICAL_assert (w.clause->pos <= size);
        while (k != middle && (v = val (r = *k)) < 0)
          k++;
      }

      if (v < 0) {
        res = false;
        break;
      }

      CADICAL_assert (v > 0);
      CADICAL_assert (lits + 2 <= k), CADICAL_assert (k <= w.clause->end ());
      w.clause->pos = k - lits;
      lits[0] = other, lits[1] = r, *k = lit;
      watch_literal (r, lit, w.clause);
      j--;
    }

    if (j != i) {

      while (i != eow)
        *j++ = *i++;

      ws.resize (j - ws.begin ());
    }
  }
#ifdef LOGGING
  if (res)
    LOG ("literal %d can be flipped", lit);
  else
    LOG ("literal %d can not be flipped", lit);
#endif

  if (res) {

    const int idx = vidx (lit);
    const signed char original_value = vals[idx];
    CADICAL_assert (original_value);
    lit = original_value < 0 ? -idx : idx;
    CADICAL_assert (val (lit) > 0);

    LOG ("flipping value of %d = 1 to %d = -1", lit, lit);

    set_val (idx, -original_value);
    CADICAL_assert (val (-lit) > 0);
    CADICAL_assert (val (lit) < 0);

    Var &v = var (idx);
    CADICAL_assert (trail[v.trail] == lit);
    trail[v.trail] = -lit;
    if (opts.ilb) {
      if (!tainted_literal)
        tainted_literal = lit;
      else {
        CADICAL_assert (val (tainted_literal));
        if (v.level < var (tainted_literal).level) {
          tainted_literal = lit;
        }
      }
    }
  } else
    LOG ("flipping value of %d failed", lit);

  return res;
}

bool Internal::flippable (int lit) {

  // Can not check inactive literals except for unused variables.

  if (!active (lit) && !flags (lit).unused ())
    return false;

  /*
  if (flags (lit).unused ()) {
    CADICAL_assert (lit <= max_var);
    mark_active (lit);
    return true;
  }
  */
  // TODO: Unused case is not handled yet
  // if (flags (lit).unused ()) return false;

  // Need to reestablish proper watching invariants as if there are no
  // blocking literals as flipping in principle does not work with them.

  if (propergated < trail.size ())
    propergate ();

  LOG ("checking whether %d is flippable", lit);

  const int idx = vidx (lit);
  const signed char original_value = vals[idx];
  CADICAL_assert (original_value);
  lit = original_value < 0 ? -idx : idx;
  CADICAL_assert (val (lit) > 0);

  // Here we go over all the clauses in which 'lit' is watched by 'lit' and
  // check whether assigning 'lit' to false would break watching invariants
  // or even make the clause false.  In contrast to 'flip' we do not try to
  // find replacement literals but do use blocking literals'.  Therefore we
  // also do not split the traversal code into two parts.

  bool res = true;

  Watches &ws = watches (lit);
  const const_watch_iterator eow = ws.end ();
  for (watch_iterator i = ws.begin (); i != eow; i++) {

    const Watch w = *i;
    const signed char b = val (w.blit);
    if (b > 0)
      continue;
    CADICAL_assert (b < 0);

    if (w.binary ()) {
      res = false;
      break;
    }

    if (w.clause->garbage)
      continue;

    literal_iterator lits = w.clause->begin ();

    const int other = lits[0] ^ lits[1] ^ lit;
    const signed char u = val (other);
    if (u > 0) {
      i->blit = other;
      continue;
    }

    const int size = w.clause->size;
    const literal_iterator middle = lits + w.clause->pos;
    const const_literal_iterator end = lits + size;
    literal_iterator k = middle;

    int r = 0;
    signed char v = -1;
    while (k != end && (v = val (r = *k)) < 0)
      k++;
    if (v < 0) {
      k = lits + 2;
      CADICAL_assert (w.clause->pos <= size);
      while (k != middle && (v = val (r = *k)) < 0)
        k++;
    }

    if (v < 0) {
      res = false;
      break;
    }

    CADICAL_assert (v > 0);
    CADICAL_assert (lits + 2 <= k);
    CADICAL_assert (k <= w.clause->end ());
    w.clause->pos = k - lits;
    i->blit = r;
  }

#ifdef LOGGING
  if (res)
    LOG ("literal %d can be flipped", lit);
  else
    LOG ("literal %d can not be flipped", lit);
#endif

  return res;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
