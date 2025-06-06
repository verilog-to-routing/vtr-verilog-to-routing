#include "global.h"

#include "internal.hpp"

#include <limits.h>

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

LratTracer::LratTracer (Internal *i, File *f, bool b)
    : internal (i), file (f), binary (b)
#ifndef CADICAL_QUIET
      ,
      added (0), deleted (0)
#endif
      ,
      latest_id (0) {
  (void) internal;
}

void LratTracer::connect_internal (Internal *i) {
  internal = i;
  file->connect_internal (internal);
  LOG ("LRAT TRACER connected to internal");
}

LratTracer::~LratTracer () {
  LOG ("LRAT TRACER delete");
  delete file;
}

/*------------------------------------------------------------------------*/

inline void LratTracer::put_binary_zero () {
  CADICAL_assert (binary);
  CADICAL_assert (file);
  file->put ((unsigned char) 0);
}

inline void LratTracer::put_binary_lit (int lit) {
  CADICAL_assert (binary);
  CADICAL_assert (file);
  CADICAL_assert (lit != INT_MIN);
  unsigned idx = abs (lit);
  CADICAL_assert (idx < (1u << 31));
  unsigned x = 2 * idx + (lit < 0);
  unsigned char ch;
  while (x & ~0x7f) {
    ch = (x & 0x7f) | 0x80;
    file->put (ch);
    x >>= 7;
  }
  ch = x;
  file->put (ch);
}

inline void LratTracer::put_binary_id (int64_t id) {
  CADICAL_assert (binary);
  CADICAL_assert (file);
  uint64_t x = abs (id);
  x = 2 * x + (id < 0);
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

void LratTracer::lrat_add_clause (int64_t id, const vector<int> &clause,
                                  const vector<int64_t> &chain) {
  if (delete_ids.size ()) {
    if (!binary)
      file->put (latest_id), file->put (" ");
    if (binary)
      file->put ('d');
    else
      file->put ("d ");
    for (auto &did : delete_ids) {
      if (binary)
        put_binary_id (did);
      else
        file->put (did), file->put (" ");
    }
    if (binary)
      put_binary_zero ();
    else
      file->put ("0\n");
    delete_ids.clear ();
  }
  latest_id = id;

  if (binary)
    file->put ('a'), put_binary_id (id);
  else
    file->put (id), file->put (" ");
  for (const auto &external_lit : clause)
    if (binary)
      put_binary_lit (external_lit);
    else
      file->put (external_lit), file->put (' ');
  if (binary)
    put_binary_zero ();
  else
    file->put ("0 ");
  for (const auto &c : chain)
    if (binary)
      put_binary_id (c);
    else
      file->put (c), file->put (' '); // in proof chain, so they get
  if (binary)
    put_binary_zero (); // since cadical has no rat-steps
  else
    file->put ("0\n"); // this is just 2c here
}

void LratTracer::lrat_delete_clause (int64_t id) {
  delete_ids.push_back (id); // pushing off deletion for later
}

/*------------------------------------------------------------------------*/

void LratTracer::add_derived_clause (int64_t id, bool,
                                     const vector<int> &clause,
                                     const vector<int64_t> &chain) {
  if (file->closed ())
    return;
  LOG ("LRAT TRACER tracing addition of derived clause");
  lrat_add_clause (id, clause, chain);
#ifndef CADICAL_QUIET
  added++;
#endif
}

void LratTracer::delete_clause (int64_t id, bool, const vector<int> &) {
  if (file->closed ())
    return;
  LOG ("LRAT TRACER tracing deletion of clause");
  lrat_delete_clause (id);
#ifndef CADICAL_QUIET
  deleted++;
#endif
}

void LratTracer::begin_proof (int64_t id) {
  if (file->closed ())
    return;
  LOG ("LRAT TRACER tracing begin of proof");
  latest_id = id;
}

/*------------------------------------------------------------------------*/

bool LratTracer::closed () { return file->closed (); }

#ifndef CADICAL_QUIET

void LratTracer::print_statistics () {
  uint64_t bytes = file->bytes ();
  uint64_t total = added + deleted;
  MSG ("LRAT %" PRId64 " added clauses %.2f%%", added,
       percent (added, total));
  MSG ("LRAT %" PRId64 " deleted clauses %.2f%%", deleted,
       percent (deleted, total));
  MSG ("LRAT %" PRId64 " bytes (%.2f MB)", bytes,
       bytes / (double) (1 << 20));
}

#endif

void LratTracer::close (bool print) {
  CADICAL_assert (!closed ());
  file->close ();
#ifndef CADICAL_QUIET
  if (print) {
    MSG ("LRAT proof file '%s' closed", file->name ());
    print_statistics ();
  }
#else
  (void) print;
#endif
}

void LratTracer::flush (bool print) {
  CADICAL_assert (!closed ());
  file->flush ();
#ifndef CADICAL_QUIET
  if (print) {
    MSG ("LRAT proof file '%s' flushed", file->name ());
    print_statistics ();
  }
#else
  (void) print;
#endif
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
