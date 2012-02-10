/* VQM Parser Grammar File
 *
 * This file describes the grammar for the VQM file format, as documented in the
 * Altera's QUIP 4.0 package.
 *
 * Author: Tomasz Czajkowski
 * Date: September 17, 2004
 * NOTES/REVISIONS:
 * Note 1	-	In this file you will notice that often structures are freed
 *				without actually releasing memory for the data they contain.
 *				This is because the data pointers, mostly arrays and string pointers,
 *				are passed to other structures that DO NOT COPY the data over, but rather
 *				store a pointer to it. This is done to avoid unnecessary malloc calls. (TC)
 * April 21, 2005 - Modified the parser to handle bits strings and allow wysiwyg connections
 *					to be specified as concatenation statements. (TC)
 */

%{

#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
//#include <crtdbg.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "vqm_dll.h"
#include "vqm_common.h"

extern "C"
{
	int yyparse(void);
	int yylex(void);  
	int yywrap()
		{
	          return 1;
		}

}

%}

/********************************************************/
/**** DEFINE STRUCTURES *********************************/
/********************************************************/

%union
{
	int value;
	char *string;
}

/********************************************************/
/**** DEFINE TOKENS *************************************/
/********************************************************/


%token TOKEN_MODULE
%token TOKEN_ENDMODULE
//%token TOKEN_REGULARID
//%token TOKEN_ESCAPEDID
//%token TOKEN_WYSIWYGID
%token TOKEN_DEFPARAM
%token TOKEN_ASSIGN
%token TOKEN_INPUT
%token TOKEN_OUTPUT
%token TOKEN_INOUT
//%token TOKEN_INTCONSTANT
%token TOKEN_CONST_0
%token TOKEN_CONST_1
%token TOKEN_CONST_Z
//%token TOKEN_STRING
//%token TOKEN_BITSTRING
%token TOKEN_WIRE
%token <string> TOKEN_STRING TOKEN_BITSTRING TOKEN_REGULARID TOKEN_ESCAPEDID
%token <value> TOKEN_INTCONSTANT TOKEN_WYSIWYGID
%type <string> stringConstant header
%type <value> bitConstant RValue Concat ConnectionList Connection PinType IdentifierList Identifier TOKEN_CONST_0 TOKEN_CONST_1 TOKEN_WIRE TOKEN_INPUT TOKEN_OUTPUT TOKEN_INOUT

/********************************************************/
/**** DEFINE START POINT ********************************/
/********************************************************/


%start design
%%

design:		modules {}
			;

modules:	/* Empty */ {}
			|	modules module {}
			;

module:		header declarations statements footer
			{
				/* Now create a module structure and store all data you just read into it. */
				add_module($1, &pin_list, &assignment_list, &node_list);
			}
			;

header:		TOKEN_MODULE TOKEN_REGULARID '(' IdentifierList ')' ';'
			{
				t_array_ref *identifiers = (t_array_ref *) $4;
				int index;
				
				/* Free the list of ports as they are defined later on more explicitly. */
				if (identifiers != NULL)
				{
					for(index = 0; index < identifiers->array_size; index++)
					{
						t_identifier_pass *ident = (t_identifier_pass *) identifiers->pointer[index];
						
						if (ident != NULL)
						{
							free(ident->name);
							free(ident);
						}
					}
					free(identifiers->pointer);
					free(identifiers);
				}
				/* Pass back the name of the module */
				$$ = $2;				
			}
			;


declarations:	/* Empty */ {}
			|	declarations declaration {}
			;

declaration:	PinType IdentifierList ';'
				{
					/* Define action for creating a single wire */
					t_array_ref *list = (t_array_ref *) $2;

					create_pins_from_list(&list, 0, 0, (t_pin_def_type)$1, T_FALSE);
				}
			|	PinType '[' TOKEN_INTCONSTANT ':' TOKEN_INTCONSTANT ']' IdentifierList ';'
				{
					/* Define action for creating a bus */
					t_array_ref *list = (t_array_ref *) $7;

					create_pins_from_list(&list, $3, $5, (t_pin_def_type)$1, T_TRUE);
				}
			;

footer:		TOKEN_ENDMODULE	{}
			;

statements:	/* Empty */ {}
			| statements statement {}
			;

statement:		AssignStatement {}
			|	TriStatement {}
			|	WysiwygStatement {}
			|	ComponentStatement {}
			|	DefParamStatement {}
			;

AssignStatement:	TOKEN_ASSIGN Identifier '=' RValue ';'
					{
						/* Get target data reference */
						t_identifier_pass *target_id = (t_identifier_pass *) $2;
						t_pin_def *target_pin = locate_net_by_name(target_id->name);
						t_rvalue *rval = (t_rvalue *) $4;

						/* Initialize wire index for the target wire */
						int target_wire_index = target_pin->left;
						t_boolean invert_signal = T_FALSE;

						/* Get pointer to source data if used */
						t_identifier_pass *source_id = (t_identifier_pass *) (rval->data.data_structure);
						t_array_ref *ref_array = (t_array_ref *) (rval->data.data_structure);
						t_pin_def *pin;

						int wire_index;

						switch(rval->type)
						{
							case RVALUE_IDENTIFIER_BAR:
								invert_signal = T_TRUE;
							case RVALUE_IDENTIFIER:
								/* Get source wire info */
								pin = locate_net_by_name(source_id->name);
								
								if (target_id->indexed == T_TRUE)
								{
									target_wire_index = target_id->index;
								}
								/* Iterate through all wires. The trick here is count the wire indicies
								 * from left to right (either in increasing or decreasing order as may be the case). */
								for(	wire_index = pin->left;
										((pin->left > pin->right) ? (wire_index >= pin->right) : (wire_index <= pin->right));
										((pin->left > pin->right) ? wire_index-- : wire_index++))
								{
									if (source_id->indexed == T_TRUE)
									{
										wire_index = source_id->index;
									}
									add_assignment(pin, wire_index, target_pin, target_wire_index,
													T_FALSE, NULL, 0, -1, invert_signal);

									if ((target_id->indexed == T_TRUE)||(target_pin->indexed == T_FALSE))
									{
										break;
									}
									if (target_pin->left > target_pin->right)
									{
										target_wire_index--;
									}
									else
									{
										target_wire_index++;
									}
								}

								free(source_id->name);
								free(source_id);
								break;
							case RVALUE_BIT_VALUE:
								for(	target_wire_index = target_pin->left;
										((target_pin->left > target_pin->right) ? (target_wire_index >= target_pin->right) : (target_wire_index <= target_pin->right));
										((target_pin->left > target_pin->right) ? target_wire_index-- : target_wire_index++))
								{
									if (target_id->indexed == T_TRUE)
									{
										add_assignment(NULL, 0, target_pin, target_id->index,
															T_FALSE, NULL, 0, rval->data.bit_value, T_FALSE);
										break;
									}
									else
									{
										add_assignment(NULL, 0, target_pin, target_wire_index,
															T_FALSE, NULL, 0, rval->data.bit_value, T_FALSE);
									}
								}
								break;
							case RVALUE_CONCAT:
								add_concatenation_assignments(ref_array, target_pin, T_FALSE);
								break;
							default:
								printf("Should not happen!\n");
								assert(0);
								break;
						}
						free(target_id->name);
						free(target_id);
						free(rval);
					}
					;

RValue:		Identifier
			{
				t_rvalue *rval = (t_rvalue *) malloc(sizeof(t_rvalue));
				assert(rval != NULL);
				rval->type = RVALUE_IDENTIFIER;

				/* Pointer to t_identifier_pass structure */
				rval->data.data_structure = (void *) $1; 
				$$ = (long) rval;
			}
		|	'~' Identifier
			{
				t_rvalue *rval = (t_rvalue *) malloc(sizeof(t_rvalue));
				assert(rval != NULL);
				rval->type = RVALUE_IDENTIFIER_BAR;

				/* Pointer to t_identifier_pass structure */
				rval->data.data_structure = (void *) $2; 
				$$ = (long) rval;
			}
		|	bitConstant
			{
				t_rvalue *rval = (t_rvalue *) malloc(sizeof(t_rvalue));
				assert(rval != NULL);

				rval->type = RVALUE_BIT_VALUE;
				rval->data.bit_value = $1; 
				$$ = (long) rval;
			}
		|	Concat
			{
				t_rvalue *rval = (t_rvalue *) malloc(sizeof(t_rvalue));
				assert(rval != NULL);

				rval->type = RVALUE_CONCAT;
				/* Pointer to t_array_ref structure that lists all identifiers to be concatenated */
				rval->data.data_structure = (void *) $1; 
				$$ = (long) rval;
			}
		;

bitConstant:	TOKEN_CONST_0	{ $$ = $1; }
			|	TOKEN_CONST_1	{ $$ = $1; }
			;

Concat:		'{' IdentifierList '}' { $$ = $2; }
		;


TriStatement:	TOKEN_ASSIGN Identifier '=' Identifier '?' Identifier ':' TOKEN_CONST_Z ';'
				{
					t_identifier_pass *target		= (t_identifier_pass *) $2;
					t_identifier_pass *tri_control	= (t_identifier_pass *) $4;
					t_identifier_pass *source		= (t_identifier_pass *) $6;
					t_pin_def						*src_pin, *tar_pin, *tri_pin;
					int								src_index, tar_index, tri_index;

					src_pin = locate_net_by_name(source->name);
					tar_pin = locate_net_by_name(target->name);
					tri_pin = locate_net_by_name(tri_control->name);

					assert((src_pin != NULL) && (tar_pin != NULL) && (tri_pin != NULL));

					/* check pin assignments */
					src_index = source->index;
					if (source->indexed == T_FALSE)
					{
						src_index = my_min(src_pin->left, src_pin->right) - 1;	
					}
					tar_index = target->index;
					if (target->indexed == T_FALSE)
					{
						tar_index = my_min(tar_pin->left, tar_pin->right) - 1;	
					}
					tri_index = tri_control->index;
					if (tri_control->indexed == T_FALSE)
					{
						tri_index = my_min(tri_pin->left, tri_pin->right) - 1;	
					}

					/* Find pins */
					add_assignment(src_pin, src_index, tar_pin, tar_index,
									T_TRUE, tri_pin, tri_index, -1, T_FALSE);

					/* Free memory */
					free(tri_control->name);
					free(tri_control);
					free(source->name);
					free(source);
					free(target->name);
					free(target);
				}
			;

WysiwygStatement:	TOKEN_WYSIWYGID Identifier '(' ConnectionList ')' ';'
					{
						/*
						t_identifier_pass *ident = (t_identifier_pass *) $2;
						t_array_ref *all_connections = (t_array_ref *) $4;
						char *name;
						char index[20];

						if (ident->indexed == T_TRUE)
						{
							sprintf(index,"[%i]",ident->index);
						}
						else
						{
							index[0] = 0;
						}
						name = (char *) malloc(strlen(ident->name)+strlen(index)+1);
						assert(name != NULL);
						sprintf(name,"%s%s", ident->name, index);

						* Add node to the global node list *
						add_node((t_stratix_cells)$1, name, &all_connections);
						
						* Now free the identifier memory *
						free(ident->name);
						free(ident);
						*/
						assert(0);
					}
				;

ComponentStatement:	TOKEN_REGULARID Identifier '(' ConnectionList ')' ';'
					{
						t_identifier_pass *ident = (t_identifier_pass *) $2;
						t_array_ref *all_connections = (t_array_ref *) $4;
						char *name;
						char index[20];

						if (ident->indexed == T_TRUE)
						{
							sprintf(index,"[%i]",ident->index);
						}
						else
						{
							index[0] = 0;
						}
						name = (char *) malloc(strlen(ident->name)+strlen(index)+1);
						assert(name != NULL);
						sprintf(name,"%s%s", ident->name, index);

						/* Add node to the global node list */
						add_node($1, name, &all_connections);
						
						/* Now free the identifier memory */
						free(ident->name);
						free(ident);
					}
					;

DefParamStatement:	TOKEN_DEFPARAM Identifier '.' TOKEN_REGULARID '=' stringConstant ';'
					{
						/* To create a proper identifier name, append [<wire_index>] if
						 * and only if the identifier is indexed. Ignore it otherwise. */
						t_identifier_pass *identifier = (t_identifier_pass *) $2;
						define_instance_parameter(identifier, $4, $6, 0);
						free(identifier->name);
						free(identifier);						
					}
				|	TOKEN_DEFPARAM Identifier '.' TOKEN_REGULARID '=' TOKEN_INTCONSTANT ';'
					{
						/* To create a proper identifier name, append [<wire_index>] if
						 * and only if the identifier is indexed. Ignore it otherwise. */
						t_identifier_pass *identifier = (t_identifier_pass *) $2;
						define_instance_parameter(identifier, $4, NULL, $6);
						free(identifier->name);
						free(identifier);						
					}
				 ;

ConnectionList:		Connection
					{
						/* Create the first array entry for connections. Since a single connection is already an array,
						 * then just pass the array */
						$$ = $1;
					}
				|	ConnectionList ',' Connection
					{
						int index;
						
						/* Get a pointer to the list of connections */
						t_array_ref *all_connections = (t_array_ref *) $1;
						t_array_ref *new_connections = (t_array_ref *) $3;
						assert(all_connections != NULL);

						/* Add connection to the list */
						for (index = 0; index < new_connections->array_size; index++)
						{
							all_connections->array_size =
								append_array_element((long) new_connections->pointer[index],
													(long **) &(all_connections->pointer),
													all_connections->array_size);
						}
						/* Free the temporary array */
						free(new_connections->pointer);
						free(new_connections);
						/* Return list connection reference */
						$$ = (long) all_connections;
					}
				;

Connection:			'.' TOKEN_REGULARID '(' Identifier ')'
					{
						/* Create a link between a pin and a port name on a sub component (node). The identifier is
						 * only used to locate the net the port is to be associated with so free the identifier when done.
						 */
						t_identifier_pass *ident = (t_identifier_pass *) $4;
						t_array_ref *array_of_connections;
						t_array_ref *port_associations;
						int index;
						
						array_of_connections = (t_array_ref *) malloc(sizeof(t_array_ref));
						assert(array_of_connections != NULL);
						
						array_of_connections->pointer = NULL;
						array_of_connections->array_size = 0;

						port_associations = associate_identifier_with_port_name(ident, $2, -1);
						if (port_associations != NULL)
						{
							for(index = 0; index < port_associations->array_size; index++)
							{
								array_of_connections->array_size =
									append_array_element((long) port_associations->pointer[index],
															(long **) &(array_of_connections->pointer),
															array_of_connections->array_size);
							}
							free(port_associations->pointer);
							free(port_associations);
						}
						else
						{
							free($2);
						}						
						$$ = (long) array_of_connections;
						free(ident->name);
						free(ident);
					}
				|	'.' TOKEN_ESCAPEDID '(' Identifier ')'
					{
						/* Create a link between a pin and a port name on a sub component (node). The identifier is
						 * only used to locate the net the port is to be associated with so free the identifier when done.
						 */
						t_identifier_pass *ident = (t_identifier_pass *) $4;
						t_array_ref *array_of_connections;
						t_array_ref *port_associations;
						int index;
						
						array_of_connections = (t_array_ref *) malloc(sizeof(t_array_ref));
						assert(array_of_connections != NULL);
						
						array_of_connections->pointer = NULL;
						array_of_connections->array_size = 0;
						port_associations = associate_identifier_with_port_name(ident, $2, -1);
						if (port_associations != NULL)
						{
							for(index = 0; index < port_associations->array_size; index++)
							{
								array_of_connections->array_size =
									append_array_element((long) port_associations->pointer[index],
															(long **) &(array_of_connections->pointer),
															array_of_connections->array_size);
							}
							free(port_associations->pointer);
							free(port_associations);	
						}
						else
						{
							free($2);
						}
						$$ = (long) array_of_connections;
						free(ident->name);
						free(ident);
					}
				|	'.' TOKEN_REGULARID '(' Concat ')'
					{
						/* Handle the case where an identifier is actually a concatenation of wires. This is not
						 * consistent with the language definition for the VQM files but seems to happen anyways.
						 *
						 * To handle this case create a temporary wire of appropriate size. Add it to the list of nets.
						 * Once the net is created add wire assignment to associate the concatenated wires with the new net.
						 * Then, use the newly created net as the connection reference to the specified port.
						 */
						t_array_ref *ref_array = (t_array_ref *)($4);
						t_array_ref *connections;

						connections = create_wire_port_connections(ref_array, $2);
						/* The ref_array is now useless so free it up. */
						free(ref_array->pointer);
						free(ref_array);						 
						$$ = (long) connections;
					}
				|	'.' TOKEN_ESCAPEDID '(' Concat ')'
					{
						/* Handle the case where an identifier is actually a concatenation of wires. This is not
						 * consistent with the language definition for the VQM files but seems to happen anyways.
						 *
						 * To handle this case create a temporary wire of appropriate size. Add it to the list of nets.
						 * Once the net is created add wire assignment to associate the concatenated wires with the new net.
						 * Then, use the newly created net as the connection reference to the specified port.
						 */
						t_array_ref *ref_array = (t_array_ref *)($4);
						t_array_ref *connections;

						connections = create_wire_port_connections(ref_array, $2);
						 
						/* The ref_array is now useless so free it up. */
						free(ref_array->pointer);
						free(ref_array);						 
						$$ = (long) connections;
					}
				;

IdentifierList:		Identifier
					{
						/* Create the first array entry for the identifier. */
						t_array_ref *id_list = (t_array_ref *) malloc(sizeof(t_array_ref));

						assert(id_list != NULL);

						id_list->array_size = 0;
						id_list->pointer = NULL;

						/* Add the new identifier to the list */
						id_list->array_size = 
							append_array_element((long) $1, (long **) &(id_list->pointer), id_list->array_size);

						/* Return list pointer */
						$$ = (long) id_list;
					}
				|	IdentifierList ',' Identifier
					{
						/* Obtain pointer to existing list */
						t_array_ref *id_list = (t_array_ref *) $1;

						/* Add new identifier */
						id_list->array_size = 
							append_array_element((long) $3, (long **) &(id_list->pointer), id_list->array_size);

						/* Return list pointer */
						$$ = (long) id_list;
					}
				;

Identifier:		TOKEN_ESCAPEDID
				{
					/* Allocate space for an identifier. Set index to -1 since its unindexed */
					$$ = (long) allocate_identifier($1, T_FALSE, -1);
				}
			|	TOKEN_REGULARID '[' TOKEN_INTCONSTANT ']'
				{
					/* Allocate space for an identifier. Specify index used */
					$$ = (long) allocate_identifier($1, T_TRUE, $3);
				}
			|	TOKEN_REGULARID
				{
					/* Allocate space for an identifier. Set index to -1 since its unindexed */
					if (strncmp($1,RESERVED_UNUSED_CLOCK_INPUT,RESERVED_UNUSED_CLOCK_INPUT_STRING_LENGTH) == 0)
					{
						/* Eliminate dummy wires that only signify a lack of a wire */
						free($1);
						$$ = (int) NULL;
					}
					else
					{
						$$ = (long) allocate_identifier($1, T_FALSE, -1);
					}
				}
			;

stringConstant:	TOKEN_STRING	{ $$ = $1; }
			|	TOKEN_BITSTRING	{ $$ = $1; }
			;

PinType:	TOKEN_INPUT	{ $$ = $1; }
		|	TOKEN_OUTPUT { $$ = $1; }
		|	TOKEN_INOUT { $$ = $1; }
		|	TOKEN_WIRE	{ $$ = $1; }
		;


%%


/********************************************************/
/**** DEFINE ERROR HANDLER FUNCTION *********************/
/********************************************************/


int yyerror(char *s)
{
	sprintf(most_recent_error, "%s occured at line %i: %s\r\n", s, yylineno, yytext);
	printf(most_recent_error);
	return 0;
}

/*int main(void) {

  yyparse();

  return 0;

}*/
