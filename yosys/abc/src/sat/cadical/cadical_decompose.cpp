#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

void Internal::decompose_analyze_binary_chain (DFS *dfs, int from) {
  if (!lrat)
    return;
  LOG ("binary chain starting at %d", from);
  DFS &from_dfs = dfs[vlit (from)];
  Clause *reason = from_dfs.parent;
  if (!reason)
    return;
  CADICAL_assert (reason->size == 2);
  mini_chain.push_back (reason->id);
  int other = reason->literals[0];
  other = other == from ? -reason->literals[1] : -other;
  Flags &f = flags (other);
  if (f.seen)
    return;
  f.seen = true;
  analyzed.push_back (other);
  decompose_analyze_binary_chain (dfs, other);
}

vector<Clause *> Internal::decompose_analyze_binary_clauses (DFS *dfs,
                                                             int from) {
  vector<Clause *> result;
  LOG ("binary chain starting at %d", from);
  DFS &from_dfs = dfs[vlit (from)];
  Clause *reason = from_dfs.parent;
  while (reason) {
    result.push_back (reason);
    CADICAL_assert (reason->size == 2);
    int other = reason->literals[0];
    other = other == from ? -reason->literals[1] : -other;
    Flags &f = flags (other);
    if (f.seen)
      break;
    f.seen = true;
    analyzed.push_back (other);
    from = other;
    DFS &from_dfs = dfs[vlit (from)];
    reason = from_dfs.parent;
  }
  return result;
}

void Internal::decompose_conflicting_scc_lrat (DFS *dfs, vector<int> &scc) {
  if (!lrat)
    return;
  CADICAL_assert (lrat_chain.empty ());
  CADICAL_assert (mini_chain.empty ());
  for (auto &lit : scc) {
    Flags &f = flags (lit);
    if (f.seen)
      return;
    f.seen = true;
    analyzed.push_back (lit);
    decompose_analyze_binary_chain (dfs, lit);
    for (auto p = mini_chain.rbegin (); p != mini_chain.rend (); p++) {
      lrat_chain.push_back (*p);
    }
    mini_chain.clear ();
  }
  clear_analyzed_literals ();
}

void Internal::build_lrat_for_clause (
    const vector<vector<Clause *>> &dfs_chains, bool invert) {
  CADICAL_assert (lrat);
  LOG ("building chain for not subsumed clause");
  CADICAL_assert (lrat_chain.empty ());
  CADICAL_assert (sign_marked.empty ());
  // build chain for each replaced literal
  for (const auto lit : clause) {
    auto other = lit;
    if (val (other) > 0) {
      if (marked_decomposed (other))
        continue;
      mark_decomposed (other);
      int64_t id = unit_id (other);
      lrat_chain.push_back (id);
      continue;
    }
    CADICAL_assert (mini_chain.empty ());
    for (auto p : dfs_chains[vlit (other)]) {
      if (marked_decomposed (other))
        continue;
      mark_decomposed (other);
      int implied = p->literals[0];
      implied = implied == other ? -p->literals[1] : -implied;
      LOG ("ADDED %d -> %d (%" PRId64 ")", implied, other, p->id);
      other = implied;
      mini_chain.push_back (p->id);
      if (val (implied) <= 0)
        continue;
      if (marked_decomposed (implied))
        break;
      mark_decomposed (implied);
      int64_t id = unit_id (implied);
      mini_chain.push_back (id);
      break;
    }
    if (invert)
      for (auto p = mini_chain.rbegin (); p != mini_chain.rend (); p++)
        lrat_chain.push_back (*p);
    else
      for (auto p = mini_chain.begin (); p != mini_chain.end (); p++)
        lrat_chain.push_back (*p);
    mini_chain.clear ();
  }
  clear_sign_marked_literals ();
  LOG (lrat_chain, "lrat_chain:");
}

void Internal::clear_sign_marked_literals () {
  LOG ("clearing %zd marked literals", sign_marked.size ());
  for (const auto &lit : sign_marked) {
    // CADICAL_assert (marked_signed (lit));  violated on purpose in factor
    unmark_decomposed (lit);
  }
  sign_marked.clear ();
}

// This performs one round of Tarjan's algorithm, e.g., equivalent literal
// detection and substitution, on the whole formula.  We might want to
// repeat it since its application might produce new binary clauses or
// units. Such units might even result in an empty clause.

bool Internal::decompose_round () {

  if (!opts.decompose)
    return false;
  if (unsat)
    return false;
  if (terminated_asynchronously ())
    return false;

  CADICAL_assert (!level);

  START_SIMPLIFIER (decompose, DECOMP);

  stats.decompositions++;

  const size_t size_dfs = 2 * (1 + (size_t) max_var);
  DFS *dfs = new DFS[size_dfs];
  DeferDeleteArray<DFS> dfs_delete (dfs);
  int *reprs = new int[size_dfs];
  DeferDeleteArray<int> reprs_delete (reprs);
  clear_n (reprs, size_dfs);
  vector<vector<Clause *>> dfs_chains;
  dfs_chains.resize (size_dfs);
  if (lrat) {
    for (size_t i = 0; i > size_dfs; i++) {
      vector<Clause *> empty;
      dfs_chains[i] = empty;
    }
  }

  int substituted = 0;
#ifndef CADICAL_QUIET
  int non_trivial_sccs = 0;
  int before = active ();
#endif
  unsigned dfs_idx = 0;

  vector<int> work; // depth first search working stack
  vector<int> scc;  // collects members of one SCC

  // The binary implication graph might have disconnected components and
  // thus we have in general to start several depth first searches.

  for (auto root_idx : vars) {
    if (unsat)
      break;
    if (!active (root_idx))
      continue;
    for (int root_sign = -1; !unsat && root_sign <= 1; root_sign += 2) {
      int root = root_sign * root_idx;
      if (dfs[vlit (root)].min == TRAVERSED)
        continue; // skip traversed
      LOG ("new dfs search starting at root %d", root);
      CADICAL_assert (work.empty ());
      CADICAL_assert (scc.empty ());
      work.push_back (root);
      while (!unsat && !work.empty ()) {
        int parent = work.back ();
        DFS &parent_dfs = dfs[vlit (parent)];
        if (parent_dfs.min == TRAVERSED) { // skip traversed
          CADICAL_assert (reprs[vlit (parent)]);
          work.pop_back ();
        } else {
          CADICAL_assert (!reprs[vlit (parent)]);

          // Go over all implied literals, thus need to iterate over all
          // binary watched clauses with the negation of 'parent'.

          Watches &ws = watches (-parent);

          // Two cases: Either the node has never been visited before, i.e.,
          // it's depth first search index is zero, then perform the
          // 'pre-fix' work before visiting it's children.  Otherwise all
          // it's children and nodes reachable from those children have been
          // visited and their minimum reachable depth first search index
          // has been computed.  This second case is the 'post-fix' work.

          if (parent_dfs.idx) { // post-fix

            work.pop_back (); // 'parent' done

            // Get the minimum reachable depth first search index reachable
            // from the children of 'parent'.

            unsigned new_min = parent_dfs.min;

            for (const auto &w : ws) {
              if (!w.binary ())
                continue;
              const int child = w.blit;
              if (!active (child))
                continue;
              DFS &child_dfs = dfs[vlit (child)];
              if (new_min > child_dfs.min)
                new_min = child_dfs.min;
            }

            LOG ("post-fix work dfs search %d index %u reaches minimum %u",
                 parent, parent_dfs.idx, new_min);

            if (parent_dfs.idx == new_min) { // entry to SCC

              // All nodes on the 'scc' stack after and including 'parent'
              // are in the same SCC.  Their representative is computed as
              // the smallest literal (index-wise) in the SCC.  If the SCC
              // contains both a literal and its negation, then the formula
              // becomes unsatisfiable.

              if (lrat) {
                CADICAL_assert (analyzed.empty ());
                int other, first = 0;
                bool conflicting = false;
                size_t j = scc.size ();
                do {
                  CADICAL_assert (j > 0);
                  other = scc[--j];
                  if (!first || vlit (other) < vlit (first))
                    first = other;
                  Flags &f = flags (other);
                  if (other == -parent) {
                    conflicting = true; // conflicting scc
                  }
                  if (f.seen) {
                    continue; // also conflicting scc
                  }
                  f.seen = true;
                  analyzed.push_back (other);
                } while (other != parent);

                CADICAL_assert (!conflicting || first > 0);
                vector<int> to_justify;
                if (conflicting) {
                  LOG ("conflicting scc simulating up at %d", parent);
                  to_justify.push_back (-parent);
                } else
                  to_justify.push_back (first);
                while (!to_justify.empty ()) {
                  const int next = to_justify.back ();
                  to_justify.pop_back ();
                  Watches &next_ws = watches (-next);
                  for (const auto &w : next_ws) {
                    if (!w.binary ())
                      continue;
                    const int child = w.blit;
                    if (!active (child))
                      continue;
                    if (!flags (child).seen)
                      continue;
                    DFS &child_dfs = dfs[vlit (child)];
                    if (child_dfs.parent)
                      continue;
                    child_dfs.parent = w.clause;
                    to_justify.push_back (child);
                  }
                }

                clear_analyzed_literals ();
              }

              int other, repr = parent;
#ifndef CADICAL_QUIET
              int size = 0;
#endif
              CADICAL_assert (!scc.empty ());
              size_t j = scc.size ();
              do {
                CADICAL_assert (j > 0);
                other = scc[--j];
                if (other == -parent) {
                  LOG ("both %d and %d in one SCC", parent, -parent);
                  if (lrat) {
                    Flags &f = flags (-parent);
                    f.seen = true;
                    analyzed.push_back (-parent);
                    decompose_analyze_binary_chain (dfs, parent);
                    for (auto p : mini_chain)
                      lrat_chain.push_back (p);
                    mini_chain.clear ();
                  }
                  assign_unit (parent);
#ifndef CADICAL_NDEBUG
                  bool ok =
#endif
                      propagate ();
                  CADICAL_assert (!ok);
                  learn_empty_clause ();
                  lrat_chain.clear ();
                } else {
                  if (abs (other) < abs (repr))
                    repr = other;
#ifndef CADICAL_QUIET
                  size++;
#endif
                }
              } while (!unsat && other != parent);

              if (unsat)
                break;
#ifndef CADICAL_QUIET
              LOG ("SCC of representative %d of size %d", repr, size);
#endif
              do {
                CADICAL_assert (!scc.empty ());
                other = scc.back ();
                scc.pop_back ();
                dfs[vlit (other)].min = TRAVERSED;
                if (frozen (other)) {
                  reprs[vlit (other)] = other;
                  continue;
                }
                reprs[vlit (other)] = repr;
                if (other == repr)
                  continue;
                substituted++;
                LOG ("literal %d in SCC of %d", other, repr);
                if (!lrat)
                  continue;
                CADICAL_assert (mini_chain.empty ());
                Flags &f = flags (repr);
                f.seen = true;
                analyzed.push_back (repr);
                // no need to reverse dfs_chain because this is handled by
                // build_lrat_for_clause.
                dfs_chains[vlit (other)] =
                    decompose_analyze_binary_clauses (dfs, other);
                clear_analyzed_literals ();
              } while (other != parent);

#ifndef CADICAL_QUIET
              if (size > 1)
                non_trivial_sccs++;
#endif

            } else {

              // Current node 'parent' is in a non-trivial SCC but is not
              // the entry point of the SCC in this depth first search, so
              // keep it on the SCC stack until the entry point is reached.

              parent_dfs.min = new_min;
            }

          } else { // pre-fix

            dfs_idx++;
            CADICAL_assert (dfs_idx < TRAVERSED);
            parent_dfs.idx = parent_dfs.min = dfs_idx;
            scc.push_back (parent);

            LOG ("pre-fix work dfs search %d index %u", parent, dfs_idx);

            // Now traverse all the children in the binary implication
            // graph but keep 'parent' on the stack for 'post-fix' work.

            for (const auto &w : ws) {
              if (!w.binary ())
                continue;
              const int child = w.blit;
              if (!active (child))
                continue;
              DFS &child_dfs = dfs[vlit (child)];
              if (child_dfs.idx)
                continue;
              work.push_back (child);
            }
          }
        }
      }
    }
  }

  erase_vector (work);
  erase_vector (scc);
  // delete [] dfs; need to postpone until after changing clauses...

  // Only keep the representatives 'repr' mapping.

  PHASE ("decompose", stats.decompositions,
         "%d non-trivial sccs, %d substituted %.2f%%", non_trivial_sccs,
         substituted, percent (substituted, before));

  bool new_unit = false, new_binary_clause = false;

  // Finally, mark substituted literals as such and push the equivalences of
  // the substituted literals to their representative on the extension
  // stack to fix an assignment during 'extend'.
  // It is also necessary to do so for proper IDRUP/LIDRUP/Resolution proofs

  vector<int64_t> decompose_ids;
  const size_t size = 2 * (1 + (size_t) max_var);
  decompose_ids.resize (size);

  for (auto idx : vars) {
    if (!substituted)
      break;
    if (unsat)
      break;
    if (!active (idx))
      continue;
    int other = reprs[vlit (idx)];
    if (other == idx)
      continue;
    CADICAL_assert (!flags (other).eliminated ());
    CADICAL_assert (!flags (other).substituted ());

    LOG ("marking equivalence of %d and %d", idx, other);
    CADICAL_assert (clause.empty ());
    CADICAL_assert (lrat_chain.empty ());
    clause.push_back (other);
    clause.push_back (-idx);
    if (lrat) {
      build_lrat_for_clause (dfs_chains);
      CADICAL_assert (!lrat_chain.empty ());
    }

    const int64_t id1 = ++clause_id;
    if (proof) {
      proof->add_derived_clause (id1, false, clause, lrat_chain);
      proof->weaken_minus (id1, clause);
    }
    external->push_binary_clause_on_extension_stack (id1, -idx, other);

    decompose_ids[vlit (-idx)] = id1;

    lrat_chain.clear ();
    clause.clear ();

    CADICAL_assert (clause.empty ());
    CADICAL_assert (lrat_chain.empty ());
    clause.push_back (idx);
    clause.push_back (-other);
    if (lrat) {
      build_lrat_for_clause (dfs_chains);
      CADICAL_assert (!lrat_chain.empty ());
    }
    const int64_t id2 = ++clause_id;
    if (proof) {
      proof->add_derived_clause (id2, false, clause, lrat_chain);
      proof->weaken_minus (id2, clause);
    }
    external->push_binary_clause_on_extension_stack (id2, idx, -other);
    decompose_ids[vlit (idx)] = id2;

    clause.clear ();
    lrat_chain.clear ();
  }

  vector<Clause *> postponed_garbage;

  // Now go over all clauses and find clause which contain literals that
  // should be substituted by their representative.

  size_t clauses_size = clauses.size ();
#ifndef CADICAL_QUIET
  size_t garbage = 0, replaced = 0;
#endif
  for (size_t i = 0; substituted && !unsat && i < clauses_size; i++) {
    Clause *c = clauses[i];
    if (c->garbage)
      continue;
    int j, size = c->size;
    for (j = 0; j < size; j++) {
      const int lit = c->literals[j];
      if (reprs[vlit (lit)] != lit)
        break;
    }

    if (j == size)
      continue;

#ifndef CADICAL_QUIET
    replaced++;
#endif
    LOG (c, "first substituted literal %d in", substituted);

    // Now copy the result to 'clause'.  Substitute literals if they have a
    // different representative.  Skip duplicates and false literals.  If a
    // literal occurs in both phases or is assigned to true the clause is
    // satisfied and can be marked as garbage.

    CADICAL_assert (clause.empty ());
    CADICAL_assert (lrat_chain.empty ());
    CADICAL_assert (analyzed.empty ());
    bool satisfied = false;

    for (int k = 0; !satisfied && k < size; k++) {
      const int lit = c->literals[k];
      signed char tmp = val (lit);
      if (tmp > 0)
        satisfied = true;
      else if (tmp < 0) {
        if (!lrat)
          continue;
        Flags &f = flags (lit);
        if (f.seen)
          continue;
        f.seen = true;
        analyzed.push_back (lit);
        int64_t id = unit_id (-lit);
        lrat_chain.push_back (id);
        continue;
      } else {
        const int other = reprs[vlit (lit)];
        tmp = val (other);
        if (tmp < 0) {
          if (!lrat)
            continue;
          Flags &f = flags (other);
          if (!f.seen) {
            f.seen = true;
            analyzed.push_back (other);
            int64_t id = unit_id (-other);
            lrat_chain.push_back (id);
          }
          if (other == lit)
            continue;
          int64_t id = decompose_ids[vlit (-lit)];
          CADICAL_assert (id);
          lrat_chain.push_back (id);
          continue;
        } else if (tmp > 0)
          satisfied = true;
        else {
          tmp = marked (other);
          if (tmp < 0)
            satisfied = true;
          else if (!tmp) {
            mark (other);
            clause.push_back (other);
          }
          if (other == lit)
            continue;
          if (!lrat)
            continue;
          int64_t id = decompose_ids[vlit (-lit)];
          CADICAL_assert (id);
          lrat_chain.push_back (id);
        }
      }
    }
    if (lrat)
      lrat_chain.push_back (c->id);
    clear_analyzed_literals ();
    LOG (lrat_chain, "lrat_chain:");
    if (satisfied) {
      LOG (c, "satisfied after substitution (postponed)");
      postponed_garbage.push_back (c);
#ifndef CADICAL_QUIET
      garbage++;
#endif
    } else if (!clause.size ()) {
      LOG ("learned empty clause during decompose");
      learn_empty_clause ();
    } else if (clause.size () == 1) {
      LOG (c, "unit %d after substitution", clause[0]);
      assign_unit (clause[0]);
      mark_garbage (c);
      new_unit = true;
#ifndef CADICAL_QUIET
      garbage++;
#endif
    } else if (c->literals[0] != clause[0] || c->literals[1] != clause[1]) {
      LOG ("need new clause since at least one watched literal changed");
      if (clause.size () == 2)
        new_binary_clause = true;
      size_t d_clause_idx = clauses.size ();
      Clause *d = new_clause_as (c);
      CADICAL_assert (clauses[d_clause_idx] == d);
      clauses[d_clause_idx] = c;
      clauses[i] = d;
      mark_garbage (c);
#ifndef CADICAL_QUIET
      garbage++;
#endif
    } else {
      LOG ("simply shrinking clause since watches did not change");
      CADICAL_assert (c->size > 2);
      if (!c->redundant)
        mark_removed (c);
      if (proof) {
        proof->add_derived_clause (++clause_id, c->redundant, clause,
                                   lrat_chain);
        proof->delete_clause (c);
        c->id = clause_id;
      }
      size_t l;
      int *literals = c->literals;
      for (l = 2; l < clause.size (); l++)
        literals[l] = clause[l];
      int flushed = c->size - (int) l;
      if (flushed) {
        if (l == 2)
          new_binary_clause = true;
        LOG ("flushed %d literals", flushed);
        (void) shrink_clause (c, l);
      } else if (likely_to_be_kept_clause (c))
        mark_added (c);
      // we have shrunken c->size to l so even though there is an CADICAL_assertion
      // for c->size > 2 at the beginning of this else block, the new size
      // can be 2 now.
      if (c->size == 2) { // cheaper to update only new binary clauses
        CADICAL_assert (new_binary_clause);
        update_watch_size (watches (c->literals[0]), c->literals[1], c);
        update_watch_size (watches (c->literals[1]), c->literals[0], c);
      }
      LOG (c, "substituted");
    }
    while (!clause.empty ()) {
      int lit = clause.back ();
      clause.pop_back ();
      CADICAL_assert (marked (lit) > 0);
      unmark (lit);
    }
    lrat_chain.clear ();
  }

  if (proof) {
    for (auto idx : vars) {
      if (!substituted)
        break;
      if (!active (idx))
        continue;
      const int64_t id1 = decompose_ids[vlit (-idx)];
      if (!id1)
        continue;
      int other = reprs[vlit (idx)];
      CADICAL_assert (other != idx);
      CADICAL_assert (!flags (other).eliminated ());
      CADICAL_assert (!flags (other).substituted ());

      clause.push_back (other);
      clause.push_back (-idx);
      proof->delete_clause (id1, false, clause);
      clause.clear ();

      clause.push_back (idx);
      clause.push_back (-other);
      const int64_t id2 = decompose_ids[vlit (idx)];
      proof->delete_clause (id2, false, clause);
      clause.clear ();
    }
  }

  if (!unsat && !postponed_garbage.empty ()) {
    LOG ("now marking %zd postponed garbage clauses",
         postponed_garbage.size ());
    for (const auto &c : postponed_garbage)
      mark_garbage (c);
  }
  erase_vector (postponed_garbage);

  PHASE ("decompose", stats.decompositions,
         "%zd clauses replaced %.2f%% producing %zd garbage clauses %.2f%%",
         replaced, percent (replaced, clauses_size), garbage,
         percent (garbage, replaced));

  erase_vector (scc);

  // Propagate found units.

  if (!unsat && propagated < trail.size () && !propagate ()) {
    LOG ("empty clause after propagating units from substitution");
    learn_empty_clause ();
  }

  for (auto idx : vars) {
    if (!substituted)
      break;
    if (unsat)
      break;
    if (!active (idx))
      continue;
    int other = reprs[vlit (idx)];
    if (other == idx)
      continue;
    CADICAL_assert (!flags (other).eliminated ());
    CADICAL_assert (!flags (other).substituted ());
    if (!flags (other).fixed ())
      mark_substituted (idx);
  }

  reprs_delete.free ();
  dfs_delete.free ();
  erase_vector (dfs_chains);

  if (substituted)
    flush_all_occs_and_watches (); // particularly the 'blit's

  bool success =
      unsat || (substituted > 0 && (new_unit || new_binary_clause));
  report ('d', !opts.reportall && !success);

  STOP_SIMPLIFIER (decompose, DECOMP);

  return success;
}

void Internal::decompose () {
  for (int round = 1; round <= opts.decomposerounds; round++)
    if (!decompose_round ())
      break;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
