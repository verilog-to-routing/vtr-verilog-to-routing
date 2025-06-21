#ifndef _cadical_kitten_h_INCLUDED
#define _cadical_kitten_h_INCLUDED

#include "global.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

ABC_NAMESPACE_HEADER_START

typedef struct cadical_kitten cadical_kitten;

cadical_kitten *cadical_kitten_init (void);
void cadical_kitten_clear (cadical_kitten *);
void cadical_kitten_release (cadical_kitten *);

#ifdef LOGGING
void cadical_kitten_set_logging (cadical_kitten *cadical_kitten);
#endif

void cadical_kitten_track_antecedents (cadical_kitten *);

void cadical_kitten_shuffle_clauses (cadical_kitten *);
void cadical_kitten_flip_phases (cadical_kitten *);
void cadical_kitten_randomize_phases (cadical_kitten *);

void cadical_kitten_assume (cadical_kitten *, unsigned lit);
void cadical_kitten_assume_signed (cadical_kitten *, int lit);

void cadical_kitten_clause (cadical_kitten *, size_t size, unsigned *);
void citten_clause_with_id (cadical_kitten *, unsigned id, size_t size, int *);
void cadical_kitten_unit (cadical_kitten *, unsigned);
void cadical_kitten_binary (cadical_kitten *, unsigned, unsigned);

void cadical_kitten_clause_with_id_and_exception (cadical_kitten *, unsigned id,
                                          size_t size, const unsigned *,
                                          unsigned except);

void citten_clause_with_id_and_exception (cadical_kitten *, unsigned id,
                                          size_t size, const int *,
                                          unsigned except);
void citten_clause_with_id_and_equivalence (cadical_kitten *, unsigned id,
                                            size_t size, const int *,
                                            unsigned, unsigned);
void cadical_kitten_no_ticks_limit (cadical_kitten *);
void cadical_kitten_set_ticks_limit (cadical_kitten *, uint64_t);
uint64_t cadical_kitten_current_ticks (cadical_kitten *);

void cadical_kitten_no_terminator (cadical_kitten *);
void cadical_kitten_set_terminator (cadical_kitten *, void *, int (*) (void *));

int cadical_kitten_solve (cadical_kitten *);
int cadical_kitten_status (cadical_kitten *);

signed char cadical_kitten_value (cadical_kitten *, unsigned);
signed char cadical_kitten_signed_value (cadical_kitten *, int); // converts second argument
signed char cadical_kitten_fixed (cadical_kitten *, unsigned);
signed char cadical_kitten_fixed_signed (cadical_kitten *, int); // converts
bool cadical_kitten_failed (cadical_kitten *, unsigned);
bool cadical_kitten_flip_literal (cadical_kitten *, unsigned);
bool cadical_kitten_flip_signed_literal (cadical_kitten *, int);

unsigned cadical_kitten_compute_clausal_core (cadical_kitten *, uint64_t *learned);
void cadical_kitten_shrink_to_clausal_core (cadical_kitten *);

void cadical_kitten_traverse_core_ids (cadical_kitten *, void *state,
                               void (*traverse) (void *state, unsigned id));

void cadical_kitten_traverse_core_clauses (cadical_kitten *, void *state,
                                   void (*traverse) (void *state,
                                                     bool learned, size_t,
                                                     const unsigned *));
void cadical_kitten_traverse_core_clauses_with_id (
    cadical_kitten *, void *state,
    void (*traverse) (void *state, unsigned, bool learned, size_t,
                      const unsigned *));
void cadical_kitten_trace_core (cadical_kitten *, void *state,
                        void (*trace) (void *, unsigned, unsigned, bool,
                                       size_t, const unsigned *, size_t,
                                       const unsigned *));

int cadical_kitten_compute_prime_implicant (cadical_kitten *cadical_kitten, void *state,
                                    bool (*ignore) (void *, unsigned));

void cadical_kitten_add_prime_implicant (cadical_kitten *cadical_kitten, void *state, int side,
                                 void (*add_implicant) (void *, int, size_t,
                                                        const unsigned *));

int cadical_kitten_flip_and_implicant_for_signed_literal (cadical_kitten *cadical_kitten, int elit);

ABC_NAMESPACE_HEADER_END

#endif
