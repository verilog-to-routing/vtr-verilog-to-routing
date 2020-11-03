// In this file basic functions are defined. These functions can be used anywhere.
//
// Author: Tomasz Czajkowski
// Date: June 9, 2004
// NOTES/REVISIONS:
// June 9, 2004  - I took this code from my MASc thesis code. (TC)
// July 29, 2004 - Modified the remove_element_by_* functions to include preserve_order
//				   parameter. When the parameter is T_FALSE, which is default, then
//				   the removal of an element from the array moves the last array element
//				   into the position of the element being removed. This does not preeserve
//				   order. To preserve the order of elements in the array set the
//				   preserve_order flag to T_TRUE. This will force the remove_element_by_*
//				   functions to move all elements that follow the one being removed to be
//				   moved down one index location. So for i > index of element being removed
//				   then array[i-1] = array[i]. (TC)
// September 17, 2004 -	Added code to manage lists of VQM objects and their
//						interconnections. (TC)
// April 20, 2005 - Adding code to handle concatenation statements that are implicit in the
//					instantiation of a module. (TC)
// March 4, 2013 - Modifying code to perform a couting pass followed by an allocation
//                 pass to decrease memory fragmentation by reducing the number of realloc
//                 calls.  Additionally remove linear growth regieme for array allocation,
//                 as this will also cause memory fragmentaiton.
//                 Modify pointer-integer casts  ot use uintptr_t which is
//                 gaurenteed to match the size of a pointer on any platform.  This prevents
//                 bad addresses being generated due to sign extension when converting from
//                 a 32 bit integer type to a 64 bit pointer type.
//                 Also fixed various compiler warnings from casting and signed/unsigned
//                 comparisons. (Kevin Murray)
/////////////////////////////////////////////////////////////////////////////////////////////

/*******************************************************************************************/
/****************************             INCLUDES               ***************************/
/*******************************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include "vqm_dll.h"
#include "vqm_common.h"
#include "vtr_assert.h"


/*******************************************************************************************/
/****************************              MACROS                ***************************/
/*******************************************************************************************/

#define MINIMUM_ARRAY_SIZE		4
/* Defines the minimum number of elements the array is to be created for. This macro and
 * the next one are created to minimize the number of malloc / free calls when handling
 * variable size arrays. This bound must be a power of 2.
 */


#define	UPPER_GROWTH_BOUND		128
/* Once the array reaches this number of elements the growth of the array will be linear.
 * Once the array reaches this size it will be increased by the number specified above every
 * time it needs to have more memory reallocated. This is to prevent the array growth to the
 * size where almost half of the array is unused, but still takes memory space.
 * This bound must be a power of 2.
 */

#define ELEMENT_SIZE    sizeof(intptr_t)

/*******************************************************************************************/
/****************************           DECLARATIONS             ***************************/
/*******************************************************************************************/

t_array_ref *module_list = NULL;
t_array_ref *assignment_list = NULL;
t_array_ref *node_list = NULL;
t_array_ref *pin_list = NULL;
t_hash_table *pin_hash = NULL;
t_node	*most_recently_used_node = NULL;

/*  //Moved to vqm_common.h
 *void free_module(void *pointer);
 *void free_pin_def(void *pointer);
 *void free_assignment(void *pointer);
 *void free_node(void *pointer);
 *void free_parameter(void *pointer);
 *void free_port_association(void *pointer);
 */
int find_position_for_net_in_array(char *name, t_array_ref *net_list);

char most_recent_error[ERROR_LENGTH];

size_t hash_func(char* key, t_hash_table* hash_table);
t_hash_elem* get_hash_entry(char* key, t_hash_table* hash_table);
void insert_hash(char* key, size_t value, t_hash_table* hash_table);
t_index_pass find_position_for_net_in_array_by_hash(char* name, t_array_ref *net_list);
t_node_port_association *create_node_port_association(char *port_name, int port_index, t_pin_def *pin, int wire_index);
/*******************************************************************************************/
/****************************          IMPLEMENTATION            ***************************/
/*******************************************************************************************/


void vqm_data_cleanup()
/* Clean up data structures. This destroys anything within the list of modules.
 * Any other data structure is temporary and should have been handled already, so do nothing to it.
 */
{
	/* Go through the module list and free it. */
	if (module_list != NULL)
	{
		t_module **modules = (t_module **) module_list->pointer;
		int num_modules = module_list->array_size;

		if (modules != NULL)
		{
			deallocate_array(	(uintptr_t *) modules,
								num_modules,
								free_module);
		}
		free(module_list);
		module_list = NULL;
	}
	/* Set other reference lists to NULL */
	module_list = NULL;
	assignment_list = NULL;
	node_list = NULL;
	pin_list = NULL;
	most_recently_used_node = NULL;
	most_recent_error[0] = 0;
}


t_array_ref *vqm_get_module_list()
/* get the module list */
{
	return module_list;
}


void free_module(void *pointer)
/* Free the module and all data within it.
 */
{
	t_module *module = (t_module *) pointer;

	if (module != NULL)
	{
		/* Free module name */
		free(module->name);

		/* Free list connections - This includes IO ports */
		deallocate_array(	(uintptr_t *) (module->array_of_pins),
							module->number_of_pins,
							free_pin_def);

		/* Assignment statements */
		deallocate_array(	(uintptr_t *) (module->array_of_assignments),
							module->number_of_assignments,
							free_assignment);

		/* Array of nodes */
		deallocate_array(	(uintptr_t *) (module->array_of_nodes),
							module->number_of_nodes,
							free_node);

		/* Free module structure */
		free(module);
	}
}


void free_pin_def(void *pointer)
/* Free pin definition
 */
{
	t_pin_def *pin = (t_pin_def *) pointer;

	if (pin != NULL)
	{
		free(pin->name);
		free(pin);
	}
}


void free_assignment(void *pointer)
/* Free assignment statement
 */
{
	t_assign *assignment = (t_assign *) pointer;

	if (assignment != NULL)
	{
		free(assignment);
	}
}


void free_node(void *pointer)
/* Free a node
 */
{
	t_node *node = (t_node *) pointer;

	if (node != NULL)
	{
		/* Free type */
		free(node->type);

		/* Free name */
		free(node->name);

		/* Free parameters */
		deallocate_array(	(uintptr_t *) (node->array_of_params),
							node->number_of_params,
							free_parameter);

		/* Free port and wire list */
		deallocate_array(	(uintptr_t *) (node->array_of_ports),
							node->number_of_ports,
							free_port_association);

		free(node);
	}
}


void free_parameter(void *pointer)
/* Free node parameter */
{
	t_node_parameter *param = (t_node_parameter *) pointer;

	if (param != NULL)
	{
		free(param->name);

		if (param->type == NODE_PARAMETER_STRING)
		{
			free(param->value.string_value);
		}

		free(param);
	}
}


void free_port_association(void *pointer)
/* Free module port association */
{
	t_node_port_association *association = (t_node_port_association *) pointer;

	if (association != NULL)
	{
		free(association->port_name);
		free(association);
	}
}


void add_module(char *name, t_array_ref **pins, t_array_ref **assignments, t_array_ref **nodes, t_parse_info* parse_info)
/* Given a name and a list of ports, wires, cells and assignments, create a module description. */
{
	t_module *new_module;
	t_array_ref *m_pins = *pins;
	t_array_ref *m_assig = *assignments;
	t_array_ref *m_nodes = *nodes;

	new_module = (t_module *) malloc(sizeof(t_module));

	VTR_ASSERT(new_module != NULL);

	new_module->name = name;

	if (m_assig != NULL){
		new_module->array_of_assignments	= (t_assign **) m_assig->pointer;
		new_module->number_of_assignments	= m_assig->array_size;
	} else {
		new_module->array_of_assignments	= NULL;
		new_module->number_of_assignments	= 0;
	}

	if (m_pins != NULL){
		new_module->array_of_pins			= (t_pin_def **) m_pins->pointer;
		new_module->number_of_pins			= m_pins->array_size;
	} else {
		new_module->array_of_pins			= NULL;
		new_module->number_of_pins			= 0;
	}

	if (m_nodes != NULL){
		new_module->array_of_nodes			= (t_node **) m_nodes->pointer;
		new_module->number_of_nodes			= m_nodes->array_size;
	} else {
		new_module->array_of_nodes			= NULL;
		new_module->number_of_nodes			= 0;
	}

	/* Add module to the list of modules */
	if (module_list == NULL)
	{
		module_list = (t_array_ref *) malloc(sizeof(t_array_ref));
		VTR_ASSERT(module_list != NULL);
        //Allocate the size of the module list array based on the result
        //of the counting pass
		module_list->pointer = (void**) malloc(parse_info->number_of_modules*ELEMENT_SIZE);
		module_list->array_size = 0;
		module_list->allocated_size = parse_info->number_of_modules;
	}

    //Add module to list
    append_array_element((intptr_t) (new_module), module_list);

			//blah
	/* Release previous association */
	free(m_pins);
	free(m_nodes);
	free(m_assig);
	*assignments = NULL;
	*pins = NULL;
	*nodes = NULL;
}


t_pin_def *add_pin(char *name, int left, int right, t_pin_def_type type, t_parse_info* parse_info)
/* Create a new pin definition. This can be a wire or an IO port of the module.
 */
{
	t_pin_def *pin;

	/* Create new pin */
	pin = (t_pin_def *) malloc(sizeof(t_pin_def));

	VTR_ASSERT(pin != NULL);

	/* Fill in pin data */
	pin->name = name;
	pin->left = left;
	pin->right = right;
	pin->type = type;
	pin->indexed = (left != right) ? T_TRUE : T_FALSE;

	/* Create pin list if not present already */
	if (pin_list == NULL)
	{
		pin_list = (t_array_ref *) malloc(sizeof(t_array_ref));

		VTR_ASSERT(pin_list != NULL);

		pin_list->array_size = 0;
		pin_list->allocated_size = parse_info->number_of_pins;
		pin_list->pointer = (void**) malloc(parse_info->number_of_pins * ELEMENT_SIZE);
	}

    /* Create the hash table for quick name -> pin_index lookup */
    if (pin_hash == NULL) {

        pin_hash = (t_hash_table*) malloc(sizeof(t_hash_table));

        //Allocate twice as many spaces in the hash table as is needed, this should prevent
        //the table from becoming too full.
        pin_hash->table = (t_hash_elem*) malloc(2*parse_info->number_of_pins*sizeof(t_hash_elem));
        pin_hash->size = 2*parse_info->number_of_pins;
    }

    //Append the element, the position is one less thatn the size of the array
    //which is returned by append_array_element
    size_t position = append_array_element( (intptr_t) pin, pin_list) - 1;

    //Add the pin to the hash for quick name -> pin look up
    insert_hash(pin->name, position, pin_hash);

	return pin;
}


void add_assignment(t_pin_def *source, int source_index, t_pin_def *target, int target_index,
					t_boolean tristated, t_pin_def *tri_control, int tri_control_index, int constant,
                    t_boolean invert, t_parse_info* parse_info)
/* Add an assignment statement to the list of assignment statements. Use the source and target names
 * of pins to find corresponding t_pin_def references to create assignment statement.
 */
{
	int source_max = -1, source_min = -1, target_max = -1, target_min = -1, tri_min = -1, tri_max = -1;
	t_assign *assignment = NULL;

	/* Check for validity of assignments */
	VTR_ASSERT(target != NULL);
	VTR_ASSERT((tri_control != NULL) || (tristated == T_FALSE));

	if (source != NULL)
	{
		source_max = source->left;
		if (source->right > source_max)
		{
			source_min = source->left;
			source_max = source->right;
		}
		else
		{
			source_min = source->right;
		}
		if (source->left - source->right == 0)
		{
			/* In case of a single wire source, assign the entire wire. */
			source_index = source_min-1;
		}
		VTR_ASSERT((source_index >= (source_min-1) && source_index <= source_max));
	}

	target_max = target->left;
	if (target->right > target_max)
	{
		target_min = target->left;
		target_max = target->right;
	}
	else
	{
		target_min = target->right;
	}

	VTR_ASSERT((target_index >= (target_min-1) && target_index <= target_max));

	/* Check for the tristate buffer */
	if (tristated == T_TRUE)
	{
		tri_max = tri_control->left;
		if (tri_control->right > tri_max)
		{
			tri_min = tri_control->left;
			tri_max = tri_control->right;
		}
		else
		{
			tri_min = tri_control->right;
		}

		VTR_ASSERT((tri_control_index >= (tri_min-1) && tri_control_index <= tri_max));
	}

	if (target->left - target->right == 0)
	{
		/* In case of a single wire target, assign the entire wire. */
		target_index = target->left-1;
	}

	/* Create assignment statement list if not present already */
	if (assignment_list == NULL)
	{
		assignment_list = (t_array_ref *) malloc(sizeof(t_array_ref));

		VTR_ASSERT(assignment_list != NULL);
		assignment_list->array_size = 0;
		assignment_list->allocated_size = parse_info->number_of_assignments;
		assignment_list->pointer = (void**) malloc(parse_info->number_of_assignments*ELEMENT_SIZE);
	}
	if (((target_max-target_min > 0) && (target_index > target_min - 1)) || (target->indexed == T_FALSE))
	{
		/* Create an assignment statement */
		assignment = (t_assign *) malloc(sizeof(t_assign));

		VTR_ASSERT(assignment != NULL);
		assignment->source = source;
		assignment->source_index = source_index;
		assignment->target = target;
		assignment->target_index = target_index;
		assignment->is_tristated = tristated;
		assignment->tri_control = tri_control;
		assignment->tri_control_index = tri_control_index;
		assignment->value = constant;
		assignment->inversion = invert;

		/* Add this assignment statement to list of assignment statements */
        append_array_element( (intptr_t) assignment, assignment_list);
	}
	else
	{
		/* Now, if this is a bus assignment then break it up into wire-to-wire
		 * assignments for easier processing later on. */
		int wire_index;
        source_index = -1;

		if (source != NULL)
		{
			source_index = source_min;
			assert (target_max - target_min <= source_max - source_min);	/*ensure target bus is equal to or smaller than source bus*/
		}

		for(wire_index = target_min; wire_index != target_max + 1; wire_index ++)
		{
			/* Create an assignment statement */
			assignment = (t_assign *) malloc(sizeof(t_assign));

			VTR_ASSERT(assignment != NULL);
			assignment->source = source;
			assignment->source_index = 0;
			if (source != NULL)
			{
				assignment->source_index = source_index;
				source_index++;
			}
			assignment->target = target;
			assignment->target_index = wire_index;
			assignment->is_tristated = tristated;
			assignment->tri_control = tri_control;
			assignment->tri_control_index = tri_control_index;
			assignment->value = constant;
			assignment->inversion = invert;

			/* Add this assignment statement to list of assignment statements */
            append_array_element( (intptr_t) assignment, assignment_list);
		}
	}
}


void add_node(char* type, char *name, t_array_ref **ports, t_parse_info* parse_info)
/* Add node to the list of nodes. */
{
	t_array_ref *m_ports = *ports;
	t_node		*my_node;
	size_t		index;
	VTR_ASSERT(m_ports != NULL);

	/* Create new node */
	my_node = (t_node *) malloc(sizeof(t_node));

	VTR_ASSERT(my_node != NULL);

	my_node->type = type;
	my_node->name = name; /* Memory already allocated */
	my_node->array_of_params = NULL;
	my_node->number_of_params = 0;

	/* Create wire-to-wire assignments for ports, even if the input/output is a bus. */
	for(index = 0; index < m_ports->array_size; index++)
	{
		t_node_port_association *association = (t_node_port_association *) m_ports->pointer[index];
		int temp = my_min(association->associated_net->left, association->associated_net->right);

		if ((association->associated_net->left - association->associated_net->right != 0) &&
			(association->port_index == -1) && (association->wire_index < temp))
		{
			size_t counter;
            int offset;
			int change;
			int wire_index;
			t_node_port_association *new_assoc;
			t_pin_def *net = association->associated_net;

			/* This is a bus assignment to a port, so change it to a series of single wire-to-port assignments */
			offset = my_abs(association->associated_net->left - association->associated_net->right);
			association->port_index = offset - 1;
			association->wire_index = net->left;
			change = (association->associated_net->left > association->associated_net->right) ? -1:1;
			wire_index = net->left + change;
			for(counter = index+1; counter < index + offset; counter++)
			{
				new_assoc = (t_node_port_association *) malloc(sizeof(t_node_port_association));

				new_assoc->associated_net = net;
				new_assoc->port_index = counter;
				new_assoc->port_name = (char *) malloc(strlen(association->port_name)+1);
				strcpy(new_assoc->port_name, (char*)malloc(strlen(association->port_name)));
				new_assoc->wire_index = wire_index;
				wire_index += change;
				m_ports->array_size = insert_element_at_index((intptr_t) new_assoc, m_ports, counter);
			}
			index += offset + 1;
		}
	}

	/* Assign array of port-wire associations to the node */
	my_node->array_of_ports = (t_node_port_association **) m_ports->pointer;
	my_node->number_of_ports = m_ports->array_size;

	if (node_list == NULL)
	{
		node_list = (t_array_ref *)malloc(sizeof(t_array_ref));

		VTR_ASSERT(node_list != NULL);

		node_list->array_size = 0;
		node_list->allocated_size = parse_info->number_of_nodes;
		node_list->pointer = (void**) malloc(parse_info->number_of_nodes * ELEMENT_SIZE);
	}

	/* Add node to the list */
    append_array_element((intptr_t) my_node, node_list);

	/* Set most recently used node */
	most_recently_used_node = my_node;

	/* Free prior associations */
	free(m_ports);
	*ports = NULL;
}


t_identifier_pass *allocate_identifier(char *name, t_boolean indexed, int index)
/* Allocate memory for temporary identifier specification.
 */
{
	t_identifier_pass *item;

	item = (t_identifier_pass *) malloc(sizeof(t_identifier_pass));

	VTR_ASSERT(item != NULL);

	item->name = name;
	item->indexed = indexed;
	item->index = index;

	return item;
}


t_pin_def *locate_net_by_name(char *name)
/* Given a specific pin name locate a net in the circuit.
 */
{
	t_pin_def *result = NULL;

	VTR_ASSERT(pin_list != NULL);

	/*index = find_position_for_net_in_array(name, pin_list);*/
    t_index_pass index_pass = find_position_for_net_in_array_by_hash(name, pin_list);

	if (index_pass.found == T_TRUE && index_pass.index < pin_list->array_size)
	{
		result = (t_pin_def *) pin_list->pointer[index_pass.index];
		if (strcmp(result->name, name) != 0)
		{
			result = NULL;
		}
	}
	return result;
}


size_t hash_func(char* key, t_hash_table* hash_table)
/* A hash function based on the key string, and
 * size of the hash table
 */
{
    //VPRs hash function

    int i;
	int hash_val = 0, mult = 1;

	i = strlen(key);
	for (i = strlen(key) - 1; i >= 0; i--) {
		hash_val += mult * ((int) key[i]);
		mult *= 7;
	}
	hash_val += (int) key[0];
	hash_val %= hash_table->size;

	hash_val = abs(hash_val);

    return hash_val;
}

t_hash_elem* get_hash_entry(char* key, t_hash_table* hash_table)
/* Given a key, returns the hash table entry.
 * The returned entry may be either valid or empty,
 * caller must handle both cases. If the
 */
{
    //Simple hash function
    size_t hash_index = hash_func(key, hash_table);

    //The hash table entry
    t_hash_elem* entry = &hash_table->table[hash_index];

    //Check the linked list for this table entry to find the matching name
    while(entry->next != NULL) {
        if(strcmp(key, entry->key) == 0) {
            //Match
            break;
        }

        //Check the next entry in the linked list
        entry = entry->next;
    }

    return entry;
}

void insert_hash(char* key, size_t value, t_hash_table* hash_table)
/* Inserts the key, value pair into the hash_table
 */
{
    //Find the emtpy slot for this key, or the last member of the linked list
    //in the slot
    t_hash_elem* entry = get_hash_entry(key, hash_table);

    if(entry->key != NULL) {
        //Entry is in use

        if(strcmp(key, entry->key) == 0) {
            //Duplicate entry
            printf("Warning: atempting to add duplicate hash entry for key: '%s'. No action taken.\n", key);
            return; //Do nothing

        } else {
            //If we didn't match key, we should already be at the end of the linked list
            // from get_hash_entry
            VTR_ASSERT(entry->next == NULL);

            //Allocate a new entry
            t_hash_elem* new_entry = (t_hash_elem*) malloc(sizeof(t_hash_elem));

            //Add to the end of the linked list
            entry->next = new_entry;

            //Advance to the new entry
            entry = entry->next;
        }
    }

    //Now have an empty entry to work with, set the appropriate values
    entry->key = key;
    entry->value = value;
    entry->next = NULL;

    return;
}

void print_hash_stats(t_hash_table* hash_table)
/* Prints usage statistics of internal hash table
 */
{
    size_t used_table_entries = 0;
    size_t total_elements = 0;
    size_t maximum_list_length = 0;

    size_t index;
    for(index = 0; index < hash_table->size; index++) {
        t_hash_elem* entry = &hash_table->table[index];

        size_t list_length = 0;
        if(entry->key != NULL) {
            used_table_entries++;
            list_length++;

            while(entry->next != NULL) {
                entry = entry->next;
                list_length++;
            }

            if(list_length > maximum_list_length) {
                maximum_list_length = list_length;
            }

            total_elements += list_length;
        }
    }

    printf("Hash Table Stats\n");
    printf("\tTable Entry Usage: %zu/%zu (%.4f)\n", used_table_entries, hash_table->size, (float) used_table_entries/hash_table->size);
    printf("\tTotal Elements Stored: %zu\n", total_elements);
    printf("\tLongest Entry Linked List: %zu\n", maximum_list_length);
}

t_index_pass find_position_for_net_in_array_by_hash(char* name, t_array_ref* /*net_list*/)
/* Given a net name use a simple hash function to return the index into the
 * has table.  Retrieve the hash table element, which should contain the
 * index into the netlist for the named net.
 */
{
    t_index_pass net_index;

    //Init
    net_index.found = T_FALSE;
    net_index.index = 0;

    t_hash_elem* entry = get_hash_entry(name, pin_hash);

    //We now should have a matching element, the name field should not
    //be NULL.  If it is, then this name hasn't been seen before.
    if (entry->key == NULL) {
        net_index.found = T_FALSE;
        net_index.index = 0; //Set index to zero, but it isn't really valid

    } else {
        //We now have a valid entry, so return the appropriate index value
        net_index.found = T_TRUE;
        net_index.index = entry->value;
    }

    return net_index;
}

int find_position_for_net_in_array(char *name, t_array_ref *net_list)
/* Given a net name try to locate it in the list of nets. Use a binary search to
 * find the net. If the net found then return the index in the array at which it is located.
 * Otherwise, return the position in which it should be located.
 */
{
	int				left = 0;
	int				right = 0;
	size_t			index = 0;
	size_t			position = 0;
	t_pin_def		*net;

	VTR_ASSERT(net_list != NULL);
	right = net_list->array_size - 1;
	/* First check trivial cases */
	if (right < 0)
	{
		/* When empty then the solution is obvious */
		return position;
	}
	net = (t_pin_def *) net_list->pointer[right];
	if (strcmp(name, net->name) > 0)
	{
		/* When the name of the node is "greater than" the last node name then
		 * the new element should be inserted at the end.
		 */
		return net_list->array_size;
	}
	net = (t_pin_def *) net_list->pointer[left];
	if (strcmp(name, net->name) < 0)
	{
		/* When the name of the node is "less than" the first node name then
		 * the new element should be inserted at the front.
		 */
		return 0;
	}

	/* Otherwise perform binary search */
	while (left <= right)
	{
		int compare;

		/* Look at the middle element */
		index = (left + right) / 2;
		net = (t_pin_def *) net_list->pointer[index];
		compare = strcmp(name, net->name);
		if (compare > 0)
		{
			/* If name is greater than the name of the current node then search
			 * the latter half of the array */
			left = index+1;
		}
		else if (compare < 0)
		{
			/* If name is less than the name of the current node then search
			 * the earlier half of the array */
			right = index - 1;
		}
		else
		{
			/* Node is found so stop */
			break;
		}
	}
	position = index;
	if (position < net_list->array_size)
	{
		/* If the net is not found then if the record at index then determine if
		 * the net with the specified name should be at index or index+1.
		 */
		net = (t_pin_def *) net_list->pointer[position];
		if (strcmp(net->name, name) < 0)
		{
			position++;
		}
	}
	return position;
}


t_node_port_association *create_node_port_association(char *port_name, int port_index, t_pin_def *pin, int wire_index)
/* Create an association between a port_name and a pin/wire for a particular node.
 */
{
	t_node_port_association *result = NULL;

	/* Validate pin assignment */
	VTR_ASSERT(port_name != NULL);
	VTR_ASSERT(pin != NULL);
	VTR_ASSERT(((wire_index >= pin->left) && (wire_index <= pin->right)) ||
		   ((wire_index >= pin->right) && (wire_index <= pin->left)));

	/* Create association */
	result = (t_node_port_association *) malloc(sizeof(t_node_port_association));

	VTR_ASSERT(result != NULL);

	result->port_name = port_name;
	result->port_index = port_index;
	result->associated_net = pin;
	result->wire_index = wire_index;

	return result;
}


t_array_ref *associate_identifier_with_port_name(t_identifier_pass *identifier, char *port_name, int port_index)
/* Given a port and an identifier, create an association between them. An association is bascially
 * a structure that states that a named port is connected to a particular bus or a single wire from
 * a bus.
 */
{
	t_identifier_pass *ident = (t_identifier_pass *) identifier;
	t_pin_def *pin = locate_net_by_name(ident->name);
	t_array_ref *result = NULL;

	if (pin == NULL)
	{
		return NULL;
	}

	VTR_ASSERT(pin != NULL);

	result = (t_array_ref *) malloc(sizeof(t_array_ref));
	VTR_ASSERT(result != NULL);
	result->array_size = 0;
	result->allocated_size = 0;
	result->pointer = NULL;

	if (ident->indexed == T_TRUE)
	{
        append_array_element( (intptr_t) create_node_port_association(port_name, port_index, pin, ident->index), result);
	}
	else
	{
		int local_index = my_min(pin->left, pin->right)-1;
		if (pin->left - pin->right == 0)
		{
			local_index = pin->left;
            append_array_element( (intptr_t) create_node_port_association(port_name, port_index, pin, local_index), result);
		}
		else
		{
			/* In case of a bus wire being used as input and output, separate the bus into individual wires. */
			int index;
			int direction = (pin->left > pin->right) ? -1:1;
			/* Assuming that each port has its left index greater than the right index and that the right index is always 0. */
			int port = my_abs(pin->left - pin->right);
			int port_name_length = strlen(port_name);

            append_array_element( (intptr_t) create_node_port_association(port_name, port, pin, pin->left), result);
			port--;
			for(index = pin->left+direction; index != pin->right + direction; index += direction)
			{
				char *temp_port_name = (char*)malloc(port_name_length+1);

				strcpy(temp_port_name, port_name);
                append_array_element((intptr_t)	create_node_port_association(temp_port_name, port, pin, index), result);
				port--;
			}
		}
	}
	return result;
}


t_node *locate_node_by_name(char *name)
/* Look through the list of nodes and find one with a matching name. If such a node is not
 * found then return NULL.
 */
{
	size_t index;
	t_node *result = NULL;

	VTR_ASSERT(name != NULL);

	/* Perform a linear search */
	for(index = 0; index < node_list->array_size; index++)
	{
		t_node *temp_node = (t_node *) (node_list->pointer[index]);

		if (strcmp(temp_node->name, name) == 0)
		{
			result = temp_node;
			break;
		}
	}

	return result;
}


void create_pins_from_list(t_array_ref **list_of_pins, int left, int right, t_pin_def_type type, t_boolean indexed, t_parse_info* parse_info)
/* Given a list of pins to add to the module, go through the list and create a pin/wire for every
 * pin name in the list. Then free the list_of_pins array.
 */
{
	size_t index;
	t_array_ref *list = (t_array_ref *) *list_of_pins;

	/* Define action for creating a bus */
	for(index = 0; index < list->array_size; index++)
	{
		t_identifier_pass *identifier = (t_identifier_pass *) list->pointer[index];
		t_pin_def *new_pin;

		/* No need to free the name field of the identifier as it is now pointed to by the
		 * pin reference.
		 */
		new_pin = add_pin(identifier->name, left, right, type, parse_info);
		new_pin->indexed = indexed;
		free(identifier);
	}
	free(list->pointer);
	free(list);
	list = NULL;
	*list_of_pins = list;
}


t_array_ref *create_array_of_net_to_port_assignments(t_array_ref *con_array)
/* Go through a list of wires that are to be associated with the port and create a t_identifier_pass
 * structure for each wire to port connection. Return an array of such structures to the caller.
 */
{
	size_t index;
	int pin_index;
	t_identifier_pass *source;
	t_pin_def *pin;
	t_array_ref *array_of_identifiers = NULL;

	/* Create an array of identifiers to return to the caller */
	array_of_identifiers = (t_array_ref *) malloc(sizeof(t_array_ref));
	VTR_ASSERT(array_of_identifiers != NULL);
	array_of_identifiers->array_size = 0;
	array_of_identifiers->allocated_size = 0;
	array_of_identifiers->pointer = NULL;

	for (index = 0; index < con_array->array_size; index++)
	{
		source = (t_identifier_pass *) con_array->pointer[index];
		if (source == NULL)
		{
			/* This means that the source does not exist. In its place a stub wire is used to signify that the wire
			 * does not actually exist. This happens when some clock inputs to memory are not assigned, but
			 * it is necessary to assign something to the input.
			 */
            append_array_element( (intptr_t) NULL, array_of_identifiers);
		}
		else
		{
			/* Otherwise create a t_identifier_pass structure and pass it on. */
			t_identifier_pass *new_identifier;

			pin = locate_net_by_name(source->name);
			if (pin == NULL)
			{
				/* If the pin does not exist then it means that it is a dummy pin. */
                append_array_element( (intptr_t) NULL, array_of_identifiers);
			}
			else
			{
				if ((my_abs(pin->left - pin->right) > 0) && (source->indexed == T_FALSE))
				{
					int offset = ((pin->left > pin->right) ? (-1):1);
					int length = strlen(pin->name);

					for(pin_index = pin->left;
						pin_index != pin->right + offset;
						pin_index += offset)
					{
						char *name = (char *) malloc(length+1);
						strcpy(name, pin->name);
						new_identifier = allocate_identifier(name, T_TRUE, pin_index);
                        append_array_element( (intptr_t) new_identifier, array_of_identifiers);
					}
				}
				else
				{
					/* Otherwise use the old identifier pass structure */
                    append_array_element( (intptr_t) source, array_of_identifiers);
					source = NULL;
				}
			}
			/* Delete the previous t_identifier_pass structure. */
			if (source != NULL)
			{
				free(source->name);
				free(source);
				source = NULL;
			}
		}
	}

	VTR_ASSERT(array_of_identifiers != NULL);
	return array_of_identifiers;
}


t_array_ref *create_wire_port_connections(t_array_ref *concat_array, char *port_name)
/* Take an array of concatenated wires and connect each wire to the corresponding port */
{
	size_t index;
	t_array_ref *connections, *identifier_list;

	identifier_list = create_array_of_net_to_port_assignments(concat_array);

	connections = (t_array_ref *) malloc(sizeof(t_array_ref));

	VTR_ASSERT(connections != NULL);
	connections->pointer = NULL;
	connections->array_size = 0;
	connections->allocated_size = 0;

	for(index = 0; index < identifier_list->array_size; index++)
	{
		t_identifier_pass *ident = (t_identifier_pass *) identifier_list->pointer[index];
		char *temp;
		size_t association_index;
		t_array_ref *association;

		if (ident == NULL)
		{
			continue;
		}
		/* Copy the name of the port */
		temp = (char *)malloc(strlen(port_name)+1);
		strcpy(temp, port_name);

		/* Set port index that is array_size-1-index. This is to ensure that the first wire
		 * listed in the concatenation set is associated with the Most significant bit of the port.
		 */
		association = associate_identifier_with_port_name(ident, temp, identifier_list->array_size - index - 1);
		if (association != NULL)
		{
			for(association_index = 0; association_index < association->array_size; association_index++)
			{
                append_array_element(	(intptr_t)association->pointer[association_index], connections);
			}
			free(association->pointer);
			free(association);
		}
		else
		{
			free(temp);
		}
		/* Free identifier */
		free(ident->name);
		free(ident);
	}
	free(identifier_list->pointer);
	free(identifier_list);
	free(port_name);
	return connections;
}


void add_concatenation_assignments(t_array_ref *con_array, t_pin_def *target_pin, t_boolean invert_wire, t_parse_info* parse_info)
/* Create a set of concatenation assignment statements. To do this go through all the wires/buses that
 * are to be concatenated
 * and connect the source and target wires, one wire at a time. For example,
 * wire [3:0]a;
 * wire [2:0]b;
 * wire c;
 *
 * assign a = {b, c};
 *
 * We begin assigning wires of a starting at the left index (3). THe first wire to be assigned comes from
 * wire b, where we also start assignments from the left index. Thus, the first assignment is
 * assign a[3] = b[2];
 *
 * we then proceed to the next wire, be decrementing index of a by 1 and index of b by 1. This yields the second
 * assignment:
 * assign a[2] = b[1];
 *
 * And so on. The final two assignemnts are:
 * assign a[1] = b[0];
 * assign a[0] = c;
 */
{
	size_t index;
    int wire_index, target_wire_index;
	t_pin_def *pin;

	VTR_ASSERT(target_pin != NULL);

	target_wire_index = target_pin->left;
	/* Now, for each wire of the net create an assignment statement.
	 * There is an implied assumption that the list of wires concatenated into a single wire is
	 * a set of wires that DRIVE the bus. Note that this bus will be eliminated once the circuit created as
	 * the bus wire is only temporary.
	 */
	for (index = 0; index <  con_array->array_size; index++)
	{
		/* Get source wire info */
		t_identifier_pass *source = (t_identifier_pass *) con_array->pointer[index];

		if (source == NULL)
		{
			/* Go to the next wire in the target bus. */
			if (target_pin->left > target_pin->right)
			{
				target_wire_index--;
			}
			else
			{
				target_wire_index++;
			}
			continue;
		}
		pin = locate_net_by_name(source->name);

		/* Iterate through all wires. The trick here is count the wire indicies
			* from left to right (either in increasing or decreasing order).
			*/
		for(	wire_index = pin->left;
				((pin->left > pin->right) ? (wire_index >= pin->right) : (wire_index <= pin->right));
				((pin->left > pin->right) ? wire_index-- : wire_index++))
		{
			/* Create wire to wire assignment */
			add_assignment(pin, wire_index, target_pin, target_wire_index,
							T_FALSE, NULL, 0, -1, invert_wire, parse_info);

			/* Go to the next wire in the target bus. */
			if (target_pin->left > target_pin->right)
			{
				target_wire_index--;
			}
			else
			{
				target_wire_index++;
			}
		}
		free(source->name);
		free(source);
	}
	free(con_array->pointer);
	free(con_array);
}


void define_instance_parameter(t_identifier_pass *identifier, char *parameter_name, char *string_value, int integer_value)
/* This function creates a parameter for an instance of a module. */
{
	/* To create a proper identifier name, append [<wire_index>] if
		* and only if the identifier is indexed. Ignore it otherwise. */
	char *name;
	char index[20];
	t_node *local_node = NULL;
	t_node_parameter *param = NULL;

	if (identifier->indexed == T_TRUE)
	{
		sprintf(index,"[%i]",identifier->index);
	}
	else
	{
		index[0] = 0;
	}
	name = (char *) malloc(strlen(identifier->name)+strlen(index)+1);
	VTR_ASSERT(name != NULL);
	sprintf(name,"%s%s", identifier->name, index);

	/* Create a parameter for the node specified by the identifier */
	/* First check if the node is the same one as most recently used one. */

	VTR_ASSERT(most_recently_used_node != NULL);

	/* Check if the node name is the same as the name of the most recently used/created node.
	 * This is most often the case as the parameter definitions come after the module declaration. */
	if( strcmp(name, most_recently_used_node->name) == 0)
	{
		local_node = most_recently_used_node;
	}
	else
	{
		/* If not then look for a node with the specified name */
		local_node = locate_node_by_name(name);
		most_recently_used_node = local_node;
	}

	VTR_ASSERT(local_node != NULL);

	/* Add parameter for the specified node */
	param = (t_node_parameter *) malloc(sizeof(t_node_parameter));

	VTR_ASSERT(param != NULL);

	param->name = parameter_name;
	if (string_value == NULL)
	{
		param->type = NODE_PARAMETER_INTEGER;
		param->value.integer_value = integer_value;
	}
	else
	{
		param->type = NODE_PARAMETER_STRING;
		param->value.string_value = string_value;
	}

    t_array_ref* param_array_ref = append_array_element_wrapper((intptr_t) param, (intptr_t **) local_node->array_of_params, local_node->number_of_params);
    local_node->number_of_params = param_array_ref->array_size;
    local_node->array_of_params = (t_node_parameter**) param_array_ref->pointer;

	/* Release memory used by local data */
	free(name);
    free(param_array_ref);
}


/*******************************************************************************************/
/****************************         ARRAY FUNCTIONS            ***************************/
/*******************************************************************************************/


uintptr_t *allocate_array(int element_count)
/* This funciton allocates the memory for the specified array. The size is specified in
 * number of elements, not number of bytes. When allocating memory this function will
 * multiply the element_count by sizeof(int).
 */
{
	uintptr_t *array;
	int array_size;

	array_size = calculate_array_size_using_bounds(element_count);
	array = (uintptr_t *) malloc(array_size * ELEMENT_SIZE);

	VTR_ASSERT(array != NULL);

	return array;
}


void deallocate_array(uintptr_t *array, size_t element_count, void (*free_element)(void *element))
/* This funciton frees the memory allocated by the array. You must pass
 * a function that frees the memory allocated for the elements of the array.
 * If NULL is passed for that function then the elements themselves will not be freed.
 * Use with care.
 */
{
	size_t index;

	if (array != NULL)
	{
		if (free_element != NULL)
		{
			for (index = 0; index < element_count; index++)
			{
				free_element((void *) array[index]);
			}
		}
		free(array);
	}
}


void **reallocate_array(t_array_ref* array_ref, int new_element_count)
/* This funciton reallocates the memory for the specified array. The size is specified in
 * number of elements, not number of bytes. When reallocating memory this function will
 * multiply the element_count by sizeof(int).
 */
{
	void** new_array;

	new_array = (void**) realloc(array_ref->pointer, new_element_count * sizeof(long));

	VTR_ASSERT(new_array != NULL);

	return new_array;
}

t_array_ref *append_array_element_wrapper(intptr_t element, intptr_t** array, int element_count)
/* A wrapper around the append_array_element function, for allocations that
 * do not use the t_array_ref structure.
 */
{
    t_array_ref* array_ref = (t_array_ref*) malloc(sizeof(t_array_ref));
    array_ref->array_size = element_count;
    array_ref->allocated_size = element_count;
    array_ref->pointer = (void**) array;

    //Reallocates the array if needed and appends the element
    append_array_element(element, array_ref);

    //Return the new count
    return array_ref;

}

size_t append_array_element(intptr_t element, t_array_ref *array_ref)
/* This function adds an element to the end of the array. If need be the array
 * will be resized. If the array was NULL to begin with the memory will be
 * allocated to it, because realloc behaves like malloc when a NULL pointer
 * is passed to it as parameter. This function returns the new size of the array.
 */
{
    /*printf("Appending Array Element: %zu in array_ref: %p, with pointer array: %p, of capacity %d/%d\n", element, array_ref, array_ref->pointer, array_ref->array_size, array_ref->allocated_size);*/
	size_t new_allocated_size;
    size_t element_index;

	VTR_ASSERT(array_ref != NULL);

	/* Check array element size */
    new_allocated_size = calculate_array_size_using_bounds(array_ref->array_size + 1);

    if(new_allocated_size > array_ref->allocated_size) {
        array_ref->pointer = reallocate_array(array_ref, new_allocated_size);
        array_ref->allocated_size = new_allocated_size;
    }

    array_ref->array_size++; //Use one more slot
    element_index = array_ref->array_size - 1; //The last position

    array_ref->pointer[element_index] = (void*) element;

	return array_ref->array_size;
}


int remove_element_by_index(int element_index, int *array, int element_count, t_boolean preserve_order)
/* This function removes the element from the array, given this elements index.
 * At the same time it moves part of the array so that the gap created by the deleted
 * element is covered, if such a gap was created.
 */
{
	VTR_ASSERT(array != NULL);
	VTR_ASSERT(element_count > 0);

	if (preserve_order == T_TRUE)
	{
		if (element_count - 1 != element_index)
		{
			int index;

			/* Copy over unchanged elements. */
			for(index = element_index; index < element_count - 1; index++)
			{
				array[index] = array[index+1];
			}
		}
		/* If this was the last element then no work needed to be done. If it was not
		 * the last element then the new array element count is still decreased by 1. */
	}
	else
	{
		/* When ordering need not be preserved then instead of copying all
		 * elements, just move the last one in place of the current one.
		 */
		if ((element_count > 1) && (element_index < element_count-1))
		{
			array[element_index] = array[element_count-1];
		}
	}
	return element_count - 1;
}


int get_element_index(int element, int *array, int element_count)
/* This function returns the index of the element specified.
 */
{
	int index;

	VTR_ASSERT(array != NULL);
	VTR_ASSERT(element_count > 0);

	/* Search for element */
	for(index = 0; index < element_count; index++)
	{
		if (array[index] == element)
		{
			break;
		}
	}

	if (index >= element_count)
	{
		index = -1;
	}
	return index;
}


t_boolean is_element_in_array(int element, int *array, int element_count)
/* This function searches for the element in the array. If found the function returns
 * T_TRUE and T_FALSE otherwise.
 */
{
	t_boolean result = T_FALSE;
	int index;

	if ((array != NULL) && (element_count > 0))
	{
		for(index = 0; index < element_count; index++)
		{
			if (array[index] == element)
			{
				result = T_TRUE;
				break;
			}
		}
	}
	return result;
}


int remove_element_by_content(int element_content, int *array, int element_count, t_boolean preserve_order)
/* This function removes the element from the array, given this elements content.
 * It first finds the index of the element in the array then uses the remove_element_by_index
 * function to actually remove the element.
 */
{
	int index;
	int new_element_count = element_count;

	if ((array != NULL) && (element_count > 0))
	{
		index = get_element_index(element_content, array, element_count);

		if (index >= 0)
		{
			/* Element found */
			new_element_count = remove_element_by_index(index, array, element_count, preserve_order);
		}
	}
	else
	{
		new_element_count = 0;
	}
	return new_element_count;
}


int insert_element_at_index(intptr_t element, t_array_ref* array_ref, int insert_index)
/* Given an array and a new element, insert the new element at the specified location.
 * The specified location cannot be greater than the total element count and less than zero.
 */
{
	int new_element_count = 0;
    int old_element_count = array_ref->array_size;

	/*VTR_ASSERT(array_ref->pointer != NULL);*/
	VTR_ASSERT(insert_index >= 0 && insert_index <= old_element_count);

	new_element_count = append_array_element(element, array_ref);

	/* If this is not the trivial case then move the last element
	 * (the one we just added) to the position specified by insert_index parameter */
	if ((old_element_count != 0) && (insert_index != old_element_count))
	{
		int index;

		/* Move all elements starting at insert index over 1 */
		for(index = old_element_count - 1; index >= insert_index; index--)
		{
			array_ref->pointer[index+1] = array_ref->pointer[index];
		}

		/* Insert new element at the insert_index location */
		array_ref->pointer[insert_index] = (void*) element;
	}

	return new_element_count;
}


int calculate_array_size_using_bounds(int element_count)
/* This function calculates the size of the array given the element count. */
{
	int power_count = 0;
	int array_size = element_count;

	VTR_ASSERT(array_size >= 0);

    //Find the smallest power of 2 which will contain element_count elements
    while(array_size > 0)
    {
        array_size = array_size >> 1;
        power_count++;
    }

    array_size = 1 << power_count;
	return array_size;
}
