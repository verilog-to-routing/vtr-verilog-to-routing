/*
 * Copyright 2022 CAS—Atlantic (University of New Brunswick, CASA)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _STRING_CACHE_H_
#define _STRING_CACHE_H_

struct STRING_CACHE {
    long size;
    long string_hash_size;
    long free;
    long mod;
    long mul;
    char **string;
    void **data;
    long *string_hash;
    long *next_string;
};

/* creates the hash where it is indexed by a string and the void ** holds the data */
STRING_CACHE *sc_new_string_cache(void);
/* returns an index of the spot where string is */
long sc_lookup_string(STRING_CACHE *sc, const char *string);
/* adds an element into the cache and returns and id...check with cache_name->data[i] == NULL to see if already added */
long sc_add_string(STRING_CACHE *sc, const char *string);
void *sc_do_alloc(long, long);

/* free the cache */
STRING_CACHE *sc_free_string_cache(STRING_CACHE *sc);

#endif // _STRING_CACHE_H_
