/*path_length.c                                                       *
 *originallly created April 16, 1998, Alexander (Sandy) Marquardt     *
 *calculates path lengths and block distances from inputs and outputs *
 *and determines net slacks, as well as criticalities of nets         */

#include <stdio.h>
#include <math.h>
#include "util.h"
#include "vpack.h"
#include "globals.h"
#include "cluster.h"
#include "path_length.h"
#include "heapsort.h"

#define CRITICAL_LENGTH_STATS  /*this controls whether or not the progam will */
                                    /*print out extra data about  the status of    */
                                    /*the packing after each timing-analysis       */
#define CRITICAL_PATH_STATS  /*print out blocks and nets that vpack thinks are*/
                                  /*the critical path before and after packing     */

/****************** Defines for this module ****************************************/

/*#define DEBUG_PATH_LENGTH 1*/          /*Tells path_length.c to output some   */
                                         /*useful data structures to debugPL.log*/

#define MAX_ALLOWED_PATH_LEN 10000.0/*really big number to initialize required      *
				      arrival times.                                */

/*The following defines assume that the user has assigned a inter_cluster_net_delay *
 *value of 1, and is using the intra_cluster_net_delay values to scale the ratio    *
 *between the two. It is strongly recommended that inter_cluster_net_delay be set to*
 *1 in all cases. */


#define MIN_SLACK_ALLOWED 1e-6  /*any slack values below this number are set to     *
				  *zero since the resulting value is caused by       *
				  *inaccuracies in floating point calculations       */
#define SCALE_NUM_PATHS 1e-2     /*this value is used as a multiplier to assign a    *
				  *slightly higher criticality value to nets that    *
				  *affect a large number of critical paths versus    *
				  *nets that do not have such a broad effect.        *
				  *Note that this multiplier is intentionally very   * 
				  *small compared to the total criticality because   *
				  *we want to make sure that net criticality is      *
				  *primarily determined by slacks, with this acting  *
				  *only as a tie-breaker between otherwise equal nets*/
#define SCALE_DISTANCE_VAL 1e-4  /*this value is used as a multiplier to assign a    *
				  *slightly higher criticality value to nets that    *
				  *are otherwise equal but one is  farther           *
				  *from sources (the farther one being made slightly *
				  *more critical)                                    */
#define EQUAL_DEF 1e-6            /*used in some if == equations to allow very       *
				   *close values to be considered equal              */


/****************** Structures for this module *************************************/

struct s_block_timing {float delay_from_source;
                       float required_arrival_time;
                       int *net_pin_index;  
                       long num_max_inputs;
                       long num_max_outputs;};
/* delay_from_source: delay to block's input from the farthest source    *
 * required_arrival_time: What is the latest that a signal can arrive at *
 *                   an input to this block without becoming critical    *
 * net_pin_index[]:  For each input on this block, this indicates which  *
 *                   pin in nets[].pins[] the input net is corresponds to*
 * num_max_inputs:   How many paths on the inputs to this block have the *
 *                   maximum arrival time of paths that are inputs to    *
 *                   this block.                            .            *
 * num_max_ouputs:   How many paths on the output from this block have   *
 *                   the smallest required arrival time of paths that are*
 *                   driven by this block                                */

struct s_net_timing {float* net_pin_arrival_time;
                     float* net_slack;
                     float* net_forward_criticality;
                     float* net_backward_criticality;};
/*net_pin_arrival_time[]:parallel array to s_net[].pins[]. When does the  *
 *                       output of this net become valid at the pin which *
 *                       it is driving.                                   *
 * net_slack[]:      [0..lut_size], required_arrival_time - net_arrival_  *
 *                   time.                                                *
 * net_forward_criticality[]:[0..lut_size], is set to                     *
 *                   (1-(slack/max_slack)) plus a tie breaker portion     *
 *                   determined by the block driving the net (see the     *
 *                   calculate_slack_and_criticality function for a       *
 *                   description of tie-breakers).                        *
 *                   this has a value between 0 and 1 with higher values  *
 *                   indicating a more critical net.                      *
 * net_backward_criticality[]: [0..lut_size], is set to                   *
 *                   (1-(slack/max_slack). plus a tie-breaker portion     *
 *                   determined by the block being driven by the net.     */


/****************** Variables Global to this module *******************************/
struct s_block_timing *block_timing;
struct s_net_timing *net_timing;

static int *block_has_distance_q; /*which blocks have been allocated distance      *
				  *from sources                                    */
static int *block_has_req_arr_time_q; /*which blocks have been allocated required  *
			               *arrival times                              */

static int *block_in_remaining; /*How many adjacent input blocks have not been     *
				 *discovered. Once all input blocks have been      *
				 *discovered, this block is added to the           *
				 *block_has_distance_q                             */
static int *block_out_remaining; /*How many adjacent blocks on the output net have *
				  *not been discovered yet. Once all output blocks *
				  *have been discovered, this block is added to the*
				  *block_has_req_arr_time_q                        */
static int *block_in_count;      /*How many inputs on this block are valid         */
static int *block_out_count;     /*How many blocks are being driven by the output  */

static boolean *block_is_in_initial_source_q;/*indicates whether a block is a      *
					      *  source or not                     */
static boolean *block_is_in_initial_sink_q;  /*indicates whether a block is a      *
				              *sink or not                         */
static boolean *block_is_in_distance_q;/*indicates whether a block has been        *
				      * already added to the block_has_distance_q  */
static boolean *block_is_in_req_arr_time_q;  /*indicates whether a block has been  *
				      *added to the the block_has_req_arr_time_q   */

static boolean *is_clock; /*pointer to the is_clock array declared in main function*/

static float *criticality; /* [0..num_blocks-1] keeps track of how critical each   *
			    * block is, this is primarily used for determining     *
			    * which block to use as a cluster seed.                *
			    * The criticality of a block is equal to the           *
			    * of the most critical net on its input pins.          */
static int *critindexarray; /* [0..num_blocks-1] Each value in this array is a     *
		             * pointer to the criticality array.  The index of the *
			     * most critical block is first, least critical last   */
static int indexofcrit;     /*next index in the critindexarray that will be  read  *
			     *this value is reset every time a new timing analysis *
			     *and sort is done. It is modified every time          *
			     *get_most_critical_block is called                    */

static int lut_size;      
static int initial_num_sinks_in_q; /* allows us to remember what blocks are sinks  *
				    * even after the queue has been expanded       */
static int initial_num_sources_in_q; /*ditto for sources                           */


#ifdef CRITICAL_PATH_STATS
static FILE *critical_report_f;  /*used for printing out the critical path details*/
#endif

#ifdef CRITICAL_LENGTH_STATS
static FILE *length_f;           /*records the length of the critical path as the *
				  *program proceeds */
#endif


/************* Declarations of subroutines local to this module *************/

static void print_stats(int num_blocks_clustered, int num_clusters,
                 float farthest_distance,
                 int num_critical_conn, float avg_dist_to_sinks,
		 float avg_point_to_point_dist);



/******************** Subroutine Definitions **************************************/

/******************************/

static void alloc_and_init_structures(void)
  /*allocates data structures required for timing analysis and initializes them to*
   *default values   */

{
  int iblk,netpin,inet,blockpin;
  int blk,ipin;
  int numpins;
  int startidx,endidx;

  block_has_distance_q=(int*)my_malloc(num_blocks*sizeof(int));
  block_has_req_arr_time_q=(int*)my_malloc(num_blocks*sizeof(int));
  block_in_remaining=(int*)my_malloc(num_blocks*sizeof(int));
  block_out_remaining=(int*)my_malloc(num_blocks*sizeof(int));
  block_in_count=(int*)my_malloc(num_blocks*sizeof(int));
  block_out_count=(int*)my_malloc(num_blocks*sizeof(int));

  /*calloc inits the following four arrays to FALSE*/
  block_is_in_initial_source_q=(boolean*)my_calloc(num_blocks,sizeof(boolean)); 
  block_is_in_initial_sink_q=(boolean*)my_calloc(num_blocks,sizeof(boolean));
  block_is_in_distance_q=(boolean*)my_calloc(num_blocks,sizeof(boolean)); 
  block_is_in_req_arr_time_q=(boolean*)my_calloc(num_blocks,sizeof(boolean));

  block_timing=(struct s_block_timing*)my_malloc(num_blocks*
					       sizeof(struct s_block_timing));
  criticality=(float*)my_malloc(num_blocks*sizeof(float));
  critindexarray=(int*)my_malloc(num_blocks*sizeof(int));


  /*initialize and allocate the block_timing internal structures*/
  for (iblk=0;iblk<num_blocks;iblk++){   
    
    block_timing[iblk].delay_from_source= -1.0;
    block_timing[iblk].required_arrival_time= MAX_ALLOWED_PATH_LEN;
    block_timing[iblk].num_max_inputs=1;
    block_timing[iblk].num_max_outputs=1;
    criticality[iblk]= -1.0;

    if (block[iblk].type==INPAD || block[iblk].type==OUTPAD){
      startidx=0;
      endidx=0;
      block_timing[iblk].net_pin_index=(int*)my_malloc(sizeof(int));
    }
    else {
      startidx=0;
      endidx=lut_size;
      block_timing[iblk].net_pin_index=(int*)my_malloc((lut_size+1)*sizeof(int));
    }

    for (ipin=startidx;ipin<=endidx;ipin++){
      block_timing[iblk].net_pin_index[ipin]=-1;
    }
  }

  /*update the block_timing.net_pin_index values*/
  for (inet=0;inet<num_nets;inet++) {
    if (net[inet].pins[0] != OPEN) {

      for (netpin=0;netpin<net[inet].num_pins;netpin++){
	blk=net[inet].pins[netpin];
	if (block[blk].type == INPAD)
	  block_timing[blk].net_pin_index[0]=0;  /*pin 0 of net is always on driving block*/
	else if (block[blk].type == OUTPAD)
	  block_timing[blk].net_pin_index[0]=netpin;
	else if (netpin == 0)
	  block_timing[blk].net_pin_index[0]=0;
        else{
	  startidx=1;
	  endidx=lut_size;
	  for (blockpin=startidx;blockpin<=endidx;blockpin++){
	    if (block[blk].nets[blockpin] == inet){
	      block_timing[blk].net_pin_index[blockpin]=netpin;
	    }
	  }
	}
      }
    }
  }
  

  /*allocate and initialize the net_timing structure*/
  net_timing=(struct s_net_timing*)my_malloc(num_nets*sizeof(struct s_net_timing));

  for (inet=0;inet<num_nets;inet++){

    numpins=net[inet].num_pins;

    net_timing[inet].net_pin_arrival_time=(float*)my_malloc(numpins*sizeof(float));
    net_timing[inet].net_slack=(float*)my_malloc(numpins*sizeof(float));
    net_timing[inet].net_forward_criticality=(float*)my_malloc(numpins*sizeof(float));
    net_timing[inet].net_backward_criticality=(float*)my_malloc(numpins*sizeof(float));

    for (ipin=0;ipin<numpins;ipin++) {
      net_timing[inet].net_pin_arrival_time[ipin]=0.0;
      net_timing[inet].net_slack[ipin]=-1.0;
      net_timing[inet].net_forward_criticality[ipin]=-1.0;
      net_timing[inet].net_backward_criticality[ipin]=-1.0;
    }
  }
}
/******************************/

static void set_block_input_output_counts(void)
  /*set number of inputs and outputs that a block can expect to have*/
{
  int blkidx;
  int outnet;

  for (blkidx=0;blkidx<num_blocks;blkidx++){

    /*set number of inputs for each block*/
    if (block[blkidx].type == OUTPAD)
      block_in_count[blkidx]=1;
    else if (block[blkidx].type == INPAD)
      block_in_count[blkidx]=0;
    else if (block[blkidx].type == LUT)
      block_in_count[blkidx]=block[blkidx].num_nets-1; /*-1 for output*/
    else if (block[blkidx].type == LATCH || block[blkidx].type == LUT_AND_LATCH)
      block_in_count[blkidx]=block[blkidx].num_nets-2; /*-2 for clock+output*/


    /*set number of outputs for each block*/
    if (block[blkidx].type == OUTPAD) {
      outnet=OPEN;
      block_out_count[blkidx]=0;
    }
    else {
      outnet=block[blkidx].nets[0];

      if (is_clock[outnet])
	/*ignore clock nets*/
	block_out_count[blkidx]=0;

      else if (block[blkidx].type == INPAD)
	block_out_count[blkidx]=1;  

      else
	block_out_count[blkidx]=net[outnet].num_pins-1;
    }  
  }
}


/******************************/
static void get_sources_and_sinks(void) {
  /*determines what blocks are sources, and which are sinks, and allocates *
   *the various queue structures to keep track of where the sources and    *
   *sinks are */


  int blkidx;
  int q_next_source_to_add;
  int q_next_sink_to_add;


  q_next_source_to_add=0;
  q_next_sink_to_add=0;

  /* If a block has no inputs, then it is considered to be a source (could happen */
  /* for a constant generator)     */

  for (blkidx=0;blkidx<num_blocks;blkidx++){

    block_is_in_initial_source_q[blkidx]=FALSE;
    block_is_in_initial_sink_q[blkidx]=FALSE;
    block_has_distance_q[blkidx]=-1;
    block_has_req_arr_time_q[blkidx]=-1;
    
    /*mark sources*/
    if (block[blkidx].type == INPAD || block[blkidx].type == LATCH || 
	block[blkidx].type == LUT_AND_LATCH) {

      block_has_distance_q[q_next_source_to_add]=blkidx;
      q_next_source_to_add++;
      block_timing[blkidx].delay_from_source=0.0;
      block_is_in_initial_source_q[blkidx]=TRUE;
    }
    /*mark other sources*/
    else if (block[blkidx].num_nets == 1 && block[blkidx].type != OUTPAD) { 
      /* a constant generator, consider it a source*/

      block_has_distance_q[q_next_source_to_add]=blkidx;
      q_next_source_to_add++;
      block_timing[blkidx].delay_from_source=0.0;
      block_is_in_initial_source_q[blkidx]=TRUE;
    }

    /*mark sinks*/
    if (block[blkidx].type == OUTPAD || block[blkidx].type == LATCH || 
	block[blkidx].type == LUT_AND_LATCH) {

      block_has_req_arr_time_q[q_next_sink_to_add]=blkidx;
      q_next_sink_to_add++;

      block_is_in_initial_sink_q[blkidx]=TRUE;
    }
    /*mark other sinks*/
    else if (is_clock[block[blkidx].nets[0]] && block[blkidx].type != INPAD){ 
      /*output from this block is a clock, consider this block a sink*/

      block_has_req_arr_time_q[q_next_sink_to_add]=blkidx;
      q_next_sink_to_add++;

      block_is_in_initial_sink_q[blkidx]=TRUE;
    }  
  }

  initial_num_sinks_in_q=q_next_sink_to_add;
  initial_num_sources_in_q=q_next_source_to_add;
}

/******************************/
static void reset_blocks_to_initial_state(void) {
  /*this procedure sets up the structures so that a new timing analysis can *
   *be done, basically it sets the structures to the state that they were in*
   *after the calls to get_sources_and_sinks, and                           *
   *set_block_input_output_counts                                           */

  int blkidx;

  for (blkidx=0;blkidx<num_blocks;blkidx++){
    block_in_remaining[blkidx]=block_in_count[blkidx];
    block_out_remaining[blkidx]=block_out_count[blkidx];
    block_is_in_distance_q[blkidx]=block_is_in_initial_source_q[blkidx];
    block_is_in_req_arr_time_q[blkidx]=block_is_in_initial_sink_q[blkidx];
    block_timing[blkidx].delay_from_source= 0.0;
    block_timing[blkidx].num_max_inputs=1;
    block_timing[blkidx].num_max_outputs=1;
    criticality[blkidx]= -1.0;
    
  }
}
/******************************/
static float calculate_arrival_time_at_each_pin(int block_discovered,
						int parent_block, float block_delay,
						float inter_cluster_net_delay,
						float intra_cluster_net_delay,
						float *point_to_point_dist_sum) {

  /*Calculate the arrival time at each discovered block*/

  float arrival_t;
  int cluster_of_block_discovered;



  cluster_of_block_discovered=get_cluster_of_block(block_discovered);

  if (cluster_of_block_discovered != NO_CLUSTER &&
      cluster_of_block_discovered == get_cluster_of_block(parent_block)){
    /*the block discovered is in the same cluster as it's parent block*/
    (*point_to_point_dist_sum) += intra_cluster_net_delay + block_delay;

    if (!block_is_in_initial_source_q[parent_block]){
      arrival_t=block_timing[parent_block].delay_from_source +
	intra_cluster_net_delay + block_delay;
    }
    else{ /*the parent is a source, so it contributes no previous delay*/
      arrival_t=intra_cluster_net_delay+block_delay;
    }
  }
  else {/*the discovered block is in a different cluster from the parent block*/
    (*point_to_point_dist_sum) += inter_cluster_net_delay + block_delay;

    if (!block_is_in_initial_source_q[parent_block]){
      arrival_t=block_timing[parent_block].delay_from_source +
	inter_cluster_net_delay+block_delay;	   
    }
    else{ /*the parent is a source, so it contributes no previous delay*/
      arrival_t=inter_cluster_net_delay+block_delay;
    }
  }

  return arrival_t;
}
/******************************/
static void update_paths_through_each_block(int block_discovered, int parent_block,
					    float arrival_time,
					    int *loc_biggest_num_max_inputs)
{

  /*Update the number of paths through each block*/

	  /*check for approximate equality*/
  if (fabs(block_timing[block_discovered].delay_from_source - 
	   arrival_time) <EQUAL_DEF){
    if (!block_is_in_initial_source_q[parent_block])
      block_timing[block_discovered].num_max_inputs +=
	block_timing[parent_block].num_max_inputs;
    else
      block_timing[block_discovered].num_max_inputs += 1;
  }
  else if (block_timing[block_discovered].delay_from_source < arrival_time) {
    if (!block_is_in_initial_source_q[parent_block])
      block_timing[block_discovered].num_max_inputs = 
	block_timing[parent_block].num_max_inputs;
    else
      block_timing[block_discovered].num_max_inputs = 1;
  }

  if (block_timing[block_discovered].num_max_inputs > *loc_biggest_num_max_inputs)
    *loc_biggest_num_max_inputs=block_timing[block_discovered].num_max_inputs;

	 

}

/******************************/
static int calculate_sink_dist_sum(void) {

  int sum, idx, blknum;

  sum = 0;
  
  for (idx = 0; idx<initial_num_sinks_in_q; idx++) {
    blknum = block_has_req_arr_time_q[idx];
    sum += block_timing[blknum].delay_from_source;
  }
  return sum;
}
/******************************/
static void distance_from_sources(float* maximum_arrival_time, float block_delay,
				  float inter_cluster_net_delay, 
				  float intra_cluster_net_delay,
				  int *farthest_block_discovered,
				  long *biggest_num_max_inputs,
				  float *farthest_distance,
				  float *avg_dist_of_sinks,
				  float *avg_point_to_point_dist){

  /*calculate the distances from sources */
  /*maximum_arrival_time returns the distance of the farthest sink*/

  /*This algorithm traverses the netlist from sources using a breadth first traversal*
   *which adds blocks to the block_has_distance_q. Blocks which are farther from     *
   *the sources are added to the queue last. By farther, I am not talking about delay*
   *but rather I am talking about number of blocks between the source and a block    */

  int netpinidx, q_head, outnet, parent_block;
  int block_discovered;
  int startidx,endidx;
  int next_blk_to_add_to_dist_q;
  int farthest_block;
  int loc_biggest_num_max_inputs;

  float arrival_time,max_arrival_time, farthest_delay;
  float sink_dist_sum, point_to_point_dist_sum;
  int point_to_point_num;



  max_arrival_time=0.0;
  farthest_delay = 0.0;
  farthest_block=0;
  q_head=0;
  loc_biggest_num_max_inputs=0;

  sink_dist_sum = 0.0;
  point_to_point_dist_sum = 0.0;
  point_to_point_num = 0;

  next_blk_to_add_to_dist_q=initial_num_sources_in_q;

  while (q_head < next_blk_to_add_to_dist_q) {

    parent_block=block_has_distance_q[q_head];

    /*if this is an outpad then nets[0] is actually an input to the pad, not an output*/
    if (block[parent_block].type == OUTPAD)
      outnet=OPEN;
    else
      outnet=block[parent_block].nets[0];


    /*ignore clock nets, and open nets*/
    if (outnet != OPEN) { 
      if (!is_clock[outnet]) {

	startidx=1;
	endidx=net[outnet].num_pins-1; /* subtract 1 to keep in range*/

	for (netpinidx=startidx;netpinidx<=endidx;netpinidx++){
	  block_discovered=net[outnet].pins[netpinidx];

	  /*Calculate the arrival time at each discovered pin of each block*/
	  arrival_time = calculate_arrival_time_at_each_pin(block_discovered, parent_block, 
							    block_delay, 
							    inter_cluster_net_delay, 
							    intra_cluster_net_delay,
							    &point_to_point_dist_sum);
	  point_to_point_num ++;

	  /*Update the number of paths through each block*/
	  update_paths_through_each_block(block_discovered, parent_block, arrival_time, 
					  &loc_biggest_num_max_inputs);
	  

	  net_timing[outnet].net_pin_arrival_time[netpinidx]=arrival_time;


	  if (arrival_time > block_timing[block_discovered].delay_from_source){
	    block_timing[block_discovered].delay_from_source=arrival_time;
	  }


	  if (fabs(arrival_time - max_arrival_time) >=  EQUAL_DEF && arrival_time > max_arrival_time){
	      max_arrival_time=arrival_time;
	      farthest_block=block_discovered;
	  }

	  block_in_remaining[block_discovered]--;

	  if (block_in_remaining[block_discovered] == 0) {
	    /*all inputs have been discovered, we now know how far this block is from an input*/
	    /*do not add previously added blocks to the queue.*/
	    if (!block_is_in_distance_q[block_discovered]){
	      block_has_distance_q[next_blk_to_add_to_dist_q]=block_discovered;
	      next_blk_to_add_to_dist_q++;
	      block_is_in_distance_q[block_discovered]=TRUE;
	    }
	  }

#ifdef DEBUG
	  else if (block_in_remaining[block_discovered]<0) {
	    printf("ERROR in distance_from_sources, a block has been discovered"
                   "more times than its number of inputs warrants\n");
	    exit(1);
	  }
#endif

	} /*for.....*/
      } /*!is_clock....*/
    } /*outnet...*/

    /*on the first call of this procedure, we print out some statistics*
     *about the circuit which we are packing*/
    q_head++;

  } /*while ....*/

  sink_dist_sum = calculate_sink_dist_sum();

  *farthest_distance = max_arrival_time;
  *farthest_block_discovered=farthest_block;
  *maximum_arrival_time=max_arrival_time;
  *biggest_num_max_inputs=loc_biggest_num_max_inputs;
  *avg_dist_of_sinks = sink_dist_sum / (float)initial_num_sinks_in_q;
  *avg_point_to_point_dist = point_to_point_dist_sum / (float)point_to_point_num;
}

/******************************/
static void assign_required_arrival_times_to_max(float max_arrival_time) {
  /*this procedure assigns max_arrival_time to the required_arrival_time*
   *for all blocks. For any blocks that are not sinks, this value will  *
   *be changed in the update_required_arrival_times procedure. It is    *
   *necessary to assign this "upper bound" value here, so that the      *
   *algorithm can find the smallest required_arrival_time for each node */

  int i;
  
  for (i=0; i<num_blocks; i++)
    block_timing[i].required_arrival_time = max_arrival_time;
}

/******************************/
static void update_required_arrival_times(float block_delay, 
					  float inter_cluster_net_delay, 
					  float intra_cluster_net_delay,
					  long *biggest_num_max_outputs){
  /*calculate the required arrival times for each block*/

  /*this procedure traverses the netlist in a breadth first manner from the sinks*/

  int i, q_head, innet,sink_block;
  int block_discovered;
  int startidx, endidx;
  int next_blk_to_add_to_req_arr_time_q;
  int netpin;
  int cluster_of_block_discovered;
  int loc_biggest_num_max_outputs;

  float required_arrival_t;



  q_head=0;
  next_blk_to_add_to_req_arr_time_q=initial_num_sinks_in_q;
  loc_biggest_num_max_outputs=0;


  while (q_head<next_blk_to_add_to_req_arr_time_q ) {

    sink_block=block_has_req_arr_time_q[q_head];
   
    /*there are no inputs to an inpad*/ 
    if (block[sink_block].type == INPAD) {
      q_head++;
      continue;
    }

    /*if this is an output pad, then the input to the pad is net[0]*/
    if (block[sink_block].type == OUTPAD){
      startidx=0;
      endidx=0;
    }
    else { /*ignore block.net[0] since it is an output.*/
      startidx=1;
      endidx=lut_size;  /*also ignore clock nets (which would be in nets[lut+1] )*/
    }


    for (i=startidx;i<=endidx;i++) {

      /*ignore OPEN nets*/
      if (block[sink_block].nets[i] != OPEN) {

	innet=block[sink_block].nets[i];

	block_discovered=net[innet].pins[0];


	netpin=block_timing[sink_block].net_pin_index[i];

	cluster_of_block_discovered=get_cluster_of_block(block_discovered);

	if (cluster_of_block_discovered != NO_CLUSTER &&
	    cluster_of_block_discovered == get_cluster_of_block(sink_block)){
	  required_arrival_t=block_timing[sink_block].required_arrival_time-
	    intra_cluster_net_delay-block_delay;
	}
	else{
	  required_arrival_t=block_timing[sink_block].required_arrival_time-
	    inter_cluster_net_delay-block_delay;
	}

	block_out_remaining[block_discovered]--;


	if (fabs(block_timing[block_discovered].delay_from_source - 
		                               required_arrival_t) < EQUAL_DEF){
	  if (!block_is_in_initial_sink_q[sink_block])
	    block_timing[block_discovered].num_max_outputs +=
	      block_timing[sink_block].num_max_outputs;
	  else
	    block_timing[block_discovered].num_max_outputs += 1;
	}
	else if (block_timing[block_discovered].required_arrival_time > required_arrival_t) {
	  if (!block_is_in_initial_sink_q[sink_block])
	    block_timing[block_discovered].num_max_outputs = 
	      block_timing[sink_block].num_max_outputs;
	  else
	    block_timing[block_discovered].num_max_outputs = 1;
	}

	if (block_timing[block_discovered].num_max_outputs > loc_biggest_num_max_outputs)
	  loc_biggest_num_max_outputs=block_timing[block_discovered].num_max_outputs;


	/* update arrival times of blocks that are not sinks *
	 * (sinks already have valid values)                 */
	if (!block_is_in_initial_sink_q[block_discovered])
	  if (block_timing[block_discovered].required_arrival_time > required_arrival_t)
	    block_timing[block_discovered].required_arrival_time=required_arrival_t;


	if (block_out_remaining[block_discovered] == 0) {

	  /*previously added blocks are not added to the queue.*/
	  if (!block_is_in_req_arr_time_q[block_discovered]){
	    block_has_req_arr_time_q[next_blk_to_add_to_req_arr_time_q]=block_discovered;
	    next_blk_to_add_to_req_arr_time_q++;
	    block_is_in_req_arr_time_q[block_discovered]=TRUE;
	  }
	}
      } /* block[sink_block]...*/
  
    } /*for (i=start....*/

    q_head++;
  }

  *biggest_num_max_outputs=loc_biggest_num_max_outputs;
}
/******************************/
static void calculate_slack_and_criticality(float inter_cluster_net_delay,
					    float intra_cluster_net_delay,
					    float max_arrival_time, 
					    long sum_biggest_max,
					    int *num_critical_connections) {
  /*assign slack and criticality values to the net_timing structure. It *
   *is calculated for each pin on each net except for the pins which    *
   *are driving the net (they are left at the undefined value of -1).   *
   *Also compute for the criticality array (which is parallel to the    *
   *block array) */

  /*The criticality of a block is determined primarily by the slack on  *
   *it's input nets using the one that has the smallest value. It is    *
   * modified as described below.                                       */


  /*Criticality is determined by three factors.                         *
   *1. The most important, and most highly weighted factor is the slack *
   *   on the nets. A net with very little slack will be assigned a high*
   *   criticality value, as will the block which is being driven by the*  
   *   net.                                                             *
   *The next two factors are used only as a tie-breaking mechanisim for *
   *nets that have the same criticality values. They will never cause a *
   *net with more slack to become more critical than a net with less    *
   *slack.                                                              *
   *2. The number of paths passing through a block adjacent to a net is *
   *   the primary tie breaker for selecting between nets that have     *
   *   equal slacks. The net_forward_criticality of a net is affected   *
   *   by the block driving the net, the net_backward_criticality is    *
   *   affected by the block that is being driven by the net.           *
   *   There are two parameters which keep track of the number of paths *
   *   which pass through a block. The num_max_inputs keeps track of how*
   *   many paths that are on the input side of each block that can be  *
   *   be affected by making a block clustered. The num_max_outputs     *
   *   keeps track of how many paths that are on the output side of each*
   *   block that can be affected by making a block clustered. We weight*
   *   blocks that affect more paths slightly higher than blocks that   *
   *   do not affect as many.                                           *
   *3. Given that there is more than one net with the same slack, and *
   *   same number of paths, the third, and least weighted tie breaker  *
   *   is the distance from the sources. If two blocks have the same    *
   *   value for num_max_inputs and num_max_outputs, then the farther of*
   *   the two blocks and the nets that they are driving are assigned a *
   *   slightly higher criticality.                                     *
   *   Remember, that this is only a tie breaker if number 1. and 2. are*
   *   equal, otherwise it has no effect. The weighting of this         *
   *   parameter is set in SCALE_DISTANCE_VAL, which is small enough to *
   *   ensure that it is only a tie breaker.                            */

  int iblk,iblkpin;
  int startidx,endidx;
  int netpin;
  int currentnet;
  int drivingblock;

  static float *max_num_paths_scaling_value=NULL;
  static float *distance_scaling_value=NULL;

  float currslack, maxslack, minslack;
  float currcriticality;

  int critical_connection_count;

  critical_connection_count = 0;


  /*find slack values*/

  maxslack=0.0;
  minslack = MAX_ALLOWED_PATH_LEN;

  /*define the max_num_paths_scaling_value array if it is undefined*/
  if (max_num_paths_scaling_value == NULL) {
    max_num_paths_scaling_value=(float*)my_malloc(num_blocks * sizeof(float));
    distance_scaling_value=(float*)my_malloc(num_blocks * sizeof(float));
  }

  /*calculate slack values*/
  for (iblk=0;iblk<num_blocks;iblk++) {

    if (block[iblk].type != INPAD) {

      /*if this is an output pad, then the input to the pad is net[0]*/
      if (block[iblk].type == OUTPAD) {
	startidx=0;
	endidx=0;
      }
      else {
	startidx=1;
	endidx=lut_size;
      }


      for (iblkpin=startidx;iblkpin<=endidx;iblkpin++) {

	currentnet=block[iblk].nets[iblkpin];

	if (currentnet!=OPEN) {
	  
	  netpin=block_timing[iblk].net_pin_index[iblkpin];

	  currslack=block_timing[iblk].required_arrival_time-
	    net_timing[currentnet].net_pin_arrival_time[netpin];
	  
	  if (currslack < MIN_SLACK_ALLOWED) 
	    currslack = 0;
	 	  
	  net_timing[currentnet].net_slack[netpin] = currslack;

	  if (fabs(currslack - minslack) < EQUAL_DEF) {
	    critical_connection_count ++;
	  }
	  else if (currslack < minslack) {
	    minslack = currslack;
	    critical_connection_count = 1;
	  }

	  if (currslack > maxslack)
	    maxslack=currslack;
	}    
      }
    }
  }

  /*Initialize the scaling values*/
  for (iblk=0;iblk<num_blocks;iblk++) {

    max_num_paths_scaling_value[iblk]=
      SCALE_NUM_PATHS * 
      ((float)block_timing[iblk].num_max_inputs + 
      (float)block_timing[iblk].num_max_outputs)/(float)sum_biggest_max;

    distance_scaling_value[iblk]=
      SCALE_DISTANCE_VAL *
      block_timing[iblk].delay_from_source/max_arrival_time;
  }

  /*calculate criticality values*/

  for (iblk=0;iblk<num_blocks;iblk++) {

    if (block[iblk].type != INPAD) {

      /*if this is an output pad, then the input to the pad is net[0]*/
      if (block[iblk].type == OUTPAD) {
	startidx=0;
	endidx=0;
      }
      else {
	startidx=1;
	endidx=lut_size;
      }

      for (iblkpin=startidx;iblkpin<=endidx;iblkpin++) {

	currentnet=block[iblk].nets[iblkpin];

	if (currentnet != OPEN) {

	  drivingblock=net[currentnet].pins[0];

	  netpin=block_timing[iblk].net_pin_index[iblkpin];

	  currslack=net_timing[currentnet].net_slack[netpin];


	  /*calculate the criticality of the nets*/

	  /*Net forward criticality tie-breakers are determined by the block *
	   *that is driving the net    */
	  if (maxslack > MIN_SLACK_ALLOWED)
	    currcriticality= (1-currslack/maxslack)+ 
	      max_num_paths_scaling_value[drivingblock] +
	      distance_scaling_value[drivingblock];
	  else
	    currcriticality=1.0 + max_num_paths_scaling_value[drivingblock] +
	      distance_scaling_value[drivingblock];

	  net_timing[currentnet].net_forward_criticality[netpin]=currcriticality; 
	  

	  /*now calculate the criticality of the blocks, and the *
	  * net_backward_criticality values*/

	  /*net_backward_criticality tie-breakers are determined by the block*
	   *that is being driven by the net */

	  /*block criticality tie-breakers are determined by the block under *
           *consideration. Most of the criticality score of a block is       *
	   *determined by the slack on the input net with the smallest       *
           *slack value */
	  if (maxslack > MIN_SLACK_ALLOWED)
	    currcriticality= (1-currslack/maxslack)+ 
	      max_num_paths_scaling_value[iblk] +
	      distance_scaling_value[iblk];
	  else
	    currcriticality=1.0 + max_num_paths_scaling_value[iblk] +
	      distance_scaling_value[iblk];

	  net_timing[currentnet].net_backward_criticality[netpin]=currcriticality; 

	  if (get_cluster_of_block(iblk) == NO_CLUSTER) {
	    if (currcriticality > criticality[iblk]) {
	     criticality[iblk] = currcriticality;
	    }
	  }	  
	}  
      }
    }
  }
  
  *num_critical_connections = critical_connection_count;
}

/******************************/
static void sort_blocks_by_criticality(void) {
  /*sorts the critindex array such that the first value is the position in the *
   *criticality array of the most critical block, and the last value is the    *
   *position in the criticality array of the least critical block, blocks that *
   *have already been clustered have been assigned a criticality of -1 so that *
   *they will appear at the end of the sorted list.*/

  heapsort(critindexarray, criticality, num_blocks);
  indexofcrit=0;

}

/******************************/
#ifdef DEBUG_PATH_LENGTH

static void output_structures(void) {

  /*This function prints out the internal net_timing and block_timing data  *
   *values. If a value is -1 (for slack or criticality) then the value is   *
   *undefined, this is expected on the outputs of blocks.                   */
 
  int blkidx,blkpinidx; 
  int netpin;
  int currentnet;
  static FILE *debuglogfile;

  int startidx, endidx;

 
  debuglogfile=my_fopen("debugPL.log","w",0);

  fprintf(debuglogfile,"BEWARE that this file only represents the final values\n\n");

  /*for each block, print out Required arrival time, and slack on each input*/
  fprintf(debuglogfile,"\n\n    NAME            TYPE Dist From    Required Arrive");
  for (blkpinidx=0;blkpinidx<=lut_size;blkpinidx++)
    fprintf(debuglogfile, " slack[%2d]",blkpinidx);
  fprintf(debuglogfile,"\n");
  fprintf(debuglogfile,"                           Source           Time\n\n");

  for (blkidx=0; blkidx<num_blocks; blkidx++) {
    fprintf(debuglogfile,"%2d: %-15.15s %4d %9.2f %18.2f",blkidx,block[blkidx].name, 
	    block[blkidx].type, block_timing[blkidx].delay_from_source,
	    block_timing[blkidx].required_arrival_time);

    /*if this is an output pad, then the input to the pad is block[].nets[0]*/
    if (block[blkidx].type == OUTPAD){
      startidx=0;
      endidx=0;
    }
    else { 
      startidx=0;
      endidx=lut_size;  /* ignore clock nets (which would be in block[].nets[lut+1] )*/
    }
	   

    /*INPADs do not have inputs, so ignore*/
    if (block[blkidx].type != INPAD) {
      for (blkpinidx=startidx;blkpinidx<=endidx;blkpinidx++) {

	currentnet=block[blkidx].nets[blkpinidx];

	if (currentnet != OPEN) { 
	  netpin=block_timing[blkidx].net_pin_index[blkpinidx];
	  fprintf(debuglogfile,"%9.2f ",net_timing[currentnet].net_slack[netpin]);
	}
	else
	  fprintf(debuglogfile,"          ");

      }
    }
    fprintf(debuglogfile,"\n");
  }


  /*print out criticalities of each net*/
  fprintf(debuglogfile,"\n\n    NAME            ");
  for (blkidx=0;blkidx<=lut_size;blkidx++)
    fprintf(debuglogfile, "Crit[%2d] ",blkidx);
  fprintf(debuglogfile,"\n");
  for (blkidx=0; blkidx<num_blocks; blkidx++) {

    fprintf(debuglogfile,"%2d: %-15.15s",blkidx,block[blkidx].name);

    /*if this is an output pad, then the input to the pad is block[].nets[0]*/
    if (block[blkidx].type == OUTPAD){
      startidx=0;
      endidx=0;
    }
    else { 
      startidx=0;
      endidx=lut_size;  /* ignore clock nets (which would be in block[].nets[lut+1] )*/
    }

    /*INPADs do not have inputs, so ignore*/
    if (block[blkidx].type != INPAD) {

      for (blkpinidx=startidx;blkpinidx<=endidx;blkpinidx++) {

	currentnet=block[blkidx].nets[blkpinidx];

	if (currentnet != OPEN) { 

	  netpin=block_timing[blkidx].net_pin_index[blkpinidx];
	  fprintf(debuglogfile,"%8.2f ",
		  net_timing[currentnet].net_forward_criticality[netpin]);

	}
	else
	  fprintf(debuglogfile,"          ");
      }
    }
    fprintf(debuglogfile,"\n");
  }

  fclose(debuglogfile);

}
#endif



/*****************************************/
#ifdef CRITICAL_LENGTH_STATS
static void print_stats(int num_blocks_clustered, int num_clusters,
                 float farthest_distance,
                 int num_critical_conn, float avg_dist_to_sinks,
		 float avg_point_to_point_dist) {

  static int previous_cluster = -1;
  int cluster_num;
  cluster_num = num_clusters - 1;

  if (cluster_num != previous_cluster) {
    previous_cluster = cluster_num;
    fprintf (length_f, "\n");
  }

  fprintf (length_f, "%7d %7d  %7.2f %7d %8.5f %8.5f\n", 
	   cluster_num, num_blocks_clustered,
	   farthest_distance, num_critical_conn, avg_dist_to_sinks, 
	   avg_point_to_point_dist);

}
#endif
/******************** Globally Accessable Function Definitions *******************/
/*****************************************/
int get_net_pin_number(int blk, int blkpin) {
  /*returns the pin number in the net structure that the pin on this block */
  /*corresponds to */

  return(block_timing[blk].net_pin_index[blkpin]);

}

/*****************************************/
float get_net_pin_forward_criticality(int netnum, int pinnum) {
  /*returns the criticality for a particular branch of a net*/

  return(net_timing[netnum].net_forward_criticality[pinnum]);

}
/*****************************************/
float get_net_pin_backward_criticality(int netnum, int pinnum) {
  /*returns the criticality for a particular block*/

   return(net_timing[netnum].net_backward_criticality[pinnum]);
}
/*****************************************/
boolean report_critical_path(float block_delay, float inter_cluster_net_delay, 
			  float intra_cluster_net_delay, int farthest_block,
			  int cluster_size, boolean print_data) {
  /*finds the most critical path (longest path) and reports the length, and *
   *which blocks and nets are used by the path. */

  /*If all blocks on critical path have been clustered this function returns TRUE*
   *otherwise it returns FALSE.*/

  /*do not delete this function, as it is required to know when the critical path has been*/
  /*minimized as much as it possibly can -- this allows us to go into pin-sharing mode and*/
  /*save execution time */


  int pinidx; 
  int currentblock;
  int nextblock;
  int startidx, endidx;
  int inputnet, inputnetpin;
  int critnet;
  int numclusters, num_clusterable_blocks_on_path;
  int total_num_blocks_on_path, num_ext_nets_on_path;
  float mostcrit, netcrit;
  int cluster_of_nextblock;
  boolean all_clustered;
  boolean first_pass;

  all_clustered=TRUE;
  first_pass = TRUE;

#ifdef CRITICAL_PATH_STATS
  if (print_data) {
    fprintf(critical_report_f,"Using the following values, the most critical "
	    "path is determined\n");

    fprintf(critical_report_f,"block_delay=             %5.2f\n",block_delay);
    fprintf(critical_report_f,"inter_cluster_net_delay= %5.2f\n",inter_cluster_net_delay);
    fprintf(critical_report_f,"intra_cluter_net_delay=  %5.2f\n\n",intra_cluster_net_delay);

    fprintf(critical_report_f,"Critical path is reported from output to input\n\n");
  }
#endif

  if (get_cluster_of_block(farthest_block) == NEVER_CLUSTER){
    numclusters=0;
  }
  else{
    numclusters=1;
  }

  num_ext_nets_on_path=0;
  currentblock=farthest_block;
  num_clusterable_blocks_on_path=0;
  total_num_blocks_on_path=0;

  while (1) {
#ifdef CRITICAL_PATH_STATS
    if (print_data) {
      fprintf(critical_report_f,"Block #%5d \tName:%14s \tArrival Time=%7.2f "
	      "\tCluster=%5d\n",
	      currentblock,block[currentblock].name,
	      block_timing[currentblock].delay_from_source,
	      get_cluster_of_block(currentblock));
    }
#endif

    if (get_cluster_of_block(currentblock) == NO_CLUSTER)
      all_clustered=FALSE;

    total_num_blocks_on_path++;

    if (get_cluster_of_block(currentblock) != NEVER_CLUSTER)
      num_clusterable_blocks_on_path++;

    if (block_is_in_initial_source_q[currentblock] && !first_pass) {
      /*we have found the source of this critical path, we are done*/
      break;
    }
 
    first_pass = FALSE;

    if (block[currentblock].type == OUTPAD){
      startidx=0;
      endidx=0;
    }
    else {
      startidx=1;
      endidx=lut_size;
    }
    mostcrit=0.0;

    nextblock = NO_CLUSTER; 
    critnet = NO_CLUSTER;

    for (pinidx=startidx; pinidx<=endidx; pinidx++) {
      inputnet=block[currentblock].nets[pinidx];

      if (inputnet != OPEN) {
	inputnetpin=block_timing[currentblock].net_pin_index[pinidx];
	netcrit=net_timing[inputnet].net_forward_criticality[inputnetpin];
	if (netcrit > mostcrit) {
	  mostcrit=netcrit;
	  nextblock=net[inputnet].pins[0];
	  critnet=inputnet;
	}
      }
    }

    if (nextblock != NO_CLUSTER)
      cluster_of_nextblock=get_cluster_of_block(nextblock);
    else 
      cluster_of_nextblock = NO_CLUSTER;

    if (cluster_of_nextblock == NO_CLUSTER ||
	get_cluster_of_block(currentblock) != cluster_of_nextblock){
      if (cluster_of_nextblock != NEVER_CLUSTER){
	numclusters++;
      }
      num_ext_nets_on_path++;
    }
    
#ifdef CRITICAL_PATH_STATS
    if (print_data) {
      fprintf(critical_report_f,"Net   #%5d \tName:%14s \tCriticality =%9.5f\n", critnet, 
	      net[critnet].name, mostcrit);
    }
#endif
    currentblock=nextblock;
  }

#ifdef CRITICAL_PATH_STATS
  if (print_data) {
    fprintf(critical_report_f, "\n\n%d clusterable blocks packed into %d Clusters of size %d\n", 
	    num_clusterable_blocks_on_path, numclusters, cluster_size);
    fprintf(critical_report_f, "Total Number of Blocks on Crit Path= %d\n",
	    total_num_blocks_on_path);
    fprintf(critical_report_f, "Number of external nets on Crit Path=%d\n",
	    num_ext_nets_on_path);
  }
  fflush(critical_report_f);

  if (print_data)
    fprintf(critical_report_f,"\n\n");
#endif

  return all_clustered;

}

/*****************************************/
void calculate_timing_information(float block_delay, float inter_cluster_net_delay, 
				  float intra_cluster_net_delay,
				  int num_clusters,
				  int num_blocks_clustered,
				  int *farthest_block_discovered){
  /*This should be called whenever it is desired to have the timing and delay *
   *values updated */

  float max_arrival_time;
  long biggest_num_max_inputs;
  long biggest_num_max_outputs;
  long sum_biggest_max;
  float farthest_distance;
  float avg_dist_of_sinks, avg_point_to_point_dist;
  int num_critical_connections;

  reset_blocks_to_initial_state();

  /*first find distances from sources*/

  distance_from_sources(&max_arrival_time, block_delay, inter_cluster_net_delay, 
			intra_cluster_net_delay, 
			farthest_block_discovered, 
			&biggest_num_max_inputs,
			&farthest_distance,
			&avg_dist_of_sinks,
			&avg_point_to_point_dist);

  /*now find distances from sinks*/

  assign_required_arrival_times_to_max(max_arrival_time);

  update_required_arrival_times(block_delay, inter_cluster_net_delay, 
				intra_cluster_net_delay, &biggest_num_max_outputs);
  
  sum_biggest_max = biggest_num_max_inputs+biggest_num_max_outputs;

  calculate_slack_and_criticality(inter_cluster_net_delay, intra_cluster_net_delay, 
				  max_arrival_time, sum_biggest_max, 
				  &num_critical_connections);

  sort_blocks_by_criticality();

#ifdef CRITICAL_LENGTH_STATS
  print_stats(num_blocks_clustered, num_clusters,
	      farthest_distance, num_critical_connections, avg_dist_of_sinks, 
	      avg_point_to_point_dist);
#endif

#ifdef DEBUG_PATH_LENGTH
  /*output debugging information*/
  output_structures();

#endif
}
/******************************/
int get_most_critical_unclustered_block(void) {
  /*calculate_timing_information must be called before this, or this function*
   *will return garbage */

  /*indexofcrit is a global variable that is reset to 0 each time a          *
   *timing analysis is done (reset in  sort_blocks_by_criticality)           */
 
  int critidx, blkidx;

  for (critidx=indexofcrit; critidx<num_blocks; critidx++) {
    blkidx=critindexarray[critidx];
    if (get_cluster_of_block(blkidx) == NO_CLUSTER) {
      indexofcrit=critidx+1;
      return(blkidx);
    }
  }

  /*if it makes it to here , there are no more blocks available*/
  return(NO_CLUSTER);
}
/******************************/
void alloc_and_init_path_length(int lut_sz, boolean *is_clk){
  /*this only needs to be called once*/

  lut_size=lut_sz;
  is_clock=is_clk;
  
#ifdef CRITICAL_PATH_STATS
  critical_report_f = my_fopen("vpack_critical_path.echo","w",0);
#endif

#ifdef CRITICAL_LENGTH_STATS
  length_f = my_fopen("vpack_critical_length.echo","w",0);

  fprintf(length_f, "%7s %7s %7s %7s %8s %8s\n",
	  "", "Num","","Num","Avg", "Avg");
  fprintf(length_f, "%7s %7s %7s %7s %8s %8s\n",
	  "Cluster","Blocks","Crit","Crit", "Dist","Pt to Pt");
  fprintf(length_f, "%7s %7s %7s %7s %8s %8s\n",
	  "Num","Packed","Dist","Conn","Sinks","Dist");
  fprintf(length_f, "%7s %7s %7s %7s %8s %8s\n",
	  "-------","-------","-------","-------",
	  "--------","--------");
#endif

  alloc_and_init_structures();
  get_sources_and_sinks();
  set_block_input_output_counts();
}
/******************************/

void free_path_length_memory(void){
  /*this should only be called when the internal structures are no longer required*/

  int iblk, inet;

  free(block_has_distance_q);
  free(block_has_req_arr_time_q);
  free(block_in_remaining);
  free(block_out_remaining);
  free(block_in_count);
  free(block_out_count);
  free(block_is_in_distance_q);
  free(block_is_in_initial_source_q);
  free(block_is_in_initial_sink_q);
  free(block_is_in_req_arr_time_q);
  free(criticality);
  free(critindexarray);

  for (iblk=0; iblk<num_blocks;iblk++)
    free(block_timing[iblk].net_pin_index);
  free(block_timing);

  for (inet=0;inet<num_nets;inet++) {
    free(net_timing[inet].net_pin_arrival_time);
    free(net_timing[inet].net_slack);
    free(net_timing[inet].net_forward_criticality);
    free(net_timing[inet].net_backward_criticality);
  }
  free(net_timing);

#ifdef CRITICAL_PATH_STATS
  fclose(critical_report_f);
#endif

#ifdef CRITICAL_LENGTH_STATS
  fclose(length_f);
#endif
}
