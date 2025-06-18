#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Globally blocked clause elimination (which we call here 'conditioning')
// is described first in the PhD thesis of Benjamin Kiesl from 2019.  An
// extended version, which in particular describes the algorithm implemented
// below is in our invited ATVA'19 paper [KieslHeuleBiere-ATVA'19].  This
// accordingly needs witnesses consisting potentially of more than one
// literal.  It is the first technique implemented in CaDiCaL with this
// feature (PR clause elimination thus should work in principle too).

// Basically globally blocked clauses are like set blocked clauses, except
// that the witness cube (of literals to be flipped during reconstruction)
// can contain variables which are not in the blocked clause.  This
// can simulate some interesting global optimizations like 'headlines' from
// the FAN algorithm for ATPG.  The technique was actually motivated to
// simulate this optimization.  It turns out that globally blocked clauses
// can be seen as 'conditional autarkies', where in essence the condition
// cube is the negation of the globally blocked redundant clause (it
// needs to contain one autarky literal though) and the autarky part
// represents the witness.

/*------------------------------------------------------------------------*/

// Elimination of globally blocked clauses is first tried in regular
// intervals in terms of the number of conflicts.  Then the main heuristics
// is to trigger 'condition' if the decision level is above the current
// moving average of the back jump level.

// TODO We might need to consider less frequent conditioning.

bool Internal::conditioning () {

  if (!opts.condition)
    return false;
  if (!preprocessing && !opts.inprocessing)
    return false;
  if (preprocessing)
    CADICAL_assert (lim.preprocessing);

  // Triggered in regular 'opts.conditionint' conflict intervals.
  //
  if (lim.condition > stats.conflicts)
    return false;

  if (!level)
    return false; // One decision necessary.

  if (level <= averages.current.jump)
    return false; // Main heuristic.

  if (!stats.current.irredundant)
    return false;
  double remain = active ();
  if (!remain)
    return false;
  double ratio = stats.current.irredundant / remain;
  return ratio <= opts.conditionmaxrat;
}

/*------------------------------------------------------------------------*/

// We start with the current assignment and then temporarily unassign
// literals.  They are reassigned afterwards.  The global state of the CDCL
// solver should not change though.  Thus we copied from 'search_unassign'
// in 'backtrack.cpp' what is needed to unassign literals and then from
// 'search_assign' in 'propagate.cpp' what is needed for reassigning
// literals, but restricted the copied code to only updating the actual
// assignment (in 'vals') and not changing anything else.

// We use temporarily unassigning for two purposes.  First, if a conditional
// literal does not occur negated in a candidate clause it is unassigned.
// Second, as a minor optimization, we first unassign all root-level
// assigned (fixed) literals, to avoid checking the decision level of
// literals during the procedure.

void Internal::condition_unassign (int lit) {
  LOG ("condition unassign %d", lit);
  CADICAL_assert (val (lit) > 0);
  set_val (lit, 0);
}

void Internal::condition_assign (int lit) {
  LOG ("condition assign %d", lit);
  CADICAL_assert (!val (lit));
  set_val (lit, 1);
}

/*------------------------------------------------------------------------*/

// The current partition into conditional part and autarky part during
// refinement is represented through a conditional bit in 'marks'.

inline bool Internal::is_conditional_literal (int lit) const {
  return val (lit) > 0 && getbit (lit, 0);
}

inline bool Internal::is_autarky_literal (int lit) const {
  return val (lit) > 0 && !getbit (lit, 0);
}

inline void Internal::mark_as_conditional_literal (int lit) {
  LOG ("marking %d as conditional literal", lit);
  CADICAL_assert (val (lit) > 0);
  setbit (lit, 0);
  CADICAL_assert (is_conditional_literal (lit));
  CADICAL_assert (!is_autarky_literal (lit));
}

inline void Internal::unmark_as_conditional_literal (int lit) {
  LOG ("unmarking %d as conditional literal", lit);
  CADICAL_assert (is_conditional_literal (lit));
  unsetbit (lit, 0);
}

/*------------------------------------------------------------------------*/

// We also need to know the literals which are in the current clause.  These
// are just marked (also in 'marks' but with the (signed) upper two bits).
// We need a signed mark here, since we have to distinguish positive and
// negative occurrences of literals in the candidate clause.

inline bool Internal::is_in_candidate_clause (int lit) const {
  return marked67 (lit) > 0;
}

inline void Internal::mark_in_candidate_clause (int lit) {
  LOG ("marking %d as literal of the candidate clause", lit);
  mark67 (lit);
  CADICAL_assert (is_in_candidate_clause (lit));
  CADICAL_assert (!is_in_candidate_clause (-lit));
}

inline void Internal::unmark_in_candidate_clause (int lit) {
  LOG ("unmarking %d as literal of the candidate clause", lit);
  CADICAL_assert (is_in_candidate_clause (lit));
  unmark67 (lit);
}

/*------------------------------------------------------------------------*/

struct less_conditioned {
  bool operator() (Clause *a, Clause *b) {
    return !a->conditioned && b->conditioned;
  }
};

// This is the function for eliminating globally blocked clauses.  It is
// triggered during CDCL search according to 'conditioning' above and uses
// the current assignment as basis to find globally blocked clauses.

long Internal::condition_round (long delta) {

  long limit;
#ifndef CADICAL_QUIET
  long props = 0;
#endif
  if (LONG_MAX - delta < stats.condprops)
    limit = LONG_MAX;
  else
    limit = stats.condprops + delta;

  size_t initial_trail_level = trail.size ();
  int initial_level = level;

  LOG ("initial trail level %zd", initial_trail_level);

  protect_reasons ();

#if defined(LOGGING) || !defined(CADICAL_NDEBUG)
  int additionally_assigned = 0;
#endif

  for (auto idx : vars) {
    const signed char tmp = val (idx);
    Var &v = var (idx);
    if (tmp) {
      if (v.level) {
        const int lit = tmp < 0 ? -idx : idx;
        if (!active (idx)) {
          LOG ("temporarily unassigning inactive literal %d", lit);
          condition_unassign (lit);
        }
        if (frozen (idx)) {
          LOG ("temporarily unassigning frozen literal %d", lit);
          condition_unassign (lit);
        }
      }
    } else if (frozen (idx)) {
      LOG ("keeping frozen literal %d unassigned", idx);
    } else if (!active (idx)) {
      LOG ("keeping inactive literal %d unassigned", idx);
    } else { // if (preprocessing) {
      if (initial_level == level) {
        level++;
        LOG ("new condition decision level");
      }
      const int lit = decide_phase (idx, true);
      condition_assign (lit);
      v.level = level;
      trail.push_back (lit);
#if defined(LOGGING) || !defined(CADICAL_NDEBUG)
      additionally_assigned++;
#endif
    }
  }
  LOG ("assigned %d additional literals", additionally_assigned);

  // We compute statistics about the size of the assignments.
  //
  // The initial assignment consists of the non-root-level assigned literals
  // split into a conditional and an autarky part.  The conditional part
  // consists of literals assigned true and occurring negated in a clause
  // (touch the clause), which does not contain another literal assigned to
  // true.  This initial partition is the same for all refinements used in
  // checking whether a candidate clause is globally blocked.
  //
  // For each candidate clause some of the conditional literals have to be
  // unassigned, and the autarky is shrunken by turning some of the autarky
  // literals into conditional literals (which might get unassigned in a
  // later refinement though).
  //
  // The fix-point of this procedure produces a final assignment, which
  // consists of the remaining assigned literals, again split into a
  // conditional and an autarky part.
  //
  struct {
    size_t assigned, conditional, autarky;
  } initial, remain;

  initial.assigned = 0;
  for (auto idx : vars) {
    const signed char tmp = val (idx);
    if (!tmp)
      continue;
    if (!var (idx).level)
      continue;
    LOG ("initial assignment %ds", tmp < 0 ? -idx : idx);
    initial.assigned++;
  }

  PHASE ("condition", stats.conditionings, "initial assignment of size %zd",
         initial.assigned);

  // For each candidate clause we refine the assignment (monotonically),
  // by unassigning some conditional literals and turning some autarky
  // literals into conditionals.
  //
  // As the conditional part is usually smaller than the autarky part our
  // implementation only explicitly maintains the initial conditional part,
  // with conditional bit set to true through 'mark_as_conditional_literal'.
  // The autarky part consists of all literals assigned true which do not
  // have their conditional bit set to true.  Since in both cases the
  // literal has to be assigned true, we only need a single bit for both the
  // literal as well as its negation (it does not have to be 'signed').
  //
  vector<int> conditional;

  vector<Clause *> candidates; // Gather candidate clauses.
#ifndef CADICAL_QUIET
  size_t watched = 0; // Number of watched clauses.
#endif

  initial.autarky = initial.assigned; // Initially all are in autarky
  initial.conditional = 0;            // and none in conditional part.

  // Upper bound on the number of watched clauses. In principle one could
  // use 'SIZE_MAX' but this is not available by default (yet).
  //
  const size_t size_max = clauses.size () + 1;

  // Initialize additional occurrence lists.
  //
  init_occs ();

  // Number of previously conditioned and unconditioned candidates.
  //
  size_t conditioned = 0, unconditioned = 0;

  // Now go over all (non-garbage) irredundant clauses and check whether
  // they are candidates, have to be watched, or whether they force the
  // negation of some of their literals to be conditional initially.
  //
  for (const auto &c : clauses) {
    if (c->garbage)
      continue; // Can already be ignored.
    if (c->redundant)
      continue; // Ignore redundant clauses too.

    // First determine the following numbers for the candidate clause
    // (restricted to non-root-level assignments).
    //
    int positive = 0; // Number true literals.
    int negative = 0; // Number false literals.
    int watch = 0;    // True Literal to watch.
    //
    size_t minsize = size_max; // Number of occurrences of 'watch'.
    //
    // But also ignore root-level satisfied but not yet garbage clauses.
    //
    bool satisfied = false; // Root level satisfied.
    //
    for (const_literal_iterator l = c->begin ();
         !satisfied && l != c->end (); l++) {
      const int lit = *l;
      const signed char tmp = val (lit);
      if (tmp && !var (lit).level)
        satisfied = (tmp > 0);
      else if (tmp < 0)
        negative++;
      else if (tmp > 0) {
        const size_t size = occs (lit).size ();
        if (size < minsize)
          watch = lit, minsize = size;
        positive++;
      }
    }
    if (satisfied) {    // Ignore root-level satisfied clauses.
      mark_garbage (c); // But mark them as garbage already now.
      continue;         // ... with next clause 'c'.
    }

    // Candidates are clauses with at least a positive literal in it.
    //
    if (positive > 0) {
      LOG (c, "found %d positive literals in candidate", positive);
      candidates.push_back (c);
      if (c->conditioned)
        conditioned++;
      else
        unconditioned++;
    }

    // Only one positive literal in each clauses with also at least one
    // negative literal has to be watched in occurrence lists.  These
    // watched clauses will be checked to contain only negative literals as
    // soon such a positive literal is unassigned.  If this is the case
    // these false literals have to be unassigned and potentially new
    // conditional literals have to be determined.
    //
    // Note that only conditional literals are unassigned. However it does
    // not matter that we might also watch autarky literals, because either
    // such an autarky literal remains a witness that the clause is
    // satisfied as long it remains an autarky literal. Otherwise at one
    // point it becomes conditional and is unassigned, but then a
    // replacement watch will be searched.
    //
    if (negative > 0 && positive > 0) {
      LOG (c, "found %d negative literals in candidate", negative);
      CADICAL_assert (watch);
      CADICAL_assert (val (watch) > 0);
      Occs &os = occs (watch);
      CADICAL_assert (os.size () == minsize);
      os.push_back (c);
#ifndef CADICAL_QUIET
      watched++;
#endif
      LOG (c, "watching %d with %zd occurrences in", watch, minsize);
    }

    // The initial global conditional part for the current assignment is
    // extracted from clauses with only negative literals.  It is the same
    // for all considered candidate clauses. These negative literals make up
    // the global conditional part, are marked here.
    //
    if (negative > 0 && !positive) {

      size_t new_conditionals = 0;

      for (const_literal_iterator l = c->begin (); l != c->end (); l++) {
        const int lit = *l;
        signed char tmp = val (lit);
        if (!tmp)
          continue;
        CADICAL_assert (tmp < 0);
        if (!var (lit).level)
          continue; // Not unassigned yet!
        if (is_conditional_literal (-lit))
          continue;
        mark_as_conditional_literal (-lit);
        conditional.push_back (-lit);
        new_conditionals++;
      }
      if (new_conditionals > 0)
        LOG (c, "marked %zu negations of literals as conditional in",
             new_conditionals);

      initial.conditional += new_conditionals;
      CADICAL_assert (initial.autarky >= new_conditionals);
      initial.autarky -= new_conditionals;
    }

  } // End of loop over all clauses to collect candidates etc.

  PHASE ("condition", stats.conditionings, "found %zd candidate clauses",
         candidates.size ());
  PHASE ("condition", stats.conditionings,
         "watching %zu literals and clauses", watched);
  PHASE ("condition", stats.conditionings,
         "initially %zd conditional literals %.0f%%", initial.conditional,
         percent (initial.conditional, initial.assigned));
  PHASE ("condition", stats.conditionings,
         "initially %zd autarky literals %.0f%%", initial.autarky,
         percent (initial.autarky, initial.assigned));
#ifdef LOGGING
  for (size_t i = 0; i < conditional.size (); i++) {
    LOG ("initial conditional %d", conditional[i]);
    CADICAL_assert (is_conditional_literal (conditional[i]));
  }
  for (size_t i = 0; i < trail.size (); i++)
    if (is_autarky_literal (trail[i]))
      LOG ("initial autarky %d", trail[i]);
#endif
  CADICAL_assert (initial.conditional == conditional.size ());
  CADICAL_assert (initial.assigned == initial.conditional + initial.autarky);

  stats.condassinit += initial.assigned;
  stats.condcondinit += initial.conditional;
  stats.condautinit += initial.autarky;
  stats.condassvars += active ();

  // To speed-up and particularly simplify the code we unassign all
  // root-level variables temporarily, actually all inactive assigned
  // variables.  This allows us to avoid tests on whether an assigned
  // literal is actually root-level assigned and thus should be ignored (not
  // considered to be assigned).  For this to work we have to ignore root
  // level satisfied clauses as done above.  These are neither candidates
  // nor have to be watched.  Remaining originally root-level assigned
  // literals in clauses are only set to false.
  //
  for (const auto &lit : trail)
    if (fixed (lit))
      condition_unassign (lit);

  // Stack to save temporarily unassigned (conditional) literals.
  //
  vector<int> unassigned;

  // Make sure to focus on clauses not tried before by marking clauses which
  // have been checked before using the 'conditioned' bit of clauses. If all
  // candidates have their bit set, we have to reset it.  Since the
  // assignment might be completely different then last time and thus also
  // the set of candidates this method does not really exactly lead to a
  // round robin scheme of scheduling clauses.
  //
  // TODO consider computing conditioned and unconditioned over all clauses.
  //
  CADICAL_assert (conditioned + unconditioned == candidates.size ());
  if (conditioned && unconditioned) {
    stable_sort (candidates.begin (), candidates.end (),
                 less_conditioned ());
    PHASE ("condition", stats.conditionings,
           "focusing on %zd candidates %.0f%% not tried last time",
           unconditioned, percent (unconditioned, candidates.size ()));
  } else if (conditioned && !unconditioned) {
    for (auto const &c : candidates) {
      CADICAL_assert (c->conditioned);
      c->conditioned = false; // Reset 'conditioned' bit.
    }
    PHASE ("condition", stats.conditionings,
           "all %zd candidates tried before", conditioned);
  } else {
    CADICAL_assert (!conditioned);
    PHASE ("condition", stats.conditionings, "all %zd candidates are fresh",
           unconditioned);
  }

  // TODO prune assignments further!
  // And thus might result in less watched clauses.
  // So watching should be done here and not earlier.
  // Also, see below, we might need to consider the negation of unassigned
  // literals in candidate clauses as being watched.

  // Now try to block all candidate clauses.
  //
  long blocked = 0; // Number of Successfully blocked clauses.
  //
#ifndef CADICAL_QUIET
  size_t untried = candidates.size ();
#endif
  for (const auto &c : candidates) {

    if (initial.autarky <= 0)
      break;

    if (c->reason)
      continue;

    bool terminated_or_limit_hit = true;
    if (terminated_asynchronously ())
      LOG ("asynchronous termination detected");
    else if (stats.condprops >= limit)
      LOG ("condition propagation limit %ld hit", limit);
    else
      terminated_or_limit_hit = false;

    if (terminated_or_limit_hit) {
      PHASE ("condition", stats.conditionings,
             "%zd candidates %.0f%% not tried after %ld propagations",
             untried, percent (untried, candidates.size ()), props);
      break;
    }
#ifndef CADICAL_QUIET
    untried--;
#endif
    CADICAL_assert (!c->garbage);
    CADICAL_assert (!c->redundant);

    LOG (c, "candidate");
    c->conditioned = 1; // Next time later.

    // We watch an autarky literal in the clause, and can stop trying to
    // globally block the clause as soon it turns into a conditional
    // literal and we can not find another one.  If the fix-point assignment
    // is reached and we still have an autarky literal left the watched one
    // is reported as witness for this clause being globally blocked.
    //
    int watched_autarky_literal = 0;

    // First mark all true literals in the candidate clause and find an
    // autarky literal which witnesses that this clause has still a chance
    // to be globally blocked.
    //
    for (const_literal_iterator l = c->begin (); l != c->end (); l++) {
      const int lit = *l;
      mark_in_candidate_clause (lit);
      if (watched_autarky_literal)
        continue;
      if (!is_autarky_literal (lit))
        continue;
      watched_autarky_literal = lit;

      // TODO assign non-assigned literals to false?
      // Which might need to trigger watching additional clauses.
    }

    if (!watched_autarky_literal) {
      LOG ("no initial autarky literal found");
      for (const_literal_iterator l = c->begin (); l != c->end (); l++)
        unmark_in_candidate_clause (*l);
      continue;
    }

    stats.condcands++; // Only now ...

    LOG ("watching first autarky literal %d", watched_autarky_literal);

    // Save assignment sizes for statistics, logging and checking.
    //
    remain = initial;

    // Position of next conditional and unassigned literal to process in the
    // 'conditional' and the 'unassigned' stack.
    //
    struct {
      size_t conditional, unassigned;
    } next = {0, 0};

    CADICAL_assert (unassigned.empty ());
    CADICAL_assert (conditional.size () == initial.conditional);

    while (watched_autarky_literal && stats.condprops < limit &&
           next.conditional < conditional.size ()) {

      CADICAL_assert (next.unassigned == unassigned.size ());

      const int conditional_lit = conditional[next.conditional++];
      LOG ("processing next conditional %d", conditional_lit);
      CADICAL_assert (is_conditional_literal (conditional_lit));

      if (is_in_candidate_clause (-conditional_lit)) {
        LOG ("conditional %d negated in candidate clause", conditional_lit);
        continue;
      }

      LOG ("conditional %d does not occur negated in candidate clause",
           conditional_lit);

      condition_unassign (conditional_lit);
      CADICAL_assert (!is_conditional_literal (conditional_lit));
      unassigned.push_back (conditional_lit);

      CADICAL_assert (remain.assigned > 0);
      CADICAL_assert (remain.conditional > 0);
      remain.conditional--;
      remain.assigned--;

      while (watched_autarky_literal && stats.condprops < limit &&
             next.unassigned < unassigned.size ()) {
        const int unassigned_lit = unassigned[next.unassigned++];
        LOG ("processing next unassigned %d", unassigned_lit);
        CADICAL_assert (!val (unassigned_lit));
#ifndef CADICAL_QUIET
        props++;
#endif
        stats.condprops++;

        Occs &os = occs (unassigned_lit);
        if (os.empty ())
          continue;

        // Traverse all watched clauses of 'unassigned_lit' and find
        // replacement watches or if none is found turn the negation of all
        // false autarky literals in that clause into conditional literals.
        // If one of those autarky literals is the watched autarky literal
        // in the candidate clause, that one has to be updated too.
        //
        // We expect that this loop is a hot-spot for the procedure and thus
        // are more careful about accessing end points for iterating.
        //
        auto i = os.begin (), j = i;
        for (; watched_autarky_literal && j != os.end (); j++) {
          Clause *d = *i++ = *j;

          int replacement = 0; // New watched literal in 'd'.
          int negative = 0;    // Negative autarky literals in 'd'.

          for (const_literal_iterator l = d->begin (); l != d->end ();
               l++) {
            const int lit = *l;
            const signed char tmp = val (lit);
            if (tmp > 0)
              replacement = lit;
            if (tmp < 0 && is_autarky_literal (-lit))
              negative++;
          }

          if (replacement) {
            LOG ("found replacement %d for unassigned %d", replacement,
                 unassigned_lit);
            LOG (d, "unwatching %d in", unassigned_lit);
            i--; // Drop watch!
            LOG (d, "watching %d in", replacement);

            CADICAL_assert (replacement != unassigned_lit);
            occs (replacement).push_back (d);

            continue; // ... with next watched clause 'd'.
          }

          LOG ("no replacement found for unassigned %d", unassigned_lit);

          // Keep watching 'd' by 'unassigned_lit' if no replacement found.

          if (!negative) {
            LOG (d, "no negative autarky literals left in");
            continue; // ... with next watched clause 'd'.
          }

          LOG (d, "found %d negative autarky literals in", negative);

          for (const_literal_iterator l = d->begin ();
               watched_autarky_literal && l != d->end (); l++) {
            const int lit = *l;
            if (!is_autarky_literal (-lit))
              continue;
            mark_as_conditional_literal (-lit);
            conditional.push_back (-lit);

            remain.conditional++;
            CADICAL_assert (remain.autarky > 0);
            remain.autarky--;

            if (-lit != watched_autarky_literal)
              continue;

            LOG ("need to replace autarky literal %d in candidate", -lit);
            replacement = 0;

            // TODO save starting point because we only move it forward?

            for (const_literal_iterator k = c->begin ();
                 !replacement && k != c->end (); k++) {
              const int other = *k;
              if (is_autarky_literal (other))
                replacement = other;
            }
            watched_autarky_literal = replacement;

            if (replacement) {
              LOG (c, "watching autarky %d instead %d in candidate",
                   replacement, watched_autarky_literal);
              watched_autarky_literal = replacement;
            } else {
              LOG ("failed to find an autarky replacement");
              watched_autarky_literal = 0; // Breaks out of 4 loops!!!!!
            }
          } // End of loop of turning autarky literals into conditionals.
        } // End of loop of all watched clauses of an unassigned literal.
        //
        // We might abort the occurrence traversal early but already
        // removed some watches, thus have to just copy the rest.
        //
        if (i < j) {
          while (j != os.end ())
            *i++ = *j++;
          LOG ("flushed %zd occurrences of %d", os.end () - i,
               unassigned_lit);
          os.resize (i - os.begin ());
        }
      } // End of loop which goes over all unprocessed unassigned literals.
    } // End of loop which goes over all unprocessed conditional literals.

    // We are still processing the candidate 'c' and now have reached a
    // final fix-point assignment partitioned into a conditional and an
    // autarky part, or during unassigned literals figured that there is no
    // positive autarky literal left in 'c'.

    LOG ("remaining assignment of size %zd", remain.assigned);
    LOG ("remaining conditional part of size %zd", remain.conditional);
    LOG ("remaining autarky part of size %zd", remain.autarky);
    //
    CADICAL_assert (remain.assigned - remain.conditional == remain.autarky);
    //
#if defined(LOGGING) || !defined(CADICAL_NDEBUG)
    //
    // This is a sanity check, that the size of our implicit representation
    // of the autarky part matches our 'remain' counts.  We need the same
    // code for determining autarky literals as in the loop below which adds
    // autarky literals to the extension stack.
    //
    struct {
      size_t assigned, conditional, autarky;
    } check;
    check.assigned = check.conditional = check.autarky = 0;
    for (size_t i = 0; i < trail.size (); i++) {
      const int lit = trail[i];
      if (val (lit)) {
        check.assigned++;
        if (is_conditional_literal (lit)) {
          LOG ("remaining conditional %d", lit);
          CADICAL_assert (!is_autarky_literal (lit));
          check.conditional++;
        } else {
          CADICAL_assert (is_autarky_literal (lit));
          LOG ("remaining autarky %d", lit);
          check.autarky++;
        }
      } else {
        CADICAL_assert (!is_autarky_literal (lit));
        CADICAL_assert (!is_conditional_literal (lit));
      }
    }
    CADICAL_assert (remain.assigned == check.assigned);
    CADICAL_assert (remain.conditional == check.conditional);
    CADICAL_assert (remain.autarky == check.autarky);
#endif

    // Success if an autarky literal is left in the clause and
    // we did not abort the loop too early because the propagation
    // limit was hit.
    //
    if (watched_autarky_literal && stats.condprops < limit) {
      CADICAL_assert (is_autarky_literal (watched_autarky_literal));
      CADICAL_assert (is_in_candidate_clause (watched_autarky_literal));

      blocked++;
      stats.conditioned++;
      LOG (c, "positive autarky literal %d globally blocks",
           watched_autarky_literal);

      LOG ("remaining %zd assigned literals %.0f%%", remain.assigned,
           percent (remain.assigned, initial.assigned));
      LOG ("remaining %zd conditional literals %.0f%%", remain.conditional,
           percent (remain.conditional, remain.assigned));
      LOG ("remaining %zd autarky literals %.0f%%", remain.autarky,
           percent (remain.autarky, remain.assigned));

      // A satisfying assignment of a formula after removing a globally
      // blocked clause might not satisfy that clause.  As for variable
      // elimination and classical blocked clauses, we thus maintain an
      // extension stack for reconstructing an assignment which both
      // satisfies the remaining formula as well as the clause.
      //
      // For globally blocked clauses we simply have to flip all literals in
      // the autarky part and thus save the autarky on the extension stack
      // in addition to the removed clause.  In the classical situation (in
      // bounded variable elimination etc.) we simply save one literal on
      // the extension stack.
      //
      // TODO find a way to shrink the autarky part or some other way to
      // avoid pushing too many literals on the extension stack.
      //
      external->push_zero_on_extension_stack ();
      for (const auto &lit : trail)
        if (is_autarky_literal (lit))
          external->push_witness_literal_on_extension_stack (lit);
      if (proof)
        proof->weaken_minus (c);
      external->push_clause_on_extension_stack (c);

      mark_garbage (c);

      stats.condassrem += remain.assigned;
      stats.condcondrem += remain.conditional;
      stats.condautrem += remain.autarky;
      stats.condassirem += initial.assigned;
    }

    // In this last part specific to one candidate clause, we have to get
    // back to the initial assignment and reset conditionals.  First we
    // assign all the unassigned literals (if necessary).
    //
    if (!unassigned.empty ()) {
      LOG ("reassigning %zd literals", unassigned.size ());
      while (!unassigned.empty ()) {
        const int lit = unassigned.back ();
        unassigned.pop_back ();
        condition_assign (lit);
      }
    }

    // Then we remove from the conditional stack autarky literals which
    // became conditional and also reset their 'conditional' bit.
    //
    if (initial.conditional < conditional.size ()) {
      LOG ("flushing %zd autarky literals from conditional stack",
           conditional.size () - initial.conditional);
      while (initial.conditional < conditional.size ()) {
        const int lit = conditional.back ();
        conditional.pop_back ();
        unmark_as_conditional_literal (lit);
      }
    }

    // Finally unmark all literals in the candidate clause.
    //
    for (const_literal_iterator l = c->begin (); l != c->end (); l++)
      unmark_in_candidate_clause (*l);

  } // End of loop over all candidate clauses.

  PHASE ("condition", stats.conditionings,
         "globally blocked %ld clauses %.0f%%", blocked,
         percent (blocked, candidates.size ()));

  // Unmark initial conditional variables.
  //
  for (const auto &lit : conditional)
    unmark_as_conditional_literal (lit);

  erase_vector (unassigned);
  erase_vector (conditional);
  erase_vector (candidates);

  // Unassign additionally assigned literals.
  //
#if defined(LOGGING) || !defined(CADICAL_NDEBUG)
  int additionally_unassigned = 0;
#endif
  while (trail.size () > initial_trail_level) {
    int lit = trail.back ();
    trail.pop_back ();
    condition_unassign (lit);
#if defined(LOGGING) || !defined(CADICAL_NDEBUG)
    additionally_unassigned++;
#endif
  }
  LOG ("unassigned %d additionally assigned literals",
       additionally_unassigned);
  CADICAL_assert (additionally_unassigned == additionally_assigned);

  if (level > initial_level) {
    LOG ("reset condition decision level");
    level = initial_level;
  }

  reset_occs ();
  delete_garbage_clauses ();

  // Reassign previously assigned variables again.
  //
  LOG ("reassigning previously assigned variables");
  for (size_t i = 0; i < initial_trail_level; i++) {
    const int lit = trail[i];
    const signed char tmp = val (lit);
    CADICAL_assert (tmp >= 0);
    if (!tmp)
      condition_assign (lit);
  }

#ifndef CADICAL_NDEBUG
  for (const auto &lit : trail)
    CADICAL_assert (!marked (lit));
#endif

  unprotect_reasons ();

  return blocked;
}

void Internal::condition (bool update_limits) {

  if (unsat)
    return;
  if (!stats.current.irredundant)
    return;

  START_SIMPLIFIER (condition, CONDITION);
  stats.conditionings++;

  // Propagation limit to avoid too much work in 'condition'.  We mark
  // tried candidate clauses after giving up, such that next time we run
  // 'condition' we can try them.
  //
  long limit = stats.propagations.search;
  limit *= opts.conditioneffort;
  limit /= 1000;
  if (limit < opts.conditionmineff)
    limit = opts.conditionmineff;
  if (limit > opts.conditionmaxeff)
    limit = opts.conditionmaxeff;
  CADICAL_assert (stats.current.irredundant);
  limit *= 2.0 * active () / (double) stats.current.irredundant;
  limit = max (limit, 2l * active ());

  PHASE ("condition", stats.conditionings,
         "started after %" PRIu64 " conflicts limited by %ld propagations",
         stats.conflicts, limit);

  long blocked = condition_round (limit);

  STOP_SIMPLIFIER (condition, CONDITION);
  report ('g', !blocked);

  if (!update_limits)
    return;

  long delta = opts.conditionint * (stats.conditionings + 1);
  lim.condition = stats.conflicts + delta;

  PHASE ("condition", stats.conditionings,
         "next limit at %" PRIu64 " after %ld conflicts", lim.condition,
         delta);
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
