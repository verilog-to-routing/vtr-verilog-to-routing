#ifndef _drattracer_h_INCLUDED
#define _drattracer_h_INCLUDED

#include "global.h"

#include "tracer.hpp"

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

class DratTracer : public FileTracer {

  Internal *internal;
  File *file;
  bool binary;
#ifndef CADICAL_QUIET
  int64_t added, deleted;
#endif
  void put_binary_zero ();
  void put_binary_lit (int external_lit);

  // support DRAT
  void drat_add_clause (const vector<int> &);
  void drat_delete_clause (const vector<int> &);

public:
  // own and delete 'file'
  DratTracer (Internal *, File *file, bool binary);
  ~DratTracer ();

  void connect_internal (Internal *i) override;
  void begin_proof (int64_t) override {} // skip

  void add_original_clause (int64_t, bool, const vector<int> &,
                            bool = false) override {} // skip

  void add_derived_clause (int64_t, bool, const vector<int> &,
                           const vector<int64_t> &) override;

  void delete_clause (int64_t, bool, const vector<int> &) override;

  void finalize_clause (int64_t, const vector<int> &) override {} // skip

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
