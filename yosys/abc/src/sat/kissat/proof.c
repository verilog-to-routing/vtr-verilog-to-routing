#include "global.h"

#ifndef KISSAT_NPROOFS

#include "allocate.h"
#include "error.h"
#include "file.h"
#include "inline.h"

#undef KISSAT_NDEBUG

#ifndef KISSAT_NDEBUG
#include <string.h>
#endif

#define size_buffer (1u << 20)

struct write_buffer {
  unsigned char chars[size_buffer];
  size_t pos;
};

typedef struct write_buffer write_buffer;

struct proof {
  write_buffer buffer;
  kissat *solver;
  bool binary;
  file *file;
  ints line;
  uint64_t added;
  uint64_t deleted;
  uint64_t lines;
  uint64_t literals;
#ifndef KISSAT_NDEBUG
  bool empty;
  char *units;
  size_t size_units;
#endif
#if !defined(KISSAT_NDEBUG) || defined(LOGGING)
  unsigneds imported;
#endif
};

#undef LOGPREFIX
#define LOGPREFIX "PROOF"

#define LOGIMPORTED3(...) \
  LOGLITS3 (SIZE_STACK (proof->imported), BEGIN_STACK (proof->imported), \
            __VA_ARGS__)

#define LOGLINE3(...) \
  LOGINTS3 (SIZE_STACK (proof->line), BEGIN_STACK (proof->line), \
            __VA_ARGS__)

void kissat_init_proof (kissat *solver, file *file, bool binary) {
  KISSAT_assert (file);
  KISSAT_assert (!solver->proof);
  proof *proof = kissat_calloc (solver, 1, sizeof (struct proof));
  proof->binary = binary;
  proof->file = file;
  proof->solver = solver;
  solver->proof = proof;
  LOG ("starting to trace %s proof", binary ? "binary" : "non-binary");
}

static void flush_buffer (proof *proof) {
  size_t bytes = proof->buffer.pos;
  if (!bytes)
    return;
  size_t written = kissat_write (proof->file, proof->buffer.chars, bytes);
  if (bytes != written)
    kissat_fatal ("flushing %zu bytes in proof write-buffer failed", bytes);
  proof->buffer.pos = 0;
}

void kissat_release_proof (kissat *solver) {
  proof *proof = solver->proof;
  KISSAT_assert (proof);
  LOG ("stopping to trace proof");
  flush_buffer (proof);
  kissat_flush (proof->file);
  RELEASE_STACK (proof->line);
#ifndef KISSAT_NDEBUG
  kissat_free (solver, proof->units, proof->size_units);
#endif
#if !defined(KISSAT_NDEBUG) || defined(LOGGING)
  RELEASE_STACK (proof->imported);
#endif
  kissat_free (solver, proof, sizeof (struct proof));
  solver->proof = 0;
}

#ifndef KISSAT_QUIET

#include <inttypes.h>

#define PERCENT_LINES(NAME) kissat_percent (proof->NAME, proof->lines)

void kissat_print_proof_statistics (kissat *solver, bool verbose) {
  proof *proof = solver->proof;
  PRINT_STAT ("proof_added", proof->added, PERCENT_LINES (added), "%",
              "per line");
  PRINT_STAT ("proof_bytes", proof->file->bytes,
              proof->file->bytes / (double) (1 << 20), "MB", "");
  PRINT_STAT ("proof_deleted", proof->deleted, PERCENT_LINES (deleted), "%",
              "per line");
  if (verbose)
    PRINT_STAT ("proof_lines", proof->lines, 100, "%", "");
  if (verbose)
    PRINT_STAT ("proof_literals", proof->literals,
                kissat_average (proof->literals, proof->lines), "",
                "per line");
}

#endif

// clang-format off

static inline void write_char (proof *, unsigned char)
  ATTRIBUTE_ALWAYS_INLINE;

static inline void import_external_proof_literal (kissat *, proof *, int)
  ATTRIBUTE_ALWAYS_INLINE;

static inline void
import_internal_proof_literal (kissat *, proof *, unsigned)
  ATTRIBUTE_ALWAYS_INLINE;

// clang-format on

static inline void write_char (proof *proof, unsigned char ch) {
  write_buffer *buffer = &proof->buffer;
  if (buffer->pos == size_buffer)
    flush_buffer (proof);
  buffer->chars[buffer->pos++] = ch;
}

static inline void import_internal_proof_literal (kissat *solver,
                                                  proof *proof,
                                                  unsigned ilit) {
  int elit = kissat_export_literal (solver, ilit);
  KISSAT_assert (elit);
  PUSH_STACK (proof->line, elit);
  proof->literals++;
#if !defined(KISSAT_NDEBUG) || defined(LOGGING)
  PUSH_STACK (proof->imported, ilit);
#endif
}

static inline void import_external_proof_literal (kissat *solver,
                                                  proof *proof, int elit) {
  KISSAT_assert (elit);
  PUSH_STACK (proof->line, elit);
  proof->literals++;
#ifndef KISSAT_NDEBUG
  KISSAT_assert (EMPTY_STACK (proof->imported));
#endif
}

static void import_internal_proof_binary (kissat *solver, proof *proof,
                                          unsigned a, unsigned b) {
  KISSAT_assert (EMPTY_STACK (proof->line));
  import_internal_proof_literal (solver, proof, a);
  import_internal_proof_literal (solver, proof, b);
}

static void import_internal_proof_literals (kissat *solver, proof *proof,
                                            size_t size,
                                            const unsigned *ilits) {
  KISSAT_assert (EMPTY_STACK (proof->line));
  KISSAT_assert (size <= UINT_MAX);
  for (size_t i = 0; i < size; i++)
    import_internal_proof_literal (solver, proof, ilits[i]);
}

static void import_external_proof_literals (kissat *solver, proof *proof,
                                            size_t size, const int *elits) {
  KISSAT_assert (EMPTY_STACK (proof->line));
  KISSAT_assert (size <= UINT_MAX);
  for (size_t i = 0; i < size; i++)
    import_external_proof_literal (solver, proof, elits[i]);
}

static void import_proof_clause (kissat *solver, proof *proof,
                                 const clause *c) {
  import_internal_proof_literals (solver, proof, c->size, c->lits);
}

static void print_binary_proof_line (proof *proof) {
  KISSAT_assert (proof->binary);
  for (all_stack (int, elit, proof->line)) {
    unsigned x = 2u * ABS (elit) + (elit < 0);
    unsigned char ch;
    while (x & ~0x7f) {
      ch = (x & 0x7f) | 0x80;
      write_char (proof, ch);
      x >>= 7;
    }
    write_char (proof, x);
  }
  write_char (proof, 0);
}

static void print_non_binary_proof_line (proof *proof) {
  KISSAT_assert (!proof->binary);
  char buffer[16];
  char *end_of_buffer = buffer + sizeof buffer;
  *--end_of_buffer = 0;
  for (all_stack (int, elit, proof->line)) {
    char *p = end_of_buffer;
    KISSAT_assert (!*p);
    KISSAT_assert (elit);
    KISSAT_assert (elit != INT_MIN);
    unsigned eidx;
    if (elit < 0) {
      write_char (proof, '-');
      eidx = -elit;
    } else
      eidx = elit;
    for (unsigned tmp = eidx; tmp; tmp /= 10)
      *--p = '0' + (tmp % 10);
    while (p != end_of_buffer)
      write_char (proof, *p++);
    write_char (proof, ' ');
  }
  write_char (proof, '0');
  write_char (proof, '\n');
}

static void print_proof_line (proof *proof) {
  proof->lines++;
  if (proof->binary)
    print_binary_proof_line (proof);
  else
    print_non_binary_proof_line (proof);
  CLEAR_STACK (proof->line);
#if !defined(KISSAT_NDEBUG) || defined(LOGGING)
  CLEAR_STACK (proof->imported);
#endif
#ifndef KISSAT_NOPTIONS
  kissat *solver = proof->solver;
#endif
  if (GET_OPTION (flushproof)) {
    flush_buffer (proof);
    kissat_flush (proof->file);
  }
}

#ifndef KISSAT_NDEBUG

static unsigned external_to_proof_literal (int elit) {
  KISSAT_assert (elit);
  KISSAT_assert (elit != INT_MIN);
  return 2u * (abs (elit) - 1) + (elit < 0);
}

static void resize_proof_units (proof *proof, unsigned plit) {
  kissat *solver = proof->solver;
  const size_t old_size = proof->size_units;
  size_t new_size = old_size ? old_size : 2;
  while (new_size <= plit)
    new_size *= 2;
  char *new_units = kissat_calloc (solver, new_size, 1);
  if (old_size)
    memcpy (new_units, proof->units, old_size);
  kissat_dealloc (solver, proof->units, old_size, 1);
  proof->units = new_units;
  proof->size_units = new_size;
}

static void check_repeated_proof_lines (proof *proof) {
  size_t size = SIZE_STACK (proof->line);
  if (!size) {
    KISSAT_assert (!proof->empty);
    proof->empty = true;
  } else if (size == 1) {
    const int eunit = PEEK_STACK (proof->line, 0);
    const unsigned punit = external_to_proof_literal (eunit);
    KISSAT_assert (punit != INVALID_LIT);
    if (!proof->size_units || proof->size_units <= punit)
      resize_proof_units (proof, punit);
    proof->units[punit] = 1;
  }
}

#endif

static void print_added_proof_line (proof *proof) {
  proof->added++;
#ifdef LOGGING
  struct kissat *solver = proof->solver;
  KISSAT_assert (SIZE_STACK (proof->imported) == SIZE_STACK (proof->line));
  LOGIMPORTED3 ("added proof line");
  LOGLINE3 ("added proof line");
#endif
#ifndef KISSAT_NDEBUG
  check_repeated_proof_lines (proof);
#endif
  if (proof->binary)
    write_char (proof, 'a');
  print_proof_line (proof);
}

static void print_delete_proof_line (proof *proof) {
  proof->deleted++;
#ifdef LOGGING
  struct kissat *solver = proof->solver;
  if (SIZE_STACK (proof->imported) == SIZE_STACK (proof->line))
    LOGIMPORTED3 ("deleted internal proof line");
  LOGLINE3 ("deleted external proof line");
#endif
  write_char (proof, 'd');
  if (!proof->binary)
    write_char (proof, ' ');
  print_proof_line (proof);
}

void kissat_add_binary_to_proof (kissat *solver, unsigned a, unsigned b) {
  proof *proof = solver->proof;
  KISSAT_assert (proof);
  import_internal_proof_binary (solver, proof, a, b);
  print_added_proof_line (proof);
}

void kissat_add_clause_to_proof (kissat *solver, const clause *c) {
  proof *proof = solver->proof;
  KISSAT_assert (proof);
  import_proof_clause (solver, proof, c);
  print_added_proof_line (proof);
}

void kissat_add_empty_to_proof (kissat *solver) {
  proof *proof = solver->proof;
  KISSAT_assert (proof);
  KISSAT_assert (EMPTY_STACK (proof->line));
  print_added_proof_line (proof);
}

void kissat_add_lits_to_proof (kissat *solver, size_t size,
                               const unsigned *ilits) {
  proof *proof = solver->proof;
  KISSAT_assert (proof);
  import_internal_proof_literals (solver, proof, size, ilits);
  print_added_proof_line (proof);
}

void kissat_add_unit_to_proof (kissat *solver, unsigned ilit) {
  proof *proof = solver->proof;
  KISSAT_assert (proof);
  KISSAT_assert (EMPTY_STACK (proof->line));
  import_internal_proof_literal (solver, proof, ilit);
  print_added_proof_line (proof);
}

void kissat_shrink_clause_in_proof (kissat *solver, const clause *c,
                                    unsigned remove, unsigned keep) {
  proof *proof = solver->proof;
  const value *const values = solver->values;
  KISSAT_assert (EMPTY_STACK (proof->line));
  const unsigned *ilits = c->lits;
  const unsigned size = c->size;
  for (unsigned i = 0; i != size; i++) {
    const unsigned ilit = ilits[i];
    if (ilit == remove)
      continue;
    if (ilit != keep && values[ilit] < 0 && !LEVEL (ilit))
      continue;
    import_internal_proof_literal (solver, proof, ilit);
  }
  print_added_proof_line (proof);
  import_proof_clause (solver, proof, c);
  print_delete_proof_line (proof);
}

void kissat_delete_binary_from_proof (kissat *solver, unsigned a,
                                      unsigned b) {
  proof *proof = solver->proof;
  KISSAT_assert (proof);
  import_internal_proof_binary (solver, proof, a, b);
  print_delete_proof_line (proof);
}

void kissat_delete_clause_from_proof (kissat *solver, const clause *c) {
  proof *proof = solver->proof;
  KISSAT_assert (proof);
  import_proof_clause (solver, proof, c);
  print_delete_proof_line (proof);
}

void kissat_delete_external_from_proof (kissat *solver, size_t size,
                                        const int *elits) {
  proof *proof = solver->proof;
  KISSAT_assert (proof);
  LOGINTS3 (size, elits, "explicitly deleted");
  import_external_proof_literals (solver, proof, size, elits);
  print_delete_proof_line (proof);
}

void kissat_delete_internal_from_proof (kissat *solver, size_t size,
                                        const unsigned *ilits) {
  proof *proof = solver->proof;
  KISSAT_assert (proof);
  import_internal_proof_literals (solver, proof, size, ilits);
  print_delete_proof_line (proof);
}

#else
int kissat_proof_dummy_to_avoid_warning;
#endif
