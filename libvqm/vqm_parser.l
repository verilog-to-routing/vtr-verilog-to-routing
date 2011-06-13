/* VQM Parser
 *
 * This is the token file for the VQM parser. The VQM parser is based on the
 * VQM file format description provided by Altera's QUIP 4.0 package.
 *
 * Author: Tomasz Czajkowski
 * Date: September 17, 2004
 * NOTES/REVISIONS:
 */

/*
	Hard-coded tokens for Stratix I and II Devices. The parser has been
	expanded to accomodate a general device as defined in proper VQM format,
	but these were preserved for archival purposes. 

stratix_lcell								yylval.value = STRATIX_LCELL; return TOKEN_WYSIWYGID;
stratix_io									yylval.value = STRATIX_IO_CELL; return TOKEN_WYSIWYGID;
stratix_ram_block							yylval.value = STRATIX_RAM_BLOCK; return TOKEN_WYSIWYGID;
stratix_mac_mult							yylval.value = STRATIX_MAC_MULT; return TOKEN_WYSIWYGID;
stratix_mac_out								yylval.value = STRATIX_MAC_OUT; return TOKEN_WYSIWYGID;
stratixii_lcell_comb						yylval.value = STRATIX_II_LCELL_COMB; return TOKEN_WYSIWYGID;
stratixii_lcell_ff							yylval.value = STRATIX_II_LCELL_FF; return TOKEN_WYSIWYGID;
stratixii_io								yylval.value = STRATIX_II_IO_CELL; return TOKEN_WYSIWYGID;
stratixii_ram_block							yylval.value = STRATIX_II_RAM_BLOCK; return TOKEN_WYSIWYGID;
stratixii_mac_mult							yylval.value = STRATIX_II_MAC_MULT; return TOKEN_WYSIWYGID;
stratixii_mac_out							yylval.value = STRATIX_II_MAC_OUT; return TOKEN_WYSIWYGID;
stratixii_dll								yylval.value = STRATIX_II_DLL; return TOKEN_WYSIWYGID;
stratixii_pll								yylval.value = STRATIX_II_PLL; return TOKEN_WYSIWYGID;
stratixii_clkbuf							yylval.value = STRATIX_II_CLOCK_BUF; return TOKEN_WYSIWYGID;
stratixii_clkctrl							yylval.value = STRATIX_II_CLOCK_BUF; return TOKEN_WYSIWYGID;
*/

%{
    extern "C"
    {
        int yylex(void);
    }


/*********************************************************************/
/**** PUT INCLUDE FILES HERE *****************************************/
/*********************************************************************/

#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include "vqm_parser.tab.h"
#include "vqm_stratix.h"
#include "vqm_dll.h"
#include "vqm_common.h"

%}

/*********************************************************************/
/**** PUT OPTIONS HERE ***********************************************/
/*********************************************************************/

%option noyywrap
%option yylineno

/*********************************************************************/
/**** PUT SYMBOL DEFINITIONS HERE ************************************/
/*********************************************************************/

%%
^[ \t]*\/\/[^\n]*\n							/* skip one-line comments */
[ \t]+										/* skip white spaces */
\n											/* skip empty lines */
module										return TOKEN_MODULE;
endmodule									return TOKEN_ENDMODULE;
defparam									return TOKEN_DEFPARAM;
input										yylval.value = PIN_INPUT; return TOKEN_INPUT;
output										yylval.value = PIN_OUTPUT; return TOKEN_OUTPUT;
inout										yylval.value = PIN_INOUT; return TOKEN_INOUT;
wire										yylval.value = PIN_WIRE; return TOKEN_WIRE;
assign										return TOKEN_ASSIGN;
1'b0										yylval.value = 0; return TOKEN_CONST_0;
1'b1										yylval.value = 1; return TOKEN_CONST_1;
1'bz										return TOKEN_CONST_Z;
[a-zA-Z_][a-zA-Z_0-9$]*						yylval.string = (char *)malloc(yyleng+1); strcpy(yylval.string, yytext); return TOKEN_REGULARID;
[-]?[1-9][0-9]*|0+							yylval.value = atoi(yytext); return TOKEN_INTCONSTANT;
\\[^ ^\t]+									yylval.string = (char *)malloc(yyleng); strcpy(yylval.string, yytext+1); return TOKEN_ESCAPEDID;
\"[^\"]*\"									yylval.string = (char *)malloc(yyleng-1); strncpy(yylval.string, yytext+1, yyleng-2); yylval.string[yyleng-2] = 0; return TOKEN_STRING;
[1-9][0-9]*'b[0-9]+							yylval.string = (char *)malloc(yyleng+1); strcpy(yylval.string, yytext); return TOKEN_BITSTRING;
.											return (int)(*yytext);
%%
