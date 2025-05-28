//===--- cnf_reader.h -------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "satoko.h"
#include "solver.h"
#include "utils/mem.h"
#include "utils/vec/vec_uint.h"

#include "misc/util/abc_global.h"
ABC_NAMESPACE_IMPL_START

/** Read the file into an internal buffer.
 *
 * This function will receive a file name. The return data is a string ended
 * with '\0'.
 *
 */
static char * file_open(const char *fname)
{
    FILE *file = fopen(fname, "rb");
    char *buffer;
    int sz_file;
    int ret;

    if (file == NULL) {
        printf("Couldn't open file: %s\n", fname);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    sz_file = ftell(file);
    rewind(file);
    buffer = satoko_alloc(char, sz_file + 3);
    ret = fread(buffer, sz_file, 1, file);
    buffer[sz_file + 0] = '\n';
    buffer[sz_file + 1] = '\0';
    return buffer;
}

static void skip_spaces(char **pos)
{
    assert(pos != NULL);
    for (; isspace(**pos); (*pos)++);
}

static void skip_line(char **pos)
{
    assert(pos != NULL);
    for(; **pos != '\n' && **pos != '\r' && **pos != EOF; (*pos)++);
    if (**pos != EOF)
        (*pos)++;
    return;
}

static int read_int(char **token)
{
    int value = 0;
    int neg = 0;

    skip_spaces(token);
    if (**token == '-') {
        neg = 1;
        (*token)++;
    } else if (**token == '+')
        (*token)++;

    if (!isdigit(**token)) {
        printf("Parsing error. Unexpected char: %c.\n", **token);
        exit(EXIT_FAILURE);
    }
    while (isdigit(**token)) {
        value = (value * 10) + (**token - '0');
        (*token)++;
    }
    return neg ? -value : value;
}

static void read_clause(char **token, vec_uint_t *lits)
{
    int var;
    unsigned sign;

    vec_uint_clear(lits);
    while (1) {
        var = read_int(token);
        if (var == 0)
            break;
        sign = (var > 0);
        var = abs(var) - 1;
        vec_uint_push_back(lits, var2lit((unsigned) var, (char)!sign));
    }
}

/** Start the solver and reads the DIMAC file.
 *
 * Returns false upon immediate conflict.
 */
int satoko_parse_dimacs(char *fname, satoko_t **solver)
{
    satoko_t *p = NULL;
    vec_uint_t *lits = NULL;
    int n_var;
    int n_clause;
    char *buffer = file_open(fname);
    char *token;

    if (buffer == NULL)
        return -1;

    token = buffer;
    while (1) {
        skip_spaces(&token);
        if (*token == 0)
            break;
        else if (*token == 'c')
            skip_line(&token);
        else if (*token == 'p') {
            token++;
            skip_spaces(&token);
            for(; !isspace(*token); token++); /* skip 'cnf' */

            n_var = read_int(&token);
            n_clause = read_int(&token);
            skip_line(&token);
            lits = vec_uint_alloc((unsigned) n_var);
            p = satoko_create();
        } else {
            if (lits == NULL) {
                printf("There is no parameter line.\n");
                satoko_free(buffer);
                return -1;
            }
            read_clause(&token, lits);
            if (!satoko_add_clause(p, (int*)vec_uint_data(lits), vec_uint_size(lits))) {
                printf("Unable to add clause: ");
                vec_uint_print(lits);
                return SATOKO_ERR;
            }
            }
    }
    vec_uint_free(lits);
    satoko_free(buffer);
    *solver = p;
    return SATOKO_OK;
}

ABC_NAMESPACE_IMPL_END
