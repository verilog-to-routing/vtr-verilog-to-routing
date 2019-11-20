#include <stdio.h>
#include "vpack.h"
#include "ext.h"
#include "util.h"
#include "cluster.h"
#include "output_clustering.h"

#define NO_CLUSTER -1
#define NEVER_CLUSTER -2
#define NOT_VALID -10000  /* Marks gains that aren't valid */
                          /* Ensure no gain can ever be this negative! */

enum e_gain_update {GAIN, NO_GAIN};
enum e_gain_type {SHARED_PINS, INPUT_REDUCTION};
enum e_feasibility {FEASIBLE, INFEASIBLE};
enum e_removal_policy {REMOVE_CLUSTERED, LEAVE_CLUSTERED};

/* Linked list structure.  Stores one integer (iblk). */
struct s_ilink {int iblk; struct s_ilink *next;};

/* 1/MARKED_FRAC is the fraction of nets or blocks that must be *
 * marked in order for the brute force (go through the whole    *
 * data structure linearly) gain update etc. code to be used.   *
 * This is done for speed only; make MARKED_FRAC whatever       *
 * number speeds the code up most.                              */
#define MARKED_FRAC 2   

/* [0..num_nets-1].  How many pins of each net are contained in the *
 * currently open cluster?                                          */
static int *num_pins_of_net_in_cluster;

/* [0..num_nets-1].  Is the driver for this net in the open cluster? */
static boolean *net_output_in_cluster;

/* [0..num_blocks-1].  Which cluster (numbered from 0 to num_cluster - 1) *
 * is this block in?  If not assigned to a cluster yet, it is NO_CLUSTER. */
static int *cluster_of_block;

/* [0..num_blocks-1].  Reduction in number of pins to connect that would*
 * result from each block being added to the currently open cluster.    *
 * gain[iblk] is NOT_VALID if it hasn't been computed for iblk yet.     */
static int *gain;

/* [0..num_marked_nets] and [0..num_marked_blocks] respectively.  List  *
 * the indices of the nets and blocks that have had their num_pins_of_  *
 * net_in_cluster and gain entries altered.                             */
static int *marked_nets, *marked_blocks;
static int num_marked_nets, num_marked_blocks;

/* To speed up some linear searches and such, I store the index of the *
 * first clusterable (LUT, LATCH, etc.) block.                         */
static int first_clusterable_block = -1;

/* Keeps a linked list of the unclustered blocks to speed up looking for *
 * unclustered blocks with a certain number of *external* inputs.        *
 * [0..lut_size].  Unclustered_list_head[i] points to the head of the    *
 * list of LUTs with i inputs to be hooked up via external interconnect. */
static struct s_ilink *unclustered_list_head;
static struct s_ilink *memory_pool; /* Declared here so I can free easily. */

/* Does the block that drives the output of this net also appear as a   *
 * receiver (input) pin of the net?  [0..num_nets-1].  This is used     *
 * in the gain routines to avoid double counting the connections from   *
 * the current cluster to other blocks (hence yielding better           *
 * clusterings).  The only time a block should connect to the same net  *
 * twice is when one connection is an output and the other is an input, *
 * so this should take care of all multiple connections.                */
static boolean *net_output_feeds_driving_block_input;


int num_input_pins (int iblk) {

/* Returns the number of used input pins on this block. */

 int conn_inps;

 switch (block[iblk].type) {

 case LUT:
    conn_inps = block[iblk].num_nets - 1;   /* -1 for output. */
    break;

 case LUT_AND_LATCH:
    conn_inps = block[iblk].num_nets - 2;  /* -2 for output + clock */
    break;

 case LATCH:
    conn_inps = block[iblk].num_nets - 2;  /* -2 for output + clock */
    break;

 default:
/* This routine should only be used for logic blocks. */
    printf("Error in num_input_pins:  Unexpected block type %d"
           "\n for block %d.  Aborting.\n", block[iblk].type, iblk);
    exit(1);
    break;
 }
 
 return (conn_inps);
}


static int num_ext_inputs (int iblk, int lut_size) {

/* Returns the number of input pins on this block that must be hooked * 
 * up through external interconnect.  That is, the number of input    *
 * pins used - the number which connect (internally) to the output.   */

 int ext_inps, output_net, ipin;

 ext_inps = num_input_pins(iblk);

 output_net = block[iblk].nets[0];
 
/* NB:  I could speed things up a bit by computing the number of inputs *
 * and number of external inputs for each logic block at the start of   *
 * clustering and storing them in arrays.  Look into if speed is a      *
 * problem.                                                             */

 for (ipin=1;ipin<=lut_size;ipin++) {
    if (block[iblk].nets[ipin] == output_net)
       ext_inps--;
 }

 return (ext_inps);
}


static void check_clocks (boolean *is_clock, int lut_size) {

/* Checks that nets used as clock inputs to latches are never also used *
 * as LUT inputs.  It's electrically questionable, and more importantly *
 * would break the clustering code.                                     */

 int inet, iblk, ipin;

 for (iblk=0;iblk<num_blocks;iblk++) {
    if (block[iblk].type == LUT || block[iblk].type == LATCH || 
             block[iblk].type == LUT_AND_LATCH) {
       for (ipin=1;ipin<lut_size+1;ipin++) {
          inet = block[iblk].nets[ipin]; 
          if (is_clock[inet]) {
            printf("Error in check_clocks.  Net %d (%s) is a clock, but also\n"
                   "connects to a LUT input on block %d (%s).\n", inet, 
                      net[inet].name, iblk, block[iblk].name);
            printf("This would break the current clustering implementation \n"
                   "and is electrically questionable, so clustering has been\n"
                   "aborted.\n");
            exit (1);
          }
       }
    }
 }
}


static void check_for_duplicate_inputs (int lut_size) {

/* Checks that each input pin of a LUT connects to a distinct net.  *
 * It doesn't make sense from a tech-mapping point of view to feed  *
 * the same input into a LUT more than once, and more importantly,  *
 * some of the clustering code assumes that all LUT inputs are      *
 * unique.                                                          */

 int iblk, ipin, inet;
 int *num_conns_to_net;
 
 num_conns_to_net = my_malloc (num_nets * sizeof(net));

 for (inet=0;inet<num_nets;inet++) 
    num_conns_to_net[inet] = 0;

 for (iblk=0;iblk<num_blocks;iblk++) {
    if (block[iblk].type == LUT || block[iblk].type == LATCH ||
          block[iblk].type == LUT_AND_LATCH) {
       for (ipin=1;ipin<=lut_size;ipin++) {
          inet = block[iblk].nets[ipin];
          if (inet != OPEN) {
             num_conns_to_net[inet]++;
             if (num_conns_to_net[inet] != 1) {
                printf("\nError in check_for_duplicate_inputs.\n"
                       "Block #%d (%s) connects multiple input pins to"
                       " net #%d (%s).\n", iblk, block[iblk].name, inet,
                           net[inet].name);
                printf("Multiple connections to a LUT's inputs doesn't "
                       "make sense from a technology \n"
                       "mapping standpoint, and the clusterer assumes "
                       "LUT inputs are unique.\n"
                       "Aborting.\n");
                exit(1);
             }
          }
       }

/* Reset count for next block. */
       for (ipin=1;ipin<=lut_size;ipin++) {
          inet = block[iblk].nets[ipin];
          if (inet != OPEN) 
             num_conns_to_net[inet] = 0;
       }
    }
 }

 free (num_conns_to_net);
}


static void alloc_and_init_clustering (int lut_size, int cluster_size) {

/* Allocates the main data structures used for clustering and properly *
 * initializes them.                                                   */

 int i, ext_inps, ipin, driving_blk, inet;
 struct s_ilink *next_ptr;

 cluster_of_block = (int *) my_malloc (num_blocks * sizeof(int));
 gain = (int *) my_malloc (num_blocks * sizeof (int));

 for (i=0;i<num_blocks;i++) {
    gain[i] = NOT_VALID;
    if (block[i].type == LUT || block[i].type == LUT_AND_LATCH ||
                 block[i].type == LATCH) {
        cluster_of_block[i] = NO_CLUSTER;
        if (first_clusterable_block == -1)
           first_clusterable_block = i;
    }
    else
        cluster_of_block[i] = NEVER_CLUSTER; 
 }

#ifdef DEBUG
    if (first_clusterable_block != num_p_inputs + num_p_outputs) {
       printf("Error in alloc_and_init_clustering.\n");
       printf("first_clusterable_block = %d, expected %d.\n",
            first_clusterable_block, num_p_inputs + num_p_outputs);
       exit (1);
    }
#endif

 num_pins_of_net_in_cluster = (int *) my_malloc (num_nets * sizeof(int));
 net_output_in_cluster = (boolean *) my_malloc (num_nets * sizeof(boolean));

 for (i=0;i<num_nets;i++) {
    num_pins_of_net_in_cluster[i] = 0;
    net_output_in_cluster[i] = FALSE;
 }

 marked_nets = (int *) my_malloc ((lut_size + 2) * cluster_size * sizeof(int));
 marked_blocks = (int *) my_malloc (num_blocks * sizeof(int));

 num_marked_nets = 0;
 num_marked_blocks = 0;

 unclustered_list_head = (struct s_ilink *) my_malloc ((lut_size+1) * 
                        sizeof(struct s_ilink));
 for (i=0;i<=lut_size;i++)
    unclustered_list_head[i].next = NULL;
 
 memory_pool = (struct s_ilink *) my_malloc ((num_blocks - 
             first_clusterable_block) * sizeof (struct s_ilink));
 next_ptr = memory_pool;
 
 for (i=first_clusterable_block;i<num_blocks;i++) {
    if (block[i].type == LUT || block[i].type == LUT_AND_LATCH ||
                 block[i].type == LATCH) {
       ext_inps = num_ext_inputs(i, lut_size);
       next_ptr->next = unclustered_list_head[ext_inps].next;
       unclustered_list_head[ext_inps].next = next_ptr;
       next_ptr->iblk = i;
       next_ptr++;
    }
 }

 net_output_feeds_driving_block_input = (boolean *) my_malloc (
                 num_nets * sizeof (boolean));

 for (inet=0;inet<num_nets;inet++) {
    net_output_feeds_driving_block_input[inet] = FALSE;
    driving_blk = net[inet].pins[0];
    for (ipin=1;ipin<net[inet].num_pins;ipin++) {
       if (net[inet].pins[ipin] == driving_blk) {
          net_output_feeds_driving_block_input[inet] = TRUE;
          break;
       }
    }
 }
}


static void free_clustering (void) {

/* Releases all the memory used by clustering data structures.   */

 free (cluster_of_block);
 free (gain);
 free (num_pins_of_net_in_cluster);
 free (net_output_in_cluster);
 free (marked_nets);
 free (marked_blocks);
 free (unclustered_list_head);
 free (memory_pool); 
 free (net_output_feeds_driving_block_input);
}


static boolean clocks_feasible (int iblk, int idummy, int clocks_avail,
                int lut_size, boolean *dummy) {

/* Checks if block iblk could be adding to the open cluster without *
 * violating clocking constraints.  Returns TRUE if it's OK.  Some  *
 * parameters are unused since this function needs the same inter-  *
 * face as the other feasibility checkers.                          */

 int inet;
 
/* Recall that LUT output can't internally connect to clocks. */

 inet = block[iblk].nets[lut_size+1];
 if (inet != OPEN) {
    if (num_pins_of_net_in_cluster[inet] == 0) {
       clocks_avail--;
    }   
    else if (num_pins_of_net_in_cluster[inet] == 1 &&
                net_output_in_cluster[inet]) {
       clocks_avail--;
    }   
 }

 if (clocks_avail >= 0) 
    return (TRUE);
 else
    return (FALSE);
}


static int get_block_by_num_ext_inputs (int ext_inps, int lut_size, int 
      clocks_avail, enum e_removal_policy remove_flag) {

/* This routine returns a block which has not been clustered, has  *
 * no connection to the current cluster, satisfies the cluster     *
 * clock constraints, and has ext_inps external inputs.  If        *
 * there is no such block it returns NO_CLUSTER.  Remove_flag      *
 * controls whether or not blocks that have already been clustered *
 * are removed from the unclustered_list data structures.  NB:     *
 * to get a block regardless of clock constraints just set clocks_ *
 * avail > 0.                                                      */

 struct s_ilink *ptr, *prev_ptr;
 int iblk;
 
 prev_ptr = &unclustered_list_head[ext_inps];
 ptr = unclustered_list_head[ext_inps].next;

 while (ptr != NULL) {
    iblk = ptr->iblk;
    if (cluster_of_block[iblk] == NO_CLUSTER) {
       if (clocks_feasible (iblk, 0, clocks_avail, lut_size, NULL)) 
          return (iblk);
    }
    else if (remove_flag == REMOVE_CLUSTERED) {
       prev_ptr->next = ptr->next;
    }
    ptr = ptr->next;
 }
 
 return (NO_CLUSTER);
}


static int get_free_block_with_most_ext_inputs (int lut_size, int inputs_avail, 
       int clocks_avail) {

/* This routine is used to find the first seed block for the clustering   *
 * and to find new blocks for clustering when there are no feasible       *
 * blocks with any attraction to the current cluster (i.e. it finds       *
 * blocks which are unconnected from the current cluster).  It returns    *
 * the block with the largest number of used inputs that satisfies the    *
 * clocking and number of inputs constraints.  If no suitable block is    *
 * found, the routine returns NO_CLUSTER.                                 */

 int iblk, ext_inps, istart;

 istart = min (lut_size, inputs_avail);

 for (ext_inps=istart;ext_inps>=0;ext_inps--) {
    iblk = get_block_by_num_ext_inputs (ext_inps, lut_size, clocks_avail, 
                    REMOVE_CLUSTERED);
    if (iblk != NO_CLUSTER) 
       return (iblk);
 }

 return (NO_CLUSTER);   /* No suitable block found. */
}


static void reset_cluster (int *inputs_used, int *luts_used, 
       int *clocks_used) {

/* Call this routine when starting to fill up a new cluster.  It resets *
 * the gain vector, etc.                                                */

 int i;

/* If statement below is for speed.  If nets are reasonably low-fanout,  *
 * only a relatively small number of blocks will be marked, and updating *
 * only those block structures will be fastest.  If almost all blocks    *
 * have been touched it should be faster to just run through them all    *
 * in order (less addressing and better cache locality).                 */

 *inputs_used = 0;
 *luts_used = 0;
 *clocks_used = 0;

 if (num_marked_blocks < num_blocks/MARKED_FRAC) {
    for (i=0;i<num_marked_blocks;i++) 
       gain[marked_blocks[i]] = NOT_VALID;
 }
 else {
    for (i=first_clusterable_block;i<num_blocks;i++) 
       gain[i] = NOT_VALID;
 }
 
 num_marked_blocks = 0;

/* num_marked_nets is probably always less than num_nets/2, but what the *
 * heck?                                                                 */

 if (num_marked_nets < num_nets/MARKED_FRAC) {
    for (i=0;i<num_marked_nets;i++) {
       num_pins_of_net_in_cluster[marked_nets[i]] = 0;
       net_output_in_cluster[marked_nets[i]] = FALSE;
    }
 }
 else {
    for (i=0;i<num_nets;i++) {
       num_pins_of_net_in_cluster[i] = 0;
       net_output_in_cluster[i] = FALSE;
    }
 }
 
 num_marked_nets = 0;
}


static void mark_and_update_gain (int inet, enum e_gain_update gain_flag,
        enum e_gain_type gain_type, int lut_size) {

/* Updates the marked data structures, and if gain_flag is GAIN,  *
 * the gain when a logic block is added to a cluster.  The gain   *
 * is basically the number of input pins saved when a block is    *
 * added to a cluster (vs. putting it in a cluster with unrelated *
 * logic).  When a logic block has more than one pin of a net     *
 * connected to it (in practice this only happens when you have   *
 * a LUT + LATCH logic block where the output is fed back to an   *
 * input) the gain from a cluster to this block will increase by  *
 * however many pins the net has on this block, rather than 1.  I *
 * consider this a "feature", since it rarely occurs and such     *
 * blocks do save extra pins on the cluster.                      */

 int iblk, ipin, ifirst;

/* Mark net as being visited, if necessary. */

 if (num_pins_of_net_in_cluster[inet] == 0) {
    marked_nets[num_marked_nets] = inet;
    num_marked_nets++;
 }

/* Update gains of affected blocks. */

 if (num_pins_of_net_in_cluster[inet] == 0 && 
           gain_flag == GAIN) {

/* Check if this net lists its driving block twice.  If so, avoid  *
 * double counting this block by skipping the first (driving) pin. */

    if (net_output_feeds_driving_block_input[inet] == FALSE) 
       ifirst = 0;
    else
       ifirst = 1;
    
    for (ipin=ifirst;ipin<net[inet].num_pins;ipin++) {
       iblk = net[inet].pins[ipin];
       if (cluster_of_block[iblk] == NO_CLUSTER) {
          if (gain[iblk] == NOT_VALID) {
             marked_blocks[num_marked_blocks] = iblk;
             num_marked_blocks++;
             if (gain_type == SHARED_PINS) 
                gain[iblk] = 1;
             else              /* INPUT_REDUCTION */
                gain[iblk] = 1 - num_ext_inputs (iblk, lut_size);
          }
          else {
             gain[iblk]++;
          }
       }
    }
 }
    
 num_pins_of_net_in_cluster[inet]++;
}


static void add_to_cluster (int new_blk, int cluster_index, int lut_size, 
      boolean *is_clock, boolean global_clocks, int *inputs_used,
      int *luts_used, int *clocks_used) {

/* Marks new_blk as belonging to cluster cluster_index and updates  *
 * all the gain, etc. structures.  Cluster_index should always      *
 * be the index of the currently open cluster.                      */

 int ipin, inet;

 cluster_of_block[new_blk] = cluster_index;
 (*luts_used)++;
 
 inet = block[new_blk].nets[0];    /* Output pin first. */

/* Output should never be open but I check anyway.  I don't change  *
 * the gain for clock outputs when clocks are globally distributed  *
 * because I assume there is no real need to pack similarly clocked *
 * FFs together then.  Note that by updating the gain when the      *
 * clock driver is placed in a cluster implies that the output of   *
 * LUTs can be connected to clock inputs internally.  Probably not  *
 * true, but it doesn't make much difference, since it will still   *
 * make local routing of this clock very short, and none of my      *
 * benchmarks actually generate local clocks (all come from pads).  */

 if (inet != OPEN) {
    if (!is_clock[inet] || !global_clocks) 
       mark_and_update_gain (inet, GAIN, SHARED_PINS, lut_size);
    else
       mark_and_update_gain (inet, NO_GAIN, SHARED_PINS, lut_size);

    net_output_in_cluster[inet] = TRUE;

    if (num_pins_of_net_in_cluster[inet] > 1 && !is_clock[inet])
       (*inputs_used)--;
 }

 for (ipin=1;ipin<=lut_size;ipin++) {   /*  LUT input pins. */

    inet = block[new_blk].nets[ipin];
    if (inet != OPEN) {
       mark_and_update_gain (inet, GAIN, SHARED_PINS, lut_size);

/* If num_pins_of_net_in_cluster != 1, then either the output is *
 * in the cluster or an input for this net is already in the     *
 * cluster.  In either case we don't need a new pin.             */

       if (num_pins_of_net_in_cluster[inet] == 1) 
          (*inputs_used)++;
    }
 }

/* Note:  The code below ONLY WORKS when nets that go to clock inputs *
 * NEVER go to the input of a LUT.  It doesn't really make electrical *
 * sense for that to happen, and I check this in the check_clocks     *
 * function.  Don't disable that sanity check.                        */

 inet = block[new_blk].nets[lut_size+1];    /* Clock input pin. */
 if (inet != OPEN) {
    if (global_clocks) 
       mark_and_update_gain (inet, NO_GAIN, SHARED_PINS, lut_size);
    else
       mark_and_update_gain (inet, GAIN, SHARED_PINS, lut_size);

    if (num_pins_of_net_in_cluster[inet] == 1) {
       (*clocks_used)++;
    }
    else if (num_pins_of_net_in_cluster[inet] == 2 &&
                net_output_in_cluster[inet]) {
          (*clocks_used)++;
    }
/* Note: unlike inputs, I'm currently assuming there is no internal *
 * connection in a cluster from LUT outputs to clock inputs.        */
 }
}


static boolean inputs_and_clocks_feasible (int iblk, int inputs_avail, 
      int clocks_avail, int lut_size, boolean *is_clock) {

/* Checks if adding iblk to the currently open cluster would violate *
 * the cluster input and clock pin limitations.  Returns TRUE if     *
 * iblk can be added to the cluster, FALSE otherwise.                */

 int ipin, inet, output_net;

/* Output first. */

 output_net = block[iblk].nets[0];
 if (num_pins_of_net_in_cluster[output_net] != 0 && !is_clock[output_net])
      inputs_avail++;

/* Inputs. */

 for (ipin=1;ipin<=lut_size;ipin++) {
    inet = block[iblk].nets[ipin];
    if (inet != OPEN) {
       if (num_pins_of_net_in_cluster[inet] == 0 && inet != output_net)
          inputs_avail--;
    }
 }

 if (clocks_feasible (iblk, inputs_avail, clocks_avail, lut_size,
                      is_clock) == FALSE) 
    return (FALSE); 

 if (inputs_avail >= 0) 
    return (TRUE);
 else
    return (FALSE);
}


static int get_highest_gain_block (int inputs_avail, int clocks_avail,
        int lut_size, boolean *is_clock, boolean (*is_feasible) (
        int iblk, int inputs_avail, int clocks_avail, int lut_size,
        boolean *is_clock)) {

/* This routine finds the block with the highest gain that is   *
 * not currently in a cluster and satisfies the feasibility     *
 * function passed in as is_feasible.  If there are no feasible *
 * blocks it returns NO_CLUSTER.                                */

 int best_gain, best_block, i, iblk;

 best_gain = NOT_VALID + 1; 
 best_block = NO_CLUSTER;

/* Divide into two cases for speed only. */
/* Typical case:  not too many blocks have been marked. */

 if (num_marked_blocks < num_blocks / MARKED_FRAC) {
    for (i=0;i<num_marked_blocks;i++) {
       iblk = marked_blocks[i];
       if (cluster_of_block[iblk] == NO_CLUSTER) {
          if (gain[iblk] > best_gain) {
             if (is_feasible (iblk, inputs_avail, clocks_avail,
                    lut_size, is_clock)) {
                best_gain = gain[iblk];
                best_block = iblk;
             }
          }
       }
    }
 }
 
 else {        /* Some high fanout nets marked lots of blocks. */
    for (iblk=first_clusterable_block;iblk<num_blocks;iblk++) {
       if (cluster_of_block[iblk] == NO_CLUSTER) {
          if (gain[iblk] > best_gain) {
             if (is_feasible (iblk, inputs_avail, clocks_avail,
                    lut_size, is_clock)) {
                best_gain = gain[iblk];
                best_block = iblk;
             }
          }
       }
    }
 }

 return (best_block);
}


static int get_lut_for_cluster (int inputs_avail, int clocks_avail, 
       int cluster_size, int lut_size, int luts_used, boolean *is_clock) {

/* Finds the LUT with the the greatest gain that satisifies the      *
 * input, clock and capacity constraints of a cluster that are       *
 * passed in.  If no suitable block is found it returns NO_CLUSTER.  */
 
 int best_block;

 if (luts_used == cluster_size)
    return (NO_CLUSTER);

 best_block = get_highest_gain_block (inputs_avail, clocks_avail,
                    lut_size, is_clock, inputs_and_clocks_feasible);

/* If no blocks have any gain to the current cluster, the code above *
 * will not find anything.  However, another block with no inputs in *
 * common with the cluster may still be inserted into the cluster.   */

 if (best_block == NO_CLUSTER) 
    best_block = get_free_block_with_most_ext_inputs (lut_size,
                   inputs_avail, clocks_avail);

 return (best_block);
}



static void load_gain_for_hill_climbing (boolean *is_clock, int lut_size) {

/* Since the cost function changes during hill climbing, this routine *
 * reloads the gain (cost function) array with the correct values for *
 * the hill climbing routines.  The gain array is now used to store   *
 * the *reduction* in the number of input pins required by the        *
 * open cluster if each block were added to it.  If adding a block to *
 * the cluster would reduce the number of inputs needed, the gain will*
 * be positive.                                                       */

 int iblk, inet, ipin, i, output_net;

 if (num_marked_blocks < num_blocks / MARKED_FRAC) {
    for (i=0;i<num_marked_blocks;i++) {
       iblk = marked_blocks[i];
       gain[iblk] = 0;

       if (cluster_of_block[iblk] != NO_CLUSTER)
          continue;

       output_net = block[iblk].nets[0];   /* Output */
       if (num_pins_of_net_in_cluster[output_net] > 0 && 
                    !is_clock[output_net])
          gain[iblk]++;

       for (ipin=1;ipin<=lut_size;ipin++) {    /* Inputs */
          inet = block[iblk].nets[ipin];
          if (inet != OPEN) {
             if (num_pins_of_net_in_cluster[inet] == 0 && 
                  inet != output_net) 
                gain[iblk]--;
          }
       }
    }
 }

 else {        /* Lots of blocks have been marked. */
    for (iblk=first_clusterable_block;iblk<num_blocks;iblk++) {
       gain[iblk] = 0;

       if (cluster_of_block[iblk] != NO_CLUSTER)
          continue;

       output_net = block[iblk].nets[0];   /* Output */
       if (num_pins_of_net_in_cluster[output_net] > 0 && 
                    !is_clock[output_net])
          gain[iblk]++;

       for (ipin=1;ipin<=lut_size;ipin++) {    /* Inputs */
          inet = block[iblk].nets[ipin];
          if (inet != OPEN) {
             if (num_pins_of_net_in_cluster[inet] == 0 &&
                   inet != output_net) 
                gain[iblk]--;
          }
       }
    
    }
 }
}


static void reload_gain_for_greedy (int lut_size, boolean *is_clock, 
        int global_clocks) {

/* This routine reloads the gain array with the proper values for *
 * greedy clustering.  It is only used when one wants to find     *
 * a seed using sharing after the hill_climbing step has mucked   *
 * about with the gain array.                                     */

 int iblk, inet, ipin, i;

  if (num_marked_blocks < num_blocks / MARKED_FRAC) {
    for (i=0;i<num_marked_blocks;i++) {
       iblk = marked_blocks[i];
       gain[iblk] = 0;

       if (cluster_of_block[iblk] != NO_CLUSTER)
          continue;

/* net_output_feeds_driving_block_input check avoids double counting. */

       inet = block[iblk].nets[0];   /* Output */
       if (num_pins_of_net_in_cluster[inet] > 0 && !is_clock[inet]
              && net_output_feeds_driving_block_input == FALSE)
          gain[iblk]++;

       for (ipin=1;ipin<=lut_size;ipin++) {    /* Inputs */
          inet = block[iblk].nets[ipin];
          if (inet != OPEN) {
             if (num_pins_of_net_in_cluster[inet] > 0)
                gain[iblk]++;
          }
       }   
       
       if (!global_clocks) {
          inet = block[iblk].nets[lut_size+1];  /* Clock */
          if (inet != OPEN) {
             if (num_pins_of_net_in_cluster[inet] > 1 || 
                     (num_pins_of_net_in_cluster[inet] == 1 &&
                     net_output_in_cluster[inet] == FALSE))
                gain[inet]++;
          }
       }
    }   
 }

 else {        /* Lots of blocks have been marked. */
    for (iblk=first_clusterable_block;iblk<num_blocks;iblk++) {
       gain[iblk] = 0;

       if (cluster_of_block[iblk] != NO_CLUSTER)
          continue;
 
       inet = block[iblk].nets[0];   /* Output */
       if (num_pins_of_net_in_cluster[inet] > 0 && !is_clock[inet]
              && net_output_feeds_driving_block_input == FALSE)
          gain[iblk]++;
 
       for (ipin=1;ipin<=lut_size;ipin++) {    /* Inputs */
          inet = block[iblk].nets[ipin];
          if (inet != OPEN) {
             if (num_pins_of_net_in_cluster[inet] > 0)
                gain[iblk]++;
          }
       }
       if (!global_clocks) { 
          inet = block[iblk].nets[lut_size+1];  /* Clock */ 
          if (inet != OPEN) { 
             if (num_pins_of_net_in_cluster[inet] > 1 ||  
                     (num_pins_of_net_in_cluster[inet] == 1 &&
                     net_output_in_cluster[inet] == FALSE))
                gain[inet]++;   
          }
       }
    }   
 }
}


static int get_free_block_with_fewest_ext_inputs (int lut_size, int 
          clocks_avail) {

/* Used by the hill climbing code.  Finds the LUT that has the     *
 * fewest used inputs and is compatible with the cluster clocks.   *
 * Returns NO_CLUSTER if no suitable block is found.               */

 int iblk, ext_inps;
 
 for (ext_inps=0;ext_inps<=lut_size;ext_inps++) {
    iblk = get_block_by_num_ext_inputs (ext_inps, lut_size, clocks_avail,
                 LEAVE_CLUSTERED);
    if (iblk != NO_CLUSTER)
       return (iblk);
 }

 return (NO_CLUSTER);
}


static int get_most_feasible_block (int inputs_avail, int clocks_avail,
        int lut_size) {

/* Finds the block that is closest to being feasible for the hill    *
 * climbing step.  If no block can ever become feasible (all blocks  *
 * already clustered or have incompatible clocking) the routine      *
 * returns NO_CLUSTER.  Otherwise, it returns the best block to add  *
 * to the cluster and passes back the new number of inputs available *
 * and clocks available through the pointers.                        */

 int best_connected_block, lowest_ext_input_block;

 best_connected_block = get_highest_gain_block (inputs_avail, 
              clocks_avail, lut_size, NULL, clocks_feasible);

/* It could be that there are no clock feasible blocks with any  *
 * connection to the current cluster, or that there are blocks   *
 * that aren't connected to the cluster that have so few inputs  *
 * they are the most feasible blocks.  Hence code below.         */

 lowest_ext_input_block = get_free_block_with_fewest_ext_inputs (
                lut_size, clocks_avail);

 if (best_connected_block == NO_CLUSTER)
    return (lowest_ext_input_block);

 else if (lowest_ext_input_block == NO_CLUSTER) 
    return (best_connected_block);
 
 else {
    if (gain[best_connected_block] >= -num_ext_inputs(
                          lowest_ext_input_block, lut_size))
       return (best_connected_block);
    else
       return (lowest_ext_input_block);
 }
}


static void hill_climbing_add_to_cluster (int new_blk, int cluster_index, 
         int lut_size, boolean *is_clock, int *clocks_avail) {

/* Adds new_blk to the currently open cluster.  New_blk will be clock *
 * feasible but may not be input-feasible.                            */

 int ipin, inet;

 cluster_of_block[new_blk] = cluster_index;
 
 inet = block[new_blk].nets[0];    /* Output pin first. */
 if (!is_clock[inet]) 
    mark_and_update_gain (inet, GAIN, INPUT_REDUCTION, lut_size);
 else
    mark_and_update_gain (inet, NO_GAIN, INPUT_REDUCTION, lut_size);

 net_output_in_cluster[inet] = TRUE;

 for (ipin=1;ipin<=lut_size;ipin++) {   /*  LUT input pins. */
    inet = block[new_blk].nets[ipin];
    if (inet != OPEN) 
       mark_and_update_gain (inet, GAIN, INPUT_REDUCTION, lut_size);
 }

/* Note:  The code below ONLY WORKS when nets that go to clock inputs *
 * NEVER go to the input of a LUT.  It doesn't really make electrical *
 * sense for that to happen, and I check this in the check_clocks     *  
 * function.  Don't disable that sanity check.                        */ 
 
 inet = block[new_blk].nets[lut_size+1];    /* Clock input pin. */
 if (inet != OPEN) {
    mark_and_update_gain (inet, NO_GAIN, INPUT_REDUCTION, lut_size);
 
    if (num_pins_of_net_in_cluster[inet] == 1) {
       (*clocks_avail)--;
    }
    else if (num_pins_of_net_in_cluster[inet] == 2 &&
                net_output_in_cluster[inet]) {
          (*clocks_avail)--;
    }
/* Note: unlike inputs, I'm currently assuming there is no internal *
 * connection in a cluster from LUT outputs to clock inputs.        */
 }
}


static void  hill_climbing (int initial_inputs_avail, int clocks_avail, 
         int cluster_size, int lut_size, int initial_luts_used, 
         int cluster_index, boolean *is_clock, boolean global_clocks,
         enum e_cluster_seed cluster_seed_type) {

/* This routine is called when we find that no more luts can be packed *
 * into the open cluster.  If this is because the cluster is full,     *
 * we immediately exit since there's nothing to do.  If there is still *
 * room in the cluster, however, we may be able to pack more luts in.  *
 * This routine then looks for LUTs which meet the clocking            *
 * constraints of the open cluster which will produce the smallest     *
 * increase in the number of inputs used.  Because adding a LUT can    *
 * *reduce* the number of inputs required by a cluster (when all       *
 * inputs are already in the cluster, and the output of the lut        *
 * already is an input to the cluster), an infeasible cluster could    *
 * become feasible after more luts are added to it.  Hence the hill    *
 * climbing name.  Note also that the attraction function is different *
 * in this function than in the first pass clustering routine -- here  *
 * we're just looking for luts that will fit, not ones with the most   *
 * pins shared.  The routine keeps track after each move of whether or *
 * not the resulting cluster is feasible.  Once the cluster is full    *
 * the cluster state is rolled back to its last feasible configuration.*
 * If this routine didn't accomplish anything that configuration will  *
 * be the cluster config. when this routine was called.                */

/* I have to keep track of the key cluster information after each *
 * block is added so that I can later "roll back" the cluster to  *
 * the last feasible configuration.                               */

 static int *inputs_avail = NULL;    /* 0..cluster_size */
 static int *block_added = NULL;     /* Which block was added? */
 int luts_used, iblk, i, ipin, inet;

/* Only allocate once.  Ugly hack, but what can I do?  Can't free the *
 * memory easily when it's done this way either.                      */
 if (inputs_avail == NULL) {
    inputs_avail = (int *) my_malloc (cluster_size * sizeof(int));
    block_added = (int *) my_malloc (cluster_size * sizeof(int));
 }

 if (initial_luts_used == cluster_size) 
    return;

 luts_used = initial_luts_used;
 inputs_avail[luts_used-1] = initial_inputs_avail;

 load_gain_for_hill_climbing (is_clock, lut_size);

 while (luts_used != cluster_size) {
    iblk = get_most_feasible_block (inputs_avail[luts_used-1], 
             clocks_avail, lut_size);
    if (iblk == NO_CLUSTER)
       break;

    luts_used++;
    inputs_avail[luts_used-1] = inputs_avail[luts_used-2] + gain[iblk];
    block_added[luts_used-1] = iblk;
    hill_climbing_add_to_cluster (iblk, cluster_index, lut_size, 
         is_clock, &clocks_avail);

/* Early exit check.  Each LUT added can reduce the number of inputs *
 * required by at most one, so check if it's hopeless.               */

    if (inputs_avail[luts_used-1] + cluster_size - luts_used < 0)
       break;
 }

/* Now roll back to the last feasible clustering. */
/* NB:  A future enhancement would be to try replicating blocks.  When *
 * a clustering with i blocks isn't feasible below, try finding a      *
 * block which if replicated would reduce the inputs used by one and   *
 * replace the i'th block with it and so on.                           */
 
 for (i=luts_used-1;i>=initial_luts_used;i--) {
    if (inputs_avail[i] >= 0)   /* Found feasible solution! */
       break;
    iblk = block_added[i];

/* Infeasible.  Back up one block.  */
    cluster_of_block[iblk] = NO_CLUSTER;

/* Back up code below only necessary if I need the correct gains to *
 * the current cluster in order to find the new seed block.         */

    if (cluster_seed_type == MAX_SHARING) {
       inet = block[iblk].nets[0];  /* Output */
       net_output_in_cluster[inet] = FALSE;
       num_pins_of_net_in_cluster[inet]--;
    
       for (ipin=1;ipin<=lut_size+1;ipin++) {   /* Inputs + clock */
          inet = block[iblk].nets[ipin];
          if (inet != OPEN) 
             num_pins_of_net_in_cluster[inet]--; 
       }
    }
 }

 if (cluster_seed_type == MAX_SHARING) 
    reload_gain_for_greedy (lut_size, is_clock, global_clocks);
}


static void alloc_and_load_cluster_info (int ***cluster_contents_ptr, 
        int **cluster_occupancy_ptr, int num_clusters, int cluster_size) {

/* Allocates and loads the cluster_contents and cluster_occupancy    *
 * arrays.  Also checks that all logic blocks have been assigned to  *
 * a legal cluster, no clusters have more than cluster_size logic    *
 * blocks, and that no pads have been put into clusters.             */

 int **cluster_contents;  /* [0..num_clusters-1][0..cluster_size-1] */
 int *cluster_occupancy;  /* [0..num_clusters-1] */
 int iblk, icluster;

 cluster_contents = (int **) alloc_matrix (0,num_clusters-1,0,cluster_size-1,
                                 sizeof(int));
 cluster_occupancy = (int *) my_calloc (num_clusters, sizeof (int));

 for (iblk=0;iblk<num_blocks;iblk++) {
 
    if (block[iblk].type == LUT || block[iblk].type == LATCH ||
             block[iblk].type == LUT_AND_LATCH) {
       icluster = cluster_of_block[iblk];
       if (icluster < 0 || icluster > num_clusters-1) {
          printf("Error in alloc_and_load_cluster_info:  "
                "cluster number (%d) for block\n"
                "%d is out of range.\n", icluster, iblk);
          exit (1);
       } 
 
       cluster_occupancy[icluster]++;
       if (cluster_occupancy[icluster] > cluster_size) {
          printf("Error in alloc_and_load_cluster_info:  "
                 "cluster %d contains more than\n"
                 "%d blocks.\n", icluster, cluster_size);
          exit (1);
       } 
       cluster_contents[icluster][cluster_occupancy[icluster]-1] = iblk;
 
    }
 
    else {    /* INPAD or OUTPAD */
       icluster = cluster_of_block[iblk];
       if (icluster != NEVER_CLUSTER) {
          printf("Error in alloc_and_load_cluster_info:  "
                 "block %d is a pad but has\n"
                 "been assigned to cluster %d.\n", iblk, icluster);
          exit (1);
       } 
    }
 }

 *cluster_occupancy_ptr = cluster_occupancy;
 *cluster_contents_ptr = cluster_contents;
}


static void check_clustering (int num_clusters, int cluster_size, 
        int inputs_per_cluster, int clocks_per_cluster, int lut_size,
        boolean *is_clock, int **cluster_contents, int *cluster_occupancy) {

/* This routine checks that the clustering we have found is indeed a *
 * legal clustering.  It also outputs various statistics about the   *
 * clustering.                                                       */
 
 int iblk, inet, icluster, i, ipin;
 int inputs_used, clocks_used;
 int max_inputs_used, min_inputs_used, summ_inputs_used;
 int max_clocks_used, min_clocks_used, summ_clocks_used;
 int max_occupancy, min_occupancy, num_logic_blocks;

/* Note:  some checking was already done in alloc_and_load_cluster_info. */

/* Brute-force reset of data structures I will need during checking.  */

 for (inet=0;inet<num_nets;inet++) {
    num_pins_of_net_in_cluster[inet] = 0;
    net_output_in_cluster[inet] = FALSE;
 }

/* Check that each cluster satisifies the input and clock pin  *
 * constraints and computes some statistics.                   */

 max_inputs_used = -1;
 max_clocks_used = -1;
 max_occupancy = -1;
 min_inputs_used = inputs_per_cluster + 1;
 min_clocks_used = clocks_per_cluster + 1;
 min_occupancy = cluster_size + 1;
 summ_inputs_used = 0;
 summ_clocks_used = 0;

 for (icluster=0;icluster<num_clusters;icluster++) {

    if (cluster_occupancy[icluster] <= 0) {
       printf("Error:  cluster %d contains %d logic blocks.\n", icluster, 
               cluster_occupancy[icluster]);
       exit (1);
    }

    inputs_used = 0;
    clocks_used = 0;

    for (i=0;i<cluster_occupancy[icluster];i++) {
       iblk = cluster_contents[icluster][i];

       inet = block[iblk].nets[0];              /* Output */
       if (num_pins_of_net_in_cluster[inet] > 0 && !is_clock[inet]) 
          inputs_used--;
       num_pins_of_net_in_cluster[inet]++;
       net_output_in_cluster[inet] = TRUE;
       
       for (ipin=1;ipin<=lut_size;ipin++) {  /* LUT inputs */
          inet = block[iblk].nets[ipin];
          if (inet != OPEN) {
             if (num_pins_of_net_in_cluster[inet] == 0)  
                inputs_used++;
             num_pins_of_net_in_cluster[inet]++;
          }
       }

       inet = block[iblk].nets[lut_size+1];     /* Clock */
          if (inet != OPEN) {
             if (num_pins_of_net_in_cluster[inet] == 0 ||
                     (num_pins_of_net_in_cluster[inet] == 1 && 
                     net_output_in_cluster[inet])) 
                clocks_used++;
             num_pins_of_net_in_cluster[inet]++;
  
          }
    }

    if (inputs_used > inputs_per_cluster) {
       printf("Error in check_clustering:  cluster %d uses %d inputs.\n",
               icluster, inputs_used);
       exit (1);
    }
    if (clocks_used > clocks_per_cluster) {
       printf("Error in check_clustering:  cluster %d uses %d clocks.\n",
               icluster, clocks_used);
       exit (1);
    }
    
/* Reset data structures for check of next cluster */

    for (i=0;i<cluster_occupancy[icluster];i++) {
       iblk = cluster_contents[icluster][i];
       inet = block[iblk].nets[0];
       num_pins_of_net_in_cluster[inet] = 0;
       net_output_in_cluster[inet] = FALSE;

       for (ipin=1;ipin<=lut_size+1;ipin++) {
          inet = block[iblk].nets[ipin];
          if (inet != OPEN) 
             num_pins_of_net_in_cluster[inet] = 0;
       }
    }

/* Compute statistical information. */

    min_inputs_used = min (min_inputs_used, inputs_used);
    max_inputs_used = max (max_inputs_used, inputs_used);
    summ_inputs_used += inputs_used;

    min_clocks_used = min (min_clocks_used, clocks_used);
    max_clocks_used = max (max_clocks_used, clocks_used);
    summ_clocks_used += clocks_used;
 
    min_occupancy = min (min_occupancy, cluster_occupancy[icluster]);
    max_occupancy = max (max_occupancy, cluster_occupancy[icluster]);

 }      /* End for loop over all clusters. */


 printf("Completed clustering consistency check successfully.\n\n");

 printf("Clustering Statistics:\n\n");
 num_logic_blocks = num_blocks - num_p_inputs - num_p_outputs;
 printf("%d Logic Blocks packed into %d Clusters.\n", num_logic_blocks,
         num_clusters);
 printf("\n\t\t\tAverage\t\tMin\tMax\n");
 printf("Logic Blocks / Cluster\t%f\t%d\t%d\n", (float) num_logic_blocks /
        (float) num_clusters, min_occupancy, max_occupancy);
 printf("Used Inputs / Cluster\t%f\t%d\t%d\n", (float) summ_inputs_used /
        (float) num_clusters, min_inputs_used, max_inputs_used);
 printf("Used Clocks / Cluster\t%f\t%d\t%d\n", (float) summ_clocks_used / 
        (float) num_clusters, min_clocks_used, max_clocks_used);
 printf("\n");
}


void do_clustering (int cluster_size, int inputs_per_cluster,
       int clocks_per_cluster, int lut_size, boolean global_clocks,
       boolean *is_clock, boolean hill_climbing_flag, 
       enum e_cluster_seed cluster_seed_type, char *out_fname) {

/* Does the actual work of clustering multiple LUT+FF logic blocks *
 * into clusters.                                                  */

 int num_clusters, istart;
 int inputs_used, luts_used, clocks_used, next_blk;
 int *cluster_occupancy, **cluster_contents;

 check_clocks (is_clock, lut_size);
 check_for_duplicate_inputs (lut_size);
 alloc_and_init_clustering (lut_size, cluster_size);
 num_clusters = 0;
 
 istart = get_free_block_with_most_ext_inputs (lut_size, 
            inputs_per_cluster, clocks_per_cluster);

 while (istart != NO_CLUSTER) {
    reset_cluster(&inputs_used, &luts_used, &clocks_used);
    num_clusters++;
    add_to_cluster(istart, num_clusters - 1, lut_size, is_clock, 
         global_clocks, &inputs_used, &luts_used, &clocks_used);

    next_blk = get_lut_for_cluster (inputs_per_cluster - inputs_used, 
               clocks_per_cluster - clocks_used, cluster_size, lut_size,
               luts_used, is_clock);

    while (next_blk != NO_CLUSTER) {
       add_to_cluster(next_blk, num_clusters - 1, lut_size, is_clock, 
                  global_clocks, &inputs_used, &luts_used, &clocks_used);
       next_blk = get_lut_for_cluster (inputs_per_cluster - inputs_used, 
                  clocks_per_cluster - clocks_used, cluster_size, 
                  lut_size, luts_used, is_clock);
    }
   
    if (hill_climbing_flag)
       hill_climbing (inputs_per_cluster - inputs_used,
                  clocks_per_cluster - clocks_used, cluster_size, lut_size, 
                  luts_used, num_clusters - 1, is_clock, global_clocks,
                  cluster_seed_type);

    if (cluster_seed_type == MAX_SHARING) 
       istart = get_lut_for_cluster (inputs_per_cluster, clocks_per_cluster,
                cluster_size, lut_size, 0, is_clock);
    else
       istart = get_free_block_with_most_ext_inputs (lut_size, 
                  inputs_per_cluster, clocks_per_cluster);
 }

 alloc_and_load_cluster_info (&cluster_contents, &cluster_occupancy, 
               num_clusters, cluster_size); 
 check_clustering (num_clusters, cluster_size, inputs_per_cluster,
               clocks_per_cluster, lut_size, is_clock, cluster_contents,
               cluster_occupancy);
 output_clustering (cluster_contents, cluster_occupancy, cluster_size,
               inputs_per_cluster, clocks_per_cluster, num_clusters, 
               lut_size, global_clocks, is_clock, out_fname);

 free (cluster_occupancy);
 free_matrix ((void **) cluster_contents, 0, num_clusters-1, 0, 
           sizeof (int));
 free_clustering ();
}
