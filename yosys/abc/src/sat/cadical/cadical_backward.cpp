#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Provide eager backward subsumption for resolved clauses.

// The eliminator maintains a queue of clauses that are new and have to be
// checked to subsume or strengthen other (longer or same size) clauses.

void Eliminator::enqueue (Clause *c) {
  if (!internal->opts.elimbackward)
    return;
  if (c->enqueued)
    return;
  LOG (c, "backward enqueue");
  backward.push (c);
  c->enqueued = true;
}

Clause *Eliminator::dequeue () {
  if (backward.empty ())
    return 0;
  Clause *res = backward.front ();
  backward.pop ();
  CADICAL_assert (res->enqueued);
  res->enqueued = false;
  LOG (res, "backward dequeue");
  return res;
}

Eliminator::~Eliminator () {
  while (dequeue ())
    ;
}

/*------------------------------------------------------------------------*/

void Internal::elim_backward_clause (Eliminator &eliminator, Clause *c) {
  CADICAL_assert (opts.elimbackward);
  CADICAL_assert (!c->redundant);
  if (c->garbage)
    return;
  LOG (c, "attempting backward subsumption and strengthening with");
  size_t len = UINT_MAX;
  unsigned size = 0;
  int best = 0;
  bool satisfied = false;
  CADICAL_assert (mini_chain.empty ());
  for (const auto &lit : *c) {
    const signed char tmp = val (lit);
    if (tmp > 0) {
      satisfied = true;
      break;
    }
    if (tmp < 0)
      continue;
    size_t l = occs (lit).size ();
    LOG ("literal %d occurs %zd times", lit, l);
    if (l < len)
      best = lit, len = l;
    mark (lit);
    size++;
  }
  if (satisfied) {
    LOG ("clause actually already satisfied");
    elim_update_removed_clause (eliminator, c);
    mark_garbage (c);
  } else if (len > (size_t) opts.elimocclim) {
    LOG ("skipping backward subsumption due to too many occurrences");
  } else {
    CADICAL_assert (len);
    LOG ("literal %d has smallest number of occurrences %zd", best, len);
    LOG ("marked %d literals in clause of size %d", size, c->size);
    for (auto &d : occs (best)) {
      if (d == c)
        continue;
      if (d->garbage)
        continue;
      if ((unsigned) d->size < size)
        continue;
      int negated = 0;
      unsigned found = 0;
      satisfied = false;
      for (const auto &lit : *d) {
        signed char tmp = val (lit);
        if (tmp > 0) {
          satisfied = true;
          break;
        }
        if (tmp < 0)
          continue;
        tmp = marked (lit);
        if (!tmp)
          continue;
        if (tmp < 0) {
          if (negated) {
            size = UINT_MAX;
            break;
          } else
            negated = lit;
        }
        if (++found == size)
          break;
      }
      if (satisfied) {
        LOG (d, "found satisfied clause");
        elim_update_removed_clause (eliminator, d);
        mark_garbage (d);
      } else if (found == size) {
        if (!negated) {
          LOG (d, "found subsumed clause");
          elim_update_removed_clause (eliminator, d);
          mark_garbage (d);
          stats.subsumed++;
          stats.elimbwsub++;
        } else {
          int unit = 0;
          CADICAL_assert (minimize_chain.empty ());
          CADICAL_assert (analyzed.empty ());
          CADICAL_assert (lrat_chain.empty ());
          // figure out wether we strengthen c or get a new unit
          for (const auto &lit : *d) {
            const signed char tmp = val (lit);
            if (tmp < 0) {
              if (!lrat)
                continue;
              Flags &f = flags (lit);
              CADICAL_assert (!f.seen);
              if (f.seen)
                continue;
              f.seen = true;
              analyzed.push_back (lit);
              continue;
            }
            if (tmp > 0) {
              satisfied = true;
              break;
            }
            if (lit == negated)
              continue;
            if (unit) {
              unit = INT_MIN;
              continue; // needed to guarantee d is not satsified
            } else
              unit = lit;
          }
          if (lrat && !satisfied) {
            // if we found a unit we need to add all unit ids from
            // {c\d}U{d\c} otherwise just the unit ids from {c\d}
            for (const auto &lit : *c) {
              const signed char tmp = val (lit);
              CADICAL_assert (tmp <= 0);
              if (tmp >= 0)
                continue;
              Flags &f = flags (lit);
              if (f.seen && unit && unit == INT_MIN) {
                f.seen = false;
                continue;
              } else if (!f.seen) {
                f.seen = true;
                analyzed.push_back (lit);
              }
            }
            if (unit == INT_MIN) { // we do not need units from {d\c}
              for (const auto &lit : *d) {
                flags (lit).seen = false;
              }
            }
            for (const auto &lit : analyzed) {
              Flags &f = flags (lit);
              if (!f.seen) {
                f.seen = true;
                continue;
              }
              int64_t id = unit_id (-lit);
              lrat_chain.push_back (id);
            }
            clear_analyzed_literals ();
            lrat_chain.push_back (d->id);
            lrat_chain.push_back (c->id);
          } else if (lrat)
            clear_analyzed_literals ();
          if (satisfied) {
            CADICAL_assert (lrat_chain.empty ());
            mark_garbage (d);
            elim_update_removed_clause (eliminator, d);
          } else if (unit && unit != INT_MIN) {
            CADICAL_assert (unit);
            LOG (d, "unit %d through hyper unary resolution with", unit);
            assign_unit (unit);
            elim_propagate (eliminator, unit);
            lrat_chain.clear ();
            break;
          } else if (occs (negated).size () <= (size_t) opts.elimocclim) {
            strengthen_clause (d, negated);
            remove_occs (occs (negated), d);
            elim_update_removed_lit (eliminator, negated);
            stats.elimbwstr++;
            CADICAL_assert (negated != best);
            eliminator.enqueue (d);
          }
          lrat_chain.clear ();
        }
      }
    }
  }
  mini_chain.clear ();
  unmark (c);
}

/*------------------------------------------------------------------------*/

void Internal::elim_backward_clauses (Eliminator &eliminator) {
  if (!opts.elimbackward) {
    CADICAL_assert (eliminator.backward.empty ());
    return;
  }
  START (backward);
  LOG ("attempting backward subsumption and strengthening with %zd clauses",
       eliminator.backward.size ());
  Clause *c;
  while (!unsat && (c = eliminator.dequeue ()))
    elim_backward_clause (eliminator, c);
  STOP (backward);
}

/*------------------------------------------------------------------------*/

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
