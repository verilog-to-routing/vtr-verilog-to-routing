%{
/*
Copyright (c) 2009 Peter Andrew Jamieson (jamieson.peter@gmail.com)

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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "parse_making_ast.h"
 
#define PARSE {printf("here\n");}

#ifndef YYLINENO
int yylineno;
#define YYLINENO yylineno
#else
extern int yylineno;
#endif

void yyerror(const char *str);
int yywrap();
int yyparse();
int yylex(void);

 // RESPONCE in an error
void yyerror(const char *str)
{
	fprintf(stderr,"error in parsing: %s - on line number %d\n",str, yylineno);
	exit(-1);
}
 
// point of continued file reading
int yywrap()
{
	return 1;
}

%}

%union{
	char *id_name;
	char *num_value;
	ast_node_t *node;
} 

%token <id_name> vSYMBOL_ID 
%token <num_value> vNUMBER_ID
%token <num_value> vDELAY_ID
%token vALWAYS vAND vASSIGN vBEGIN vCASE vDEFAULT vDEFINE vELSE vEND vENDCASE 
%token vENDMODULE vIF vINOUT vINPUT vMODULE vNAND vNEGEDGE vNOR vNOT vOR vFOR 
%token vOUTPUT vPARAMETER vPOSEDGE vREG vWIRE vXNOR vXOR vDEFPARAM voANDAND 
%token voOROR voLTE voGTE voSLEFT voSRIGHT voEQUAL voNOTEQUAL voCASEEQUAL 
%token voCASENOTEQUAL voXNOR voNAND voNOR vWHILE vINTEGER 
%token vNOT_SUPPORT
%token '?' ':' '|' '^' '&' '<' '>' '+' '-' '*' '/' '%' '(' ')' '{' '}' '[' ']'

%right '?' ':'  
%left voOROR
%left voANDAND    
%left '|'
%left '^' voXNOR voNOR
%left '&' voNAND
%left voEQUAL voNOTEQUAL voCASEEQUAL voCASENOTEQUAL 
%left voGTE voLTE '<' '>'
%left voSLEFT voSRIGHT 
%left '+' '-'   
%left '*' '/' '%'
%left '~' '!'
%left '{' '}'
%left UOR
%left UAND
%left UNOT
%left UNAND
%left UNOR
%left UXNOR
%left UXOR
%left ULNOT
%left UADD
%left UMINUS

%nonassoc LOWER_THAN_ELSE
%nonassoc vELSE

%type <node> source_text items define module list_of_module_items module_item 
%type <node> parameter_declaration input_declaration output_declaration defparam_declaration
%type <node> inout_declaration variable_list variable net_declaration 
%type <node> continuous_assign gate_declaration gate_instance 
%type <node> module_instantiation module_instance list_of_module_connections 
%type <node> module_connection always statement blocking_assignment 
%type <node> non_blocking_assignment case_item_list case_items seq_block 
%type <node> stmt_list delay_control event_expression_list event_expression 
%type <node> expression primary expression_list module_parameter
%type <node> list_of_module_parameters

%%

// 1 Source Text
source_text: items								{next_parsed_verilog_file($1);}
	;

items: items module								{
											if ($1 != NULL)
											{
												$$ = newList_entry($1, $2);
											}
											else
											{
												$$ = newList(FILE_ITEMS, $2);
											}
										}
	| items define								{$$ = $1;}
	| module								{$$ = newList(FILE_ITEMS, $1);} 
	| define								{$$ = NULL;} 
	;

define: vDEFINE vSYMBOL_ID vNUMBER_ID						{$$ = NULL; newConstant($2, $3, yylineno);}
	;

module: vMODULE vSYMBOL_ID '(' variable_list ')' ';' list_of_module_items vENDMODULE	{$$ = newModule($2, $4, $7, yylineno);}
	| vMODULE vSYMBOL_ID '(' variable_list ',' ')' ';' list_of_module_items vENDMODULE	{$$ = newModule($2, $4, $8, yylineno);}
	| vMODULE vSYMBOL_ID '(' ')' ';' list_of_module_items vENDMODULE		{$$ = newModule($2, NULL, $6, yylineno);}	
	;

list_of_module_items: list_of_module_items module_item				{$$ = newList_entry($1, $2);}
	| module_item								{$$ = newList(MODULE_ITEMS, $1);}
	;

module_item: parameter_declaration						{$$ = $1;}
	| input_declaration							{$$ = $1;}
	| output_declaration							{$$ = $1;}
	| inout_declaration							{$$ = $1;}
	| net_declaration							{$$ = $1;}
	| continuous_assign							{$$ = $1;}
	| gate_declaration							{$$ = $1;}
	| module_instantiation							{$$ = $1;}
	| always								{$$ = $1;}
	| defparam_declaration					{$$ = $1;}
	;

// 2 Declarations	{$$ = NULL;}
parameter_declaration: vPARAMETER variable_list ';'				{$$ = markAndProcessSymbolListWith(PARAMETER, $2);}
	;
	
defparam_declaration: vDEFPARAM variable_list ';'               {$$ = newDefparam(MODULE_PARAMETER_LIST, $2, yylineno);}
	;

					
input_declaration: vINPUT variable_list ';'					{$$ = markAndProcessSymbolListWith(INPUT, $2);}
	;

output_declaration: vOUTPUT variable_list ';'					{$$ = markAndProcessSymbolListWith(OUTPUT, $2);}
	;

inout_declaration: vINOUT variable_list ';'					{$$ = markAndProcessSymbolListWith(INOUT, $2);}
	;

net_declaration: vWIRE variable_list ';'					{$$ = markAndProcessSymbolListWith(WIRE, $2);}
	| vREG variable_list ';'						{$$ = markAndProcessSymbolListWith(REG, $2);}
	| vINTEGER variable_list ';'						{$$ = markAndProcessSymbolListWith(INTEGER, $2);}
	;

variable_list: variable_list ',' variable					{$$ = newList_entry($1, $3);}
	| variable_list '.' variable					{$$ = newList_entry($1, $3);}    //Only for parameter
	| variable								{$$ = newList(VAR_DECLARE_LIST, $1);}
	;

variable: vSYMBOL_ID								{$$ = newVarDeclare($1, NULL, NULL, NULL, NULL, NULL, yylineno);}
	| '[' expression ':' expression ']' vSYMBOL_ID				{$$ = newVarDeclare($6, $2, $4, NULL, NULL, NULL, yylineno);}
	| '[' expression ':' expression ']' vSYMBOL_ID '[' expression ':' expression ']'	{$$ = newVarDeclare($6, $2, $4, $8, $10, NULL, yylineno);}
	| '[' expression ':' expression ']' vSYMBOL_ID '=' primary		{$$ = newVarDeclare($6, $2, $4, NULL, NULL, $8, yylineno);} // ONLY FOR PARAMETER
	| vSYMBOL_ID '=' primary		 				{$$ = newVarDeclare($1, NULL, NULL, NULL, NULL, $3, yylineno);} // ONLY FOR PARAMETER
	;

continuous_assign: vASSIGN blocking_assignment ';'				{$$ = newAssign($2, yylineno);}
	;

// 3 Primitive Instances	{$$ = NULL;}
gate_declaration: vAND gate_instance ';'					{$$ = newGate(BITWISE_AND, $2, yylineno);}
	| vNAND	gate_instance ';'						{$$ = newGate(BITWISE_NAND, $2, yylineno);}
	| vNOR gate_instance ';'						{$$ = newGate(BITWISE_NOR, $2, yylineno);}
	| vNOT gate_instance ';'						{$$ = newGate(BITWISE_NOT, $2, yylineno);}
	| vOR gate_instance ';'							{$$ = newGate(BITWISE_OR, $2, yylineno);}
	| vXNOR gate_instance ';'						{$$ = newGate(BITWISE_XNOR, $2, yylineno);}
	| vXOR gate_instance ';'						{$$ = newGate(BITWISE_XOR, $2, yylineno);}
	;

gate_instance: vSYMBOL_ID '(' expression ',' expression ',' expression ')'	{$$ = newGateInstance($1, $3, $5, $7, yylineno);}
	| '(' expression ',' expression ',' expression ')'			{$$ = newGateInstance(NULL, $2, $4, $6, yylineno);}
	| vSYMBOL_ID '(' expression ',' expression ')'				{$$ = newGateInstance($1, $3, $5, NULL, yylineno);}
	| '(' expression ',' expression ')'					{$$ = newGateInstance(NULL, $2, $4, NULL, yylineno);}
	;

// 4 MOdule Instantiations	{$$ = NULL;}
module_instantiation: vSYMBOL_ID module_instance				{$$ = newModuleInstance($1, $2, yylineno);}
	;

module_instance: vSYMBOL_ID '(' list_of_module_connections ')' ';'		{$$ = newModuleNamedInstance($1, $3, NULL, yylineno);}
	| '#' '(' list_of_module_parameters ')' vSYMBOL_ID '(' list_of_module_connections ')' ';'	{$$ = newModuleNamedInstance($5, $7, $3, yylineno); }
	| vSYMBOL_ID '(' ')' ';'						{$$ = newModuleNamedInstance($1, NULL, NULL, yylineno);}
	| '#' '(' list_of_module_parameters ')' vSYMBOL_ID '(' ')' ';'		{$$ = newModuleNamedInstance($5, NULL, $3, yylineno);}
 	;

list_of_module_connections: list_of_module_connections ',' module_connection	{$$ = newList_entry($1, $3);}
	| module_connection							{$$ = newList(MODULE_CONNECT_LIST, $1);}
	;

module_connection: '.' vSYMBOL_ID '(' expression ')'				{$$ = newModuleConnection($2, $4, yylineno);}
	| expression								{$$ = newModuleConnection(NULL, $1, yylineno);}
	;

list_of_module_parameters: list_of_module_parameters ',' module_parameter	{$$ = newList_entry($1, $3);}
	| module_parameter							{$$ = newList(MODULE_PARAMETER_LIST, $1);}
	;

module_parameter: '.' vSYMBOL_ID '(' expression ')'				{$$ = newModuleParameter($2, $4, yylineno);}
	| expression								{$$ = newModuleParameter(NULL, $1, yylineno);}
	;

// 5 Behavioral Statements	{$$ = NULL;}
always: vALWAYS delay_control statement 					{$$ = newAlways($2, $3, yylineno);}
	;

statement: seq_block								{$$ = $1;}
	| blocking_assignment ';'						{$$ = $1;}
	| non_blocking_assignment ';'						{$$ = $1;}
	| vIF '(' expression ')' statement %prec LOWER_THAN_ELSE 		{$$ = newIf($3, $5, NULL, yylineno);}
	| vIF '(' expression ')' statement vELSE statement 			{$$ = newIf($3, $5, $7, yylineno);}
	| vCASE '(' expression ')' case_item_list vENDCASE			{$$ = newCase($3, $5, yylineno);}
	| vFOR '(' blocking_assignment ';' expression ';' blocking_assignment ')' statement  	{$$ = newFor($3, $5, $7, $9, yylineno);}
	| vWHILE '(' expression ')' statement  					{$$ = newWhile($3, $5, yylineno);}
	| ';'									{$$ = NULL;}
	;
	 
blocking_assignment: primary '=' expression 					{$$ = newBlocking($1, $3, yylineno);}
	| primary '=' vDELAY_ID expression					{$$ = newBlocking($1, $4, yylineno);}
	;

non_blocking_assignment: primary voLTE expression 				{$$ = newNonBlocking($1, $3, yylineno);}
	| primary voLTE vDELAY_ID expression					{$$ = newNonBlocking($1, $4, yylineno);}
	;

case_item_list: case_item_list case_items					{$$ = newList_entry($1, $2);}
	| case_items								{$$ = newList(CASE_LIST, $1);}
	;

case_items: expression ':' statement						{$$ = newCaseItem($1, $3, yylineno);}
	| vDEFAULT ':' statement						{$$ = newDefaultCase($3, yylineno);}
	;

seq_block: vBEGIN stmt_list vEND 						{$$ = $2;}
	;

stmt_list: stmt_list statement 							{$$ = newList_entry($1, $2);}
	| statement								{$$ = newList(BLOCK, $1);}
	;

delay_control: '@' '(' event_expression_list ')' 				{$$ = $3;}
	| '@' '(' '*' ')'							{$$ = NULL;}
	;

// 7 Expressions	{$$ = NULL;}
event_expression_list: event_expression_list vOR event_expression		{$$ = newList_entry($1, $3);}
	| event_expression							{$$ = newList(DELAY_CONTROL, $1);}
	;

event_expression: primary							{$$ = $1;}
	| vPOSEDGE vSYMBOL_ID							{$$ = newPosedgeSymbol($2, yylineno);}
	| vNEGEDGE vSYMBOL_ID							{$$ = newNegedgeSymbol($2, yylineno);}
	;

expression: primary								{$$ = $1;}
	| '+' expression %prec UADD						{$$ = newUnaryOperation(ADD, $2, yylineno);}
	| '-' expression %prec UMINUS						{$$ = newUnaryOperation(MINUS, $2, yylineno);}
	| '~' expression %prec UNOT						{$$ = newUnaryOperation(BITWISE_NOT, $2, yylineno);}
	| '&' expression %prec UAND						{$$ = newUnaryOperation(BITWISE_AND, $2, yylineno);}
	| '|' expression %prec UOR						{$$ = newUnaryOperation(BITWISE_OR, $2, yylineno);}
	| voNAND expression %prec UNAND						{$$ = newUnaryOperation(BITWISE_NAND, $2, yylineno);}
	| voNOR expression %prec UNOR						{$$ = newUnaryOperation(BITWISE_NOR, $2, yylineno);}
	| voXNOR  expression %prec UXNOR					{$$ = newUnaryOperation(BITWISE_XNOR, $2, yylineno);}
	| '!' expression %prec ULNOT						{$$ = newUnaryOperation(LOGICAL_NOT, $2, yylineno);}
	| '^' expression %prec UXOR						{$$ = newUnaryOperation(BITWISE_XOR, $2, yylineno);}
	| expression '^' expression						{$$ = newBinaryOperation(BITWISE_XOR, $1, $3, yylineno);}
	| expression '*' expression						{$$ = newBinaryOperation(MULTIPLY, $1, $3, yylineno);}
	| expression '/' expression						{$$ = newBinaryOperation(DIVIDE, $1, $3, yylineno);}
	| expression '%' expression						{$$ = newBinaryOperation(MODULO, $1, $3, yylineno);}
	| expression '+' expression						{$$ = newBinaryOperation(ADD, $1, $3, yylineno);}
	| expression '-' expression						{$$ = newBinaryOperation(MINUS, $1, $3, yylineno);}
	| expression '&' expression						{$$ = newBinaryOperation(BITWISE_AND, $1, $3, yylineno);}
	| expression '|' expression						{$$ = newBinaryOperation(BITWISE_OR, $1, $3, yylineno);}
	| expression voNAND expression						{$$ = newBinaryOperation(BITWISE_NAND, $1, $3, yylineno);}
	| expression voNOR expression						{$$ = newBinaryOperation(BITWISE_NOR, $1, $3, yylineno);}
	| expression voXNOR expression						{$$ = newBinaryOperation(BITWISE_XNOR, $1, $3, yylineno);}
	| expression '<' expression						{$$ = newBinaryOperation(LT, $1, $3, yylineno);}
	| expression '>' expression						{$$ = newBinaryOperation(GT, $1, $3, yylineno);}
	| expression voSRIGHT expression					{$$ = newBinaryOperation(SR, $1, $3, yylineno);}
	| expression voSLEFT expression						{$$ = newBinaryOperation(SL, $1, $3, yylineno);}
	| expression voEQUAL expression						{$$ = newBinaryOperation(LOGICAL_EQUAL, $1, $3, yylineno);}
	| expression voNOTEQUAL expression					{$$ = newBinaryOperation(NOT_EQUAL, $1, $3, yylineno);}
	| expression voLTE expression						{$$ = newBinaryOperation(LTE, $1, $3, yylineno);}
	| expression voGTE expression						{$$ = newBinaryOperation(GTE, $1, $3, yylineno);}
	| expression voCASEEQUAL expression					{$$ = newBinaryOperation(CASE_EQUAL, $1, $3, yylineno);}
	| expression voCASENOTEQUAL expression					{$$ = newBinaryOperation(CASE_NOT_EQUAL, $1, $3, yylineno);}
	| expression voOROR expression						{$$ = newBinaryOperation(LOGICAL_OR, $1, $3, yylineno);}
	| expression voANDAND expression					{$$ = newBinaryOperation(LOGICAL_AND, $1, $3, yylineno);}
	| expression '?' expression ':' expression				{$$ = newIfQuestion($1, $3, $5, yylineno);}
	| '(' expression ')'							{$$ = $2;}
										/* rule below is strictly unnecessary, causes a bison conflict, 
										but simplifies the AST so that expression_lists with one child 
										(i.e. just an expression) does not create a CONCATENATE parent) */
	| '{' expression '{' expression '}' '}'					{$$ = newListReplicate( $2, $4 ); }
	| '{' expression '{' expression_list '}' '}'				{$$ = newListReplicate( $2, $4 ); }
	;

primary: vNUMBER_ID								{$$ = newNumberNode($1, yylineno);}
	| vSYMBOL_ID								{$$ = newSymbolNode($1, yylineno);}
	| vSYMBOL_ID '[' expression ']'						{$$ = newArrayRef($1, $3, yylineno);}
	| vSYMBOL_ID '[' expression ':' expression ']'				{$$ = newRangeRef($1, $3, $5, yylineno);}
	| '{' expression_list '}' 						{$$ = $2; ($2)->types.concat.num_bit_strings = -1;}
	;
	 
expression_list: expression_list ',' expression					{$$ = newList_entry($1, $3); /* note this will be in order lsb = greatest to msb = 0 in the node child list */}
	| expression								{$$ = newList(CONCATENATE, $1);}
	;

%%
