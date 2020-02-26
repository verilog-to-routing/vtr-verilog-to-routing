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
#include "odin_types.h"
#include "odin_globals.h"
#include "odin_error.h"
#include "ast_util.h"
#include "parse_making_ast.h"
#include "vtr_memory.h"
#include "scope_util.h"

extern int my_yylineno;

void yyerror(const char *str){	delayed_error_message(PARSE_ERROR, 0 , my_yylineno, current_parse_file,"error in parsing: (%s)\n",str);}
int yywrap(){	return 1;}
int yylex(void);

%}

%locations

%union{
	char *id_name;
	char *num_value;
	ast_node_t *node;
	ids id;
}
%token <id_name> vSYMBOL_ID
%token <num_value> vNUMBER vDELAY_ID
%token vALWAYS vAUTOMATIC vINITIAL vSPECIFY vAND vASSIGN vBEGIN vCASE vDEFAULT vELSE vEND vENDCASE
%token vENDMODULE vENDSPECIFY vENDGENERATE vENDFUNCTION vENDTASK vIF vINOUT vINPUT vMODULE vGENERATE vFUNCTION vTASK
%token vOUTPUT vPARAMETER vLOCALPARAM vPOSEDGE vXNOR vXOR vDEFPARAM voANDAND vNAND vNEGEDGE vNOR vNOT vOR vFOR vBUF
%token voOROR voLTE voGTE voPAL voSLEFT voSRIGHT voASRIGHT voEQUAL voNOTEQUAL voCASEEQUAL
%token voCASENOTEQUAL voXNOR voNAND voNOR vWHILE vINTEGER vCLOG2 vGENVAR
%token vPLUS_COLON vMINUS_COLON vSPECPARAM voUNSIGNED voSIGNED vSIGNED vCFUNC
%token '?' ':' '|' '^' '&' '<' '>' '+' '-' '*' '/' '%' '(' ')' '{' '}' '[' ']' '~' '!' ';' '#' ',' '.' '@' '='

	/* preprocessor directives */
%token preDEFAULT_NETTYPE

	/* net types */
%token netWIRE netTRI netTRI0 netTRI1 netWAND netWOR netTRIAND netTRIOR netTRIREG netUWIRE netNONE netREG

%right '?' ':'
%left voOROR
%left voANDAND
%left '|'
%left '^' voXNOR voNOR
%left '&' voNAND
%left voEQUAL voNOTEQUAL voCASEEQUAL voCASENOTEQUAL
%left voGTE voLTE '<' '>'
%left voSLEFT voSRIGHT voASRIGHT
%left '+' '-'
%left '*' '/' '%'
%right voPOWER
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

%type <id>	 wire_types reg_types net_types net_direction
%type <node> source_text item items module list_of_module_items list_of_task_items list_of_function_items module_item function_item task_item module_parameters module_ports
%type <node> list_of_parameter_declaration parameter_declaration task_parameter_declaration specparam_declaration list_of_port_declaration port_declaration
%type <node> defparam_declaration defparam_variable_list defparam_variable function_declaration function_port_list list_of_function_inputs function_input_declaration 
%type <node> task_declaration task_input_declaration task_output_declaration task_inout_declaration
%type <node> io_declaration variable_list function_return list_of_function_return_variable function_return_variable function_integer_declaration
%type <node> integer_type_variable_list variable integer_type_variable
%type <node> net_declaration integer_declaration function_instantiation task_instantiation genvar_declaration
%type <node> procedural_continuous_assignment multiple_inputs_gate_instance list_of_single_input_gate_declaration_instance
%type <node> list_of_multiple_inputs_gate_declaration_instance list_of_module_instance
%type <node> gate_declaration single_input_gate_instance
%type <node> module_instantiation module_instance function_instance task_instance list_of_module_connections list_of_function_connections list_of_task_connections
%type <node> list_of_multiple_inputs_gate_connections
%type <node> module_connection tf_connection always statement function_statement function_statement_items task_statement blocking_assignment
%type <node> non_blocking_assignment conditional_statement function_conditional_statement case_statement function_case_statement case_item_list function_case_item_list case_items function_case_items seq_block function_seq_block
%type <node> stmt_list function_stmt_list delay_control event_expression_list event_expression loop_statement function_loop_statement
%type <node> expression primary expression_list module_parameter
%type <node> list_of_module_parameters localparam_declaration
%type <node> specify_block list_of_specify_items
%type <node> c_function_expression_list c_function
%type <node> initial_block list_of_blocking_assignment
%type <node> list_of_generate_block_items generate_item generate_block_item generate loop_generate_construct if_generate_construct 
%type <node> case_generate_construct case_generate_item_list case_generate_items generate_block generate_localparam_declaration generate_defparam_declaration

%%

source_text:
	items	{ next_parsed_verilog_file($1);}
	;

items:
	items item 									{ ($2 == NULL)? $$ = $1 : ( ($1 == NULL)? $$ = newList(FILE_ITEMS, $2, my_yylineno): newList_entry($1, $2) ); }
	| item 										{ ($1 == NULL)? $$ = $1 : $$ = newList(FILE_ITEMS, $1, my_yylineno); }
	;

item:
	module															{ $$ = $1; }
	| preDEFAULT_NETTYPE wire_types									{ default_net_type = $2; $$ = NULL; }
	;

module:
	vMODULE vSYMBOL_ID module_parameters module_ports ';' list_of_module_items vENDMODULE			{$$ = newModule($2, $3, $4, $6, my_yylineno);}
	| vMODULE vSYMBOL_ID module_parameters ';' list_of_module_items vENDMODULE						{$$ = newModule($2, $3, NULL, $5, my_yylineno);}
	| vMODULE vSYMBOL_ID module_ports ';' list_of_module_items vENDMODULE							{$$ = newModule($2, NULL, $3, $5, my_yylineno);}
	| vMODULE vSYMBOL_ID ';' list_of_module_items vENDMODULE										{$$ = newModule($2, NULL, NULL, $4, my_yylineno);}
	;

module_parameters:
	'#' '(' list_of_parameter_declaration ')'			{$$ = $3;}
	| '#' '(' list_of_parameter_declaration ',' ')'		{$$ = $3;}
	| '#' '(' ')'										{$$ = NULL;}
	;

list_of_parameter_declaration:
	list_of_parameter_declaration ',' vPARAMETER vSIGNED variable	{$$ = newList_entry($1, markAndProcessParameterWith(PARAMETER, $5, true));}
	| list_of_parameter_declaration ',' vPARAMETER variable			{$$ = newList_entry($1, markAndProcessParameterWith(PARAMETER, $4, false));}
	| list_of_parameter_declaration ',' variable					{$$ = newList_entry($1, markAndProcessParameterWith(PARAMETER, $3, false));} 
	| vPARAMETER vSIGNED variable									{$$ = newList(VAR_DECLARE_LIST, markAndProcessParameterWith(PARAMETER, $3, true), my_yylineno);}
	| vPARAMETER variable											{$$ = newList(VAR_DECLARE_LIST, markAndProcessParameterWith(PARAMETER, $2, false), my_yylineno);}
	;

module_ports:
	'(' list_of_port_declaration ')'					{$$ = $2;}
	| '(' list_of_port_declaration ',' ')'				{$$ = $2;}
	| '(' ')'											{$$ = NULL;}
	;

list_of_port_declaration:
	list_of_port_declaration ',' port_declaration		{$$ = newList_entry($1, $3);}
	| port_declaration									{$$ = newList(VAR_DECLARE_LIST, $1, my_yylineno);}
	;
	
port_declaration:
	net_direction net_types vSIGNED variable			{$$ = markAndProcessPortWith(MODULE, $1, $2, $4, true);}
	| net_direction net_types variable					{$$ = markAndProcessPortWith(MODULE, $1, $2, $3, false);}
	| net_direction variable							{$$ = markAndProcessPortWith(MODULE, $1, NO_ID, $2, false);}
	| net_direction vINTEGER integer_type_variable		{$$ = markAndProcessPortWith(MODULE, $1, INTEGER, $3, true);}
	| net_direction vSIGNED variable					{$$ = markAndProcessPortWith(MODULE, $1, NO_ID, $3, true);}
	| variable											{$$ = $1;}
	;

list_of_module_items:
	list_of_module_items module_item	{$$ = newList_entry($1, $2);}
	| module_item						{$$ = newList(MODULE_ITEMS, $1, my_yylineno);}
	;

module_item:
	parameter_declaration	{$$ = $1;}
	| io_declaration	 	{$$ = $1;}
	| specify_block			{$$ = $1;}
	| generate_item			{$$ = $1;}
	| generate				{$$ = $1;}
	| error ';'				{$$ = NULL;}
	;

list_of_generate_block_items:
	list_of_generate_block_items generate_block_item	{$$ = newList_entry($1, $2);}
	| generate_block_item								{$$ = newList(BLOCK, $1, my_yylineno);}
	;

generate_item:
	localparam_declaration				{$$ = $1;}
	| net_declaration 					{$$ = $1;}
	| genvar_declaration				{$$ = $1;}
	| integer_declaration 				{$$ = $1;}
	| procedural_continuous_assignment	{$$ = $1;}
	| gate_declaration					{$$ = $1;}
	| module_instantiation				{$$ = $1;}
	| function_declaration				{$$ = $1;}
	| task_declaration					{$$ = $1;}
	| initial_block						{$$ = $1;}
	| always							{$$ = $1;}
	| defparam_declaration				{$$ = $1;}
	| c_function ';'					{$$ = $1;}
	| if_generate_construct				{$$ = $1;}
	| case_generate_construct			{$$ = $1;}
	| loop_generate_construct			{$$ = $1;}
	;

generate_block_item:
	generate_localparam_declaration			{$$ = $1;}
	| net_declaration 						{$$ = $1;}
	| genvar_declaration					{$$ = $1;}
	| integer_declaration 					{$$ = $1;}
	| procedural_continuous_assignment		{$$ = $1;}
	| gate_declaration						{$$ = $1;}
	| module_instantiation					{$$ = $1;}
	| function_declaration					{$$ = $1;}
	| task_declaration						{$$ = $1;}
	| initial_block							{$$ = $1;}
	| always								{$$ = $1;}
	| generate_defparam_declaration			{$$ = $1;}
	| c_function ';'						{$$ = $1;}
	| if_generate_construct					{$$ = $1;}
	| case_generate_construct				{$$ = $1;}
	| loop_generate_construct				{$$ = $1;}
	;

function_declaration:
	vFUNCTION function_return ';' list_of_function_items vENDFUNCTION									{$$ = newFunction($2, NULL, $4, my_yylineno, false);}
	| vFUNCTION vAUTOMATIC function_return ';' list_of_function_items vENDFUNCTION						{$$ = newFunction($3, NULL, $5, my_yylineno, true);}
	| vFUNCTION function_return function_port_list ';' list_of_function_items vENDFUNCTION				{$$ = newFunction($2, $3, $5, my_yylineno, false);}
	| vFUNCTION vAUTOMATIC function_return function_port_list ';' list_of_function_items vENDFUNCTION	{$$ = newFunction($3, $4, $6, my_yylineno, true);}
	;

task_declaration:
	vTASK vSYMBOL_ID ';' list_of_task_items vENDTASK													{$$ = newTask($2, NULL, $4, my_yylineno, false);}
	| vTASK vSYMBOL_ID module_ports ';' list_of_task_items vENDTASK										{$$ = newTask($2, $3, $5, my_yylineno, false);}
	| vTASK vAUTOMATIC vSYMBOL_ID ';' list_of_task_items vENDTASK										{$$ = newTask($3, NULL, $5, my_yylineno, true);}
	| vTASK vAUTOMATIC vSYMBOL_ID module_ports ';' list_of_task_items vENDTASK							{$$ = newTask($3, $4, $6, my_yylineno, true);}
	;

initial_block:
	vINITIAL statement	{$$ = newInitial($2, my_yylineno); }
	;

	/* Specify is unsynthesizable, we discard everything within */
specify_block:
	vSPECIFY list_of_specify_items vENDSPECIFY	{$$ = $2;}
	;

list_of_specify_items:
	list_of_specify_items specparam_declaration			{$$ = $2;}
	| specparam_declaration								{$$ = $1;}
	;

specparam_declaration:
	'(' primary voPAL expression ')' '=' primary ';'	{free_whole_tree($2); free_whole_tree($4); free_whole_tree($7); $$ = NULL;}
	| vSPECPARAM variable_list ';'						{free_whole_tree($2); $$ = NULL;}
	;
	
list_of_function_items:
	list_of_function_items function_item	{$$ = newList_entry($1, $2);}
	| function_item				{$$ = newList(FUNCTION_ITEMS, $1, my_yylineno);}
	;

task_input_declaration:
	vINPUT net_declaration					{$$ = markAndProcessSymbolListWith(TASK,INPUT, $2, false);}
	| vINPUT integer_declaration			{$$ = markAndProcessSymbolListWith(TASK,INPUT, $2, true);}
	| vINPUT vSIGNED variable_list ';'		{$$ = markAndProcessSymbolListWith(TASK,INPUT, $3, true);}
	| vINPUT variable_list ';'				{$$ = markAndProcessSymbolListWith(TASK,INPUT, $2, false);}
	;

task_output_declaration:
	vOUTPUT net_declaration					{$$ = markAndProcessSymbolListWith(TASK,OUTPUT, $2, false);}
	| vOUTPUT integer_declaration			{$$ = markAndProcessSymbolListWith(TASK,OUTPUT, $2, true);}
	| vOUTPUT vSIGNED variable_list ';'		{$$ = markAndProcessSymbolListWith(TASK,OUTPUT, $3, true);}
	| vOUTPUT variable_list ';'				{$$ = markAndProcessSymbolListWith(TASK,OUTPUT, $2, false);}
	;

task_inout_declaration:
	vINOUT net_declaration					{$$ = markAndProcessSymbolListWith(TASK,INOUT, $2, false);}
	| vINOUT integer_declaration			{$$ = markAndProcessSymbolListWith(TASK,INOUT, $2, true);}
	| vINOUT vSIGNED variable_list ';'		{$$ = markAndProcessSymbolListWith(TASK,INOUT, $3, true);}
	| vINOUT variable_list ';'				{$$ = markAndProcessSymbolListWith(TASK,INOUT, $2, false);}
	;

list_of_task_items:
	list_of_task_items task_item 	{$$ = newList_entry($1, $2); }
	| task_item						{$$ = newList(TASK_ITEMS, $1, my_yylineno); }
	;

function_item: 
	parameter_declaration			{$$ = $1;}
	| function_input_declaration	{$$ = $1;}
	| net_declaration 				{$$ = $1;}
	| function_integer_declaration ';' 			{$$ = $1;}
	| defparam_declaration			{$$ = $1;}
	| function_statement			{$$ = $1;}
	| error ';'						{$$ = NULL;}
	;

task_item:
	task_parameter_declaration			{$$ = $1;}
	| task_input_declaration			{$$ = $1;}
	| task_output_declaration			{$$ = $1;}
	| task_inout_declaration			{$$ = $1;}
	| net_declaration 					{$$ = $1;}
	| integer_declaration 				{$$ = $1;}
	| defparam_declaration				{$$ = $1;}
	| task_statement					{$$ = $1;} 
	;

function_input_declaration:
	vINPUT vSIGNED variable_list ';'	{$$ = markAndProcessSymbolListWith(FUNCTION, INPUT, $3, true);}
	| vINPUT variable_list ';'			{$$ = markAndProcessSymbolListWith(FUNCTION, INPUT, $2, false);}
	| vINPUT function_integer_declaration ';' {$$ = markAndProcessSymbolListWith(FUNCTION, INPUT, $2, false);}
	;

parameter_declaration:
	vPARAMETER vSIGNED variable_list ';'	{$$ = markAndProcessSymbolListWith(MODULE,PARAMETER, $3, true);}
	| vPARAMETER variable_list ';'			{$$ = markAndProcessSymbolListWith(MODULE,PARAMETER, $2, false);}
	;

task_parameter_declaration:
	vPARAMETER vSIGNED variable_list ';'	{$$ = markAndProcessSymbolListWith(TASK,PARAMETER, $3, true);}
	| vPARAMETER variable_list ';'			{$$ = markAndProcessSymbolListWith(TASK,PARAMETER, $2, false);}
	;

localparam_declaration:
	vLOCALPARAM vSIGNED variable_list ';'	{$$ = markAndProcessSymbolListWith(MODULE,LOCALPARAM, $3, true);}
	| vLOCALPARAM variable_list ';'			{$$ = markAndProcessSymbolListWith(MODULE,LOCALPARAM, $2, false);}
	;

generate_localparam_declaration:
	vLOCALPARAM vSIGNED variable_list ';'	{$$ = markAndProcessSymbolListWith(BLOCK,LOCALPARAM, $3, true);}
	| vLOCALPARAM variable_list ';'			{$$ = markAndProcessSymbolListWith(BLOCK,LOCALPARAM, $2, false);}
	;

defparam_declaration:
	vDEFPARAM defparam_variable_list ';'	{$$ = newDefparam(MODULE, $2, my_yylineno);}
	;

generate_defparam_declaration:
	vDEFPARAM defparam_variable_list ';'	{$$ = newDefparam(BLOCK, $2, my_yylineno);}
	;

io_declaration:
	net_direction net_declaration					{$$ = markAndProcessSymbolListWith(MODULE,$1, $2, false);}
	| net_direction integer_declaration			{$$ = markAndProcessSymbolListWith(MODULE,$1, $2, true);}
	| net_direction vSIGNED variable_list ';'		{$$ = markAndProcessSymbolListWith(MODULE,$1, $3, true);}
	| net_direction variable_list ';'				{$$ = markAndProcessSymbolListWith(MODULE,$1, $2, false);}
	;

net_declaration:
	net_types vSIGNED variable_list ';'			{$$ = markAndProcessSymbolListWith(MODULE, $1, $3, true);}
	| net_types variable_list ';'				{$$ = markAndProcessSymbolListWith(MODULE, $1, $2, false);}
	;

integer_declaration:
	vINTEGER integer_type_variable_list ';'	{$$ = markAndProcessSymbolListWith(MODULE,INTEGER, $2, true);}
	;

genvar_declaration:
	vGENVAR integer_type_variable_list ';'	{$$ = markAndProcessSymbolListWith(MODULE,GENVAR, $2, true);}
	;

function_return:
	list_of_function_return_variable							{$$ = markAndProcessSymbolListWith(FUNCTION, OUTPUT, $1, false);}
	| vSIGNED list_of_function_return_variable				{$$ = markAndProcessSymbolListWith(FUNCTION, OUTPUT, $2, true);}
	| function_integer_declaration							{$$ = markAndProcessSymbolListWith(FUNCTION, OUTPUT, $1, false);}		
	;

list_of_function_return_variable:
	function_return_variable			{$$ = newList(VAR_DECLARE_LIST, $1, my_yylineno);}
	;

function_return_variable:
	vSYMBOL_ID											{$$ = newVarDeclare($1, NULL, NULL, NULL, NULL, NULL, my_yylineno);}								
	| '[' expression ':' expression ']' vSYMBOL_ID		{$$ = newVarDeclare($6, $2, $4, NULL, NULL, NULL, my_yylineno);}
	;

function_integer_declaration:
	vINTEGER integer_type_variable_list 				{$$ = markAndProcessSymbolListWith(FUNCTION, INTEGER, $2, true);}
	;

function_port_list:
	'(' list_of_function_inputs ')'					{$$ = $2;}
	| '(' list_of_function_inputs ',' ')'				{$$ = $2;}
	| '(' ')'											{$$ = NULL;}
	;

list_of_function_inputs:
	list_of_function_inputs ',' function_input_declaration		{$$ = newList_entry($1, $3);}
	| function_input_declaration									{$$ = newList(VAR_DECLARE_LIST, $1, my_yylineno);}
	;
	
variable_list:
	variable_list ',' variable						{$$ = newList_entry($1, $3);}
	| variable										{$$ = newList(VAR_DECLARE_LIST, $1, my_yylineno);}
	;

defparam_variable_list:
	defparam_variable_list ',' defparam_variable    {$$ = newList_entry($1, $3);}
	| defparam_variable								{$$ = newList(VAR_DECLARE_LIST, $1, my_yylineno);}
	;

integer_type_variable_list:
	integer_type_variable_list ',' integer_type_variable	{$$ = newList_entry($1, $3);}
	| integer_type_variable									{$$ = newList(VAR_DECLARE_LIST, $1, my_yylineno);}
	;

variable:
	vSYMBOL_ID														{$$ = newVarDeclare($1, NULL, NULL, NULL, NULL, NULL, my_yylineno);}
	| '[' expression ':' expression ']' vSYMBOL_ID										{$$ = newVarDeclare($6, $2, $4, NULL, NULL, NULL, my_yylineno);}
	| '[' expression ':' expression ']' vSYMBOL_ID '[' expression ':' expression ']'					{$$ = newVarDeclare($6, $2, $4, $8, $10, NULL, my_yylineno);}
	| '[' expression ':' expression ']' vSYMBOL_ID '[' expression ':' expression ']' '[' expression ':' expression ']'	{$$ = newVarDeclare2D($6, $2, $4, $8, $10,$13,$15, NULL, my_yylineno);}
	| '[' expression ':' expression ']' vSYMBOL_ID '=' expression								{$$ = newVarDeclare($6, $2, $4, NULL, NULL, $8, my_yylineno);}
	| vSYMBOL_ID '=' expression												{$$ = newVarDeclare($1, NULL, NULL, NULL, NULL, $3, my_yylineno);}
	;

defparam_variable:
	vSYMBOL_ID '=' expression				{$$ = newDefparamVarDeclare($1, NULL, NULL, NULL, NULL, $3, my_yylineno);}
	;
	
integer_type_variable:
	vSYMBOL_ID					{$$ = newIntegerTypeVarDeclare($1, NULL, NULL, NULL, NULL, NULL, my_yylineno);}
	| vSYMBOL_ID '[' expression ':' expression ']'	{$$ = newIntegerTypeVarDeclare($1, NULL, NULL, $3, $5, NULL, my_yylineno);}
	| vSYMBOL_ID '=' primary			{$$ = newIntegerTypeVarDeclare($1, NULL, NULL, NULL, NULL, $3, my_yylineno);} 
	;

procedural_continuous_assignment:
	vASSIGN list_of_blocking_assignment ';'	{$$ = $2;}
	;

list_of_blocking_assignment:
	list_of_blocking_assignment ',' blocking_assignment	{$$ = newList_entry($1, $3);}
	| blocking_assignment					{$$ = newList(ASSIGN, $1, my_yylineno);}
	;

gate_declaration:
	vAND list_of_multiple_inputs_gate_declaration_instance ';'	{$$ = newGate(BITWISE_AND, $2, my_yylineno);}
	| vNAND	list_of_multiple_inputs_gate_declaration_instance ';'	{$$ = newGate(BITWISE_NAND, $2, my_yylineno);}
	| vNOR list_of_multiple_inputs_gate_declaration_instance ';'	{$$ = newGate(BITWISE_NOR, $2, my_yylineno);}
	| vNOT list_of_single_input_gate_declaration_instance ';'	{$$ = newGate(BITWISE_NOT, $2, my_yylineno);}
	| vOR list_of_multiple_inputs_gate_declaration_instance ';'	{$$ = newGate(BITWISE_OR, $2, my_yylineno);}
	| vXNOR list_of_multiple_inputs_gate_declaration_instance ';'	{$$ = newGate(BITWISE_XNOR, $2, my_yylineno);}
	| vXOR list_of_multiple_inputs_gate_declaration_instance ';'	{$$ = newGate(BITWISE_XOR, $2, my_yylineno);}
	| vBUF list_of_single_input_gate_declaration_instance ';'	{$$ = newGate(BUF_NODE, $2, my_yylineno);}
	;

list_of_multiple_inputs_gate_declaration_instance:
	list_of_multiple_inputs_gate_declaration_instance ',' multiple_inputs_gate_instance	{$$ = newList_entry($1, $3);}
	| multiple_inputs_gate_instance								{$$ = newList(ONE_GATE_INSTANCE, $1, my_yylineno);}
	;

list_of_single_input_gate_declaration_instance:
	list_of_single_input_gate_declaration_instance ',' single_input_gate_instance	{$$ = newList_entry($1, $3);}
	| single_input_gate_instance							{$$ = newList(ONE_GATE_INSTANCE, $1, my_yylineno);}
	;

single_input_gate_instance:
	vSYMBOL_ID '(' expression ',' expression ')'	{$$ = newGateInstance($1, $3, $5, NULL, my_yylineno);}
	| '(' expression ',' expression ')'		{$$ = newGateInstance(NULL, $2, $4, NULL, my_yylineno);}
	;

multiple_inputs_gate_instance:
	vSYMBOL_ID '(' expression ',' expression ',' list_of_multiple_inputs_gate_connections ')'	{$$ = newMultipleInputsGateInstance($1, $3, $5, $7, my_yylineno);}
	| '(' expression ',' expression ',' list_of_multiple_inputs_gate_connections ')'	{$$ = newMultipleInputsGateInstance(NULL, $2, $4, $6, my_yylineno);}
	;

list_of_multiple_inputs_gate_connections: list_of_multiple_inputs_gate_connections ',' expression	{$$ = newList_entry($1, $3);}
	| expression											{$$ = newModuleConnection(NULL, $1, my_yylineno);}
	;

module_instantiation: 
	vSYMBOL_ID list_of_module_instance ';' 							{$$ = newModuleInstance($1, $2, my_yylineno);}
	;

task_instantiation:
	vSYMBOL_ID '(' task_instance ')' ';'												{$$ = newTaskInstance($1, $3, NULL, my_yylineno);}
	| vSYMBOL_ID '(' ')' ';'														{$$ = newTaskInstance($1, NULL, NULL, my_yylineno);}
	| '#' '(' list_of_module_parameters ')' vSYMBOL_ID '(' task_instance ')' ';'		{$$ = newTaskInstance($5, $7, $3, my_yylineno);}
	| '#' '(' list_of_module_parameters ')' vSYMBOL_ID '(' ')' ';'				{$$ = newTaskInstance($5, NULL, $3, my_yylineno);}
	;

list_of_module_instance:
	list_of_module_instance ',' module_instance                     {$$ = newList_entry($1, $3);}
	| module_instance                                                {$$ = newList(ONE_MODULE_INSTANCE, $1, my_yylineno);}
	;

function_instantiation: 
	vSYMBOL_ID function_instance 									{$$ = newFunctionInstance($1, $2, my_yylineno);}
	;

task_instance: 
	list_of_task_connections											 {$$ = newTaskNamedInstance($1, my_yylineno);}
	;

function_instance:
	'(' list_of_function_connections ')'	{$$ = newFunctionNamedInstance($2, NULL, my_yylineno);}
	;

module_instance:
	vSYMBOL_ID '(' list_of_module_connections ')'						{$$ = newModuleNamedInstance($1, $3, NULL, my_yylineno);}
	| '#' '(' list_of_module_parameters ')' vSYMBOL_ID '(' list_of_module_connections ')'	{$$ = newModuleNamedInstance($5, $7, $3, my_yylineno); }
	| vSYMBOL_ID '(' ')'									{$$ = newModuleNamedInstance($1, NULL, NULL, my_yylineno);}
	| '#' '(' list_of_module_parameters ')' vSYMBOL_ID '(' ')'				{$$ = newModuleNamedInstance($5, NULL, $3, my_yylineno);}
	;

list_of_function_connections:
	list_of_function_connections ',' tf_connection	{$$ = newList_entry($1, $3);}
	| tf_connection					{$$ = newfunctionList(MODULE_CONNECT_LIST, $1, my_yylineno);}
	;

list_of_task_connections:
	list_of_task_connections ',' tf_connection	{$$ = newList_entry($1, $3);}
	| tf_connection					{$$ = newList(MODULE_CONNECT_LIST, $1, my_yylineno);}
	;

list_of_module_connections:
	list_of_module_connections ',' module_connection	{$$ = newList_entry($1, $3);}
	| list_of_module_connections ',' 					{$$ = newList_entry($1, newModuleConnection(NULL, NULL, my_yylineno));}
	| module_connection					{$$ = newList(MODULE_CONNECT_LIST, $1, my_yylineno);}
	;

tf_connection:
	'.' vSYMBOL_ID '(' expression ')'	{$$ = newModuleConnection($2, $4, my_yylineno);}
	| expression				{$$ = newModuleConnection(NULL, $1, my_yylineno);}
	;

module_connection:
	'.' vSYMBOL_ID '(' expression ')'	{$$ = newModuleConnection($2, $4, my_yylineno);}
	| '.' vSYMBOL_ID '(' ')'			{$$ = newModuleConnection($2, NULL, my_yylineno);}
	| expression				{$$ = newModuleConnection(NULL, $1, my_yylineno);}
	;

list_of_module_parameters:
	list_of_module_parameters ',' module_parameter	{$$ = newList_entry($1, $3);}
	| module_parameter				{$$ = newList(MODULE_PARAMETER_LIST, $1, my_yylineno);}
	;

module_parameter:
	'.' vSYMBOL_ID '(' expression ')'	{$$ = newModuleParameter($2, $4, my_yylineno);}
	| '.' vSYMBOL_ID '(' ')'		{$$ = newModuleParameter($2, NULL, my_yylineno);}
	| expression				{$$ = newModuleParameter(NULL, $1, my_yylineno);}
	;

always:
	vALWAYS delay_control statement 					{$$ = newAlways($2, $3, my_yylineno);}
	| vALWAYS statement									{$$ = newAlways(NULL, $2, my_yylineno);}
	;

generate:
	vGENERATE generate_item vENDGENERATE	{$$ = $2;}
	;

loop_generate_construct:
	vFOR '(' blocking_assignment ';' expression ';' blocking_assignment ')' generate_block	{$$ = newFor($3, $5, $7, $9, my_yylineno);}
	;

if_generate_construct:
	vIF '(' expression ')' generate_block %prec LOWER_THAN_ELSE		{$$ = newIf($3, $5, NULL, my_yylineno);}
	| vIF '(' expression ')' generate_block vELSE generate_block	{$$ = newIf($3, $5, $7, my_yylineno);}
	;

case_generate_construct:
	vCASE '(' expression ')' case_generate_item_list vENDCASE			{$$ = newCase($3, $5, my_yylineno);}
	;

case_generate_item_list:
	case_generate_item_list case_generate_items	{$$ = newList_entry($1, $2);}
	| case_generate_items			{$$ = newList(CASE_LIST, $1, my_yylineno);}
	;

case_generate_items:
	expression ':' generate_block	{$$ = newCaseItem($1, $3, my_yylineno);}
	| vDEFAULT ':' generate_block	{$$ = newDefaultCase($3, my_yylineno);}
	;

generate_block:
	vBEGIN ':' vSYMBOL_ID list_of_generate_block_items vEND		{$$ = newBlock($3, $4);}
	| vBEGIN list_of_generate_block_items vEND					{$$ = newBlock(NULL, $2);}
	| generate_block_item										{$$ = newBlock(NULL, newList(BLOCK, $1, my_yylineno));}
	;

function_statement:
	function_statement_items				{$$ = newStatement($1, my_yylineno);}
	;

task_statement:
	statement	{$$ = newStatement($1, my_yylineno);}
	;

statement:
	seq_block													{$$ = $1;}
	| task_instantiation										{$$ = $1;}
	| c_function ';'											{$$ = $1;}
	| blocking_assignment ';'									{$$ = $1;}
	| non_blocking_assignment ';'								{$$ = $1;}
	| procedural_continuous_assignment							{$$ = $1;}
	| conditional_statement										{$$ = $1;}
	| case_statement											{$$ = $1;}
	| loop_statement											{$$ = newList(BLOCK, $1, my_yylineno);}
	| ';'														{$$ = NULL;}
	;

function_statement_items:
	function_seq_block											{$$ = $1;}
	| c_function ';'											{$$ = $1;}
	| blocking_assignment ';'									{$$ = $1;}
	| function_conditional_statement							{$$ = $1;}
	| function_case_statement									{$$ = $1;}
	| function_loop_statement									{$$ = newList(BLOCK, $1, my_yylineno);}
	| ';'														{$$ = NULL;}
	;

function_seq_block:
	vBEGIN function_stmt_list vEND					{$$ = newBlock(NULL, $2);}
	| vBEGIN ':' vSYMBOL_ID function_stmt_list vEND	{$$ = newBlock($3, $4);}
	;

function_stmt_list:
	function_stmt_list function_statement_items 	{$$ = newList_entry($1, $2);}
	| function_statement_items						{$$ = newList(BLOCK, $1, my_yylineno);}
	;

function_loop_statement:
	vFOR '(' blocking_assignment ';' expression ';' blocking_assignment ')' function_statement_items	{$$ = newFor($3, $5, $7, $9, my_yylineno);}
	| vWHILE '(' expression ')' function_statement_items							{$$ = newWhile($3, $5, my_yylineno);}
	;

loop_statement:
	vFOR '(' blocking_assignment ';' expression ';' blocking_assignment ')' statement	{$$ = newFor($3, $5, $7, $9, my_yylineno);}
	| vWHILE '(' expression ')' statement							{$$ = newWhile($3, $5, my_yylineno);}
	;

case_statement:
	vCASE '(' expression ')' case_item_list vENDCASE			{$$ = newCase($3, $5, my_yylineno);}
	;

function_case_statement:
	vCASE '(' expression ')' function_case_item_list vENDCASE			{$$ = newCase($3, $5, my_yylineno);}
	;

function_conditional_statement:
	vIF '(' expression ')' function_statement_items %prec LOWER_THAN_ELSE						{$$ = newIf($3, $5, NULL, my_yylineno);}
	| vIF '(' expression ')' function_statement_items vELSE function_statement_items			{$$ = newIf($3, $5, $7, my_yylineno);}
	;

conditional_statement:
	vIF '(' expression ')' statement %prec LOWER_THAN_ELSE	{$$ = newIf($3, $5, NULL, my_yylineno);}
	| vIF '(' expression ')' statement vELSE statement			{$$ = newIf($3, $5, $7, my_yylineno);}
	;

blocking_assignment:
	primary '=' expression			{$$ = newBlocking($1, $3, my_yylineno);}
	| primary '=' vDELAY_ID expression	{$$ = newBlocking($1, $4, my_yylineno);}
	;

non_blocking_assignment:
	primary voLTE expression		{$$ = newNonBlocking($1, $3, my_yylineno);}
	| primary voLTE vDELAY_ID expression	{$$ = newNonBlocking($1, $4, my_yylineno);}
	;

case_item_list:
	case_item_list case_items	{$$ = newList_entry($1, $2);}
	| case_items			{$$ = newList(CASE_LIST, $1, my_yylineno);}
	;

function_case_item_list:
	function_case_item_list function_case_items	{$$ = newList_entry($1, $2);}
	| function_case_items			{$$ = newList(CASE_LIST, $1, my_yylineno);}
	;

function_case_items:
	expression ':' function_statement_items	{$$ = newCaseItem($1, $3, my_yylineno);}
	| vDEFAULT ':' function_statement_items	{$$ = newDefaultCase($3, my_yylineno);}
	;

case_items:
	expression ':' statement	{$$ = newCaseItem($1, $3, my_yylineno);}
	| vDEFAULT ':' statement	{$$ = newDefaultCase($3, my_yylineno);}
	;
	
seq_block:
	vBEGIN stmt_list vEND					{$$ = newBlock(NULL, $2);}
	| vBEGIN ':' vSYMBOL_ID stmt_list vEND	{$$ = newBlock($3, $4);}
	;

stmt_list:
	stmt_list statement	{$$ = newList_entry($1, $2);}
	| statement		{$$ = newList(BLOCK, $1, my_yylineno);}
	;

delay_control:
	'@' '(' event_expression_list ')'	{$$ = $3;}
	| '@' '*'				{$$ = NULL;}
	| '@' '(' '*' ')'			{$$ = NULL;}
	;

event_expression_list:
	event_expression_list vOR event_expression	{$$ = newList_entry($1, $3);}
	| event_expression_list ',' event_expression	{$$ = newList_entry($1, $3);}
	| event_expression				{$$ = newList(DELAY_CONTROL, $1, my_yylineno);}
	;

event_expression:
	primary			{$$ = $1;}
	| vPOSEDGE vSYMBOL_ID	{$$ = newPosedgeSymbol($2, my_yylineno);}
	| vNEGEDGE vSYMBOL_ID	{$$ = newNegedgeSymbol($2, my_yylineno);}
	;

expression:
	primary											{$$ = $1;}
	| '+' expression %prec UADD						{$$ = newUnaryOperation(ADD, $2, my_yylineno);}
	| '-' expression %prec UMINUS					{$$ = newUnaryOperation(MINUS, $2, my_yylineno);}
	| '~' expression %prec UNOT						{$$ = newUnaryOperation(BITWISE_NOT, $2, my_yylineno);}
	| '&' expression %prec UAND						{$$ = newUnaryOperation(BITWISE_AND, $2, my_yylineno);}
	| '|' expression %prec UOR						{$$ = newUnaryOperation(BITWISE_OR, $2, my_yylineno);}
	| voNAND expression %prec UNAND					{$$ = newUnaryOperation(BITWISE_NAND, $2, my_yylineno);}
	| voNOR expression %prec UNOR					{$$ = newUnaryOperation(BITWISE_NOR, $2, my_yylineno);}
	| voXNOR  expression %prec UXNOR				{$$ = newUnaryOperation(BITWISE_XNOR, $2, my_yylineno);}
	| '!' expression %prec ULNOT					{$$ = newUnaryOperation(LOGICAL_NOT, $2, my_yylineno);}
	| '^' expression %prec UXOR						{$$ = newUnaryOperation(BITWISE_XOR, $2, my_yylineno);}
	| vCLOG2 '(' expression ')'						{$$ = newUnaryOperation(CLOG2, $3, my_yylineno);}
	| voUNSIGNED '(' expression ')'					{$$ = newUnaryOperation(UNSIGNED, $3, my_yylineno);}
	| voSIGNED '(' expression ')'					{$$ = newUnaryOperation(SIGNED, $3, my_yylineno);}
	| expression voPOWER expression					{$$ = newBinaryOperation(POWER,$1, $3, my_yylineno);}
	| expression '*' expression						{$$ = newBinaryOperation(MULTIPLY, $1, $3, my_yylineno);}
	| expression '/' expression						{$$ = newBinaryOperation(DIVIDE, $1, $3, my_yylineno);}
	| expression '%' expression						{$$ = newBinaryOperation(MODULO, $1, $3, my_yylineno);}
	| expression '+' expression						{$$ = newBinaryOperation(ADD, $1, $3, my_yylineno);}
	| expression '-' expression						{$$ = newBinaryOperation(MINUS, $1, $3, my_yylineno);}
	| expression voSRIGHT expression				{$$ = newBinaryOperation(SR, $1, $3, my_yylineno);}
	| expression voASRIGHT expression				{$$ = newBinaryOperation(ASR, $1, $3, my_yylineno);}
	| expression voSLEFT expression					{$$ = newBinaryOperation(SL, $1, $3, my_yylineno);}
	| expression '<' expression						{$$ = newBinaryOperation(LT, $1, $3, my_yylineno);}
	| expression '>' expression						{$$ = newBinaryOperation(GT, $1, $3, my_yylineno);}
	| expression voLTE expression					{$$ = newBinaryOperation(LTE, $1, $3, my_yylineno);}
	| expression voGTE expression					{$$ = newBinaryOperation(GTE, $1, $3, my_yylineno);}
	| expression voEQUAL expression					{$$ = newBinaryOperation(LOGICAL_EQUAL, $1, $3, my_yylineno);}
	| expression voNOTEQUAL expression				{$$ = newBinaryOperation(NOT_EQUAL, $1, $3, my_yylineno);}
	| expression voCASEEQUAL expression				{$$ = newBinaryOperation(CASE_EQUAL, $1, $3, my_yylineno);}
	| expression voCASENOTEQUAL expression			{$$ = newBinaryOperation(CASE_NOT_EQUAL, $1, $3, my_yylineno);}
	| expression '&' expression						{$$ = newBinaryOperation(BITWISE_AND, $1, $3, my_yylineno);}
	| expression voNAND expression					{$$ = newBinaryOperation(BITWISE_NAND, $1, $3, my_yylineno);}
	| expression '^' expression						{$$ = newBinaryOperation(BITWISE_XOR, $1, $3, my_yylineno);}
	| expression voXNOR expression					{$$ = newBinaryOperation(BITWISE_XNOR, $1, $3, my_yylineno);}
	| expression '|' expression						{$$ = newBinaryOperation(BITWISE_OR, $1, $3, my_yylineno);}
	| expression voNOR expression					{$$ = newBinaryOperation(BITWISE_NOR, $1, $3, my_yylineno);}
	| expression voANDAND expression				{$$ = newBinaryOperation(LOGICAL_AND, $1, $3, my_yylineno);}
	| expression voOROR expression					{$$ = newBinaryOperation(LOGICAL_OR, $1, $3, my_yylineno);}
	| expression '?' expression ':' expression		{$$ = newIfQuestion($1, $3, $5, my_yylineno);}
	| function_instantiation						{$$ = $1;}
	| '(' expression ')'							{$$ = $2;}
	| '{' expression '{' expression_list '}' '}'	{$$ = newListReplicate( $2, $4, my_yylineno); }
	| c_function									{$$ = $1;}
	;

primary:
	vNUMBER													{$$ = newNumberNode($1, my_yylineno);}
	| vSYMBOL_ID											{$$ = newSymbolNode($1, my_yylineno);}
	| vSYMBOL_ID '[' expression ']'							{$$ = newArrayRef($1, $3, my_yylineno);}
	| vSYMBOL_ID '[' expression ']' '[' expression ']'		{$$ = newArrayRef2D($1, $3, $6, my_yylineno);}
	| vSYMBOL_ID '[' expression vPLUS_COLON expression ']'	{$$ = newPlusColonRangeRef($1, $3, $5, my_yylineno);}
	| vSYMBOL_ID '[' expression vMINUS_COLON expression ']'	{$$ = newMinusColonRangeRef($1, $3, $5, my_yylineno);}
	| vSYMBOL_ID '[' expression ':' expression ']'			{$$ = newRangeRef($1, $3, $5, my_yylineno);}
	| vSYMBOL_ID '[' expression ':' expression ']' '[' expression ':' expression ']'	{$$ = newRangeRef2D($1, $3, $5, $8, $10, my_yylineno);}
	| '{' expression_list '}'								{$$ = $2; ($2)->types.concat.num_bit_strings = -1;}
	;
expression_list:
	expression_list ',' expression	{$$ = newList_entry($1, $3); /* note this will be in order lsb = greatest to msb = 0 in the node child list */}
	| expression					{$$ = newList(CONCATENATE, $1, my_yylineno);}
	;

 c_function:
	vCFUNC '(' c_function_expression_list ')'	{$$ = NULL;}
	| vCFUNC '(' ')'							{$$ = NULL;}
	| vCFUNC									{$$ = NULL;}
	;

 c_function_expression_list:
	expression ',' c_function_expression_list	{$$ = free_whole_tree($1);}
	| expression								{$$ = free_whole_tree($1);}
	;

wire_types: 
	netWIRE 														{ $$ = WIRE; }
	| netTRI 														{ $$ = WIRE; }
	| netTRI0 														{ $$ = WIRE; }
	| netTRI1 														{ $$ = WIRE; }
	| netWAND 														{ $$ = WIRE; }
	| netTRIAND 													{ $$ = WIRE; }
	| netWOR 														{ $$ = WIRE; }
	| netTRIOR 														{ $$ = WIRE; }
	| netTRIREG 													{ $$ = WIRE; }
	| netUWIRE 														{ $$ = WIRE; }
	| netNONE														{ $$ = WIRE; }
	;

reg_types: 
	netREG 															{ $$ = REG; }
	;

net_types:
	wire_types														{ $$ = $1; }
	| reg_types														{ $$ = $1; }	
	;

net_direction:
	vINPUT															{ $$ = INPUT; }
	| vOUTPUT														{ $$ = OUTPUT; }
	| vINOUT														{ $$ = INOUT; }
	;

%%
