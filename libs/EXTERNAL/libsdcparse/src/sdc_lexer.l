%{
    /*
     * Include Files
     */
    #include "sdcparse.hpp"
    #include "sdc_common.hpp"
    #include "sdc_error.hpp"

    #include "sdc_lexer.hpp"

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

/* Avoid unused yyinput function warning */
%option noinput

/* isatty() doesn't exist on windows */
%option never-interactive

/* no default rule to echo unrecongaized tokens to output */
%option nodefault

/*%option bison-bridge*/
%option reentrant

/*
 * Use a prefix to avoid name clashes with other
 * flex lexers
 */
%option prefix="sdcparse_" 

/* Common character classes */
BACK_SLASH [\\]
WS [ \t]
ENDL (\n|\n\r|\r\n)
DIGIT10 [0-9]
NAN ((?i:nan)(\([^{WS}]*\))?)
INF (?i:inf|infinity)
FLOAT_BASE (({DIGIT10}*\.?{DIGIT10}+)|({DIGIT10}+\.))
FLOAT_EXP ([eE][-+]?{DIGIT10}+)
FLOAT_NUMBER ({NAN}|([-+]?({INF}|({FLOAT_BASE}{FLOAT_EXP}?))))
INT_NUMBER ([-+]?{DIGIT10}+)
ALPHA [a-zA-Z_]
SYMBOL [-~|:*\\/]
ID_FIRST ({ALPHA})
ID_MID ({ALPHA}|{DIGIT10}|{SYMBOL})
ID_INDEX (\[{DIGIT10}+\])
ID ({ID_FIRST}{ID_MID}*{ID_INDEX}?)
STR ({ID}|\*{ID}?)
ESCAPED_STR (\{{WS}*[^ \t]*{WS}*\})

/*
 * Symbol Definitions
 */
%%
#.*                             { /* skip comments */ }
{BACK_SLASH}{WS}*{ENDL}         { /* line continuation don't send EOL to parser */ }
{ENDL}                          { return sdcparse::Parser::make_EOL(); }
{WS}+                           { /* skip white space */ }

"create_clock"                  { return sdcparse::Parser::make_CMD_CREATE_CLOCK(); }
"set_input_delay"               { return sdcparse::Parser::make_CMD_SET_INPUT_DELAY(); }
"set_output_delay"              { return sdcparse::Parser::make_CMD_SET_OUTPUT_DELAY(); }
"set_clock_groups"              { return sdcparse::Parser::make_CMD_SET_CLOCK_GROUPS(); }
"set_false_path"                { return sdcparse::Parser::make_CMD_SET_FALSE_PATH(); }
"set_max_delay"                 { return sdcparse::Parser::make_CMD_SET_MAX_DELAY(); }
"set_min_delay"                 { return sdcparse::Parser::make_CMD_SET_MIN_DELAY(); }
"set_multicycle_path"           { return sdcparse::Parser::make_CMD_SET_MULTICYCLE_PATH(); }
"set_clock_uncertainty"         { return sdcparse::Parser::make_CMD_SET_CLOCK_UNCERTAINTY(); }
"set_clock_latency"             { return sdcparse::Parser::make_CMD_SET_CLOCK_LATENCY(); }
"set_disable_timing"            { return sdcparse::Parser::make_CMD_SET_DISABLE_TIMING(); }
"set_timing_derate"             { return sdcparse::Parser::make_CMD_SET_TIMING_DERATE(); }

"get_ports"                     { return sdcparse::Parser::make_CMD_GET_PORTS(); }
"get_clocks"                    { return sdcparse::Parser::make_CMD_GET_CLOCKS(); }
"get_cells"                     { return sdcparse::Parser::make_CMD_GET_CELLS(); }
"get_pins"                      { return sdcparse::Parser::make_CMD_GET_PINS(); }

"-period"                       { return sdcparse::Parser::make_ARG_PERIOD(); }
"-name"                         { return sdcparse::Parser::make_ARG_NAME(); }
"-waveform"                     { return sdcparse::Parser::make_ARG_WAVEFORM(); }
"-clock"                        { return sdcparse::Parser::make_ARG_CLOCK(); }
"-max"                          { return sdcparse::Parser::make_ARG_MAX(); }
"-min"                          { return sdcparse::Parser::make_ARG_MIN(); }
"-exclusive"                    { return sdcparse::Parser::make_ARG_EXCLUSIVE(); }
"-group"                        { return sdcparse::Parser::make_ARG_GROUP(); }
"-from"                         { return sdcparse::Parser::make_ARG_FROM(); }
"-to"                           { return sdcparse::Parser::make_ARG_TO(); }
"-setup"                        { return sdcparse::Parser::make_ARG_SETUP(); }
"-hold"                         { return sdcparse::Parser::make_ARG_HOLD(); }
"-early"                        { return sdcparse::Parser::make_ARG_EARLY(); }
"-late"                         { return sdcparse::Parser::make_ARG_LATE(); }
"-cell_delay"                   { return sdcparse::Parser::make_ARG_CELL_DELAY(); }
"-net_delay"                    { return sdcparse::Parser::make_ARG_NET_DELAY(); }
"-source"                       { return sdcparse::Parser::make_ARG_SOURCE(); }

{INT_NUMBER}                    { return sdcparse::Parser::make_INT_NUMBER(atoi(sdcparse_get_text(yyscanner))); }
{FLOAT_NUMBER}                  { return sdcparse::Parser::make_FLOAT_NUMBER(atof(sdcparse_get_text(yyscanner))); }
{STR}                           { 
                                  return sdcparse::Parser::make_STRING(sdcparse_get_text(yyscanner)); 
                                }
{ESCAPED_STR}                   {
                                  /*
                                   * Any continguous sequence of non-white space characters, 
                                   * with optional leading and/or trailing whitespace that falls
                                   * insde curly braces is interpreted as an escaped string.  
                                   * However when we set the yylval, we need to be careful
                                   * to avoid including the curly braces and the leading/trailing
                                   * white-space.
                                   */
                                  size_t len = strlen(sdcparse_get_text(yyscanner));
                                  char* tmp_str = sdcparse::strndup(sdcparse_get_text(yyscanner)+1, len-2); //Duplicate and trim braces

                                  char* modified_str = tmp_str; //Save the original tmp_str for freeing

                                  //Remove leading whitespace
                                  while(isspace(*modified_str)) modified_str++;

                                  //Find trailing whitespace
                                  char* tmp_ptr = modified_str;
                                  while(!isspace(*tmp_ptr) && *tmp_ptr != '\0') {
                                      tmp_ptr++;
                                  }
                                  
                                  //Remove trailing white space
                                  //
                                  //tmp_ptr is now at the first trailing white space character
                                  //(or the existing null terminator), so we can write a '\0' 
                                  //to terminate the string here.
                                  *tmp_ptr = '\0';

                                  auto ret = sdcparse::Parser::make_ESCAPED_STRING(modified_str);

                                  //Clean-up the temporary string
                                  free(tmp_str);

                                  return ret; 
                                }
\[                              { return sdcparse::Parser::make_LSPAR(); }
\]                              { return sdcparse::Parser::make_RSPAR(); }
\{                              { return sdcparse::Parser::make_LCPAR(); }
\}                              { return sdcparse::Parser::make_RCPAR(); }
<<EOF>>                         { /* If the file has no blank line at the end there will
                                     not be the expected EOL following the last command. 
                                     So first time through, return EOL, and subsequently 
                                     return 0 (which indicated end of file). This ensures
                                     there will always be an EOL provided to the parser. 
                                     However it may also generate a stray EOL if the last
                                     line IS blank - so the parse must handle those correctly. */
                                  static bool once; return (once = !once) ? sdcparse::Parser::make_EOL() : sdcparse::Parser::make_EOF();
                                }
.                               { 
                                  sdc_error_wrap(callback, sdcparse_get_lineno(yyscanner), sdcparse_get_text(yyscanner), "Unrecognized character"); 
                                }
%%
