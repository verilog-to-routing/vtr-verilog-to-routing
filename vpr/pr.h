#define DEBUG 1     /* Echoes input & checks error conditions */
                    /* Only causes about a 1% speed degradation in V 3.10 */
#define TOKENS " \t\n"   /* Input file parsing. */
/*#define VERBOSE 1*/      /* Prints all sorts of intermediate data */
/*#define NDEBUG */        /* Turns off assertion checking for extra speed */

/* Block Types */

enum e_block_types {CLB, OUTPAD, INPAD, IO, ILLEGAL};

enum e_pin_type {OPEN = -1, DRIVER = 0, RECEIVER = 1};
/* Pin is unconnected, driving a net or in the fanout, respectively. */

enum e_side {TOP = 0, BOTTOM = 1, LEFT = 2, RIGHT = 3};

#define MINOR 0  /* For update_screen.  Denotes importance of update. */
#define MAJOR 1

#define HUGE_FLOAT 1.e30

enum sched_type {AUTO_SCHED, USER_SCHED};   /* Annealing schedule */

enum pic_type {PLACEMENT, ROUTING};  /* What's on screen? */

#define FROM 0      /* What block connected to this net has moved? */
#define TO 1
#define FROM_AND_TO 2

#define SMALL_NET 4    /* Cut off for incremental bounding box updates. */
                       /* 4 is fastest -- I checked.                    */

/* For comp_cost.  NORMAL means use the method that generates updateable  *
 * bounding boxes for speed.  CHECK means compute all bounding boxes from *
 * scratch using a very simple routine to allow checks of the other       *
 * costs.                                                                 */

enum cost_methods {NORMAL, CHECK};

/* For the placer.  Different types of cost functions that can be used. */

enum place_c_types {LINEAR_CONG, NONLINEAR_CONG};

enum ops {PLACE_AND_ROUTE, PLACE_ONLY, ROUTE_ONLY};

enum pfreq {PLACE_NEVER, PLACE_ONCE, PLACE_ALWAYS};

/* Are the pads free to be moved, locked in a random configuration, or *
 * locked in user-specified positions?                                 */

enum e_pad_loc_type {FREE, RANDOM, USER};
                                


struct s_net {char *name; int num_pins; int *pins;
   float ncost; float tempcost;}; 
/* name:  ASCII net name for informative annotations in the output   *
 *        only, so it doesn't matter if they're truncated to         *
 *        NAMELENGTH.                                                *
 * num_pins:  Number of pins on this net.                            *
 * pins[]: Array containing the blocks to which the pins of this net *
 *         connect.  Output in pins[0], inputs in other entries.     *
 * ncost:  Cost of this net.  Used by the placer.                    *
 * tempcost:  Temporary cost of this net.  Used by the placer.       */

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

enum stat {UNIFORM, GAUSSIAN, PULSE, DELTA};
struct s_chan {enum stat type; float peak; float width; float xpeak;
  float dc;};  
/* Width is standard dev. for Gaussian.  xpeak is where peak     *
 * occurs. dc is the dc offset for Gaussian and pulse waveforms. */

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

struct s_subblock {char *name; int output; int clock; int *inputs;};
/* This structure stores the contents of each logic block, in terms *
 * of the basic LUTs that make up the cluster.  This information is *
 * used only for timing analysis.  Note that it is possible to      *
 * describe essentially arbitrary timing patterns inside a logic    *
 * block via the correct pattern of LUTs.                           *
 * name:    Name of this subblock.                                  *
 * output:  Number of the block pin which the LUT output drives.    *
 * clock:   Number of clb pin that drives the clock (or OPEN)       *
 * inputs:  [0..sub_block_lut_size-1].  Number of clb pin that      *
 *          drives this input (or OPEN).                            */

struct s_annealing_sched {enum sched_type type; float inner_num; float init_t;
     float alpha_t; float exit_t;};
/* Annealing schedule information for the placer.  The schedule type      *
 * is either USER_SCHED or AUTO_SCHED.  Inner_num is multiplied by        *
 * num_blocks^4/3 to find the number of moves per temperature.  The       *
 * remaining information is used only for USER_SCHED, and have the        *
 * obvious meanings.                                                      */

struct s_placer_opts {int place_cost_type; float place_cost_exp;
       int place_chan_width; enum e_pad_loc_type pad_loc_type; 
       char *pad_loc_file; enum pfreq place_freq; int num_regions;};
/* Various options for the placer.                                           *
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
 *               computed on an array of num_regions x num_regions basis.    */


enum e_route_type {GLOBAL, DETAILED};

struct s_router_opts {float initial_pres_fac; float pres_fac_mult;
   float acc_fac; float bend_cost; int max_router_iterations;
   int bb_factor; enum e_route_type route_type;};
/* All the parameters controlling the router's operation are in this   *
 * structure.                                                          *
 * initial_pres_fac:  Initial present sharing penalty factor for       *
 *                    Pathfinder.                                      *
 * pres_fac_mult:  Amount by which pres_fac is multiplied each         *
 *                 routing iteration.                                  *
 * acc_fac:  Historical congestion cost multiplier.  Used unchanged    *
 *           for all iterations.                                       *
 * bend_cost:  Cost of a bend when performing global routing.          *
 * max_router_iterations:  Maximum number of iterations before giving  *
 *                up.                                                  *
 * bb_factor:  Linear distance a route can go outside the net bounding *
 *             box.                                                    *
 * route_type:  GLOBAL or DETAILED.                                    */
 
enum e_switch_block_type {SUBSET, WILTON, UNIVERSAL};
enum e_Fc_type {ABSOLUTE, FRACTIONAL};

struct s_det_routing_arch {enum e_Fc_type Fc_type; float Fc_output; 
   float Fc_input; float Fc_pad; enum e_switch_block_type switch_block_type;};
/* Defines the detailed routing architecture of the FPGA.  Only important  *
 * if the route_type is DETAILED.                                          *
 * Fc_type:   Are the Fc values below absolute numbers, or fractions of W? *
 * Fc_output:  Number of tracks to which each clb output pin connect in    *
 *             each channel to which it is adjacent.                       *
 * Fc_input:  Number of tracks to which each clb input pin connects.       *
 * Fc_pad:    Number of tracks to which each I/O pad connects.             *
 * switch_block_type:  Pattern of switches at each switch block.  I        *
 *           assume Fs is always 3.  If the type is SUBSET, I use a        *
 *           Xilinx-like switch block where track i in one channel always  *
 *           connects to track i in other channels.  If type is WILTON,    *
 *           I use a switch block where track i does not always connect    *
 *           to track i in other channels.  See Steve Wilton, Phd Thesis,  *
 *           University of Toronto, 1996.  The UNIVERSAL switch block is   *
 *           from Y. W. Chang et al, TODAES, Jan. 1996, pp. 80 - 101.      */
   

struct s_linked_f_pointer {struct s_linked_f_pointer *next; float *fptr;};
/* A linked list of float pointers.  Used for keeping track of   *
 * which pathcosts in the router have been changed.              */

/* Uncomment lines below to save some memory, at the cost of debugging ease. */
/*enum e_rr_type {SOURCE, SINK, IPIN, OPIN, CHANX, CHANY, EMPTY_PAD}; */
/* typedef short t_rr_type */

typedef enum {SOURCE, SINK, IPIN, OPIN, CHANX, CHANY, EMPTY_PAD} t_rr_type;

/* Type of a routing resource node.  x-directed channel segment,   *
 * y-directed channel segment, input pin to a clb to pad, output   *
 * from a clb or pad (i.e. output pin of a net) and:               *
 * SOURCE:  A dummy node that is a logical output within a block   *
 *          -- i.e., the gate that generates a signal.             *
 * SINK:    A dummy node that is a logical input within a block    *
 *          -- i.e. the gate that needs a signal.                  *
 * EMPTY_PAD:   A pad location that the placer didn't use (i.e.    *
 *              unoccupied.  It has no connections to other nodes. */

struct s_heap {int index; float cost; union {int prev_node; 
       struct s_heap *next;} u;};
/* Used by the heap as its fundamental data structure.           *
 * index:   Index (ID) of this routing resource node.            *
 * cost:    Cost up to and including this node.                  *
 * u.prev_node:  Index (ID) of the predecessor to this node for  * 
 *          use in traceback.  NO_PREVIOUS if none.              *
 * u.next:  pointer to the next s_heap structure in the free     *
 *          linked list.  Not used when on the heap.             */

struct s_trace {int index; struct s_trace *next;};
/* Basic element used to store the traceback (routing) of each net.   *
 * index:   Array index (ID) of this routing resource node.           *
 * next:    pointer to the next traceback element in this route.      */


#define NO_PREVIOUS -1

struct s_rr_node {int prev_node; float cost; float path_cost; 
     short target_flag; short xlow; short xhigh; short ylow; short yhigh; 
     short num_edges; int *edges;};
/* Main structure describing one routing resource node.  Everything in  *
 * this structure should be used in the actual maze expansion or        *
 * other Steiner tree heuristic, since that is the most frequently      *
 * repeated step in routing.  Other node information not needed for     *
 * tree construction should be put in one of the parallel structure     *
 * arrays (conserves memory bandwidth and cache for most CPU intensive  *
 * routing step).                                                       *
 * prev_node:  Index of the previous node used to reach this one;       *
 *             used to generate the traceback.  If there is no          *
 *             predecessor, prev_node = NO_PREVIOUS.                    *
 * cost:       Total cost of using this node.                           *
 * path_cost:  Total cost of the path up to and including this node.    *
 * target_flag:  Is this node a target (sink) for the current routing?  *
 *               TRUE or FALSE (boolean) squeezed into a short.         *
 * xlow, xhigh, ylow, yhigh:  Integer coordinates (see route.c for      *
 *       coordinate system) of the ends of this routing resource.       *
 *       xlow = xhigh and ylow = yhigh for pins or for segments of      *
 *       length 1.  These values are used to decide whether or not this *
 *       node should be added to the expansion heap, based on things    *
 *       like whether it's outside the net bounding box or is moving    *
 *       further away from the target, etc.                             *
 * num_edges:  Number of edges exiting this node.  That is, the number  *
 *             of nodes to which it connects.                           *
 * edges[0..num_edges-1]:  Array of indices of the neighbours of this   *
 *                         node.                                        */

struct s_rr_node_cost_inf {float base_cost; float acc_cost; short occ; 
       short capacity;};
/* Data structure used to compute the cost of each routing resource node. *
 * NB:  s_rr_node[i] and s_rr_node_cost_inf[i] describe the same node.    *
 * base_cost:  The basic cost of using this node.  Allows longer segments *
 *             to have a higher base_cost to reflect their greater area.  *
 * acc_cost:   The accumulated cost term from previous Pathfinder         *
 *             iterations.                                                *
 * occ:        Current occupancy (usage) of this node.                    *
 * capacity:   Capacity of this node (number of routes that can use it).  */

/* Will probably change this enumerated type to short to save some space  *
 * later.                                                                 */

struct s_rr_node_draw_inf {t_rr_type type; short ptc_num;};
/* Some extra information about routing resources that should only be needed *
 * by things like the drawing routines and the sanity checker.               *
 * type:  What is this routing resource?                                     *
 * ptc_num:  Pin, track or class number, depending on rr_node type.  Needed  *
 *           to properly draw.                                               */
