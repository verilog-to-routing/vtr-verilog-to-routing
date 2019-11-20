#include <string.h>
#include <stdio.h>
#include "util.h"
#include "pr.h"
#include "ext.h"
#include "read_netlist.h"
#include "hash.h"
#include <assert.h>


/* This source file reads in a .net file.  A .net file is a netlist  *
 * format defined by me to allow arbitary logic blocks (clbs) to     *
 * make up a netlist.  There are three legal block types: .input,    *
 * .output, and .clb.  To define a block, you start a line with one  *
 * of these keywords; the function of each type of block is obvious. *
 * After the block type keyword, you specify the name of this block. *
 * The line below must start with the keyword pinlist:, and gives a  *
 * list of the nets connected to the pins of this block.  .input and *
 * .output blocks are pads and have only one net connected to them.  *
 * The number of pins on a .clb block is specified in the arch-      *
 * itecture file, as are the equivalences between pins.  Each        *
 * clb must have the same number of pins (heterogeneous FPGAs are    *
 * currently not supported) so any pins which are not used on a clb  *
 * must be identified with the reserved word "open".  All keywords   *
 * must be lower case.                                               *
 *                                                                   *
 * The lines immediately below the pinlist line must specify the     *
 * contents of the clb.  Each .subblock line lists the name of the   *
 * subblock, followed by the clb pin number to which each subblock   *
 * pin should connect.  Each subblock is assummed to consist of a    *
 * LUT with subblock_lut_size inputs, a FF, and an output.  The pin  * 
 * order is input1, input2, ... output, clock.  The architecture     *
 * file sets the number of subblocks per clb and the LUT size used.  *
 * Subblocks are used only for timing analysis.  An example clb      *
 * declaration is:                                                   *
 *                                                                   *
 * .clb name_of_clb  # comment                                       *
 *  pinlist:  net_1 net_2 my_net net_of_mine open D open             *
 *  subblock: sub_1 0 1 2 3 open 5 open                              *
 *                                                                   *
 * Ending a line with a backslash (\) means it is continued on the   *
 * line below.  A sharp sign (#) indicates the rest of a line is     *
 * a comment.                                                        *
 * The vpack program can be used to convert a flat blif netlist      *
 * into .net format.                                                 *
 *                                                                   *
 * V. Betz, Jan. 29, 1997.                                           */

/* A note about the way the character buffer, buf, is passed around. *
 * strtok does not make a local copy of the character string         *
 * initially passed to it, so you must make sure it always exists.   *
 * Hence, I just use one buffer declared in read_net and pass it     *
 * downstream.  Starting a new line with an automatic variable       *
 * buffer in a downstream subroutine and then returning and trying   *
 * to keep tokenizing it would cause problems since the buffer now   *
 * lies on a stale part of the stack and can be overwritten.         */

/* Temporary storage used during parsing. */

static int *num_driver, *temp_num_pins;
static struct s_hash **hash_table;
static int temp_block_storage;

/* Used for memory chunking. */

static int chunk_bytes_avail = 0;
static char *chunk_next_avail_mem = NULL;

static int add_net (char *ptr, enum e_pin_type type, int bnum, int pclass, 
       int doall); 
static char *get_tok(char *buf, int doall, FILE *fp_net);
static void add_io (int doall, int type, FILE *fp_net, char *buf);
static char *add_clb (int doall, FILE *fp_net, char *buf);
static void add_global (int doall, FILE *fp_net, char *buf);
static void init_parse(int doall);
static void check_netlist (void);
static void free_parse (void);
static void parse_name_and_pinlist (int doall, FILE *fp_net, char *buf);
static int get_num_conn (int bnum);


void read_net (char *net_file) {

/* Main routine that parses a netlist file in my (.net) format. */

 char buf[BUFSIZE], *ptr;
 int doall;
 FILE *fp_net;

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
 check_netlist ();
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
 }

/* Allocate memory for second (load) pass */ 

 else {   
    net = (struct s_net *) my_malloc (num_nets*sizeof(struct s_net));
    block = (struct s_block *) my_malloc (num_blocks*
        sizeof(struct s_block));   
    is_global = (int *) my_calloc (num_nets, sizeof(int));
    num_driver = (int *) my_malloc (num_nets * sizeof(int));
    temp_num_pins = (int *) my_malloc (num_nets * sizeof(int));
    net_pin_class = (int **) my_malloc (num_nets * sizeof (int *));

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
       net[nindex].pins = (int *) my_chunk_malloc(pin_count *
               sizeof(int), NULL, &chunk_bytes_avail, &chunk_next_avail_mem);

       net_pin_class[nindex] = (int *) my_chunk_malloc (pin_count * 
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
   subblock_inf = (struct s_subblock **) my_malloc (num_blocks * 
                  sizeof(struct s_block *));

   for (i=0;i<num_blocks;i++) {
      if (num_subblocks_per_block[i] == 0) 
         subblock_inf[i] = NULL;
      else {
         subblock_inf[i] = (struct s_subblock *) my_chunk_malloc (
            num_subblocks_per_block[i] * sizeof (struct s_subblock), NULL,
            &chunk_bytes_avail, &chunk_next_avail_mem);
         for (j=0;j<num_subblocks_per_block[i];j++) 
            subblock_inf[i][j].inputs = (int *) my_chunk_malloc
                 (subblock_lut_size * sizeof(int), NULL, &chunk_bytes_avail,
                   &chunk_next_avail_mem);
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


static int get_pin_number (char *ptr, int min_val, int max_val) {

/* Returns either the number in ptr or OPEN.  Ptr must contain  *
 * "open" or an integer between min_val and max_val or an error *
 * message is printed and the routine terminates the program.   */

 int val;

 if (strcmp("open",ptr) == 0) 
    return (OPEN);

 val = atoi (ptr);

 if (val < min_val || val > max_val) {
    printf("Error in get_pin_number on line %d of netlist file.\n", 
         linenum);
    printf("Pin %d is out of legal range (%d to %d).\nAborting.\n\n",
         val, min_val, max_val);
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
             sizeof(char), NULL, &chunk_bytes_avail, &chunk_next_avail_mem);
    strcpy (subblock_inf[bnum][num_subblocks-1].name, ptr);
 }

 ptr = my_strtok(NULL,TOKENS,fp_net,temp_buf);

 while (ptr != NULL) {    /* For each subblock pin. */
    if (doall == 1) {
       connect_to = get_pin_number (ptr, 0, pins_per_clb-1);
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

    if (strcmp("subblock:", temp_buf) == 0) {
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
 int pin_index, class, inet;
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
    
    class = clb_pin_class[pin_index];
    type = class_inf[class].type;      /* DRIVER or RECEIVER */

    if (strcmp(ptr,"open") != 0) {     /* Pin is connected. */
       inet = add_net(ptr,type,num_blocks-1,class,doall);

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
   
/* Note the dummy class for io pins.  Change this if necessary. */

    inet = add_net(ptr,type,num_blocks-1,-1,doall);
 
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

/* Doall is 0 for the first (counting) pass and 1 for the second        *
 * (loading) pass.  fp_net is a pointer to the netlist file.  This      *
 * routine sets the proper entry(ies) in is_global to 1 during the      *
 * loading pass.  The routine does nothing during the counting pass. If *
 * is_global = 1 for a net, it will not be considered in the placement  *
 * cost function, nor will it be routed.  This is useful for global     *
 * signals like clocks that generally have dedicated routing in FPGAs.  */

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
    is_global[nindex] = 1;    /* Flagged as global net */

    ptr = my_strtok(NULL,TOKENS,fp_net,buf);
 }
}


static int add_net (char *ptr, enum e_pin_type type, int bnum, int pclass, 
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
    net[nindex].pins[j] = bnum;
    net_pin_class[nindex][j] = pclass;
    return (nindex);
 }
}


static void print_pinnum (FILE *fp, int pinnum) {

/* This routine prints out either OPEN or the pin number, to file fp. */

 if (pinnum == OPEN) 
    fprintf(fp,"\tOPEN");
 else
    fprintf(fp,"\t%d", pinnum);
}


void netlist_echo (char *foutput, char *net_file) {

/* Prints out the netlist related data structures into the file    *
 * fname.                                                          */

 int i, j, ipin, max_pin;	
 FILE *fp;
 
 fp = my_fopen (foutput,"w",0); 

 fprintf(fp,"Input netlist file: %s\n", net_file);
 fprintf(fp,"num_p_inputs: %d, num_p_outputs: %d, num_clbs: %d\n",
             num_p_inputs,num_p_outputs,num_clbs);
 fprintf(fp,"num_blocks: %d, num_nets: %d, num_globals: %d\n",
             num_blocks, num_nets, num_globals);
 fprintf(fp,"\nNet\tName\t\t#Pins\tDriver\t\tRecvs. (block, class)\n");

 for (i=0;i<num_nets;i++) {
    fprintf(fp,"\n%d\t%s\t", i, net[i].name);
    if (strlen(net[i].name) < 8)
       fprintf(fp,"\t");         /* Name field is 16 chars wide */
    fprintf(fp,"%d",net[i].num_pins);
    for (j=0;j<net[i].num_pins;j++) 
        fprintf(fp,"\t(%4d,%4d)",net[i].pins[j], net_pin_class[i][j]);
 }

 fprintf(fp,"\n\n\nBlock List:\t\tBlock Type Legend:\n");
 fprintf(fp,"\t\t\tINPAD = %d\tOUTPAD = %d\n", INPAD, OUTPAD);
 fprintf(fp,"\t\t\tCLB = %d\n\n", CLB);

 fprintf(fp,"\nBlock\tName\t\tType\tPin Connections\n\n");

 for (i=0;i<num_blocks;i++) { 
    fprintf(fp,"\n%d\t%s\t",i,block[i].name);
    if (strlen(block[i].name) < 8)
       fprintf(fp,"\t");         /* Name field is 16 chars wide */
    fprintf(fp,"%d", block[i].type);

    if (block[i].type == INPAD || block[i].type == OUTPAD) 
       max_pin = 1;
    else
       max_pin = pins_per_clb;
 
    for (j=0;j<max_pin;j++) 
        print_pinnum (fp, block[i].nets[j]);
 }  

 fprintf(fp,"\n");

/* Now print out subblock info. */

 fprintf(fp,"\n\nSubblock List:\n\n");

 for (i=0;i<num_blocks;i++) {
    if (block[i].type != CLB) 
       continue;

    fprintf(fp,"\nBlock: %d (%s)\tNum_subblocks: %d\n", i,
                  block[i].name, num_subblocks_per_block[i]);

   /* Print header. */

    fprintf(fp,"Index\tName\t\tInputs");
    for (j=0;j<subblock_lut_size;j++)
       fprintf(fp,"\t");
    fprintf(fp,"Output\tClock\n");

   /* Print subblock info for block i. */

    for (j=0;j<num_subblocks_per_block[i];j++) {
       fprintf(fp,"%d\t%s", j, subblock_inf[i][j].name);
       if (strlen(subblock_inf[i][j].name) < 8) 
          fprintf(fp,"\t");         /* Name field is 16 characters */
       for (ipin=0;ipin<subblock_lut_size;ipin++) 
          print_pinnum (fp, subblock_inf[i][j].inputs[ipin]);
       print_pinnum (fp, subblock_inf[i][j].output);
       print_pinnum (fp, subblock_inf[i][j].clock);
       fprintf(fp,"\n");
    }
 }

 fclose(fp);
}


static void check_netlist (void) {

/* Checks the input netlist for various errors.  */

 int i, ipin, icmp, error, num_conn, class, isubblk, clb_pin;
 struct s_hash **block_hash_table, *h_ptr;
 struct s_hash_iterator hash_iterator;
 
 error = 0;

 for (i=0;i<num_nets;i++) {
    if (num_driver[i] != 1) {
       printf ("Error:  net %s has %d signals driving it.\n",
           net[i].name,num_driver[i]);
       error++;
    }
    if ((net[i].num_pins - num_driver[i]) < 1) {
       printf("Error:  net %s has no fanout.\n",net[i].name);
       error++;
    }
 }

 for (i=0;i<num_blocks;i++) {
    num_conn = get_num_conn (i);

    if (block[i].type == CLB) {
       if (num_conn < 2) {
          printf("Warning:  logic block #%d (%s) has only %d pin.\n",
             i,block[i].name,num_conn);

/* Allow the case where we have only one OUTPUT pin connected to continue. *
 * This is used sometimes as a constant generator for a primary output,    *
 * but I will still warn the user.  If the only pin connected is an input, *
 * abort.                                                                  */

          if (num_conn == 1) {
             for (ipin=0;ipin<pins_per_clb;ipin++) {
                if (block[i].nets[ipin] != OPEN) {
                   class = clb_pin_class[ipin];

                   if (class_inf[class].type != DRIVER) {
                      error++;
                   }
                   else {
                      printf("\tPin is an output -- may be a constant "
                        "generator.  Non-fatal, but check this.\n");
                   }

                   break;
                }
             }
          }
          else {
             error++;
          }
       }

/* This case should already have been flagged as an error -- this is *
 * just a redundant double check.                                    */

       if (num_conn > pins_per_clb) {
          printf("Error:  logic block #%d with output %s has %d pins.\n",
             i,block[i].name,num_conn);
          error++;
       }

/* Now check that the subblock information makes sense. */
       
       if (num_subblocks_per_block[i] < 1 || num_subblocks_per_block[i] >
             max_subblocks_per_block) {
          printf("Error:  block #%d (%s) contains %d subblocks.\n",
             i, block[i].name, num_subblocks_per_block[i]);
          error++;
       }

       for (isubblk=0;isubblk<num_subblocks_per_block[i];isubblk++) {
          for (ipin=0;ipin<subblock_lut_size;ipin++) {  /* Input pins */
             clb_pin = subblock_inf[i][isubblk].inputs[ipin];
             if (clb_pin != OPEN) {
                if (clb_pin < 0 || clb_pin > pins_per_clb) {
                   printf("Error:  Block #%d (%s) subblock #%d (%s) pin #%d "
                      "connects to nonexistent clb pin #%d.\n", i, 
                      block[i].name, isubblk, subblock_inf[i][isubblk].name,
                      ipin, clb_pin);
                   error++;
                }
             }
          }
     
      /* Subblock output pin. */
          clb_pin = subblock_inf[i][isubblk].output;
           /* Output can't be OPEN */
          if (clb_pin < 0 || clb_pin > pins_per_clb) { 
             printf("Error:  Block #%d (%s) subblock #%d (%s) pin #%d \n"
                    "connects to nonexistent clb pin #%d.\n", i, 
                    block[i].name, isubblk, subblock_inf[i][isubblk].name,
                    ipin, clb_pin);
             error++;
          }
          class = clb_pin_class[clb_pin];
          if (class_inf[class].type != DRIVER) {
             printf("Error:  Block #%d (%s) subblock #%d (%s) output pin \n"
                    "connects to clb input pin #%d.\n", i, block[i].name, 
                    isubblk, subblock_inf[i][isubblk].name, clb_pin);
             error++;
          }

      /* Subblock clock pin. */
          clb_pin = subblock_inf[i][isubblk].clock;
          if (clb_pin != OPEN) {
             if (clb_pin < 0 || clb_pin > pins_per_clb) {
                printf("Error:  Block #%d (%s) subblock #%d (%s) pin #%d "
                       "connects to nonexistent clb pin #%d.\n", i, 
                        block[i].name, isubblk, subblock_inf[i][isubblk].name,
                        ipin, clb_pin);
                error++;
                
             }
             class = clb_pin_class[clb_pin];
             if (class_inf[class].type != RECEIVER) { 
                printf("Error:  Block #%d (%s) subblock #%d (%s) clock pin \n"
                       "connects to clb output pin #%d.\n", i, block[i].name, 
                       isubblk, subblock_inf[i][isubblk].name, clb_pin);
                error++;
             }
          }
       }  /* End subblock for loop. */
    }  /* End clb if */

    else {    /* IO block */

/* This error check is also a redundant double check.                 */

       if (num_conn != 1) {
          printf("Error:  io block #%d (%s) of type %d"
             "has %d pins.\n", i, block[i].name, block[i].type,
             num_conn);
          error++;
       }

  /* IO blocks must have no subblock information. */
       if (num_subblocks_per_block[i] != 0) {
          printf("Error:  IO block #%d (%s) contains %d subblocks.\n"
              "Expected 0.\n", i, block[i].name, num_subblocks_per_block[i]);
          error++;
       }
    }
 }

/* Check that no net connects to the same pin class more than once on      *
 * the same block.  I can think of no reason why a netlist would ever      *
 * want to do this, and it would break an assumption used when updating    *
 * the targets still to be reached when routing a net.                     */

 for (i=0;i<num_nets;i++) {
    for (ipin=0;ipin<net[i].num_pins-1;ipin++) {
       for (icmp=ipin+1;icmp<net[i].num_pins;icmp++) {
      
          /* Two pins connect to the same block ? */

          if (net[i].pins[ipin] == net[i].pins[icmp]) {
             if (net_pin_class[i][ipin] == net_pin_class[i][icmp]) {
                printf("Error:  net %d (%s) pins %d and %d connect\n",
                   i, net[i].name, ipin, icmp);
                printf("to the same block and pin class.\n");
                printf("This does not make sense and is not allowed.\n");
                error++;
             }
          }
       } 
    }
 }

/* Now check that all blocks have unique names. */
 
 block_hash_table = alloc_hash_table ();
 for (i=0;i<num_blocks;i++) 
    h_ptr = insert_in_hash_table (block_hash_table, block[i].name, i);

 hash_iterator = start_hash_table_iterator ();
 h_ptr = get_next_hash (block_hash_table, &hash_iterator);
 
 while (h_ptr != NULL) {
    if (h_ptr->count != 1) {
       printf ("Error:  %d blocks are named %s.  Block names must be unique."
               "\n", h_ptr->count, h_ptr->name);
       error++;
    }
    h_ptr = get_next_hash (block_hash_table, &hash_iterator);
 }
 
 free_hash_table (block_hash_table);
 
 if (error != 0) {
    printf("Found %d fatal Errors in the input netlist.\n",error);
    exit(1);
 }
}


static int get_num_conn (int bnum) {

/* This routine returns the number of connections to a block. */

 int i, num_conn;

 num_conn = 0;

 for (i=0;i<pins_per_clb;i++) {
    if (block[bnum].nets[i] != OPEN) 
       num_conn++;
 }

 return (num_conn);
}


static void free_parse (void) {  

/* Release memory needed only during circuit netlist parsing. */

 free (num_driver);
 free_hash_table (hash_table);
 free (temp_num_pins);
}
