#ifndef SPEC
#define DEBUG 1     /* Echoes input & checks error conditions */
                    /* Only causes about a 1% speed degradation in V 3.10 */
#endif

/*#define PRINT_SINK_DELAYS*/ /*prints the sink delays to files*/
/*#define PRINT_NET_SLACKS*/ /*prints out all slacks in the circuit*/
/*#define PRINT_PLACE_CRIT_PATH*/ /*prints out placement estimated critical path*/
/*#define PRINT_NET_DELAYS*/ /*prints out delays for all connections*/
/*#define PRINT_TIMING_GRAPH*/ /*prints out the timing graph */

#ifdef SPEC
#define NO_GRAPHICS    /* Rips out graphics (for non-X11 systems)      */
#define NDEBUG         /* Turns off assertion checking for extra speed */
#endif

#define TOKENS " \t\n"   /* Input file parsing. */
/*#define VERBOSE 1*/      /* Prints all sorts of intermediate data */

/* Block Types */

enum e_block_types {CLB, OUTPAD, INPAD, IO, ILLEGAL};

enum e_pin_type {OPEN = -1, DRIVER = 0, RECEIVER = 1};
/* Pin is unconnected, driving a net or in the fanout, respectively. */

enum e_side {TOP = 0, BOTTOM = 1, LEFT = 2, RIGHT = 3};

#define MINOR 0  /* For update_screen.  Denotes importance of update. */
#define MAJOR 1

#define HUGE_FLOAT 1.e30

/* Want to avoid overflows of shorts.  OPINs can have edges to 4 * width if  *
 * they are on all 4 sides, so set MAX_CHANNEL_WIDTH to 8000.                */

#define MAX_CHANNEL_WIDTH 8000
#define MAX_SHORT 32767

enum sched_type {AUTO_SCHED, USER_SCHED};   /* Annealing schedule */

enum pic_type {NO_PICTURE, PLACEMENT, ROUTING};  /* What's on screen? */

/* For the placer.  Different types of cost functions that can be used. */

enum place_c_types {LINEAR_CONG, NONLINEAR_CONG};

enum e_operation {PLACE_AND_ROUTE, PLACE_ONLY, ROUTE_ONLY, 
                  TIMING_ANALYSIS_ONLY};

enum pfreq {PLACE_NEVER, PLACE_ONCE, PLACE_ALWAYS};

/* Are the pads free to be moved, locked in a random configuration, or *
 * locked in user-specified positions?                                 */

enum e_pad_loc_type {FREE, RANDOM, USER};
                                

struct s_net {char *name; int num_pins; int *blocks; int *blk_pin; };

/* name:  ASCII net name for informative annotations in the output.          *
 * num_pins:  Number of pins on this net.                                    *
 * blocks: [0..num_pins-1]. Contains the blocks to which the pins of this    *
 *         net connect.  Output in pins[0], inputs in other entries.         *
 * blk_pin: [0..num_pins-1]. Contains the number of the pin (on a block) to  *
 *          which each net terminal connects.  Since I/O pads have only one  *
 *          pin, I set blk_pin to OPEN for them (it should only be used for  *
 *          clb pins).  For clbs, it is the block pin number, as expected.   */


struct s_block {char *name; enum e_block_types type; int *nets; int x;
        int y;}; 

/* name:  Taken from the net which it drives.                        *
 * type:  CLB, INPAD or OUTPAD                                       *
 * nets[]:  List of nets connected to this block.  If nets[i] = OPEN *
            no net is connected to pin i.                            *
 * x,y:  physical location of the placed block.                      */ 


struct s_clb {enum e_block_types type; int occ; union { int block; 
          int *io_blocks;} u;};

/* type: CLB, IO or ILLEGAL.                                         *
 * occ:  number of logical blocks in this physical group.            *
 * u.block: number of the block occupying this group if it is a CLB. *
 * u.io_blocks[]: numbers of other blocks occupying groups (for      *
 *                IO's), up to u.io_blocks[occ - 1]                   */


struct s_bb {int xmin; int xmax; int ymin; int ymax;};

/* Stores the bounding box of a net in terms of the minimum and  *
 * maximum coordinates of the blocks forming the net, clipped to *
 * the region (1..nx, 1..ny).                                    */


enum e_stat {UNIFORM, GAUSSIAN, PULSE, DELTA};
typedef struct {enum e_stat type; float peak; float width; float xpeak;
          float dc; } t_chan;

/* Width is standard dev. for Gaussian.  xpeak is where peak     *
 * occurs. dc is the dc offset for Gaussian and pulse waveforms. */


typedef struct {float chan_width_io; t_chan chan_x_dist; t_chan chan_y_dist; }
          t_chan_width_dist;

/* chan_width_io:  The relative width of the I/O channel between the pads    *
 *                 and logic array.                                          *
 * chan_x_dist: Describes the x-directed channel width distribution.         *
 * chan_y_dist: Describes the y-directed channel width distribution.         */


struct s_class {enum e_pin_type type; int num_pins; int *pinlist;};   

/* type:  DRIVER or RECEIVER (what is this pinclass?)              *
 * num_pins:  The number of logically equivalent pins forming this *
 *           class.                                                *
 * pinlist[]:  List of clb pin numbers which belong to this class. */


struct s_place_region {float capacity; float inv_capacity;
    float occupancy; float cost;};

/* capacity:   Capacity of this region, in tracks.               *
 * occupancy:  Expected number of tracks that will be occupied.  *
 * cost:       Current cost of this usage.                       */


typedef struct {char *name; int output; int clock; int *inputs;} t_subblock;

/* This structure stores the contents of each logic block, in terms     *
 * of the basic LUTs that make up the cluster.  This information is     *
 * used only for timing analysis.  Note that it is possible to          *
 * describe essentially arbitrary timing patterns inside a logic        *
 * block via the correct pattern of LUTs.                               *
 * name:    Name of this subblock.                                      *
 * output:  Number of the clb pin which the LUT output drives, or OPEN  *
 * clock:   Number of clb pin that drives the clock (or OPEN)           *
 * inputs:  [0..sub_block_lut_size-1].  Number of clb pin that drives   *
 *          this input, or number of subblock output + pins_per_clb if  *
 *          this pin is driven by a subblock output, or OPEN if unused. */


typedef struct {t_subblock **subblock_inf; int *num_subblocks_per_block;
        int max_subblocks_per_block; int subblock_lut_size; int num_ff;
        int num_const_gen; struct s_linked_vptr *chunk_head_ptr;} 
        t_subblock_data;

/* This structure contains all the information relevant to subblocks (what's *
 * in each logic block).  This makes it easy to pass around the subblock     *
 * data all at once.  This stuff is used only for timing analysis.           *
 * subblock_inf: [0..num_blocks-1][0..num_subblock_per_block[iblk]-1].       *
 *               Contents of each logic block.  Not valid for IO blocks.     *
 * num_subblock_per_block:  [0..num_blocks-1].  Number of subblocks in each  *
 *                  block.  0 for IOs, between 1 and max_subblocks_per_block *
 *                  for CLBs.                                                *
 * max_subblocks_per_block:  Maximum number of subblocks any logic block can *
 *                           have.                                           *
 * subblock_lut_size:  Number of inputs to each subblock LUT.                *
 * num_ff:  Number of flip flops in the input netlist (i.e. clocked sblks).  *
 * num_const_gen:  Number of subblock constant generators in the netlist.    *
 * chunk_head_ptr: Points to the start of the list of memory chunks used for *
 *                 subblock storage.  Needed to free the subblock data.      */


struct s_annealing_sched {enum sched_type type; float inner_num; float init_t;
     float alpha_t; float exit_t;};

/* Annealing schedule information for the placer.  The schedule type      *
 * is either USER_SCHED or AUTO_SCHED.  Inner_num is multiplied by        *
 * num_blocks^4/3 to find the number of moves per temperature.  The       *
 * remaining information is used only for USER_SCHED, and have the        *
 * obvious meanings.                                                      */


enum e_place_algorithm {BOUNDING_BOX_PLACE, NET_TIMING_DRIVEN_PLACE, PATH_TIMING_DRIVEN_PLACE};
struct s_placer_opts {enum e_place_algorithm place_algorithm;
       float timing_tradeoff;
       int block_dist;
       enum place_c_types place_cost_type; float place_cost_exp;
       int place_chan_width; enum e_pad_loc_type pad_loc_type; 
       char *pad_loc_file; enum pfreq place_freq; int num_regions;
       int recompute_crit_iter;
       boolean enable_timing_computations;
       int inner_loop_recompute_divider;
       float td_place_exp_first;
       float td_place_exp_last;};

/* Various options for the placer.                                           *
 * place_algorithm:  BOUNDING_BOX_PLACE or NET_TIMING_DRIVEN_PLACE, or       *
 *                   PATH_TIMING_DRIVEN_PLACE                                *
 * timing_tradeoff:  When TIMING_DRIVEN_PLACE mode, what is the tradeoff *
 *                   timing driven and BOUNDING_BOX_PLACE.                   *
 * block_dist:  Initial guess of how far apart blocks on the critical path   *
 *              This is used to compute the initial slacks and criticalities *
 * place_cost_type:  LINEAR_CONG or NONLINEAR_CONG.                          *
 * place_cost_exp:  Power to which denominator is raised for linear_cong.    *
 * place_chan_width:  The channel width assumed if only one placement is     *
 *                    performed.                                             *
 * pad_loc_type:  Are pins FREE, fixed randomly, or fixed from a file.  *
 * pad_loc_file:  File to read pin locations form if pad_loc_type  *
 *                     is USER.                                              *
 * place_freq:  Should the placement be skipped, done once, or done for each *
 *              channel width in the binary search.                          *
 * num_regions:  Used only with NONLINEAR_CONG; in that case, congestion is  *
 *               computed on an array of num_regions x num_regions basis.    *
 * recompute_crit_iter: how many temperature stages pass before we recompute *
 *               criticalities based on average point to point delay         *
 * enable_timing_computations: in bounding_box mode, normally, timing        *
 *               information is not produced, this causes the information    *
 *               to be computed. in *_TIMING_DRIVEN modes, this has no effect*
 * inner_loop_crit_divider: (move_lim/inner_loop_crit_divider) determines how*
 *               many inner_loop iterations pass before a recompute of       *
 *               criticalities is done.                                      *
 * td_place_exp_first: exponent that is used on the timing_driven criticlity *
 *               it is the value that the exponent starts at.                *
 * td_place_exp_last: value that the criticality exponent will be at the end */


enum e_route_type {GLOBAL, DETAILED};
enum e_router_algorithm {BREADTH_FIRST, TIMING_DRIVEN};
enum e_base_cost_type {INTRINSIC_DELAY, DELAY_NORMALIZED, DEMAND_ONLY}; 
#define NO_FIXED_CHANNEL_WIDTH -1

struct s_router_opts {float first_iter_pres_fac; float initial_pres_fac;
   float pres_fac_mult; float acc_fac; float bend_cost;
   int max_router_iterations; int bb_factor; enum e_route_type route_type;
   int fixed_channel_width; enum e_router_algorithm router_algorithm;
   enum e_base_cost_type base_cost_type; float astar_fac; 
   float max_criticality; float criticality_exp;}; 

/* All the parameters controlling the router's operation are in this        *
 * structure.                                                               *
 * first_iter_pres_fac:  Present sharing penalty factor used for the        *
 *                 very first (congestion mapping) Pathfinder iteration.    *
 * initial_pres_fac:  Initial present sharing penalty factor for            *
 *                    Pathfinder; used to set pres_fac on 2nd iteration.    *
 * pres_fac_mult:  Amount by which pres_fac is multiplied each              *
 *                 routing iteration.                                       *
 * acc_fac:  Historical congestion cost multiplier.  Used unchanged         *
 *           for all iterations.                                            *
 * bend_cost:  Cost of a bend (usually non-zero only for global routing).   *
 * max_router_iterations:  Maximum number of iterations before giving       *
 *                up.                                                       *
 * bb_factor:  Linear distance a route can go outside the net bounding      *
 *             box.                                                         *
 * route_type:  GLOBAL or DETAILED.                                         *
 * fixed_channel_width:  Only attempt to route the design once, with the    *
 *                       channel width given.  If this variable is          *
 *                       == NO_FIXED_CHANNEL_WIDTH, do a binary search      *
 *                       on channel width.                                  *
 * router_algorithm:  BREADTH_FIRST or TIMING_DRIVEN.  Selects the desired  *
 *                    routing algorithm.                                    *
 * base_cost_type: Specifies how to compute the base cost of each type of   *
 *                 rr_node.  INTRINSIC_DELAY -> base_cost = intrinsic delay *
 *                 of each node.  DELAY_NORMALIZED -> base_cost = "demand"  *
 *                 x average delay to route past 1 CLB.  DEMAND_ONLY ->     *
 *                 expected demand of this node (old breadth-first costs).  *
 *                                                                          *
 * The following parameters are used only by the timing-driven router.      *
 *                                                                          *
 * astar_fac:  Factor (alpha) used to weight expected future costs to       *
 *             target in the timing_driven router.  astar_fac = 0 leads to  *
 *             an essentially breadth-first search, astar_fac = 1 is near   *
 *             the usual astar algorithm and astar_fac > 1 are more         *
 *             aggressive.                                                  *
 * max_criticality: The maximum criticality factor (from 0 to 1) any sink   *
 *                  will ever have (i.e. clip criticality to this number).  *
 * criticality_exp: Set criticality to (path_length(sink) / longest_path) ^ *
 *                  criticality_exp (then clip to max_criticality).         */
 

enum e_switch_block_type {SUBSET, WILTON, UNIVERSAL};
enum e_Fc_type {ABSOLUTE, FRACTIONAL};

struct s_det_routing_arch {enum e_Fc_type Fc_type; float Fc_output; 
   float Fc_input; float Fc_pad; enum e_switch_block_type switch_block_type;
   int num_segment; short num_switch; short delayless_switch; short 
   wire_to_ipin_switch; float R_minW_nmos; float R_minW_pmos;};

/* Defines the detailed routing architecture of the FPGA.  Only important   *
 * if the route_type is DETAILED.                                           *
 * Fc_type:   Are the Fc values below absolute numbers, or fractions of W?  *
 * Fc_output:  Number of tracks to which each clb output pin connect in     *
 *             each channel to which it is adjacent.                        *
 * Fc_input:  Number of tracks to which each clb input pin connects.        *
 * Fc_pad:    Number of tracks to which each I/O pad connects.              *
 * switch_block_type:  Pattern of switches at each switch block.  I         *
 *           assume Fs is always 3.  If the type is SUBSET, I use a         *
 *           Xilinx-like switch block where track i in one channel always   *
 *           connects to track i in other channels.  If type is WILTON,     *
 *           I use a switch block where track i does not always connect     *
 *           to track i in other channels.  See Steve Wilton, Phd Thesis,   *
 *           University of Toronto, 1996.  The UNIVERSAL switch block is    *
 *           from Y. W. Chang et al, TODAES, Jan. 1996, pp. 80 - 101.       *
 * num_segment:  Number of distinct segment types in the FPGA.              *
 * num_switch:  Number of distinct switch types (pass transistors or        *
 *              buffers) in the FPGA.                                       *
 * delayless_switch:  Index of a zero delay switch (used to connect things  *
 *                    that should have no delay).                           *
 * wire_to_ipin_switch:  Index of a switch used to connect wire segments    *
 *                       to clb or pad input pins (IPINs).                  *
 * R_minW_nmos:  Resistance (in Ohms) of a minimum width nmos transistor.   *
 *               Used only in the FPGA area model.                          *
 * R_minW_pmos:  Resistance (in Ohms) of a minimum width pmos transistor.   */


typedef struct {float frequency; int length; short wire_switch; 
        short opin_switch; float frac_cb; float frac_sb; boolean longline; 
        float Rmetal; float Cmetal;} t_segment_inf;

/* Lists all the important information about a certain segment type.  Only   *
 * used if the route_type is DETAILED.  [0 .. det_routing_arch.num_segment]  *
 * frequency:  Fraction of tracks which are of this segment type.            *
 * length:     Length (in clbs) of the segment.                              *
 * wire_switch:  Index of the switch type that connects other wires *to*     *
 *               this segment.                                               *
 * opin_switch:  Index of the switch type that connects output pins (OPINs)  *
 *               *to* this segment.                                          *
 * frac_cb:  The fraction of logic blocks along its length to which this     *
 *           segment can connect.  (i.e. internal population).               *
 * frac_sb:  The fraction of the length + 1 switch blocks along the segment  *
 *           to which the segment can connect.  Segments that aren't long    *
 *           lines must connect to at least two switch boxes.                *
 * Cmetal: Capacitance of a routing track, per unit logic block length.      *
 * Rmetal: Resistance of a routing track, per unit logic block length.       */


struct s_switch_inf {boolean buffered; float R; float Cin; float Cout; float
          Tdel;};

/* Lists all the important information about a switch type.                  *
 * [0 .. det_routing_arch.num_switch]                                        *
 * buffered:  Does this switch include a buffer?                             *
 * R:  Equivalent resistance of the buffer/switch.                           *
 * Cin:  Input capacitance.                                                  *
 * Cout:  Output capacitance.                                                *
 * Tdel:  Intrinsic delay.  The delay through an unloaded switch is          *
 *        Tdel + R * Cout.                                                   */


typedef struct {int length; int start; boolean longline; boolean *sb; 
          boolean *cb; short wire_switch; short opin_switch; float Rmetal; 
          float Cmetal; int index;} t_seg_details;

/* Lists detailed information about segmentation.  [0 .. W-1].              *
 * length:  length of segment.                                              *
 * start:  index at which a segment starts in channel 0.                    *
 * longline:  TRUE if this segment spans the entire channel.                *
 * sb:  [0..length]:  TRUE for every channel intersection, relative to the  *
 *      segment start, at which there is a switch box.                      *
 * cb:  [0..length-1]:  TRUE for every logic block along the segment at     *
 *      which there is a connection box.                                    *
 * wire_switch:  Index of the switch type that connects other wires *to*    *
 *               this segment.                                              *
 * opin_switch:  Index of the switch type that connects output pins (OPINs) *
 *               *to* this segment.                                         *
 * Cmetal: Capacitance of a routing track, per unit logic block length.     *
 * Rmetal: Resistance of a routing track, per unit logic block length.      *
 * index: index of the segment type used for this track.                    */


typedef struct {float T_comb; float T_seq_in; float T_seq_out;} t_T_subblock;

/* Gives the delays through a subblock.                                    *
 * T_comb: The delay from input to output when the subblock is used in     *
 *         combinational mode (clock input is open).                       *
 * T_seq_in: The delay from subblock input to storage element when the     *
 *           subblock is in sequential mode (clock input not open).  The   *
 *           delay includes any combinational logic delay (e.g. a LUT)     *
 *           done before the signal is latched, plus the setup time.       *
 * T_seq_out: The delay from storage element to subblock output when the   *
 *            subblock is in sequential mode.  Includes clock_to_Q plus    *
 *            any combinational path (muxes, etc.) on the output.          */


typedef struct {boolean timing_analysis_enabled; float C_ipin_cblock; 
          float T_ipin_cblock; float T_sblk_opin_to_sblk_ipin; float 
          T_clb_ipin_to_sblk_ipin; float T_sblk_opin_to_clb_opin; 
          float T_ipad; float T_opad; t_T_subblock *T_subblock;} t_timing_inf;

/* C_ipin_cblock: Capacitance added to a routing track by the isolation     *
 *                buffer between a track and the Cblocks at an (i,j) loc.   *
 * T_ipin_cblock: Delay through an input pin connection box (from a         *
 *                   routing track to a logic block input pin).             *
 * T_sblk_opin_to_sblk_ipin: Delay through the local interconnect           *
 *       (muxes, wires or whatever) in a clb containing multiple subblocks. *
 *       That is, the delay from a subblock output to the input of another  *
 *       subblock in the same clb.                                          *
 * T_clb_ipin_to_sblk_ipin: Delay from a clb input pin to any subblock      *
 *                   input pin (e.g. the mux delay in an Altera 8K clb).    *
 * T_sblk_opin_to_clb_opin: Delay from a subblock output to a clb output.   *
 *                   Will be 0 in many architectures.                       *
 * T_ipad:  Delay through an input pad.                                     *
 * T_opad:  Delay through an output pad.                                    *
 * *T_subblock: Array giving the delay through each subblock.               *
 *              [0..max_subblocks_per_block - 1]                            */


struct s_linked_f_pointer {struct s_linked_f_pointer *next; float *fptr;};

/* A linked list of float pointers.  Used for keeping track of   *
 * which pathcosts in the router have been changed.              */


/* Uncomment lines below to save some memory, at the cost of debugging ease. */
/*enum e_rr_type {SOURCE, SINK, IPIN, OPIN, CHANX, CHANY}; */
/* typedef short t_rr_type */

typedef enum {SOURCE, SINK, IPIN, OPIN, CHANX, CHANY} t_rr_type;

/* Type of a routing resource node.  x-directed channel segment,   *
 * y-directed channel segment, input pin to a clb to pad, output   *
 * from a clb or pad (i.e. output pin of a net) and:               *
 * SOURCE:  A dummy node that is a logical output within a block   *
 *          -- i.e., the gate that generates a signal.             *
 * SINK:    A dummy node that is a logical input within a block    *
 *          -- i.e. the gate that needs a signal.                  */


struct s_trace {int index; short iswitch; struct s_trace *next;};

/* Basic element used to store the traceback (routing) of each net.        *
 * index:   Array index (ID) of this routing resource node.                *
 * iswitch:  Index of the switch type used to go from this rr_node to      *
 *           the next one in the routing.  OPEN if there is no next node   *
 *           (i.e. this node is the last one (a SINK) in a branch of the   *
 *           net's routing).                                               *
 * next:    pointer to the next traceback element in this route.           */


#define NO_PREVIOUS -1

typedef struct {short xlow; short xhigh; short ylow; short yhigh; 
     short ptc_num; short cost_index; short occ; short capacity; 
     short num_edges; t_rr_type type; int *edges; short *switches; float R;
     float C; } t_rr_node;

/* Main structure describing one routing resource node.  Everything in       *
 * this structure should describe the graph -- information needed only       *
 * to store algorithm-specific data should be stored in one of the           *
 * parallel rr_node_?? structures.                                           *
 *                                                                           *
 * xlow, xhigh, ylow, yhigh:  Integer coordinates (see route.c for           *
 *       coordinate system) of the ends of this routing resource.            *
 *       xlow = xhigh and ylow = yhigh for pins or for segments of           *
 *       length 1.  These values are used to decide whether or not this      *
 *       node should be added to the expansion heap, based on things         *
 *       like whether it's outside the net bounding box or is moving         *
 *       further away from the target, etc.                                  *
 * type:  What is this routing resource?                                     *
 * ptc_num:  Pin, track or class number, depending on rr_node type.          *
 *           Needed to properly draw.                                        *
 * cost_index: An integer index into the table of routing resource indexed   *
 *             data (this indirection allows quick dynamic changes of rr     *
 *             base costs, and some memory storage savings for fields that   *
 *             have only a few distinct values).                             *
 * occ:        Current occupancy (usage) of this node.                       *
 * capacity:   Capacity of this node (number of routes that can use it).     *
 * num_edges:  Number of edges exiting this node.  That is, the number       *
 *             of nodes to which it connects.                                *
 * edges[0..num_edges-1]:  Array of indices of the neighbours of this        *
 *                         node.                                             *
 * switches[0..num_edges-1]:  Array of switch indexes for each of the        *
 *                            edges leaving this node.                       *
 *                                                                           *
 * The following parameters are only needed for timing analysis.             *
 * R:  Resistance to go through this node.  This is only metal               *
 *     resistance (end to end, so conservative) -- it doesn't include the    *
 *     switch that leads to another rr_node.                                 *
 * C:  Total capacitance of this node.  Includes metal capacitance, the      *
 *     input capacitance of all switches hanging off the node, the           *
 *     output capacitance of all switches to the node, and the connection    *
 *     box buffer capacitances hanging off it.                               */


typedef struct {float base_cost; float saved_base_cost; int ortho_cost_index; 
                int seg_index; float inv_length; float T_linear; 
                float T_quadratic; float C_load; } t_rr_indexed_data;

/* Data that is pointed to by the .cost_index member of t_rr_node.  It's     *
 * purpose is to store the base_cost so that it can be quickly changed       *
 * and to store fields that have only a few different values (like           *
 * seg_index) or whose values should be an average over all rr_nodes of a    *
 * certain type (like T_linear etc., which are used to predict remaining     *
 * delay in the timing_driven router).                                       *
 *                                                                           *
 * base_cost:  The basic cost of using an rr_node.                           *
 * ortho_cost_index:  The index of the type of rr_node that generally        *
 *                    connects to this type of rr_node, but runs in the      *
 *                    orthogonal direction (e.g. vertical if the direction   *
 *                    of this member is horizontal).                         *
 * seg_index:  Index into segment_inf of this segment type if this type of   *
 *             rr_node is an CHANX or CHANY; OPEN (-1) otherwise.            *
 * inv_length:  1/length of this type of segment.                            *
 * T_linear:  Delay through N segments of this type is N * T_linear + N^2 *  *
 *            T_quadratic.  For buffered segments all delay is T_linear.     *
 * T_quadratic:  Dominant delay for unbuffered segments, 0 for buffered      *
 *               segments.                                                   *
 * C_load:  Load capacitance seen by the driver for each segment added to    *
 *          the chain driven by the driver.  0 for buffered segments.        */


enum e_cost_indices {SOURCE_COST_INDEX = 0, SINK_COST_INDEX, OPIN_COST_INDEX,
      IPIN_COST_INDEX, CHANX_COST_INDEX_START};

/* Gives the index of the SOURCE, SINK, OPIN, IPIN, etc. member of           *
 * rr_indexed_data.                                                          */
