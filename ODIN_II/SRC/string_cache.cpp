/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <cstdio>
#include <cstring>

#include "string_cache.h"

#include "vtr_util.h"
#include "vtr_memory.h"

unsigned long
string_hash(STRING_CACHE* sc,
            const char* string);
void generate_sc_hash(STRING_CACHE* sc);

unsigned long
string_hash(STRING_CACHE* sc,
            const char* string) {
    long a, i, mod, mul;

    a = 0;
    mod = sc->mod;
    mul = sc->mul;
    for (i = 0; string[i]; i++)
        a = (a * mul + (unsigned char)string[i]) % mod;
    return a;
}

void generate_sc_hash(STRING_CACHE* sc) {
    long i;
    long hash;

    if (sc->string_hash != NULL)
        vtr::free(sc->string_hash);
    if (sc->next_string != NULL)
        vtr::free(sc->next_string);
    sc->string_hash_size = sc->size * 2 + 11;
    sc->string_hash = (long*)sc_do_alloc(sc->string_hash_size, sizeof(long));
    sc->next_string = (long*)sc_do_alloc(sc->size, sizeof(long));
    memset(sc->string_hash, 0xff, sc->string_hash_size * sizeof(long));
    memset(sc->next_string, 0xff, sc->size * sizeof(long));
    for (i = 0; i < sc->free; i++) {
        hash = string_hash(sc, sc->string[i]) % sc->string_hash_size;
        sc->next_string[i] = sc->string_hash[hash];
        sc->string_hash[hash] = i;
    }
}

STRING_CACHE*
sc_new_string_cache(void) {
    STRING_CACHE* sc;

    sc = (STRING_CACHE*)sc_do_alloc(1, sizeof(STRING_CACHE));
    sc->size = 100;
    sc->string_hash_size = 0;
    sc->string_hash = NULL;
    sc->next_string = NULL;
    sc->free = 0;
    sc->string = (char**)sc_do_alloc(sc->size, sizeof(char*));
    sc->data = (void**)sc_do_alloc(sc->size, sizeof(void*));
    sc->mod = 834535547;
    sc->mul = 247999;
    generate_sc_hash(sc);
    return sc;
}

long sc_lookup_string(STRING_CACHE* sc,
                      const char* string) {
    long i, hash;

    if (sc == NULL) {
        return -1;
    } else {
        hash = string_hash(sc, string) % sc->string_hash_size;
        i = sc->string_hash[hash];
        while (i >= 0) {
            if (!strcmp(sc->string[i], string))
                return i;
            i = sc->next_string[i];
        }
        return -1;
    }
}

long sc_add_string(STRING_CACHE* sc,
                   const char* string) {
    long i;
    long hash;
    void* a;

    i = sc_lookup_string(sc, string);
    if (i >= 0)
        return i;
    if (sc->free >= sc->size) {
        sc->size = sc->size * 2 + 10;

        a = sc_do_alloc(sc->size, sizeof(char*));
        if (sc->free > 0)
            memcpy(a, sc->string, sc->free * sizeof(char*));
        vtr::free(sc->string);
        sc->string = (char**)a;

        a = sc_do_alloc(sc->size, sizeof(void*));
        if (sc->free > 0)
            memcpy(a, sc->data, sc->free * sizeof(void*));
        vtr::free(sc->data);
        sc->data = (void**)a;

        generate_sc_hash(sc);
    }
    i = sc->free;
    sc->free++;
    sc->string[i] = vtr::strdup(string);
    sc->data[i] = NULL;
    hash = string_hash(sc, string) % sc->string_hash_size;
    sc->next_string[i] = sc->string_hash[hash];
    sc->string_hash[hash] = i;
    return i;
}

void* sc_do_alloc(long a,
                  long b) {
    void* r;

    if (a < 1)
        a = 1;
    if (b < 1)
        b = 1;
    r = vtr::calloc(a, b);
    while (r == NULL) {
        fprintf(stderr,
                "Failed to allocated %ld chunks of %ld bytes (%ld bytes total)\n",
                a, b, a * b);
        r = vtr::calloc(a, b);
    }
    return r;
}

STRING_CACHE* sc_free_string_cache(STRING_CACHE* sc) {
    if (sc != NULL) {
        if (sc->string != NULL) {
            for (long i = 0; i < sc->free; i++) {
                if (sc->string[i] != NULL) {
                    vtr::free(sc->string[i]);
                }
                sc->string[i] = NULL;
            }
            vtr::free(sc->string);
        }
        sc->string = NULL;

        if (sc->data != NULL) {
            vtr::free(sc->data);
        }
        sc->data = NULL;

        if (sc->string_hash != NULL) {
            vtr::free(sc->string_hash);
        }
        sc->string_hash = NULL;

        if (sc->next_string != NULL) {
            vtr::free(sc->next_string);
        }
        sc->next_string = NULL;

        vtr::free(sc);
    }
    sc = NULL;

    return sc;
}

void sc_merge_string_cache(STRING_CACHE** source_ref, STRING_CACHE* destination) {
    STRING_CACHE* source = (*source_ref);
    for (int source_spot = 0; source_spot < source->free; source_spot++) {
        long destination_spot = sc_add_string(destination, source->string[source_spot]);
        destination->data[destination_spot] = source->data[source_spot];

        source->data[source_spot] = NULL;
    }

    /* now cleanup */
    sc_free_string_cache(source);
    (*source_ref) = NULL;
}
