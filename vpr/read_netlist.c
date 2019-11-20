#include <string.h>
#include <stdio.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "read_netlist.h"
#include "hash.h"
#include "check_netlist.h"
#include <assert.h>


/* This source file reads in a .net file.  A .net file is a netlist   *
 * format defined by me to allow arbitary logic blocks (clbs) to      *
 * make up a netlist.  There are three legal block types: .input,     *
 * .output, and .clb.  To define a block, you start a line with one   *
 * of these keywords; the function of each type of block is obvious.  *
 * After the block type keyword, you specify the name of this block.  *
 * The line below must start with the keyword pinlist:, and gives a   *
 * list of the nets connected to the pins of this block.  .input and  *
 * .output blocks are pads and have only one net connected to them.   *
 * The number of pins on a .clb block is specified in the arch-       *
 * itecture file, as are the equivalences between pins.  Each         *
 * clb must have the same number of pins (heterogeneous FPGAs are     *
 * currently not supported) so any pins which are not used on a clb   *
 * must be identified with the reserved word "open".  All keywords    *
 * must be lower case.                                                *
 *                                                                    *
 * The lines immediately below the pinlist line must specify the      *
 * contents of the clb.  Each .subblock line lists the name of the    *
 * subblock, followed by the clb pin number or subblock output to     *
 * which each subblock pin should connect.  Each subblock is assummed *
 * to consist of a LUT with subblock_lut_size inputs, a FF, and an    *
 * output.  The pin order is input1, input2, ... output, clock.  The  *
 * architecture file sets the number of subblocks per clb and the LUT *
 * size used.  Subblocks are used only for timing analysis.  An       *
 * example clb declaration is:                                        *
 *                                                                    *
 * .clb name_of_clb  # comment                                        *
 *  pinlist:  net_1 net_2 my_net net_of_mine open D open              *
 *  subblock: sub_1 0 1 2 3 open open open                            *
 *  subblock: sub_2 1 3 ble_0 open open 5 open                        *
 *                                                                    *
 * Notice that the output of the first subblock (ble_0) is used only  *
 * by the second subblock.  This is fine.                             *
 *                                                                    *
 * Ending a line with a backslash (\) means it is continued on the    *
 * line below.  A sharp sign (#) indicates the rest of a line is      *
 * a comment.                                                         *
 * The vpack program can be used to convert a flat blif netlist       *
 * into .net format.                                                  *
 *                                                                    *
 * V. Betz, Jan. 29, 1997.                                            */

/* A note about the way the character buffer, buf, is passed around. *
 * strtok does not make a local copy of the character string         *
 * initially passed to it, so you must make sure it always exists.   *
 * Hence, I just use one buffer declared in read_net and pass it     *
 * downstream.  Starting a new line with an automatic variable       *
 * buffer in a downstream subroutine and then returning and trying   *
 * to keep tokenizing it would cause problems since the buffer now   *
 * lies on a stale part of the stack and can be overwritten.         */


/*************************** Variables local to this module *****************/

/* Temporary storage used during parsing. */

static int *num_driver, *temp_num_pins;
static struct s_hash **hash_table;
static int temp_block_storage;

/* Used for memory chunking of everything except subblock data. */

static int chunk_bytes_avail = 0;
static char *chunk_next_avail_mem = NULL;


/* Subblock data can be accessed anywhere within this module.  Pointers to *
 * the main subblock data structures are passed back to the rest of the    *
 * program through the subblock_data_ptr structure passed to read_net.     */

static int max_subblocks_per_block;
static int subblock_lut_size;
static t_subblock **subblock_inf;
static int *num_subblocks_per_block;

/* The subblock data is put in its own "chunk" so it can be freed without  *
 * hosing the other netlist data.                                          */

static int ch_subblock_bytes_avail;
static char *ch_subblock_next_avail_mem;
static struct s_linked_vptr *ch_subblock_head_ptr;



/************************ Subroutines local to this module ******************/

static int add_net (char *ptr, enum e_pin_type type, int bnum, int blk_pnum, 
       int doall); 

static char *get_tok(char *buf, int doall, FILE *fp_net);
static void add_io (int doall, int type, FILE *fp_net, char *buf);
static char *add_clb (int doall, FILE *fp_net, char *buf);
static void add_global (int doall, FILE *fp_net, char *buf);
static void init_parse(int doall);
static void free_parse (void);
static void parse_name_and_pinlist (int doall, FILE *fp_net, char *buf);
static int get_pin_number (char *ptr); 

static void load_subblock_array (int doall, FILE *fp_net, char *temp_buf, 
     int num_subblocks, int bnum); 

static void set_subblock_count (int bnum, int num_subblocks); 
static char *parse_subblocks (int doall, FILE *fp_net, char *buf, int bnum); 



/********************** Subroutine definitions *******************************/



void read_net (char *net_file, t_subblock_data *subblock_data_ptr) {

/* Main routine that parses a netlist file in my (.net) format. */

 char buf[BUFSIZE], *ptr;
 int doall;
 FILE *fp_net;

/* Make two variables below accessible anywhere in the module because I'm *
 * too lazy to pass them all over the place.                              */

 max_subblocks_per_block = subblock_data_ptr->max_subblocks_per_block;
 subblock_lut_size = subblock_data_ptr->subblock_lut_size;

 fp_net = my_fopen (net_file, "r", 0);

/* First pass builds the symbol table and counts the number of pins  *
 * on each net.  Then I allocate exactly the right amount of storage *
 * for each net.  Finally, the second pass loads the block and net   *
 * arrays.                                                           */

 for (doall=0;doall<=1;doall++) {  /* Pass number. */
    init_parse(doall);

    linenum = 0;   /* Reset line number. */
    ptr = my_fgets (buf, BUFSIZE, fp_net);

    while (ptr != NULL) {
       ptr = get_tok (buf, doall, fp_net);
    }
    rewind (fp_net);  /* Start at beginning of file again */
 } 

 fclose(fp_net);

/* Return the three data structures below through subblock_data_ptr.        */

 subblock_data_ptr->subblock_inf = subblock_inf;
 subblock_data_ptr->num_subblocks_per_block = num_subblocks_per_block;
 subblock_data_ptr->chunk_head_ptr = ch_subblock_head_ptr;

 check_netlist (subblock_data_ptr, num_driver);
 free_parse();
}


static void init_parse(int doall) {

/* Allocates and initializes the data structures needed for the parse. */

 int i, j, len, nindex, pin_count;
 int *tmp_ptr;
 struct s_hash_iterator hash_iterator;
 struct s_hash *h_ptr;


 if (!doall) {  /* Initialization before first (counting) pass */
    num_nets = 0;  
    hash_table = alloc_hash_table ();

#define INITIAL_BLOCK_STORAGE 2000
    temp_block_storage = INITIAL_BLOCK_STORAGE;
    num_subblocks_per_block = my_malloc (INITIAL_BLOCK_STORAGE *
             sizeof(int));

    ch_subblock_bytes_avail = 0;
    ch_subblock_next_avail_mem = NULL;
    ch_subblock_head_ptr = NULL;
 }

/* Allocate memory for second (load) pass */ 

 else {   
    net = (struct s_net *) my_malloc (num_nets*sizeof(struct s_net));
    block = (struct s_block *) my_malloc (num_blocks*
        sizeof(struct s_block));   
    is_global = (boolean *) my_calloc (num_nets, sizeof(boolean));
    num_driver = (int *) my_malloc (num_nets * sizeof(int));
    temp_num_pins = (int *) my_malloc (num_nets * sizeof(int));

    for (i=0;i<num_nets;i++) {
       num_driver[i] = 0;
       net[i].num_pins = 0;
    }

/* Allocate block pin connection storage.  Some is wasted for io blocks. *
 * Method used below "chunks" the malloc of a bunch of small things to   *
 * reduce the memory housekeeping overhead of malloc.                    */

    tmp_ptr = (int *) my_malloc (pins_per_clb * num_blocks * sizeof(int));
    for (i=0;i<num_blocks;i++) 
       block[i].nets = tmp_ptr + i * pins_per_clb;

/* I use my_chunk_malloc for some storage locations below.  my_chunk_malloc  *
 * avoids the 12 byte or so overhead incurred by malloc, but since I call it *
 * with a NULL head_ptr, it will not keep around enough information to ever  *
 * free these data arrays.  If you ever have compatibility problems on a     *
 * non-SPARC architecture, just change all the my_chunk_malloc calls to      *
 * my_malloc calls.                                                          */

    hash_iterator = start_hash_table_iterator ();
    h_ptr = get_next_hash (hash_table, &hash_iterator);
    
    while (h_ptr != NULL) {
       nindex = h_ptr->index;
       pin_count = h_ptr->count;
       net[nindex].blocks = (int *) my_chunk_malloc(pin_count *
               sizeof(int), NULL, &chunk_bytes_avail, &chunk_next_avail_mem);

       net[nindex].blk_pin = (int *) my_chunk_malloc (pin_count * 
               sizeof(int), NULL, &chunk_bytes_avail, &chunk_next_avail_mem);

/* For avoiding assigning values beyond end of pins array. */

       temp_num_pins[nindex] = pin_count;

       len = strlen (h_ptr->name);
       net[nindex].name = (char *) my_chunk_malloc ((len + 1) *
            sizeof(char), NULL, &chunk_bytes_avail, &chunk_next_avail_mem);
       strcpy (net[nindex].name, h_ptr->name);
       h_ptr = get_next_hash (hash_table, &hash_iterator);
    }

/* Allocate storage for subblock info. (what's in each logic block) */

   num_subblocks_per_block = (int *) my_realloc (num_subblocks_per_block,
                  num_blocks * sizeof (int));
   subblock_inf = (t_subblock **) my_malloc (num_blocks * 
                        sizeof(t_subblock *));

   for (i=0;i<num_blocks;i++) {
      if (num_subblocks_per_block[i] == 0) 
         subblock_inf[i] = NULL;
      else {
         subblock_inf[i] = (t_subblock *) my_chunk_malloc (
              num_subblocks_per_block[i] * sizeof (t_subblock), 
              &ch_subblock_head_ptr, &ch_subblock_bytes_avail, 
              &ch_subblock_next_avail_mem);
         for (j=0;j<num_subblocks_per_block[i];j++) 
            subblock_inf[i][j].inputs = (int *) my_chunk_malloc
                 (subblock_lut_size * sizeof(int), &ch_subblock_head_ptr, 
                 &ch_subblock_bytes_avail, &ch_subblock_next_avail_mem);
      }
   }
 }

/* Initializations for both passes. */

 linenum = 0;
 num_p_inputs = 0;
 num_p_outputs = 0;
 num_clbs = 0;
 num_blocks = 0;
 num_globals = 0;
}


static char *get_tok (char *buf, int doall, FILE *fp_net) {

/* Figures out which, if any token is at the start of this line and *
 * takes the appropriate action.  It always returns a pointer to    *
 * the next line (I need to do this so I can do some lookahead).    */

 char *ptr; 
 
 ptr = my_strtok(buf,TOKENS,fp_net,buf);

 if (ptr == NULL) {                       /* Empty line.  Skip. */
    ptr = my_fgets(buf, BUFSIZE, fp_net);
    return (ptr);
 }
 
 if (strcmp(ptr,".clb") == 0) {
    ptr = add_clb (doall, fp_net, buf);
    return (ptr);
 }

 if (strcmp(ptr,".input") == 0) {
    add_io (doall, INPAD, fp_net, buf);
    ptr = my_fgets(buf, BUFSIZE, fp_net);
    return (ptr);
 }

 if (strcmp(ptr,".output") == 0) {
    add_io (doall, OUTPAD, fp_net, buf);
    ptr = my_fgets(buf, BUFSIZE, fp_net);
    return (ptr);
 }

 if (strcmp(ptr,".global") == 0) {
    add_global (doall, fp_net, buf);
    ptr = my_fgets(buf, BUFSIZE, fp_net);
    return (ptr);
 }

 printf ("Error in get_tok while parsing netlist file.\n");
 printf ("Line %d starts with an invalid token (%s).\n", linenum, ptr);
 exit (1);
}


static int get_pin_number (char *ptr) {

/* Returns the pin number to which a subblock pin connects.  This pin number *
 * can be OPEN, one of the clb pins (from 0 to clb_size-1) or a "hidden"     *
 * pin that refers to the output of one of the subblocks within the clb      *
 * (the output of subblock 0 is pin clb_size, the output of subblock         *
 * max_subblocks_per_block is clb_size + max_subblocks_per_block-1).         */

 int val;

 if (strcmp("open",ptr) == 0) 
    return (OPEN);

 if (strncmp ("ble_", ptr, 4) == 0) {   /* "Hidden" pin (Subblock output) */
    val = my_atoi (ptr + 4);
    if (val < 0 || val >= max_subblocks_per_block) {
       printf("Error in get_pin_number on line %d of netlist file.\n", 
            linenum);
       printf("Pin ble_%d is out of legal range (ble_%d to ble_%d).\n"
            "Aborting.\n\n", val, 0, max_subblocks_per_block - 1);
       exit (1);
    }
    val += pins_per_clb;  /* pins_per_clb .. pins_per_clb + max_subblocks-1 */
    return (val);
 }

 /* Clb input pin. */

 val = my_atoi (ptr);

 if (val < 0 || val >= pins_per_clb) {
    printf("Error in get_pin_number on line %d of netlist file.\n", 
         linenum);
    printf("Pin %d is out of legal range (%d to %d).\nAborting.\n\n",
         val, 0, pins_per_clb - 1);
    exit (1);
 }

 return (val);
}


static void load_subblock_array (int doall, FILE *fp_net,
           char *temp_buf, int num_subblocks, int bnum) {

/* Parses one subblock line and, if doall is 1, loads the proper   *
 * arrays.  Each subblock line is of the format:                   *
 * subblock: <name> <ipin0> <ipin1> .. <ipin[subblock_lut_size-1]> *
 *          <opin> <clockpin>                                      */
 
 int ipin, len, connect_to;
 char *ptr;

 ipin = 0;
 ptr = my_strtok(NULL,TOKENS,fp_net,temp_buf);

 if (ptr == NULL) {
    printf("Error in load_subblock_array on line %d of netlist file.\n",
             linenum);
    printf("Subblock name is missing.\nAborting.\n\n");
    exit (1);
 }
    
/* Load subblock name if this is the load pass. */
 if (doall == 1) {
    len = strlen (ptr);
    subblock_inf[bnum][num_subblocks-1].name = my_chunk_malloc ((len+1) *
             sizeof(char), &ch_subblock_head_ptr, &ch_subblock_bytes_avail,
             &ch_subblock_next_avail_mem);
    strcpy (subblock_inf[bnum][num_subblocks-1].name, ptr);
 }

 ptr = my_strtok(NULL,TOKENS,fp_net,temp_buf);

 while (ptr != NULL) {    /* For each subblock pin. */
    if (doall == 1) {
       connect_to = get_pin_number (ptr);
       if (ipin < subblock_lut_size) {      /* LUT input. */
          subblock_inf[bnum][num_subblocks-1].inputs[ipin] = connect_to;
       }
       else if (ipin == subblock_lut_size) {   /* LUT output. */
          subblock_inf[bnum][num_subblocks-1].output = connect_to;
       }
       else if (ipin == subblock_lut_size+1) {   /* Clock input. */
          subblock_inf[bnum][num_subblocks-1].clock = connect_to;
       }
    }
    ipin++;
    ptr = my_strtok(NULL,TOKENS,fp_net,temp_buf);
 }

 if (ipin != subblock_lut_size + 2) {
    printf("Error in load_subblock_array at line %d of netlist file.\n",
            linenum); 
    printf("Subblock had %d pins, expected %d.\n", ipin, 
            subblock_lut_size+2);
    printf("Aborting.\n\n");
    exit (1);
 }
}


static void set_subblock_count (int bnum, int num_subblocks) {

/* Sets the temporary subblock count for block bnum to num_subblocks. *
 * Properly allocates whatever temporary storage is needed.           */
 
 if (bnum >= temp_block_storage) {
    temp_block_storage *= 2;
    num_subblocks_per_block = (int *) my_realloc 
            (num_subblocks_per_block, temp_block_storage * sizeof (int));
 }
 
 num_subblocks_per_block[bnum] = num_subblocks;
}


static char *parse_subblocks (int doall, FILE *fp_net, char *buf,
               int bnum) {

/* Loads the subblock arrays with the proper values. */

 char temp_buf[BUFSIZE], *ptr;
 int num_subblocks;

 num_subblocks = 0;

 while (1) {
    ptr = my_fgets (temp_buf, BUFSIZE, fp_net);
    if (ptr == NULL)   
       break;          /* EOF */

  /* Save line in case it's not a sublock */
    strcpy (buf, temp_buf);

    ptr = my_strtok(temp_buf,TOKENS,fp_net,temp_buf);
    if (ptr == NULL) 
       continue;       /* Blank or comment line.  Skip. */

    if (strcmp("subblock:", ptr) == 0) {
       num_subblocks++; 
       load_subblock_array (doall, fp_net, temp_buf, num_subblocks,
               bnum);
    }
    else {
       break;  /* Subblock list has ended.  Buf contains next line. */
    }
 }   /* End infinite while */

 if (num_subblocks < 1 || num_subblocks > max_subblocks_per_block) {
    printf("Error in parse_subblocks on line %d of netlist file.\n",
         linenum);
    printf("Block #%d has %d subblocks.  Out of range.\n",
         bnum, num_subblocks);
    printf("Aborting.\n\n");
    exit (1);
 }

 if (doall == 0)
    set_subblock_count (bnum, num_subblocks);
 else 
    assert (num_subblocks == num_subblocks_per_block[bnum]);

 return (ptr);
}


static char *add_clb (int doall, FILE *fp_net, char *buf) {

/* Adds the clb (.clb) currently being parsed to the block array.  Adds *
 * its pins to the nets data structure by calling add_net.  If doall is *
 * zero this is a counting pass; if it is 1 this is the final (loading) *
 * pass.                                                                */

 char *ptr;
 int pin_index, iclass, inet;
 enum e_pin_type type;

 num_blocks++;
 parse_name_and_pinlist (doall, fp_net, buf);
 num_clbs++;

 if (doall) 
    block[num_blocks - 1].type = CLB;

 pin_index = -1;
 ptr = my_strtok(NULL,TOKENS,fp_net,buf);

 while (ptr != NULL) {
    pin_index++;
    if (pin_index >= pins_per_clb) {
       printf("Error in add_clb on line %d of netlist file.\n",linenum);
       printf("Too many pins on this clb.  Expected %d.\n",pins_per_clb);
       exit (1);
    }
    
    iclass = clb_pin_class[pin_index];
    type = class_inf[iclass].type;      /* DRIVER or RECEIVER */

    if (strcmp(ptr,"open") != 0) {     /* Pin is connected. */
       inet = add_net (ptr, type, num_blocks-1, pin_index, doall);

       if (doall)                      /* Loading pass only */
          block[num_blocks - 1].nets[pin_index] = inet; 
    }
    else {                             /* Pin is unconnected (open) */
       if (doall) 
          block[num_blocks - 1].nets[pin_index] = OPEN;
    }

    ptr = my_strtok(NULL,TOKENS,fp_net,buf);
 }

 if (pin_index != pins_per_clb - 1) {
    printf("Error in add_clb on line %d of netlist file.\n",linenum);
    printf("Expected %d pins on clb, got %d.\n", pins_per_clb, pin_index + 1);
    exit (1);
 }

 ptr = parse_subblocks (doall, fp_net, buf, num_blocks-1);
 return (ptr);
}


static void add_io (int doall, int block_type, FILE *fp_net, char *buf) {

/* Adds the INPAD or OUTPAD (specified by block_type)  currently being  *
 * parsed to the block array.  Adds its pin to the nets data structure  *
 * by calling add_net.  If doall is zero this is a counting pass; if it *
 * is 1 this is the final (loading) pass.                               */

 char *ptr;
 int inet, pin_index, i;
 enum e_pin_type type;
 
 num_blocks++;
 if (doall == 0)
    set_subblock_count (num_blocks-1, 0);    /* No subblocks for IO */
 parse_name_and_pinlist (doall, fp_net, buf); 

 if (block_type == INPAD) {
    num_p_inputs++;
    type = DRIVER;
 }
 else {
    num_p_outputs++;
    type = RECEIVER;
 }
 
 if (doall)
    block[num_blocks - 1].type = block_type;
 
 pin_index = -1;
 ptr = my_strtok(NULL,TOKENS,fp_net,buf);
 
 while (ptr != NULL) {
    pin_index++;
    if (pin_index >= 1) {
       printf("Error in add_io on line %d of netlist file.\n",linenum);
       printf("Too many pins on this io.  Expected 1.\n");
       exit (1);
    }
 
    if (strcmp(ptr,"open") == 0) {     /* Pin unconnected. */
       printf("Error in add_io, line %d of netlist file.\n",linenum);
       printf("Inputs and Outputs cannot have open pins.\n");
       exit (1);
    }
   
/* Note the dummy pin number for IO pins.  Change this if necessary. I set *
 * them to OPEN because I want the code to crash if I try to look up the   *
 * class of an I/O pin (since I/O pins don't have classes).                */

    inet = add_net (ptr, type, num_blocks-1, OPEN, doall);
 
    if (doall)                      /* Loading pass only */
       block[num_blocks - 1].nets[pin_index] = inet;

    ptr = my_strtok(NULL,TOKENS,fp_net,buf);
 }
 
 if (pin_index != 0) {
    printf("Error in add_io on line %d of netlist file.\n",linenum);
    printf("Expected 1 pin on pad, got %d.\n", pin_index + 1);
    exit (1);
 }
 
 if (doall) {
    for (i=1;i<pins_per_clb;i++) 
       block[num_blocks-1].nets[i] = OPEN;
 }
}


static void parse_name_and_pinlist (int doall, FILE *fp_net, char *buf) {

/* This routine does the first part of the parsing of a block.  It is *
 * called whenever any type of block (.clb, .input or .output) is to  *
 * be parsed.  It increments the block count (num_blocks), and checks *
 * that the block has a name.  If doall is 1, this is the loading     *
 * pass and it copies the name to the block data structure.  Finally  *
 * it checks that the pinlist: keyword exists.  On return, my_strtok  *
 * is set so that the next call will get the first net connected to   *
 * this block.                                                        */

 char *ptr;
 int len;
 
/* Get block name. */
 
 ptr = my_strtok(NULL,TOKENS,fp_net,buf);
 if (ptr == NULL) {
    printf("Error in parse_name_and_pinlist on line %d of netlist file.\n",
       linenum);
    printf(".clb, .input or .output line has no associated name.\n");
    exit (1);
 }
 
 if (doall == 1) {    /* Second (loading) pass, store block name */
    len = strlen (ptr);
    block[num_blocks-1].name = my_chunk_malloc ((len + 1) * sizeof(char),
                  NULL, &chunk_bytes_avail, &chunk_next_avail_mem);
    strcpy (block[num_blocks-1].name, ptr);
 }
 
 ptr = my_strtok (NULL,TOKENS,fp_net,buf);
 if (ptr != NULL) {
    printf("Error in parse_name_and_pinlist on line %d of netlist file.\n",
       linenum);
    printf("Extra characters at end of line.\n");
    exit (1);
 }
 
/* Now get pinlist from the next line.  Note that a NULL return value *
 * from my_gets means EOF, while a NULL return from my_strtok just    *
 * means we had a blank or comment line.                              */
 
 do {
    ptr = my_fgets (buf, BUFSIZE, fp_net);
    if (ptr == NULL) {
       printf("Error in parse_name_and_pinlist on line %d of netlist file.\n",
          linenum);
       printf("Missing pinlist: keyword.\n");
       exit (1);
    }

    ptr = my_strtok(buf,TOKENS,fp_net,buf);
 } while (ptr == NULL);
 
 if (strcmp (ptr, "pinlist:") != 0) {
    printf("Error in parse_name_and_pinlist on line %d of netlist file.\n",
       linenum);
    printf("Expected pinlist: keyword, got %s.\n",ptr);
    exit (1);
 }
}


static void add_global (int doall, FILE *fp_net, char *buf) {

/* Doall is 0 for the first (counting) pass and 1 for the second           *
 * (loading) pass.  fp_net is a pointer to the netlist file.  This         *
 * routine sets the proper entry(ies) in is_global to TRUE during the      *
 * loading pass.  The routine does nothing during the counting pass. If    *
 * is_global = TRUE for a net, it will not be considered in the placement  *
 * cost function, nor will it be routed.  This is useful for global        *
 * signals like clocks that generally have dedicated routing in FPGAs.     */

 char *ptr;
 struct s_hash *h_ptr;
 int nindex;

/* Do nothing if this is the counting pass. */

 if (doall == 0) 
    return; 

 ptr = my_strtok(NULL,TOKENS,fp_net,buf);

 while (ptr != NULL) {     /* For each .global signal */
    num_globals++;

    h_ptr = get_hash_entry (hash_table, ptr);

    if (h_ptr == NULL) {        /* Net was not found in list! */
       printf("Error in add_global on netlist file line %d.\n",linenum);
       printf("Global signal %s does not exist.\n",ptr);
       exit(1);
    }
   
    nindex = h_ptr->index;
    is_global[nindex] = TRUE;    /* Flagged as global net */

    ptr = my_strtok(NULL,TOKENS,fp_net,buf);
 }
}


static int add_net (char *ptr, enum e_pin_type type, int bnum, int blk_pnum, 
          int doall) {   

/* This routine is given a net name in *ptr, either DRIVER or RECEIVER *
 * specifying whether the block number given by bnum is driving this   *
 * net or in the fan-out and doall, which is 0 for the counting pass   *
 * and 1 for the loading pass.  It updates the net data structure and  *
 * returns the net number so the calling routine can update the block  *
 * data structure.                                                     */

 struct s_hash *h_ptr;
 int j, nindex;

 if (doall == 0) {             /* Counting pass only */
    h_ptr = insert_in_hash_table (hash_table, ptr, num_nets);
    nindex = h_ptr->index;

    if (nindex == num_nets)    /* Net was not in the hash table */
       num_nets++;

    return (nindex);
 }

 else {                        /* Load pass */
    h_ptr = get_hash_entry (hash_table, ptr);
    nindex = h_ptr->index;

    if (h_ptr == NULL) {
       printf("Error in add_net:  the second (load) pass found could not\n"); 
       printf("find net %s in the symbol table.\n", ptr);
       exit(1);
    }

    net[nindex].num_pins++;
    if (type == DRIVER) {
       num_driver[nindex]++;
       j=0;           /* Driver always in position 0 of pinlist */
    }    

    else {
       j = net[nindex].num_pins - num_driver[nindex];
   /* num_driver is the number of signal drivers of this net. *
    * should always be zero or 1 unless the netlist is bad.   */

       if (j >= temp_num_pins[nindex]) {
          printf("Error:  Net #%d (%s) has no driver and will cause\n",
             nindex, ptr);
          printf("memory corruption.\n");
          exit(1);
       } 
    }   
    net[nindex].blocks[j] = bnum;
    net[nindex].blk_pin[j] = blk_pnum;
    return (nindex);
 }
}


static void free_parse (void) {  

/* Release memory needed only during circuit netlist parsing. */

 free (num_driver);
 free_hash_table (hash_table);
 free (temp_num_pins);
}
