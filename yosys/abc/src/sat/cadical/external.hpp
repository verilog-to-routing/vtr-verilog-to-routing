#ifndef _external_hpp_INCLUDED
#define _external_hpp_INCLUDED

#include "global.h"

/*------------------------------------------------------------------------*/

#include "range.hpp"
#include <unordered_map>
#include <vector>

/*------------------------------------------------------------------------*/

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

using namespace std;

/*------------------------------------------------------------------------*/

// The CaDiCaL code is split into three layers:
//
//   Solver:       facade object providing the actual API of the solver
//   External:     communication layer between 'Solver' and 'Internal'
//   Internal:     the actual solver code
//
// Note, that 'Solver' is defined in 'cadical.hpp' and 'solver.cpp', while
// 'External' and 'Internal' in '{external,internal}.{hpp,cpp}'.
//
// Also note, that any user should access the library only through the
// 'Solver' API.  For the library internal 'Parser' code we make an
// exception and allow access to both 'External' and 'Internal'.  The former
// to enforce the same external to internal mapping of variables and the
// latter for profiling and messages.  The same applies to 'App'.
//
// The 'External' class provided here stores the information needed to map
// external variable indices to internal variables (actually literals).
// This is helpful for shrinking the working size of the internal solver
// after many variables become inactive.  It will also help to provide
// support for extended resolution in the future, since it allows to
// introduce only internally visible variables (even though we do not know
// how to support generating incremental proofs in this situation yet).
//
// External literals are usually called 'elit' and internal 'ilit'.

/*------------------------------------------------------------------------*/

struct Clause;
struct Internal;
struct CubesWithStatus;

/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/

struct External {

  /*==== start of state ==================================================*/

  Internal *internal; // The actual internal solver.

  int max_var;  // External maximum variable index.
  size_t vsize; // Allocated external size.

  vector<bool> vals; // Current external (extended) assignment.
  vector<int> e2i;   // External 'idx' to internal 'lit'.

  vector<int> assumptions; // External assumptions.
  vector<int> constraint;  // External constraint. Terminated by zero.

  vector<int64_t>
      ext_units; // External units. Needed to compute LRAT for eclause
  vector<bool> ext_flags; // to avoid duplicate units
  vector<int> eclause;    // External version of original input clause.
  // The extension stack for reconstructing complete satisfying assignments
  // (models) of the original external formula is kept in this external
  // solver object. It keeps track of blocked clauses and clauses containing
  // eliminated variable.  These irredundant clauses are stored in terms of
  // external literals on the 'extension' stack after mapping the
  // internal literals given as arguments with 'externalize'.

  bool extended; // Have been extended.
  bool concluded;
  vector<int> extension; // Solution reconstruction extension stack.

  vector<bool> witness; // Literal witness on extension stack.
  vector<bool> tainted; // Literal tainted in adding literals.

  vector<bool> ervars; // Variables added through Extended Resolution.

  vector<unsigned> frozentab; // Reference counts for frozen variables.

  // Regularly checked terminator if non-zero.  The terminator is set from
  // 'Solver::set (Terminator *)' and checked by 'Internal::terminating ()'.

  Terminator *terminator;

  // If there is a learner export learned clauses.

  Learner *learner;

  void export_learned_empty_clause ();
  void export_learned_unit_clause (int ilit);
  void export_learned_large_clause (const vector<int> &);

  // If there is a listener for fixed assignments.

  FixedAssignmentListener *fixed_listener;

  // If there is an external propagator.

  ExternalPropagator *propagator;

  vector<bool> is_observed; // Quick flag for each external variable

  // Saved 'forgettable' original clauses coming from the external
  // propagator. The value of the map starts with a Boolean flag indicating
  // if the clause is still present or got already deleted, and then
  // followed by the literals of the clause.
  unordered_map<uint64_t, vector<int>> forgettable_original;

  void add_observed_var (int elit);
  void remove_observed_var (int elit);
  void reset_observed_vars ();

  bool observed (int elit);
  bool is_witness (int elit);
  bool is_decision (int elit);

  void force_backtrack (size_t new_level);

  //----------------------------------------------------------------------//

  signed char *solution; // Given solution checking for debugging.
  int solution_size;     // Given solution checking for debugging.
  vector<int> original;  // Saved original formula for checking.

  // If 'opts.checkfrozen' is set make sure that only literals are added
  // which were never completely molten before.  These molten literals are
  // marked at the beginning of the 'solve' call.  Note that variables
  // larger than 'max_var' are not molten and can thus always be used in the
  // future.  Only needed to check and debug old style freeze semantics.
  //
  vector<bool> moltentab;

  //----------------------------------------------------------------------//

  const Range vars; // Provides safe variable iterations.

  /*==== end of state ====================================================*/

  // These two just factor out common sanity (CADICAL_assertion) checking code.

  inline int vidx (int elit) const {
    CADICAL_assert (elit);
    CADICAL_assert (elit != INT_MIN);
    int res = abs (elit);
    CADICAL_assert (res <= max_var);
    return res;
  }

  inline int vlit (int elit) const {
    CADICAL_assert (elit);
    CADICAL_assert (elit != INT_MIN);
    CADICAL_assert (abs (elit) <= max_var);
    return elit;
  }

  inline bool is_valid_input (int elit) {
    CADICAL_assert (elit);
    CADICAL_assert (elit != INT_MIN);
    int eidx = abs (elit);
    return eidx > max_var || !ervars[eidx];
  }

  /*----------------------------------------------------------------------*/

  // The following five functions push individual literals or clauses on the
  // extension stack.  They all take internal literals as argument, and map
  // them back to external literals first, before pushing them on the stack.

  void push_zero_on_extension_stack ();

  // Our general version of extension stacks always pushes a set of witness
  // literals (for variable elimination the literal of the eliminated
  // literal and for blocked clauses the blocking literal) followed by all
  // the clause literals starting with and separated by zero.
  //
  void push_clause_literal_on_extension_stack (int ilit);
  void push_witness_literal_on_extension_stack (int ilit);

  void push_clause_on_extension_stack (Clause *);
  void push_clause_on_extension_stack (Clause *, int witness);
  void push_binary_clause_on_extension_stack (int64_t id, int witness,
                                              int other);

  // The main 'extend' function which extends an internal assignment to an
  // external assignment using the extension stack (and sets 'extended').
  //
  void extend ();
  void conclude_sat ();

  /*----------------------------------------------------------------------*/

  // Marking external literals.

  unsigned elit2ulit (int elit) const {
    CADICAL_assert (elit);
    CADICAL_assert (elit != INT_MIN);
    const int idx = abs (elit) - 1;
    CADICAL_assert (idx <= max_var);
    return 2u * idx + (elit < 0);
  }

  bool marked (const vector<bool> &map, int elit) const {
    const unsigned ulit = elit2ulit (elit);
    return ulit < map.size () ? map[ulit] : false;
  }

  void mark (vector<bool> &map, int elit) {
    const unsigned ulit = elit2ulit (elit);
    if (ulit >= map.size ())
      map.resize (ulit + 1, false);
    map[ulit] = true;
  }

  void unmark (vector<bool> &map, int elit) {
    const unsigned ulit = elit2ulit (elit);
    if (ulit < map.size ())
      map[ulit] = false;
  }

  /*----------------------------------------------------------------------*/

  void push_external_clause_and_witness_on_extension_stack (
      const vector<int> &clause, const vector<int> &witness, int64_t id);

  void push_id_on_extension_stack (int64_t id);

  // Restore a clause, which was pushed on the extension stack.
  void restore_clause (const vector<int>::const_iterator &begin,
                       const vector<int>::const_iterator &end,
                       const int64_t id);

  void restore_clauses ();

  /*----------------------------------------------------------------------*/

  // Explicitly freeze and melt literals (instead of just freezing
  // internally and implicitly assumed literals).  Passes on freezing and
  // melting to the internal solver, which has separate frozen counters.

  void freeze (int elit);
  void melt (int elit);

  bool frozen (int elit) {
    CADICAL_assert (elit);
    CADICAL_assert (elit != INT_MIN);
    int eidx = abs (elit);
    if (eidx > max_var)
      return false;
    if (eidx >= (int) frozentab.size ())
      return false;
    return frozentab[eidx] > 0;
  }

  /*----------------------------------------------------------------------*/

  External (Internal *);
  ~External ();

  void enlarge (int new_max_var); // Enlarge allocated 'vsize'.
  void init (int new_max_var,
             bool extension = false); // Initialize up-to 'new_max_var'.

  int internalize (
      int,
      bool extension = false); // Translate external to internal literal.

  /*----------------------------------------------------------------------*/

  // According to the CaDiCaL API contract (as well as IPASIR) we have to
  // forget about the previous assumptions after a 'solve' call.  This
  // should however be delayed until we transition out of an 'UNSATISFIED'
  // state, i.e., after no more 'failed' calls are expected.  Note that
  // 'failed' requires to know the failing assumptions, and the 'failed'
  // status of those should cleared before at start of the next 'solve'.
  // As a consequence 'reset_assumptions' is only called from
  // 'transition_to_unknown_state' in API calls in 'solver.cpp'.

  void reset_assumptions ();

  // Similarly to 'failed', 'conclude' needs to know about failing
  // assumptions and therefore needs to be reset when leaving the
  // 'UNSATISFIED' state.
  //
  void reset_concluded ();

  // Similarly a valid external assignment obtained through 'extend' has to
  // be reset at each point it risks to become invalid.  This is done
  // in the external layer in 'external.cpp' functions..

  void reset_extended ();

  // Finally, the semantics of incremental solving also require that limits
  // are only valid for the next 'solve' call.  Since the limits can not
  // really be queried, handling them is less complex and they are just
  // reset immediately at the end of 'External::solve'.

  void reset_limits ();

  /*----------------------------------------------------------------------*/

  // Proxies to IPASIR functions.

  void add (int elit);
  void assume (int elit);
  int solve (bool preprocess_only);

  // We call it 'ival' as abbreviation for 'val' with 'int' return type to
  // avoid bugs due to using 'signed char tmp = val (lit)', which might turn
  // a negative value into a positive one (happened in 'extend').
  //
  inline int ival (int elit) const {
    CADICAL_assert (elit != INT_MIN);
    int eidx = abs (elit);
    bool val = false;
    if (eidx <= max_var && (size_t) eidx < vals.size ())
      val = vals[eidx];
    if (elit < 0)
      val = !val;
    return val ? elit : -elit;
  }

  bool flip (int elit);
  bool flippable (int elit);

  bool failed (int elit);

  void terminate ();

  // Other important non IPASIR functions.

  /*----------------------------------------------------------------------*/

  // Add literal to external constraint.
  //
  void constrain (int elit);

  // Returns true if 'solve' returned 20 because of the constraint.
  //
  bool failed_constraint ();

  // Deletes the current constraint clause. Called on
  // 'transition_to_unknown_state' and if a new constraint is added. Can be
  // called directly using the API.
  //
  void reset_constraint ();

  /*----------------------------------------------------------------------*/

  int propagate_assumptions ();
  void implied (std::vector<int> &entrailed);
  void conclude_unknown ();

  /*----------------------------------------------------------------------*/
  int lookahead ();
  CaDiCaL::CubesWithStatus generate_cubes (int, int);

  int fixed (int elit) const; // Implemented in 'internal.hpp'.

  /*----------------------------------------------------------------------*/

  void phase (int elit);
  void unphase (int elit);

  /*----------------------------------------------------------------------*/

  // Traversal functions for the witness stack and units.  The explanation
  // in 'external.cpp' for why we have to distinguish these cases.

  bool traverse_all_frozen_units_as_clauses (ClauseIterator &);
  bool traverse_all_non_frozen_units_as_witnesses (WitnessIterator &);
  bool traverse_witnesses_backward (WitnessIterator &);
  bool traverse_witnesses_forward (WitnessIterator &);

  /*----------------------------------------------------------------------*/

  // Copy flags for determining preprocessing state.

  void copy_flags (External &other) const;

  /*----------------------------------------------------------------------*/

  // Check solver behaves as expected during testing and debugging.

  void check_assumptions_satisfied ();
  void check_constraint_satisfied ();
  void check_failing ();

  void check_solution_on_learned_clause ();
  void check_solution_on_shrunken_clause (Clause *);
  void check_solution_on_learned_unit_clause (int unit);
  void check_no_solution_after_learning_empty_clause ();

  void check_learned_empty_clause () {
    if (solution)
      check_no_solution_after_learning_empty_clause ();
  }

  void check_learned_unit_clause (int unit) {
    if (solution)
      check_solution_on_learned_unit_clause (unit);
  }

  void check_learned_clause () {
    if (solution)
      check_solution_on_learned_clause ();
  }

  void check_shrunken_clause (Clause *c) {
    if (solution)
      check_solution_on_shrunken_clause (c);
  }

  void check_assignment (int (External::*assignment) (int) const);

  void check_satisfiable ();
  void check_unsatisfiable ();

  void check_solve_result (int res);

  void update_molten_literals ();

  /*----------------------------------------------------------------------*/

  // For debugging and testing only.  See 'solution.hpp' for more details.
  // TODO: if elit > solution_size, elit is an extension variable. For now
  // the clause will count as satisfied regardless. For the future one
  // should check that actually there is one consistent extension for the
  // solution that satisfies the clauses with this extension variable (by
  // setting it to a value once a clause is learned which is not satisfied
  // already).
  //
  inline int sol (int elit) const {
    CADICAL_assert (solution);
    CADICAL_assert (elit != INT_MIN);
    int eidx = abs (elit);
    if (eidx > max_var)
      return 0;
    else if (eidx > solution_size)
      return elit;
    signed char value = solution[eidx];
    if (!value)
      return 0;
    if (elit < 0)
      value = -value;
    return value > 0 ? elit : -elit;
  }
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
