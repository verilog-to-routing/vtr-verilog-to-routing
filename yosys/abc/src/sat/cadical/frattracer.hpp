#ifndef _frattracer_h_INCLUDED
#define _frattracer_h_INCLUDED

#include "global.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

class FratTracer : public FileTracer {

  Internal *internal;
  File *file;
  bool binary;
  bool with_antecedents;

#ifndef CADICAL_QUIET
  int64_t added, deleted;
  int64_t finalized, original;
#endif

  vector<int64_t> delete_ids;

  void put_binary_zero ();
  void put_binary_lit (int external_lit);
  void put_binary_id (int64_t id, bool = false);

  // support FRAT
  void frat_add_original_clause (int64_t, const vector<int> &);
  void frat_add_derived_clause (int64_t, const vector<int> &);
  void frat_add_derived_clause (int64_t, const vector<int> &,
                                const vector<int64_t> &);
  void frat_delete_clause (int64_t, const vector<int> &);
  void frat_finalize_clause (int64_t, const vector<int> &);

public:
  // own and delete 'file'
  FratTracer (Internal *, File *file, bool binary, bool antecedents);
  ~FratTracer ();

  void connect_internal (Internal *i) override;
  void begin_proof (int64_t) override {} // skip

  void add_original_clause (int64_t, bool, const vector<int> &,
                            bool = false) override;

  void add_derived_clause (int64_t, bool, const vector<int> &,
                           const vector<int64_t> &) override;

  void delete_clause (int64_t, bool, const vector<int> &) override;

  void finalize_clause (int64_t, const vector<int> &) override;

  void report_status (int, int64_t) override {} // skip

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
