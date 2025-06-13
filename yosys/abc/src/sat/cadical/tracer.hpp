#ifndef _tracer_hpp_INCLUDED
#define _tracer_hpp_INCLUDED

#include "global.h"

#include <cstdint>
#include <vector>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

struct Internal;

enum ConclusionType { CONFLICT = 1, ASSUMPTIONS = 2, CONSTRAINT = 4 };

// Proof tracer class to observer all possible proof events,
// such as added or deleted clauses.
// An implementation can decide on which events to act.
//
class Tracer {

public:
  Tracer () {}
  virtual ~Tracer () {}

  /*------------------------------------------------------------------------*/
  /*                                                                        */
  /*                            Basic Events */
  /*                                                                        */
  /*------------------------------------------------------------------------*/

  // Notify the tracer that a original clause has been added.
  // Includes ID and whether the clause is redundant or irredundant
  // Arguments: ID, redundant, clause, restored
  //
  virtual void add_original_clause (int64_t, bool, const std::vector<int> &,
                                    bool = false) {}

  // Notify the observer that a new clause has been derived.
  // Includes ID and whether the clause is redundant or irredundant
  // If antecedents are derived they will be included here.
  // Arguments: ID, redundant, clause, antecedents
  //
  virtual void add_derived_clause (int64_t, bool, const std::vector<int> &,
                                   const std::vector<int64_t> &) {}

  // Notify the observer that a clause is deleted.
  // Includes ID and redundant/irredundant
  // Arguments: ID, redundant, clause
  //
  virtual void delete_clause (int64_t, bool, const std::vector<int> &) {}

  // Notify the observer that a clause is deleted.
  // Includes ID and redundant/irredundant
  // Arguments: ID, redundant, clause
  //
  virtual void demote_clause (uint64_t, const std::vector<int> &) {}

  // Notify the observer to remember that the clause might be restored later
  // Arguments: ID, clause
  //
  virtual void weaken_minus (int64_t, const std::vector<int> &) {}

  // Notify the observer that a clause is strengthened
  // Arguments: ID
  //
  virtual void strengthen (int64_t) {}

  // Notify the observer that the solve call ends with status StatusType
  // If the status is UNSAT and an empty clause has been derived, the second
  // argument will contain its id.
  // Note that the empty clause is already added through add_derived_clause
  // and finalized with finalize_clause
  // Arguments: int, ID
  //
  virtual void report_status (int, int64_t) {}

  /*------------------------------------------------------------------------*/
  /*                                                                        */
  /*                   Specifically non-incremental */
  /*                                                                        */
  /*------------------------------------------------------------------------*/

  // Notify the observer that a clause is finalized.
  // Arguments: ID, clause
  //
  virtual void finalize_clause (int64_t, const std::vector<int> &) {}

  // Notify the observer that the proof begins with a set of reserved ids
  // for original clauses. Given ID is the first derived clause ID.
  // Arguments: ID
  //
  virtual void begin_proof (int64_t) {}

  /*------------------------------------------------------------------------*/
  /*                                                                        */
  /*                      Specifically incremental */
  /*                                                                        */
  /*------------------------------------------------------------------------*/

  // Notify the observer that an assumption has been added
  // Arguments: assumption_literal
  //
  virtual void solve_query () {}

  // Notify the observer that an assumption has been added
  // Arguments: assumption_literal
  //
  virtual void add_assumption (int) {}

  // Notify the observer that a constraint has been added
  // Arguments: constraint_clause
  //
  virtual void add_constraint (const std::vector<int> &) {}

  // Notify the observer that assumptions and constraints are reset
  //
  virtual void reset_assumptions () {}

  // Notify the observer that this clause could be derived, which
  // is the negation of a core of failing assumptions/constraints.
  // If antecedents are derived they will be included here.
  // Arguments: ID, clause, antecedents
  //
  virtual void add_assumption_clause (int64_t, const std::vector<int> &,
                                      const std::vector<int64_t> &) {}

  // Notify the observer that conclude unsat was requested.
  // will give either the id of the empty clause, the id of a failing
  // assumption clause or the ids of the failing constrain clauses
  // Arguments: conclusion_type, clause_ids
  //
  virtual void conclude_unsat (ConclusionType,
                               const std::vector<int64_t> &) {}

  // Notify the observer that conclude sat was requested.
  // will give the complete model as a vector.
  //
  virtual void conclude_sat (const std::vector<int> &) {}

  // Notify the observer that conclude unknown was requested.
  // will give the current trail as a vector.
  //
  virtual void conclude_unknown (const std::vector<int> &) {}
};

/*--------------------------------------------------------------------------*/

// Following tracers for internal use.

struct InternalTracer : public Tracer {
public:
  InternalTracer () {}
  virtual ~InternalTracer () {}

  virtual void connect_internal (Internal *) {}
};

class StatTracer : public InternalTracer {
public:
  StatTracer () {}
  virtual ~StatTracer () {}

  virtual void print_stats () {}
};

class FileTracer : public InternalTracer {

public:
  FileTracer () {}
  virtual ~FileTracer () {}

  virtual bool closed () = 0;
  virtual void close (bool print = false) = 0;
  virtual void flush (bool print = false) = 0;
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
