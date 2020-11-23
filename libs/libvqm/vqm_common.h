// In this file basic functions are defined. These functions can be used anywhere.
//
// Author: Tomasz Czajkowski
// Date: June 9, 2004
// NOTES/REVISIONS:
/////////////////////////////////////////////////////////////////////////////////////////////


#if !defined (__COMMON_FUNCTIONS__)
#define __COMMON_FUNCTIONS__

/*******************************************************************************************/
/****************************             INCLUDES               ***************************/
/*******************************************************************************************/

#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <stdint.h>
#include "vqm_dll.h"


/*****************************************************/
/*** MACROS DECLARATIONS   ***************************/
/*****************************************************/

#define my_min(a,b)		(a > b) ? b : a
#define my_abs(a)		(a > 0) ? a : -a

#define RESERVED_UNUSED_CLOCK_INPUT "clk_unconnected_wire"
#define RESERVED_UNUSED_CLOCK_INPUT_STRING_LENGTH 20
/* Because memory module can have up to 4 clock signals, it is possible for some of them to be left unused.
 * A wire that starts with the name specified above signifies this. */

#define ERROR_LENGTH	5000

/*****************************************************/
/*** VARIABLE DECLARATIONS ***************************/
/*****************************************************/

extern t_array_ref *module_list;
extern t_array_ref *assignment_list;
extern t_array_ref *node_list;
extern t_array_ref *pin_list;
extern t_hash_table *pin_hash;

/*******************************************************************************************/
/****************************           DECLARATIONS             ***************************/
/*******************************************************************************************/

//void 					vqm_data_cleanup();
uintptr_t			*allocate_array(int element_count);
void				deallocate_array(uintptr_t *array, size_t element_count, void (*free_element)(void *element));
void				**reallocate_array(t_array_ref* array_ref, int new_element_count);
t_array_ref         *append_array_element_wrapper(intptr_t element, intptr_t** array, int element_count);
size_t				append_array_element(intptr_t element, t_array_ref* array_ref);
int					remove_element_by_index(int element_index, int *array, int element_count, t_boolean preserve_order);
int					get_element_index(int element, int *array, int element_count);
t_boolean			is_element_in_array(int element, int *array, int element_count);
int					remove_element_by_content(int element_content, int *array, int element_count, t_boolean preserve_order);
int					insert_element_at_index(intptr_t element, t_array_ref* array_ref, int insert_index);
int					calculate_array_size_using_bounds(int element_count);
void				add_module(char *name, t_array_ref **pins, t_array_ref **assignments, t_array_ref **nodes, t_parse_info* parse_info);
t_pin_def			*add_pin(char *name, int left, int right, t_pin_def_type type, t_parse_info* parse_info);
void				add_assignment(t_pin_def *source, int source_index, t_pin_def *target, int target_index, t_boolean tristated,
						   t_pin_def *tri_control, int tri_control_index, int constant, t_boolean invert, t_parse_info* parse_info);
void				add_node(char* type, char *name, t_array_ref **ports, t_parse_info* parse_info);
t_identifier_pass	*allocate_identifier(char *name, t_boolean indexed, int index);
t_pin_def			*locate_net_by_name(char *name);
t_array_ref			*associate_identifier_with_port_name(t_identifier_pass *identifier, char *port_name, int port_index);
t_node				*locate_node_by_name(char *name);
void				create_pins_from_list(t_array_ref **list_of_pins, int left, int right, t_pin_def_type type, t_boolean indexed, t_parse_info* parse_info);
t_array_ref			*create_array_of_net_to_port_assignments(t_array_ref *con_array);
void				define_instance_parameter(t_identifier_pass *identifier, char *parameter_name, char *string_value, int integer_value);
void				add_concatenation_assignments(t_array_ref *con_array, t_pin_def *target_pin, t_boolean invert_wire, t_parse_info* parse_info);
t_array_ref			*create_wire_port_connections(t_array_ref *concat_array, char *port_name);

void print_hash_stats(t_hash_table* hash_table);

void free_module(void *pointer);
void free_pin_def(void *pointer);
void free_assignment(void *pointer);
void free_node(void *pointer);
void free_parameter(void *pointer);
void free_port_association(void *pointer);

extern t_node		*most_recently_used_node;
extern char			most_recent_error[ERROR_LENGTH];
extern char*		yytext;
extern int			yylineno;
extern FILE			*yyin;

int			yyerror(t_parse_info* parse_info, const char *s);
extern void 			yyrestart(FILE *input_file);
#endif
