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

#include "odin_error.h"
#include "odin_types.h"
#include "odin_util.h"
#include "odin_globals.h"
#include "verilog_bison.h"
#include "vtr_util.h"
#include "scope_util.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>
#include <string.h>

#define RECURSIVE_LIMIT 256

#define YY_USER_ACTION {my_yycolumn = current_yycolumn; current_yycolumn = yyleng;}

/* the define below helps with watching the parser go token by token */
#define UNSUPPORTED_TOKEN 	{ delayed_error_message(PARSE_ERROR, my_yycolumn, my_yylineno, current_parse_file, "%s", "Unsuported token"); }

#define _STATE_TOP(str)		{ BEGIN(_state_top(str)); }
#define POP_STATE()			{ _pop_state(); _STATE_TOP("Popped to "); }
#define PUSH_STATE(state)	{ _push_state(state); _STATE_TOP("Pushed to ");}
#define CHANGE_STATE(state) { _pop_state(); _push_state(state); _STATE_TOP("Switched to "); }

struct defines_t 
{
	std::vector<std::string> args;
	std::string body;
};

void MP();
int _state_top(const char *str);
void _pop_state();	
void _push_state(int state);

void push_include(const char *file_name);
bool pop_include();
void pop_buffer_state();
void lex_string(const char * str);

bool ifdef(const char* id);
void new_define_t(const char* id);
void free_define_map();
void add_args_to_define(const char* str);
void append_to_define_body(char c);
void finalize_define();

std::string get_simple_define(const char *str);
std::string get_complex_define();
void next_define_arg();
void define_arg_push_back(char c);
void load_define(const char *str);
void initialize_defaults();

int my_yylineno = 0;
int my_yycolumn = 1;
int current_yycolumn = 1;
std::unordered_map<std::string, defines_t*> defines_map;
defines_t *current_define = NULL;
std::vector<int> current_include_stack;
std::vector<int> state_stack = { 0 };
std::vector<std::string> current_args;
std::string current_define_body;

%}

%x INCLUDE BRACKET_MATCH COMMENT MULTI_LINE_COMMENT DEFINE_BODY SKIP ELIFCONDITION NEG_CONDITION CONDITION DEFINE_REMOVAL VAR_PARSE PARSE_DEFINE DEFINE_ARGS

%option noyywrap
%option nounput
%option noinput
%option never-interactive
%option nounistd
%option nodefault
%option case-sensitive


vBIN [Bb][_]*[ZzXx0-1][ZzXx0-1_]*
vOCT [Oo][_]*[ZzXx0-7][ZzXx0-7_]*
vDEC [Dd][_]*[[:digit:]][[:digit:]_]*
vHEX [Hh][_]*[ZzXx[:xdigit:]][ZzXx[:xdigit:]_]*
vINT [[:digit:]][[:digit:]_]*
vWORD [[:alpha:]_][[:alnum:]_$]*
vSTRING ["][^"]*["]  
vPUNCT 	[\?\:\|\^\&\<\>\-\*\/\%\(\)\{\}\[\]\~\!\;\#\,\.\@\=\+]

%%

	/* preproc helpers */
<VAR_PARSE>{vWORD}					{ MP(); add_args_to_define(yytext); }
<VAR_PARSE>[\,]						{  }
<VAR_PARSE>[\)]						{ CHANGE_STATE(DEFINE_BODY); }
<PARSE_DEFINE>{vWORD}[\(]			{ MP(); new_define_t(yytext); CHANGE_STATE(VAR_PARSE);}
<PARSE_DEFINE>{vWORD}				{ MP(); new_define_t(yytext); CHANGE_STATE(DEFINE_BODY);}
<INITIAL>"`define"					{ MP(); PUSH_STATE(PARSE_DEFINE); }

<DEFINE_REMOVAL>{vWORD}				{ MP(); defines_map.erase(yytext); POP_STATE();}
<INITIAL>"`undef"					{ MP(); PUSH_STATE(DEFINE_REMOVAL); }

<CONDITION>{vWORD}					{ MP(); CHANGE_STATE( ((ifdef(yytext))?  INITIAL: SKIP) ); }
<NEG_CONDITION>{vWORD}				{ MP(); CHANGE_STATE( ((ifdef(yytext))?  SKIP: INITIAL) ); }

	/* since the condition did not hold true, we evaluate these statement */
<SKIP>"`elsif"						{ MP(); CHANGE_STATE(CONDITION); }
<SKIP>"`else"						{ MP(); CHANGE_STATE(INITIAL);}

	/* since the condition held true, we need to skip these */
<INITIAL>"`elsif"					{ MP(); CHANGE_STATE(SKIP); }
<INITIAL>"`else"					{ MP(); CHANGE_STATE(SKIP);}

	/* entry point */
<INITIAL>"`ifdef"					{ MP(); PUSH_STATE(CONDITION);}
<INITIAL>"`ifndef"					{ MP(); PUSH_STATE(NEG_CONDITION);}

	/* exit point */
<INITIAL,SKIP>"`endif"				{ MP(); POP_STATE();}

<INITIAL>"`include"             	{ MP(); PUSH_STATE(INCLUDE); }
<INCLUDE>{vSTRING}      			{ MP(); push_include(yytext); POP_STATE(); }

<INITIAL>"`default_nettype"			{ MP(); return preDEFAULT_NETTYPE;}
<INITIAL>"`resetall"				{ MP(); initialize_defaults();}

	/* unsupported commands, we skip the rest of the line */
<INITIAL>"`timescale"				{ MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`pragma"					{ MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`line"					{ MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`celldefine"				{ MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`endcelldefine"			{ MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`begin_keywords"			{ MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`end_keywords"			{ MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`nounconnected_drive"		{ MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`unconnected_drive"		{ MP(); PUSH_STATE(COMMENT); }


<DEFINE_ARGS>[\)]					{ MP(); POP_STATE(); lex_string(get_complex_define().c_str()); }
<DEFINE_ARGS>[\,]					{ MP(); next_define_arg(); }
<BRACKET_MATCH>[\)]					{ MP(); POP_STATE(); define_arg_push_back(yytext[0]); }
<DEFINE_ARGS,BRACKET_MATCH>[\(]		{ MP(); PUSH_STATE(BRACKET_MATCH); define_arg_push_back(yytext[0]);}
<INITIAL>[\`]{vWORD}[\(]			{ MP(); load_define(yytext); PUSH_STATE(DEFINE_ARGS); }
<INITIAL>[\`]{vWORD}				{ MP(); lex_string(get_simple_define(yytext).c_str());}


	/* Begin Scoped items */
<INITIAL>"begin"					{ MP(); push_scope(); return vBEGIN;}
<INITIAL>"function"					{ MP(); push_scope(); return vFUNCTION;}
<INITIAL>"module"					{ MP(); push_scope(); return vMODULE;}
<INITIAL>"macromodule"				{ MP(); push_scope(); return vMODULE;}
<INITIAL>"task"						{ MP(); push_scope(); return vTASK;}

	/* End Scoped items */
<INITIAL>"end"						{ MP(); return vEND;}
<INITIAL>"endfunction"				{ MP(); return vENDFUNCTION;}
<INITIAL>"endmodule"				{ MP(); return vENDMODULE;}
<INITIAL>"endtask"					{ MP(); return vENDTASK;}

	/*	Keywords	*/
<INITIAL>"always"					{ MP(); return vALWAYS;}
<INITIAL>"and"						{ MP(); return vAND;}
<INITIAL>"assign"					{ MP(); return vASSIGN;}
<INITIAL>"automatic"				{ MP(); return vAUTOMATIC;}
<INITIAL>"case"						{ MP(); return vCASE;}
<INITIAL>"default"					{ MP(); return vDEFAULT;}
<INITIAL>"defparam"					{ MP(); return vDEFPARAM;}
<INITIAL>"else"						{ MP(); return vELSE;}
<INITIAL>"endcase"					{ MP(); return vENDCASE;}
<INITIAL>"endspecify"				{ MP(); return vENDSPECIFY;}
<INITIAL>"endgenerate"				{ MP(); return vENDGENERATE;}
<INITIAL>"for"						{ MP(); return vFOR;}
<INITIAL>"if"						{ MP(); return vIF;}
<INITIAL>"initial"					{ MP(); return vINITIAL;}
<INITIAL>"inout"					{ MP(); return vINOUT;}
<INITIAL>"input"					{ MP(); return vINPUT;}
<INITIAL>"integer"					{ MP(); return vINTEGER;}
<INITIAL>"generate"					{ MP(); return vGENERATE;}
<INITIAL>"genvar"					{ MP(); return vGENVAR;}
<INITIAL>"nand"						{ MP(); return vNAND;}
<INITIAL>"negedge"					{ MP(); return vNEGEDGE;}
<INITIAL>"nor"						{ MP(); return vNOR;}
<INITIAL>"not"						{ MP(); return vNOT;}
<INITIAL>"or"						{ MP(); return vOR;}
<INITIAL>"output"					{ MP(); return vOUTPUT;}
<INITIAL>"parameter"				{ MP(); return vPARAMETER;}
<INITIAL>"localparam"				{ MP(); return vLOCALPARAM;}
<INITIAL>"posedge"					{ MP(); return vPOSEDGE;}
<INITIAL>"signed"					{ MP(); return vSIGNED;}
<INITIAL>"specify"					{ MP(); return vSPECIFY;}
<INITIAL>"while"					{ MP(); return vWHILE;}
<INITIAL>"xnor"						{ MP(); return vXNOR;}
<INITIAL>"xor"						{ MP(); return vXOR;}
<INITIAL>"specparam"				{ MP(); return vSPECPARAM;}

	/* Net types */
<INITIAL>"wire"						{ MP(); return netWIRE;}
<INITIAL>"tri"						{ MP(); return netTRI;}
<INITIAL>"tri0"						{ MP(); return netTRI0;}
<INITIAL>"tri1"						{ MP(); return netTRI1;}
<INITIAL>"wand"						{ MP(); return netWAND;}
<INITIAL>"wor"						{ MP(); return netWOR;}
<INITIAL>"triand"					{ MP(); return netTRIAND;}
<INITIAL>"trior"					{ MP(); return netTRIOR;}
<INITIAL>"trireg"					{ MP(); return netTRIREG;}
<INITIAL>"uwire"					{ MP(); return netUWIRE;}
<INITIAL>"none"						{ MP(); return netNONE;}
<INITIAL>"reg"						{ MP(); return netREG;}

	/*	unsupported Keywords		*/
<INITIAL>"buf"						{ UNSUPPORTED_TOKEN;}
<INITIAL>"casex"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"casez"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"disable"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"edge"						{ UNSUPPORTED_TOKEN;}
<INITIAL>"scalared"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"bufif0"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"bufif1"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"cmos"						{ UNSUPPORTED_TOKEN;}
<INITIAL>"deassign"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"endprimitive"				{ UNSUPPORTED_TOKEN;}
<INITIAL>"endtable"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"event"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"force"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"forever"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"fork"						{ UNSUPPORTED_TOKEN;}
<INITIAL>"highz0"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"highz1"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"join"						{ UNSUPPORTED_TOKEN;}
<INITIAL>"large"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"medium"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"nmos"						{ UNSUPPORTED_TOKEN;}
<INITIAL>"notif0"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"notif1"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"pmos"						{ UNSUPPORTED_TOKEN;}
<INITIAL>"primitive"				{ UNSUPPORTED_TOKEN;}
<INITIAL>"pull0"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"pull1"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"pulldown"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"pullup"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"rcmos"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"release"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"repeat"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"rnmos"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"rpmos"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"rtran"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"rtranif0"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"rtranif1"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"small"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"strong0"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"strong1"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"supply0"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"supply1"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"table"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"time"						{ UNSUPPORTED_TOKEN;}
<INITIAL>"tran"						{ UNSUPPORTED_TOKEN;}
<INITIAL>"tranif0"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"tranif1"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"vectored"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"wait"						{ UNSUPPORTED_TOKEN;}
<INITIAL>"weak0"					{ UNSUPPORTED_TOKEN;}
<INITIAL>"weak1"					{ UNSUPPORTED_TOKEN;}


	/* Operators */	
<INITIAL>"**"						{ MP(); return voPOWER;}
<INITIAL>"&&"						{ MP(); return voANDAND;}
<INITIAL>"||"						{ MP(); return voOROR;}
<INITIAL>"<="						{ MP(); return voLTE;}
<INITIAL>"=>"						{ MP(); return voPAL;}
<INITIAL>">="						{ MP(); return voGTE;}
<INITIAL>"<<"						{ MP(); return voSLEFT;}
<INITIAL>"<<<"						{ MP(); return voSLEFT;}
<INITIAL>">>"						{ MP(); return voSRIGHT;}
<INITIAL>">>>"						{ MP(); return voASRIGHT;}
<INITIAL>"=="						{ MP(); return voEQUAL;}
<INITIAL>"!="						{ MP(); return voNOTEQUAL;}
<INITIAL>"==="						{ MP(); return voCASEEQUAL;}
<INITIAL>"!=="						{ MP(); return voCASENOTEQUAL;}
<INITIAL>"^~"						{ MP(); return voXNOR;}
<INITIAL>"~^"						{ MP(); return voXNOR;}
<INITIAL>"~&"						{ MP(); return voNAND;}
<INITIAL>"~|"						{ MP(); return voNOR;}
<INITIAL>"+:"						{ MP(); return vPLUS_COLON;}
<INITIAL>"-:"						{ MP(); return vMINUS_COLON;}

	/*	unsupported Operators	*/
<INITIAL>"&&&"						{ UNSUPPORTED_TOKEN;}

	/*	C functions	*/	
<INITIAL>"$clog2"					{ MP(); return vCLOG2;}
<INITIAL>"$unsigned"				{ MP(); return voUNSIGNED;}
<INITIAL>"$signed"					{ MP(); return voSIGNED;}

<INITIAL>[\$]{vWORD}				{ MP(); return vCFUNC;}

	/* Delay */
<INITIAL>#{vINT}					{ MP(); yylval.num_value = vtr::strdup(yytext+1); return vDELAY_ID; }

	/* Integers */
<INITIAL>{vINT}						{ MP(); yylval.num_value = vtr::strdup(yytext); return vNUMBER; }

	/* Strings */
<INITIAL>{vSTRING}					{ MP(); yylval.num_value = vtr::strdup(yytext); return vNUMBER; }

	/* Numbers */
<INITIAL>[[:digit:]]*'[sS]?{vBIN}	{ MP(); yylval.num_value = vtr::strdup(yytext); return vNUMBER; }
<INITIAL>[[:digit:]]*'[sS]?{vHEX}	{ MP(); yylval.num_value = vtr::strdup(yytext); return vNUMBER; }
<INITIAL>[[:digit:]]*'[sS]?{vOCT}	{ MP(); yylval.num_value = vtr::strdup(yytext); return vNUMBER; }
<INITIAL>[[:digit:]]*'[sS]?{vDEC}	{ MP(); yylval.num_value = vtr::strdup(yytext); return vNUMBER; }

	/*	operands	*/
<INITIAL>{vWORD}(\.{vWORD})*		{ MP(); yylval.id_name = vtr::strdup(yytext); return vSYMBOL_ID; }

	/* return operators */
<INITIAL>{vPUNCT}					{ MP(); return yytext[0]; }

	/* general stuff */

	/* single line comment */
<*>[\/][\/]							{ 
										int state = _state_top("");
										if (state == DEFINE_BODY)
										{
											/**
											* single line comments will automaticaly continue on if we
											* escape the new line, to prevent issues, we stop processing the macro body
											*/
											finalize_define();
											_pop_state();
										}
										if(state != COMMENT
										&& state != MULTI_LINE_COMMENT)
										{
											PUSH_STATE(COMMENT);
										}
									}
	/* multi line comment */
<*>[\/][\*]							{ 
										int state = _state_top("");
										if(state != COMMENT
										&& state != MULTI_LINE_COMMENT)
										{
											PUSH_STATE(MULTI_LINE_COMMENT);
										}
									}

<MULTI_LINE_COMMENT>[\*][\/]			{ POP_STATE(); }

<*>[[:blank:]]+						{ 	
										int state = _state_top("");
										if(state == DEFINE_BODY)
										{
											append_to_define_body(' ');
										}
									}

<*><<EOF>>							{ if ( ! pop_include() ){ free_define_map(); yyterminate(); } }

	/* skip escapped newline */
<*>\\\r?\n							{ my_yylineno++; my_yycolumn = 1; }

	/* deal with new lines */
<*>\r?\n							{ 
										bool done = false;
										do{
											int state = _state_top("");

											if(state == DEFINE_BODY)
											{
												finalize_define();
											} 

											done = ( state != DEFINE_BODY && state != COMMENT );
											if(!done)
											{
												POP_STATE();
											}

										}while(!done);

										my_yylineno++;
										my_yycolumn = 1;
									}

	/* catch all */
<*>.            					{ 
										MP(); 
										int state = _state_top("");
										if(state == DEFINE_BODY)
										{
											append_to_define_body(yytext[0]);
										}
										else if(state == DEFINE_ARGS
										|| 		state == BRACKET_MATCH)
										{
											define_arg_push_back(yytext[0]);
										} 
									}				

%%

void MP()		
{ 
	if (configuration.print_parse_tokens) 
	{
		printf("%d %s\n", my_yylineno, yytext);
	} 
}

int _state_top(const char *str)		
{ 
	int state = state_stack.back(); 
	if (configuration.print_parse_tokens && strlen(str)) 
	{
		printf("%s state: %s\n", str,
			(state == INCLUDE)? 			"INCLUDE":
			(state == COMMENT)? 			"COMMENT":
			(state == MULTI_LINE_COMMENT)? 	"MULTI_LINE_COMMENT":
			(state == DEFINE_BODY)? 		"DEFINE_BODY":
			(state == SKIP)? 				"SKIP":
			(state == ELIFCONDITION)? 		"ELIFCONDITION":
			(state == NEG_CONDITION)? 		"NEG_CONDITION":
			(state == CONDITION)? 			"CONDITION":
			(state == DEFINE_REMOVAL)? 		"DEFINE_REMOVAL":
			(state == VAR_PARSE)? 			"VAR_PARSE":
			(state == PARSE_DEFINE)? 		"PARSE_DEFINE":
			(state == DEFINE_ARGS)? 		"DEFINE_ARGS":
											"INITIAL"
		);
	} 
	return state;
}

void _pop_state()		
{ 
	state_stack.pop_back(); 
	if(state_stack.empty())
	{
		state_stack.push_back(INITIAL);
	} 
}

void _push_state(int state)	
{ 
	state_stack.push_back(state); 
}

static bool has_current_parse_file()
{
	return (
		current_parse_file < include_file_names.size()
		&& current_parse_file >= 0
	);
}

void lex_string(const char * str)
{

	if (configuration.print_parse_tokens) 
	{
		printf("Processing define %s\n", str);
	} 

	if(has_current_parse_file() 
	&& current_include_stack.back() == current_parse_file)
	{
	 	include_file_names[current_parse_file].second = my_yylineno;
	}

	/* check current depth, prevent too much macro recursion */
	if(current_include_stack.size() > RECURSIVE_LIMIT)
	{
		error_message(PARSE_ERROR, include_file_names[current_parse_file].second, current_parse_file, 
			"Reached upper macro recursion limit of %d", 
			RECURSIVE_LIMIT);
	}
	else if(current_include_stack.size() > (RECURSIVE_LIMIT/2))
	{
		warning_message(PARSE_ERROR, include_file_names[current_parse_file].second, current_parse_file, 
			"Reached halfway to upper macro recursion limit of %d", 
			RECURSIVE_LIMIT);
	}


	current_include_stack.push_back(-1);
	my_yylineno = 0;

	YY_BUFFER_STATE cur = YY_CURRENT_BUFFER;
	YY_BUFFER_STATE yybuff = yy_scan_string(str);
	yy_switch_to_buffer(cur);
	yypush_buffer_state(yybuff);

}

void push_include(const char *file_name)
{

	printf("Adding file %s to parse list\n", file_name);

	std::string tmp(file_name);

	if(tmp[0] == '"')
	{
		tmp.erase(0,1);
	}

	if(tmp.back() == '"')
	{
		tmp.pop_back();
	}

	std::string current_file = "";
	if(has_current_parse_file())
	{
		current_file = include_file_names[current_parse_file].first;
		if(current_include_stack.back() == current_parse_file)
		{
	 		include_file_names[current_parse_file].second = my_yylineno;
		}
	}

	/* we add the path from the current file */
	size_t loc = current_file.find_last_of("/");
	if(loc == std::string::npos)
	{
		current_file = tmp;
	}
	else
	{
		current_file = current_file.substr(0, loc + 1) + tmp;
	}

	yyin = fopen(current_file.c_str(), "r");
	if(yyin == NULL)
	{
		printf("Unable to open %s, trying %s\n", current_file.c_str(), tmp.c_str());
		current_file = tmp;
		yyin = open_file(current_file.c_str(), "r");
	}
	
	my_yylineno = 0;
	current_include_stack.push_back(include_file_names.size());
	include_file_names.push_back({current_file,my_yylineno});

	current_parse_file = current_include_stack.back();
	assert_supported_file_extension(include_file_names.back().first.c_str() , my_yylineno, current_parse_file); 

	YY_BUFFER_STATE yybuff = yy_create_buffer( yyin, YY_BUF_SIZE );
	yypush_buffer_state(yybuff);

}

bool pop_include()
{
	if(has_current_parse_file())
	{
		if(configuration.print_parse_tokens)
		{
			printf("Poping file %s from parse list\n", include_file_names[current_parse_file].first.c_str());
		}
		
		if(yyin)
		{
			fflush(yyin);
			fclose(yyin);
			yyin = NULL;
		}
	}

	if(!current_include_stack.empty())
	{
		current_include_stack.pop_back();
	}

	if(!current_include_stack.empty())
	{
		if(current_include_stack.back() != -1)
		{
			current_parse_file = current_include_stack.back();
		}
	}
	else
	{
		current_parse_file = -1;
	}

	if(has_current_parse_file() && current_parse_file >= 0)
	{
		my_yylineno = include_file_names[current_parse_file].second;

		if(configuration.print_parse_tokens)
		{
			printf("Reading file %s from line %d\n", include_file_names[current_parse_file].first.c_str(), my_yylineno);
		}
	}
	else
	{
		my_yylineno = -1;
	}
	
	yypop_buffer_state(); 
	return ( YY_CURRENT_BUFFER );
}

void initialize_defaults()
{
	default_net_type = WIRE;
	current_define = NULL;
	free_define_map();
}

void new_define_t(const char* id)
{
	std::string tmp(id);
	if(tmp.back() == '(')
	{
		tmp.pop_back();
	}

	defines_t *new_define = new defines_t();
	new_define->args = std::vector<std::string>();
	new_define->body = "";
	
	if(defines_map.find(tmp) != defines_map.end())
	{
		warning_message(PARSE_ERROR, my_yylineno, current_parse_file, "%s is redefined, overwritting its value", id);
		delete defines_map[tmp];	
	}

	defines_map[tmp] = new_define;
	current_define = new_define;
}

void free_define_map()
{
	for(auto kv: defines_map)
	{
		delete kv.second;
	}
}

std::string get_simple_define(const char *str)
{
	load_define(str);
	return get_complex_define();
}

std::string get_complex_define()
{
	if(current_define)
	{
		if( current_args.size() != current_define->args.size())
		{
			error_message(PARSE_ERROR, my_yylineno, current_parse_file, 
				"define is being used with wrong number of arguments, Expected %ld, got %ld", 
				current_args.size(), 
				current_define->args.size());
		}

		for(int i=0; i<current_define->args.size(); i++)
		{
			std::string original_arg = current_define->args[i];
			std::string replacement_arg = current_args[i];

			size_t pos = current_define_body.find( original_arg, 0 );
			while ( pos != std::string::npos )
			{
				current_define_body.erase(pos, original_arg.size());
				current_define_body.insert( pos, replacement_arg );
				pos += replacement_arg.size();
				pos = current_define_body.find( replacement_arg, pos );
			}
		}

		if(configuration.print_parse_tokens )
		{
			printf("DEFINE = %s\n",  current_define_body.c_str());
		}
	}
	return current_define_body;
}

void next_define_arg()
{
	current_args.push_back("");
}

void define_arg_push_back(char c)
{
	if(current_args.empty())
	{
		next_define_arg();
	}

	current_args.back().push_back(c);
}

void load_define(const char *str)
{
	std::string tmp(str);

	current_define = NULL;
	current_define_body = "";
	current_args = std::vector<std::string>();

	// compiler specific macros
	if(tmp == "`__LINE__") 
	{
		current_define_body = std::to_string(my_yylineno);
	}
	else if (tmp == "`__FILE__")
	{
		current_define_body = std::string(configuration.list_of_file_names[current_parse_file]);
	}
	else
	{
		if(tmp[0] == '`')
		{
			tmp.erase(0,1);
		}

		if(tmp.back() == '(')
		{
			tmp.pop_back();
		}

		auto itter = defines_map.find(tmp);
		if(itter == defines_map.end())
		{
			warning_message(PARSE_ERROR, my_yylineno, current_parse_file, 
				"%s define cannot be found, replacing with empty string and continuing synthesis", 
				tmp.c_str());
		}
		else
		{
			current_define = itter->second;
			current_define_body = current_define->body;
		}
	}
}

void append_to_define_body(char c)
{
	current_define->body.push_back(c);
}

void add_args_to_define(const char* str)
{
	std::string tmp(str);
	for(int i=0; i < current_define->args.size(); i++)
	{
		if(tmp == current_define->args[i])
		{
			error_message(PARSE_ERROR, my_yylineno, current_parse_file, 
				"%s","define has two argument with same name");
		}
	}

	current_define->args.push_back(std::string(str));
}

void finalize_define()
{
	current_define = NULL;
}

bool ifdef(const char* id)
{
	return ( defines_map.find(std::string(id)) != defines_map.end() );
}
