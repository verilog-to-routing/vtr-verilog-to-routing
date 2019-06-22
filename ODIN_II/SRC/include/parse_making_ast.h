#include "odin_types.h"

void parse_to_ast();

/* INITIALISATIONS */
void init_parser();
void cleanup_parser();

void cleanup_hard_blocks();

void init_parser_for_file();
void cleanup_parser_for_file();


/* GENERAL PARSER NODES */
ast_node_t *newSymbolNode(char *id, int line_number);
ast_node_t *newNumberNode(char *num, bases base, signedness sign, int line_number);
ast_node_t *newList(ids type_id, ast_node_t *expression);
ast_node_t *newList_entry(ast_node_t *concat_node, ast_node_t *expression);
ast_node_t *newListReplicate(ast_node_t *exp, ast_node_t *child );
ast_node_t *markAndProcessPortWith(ids top_type, ids port_id, ids net_id, ast_node_t *port);
ast_node_t *markAndProcessParameterWith(ids top_type, ids id, ast_node_t *parameter);
ast_node_t *markAndProcessSymbolListWith(ids top_type, ids id, ast_node_t *symbol_list);

/* EXPRESSIONS */
ast_node_t *newArrayRef(char *id, ast_node_t *expression, int line_number);
ast_node_t *newArrayRef2D(char *id, ast_node_t *expression1, ast_node_t *expression2, int line_number);
ast_node_t *newRangeRef(char *id, ast_node_t *expression1, ast_node_t *expression2, int line_number);
ast_node_t *newMinusColonRangeRef(char *id, ast_node_t *expression1, ast_node_t *expression2, int line_number);
ast_node_t *newPlusColonRangeRef(char *id, ast_node_t *expression1, ast_node_t *expression2, int line_number);

ast_node_t *newRangeRef2D(char *id, ast_node_t *expression1, ast_node_t *expression2, ast_node_t *expression3, ast_node_t *expression4, int line_number);
ast_node_t *newBinaryOperation(operation_list op_id, ast_node_t *expression1, ast_node_t *expression2, int line_number);
ast_node_t *newExpandPower(operation_list op_id, ast_node_t *expression1, ast_node_t *expression2, int line_number);
ast_node_t *newUnaryOperation(operation_list op_id, ast_node_t *expression, int line_number);

/* EVENT LIST AND DELAY CONTROL */
ast_node_t *newPosedgeSymbol(char *symbol, int line_number);
ast_node_t *newNegedgeSymbol(char *symbol, int line_number);

/* STATEMENTS */
ast_node_t *newCaseItem(ast_node_t *expression, ast_node_t *statement, int line_number);
ast_node_t *newDefaultCase(ast_node_t *statement, int line_number);
ast_node_t *newNonBlocking(ast_node_t *expression1, ast_node_t *expression2, int line_number);
ast_node_t *newInitial(ast_node_t *expression1, int line_number);
ast_node_t *newBlocking(ast_node_t *expression1, ast_node_t *expression2, int line_number);
ast_node_t *newIf(ast_node_t *compare_expression, ast_node_t *true_expression, ast_node_t *false_expression, int line_number);
ast_node_t *newIfQuestion(ast_node_t *compare_expression, ast_node_t *true_expression, ast_node_t *false_expression, int line_number);
ast_node_t *newCase(ast_node_t *compare_expression, ast_node_t *case_list, int line_number);
ast_node_t *newAlways(ast_node_t *delay_control, ast_node_t *statements, int line_number);
ast_node_t *newGenerate(ast_node_t *instantiations, int line_number);
ast_node_t *newFor(ast_node_t *initial, ast_node_t *compare_expression, ast_node_t *terminal, ast_node_t *statement, int line_number);
ast_node_t *newWhile(ast_node_t *compare_expression, ast_node_t *statement, int line_number);

/* MODULE INSTANCES FUNCTIONS */
ast_node_t *newModuleConnection(char* id, ast_node_t *expression, int line_number);
ast_node_t *newModuleNamedInstance(char* unique_name, ast_node_t *module_connect_list, ast_node_t *module_parameter_list, int line_number);
ast_node_t *newFunctionNamedInstance(ast_node_t *module_connect_list, ast_node_t *module_parameter_list, int line_number);
ast_node_t *newModuleInstance(char* module_ref_name, ast_node_t *module_named_instance, int line_number);
ast_node_t *newFunctionInstance(char* function_ref_name, ast_node_t *function_named_instance, int line_number);
ast_node_t *newModuleParameter(char* id, ast_node_t *expression, int line_number);

/* FUNCTION INSTANCES FUNCTIONS */
ast_node_t *newfunctionList(ids node_type, ast_node_t *child);
ast_node_t *newParallelConnection(ast_node_t *expression1, ast_node_t *expression2, int line_number);

/* GATE INSTANCE */
ast_node_t *newGateInstance(char* gate_instance_name, ast_node_t *expression1, ast_node_t *expression2, ast_node_t *expression3, int line_number);
ast_node_t *newMultipleInputsGateInstance(char* gate_instance_name, ast_node_t *expression1, ast_node_t *expression2, ast_node_t *expression3, int line_number);
ast_node_t *newGate(operation_list gate_type, ast_node_t *gate_instance, int line_number);

/* MODULE ITEMS */
ast_node_t *newAssign(ast_node_t *statement, int line_number);
ast_node_t *newVarDeclare(char* symbol, ast_node_t *expression1, ast_node_t *expression2, ast_node_t *expression3, ast_node_t *expression4, ast_node_t *value, int line_number);
ast_node_t *newVarDeclare2D(char* symbol, ast_node_t *expression1, ast_node_t *expression2, ast_node_t *expression3, ast_node_t *expression4, ast_node_t *expression5, ast_node_t *expression6,ast_node_t *value, int line_number);
ast_node_t *newIntegerTypeVarDeclare(char* symbol, ast_node_t *expression1, ast_node_t *expression2, ast_node_t *expression3, ast_node_t *expression4, ast_node_t *value, int line_number);

/* HIGH LEVEL ITEMS */
ast_node_t *newModule(char* module_name, ast_node_t *list_of_parameters, ast_node_t *list_of_ports, ast_node_t *list_of_module_items, int line_number);
ast_node_t *newFunction(ast_node_t *list_of_ports, ast_node_t *list_of_module_items, int line_number);
void next_module();
void next_function();
ast_node_t *newDefparam(ids id, ast_node_t *val, int line_number);

void next_parsed_verilog_file(ast_node_t *file_items_list);

/* VISUALIZATION */
void graphVizOutputAst(std::string path, ast_node_t *top);
void graphVizOutputAst_traverse_node(FILE *fp, ast_node_t *node, ast_node_t *from, int from_num);
