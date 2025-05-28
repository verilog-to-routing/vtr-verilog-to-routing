#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

IdrupTracer::IdrupTracer (Internal *i, File *f, bool b)
    : internal (i), file (f), binary (b), num_clauses (0), size_clauses (0),
      clauses (0), last_hash (0), last_id (0), last_clause (0)
#ifndef CADICAL_QUIET
      ,
      added (0), deleted (0)
#endif
{
  (void) internal;

  // Initialize random number table for hash function.
  //
  Random random (42);
  for (unsigned n = 0; n < num_nonces; n++) {
    uint64_t nonce = random.next ();
    if (!(nonce & 1))
      nonce++;
    CADICAL_assert (nonce), CADICAL_assert (nonce & 1);
    nonces[n] = nonce;
  }
#ifndef CADICAL_NDEBUG
  binary = b;
#else
  (void) b;
#endif
  piping = file->piping ();
}

void IdrupTracer::connect_internal (Internal *i) {
  internal = i;
  file->connect_internal (internal);
  LOG ("IDRUP TRACER connected to internal");
}

IdrupTracer::~IdrupTracer () {
  LOG ("IDRUP TRACER delete");
  delete file;
  for (size_t i = 0; i < size_clauses; i++)
    for (IdrupClause *c = clauses[i], *next; c; c = next)
      next = c->next, delete_clause (c);
  delete[] clauses;
}

/*------------------------------------------------------------------------*/

void IdrupTracer::enlarge_clauses () {
  CADICAL_assert (num_clauses == size_clauses);
  const uint64_t new_size_clauses = size_clauses ? 2 * size_clauses : 1;
  LOG ("IDRUP Tracer enlarging clauses of tracer from %" PRIu64
       " to %" PRIu64,
       (uint64_t) size_clauses, (uint64_t) new_size_clauses);
  IdrupClause **new_clauses;
  new_clauses = new IdrupClause *[new_size_clauses];
  clear_n (new_clauses, new_size_clauses);
  for (uint64_t i = 0; i < size_clauses; i++) {
    for (IdrupClause *c = clauses[i], *next; c; c = next) {
      next = c->next;
      const uint64_t h = reduce_hash (c->hash, new_size_clauses);
      c->next = new_clauses[h];
      new_clauses[h] = c;
    }
  }
  delete[] clauses;
  clauses = new_clauses;
  size_clauses = new_size_clauses;
}

IdrupClause *IdrupTracer::new_clause () {
  const size_t size = imported_clause.size ();
  CADICAL_assert (size <= UINT_MAX);
  const int off = size ? -1 : 0;
  const size_t bytes = sizeof (IdrupClause) + (size - off) * sizeof (int);
  IdrupClause *res = (IdrupClause *) new char[bytes];
  res->next = 0;
  res->hash = last_hash;
  res->id = last_id;
  res->size = size;
  int *literals = res->literals, *p = literals;
  for (const auto &lit : imported_clause) {
    *p++ = lit;
  }
  last_clause = res;
  num_clauses++;
  return res;
}

void IdrupTracer::delete_clause (IdrupClause *c) {
  CADICAL_assert (c);
  num_clauses--;
  delete[] (char *) c;
}

uint64_t IdrupTracer::reduce_hash (uint64_t hash, uint64_t size) {
  CADICAL_assert (size > 0);
  unsigned shift = 32;
  uint64_t res = hash;
  while ((((uint64_t) 1) << shift) > size) {
    res ^= res >> shift;
    shift >>= 1;
  }
  res &= size - 1;
  CADICAL_assert (res < size);
  return res;
}

uint64_t IdrupTracer::compute_hash (const int64_t id) {
  CADICAL_assert (id > 0);
  unsigned j = id % num_nonces;
  uint64_t tmp = nonces[j] * (uint64_t) id;
  return last_hash = tmp;
}

bool IdrupTracer::find_and_delete (const int64_t id) {
  if (!num_clauses)
    return false;
  IdrupClause **res = 0, *c;
  const uint64_t hash = compute_hash (id);
  const uint64_t h = reduce_hash (hash, size_clauses);
  for (res = clauses + h; (c = *res); res = &c->next) {
    if (c->hash == hash && c->id == id) {
      break;
    }
    if (!c->next)
      return false;
  }
  if (!c)
    return false;
  CADICAL_assert (c && res);
  *res = c->next;
  int *begin = c->literals;
  for (size_t i = 0; i < c->size; i++) {
    imported_clause.push_back (begin[i]);
  }
  delete_clause (c);
  return true;
}

void IdrupTracer::insert () {
  if (num_clauses == size_clauses)
    enlarge_clauses ();
  const uint64_t h = reduce_hash (compute_hash (last_id), size_clauses);
  IdrupClause *c = new_clause ();
  c->next = clauses[h];
  clauses[h] = c;
}

/*------------------------------------------------------------------------*/

inline void IdrupTracer::flush_if_piping () {
  if (piping)
    file->flush ();
}

inline void IdrupTracer::put_binary_zero () {
  CADICAL_assert (binary);
  CADICAL_assert (file);
  file->put ((unsigned char) 0);
}

inline void IdrupTracer::put_binary_lit (int lit) {
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

inline void IdrupTracer::put_binary_id (int64_t id, bool can_be_negative) {
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

void IdrupTracer::idrup_add_restored_clause (const vector<int> &clause) {
  if (binary)
    file->put ('r');
  else
    file->put ("r ");
  for (const auto &external_lit : clause)
    if (binary)
      put_binary_lit (external_lit);
    else
      file->put (external_lit), file->put (' ');
  if (binary)
    put_binary_zero ();
  else
    file->put ("0\n");
  // flush_if_piping ();
}

void IdrupTracer::idrup_add_derived_clause (const vector<int> &clause) {
  if (binary)
    file->put ('l');
  else
    file->put ("l ");
  for (const auto &external_lit : clause)
    if (binary)
      put_binary_lit (external_lit);
    else
      file->put (external_lit), file->put (' ');
  if (binary)
    put_binary_zero ();
  else
    file->put ("0\n");
  // flush_if_piping ();
}

void IdrupTracer::idrup_add_original_clause (const vector<int> &clause) {
  if (binary)
    file->put ('i');
  else
    file->put ("i ");
  for (const auto &external_lit : clause)
    if (binary)
      put_binary_lit (external_lit);
    else
      file->put (external_lit), file->put (' ');
  if (binary)
    put_binary_zero ();
  else
    file->put ("0\n");
  // flush_if_piping ();
}

void IdrupTracer::idrup_delete_clause (int64_t id,
                                       const vector<int> &clause) {
  if (find_and_delete (id)) {
    CADICAL_assert (imported_clause.empty ());
    if (binary)
      file->put ('w');
    else
      file->put ("w ");
#ifndef CADICAL_QUIET
    weakened++;
#endif
  } else {
    if (binary)
      file->put ('d');
    else
      file->put ("d ");
#ifndef CADICAL_QUIET
    deleted++;
#endif
  }
  for (const auto &external_lit : clause)
    if (binary)
      put_binary_lit (external_lit);
    else
      file->put (external_lit), file->put (' ');
  if (binary)
    put_binary_zero ();
  else
    file->put ("0\n");
  // flush_if_piping ();
}

void IdrupTracer::idrup_conclude_and_delete (
    const vector<int64_t> &conclusion) {
  uint64_t size = conclusion.size ();
  if (size > 1) {
    if (binary) {
      file->put ('U');
      put_binary_id (size);
    } else {
      file->put ("U ");
      file->put (size), file->put ("\n");
    }
  }
  for (auto &id : conclusion) {
    if (binary)
      file->put ('u');
    else
      file->put ("u ");
    (void) find_and_delete (id);
    for (const auto &external_lit : imported_clause) {
      // flip sign...
      const auto not_elit = -external_lit;
      if (binary)
        put_binary_lit (not_elit);
      else
        file->put (not_elit), file->put (' ');
    }
    if (binary)
      put_binary_zero ();
    else
      file->put ("0\n");
    imported_clause.clear ();
  }
  flush_if_piping ();
}

void IdrupTracer::idrup_report_status (int status) {
  if (binary)
    file->put ('s');
  else
    file->put ("s ");
  if (status == SATISFIABLE)
    file->put ("SATISFIABLE");
  else if (status == UNSATISFIABLE)
    file->put ("UNSATISFIABLE");
  else
    file->put ("UNKNOWN");
  if (!binary)
    file->put ("\n");
  flush_if_piping ();
}

void IdrupTracer::idrup_conclude_sat (const vector<int> &model) {
  if (binary)
    file->put ('m');
  else
    file->put ("m ");
  for (auto &lit : model) {
    if (binary)
      put_binary_lit (lit);
    else
      file->put (lit), file->put (' ');
  }
  if (binary)
    put_binary_zero ();
  else
    file->put ("0\n");
  flush_if_piping ();
}

void IdrupTracer::idrup_conclude_unknown (const vector<int> &trail) {
  if (binary)
    file->put ('e');
  else
    file->put ("e ");
  for (auto &lit : trail) {
    if (binary)
      put_binary_lit (lit);
    else
      file->put (lit), file->put (' ');
  }
  if (binary)
    put_binary_zero ();
  else
    file->put ("0\n");
  flush_if_piping ();
}

void IdrupTracer::idrup_solve_query () {
  if (binary)
    file->put ('q');
  else
    file->put ("q ");
  for (auto &lit : assumptions) {
    if (binary)
      put_binary_lit (lit);
    else
      file->put (lit), file->put (' ');
  }
  if (binary)
    put_binary_zero ();
  else
    file->put ("0\n");
  flush_if_piping ();
}

/*------------------------------------------------------------------------*/

void IdrupTracer::add_derived_clause (int64_t, bool,
                                      const vector<int> &clause,
                                      const vector<int64_t> &) {
  if (file->closed ())
    return;
  CADICAL_assert (imported_clause.empty ());
  LOG (clause, "IDRUP TRACER tracing addition of derived clause");
  idrup_add_derived_clause (clause);
#ifndef CADICAL_QUIET
  added++;
#endif
}

void IdrupTracer::add_assumption_clause (int64_t id,
                                         const vector<int> &clause,
                                         const vector<int64_t> &) {
  if (file->closed ())
    return;
  CADICAL_assert (imported_clause.empty ());
  LOG (clause, "IDRUP TRACER tracing addition of assumption clause");
  for (auto &lit : clause)
    imported_clause.push_back (lit);
  last_id = id;
  insert ();
  imported_clause.clear ();
}

void IdrupTracer::delete_clause (int64_t id, bool,
                                 const vector<int> &clause) {
  if (file->closed ())
    return;
  CADICAL_assert (imported_clause.empty ());
  LOG ("IDRUP TRACER tracing deletion of clause[%" PRId64 "]", id);
  idrup_delete_clause (id, clause);
}

void IdrupTracer::weaken_minus (int64_t id, const vector<int> &) {
  if (file->closed ())
    return;
  CADICAL_assert (imported_clause.empty ());
  LOG ("IDRUP TRACER tracing weaken minus of clause[%" PRId64 "]", id);
  last_id = id;
  insert ();
#ifndef CADICAL_QUIET
  weakened++;
#endif
}

void IdrupTracer::conclude_unsat (ConclusionType,
                                  const vector<int64_t> &conclusion) {
  if (file->closed ())
    return;
  CADICAL_assert (imported_clause.empty ());
  LOG (conclusion, "IDRUP TRACER tracing conclusion of clause(s)");
  idrup_conclude_and_delete (conclusion);
}

void IdrupTracer::add_original_clause (int64_t id, bool,
                                       const vector<int> &clause,
                                       bool restored) {
  if (file->closed ())
    return;
  if (!restored) {
    LOG (clause, "IDRUP TRACER tracing addition of original clause");
#ifndef CADICAL_QUIET
    original++;
#endif
    return idrup_add_original_clause (clause);
  }
  CADICAL_assert (restored);
  if (find_and_delete (id)) {
    LOG (clause,
         "IDRUP TRACER the clause was not yet weakened, so no restore");
    return;
  }
  LOG (clause, "IDRUP TRACER tracing addition of restored clause");
  idrup_add_restored_clause (clause);
#ifndef CADICAL_QUIET
  restore++;
#endif
}

void IdrupTracer::report_status (int status, int64_t) {
  if (file->closed ())
    return;
  LOG ("IDRUP TRACER tracing report of status %d", status);
  idrup_report_status (status);
}

void IdrupTracer::conclude_sat (const vector<int> &model) {
  if (file->closed ())
    return;
  LOG (model, "IDRUP TRACER tracing conclusion of model");
  idrup_conclude_sat (model);
}

void IdrupTracer::conclude_unknown (const vector<int> &trail) {
  if (file->closed ())
    return;
  LOG (trail, "IDRUP TRACER tracing conclusion of unknown state");
  idrup_conclude_unknown (trail);
}

void IdrupTracer::solve_query () {
  if (file->closed ())
    return;
  LOG (assumptions, "IDRUP TRACER tracing solve query with assumptions");
  idrup_solve_query ();
#ifndef CADICAL_QUIET
  solved++;
#endif
}

void IdrupTracer::add_assumption (int lit) {
  LOG ("IDRUP TRACER tracing addition of assumption %d", lit);
  assumptions.push_back (lit);
}

void IdrupTracer::reset_assumptions () {
  LOG (assumptions, "IDRUP TRACER tracing reset of assumptions");
  assumptions.clear ();
}

/*------------------------------------------------------------------------*/

bool IdrupTracer::closed () { return file->closed (); }

#ifndef CADICAL_QUIET

void IdrupTracer::print_statistics () {
  // TODO complete this.
  uint64_t bytes = file->bytes ();
  uint64_t total = added + deleted + weakened + restore + original;
  MSG ("LIDRUP %" PRId64 " original clauses %.2f%%", original,
       percent (original, total));
  MSG ("LIDRUP %" PRId64 " learned clauses %.2f%%", added,
       percent (added, total));
  MSG ("LIDRUP %" PRId64 " deleted clauses %.2f%%", deleted,
       percent (deleted, total));
  MSG ("LIDRUP %" PRId64 " weakened clauses %.2f%%", weakened,
       percent (weakened, total));
  MSG ("LIDRUP %" PRId64 " restored clauses %.2f%%", restore,
       percent (restore, total));
  MSG ("LIDRUP %" PRId64 " queries %.2f", solved, relative (solved, total));
  MSG ("IDRUP %" PRId64 " bytes (%.2f MB)", bytes,
       bytes / (double) (1 << 20));
}

#endif

void IdrupTracer::close (bool print) {
  CADICAL_assert (!closed ());
  file->close ();
#ifndef CADICAL_QUIET
  if (print) {
    MSG ("IDRUP proof file '%s' closed", file->name ());
    print_statistics ();
  }
#else
  (void) print;
#endif
}

void IdrupTracer::flush (bool print) {
  CADICAL_assert (!closed ());
  file->flush ();
#ifndef CADICAL_QUIET
  if (print) {
    MSG ("IDRUP proof file '%s' flushed", file->name ());
    print_statistics ();
  }
#else
  (void) print;
#endif
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
