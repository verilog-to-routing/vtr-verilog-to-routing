#ifndef _lratchecker_hpp_INCLUDED
#define _lratchecker_hpp_INCLUDED

#include "global.h"

/*------------------------------------------------------------------------*/
#include <unordered_map>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// This checker implements an LRUP checker.
// It requires LRAT-style proof chains for each learned clause
//
// Most of the infrastructure is taken from checker, but without the
// propagation

/*------------------------------------------------------------------------*/

struct LratCheckerClause {
  LratCheckerClause *next; // collision chain link for hash table
  uint64_t hash;           // previously computed full 64-bit hash
  int64_t id;              // id of clause
  bool garbage;            // for garbage clauses
  unsigned size;
  bool used;
  bool tautological;
  int literals[1]; // 'literals' of length 'size'
};

/*------------------------------------------------------------------------*/

class LratChecker : public StatTracer {

  Internal *internal;

  // Capacity of variable values.
  //
  int64_t size_vars;

  // The 'watchers' and 'marks' data structures are not that time critical
  // and thus we access them by first mapping a literal to 'unsigned'.
  //
  static unsigned l2u (int lit);

  signed char &checked_lit (int lit);
  signed char &mark (int lit);

  vector<signed char> checked_lits;
  vector<signed char> marks; // mark bits of literals
  unordered_map<int64_t, vector<int>> clauses_to_reconstruct;
  vector<int> assumptions;
  vector<int> constraint;
  bool concluded;

  uint64_t num_clauses; // number of clauses in hash table
  uint64_t num_finalized;
  uint64_t num_garbage;        // number of garbage clauses
  uint64_t size_clauses;       // size of clause hash table
  LratCheckerClause **clauses; // hash table of clauses
  LratCheckerClause *garbage;  // linked list of garbage clauses

  vector<int> imported_clause; // original clause for reporting
  vector<int64_t> assumption_clauses;

  void enlarge_vars (int64_t idx);
  void import_literal (int lit);
  void import_clause (const vector<int> &);

  static const unsigned num_nonces = 4;

  uint64_t nonces[num_nonces];     // random numbers for hashing
  uint64_t last_hash;              // last computed hash value of clause
  int64_t last_id;                 // id of the last added/deleted clause
  int64_t current_id;              // id of the last added clause
  uint64_t compute_hash (int64_t); // compute and save hash value of clause

  // Reduce hash value to the actual size.
  //
  static uint64_t reduce_hash (uint64_t hash, uint64_t size);

  void enlarge_clauses (); // enlarge hash table for clauses
  void insert ();          // insert clause in hash table
  LratCheckerClause **
  find (const int64_t); // find clause position in hash table

  void add_clause (const char *type);

  void collect_garbage_clauses ();

  LratCheckerClause *new_clause ();
  void delete_clause (LratCheckerClause *);

  bool check (vector<int64_t>);            // check RUP
  bool check_resolution (vector<int64_t>); // check resolution
  bool check_blocked (vector<int64_t>);    // check ER

  struct {

    int64_t added;    // number of added clauses
    int64_t original; // number of added original clauses
    int64_t derived;  // number of added derived clauses

    int64_t deleted;   // number of deleted clauses
    int64_t finalized; // number of finalized clauses

    int64_t insertions; // number of clauses added to hash table
    int64_t collisions; // number of hash collisions in 'find'
    int64_t searches;   // number of searched clauses in 'find'

    int64_t checks; // number of implication checks

    int64_t collections; // garbage collections

  } stats;

public:
  LratChecker (Internal *);
  virtual ~LratChecker ();

  void connect_internal (Internal *i) override;
  void begin_proof (int64_t) override;

  void add_original_clause (int64_t, bool, const vector<int> &,
                            bool restore) override;
  void restore_clause (int64_t, const vector<int> &);

  // check the proof chain for the new clause and add it to the checker
  void add_derived_clause (int64_t, bool, const vector<int> &,
                           const vector<int64_t> &) override;

  // check if the clause is present and delete it from the checker
  void delete_clause (int64_t, bool, const vector<int> &) override;
  // check if the clause is present and delete it from the checker
  void weaken_minus (int64_t, const vector<int> &) override;

  // check if the clause is present and delete it from the checker
  void finalize_clause (int64_t, const vector<int> &) override;

  // check the proof chain of the assumption clause and delete it
  // immediately also check that they contain only assumptions and
  // constraints
  void add_assumption_clause (int64_t, const vector<int> &,
                              const vector<int64_t> &) override;

  // mark lit as assumption
  void add_assumption (int) override;

  // mark lits as constraint
  void add_constraint (const vector<int> &) override;

  void reset_assumptions () override;

  // check if all clauses have been deleted
  void report_status (int, int64_t) override;

  void conclude_unsat (ConclusionType, const vector<int64_t> &) override;

  void print_stats () override;
  void dump (); // for debugging purposes only
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
