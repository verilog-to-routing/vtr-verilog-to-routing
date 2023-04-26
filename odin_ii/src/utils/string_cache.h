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

#ifndef __STRING_CACHE_H__
#define __STRING_CACHE_H__

struct STRING_CACHE {
    long size;
    long string_hash_size;
    long free;
    long mod;
    long mul;
    char** string;
    void** data;
    long* string_hash;
    long* next_string;
};

/* creates the hash where it is indexed by a string and the void ** holds the data */
STRING_CACHE* sc_new_string_cache(void);
/* returns an index of the spot where string is */
long sc_lookup_string(STRING_CACHE* sc, const char* string);
/* adds an element into the cache and returns and id...check with cache_name->data[i] == NULL to see if already added */
long sc_add_string(STRING_CACHE* sc, const char* string);
void* sc_do_alloc(long, long);

/* free the cache */
STRING_CACHE* sc_free_string_cache(STRING_CACHE* sc);

void sc_merge_string_cache(STRING_CACHE** source_ref, STRING_CACHE* destination);

#endif
