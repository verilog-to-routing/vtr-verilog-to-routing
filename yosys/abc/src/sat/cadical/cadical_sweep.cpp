#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

Sweeper::Sweeper (Internal *i)
    : internal (i), random (internal->opts.seed) {
  random += internal->stats.sweep; // different seed every time
  internal->init_sweeper (*this);
}

Sweeper::~Sweeper () {
  // this is already called actively
  // internal->release_sweeper (this);
  return;
}

#define INVALID64 INT64_MAX
#define INVALID UINT_MAX

int Internal::sweep_solve () {
  START (sweepsolve);
  cadical_kitten_randomize_phases (citten);
  stats.sweep_solved++;
  int res = cadical_kitten_solve (citten);
  if (res == 10)
    stats.sweep_sat++;
  if (res == 20)
    stats.sweep_unsat++;
  STOP (sweepsolve);
  return res;
}

bool Internal::sweep_flip (int lit) {
  START (sweepflip);
  bool res = cadical_kitten_flip_signed_literal (citten, lit);
  STOP (sweepflip);
  return res;
}

int Internal::sweep_flip_and_implicant (int lit) {
  START (sweepimplicant);
  int res = cadical_kitten_flip_and_implicant_for_signed_literal (citten, lit);
  STOP (sweepimplicant);
  return res;
}

void Internal::sweep_set_cadical_kitten_ticks_limit (Sweeper &sweeper) {
  uint64_t remaining = 0;
  const uint64_t current = sweeper.current_ticks;
  if (current < sweeper.limit.ticks)
    remaining = sweeper.limit.ticks - current;
  LOG ("'cadical_kitten_ticks' remaining %" PRIu64, remaining);
  cadical_kitten_set_ticks_limit (citten, remaining);
}

void Internal::sweep_update_noccs (Clause *c) {
  if (c->redundant)
    return;
  for (const auto &lit : *c) {
    noccs (lit)--;
  }
}

bool Internal::can_sweep_clause (Clause *c) {
  if (c->garbage)
    return false;
  if (!c->redundant)
    return true;
  return c->size == 2; // && !c->hyper;  // could ignore hyper
}

// essentially do full occurence list as in elim.cpp
void Internal::sweep_dense_mode_and_watch_irredundant () {
  reset_watches ();

  init_noccs ();

  // mark satisfied irredundant clauses as garbage
  for (const auto &c : clauses) {
    if (!can_sweep_clause (c))
      continue;
    bool satisfied = false;
    for (const auto &lit : *c) {
      const signed char tmp = val (lit);
      if (tmp <= 0)
        continue;
      if (tmp > 0) {
        satisfied = true;
        break;
      }
    }
    if (satisfied)
      mark_garbage (c); // forces more precise counts
    else {
      for (const auto &lit : *c)
        noccs (lit)++;
    }
  }

  init_occs ();

  // Connect irredundant clauses.
  //
  for (const auto &c : clauses) {
    if (!c->garbage) {
      for (const auto &lit : *c)
        if (active (lit))
          occs (lit).push_back (c);
    } else if (c->size == 2) {
      if (!c->flushed) {
        if (proof) {
          c->flushed = true;
          proof->delete_clause (c);
        }
      }
    }
  }
}

// go back to two watch scheme
void Internal::sweep_sparse_mode () {
  reset_occs ();
  reset_noccs ();
  init_watches ();
  connect_watches ();
}

// propagate without watches but full occurence list
void Internal::sweep_dense_propagate (Sweeper &sweeper) {
  vector<int> &work = sweeper.propagate;
  size_t i = 0;
  uint64_t &ticks = sweeper.current_ticks;
  while (i < work.size ()) {
    int lit = work[i++];
    LOG ("sweeping propagation of %d", lit);
    CADICAL_assert (val (lit) > 0);
    ticks += 1 + cache_lines (occs (-lit).size (), sizeof (Clause *));
    const Occs &ns = occs (-lit);
    for (const auto &c : ns) {
      ticks++;
      if (!can_sweep_clause (c))
        continue;
      int unit = 0, satisfied = 0;
      for (const auto &other : *c) {
        const signed char tmp = val (other);
        if (tmp < 0)
          continue;
        if (tmp > 0) {
          satisfied = other;
          break;
        }
        if (unit)
          unit = INT_MIN;
        else
          unit = other;
      }
      if (satisfied) {
        LOG (c, "sweeping propagation of %d finds %d satisfied", lit,
             satisfied);
        mark_garbage (c);
        sweep_update_noccs (c);
      } else if (!unit) {
        LOG ("empty clause during sweeping propagation of %d", lit);
        // need to set conflict = c for lrat
        conflict = c;
        learn_empty_clause ();
        conflict = 0;
        break;
      } else if (unit != INT_MIN) {
        LOG ("new unit %d during sweeping propagation of %d", unit, lit);
        build_chain_for_units (unit, c, 0);
        assign_unit (unit);
        work.push_back (unit);
        ticks++;
      }
    }
    if (unsat)
      break;

    // not necessary but should help
    ticks += 1 + cache_lines (occs (lit).size (), sizeof (Clause *));
    const Occs &ps = occs (lit);
    for (const auto &c : ps) {
      ticks++;
      if (c->garbage)
        continue;
      // if (c->redundant)  // TODO I assume it does not hurt to mark
      // everything here continue;
      LOG (c, "sweeping propagation of %d produces satisfied", lit);
      mark_garbage (c);
      sweep_update_noccs (c);
    }
  }
  work.clear ();
}

bool Internal::cadical_kitten_ticks_limit_hit (Sweeper &sweeper, const char *when) {
  const uint64_t current =
      cadical_kitten_current_ticks (citten) + sweeper.current_ticks;
  if (current >= sweeper.limit.ticks) {
    LOG ("'cadical_kitten_ticks' limit of %" PRIu64 " ticks hit after %" PRIu64
         " ticks during %s",
         sweeper.limit.ticks, current, when);
    return true;
  }
#ifndef LOGGING
  (void) when;
#endif
  return false;
}

void Internal::init_sweeper (Sweeper &sweeper) {
  sweeper.encoded = 0;
  enlarge_zero (sweeper.depths, max_var + 1);
  sweeper.reprs = new int[2 * max_var + 1];
  sweeper.reprs += max_var;
  enlarge_zero (sweeper.prev, max_var + 1);
  enlarge_zero (sweeper.next, max_var + 1);
  for (const auto &lit : lits)
    sweeper.reprs[lit] = lit;
  sweeper.first = sweeper.last = 0;
  sweeper.current_ticks =
      2 *
      clauses
          .size (); // initialize with the cost of building full occ list.
  sweeper.current_ticks +=
      2 + 2 * cache_lines (clauses.size (), sizeof (Clause *));
  CADICAL_assert (!citten);
  citten = cadical_kitten_init ();
  citten_clear_track_log_terminate ();

  sweep_dense_mode_and_watch_irredundant (); // full occurence list

  if (lrat) {
    sweeper.prev_units.push_back (0);
    for (const auto &idx : vars) {
      sweeper.prev_units.push_back (val (idx) != 0);
    }
  }

  unsigned completed = stats.sweep_completed;
  const unsigned max_completed = 32;
  if (completed > max_completed)
    completed = max_completed;

  uint64_t vars_limit = opts.sweepvars;
  vars_limit <<= completed;
  const unsigned max_vars_limit = opts.sweepmaxvars;
  if (vars_limit > max_vars_limit)
    vars_limit = max_vars_limit;
  sweeper.limit.vars = vars_limit;
  VERBOSE (3, "sweeper variable limit %u", sweeper.limit.vars);

  uint64_t depth_limit = stats.sweep_completed;
  depth_limit += opts.sweepdepth;
  const unsigned max_depth = opts.sweepmaxdepth;
  if (depth_limit > max_depth)
    depth_limit = max_depth;
  sweeper.limit.depth = depth_limit;
  VERBOSE (3, "sweeper depth limit %u", sweeper.limit.depth);

  uint64_t clause_limit = opts.sweepclauses;
  clause_limit <<= completed;
  const unsigned max_clause_limit = opts.sweepmaxclauses;
  if (clause_limit > max_clause_limit)
    clause_limit = max_clause_limit;
  sweeper.limit.clauses = clause_limit;
  VERBOSE (3, "sweeper clause limit %u", sweeper.limit.clauses);
}

void Internal::release_sweeper (Sweeper &sweeper) {

  sweeper.reprs -= max_var;
  delete[] sweeper.reprs;

  erase_vector (sweeper.depths);
  erase_vector (sweeper.prev);
  erase_vector (sweeper.next);
  erase_vector (sweeper.vars);
  erase_vector (sweeper.clause);
  erase_vector (sweeper.backbone);
  erase_vector (sweeper.partition);
  erase_vector (sweeper.prev_units);
  for (unsigned i = 0; i < 2; i++)
    erase_vector (sweeper.core[i]);

  cadical_kitten_release (citten);
  citten = 0;
  stats.ticks.sweep += sweeper.current_ticks;
  sweep_sparse_mode ();
  return;
}

void Internal::clear_sweeper (Sweeper &sweeper) {
  LOG ("clearing sweeping environment");
  sweeper.current_ticks += cadical_kitten_current_ticks (citten);

  citten_clear_track_log_terminate ();
  for (auto &idx : sweeper.vars) {
    CADICAL_assert (sweeper.depths[idx]);
    sweeper.depths[idx] = 0;
  }
  sweeper.vars.clear ();
  for (auto c : sweeper.clauses) {
    CADICAL_assert (c->swept);
    c->swept = false;
  }
  sweeper.clauses.clear ();
  sweeper.backbone.clear ();
  sweeper.partition.clear ();
  sweeper.encoded = 0;
  sweep_set_cadical_kitten_ticks_limit (sweeper);
}

int Internal::sweep_repr (Sweeper &sweeper, int lit) {
  int res;
  {
    int prev = lit;
    while ((res = sweeper.reprs[prev]) != prev)
      prev = res;
  }
  if (res == lit)
    return res;
  LOG ("sweeping repr[%d] = %d", lit, res);
  {
    const int not_res = -res;
    int next, prev = lit;
    while ((next = sweeper.reprs[prev]) != res) {
      const int not_prev = -prev;
      sweeper.reprs[not_prev] = not_res;
      sweeper.reprs[prev] = res;
      prev = next;
    }
    CADICAL_assert (sweeper.reprs[-prev] == not_res);
  }
  return res;
}

void Internal::add_literal_to_environment (Sweeper &sweeper, unsigned depth,
                                           int lit) {
  const int repr = sweep_repr (sweeper, lit);
  if (repr != lit)
    return;
  const int idx = abs (lit);
  if (sweeper.depths[idx])
    return;
  CADICAL_assert (depth < UINT_MAX);
  sweeper.depths[idx] = depth + 1;
  CADICAL_assert (idx);
  sweeper.vars.push_back (idx);
  LOG ("sweeping[%u] adding literal %d", depth, lit);
}

void Internal::sweep_add_clause (Sweeper &sweeper, unsigned depth) {
  // TODO: CADICAL_assertion fails, check if this an issue or can be avoided
  // CADICAL_assert (sweeper.clause.size () > 1);
  for (const auto &lit : sweeper.clause)
    add_literal_to_environment (sweeper, depth, lit);
  citten_clause_with_id (citten, sweeper.clauses.size (),
                         sweeper.clause.size (), sweeper.clause.data ());
  sweeper.clause.clear ();
  if (opts.sweepcountbinary || sweeper.clause.size () > 2)
    sweeper.encoded++;
}

void Internal::sweep_clause (Sweeper &sweeper, unsigned depth, Clause *c) {
  if (c->swept)
    return;
  CADICAL_assert (can_sweep_clause (c));
  LOG (c, "sweeping[%u]", depth);
  CADICAL_assert (sweeper.clause.empty ());
  for (const auto &lit : *c) {
    const signed char tmp = val (lit);
    if (tmp > 0) {
      mark_garbage (c);
      sweep_update_noccs (c);
      sweeper.clause.clear ();
      return;
    }
    if (tmp < 0) {
      if (lrat)
        sweeper.prev_units[abs (lit)] = true;
      continue;
    }
    sweeper.clause.push_back (lit);
  }
  c->swept = true;
  sweep_add_clause (sweeper, depth);
  sweeper.clauses.push_back (c);
}

extern "C" {
static void save_core_clause (void *state, unsigned id, bool learned,
                              size_t size, const unsigned *lits) {
  Sweeper *sweeper = (Sweeper *) state;
  Internal *internal = sweeper->internal;
  if (internal->unsat)
    return;
  vector<sweep_proof_clause> &core = sweeper->core[sweeper->save];
  sweep_proof_clause pc;
  if (learned) {
    pc.sweep_id = INVALID; // necessary
    pc.cad_id = INVALID64; // delay giving ids
  } else {
    pc.sweep_id = id; // necessary
    CADICAL_assert (id < sweeper->clauses.size ());
    pc.cad_id = sweeper->clauses[id]->id;
  }
  pc.kit_id = 0;
  pc.learned = learned;
  const unsigned *end = lits + size;
  for (const unsigned *p = lits; p != end; p++) {
    pc.literals.push_back (internal->citten2lit (*p)); // conversion
  }
#ifdef LOGGING
  LOG (pc.literals, "traced %s",
       pc.learned == true ? "learned" : "original");
#endif
  core.push_back (pc);
}

static void save_core_clause_with_lrat (void *state, unsigned cid,
                                        unsigned id, bool learned,
                                        size_t size, const unsigned *lits,
                                        size_t chain_size,
                                        const unsigned *chain) {
  Sweeper *sweeper = (Sweeper *) state;
  Internal *internal = sweeper->internal;
  if (internal->unsat)
    return;
  vector<sweep_proof_clause> &core = sweeper->core[sweeper->save];
  vector<Clause *> &clauses = sweeper->clauses;
  sweep_proof_clause pc;
  pc.kit_id = cid;
  pc.learned = learned;
  pc.sweep_id = INVALID;
  pc.cad_id = INVALID64;
  if (!learned) {
    CADICAL_assert (size);
    CADICAL_assert (!chain_size);
    CADICAL_assert (id < clauses.size ());
    pc.sweep_id = id;
    CADICAL_assert (id < clauses.size ());
    pc.cad_id = clauses[id]->id;
    for (const auto &lit : *clauses[id]) {
      pc.literals.push_back (lit);
    }
  } else {
    CADICAL_assert (chain_size);
    pc.sweep_id = INVALID;
    pc.cad_id = INVALID64; // delay giving ids
    const unsigned *end = lits + size;
    for (const unsigned *p = lits; p != end; p++) {
      pc.literals.push_back (internal->citten2lit (*p)); // conversion
    }
    for (const unsigned *p = chain + chain_size; p != chain; p--) {
      pc.chain.push_back (*(p - 1));
    }
  }
#ifdef LOGGING
  if (learned)
    LOG (pc.literals, "traced %s",
         pc.learned == true ? "learned" : "original");
  else {
    CADICAL_assert (id < clauses.size ());
    LOG (clauses[id], "traced");
  }
#endif
  core.push_back (pc);
}

static int citten_terminate (void *data) {
  return ((Internal *) data)->terminated_asynchronously ();
}

} // end extern C

void Internal::citten_clear_track_log_terminate () {
  CADICAL_assert (citten);
  cadical_kitten_clear (citten);
  cadical_kitten_track_antecedents (citten);
  if (external->terminator)
    cadical_kitten_set_terminator (citten, internal, citten_terminate);
#ifdef LOGGING
  if (opts.log)
    cadical_kitten_set_logging (citten);
#endif
}

void Internal::add_core (Sweeper &sweeper, unsigned core_idx) {
  if (unsat)
    return;
  LOG ("check and add extracted core[%u] lemmas to proof", core_idx);
  CADICAL_assert (core_idx == 0 || core_idx == 1);
  vector<sweep_proof_clause> &core = sweeper.core[core_idx];

  CADICAL_assert (!lrat || proof);

  unsigned unsat_size = 0;
  for (auto &pc : core) {
    unsat_size++;

    if (!pc.learned) {
      LOG (pc.literals,
           "not adding already present core[%u] cadical_kitten[%u] clause",
           core_idx, pc.kit_id);
      continue;
    }

    LOG (pc.literals, "adding extracted core[%u] cadical_kitten[%u] lemma",
         core_idx, pc.kit_id);

    const unsigned new_size = pc.literals.size ();

    if (lrat) {
      CADICAL_assert (pc.cad_id == INVALID64);
      for (auto &cid : pc.chain) {
        int64_t id = 0;
        for (const auto &cpc : core) {
          if (cpc.kit_id == cid) {
            if (cpc.learned)
              id = cpc.cad_id;
            else {
              id = cpc.cad_id;
              CADICAL_assert (cpc.cad_id == sweeper.clauses[cpc.sweep_id]->id);
              CADICAL_assert (!sweeper.clauses[cpc.sweep_id]->garbage);
              // avoid duplicate ids of units with seen flags
              for (const auto &lit : cpc.literals) {
                if (val (lit) >= 0)
                  continue;
                if (flags (lit).seen)
                  continue;
                const int idx = abs (lit);
                if (sweeper.prev_units[idx]) {
                  int64_t uid = unit_id (-lit);
                  lrat_chain.push_back (uid);
                  analyzed.push_back (lit);
                  flags (lit).seen = true;
                }
              }
            }
            break;
          }
        }
        CADICAL_assert (id);
        if (id != INVALID64)
          lrat_chain.push_back (id);
      }
      clear_analyzed_literals ();
    }

    if (!new_size) {
      LOG ("sweeping produced empty clause");
      learn_empty_clause ();
      core.resize (unsat_size);
      return;
    }

    if (new_size == 1) {
      int unit = pc.literals[0];
      if (val (unit) > 0) {
        LOG ("already assigned sweeping unit %d", unit);
        lrat_chain.clear ();
      } else if (val (unit) < 0) {
        LOG ("falsified sweeping unit %d leads to empty clause", unit);
        if (lrat) {
          int64_t id = unit_id (-unit);
          lrat_chain.push_back (id);
        }
        learn_empty_clause ();
        core.resize (unsat_size);
        return;
      } else {
        LOG ("sweeping produced unit %d", unit);
        assign_unit (unit);
        stats.sweep_units++;
        sweeper.propagate.push_back (unit);
      }
      if (proof && lrat)
        pc.cad_id = unit_id (unit);
      continue;
    }

    CADICAL_assert (new_size > 1);
    CADICAL_assert (pc.learned);

    if (proof) {
      pc.cad_id = ++clause_id;
      proof->add_derived_clause (pc.cad_id, true, pc.literals, lrat_chain);
      lrat_chain.clear ();
    }
  }
}

void Internal::save_core (Sweeper &sweeper, unsigned core) {
  LOG ("saving extracted core[%u] lemmas", core);
  CADICAL_assert (core == 0 || core == 1);
  CADICAL_assert (sweeper.core[core].empty ());
  sweeper.save = core;
  cadical_kitten_compute_clausal_core (citten, 0);
  if (lrat)
    cadical_kitten_trace_core (citten, &sweeper, save_core_clause_with_lrat);
  else
    cadical_kitten_traverse_core_clauses_with_id (citten, &sweeper,
                                          save_core_clause);
}

void Internal::clear_core (Sweeper &sweeper, unsigned core_idx) {
  CADICAL_assert (core_idx == 0 || core_idx == 1);
  LOG ("clearing core[%u] lemmas", core_idx);
  vector<sweep_proof_clause> &core = sweeper.core[core_idx];
  if (proof) {
    LOG ("deleting sub-solver core clauses");
    for (auto &pc : core) {
      if (pc.learned && pc.literals.size () > 1)
        proof->delete_clause (pc.cad_id, true, pc.literals);
    }
  }
  core.clear ();
}

void Internal::save_add_clear_core (Sweeper &sweeper) {
  save_core (sweeper, 0);
  add_core (sweeper, 0);
  clear_core (sweeper, 0);
}

void Internal::init_backbone_and_partition (Sweeper &sweeper) {
  LOG ("initializing backbone and equivalent literals candidates");
  sweeper.backbone.clear ();
  sweeper.partition.clear ();
  for (const auto &idx : sweeper.vars) {
    if (!active (idx))
      continue;
    CADICAL_assert (idx > 0);
    const int lit = idx;
    const int not_lit = -lit;
    const signed char tmp = cadical_kitten_signed_value (citten, lit);
    const int candidate = (tmp < 0) ? not_lit : lit;
    LOG ("sweeping candidate %d", candidate);
    sweeper.backbone.push_back (candidate);
    sweeper.partition.push_back (candidate);
  }
  sweeper.partition.push_back (0);

  LOG (sweeper.backbone, "initialized backbone candidates");
  LOG (sweeper.partition, "initialized equivalence candidates");
}

void Internal::sweep_empty_clause (Sweeper &sweeper) {
  CADICAL_assert (!unsat);
  save_add_clear_core (sweeper);
  CADICAL_assert (unsat);
}

void Internal::sweep_refine_partition (Sweeper &sweeper) {
  LOG ("refining partition");
  vector<int> &old_partition = sweeper.partition;
  vector<int> new_partition;
  auto old_begin = old_partition.begin ();
  const auto old_end = old_partition.end ();
#ifdef LOGGING
  unsigned old_classes = 0;
  unsigned new_classes = 0;
#endif
  for (auto p = old_begin, q = p; p != old_end; p = q + 1) {
    unsigned assigned_true = 0;
    int other;
    for (q = p; (other = *q) != 0; q++) {
      if (sweep_repr (sweeper, other) != other)
        continue;
      if (val (other))
        continue;
      signed char value = cadical_kitten_signed_value (citten, other);
      if (!value)
        LOG ("dropping sub-solver unassigned %d", other);
      else if (value > 0) {
        new_partition.push_back (other);
        assigned_true++;
      }
    }
#ifdef LOGGING
    LOG ("refining class %u of size %zu", old_classes, (size_t) (q - p));
    old_classes++;
#endif
    if (assigned_true == 0)
      LOG ("no positive literal in class");
    else if (assigned_true == 1) {
#ifdef LOGGING
      other =
#else
      (void)
#endif
          new_partition.back ();
      new_partition.pop_back ();
      LOG ("dropping singleton class %d", other);
    } else {
      LOG ("%u positive literal in class", assigned_true);
      new_partition.push_back (0);
#ifdef LOGGING
      new_classes++;
#endif
    }

    unsigned assigned_false = 0;
    for (q = p; (other = *q) != 0; q++) {
      if (sweep_repr (sweeper, other) != other)
        continue;
      if (val (other))
        continue;
      signed char value = cadical_kitten_signed_value (citten, other);
      if (value < 0) {
        new_partition.push_back (other);
        assigned_false++;
      }
    }

    if (assigned_false == 0)
      LOG ("no negative literal in class");
    else if (assigned_false == 1) {
#ifdef LOGGING
      other =
#else
      (void)
#endif
          new_partition.back ();
      new_partition.pop_back ();
      LOG ("dropping singleton class %d", other);
    } else {
      LOG ("%u negative literal in class", assigned_false);
      new_partition.push_back (0);
#ifdef LOGGING
      new_classes++;
#endif
    }
  }
  old_partition.swap (new_partition);
  LOG ("refined %u classes into %u", old_classes, new_classes);
}

void Internal::sweep_refine_backbone (Sweeper &sweeper) {
  LOG ("refining backbone candidates");
  const auto end = sweeper.backbone.end ();
  auto q = sweeper.backbone.begin ();
  for (auto p = q; p != end; p++) {
    const int lit = *p;
    if (val (lit))
      continue;
    signed char value = cadical_kitten_signed_value (citten, lit);
    if (!value)
      LOG ("dropping sub-solver unassigned %d", lit);
    else if (value > 0)
      *q++ = lit;
  }
  sweeper.backbone.resize (q - sweeper.backbone.begin ());
}

void Internal::sweep_refine (Sweeper &sweeper) {
  CADICAL_assert (cadical_kitten_status (citten) == 10);
  if (sweeper.backbone.empty ())
    LOG ("no need to refine empty backbone candidates");
  else
    sweep_refine_backbone (sweeper);
  if (sweeper.partition.empty ())
    LOG ("no need to refine empty partition candidates");
  else
    sweep_refine_partition (sweeper);
}

void Internal::flip_backbone_literals (Sweeper &sweeper) {
  const unsigned max_rounds = opts.sweepfliprounds;
  if (!max_rounds)
    return;
  CADICAL_assert (sweeper.backbone.size ());
  if (cadical_kitten_status (citten) != 10)
    return;
#ifdef LOGGING
  unsigned total_flipped = 0;
#endif
  unsigned flipped, round = 0;
  do {
    round++;
    flipped = 0;
    bool refine = false;
    auto begin = sweeper.backbone.begin (), q = begin, p = q;
    const auto end = sweeper.backbone.end ();
    bool limit_hit = false;
    while (p != end) {
      const int lit = *p++;
      stats.sweep_flip_backbone++;
      if (limit_hit || terminated_asynchronously ()) {
        break;
      } else if (sweep_flip (lit)) {
        LOG ("flipping backbone candidate %d succeeded", lit);
#ifdef LOGGING
        total_flipped++;
#endif
        stats.sweep_flipped_backbone++;
        flipped++;
      } else {
        LOG ("flipping backbone candidate %d failed", lit);
        *q++ = lit;
      }
    }
    while (p != end)
      *q++ = *p++;
    sweeper.backbone.resize (q - sweeper.backbone.begin ());
    LOG ("flipped %u backbone candidates in round %u", flipped, round);

    if (limit_hit)
      break;
    if (terminated_asynchronously ())
      break;
    if (cadical_kitten_ticks_limit_hit (sweeper, "backbone flipping"))
      break;
    if (refine)
      sweep_refine (sweeper);
  } while (flipped && round < max_rounds);
  LOG ("flipped %u backbone candidates in total in %u rounds",
       total_flipped, round);
}

bool Internal::sweep_extract_fixed (Sweeper &sweeper, int lit) {
  const int not_lit = -lit;
  stats.sweep_solved_backbone++;
  cadical_kitten_assume_signed (citten, not_lit);
  int res = sweep_solve ();
  if (!res) {
    stats.sweep_unknown_backbone++;
    return false;
  }
  CADICAL_assert (res == 20);
  LOG ("sweep unit %d", lit);
  save_add_clear_core (sweeper);
  stats.sweep_unsat_backbone++;
  return true;
}

bool Internal::sweep_backbone_candidate (Sweeper &sweeper, int lit) {
  LOG ("trying backbone candidate %d", lit);
  signed char value = cadical_kitten_fixed_signed (citten, lit);
  if (value) {
    stats.sweep_fixed_backbone++;
    CADICAL_assert (value > 0);
    if (val (lit) <= 0) {
      return sweep_extract_fixed (sweeper, lit);
    } else
      LOG ("literal %d already fixed", lit);
    return false;
  }

  int res = cadical_kitten_status (citten);
  if (res != 10) {
    LOG ("not flipping due to status %d != 10", res);
  }
  stats.sweep_flip_backbone++;
  if (res == 10 && sweep_flip (lit)) {
    stats.sweep_flipped_backbone++;
    LOG ("flipping %d succeeded", lit);
    // LOGBACKBONE ("refined backbone candidates");
    return false;
  }

  LOG ("flipping %d failed", lit);
  const int not_lit = -lit;
  stats.sweep_solved_backbone++;
  cadical_kitten_assume_signed (citten, not_lit);
  res = sweep_solve ();
  if (res == 10) {
    LOG ("sweeping backbone candidate %d failed", lit);
    sweep_refine (sweeper);
    stats.sweep_sat_backbone++;
    return false;
  }

  if (res == 20) {
    LOG ("sweep unit %d", lit);
    save_add_clear_core (sweeper);
    CADICAL_assert (val (lit));
    stats.sweep_unsat_backbone++;
    return true;
  }

  stats.sweep_unknown_backbone++;

  LOG ("sweeping backbone candidate %d failed", lit);
  return false;
}

// at this point the binary (lit or other) is already present
// in the proof via 'add_core'.
// We just copy it as an irredundant clause, call weaken minus
// and push it on the extension stack.
//
int64_t Internal::add_sweep_binary (sweep_proof_clause pc, int lit,
                                    int other) {
  CADICAL_assert (!unsat);
  if (unsat)
    return 0; // sanity check, should be fuzzed

  CADICAL_assert (!val (lit) && !val (other));
  if (val (lit) || val (other))
    return 0;

  if (lrat) {
    for (const auto &plit : pc.literals) {
      if (val (plit)) {
        int64_t id = unit_id (-plit);
        lrat_chain.push_back (id);
      }
    }
    lrat_chain.push_back (pc.cad_id);
  }
  clause.push_back (lit);
  clause.push_back (other);
  const int64_t id = ++clause_id;
  if (proof) {
    proof->add_derived_clause (id, false, clause, lrat_chain);
    proof->weaken_minus (id, clause);
  }
  external->push_binary_clause_on_extension_stack (id, lit, other);
  clause.clear ();
  lrat_chain.clear ();
  return id;
}

void Internal::delete_sweep_binary (const sweep_binary &sb) {
  if (unsat)
    return;
  if (!proof)
    return;
  vector<int> bin;
  bin.push_back (sb.lit);
  bin.push_back (sb.other);
  proof->delete_clause (sb.id, false, bin);
}

bool Internal::scheduled_variable (Sweeper &sweeper, int idx) {
  return sweeper.prev[idx] != 0 || sweeper.first == idx;
}

void Internal::schedule_inner (Sweeper &sweeper, int idx) {
  CADICAL_assert (idx);
  if (!active (idx))
    return;
  const int next = sweeper.next[idx];
  if (next != 0) {
    LOG ("rescheduling inner %d as last", idx);
    const unsigned prev = sweeper.prev[idx];
    CADICAL_assert (sweeper.prev[next] == idx);
    sweeper.prev[next] = prev;
    if (prev == 0) {
      CADICAL_assert (sweeper.first == idx);
      sweeper.first = next;
    } else {
      CADICAL_assert (sweeper.next[prev] == idx);
      sweeper.next[prev] = next;
    }
    const unsigned last = sweeper.last;
    if (last == 0) {
      CADICAL_assert (sweeper.first == 0);
      sweeper.first = idx;
    } else {
      CADICAL_assert (sweeper.next[last] == 0);
      sweeper.next[last] = idx;
    }
    sweeper.prev[idx] = last;
    sweeper.next[idx] = 0;
    sweeper.last = idx;
  } else if (sweeper.last != idx) {
    LOG ("scheduling inner %d as last", idx);
    const unsigned last = sweeper.last;
    if (last == 0) {
      CADICAL_assert (sweeper.first == 0);
      sweeper.first = idx;
    } else {
      CADICAL_assert (sweeper.next[last] == 0);
      sweeper.next[last] = idx;
    }
    CADICAL_assert (sweeper.next[idx] == 0);
    sweeper.prev[idx] = last;
    sweeper.last = idx;
  } else
    LOG ("keeping inner %d scheduled as last", idx);
}

void Internal::schedule_outer (Sweeper &sweeper, int idx) {
  CADICAL_assert (!scheduled_variable (sweeper, idx));
  CADICAL_assert (active (idx));
  const int first = sweeper.first;
  if (first == 0) {
    CADICAL_assert (sweeper.last == 0);
    sweeper.last = idx;
  } else {
    CADICAL_assert (sweeper.prev[first] == 0);
    sweeper.prev[first] = idx;
  }
  CADICAL_assert (sweeper.prev[idx] == 0);
  sweeper.next[idx] = first;
  sweeper.first = idx;
  LOG ("scheduling outer %d as first", idx);
}

int Internal::next_scheduled (Sweeper &sweeper) {
  int res = sweeper.last;
  if (res == 0) {
    LOG ("no more scheduled variables left");
    return 0;
  }
  CADICAL_assert (res > 0);
  LOG ("dequeuing next scheduled %d", res);
  const unsigned prev = sweeper.prev[res];
  CADICAL_assert (sweeper.next[res] == 0);
  sweeper.prev[res] = 0;
  if (prev == 0) {
    CADICAL_assert (sweeper.first == res);
    sweeper.first = 0;
  } else {
    CADICAL_assert (sweeper.next[prev] == res);
    sweeper.next[prev] = 0;
  }
  sweeper.last = prev;
  return res;
}

void Internal::sweep_substitute_lrat (Clause *c, int64_t id) {
  if (!lrat)
    return;
  for (const auto &lit : *c) {
    CADICAL_assert (val (lit) <= 0);
    if (val (lit) < 0) {
      int64_t id = unit_id (-lit);
      lrat_chain.push_back (id);
    }
  }
  lrat_chain.push_back (id);
  lrat_chain.push_back (c->id);
}

#define all_scheduled(IDX) \
  int IDX = sweeper.first, NEXT_##IDX; \
  IDX != 0 && (NEXT_##IDX = sweeper.next[IDX], true); \
  IDX = NEXT_##IDX

// Substitute equivalences in clauses (see
// 'sweep_substitute_new_equivalences' for explanation)
void Internal::substitute_connected_clauses (Sweeper &sweeper, int lit,
                                             int repr, int64_t id) {
  if (unsat)
    return;
  if (val (lit))
    return;
  if (val (repr))
    return;
  LOG ("substituting %d with %d in all irredundant clauses", lit, repr);

  CADICAL_assert (lit != repr);
  CADICAL_assert (lit != -repr);

  CADICAL_assert (active (lit));
  CADICAL_assert (active (repr));

  uint64_t &ticks = sweeper.current_ticks;

  {
    ticks += 1 + cache_lines (occs (lit).size (), sizeof (Clause *));
    Occs &ns = occs (lit);
    auto const begin = ns.begin ();
    const auto end = ns.end ();
    auto q = begin;
    auto p = q;
    while (p != end) {
      Clause *c = *q++ = *p++;
      ticks++;
      if (c->garbage)
        continue;
      CADICAL_assert (clause.empty ());
      bool satisfied = false;
      bool repr_already_watched = false;
      const int not_repr = -repr;
#ifndef CADICAL_NDEBUG
      bool found = false;
#endif
      for (const auto &other : *c) {
        if (other == lit) {
#ifndef CADICAL_NDEBUG
          CADICAL_assert (!found);
          found = true;
#endif
          clause.push_back (repr);
          continue;
        }
        CADICAL_assert (other != -lit);
        if (other == repr) {
          CADICAL_assert (!repr_already_watched);
          repr_already_watched = true;
          continue;
        }
        if (other == not_repr) {
          satisfied = true;
          break;
        }
        const signed char tmp = val (other);
        if (tmp < 0)
          continue;
        if (tmp > 0) {
          satisfied = true;
          break;
        }
        clause.push_back (other);
      }
      if (satisfied) {
        clause.clear ();
        mark_garbage (c);
        sweep_update_noccs (c);
        continue;
      }
      CADICAL_assert (found);
      const unsigned new_size = clause.size ();
      sweep_substitute_lrat (c, id);
      if (new_size == 0) {
        LOG (c, "substituted empty clause");
        CADICAL_assert (!unsat);
        learn_empty_clause ();
        break;
      }
      ticks++;
      if (new_size == 1) {
        LOG (c, "reduces to unit");
        const int unit = clause[0];
        clause.clear ();
        assign_unit (unit);
        sweeper.propagate.push_back (unit);
        mark_garbage (c);
        sweep_update_noccs (c);
        stats.sweep_units++;
        break;
      }
      CADICAL_assert (c->size >= 2);
      if (!c->redundant)
        mark_removed (c);
      uint64_t new_id = ++clause_id;
      if (proof) {
        proof->add_derived_clause (new_id, c->redundant, clause,
                                   lrat_chain);
        proof->delete_clause (c);
      }
      c->id = new_id;
      lrat_chain.clear ();
      size_t l;
      int *literals = c->literals;
      for (l = 0; l < clause.size (); l++)
        literals[l] = clause[l];
      int flushed = c->size - (int) l;
      if (flushed) {
        LOG ("flushed %d literals", flushed);
        (void) shrink_clause (c, l);
      } else if (likely_to_be_kept_clause (c))
        mark_added (c);
      LOG (c, "substituted");
      if (!repr_already_watched) {
        occs (repr).push_back (c);
        noccs (repr)++;
      }
      clause.clear ();
      q--;
    }
    while (p != end)
      *q++ = *p++;
    ns.resize (q - ns.begin ());
  }
}

// In contrast to kissat we substitute the equivalences explicitely after
// every successful round of sweeping. This is necessary in order to extract
// valid LRAT proofs for subsequent rounds of sweeping.
void Internal::sweep_substitute_new_equivalences (Sweeper &sweeper) {
  if (unsat)
    return;

  unsigned count = 0;
  CADICAL_assert (lrat_chain.empty ());

  for (const auto &sb : sweeper.binaries) {
    count++;
    const auto lit = sb.lit;
    const auto other = sb.other;
    if (abs (lit) < abs (other)) {
      substitute_connected_clauses (sweeper, -other, lit, sb.id);
    } else {
      substitute_connected_clauses (sweeper, -lit, other, sb.id);
    }
    CADICAL_assert (lrat_chain.empty ());
    if (val (lit) < 0) {
      if (lrat) {
        const int64_t lid = unit_id (-lit);
        lrat_chain.push_back (lid);
      }
      if (!val (other)) {
        if (lrat)
          lrat_chain.push_back (sb.id);
        assign_unit (other);
      } else if (val (other) < 0) {
        if (lrat) {
          const int64_t oid = unit_id (-other);
          lrat_chain.push_back (oid);
          lrat_chain.push_back (sb.id);
        }
        learn_empty_clause ();
        return;
      }
    } else if (val (other) < 0) {
      if (!val (lit)) {
        if (lrat) {
          const int64_t oid = unit_id (-other);
          lrat_chain.push_back (oid);
          lrat_chain.push_back (sb.id);
        }
        assign_unit (lit);
      } else
        CADICAL_assert (val (lit) > 0);
    }
    lrat_chain.clear ();
    delete_sweep_binary (sb);
    if (count == 2) {
      if (!val (lit) && !val (other)) {
        const auto idx = abs (lit) < abs (other) ? abs (other) : abs (lit);
        if (!flags (idx).fixed ())
          mark_substituted (idx);
      }
      count = 0;
    }
    CADICAL_assert (lrat_chain.empty ());
  }
  sweeper.binaries.clear ();
}

void Internal::sweep_remove (Sweeper &sweeper, int lit) {
  CADICAL_assert (sweeper.reprs[lit] != lit);
  vector<int> &partition = sweeper.partition;
  const auto begin_partition = partition.begin ();
  auto p = begin_partition;
  const auto end_partition = partition.end ();
  for (; *p != lit; p++)
    CADICAL_assert (p + 1 != end_partition);
  auto begin_class = p;
  while (begin_class != begin_partition && begin_class[-1] != 0)
    begin_class--;
  auto end_class = p;
  while (*end_class != 0)
    end_class++;
  const unsigned size = end_class - begin_class;
  LOG ("removing non-representative %d from equivalence class of size %u",
       lit, size);
  CADICAL_assert (size > 1);
  auto q = begin_class;
  if (size == 2) {
    LOG ("completely squashing equivalence class of %d", lit);
    for (auto r = end_class + 1; r != end_partition; r++)
      *q++ = *r;
  } else {
    for (auto r = begin_class; r != end_partition; r++)
      if (r != p)
        *q++ = *r;
  }
  partition.resize (q - partition.begin ());
}

void Internal::flip_partition_literals (Sweeper &sweeper) {
  const unsigned max_rounds = opts.sweepfliprounds;
  if (!max_rounds)
    return;
  CADICAL_assert (sweeper.partition.size ());
  if (cadical_kitten_status (citten) != 10)
    return;
#ifdef LOGGING
  unsigned total_flipped = 0;
#endif
  unsigned flipped, round = 0;
  do {
    round++;
    flipped = 0;
    bool refine = false;
    bool limit_hit = false;
    auto begin = sweeper.partition.begin (), dst = begin, src = dst;
    const auto end = sweeper.partition.end ();
    while (src != end) {
      auto end_src = src;
      while (CADICAL_assert (end_src != end), *end_src != 0)
        end_src++;
      unsigned size = end_src - src;
      CADICAL_assert (size > 1);
      auto q = dst;
      for (auto p = src; p != end_src; p++) {
        const int lit = *p;
        if (limit_hit) {
          *q++ = lit;
          continue;
        } else if (cadical_kitten_ticks_limit_hit (sweeper, "partition flipping")) {
          *q++ = lit;
          limit_hit = true;
          continue;
        } else if (sweep_flip (lit)) {
          LOG ("flipping equivalence candidate %d succeeded", lit);
#ifdef LOGGING
          total_flipped++;
#endif
          flipped++;
          if (--size < 2)
            break;
        } else {
          LOG ("flipping equivalence candidate %d failed", lit);
          *q++ = lit;
        }
        stats.sweep_flip_equivalences++;
      }
      if (size > 1) {
        *q++ = 0;
        dst = q;
      }
      src = end_src + 1;
    }
    stats.sweep_flipped_equivalences += flipped;
    sweeper.partition.resize (dst - sweeper.partition.begin ());
    LOG ("flipped %u equivalence candidates in round %u", flipped, round);
    if (terminated_asynchronously ())
      break;
    if (cadical_kitten_ticks_limit_hit (sweeper, "partition flipping"))
      break;
    if (refine)
      sweep_refine (sweeper);
  } while (flipped && round < max_rounds);
  LOG ("flipped %u equivalence candidates in total in %u rounds",
       total_flipped, round);
}

bool Internal::sweep_equivalence_candidates (Sweeper &sweeper, int lit,
                                             int other) {
  LOG ("trying equivalence candidates %d = %d", lit, other);
  const auto begin = sweeper.partition.begin ();
  auto const end = sweeper.partition.end ();
  CADICAL_assert (begin + 3 <= end);
  CADICAL_assert (end[-3] == lit);
  CADICAL_assert (end[-2] == other);
  const int third = (end - begin == 3) ? 0 : end[-4];
  int res = cadical_kitten_status (citten);
  if (res == 10) {
    stats.sweep_flip_equivalences++;
    if (sweep_flip (lit)) {
      stats.sweep_flipped_equivalences++;
      LOG ("flipping %d succeeded", lit);
      if (third == 0) {
        LOG ("squashing equivalence class of %d", lit);
        sweeper.partition.resize (sweeper.partition.size () - 3);
      } else {
        LOG ("removing %d from equivalence class of %d", lit, other);
        end[-3] = other;
        end[-2] = 0;
        sweeper.partition.resize (sweeper.partition.size () - 1);
      }
      return false;
    }
    stats.sweep_flip_equivalences++;
    if (sweep_flip (other)) {
      stats.sweep_flipped_equivalences++;
      LOG ("flipping %d succeeded", other);
      if (third == 0) {
        LOG ("squashing equivalence class of %d", lit);
        sweeper.partition.resize (sweeper.partition.size () - 3);
      } else {
        LOG ("removing %d from equivalence class of %d", other, lit);
        end[-2] = 0;
        sweeper.partition.resize (sweeper.partition.size () - 1);
      }
      return false;
    }
  }
  // frozen variables are not allowed to be eliminated.
  // It might still be beneficial to learn the binaries, if they
  // really are equivalent, but we avoid the issue by not trying
  // for equivalence at all if the non-representative is frozen.
  // i.e., the higher absolute value
  if (abs (lit) > abs (other) && frozen (lit)) {
    if (third == 0) {
      LOG ("squashing equivalence class of %d", lit);
      sweeper.partition.resize (sweeper.partition.size () - 3);
    } else {
      LOG ("removing %d from equivalence class of %d", lit, other);
      end[-3] = other;
      end[-2] = 0;
      sweeper.partition.resize (sweeper.partition.size () - 1);
    }
    return false;
  } else if (abs (other) > abs (lit) && frozen (other)) {
    if (third == 0) {
      LOG ("squashing equivalence class of %d", lit);
      sweeper.partition.resize (sweeper.partition.size () - 3);
    } else {
      LOG ("removing %d from equivalence class of %d", lit, other);
      end[-2] = 0;
      sweeper.partition.resize (sweeper.partition.size () - 1);
    }
    return false;
  }

  const int not_other = -other;
  const int not_lit = -lit;
  LOG ("flipping %d and %d both failed", lit, other);
  cadical_kitten_assume_signed (citten, not_lit);
  cadical_kitten_assume_signed (citten, other);
  stats.sweep_solved_equivalences++;
  res = sweep_solve ();
  if (res == 10) {
    stats.sweep_sat_equivalences++;
    LOG ("first sweeping implication %d -> %d failed", other, lit);
    sweep_refine (sweeper);
  } else if (!res) {
    stats.sweep_unknown_equivalences++;
    LOG ("first sweeping implication %d -> %d hit ticks limit", other, lit);
  }

  if (res != 20)
    return false;

  stats.sweep_unsat_equivalences++;
  LOG ("first sweeping implication %d -> %d succeeded", other, lit);

  save_core (sweeper, 0);

  cadical_kitten_assume_signed (citten, lit);
  cadical_kitten_assume_signed (citten, not_other);
  res = sweep_solve ();
  stats.sweep_solved_equivalences++;
  if (res == 10) {
    stats.sweep_sat_equivalences++;
    LOG ("second sweeping implication %d <- %d failed", other, lit);
    sweep_refine (sweeper);
  } else if (!res) {
    stats.sweep_unknown_equivalences++;
    LOG ("second sweeping implication %d <- %d hit ticks limit", other,
         lit);
  }

  if (res != 20) {
    sweeper.core[0].clear ();
    return false;
  }

  CADICAL_assert (res == 20);

  stats.sweep_unsat_equivalences++;
  LOG ("second sweeping implication %d <- %d succeeded too", other, lit);

  save_core (sweeper, 1);

  LOG ("sweep equivalence %d = %d", lit, other);

  // If cadical_kitten behaves as expected, the two binaries of the equivalence
  // should be stored at sweeper.core[i].back () for i in {0, 1}.
  // We pick the smaller absolute valued literal as representative and
  // store the equivalence .
  add_core (sweeper, 0);
  add_core (sweeper, 1);
  if (!val (lit) && !val (other)) {
    CADICAL_assert (sweeper.core[0].size ());
    CADICAL_assert (sweeper.core[1].size ());
    stats.sweep_equivalences++;
    sweep_binary bin1;
    sweep_binary bin2;
    if (abs (lit) > abs (other)) {
      bin1.lit = lit;
      bin1.other = not_other;
      bin2.lit = not_lit;
      bin2.other = other;
      bin1.id = add_sweep_binary (sweeper.core[0].back (), lit, not_other);
      bin2.id = add_sweep_binary (sweeper.core[1].back (), not_lit, other);
    } else {
      bin1.lit = not_other;
      bin1.other = lit;
      bin2.lit = other;
      bin2.other = not_lit;
      bin1.id = add_sweep_binary (sweeper.core[0].back (), not_other, lit);
      bin2.id = add_sweep_binary (sweeper.core[1].back (), other, not_lit);
    }
    if (bin1.id && bin2.id) {
      sweeper.binaries.push_back (bin1);
      sweeper.binaries.push_back (bin2);
    }
  }

  int repr;
  if (abs (lit) < abs (other)) {
    repr = sweeper.reprs[other] = lit;
    sweeper.reprs[not_other] = not_lit;
    sweep_remove (sweeper, other);
  } else {
    repr = sweeper.reprs[lit] = other;
    sweeper.reprs[not_lit] = not_other;
    sweep_remove (sweeper, lit);
  }
  clear_core (sweeper, 0);
  clear_core (sweeper, 1);

  const int repr_idx = abs (repr);
  schedule_inner (sweeper, repr_idx);

  return true;
}

const char *Internal::sweep_variable (Sweeper &sweeper, int idx) {
  CADICAL_assert (!unsat);
  if (!active (idx))
    return "inactive variable";
  const int start = idx;
  if (sweeper.reprs[start] != start)
    return "non-representative variable";
  CADICAL_assert (sweeper.vars.empty ());
  CADICAL_assert (sweeper.clauses.empty ());
  CADICAL_assert (sweeper.backbone.empty ());
  CADICAL_assert (sweeper.partition.empty ());
  CADICAL_assert (!sweeper.encoded);

  stats.sweep_variables++;

  LOG ("sweeping %d", idx);
  CADICAL_assert (!val (start));
  LOG ("starting sweeping[0]");
  add_literal_to_environment (sweeper, 0, start);
  LOG ("finished sweeping[0]");
  LOG ("starting sweeping[1]");

  bool limit_reached = false;
  size_t expand = 0, next = 1;
  bool success = false;
  unsigned depth = 1;

  uint64_t &ticks = sweeper.current_ticks;
  while (!limit_reached) {
    if (sweeper.encoded >= sweeper.limit.clauses) {
      LOG ("environment clause limit reached");
      limit_reached = true;
      break;
    }
    if (expand == next) {
      LOG ("finished sweeping[%u]", depth);
      if (depth >= sweeper.limit.depth) {
        LOG ("environment depth limit reached");
        break;
      }
      next = sweeper.vars.size ();
      if (expand == next) {
        LOG ("completely copied all clauses");
        break;
      }
      depth++;
      LOG ("starting sweeping[%u]", depth);
    }
    const unsigned choices = next - expand;
    if (opts.sweeprand && choices > 1) {
      const unsigned swaps = sweeper.random.pick_int (0, choices - 1);
      if (swaps) {
        CADICAL_assert (expand + swaps < sweeper.vars.size ());
        swap (sweeper.vars[expand], sweeper.vars[expand + swaps]);
      }
    }
    const int idx = sweeper.vars[expand];
    LOG ("traversing and adding clauses of %d", idx);
    for (unsigned sign = 0; sign < 2; sign++) {
      const int lit = sign ? -idx : idx;
      ticks += 1 + cache_lines (occs (lit).size (), sizeof (Clause *));
      Occs &ns = occs (lit);
      for (auto c : ns) {
        ticks++;
        if (!can_sweep_clause (c))
          continue;
        sweep_clause (sweeper, depth, c);
        if (sweeper.vars.size () >= sweeper.limit.vars) {
          LOG ("environment variable limit reached");
          limit_reached = true;
          break;
        }
      }
      if (limit_reached)
        break;
    }
    expand++;
  }
  stats.sweep_depth += depth;
  stats.sweep_clauses += sweeper.encoded;
  stats.sweep_environment += sweeper.vars.size ();
  VERBOSE (3,
           "sweeping variable %d environment of "
           "%zu variables %u clauses depth %u",
           externalize (idx), sweeper.vars.size (), sweeper.encoded, depth);

  int res;
  if (sweeper.vars.size () == 1) {
    LOG ("not sweeping literal %d with environment size 1", idx);
    goto DONE;
  }
  res = sweep_solve ();
  LOG ("sub-solver returns '%d'", res);
  if (res == 10) {
    init_backbone_and_partition (sweeper);
#ifndef CADICAL_QUIET
    uint64_t units = stats.sweep_units;
    uint64_t solved = stats.sweep_solved;
#endif
    START (sweepbackbone);
    while (sweeper.backbone.size ()) {
      if (unsat || terminated_asynchronously () ||
          cadical_kitten_ticks_limit_hit (sweeper, "backbone refinement")) {
        limit_reached = true;
      STOP_SWEEP_BACKBONE:
        STOP (sweepbackbone);
        goto DONE;
      }
      flip_backbone_literals (sweeper);
      if (terminated_asynchronously () ||
          cadical_kitten_ticks_limit_hit (sweeper, "backbone refinement")) {
        limit_reached = true;
        goto STOP_SWEEP_BACKBONE;
      }
      if (sweeper.backbone.empty ())
        break;
      const int lit = sweeper.backbone.back ();
      sweeper.backbone.pop_back ();
      if (!active (lit))
        continue;
      if (sweep_backbone_candidate (sweeper, lit))
        success = true;
    }
    STOP (sweepbackbone);
#ifndef CADICAL_QUIET
    units = stats.sweep_units - units;
    solved = stats.sweep_solved - solved;
#endif
    VERBOSE (3,
             "complete swept variable %d backbone with %" PRIu64
             " units in %" PRIu64 " solver calls",
             externalize (idx), units, solved);
    CADICAL_assert (sweeper.backbone.empty ());
#ifndef CADICAL_QUIET
    uint64_t equivalences = stats.sweep_equivalences;
    solved = stats.sweep_solved;
#endif
    START (sweepequivalences);
    while (sweeper.partition.size ()) {
      if (unsat || terminated_asynchronously () ||
          cadical_kitten_ticks_limit_hit (sweeper, "partition refinement")) {
        limit_reached = true;
      STOP_SWEEP_EQUIVALENCES:
        STOP (sweepequivalences);
        goto DONE;
      }
      flip_partition_literals (sweeper);
      if (terminated_asynchronously () ||
          cadical_kitten_ticks_limit_hit (sweeper, "backbone refinement")) {
        limit_reached = true;
        goto STOP_SWEEP_EQUIVALENCES;
      }
      if (sweeper.partition.empty ())
        break;
      if (sweeper.partition.size () > 2) {
        const auto end = sweeper.partition.end ();
        CADICAL_assert (end[-1] == 0);
        int lit = end[-3];
        int other = end[-2];
        if (sweep_equivalence_candidates (sweeper, lit, other))
          success = true;
      } else
        sweeper.partition.clear ();
    }
    STOP (sweepequivalences);
#ifndef CADICAL_QUIET
    equivalences = stats.sweep_equivalences - equivalences;
    solved = stats.sweep_solved - solved;
    if (equivalences)
      VERBOSE (3,
               "complete swept variable %d partition with %" PRIu64
               " equivalences in %" PRIu64 " solver calls",
               externalize (idx), equivalences, solved);
#endif
  } else if (res == 20)
    sweep_empty_clause (sweeper);

DONE:
  clear_sweeper (sweeper);

  if (!unsat)
    sweep_substitute_new_equivalences (sweeper);
  if (!unsat)
    sweep_dense_propagate (sweeper);

  if (success && limit_reached)
    return "successfully despite reaching limit";
  if (!success && !limit_reached)
    return "unsuccessfully without reaching limit";
  else if (success && !limit_reached)
    return "successfully without reaching limit";
  CADICAL_assert (!success && limit_reached);
  return "unsuccessfully and reached limit";
}

struct sweep_candidate {
  unsigned rank;
  int idx;
};

struct rank_sweep_candidate {
  bool operator() (sweep_candidate a, sweep_candidate b) const {
    CADICAL_assert (a.rank && b.rank);
    CADICAL_assert (a.idx > 0 && b.idx > 0);
    if (a.rank < b.rank)
      return true;
    if (b.rank < a.rank)
      return false;
    return a.idx < b.idx;
  }
};

bool Internal::scheduable_variable (Sweeper &sweeper, int idx,
                                    size_t *occ_ptr) {
  const int lit = idx;
  const size_t pos = noccs (lit);
  if (!pos)
    return false;
  const unsigned max_occurrences = sweeper.limit.clauses;
  if (pos > max_occurrences)
    return false;
  const int not_lit = -lit;
  const size_t neg = noccs (not_lit);
  if (!neg)
    return false;
  if (neg > max_occurrences)
    return false;
  *occ_ptr = pos + neg;
  return true;
}

unsigned Internal::schedule_all_other_not_scheduled_yet (Sweeper &sweeper) {
  vector<sweep_candidate> fresh;
  for (const auto &idx : vars) {
    Flags &f = flags (idx);
    if (!f.active ())
      continue;
    if (sweep_incomplete && !f.sweep)
      continue;
    if (scheduled_variable (sweeper, idx))
      continue;
    size_t occ;
    if (!scheduable_variable (sweeper, idx, &occ)) {
      f.sweep = false;
      continue;
    }
    sweep_candidate cand;
    cand.rank = occ;
    cand.idx = idx;
    fresh.push_back (cand);
  }
  const size_t size = fresh.size ();
  CADICAL_assert (size <= UINT_MAX);
  sort (fresh.begin (), fresh.end (), rank_sweep_candidate ());
  for (auto &cand : fresh)
    schedule_outer (sweeper, cand.idx);
  return size;
}

unsigned Internal::reschedule_previously_remaining (Sweeper &sweeper) {
  unsigned rescheduled = 0;
  for (const auto &idx : sweep_schedule) {
    Flags &f = flags (idx);
    if (!f.active ())
      continue;
    if (scheduled_variable (sweeper, idx))
      continue;
    size_t occ;
    if (!scheduable_variable (sweeper, idx, &occ)) {
      f.sweep = false;
      continue;
    }
    schedule_inner (sweeper, idx);
    rescheduled++;
  }
  sweep_schedule.clear ();
  return rescheduled;
}

unsigned Internal::incomplete_variables () {
  unsigned res = 0;
  for (const auto &idx : vars) {
    Flags &f = flags (idx);
    if (!f.active ())
      continue;
    if (f.sweep)
      res++;
  }
  return res;
}

void Internal::mark_incomplete (Sweeper &sweeper) {
  unsigned marked = 0;
  for (all_scheduled (idx)) {
    if (!flags (idx).sweep) {
      flags (idx).sweep = true;
      marked++;
    }
  }
  sweep_incomplete = true;
#ifndef CADICAL_QUIET
  VERBOSE (2, "marked %u scheduled sweeping variables as incomplete",
           marked);
#else
  (void) marked;
#endif
}

unsigned Internal::schedule_sweeping (Sweeper &sweeper) {
  const unsigned rescheduled = reschedule_previously_remaining (sweeper);
  const unsigned fresh = schedule_all_other_not_scheduled_yet (sweeper);
  const unsigned scheduled = fresh + rescheduled;
  const unsigned incomplete = incomplete_variables ();
#ifndef CADICAL_QUIET
  PHASE ("sweep", stats.sweep,
         "scheduled %u variables %.0f%% "
         "(%u rescheduled %.0f%%, %u incomplete %.0f%%)",
         scheduled, percent (scheduled, active ()), rescheduled,
         percent (rescheduled, scheduled), incomplete,
         percent (incomplete, scheduled));
#endif
  if (incomplete)
    CADICAL_assert (sweep_incomplete);
  else {
    if (sweep_incomplete)
      stats.sweep_completed++;
    mark_incomplete (sweeper);
  }
  return scheduled;
}

void Internal::unschedule_sweeping (Sweeper &sweeper, unsigned swept,
                                    unsigned scheduled) {
#ifdef CADICAL_QUIET
  (void) scheduled, (void) swept;
#endif
  CADICAL_assert (sweep_schedule.empty ());
  CADICAL_assert (sweep_incomplete);
  for (all_scheduled (idx))
    if (active (idx)) {
      sweep_schedule.push_back (idx);
      LOG ("untried scheduled %d", idx);
    }
#ifndef CADICAL_QUIET
  const unsigned retained = sweep_schedule.size ();
#endif
  VERBOSE (3, "retained %u variables %.0f%% to be swept next time",
           retained, percent (retained, active ()));
  const unsigned incomplete = incomplete_variables ();
  if (incomplete)
    VERBOSE (3, "need to sweep %u more variables %.0f%% for completion",
             incomplete, percent (incomplete, active ()));
  else {
    VERBOSE (3, "no more variables needed to complete sweep");
    sweep_incomplete = false;
    stats.sweep_completed++;
  }
  PHASE ("sweep", stats.sweep, "swept %u variables (%u remain %.0f%%)",
         swept, incomplete, percent (incomplete, scheduled));
}

bool Internal::sweep () {
  if (!opts.sweep)
    return false;
  if (unsat)
    return false;
  if (terminated_asynchronously ())
    return false;
  if (delaying_sweep.bumpreasons.delay ()) { // TODO need to fix Delay
    last.sweep.ticks = stats.ticks.search[0] + stats.ticks.search[1];
    return false;
  }
  delaying_sweep.bumpreasons.bypass_delay ();
  SET_EFFORT_LIMIT (tickslimit, sweep, !opts.sweepcomplete);
  delaying_sweep.bumpreasons.unbypass_delay ();

  CADICAL_assert (!level);
  START_SIMPLIFIER (sweep, SWEEP);
  stats.sweep++;
  uint64_t equivalences = stats.sweep_equivalences;
  uint64_t units = stats.sweep_units;
  Sweeper sweeper = Sweeper (this);
  if (opts.sweepcomplete)
    sweeper.limit.ticks = INT64_MAX;
  else
    sweeper.limit.ticks = tickslimit - stats.ticks.sweep;
  sweep_set_cadical_kitten_ticks_limit (sweeper);
  const unsigned scheduled = schedule_sweeping (sweeper);
  uint64_t swept = 0, limit = 10;
  for (;;) {
    if (unsat)
      break;
    if (terminated_asynchronously ())
      break;
    if (cadical_kitten_ticks_limit_hit (sweeper, "sweeping loop"))
      break;
    int idx = next_scheduled (sweeper);
    if (idx == 0)
      break;
    flags (idx).sweep = false;
#ifndef CADICAL_QUIET
    const char *res =
#endif
        sweep_variable (sweeper, idx);
    VERBOSE (2, "swept[%" PRIu64 "] external variable %d %s", swept,
             externalize (idx), res);
    if (++swept == limit) {
      VERBOSE (2,
               "found %" PRIu64 " equivalences and %" PRIu64
               " units after sweeping %" PRIu64 " variables ",
               stats.sweep_equivalences - equivalences,
               stats.sweep_units - units, swept);
      limit *= 10;
    }
  }
  VERBOSE (2, "swept %" PRIu64 " variables", swept);
  equivalences = stats.sweep_equivalences - equivalences,
  units = stats.sweep_units - units;
  PHASE ("sweep", stats.sweep,
         "found %" PRIu64 " equivalences and %" PRIu64 " units",
         equivalences, units);
  unschedule_sweeping (sweeper, swept, scheduled);
  release_sweeper (sweeper);

  if (!unsat) {
    propagated = 0;
    if (!propagate ()) {
      learn_empty_clause ();
    }
  }

  uint64_t eliminated = equivalences + units;
  report ('=', !eliminated);

  if (relative (eliminated, swept) < 0.001) {
    delaying_sweep.bumpreasons.bump_delay ();
  } else {
    delaying_sweep.bumpreasons.reduce_delay ();
  }
  STOP_SIMPLIFIER (sweep, SWEEP);
  return eliminated;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
