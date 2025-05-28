#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

VeripbTracer::VeripbTracer (Internal *i, File *f, bool b, bool a, bool c)
    : internal (i), file (f), with_antecedents (a), checked_deletions (c),
      num_clauses (0), size_clauses (0), clauses (0), last_hash (0),
      last_id (0), last_clause (0)
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
}

void VeripbTracer::connect_internal (Internal *i) {
  internal = i;
  file->connect_internal (internal);
  LOG ("VERIPB TRACER connected to internal");
}

VeripbTracer::~VeripbTracer () {
  LOG ("VERIPB TRACER delete");
  delete file;
  for (size_t i = 0; i < size_clauses; i++)
    for (HashId *c = clauses[i], *next; c; c = next)
      next = c->next, delete_clause (c);
  delete[] clauses;
}

/*------------------------------------------------------------------------*/

void VeripbTracer::enlarge_clauses () {
  CADICAL_assert (num_clauses == size_clauses);
  const uint64_t new_size_clauses = size_clauses ? 2 * size_clauses : 1;
  LOG ("VeriPB Tracer enlarging clauses from %" PRIu64 " to %" PRIu64,
       (uint64_t) size_clauses, (uint64_t) new_size_clauses);
  HashId **new_clauses;
  new_clauses = new HashId *[new_size_clauses];
  clear_n (new_clauses, new_size_clauses);
  for (uint64_t i = 0; i < size_clauses; i++) {
    for (HashId *c = clauses[i], *next; c; c = next) {
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

HashId *VeripbTracer::new_clause () {
  HashId *res = new HashId ();
  res->next = 0;
  res->hash = last_hash;
  res->id = last_id;
  last_clause = res;
  num_clauses++;
  return res;
}

void VeripbTracer::delete_clause (HashId *c) {
  CADICAL_assert (c);
  num_clauses--;
  delete c;
}

uint64_t VeripbTracer::reduce_hash (uint64_t hash, uint64_t size) {
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

uint64_t VeripbTracer::compute_hash (const int64_t id) {
  CADICAL_assert (id > 0);
  unsigned j = id % num_nonces;             // Dont know if this is a good
  uint64_t tmp = nonces[j] * (uint64_t) id; // hash funktion or even better
  return last_hash = tmp;                   // than just using id.
}

bool VeripbTracer::find_and_delete (const int64_t id) {
  if (!num_clauses)
    return false;
  /*
  if (last_clause && last_clause->id == id) {
    const uint64_t h = reduce_hash (last_clause->hash, size_clauses);
    clauses[h] = last_clause->next;
    delete last_clause;
    return true;
  }
  */
  HashId **res = 0, *c;
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
  delete_clause (c);
  return true;
}

void VeripbTracer::insert () {
  if (num_clauses == size_clauses)
    enlarge_clauses ();
  const uint64_t h = reduce_hash (compute_hash (last_id), size_clauses);
  HashId *c = new_clause ();
  c->next = clauses[h];
  clauses[h] = c;
}

/*------------------------------------------------------------------------*/

inline void VeripbTracer::put_binary_zero () {
  CADICAL_assert (binary);
  CADICAL_assert (file);
  file->put ((unsigned char) 0);
}

inline void VeripbTracer::put_binary_lit (int lit) {
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

inline void VeripbTracer::put_binary_id (int64_t id, bool can_be_negative) {
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

void VeripbTracer::veripb_add_derived_clause (
    int64_t id, bool redundant, const vector<int> &clause,
    const vector<int64_t> &chain) {
  file->put ("pol ");
  bool first = true;
  for (auto p = chain.rbegin (); p != chain.rend (); p++) {
    auto cid = *p;
    if (first) {
      first = false;
      file->put (cid);
    } else {
      file->put (' ');
      file->put (cid);
      file->put (" + s");
    }
  }
  file->put ("\n");
  file->put ("e ");
  for (const auto &external_lit : clause) {
    file->put ("1 ");
    if (external_lit < 0)
      file->put ('~');
    file->put ('x');
    file->put (abs (external_lit));
    file->put (' ');
  }
  file->put (">= 1 ; ");
  file->put (id);
  file->put (" ;\n");
  if (!redundant && checked_deletions) {
    file->put ("core id ");
    file->put (id);
    file->put ("\n");
  }
}

void VeripbTracer::veripb_add_derived_clause (int64_t id, bool redundant,
                                              const vector<int> &clause) {
  file->put ("rup ");
  for (const auto &external_lit : clause) {
    file->put ("1 ");
    if (external_lit < 0)
      file->put ('~');
    file->put ('x');
    file->put (abs (external_lit));
    file->put (' ');
  }
  file->put (">= 1 ;\n");
  if (!redundant && checked_deletions) {
    file->put ("core id ");
    file->put (id);
    file->put ("\n");
  }
}

void VeripbTracer::veripb_begin_proof (int64_t reserved_ids) {
  file->put ("pseudo-Boolean proof version 2.0\n");
  file->put ("f ");
  file->put (reserved_ids);
  file->put ("\n");
}

void VeripbTracer::veripb_delete_clause (int64_t id, bool redundant) {
  if (!redundant && checked_deletions && find_and_delete (id))
    return;
  if (redundant || !checked_deletions)
    file->put ("del id ");
  else {
    file->put ("delc ");
  }
  file->put (id);
  file->put ("\n");
}

void VeripbTracer::veripb_report_status (bool unsat, int64_t conflict_id) {
  file->put ("output NONE\n");
  if (unsat) {
    file->put ("conclusion UNSAT : ");
    file->put (conflict_id);
    file->put (" \n");
  } else
    file->put ("conclusion NONE\n");
  file->put ("end pseudo-Boolean proof\n");
}

void VeripbTracer::veripb_strengthen (int64_t id) {
  if (!checked_deletions)
    return;
  file->put ("core id ");
  file->put (id);
  file->put ("\n");
}

/*------------------------------------------------------------------------*/

void VeripbTracer::begin_proof (int64_t id) {
  if (file->closed ())
    return;
  LOG ("VERIPB TRACER tracing start of proof with %" PRId64
       "original clauses",
       id);
  veripb_begin_proof (id);
}

void VeripbTracer::add_derived_clause (int64_t id, bool redundant,
                                       const vector<int> &clause,
                                       const vector<int64_t> &chain) {
  if (file->closed ())
    return;
  LOG ("VERIPB TRACER tracing addition of derived clause[%" PRId64 "]", id);
  if (with_antecedents)
    veripb_add_derived_clause (id, redundant, clause, chain);
  else
    veripb_add_derived_clause (id, redundant, clause);
#ifndef CADICAL_QUIET
  added++;
#endif
}

void VeripbTracer::delete_clause (int64_t id, bool redundant,
                                  const vector<int> &) {
  if (file->closed ())
    return;
  LOG ("VERIPB TRACER tracing deletion of clause[%" PRId64 "]", id);
  veripb_delete_clause (id, redundant);
#ifndef CADICAL_QUIET
  deleted++;
#endif
}

void VeripbTracer::report_status (int status, int64_t conflict_id) {
  if (file->closed ())
    return;
#ifdef LOGGING
  if (conflict_id)
    LOG ("VERIPB TRACER tracing finalization of proof with empty "
         "clause[%" PRId64 "]",
         conflict_id);
#endif
  veripb_report_status (status == UNSATISFIABLE, conflict_id);
}

void VeripbTracer::weaken_minus (int64_t id, const vector<int> &) {
  if (!checked_deletions)
    return;
  if (file->closed ())
    return;
  LOG ("VERIPB TRACER tracing weaken minus of clause[%" PRId64 "]", id);
  last_id = id;
  insert ();
}

void VeripbTracer::strengthen (int64_t id) {
  if (file->closed ())
    return;
  LOG ("VERIPB TRACER tracing strengthen of clause[%" PRId64 "]", id);
  veripb_strengthen (id);
}

/*------------------------------------------------------------------------*/

bool VeripbTracer::closed () { return file->closed (); }

#ifndef CADICAL_QUIET

void VeripbTracer::print_statistics () {
  // TODO complete
  uint64_t bytes = file->bytes ();
  uint64_t total = added + deleted;
  MSG ("VeriPB %" PRId64 " added clauses %.2f%%", added,
       percent (added, total));
  MSG ("VeriPB %" PRId64 " deleted clauses %.2f%%", deleted,
       percent (deleted, total));
  MSG ("VeriPB %" PRId64 " bytes (%.2f MB)", bytes,
       bytes / (double) (1 << 20));
}

#endif

void VeripbTracer::close (bool print) {
  CADICAL_assert (!closed ());
  file->close ();
#ifndef CADICAL_QUIET
  if (print) {
    MSG ("VeriPB proof file '%s' closed", file->name ());
    print_statistics ();
  }
#else
  (void) print;
#endif
}

void VeripbTracer::flush (bool print) {
  CADICAL_assert (!closed ());
  file->flush ();
#ifndef CADICAL_QUIET
  if (print) {
    MSG ("VeriPB proof file '%s' flushed", file->name ());
    print_statistics ();
  }
#else
  (void) print;
#endif
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
