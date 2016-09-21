%{
    /*
     * Include Files
     */
    #include "sdc.hpp"
    #include "sdc_common.hpp"

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

/*
 * Symbol Definitions
 */
%%
"#".*                           { /* skip comments */ }
[\n\r]+                         { return sdcparse::Parser::make_EOL(); }
[ \t]+                          { /* skip white space */ }

"create_clock"                  { return sdcparse::Parser::make_CMD_CREATE_CLOCK(); }
"set_input_delay"               { return sdcparse::Parser::make_CMD_SET_INPUT_DELAY(); }
"set_output_delay"              { return sdcparse::Parser::make_CMD_SET_OUTPUT_DELAY(); }
"set_clock_groups"              { return sdcparse::Parser::make_CMD_SET_CLOCK_GROUPS(); }
"set_false_path"                { return sdcparse::Parser::make_CMD_SET_FALSE_PATH(); }
"set_max_delay"                 { return sdcparse::Parser::make_CMD_SET_MAX_DELAY(); }
"set_multicycle_path"           { return sdcparse::Parser::make_CMD_SET_MULTICYCLE_PATH(); }

"get_ports"                     { return sdcparse::Parser::make_CMD_GET_PORTS(); }
"get_clocks"                    { return sdcparse::Parser::make_CMD_GET_CLOCKS(); }

"-period"                       { return sdcparse::Parser::make_ARG_PERIOD(); }
"-name"                         { return sdcparse::Parser::make_ARG_NAME(); }
"-waveform"                     { return sdcparse::Parser::make_ARG_WAVEFORM(); }
"-clock"                        { return sdcparse::Parser::make_ARG_CLOCK(); }
"-max"                          { return sdcparse::Parser::make_ARG_MAX(); }
"-exclusive"                    { return sdcparse::Parser::make_ARG_EXCLUSIVE(); }
"-group"                        { return sdcparse::Parser::make_ARG_GROUP(); }
"-from"                         { return sdcparse::Parser::make_ARG_FROM(); }
"-to"                           { return sdcparse::Parser::make_ARG_TO(); }
"-setup"                        { return sdcparse::Parser::make_ARG_SETUP(); }

[-+]?[0-9]+                     { return sdcparse::Parser::make_INT_NUMBER(atoi(sdcparse_get_text(yyscanner))); }
[-+]?([0-9]*\.?[0-9]+|[0-9]+\.) { return sdcparse::Parser::make_FLOAT_NUMBER(atof(sdcparse_get_text(yyscanner))); }
\{[ \t]*[^ \t]*[ \t]*\}         { 
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
[-a-zA-Z0-9_~|:*\\/]*           { 
                                  return sdcparse::Parser::make_STRING(sdcparse_get_text(yyscanner)); 
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
                                     line IS blank - so the parse must handle those correclty. */
                                  static bool once; return (once = !once) ? sdcparse::Parser::make_EOL() : sdcparse::Parser::make_EOF();
                                }
.                               { assert(false); }
%%
