#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

// Equivalent literal substitution in 'decompose' and shrinking in 'subsume'
// or 'vivify' might produce duplicated binary clauses.  They can not be
// found in 'subsume' nor 'vivify' since we explicitly do not consider
// binary clauses as candidates to be shrunken or subsumed.  They are
// detected here by a simple scan of watch lists and then marked as garbage.
// This is actually also quite fast.

// Further it might also be possible that two binary clauses can be resolved
// to produce a unit (we call it 'hyper unary resolution').  For example
// resolving the binary clauses '1 -2' and '1 2' produces the unit '1'.
// This could be found by probing in 'probe' unless '-1' also occurs in a
// binary clause (add the clause '-1 2' to those two clauses) in which case
// '1' as well as '2' both occur positively as well as negatively and none
// of them nor their negation is considered as probe

void Internal::mark_duplicated_binary_clauses_as_garbage () {

  if (!opts.deduplicate)
    return;
  if (unsat)
    return;
  if (terminated_asynchronously ())
    return;

  START_SIMPLIFIER (deduplicate, DEDUP);
  stats.deduplications++;

  CADICAL_assert (!level);
  CADICAL_assert (watching ());

  vector<int> stack; // To save marked literals and unmark them later.

  int64_t subsumed = 0;
  int64_t units = 0;

  for (auto idx : vars) {

    if (unsat)
      break;
    if (!active (idx))
      continue;
    int unit = 0;

    for (int sign = -1; !unit && sign <= 1; sign += 2) {

      const int lit = sign * idx; // Consider all literals.

      CADICAL_assert (stack.empty ());
      Watches &ws = watches (lit);

      // We are removing references to garbage clause. Thus no 'auto'.

      const const_watch_iterator end = ws.end ();
      watch_iterator j = ws.begin ();
      const_watch_iterator i;

      for (i = j; !unit && i != end; i++) {
        Watch w = *j++ = *i;
        if (!w.binary ())
          continue;
        int other = w.blit;
        const int tmp = marked (other);
        Clause *c = w.clause;

        if (tmp > 0) { // Found duplicated binary clause.

          if (c->garbage) {
            j--;
            continue;
          }
          LOG (c, "found duplicated");

          // The previous identical clause 'd' might be redundant and if the
          // second clause 'c' is not (so irredundant), then we have to keep
          // 'c' instead of 'd', thus we search for it and replace it.

          if (!c->redundant) {
            watch_iterator k;
            for (k = ws.begin ();; k++) {
              CADICAL_assert (k != i);
              if (!k->binary ())
                continue;
              if (k->blit != other)
                continue;
              Clause *d = k->clause;
              if (d->garbage)
                continue;
              c = d;
              break;
            }
            *k = w;
          }

          LOG (c, "mark garbage duplicated");
          stats.subsumed++;
          stats.deduplicated++;
          subsumed++;
          mark_garbage (c);
          j--;

        } else if (tmp < 0) { // Hyper unary resolution.

          LOG ("found %d %d and %d %d which produces unit %d", lit, -other,
               lit, other, lit);
          unit = lit;
          if (lrat) {
            // taken from fradical
            CADICAL_assert (lrat_chain.empty ());
            lrat_chain.push_back (c->id);
            // We've forgotten where the other binary clause is, so go find
            // it again
            for (watch_iterator k = ws.begin ();; k++) {
              CADICAL_assert (k != i);
              if (!k->binary ())
                continue;
              if (k->blit != -other)
                continue;
              lrat_chain.push_back (k->clause->id);
              break;
            }
          }
          j = ws.begin (); // Flush 'ws'.
          units++;

        } else {
          if (c->garbage)
            continue;
          mark (other);
          stack.push_back (other);
        }
      }

      if (j == ws.begin ())
        erase_vector (ws);
      else if (j != end)
        ws.resize (j - ws.begin ()); // Shrink watchers.

      for (const auto &other : stack)
        unmark (other);

      stack.clear ();
    }

    // Propagation potentially messes up the watches and thus we can not
    // propagate the unit immediately after finding it.  Instead we break
    // out of both loops and assign and propagate the unit here.

    if (unit) {

      stats.failed++;
      stats.hyperunary++;
      assign_unit (unit);
      // lrat_chain.clear ();   done in search_assign

      if (!propagate ()) {
        LOG ("empty clause after propagating unit");
        learn_empty_clause ();
      }
    }
  }
  STOP_SIMPLIFIER (deduplicate, DEDUP);

  report ('2', !opts.reportall && !(subsumed + units));
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
