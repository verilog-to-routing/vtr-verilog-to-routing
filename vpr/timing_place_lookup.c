#include <stdio.h>
#include <math.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "route_common.h"
#include "place_and_route.h"
#include "route_tree_timing.h"
#include "route_timing.h"
#include "timing_place_lookup.h"
#include "rr_graph.h"
#include "route_export.h"


/*this file contains routines that generate the array containing*/
/*the delays between blocks, this is used in the timing driven  */
/*placement routines */

/*To compute delay between blocks we place temporary blocks at */
/*different locations in the FPGA and route nets  between      */
/*the blocks.  From this procedure we generate a lookup table  */
/*which tells us the delay between different locations in      */
/*the FPGA */

/*Note: if you do not have logically equivalent output and    */
/*inputs, some of this code must be re-written.               */

/*Note: these routines assume that there is a uniform and even */
/*distribution of the different wire segments. If this is not  */
/*the case, then this lookup table will be off */

#define NET_COUNT 1 /*we only use one net in these routines,   */
                    /*it is repeatedly routed and ripped up    */
                    /*to compute delays between different      */
                    /*locations, this value should not change  */
#define NET_USED 0 /*we use net at location zero of the net    */
                   /*structure                                 */
#define NET_USED_SOURCE_BLOCK 0  /*net.block[0] is source block*/
#define NET_USED_SINK_BLOCK 1    /*net.block[1] is sink block */
#define SOURCE_BLOCK 0 /*block[0] is source */
#define SINK_BLOCK 1 /*block[1] is sink*/

#define BLOCK_COUNT 2 /*use 2 blocks to compute delay between  */
                      /*the various FPGA locations             */
                      /*do not change this number unless you   */
                      /*really know what you are doing, it is  */
                      /*assumed that the net only connects to  */
                      /*two blocks*/

#define DEBUG_TIMING_PLACE_LOOKUP /*initialize arrays to known state*/

#define DUMPFILE "lookup_dump.echo"
/*#define PRINT_ARRAYS*/ /*only used during debugging, calls routine to  */
                     /*print out the various lookup arrays           */

/***variables that are exported to other modules***/

/*the delta arrays are used to contain the best case routing delay */
/*between different locations on the FPGA. */


float **delta_inpad_to_clb;
float **delta_clb_to_clb;
float **delta_clb_to_outpad;
float **delta_inpad_to_outpad;


/*** Other Global Arrays ******/
/* I could have allocated these as local variables, and passed them all */
/* around, but was too lazy, since this is a small file, it should not  */
/* be a big problem */

static float **net_delay;
static float **net_slack;
static float *pin_criticality;
static int *sink_order;
static t_rt_node **rt_node_of_sink;

t_ivec **clb_opins_used_locally;

static FILE *lookup_dump; /*if debugging mode is on, print out to   */
			    /*the file defined in DUMPFILE */

/*** Function Prototypes *****/

static void alloc_net(void);

static void alloc_block(void);

static void alloc_and_assign_internal_structures(struct s_net **original_net, 
					  struct s_block **original_block, 
					  int *original_num_nets, 
					  int *original_num_blocks);

static void free_and_reset_internal_structures(struct s_net *original_net, 
 					struct s_block *original_block, 
					int original_num_nets, 
					int original_num_blocks);

static void setup_chan_width (struct s_router_opts router_opts,
		       t_chan_width_dist chan_width_dist);

static void alloc_routing_structs(struct s_router_opts router_opts, 
			   struct s_det_routing_arch det_routing_arch, 
			   t_segment_inf *segment_inf, 
			   t_timing_inf timing_inf, t_subblock_data subblock_data);

static void free_routing_structs (struct s_router_opts router_opts, 
			   struct s_det_routing_arch det_routing_arch, 
			   t_segment_inf *segment_inf, 
			   t_timing_inf timing_inf
			   );

static void assign_locations (enum e_block_types source_type, 
			      int source_x_loc, int source_y_loc, 
			      enum e_block_types sink_type,
			      int sink_x_loc, int sink_y_loc);

static float assign_blocks_and_route_net (enum e_block_types source_type, 
				   int source_x_loc, int source_y_loc, 
				   enum e_block_types sink_type,
				   int sink_x_loc, int sink_y_loc,
				   struct s_router_opts router_opts,
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf);

static void alloc_delta_arrays(void);

static void free_delta_arrays(void);

static void generic_compute_matrix(float ***matrix_ptr, 
				   enum e_block_types source_type, 
				   enum e_block_types sink_type, int source_x, 
				   int source_y, int start_x,  
				   int end_x, int start_y, int end_y, 
				   struct s_router_opts router_opts,
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf);

static void compute_delta_clb_to_clb (struct s_router_opts router_opts,
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf, int longest_length);

static void compute_delta_inpad_to_clb (struct s_router_opts router_opts,
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf);

static void compute_delta_clb_to_outpad(struct s_router_opts router_opts,
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf);

static void compute_delta_inpad_to_outpad(struct s_router_opts router_opts,
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf);

static void compute_delta_arrays (struct s_router_opts router_opts,
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf, int longest_length);

static int get_first_pin(enum e_pin_type pintype);

static int get_longest_segment_length(struct s_det_routing_arch det_routing_arch,
				      t_segment_inf *segment_inf);
static void init_occ(void);

#ifdef PRINT_ARRAYS
static void  print_array(float **array_to_print, int x1, int x2, int y1, int y2);
#endif
/**************************************/
static int  get_first_pin(enum e_pin_type pintype) {
  /*this code assumes logical equivilance between all driving pins*/
  /*global pins are not hooked up to the temporary net */

   int i, currpin;

  currpin = 0;
  for (i=0; i<num_class; i++) {
    if (class_inf[i].type == pintype && !is_global_clb_pin[currpin]) 
      return(class_inf[i].pinlist[0]);
    else 
      currpin += class_inf[i].num_pins;
  } 
  exit(0); /*should never hit this line*/
}

/**************************************/
static int get_longest_segment_length(struct s_det_routing_arch det_routing_arch,
				      t_segment_inf *segment_inf){

  int i,length;
  
  length = 0;
  for (i=0; i<det_routing_arch.num_segment; i++) {
    if (segment_inf[i].length > length)
      length = segment_inf[i].length;
  }
  return(length);
}

/**************************************/
static void alloc_net(void) {

  int i, len;

  net = (struct s_net *) my_malloc (num_nets * sizeof(struct s_net));
  for (i=0; i<NET_COUNT; i++) {

    len = strlen ("TEMP_NET");
    net[i].name = (char *) my_malloc ((len + 1) * sizeof(char));
    strcpy (net[NET_USED].name,"TEMP_NET");

    net[i].num_pins = BLOCK_COUNT;
    net[i].blocks = (int *) my_malloc(BLOCK_COUNT * sizeof(int));
    net[i].blocks[NET_USED_SOURCE_BLOCK] = NET_USED_SOURCE_BLOCK;  /*driving block*/
    net[i].blocks[NET_USED_SINK_BLOCK] = NET_USED_SINK_BLOCK;  /*target block */

    net[i].blk_pin =  (int *) my_malloc(BLOCK_COUNT * sizeof(int));
    /*the values for this are allocated in assign_blocks_and_route_net*/

  }
}
/**************************************/
static void alloc_block(void) {

  /*allocates block structure, and assigns values to known parameters*/
  /*type and x,y fields are left undefined at this stage since they  */
  /*are not known until we start moving blocks through the clb array */

  int ix_b, ix_p, len;

  block = (struct s_block *) my_malloc (num_blocks * sizeof(struct s_block));

  for (ix_b=0;ix_b<BLOCK_COUNT;ix_b++){ 
    len = strlen ("TEMP_BLOCK");
    block[ix_b].name = (char *) my_malloc ((len + 1) * sizeof(char));
    strcpy (block[ix_b].name, "TEMP_BLOCK");

    block[ix_b].nets = (int *) my_malloc (pins_per_clb * sizeof(int));
    block[ix_b].nets[0] = 0;
    for (ix_p=1; ix_p < pins_per_clb; ix_p++) 
      block[ix_b].nets[ix_p] = OPEN;
  }
}
/**************************************/
static void init_occ(void){ 
  
  int i, j;

  for (i=0;i<=nx+1;i++) {
    for (j=0;j<=ny+1; j++) {
      clb[i][j].occ = 0;
    }
  }
}
/**************************************/
static void alloc_and_assign_internal_structures(struct s_net **original_net, 
					  struct s_block **original_block, 
					  int *original_num_nets, 
					  int *original_num_blocks) {
  /*allocate new data structures to hold net, and block info*/

  *original_net = net;
  *original_num_nets = num_nets;
  num_nets = NET_COUNT;
  alloc_net();

  *original_block = block;
  *original_num_blocks = num_blocks;
  num_blocks = BLOCK_COUNT;
  alloc_block();

  
  /* [0..num_nets-1][1..num_pins-1] */
  net_delay = (float**) alloc_matrix(0,NET_COUNT-1,1,BLOCK_COUNT-1,sizeof(float));
  net_slack = (float**) alloc_matrix(0,NET_COUNT-1,1,BLOCK_COUNT-1,sizeof(float));

  init_occ();

}

/**************************************/
static void free_and_reset_internal_structures(struct s_net *original_net, 
					struct s_block *original_block, 
					int original_num_nets, 
					int original_num_blocks) {
  /*reset gloabal data structures to the state that they were in before these*/
  /*lookup computation routines were called */

  int i;


  /*there should be only one net to free, but this is safer*/
  for (i=0; i<NET_COUNT; i++) {
    free (net[i].name);
    free (net[i].blocks);
    free (net[i].blk_pin);
  }
  free (net);
  net = original_net;

  for (i=0; i< BLOCK_COUNT; i++) {
    free (block[i].name);
    free (block[i].nets); 
  }
  free (block);
  block = original_block;
  
  num_nets = original_num_nets;
  num_blocks = original_num_blocks;
  
  free_matrix (net_delay, 0,NET_COUNT-1,1, sizeof(float));
  free_matrix (net_slack, 0,NET_COUNT-1,1, sizeof(float));

}
/**************************************/
static void setup_chan_width (struct s_router_opts router_opts,
		       t_chan_width_dist chan_width_dist) {
  /*we give plenty of tracks, this increases routability for the */
  /*lookup table generation */ 

  int width_fac; 

  if (router_opts.fixed_channel_width == NO_FIXED_CHANNEL_WIDTH)
    width_fac = 4 * pins_per_clb; /*this is 2x the value that binary search starts*/
                                  /*this should be enough to allow most pins to   */
                                  /*connect to tracks in the architecture */
  else
    width_fac = router_opts.fixed_channel_width;

  init_chan(width_fac, chan_width_dist);
}
/**************************************/
static void alloc_routing_structs (struct s_router_opts router_opts, 
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf,
                                   t_subblock_data subblock_data) {

  int bb_factor;

  /*calls routines that set up routing resource graph and associated structures*/


  /*must set up dummy blocks for the first pass through*/
  assign_locations(CLB, 1, 1, CLB, nx, ny);

  clb_opins_used_locally = alloc_route_structs(subblock_data);

  free_rr_graph();
  build_rr_graph (router_opts.route_type, det_routing_arch, segment_inf,
		  timing_inf, router_opts.base_cost_type);
  alloc_and_load_rr_node_route_structs();

  alloc_timing_driven_route_structs (&pin_criticality, &sink_order, &rt_node_of_sink);


  bb_factor = nx + ny; /*set it to a huge value*/
  init_route_structs(bb_factor);


}
/**************************************/
static void free_routing_structs (struct s_router_opts router_opts, 
			   struct s_det_routing_arch det_routing_arch, 
			   t_segment_inf *segment_inf, 
			   t_timing_inf timing_inf
			   ) {

  free_rr_graph_internals (router_opts.route_type, det_routing_arch, segment_inf,
		  timing_inf, router_opts.base_cost_type);
  free_rr_graph();

  free_rr_node_route_structs();
  free_route_structs(clb_opins_used_locally);
  free_trace_structs();

  free_timing_driven_route_structs (pin_criticality, sink_order, rt_node_of_sink);

}
/**************************************/
static void assign_locations (enum e_block_types source_type, 
			      int source_x_loc, int source_y_loc, 
			      enum e_block_types sink_type,
			      int sink_x_loc, int sink_y_loc) {
  /*all routing occurs between block 0 (source) and block 1 (sink)*/
  int isubblk;


  block[SOURCE_BLOCK].type = source_type;
  block[SINK_BLOCK].type = sink_type;

  block[SOURCE_BLOCK].x = source_x_loc;
  block[SOURCE_BLOCK].y = source_y_loc;
  
  block[SINK_BLOCK].x = sink_x_loc;
  block[SINK_BLOCK].y = sink_y_loc;


  if (source_type == CLB){
    net[NET_USED].blk_pin[NET_USED_SOURCE_BLOCK] = get_first_pin(DRIVER);
    clb[source_x_loc][source_y_loc].u.block = SOURCE_BLOCK;
    clb[source_x_loc][source_y_loc].occ += 1;
  }
  else {
     net[NET_USED].blk_pin[NET_USED_SOURCE_BLOCK] = OPEN;
     isubblk = clb[source_x_loc][source_y_loc].occ;
     clb[source_x_loc][source_y_loc].u.io_blocks[isubblk] = SOURCE_BLOCK;
     clb[source_x_loc][source_y_loc].occ += 1;
  }
  if (sink_type == CLB){
    net[NET_USED].blk_pin[NET_USED_SINK_BLOCK] = get_first_pin(RECEIVER);
    clb[sink_x_loc][sink_y_loc].u.block = SINK_BLOCK;
    clb[sink_x_loc][sink_y_loc].occ += 1;
  }
  else{
     net[NET_USED].blk_pin[NET_USED_SINK_BLOCK] = OPEN;
     isubblk = clb[sink_x_loc][sink_y_loc].occ;
     clb[sink_x_loc][sink_y_loc].u.io_blocks[isubblk] = SINK_BLOCK;
     clb[sink_x_loc][sink_y_loc].occ += 1;
  }
}
/**************************************/
static float assign_blocks_and_route_net (enum e_block_types source_type, 
				   int source_x_loc, int source_y_loc, 
				   enum e_block_types sink_type,
				   int sink_x_loc, int sink_y_loc,
				   struct s_router_opts router_opts,
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf
				   ){

  /*places blocks at the specified locations, and routes a net between them*/
  /*returns the delay of this net */

  boolean is_routeable;
  int ipin;
  float pres_fac, T_crit;
  float net_delay_value;
  int **rr_node_indices;
  int nodes_per_chan;



  net_delay_value = IMPOSSIBLE; /*set to known value for debug purposes*/

  assign_locations( source_type, source_x_loc, source_y_loc, 
		 sink_type, sink_x_loc, sink_y_loc);

  rr_node_indices = get_rr_node_indices();
  nodes_per_chan = get_nodes_per_chan();

  load_net_rr_terminals(rr_node_indices, nodes_per_chan);
  
  T_crit = 1;
  pres_fac = 0; /* ignore congestion */

  for (ipin=1; ipin< net[NET_USED].num_pins; ipin ++)
    net_slack[NET_USED][ipin] = 0;

  is_routeable = timing_driven_route_net (NET_USED, pres_fac, 
		 router_opts.max_criticality, router_opts.criticality_exp, 
		 router_opts.astar_fac, router_opts.bend_cost,
                 net_slack[NET_USED], pin_criticality, sink_order,
                 rt_node_of_sink, T_crit, net_delay[NET_USED]);
  
  net_delay_value = net_delay[NET_USED][NET_USED_SINK_BLOCK];

  clb[source_x_loc][source_y_loc].occ = 0;
  clb[sink_x_loc][sink_y_loc].occ = 0;

  return (net_delay_value);

}

/**************************************/
static void alloc_delta_arrays(void) {

  int id_x, id_y;

  delta_clb_to_clb = (float **) alloc_matrix (0, nx-1, 0, ny-1,sizeof(float));
  delta_inpad_to_clb = (float **) alloc_matrix (0, nx, 0, ny, sizeof(float));
  delta_clb_to_outpad = (float **) alloc_matrix (0, nx, 0, ny, sizeof(float));
  delta_inpad_to_outpad = (float **) alloc_matrix (0, nx+1, 0, ny+1, sizeof(float));


  /*initialize all of the array locations to -1*/
  
  for (id_x = 0; id_x <= nx; id_x ++) {
    for (id_y = 0; id_y <= ny; id_y ++) {
      delta_inpad_to_clb[id_x][id_y] = IMPOSSIBLE;
    }
  }
  for (id_x = 0; id_x <= nx-1; id_x ++) {
    for (id_y = 0; id_y <= ny-1; id_y ++) {
      delta_clb_to_clb[id_x][id_y] = IMPOSSIBLE;  
    }
  }
  for (id_x = 0; id_x <= nx; id_x ++) {
    for (id_y = 0; id_y <= ny; id_y ++) {
      delta_clb_to_outpad[id_x][id_y] = IMPOSSIBLE;
    }
  }
  for (id_x = 0; id_x <= nx+1; id_x ++) {
    for (id_y = 0; id_y <= ny+1; id_y ++) {
      delta_inpad_to_outpad[id_x][id_y] = IMPOSSIBLE;
    }
  }
}

/**************************************/
static void free_delta_arrays(void) {

  free_matrix(delta_inpad_to_clb,0, nx, 0, sizeof(float));
  free_matrix(delta_clb_to_clb, 0, nx-1, 0, sizeof(float));
  free_matrix(delta_clb_to_outpad, 0, nx, 0, sizeof(float));
  free_matrix(delta_inpad_to_outpad, 0, nx+1, 0, sizeof(float));

}
/**************************************/
static void generic_compute_matrix(float ***matrix_ptr, 
                            enum e_block_types source_type, 
			    enum e_block_types sink_type, int source_x, 
			    int source_y, int start_x,  
			    int end_x, int start_y, int end_y, 
			    struct s_router_opts router_opts,
			    struct s_det_routing_arch det_routing_arch, 
			    t_segment_inf *segment_inf, 
			    t_timing_inf timing_inf
			    ){

  int delta_x, delta_y;
  int sink_x, sink_y;

  for (sink_x = start_x; sink_x <= end_x; sink_x ++) {
    for (sink_y = start_y; sink_y <= end_y; sink_y ++) {
      delta_x = abs(sink_x - source_x);
      delta_y = abs(sink_y - source_y);

      if (delta_x == 0 && delta_y ==0) 
	continue;  /*do not compute distance from a block to itself     */
                   /*if a value is desired, pre-assign it somewhere else*/

      (*matrix_ptr)[delta_x][delta_y] = 
	assign_blocks_and_route_net (source_type, source_x, source_y, 
				     sink_type, sink_x, sink_y, router_opts,
				     det_routing_arch, 
				     segment_inf, 
				     timing_inf
				     );
    }
  }
}
/**************************************/
static void compute_delta_clb_to_clb (struct s_router_opts router_opts,
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf, int longest_length) {

  /*this routine must compute delay values in a slightly different way than the*/
  /*other compute routines. We cannot use a location close to the edge as the  */
  /*source location for the majority of the delay computations  because this   */
  /*would give gradually increasing delay values. To avoid this from happening */
  /*a clb that is at least longest_length away from an edge should be chosen   */
  /*as a source , if longest_length is more than 0.5 of the total size then    */
  /*choose a CLB at the center as the source CLB */

  int source_x, source_y, sink_x, sink_y;
  int start_x, start_y, end_x, end_y;
  int delta_x, delta_y;
  enum e_block_types source_type, sink_type;

  source_type = CLB;
  sink_type = CLB;

  if (longest_length < 0.5 * (nx)){
    start_x = longest_length;
  }
  else {
    start_x = (int)(0.5 * nx);
  }
  end_x = nx;
  source_x = start_x;

  if (longest_length < 0.5 * (ny)) {
    start_y = longest_length;
  }
  else {
    start_y = (int) (0.5 * ny);
  }
  end_y = ny;
  source_y = start_y;


  /*don't put the sink all the way to the corner, until it is necessary*/
  for (sink_x = start_x; sink_x <= end_x-1; sink_x ++) {
    for (sink_y = start_y; sink_y <= end_y-1; sink_y ++) {
      delta_x = abs(sink_x - source_x);
      delta_y = abs(sink_y - source_y);

      if (delta_x == 0 && delta_y ==0) {
	delta_clb_to_clb[delta_x][delta_y] = 0.0;
	continue; 
      }
      delta_clb_to_clb[delta_x][delta_y] =
	assign_blocks_and_route_net (source_type, source_x, source_y, sink_type, 
				     sink_x, sink_y, router_opts, det_routing_arch,
				     segment_inf,timing_inf);
    }

  }


  sink_x = end_x-1;
  sink_y = end_y-1;

  for (source_x = start_x-1; source_x >=1; source_x --) {
    for (source_y = start_y; source_y <= end_y-1; source_y ++) {
      delta_x = abs(sink_x - source_x);
      delta_y = abs(sink_y - source_y);

      delta_clb_to_clb[delta_x][delta_y] =
	assign_blocks_and_route_net (source_type, source_x, source_y, sink_type, 
				     sink_x, sink_y, router_opts, det_routing_arch,
				     segment_inf,timing_inf);
    }
  }

  for (source_x = 1; source_x <= end_x-1; source_x ++) {
    for (source_y =1; source_y < start_y; source_y++ ) {
      delta_x = abs(sink_x - source_x);
      delta_y = abs(sink_y - source_y);

      delta_clb_to_clb[delta_x][delta_y] =
	assign_blocks_and_route_net (source_type, source_x, source_y, sink_type, 
				     sink_x, sink_y, router_opts, det_routing_arch,
				     segment_inf,timing_inf);
    }
  }


  /*now move sink into the top right corner*/
  sink_x = end_x;
  sink_y = end_y;
  source_x = 1;
  for (source_y =1; source_y <= end_y; source_y ++) {
    delta_x = abs(sink_x - source_x);
    delta_y = abs(sink_y - source_y);

    delta_clb_to_clb[delta_x][delta_y] =
      assign_blocks_and_route_net (source_type, source_x, source_y, sink_type, 
				   sink_x, sink_y, router_opts, det_routing_arch,
				   segment_inf,timing_inf);

  }
  
  sink_x = end_x;
  sink_y = end_y;
  source_y = 1;
  for (source_x=1; source_x <= end_x; source_x ++) {
    delta_x = abs(sink_x - source_x);
    delta_y = abs(sink_y - source_y);

    delta_clb_to_clb[delta_x][delta_y] =
      assign_blocks_and_route_net (source_type, source_x, source_y, sink_type, 
				   sink_x, sink_y, router_opts, det_routing_arch,
				   segment_inf,timing_inf);
  }
}
/**************************************/
static void compute_delta_inpad_to_clb (struct s_router_opts router_opts,
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf) {

  int source_x, source_y;
  int start_x, start_y, end_x, end_y;
  enum e_block_types source_type, sink_type;

  source_type = INPAD;  
  sink_type = CLB;

  delta_inpad_to_clb[0][0] = IMPOSSIBLE;
  delta_inpad_to_clb[nx][ny] = IMPOSSIBLE;

  source_x = 0; source_y = 1;

  start_x = 1; end_x = nx; start_y = 1; end_y = ny;
  generic_compute_matrix(&delta_inpad_to_clb, source_type, sink_type, 
			 source_x, source_y, start_x, end_x, start_y, end_y, 
			 router_opts, det_routing_arch, segment_inf,timing_inf); 
  
  source_x = 1; source_y =0;
  
  start_x = 1; end_x = 1; start_y = 1; end_y = ny;
  generic_compute_matrix(&delta_inpad_to_clb, source_type, sink_type, 
			 source_x, source_y, start_x, end_x, start_y, end_y,
			 router_opts, det_routing_arch, segment_inf,timing_inf);

  start_x = 1; end_x = nx; start_y = ny; end_y = ny;
  generic_compute_matrix(&delta_inpad_to_clb, source_type, sink_type, 
			 source_x, source_y, start_x, end_x, start_y, end_y,
			 router_opts, det_routing_arch, segment_inf,timing_inf); 

}
/**************************************/
static void compute_delta_clb_to_outpad(struct s_router_opts router_opts,
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf) {

  int source_x, source_y, sink_x, sink_y;
  int delta_x, delta_y;
  enum e_block_types source_type, sink_type;

  source_type = CLB;
  sink_type = OUTPAD;
  
  delta_clb_to_outpad[0][0] = IMPOSSIBLE;
  delta_clb_to_outpad[nx][ny] = IMPOSSIBLE;
  
  sink_x = 0; sink_y = 1;
  for (source_x = 1; source_x <= nx; source_x ++) {
    for (source_y =1; source_y <= ny; source_y ++) {
      delta_x = abs(source_x - sink_x); 
      delta_y = abs(source_y - sink_y);

      delta_clb_to_outpad[delta_x][delta_y] =
	assign_blocks_and_route_net (source_type, source_x, source_y, sink_type, 
				     sink_x, sink_y,router_opts, det_routing_arch, 
				     segment_inf,timing_inf);
    }
  }
  
  sink_x = 1; sink_y = 0;
  source_x = 1;
  delta_x = abs(source_x - sink_x);
  for (source_y = 1; source_y <= ny; source_y ++) {
    delta_y = abs(source_y - sink_y);
    delta_clb_to_outpad[delta_x][delta_y] =
      assign_blocks_and_route_net (source_type, source_x, source_y, sink_type, 
				   sink_x, sink_y, router_opts, det_routing_arch,
				   segment_inf,timing_inf);
  }

  sink_x = 1; sink_y = 0;
  source_y = ny;
  delta_y = abs(source_y - sink_y);
  for (source_x = 2; source_x <= nx; source_x ++) {
    delta_x = abs(source_x - sink_x);
    delta_clb_to_outpad[delta_x][delta_y] =
      assign_blocks_and_route_net (source_type, source_x, source_y, sink_type, 
				   sink_x, sink_y, router_opts, det_routing_arch, 
				   segment_inf,timing_inf);
  }
}

/**************************************/
static void compute_delta_inpad_to_outpad(struct s_router_opts router_opts,
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf) {

  int source_x, source_y, sink_x, sink_y;
  int delta_x, delta_y;
  enum e_block_types source_type, sink_type;

  source_type = INPAD;
  sink_type = OUTPAD;

  delta_inpad_to_outpad[0][0] = 0; /*delay to itself is 0 (this can happen)*/
  delta_inpad_to_outpad[nx+1][ny+1] = IMPOSSIBLE;
  delta_inpad_to_outpad[0][ny]= IMPOSSIBLE;
  delta_inpad_to_outpad[nx][0]= IMPOSSIBLE;
  delta_inpad_to_outpad[nx][ny+1]= IMPOSSIBLE;
  delta_inpad_to_outpad[nx+1][ny]= IMPOSSIBLE;


  source_x = 0;
  source_y = 1;
  sink_x = 0;
  delta_x = abs (sink_x - source_x);


  for (sink_y = 2; sink_y <= ny; sink_y ++) {
    delta_y = abs (sink_y - source_y);
    delta_inpad_to_outpad[delta_x][delta_y] =
	assign_blocks_and_route_net (source_type, source_x, source_y, sink_type, 
				     sink_x, sink_y, router_opts, det_routing_arch, 
				     segment_inf,timing_inf);

  }
 
  source_x = 0;
  source_y = 1;
  sink_x = nx+1;
  delta_x = abs (sink_x - source_x);

  for (sink_y = 1; sink_y <= ny; sink_y ++) {
    delta_y = abs (sink_y - source_y);
    delta_inpad_to_outpad[delta_x][delta_y] =
	assign_blocks_and_route_net (source_type, source_x, source_y, sink_type, 
				     sink_x, sink_y, router_opts, det_routing_arch, 
				     segment_inf,timing_inf);

  }


  source_x = 1;
  source_y = 0;
  sink_y = 0;
  delta_y = abs (sink_y - source_y);
  
  for (sink_x = 2; sink_x <= nx; sink_x ++) {
    delta_x = abs(sink_x - source_x);
    delta_inpad_to_outpad[delta_x][delta_y] =
	assign_blocks_and_route_net (source_type, source_x, source_y, sink_type, 
				     sink_x, sink_y, router_opts, det_routing_arch, 
				     segment_inf,timing_inf);

  }
  
  source_x = 1;
  source_y = 0;
  sink_y = ny+1;
  delta_y = abs (sink_y - source_y);
  
  for (sink_x = 1; sink_x <= nx; sink_x ++) {
    delta_x = abs(sink_x - source_x);
    delta_inpad_to_outpad[delta_x][delta_y] =
	assign_blocks_and_route_net (source_type, source_x, source_y, sink_type, 
				     sink_x, sink_y, router_opts, det_routing_arch, 
				     segment_inf,timing_inf);

  }

  source_x = 0;
  sink_y = ny+1;
  for (source_y = 1; source_y <= ny; source_y ++) {
    for (sink_x = 1; sink_x <= nx; sink_x ++) {
      delta_y = abs(source_y - sink_y);
      delta_x = abs(source_x - sink_x);
      delta_inpad_to_outpad[delta_x][delta_y] =
	assign_blocks_and_route_net (source_type, source_x, source_y, sink_type, 
				     sink_x, sink_y, router_opts, det_routing_arch,
				     segment_inf,timing_inf);

    }
  }  
}
/**************************************/
#ifdef PRINT_ARRAYS
static void  print_array(float **array_to_print, int x1, int x2, int y1, int y2) {


  int idx_x, idx_y;

  fprintf(lookup_dump, "\nPrinting Array \n\n");

  for (idx_y = y2; idx_y >= y1; idx_y --) {
    for (idx_x=x1; idx_x <= x2; idx_x ++) {
      fprintf(lookup_dump, " %9.2e",array_to_print[idx_x][idx_y]);
    }
    fprintf(lookup_dump, "\n");
  }
  fprintf(lookup_dump,"\n\n");
}
#endif
/**************************************/
static void compute_delta_arrays (struct s_router_opts router_opts,
				   struct s_det_routing_arch det_routing_arch, 
				   t_segment_inf *segment_inf, 
				   t_timing_inf timing_inf, int longest_length) {


  printf("Computing delta_clb_to_clb lookup matrix, may take a few seconds, please wait...\n");
  compute_delta_clb_to_clb(router_opts, det_routing_arch, segment_inf,timing_inf,
			   longest_length);
  printf("Computing delta_inpad_to_clb lookup matrix, may take a few seconds, please wait...\n");
  compute_delta_inpad_to_clb(router_opts, det_routing_arch, segment_inf,timing_inf);
  printf("Computing delta_clb_to_outpad lookup matrix, may take a few seconds, please wait...\n");
  compute_delta_clb_to_outpad(router_opts, det_routing_arch, segment_inf,timing_inf);
  printf("Computing delta_inpad_to_outpad lookup matrix, may take a few seconds, please wait...\n");
  compute_delta_inpad_to_outpad(router_opts, det_routing_arch, segment_inf,timing_inf);

#ifdef PRINT_ARRAYS
  lookup_dump = my_fopen(DUMPFILE,"w",0);
  fprintf(lookup_dump,"\n\nprinting delta_clb_to_clb\n");
  print_array(delta_clb_to_clb, 0 , nx-1, 0, ny-1); 
  fprintf(lookup_dump,"\n\nprinting delta_inpad_to_clb\n");
  print_array(delta_inpad_to_clb, 0, nx, 0, ny);
  fprintf(lookup_dump,"\n\nprinting delta_clb_to_outpad\n");
  print_array(delta_clb_to_outpad, 0, nx, 0, ny);
  fprintf(lookup_dump,"\n\nprinting delta_inpad_to_outpad\n");
  print_array(delta_inpad_to_outpad, 0, nx+1, 0, ny+ 1);
  fclose(lookup_dump);
#endif

}
/******* Globally Accessable Functions **********/

/**************************************/
void compute_delay_lookup_tables(struct s_router_opts router_opts, 
			   struct s_det_routing_arch det_routing_arch, 
			   t_segment_inf *segment_inf, 
			   t_timing_inf timing_inf, 
			   t_chan_width_dist chan_width_dist, t_subblock_data subblock_data) {




  static struct s_net *original_net;/*this will be used as a pointer to remember what*/
  /*the "real" nets in the circuit are. This is    */
  /*required because we are using the net structure*/
  /*in these routines to find delays between blocks*/
  static struct s_block *original_block; /*same def as original_nets, but for block  */

  static int original_num_nets;
  static int original_num_blocks;
  static int longest_length;



  alloc_and_assign_internal_structures(&original_net, 
				       &original_block, &original_num_nets, 
				       &original_num_blocks);			
  setup_chan_width(router_opts, chan_width_dist);

  alloc_routing_structs(router_opts, det_routing_arch, segment_inf, timing_inf, subblock_data);

  longest_length = get_longest_segment_length( det_routing_arch, segment_inf);


  /*now setup and compute the actual arrays */
  alloc_delta_arrays();
  compute_delta_arrays(router_opts, det_routing_arch, segment_inf,timing_inf, 
		       longest_length);


  /*free all data structures that are no longer needed*/
  free_routing_structs(router_opts, det_routing_arch, segment_inf, timing_inf);

  free_and_reset_internal_structures(original_net, original_block, 
				     original_num_nets, original_num_blocks);
}

/**************************************/
void free_place_lookup_structs(void) {

  free_delta_arrays();

}
