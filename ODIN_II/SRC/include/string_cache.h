#ifndef __STRING_CACHE_H__
#define __STRING_CACHE_H__

/*
 * Copyright (c) 2001 Vladimir Dergachev (volodya@users.sourceforge.net)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */


typedef struct {
	long size;
	long string_hash_size;
	long free;
	long mod;
	long mul;
	char **string;
	void **data;
	long *string_hash;
	long *next_string;
} STRING_CACHE;

/* creates the hash where it is indexed by a string and the void ** holds the data */
STRING_CACHE *sc_new_string_cache(void);
/* returns an index of the spot where string is */
long sc_lookup_string(STRING_CACHE *sc, const char * string);
/* adds an element into the cache and returns and id...check with cache_name->data[i] == NULL to see if already added */
long sc_add_string(STRING_CACHE *sc, const char *string);
int sc_valid_id(STRING_CACHE *sc, long string_id);
void * sc_do_alloc(long, long);
bool sc_remove_string(STRING_CACHE * sc, const char *string);

/* free the cache */
STRING_CACHE * sc_free_string_cache(STRING_CACHE *sc);

#endif
