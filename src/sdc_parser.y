/* C++ parsers require Bison 3 */
%require "3.0"
%language "C++"

/* Write-out tokens header file */
%defines

/* Use Bison's 'variant' to store values. 
 * This allows us to use non POD types (e.g.
 * with constructors/destrictors), which is
 * not possible with the default mode which
 * uses unions.
 */
%define api.value.type variant

/* 
 * Use the 'complete' symbol type (i.e. variant)
 * in the lexer
 */
%define api.token.constructor

/*
 * Add a prefix the make_* functions used to
 * create the symbols
 */
%define api.token.prefix {TOKEN_}

/*
 * Use a re-entrant (no global vars) parser
 */
/*%define api.pure full*/

/* Wrap everything in our namespace */
%define api.namespace {sdcparse}

/* Name the parser class */
%define parser_class_name {Parser}

/* Match the flex prefix */
%define api.prefix {sdcparse_}

/* Extra checks for correct usage */
%define parse.assert

/* Enable debugging info */
%define parse.trace

/* Better error reporting */
%define parse.error verbose

/* 
 * Fixes inaccuracy in verbose error reporting.
 * May be slow for some grammars.
 */
/*%define parse.lac full*/

/* Track locations */
/*%locations*/

/* Generate a table of token names */
%token-table

%lex-param {Lexer& lexer}
%parse-param {Lexer& lexer}
%parse-param {Callback& callback}


%code requires {
    #include <memory>
    #include "sdcparse.hpp"
    #include "sdc_lexer_fwd.hpp"
}

%code top {
    #include "sdc_lexer.hpp"
    //Bison calls sdcparse_lex() to get the next token.
    //We use the Lexer class as the interface to the lexer, so we
    //re-defined the function to tell Bison how to get the next token.
    static sdcparse::Parser::symbol_type sdcparse_lex(sdcparse::Lexer& lexer) {
        return lexer.next_token();
    }
}

%{

#include <stdio.h>
#include "assert.h"

#include "sdcparse.hpp"
#include "sdc_common.hpp"
#include "sdc_error.hpp"

using namespace sdcparse;

%}

/* declare constant */
%token CMD_CREATE_CLOCK "create_clock"
%token CMD_SET_CLOCK_GROUPS "set_clock_groups"
%token CMD_SET_FALSE_PATH "set_false_path"
%token CMD_SET_MAX_DELAY "set_max_delay"
%token CMD_SET_MIN_DELAY "set_min_delay"
%token CMD_SET_MULTICYCLE_PATH "set_multicycle_path"
%token CMD_SET_INPUT_DELAY "set_input_delay"
%token CMD_SET_OUTPUT_DELAY "set_output_delay"
%token CMD_SET_CLOCK_UNCERTAINTY "set_clock_uncertainty"
%token CMD_SET_CLOCK_LATENCY "set_clock_latency"
%token CMD_SET_DISABLE_TIMING "set_disable_timing"
%token CMD_SET_TIMING_DERATE "set_timing_derate"

%token CMD_GET_PORTS "get_ports"
%token CMD_GET_CLOCKS "get_clocks"
%token CMD_GET_CELLS "get_cells"
%token CMD_GET_PINS "get_pins"

%token ARG_PERIOD "-period"
%token ARG_WAVEFORM "-waveform"
%token ARG_NAME "-name" 
%token ARG_EXCLUSIVE "-exclusive"
%token ARG_GROUP "-group"
%token ARG_FROM "-from"
%token ARG_TO "-to"
%token ARG_SETUP "-setup"
%token ARG_HOLD "-hold"
%token ARG_CLOCK "-clock"
%token ARG_MAX "-max"
%token ARG_MIN "-min"
%token ARG_EARLY "-early"
%token ARG_LATE "-late"
%token ARG_CELL_DELAY "-cell_delay"
%token ARG_NET_DELAY "-net_delay"
%token ARG_SOURCE "-source"

%token LSPAR "["
%token RSPAR "]"
%token LCPAR "{"
%token RCPAR "}"

%token EOL "end-of-line"
%token EOF 0 "end-of-file"

/* declare variable tokens */
%token <std::string> STRING
%token <std::string> ESCAPED_STRING
%token <std::string> CHAR
%token <float> FLOAT_NUMBER
%token <int> INT_NUMBER

/* declare types */
%type <std::string> string
%type <float> number
%type <float> float_number
%type <int> int_number

%type <CreateClock> cmd_create_clock
%type <SetIoDelay> cmd_set_input_delay cmd_set_output_delay
%type <SetClockGroups> cmd_set_clock_groups
%type <SetFalsePath> cmd_set_false_path
%type <SetMinMaxDelay> cmd_set_max_delay cmd_set_min_delay
%type <SetMulticyclePath> cmd_set_multicycle_path
%type <SetClockUncertainty> cmd_set_clock_uncertainty
%type <SetClockLatency> cmd_set_clock_latency
%type <SetDisableTiming> cmd_set_disable_timing
%type <SetTimingDerate> cmd_set_timing_derate

%type <StringGroup> stringGroup cmd_get_ports cmd_get_clocks cmd_get_cells cmd_get_pins

/* Top level rule */
%start sdc_commands

%%

sdc_commands:                                    { }
    | sdc_commands cmd_create_clock EOL          { callback.lineno(lexer.lineno()-1); add_sdc_create_clock(callback, lexer, $2); }
    | sdc_commands cmd_set_input_delay EOL       { callback.lineno(lexer.lineno()-1); add_sdc_set_io_delay(callback, lexer, $2); }
    | sdc_commands cmd_set_output_delay EOL      { callback.lineno(lexer.lineno()-1); add_sdc_set_io_delay(callback, lexer, $2); }
    | sdc_commands cmd_set_clock_groups EOL      { callback.lineno(lexer.lineno()-1); add_sdc_set_clock_groups(callback, lexer, $2); }
    | sdc_commands cmd_set_false_path EOL        { callback.lineno(lexer.lineno()-1); add_sdc_set_false_path(callback, lexer, $2); }
    | sdc_commands cmd_set_max_delay EOL         { callback.lineno(lexer.lineno()-1); add_sdc_set_min_max_delay(callback, lexer, $2); }
    | sdc_commands cmd_set_min_delay EOL         { callback.lineno(lexer.lineno()-1); add_sdc_set_min_max_delay(callback, lexer, $2); }
    | sdc_commands cmd_set_multicycle_path EOL   { callback.lineno(lexer.lineno()-1); add_sdc_set_multicycle_path(callback, lexer, $2); }
    | sdc_commands cmd_set_clock_uncertainty EOL { callback.lineno(lexer.lineno()-1); add_sdc_set_clock_uncertainty(callback, lexer, $2); }
    | sdc_commands cmd_set_clock_latency EOL     { callback.lineno(lexer.lineno()-1); add_sdc_set_clock_latency(callback, lexer, $2); }
    | sdc_commands cmd_set_disable_timing EOL    { callback.lineno(lexer.lineno()-1); add_sdc_set_disable_timing(callback, lexer, $2); }
    | sdc_commands cmd_set_timing_derate EOL     { callback.lineno(lexer.lineno()-1); add_sdc_set_timing_derate(callback, lexer, $2); }
    | sdc_commands EOL                           { /* Eat stray EOL symbols */ }
    ;

cmd_create_clock: CMD_CREATE_CLOCK                          { $$ = CreateClock(); }
    | cmd_create_clock ARG_PERIOD number                    { $$ = $1; sdc_create_clock_set_period(callback, lexer, $$, $3); }
    | cmd_create_clock ARG_NAME string                      { $$ = $1; sdc_create_clock_set_name(callback, lexer, $$, $3); }
    | cmd_create_clock ARG_WAVEFORM LCPAR number number RCPAR   { $$ = $1; sdc_create_clock_set_waveform(callback, lexer, $$, $4, $5); }
    | cmd_create_clock LCPAR stringGroup RCPAR                  { $$ = $1; sdc_create_clock_add_targets(callback, lexer, $$, $3); 
                                                            }
    | cmd_create_clock string                               { $$ = $1; sdc_create_clock_add_targets(callback, lexer, $$, 
                                                                        make_sdc_string_group(sdcparse::StringGroupType::STRING, $2)); 
                                                            }
    ;

cmd_set_input_delay: CMD_SET_INPUT_DELAY        { $$ = SetIoDelay(IoDelayType::INPUT); }
    | cmd_set_input_delay ARG_CLOCK string      { $$ = $1; sdc_set_io_delay_set_clock(callback, lexer, $$, $3); }
    | cmd_set_input_delay ARG_MAX               { $$ = $1; sdc_set_io_delay_set_max(callback, lexer, $$); }
    | cmd_set_input_delay ARG_MIN               { $$ = $1; sdc_set_io_delay_set_min(callback, lexer, $$); }
    | cmd_set_input_delay number                { $$ = $1; sdc_set_io_delay_set_value(callback, lexer, $$, $2); }
    | cmd_set_input_delay LSPAR cmd_get_ports RSPAR { $$ = $1; sdc_set_io_delay_set_ports(callback, lexer, $$, $3); }
    ;

cmd_set_output_delay: CMD_SET_OUTPUT_DELAY       { $$ = SetIoDelay(IoDelayType::OUTPUT); }
    | cmd_set_output_delay ARG_CLOCK string      { $$ = $1; sdc_set_io_delay_set_clock(callback, lexer, $$, $3); }
    | cmd_set_output_delay ARG_MAX               { $$ = $1; sdc_set_io_delay_set_max(callback, lexer, $$); }
    | cmd_set_output_delay ARG_MIN               { $$ = $1; sdc_set_io_delay_set_min(callback, lexer, $$); }
    | cmd_set_output_delay number                { $$ = $1; sdc_set_io_delay_set_value(callback, lexer, $$, $2); }
    | cmd_set_output_delay LSPAR cmd_get_ports RSPAR { $$ = $1; sdc_set_io_delay_set_ports(callback, lexer, $$, $3); }
    ;

cmd_set_clock_groups: CMD_SET_CLOCK_GROUPS                  { $$ = SetClockGroups(); }
    | cmd_set_clock_groups ARG_EXCLUSIVE                    { $$ = $1; sdc_set_clock_groups_set_type(callback, lexer, $$, ClockGroupsType::EXCLUSIVE); }
    | cmd_set_clock_groups ARG_GROUP LSPAR cmd_get_clocks RSPAR { $$ = $1; sdc_set_clock_groups_add_group(callback, lexer, $$, $4); }
    | cmd_set_clock_groups ARG_GROUP LCPAR stringGroup RCPAR    { $$ = $1; sdc_set_clock_groups_add_group(callback, lexer, $$, $4); }
    | cmd_set_clock_groups ARG_GROUP     string             { $$ = $1; sdc_set_clock_groups_add_group(callback, lexer, $$, 
                                                                    make_sdc_string_group(sdcparse::StringGroupType::STRING, $3)); 
                                                            }
    ;

cmd_set_false_path: CMD_SET_FALSE_PATH                      { $$ = SetFalsePath(); }
    | cmd_set_false_path ARG_FROM LSPAR cmd_get_clocks RSPAR    { $$ = $1; sdc_set_false_path_add_to_from_group(callback, lexer, $$, $4, FromToType::FROM); }
    | cmd_set_false_path ARG_TO   LSPAR cmd_get_clocks RSPAR    { $$ = $1; sdc_set_false_path_add_to_from_group(callback, lexer, $$, $4, FromToType::TO  ); }
    | cmd_set_false_path ARG_FROM LCPAR stringGroup RCPAR       { $$ = $1; sdc_set_false_path_add_to_from_group(callback, lexer, $$, $4, FromToType::FROM); }
    | cmd_set_false_path ARG_TO   LCPAR stringGroup RCPAR       { $$ = $1; sdc_set_false_path_add_to_from_group(callback, lexer, $$, $4, FromToType::TO  ); }
    | cmd_set_false_path ARG_FROM     string                { $$ = $1; sdc_set_false_path_add_to_from_group(callback, lexer, $$, 
                                                                    make_sdc_string_group(sdcparse::StringGroupType::STRING, $3), 
                                                                    FromToType::FROM); 
                                                            }
    | cmd_set_false_path ARG_TO       string                { $$ = $1; sdc_set_false_path_add_to_from_group(callback, lexer, $$, 
                                                                    make_sdc_string_group(sdcparse::StringGroupType::STRING, $3), 
                                                                    FromToType::TO  ); 
                                                            }
    ;

cmd_set_max_delay: CMD_SET_MAX_DELAY                        { $$ = SetMinMaxDelay(MinMaxType::MAX); }
    | cmd_set_max_delay number                              { $$ = $1; sdc_set_min_max_delay_set_value(callback, lexer, $$, $2); }
    | cmd_set_max_delay ARG_FROM LSPAR cmd_get_clocks RSPAR { $$ = $1; sdc_set_min_max_delay_add_to_from_group(callback, lexer, $$, $4, FromToType::FROM); }
    | cmd_set_max_delay ARG_TO   LSPAR cmd_get_clocks RSPAR { $$ = $1; sdc_set_min_max_delay_add_to_from_group(callback, lexer, $$, $4, FromToType::TO  ); }
    | cmd_set_max_delay ARG_FROM LCPAR stringGroup RCPAR    { $$ = $1; sdc_set_min_max_delay_add_to_from_group(callback, lexer, $$, $4, FromToType::FROM); }
    | cmd_set_max_delay ARG_TO   LCPAR stringGroup RCPAR    { $$ = $1; sdc_set_min_max_delay_add_to_from_group(callback, lexer, $$, $4, FromToType::TO  ); }
    | cmd_set_max_delay ARG_FROM     string                 { $$ = $1; sdc_set_min_max_delay_add_to_from_group(callback, lexer, $$, 
                                                                    make_sdc_string_group(sdcparse::StringGroupType::STRING, $3), 
                                                                    FromToType::FROM);
                                                            }
    | cmd_set_max_delay ARG_TO       string                 { $$ = $1; sdc_set_min_max_delay_add_to_from_group(callback, lexer, $$, 
                                                                    make_sdc_string_group(sdcparse::StringGroupType::STRING, $3),
                                                                    FromToType::TO);
                                                            }
    ;

cmd_set_min_delay: CMD_SET_MIN_DELAY                        { $$ = SetMinMaxDelay(MinMaxType::MIN); }
    | cmd_set_min_delay number                              { $$ = $1; sdc_set_min_max_delay_set_value(callback, lexer, $$, $2); }
    | cmd_set_min_delay ARG_FROM LSPAR cmd_get_clocks RSPAR { $$ = $1; sdc_set_min_max_delay_add_to_from_group(callback, lexer, $$, $4, FromToType::FROM); }
    | cmd_set_min_delay ARG_TO   LSPAR cmd_get_clocks RSPAR { $$ = $1; sdc_set_min_max_delay_add_to_from_group(callback, lexer, $$, $4, FromToType::TO  ); }
    | cmd_set_min_delay ARG_FROM LCPAR stringGroup RCPAR    { $$ = $1; sdc_set_min_max_delay_add_to_from_group(callback, lexer, $$, $4, FromToType::FROM); }
    | cmd_set_min_delay ARG_TO   LCPAR stringGroup RCPAR    { $$ = $1; sdc_set_min_max_delay_add_to_from_group(callback, lexer, $$, $4, FromToType::TO  ); }
    | cmd_set_min_delay ARG_FROM     string                 { $$ = $1;
                                                              sdc_set_min_max_delay_add_to_from_group(callback, lexer, $$, 
                                                                    make_sdc_string_group(sdcparse::StringGroupType::STRING, $3), 
                                                                    FromToType::FROM);
                                                            }
    | cmd_set_min_delay ARG_TO       string                 { $$ = $1; 
                                                              sdc_set_min_max_delay_add_to_from_group(callback, lexer, $$, 
                                                                    make_sdc_string_group(sdcparse::StringGroupType::STRING, $3),
                                                                    FromToType::TO);
                                                            }
    ;

cmd_set_multicycle_path: CMD_SET_MULTICYCLE_PATH                  { $$ = SetMulticyclePath(); }
    | cmd_set_multicycle_path int_number                          { $$ = $1; sdc_set_multicycle_path_set_mcp_value(callback, lexer, $$, $2); }
    | cmd_set_multicycle_path ARG_SETUP                           { $$ = $1; sdc_set_multicycle_path_set_setup(callback, lexer, $$); }
    | cmd_set_multicycle_path ARG_HOLD                            { $$ = $1; sdc_set_multicycle_path_set_hold(callback, lexer, $$); }
    | cmd_set_multicycle_path ARG_FROM LSPAR cmd_get_clocks RSPAR { $$ = $1; sdc_set_multicycle_path_add_to_from_group(callback, lexer, $$, $4, FromToType::FROM); }
    | cmd_set_multicycle_path ARG_TO   LSPAR cmd_get_clocks RSPAR { $$ = $1; sdc_set_multicycle_path_add_to_from_group(callback, lexer, $$, $4, FromToType::TO); }
    | cmd_set_multicycle_path ARG_FROM LCPAR stringGroup RCPAR    { $$ = $1; sdc_set_multicycle_path_add_to_from_group(callback, lexer, $$, $4, FromToType::FROM); }
    | cmd_set_multicycle_path ARG_TO   LCPAR stringGroup RCPAR    { $$ = $1; sdc_set_multicycle_path_add_to_from_group(callback, lexer, $$, $4, FromToType::TO); }
    | cmd_set_multicycle_path ARG_FROM     string                 { $$ = $1; sdc_set_multicycle_path_add_to_from_group(callback, lexer, $$, 
                                                                          make_sdc_string_group(sdcparse::StringGroupType::STRING, $3), 
                                                                          FromToType::FROM);
                                                                  }
    | cmd_set_multicycle_path ARG_TO       string                 { $$ = $1; sdc_set_multicycle_path_add_to_from_group(callback, lexer, $$, 
                                                                          make_sdc_string_group(sdcparse::StringGroupType::STRING, $3),
                                                                          FromToType::TO);
                                                                  }
    ;

cmd_set_clock_uncertainty: CMD_SET_CLOCK_UNCERTAINTY                { $$ = SetClockUncertainty(); }
    | cmd_set_clock_uncertainty ARG_SETUP                           { $$ = $1; sdc_set_clock_uncertainty_set_setup(callback, lexer, $$); }
    | cmd_set_clock_uncertainty ARG_HOLD                            { $$ = $1; sdc_set_clock_uncertainty_set_hold(callback, lexer, $$); }
    | cmd_set_clock_uncertainty number                              { $$ = $1; sdc_set_clock_uncertainty_set_value(callback, lexer, $$, $2); }
    | cmd_set_clock_uncertainty ARG_FROM LSPAR cmd_get_clocks RSPAR { $$ = $1; sdc_set_clock_uncertainty_add_to_from_group(callback, lexer, $$, $4, FromToType::FROM); }
    | cmd_set_clock_uncertainty ARG_TO   LSPAR cmd_get_clocks RSPAR { $$ = $1; sdc_set_clock_uncertainty_add_to_from_group(callback, lexer, $$, $4, FromToType::TO); }
    | cmd_set_clock_uncertainty ARG_FROM LCPAR stringGroup RCPAR    { $$ = $1; sdc_set_clock_uncertainty_add_to_from_group(callback, lexer, $$, $4, FromToType::FROM); }
    | cmd_set_clock_uncertainty ARG_TO   LCPAR stringGroup RCPAR    { $$ = $1; sdc_set_clock_uncertainty_add_to_from_group(callback, lexer, $$, $4, FromToType::TO); }
    | cmd_set_clock_uncertainty ARG_FROM     string                 { $$ = $1;
                                                                      sdc_set_clock_uncertainty_add_to_from_group(callback, lexer, $$, 
                                                                          make_sdc_string_group(sdcparse::StringGroupType::STRING, $3),
                                                                          FromToType::FROM);
                                                                    }
    | cmd_set_clock_uncertainty ARG_TO       string                 { $$ = $1;
                                                                      sdc_set_clock_uncertainty_add_to_from_group(callback, lexer, $$, 
                                                                          make_sdc_string_group(sdcparse::StringGroupType::STRING, $3),
                                                                          FromToType::TO);
                                                                    }
    ;

cmd_set_clock_latency: CMD_SET_CLOCK_LATENCY                    { $$ = SetClockLatency(); }
    | cmd_set_clock_latency ARG_SOURCE                          { $$ = $1; sdc_set_clock_latency_set_type(callback, lexer, $$, ClockLatencyType::SOURCE); }
    | cmd_set_clock_latency ARG_EARLY                           { $$ = $1; sdc_set_clock_latency_early(callback, lexer, $$); }
    | cmd_set_clock_latency ARG_LATE                            { $$ = $1; sdc_set_clock_latency_late(callback, lexer, $$); }
    | cmd_set_clock_latency number                              { $$ = $1; sdc_set_clock_latency_set_value(callback, lexer, $$, $2); }
    | cmd_set_clock_latency LSPAR cmd_get_clocks RSPAR          { $$ = $1; sdc_set_clock_latency_set_clocks(callback, lexer, $$, $3); }
    ;


cmd_set_disable_timing: CMD_SET_DISABLE_TIMING                       { $$ = SetDisableTiming(); }
    | cmd_set_disable_timing ARG_FROM LSPAR cmd_get_pins RSPAR { $$ = $1; sdc_set_disable_timing_add_to_from_group(callback, lexer, $$, $4, FromToType::FROM); }
    | cmd_set_disable_timing ARG_TO   LSPAR cmd_get_pins RSPAR { $$ = $1; sdc_set_disable_timing_add_to_from_group(callback, lexer, $$, $4, FromToType::TO  ); }
    | cmd_set_disable_timing ARG_FROM LCPAR stringGroup RCPAR    { $$ = $1; sdc_set_disable_timing_add_to_from_group(callback, lexer, $$, $4, FromToType::FROM); }
    | cmd_set_disable_timing ARG_TO   LCPAR stringGroup RCPAR    { $$ = $1; sdc_set_disable_timing_add_to_from_group(callback, lexer, $$, $4, FromToType::TO  ); }
    | cmd_set_disable_timing ARG_FROM     string                 { $$ = $1; 
                                                                   sdc_set_disable_timing_add_to_from_group(callback, lexer, $$, 
                                                                     make_sdc_string_group(sdcparse::StringGroupType::STRING, $3), 
                                                                     FromToType::FROM); 
                                                                 }
    | cmd_set_disable_timing ARG_TO       string                 { $$ = $1; 
                                                                   sdc_set_disable_timing_add_to_from_group(callback, lexer, $$, 
                                                                     make_sdc_string_group(sdcparse::StringGroupType::STRING, $3), 
                                                                     FromToType::TO  ); 
                                                                 }
    ;

cmd_set_timing_derate: CMD_SET_TIMING_DERATE    { $$ = SetTimingDerate(); }
    | cmd_set_timing_derate ARG_EARLY           { $$ = $1; sdc_set_timing_derate_early(callback, lexer, $$); }
    | cmd_set_timing_derate ARG_LATE            { $$ = $1; sdc_set_timing_derate_late(callback, lexer, $$); }
    | cmd_set_timing_derate ARG_CELL_DELAY      { $$ = $1; sdc_set_timing_derate_target_type(callback, lexer, $$, TimingDerateTargetType::NET); }
    | cmd_set_timing_derate ARG_NET_DELAY       { $$ = $1; sdc_set_timing_derate_target_type(callback, lexer, $$, TimingDerateTargetType::CELL); }
    | cmd_set_timing_derate number              { $$ = $1; sdc_set_timing_derate_value(callback, lexer, $$, $2); }
    | cmd_set_timing_derate LSPAR cmd_get_cells RSPAR { $$ = $1; sdc_set_timing_derate_targets(callback, lexer, $$, $3); }
    ;

cmd_get_ports: CMD_GET_PORTS            { $$ = StringGroup(StringGroupType::PORT); }
    | cmd_get_ports LCPAR stringGroup RCPAR { $$ = $1; sdc_string_group_add_strings($$, $3); }
    | cmd_get_ports string              { $$ = $1; sdc_string_group_add_string($$, $2); }
    ;

cmd_get_clocks: CMD_GET_CLOCKS              { $$ = StringGroup(StringGroupType::CLOCK); }
    | cmd_get_clocks LCPAR stringGroup RCPAR    { $$ = $1; sdc_string_group_add_strings($$, $3); }
    | cmd_get_clocks string                 { $$ = $1; sdc_string_group_add_string($$, $2); }
    ;

cmd_get_cells: CMD_GET_CELLS            { $$ = StringGroup(StringGroupType::CELL); }
    | cmd_get_cells LCPAR stringGroup RCPAR { $$ = $1; sdc_string_group_add_strings($$, $3); }
    | cmd_get_cells string              { $$ = $1; sdc_string_group_add_string($$, $2); }
    ;

cmd_get_pins: CMD_GET_PINS            { $$ = StringGroup(StringGroupType::PIN); }
    | cmd_get_pins LCPAR stringGroup RCPAR { $$ = $1; sdc_string_group_add_strings($$, $3); }
    | cmd_get_pins string              { $$ = $1; sdc_string_group_add_string($$, $2); }
    ;


stringGroup: /*empty*/   { $$ = StringGroup(StringGroupType::STRING); }
    | stringGroup string { $$ = $1; sdc_string_group_add_string($$, $2); } 

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


void sdcparse::Parser::error(const std::string& msg) {
    sdc_error_wrap(callback, lexer.lineno(), lexer.text(), msg.c_str());
}
