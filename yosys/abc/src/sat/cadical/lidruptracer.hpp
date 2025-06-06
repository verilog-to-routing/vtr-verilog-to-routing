#ifndef _lidruptracer_h_INCLUDED
#define _lidruptracer_h_INCLUDED

#include "global.h"

ABC_NAMESPACE_CXX_HEADER_START

class FileTracer;

namespace CaDiCaL {

struct LidrupClause {
  LidrupClause *next; // collision chain link for hash table
  uint64_t hash;      // previously computed full 64-bit hash
  int64_t id;         // id of clause
  std::vector<int64_t> chain;
  std::vector<int> literals;
};

class LidrupTracer : public FileTracer {

  Internal *internal;
  File *file;
  bool binary;
  bool piping; // The 'file' is a pipe and needs eagerly flushing.

  // hash table for conclusion
  //
  uint64_t num_clauses;   // number of clauses in hash table
  uint64_t size_clauses;  // size of clause hash table
  LidrupClause **clauses; // hash table of clauses
  vector<int> imported_clause;
  vector<int> assumptions;
  vector<int64_t> imported_chain;
  vector<int64_t> batch_weaken;
  vector<int64_t> batch_delete;
  vector<int64_t> batch_restore;

  static const unsigned num_nonces = 4;

  uint64_t nonces[num_nonces]; // random numbers for hashing
  uint64_t last_hash;          // last computed hash value of clause
  int64_t last_id;             // id of the last added clause
  LidrupClause *last_clause;
  uint64_t compute_hash (int64_t); // compute and save hash value of clause

  LidrupClause *new_clause ();
  void delete_clause (LidrupClause *);

  static uint64_t reduce_hash (uint64_t hash, uint64_t size);

  void enlarge_clauses (); // enlarge hash table for clauses
  void insert ();          // insert clause in hash table
  bool
  find_and_delete (const int64_t); // find clause position in hash table

#ifndef CADICAL_QUIET
  int64_t added, deleted, weakened, restore, original, solved, batched;
#endif

  void flush_if_piping ();

  void put_binary_zero ();
  void put_binary_lit (int external_lit);
  void put_binary_id (int64_t id, bool = true);

  void lidrup_add_derived_clause (int64_t id, const vector<int> &clause,
                                  const vector<int64_t> &chain);
  void lidrup_delete_clause (int64_t id); //, const vector<int> &clause);
  void
  lidrup_add_restored_clause (int64_t id); //, const vector<int> &clause);
  void lidrup_add_original_clause (int64_t id, const vector<int> &clause);
  void lidrup_conclude_and_delete (const vector<int64_t> &conclusion);
  void lidrup_report_status (int status);
  void lidrup_conclude_sat (const vector<int> &model);
  void lidrup_conclude_unknown (const vector<int> &trail);
  void lidrup_solve_query ();
  void lidrup_batch_weaken_restore_and_delete ();

public:
  LidrupTracer (Internal *, File *file, bool);
  ~LidrupTracer ();

  // proof section:
  void add_derived_clause (int64_t, bool, const vector<int> &,
                           const vector<int64_t> &) override;
  void add_assumption_clause (int64_t, const vector<int> &,
                              const vector<int64_t> &) override;
  void weaken_minus (int64_t, const vector<int> &) override;
  void delete_clause (int64_t, bool, const vector<int> &) override;
  void add_original_clause (int64_t, bool, const vector<int> &,
                            bool = false) override;
  void report_status (int, int64_t) override;
  void conclude_sat (const vector<int> &) override;
  void conclude_unsat (ConclusionType, const vector<int64_t> &) override;
  void conclude_unknown (const vector<int> &) override;

  void solve_query () override;
  void add_assumption (int) override;
  void reset_assumptions () override;

  // skip
  void begin_proof (int64_t) override {}
  void finalize_clause (int64_t, const vector<int> &) override {}
  void strengthen (int64_t) override {}
  void add_constraint (const vector<int> &) override {}

  // logging and file io
  void connect_internal (Internal *i) override;

#ifndef CADICAL_QUIET
  void print_statistics ();
#endif
  bool closed () override;
  void close (bool) override;
  void flush (bool) override;
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
