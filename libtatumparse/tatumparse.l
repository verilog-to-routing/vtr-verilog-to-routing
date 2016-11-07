%{
    /*
     * Include Files
     */
    #include "tatumparse.hpp"
    #include "tatumparse_common.hpp"
    #include "tatumparse_error.hpp"
    #include "tatumparse_lexer.hpp"

%}

/*
 * Options
 */

/* track line numbers*/
%option yylineno 

/* No lexing accross files */
%option noyywrap

/* unistd.h doesn't exist on windows */
%option nounistd

/* Avoid unused yyunput function warning */
%option nounput

/* isatty() doesn't exist on windows */
%option never-interactive

/* no default rule to echo unrecongaized tokens to output */
%option nodefault

%option reentrant

/*
 * Use a prefix to avoid name clashes with other
 * flex lexers
 */
%option prefix="tatumparse_" 

/* Common character classes */
ALPHA_SYMBOL [-a-zA-Z_~|:*/\[\]\.\{\}^+$]
DIGIT [0-9]
ALPHA_NUM_SYMBOL ({ALPHA_SYMBOL}|{DIGIT})
BACK_SLASH [\\]
WS [ \t]
ENDL (\n|\n\r|\r\n)

/*
 * Symbol Definitions
 */
%%
#.*{ENDL}                       { /* ignore comments */ }
^{WS}*{ENDL}                    { /* Ignore blank lines. */ }
{ENDL}                          { 
                                  return tatumparse::Parser::make_EOL(); 
                                }
{WS}+                           { /*skip white space*/ }

timing_graph:                   { return tatumparse::Parser::make_TIMING_GRAPH(); }
node:                           { return tatumparse::Parser::make_NODE(); }
type:                           { return tatumparse::Parser::make_TYPE(); }
SOURCE                          { return tatumparse::Parser::make_SOURCE(); }
SINK                            { return tatumparse::Parser::make_SINK(); }
IPIN                            { return tatumparse::Parser::make_IPIN(); }
OPIN                            { return tatumparse::Parser::make_OPIN(); }
in_edges:                       { return tatumparse::Parser::make_IN_EDGES(); }
out_edges:                      { return tatumparse::Parser::make_OUT_EDGES(); }
edge:                           { return tatumparse::Parser::make_EDGE(); }
src_node:                       { return tatumparse::Parser::make_SRC_NODE(); }
sink_node:                      { return tatumparse::Parser::make_SINK_NODE(); }
timing_constraints:             { return tatumparse::Parser::make_TIMING_CONSTRAINTS(); }

CLOCK                           { return tatumparse::Parser::make_CLOCK(); }
CLOCK_SOURCE                    { return tatumparse::Parser::make_CLOCK_SOURCE(); }
INPUT_CONSTRAINT                { return tatumparse::Parser::make_INPUT_CONSTRAINT(); }
OUTPUT_CONSTRAINT               { return tatumparse::Parser::make_OUTPUT_CONSTRAINT(); }
SETUP_CONSTRAINT                { return tatumparse::Parser::make_SETUP_CONSTRAINT(); }
HOLD_CONSTRAINT                 { return tatumparse::Parser::make_HOLD_CONSTRAINT(); }
domain:                         { return tatumparse::Parser::make_DOMAIN(); }
name:                           { return tatumparse::Parser::make_NAME(); }
constraint:                     { return tatumparse::Parser::make_CONSTRAINT(); }
src_domain:                     { return tatumparse::Parser::make_SRC_DOMAIN(); }
sink_domain:                    { return tatumparse::Parser::make_SINK_DOMAIN(); }

analysis_result:                { return tatumparse::Parser::make_ANALYSIS_RESULTS(); }
SETUP_DATA                      { return tatumparse::Parser::make_SETUP_DATA(); }
SETUP_CLOCK                     { return tatumparse::Parser::make_SETUP_CLOCK(); }
HOLD_DATA                       { return tatumparse::Parser::make_HOLD_DATA(); }
HOLD_CLOCK                      { return tatumparse::Parser::make_HOLD_CLOCK(); }

arr:                            { return tatumparse::Parser::make_ARR(); }
req:                            { return tatumparse::Parser::make_REQ(); }

{DIGIT}+                        { return tatumparse::Parser::make_INT(atoi(tatumparse_get_text(yyscanner))); }
([-+]?({DIGIT}*\.?{DIGIT}+|{DIGIT}+\.)([eE][-+]?{DIGIT}+)?)|(nan) { 
                                    return tatumparse::Parser::make_FLOAT(atof(tatumparse_get_text(yyscanner)));
                                }
\"{ALPHA_NUM_SYMBOL}+\"         { 
                                    //We trim off the surrounding quotes
                                    const char* quoted_text = tatumparse_get_text(yyscanner);
                                    size_t len = strlen(quoted_text);

                                    return tatumparse::Parser::make_STRING(std::string(quoted_text + 1, len-2)); 
                                }

<<EOF>>                         { /* If the file has no blank line at the end there will
                                     not be the expected EOL following the last command. 
                                     So first time through, return EOL, and subsequently 
                                     return 0 (which indicated end of file). This ensures
                                     there will always be an EOL provided to the parser. 
                                     However it may also generate a stray EOL if the last
                                     line IS blank - so the parser must handle those correctly. */
                                  static bool once; return (once = !once) ? tatumparse::Parser::make_EOL() : tatumparse::Parser::make_EOF();
                                }
.                               { tatumparse::tatum_error_wrap(callback, tatumparse_get_lineno(yyscanner), tatumparse_get_text(yyscanner), "unrecognized character"); }
%%
