#ifndef _checker_hpp_INCLUDED
#define _checker_hpp_INCLUDED

#include "global.h"

#include "tracer.hpp" // Alphabetically after 'checker'.

#include <cstdint>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// This checker implements an online forward DRUP proof checker enabled by
// 'opts.checkproof' (requires 'opts.check' also to be enabled).  This is
// useful for model basted testing (and delta-debugging), where we can not
// rely on an external proof checker such as 'drat-trim'.  We also do not
// have yet  a flow for offline incremental proof checking, while this
// checker here can also be used in an incremental setting.
//
// In essence the checker implements is a simple propagation online SAT
// solver with an additional hash table to find clauses fast for
// 'delete_clause'.  It requires its own data structure for clauses
// ('CheckerClause') and watches ('CheckerWatch').
//
// In our experiments the checker slows down overall SAT solving time by a
// factor of 3, which we contribute to its slightly less efficient
// implementation.

/*------------------------------------------------------------------------*/

struct CheckerClause {
  CheckerClause *next; // collision chain link for hash table
  uint64_t hash;       // previously computed full 64-bit hash
  unsigned size;       // zero if this is a garbage clause
  int literals[2];     // otherwise 'literals' of length 'size'
};

struct CheckerWatch {
  int blit;
  unsigned size;
  CheckerClause *clause;
  CheckerWatch () {}
  CheckerWatch (int b, CheckerClause *c)
      : blit (b), size (c->size), clause (c) {}
};

typedef vector<CheckerWatch> CheckerWatcher;

/*------------------------------------------------------------------------*/

class Checker : public StatTracer {

  Internal *internal;

  // Capacity of variable values.
  //
  int64_t size_vars;

  // For the assignment we want to have an as fast access as possible and
  // thus we use an array which can also be indexed by negative literals and
  // is actually valid in the range [-size_vars+1, ..., size_vars-1].
  //
  signed char *vals;

  // The 'watchers' and 'marks' data structures are not that time critical
  // and thus we access them by first mapping a literal to 'unsigned'.
  //
  static unsigned l2u (int lit);
  vector<CheckerWatcher> watchers; // watchers of literals
  vector<signed char> marks;       // mark bits of literals

  signed char &mark (int lit);
  CheckerWatcher &watcher (int lit);

  bool inconsistent; // found or added empty clause

  uint64_t num_clauses;    // number of clauses in hash table
  uint64_t num_garbage;    // number of garbage clauses
  uint64_t size_clauses;   // size of clause hash table
  CheckerClause **clauses; // hash table of clauses
  CheckerClause *garbage;  // linked list of garbage clauses

  vector<int> unsimplified; // original clause for reporting
  vector<int> simplified;   // clause for sorting

  vector<int> trail; // for propagation

  unsigned next_to_propagate; // next to propagate on trail

  void enlarge_vars (int64_t idx);
  void import_literal (int lit);
  void import_clause (const vector<int> &);
  bool tautological ();

  static const unsigned num_nonces = 4;

  uint64_t nonces[num_nonces]; // random numbers for hashing
  uint64_t last_hash;          // last computed hash value of clause
  int64_t last_id;
  uint64_t compute_hash (); // compute and save hash value of clause

  // Reduce hash value to the actual size.
  //
  static uint64_t reduce_hash (uint64_t hash, uint64_t size);

  void enlarge_clauses (); // enlarge hash table for clauses
  void insert ();          // insert clause in hash table
  CheckerClause **find (); // find clause position in hash table

  void add_clause (const char *type);

  void collect_garbage_clauses ();

  CheckerClause *new_clause ();
  void delete_clause (CheckerClause *);

  signed char val (int lit); // returns '-1', '0' or '1'

  bool clause_satisfied (CheckerClause *);

  void assign (int lit);     // assign a literal to true
  void assume (int lit);     // assume a literal
  bool propagate ();         // propagate and check for conflicts
  void backtrack (unsigned); // prepare for next clause
  bool check ();             // check simplified clause is implied
  bool check_blocked ();     // check if clause is blocked

  struct {

    int64_t added;    // number of added clauses
    int64_t original; // number of added original clauses
    int64_t derived;  // number of added derived clauses

    int64_t deleted; // number of deleted clauses

    int64_t assumptions;  // number of assumed literals
    int64_t propagations; // number of propagated literals

    int64_t insertions; // number of clauses added to hash table
    int64_t collisions; // number of hash collisions in 'find'
    int64_t searches;   // number of searched clauses in 'find'

    int64_t checks; // number of implication checks

    int64_t collections; // garbage collections
    int64_t units;

  } stats;

public:
  Checker (Internal *);
  virtual ~Checker ();

  void connect_internal (Internal *i) override;

  void add_original_clause (int64_t, bool, const vector<int> &,
                            bool = false) override;
  void add_derived_clause (int64_t, bool, const vector<int> &,
                           const vector<int64_t> &) override;
  void delete_clause (int64_t, bool, const vector<int> &) override;

  void finalize_clause (int64_t, const vector<int> &) override {} // skip
  void report_status (int, int64_t) override {}                   // skip
  void begin_proof (int64_t) override {}                          // skip
  void add_assumption_clause (int64_t, const vector<int> &,
                              const vector<int64_t> &) override;
  void print_stats () override;
  void dump (); // for debugging purposes only
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
