#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

using namespace std;

/*------------------------------------------------------------------------*/

// Enable proof logging and checking by allocating a 'Proof' object.

void Internal::new_proof_on_demand () {
  if (!proof) {
    LOG ("connecting proof to internal solver");
    proof = new Proof (this);
  }
}

void Internal::resize_unit_clauses_idx () {
  size_t new_vsize = vsize ? 2 * vsize : 1 + (size_t) max_var;
  unit_clauses_idx.resize (2 * new_vsize, 0);
}

void Internal::force_lrat () {
  if (lrat)
    return;
  CADICAL_assert (proof);
  lrat = true;
}

void Internal::connect_proof_tracer (Tracer *tracer, bool antecedents,
                                     bool finalize_clauses) {
  new_proof_on_demand ();
  if (antecedents)
    force_lrat ();
  if (finalize_clauses)
    frat = true;
  resize_unit_clauses_idx ();
  proof->connect (tracer);
  tracers.push_back (tracer);
}

void Internal::connect_proof_tracer (InternalTracer *tracer,
                                     bool antecedents,
                                     bool finalize_clauses) {
  new_proof_on_demand ();
  if (antecedents)
    force_lrat ();
  if (finalize_clauses)
    frat = true;
  resize_unit_clauses_idx ();
  tracer->connect_internal (this);
  proof->connect (tracer);
  tracers.push_back (tracer);
}

void Internal::connect_proof_tracer (StatTracer *tracer, bool antecedents,
                                     bool finalize_clauses) {
  new_proof_on_demand ();
  if (antecedents)
    force_lrat ();
  if (finalize_clauses)
    frat = true;
  resize_unit_clauses_idx ();
  tracer->connect_internal (this);
  proof->connect (tracer);
  stat_tracers.push_back (tracer);
}

void Internal::connect_proof_tracer (FileTracer *tracer, bool antecedents,
                                     bool finalize_clauses) {
  new_proof_on_demand ();
  if (antecedents)
    force_lrat ();
  if (finalize_clauses)
    frat = true;
  resize_unit_clauses_idx ();
  tracer->connect_internal (this);
  proof->connect (tracer);
  file_tracers.push_back (tracer);
}

bool Internal::disconnect_proof_tracer (Tracer *tracer) {
  auto it = std::find (tracers.begin (), tracers.end (), tracer);
  if (it != tracers.end ()) {
    tracers.erase (it);
    CADICAL_assert (proof);
    proof->disconnect (tracer);
    return true;
  }
  return false;
}

bool Internal::disconnect_proof_tracer (StatTracer *tracer) {
  auto it = std::find (stat_tracers.begin (), stat_tracers.end (), tracer);
  if (it != stat_tracers.end ()) {
    stat_tracers.erase (it);
    CADICAL_assert (proof);
    proof->disconnect (tracer);
    return true;
  }
  return false;
}

bool Internal::disconnect_proof_tracer (FileTracer *tracer) {
  auto it = std::find (file_tracers.begin (), file_tracers.end (), tracer);
  if (it != file_tracers.end ()) {
    file_tracers.erase (it);
    CADICAL_assert (proof);
    proof->disconnect (tracer);
    return true;
  }
  return false;
}

void Proof::disconnect (Tracer *t) {
  tracers.erase (std::remove (tracers.begin (), tracers.end (), t),
                 tracers.end ());
}

// Enable proof tracing.

void Internal::trace (File *file) {
  if (opts.veripb) {
    LOG ("PROOF connecting VeriPB tracer");
    bool antecedents = opts.veripb == 1 || opts.veripb == 2;
    bool deletions = opts.veripb == 2 || opts.veripb == 4;
    FileTracer *ft =
        new VeripbTracer (this, file, opts.binary, antecedents, deletions);
    connect_proof_tracer (ft, antecedents);
  } else if (opts.frat) {
    LOG ("PROOF connecting FRAT tracer");
    bool antecedents = opts.frat == 1;
    resize_unit_clauses_idx ();
    FileTracer *ft =
        new FratTracer (this, file, opts.binary, opts.frat == 1);
    connect_proof_tracer (ft, antecedents, true);
  } else if (opts.lrat) {
    LOG ("PROOF connecting LRAT tracer");
    FileTracer *ft = new LratTracer (this, file, opts.binary);
    connect_proof_tracer (ft, true);
  } else if (opts.idrup) {
    LOG ("PROOF connecting IDRUP tracer");
    FileTracer *ft = new IdrupTracer (this, file, opts.binary);
    connect_proof_tracer (ft, true);
  } else if (opts.lidrup) {
    LOG ("PROOF connecting LIDRUP tracer");
    FileTracer *ft = new LidrupTracer (this, file, opts.binary);
    connect_proof_tracer (ft, true);
  } else {
    LOG ("PROOF connecting DRAT tracer");
    FileTracer *ft = new DratTracer (this, file, opts.binary);
    connect_proof_tracer (ft, false);
  }
}

// Enable proof checking.

void Internal::check () {
  new_proof_on_demand ();
  if (opts.checkproof > 1) {
    StatTracer *lratchecker = new LratChecker (this);
    DeferDeletePtr<LratChecker> delete_lratchecker (
        (LratChecker *) lratchecker);
    LOG ("PROOF connecting LRAT proof checker");
    force_lrat ();
    frat = true;
    resize_unit_clauses_idx ();
    proof->connect (lratchecker);
    stat_tracers.push_back (lratchecker);
    delete_lratchecker.release ();
  }
  if (opts.checkproof == 1 || opts.checkproof == 3) {
    StatTracer *checker = new Checker (this);
    DeferDeletePtr<Checker> delete_checker ((Checker *) checker);
    LOG ("PROOF connecting proof checker");
    proof->connect (checker);
    stat_tracers.push_back (checker);
    delete_checker.release ();
  }
}

// We want to close a proof trace and stop checking as soon we are done.

void Internal::close_trace (bool print) {
  for (auto &tracer : file_tracers)
    tracer->close (print);
}

// We can flush a proof trace file before actually closing it.

void Internal::flush_trace (bool print) {
  for (auto &tracer : file_tracers)
    tracer->flush (print);
}

/*------------------------------------------------------------------------*/

Proof::Proof (Internal *s) : internal (s) { LOG ("PROOF new"); }

Proof::~Proof () { LOG ("PROOF delete"); }

/*------------------------------------------------------------------------*/

inline void Proof::add_literal (int internal_lit) {
  const int external_lit = internal->externalize (internal_lit);
  clause.push_back (external_lit);
}

inline void Proof::add_literals (Clause *c) {
  for (auto const &lit : *c)
    add_literal (lit);
}

inline void Proof::add_literals (const vector<int> &c) {
  for (auto const &lit : c)
    add_literal (lit);
}

/*------------------------------------------------------------------------*/

void Proof::add_original_clause (int64_t id, bool r, const vector<int> &c) {
  LOG (c, "PROOF adding original internal clause");
  add_literals (c);
  clause_id = id;
  redundant = r;
  add_original_clause ();
}

void Proof::add_external_original_clause (int64_t id, bool r,
                                          const vector<int> &c,
                                          bool restore) {
  // literals of c are already external
  CADICAL_assert (clause.empty ());
  for (auto const &lit : c)
    clause.push_back (lit);
  clause_id = id;
  redundant = r;
  add_original_clause (restore);
}

void Proof::delete_external_original_clause (int64_t id, bool r,
                                             const vector<int> &c) {
  // literals of c are already external
  CADICAL_assert (clause.empty ());
  for (auto const &lit : c)
    clause.push_back (lit);
  clause_id = id;
  redundant = r;
  delete_clause ();
}

void Proof::add_derived_empty_clause (int64_t id,
                                      const vector<int64_t> &chain) {
  LOG ("PROOF adding empty clause");
  CADICAL_assert (clause.empty ());
  CADICAL_assert (proof_chain.empty ());
  for (const auto &cid : chain)
    proof_chain.push_back (cid);
  clause_id = id;
  redundant = false;
  add_derived_clause ();
}

void Proof::add_derived_unit_clause (int64_t id, int internal_unit,
                                     const vector<int64_t> &chain) {
  LOG ("PROOF adding unit clause %d", internal_unit);
  CADICAL_assert (proof_chain.empty ());
  CADICAL_assert (clause.empty ());
  add_literal (internal_unit);
  for (const auto &cid : chain)
    proof_chain.push_back (cid);
  clause_id = id;
  redundant = false;
  add_derived_clause ();
}

/*------------------------------------------------------------------------*/

void Proof::add_derived_clause (Clause *c, const vector<int64_t> &chain) {
  LOG (c, "PROOF adding to proof derived");
  CADICAL_assert (clause.empty ());
  CADICAL_assert (proof_chain.empty ());
  add_literals (c);
  for (const auto &cid : chain)
    proof_chain.push_back (cid);
  clause_id = c->id;
  redundant = c->redundant;
  add_derived_clause ();
}

void Proof::add_derived_clause (int64_t id, bool r, const vector<int> &c,
                                const vector<int64_t> &chain) {
  LOG (c, "PROOF adding derived clause");
  CADICAL_assert (clause.empty ());
  CADICAL_assert (proof_chain.empty ());
  for (const auto &lit : c)
    add_literal (lit);
  for (const auto &cid : chain)
    proof_chain.push_back (cid);
  clause_id = id;
  redundant = r;
  add_derived_clause ();
}

void Proof::add_assumption_clause (int64_t id, const vector<int> &c,
                                   const vector<int64_t> &chain) {
  // literals of c are already external
  CADICAL_assert (clause.empty ());
  CADICAL_assert (proof_chain.empty ());
  for (const auto &lit : c)
    clause.push_back (lit);
  for (const auto &cid : chain)
    proof_chain.push_back (cid);
  clause_id = id;
  add_assumption_clause ();
}

void Proof::add_assumption (int a) {
  // a is already external
  CADICAL_assert (clause.empty ());
  CADICAL_assert (proof_chain.empty ());
  clause.push_back (a);
  add_assumption ();
}

void Proof::add_constraint (const vector<int> &c) {
  // literals of c are already external
  CADICAL_assert (clause.empty ());
  CADICAL_assert (proof_chain.empty ());
  for (const auto &lit : c)
    clause.push_back (lit);
  add_constraint ();
}

void Proof::add_assumption_clause (int64_t id, int lit,
                                   const vector<int64_t> &chain) {
  CADICAL_assert (clause.empty ());
  CADICAL_assert (proof_chain.empty ());
  clause.push_back (lit);
  for (const auto &cid : chain)
    proof_chain.push_back (cid);
  clause_id = id;
  add_assumption_clause ();
}

void Proof::delete_clause (Clause *c) {
  LOG (c, "PROOF deleting from proof");
  clause.clear (); // Can be non-empty if an allocation fails during adding.
  add_literals (c);
  clause_id = c->id;
  redundant = c->redundant;
  delete_clause (); // Increments 'statistics.deleted'.
}

void Proof::delete_clause (int64_t id, bool r, const vector<int> &c) {
  LOG (c, "PROOF deleting from proof");
  CADICAL_assert (clause.empty ());
  add_literals (c);
  clause_id = id;
  redundant = r;
  delete_clause (); // Increments 'statistics.deleted'.
}

void Proof::weaken_minus (Clause *c) {
  LOG (c, "PROOF weaken minus of");
  CADICAL_assert (clause.empty ());
  add_literals (c);
  clause_id = c->id;
  weaken_minus ();
}

void Proof::weaken_minus (int64_t id, const vector<int> &c) {
  LOG (c, "PROOF deleting from proof");
  CADICAL_assert (clause.empty ());
  add_literals (c);
  clause_id = id;
  weaken_minus ();
}

void Proof::weaken_plus (Clause *c) {
  weaken_minus (c);
  delete_clause (c); // Increments 'statistics.deleted'.
}

void Proof::weaken_plus (int64_t id, const vector<int> &c) {
  weaken_minus (id, c);
  delete_clause (id, false, c); // Increments 'statistics.deleted'.
}

void Proof::delete_unit_clause (int64_t id, const int lit) {
  LOG ("PROOF deleting unit from proof %d", lit);
  CADICAL_assert (clause.empty ());
  add_literal (lit);
  clause_id = id;
  redundant = false;
  delete_clause ();
}

void Proof::finalize_clause (Clause *c) {
  LOG (c, "PROOF finalizing clause");
  CADICAL_assert (clause.empty ());
  add_literals (c);
  clause_id = c->id;
  finalize_clause ();
}

void Proof::finalize_clause (int64_t id, const vector<int> &c) {
  LOG (c, "PROOF finalizing clause");
  CADICAL_assert (clause.empty ());
  for (const auto &lit : c)
    add_literal (lit);
  clause_id = id;
  finalize_clause ();
}

void Proof::finalize_unit (int64_t id, int lit) {
  LOG ("PROOF finalizing clause %d", lit);
  CADICAL_assert (clause.empty ());
  add_literal (lit);
  clause_id = id;
  finalize_clause ();
}

void Proof::finalize_external_unit (int64_t id, int lit) {
  LOG ("PROOF finalizing clause %d", lit);
  CADICAL_assert (clause.empty ());
  clause.push_back (lit);
  clause_id = id;
  finalize_clause ();
}

/*------------------------------------------------------------------------*/

// During garbage collection clauses are shrunken by removing falsified
// literals. To avoid copying the clause, we provide a specialized tracing
// function here, which traces the required 'add' and 'remove' operations.

void Proof::flush_clause (Clause *c) {
  LOG (c, "PROOF flushing falsified literals in");
  CADICAL_assert (clause.empty ());
  const bool antecedents = (internal->lrat || internal->frat);
  for (int i = 0; i < c->size; i++) {
    int internal_lit = c->literals[i];
    if (internal->fixed (internal_lit) < 0) {
      if (antecedents) {
        int64_t id = internal->unit_id (-internal_lit);
        proof_chain.push_back (id);
      }
      continue;
    }
    add_literal (internal_lit);
  }
  proof_chain.push_back (c->id);
  redundant = c->redundant;
  int64_t id = ++internal->clause_id;
  clause_id = id;
  add_derived_clause ();
  delete_clause (c);
  c->id = id;
}

// While strengthening clauses, e.g., through self-subsuming resolutions,
// during subsumption checking, we have a similar situation, except that we
// have to remove exactly one literal.  Again the following function allows
// to avoid copying the clause and instead provides tracing of the required
// 'add' and 'remove' operations.

void Proof::strengthen_clause (Clause *c, int remove,
                               const vector<int64_t> &chain) {
  LOG (c, "PROOF strengthen by removing %d in", remove);
  CADICAL_assert (clause.empty ());
  for (int i = 0; i < c->size; i++) {
    int internal_lit = c->literals[i];
    if (internal_lit == remove)
      continue;
    add_literal (internal_lit);
  }
  int64_t id = ++internal->clause_id;
  clause_id = id;
  redundant = c->redundant;
  for (const auto &cid : chain)
    proof_chain.push_back (cid);
  add_derived_clause ();
  delete_clause (c);
  c->id = id;
}

void Proof::otfs_strengthen_clause (Clause *c, const std::vector<int> &old,
                                    const vector<int64_t> &chain) {
  LOG (c, "PROOF otfs strengthen");
  CADICAL_assert (clause.empty ());
  for (int i = 0; i < c->size; i++) {
    int internal_lit = c->literals[i];
    add_literal (internal_lit);
  }
  int64_t id = ++internal->clause_id;
  clause_id = id;
  redundant = c->redundant;
  for (const auto &cid : chain)
    proof_chain.push_back (cid);
  add_derived_clause ();
  delete_clause (c->id, c->redundant, old);
  c->id = id;
}

void Proof::strengthen (int64_t id) {
  clause_id = id;
  strengthen ();
}

/*------------------------------------------------------------------------*/

void Proof::add_original_clause (bool restore) {
  LOG (clause, "PROOF adding original external clause");
  CADICAL_assert (clause_id);

  for (auto &tracer : tracers) {
    tracer->add_original_clause (clause_id, false, clause, restore);
  }
  clause.clear ();
  clause_id = 0;
}

void Proof::add_derived_clause () {
  LOG (clause, "PROOF adding derived external clause (redundant: %d)",
       redundant);
  CADICAL_assert (clause_id);
  for (auto &tracer : tracers) {
    tracer->add_derived_clause (clause_id, redundant, clause, proof_chain);
  }
  proof_chain.clear ();
  clause.clear ();
  clause_id = 0;
}

void Proof::delete_clause () {
  LOG (clause, "PROOF deleting external clause");
  for (auto &tracer : tracers) {
    tracer->delete_clause (clause_id, redundant, clause);
  }
  clause.clear ();
  clause_id = 0;
}

void Proof::demote_clause () {
  LOG (clause, "PROOF demoting external clause");
  CADICAL_assert (!redundant);
  for (auto &tracer : tracers) {
    tracer->demote_clause (clause_id, clause);
  }
  clause.clear ();
  clause_id = 0;
}

void Proof::weaken_minus () {
  LOG (clause, "PROOF marking as clause to restore");
  for (auto &tracer : tracers) {
    tracer->weaken_minus (clause_id, clause);
  }
  clause.clear ();
  clause_id = 0;
}

void Proof::strengthen () {
  LOG ("PROOF strengthen clause with id %" PRId64, clause_id);
  for (auto &tracer : tracers) {
    tracer->strengthen (clause_id);
  }
  clause_id = 0;
}

void Proof::finalize_clause () {
  for (auto &tracer : tracers) {
    tracer->finalize_clause (clause_id, clause);
  }
  clause.clear ();
  clause_id = 0;
}

void Proof::add_assumption_clause () {
  LOG (clause, "PROOF adding assumption clause");
  for (auto &tracer : tracers) {
    tracer->add_assumption_clause (clause_id, clause, proof_chain);
  }
  proof_chain.clear ();
  clause.clear ();
  clause_id = 0;
}

void Proof::add_assumption () {
  LOG (clause, "PROOF adding assumption");
  CADICAL_assert (clause.size () == 1);
  for (auto &tracer : tracers) {
    tracer->add_assumption (clause.back ());
  }
  clause.clear ();
}

void Proof::add_constraint () {
  LOG (clause, "PROOF adding constraint");
  for (auto &tracer : tracers) {
    tracer->add_constraint (clause);
  }
  clause.clear ();
}

void Proof::reset_assumptions () {
  LOG ("PROOF reset assumptions");
  for (auto &tracer : tracers) {
    tracer->reset_assumptions ();
  }
}

void Proof::report_status (int status, int64_t id) {
  LOG ("PROOF reporting status %d", status);
  for (auto &tracer : tracers) {
    tracer->report_status (status, id);
  }
}

void Proof::begin_proof (int64_t id) {
  LOG (clause, "PROOF begin proof");
  for (auto &tracer : tracers) {
    tracer->begin_proof (id);
  }
}

void Proof::solve_query () {
  LOG (clause, "PROOF solve query");
  for (auto &tracer : tracers) {
    tracer->solve_query ();
  }
}

void Proof::conclude_unsat (ConclusionType con,
                            const vector<int64_t> &conclusion) {
  LOG (clause, "PROOF conclude unsat");
  for (auto &tracer : tracers) {
    tracer->conclude_unsat (con, conclusion);
  }
}

void Proof::conclude_sat (const vector<int> &model) {
  LOG (clause, "PROOF conclude sat");
  for (auto &tracer : tracers) {
    tracer->conclude_sat (model);
  }
}

void Proof::conclude_unknown (const vector<int> &trail) {
  LOG (clause, "PROOF conclude unknown");
  for (auto &tracer : tracers) {
    tracer->conclude_unknown (trail);
  }
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
