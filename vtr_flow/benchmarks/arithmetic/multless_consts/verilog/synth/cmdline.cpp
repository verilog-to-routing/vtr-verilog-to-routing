#ifndef CMDLINE_ERROR
#define CMDLINE_ERROR(st,arg) { fprintf(stderr, st, arg); fprintf(stderr, "\n"); exit(1); }
#endif
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cassert>
#include "cmdline.h"

#define xmalloc malloc
#define xfree  free
#define xstrdup strdup

char * flatten_string_array(char** strings, int count, const char* delimitor) {
	int len = 0, i, ofs;
	char * result;
	for(i = 0; i < count; i++)
	    len += strlen(strings[i])+1;

	result = (char*) xmalloc( sizeof(char) * len + 1 );
    result[0] = '\0';
	ofs = 0;
	for(i = 0; i < count; i++) {
	    if(i!=0) ofs += strlen(strings[i-1]) + 1;
	    sprintf(result + ofs, "%s%s", strings[i], delimitor);
	}
	return result;
}

cmdline_t * cmdline_new(int argc, char **argv) {
    cmdline_t * result = (cmdline_t*) xmalloc(sizeof(cmdline_t));
    int i;
    result->argv = argv;
    result->argc = argc;
    if(argc!=0)
	result->marks = (cmdline_mark_t*) xmalloc(sizeof(cmdline_mark_t) * argc);
    for(i=0; i<argc; ++i)
	result->marks[i] = CMD_NONE;
    return result;
}

void cmdline_free(cmdline_t *c) {
    xfree(c->marks);
    xfree(c);
}

/* returns argc if the end is reached */
static int _cmdline_next_tagged(cmdline_t *c, int current, cmdline_mark_t mark) {
    int i = current+1;
    assert(c!=NULL);
    while(i < c->argc && c->marks[i] != mark)
	++i;
    return i;
}

/* Return the position of next unparsed argument after argv[current] */
int cmdline_next_unmarked(cmdline_t *c, int current) {
    return _cmdline_next_tagged(c,current,CMD_NONE);
}

/* Return the position of next unparsed argument after argv[current] */
int cmdline_next_marked(cmdline_t *c, int current) {
    return _cmdline_next_tagged(c,current,CMD_PARSED);
}


/* Mark argument at 'pos' as parsed and return the next unparsed position */
int cmdline_mark(cmdline_t *c, int pos) {
    assert(c!=NULL && pos < c->argc);
    c->marks[pos] = CMD_PARSED;
    return cmdline_next_unmarked(c, pos);
}

/* Unmark argument at 'pos' */
void cmdline_unmark(cmdline_t *c, int pos) {
    assert(c!=NULL && pos < c->argc);
    c->marks[pos] = CMD_NONE;
}

int cmdline_ismarked(cmdline_t *c, int pos) {
    return c->marks[pos] != CMD_NONE;
}

char * cmdline_get(cmdline_t *c, int pos) {
    assert(c!=NULL && pos < c->argc);
    return c->argv[pos];
}

char * cmdline_string_marked(cmdline_t *c) {
    int first = cmdline_next_marked(c,-1);
    if(first == c->argc)
	return xstrdup("");
    else {
	int j = first, count = 1, i=0;
	char **result, *str_result;
	while( (j = cmdline_next_marked(c,j)) != c->argc)
	    ++count;
	result = (char**) xmalloc(sizeof(char*) * count);

	j = first; i = 0;
	while( j != c->argc) {
	    result[i] = c->argv[j];
	    j = cmdline_next_marked(c,j);
	    ++i;
	}

	str_result = flatten_string_array(result, count, " ");
	xfree(result);
	return str_result;
    }
}

/* ========================================================= */

cmditer_t * cmditer(cmdline_t *c) {
    cmditer_t * iter = (cmditer_t*) xmalloc(sizeof(cmditer_t));
    iter->cmd = c;
    iter->pos = 0;
    iter->own_cmd = 0;
    iter->last_pop = -1;
    return iter;
}

cmditer_t * cmditer_new(int argc, char ** argv) {
    cmditer_t * iter = (cmditer_t*) xmalloc(sizeof(cmditer_t));
    iter->cmd = cmdline_new(argc, argv);
    iter->pos = 0;
    iter->own_cmd = 1;
    iter->last_pop = -1;
    return iter;
}

void cmditer_free(cmditer_t *iter) {
    if(iter->own_cmd)
	xfree(iter->cmd);
    xfree(iter);
}

char * cmditer_pop(cmditer_t *iter) {
    if( iter->pos == iter->cmd->argc )
	return NULL;
    else if(cmdline_ismarked(iter->cmd, iter->pos)) {
	iter->pos = cmdline_next_unmarked(iter->cmd, iter->pos);
	return cmditer_pop(iter);
    }
    else {
	char * result = iter->cmd->argv[ iter->pos ];
	cmdline_mark(iter->cmd, iter->pos);
	iter->last_pop = iter->pos;
	return result;
    }
}

/* Can only unpop 1 element */
void cmditer_unpop(cmditer_t *iter) {
    assert(iter->last_pop!=-1);
    cmdline_unmark(iter->cmd, iter->last_pop);
    iter->last_pop = -1;
}

void cmditer_skip(cmditer_t *iter) {
    assert(iter->pos != iter->cmd->argc);
    if(!cmdline_ismarked(iter->cmd, iter->pos))
	++iter->pos;
    else {
	iter->pos = cmdline_next_unmarked(iter->cmd, iter->pos);
	cmditer_skip( iter);
    }
}

char * cmditer_pop_string(cmditer_t *iter, const char *option) {
    char * str = cmditer_pop(iter);
    if(!str)
	CMDLINE_ERROR("Option '%s' requires a string argument", option)
    else
	return str;
}

int cmditer_pop_int(cmditer_t *iter, const char *option) {
    char * str = cmditer_pop(iter);
    if(!str)
	CMDLINE_ERROR("Option '%s' requires an integer argument", option)
    else {
	int result;
	if( sscanf(str, "%d", &result) == 0)
	    CMDLINE_ERROR("Option '%s' requires an integer argument", option)
	else return result;
    }
}

int cmditer_maybe_pop_posint(cmditer_t *iter) {
    char * str = cmditer_pop(iter);
    if(!str) {
	cmditer_unpop(iter);
	return 0;
    }
    else {
	int result;
	if( sscanf(str, "%d", &result) == 0 || result <= 0) {
	    cmditer_unpop(iter);
	    return 0;
	}
	else return result;
    }
}
