%{

#include <stdio.h>
#include <cassert>
#include <cstring>
#include <string>
#include <cmath>

#include "vpr_timing_graph_parse_common.h"

int yyerror(const char *msg);
extern int yylex(void);
extern int yylineno;
extern char* yytext;

%}

%union {
    char* strVal;
    double floatVal;
    int intVal;
    pin_blk_t pinBlkVal;
    domain_skew_iodelay_t domainSkewIodelayVal;
    edge_t edgeVal;
}

/* Verbose error reporting */
%error-verbose

/* declare constant tokens */
%token TGRAPH_HEADER          "timing_graph_header"
%token NUM_TNODES             "num_tnodes"

%token TN_INPAD_SOURCE        "TN_INPAD_SOURCE"
%token TN_INPAD_OPIN          "TN_INPAD_OPIN"
%token TN_OUTPAD_IPIN         "TN_OUTPAD_IPIN"
%token TN_OUTPAD_SINK         "TN_OUTPAD_SINK"
%token TN_CB_IPIN             "TN_CB_IPIN"
%token TN_CB_OPIN             "TN_CB_OPIN"
%token TN_INTERMEDIATE_NODE   "TN_INTERMEDIATE_NODE"
%token TN_PRIMITIVE_IPIN      "TN_PRIMITIVE_IPIN"
%token TN_PRIMITIVE_OPIN      "TN_PRIMITIVE_OPIN"
%token TN_FF_IPIN             "TN_FF_IPIN"
%token TN_FF_OPIN             "TN_FF_OPIN"
%token TN_FF_SINK             "TN_FF_SINK"
%token TN_FF_SOURCE           "TN_FF_SOURCE"
%token TN_FF_CLOCK            "TN_FF_CLOCK"
%token TN_CLOCK_SOURCE        "TN_CLOCK_SOURCE"
%token TN_CLOCK_OPIN          "TN_CLOCK_OPIN"
%token TN_CONSTANT_GEN_SOURCE "TN_CONSTANT_GEN_SOURCE"

%token EOL "end-of-line"
%token TAB "tab-character"

/* declare variable tokens */
%token <floatVal> FLOAT_NUMBER
%token <intVal> INT_NUMBER

/* declare types */
%type <floatVal> number
%type <floatVal> float_number
%type <intVal> int_number

%type <intVal> num_tnodes
%type <intVal> tnode
%type <intVal> node_id
%type <intVal> num_out_edges

%type <strVal> tnode_type
%type <strVal> TN_INPAD_SOURCE       
%type <strVal> TN_INPAD_OPIN         
%type <strVal> TN_OUTPAD_IPIN        
%type <strVal> TN_OUTPAD_SINK        
%type <strVal> TN_CB_IPIN            
%type <strVal> TN_CB_OPIN            
%type <strVal> TN_INTERMEDIATE_NODE  
%type <strVal> TN_PRIMITIVE_IPIN     
%type <strVal> TN_PRIMITIVE_OPIN     
%type <strVal> TN_FF_IPIN            
%type <strVal> TN_FF_OPIN            
%type <strVal> TN_FF_SINK            
%type <strVal> TN_FF_SOURCE          
%type <strVal> TN_FF_CLOCK           
%type <strVal> TN_CLOCK_SOURCE       
%type <strVal> TN_CLOCK_OPIN         
%type <strVal> TN_CONSTANT_GEN_SOURCE

%type <pinBlkVal> pin_blk
%type <domainSkewIodelayVal> domain_skew_iodelay
%type <edgeVal> tedge


/* Top level rule */
%start timing_graph 
%%

timing_graph: num_tnodes        {printf("Timing Graph of %d nodes\n", $1);}
    | timing_graph TGRAPH_HEADER{printf("Timing Graph file Header\n");}
    | timing_graph tnode        {}
    | timing_graph EOL          {}
    ;

tnode: node_id tnode_type pin_blk domain_skew_iodelay num_out_edges { printf("Node %d, Type %s, ipin %d, iblk %d, domain %d, skew %f, iodelay %f, edges %d\n", $1, $2, $3.ipin, $3.iblk, $4.domain, $4.skew, $4.iodelay, $5); }
    | tnode tedge {/*printf("edge to %d delay %e\n", $2.to_node, $2.delay);*/}
    ;

node_id: int_number TAB {$$ = $1;}
    ;

pin_blk: int_number TAB int_number TAB { $$.ipin = $1; $$.iblk = $3; }
    | TAB int_number TAB { $$.ipin = -1; $$.iblk = $2; }
    ;

domain_skew_iodelay: int_number TAB number TAB TAB { $$.domain = $1; $$.skew = $3; $$.iodelay = NAN; }
    | int_number TAB TAB number TAB { $$.domain = $1; $$.skew = NAN; $$.iodelay = $4; }
    | TAB TAB TAB TAB { $$.domain = -1; $$.skew = NAN; $$.iodelay = NAN; }
    ;

num_out_edges: int_number {$$ = $1;}
    ;

tedge: TAB int_number TAB number EOL { $$.to_node = $2; $$.delay = $4; }
    | TAB TAB TAB TAB TAB TAB TAB TAB TAB TAB int_number TAB float_number EOL { $$.to_node = $11; $$.delay = $13; }
    ;

tnode_type: TN_INPAD_SOURCE TAB     { $$ = strdup("TN_INPAD_SOURCE"); } 
    | TN_INPAD_OPIN TAB             { $$ = strdup("TN_INPAD_OPIN"); } 
    | TN_OUTPAD_IPIN TAB            { $$ = strdup("TN_OUTPAD_IPIN"); } 
    | TN_OUTPAD_SINK TAB            { $$ = strdup("TN_OUTPAD_SINK"); } 
    | TN_CB_IPIN TAB                { $$ = strdup("TN_CB_IPIN"); } 
    | TN_CB_OPIN TAB                { $$ = strdup("TN_CB_OPIN"); } 
    | TN_INTERMEDIATE_NODE TAB      { $$ = strdup("TN_INTERMEDIATE_NODE"); } 
    | TN_PRIMITIVE_IPIN TAB         { $$ = strdup("TN_PRIMITIVE_IPIN"); } 
    | TN_PRIMITIVE_OPIN TAB         { $$ = strdup("TN_PRIMITIVE_OPIN"); } 
    | TN_FF_IPIN TAB                { $$ = strdup("TN_FF_IPIN"); } 
    | TN_FF_OPIN TAB                { $$ = strdup("TN_FF_OPIN"); } 
    | TN_FF_SINK TAB                { $$ = strdup("TN_FF_SINK"); } 
    | TN_FF_SOURCE TAB              { $$ = strdup("TN_FF_SOURCE"); } 
    | TN_FF_CLOCK TAB               { $$ = strdup("TN_FF_CLOCK"); } 
    | TN_CLOCK_SOURCE TAB           { $$ = strdup("TN_CLOCK_SOURCE"); } 
    | TN_CLOCK_OPIN TAB             { $$ = strdup("TN_CLOCK_OPIN"); } 
    | TN_CONSTANT_GEN_SOURCE TAB    { $$ = strdup("TN_CONSTANT_GEN_SOURCE"); }
    ;

num_tnodes: NUM_TNODES int_number {$$ = $2; }
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
    printf("Line: %d, Text: '%s', Error: %s\n",yylineno, yytext, msg);
    return 1;
}
