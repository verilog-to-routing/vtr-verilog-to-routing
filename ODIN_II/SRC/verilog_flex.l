%{
/*
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/ 


#include <stdio.h>
#include <string.h>
#include "types.h"
#include "globals.h"
#include "verilog_bison.h"

/* the define below helps with watching the parser go token by token */
#define MP {if (to_view_parse) {printf("%d %s\n", yylineno, yytext);}}

%}

%s COMMENT

%%

	/*     Keywords */
<INITIAL>"always"		{ MP; return vALWAYS;}
<INITIAL>"and"			{ MP; return vAND;}
<INITIAL>"assign"		{ MP; return vASSIGN;}
<INITIAL>"begin"		{ MP; return vBEGIN;}
<INITIAL>"buf"			{ MP; return vNOT_SUPPORT;}
<INITIAL>"case"			{ MP; return vCASE;}
<INITIAL>"casex"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"casez"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"default"		{ MP; return vDEFAULT;}
<INITIAL>"`define"[^\n]*	{ MP; continue; }
<INITIAL>"defparam"		{ MP; return vDEFPARAM;}
<INITIAL>"disable"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"edge"			{ MP; return vNOT_SUPPORT;}
<INITIAL>"else"			{ MP; return vELSE;}
<INITIAL>"end"			{ MP; return vEND;}
<INITIAL>"endcase"		{ MP; return vENDCASE;}
<INITIAL>"endfunction"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"endmodule"		{ MP; return vENDMODULE;}
<INITIAL>"endspecify"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"endtask"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"for"			{ MP; return vFOR;}
<INITIAL>"function"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"if"			{ MP; return vIF;}
<INITIAL>"initial"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"inout"		{ MP; return vINOUT;}
<INITIAL>"input"		{ MP; return vINPUT;}
<INITIAL>"integer"		{ MP; return vINTEGER;}
<INITIAL>"macromodule"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"module"		{ MP; return vMODULE;}
<INITIAL>"nand"			{ MP; return vNAND;}
<INITIAL>"negedge"		{ MP; return vNEGEDGE;}
<INITIAL>"nor"			{ MP; return vNOR;}
<INITIAL>"not"			{ MP; return vNOT;}
<INITIAL>"or"			{ MP; return vOR;}
<INITIAL>"output"		{ MP; return vOUTPUT;}
<INITIAL>"parameter"		{ MP; return vPARAMETER;}
<INITIAL>"localparam"		{ MP; return vPARAMETER;}
<INITIAL>"posedge"		{ MP; return vPOSEDGE;}
<INITIAL>"reg"			{ MP; return vREG;}
<INITIAL>"scalared"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"specify"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"specparam"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"supply0"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"supply1"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"task"			{ MP; return vNOT_SUPPORT;}
<INITIAL>"tri"			{ MP; return vNOT_SUPPORT;}
<INITIAL>"vectored"		{ MP; return vNOT_SUPPORT;}
<INITIAL>"while"		{ MP; return vWHILE;}
<INITIAL>"wire"			{ MP; return vWIRE;}
<INITIAL>"xnor"			{ MP; return vXNOR;}
<INITIAL>"xor"			{ MP; return vXOR;}

	/* Operators */
<INITIAL>"&&"			{ MP; return voANDAND;}
<INITIAL>"||"			{ MP; return voOROR;}
<INITIAL>"<="			{ MP; return voLTE;}
<INITIAL>">="			{ MP; return voGTE;}
<INITIAL>"<<"			{ MP; return voSLEFT;}
<INITIAL>">>"			{ MP; return voSRIGHT;}
<INITIAL>"=="			{ MP; return voEQUAL;}
<INITIAL>"!="			{ MP; return voNOTEQUAL;}
<INITIAL>"==="			{ MP; return voCASEEQUAL;}
<INITIAL>"!=="			{ MP; return voCASENOTEQUAL;}
<INITIAL>"^~"			{ MP; return voXNOR;}
<INITIAL>"~^"			{ MP; return voXNOR;}
<INITIAL>"~&"			{ MP; return voNAND;}
<INITIAL>"~|"			{ MP; return voNOR;}

<INITIAL>[a-zA-Z][a-zA-Z0-9_]*				{ MP; yylval.id_name = strdup(yytext); return vSYMBOL_ID; }
<INITIAL>`[a-zA-Z][a-zA-Z0-9_]*				{ MP; yylval.id_name = strdup(yytext); return vSYMBOL_ID; }
<INITIAL>[0-9]+'[bhod][a-zA-Z0-9]*			{ MP; yylval.num_value = strdup(yytext); return vNUMBER_ID; }
<INITIAL>[0-9]+						{ MP; yylval.num_value = strdup(yytext); return vNUMBER_ID; }
<INITIAL>#[0-9]+					{ MP; yylval.num_value = strdup(yytext+1); return vDELAY_ID; }

	/* return operators */
<INITIAL>[}{;:\[\],()#=.@&!?<>%|^~+*/-]		{ MP; return yytext[0]; }

	/* general stuff */
<INITIAL>[\f\r\t\b ]+           		{ /* ignore spaces */ continue; }
<INITIAL,COMMENT>"\n"				{/* catch lines */ yylineno++; continue; }

	/* Get the comments */
<INITIAL>"/*"					{ /* catch comments */ MP; BEGIN COMMENT; continue; /* means to go into a state where we eat everything for comments*/ }
<COMMENT>.					{ continue; }
<COMMENT>"*/"					{ BEGIN 0; continue; }
<INITIAL>"//"[^\n]*				{ MP; continue; }  

<*>.|\n     					{ printf("Missing verilog.l rule: Default rule invoked in state %d: \"%s\"\n", YY_START, yytext); }
%%    
