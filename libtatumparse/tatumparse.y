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
%define api.namespace {tatumparse}

/* Name the parser class */
%define parser_class_name {Parser}

/* Match the flex prefix */
%define api.prefix {tatumparse_}

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
    #include "tatumparse.hpp"
    #include "tatumparse_lexer_fwd.hpp"
}

%code top {
    #include "tatumparse_lexer.hpp"
    //Bison calls tatumparse_lex() to get the next token.
    //We use the Lexer class as the interface to the lexer, so we
    //re-defined the function to tell Bison how to get the next token.
    static tatumparse::Parser::symbol_type tatumparse_lex(tatumparse::Lexer& lexer) {
        return lexer.next_token();
    }
}

%{

#include <stdio.h>
#include "assert.h"

#include "tatumparse.hpp"
#include "tatumparse_common.hpp"
#include "tatumparse_error.hpp"

using namespace tatumparse;

%}

/* Declare constant */
%token TIMING_GRAPH "timing_graph:"
%token NODE "node:"
%token TYPE "type:"
%token SOURCE "SOURCE"
%token SINK "SINK"
%token IPIN "IPIN"
%token OPIN "OPIN"
%token IN_EDGES "in_edges:"
%token OUT_EDGES "out_edges:"
%token EDGE "edge:"
%token SRC_NODE "src_node:"
%token SINK_NODE "sink_node:"

%token TIMING_CONSTRAINTS "timing_constraints:"
%token CLOCK "CLOCK"
%token CLOCK_SOURCE "CLOCK_SOURCE"
%token INPUT_CONSTRAINT "INPUT_CONSTRAINT"
%token OUTPUT_CONSTRAINT "OUTPUT_CONSTRAINT"
%token SETUP_CONSTRAINT "SETUP_CONSTRAINT"
%token HOLD_CONSTRAINT "HOLD_CONSTRAINT"
%token DOMAIN "domain:"
%token NAME "name:"
%token CONSTRAINT "constraint:"
%token SRC_DOMAIN "src_domain:"
%token SINK_DOMAIN "sink_domain:"

%token DELAY_MODEL "delay_model:"
%token MIN_DELAY "min_delay:"
%token MAX_DELAY "max_delay:"

%token ANALYSIS_RESULTS "analysis_results:"
%token SETUP_DATA "SETUP_DATA"
%token SETUP_CLOCK "SETUP_CLOCK"
%token HOLD_DATA "HOLD_DATA"
%token HOLD_CLOCK "HOLD_CLOCK"
%token ARR "arr:"
%token REQ "req:"

%token EOL "end-of-line"
%token EOF 0 "end-of-file"

/* declare variable tokens */
%token <std::string> STRING
%token <int> INT
%token <float> FLOAT

/* declare types */
%type <int> NodeId
%type <int> SrcNodeId
%type <int> SinkNodeId
%type <int> EdgeId
%type <tatumparse::NodeType> NodeType
%type <std::vector<int>> IntList
%type <std::vector<int>> InEdges
%type <std::vector<int>> OutEdges

%type <int> DomainId
%type <int> SrcDomainId
%type <int> SinkDomainId
%type <float> Constraint
%type <std::string> Name
%type <float> Number
%type <float> MaxDelay
%type <float> MinDelay
%type <float> Req
%type <float> Arr
%type <TagType> TagType

/* Top level rule */
%start tatum_data

%%

tatum_data: /*empty*/ { }
    | tatum_data Graph { callback.finish_graph(); }
    | tatum_data Constraints { callback.finish_constraints(); }
    | tatum_data DelayModel { callback.finish_delay_model(); }
    | tatum_data Results { callback.finish_results(); }
    | tatum_data EOL { /*eat stray EOL */ }

Graph: TIMING_GRAPH EOL { callback.start_graph(); }
     | Graph NodeId EOL NodeType EOL InEdges EOL OutEdges EOL { callback.add_node($2, $4, $6, $8); }
     | Graph EdgeId EOL SrcNodeId EOL SinkNodeId EOL { callback.add_edge($2, $4, $6); }

Constraints: TIMING_CONSTRAINTS EOL { callback.start_constraints(); }
           | Constraints TYPE CLOCK DomainId Name EOL { callback.add_clock_domain($4, $5); }
           | Constraints TYPE CLOCK_SOURCE NodeId DomainId EOL { callback.add_clock_source($4, $5); }
           | Constraints TYPE INPUT_CONSTRAINT NodeId Constraint EOL { callback.add_input_constraint($4, $5); }
           | Constraints TYPE OUTPUT_CONSTRAINT NodeId Constraint EOL { callback.add_output_constraint($4, $5); }
           | Constraints TYPE SETUP_CONSTRAINT SrcDomainId SinkDomainId Constraint EOL { callback.add_setup_constraint($4, $5, $6); }
           | Constraints TYPE HOLD_CONSTRAINT SrcDomainId SinkDomainId Constraint EOL { callback.add_hold_constraint($4, $5, $6); }

DelayModel: DELAY_MODEL EOL { callback.start_delay_model(); }
        | DelayModel EdgeId MinDelay MaxDelay EOL { callback.add_edge_delay($2, $3, $4); }

Results:  ANALYSIS_RESULTS EOL { callback.start_results(); }
        | Results TagType NodeId DomainId Arr Req EOL { callback.add_tag($2, $3, $4, $5, $6); }

Arr: ARR Number { $$ = $2; }
Req: REQ Number { $$ = $2; }

TagType: TYPE SETUP_DATA { $$ = TagType::SETUP_DATA; }
       | TYPE SETUP_CLOCK { $$ = TagType::SETUP_CLOCK; }
       | TYPE HOLD_DATA { $$ = TagType::HOLD_DATA; }
       | TYPE HOLD_CLOCK { $$ = TagType::HOLD_CLOCK; }

MaxDelay: MAX_DELAY Number { $$ = $2; }
MinDelay: MIN_DELAY Number { $$ = $2; }

DomainId: DOMAIN INT { $$ = $2; }
SrcDomainId: SRC_DOMAIN INT { $$ = $2; }
SinkDomainId: SINK_DOMAIN INT { $$ = $2; }
Constraint: CONSTRAINT Number { $$ = $2; }
Name: NAME STRING { $$ = $2; }

NodeType: TYPE SOURCE { $$ = NodeType::SOURCE; }
        | TYPE SINK { $$ = NodeType::SINK; }
        | TYPE IPIN { $$ = NodeType::IPIN; }
        | TYPE OPIN { $$ = NodeType::OPIN; }

NodeId: NODE INT { $$ = $2;}
InEdges: IN_EDGES IntList { $$ = $2; }
OutEdges: OUT_EDGES IntList { $$ = $2; }

EdgeId: EDGE INT { $$ = $2; }
SrcNodeId: SRC_NODE INT { $$ = $2; }
SinkNodeId: SINK_NODE INT { $$ = $2; }

IntList: /*empty*/ { $$ = std::vector<int>(); }
       | IntList INT { $$ = $1; $$.push_back($2); }

Number: INT { $$ = $1; }
      | FLOAT { $$ = $1; }

%%

void tatumparse::Parser::error(const std::string& msg) {
    tatum_error_wrap(callback, lexer.lineno(), lexer.text(), msg.c_str());
}
