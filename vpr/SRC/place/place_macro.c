/****************************************************************************************
  Y.G.THIEN
  29 AUG 2012

    This file contains functions related to placement macros. The term "placement macros"
  refers to a structure that contains information on blocks that need special treatment
  during placement and possibly routing. 
  
    An example of placement macros is a carry chain. Blocks in a carry chain have to be 
  placed in a specific orientation or relative placement so that the carry_in's and the 
  carry_out's are properly aligned. With that, the carry chains would be able to use the 
  direct connections specified in the arch file. Direct connections with the pin's 
  fc_value 0 would be treated specially in routing where the whole carry chain would be
  treated as a unit and regular routing would not be used to connect the carry_in's and 
  carry_out's. Floorplanning constraints may also be an example of placement macros.

    The function alloc_and_load_placement_macros allocates and loads the placement 
  macros in the following steps:
  (1) First, go through all the block types and mark down the pins that could possibly 
      be part of a placement macros. 
  (2) Then, go through the netlist of all the pins marked in (1) to find out all the 
      heads of the placement macros using criteria depending on the type of placement 
	  macros. For carry chains, the heads of the placement macros are blocks with 
	  carry_in's not connected to any nets (OPEN) while the carry_out's connected to the 
	  netlist with only 1 SINK.
  (3) Traverse from the heads to the tails of the placement macros and load the 
      information in the t_pl_macro data structure. Similar to (2), tails are identified 
	  with criteria depending on the type of placement macros. For carry chains, the 
	  tails are blocks with carry_out's not connected to any nets (OPEN) while the 
	  carry_in's is connected to the netlist which has only 1 SINK.

    The only placement macros supported at the moment are the carry chains with limited
  functionality. 
    
	Current support for placement macros are:
  (1) The arch parser for direct connections is working. The specifications of the direct
      connections are specified in sample_adder_arch.xml and also in the 
	  VPR_User_Manual.doc
  (2) The placement macros allocator and loader is working.
  (3) The initial placement of placement macros that respects the restrictions of the 
      placement macros is working.
  (4) The post-placement legality check for placement macros is working.
    
	Current limitations on placement macros are:
  (1) One block could only be a part of a carry chain. In the future, if a block is part
      of multiple placement macros, we should load 1 huge placement macro instead of 
	  multiple placement macros that contain the same block.
  (2) Bus direct connections (direct connections with multiple bits) are supported. 
      However, a 2-bit carry chain when loaded would become 2 1-bit carry chains.
	  And because of (1), only 1 1-bit carry chain would be loaded. In the future, 
	  placement macros with multiple-bit connections or multiple 1-bit connections 
	  should be allowed.
  (3) Placement macros that span longer or wider than the chip would cause an error. 
      In the future, we *might* expand the size of the chip to accommodate such 
	  placement macros that are crucial.

    In order for the carry chain support to work, two changes are required in the 
  arch file. 
  (1) For carry chain support, added in a new child in <layout> called <directlist>. 
      <directlist> specifies a list of available direct connections on the FPGA chip 
	  that are necessary for direct carry chain connections. These direct connections 
	  would be treated specially in routing if the fc_value for the pins is specified 
	  as 0. Note that only direct connections that has fc_value 0 could be used as a 
	  carry chain.
    
      A <directlist> may have 0 or more children called <direct>. For each <direct>, 
	  there are the following fields:
        1) name:  This specifies the name given to this particular direct connection.
        2) from_pin:  This specifies the SOURCEs for this direct connection. The format 
		              could be as following:
                       a) type_name.port_name, for all the pins in this port.
                       b) type_name.port_name [end_pin_index:start_pin_index], for a 
					      single pin, the end_pin_index and start_pin_index could be 
						  the same.
        3) to_pin:  This specifies the SINKs for this direct connection. The format is 
		            the same as from_pin. 
                    Note that the width of the from_pin and to_pin has to match.
        4) x_offset: This specifies the x direction that this connection is going from 
		             SOURCEs to SINKs.
        5) y_offset: This specifies the y direction that this connection is going from 
		             SOURCEs to SINKs. 
                     Note that the x_offset and y_offset could not both be 0.
        6) z_offset: This specifies the z sublocations that all the blocks in this 
		             direct connection to be at.
    
      The example of a direct connection specification below shows a possible carry chain 
	  connection going north on the FPGA chip:
       _______________________________________________________________________________
      | <directlist>                                                                  |
      |   <direct name="adder_carry" from_pin="adder.cout" to_pin="adder.cin"         |
	  |           x_offset="0" y_offset="1" z_offset="0"/>                            |
      | </directlist>                                                                 |
      |_______________________________________________________________________________|
	  A corresponding arch file that has this direct connection is sample_adder_arch.xml
      A corresponding blif file that uses this direct connection is adder.blif

  (2) As mentioned in (1), carry chain connections using the directs would only be 
      recognized if the pin's fc_value is 0. In order to achieve this, pin-based fc_value
	  is required. Hence, the new <fc> tag replaces both <fc_in> and <fc_out> tags.
	  
	  A <fc> tag may have 0 or more children called <pin>. For each <fc>, there are the 
	  following fields:
	    1) default_in_type: This specifies the default fc_type for input pins. They could
		                    be "frac", "abs" or "full".
		2) default_in_val: This specifies the default fc_value for input pins.
		3) default_out_type: This specifies the default fc_type for output pins. They could
		                     be "frac", "abs" or "full".
		4) default_out_val: This specifies the default fc_value for output pins.

	  As for the <pin> children, there are the following fields:
	    1) name: This specifies the name of the port/pin that the fc_type and fc_value 
		         apply to. The name have to be in the format "port_name" or 
				 "port_name [end_pin_index:start_pin_index]" where port_name is the name
				 of the port it apply to while end_pin_index and start_pin_index could
				 be specified to apply the fc_type and fc_value that follows to part of
				 a bus (multi-pin) port.
	    2) fc_type: This specifies the fc_type that would be applied to the specified pins.
		3) fc_val: This specifies the fc_value that would be applied to the specified pins.

	  The example of a pin-based fc_value specification below shows that the fc_values for
	  the cout and the cin ports are 0:
	   _______________________________________________________________________________
      | <fc default_in_type="frac" default_in_val="0.15" default_out_type="frac"      |
	  |     default_out_val="0.15">                                                   |
      |    <pin name="cin" fc_type="frac" fc_val="0"/>                                |
      |    <pin name="cout" fc_type="frac" fc_val="0"/>                               |
      | </fc>                                                                         |
	  |_______________________________________________________________________________|
      A corresponding arch file that has this direct connection is sample_adder_arch.xml
      A corresponding blif file that uses this direct connection is adder.blif

****************************************************************************************/

#include <cstdio>
#include <ctime>
#include <cmath>
using namespace std;

#include <assert.h>

#include "util.h"
#include "vpr_types.h"
#include "physical_types.h"
#include "globals.h"
#include "place.h"
#include "read_xml_arch_file.h"
#include "ReadOptions.h"
#include "place_macro.h"
#include "vpr_utils.h"


/******************** File-scope variables delcarations **********************/

/* f_idirect_from_blk_pin array allow us to quickly find pins that could be in a    *
 * direct connection. Values stored is the index of the possible direct connection  *
 * as specified in the arch file, OPEN (-1) is stored for pins that could not be    *
 * part of a direct chain conneciton.                                               *
 * [0...num_types-1][0...num_pins-1]                                                */
static int ** f_idirect_from_blk_pin = NULL;

/* f_direct_type_from_blk_pin array stores the value SOURCE if the pin is the       *
 * from_pin, SINK if the pin is the to_pin in the direct connection as specified in *
 * the arch file, OPEN (-1) is stored for pins that could not be part of a direct   *
 * chain conneciton.                                                                *
 * [0...num_types-1][0...num_pins-1]                                                */
static int ** f_direct_type_from_blk_pin = NULL;

/* f_imacro_from_blk_pin maps a blk_num to the corresponding macro index.           *
 * If the block is not part of a macro, the value OPEN (-1) is stored.              *
 * [0...num_blocks-1]                                                               */
static int * f_imacro_from_iblk = NULL;


/******************** Subroutine declarations ********************************/

static void find_all_the_macro (int * num_of_macro, int * pl_macro_member_blk_num_of_this_blk, 
		int * pl_macro_idirect, int * pl_macro_num_members, int ** pl_macro_member_blk_num);

static void free_imacro_from_iblk(void);

static void alloc_and_load_imacro_from_iblk(t_pl_macro * macros, int num_macros);

/******************** Subroutine definitions *********************************/

static void find_all_the_macro (int * num_of_macro, int * pl_macro_member_blk_num_of_this_blk, 
		int * pl_macro_idirect, int * pl_macro_num_members, int ** pl_macro_member_blk_num) {

	/* Compute required size:                                                *
	 * Go through all the pins with possible direct connections in           *
	 * f_idirect_from_blk_pin. Count the number of heads (which is the same  *
	 * as the number macros) and also the length of each macro               *
	 * Head - blocks with to_pin OPEN and from_pin connected                 *
	 * Tail - blocks with to_pin connected and from_pin OPEN                 */

	int iblk, from_iblk_pin, to_iblk_pin, from_inet, to_inet, from_idirect, to_idirect, 
			from_src_or_sink, to_src_or_sink;
	int next_iblk, next_inet, curr_inet;
	int num_blk_pins, num_macro; 
	int imember;

	num_macro = 0;
	for (iblk = 0; iblk < num_blocks; iblk++) {

		num_blk_pins = block[iblk].type->num_pins;
		for (to_iblk_pin = 0; to_iblk_pin < num_blk_pins; to_iblk_pin++) {
			
			to_inet = block[iblk].nets[to_iblk_pin];
			to_idirect = f_idirect_from_blk_pin[block[iblk].type->index][to_iblk_pin];
			to_src_or_sink = f_direct_type_from_blk_pin[block[iblk].type->index][to_iblk_pin];
			
			// Find to_pins (SINKs) with possible direct connection but are not 
			// connected to any net (Possible head of macro)
			if ( to_src_or_sink == SINK && to_idirect != OPEN && to_inet == OPEN ) {

				for (from_iblk_pin = 0; from_iblk_pin < num_blk_pins; from_iblk_pin++) {
					from_inet = block[iblk].nets[from_iblk_pin];
					from_idirect = f_idirect_from_blk_pin[block[iblk].type->index][from_iblk_pin];
					from_src_or_sink = f_direct_type_from_blk_pin[block[iblk].type->index][from_iblk_pin];

					// Find from_pins with the same possible direct connection that are connected.
					// Confirmed head of macro
					if ( from_src_or_sink == SOURCE && to_idirect == from_idirect && from_inet != OPEN) {
						
						// Mark down that this is the first block in the macro
						pl_macro_member_blk_num_of_this_blk[0] = iblk;
						pl_macro_idirect[num_macro] = to_idirect;
						
						// Increment the num_member count.
						pl_macro_num_members[num_macro]++;
						
						// Also find out how many members are in the macros, 
						// there are at least 2 members - 1 head and 1 tail.
						
						// Initialize the variables
						next_inet = from_inet;
						next_iblk = iblk;

						// Start finding the other members
						while (next_inet != OPEN) {

							curr_inet = next_inet;
							
							// Assume that carry chains only has 1 sink - direct connection
							assert(g_clbs_nlist.net[curr_inet].num_sinks() == 1);
							next_iblk = g_clbs_nlist.net[curr_inet].pins[1].block;
							
							// Assume that the from_iblk_pin index is the same for the next block
							assert (f_idirect_from_blk_pin[block[next_iblk].type->index][from_iblk_pin] == from_idirect
									&& f_direct_type_from_blk_pin[block[next_iblk].type->index][from_iblk_pin] == SOURCE);
							next_inet = block[next_iblk].nets[from_iblk_pin];

							// Mark down this block as a member of the macro
							imember = pl_macro_num_members[num_macro];
							pl_macro_member_blk_num_of_this_blk[imember] = next_iblk;

							// Increment the num_member count.
							pl_macro_num_members[num_macro]++;

						} // Found all the members of this macro at this point

						// Allocate the second dimension of the blk_num array since I now know the size
						pl_macro_member_blk_num[num_macro] = 
								(int *) my_calloc (pl_macro_num_members[num_macro] , sizeof(int));
						// Copy the data from the temporary array to the newly allocated array.
						for (imember = 0; imember < pl_macro_num_members[num_macro]; imember ++)
							pl_macro_member_blk_num[num_macro][imember] = pl_macro_member_blk_num_of_this_blk[imember];

						// Increment the macro count
						num_macro ++;

					} // Do nothing if the from_pins does not have same possible direct connection.
				} // Finish going through all the pins for from_pins.
			} // Do nothing if the to_pins does not have same possible direct connection.
		} // Finish going through all the pins for to_pins.
	} // Finish going through all blocks.
	
	// Now, all the data is readily stored in the temporary data structures.
	*num_of_macro = num_macro;
}


int alloc_and_load_placement_macros(t_direct_inf* directs, int num_directs, t_pl_macro ** macros){
	
	/* This function allocates and loads the macros placement macros   *
	 * and returns the total number of macros in 2 steps.              *
	 *   1) Allocate temporary data structure for maximum possible     *
	 *      size and loops through all the blocks storing the data     *
	 *      relevant to the carry chains. At the same time, also count *
	 *      the amount of memory required for the actual variables.    *
	 *   2) Allocate the actual variables with the exact amount of     *
	 *      memory. Then loads the data from the temporary data        *
	 *       structures before freeing them.                           *
	 *                                                                 *
	 * For pl_macro_member_blk_num, allocate for the first dimension   *
	 * only at first. Allocate for the second dimemsion when I know    *
	 * the size. Otherwise, the array is going to be of size           *
	 * num_blocks^2 (There are big benckmarks VPR that have num_blocks *
	 * in the 100k's range).                                           *
	 *                                                                 *
	 * The placement macro array is freed by the caller(s).            */

	/* Declaration of local variables */
	int imacro, imember, num_macro;
	int *pl_macro_idirect, *pl_macro_num_members, **pl_macro_member_blk_num, 
			*pl_macro_member_blk_num_of_this_blk;
	
	t_pl_macro * macro = NULL;
	
	/* Sets up the required variables. */
	alloc_and_load_idirect_from_blk_pin(directs, num_directs, 
			&f_idirect_from_blk_pin, &f_direct_type_from_blk_pin);

	/* Allocate maximum memory for temporary variables. */
	pl_macro_num_members = (int *) my_calloc (num_blocks , sizeof(int));
	pl_macro_idirect = (int *) my_calloc (num_blocks , sizeof(int));
	pl_macro_member_blk_num = (int **) my_calloc (num_blocks , sizeof(int*));
	pl_macro_member_blk_num_of_this_blk = (int *) my_calloc (num_blocks , sizeof(int));

	/* Compute required size:                                                *
	 * Go through all the pins with possible direct connections in           *
	 * f_idirect_from_blk_pin. Count the number of heads (which is the same  *
	 * as the number macros) and also the length of each macro               *
	 * Head - blocks with to_pin OPEN and from_pin connected                 *
	 * Tail - blocks with to_pin connected and from_pin OPEN                 */
	num_macro = 0;
	find_all_the_macro (&num_macro, pl_macro_member_blk_num_of_this_blk, 
			pl_macro_idirect, pl_macro_num_members, pl_macro_member_blk_num);

	/* Allocate the memories for the macro. */
	macro = (t_pl_macro *) my_malloc (num_macro * sizeof(t_pl_macro));

	/* Allocate the memories for the chaim members.             *
	 * Load the values from the temporary data structures.      */
	for (imacro = 0; imacro < num_macro; imacro++) {
		macro[imacro].num_blocks = pl_macro_num_members[imacro];
		macro[imacro].members = (t_pl_macro_member *) my_malloc 
										(macro[imacro].num_blocks * sizeof(t_pl_macro_member));

		/* Load the values for each member of the macro */
		for (imember = 0; imember < macro[imacro].num_blocks; imember++) {
			macro[imacro].members[imember].x_offset = imember * directs[pl_macro_idirect[imacro]].x_offset;
			macro[imacro].members[imember].y_offset = imember * directs[pl_macro_idirect[imacro]].y_offset;
			macro[imacro].members[imember].z_offset = directs[pl_macro_idirect[imacro]].z_offset;
			macro[imacro].members[imember].blk_index = pl_macro_member_blk_num[imacro][imember];
		}
	}

	/* Frees up the temporary data structures. */
	free(pl_macro_num_members);
	free(pl_macro_idirect);
	for(imacro=0; imacro < num_macro; imacro++) {
		free(pl_macro_member_blk_num[imacro]);
	}
	free(pl_macro_member_blk_num);
	free(pl_macro_member_blk_num_of_this_blk);
	
	/* Returns the pointer to the macro by reference. */
	*macros = macro;
	return (num_macro);

}

void get_imacro_from_iblk(int * imacro, int iblk, t_pl_macro * macros, int num_macros) {

	/* This mapping is needed for fast lookup's whether the block with index *
	 * iblk belongs to a placement macro or not.                             *
	 *                                                                       *
	 * The array f_imacro_from_iblk is used for the mapping for speed reason *
	 * [0...num_blocks-1]                                                    */

	/* If the array is not allocated and loaded, allocate it.                */ 
	if (f_imacro_from_iblk == NULL) {
		alloc_and_load_imacro_from_iblk(macros, num_macros);
	}

	/* Return the imacro for the block. */
	*imacro = f_imacro_from_iblk[iblk];

}

static void free_imacro_from_iblk(void) {

	/* Frees the f_imacro_from_iblk array.                    *
	 *                                                        *
	 * This function is called when the arrays are freed in   *
	 * free_placement_structs()                               */

	if (f_imacro_from_iblk != NULL) {
		free(f_imacro_from_iblk);
		f_imacro_from_iblk = NULL;
	}

}

static void alloc_and_load_imacro_from_iblk(t_pl_macro * macros, int num_macros) {

	/* Allocates and loads imacro_from_iblk array.                           *
	 *                                                                       *
	 * The array is freed in free_placement_structs()                        */

	int * temp_imacro_from_iblk = NULL;
	int imacro, imember, iblk;

	/* Allocate and initialize the values to OPEN (-1). */
	temp_imacro_from_iblk = (int *)my_malloc(num_blocks * sizeof(int));
	for(iblk = 0; iblk < num_blocks; iblk ++) {
		temp_imacro_from_iblk[iblk] = OPEN;
	}
	
	/* Load the values */
	for (imacro = 0; imacro < num_macros; imacro++) {
		for (imember = 0; imember < macros[imacro].num_blocks; imember++) {
			iblk = macros[imacro].members[imember].blk_index;
			temp_imacro_from_iblk[iblk] = imacro;
		}
	}
	
	/* Sets the file_scope variables to point at the arrays. */
	f_imacro_from_iblk = temp_imacro_from_iblk;
}

void free_placement_macros_structs(void) {

	/* This function frees up all the static data structures used. */

	// This frees up the two arrays and set the pointers to NULL
	int itype;
	if ( f_idirect_from_blk_pin != NULL ) {
		for (itype = 1; itype < num_types; itype++) {
			free(f_idirect_from_blk_pin[itype]);
		}
		free(f_idirect_from_blk_pin);
		f_idirect_from_blk_pin = NULL;
	}

	if ( f_direct_type_from_blk_pin != NULL ) {
		for (itype = 1; itype < num_types; itype++) {
			free(f_direct_type_from_blk_pin[itype]);
		}
		free(f_direct_type_from_blk_pin);
		f_direct_type_from_blk_pin = NULL;
	}

	// This frees up the imacro from iblk mapping array.
	free_imacro_from_iblk();
	
}
