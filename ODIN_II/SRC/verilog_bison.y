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
#include <stdbool.h>

#include "odin_types.h"
#include "odin_globals.h"
#include "odin_error.h"
#include "ast_util.h"
#include "odin_util.h"
#include "parse_making_ast.h"
#include "vtr_memory.h"
#include "scope_util.h"

extern loc_t my_location;

void yyerror(const char *str){	delayed_error_message(PARSER, my_location, "error in parsing: (%s)\n",str);}
int ieee_filter(int ieee_version, int return_type);
std::string ieee_string(int return_type);

int yywrap(){	return 1;}
int yylex(void);

%}
/* give detailed errors */
%define parse.error verbose

%locations

%union{
	char *id_name;
	char *num_value;
	char *str_value;
	ast_node_t *node;
	ids id;
	operation_list op;
}

/* base types */
%token <id_name> vSYMBOL_ID
%token <num_value> vNUMBER vINT_NUMBER
%token <str_value> vSTRING

/********************
 * Restricted keywords 
 */

/* Unlabeled */
%token vAUTOMATIC
%token vDEFAULT
%token vDESIGN
%token vDISABLE
%token vEVENT
%token vFORCE
%token vINCDIR
%token vINCLUDE
%token vINITIAL
%token vINSTANCE
%token vLIBLIST
%token vLIBRARY
%token vNOSHOWCANCELLED
%token vPULSESTYLE_ONDETECT
%token vPULSESTYLE_ONEVENT
%token vRELEASE
%token vSHOWCANCELLED
%token vCELL
%token vUSE

/* interconnect */
%token vINOUT vINPUT vOUTPUT

/* vars */
%token vDEFPARAM vLOCALPARAM vPARAMETER vREALTIME vSPECPARAM vTIME

/* control flow */
%token vELSE vFOR vFOREVER vFORK vJOIN vIF vIFNONE vREPEAT vWAIT vWHILE 

/* logic gates */
%token vAND vBUF vBUFIF0 vBUFIF1 vNAND vNOR vNOT vNOTIF0 vNOTIF1 vOR vXOR vXNOR

/* data types */
%token vINTEGER vREAL vSCALARED vSIGNED vVECTORED vUNSIGNED

/* power level */
%token vSMALL vMEDIUM vLARGE vHIGHZ0 vHIGHZ1 vPULL0 vPULL1 vPULLDOWN vPULLUP vSTRONG0 vSTRONG1 vSUPPLY0 vSUPPLY1 vWEAK0 vWEAK1

/* Transistor level logic */
%token vCMOS vNMOS vPMOS vRCMOS vRNMOS vRPMOS vRTRAN vRTRANIF0 vRTRANIF1 vTRAN vTRANIF0 vTRANIF1

/* net types */
%token vWIRE vWAND vWOR vTRI vTRI0 vTRI1 vTRIAND vTRIOR vTRIREG vUWIRE vNONE vREG

/* Timing Related */
%token vALWAYS vEDGE vNEGEDGE vPOSEDGE

/* statements */
%token vASSIGN vDEASSIGN

/* Blocks */
%token vBEGIN vEND
%token vCASE vENDCASE /* related */ vCASEX vCASEZ
%token vCONFIG vENDCONFIG
%token vFUNCTION vENDFUNCTION
%token vGENERATE vENDGENERATE /* related */  vGENVAR
%token vMODULE vMACROMODULE vENDMODULE
%token vPRIMITIVE vENDPRIMITIVE
%token vSPECIFY vENDSPECIFY
%token vTABLE vENDTABLE
%token vTASK vENDTASK

/* logical operators */
%token voNOTEQUAL voEQUAL voCASEEQUAL voOROR voLTE voGTE voANDAND voANDANDAND voCASENOTEQUAL '!'

/* bitwise operators */
%token voXNOR voNAND voNOR '|' '^' '&' '~'

/* arithmetic operators */
%token voSLEFT voSRIGHT voASLEFT voASRIGHT '+' '-' '*' '/' '%'

/* unclassified operator */
%token voEGT voPLUSCOLON voMINUSCOLON '='

/* catch special characters */
%token '?' ':' '<' '>' '(' ')' '{' '}' '[' ']' ';' '#' ',' '.' '@' 

/* C functions */
%token vsFINISH vsDISPLAY vsCLOG2 vsUNSIGNED vsSIGNED vsFUNCTION

/* preprocessor directives */
%token preDEFAULT_NETTYPE

%right '?' ':'
%left voOROR
%left voANDAND voANDANDAND
%left '|'
%left '^' voXNOR voNOR
%left '&' voNAND
%left voEQUAL voNOTEQUAL voCASEEQUAL voCASENOTEQUAL
%left voGTE voLTE '<' '>'
%left voSLEFT voSRIGHT voASLEFT voASRIGHT
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
%type <str_value> stringify
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
%type <node> stmt_list function_stmt_list timing_control delay_control event_expression_list event_expression loop_statement function_loop_statement
%type <node> expression primary expression_list module_parameter
%type <node> list_of_module_parameters localparam_declaration
%type <node> specify_block list_of_specify_items
%type <node> c_function_expression_list c_function
%type <node> initial_block list_of_blocking_assignment
%type <node> list_of_generate_block_items generate_item generate_block_item generate loop_generate_construct if_generate_construct 
%type <node> case_generate_construct case_generate_item_list case_generate_items generate_block generate_localparam_declaration generate_defparam_declaration

/* capture wether an operation is signed or not */
%type <op> var_signedness

%%

source_text:
	items	{ next_parsed_verilog_file($1);}
	;

items:
	items item 									{ ($2 == NULL)? $$ = $1 : ( ($1 == NULL)? $$ = newList(FILE_ITEMS, $2, my_location): newList_entry($1, $2) ); }
	| item 										{ ($1 == NULL)? $$ = $1 : $$ = newList(FILE_ITEMS, $1, my_location); }
	;

item:
	module															{ $$ = $1; }
	| preDEFAULT_NETTYPE wire_types									{ default_net_type = $2; $$ = NULL; }
	;

module:
	vMODULE vSYMBOL_ID module_parameters module_ports ';' list_of_module_items vENDMODULE			{$$ = newModule($2, $3, $4, $6, my_location);}
	| vMODULE vSYMBOL_ID module_parameters ';' list_of_module_items vENDMODULE						{$$ = newModule($2, $3, NULL, $5, my_location);}
	| vMODULE vSYMBOL_ID module_ports ';' list_of_module_items vENDMODULE							{$$ = newModule($2, NULL, $3, $5, my_location);}
	| vMODULE vSYMBOL_ID ';' list_of_module_items vENDMODULE										{$$ = newModule($2, NULL, NULL, $4, my_location);}
	;

module_parameters:
	'#' '(' list_of_parameter_declaration ')'			{$$ = $3;}
	| '#' '(' list_of_parameter_declaration ',' ')'		{$$ = $3;}
	| '#' '(' ')'										{$$ = NULL;}
	;

list_of_parameter_declaration:
	list_of_parameter_declaration ',' vPARAMETER var_signedness variable	{$$ = newList_entry($1, markAndProcessParameterWith(PARAMETER, $5, $4));}
	| list_of_parameter_declaration ',' vPARAMETER variable			{$$ = newList_entry($1, markAndProcessParameterWith(PARAMETER, $4, UNSIGNED));}
	| list_of_parameter_declaration ',' variable					{$$ = newList_entry($1, markAndProcessParameterWith(PARAMETER, $3, UNSIGNED));} 
	| vPARAMETER var_signedness variable									{$$ = newList(VAR_DECLARE_LIST, markAndProcessParameterWith(PARAMETER, $3, $2), my_location);}
	| vPARAMETER variable											{$$ = newList(VAR_DECLARE_LIST, markAndProcessParameterWith(PARAMETER, $2, UNSIGNED), my_location);}
	;

module_ports:
	'(' list_of_port_declaration ')'					{$$ = $2;}
	| '(' list_of_port_declaration ',' ')'				{$$ = $2;}
	| '(' ')'											{$$ = NULL;}
	;

list_of_port_declaration:
	list_of_port_declaration ',' port_declaration		{$$ = newList_entry($1, $3);}
	| port_declaration									{$$ = newList(VAR_DECLARE_LIST, $1, my_location);}
	;
	
port_declaration:
	net_direction net_types var_signedness variable			{$$ = markAndProcessPortWith(MODULE, $1, $2, $4, $3);}
	| net_direction net_types variable					{$$ = markAndProcessPortWith(MODULE, $1, $2, $3, UNSIGNED);}
	| net_direction variable							{$$ = markAndProcessPortWith(MODULE, $1, NO_ID, $2, UNSIGNED);}
	| net_direction vINTEGER integer_type_variable		{$$ = markAndProcessPortWith(MODULE, $1, REG, $3, SIGNED);}
	| net_direction var_signedness variable					{$$ = markAndProcessPortWith(MODULE, $1, NO_ID, $3, $2);}
	| variable											{$$ = $1;}
	;

list_of_module_items:
	list_of_module_items module_item	{$$ = newList_entry($1, $2);}
	| module_item						{$$ = newList(MODULE_ITEMS, $1, my_location);}
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
	| generate_block_item								{$$ = newList(BLOCK, $1, my_location);}
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
	vFUNCTION function_return ';' list_of_function_items vENDFUNCTION									{$$ = newFunction($2, NULL, $4, my_location, false);}
	| vFUNCTION vAUTOMATIC function_return ';' list_of_function_items vENDFUNCTION						{$$ = newFunction($3, NULL, $5, my_location, true);}
	| vFUNCTION function_return function_port_list ';' list_of_function_items vENDFUNCTION				{$$ = newFunction($2, $3, $5, my_location, false);}
	| vFUNCTION vAUTOMATIC function_return function_port_list ';' list_of_function_items vENDFUNCTION	{$$ = newFunction($3, $4, $6, my_location, true);}
	;

task_declaration:
	vTASK vSYMBOL_ID ';' list_of_task_items vENDTASK													{$$ = newTask($2, NULL, $4, my_location, false);}
	| vTASK vSYMBOL_ID module_ports ';' list_of_task_items vENDTASK										{$$ = newTask($2, $3, $5, my_location, false);}
	| vTASK vAUTOMATIC vSYMBOL_ID ';' list_of_task_items vENDTASK										{$$ = newTask($3, NULL, $5, my_location, true);}
	| vTASK vAUTOMATIC vSYMBOL_ID module_ports ';' list_of_task_items vENDTASK							{$$ = newTask($3, $4, $6, my_location, true);}
	;

initial_block:
	vINITIAL statement	{$$ = newInitial($2, my_location); }
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
	'(' primary voEGT expression ')' '=' expression ';'	{free_whole_tree($2); free_whole_tree($4); free_whole_tree($7); $$ = NULL;}
	| vSPECPARAM variable_list ';'						{free_whole_tree($2); $$ = NULL;}
	;
	
list_of_function_items:
	list_of_function_items function_item	{$$ = newList_entry($1, $2);}
	| function_item				{$$ = newList(FUNCTION_ITEMS, $1, my_location);}
	;

task_input_declaration:
	vINPUT net_declaration					{$$ = markAndProcessSymbolListWith(TASK,INPUT, $2, UNSIGNED);}
	| vINPUT integer_declaration			{$$ = markAndProcessSymbolListWith(TASK,INPUT, $2, SIGNED);}
	| vINPUT var_signedness variable_list ';'		{$$ = markAndProcessSymbolListWith(TASK,INPUT, $3, $2);}
	| vINPUT variable_list ';'				{$$ = markAndProcessSymbolListWith(TASK,INPUT, $2, UNSIGNED);}
	;

task_output_declaration:
	vOUTPUT net_declaration					{$$ = markAndProcessSymbolListWith(TASK,OUTPUT, $2, UNSIGNED);}
	| vOUTPUT integer_declaration			{$$ = markAndProcessSymbolListWith(TASK,OUTPUT, $2, SIGNED);}
	| vOUTPUT var_signedness variable_list ';'		{$$ = markAndProcessSymbolListWith(TASK,OUTPUT, $3, $2);}
	| vOUTPUT variable_list ';'				{$$ = markAndProcessSymbolListWith(TASK,OUTPUT, $2, UNSIGNED);}
	;

task_inout_declaration:
	vINOUT net_declaration					{$$ = markAndProcessSymbolListWith(TASK,INOUT, $2, UNSIGNED);}
	| vINOUT integer_declaration			{$$ = markAndProcessSymbolListWith(TASK,INOUT, $2, SIGNED);}
	| vINOUT var_signedness variable_list ';'		{$$ = markAndProcessSymbolListWith(TASK,INOUT, $3, $2);}
	| vINOUT variable_list ';'				{$$ = markAndProcessSymbolListWith(TASK,INOUT, $2, UNSIGNED);}
	;

list_of_task_items:
	list_of_task_items task_item 	{$$ = newList_entry($1, $2); }
	| task_item						{$$ = newList(TASK_ITEMS, $1, my_location); }
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
	vINPUT var_signedness variable_list ';'          {$$ = markAndProcessSymbolListWith(FUNCTION, INPUT, $3, $2);}
	| vINPUT variable_list ';'                {$$ = markAndProcessSymbolListWith(FUNCTION, INPUT, $2, UNSIGNED);}
	| vINPUT function_integer_declaration ';' {$$ = markAndProcessSymbolListWith(FUNCTION, INPUT, $2, UNSIGNED);}
	;

parameter_declaration:
	vPARAMETER var_signedness variable_list ';'	{$$ = markAndProcessSymbolListWith(MODULE,PARAMETER, $3, $2);}
	| vPARAMETER variable_list ';'			{$$ = markAndProcessSymbolListWith(MODULE,PARAMETER, $2, UNSIGNED);}
	;

task_parameter_declaration:
	vPARAMETER var_signedness variable_list ';'	{$$ = markAndProcessSymbolListWith(TASK,PARAMETER, $3, $2);}
	| vPARAMETER variable_list ';'			{$$ = markAndProcessSymbolListWith(TASK,PARAMETER, $2, UNSIGNED);}
	;

localparam_declaration:
	vLOCALPARAM var_signedness variable_list ';'	{$$ = markAndProcessSymbolListWith(MODULE,LOCALPARAM, $3, $2);}
	| vLOCALPARAM variable_list ';'			{$$ = markAndProcessSymbolListWith(MODULE,LOCALPARAM, $2, UNSIGNED);}
	;

generate_localparam_declaration:
	vLOCALPARAM var_signedness variable_list ';'	{$$ = markAndProcessSymbolListWith(BLOCK,LOCALPARAM, $3, $2);}
	| vLOCALPARAM variable_list ';'			{$$ = markAndProcessSymbolListWith(BLOCK,LOCALPARAM, $2, UNSIGNED);}
	;

defparam_declaration:
	vDEFPARAM defparam_variable_list ';'	{$$ = newDefparam(MODULE, $2, my_location);}
	;

generate_defparam_declaration:
	vDEFPARAM defparam_variable_list ';'	{$$ = newDefparam(BLOCK, $2, my_location);}
	;

io_declaration:
	net_direction net_declaration					{$$ = markAndProcessSymbolListWith(MODULE,$1, $2, UNSIGNED);}
	| net_direction integer_declaration				{$$ = markAndProcessSymbolListWith(MODULE,$1, $2, SIGNED);}
	| net_direction var_signedness variable_list ';'		{$$ = markAndProcessSymbolListWith(MODULE,$1, $3, $2);}
	| net_direction variable_list ';'				{$$ = markAndProcessSymbolListWith(MODULE,$1, $2, UNSIGNED);}
	;

net_declaration:
	net_types var_signedness variable_list ';'				{$$ = markAndProcessSymbolListWith(MODULE, $1, $3, $2);}
	| net_types variable_list ';'					{$$ = markAndProcessSymbolListWith(MODULE, $1, $2, UNSIGNED);}
	;

integer_declaration:
	vINTEGER integer_type_variable_list ';'	{$$ = markAndProcessSymbolListWith(MODULE,REG, $2, SIGNED);}
	;

genvar_declaration:
	vGENVAR integer_type_variable_list ';'	{$$ = markAndProcessSymbolListWith(MODULE,GENVAR, $2, SIGNED);}
	;

function_return:
	list_of_function_return_variable							{$$ = markAndProcessSymbolListWith(FUNCTION, OUTPUT, $1, UNSIGNED);}
	| var_signedness list_of_function_return_variable				{$$ = markAndProcessSymbolListWith(FUNCTION, OUTPUT, $2, $1);}
	| function_integer_declaration							{$$ = markAndProcessSymbolListWith(FUNCTION, OUTPUT, $1, UNSIGNED);}		
	;

list_of_function_return_variable:
	function_return_variable			{$$ = newList(VAR_DECLARE_LIST, $1, my_location);}
	;

function_return_variable:
	vSYMBOL_ID											{$$ = newVarDeclare($1, NULL, NULL, NULL, NULL, NULL, my_location);}								
	| '[' expression ':' expression ']' vSYMBOL_ID		{$$ = newVarDeclare($6, $2, $4, NULL, NULL, NULL, my_location);}
	;

function_integer_declaration:
	vINTEGER integer_type_variable_list 				{$$ = markAndProcessSymbolListWith(FUNCTION, REG, $2, SIGNED);}
	;

function_port_list:
	'(' list_of_function_inputs ')'					{$$ = $2;}
	| '(' list_of_function_inputs ',' ')'				{$$ = $2;}
	| '(' ')'											{$$ = NULL;}
	;

list_of_function_inputs:
	list_of_function_inputs ',' function_input_declaration		{$$ = newList_entry($1, $3);}
	| function_input_declaration									{$$ = newList(VAR_DECLARE_LIST, $1, my_location);}
	;
	
variable_list:
	variable_list ',' variable						{$$ = newList_entry($1, $3);}
	| variable										{$$ = newList(VAR_DECLARE_LIST, $1, my_location);}
	;

defparam_variable_list:
	defparam_variable_list ',' defparam_variable    {$$ = newList_entry($1, $3);}
	| defparam_variable								{$$ = newList(VAR_DECLARE_LIST, $1, my_location);}
	;

integer_type_variable_list:
	integer_type_variable_list ',' integer_type_variable	{$$ = newList_entry($1, $3);}
	| integer_type_variable									{$$ = newList(VAR_DECLARE_LIST, $1, my_location);}
	;

variable:
	vSYMBOL_ID														{$$ = newVarDeclare($1, NULL, NULL, NULL, NULL, NULL, my_location);}
	| '[' expression ':' expression ']' vSYMBOL_ID										{$$ = newVarDeclare($6, $2, $4, NULL, NULL, NULL, my_location);}
	| '[' expression ':' expression ']' vSYMBOL_ID '[' expression ':' expression ']'					{$$ = newVarDeclare($6, $2, $4, $8, $10, NULL, my_location);}
	| '[' expression ':' expression ']' vSYMBOL_ID '[' expression ':' expression ']' '[' expression ':' expression ']'	{$$ = newVarDeclare2D($6, $2, $4, $8, $10,$13,$15, NULL, my_location);}
	| '[' expression ':' expression ']' vSYMBOL_ID '=' expression								{$$ = newVarDeclare($6, $2, $4, NULL, NULL, $8, my_location);}
	| vSYMBOL_ID '=' expression												{$$ = newVarDeclare($1, NULL, NULL, NULL, NULL, $3, my_location);}
	;

defparam_variable:
	vSYMBOL_ID '=' expression				{$$ = newDefparamVarDeclare($1, NULL, NULL, NULL, NULL, $3, my_location);}
	;
	
integer_type_variable:
	vSYMBOL_ID					{$$ = newIntegerTypeVarDeclare($1, NULL, NULL, NULL, NULL, NULL, my_location);}
	| vSYMBOL_ID '[' expression ':' expression ']'	{$$ = newIntegerTypeVarDeclare($1, NULL, NULL, $3, $5, NULL, my_location);}
	| vSYMBOL_ID '=' primary			{$$ = newIntegerTypeVarDeclare($1, NULL, NULL, NULL, NULL, $3, my_location);} 
	;

procedural_continuous_assignment:
	vASSIGN list_of_blocking_assignment ';'	{$$ = $2;}
	;

list_of_blocking_assignment:
	list_of_blocking_assignment ',' blocking_assignment	{$$ = newList_entry($1, $3);}
	| blocking_assignment					{$$ = newList(ASSIGN, $1, my_location);}
	;

gate_declaration:
	vAND list_of_multiple_inputs_gate_declaration_instance ';'	{$$ = newGate(BITWISE_AND, $2, my_location);}
	| vNAND	list_of_multiple_inputs_gate_declaration_instance ';'	{$$ = newGate(BITWISE_NAND, $2, my_location);}
	| vNOR list_of_multiple_inputs_gate_declaration_instance ';'	{$$ = newGate(BITWISE_NOR, $2, my_location);}
	| vNOT list_of_single_input_gate_declaration_instance ';'	{$$ = newGate(BITWISE_NOT, $2, my_location);}
	| vOR list_of_multiple_inputs_gate_declaration_instance ';'	{$$ = newGate(BITWISE_OR, $2, my_location);}
	| vXNOR list_of_multiple_inputs_gate_declaration_instance ';'	{$$ = newGate(BITWISE_XNOR, $2, my_location);}
	| vXOR list_of_multiple_inputs_gate_declaration_instance ';'	{$$ = newGate(BITWISE_XOR, $2, my_location);}
	| vBUF list_of_single_input_gate_declaration_instance ';'	{$$ = newGate(BUF_NODE, $2, my_location);}
	;

list_of_multiple_inputs_gate_declaration_instance:
	list_of_multiple_inputs_gate_declaration_instance ',' multiple_inputs_gate_instance	{$$ = newList_entry($1, $3);}
	| multiple_inputs_gate_instance								{$$ = newList(ONE_GATE_INSTANCE, $1, my_location);}
	;

list_of_single_input_gate_declaration_instance:
	list_of_single_input_gate_declaration_instance ',' single_input_gate_instance	{$$ = newList_entry($1, $3);}
	| single_input_gate_instance							{$$ = newList(ONE_GATE_INSTANCE, $1, my_location);}
	;

single_input_gate_instance:
	vSYMBOL_ID '(' expression ',' expression ')'	{$$ = newGateInstance($1, $3, $5, NULL, my_location);}
	| '(' expression ',' expression ')'		{$$ = newGateInstance(NULL, $2, $4, NULL, my_location);}
	;

multiple_inputs_gate_instance:
	vSYMBOL_ID '(' expression ',' expression ',' list_of_multiple_inputs_gate_connections ')'	{$$ = newMultipleInputsGateInstance($1, $3, $5, $7, my_location);}
	| '(' expression ',' expression ',' list_of_multiple_inputs_gate_connections ')'	{$$ = newMultipleInputsGateInstance(NULL, $2, $4, $6, my_location);}
	;

list_of_multiple_inputs_gate_connections: list_of_multiple_inputs_gate_connections ',' expression	{$$ = newList_entry($1, $3);}
	| expression											{$$ = newModuleConnection(NULL, $1, my_location);}
	;

module_instantiation: 
	vSYMBOL_ID list_of_module_instance ';' 							{$$ = newModuleInstance($1, $2, my_location);}
	;

task_instantiation:
	vSYMBOL_ID '(' task_instance ')' ';'												{$$ = newTaskInstance($1, $3, NULL, my_location);}
	| vSYMBOL_ID '(' ')' ';'														{$$ = newTaskInstance($1, NULL, NULL, my_location);}
	| '#' '(' list_of_module_parameters ')' vSYMBOL_ID '(' task_instance ')' ';'		{$$ = newTaskInstance($5, $7, $3, my_location);}
	| '#' '(' list_of_module_parameters ')' vSYMBOL_ID '(' ')' ';'				{$$ = newTaskInstance($5, NULL, $3, my_location);}
	;

list_of_module_instance:
	list_of_module_instance ',' module_instance                     {$$ = newList_entry($1, $3);}
	| module_instance                                                {$$ = newList(ONE_MODULE_INSTANCE, $1, my_location);}
	;

function_instantiation: 
	vSYMBOL_ID function_instance 									{$$ = newFunctionInstance($1, $2, my_location);}
	;

task_instance: 
	list_of_task_connections											 {$$ = newTaskNamedInstance($1, my_location);}
	;

function_instance:
	'(' list_of_function_connections ')'	{$$ = newFunctionNamedInstance($2, NULL, my_location);}
	;

module_instance:
	vSYMBOL_ID '(' list_of_module_connections ')'						{$$ = newModuleNamedInstance($1, $3, NULL, my_location);}
	| '#' '(' list_of_module_parameters ')' vSYMBOL_ID '(' list_of_module_connections ')'	{$$ = newModuleNamedInstance($5, $7, $3, my_location); }
	| vSYMBOL_ID '(' ')'									{$$ = newModuleNamedInstance($1, NULL, NULL, my_location);}
	| '#' '(' list_of_module_parameters ')' vSYMBOL_ID '(' ')'				{$$ = newModuleNamedInstance($5, NULL, $3, my_location);}
	;

list_of_function_connections:
	list_of_function_connections ',' tf_connection	{$$ = newList_entry($1, $3);}
	| tf_connection					{$$ = newfunctionList(MODULE_CONNECT_LIST, $1, my_location);}
	;

list_of_task_connections:
	list_of_task_connections ',' tf_connection	{$$ = newList_entry($1, $3);}
	| tf_connection					{$$ = newList(MODULE_CONNECT_LIST, $1, my_location);}
	;

list_of_module_connections:
	list_of_module_connections ',' module_connection	{$$ = newList_entry($1, $3);}
	| list_of_module_connections ',' 					{$$ = newList_entry($1, newModuleConnection(NULL, NULL, my_location));}
	| module_connection					{$$ = newList(MODULE_CONNECT_LIST, $1, my_location);}
	;

tf_connection:
	'.' vSYMBOL_ID '(' expression ')'	{$$ = newModuleConnection($2, $4, my_location);}
	| expression				{$$ = newModuleConnection(NULL, $1, my_location);}
	;

module_connection:
	'.' vSYMBOL_ID '(' expression ')'	{$$ = newModuleConnection($2, $4, my_location);}
	| '.' vSYMBOL_ID '(' ')'			{$$ = newModuleConnection($2, NULL, my_location);}
	| expression				{$$ = newModuleConnection(NULL, $1, my_location);}
	;

list_of_module_parameters:
	list_of_module_parameters ',' module_parameter	{$$ = newList_entry($1, $3);}
	| module_parameter				{$$ = newList(MODULE_PARAMETER_LIST, $1, my_location);}
	;

module_parameter:
	'.' vSYMBOL_ID '(' expression ')'	{$$ = newModuleParameter($2, $4, my_location);}
	| '.' vSYMBOL_ID '(' ')'		{$$ = newModuleParameter($2, NULL, my_location);}
	| expression				{$$ = newModuleParameter(NULL, $1, my_location);}
	;

always:
	vALWAYS timing_control statement 					{$$ = newAlways($2, $3, my_location);}
	| vALWAYS statement									{$$ = newAlways(NULL, $2, my_location);}
	;

generate:
	vGENERATE generate_item vENDGENERATE	{$$ = $2;}
	;

loop_generate_construct:
	vFOR '(' blocking_assignment ';' expression ';' blocking_assignment ')' generate_block	{$$ = newFor($3, $5, $7, $9, my_location);}
	;

if_generate_construct:
	vIF '(' expression ')' generate_block %prec LOWER_THAN_ELSE		{$$ = newIf($3, $5, NULL, my_location);}
	| vIF '(' expression ')' generate_block vELSE generate_block	{$$ = newIf($3, $5, $7, my_location);}
	;

case_generate_construct:
	vCASE '(' expression ')' case_generate_item_list vENDCASE			{$$ = newCase($3, $5, my_location);}
	;

case_generate_item_list:
	case_generate_item_list case_generate_items	{$$ = newList_entry($1, $2);}
	| case_generate_items			{$$ = newList(CASE_LIST, $1, my_location);}
	;

case_generate_items:
	expression ':' generate_block	{$$ = newCaseItem($1, $3, my_location);}
	| vDEFAULT ':' generate_block	{$$ = newDefaultCase($3, my_location);}
	;

generate_block:
	vBEGIN ':' vSYMBOL_ID list_of_generate_block_items vEND		{$$ = newBlock($3, $4);}
	| vBEGIN list_of_generate_block_items vEND					{$$ = newBlock(NULL, $2);}
	| generate_block_item										{$$ = newBlock(NULL, newList(BLOCK, $1, my_location));}
	;

function_statement:
	function_statement_items				{$$ = newStatement($1, my_location);}
	;

task_statement:
	statement	{$$ = newStatement($1, my_location);}
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
	| loop_statement											{$$ = newList(BLOCK, $1, my_location);}
	| ';'														{$$ = NULL;}
	;

function_statement_items:
	function_seq_block											{$$ = $1;}
	| c_function ';'											{$$ = $1;}
	| blocking_assignment ';'									{$$ = $1;}
	| function_conditional_statement							{$$ = $1;}
	| function_case_statement									{$$ = $1;}
	| function_loop_statement									{$$ = newList(BLOCK, $1, my_location);}
	| ';'														{$$ = NULL;}
	;

function_seq_block:
	vBEGIN function_stmt_list vEND					{$$ = newBlock(NULL, $2);}
	| vBEGIN ':' vSYMBOL_ID function_stmt_list vEND	{$$ = newBlock($3, $4);}
	;

function_stmt_list:
	function_stmt_list function_statement_items 	{$$ = newList_entry($1, $2);}
	| function_statement_items						{$$ = newList(BLOCK, $1, my_location);}
	;

function_loop_statement:
	vFOR '(' blocking_assignment ';' expression ';' blocking_assignment ')' function_statement_items	{$$ = newFor($3, $5, $7, $9, my_location);}
	| vWHILE '(' expression ')' function_statement_items							{$$ = newWhile($3, $5, my_location);}
	;

loop_statement:
	vFOR '(' blocking_assignment ';' expression ';' blocking_assignment ')' statement	{$$ = newFor($3, $5, $7, $9, my_location);}
	| vWHILE '(' expression ')' statement							{$$ = newWhile($3, $5, my_location);}
	;

case_statement:
	vCASE '(' expression ')' case_item_list vENDCASE			{$$ = newCase($3, $5, my_location);}
	;

function_case_statement:
	vCASE '(' expression ')' function_case_item_list vENDCASE			{$$ = newCase($3, $5, my_location);}
	;

function_conditional_statement:
	vIF '(' expression ')' function_statement_items %prec LOWER_THAN_ELSE						{$$ = newIf($3, $5, NULL, my_location);}
	| vIF '(' expression ')' function_statement_items vELSE function_statement_items			{$$ = newIf($3, $5, $7, my_location);}
	;

conditional_statement:
	vIF '(' expression ')' statement %prec LOWER_THAN_ELSE	{$$ = newIf($3, $5, NULL, my_location);}
	| vIF '(' expression ')' statement vELSE statement			{$$ = newIf($3, $5, $7, my_location);}
	;

blocking_assignment:
	primary '=' expression						{$$ = newBlocking($1, $3, my_location);}
	| delay_control primary '=' expression		{$$ = newBlocking($2, $4, my_location);}
	| primary '=' delay_control expression		{$$ = newBlocking($1, $4, my_location);}
	;

non_blocking_assignment:
	primary voLTE expression					{$$ = newNonBlocking($1, $3, my_location);}
	| delay_control primary voLTE expression	{$$ = newNonBlocking($2, $4, my_location);}
	| primary voLTE delay_control expression	{$$ = newNonBlocking($1, $4, my_location);}
	;

case_item_list:
	case_item_list case_items	{$$ = newList_entry($1, $2);}
	| case_items			{$$ = newList(CASE_LIST, $1, my_location);}
	;

function_case_item_list:
	function_case_item_list function_case_items	{$$ = newList_entry($1, $2);}
	| function_case_items			{$$ = newList(CASE_LIST, $1, my_location);}
	;

function_case_items:
	expression ':' function_statement_items	{$$ = newCaseItem($1, $3, my_location);}
	| vDEFAULT ':' function_statement_items	{$$ = newDefaultCase($3, my_location);}
	;

case_items:
	expression ':' statement	{$$ = newCaseItem($1, $3, my_location);}
	| vDEFAULT ':' statement	{$$ = newDefaultCase($3, my_location);}
	;
	
seq_block:
	vBEGIN stmt_list vEND					{$$ = newBlock(NULL, $2);}
	| vBEGIN ':' vSYMBOL_ID stmt_list vEND	{$$ = newBlock($3, $4);}
	;

stmt_list:
	stmt_list statement	{$$ = newList_entry($1, $2);}
	| statement		{$$ = newList(BLOCK, $1, my_location);}
	;

timing_control:
	'@' '(' event_expression_list ')'	{$$ = $3;}
	| '@' '*'				{$$ = NULL;}
	| '@' '(' '*' ')'			{$$ = NULL;}
	;

delay_control:
	'#' vINT_NUMBER												{vtr::free($2); $$ = NULL;}
	| '#' '(' vINT_NUMBER ')'									{vtr::free($3); $$ = NULL;}
	| '#' '(' vINT_NUMBER ',' vINT_NUMBER ')'					{vtr::free($3); vtr::free($5); $$ = NULL;}
	| '#' '(' vINT_NUMBER ',' vINT_NUMBER ',' vINT_NUMBER ')'	{vtr::free($3); vtr::free($5); vtr::free($7); $$ = NULL;}
	;

event_expression_list:
	event_expression_list vOR event_expression	{$$ = newList_entry($1, $3);}
	| event_expression_list ',' event_expression	{$$ = newList_entry($1, $3);}
	| event_expression				{$$ = newList(DELAY_CONTROL, $1, my_location);}
	;

event_expression:
	primary					{$$ = $1;}
	| vPOSEDGE primary		{$$ = newPosedge($2, my_location);}
	| vNEGEDGE primary		{$$ = newNegedge($2, my_location);}
	;

stringify:
	stringify vSTRING								{$$ = str_collate($1, $2);}
	| vSTRING										{$$ = $1;}
	;

expression:
	vINT_NUMBER										{$$ = newNumberNode($1, my_location);}
	| vNUMBER										{$$ = newNumberNode($1, my_location);}
	| stringify										{$$ = newStringNode($1, my_location);}
	| primary										{$$ = $1;}
	| '+' expression %prec UADD						{$$ = newUnaryOperation(ADD, $2, my_location);}
	| '-' expression %prec UMINUS					{$$ = newUnaryOperation(MINUS, $2, my_location);}
	| '~' expression %prec UNOT						{$$ = newUnaryOperation(BITWISE_NOT, $2, my_location);}
	| '&' expression %prec UAND						{$$ = newUnaryOperation(BITWISE_AND, $2, my_location);}
	| '|' expression %prec UOR						{$$ = newUnaryOperation(BITWISE_OR, $2, my_location);}
	| voNAND expression %prec UNAND					{$$ = newUnaryOperation(BITWISE_NAND, $2, my_location);}
	| voNOR expression %prec UNOR					{$$ = newUnaryOperation(BITWISE_NOR, $2, my_location);}
	| voXNOR  expression %prec UXNOR				{$$ = newUnaryOperation(BITWISE_XNOR, $2, my_location);}
	| '!' expression %prec ULNOT					{$$ = newUnaryOperation(LOGICAL_NOT, $2, my_location);}
	| '^' expression %prec UXOR						{$$ = newUnaryOperation(BITWISE_XOR, $2, my_location);}
	| vsCLOG2 '(' expression ')'					{$$ = newUnaryOperation(CLOG2, $3, my_location);}
	| vsUNSIGNED '(' expression ')'					{$$ = newUnaryOperation(UNSIGNED, $3, my_location);}
	| vsSIGNED '(' expression ')'					{$$ = newUnaryOperation(SIGNED, $3, my_location);}
	| expression voPOWER expression					{$$ = newBinaryOperation(POWER,$1, $3, my_location);}
	| expression '*' expression						{$$ = newBinaryOperation(MULTIPLY, $1, $3, my_location);}
	| expression '/' expression						{$$ = newBinaryOperation(DIVIDE, $1, $3, my_location);}
	| expression '%' expression						{$$ = newBinaryOperation(MODULO, $1, $3, my_location);}
	| expression '+' expression						{$$ = newBinaryOperation(ADD, $1, $3, my_location);}
	| expression '-' expression						{$$ = newBinaryOperation(MINUS, $1, $3, my_location);}
	| expression voSRIGHT expression				{$$ = newBinaryOperation(SR, $1, $3, my_location);}
	| expression voASRIGHT expression				{$$ = newBinaryOperation(ASR, $1, $3, my_location);}
	| expression voSLEFT expression					{$$ = newBinaryOperation(SL, $1, $3, my_location);}
	| expression voASLEFT expression				{$$ = newBinaryOperation(ASL, $1, $3, my_location);}
	| expression '<' expression						{$$ = newBinaryOperation(LT, $1, $3, my_location);}
	| expression '>' expression						{$$ = newBinaryOperation(GT, $1, $3, my_location);}
	| expression voLTE expression					{$$ = newBinaryOperation(LTE, $1, $3, my_location);}
	| expression voGTE expression					{$$ = newBinaryOperation(GTE, $1, $3, my_location);}
	| expression voEQUAL expression					{$$ = newBinaryOperation(LOGICAL_EQUAL, $1, $3, my_location);}
	| expression voNOTEQUAL expression				{$$ = newBinaryOperation(NOT_EQUAL, $1, $3, my_location);}
	| expression voCASEEQUAL expression				{$$ = newBinaryOperation(CASE_EQUAL, $1, $3, my_location);}
	| expression voCASENOTEQUAL expression			{$$ = newBinaryOperation(CASE_NOT_EQUAL, $1, $3, my_location);}
	| expression '&' expression						{$$ = newBinaryOperation(BITWISE_AND, $1, $3, my_location);}
	| expression voNAND expression					{$$ = newBinaryOperation(BITWISE_NAND, $1, $3, my_location);}
	| expression '^' expression						{$$ = newBinaryOperation(BITWISE_XOR, $1, $3, my_location);}
	| expression voXNOR expression					{$$ = newBinaryOperation(BITWISE_XNOR, $1, $3, my_location);}
	| expression '|' expression						{$$ = newBinaryOperation(BITWISE_OR, $1, $3, my_location);}
	| expression voNOR expression					{$$ = newBinaryOperation(BITWISE_NOR, $1, $3, my_location);}
	| expression voANDAND expression				{$$ = newBinaryOperation(LOGICAL_AND, $1, $3, my_location);}
	| expression voOROR expression					{$$ = newBinaryOperation(LOGICAL_OR, $1, $3, my_location);}
	| expression '?' expression ':' expression		{$$ = newIfQuestion($1, $3, $5, my_location);}
	| function_instantiation						{$$ = $1;}
	| '(' expression ')'							{$$ = $2;}
	| '{' expression '{' expression_list '}' '}'	{$$ = newListReplicate( $2, $4, my_location); }
	;

primary:
	vSYMBOL_ID												{$$ = newSymbolNode($1, my_location);}
	| vSYMBOL_ID '[' expression ']'							{$$ = newArrayRef($1, $3, my_location);}
	| vSYMBOL_ID '[' expression ']' '[' expression ']'		{$$ = newArrayRef2D($1, $3, $6, my_location);}
	| vSYMBOL_ID '[' expression voPLUSCOLON expression ']'	{$$ = newPlusColonRangeRef($1, $3, $5, my_location);}
	| vSYMBOL_ID '[' expression voMINUSCOLON expression ']'	{$$ = newMinusColonRangeRef($1, $3, $5, my_location);}
	| vSYMBOL_ID '[' expression ':' expression ']'			{$$ = newRangeRef($1, $3, $5, my_location);}
	| vSYMBOL_ID '[' expression ':' expression ']' '[' expression ':' expression ']'	{$$ = newRangeRef2D($1, $3, $5, $8, $10, my_location);}
	| '{' expression_list '}'								{$$ = $2; ($2)->types.concat.num_bit_strings = -1;}
	;

expression_list:
	expression_list ',' expression	{$$ = newList_entry($1, $3); /* order is left to right as is given */ }
	| expression					{$$ = newList(CONCATENATE, $1, my_location);}
	;

c_function:
	vsFINISH '(' expression ')'									{$$ = newCFunction(FINISH, $3, NULL, my_location);}
	| vsDISPLAY '(' expression ')'									{$$ = newCFunction(DISPLAY, $3, NULL, my_location);}
	| vsDISPLAY '(' expression ',' c_function_expression_list ')'	{$$ = newCFunction(DISPLAY, $3, $5, my_location); /* this fails for now */}
	| vsFUNCTION '(' c_function_expression_list ')'					{$$ = free_whole_tree($3);}
	| vsFUNCTION '(' ')'											{$$ = NULL;}
	| vsFUNCTION													{$$ = NULL;}
	;

c_function_expression_list:
	c_function_expression_list ',' expression	{$$ = newList_entry($1, $3);}
	| expression								{$$ = newList(C_ARG_LIST, $1, my_location);}
	;

wire_types: 
	vWIRE 															{ $$ = WIRE; }
	| vTRI 															{ $$ = WIRE; }
	| vTRI0 														{ $$ = WIRE; }
	| vTRI1 														{ $$ = WIRE; }
	| vWAND 														{ $$ = WIRE; }
	| vTRIAND 														{ $$ = WIRE; }
	| vWOR 															{ $$ = WIRE; }
	| vTRIOR 														{ $$ = WIRE; }
	| vTRIREG 														{ $$ = WIRE; }
	| vUWIRE 														{ $$ = WIRE; }
	| vNONE															{ $$ = NO_ID; /* no type here to force an error */ }
	;

reg_types: 
	vREG 															{ $$ = REG; }
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

var_signedness:
	vUNSIGNED														{ $$ = UNSIGNED; }
	| vSIGNED														{ $$ = SIGNED; }
	;
	
%%


/**
 * This functions filters types based on the standard defined,
 */ 
int ieee_filter(int ieee_version, int return_type) {

	switch(return_type) {
		case voANDANDAND:	//fallthrough
		case voPOWER:	//fallthrough
		case voANDAND:	//fallthrough
		case voOROR:	//fallthrough
		case voLTE:	//fallthrough
		case voEGT:	//fallthrough
		case voGTE:	//fallthrough
		case voSLEFT:	//fallthrough
		case voSRIGHT:	//fallthrough
		case voEQUAL:	//fallthrough
		case voNOTEQUAL:	//fallthrough
		case voCASEEQUAL:	//fallthrough
		case voCASENOTEQUAL:	//fallthrough
		case voXNOR:	//fallthrough
		case voNAND:	//fallthrough
		case voNOR:	//fallthrough
		case vALWAYS:	//fallthrough
		case vAND:	//fallthrough
		case vASSIGN:	//fallthrough
		case vBEGIN:	//fallthrough
		case vBUF:	//fallthrough
		case vBUFIF0:	//fallthrough
		case vBUFIF1:	//fallthrough
		case vCASE:	//fallthrough
		case vCASEX:	//fallthrough
		case vCASEZ:	//fallthrough
		case vCMOS:	//fallthrough
		case vDEASSIGN:	//fallthrough
		case vDEFAULT:	//fallthrough
		case vDEFPARAM:	//fallthrough
		case vDISABLE:	//fallthrough
		case vEDGE:	//fallthrough
		case vELSE:	//fallthrough
		case vEND:	//fallthrough
		case vENDCASE:	//fallthrough
		case vENDFUNCTION:	//fallthrough
		case vENDMODULE:	//fallthrough
		case vENDPRIMITIVE:	//fallthrough
		case vENDSPECIFY:	//fallthrough
		case vENDTABLE:	//fallthrough
		case vENDTASK:	//fallthrough
		case vEVENT:	//fallthrough
		case vFOR:	//fallthrough
		case vFORCE:	//fallthrough
		case vFOREVER:	//fallthrough
		case vFORK:	//fallthrough
		case vFUNCTION:	//fallthrough
		case vHIGHZ0:	//fallthrough
		case vHIGHZ1:	//fallthrough
		case vIF:	//fallthrough
		case vIFNONE:	//fallthrough
		case vINITIAL:	//fallthrough
		case vINOUT:	//fallthrough
		case vINPUT:	//fallthrough
		case vINTEGER:	//fallthrough
		case vJOIN:	//fallthrough
		case vLARGE:	//fallthrough
		case vMACROMODULE:	//fallthrough
		case vMEDIUM:	//fallthrough
		case vMODULE:	//fallthrough
		case vNAND:	//fallthrough
		case vNEGEDGE:	//fallthrough
		case vNMOS:	//fallthrough
		case vNOR:	//fallthrough
		case vNOT:	//fallthrough
		case vNOTIF0:	//fallthrough
		case vNOTIF1:	//fallthrough
		case vOR:	//fallthrough
		case vOUTPUT:	//fallthrough
		case vPARAMETER:	//fallthrough
		case vPMOS:	//fallthrough
		case vPOSEDGE:	//fallthrough
		case vPRIMITIVE:	//fallthrough
		case vPULL0:	//fallthrough
		case vPULL1:	//fallthrough
		case vPULLDOWN:	//fallthrough
		case vPULLUP:	//fallthrough
		case vRCMOS:	//fallthrough
		case vREAL:	//fallthrough
		case vREALTIME:	//fallthrough
		case vREG:	//fallthrough
		case vRELEASE:	//fallthrough
		case vREPEAT:	//fallthrough
		case vRNMOS:	//fallthrough
		case vRPMOS:	//fallthrough
		case vRTRAN:	//fallthrough
		case vRTRANIF0:	//fallthrough
		case vRTRANIF1:	//fallthrough
		case vSCALARED:	//fallthrough
		case vSMALL:	//fallthrough
		case vSPECIFY:	//fallthrough
		case vSPECPARAM:	//fallthrough
		case vSTRONG0:	//fallthrough
		case vSTRONG1:	//fallthrough
		case vSUPPLY0:	//fallthrough
		case vSUPPLY1:	//fallthrough
		case vTABLE:	//fallthrough
		case vTASK:	//fallthrough
		case vTIME:	//fallthrough
		case vTRAN:	//fallthrough
		case vTRANIF0:	//fallthrough
		case vTRANIF1:	//fallthrough
		case vTRI:	//fallthrough
		case vTRI0:	//fallthrough
		case vTRI1:	//fallthrough
		case vTRIAND:	//fallthrough
		case vTRIOR:	//fallthrough
		case vTRIREG:	//fallthrough
		case vVECTORED:	//fallthrough
		case vWAIT:	//fallthrough
		case vWAND:	//fallthrough
		case vWEAK0:	//fallthrough
		case vWEAK1:	//fallthrough
		case vWHILE:	//fallthrough
		case vWIRE:	//fallthrough
		case vWOR:	//fallthrough
		case vXNOR:	//fallthrough
		case vXOR: {
			if(ieee_version < ieee_1995)
				delayed_error_message(PARSER, my_location, "error in parsing: (%s) only exists in ieee 1995 or newer\n",ieee_string(return_type).c_str())
			break;
		}
		case voASLEFT:	//fallthrough
		case voASRIGHT:	//fallthrough
		case voPLUSCOLON:   //fallthrough
		case voMINUSCOLON:   //fallthrough
		case vAUTOMATIC:	//fallthrough
		case vENDGENERATE:	//fallthrough
		case vGENERATE:	//fallthrough
		case vGENVAR:	//fallthrough
		case vLOCALPARAM:	//fallthrough
		case vNOSHOWCANCELLED:	//fallthrough
		case vPULSESTYLE_ONDETECT:	//fallthrough
		case vPULSESTYLE_ONEVENT:	//fallthrough
		case vSHOWCANCELLED:	//fallthrough
		case vSIGNED:	//fallthrough
		case vUNSIGNED: {
			if(ieee_version < ieee_2001_noconfig)
				delayed_error_message(PARSER, my_location, "error in parsing: (%s) only exists in ieee 2001-noconfig or newer\n",ieee_string(return_type).c_str())
			break;
		}
		case vCELL:	//fallthrough
		case vCONFIG:	//fallthrough
		case vDESIGN:	//fallthrough
		case vENDCONFIG:	//fallthrough
		case vINCDIR:	//fallthrough
		case vINCLUDE:	//fallthrough
		case vINSTANCE:	//fallthrough
		case vLIBLIST:	//fallthrough
		case vLIBRARY:	//fallthrough
		case vUSE: {
			if(ieee_version < ieee_2001)
				delayed_error_message(PARSER, my_location, "error in parsing: (%s) only exists in ieee 2001 or newer\n",ieee_string(return_type).c_str())
			break;
		}
		case vUWIRE: {
			if(ieee_version < ieee_2005)
				delayed_error_message(PARSER, my_location, "error in parsing: (%s) only exists in ieee 2005 or newer\n",ieee_string(return_type).c_str())
			break;
		}
		case vsCLOG2:
		case vsUNSIGNED:	//fallthrough
		case vsSIGNED:	//fallthrough
		case vsFINISH:	//fallthrough
		case vsDISPLAY:	//fallthrough
		case vsFUNCTION:	//fallthrough
			/* unsorted. TODO: actually sort these */
			break;
		default: {
			delayed_error_message(PARSER, my_location, "error in parsing: keyword index: %d is not a supported keyword.\n",return_type)
			break;
		}
	}
	return return_type;

}

std::string ieee_string(int return_type) {

	switch(return_type) {
		case vALWAYS: return "vALWAYS";
		case vAND: return "vAND";
		case vASSIGN: return "vASSIGN";
		case vAUTOMATIC: return "vAUTOMATIC";
		case vBEGIN: return "vBEGIN";
		case vBUF: return "vBUF";
		case vBUFIF0: return "vBUFIF0";
		case vBUFIF1: return "vBUFIF1";
		case vCASE: return "vCASE";
		case vCASEX: return "vCASEX";
		case vCASEZ: return "vCASEZ";
		case vCELL: return "vCELL";
		case vCMOS: return "vCMOS";
		case vCONFIG: return "vCONFIG";
		case vDEASSIGN: return "vDEASSIGN";
		case vDEFAULT: return "vDEFAULT";
		case vDEFPARAM: return "vDEFPARAM";
		case vDESIGN: return "vDESIGN";
		case vDISABLE: return "vDISABLE";
		case vEDGE: return "vEDGE";
		case vELSE: return "vELSE";
		case vEND: return "vEND";
		case vENDCASE: return "vENDCASE";
		case vENDCONFIG: return "vENDCONFIG";
		case vENDFUNCTION: return "vENDFUNCTION";
		case vENDGENERATE: return "vENDGENERATE";
		case vENDMODULE: return "vENDMODULE";
		case vENDPRIMITIVE: return "vENDPRIMITIVE";
		case vENDSPECIFY: return "vENDSPECIFY";
		case vENDTABLE: return "vENDTABLE";
		case vENDTASK: return "vENDTASK";
		case vEVENT: return "vEVENT";
		case vFOR: return "vFOR";
		case vFORCE: return "vFORCE";
		case vFOREVER: return "vFOREVER";
		case vFORK: return "vFORK";
		case vFUNCTION: return "vFUNCTION";
		case vGENERATE: return "vGENERATE";
		case vGENVAR: return "vGENVAR";
		case vHIGHZ0: return "vHIGHZ0";
		case vHIGHZ1: return "vHIGHZ1";
		case vIF: return "vIF";
		case vINCDIR: return "vINCDIR";
		case vINCLUDE: return "vINCLUDE";
		case vINITIAL: return "vINITIAL";
		case vINOUT: return "vINOUT";
		case vINPUT: return "vINPUT";
		case vINSTANCE: return "vINSTANCE";
		case vINTEGER: return "vINTEGER";
		case vJOIN: return "vJOIN";
		case vLARGE: return "vLARGE";
		case vLIBLIST: return "vLIBLIST";
		case vLIBRARY: return "vLIBRARY";
		case vLOCALPARAM: return "vLOCALPARAM";
		case vMEDIUM: return "vMEDIUM";
		case vMODULE: return "vMODULE";
		case vNAND: return "vNAND";
		case vNEGEDGE: return "vNEGEDGE";
		case vNMOS: return "vNMOS";
		case vNONE: return "vNONE";
		case vNOR: return "vNOR";
		case vNOSHOWCANCELLED: return "vNOSHOWCANCELLED";
		case vNOT: return "vNOT";
		case vNOTIF0: return "vNOTIF0";
		case vNOTIF1: return "vNOTIF1";
		case vOR: return "vOR";
		case vOUTPUT: return "vOUTPUT";
		case vPARAMETER: return "vPARAMETER";
		case vPMOS: return "vPMOS";
		case vPOSEDGE: return "vPOSEDGE";
		case vPRIMITIVE: return "vPRIMITIVE";
		case vPULL0: return "vPULL0";
		case vPULL1: return "vPULL1";
		case vPULLDOWN: return "vPULLDOWN";
		case vPULLUP: return "vPULLUP";
		case vPULSESTYLE_ONDETECT: return "vPULSESTYLE_ONDETECT";
		case vPULSESTYLE_ONEVENT: return "vPULSESTYLE_ONEVENT";
		case vRCMOS: return "vRCMOS";
		case vREG: return "vREG";
		case vRELEASE: return "vRELEASE";
		case vREPEAT: return "vREPEAT";
		case vRNMOS: return "vRNMOS";
		case vRPMOS: return "vRPMOS";
		case vRTRAN: return "vRTRAN";
		case vRTRANIF0: return "vRTRANIF0";
		case vRTRANIF1: return "vRTRANIF1";
		case vSCALARED: return "vSCALARED";
		case vSHOWCANCELLED: return "vSHOWCANCELLED";
		case vSIGNED: return "vSIGNED";
		case vSMALL: return "vSMALL";
		case vSPECIFY: return "vSPECIFY";
		case vSPECPARAM: return "vSPECPARAM";
		case vSTRONG0: return "vSTRONG0";
		case vSTRONG1: return "vSTRONG1";
		case vSUPPLY0: return "vSUPPLY0";
		case vSUPPLY1: return "vSUPPLY1";
		case vTABLE: return "vTABLE";
		case vTASK: return "vTASK";
		case vTIME: return "vTIME";
		case vTRAN: return "vTRAN";
		case vTRANIF0: return "vTRANIF0";
		case vTRANIF1: return "vTRANIF1";
		case vTRI0: return "vTRI0";
		case vTRI1: return "vTRI1";
		case vTRI: return "vTRI";
		case vTRIAND: return "vTRIAND";
		case vTRIOR: return "vTRIOR";
		case vTRIREG: return "vTRIREG";
		case vUNSIGNED: return "vUNSIGNED";
		case vUSE: return "vUSE";
		case vUWIRE: return "vUWIRE";
		case vVECTORED: return "vVECTORED";
		case vWAIT: return "vWAIT";
		case vWAND: return "vWAND";
		case vWEAK0: return "vWEAK0";
		case vWEAK1: return "vWEAK1";
		case vWHILE: return "vWHILE";
		case vWIRE: return "vWIRE";
		case vWOR: return "vWOR";
		case vXNOR: return "vXNOR";
		case vXOR: return "vXOR";
		case voANDAND: return "voANDAND";
		case voANDANDAND: return "voANDANDAND";
		case voASLEFT: return "voASLEFT";
		case voASRIGHT: return "voASRIGHT";
		case voCASEEQUAL: return "voCASEEQUAL";
		case voCASENOTEQUAL: return "voCASENOTEQUAL";
		case voEGT: return "voEGT";
		case voEQUAL: return "voEQUAL";
		case voGTE: return "voGTE";
		case voLTE: return "voLTE";
		case voMINUSCOLON: return "voMINUSCOLON";
		case voNAND: return "voNAND";
		case voNOR: return "voNOR";
		case voNOTEQUAL: return "voNOTEQUAL";
		case voOROR: return "voOROR";
		case voPLUSCOLON: return "voPLUSCOLON";
		case voPOWER: return "voPOWER";
		case voSLEFT: return "voSLEFT";
		case voSRIGHT: return "voSRIGHT";
		case voXNOR: return "voXNOR";
		case vsCLOG2: return "vsCLOG2";
		case vsDISPLAY: return "vsDISPLAY";
		case vsFINISH: return "vsFINISH";
		case vsFUNCTION: return "vsFUNCTION";
		case vsSIGNED: return "vsSIGNED";
		case vsUNSIGNED: return "vsUNSIGNED";
		default: break;
	}
	return "";
}

