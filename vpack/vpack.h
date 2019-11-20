#define MAXLUT 7        /* Maximum number of inputs per LUT */
#define HASHSIZE 4095
#define NAMELENGTH 16   /* Length of the name stored for each net */ 
#define DEBUG 1         /* Echoes input & checks error conditions */
/*#define VERBOSE 1*/   /* Prints all sorts of intermediate data */

enum block_types {INPAD = -2, OUTPAD, LUT, LATCH, EMPTY, LUT_AND_LATCH};
enum e_cluster_seed {MAX_SHARING, MAX_INPUTS};

#define DRIVER 0     /* Is a pin driving a net or in the fanout? */
#define RECEIVER 1
#define OPEN -1      /* Pin is unconnected. */

struct hash_nets {char *name; int index; int count; 
   struct hash_nets *next;}; 
/* count is the number of pins on this net so far. */

struct s_net {char *name; int num_pins; int *pins;};
/* name:  ASCII net name for informative annotations in the output.  *
 * num_pins:  Number of pins on this net.                            *
 * pins[]: Array containing the blocks to which the pins of this net *
 *         connect.  Output in pins[0], inputs in other entries.     */

struct s_block {char *name; enum block_types type; int num_nets;
 int nets[MAXLUT+2];};  
/* name:  Taken from the net which it drives.                        *
 * type:  LUT, INPAD, OUTPAD or LATCH.                               *
 * num_nets:  number of nets connected to this block.                *
 * nets[]:  List of nets connected to this block.  Net[0] is the     *
 *          output, others are inputs, except for OUTPAD.  OUTPADs   *
 *          only have an input, so this input is in net[0].          */
