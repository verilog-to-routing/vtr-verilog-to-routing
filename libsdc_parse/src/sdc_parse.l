/* recognize tokens for the calculator and print them out */
%{
/*
 * Include Files
 */
#include "ctype.h"
#include "sdc_common.h"
#include "sdc_parse.tab.h"
#include "string.h"
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

/*
 * Symbol Definitions
 */
%%
"#".*                           { /* skip comments */ }
[\n\r]+                         { return EOL; }
[ \t]+                          { /* skip white space */ }

"create_clock"                  { return CMD_CREATE_CLOCK; }
"set_input_delay"               { return CMD_SET_INPUT_DELAY; }
"set_output_delay"              { return CMD_SET_OUTPUT_DELAY; }
"set_clock_groups"              { return CMD_SET_CLOCK_GROUPS; }
"set_false_path"                { return CMD_SET_FALSE_PATH; }
"set_max_delay"                 { return CMD_SET_MAX_DELAY; }
"set_multicycle_path"           { return CMD_SET_MULTICYCLE_PATH; }

"get_ports"                     { return CMD_GET_PORTS; }
"get_clocks"                    { return CMD_GET_CLOCKS; }

"-period"                       { return ARG_PERIOD; }
"-name"                         { return ARG_NAME; }
"-waveform"                     { return ARG_WAVEFORM; }
"-clock"                        { return ARG_CLOCK; }
"-max"                          { return ARG_MAX; }
"-exclusive"                    { return ARG_EXCLUSIVE; }
"-group"                        { return ARG_GROUP; }
"-from"                         { return ARG_FROM; }
"-to"                           { return ARG_TO; }
"-setup"                        { return ARG_SETUP; }

[-+]?[0-9]+                     { yylval.intVal = atoi(yytext); return INT_NUMBER; }
[-+]?([0-9]*\.?[0-9]+|[0-9]+\.) { yylval.floatVal = atof(yytext); return FLOAT_NUMBER; }
\{[ \t]*[^ \t]*[ \t]*\}         { 
                                  /*
                                   * Any continguous sequence of non-white space characters, 
                                   * with optional leading and/or trailing whitespace that falls
                                   * insde curly braces is interpreted as an escaped string.  
                                   * However when we set the yylval, we need to be careful
                                   * to avoid including the curly braces and the leading/trailing
                                   * white-space.
                                   */
                                  size_t len = strlen(yytext);
                                  char* tmp_str = sdc_strndup(yytext+1, len-2); //Duplicate and trim braces

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

                                  //Copy the modified string
                                  yylval.strVal = sdc_strdup(modified_str); 

                                  //Clean-up the temporary string
                                  free(tmp_str);
                                  return ESCAPED_STRING; 
                                }
[-a-zA-Z0-9_~|:*\\/]*           { yylval.strVal = sdc_strdup(yytext); return STRING; }
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
