%{

#include <stdio.h>
#include "assert.h"

#include "sdc_common.h"


int yyerror(const char *msg);
extern int yylex(void);

%}

%union {
    char* strVal;
    double floatVal;
    int intVal;

    t_sdc_commands* sdc_commands;

    t_sdc_create_clock* create_clock;
    t_sdc_set_io_delay* set_io_delay;
    t_sdc_set_clock_groups* set_clock_groups;
    t_sdc_set_false_path* set_false_path;
    t_sdc_set_max_delay* set_max_delay;
    t_sdc_set_multicycle_path* set_multicycle_path;

    t_sdc_string_group* string_group;

}

/* Verbose error reporting */
%error-verbose

/* declare constant */
%token CMD_CREATE_CLOCK
%token CMD_SET_CLOCK_GROUPS
%token CMD_SET_FALSE_PATH
%token CMD_SET_MAX_DELAY
%token CMD_SET_MULTICYCLE_PATH
%token CMD_SET_INPUT_DELAY
%token CMD_SET_OUTPUT_DELAY

%token CMD_GET_PORTS
%token CMD_GET_CLOCKS

%token ARG_PERIOD
%token ARG_WAVEFORM
%token ARG_NAME
%token ARG_EXCLUSIVE
%token ARG_GROUP
%token ARG_FROM
%token ARG_TO
%token ARG_SETUP
%token ARG_CLOCK
%token ARG_MAX

%token EOL

/* declare variable tokens */
%token <strVal> STRING
%token <strVal> ESCAPED_STRING
%token <strVal> CHAR
%token <floatVal> FLOAT_NUMBER
%token <intVal> INT_NUMBER

/* declare types */
%type <strVal> string
%type <floatVal> number
%type <floatVal> float_number
%type <intVal> int_number

%type <sdc_commands> sdc_commands
%type <create_clock> cmd_create_clock
%type <set_io_delay> cmd_set_input_delay cmd_set_output_delay
%type <set_clock_groups> cmd_set_clock_groups
%type <set_false_path> cmd_set_false_path
%type <set_max_delay> cmd_set_max_delay
%type <set_multicycle_path> cmd_set_multicycle_path

%type <string_group> stringGroup cmd_get_ports cmd_get_clocks

/* Top level rule */
%start sdc_commands

%%
sdc_commands:                                   { g_sdc_commands = alloc_sdc_commands(); $$ = g_sdc_commands; }
    | sdc_commands cmd_create_clock EOL         { $$ = add_sdc_create_clock($1, $2); }
    | sdc_commands cmd_set_input_delay EOL      { $$ = add_sdc_set_io_delay($1, $2); }
    | sdc_commands cmd_set_output_delay EOL     { $$ = add_sdc_set_io_delay($1, $2); }
    | sdc_commands cmd_set_clock_groups EOL     { $$ = add_sdc_set_clock_groups($1, $2); }
    | sdc_commands cmd_set_false_path EOL       { $$ = add_sdc_set_false_path($1, $2); }
    | sdc_commands cmd_set_max_delay EOL        { $$ = add_sdc_set_max_delay($1, $2); }
    | sdc_commands cmd_set_multicycle_path EOL  { $$ = add_sdc_set_multicycle_path($1, $2); }
    | sdc_commands EOL                          { /* Eat stray EOL symbols */ $$ = $1; }
    ;

cmd_create_clock: CMD_CREATE_CLOCK                          { $$ = alloc_sdc_create_clock(); }
    | cmd_create_clock ARG_PERIOD number                    { $$ = sdc_create_clock_set_period($1, $3); }
    | cmd_create_clock ARG_NAME string                      { $$ = sdc_create_clock_set_name($1, $3); free($3); }
    | cmd_create_clock ARG_WAVEFORM '{' number number '}'   { $$ = sdc_create_clock_set_waveform($1, $4, $5); }
    | cmd_create_clock '{' stringGroup '}'                  { $$ = sdc_create_clock_add_targets($1, $3); free_sdc_string_group($3); }
    | cmd_create_clock string                               { $$ = sdc_create_clock_add_targets($1, make_sdc_string_group(SDC_STRING, $2)); free($2); }
    ;

cmd_set_input_delay: CMD_SET_INPUT_DELAY        { $$ = alloc_sdc_set_io_delay(SDC_INPUT_DELAY); }
    | cmd_set_input_delay ARG_CLOCK string      { $$ = sdc_set_io_delay_set_clock($1, $3); free($3); }
    | cmd_set_input_delay ARG_MAX number        { $$ = sdc_set_io_delay_set_max_value($1, $3); }
    | cmd_set_input_delay '[' cmd_get_ports ']' { $$ = sdc_set_io_delay_set_ports($1, $3); free_sdc_string_group($3); }
    ;

cmd_set_output_delay: CMD_SET_OUTPUT_DELAY       { $$ = alloc_sdc_set_io_delay(SDC_OUTPUT_DELAY); }
    | cmd_set_output_delay ARG_CLOCK string      { $$ = sdc_set_io_delay_set_clock($1, $3); free($3); }
    | cmd_set_output_delay ARG_MAX number        { $$ = sdc_set_io_delay_set_max_value($1, $3); }
    | cmd_set_output_delay '[' cmd_get_ports ']' { $$ = sdc_set_io_delay_set_ports($1, $3); free_sdc_string_group($3); }
    ;

cmd_set_clock_groups: CMD_SET_CLOCK_GROUPS                  { $$ = alloc_sdc_set_clock_groups(); }
    | cmd_set_clock_groups ARG_EXCLUSIVE                    { $$ = sdc_set_clock_groups_set_type($1, SDC_CG_EXCLUSIVE); }
    | cmd_set_clock_groups ARG_GROUP '[' cmd_get_clocks ']' { $$ = sdc_set_clock_groups_add_group($1, $4); free_sdc_string_group($4); }
    | cmd_set_clock_groups ARG_GROUP '{' stringGroup '}'    { $$ = sdc_set_clock_groups_add_group($1, $4); free_sdc_string_group($4);}
    | cmd_set_clock_groups ARG_GROUP     string             { $$ = sdc_set_clock_groups_add_group($1, make_sdc_string_group(SDC_STRING, $3)); free($3); }
    ;

cmd_set_false_path: CMD_SET_FALSE_PATH                      { $$ = alloc_sdc_set_false_path(); }
    | cmd_set_false_path ARG_FROM '[' cmd_get_clocks ']'    { $$ = sdc_set_false_path_add_to_from_group($1, $4, SDC_FROM); free_sdc_string_group($4); }
    | cmd_set_false_path ARG_TO   '[' cmd_get_clocks ']'    { $$ = sdc_set_false_path_add_to_from_group($1, $4, SDC_TO  ); free_sdc_string_group($4); }
    | cmd_set_false_path ARG_FROM '{' stringGroup '}'       { $$ = sdc_set_false_path_add_to_from_group($1, $4, SDC_FROM); free_sdc_string_group($4); }
    | cmd_set_false_path ARG_TO   '{' stringGroup '}'       { $$ = sdc_set_false_path_add_to_from_group($1, $4, SDC_TO  ); free_sdc_string_group($4); }
    | cmd_set_false_path ARG_FROM     string                { $$ = sdc_set_false_path_add_to_from_group($1, make_sdc_string_group(SDC_STRING, $3), SDC_FROM); free($3); }
    | cmd_set_false_path ARG_TO       string                { $$ = sdc_set_false_path_add_to_from_group($1, make_sdc_string_group(SDC_STRING, $3), SDC_TO  ); free($3); }
    ;

cmd_set_max_delay: CMD_SET_MAX_DELAY                        { $$ = alloc_sdc_set_max_delay(); }
    | cmd_set_max_delay number                              { $$ = sdc_set_max_delay_set_max_delay_value($1, $2); }
    | cmd_set_max_delay ARG_FROM '[' cmd_get_clocks ']'     { $$ = sdc_set_max_delay_add_to_from_group($1, $4, SDC_FROM); free_sdc_string_group($4); }
    | cmd_set_max_delay ARG_TO   '[' cmd_get_clocks ']'     { $$ = sdc_set_max_delay_add_to_from_group($1, $4, SDC_TO  ); free_sdc_string_group($4); }
    | cmd_set_max_delay ARG_FROM '{' stringGroup '}'        { $$ = sdc_set_max_delay_add_to_from_group($1, $4, SDC_FROM); free_sdc_string_group($4); }
    | cmd_set_max_delay ARG_TO   '{' stringGroup '}'        { $$ = sdc_set_max_delay_add_to_from_group($1, $4, SDC_TO  ); free_sdc_string_group($4); }
    | cmd_set_max_delay ARG_FROM     string                 { $$ = sdc_set_max_delay_add_to_from_group($1, make_sdc_string_group(SDC_STRING, $3), SDC_FROM); free($3); }
    | cmd_set_max_delay ARG_TO       string                 { $$ = sdc_set_max_delay_add_to_from_group($1, make_sdc_string_group(SDC_STRING, $3), SDC_TO  ); free($3); }
    ;

cmd_set_multicycle_path: CMD_SET_MULTICYCLE_PATH                { $$ = alloc_sdc_set_multicycle_path(); }
    | cmd_set_multicycle_path int_number                        { $$ = sdc_set_multicycle_path_set_mcp_value($1, $2); }
    | cmd_set_multicycle_path ARG_SETUP                         { $$ = sdc_set_multicycle_path_set_type($1, SDC_MCP_SETUP); }
    | cmd_set_multicycle_path ARG_FROM '[' cmd_get_clocks ']'   { $$ = sdc_set_multicycle_path_add_to_from_group($1, $4, SDC_FROM); free_sdc_string_group($4); }
    | cmd_set_multicycle_path ARG_TO   '[' cmd_get_clocks ']'   { $$ = sdc_set_multicycle_path_add_to_from_group($1, $4, SDC_TO  ); free_sdc_string_group($4); }
    | cmd_set_multicycle_path ARG_FROM '{' stringGroup '}'      { $$ = sdc_set_multicycle_path_add_to_from_group($1, $4, SDC_FROM); free_sdc_string_group($4); }
    | cmd_set_multicycle_path ARG_TO   '{' stringGroup '}'      { $$ = sdc_set_multicycle_path_add_to_from_group($1, $4, SDC_TO  ); free_sdc_string_group($4); }
    | cmd_set_multicycle_path ARG_FROM     string               { $$ = sdc_set_multicycle_path_add_to_from_group($1, make_sdc_string_group(SDC_STRING, $3), SDC_FROM); free($3); }
    | cmd_set_multicycle_path ARG_TO       string               { $$ = sdc_set_multicycle_path_add_to_from_group($1, make_sdc_string_group(SDC_STRING, $3), SDC_TO  ); free($3); }
    ;

cmd_get_ports: CMD_GET_PORTS            { $$ = alloc_sdc_string_group(SDC_PORT); }
    | cmd_get_ports '{' stringGroup '}' { $$ = sdc_string_group_add_strings($1, $3); free_sdc_string_group($3); }
    | cmd_get_ports string              { $$ = sdc_string_group_add_string($1, $2); free($2); }
    ;

cmd_get_clocks: CMD_GET_CLOCKS              { $$ = alloc_sdc_string_group(SDC_CLOCK); }
    | cmd_get_clocks '{' stringGroup '}'    { $$ = sdc_string_group_add_strings($1, $3); free_sdc_string_group($3); }
    | cmd_get_clocks string                 { $$ = sdc_string_group_add_string($1, $2); free($2); }
    ;

stringGroup: /*empty*/   { $$ = alloc_sdc_string_group(SDC_STRING); }
    | stringGroup string { $$ = sdc_string_group_add_string($1, $2); free($2); } 

string: STRING         { $$ = $1; }
    | ESCAPED_STRING        { $$ = $1; }
    ;

number: float_number { $$ = $1; }
    | int_number { $$ = $1; }
    ;

float_number: FLOAT_NUMBER { $$ = $1; }
    ;

int_number: INT_NUMBER { $$ = $1; }
    ;

%%


int yyerror(const char *msg) {
    sdc_error(yylineno, yytext, "%s\n", msg);
    return 1;
}
