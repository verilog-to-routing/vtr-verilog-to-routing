#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

FratTracer::FratTracer (Internal *i, File *f, bool b, bool a)
    : internal (i), file (f), binary (b), with_antecedents (a)
#ifndef CADICAL_QUIET
      ,
      added (0), deleted (0), finalized (0), original (0)
#endif
{
  (void) internal;
}

void FratTracer::connect_internal (Internal *i) {
  internal = i;
  file->connect_internal (internal);
  LOG ("FRAT TRACER connected to internal");
}

FratTracer::~FratTracer () {
  LOG ("FRAT TRACER delete");
  delete file;
}

/*------------------------------------------------------------------------*/

inline void FratTracer::put_binary_zero () {
  CADICAL_assert (binary);
  CADICAL_assert (file);
  file->put ((unsigned char) 0);
}

inline void FratTracer::put_binary_lit (int lit) {
  CADICAL_assert (binary);
  CADICAL_assert (file);
  CADICAL_assert (lit != INT_MIN);
  unsigned x = 2 * abs (lit) + (lit < 0);
  unsigned char ch;
  while (x & ~0x7f) {
    ch = (x & 0x7f) | 0x80;
    file->put (ch);
    x >>= 7;
  }
  ch = x;
  file->put (ch);
}

inline void FratTracer::put_binary_id (int64_t id, bool can_be_negative) {
  CADICAL_assert (binary);
  CADICAL_assert (file);
  uint64_t x = abs (id);
  if (can_be_negative) {
    x = 2 * x + (id < 0);
  }
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

void FratTracer::frat_add_original_clause (int64_t id,
                                           const vector<int> &clause) {
  if (binary)
    file->put ('o');
  else
    file->put ("o ");
  if (binary)
    put_binary_id (id);
  else
    file->put (id), file->put ("  ");
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

void FratTracer::frat_add_derived_clause (int64_t id,
                                          const vector<int> &clause) {
  if (binary)
    file->put ('a');
  else
    file->put ("a ");
  if (binary)
    put_binary_id (id);
  else
    file->put (id), file->put ("  ");
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

void FratTracer::frat_add_derived_clause (int64_t id,
                                          const vector<int> &clause,
                                          const vector<int64_t> &chain) {
  if (binary)
    file->put ('a');
  else
    file->put ("a ");
  if (binary)
    put_binary_id (id);
  else
    file->put (id), file->put ("  ");
  for (const auto &external_lit : clause)
    if (binary)
      put_binary_lit (external_lit);
    else
      file->put (external_lit), file->put (' ');
  if (binary)
    put_binary_zero (), file->put ('l');
  else
    file->put ("0  l ");
  for (const auto &c : chain)
    if (binary)
      put_binary_id (c, true); // LRAT can have negative ids
    else
      file->put (c), file->put (' '); // in proof chain, so they get
  if (binary)
    put_binary_zero (); // since cadical has no rat-steps
  else
    file->put ("0\n"); // this is just 2c here
}

void FratTracer::frat_delete_clause (int64_t id,
                                     const vector<int> &clause) {
  if (binary)
    file->put ('d');
  else
    file->put ("d ");
  if (binary)
    put_binary_id (id);
  else
    file->put (id), file->put ("  ");
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

void FratTracer::frat_finalize_clause (int64_t id,
                                       const vector<int> &clause) {
  if (binary)
    file->put ('f');
  else
    file->put ("f ");
  if (binary)
    put_binary_id (id);
  else
    file->put (id), file->put ("  ");
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

void FratTracer::add_original_clause (int64_t id, bool,
                                      const vector<int> &clause, bool) {
  if (file->closed ())
    return;
  LOG ("FRAT TRACER tracing addition of original clause");
  frat_add_original_clause (id, clause);
}

void FratTracer::add_derived_clause (int64_t id, bool,
                                     const vector<int> &clause,
                                     const vector<int64_t> &chain) {
  if (file->closed ())
    return;
  LOG ("FRAT TRACER tracing addition of derived clause");
  if (with_antecedents)
    frat_add_derived_clause (id, clause, chain);
  else
    frat_add_derived_clause (id, clause);
#ifndef CADICAL_QUIET
  added++;
#endif
}

void FratTracer::delete_clause (int64_t id, bool,
                                const vector<int> &clause) {
  if (file->closed ())
    return;
  LOG ("FRAT TRACER tracing deletion of clause");
  frat_delete_clause (id, clause);
#ifndef CADICAL_QUIET
  deleted++;
#endif
}

void FratTracer::finalize_clause (int64_t id, const vector<int> &clause) {
  if (file->closed ())
    return;
  LOG ("FRAT TRACER tracing finalization of clause");
  frat_finalize_clause (id, clause);
}

/*------------------------------------------------------------------------*/

bool FratTracer::closed () { return file->closed (); }

#ifndef CADICAL_QUIET

void FratTracer::print_statistics () {
  uint64_t bytes = file->bytes ();
  uint64_t total = original + added + deleted + finalized;
  MSG ("FRAT %" PRId64 " original clauses %.2f%%", original,
       percent (original, total));
  MSG ("FRAT %" PRId64 " added clauses %.2f%%", added,
       percent (added, total));
  MSG ("FRAT %" PRId64 " deleted clauses %.2f%%", deleted,
       percent (deleted, total));
  MSG ("FRAT %" PRId64 " finalized clauses %.2f%%", finalized,
       percent (finalized, total));
  MSG ("FRAT %" PRId64 " bytes (%.2f MB)", bytes,
       bytes / (double) (1 << 20));
}

#endif

void FratTracer::close (bool print) {
  CADICAL_assert (!closed ());
  file->close ();
#ifndef CADICAL_QUIET
  if (print) {
    MSG ("FRAT proof file '%s' closed", file->name ());
    print_statistics ();
  }
#else
  (void) print;
#endif
}

void FratTracer::flush (bool print) {
  CADICAL_assert (!closed ());
  file->flush ();
#ifndef CADICAL_QUIET
  if (print) {
    MSG ("FRAT proof file '%s' flushed", file->name ());
    print_statistics ();
  }
#else
  (void) print;
#endif
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
