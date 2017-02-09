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
#include <cmath>
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
%token CPIN "CPIN"
%token IN_EDGES "in_edges:"
%token OUT_EDGES "out_edges:"
%token EDGE "edge:"
%token SRC_NODE "src_node:"
%token SINK_NODE "sink_node:"
%token DISABLED "disabled:"

%token TRUE "true"
%token FALSE "false"

%token TIMING_CONSTRAINTS "timing_constraints:"
%token CLOCK "CLOCK"
%token CLOCK_SOURCE "CLOCK_SOURCE"
%token CONSTANT_GENERATOR "CONSTANT_GENERATOR"
%token INPUT_CONSTRAINT "INPUT_CONSTRAINT"
%token OUTPUT_CONSTRAINT "OUTPUT_CONSTRAINT"
%token SETUP_CONSTRAINT "SETUP_CONSTRAINT"
%token HOLD_CONSTRAINT "HOLD_CONSTRAINT"
%token DOMAIN "domain:"
%token NAME "name:"
%token CONSTRAINT "constraint:"
%token LAUNCH_DOMAIN "launch_domain:"
%token CAPTURE_DOMAIN "capture_domain:"

%token DELAY_MODEL "delay_model:"
%token MIN_DELAY "min_delay:"
%token MAX_DELAY "max_delay:"
%token SETUP_TIME "setup_time:"
%token HOLD_TIME "hold_time:"

%token ANALYSIS_RESULTS "analysis_results:"
%token SETUP_DATA "SETUP_DATA"
%token SETUP_DATA_ARRIVAL "SETUP_DATA_ARRIVAL"
%token SETUP_DATA_REQUIRED "SETUP_DATA_REQUIRED"
%token SETUP_LAUNCH_CLOCK "SETUP_LAUNCH_CLOCK"
%token SETUP_CAPTURE_CLOCK "SETUP_CAPTURE_CLOCK"
%token SETUP_SLACK "SETUP_SLACK"
%token HOLD_DATA "HOLD_DATA"
%token HOLD_DATA_ARRIVAL "HOLD_DATA_ARRIVAL"
%token HOLD_DATA_REQUIRED "HOLD_DATA_REQUIRED"
%token HOLD_LAUNCH_CLOCK "HOLD_LAUNCH_CLOCK"
%token HOLD_CAPTURE_CLOCK "HOLD_CAPTURE_CLOCK"
%token HOLD_SLACK "HOLD_SLACK"
%token TIME "time:"
%token SLACK "slack:"

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
%type <int> LaunchDomainId
%type <int> CaptureDomainId
%type <float> Constraint
%type <std::string> Name
%type <float> Number
%type <float> MaxDelay
%type <float> MinDelay
%type <float> SetupTime
%type <float> HoldTime
%type <float> Time
%type <float> Slack
%type <TagType> TagType
%type <bool> Disabled

%type <bool> Bool

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
     | Graph EdgeId EOL SrcNodeId EOL SinkNodeId EOL Disabled { callback.add_edge($2, $4, $6, $8); }

Constraints: TIMING_CONSTRAINTS EOL { callback.start_constraints(); }
           | Constraints TYPE CLOCK DomainId Name EOL { callback.add_clock_domain($4, $5); }
           | Constraints TYPE CLOCK_SOURCE NodeId DomainId EOL { callback.add_clock_source($4, $5); }
           | Constraints TYPE CONSTANT_GENERATOR NodeId EOL { callback.add_constant_generator($4); }
           | Constraints TYPE INPUT_CONSTRAINT NodeId DomainId Constraint EOL { callback.add_input_constraint($4, $5, $6); }
           | Constraints TYPE OUTPUT_CONSTRAINT NodeId DomainId Constraint EOL { callback.add_output_constraint($4, $5, $6); }
           | Constraints TYPE SETUP_CONSTRAINT LaunchDomainId CaptureDomainId Constraint EOL { callback.add_setup_constraint($4, $5, $6); }
           | Constraints TYPE HOLD_CONSTRAINT LaunchDomainId CaptureDomainId Constraint EOL { callback.add_hold_constraint($4, $5, $6); }

DelayModel: DELAY_MODEL EOL { callback.start_delay_model(); }
        | DelayModel EdgeId MinDelay MaxDelay EOL { callback.add_edge_delay($2, $3, $4); }
        | DelayModel EdgeId SetupTime HoldTime EOL { callback.add_edge_setup_hold_time($2, $3, $4); }

Results:  ANALYSIS_RESULTS EOL { callback.start_results(); }
        | Results TagType NodeId LaunchDomainId CaptureDomainId Time EOL { callback.add_node_tag($2, $3, $4, $5, NAN); }
        | Results TagType EdgeId LaunchDomainId CaptureDomainId Slack EOL { callback.add_edge_slack($2, $3, $4, $5, NAN); }
        | Results TagType NodeId LaunchDomainId CaptureDomainId Slack EOL { callback.add_node_slack($2, $3, $4, $5, NAN); }

Time: TIME Number { $$ = $2; }
Slack: SLACK Number { $$ = $2; }

TagType: TYPE SETUP_DATA_ARRIVAL { $$ = TagType::SETUP_DATA_ARRIVAL; }
       | TYPE SETUP_DATA_REQUIRED { $$ = TagType::SETUP_DATA_REQUIRED; }
       | TYPE SETUP_LAUNCH_CLOCK { $$ = TagType::SETUP_LAUNCH_CLOCK; }
       | TYPE SETUP_CAPTURE_CLOCK { $$ = TagType::SETUP_CAPTURE_CLOCK; }
       | TYPE SETUP_SLACK { $$ = TagType::SETUP_SLACK; }
       | TYPE HOLD_DATA_ARRIVAL { $$ = TagType::HOLD_DATA_ARRIVAL; }
       | TYPE HOLD_DATA_REQUIRED { $$ = TagType::HOLD_DATA_REQUIRED; }
       | TYPE HOLD_LAUNCH_CLOCK { $$ = TagType::HOLD_LAUNCH_CLOCK; }
       | TYPE HOLD_CAPTURE_CLOCK { $$ = TagType::HOLD_CAPTURE_CLOCK; }
       | TYPE HOLD_SLACK { $$ = TagType::HOLD_SLACK; }

MaxDelay: MAX_DELAY Number { $$ = $2; }
MinDelay: MIN_DELAY Number { $$ = $2; }
SetupTime: SETUP_TIME Number { $$ = $2; }
HoldTime: HOLD_TIME Number { $$ = $2; }

DomainId: DOMAIN INT { $$ = $2; }
LaunchDomainId: LAUNCH_DOMAIN INT { $$ = $2; }
CaptureDomainId: CAPTURE_DOMAIN INT { $$ = $2; }
Constraint: CONSTRAINT Number { $$ = $2; }
Name: NAME STRING { $$ = $2; }

NodeType: TYPE SOURCE { $$ = NodeType::SOURCE; }
        | TYPE SINK { $$ = NodeType::SINK; }
        | TYPE IPIN { $$ = NodeType::IPIN; }
        | TYPE OPIN { $$ = NodeType::OPIN; }
        | TYPE CPIN { $$ = NodeType::CPIN; }

NodeId: NODE INT { $$ = $2;}
InEdges: IN_EDGES IntList { $$ = $2; }
OutEdges: OUT_EDGES IntList { $$ = $2; }

EdgeId: EDGE INT { $$ = $2; }
SrcNodeId: SRC_NODE INT { $$ = $2; }
SinkNodeId: SINK_NODE INT { $$ = $2; }
Disabled: /* Unsipecified*/ { $$ = false; }
        | DISABLED Bool EOL { $$ = $2; }

Bool: TRUE { $$ = true; }
    | FALSE { $$ = false; }

IntList: /*empty*/ { $$ = std::vector<int>(); }
       | IntList INT { $$ = std::move($1); $$.push_back($2); }

Number: INT { $$ = $1; }
      | FLOAT { $$ = $1; }

%%

void tatumparse::Parser::error(const std::string& msg) {
    tatum_error_wrap(callback, lexer.lineno(), lexer.text(), msg.c_str());
}
