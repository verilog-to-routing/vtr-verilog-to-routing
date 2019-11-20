// Not PJs code, but very useful and used everywhere */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "string_cache.h"

unsigned long
string_hash(STRING_CACHE * sc,
	    char *string)
{
    long a, i, mod, mul;

    a = 0;
    mod = sc->mod;
    mul = sc->mul;
    for(i = 0; string[i]; i++)
	a = (a * mul + (unsigned char)string[i]) % mod;
    return a;
}
	
void
generate_sc_hash(STRING_CACHE * sc)
{
    long i;
    long hash;

    if(sc->string_hash != NULL)
		free(sc->string_hash);
    if(sc->next_string != NULL)
		free(sc->next_string);
    sc->string_hash_size = sc->size * 2 + 11;
    sc->string_hash = (long *)sc_do_alloc(sc->string_hash_size, sizeof(long));
    sc->next_string = (long *)sc_do_alloc(sc->size, sizeof(long));
    memset(sc->string_hash, 0xff, sc->string_hash_size * sizeof(long));
    memset(sc->next_string, 0xff, sc->size * sizeof(long));
    for(i = 0; i < sc->free; i++)
	{
	    hash = string_hash(sc, sc->string[i]) % sc->string_hash_size;
	    sc->next_string[i] = sc->string_hash[hash];
	    sc->string_hash[hash] = i;
	}
}


STRING_CACHE *
sc_new_string_cache(void)
{
    STRING_CACHE *sc;

    sc = (STRING_CACHE *)sc_do_alloc(1, sizeof(STRING_CACHE));
    sc->size = 100;
    sc->string_hash_size = 0;
    sc->string_hash = NULL;
    sc->next_string = NULL;
    sc->free = 0;
    sc->string = (char **)sc_do_alloc(sc->size, sizeof(char *));
    sc->data = (void **)sc_do_alloc(sc->size, sizeof(void *));
    sc->mod = 834535547;
    sc->mul = 247999;
    generate_sc_hash(sc);
    return sc;
}

long
sc_lookup_string(STRING_CACHE * sc,
	      char *string)
{
    long i, hash;

    hash = string_hash(sc, string) % sc->string_hash_size;
    i = sc->string_hash[hash];
    while(i >= 0)
	{
	    if(!strcmp(sc->string[i], string))
		return i;
	    i = sc->next_string[i];
	}
    return -1;
}

long
sc_add_string(STRING_CACHE * sc,
	   char *string)
{
    long i;
    long hash;
    void *a;

    i = sc_lookup_string(sc, string);
    if(i >= 0)
		return i;
    if(sc->free >= sc->size)
	{
	    sc->size = sc->size * 2 + 10;

	    a = sc_do_alloc(sc->size, sizeof(char *));
	    if(sc->free > 0)
		memcpy(a, sc->string, sc->free * sizeof(char *));
	    free(sc->string);
	    sc->string = (char **)a;

	    a = sc_do_alloc(sc->size, sizeof(void *));
	    if(sc->free > 0)
		memcpy(a, sc->data, sc->free * sizeof(void *));
	    free(sc->data);
	    sc->data = (void **)a;

	    generate_sc_hash(sc);
	}
    i = sc->free;
    sc->free++;
    sc->string[i] = strdup(string);
    sc->data[i] = NULL;
    hash = string_hash(sc, string) % sc->string_hash_size;
    sc->next_string[i] = sc->string_hash[hash];
    sc->string_hash[hash] = i;
    return i;
}

int
sc_valid_id(STRING_CACHE * sc,
	 long string_id)
{
    if(string_id < 0)
	return 0;
    if(string_id >= sc->free)
	return 0;
    return 1;
}

void *
sc_do_alloc(long a,
	 long b)
{
    void *r;

    if(a < 1)
	a = 1;
    if(b < 1)
	b = 1;
    r = calloc(a, b);
    while(r == NULL)
	{
	    fprintf(stderr,
		    "Failed to allocated %ld chunks of %ld bytes (%ld bytes total)\n",
		    a, b, a * b);
	    sleep(1);
	    r = calloc(a, b);
	}
    return r;
}

void
sc_free_string_cache(STRING_CACHE * sc)
{
    long i;

    if(sc == NULL)
	return;
    for(i = 0; i < sc->free; i++)
	if (sc->string != NULL)
	    free(sc->string[i]);
    free(sc->string);
    sc->string = NULL;
    if(sc->data != NULL)
	{
//	    free(sc->data);
	    sc->data = NULL;
	}
    if(sc->string_hash != NULL)
	{
	    free(sc->string_hash);
	    sc->string_hash = NULL;
	}
    if(sc->next_string != NULL)
	{
	    free(sc->next_string);
	    sc->next_string = NULL;
	}
    free(sc);
}
