#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

DratTracer::DratTracer (Internal *i, File *f, bool b)
    : internal (i), file (f), binary (b)
#ifndef CADICAL_QUIET
      ,
      added (0), deleted (0)
#endif
{
  (void) internal;
}

void DratTracer::connect_internal (Internal *i) {
  internal = i;
  file->connect_internal (internal);
  LOG ("DRAT TRACER connected to internal");
}

DratTracer::~DratTracer () {
  LOG ("DRAT TRACER delete");
  delete file;
}

/*------------------------------------------------------------------------*/

inline void DratTracer::put_binary_zero () {
  CADICAL_assert (binary);
  CADICAL_assert (file);
  file->put ((unsigned char) 0);
}

inline void DratTracer::put_binary_lit (int lit) {
  CADICAL_assert (binary);
  CADICAL_assert (file);
  CADICAL_assert (lit != INT_MIN);
  unsigned idx = abs (lit);
  CADICAL_assert (idx < (1u << 31));
  unsigned x = 2u * idx + (lit < 0);
  unsigned char ch;
  while (x & ~0x7f) {
    ch = (x & 0x7f) | 0x80;
    file->put (ch);
    x >>= 7;
  }
  ch = x;
  file->put (ch);
}

/*------------------------------------------------------------------------*/

void DratTracer::drat_add_clause (const vector<int> &clause) {
  if (binary)
    file->put ('a');
  for (const auto &external_lit : clause)
    if (binary)
      put_binary_lit (external_lit);
    else
      file->put (external_lit), file->put (' ');
  if (binary)
    put_binary_zero ();
  else
    file->put ("0\n");
}
void DratTracer::drat_delete_clause (const vector<int> &clause) {
  if (binary)
    file->put ('d');
  else
    file->put ("d ");
  for (const auto &external_lit : clause)
    if (binary)
      put_binary_lit (external_lit);
    else
      file->put (external_lit), file->put (' ');
  if (binary)
    put_binary_zero ();
  else
    file->put ("0\n");
}

/*------------------------------------------------------------------------*/

void DratTracer::add_derived_clause (int64_t, bool,
                                     const vector<int> &clause,
                                     const vector<int64_t> &) {
  if (file->closed ())
    return;
  LOG ("DRAT TRACER tracing addition of derived clause");
  drat_add_clause (clause);
#ifndef CADICAL_QUIET
  added++;
#endif
}

void DratTracer::delete_clause (int64_t, bool, const vector<int> &clause) {
  if (file->closed ())
    return;
  LOG ("DRAT TRACER tracing deletion of clause");
  drat_delete_clause (clause);
#ifndef CADICAL_QUIET
  deleted++;
#endif
}

/*------------------------------------------------------------------------*/

bool DratTracer::closed () { return file->closed (); }

#ifndef CADICAL_QUIET

void DratTracer::print_statistics () {
  uint64_t bytes = file->bytes ();
  uint64_t total = added + deleted;
  MSG ("DRAT %" PRId64 " added clauses %.2f%%", added,
       percent (added, total));
  MSG ("DRAT %" PRId64 " deleted clauses %.2f%%", deleted,
       percent (deleted, total));
  MSG ("DRAT %" PRId64 " bytes (%.2f MB)", bytes,
       bytes / (double) (1 << 20));
}

#endif

void DratTracer::close (bool print) {
  CADICAL_assert (!closed ());
  file->close ();
#ifndef CADICAL_QUIET
  if (print) {
    MSG ("DRAT proof file '%s' closed", file->name ());
    print_statistics ();
  }
#else
  (void) print;
#endif
}

void DratTracer::flush (bool print) {
  CADICAL_assert (!closed ());
  file->flush ();
#ifndef CADICAL_QUIET
  if (print) {
    MSG ("DRAT proof file '%s' flushed", file->name ());
    print_statistics ();
  }
#else
  (void) print;
#endif
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
