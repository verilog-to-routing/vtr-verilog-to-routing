#ifndef PARSE_MAKING_AST_H
#define PARSE_MAKING_AST_H

#include "odin_types.h"
#include "scope_util.h"

void parse_to_ast();

/* INITIALISATIONS */
ast_t* init_parser();
void cleanup_parser();
void cleanup_hard_blocks();

/* GENERAL PARSER NODES */
ast_node_t* newSymbolNode(char* id, loc_t loc);
ast_node_t* newNumberNode(char* num, loc_t loc);
ast_node_t* newStringNode(char* num, loc_t loc);

ast_node_t* newList(ids type_id, ast_node_t* expression, loc_t loc);
ast_node_t* newList_entry(ast_node_t* concat_node, ast_node_t* expression);
ast_node_t* newListReplicate(ast_node_t* exp, ast_node_t* child, loc_t loc);
ast_node_t* markAndProcessPortWith(ids top_type, ids port_id, ids net_id, ast_node_t* port, operation_list signedness);
ast_node_t* markAndProcessParameterWith(ids id, ast_node_t* parameter, operation_list signedness);
ast_node_t* markAndProcessSymbolListWith(ids top_type, ids id, ast_node_t* symbol_list, operation_list signedness);

/* EXPRESSIONS */
ast_node_t* newArrayRef(char* id, ast_node_t* expression, loc_t loc);
ast_node_t* newArrayRef2D(char* id, ast_node_t* expression1, ast_node_t* expression2, loc_t loc);
ast_node_t* newRangeRef(char* id, ast_node_t* expression1, ast_node_t* expression2, loc_t loc);
ast_node_t* newMinusColonRangeRef(char* id, ast_node_t* expression1, ast_node_t* expression2, loc_t loc);
ast_node_t* newPlusColonRangeRef(char* id, ast_node_t* expression1, ast_node_t* expression2, loc_t loc);

ast_node_t* newRangeRef2D(char* id, ast_node_t* expression1, ast_node_t* expression2, ast_node_t* expression3, ast_node_t* expression4, loc_t loc);
ast_node_t* newBinaryOperation(operation_list op_id, ast_node_t* expression1, ast_node_t* expression2, loc_t loc);
ast_node_t* newExpandPower(operation_list op_id, ast_node_t* expression1, ast_node_t* expression2, loc_t loc);
ast_node_t* newUnaryOperation(operation_list op_id, ast_node_t* expression, loc_t loc);

/* EVENT LIST AND TIMING CONTROL */
ast_node_t* newPosedge(ast_node_t* expression, loc_t loc);
ast_node_t* newNegedge(ast_node_t* expression, loc_t loc);

/* STATEMENTS */
ast_node_t* newCaseItem(ast_node_t* expression, ast_node_t* statement, loc_t loc);
ast_node_t* newDefaultCase(ast_node_t* statement, loc_t loc);
ast_node_t* newNonBlocking(ast_node_t* expression1, ast_node_t* expression2, loc_t loc);
ast_node_t* newInitial(ast_node_t* expression1, loc_t loc);
ast_node_t* newBlocking(ast_node_t* expression1, ast_node_t* expression2, loc_t loc);
ast_node_t* newIf(ast_node_t* compare_expression, ast_node_t* true_expression, ast_node_t* false_expression, loc_t loc);
ast_node_t* newIfQuestion(ast_node_t* compare_expression, ast_node_t* true_expression, ast_node_t* false_expression, loc_t loc);
ast_node_t* newCase(ast_node_t* compare_expression, ast_node_t* case_list, loc_t loc);
ast_node_t* newAlways(ast_node_t* delay_control, ast_node_t* statements, loc_t loc);
ast_node_t* newStatement(ast_node_t* statement, loc_t loc);
ast_node_t* newFor(ast_node_t* initial, ast_node_t* compare_expression, ast_node_t* terminal, ast_node_t* statement, loc_t loc);
ast_node_t* newWhile(ast_node_t* compare_expression, ast_node_t* statement, loc_t loc);

/* MODULE INSTANCES FUNCTIONS */
ast_node_t* newModuleConnection(char* id, ast_node_t* expression, loc_t loc);
ast_node_t* newModuleNamedInstance(char* unique_name, ast_node_t* module_connect_list, ast_node_t* module_parameter_list, loc_t loc);
ast_node_t* newFunctionNamedInstance(ast_node_t* module_connect_list, ast_node_t* module_parameter_list, loc_t loc);
ast_node_t* newTaskInstance(char* task_name, ast_node_t* task_named_instace, ast_node_t* task_parameter_list, loc_t loc);
ast_node_t* newTaskNamedInstance(ast_node_t* module_connect_list, loc_t loc);
ast_node_t* newModuleInstance(char* module_ref_name, ast_node_t* module_named_instance, loc_t loc);
ast_node_t* newFunctionInstance(char* function_ref_name, ast_node_t* function_named_instance, loc_t loc);
ast_node_t* newHardBlockInstance(char* module_ref_name, ast_node_t* module_named_instance, loc_t loc);
ast_node_t* newModuleParameter(char* id, ast_node_t* expression, loc_t loc);

/* FUNCTION INSTANCES FUNCTIONS */
ast_node_t* newfunctionList(ids node_type, ast_node_t* child, loc_t loc);
ast_node_t* newParallelConnection(ast_node_t* expression1, ast_node_t* expression2, loc_t loc);

/* GATE INSTANCE */
ast_node_t* newGateInstance(char* gate_instance_name, ast_node_t* expression1, ast_node_t* expression2, ast_node_t* expression3, loc_t loc);
ast_node_t* newMultipleInputsGateInstance(char* gate_instance_name, ast_node_t* expression1, ast_node_t* expression2, ast_node_t* expression3, loc_t loc);
ast_node_t* newGate(operation_list gate_type, ast_node_t* gate_instance, loc_t loc);

/* MODULE ITEMS */
ast_node_t* newAssign(ast_node_t* statement, loc_t loc);
ast_node_t* newVarDeclare(char* symbol, ast_node_t* expression1, ast_node_t* expression2, ast_node_t* expression3, ast_node_t* expression4, ast_node_t* value, loc_t loc);
ast_node_t* newDefparamVarDeclare(char* symbol, ast_node_t* expression1, ast_node_t* expression2, ast_node_t* expression3, ast_node_t* expression4, ast_node_t* value, loc_t loc);
ast_node_t* newVarDeclare2D(char* symbol, ast_node_t* expression1, ast_node_t* expression2, ast_node_t* expression3, ast_node_t* expression4, ast_node_t* expression5, ast_node_t* expression6, ast_node_t* value, loc_t loc);
ast_node_t* newIntegerTypeVarDeclare(char* symbol, ast_node_t* expression1, ast_node_t* expression2, ast_node_t* expression3, ast_node_t* expression4, ast_node_t* value, loc_t loc);

/* CFunction */
ast_node_t* newCFunction(ids type_id, ast_node_t* arg1, ast_node_t* va_args_child, loc_t loc);
ast_node_t* newCFunction(ids type_id, ast_node_t* arg1, ast_node_t* arg2, ast_node_t* va_args_child, loc_t loc);
ast_node_t* newCFunction(ids type_id, ast_node_t* arg1, ast_node_t* arg2, ast_node_t* arg3, ast_node_t* va_args_child, loc_t loc);

/* HIGH LEVEL ITEMS */
ast_node_t* newModule(char* module_name, ast_node_t* list_of_parameters, ast_node_t* list_of_ports, ast_node_t* list_of_module_items, loc_t loc);
ast_node_t* newFunction(ast_node_t* function_return, ast_node_t* list_of_ports, ast_node_t* list_of_module_items, loc_t loc, bool automatic);
ast_node_t* newTask(char* task_name, ast_node_t* list_of_ports, ast_node_t* list_of_task_items, loc_t loc, bool automatic);
ast_node_t* newBlock(char* id, ast_node_t* block_node);

void next_module();
void next_function();
void next_task();
ast_node_t* newDefparam(ids id, ast_node_t* val, loc_t loc);

void next_parsed_verilog_file(ast_node_t* file_items_list);

/* VISUALIZATION */
void graphVizOutputAst(std::string path, ast_node_t* top);
void graphVizOutputAst_traverse_node(FILE* fp, ast_node_t* node, ast_node_t* from, int from_num);
void graphVizOutputAst_Var_Declare(FILE* fp, ast_node_t* node, int from_num);

#endif
