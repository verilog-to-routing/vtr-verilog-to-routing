#define MAXLUT 7        /* Maximum number of inputs per LUT */
#define HASHSIZE 4095
#define NAMELENGTH 16   /* Length of the name stored for each net */ 
#define DEBUG 1         /* Echoes input & checks error conditions */
/*#define VERBOSE 1*/   /* Prints all sorts of intermediate data */

#define NO_CLUSTER -1
#define NEVER_CLUSTER -2
#define NOT_VALID -10000  /* Marks gains that aren't valid */
                          /* Ensure no gain can ever be this negative! */
#define UNDEFINED -1    

#define DRIVER 0     /* Is a pin driving a net or in the fanout? */
#define RECEIVER 1
#define OPEN -1      /* Pin is unconnected. */

#define TRUE 1 		/* PAJ - Booleans I like to use */
#define FALSE 0

enum block_types {
	INPAD = -2, 
	OUTPAD, 
	LUT, 
	LATCH, 
	EMPTY, 
	LUT_AND_LATCH,
	SUBCKT};

enum e_cluster_seed {
	TIMING, 
	MAX_INPUTS};

struct hash_nets {
	char *name; 
	int index; 
	int count; 
	struct hash_nets *next;
}; 
/* count is the number of pins on this net so far. */

struct s_net {
	char *name; 
	int num_pins; 
	int *pins;
};
/* name:  ASCII net name for informative annotations in the output.  *
 * num_pins:  Number of pins on this net.                            *
 * pins[]: Array containing the blocks to which the pins of this net *
 *         connect.  Output in pins[0], inputs in other entries.     */

struct s_block {
	char *name; 
	enum block_types type; 
	int num_nets;
	int nets[MAXLUT+2];
}; 
/* name:  Taken from the net which it drives.                        *
 * type:  LUT, INPAD, OUTPAD or LATCH.                               *
 * num_nets:  number of nets connected to this block.                *
 * nets[]:  List of nets connected to this block.  Net[0] is the     *
 *          output, others are inputs, except for OUTPAD.  OUTPADs   *
 *          only have an input, so this input is in net[0].          */

struct s_subckt {
	char *name; 
	int num_in_blocks;
	struct s_block **in_blocks;
	int num_out_blocks;
	struct s_block **out_blocks;
	int num_inputs;
	int *input_signal_to_block;
	int *input_index_to_block;
	int num_outputs;
	int *output_signal_to_block;
	int *output_index_to_block;
}; 
/* name: What we actually call the hetrogeneouts structure afterwards
 * num_nets: total number of connections
 * num_outputs: the number of outputs ... all the first pins in net 0..num_outputs
 * 				num_inputs = num_outputs - num_nets
 * nets: dynamic list of nets */
