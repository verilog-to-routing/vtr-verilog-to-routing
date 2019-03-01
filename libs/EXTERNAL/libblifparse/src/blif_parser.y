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
%define api.namespace {blifparse}

/* Name the parser class */
%define parser_class_name {Parser}

/* Match the flex prefix */
%define api.prefix {blifparse_}

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
    #include "blifparse.hpp"
    #include "blif_lexer_fwd.hpp"
}

%code top {
    #include "blif_lexer.hpp"
    //Bison calls blifparse_lex() to get the next token.
    //We use the Lexer class as the interface to the lexer, so we
    //re-defined the function to tell Bison how to get the next token.
    static blifparse::Parser::symbol_type blifparse_lex(blifparse::Lexer& lexer) {
        return lexer.next_token();
    }
}

%{

#include <stdio.h>
#include "assert.h"

#include "blifparse.hpp"
#include "blif_common.hpp"
#include "blif_error.hpp"

using namespace blifparse;

%}

/* Declare constant */
%token DOT_NAMES ".names"
%token DOT_LATCH ".latch"
%token DOT_MODEL ".model"
%token DOT_SUBCKT ".subckt"
%token DOT_INPUTS ".inputs"
%token DOT_OUTPUTS ".outputs"
%token DOT_CLOCK ".clock"
%token DOT_END ".end"
%token DOT_BLACKBOX ".blackbox"
%token LATCH_FE "fe"
%token LATCH_RE "re"
%token LATCH_AH "ah"
%token LATCH_AL "al"
%token LATCH_AS "as"
%token NIL "NIL"
%token LATCH_INIT_2 "2"
%token LATCH_INIT_3 "3"
%token LOGIC_FALSE "0"
%token LOGIC_TRUE "1"
%token LOGIC_DONT_CARE "-"
%token EQ "="
%token EOL "end-of-line"
%token EOF 0 "end-of-file"

/*BLIF extensions */
%token DOT_CONN ".conn"
%token DOT_ATTR ".attr"
%token DOT_PARAM ".param"
%token DOT_CNAME ".cname"

/* declare variable tokens */
%token <std::string> STRING

/* declare types */
%type <SubCkt> subckt
%type <Names> names
%type <std::vector<std::string>> string_list
%type <std::vector<LogicValue>> so_cover_row
%type <LogicValue> logic_value
%type <LogicValue> latch_init
%type <std::string> latch_control
%type <LatchType> latch_type

/* BLIF Extensions */
%type <Conn> conn
%type <Cname> cname
%type <Attr> attr
%type <Param> param

/* Top level rule */
%start blif_data

%%

blif_data: /*empty*/ { }
    | blif_data DOT_MODEL STRING EOL        { callback.lineno(lexer.lineno()-1); callback.begin_model($3); }
    | blif_data DOT_INPUTS string_list EOL  { callback.lineno(lexer.lineno()-1); callback.inputs($3); }
    | blif_data DOT_OUTPUTS string_list EOL { callback.lineno(lexer.lineno()-1); callback.outputs($3); }
    | blif_data names                       { callback.lineno(lexer.lineno()-1); callback.names($2.nets, $2.so_cover); }
    | blif_data subckt EOL                  { 
                                              if($2.ports.size() != $2.nets.size()) {
                                                  blif_error_wrap(callback ,lexer.lineno()-1, lexer.text(), 
                                                    "Mismatched subckt port and net connection(s) size do not match"
                                                    " (%zu ports, %zu nets)", $2.ports.size(), $2.nets.size());
                                              }
                                              callback.lineno(lexer.lineno()-1); 
                                              callback.subckt($2.model, $2.ports, $2.nets);
                                            }
    | blif_data latch EOL                   { /*callback already called */ }
    | blif_data DOT_BLACKBOX EOL            { callback.lineno(lexer.lineno()-1); callback.blackbox(); }
    | blif_data DOT_END EOL                 { callback.lineno(lexer.lineno()-1); callback.end_model(); }
    | blif_data conn EOL                    { callback.lineno(lexer.lineno()-1); callback.conn($2.src, $2.dst); }
    | blif_data cname EOL                   { callback.lineno(lexer.lineno()-1); callback.cname($2.name); }
    | blif_data attr EOL                    { callback.lineno(lexer.lineno()-1); callback.attr($2.name, $2.value); }
    | blif_data param EOL                   { callback.lineno(lexer.lineno()-1); callback.param($2.name, $2.value); }
    | blif_data EOL                         { /* eat end-of-lines */}
    ;

names: DOT_NAMES string_list EOL { $$ = Names(); $$.nets = $2; }
    | names so_cover_row EOL { 
                                $$ = std::move($1); 
                                if($$.nets.size() != $2.size()) {
                                    blif_error_wrap(callback, lexer.lineno()-1, lexer.text(),
                                        "Mismatched .names single-output cover row."
                                        " names connected to %zu net(s), but cover row has %zu element(s)",
                                        $$.nets.size(), $2.size());
                                }
                                $$.so_cover.push_back($2); 
                             }
    ;

subckt: DOT_SUBCKT STRING       { $$ = SubCkt(); $$.model = $2; }
    | subckt STRING EQ STRING   { $$ = std::move($1); $$.ports.push_back($2); $$.nets.push_back($4); }
    ;

latch: DOT_LATCH STRING STRING {
                                    //Input and output only
                                    callback.lineno(lexer.lineno()); 
                                    callback.latch($2, $3, LatchType::UNSPECIFIED, "", LogicValue::UNKOWN);
                               }
    | DOT_LATCH STRING STRING latch_type latch_control {
                                    //Input, output, type and control
                                    callback.lineno(lexer.lineno()); 
                                    callback.latch($2, $3, $4, $5, LogicValue::UNKOWN);
                               }
    | DOT_LATCH STRING STRING latch_type latch_control latch_init {
                                    //Input, output, type, control and init-value
                                    callback.lineno(lexer.lineno()); 
                                    callback.latch($2, $3, $4, $5, $6);
                               }
    | DOT_LATCH STRING STRING latch_init {
                                    //Input, output, and init-value
                                    callback.lineno(lexer.lineno());
                                    callback.latch($2, $3, LatchType::UNSPECIFIED, "", $4);
                               }
    ;

latch_init: LOGIC_TRUE { $$ = LogicValue::TRUE; }
    | LOGIC_FALSE { $$ = LogicValue::FALSE; }
    | LATCH_INIT_2 { $$ = LogicValue::DONT_CARE; }
    | LATCH_INIT_3 { $$ = LogicValue::UNKOWN; }
    ;

latch_control: STRING { $$ = $1;}
    | NIL { $$ = ""; }
    ;

latch_type: LATCH_FE { $$ = LatchType::FALLING_EDGE; }
    | LATCH_RE { $$ = LatchType::RISING_EDGE; }
    | LATCH_AH { $$ = LatchType::ACTIVE_HIGH; }
    | LATCH_AL { $$ = LatchType::ACTIVE_LOW; }
    | LATCH_AS { $$ = LatchType::ASYNCHRONOUS; }
    ;

so_cover_row: logic_value { $$ = std::vector<LogicValue>(); $$.push_back($1); }
    | so_cover_row logic_value { $$ = std::move($1); $$.push_back($2); }
    ;

logic_value: LOGIC_TRUE { $$ = LogicValue::TRUE; }
    | LOGIC_FALSE { $$ = LogicValue::FALSE; }
    | LOGIC_DONT_CARE { $$ = LogicValue::DONT_CARE; }
    ;

string_list: /*empty*/ { $$ = std::vector<std::string>(); }
    | string_list STRING { $$ = std::move($1); $$.push_back($2); }
    ;

/*
 * BLIF Extensions
 */
conn: DOT_CONN STRING STRING { $$ = Conn(); $$.src = $2; $$.dst = $3; }
cname: DOT_CNAME STRING { $$ = Cname(); $$.name = $2; }
attr: DOT_ATTR STRING STRING { $$ = Attr(); $$.name = $2; $$.value = $3; }
    | DOT_ATTR STRING { $$ = Attr(); $$.name = $2; $$.value = ""; }
param: DOT_PARAM STRING STRING { $$ = Param(); $$.name = $2; $$.value = $3; }
    | DOT_PARAM STRING { $$ = Param(); $$.name = $2; $$.value = ""; }

%%

void blifparse::Parser::error(const std::string& msg) {
    blif_error_wrap(callback, lexer.lineno(), lexer.text(), msg.c_str());
}
