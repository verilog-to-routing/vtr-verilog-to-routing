#define HASHSIZE 4095
#define DEBUG 1     /* Echoes input & checks error conditions */
                    /* Only causes about a 1% speed degradation in V 3.10 */
#define TOKENS " \t\n=,"   /* Input file parsing. */
/*#define VERBOSE 1*/      /* Prints all sorts of intermediate data */

/* Block Types */

#define CLB 0
#define OUTPAD 1
#define INPAD 2     
#define IO 3
#define ILLEGAL 4

#define OPEN -1      /* Pin is unconnected. */
#define DRIVER 0     /* Is a pin driving a net or in the fanout? */
#define RECEIVER 1

#define TOP 0    
#define BOTTOM 1     /* First index of pinloc array. */
#define LEFT 2       
#define RIGHT 3

#define NONE_FLAG 0  /* For marking channels (chan_?.flag) that have */
#define TFLAG 0x1    /* Adjacent target clbs.  Target clb on top.    */
#define BFLAG 0x2    /* Target clb on bottom.                        */
#define LFLAG 0x4    /* Target clb on left.                          */
#define RFLAG 0x8    /* Target clb on right.                         */

#define MINOR 0  /* For update_screen.  Denotes importance of update. */
#define MAJOR 1

#define AUTO_SCHED 0   /* Annealing schedule */
#define USER_SCHED 1

enum pic_type {PLACEMENT, ROUTING};  /* What's on screen? */

#define FROM 0      /* What block connected to this net has moved? */
#define TO 1
#define FROM_AND_TO 2

#define SMALL_NET 4    /* Cut off for incremental bounding box updates. */
                       /* 4 is fastest -- I checked.                    */

/* NONE means no intial route based on expected occupancy will be    *
 * performed.  DIV and SUB specify that an initial routing will be   *
 * performed, using the appropriate cost function.                   */

enum initial_route {NONE, DIV, SUB};

/* For comp_cost.  NORMAL means use the method that generates updateable  *
 * bounding boxes for speed.  CHECK means compute all bounding boxes from *
 * scratch using a very simple routine to allow checks of the other       *
 * costs.                                                                 */

enum cost_methods {NORMAL, CHECK};

/* For the placer.  Different types of cost functions that can be used. */

enum place_c_types {LINEAR_CONG, NONLINEAR_CONG};

enum ops {PLACE_AND_ROUTE, PLACE_ONLY, ROUTE_ONLY};

enum pfreq {PLACE_NEVER, PLACE_ONCE, PLACE_ALWAYS} place_freq;
                                

#define MIXED 0        /* Are pin costs immediately updated or not */
#define BLOCK 1        /* by the router?                           */
#define PATHFINDER 2   /* Or is true pathfinder used?              */

#define PINS_ONLY 0    /* Constants used by update_one_cost to decide */
#define ALL 1          /* which structures it needs to update.        */ 


/* Macros */

#define max(a,b) (((a) > (b))? (a) : (b))
#define min(a,b) ((a) > (b)? (b) : (a))

struct hash_nets {char *name; int index; int count; 
   struct hash_nets *next;}; 
/* count is the number of pins on this net so far. */

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

struct s_block {char *name; int type; int *nets; int x; int y;}; 
/* name:  Taken from the net which it drives.                        *
 * type:  CLB, INPAD or OUTPAD                                       *
 * nets[]:  List of nets connected to this block.  If nets[i] = OPEN *
            no net is connected to pin i.                            *
 * x,y:  physical location of the placed block.                      */ 

struct s_clb {int type; int occ; union { int block; int *io_blocks;} u;};
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

enum htypes {CHANX, CHANY, IPIN, OPIN, LASTONE};
/* CHANX, CHANY, IPIN, and OPIN are used to describe the type    *
 * of an object in either s_heap or s_phys_chan.  LASTONE is     *
 * used only in s_phys_chan.  LASTONE means this element has no  *
 * predessor (it was a preexisting connection and is where the   *
 * new connection for this net should terminate).                */

struct s_phys_chan {int occ; float cost; float acc_cost; 
   float path_cost; int flag; enum htypes prev_type; 
   int prev_i; int prev_j;};
/* occ:  current occupancy of this channel segment               *
 * cost:  current cost to use this channel segment.              *
 * acc_cost:  Used to remember previous demand for this segment. *
 * path_cost :  cost of the path ending at this point.           *
 * flag:  indicates if a target clb is above, below, both, etc.  *
 * in relation to this channel.                                  *
 * prev_type, prev_i, prev_j:  Type and location of the pred-    *
 *      ecessor of this element.  This is used during traceback. *
 *      One could avoid storing this info at the expense of a    *
 *      more complex traceback.                                  */

struct s_heap {enum htypes type; int i; int j; float cost;
   union {int pinnum; struct s_heap *next;} u; enum htypes
   prev_type; int prev_i; int prev_j;};
/* Used by the heap as its fundamental data structure.           *
 * type:  what is this heap or traceback element?                *
 * i,j:   location of this element.                              *
 * pinnum:  index of an input or output pin on a CLB.            *
 *          For IO pads, the pin denotes which                   *
 *          of several pads this traceback element connects to.  *
 *          Not used if the type is a channel.                   *
 * u.cost:  In the heap: cumulative cost to this point.  Not     *
 *          used in the free linked list.                        *
 * u.next:  pointer to the next structure in the free linked     *
 *          list.  Not used when on the heap.                    *
 * prev_type, prev_i, prev_j:  Type and location of the pred-    *
 *          ecessor to this element for use in traceback.        */

struct s_trace {enum htypes type; int i; int j; int pinnum;
   struct s_trace *next;};
/* Basic element used in the traceback for a net.  All elements   *
 * are identical to those in the s_heap structure except next.    *
 * Next points to the next element in the traceback linked list.  */

struct s_linked_f_pointer {struct s_linked_f_pointer *next; 
   float *fptr;};
/* A linked list of float pointers.  Used for keeping track of   *
 * which pathcosts in the router have been changed.              */

struct s_pin {int occ; float cost; float acc_cost;};
/* occ:  Current occupancy of this pin.                            *
 * cost: Cost of using this pin.                                   *
 * acc_cost:  An accumulated cost factor used to remember previous *
 *            high demand for this pin.                            */

struct s_class {int type; int num_pins; int *pinlist;};   
/* type:  DRIVER or RECEIVER (what is this pinclass?)              *
 * num_pins:  The number of logically equivalent pins forming this *
 *           class.                                                *
 * pinlist[]:  List of clb pin numbers which belong to this class. */

struct s_place_region {float capacity; float inv_capacity;
    float occupancy; float cost;};
/* capacity:   Capacity of this region, in tracks.               *
 * occupancy:  Expected number of tracks that will be occupied.  *
 * cost:       Current cost of this usage.                       */
