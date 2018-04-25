#ifndef CMDLINE_H
#define CMDLINE_H

typedef enum cmdline_mark {
    CMD_NONE,
    CMD_PARSED
} cmdline_mark_t;

typedef struct cmdline {
    char ** argv;
    int argc;
    cmdline_mark_t * marks;
} cmdline_t;

typedef struct cmditer {
    cmdline_t * cmd;
    int own_cmd;
    int pos;
    int last_pop;
} cmditer_t;

cmdline_t * cmdline_new(int argc, char **argv);
void cmdline_free(cmdline_t *c);
int cmdline_next_unmarked(cmdline_t *c, int current);
int cmdline_mark(cmdline_t *c, int pos);
void cmdline_unmark(cmdline_t *c, int pos);
int cmdline_ismarked(cmdline_t *c, int pos);
char * cmdline_get(cmdline_t *c, int pos);
char * cmdline_string_marked(cmdline_t *c);

cmditer_t * cmditer(cmdline_t *c);
cmditer_t * cmditer_new(int argc, char ** argv);
void cmditer_free(cmditer_t *iter);
char * cmditer_pop(cmditer_t *iter);
char * cmditer_pop_string(cmditer_t *iter, const char *option);
int cmditer_pop_int(cmditer_t *iter, const char *option);
int cmditer_maybe_pop_posint(cmditer_t *iter);
void cmditer_unpop(cmditer_t *iter);
void cmditer_skip(cmditer_t *iter);

#endif
