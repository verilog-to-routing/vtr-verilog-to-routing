%{
/*
 * Include Files
 */
#include "ctype.h"
#include "string.h"
#include "vpr_timing_graph_common.hpp"
#include "TimingGraph.hpp"
#include "vpr_timing_graph.parse.hpp" //Must be after TimingGraph.hpp for parser parameters

%}

/*
 * Options
 */
%option yylineno
%option noyywrap

/* unistd.h doesn't exist on windows */
%option nounistd

/* isatty() doesn't exist on windows */
%option never-interactive

/* don't use yyunput() */
%option nounput

/*
 * Symbol Definitions
 */
%%
[\n\r]+                         { return EOL; }
[ ]+                            { /* skip white space */ }
\t                              { return TAB; }

num_tnodes:                     { return NUM_TNODES; }
"Node #\tType\t\tipin\tiblk\tDomain\tSkew\tI/O Delay\t# edges\tto_node     Tdel" { return TGRAPH_HEADER; }

num_tnode_levels:               { return NUM_TNODE_LEVELS; }
Level:                          { return LEVEL; }
Num_nodes:                      { return NUM_LEVEL_NODES; }
Nodes:                          { return NODES; }
"Net #\tNet_to_driver_tnode"    { return NET_DRIVER_TNODE_HEADER; }
"Node #\t\tT_arr\t\tT_req"      { return NODE_ARR_REQ_HEADER; }

TN_INPAD_SOURCE                 { return TN_INPAD_SOURCE; }
TN_INPAD_OPIN                   { return TN_INPAD_OPIN; }
TN_OUTPAD_IPIN                  { return TN_OUTPAD_IPIN; }
TN_OUTPAD_SINK                  { return TN_OUTPAD_SINK; }
TN_CB_IPIN                      { return TN_CB_IPIN; }
TN_CB_OPIN                      { return TN_CB_OPIN; }
TN_INTERMEDIATE(_NODE)?         { return TN_INTERMEDIATE_NODE; }
TN_PRIMITIVE_IP(IN)?            { return TN_PRIMITIVE_IPIN; }
TN_PRIMITIVE_OP(IN)?            { return TN_PRIMITIVE_OPIN; }
TN_FF_IPIN                      { return TN_FF_IPIN; }
TN_FF_OPIN                      { return TN_FF_OPIN; }
TN_FF_SINK                      { return TN_FF_SINK; }
TN_FF_SOURCE                    { return TN_FF_SOURCE; }
TN_FF_CLOCK                     { return TN_FF_CLOCK; }
TN_CLOCK_SOURCE                 { return TN_CLOCK_SOURCE; }
TN_CLOCK_OPIN                   { return TN_CLOCK_OPIN; }
TN_CONSTANT_GEN(_SOURCE)?       { return TN_CONSTANT_GEN_SOURCE; }



[-+]?[0-9]+                     { yylval.intVal = atoi(yytext); return INT_NUMBER; }
[-+]?([0-9]*\.?[0-9]+|[0-9]+\.)([eE][-+]?[0-9]+)? { yylval.floatVal = atof(yytext); return FLOAT_NUMBER; }
.                               { /* Literal character to pass unrecognized stuff to parser
                                     (i.e. brackets) */
                                  return (int)(*yytext); 
                                }
<<EOF>>                         { /* If the file has no blank line at the end there will
                                     not be the expected EOL following the last command. 
                                     So first time through, return EOL, and subsequently 
                                     return 0 (which indicated end of file). This ensures
                                     there will always be an EOL provided to the parser. 
                                     However it may also generate a stray EOL if the last
                                     line IS blank - so the parse must handle those correclty. */
                                  static int once = 0; return (once = !once) ? EOL : 0; 
                                }
%%
