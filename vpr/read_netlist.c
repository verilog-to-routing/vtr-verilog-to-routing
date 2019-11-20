#include <string.h>
#include <stdio.h>
#include "pr.h"
#include "util.h"
#include "ext.h"


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
 * must be lower case.  An example .clb declaration is given below.  *
 *                                                                   *
 * .clb name_of_clb  # comment                                       *
 *  pinlist:  net_1 net_2 my_net net_of_mine open D open             *
 *                                                                   *
 * Ending a line with a backslash (\) means it is continued on the   *
 * line below.  A sharp sign (#) indicates the rest of a line is     *
 * a comment.                                                        *
 * The blifmap program can be used to convert a flat blif netlist    *
 * into .net format.                                                 *
 *                                                                   *
 * V. Betz, June 13, 1995.                                           */

/* A note about the way the character buffer, buf, is passed around. *
 * strtok does not make a local copy of the character string         *
 * initially passed to it, so you must make sure it always exists.   *
 * Hence, I just use one buffer declared in read_net and pass it     *
 * downstream.  Starting a new line with an automatic variable       *
 * buffer in a downstream subroutine and then returning and trying   *
 * to keep tokenizing it would cause problems since the buffer now   *
 * lies on a stale part of the stack and can be overwritten.         */


static int *num_driver, *temp_num_pins;
static struct hash_nets **hash;
int add_net (char *ptr, int type, int bnum, int pclass, int doall); 

void read_net (char *net_file) {
 char buf[BUFSIZE];
 int doall;
 FILE *fp_net;
 void get_tok(char *buf, int doall, FILE *fp_net);
 void init_parse(int doall);
 void check_net (void);
 void free_parse (void);

 fp_net = my_open (net_file, "r", 0);

/* First pass builds the symbol table and counts the number of pins  *
 * on each net.  Then I allocate exactly the right amount of storage *
 * for each net.  Finally, the second pass loads the block and net   *
 * arrays.                                                           */

 for (doall=0;doall<=1;doall++) {  /* Pass number. */
    init_parse(doall);

    linenum = 0;   /* Reset line number. */
    while(my_fgets(buf,BUFSIZE,fp_net) != NULL) {
       get_tok(buf, doall, fp_net);
    }
    rewind (fp_net);  /* Start at beginning of file again */
 } 
 fclose(fp_net);
 check_net();
 free_parse();
}

void init_parse(int doall) {
/* Allocates and initializes the data structures needed for the parse. */

 int i, len;
 int *tmp_ptr;
 struct hash_nets *h_ptr;

 if (!doall) {  /* Initialization before first (counting) pass */
    num_nets = 0;  
    hash = (struct hash_nets **) my_calloc(sizeof(struct hash_nets *),
            HASHSIZE);
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

    tmp_ptr = (int *) my_malloc (clb_size * num_blocks * sizeof(int));
    for (i=0;i<num_blocks;i++) 
       block[i].nets = tmp_ptr + i * clb_size;

/* I use my_small_malloc for some storage locations below.  My_small_malloc *
 * avoids the 8 byte or so overhead incurred by malloc, but does not keep   *
 * around enough information to ever free these data arrays.  If you ever   *
 * want to free this stuff or you have compatibility problems on a non      *
 * SPARC architecture, just change all the my_small_malloc calls to         *
 * my_malloc calls.                                                         */

    for (i=0;i<HASHSIZE;i++) {
       h_ptr = hash[i];   
       while (h_ptr != NULL) {
          net[h_ptr->index].pins = (int *) my_small_malloc(h_ptr->count *
               sizeof(int));
	  net_pin_class[h_ptr->index] = (int *) my_small_malloc (h_ptr->count *
               sizeof(int));

/* For avoiding assigning values beyond end of pins array. */
          temp_num_pins[h_ptr->index] = h_ptr->count;

          len = strlen (h_ptr->name);
          net[h_ptr->index].name = (char *) my_small_malloc ((len + 1) *
               sizeof(char));
          strcpy (net[h_ptr->index].name, h_ptr->name);
          h_ptr = h_ptr->next;
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

void get_tok (char *buf, int doall, FILE *fp_net) {
/* Figures out which, if any token is at the start of this line and *
 * takes the appropriate action.                                    */

 void add_io (int doall, int type, FILE *fp_net, char *buf);
 void add_clb (int doall, FILE *fp_net, char *buf);
 void add_global (int doall, FILE *fp_net, char *buf);
 char *ptr; 
 
 ptr = my_strtok(buf,TOKENS,fp_net,buf);
 if (ptr == NULL) return;
 
 if (strcmp(ptr,".clb") == 0) {
    add_clb (doall, fp_net, buf);
    return;
 }

 if (strcmp(ptr,".input") == 0) {
    add_io (doall, INPAD, fp_net, buf);
    return;
 }

 if (strcmp(ptr,".output") == 0) {
    add_io (doall, OUTPAD, fp_net, buf);
    return;
 }

 if (strcmp(ptr,".global") == 0) {
    add_global (doall, fp_net, buf);
    return;
 }

 printf ("Error in get_tok while parsing netlist file.\n");
 printf ("Line %d starts with an invalid token (%s).\n", linenum, ptr);
 exit (1);
}


void add_clb (int doall, FILE *fp_net, char *buf) {
/* Adds the clb (.clb) currently being parsed to the block array.  Adds *
 * its pins to the nets data structure by calling add_net.  If doall is *
 * zero this is a counting pass; if it is 1 this is the final (loading) *
 * pass.                                                                */

 char *ptr;
 int pin_index, class, type, inet;
 void parse_name_and_pinlist (int doall, FILE *fp_net, char *buf);

/* Note:  parse_name_and_pinlist routine increments num_blocks for me. */

 parse_name_and_pinlist (doall, fp_net, buf);
 num_clbs++;

 if (doall) 
    block[num_blocks - 1].type = CLB;

 pin_index = -1;
 ptr = my_strtok(NULL,TOKENS,fp_net,buf);

 while (ptr != NULL) {
    pin_index++;
    if (pin_index >= clb_size) {
       printf("Error in add_clb on line %d of netlist file.\n",linenum);
       printf("Too many pins on this clb.  Expected %d.\n",clb_size);
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

 if (pin_index != clb_size - 1) {
    printf("Error in add_clb on line %d of netlist file.\n",linenum);
    printf("Expected %d pins on clb, got %d.\n", clb_size, pin_index);
    exit (1);
 }
}


void add_io (int doall, int block_type, FILE *fp_net, char *buf) {
/* Adds the INPAD or OUTPAD (specified by block_type)  currently being  *
 * parsed to the block array.  Adds its pin to the nets data structure  *
 * by calling add_net.  If doall is zero this is a counting pass; if it *
 * is 1 this is the final (loading) pass.                               */

 char *ptr;
 int type, inet, pin_index, i;
 void parse_name_and_pinlist (int doall, FILE *fp_net, char *buf);
 
/* Note: the routine called below increments num_blocks. */ 
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
    for (i=1;i<clb_size;i++) 
       block[num_blocks-1].nets[i] = OPEN;
 }
}


void parse_name_and_pinlist (int doall, FILE *fp_net, char *buf) {
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
 
 num_blocks++; 
 
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
    block[num_blocks-1].name = my_small_malloc ((len + 1) * sizeof(char));
    strcpy (block[num_blocks-1].name, ptr);
 }
 
 ptr = my_strtok (NULL,TOKENS,fp_net,buf);
 if (ptr != NULL) {
    printf("Error in parse_name_and_pinlist on line %d of netlist file.\n",
       linenum);
    printf("Extra characters at end of line.\n");
    exit (1);
 }
 
/* Now get pinlist from the next line. */
 
 ptr = my_fgets (buf, BUFSIZE, fp_net);
 if (ptr == NULL) {
    printf("Error in parse_name_and_pinlist on line %d of netlist file.\n",
       linenum);
    printf("Missing pinlist: keyword.\n");
    exit (1);
 }
 
 ptr = my_strtok(buf,TOKENS,fp_net,buf);
 if (ptr == NULL) {
    printf("Error in parse_name_and_pinlist on line %d of netlist file.\n",
       linenum);
    printf("Missing pinlist: keyword.\n");
    exit (1);
 }
 
 if (strcmp (ptr, "pinlist:") != 0) {
    printf("Error in parse_name_and_pinlist on line %d of netlist file.\n",
       linenum);
    printf("Expected pinlist: keyword, got %s.\n",ptr);
    exit (1);
 }
}


void add_global (int doall, FILE *fp_net, char *buf) {
/* Doall is 0 for the first (counting) pass and 1 for the second        *
 * (loading) pass.  fp_net is a pointer to the netlist file.  This      *
 * routine sets the proper entry(ies) in is_global to 1 during the      *
 * loading pass.  The routine does nothing during the counting pass. If *
 * is_global = 1 for a net, it will not be considered in the placement  *
 * cost function, nor will it be routed.  This is useful for global     *
 * signals like clocks that generally have dedicated routing in FPGAs.  */

 char *ptr;
 struct hash_nets *h_ptr;
 int index, nindex;
 int hash_value (char *name);

/* Do nothing if this is the counting pass. */

 if (doall == 0) 
    return; 

 ptr = my_strtok(NULL,TOKENS,fp_net,buf);

 while (ptr != NULL) {     /* For each .global signal */
    num_globals++;

    index = hash_value(ptr);
    h_ptr = hash[index];
    nindex = -1;             /* Flag showing net hasn't been found yet. */

    while (h_ptr != NULL) {                /* Still something in linked list */
       if (strcmp(h_ptr->name,ptr) == 0) { /* Net already in hash table */
          nindex = h_ptr->index;
          is_global[nindex] = 1;    /* Flagged as global net */
          break;
       }
       h_ptr = h_ptr->next;
    }
  
    if (nindex == -1) {        /* Net was not found in list! */
       printf("Error in add_global on netlist file line %d.\n",linenum);
       printf("Global signal %s does not exist.\n",ptr);
       exit(1);
    }

    ptr = my_strtok(NULL,TOKENS,fp_net,buf);
 }
}


int add_net (char *ptr, int type, int bnum, int pclass, int doall) {   
/* This routine is given a net name in *ptr, either DRIVER or RECEIVER *
 * specifying whether the block number given by bnum is driving this   *
 * net or in the fan-out and doall, which is 0 for the counting pass   *
 * and 1 for the loading pass.  It updates the net data structure and  *
 * returns the net number so the calling routine can update the block  *
 * data structure.                                                     */

 struct hash_nets *h_ptr, *prev_ptr;
 int index, j, nindex;
 int hash_value (char *name);

 index = hash_value(ptr);
 h_ptr = hash[index]; 
 prev_ptr = h_ptr;

 while (h_ptr != NULL) {
    if (strcmp(h_ptr->name,ptr) == 0) { /* Net already in hash table */
       nindex = h_ptr->index;

       if (!doall) {   /* Counting pass only */
          (h_ptr->count)++;
          return (nindex);
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
    prev_ptr = h_ptr;
    h_ptr = h_ptr->next;
 }

 /* Net was not in the hash table. */

 if (doall == 1) {
    printf("Error in add_net:  the second (load) pass found could not\n");
    printf("find net %s in the symbol table.\n", ptr);
    exit(1);
 }

/* Add the net (only counting pass will add nets to symbol table). */

 num_nets++;
 h_ptr = (struct hash_nets *) my_malloc (sizeof(struct hash_nets));
 if (prev_ptr == NULL) {
    hash[index] = h_ptr;
 }     
 else {  
    prev_ptr->next = h_ptr;
 }    
 h_ptr->next = NULL;
 h_ptr->index = num_nets - 1;
 h_ptr->count = 1;
 h_ptr->name = (char *) my_malloc((strlen(ptr)+1)*sizeof(char));
 strcpy(h_ptr->name,ptr);
 return (h_ptr->index);
}


int hash_value (char *name) {
 int i,k;
 int val=0, mult=1;
 
 i = strlen(name);
 k = max (i-7,0);
 for (i=strlen(name)-1;i>=k;i--) {
    val += mult*((int) name[i]);
    mult *= 10;
 }
 val += (int) name[0];
 val %= HASHSIZE;
 return(val);
}


void echo_input (char *foutput, char *net_file) {
/* Prints out the netlist related data structures into the file    *
 * fname.                                                          */

 int i, j;	
 FILE *fp;
 
 fp = my_open (foutput,"w",0); 

 fprintf(fp,"Input netlist file: %s\n", net_file);
 fprintf(fp,"num_p_inputs: %d, num_p_outputs: %d, num_clbs: %d\n",
             num_p_inputs,num_p_outputs,num_clbs);
 fprintf(fp,"num_blocks: %d, num_nets: %d, num_globals: %d\n",
             num_blocks, num_nets, num_globals);
 fprintf(fp,"\nNet\tname\t#pins\tdriver\t\trecvs. (block, class)\n");

 for (i=0;i<num_nets;i++) {
    fprintf(fp,"\n%d\t%s\t%d",i,net[i].name,net[i].num_pins);
    for (j=0;j<net[i].num_pins;j++) 
        fprintf(fp,"\t(%4d,%4d)",net[i].pins[j], net_pin_class[i][j]);
 }

 fprintf(fp,"\n\nBlocks\nblock\tname\ttype\tpin connections\n\n");

 for (i=0;i<num_blocks;i++) { 
    fprintf(fp,"\n%d\t%s\t%d",i,block[i].name,
       block[i].type);
    for (j=0;j<clb_size;j++) 
        fprintf(fp,"\t%d",block[i].nets[j]);
 }  

 fprintf(fp,"\n");
 fclose(fp);
}


void check_net (void) {
 int i, ipin, icmp, error, num_conn, class;
 int get_num_conn (int bnum);
 
 error = 0;

 for (i=0;i<num_nets;i++) {
    if (num_driver[i] != 1) {
       printf ("Warning:  net %s has %d signals driving it.\n",
           net[i].name,num_driver[i]);
       error++;
    }
    if ((net[i].num_pins - num_driver[i]) < 1) {
       printf("Warning:  net %s has no fanout.\n",net[i].name);
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
             for (ipin=0;ipin<clb_size;ipin++) {
                if (block[i].nets[ipin] != OPEN) {
                   class = clb_pin_class[ipin];

                   if (class_inf[class].type != DRIVER) {
                      error++;
                   }
                   else {
                      printf("\tPin is an output -- may be a contant "
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

       if (num_conn > clb_size) {
          printf("Warning:  logic block #%d with output %s has %d pins.\n",
             i,block[i].name,num_conn);
          error++;
       }
    }

    else {    /* IO block */

/* This error check is also a redundant double check.                 */

       if (num_conn != 1) {
          printf("Warning:  io block #%d (%s) of type %d"
             "has %d pins.\n", i, block[i].name, block[i].type,
             num_conn);
          error++;
       }
    }
 }

/* Check that no net connects to the same pin class more than once on  *
 * the same block.  I can think of no reason why a netlist would ever  *
 * want to do this, and it would break an assumption used in update_   *
 * markers.                                                            */

 for (i=0;i<num_nets;i++) {
    for (ipin=0;ipin<net[i].num_pins-1;ipin++) {
       for (icmp=ipin+1;icmp<net[i].num_pins;icmp++) {
      
          /* Two pins connect to the same block ? */

          if (net[i].pins[ipin] == net[i].pins[icmp]) {
             if (net_pin_class[i][ipin] == net_pin_class[i][icmp]) {
                printf("Warning:  net %d (%s) pins %d and %d connect\n",
                   i, net[i].name, ipin, icmp);
                printf("to the same block and pin class.\n");
                printf("This does not make sense and is not allowed.\n");
                error++;
             }
          }
       } 
    }
 }
 
 if (error != 0) {
    printf("Found %d fatal Errors in the input netlist.\n",error);
    exit(1);
 }
}


int get_num_conn (int bnum) {
/* This routine returns the number of connections to a block. */

 int i, num_conn;

 num_conn = 0;

 for (i=0;i<clb_size;i++) {
    if (block[bnum].nets[i] != OPEN) 
       num_conn++;
 }

 return (num_conn);
}


void free_parse (void) {  
 /* Release memory needed only during circuit netlist parsing. */
 int i;
 struct hash_nets *h_ptr, *temp_ptr;

 for (i=0;i<HASHSIZE;i++) {
    h_ptr = hash[i];
    while (h_ptr != NULL) {
       free ((void *) h_ptr->name);
       temp_ptr = h_ptr->next;
       free ((void *) h_ptr);
       h_ptr = temp_ptr;
    }
 }
 free ((void *) num_driver);
 free ((void *) hash);
 free ((void *) temp_num_pins);
}
