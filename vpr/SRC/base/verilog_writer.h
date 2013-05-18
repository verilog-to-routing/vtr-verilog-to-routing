#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include "util.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "globals.h"
#include "read_place.h"
#include "draw.h"
#include "stats.h"
#include "check_route.h"
#include "rr_graph.h"
#include "path_delay.h"
#include "net_delay.h"
#include "timing_place.h"
#include "read_xml_arch_file.h"
#include "ReadOptions.h"
#include "physical_types.h"
#include "globals.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"

/* 
verilog_writer.c defines the main functions used to: 
   
   1) identify the primitives in a design
   2) find the connectivity between primitives
   3) Write the verilog code representing the post-synthesized (packed,placed,routed) design consisting of LUTs, IOs, Flip Flops, Multiplier blocks, and RAM blocks
   4) Write the Standard Delay Format(SDF) file corresponding to the verilog code mentioned in (3). The SDF file contains all the timing information of the design which 
      allows the user to perform timing simulation
*/




/* ***************************************************************************************************************
   The pb_list data structure is used in a linked list by the function traverse_clb that will find primitives.
   This data structure represents a single primitive.

   pb: A pointer to the t_pb data structure representing the primitive.
   pb_graph: A pointer to the t_pb_graph_node representing the primitive.
   driver_pin: (Only applicable to "find_connected_primitives_downhill" & "find_connected_primitives_uphill")
               A pointer to the t_pb_graph_pin data structure corresponding to the pin that drives the signal on the load_pin of this pb primitive.
	       The port index and pin number for the pin is accessible through this data structure.
   load_pin: (Only applicable to "find_connected_primitives_downhill" & "find_connected_primitives_uphill")
             A pointer to the t_pb_graph_pin data structure corresponding to the input pin that receives the signal from the driver_pin.
             The port index and pin number for the pin is accessible through this data structure.
   next: pointer to the next pb primitive found in the linked list*/
typedef struct found_pins{

  t_pb *pb;

  struct found_pins *next;

}pb_list;


/* ***************************************************************************************************************
   The conn_list data structure is used in a linked list by functions that will be used by functions that will find the connectivity between primitives.
   This data structure represents a single driver to load pair of primitives.

   driver_pb: A pointer to the t_pb data structure representing the driver primitive.
   load_pb: A pointer to the t_pb data structure representing the load primitive.
   driver_pin: A pointer to the t_pb_graph_pin data structure corresponding to the pin that drives the signal on the load_pin of this pb primitive.
	       The port index and pin number for the pin is accessible through this data structure.
   load_pin: A pointer to the t_pb_graph_pin data structure corresponding to the input pin that receives the signal from the driver_pin.
             The port index and pin number for the pin is accessible through this data structure.
   driver_to_load_delay: The delay, in seconds, for a signal to propagate from the driver pin to the load pin.
   next: pointer to the next driver-load pair found.
*/
typedef struct found_connectivity{

  t_pb *driver_pb;
  t_pb *load_pb;

  t_pb_graph_pin *driver_pin;
  t_pb_graph_pin *load_pin;
  
  float driver_to_load_delay;

  struct found_connectivity *next;

}conn_list;


/*The verilog_writer function is the main function that will generate and write to the verilog and SDF files
  Al the functions declared bellow are called directly or indirectly by verilog_writer.c
  net_delay is a float 2D array containing the inter clb delay information.*/
void verilog_writer(void);

/*The traverse_clb function returns a linked list of all the primitives inside a complex block.
  These primitives may be LUTs , FlipFlops , etc.
  block_num: the block number for the complex block.
  pb: The t_pb data structure corresponding to the complex block. (i.e block[block_num].pb)
  prim_list: A pin_list pointer corresponding to the head of the linked list. The function will populate this head pointer.*/
pb_list *traverse_clb(t_pb *pb, pb_list *prim_list);


/*The find_connected_primitives_downhill function will return a linked list of all the primitives that a particular primitive connects to.
  block_num: the block number of the complex block that the primitive resides in.
  pb: A pointer to the t_pb data structure that represents the primitive (not the complex block).
  list: A head pointer to the start of a linked list. This function will populate the linked list. The linked list can be empty (i.e list=NULL)
        or contain other primitives.*/
conn_list *find_connected_primitives_downhill(int block_num , t_pb *pb , conn_list *list);

/*The function insert_to_linked_list inserts a new primitive to the pb_list type linked list pointed by "list".*/
pb_list *insert_to_linked_list(t_pb *pb_new , pb_list *list);

/*The function insert_to_linked_list_conn inserts a new primitive to the conn_list type linked list pointed by "list".*/
conn_list *insert_to_linked_list_conn(t_pb *driver_new , t_pb *load_new , t_pb_graph_pin *driver_pin_ , t_pb_graph_pin *load_pin_ , float path_delay , conn_list *list);

/*The traverse_linked_list function prints the entire pb_list type linked list pointed to by "list"*/
void traverse_linked_list(pb_list *list);

/*The traverse_linked_list_conn function prints the entire conn_list type linked list pointed to by "list"*/
void traverse_linked_list_conn(conn_list *list);

/*The free_linked_list function frees the memory used by the pb_list type linked list pointed to by "list"*/
pb_list *free_linked_list(pb_list *list);

/*The free_linked_list_conn function frees the memory used by the conn_list type linked list pointed to by "list"*/
conn_list *free_linked_list_conn(conn_list *list);

/*The function instantiate_top_level_module instantiates the top level verilog module of the post-synthesized circuit and the list of inputs and outputs to that module*/
void instantiate_top_level_module(FILE *Verilog);

/*The instantiate_wires function instantiates all the wire in the post-synthesized design*/
void instantiate_wires(FILE *Verilog);

/*The function instantiate_input_interconnect will instantiate the interconnect segments from input pins to the rest of the design*/
void instantiate_input_interconnect(FILE *Verilog , FILE *SDF , char *clock_name);

/*This function instantiates the interconnect modules that connect from the output pins of the primitive "pb" to whatever it connects to*/
void instantiate_interconnect(FILE *Verilog , int block_num , t_pb *pb , FILE *SDF);

/*This function instantiates the primitive verilog modules e.g. LUTs ,Flip Flops, etc.*/
void instantiate_primitive_modules(FILE *Verilog , char *clock_name , FILE *SDF);

/*This function returns the truth table corresponding to the LUT primitive represented by "pb"*/
char *load_truth_table(int inputs , t_pb *pb);

/*The names of some primitives contain certain characters that would cause syntax errors in Verilog (e.g. '^' , '~' , '[' , etc. ). This function returns a new string
  with those illegal characters removed and replaced with '_'*/
char *fix_name(char *name);

/*This function finds the number of inputs to a primitive.*/
int find_number_of_inputs(t_pb *pb);

/*This function is a utility function used by load_truth_table and load_truth_table_new functions. It will return the index of a particular row in the truth table*/
int find_index(char *row,int inputs);

/*This function is a utility function called by intantiate_interconnect*/
void interconnect_printing(FILE *fp , conn_list *downhill);

/*This function will instantiate the header of te Standar Delay Format (SDF) file.*/
void instantiate_SDF_header(FILE *SDF);

/*This funciton will instantiate the SDF cell that contains the delay information of the Verilog interconnect modules*/
void SDF_interconnect_delay_printing(FILE *SDF , conn_list *downhill);

/*This function instantiates the SDF cell that contains the delay information of a LUT*/
void sdf_LUT_delay_printing(FILE *SDF , t_pb *pb);

/*This function instantiates the SdF cell that contains the delay information of a Flip Flop*/
void sdf_DFF_delay_printing(FILE *SDF , t_pb *pb);

/*This function instantiates the SdF cell that contains the delay information of a Multiplier*/
void SDF_Mult_delay_printing(FILE *SDF , t_pb *pb);

/*This function instantiates the SdF cell that contains the delay information of a Adder*/
void SDF_Adder_delay_printing(FILE *SDF , t_pb *pb);

/*Finds and returns the name of the clock signal int he circuit*/
char *find_clock_name(void);

/*This function instantiates the SdF cell that contains the delay information of a Single_port_RAM*/
void SDF_ram_single_port_delay_printing(FILE *SDF , t_pb *pb);

/*This function instantiates the SdF cell that contains the delay information of a Dual_port_RAM*/
void SDF_ram_dual_port_delay_printing(FILE *SDF , t_pb *pb);
