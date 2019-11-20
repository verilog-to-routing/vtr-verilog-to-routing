#include <string.h>
#include <stdio.h>
#include "blifmap.h"
#include "util.h"
#include "ext.h"


/* This source file will read in a FLAT blif netlist consisting     *
* of .inputs, .outputs, .names and .latch commands.  It currently   *
* does not handle hierarchical blif files.  Hierarchical            *
* blif files can be flattened via the read_blif and write_blif      *
* commands of sis.  LUT circuits should only have .names commands;  *
* there should be no gates.  This parser performs limited error     *
* checking concerning the consistency of the netlist it obtains.    *
* .inputs and .outputs statements must be given; this parser does   *
* not infer primary inputs and outputs from non-driven and fanout   *
* free nodes.  This parser can be extended to do this if necessary, *
* or the sis read_blif and write_blif commands can be used to put a *
* netlist into the standard format.                                 *
* V. Betz, August 25, 1994.                                         *
* Added more error checking, March 30, 1995, V. Betz                */


static int *num_driver, *temp_num_pins;
static int ilines, olines,endlines;  /* # of .input, .output and .end lines */
static struct hash_nets **hash;
int add_net (char *ptr, int type, int bnum, int doall); 

void read_blif (FILE *blif) {
 char buffer[BUFSIZE];
 int pass, done, doall;
 void get_tok(char *buffer, int pass, int doall, int *done);
 void init_parse(int doall);
 void check_net (void);
 void free_parse (void);

 for (doall=0;doall<=1;doall++) {
    init_parse(doall);

/* Three passes to ensure inputs are first blocks, outputs second and    *
 * LUTs and latches third.  Just makes the output netlist more readable. */

    for (pass=1;pass<=3;pass++) { 
       linenum = 0;   /* Reset line number. */
       done = 0;
       while((my_fgets(buffer,BUFSIZE,blif) != NULL) && !done) {
          get_tok(buffer, pass, doall, &done);
       }
       rewind (blif);  /* Start at beginning of file again */
    }
 } 
 fclose(blif);
 check_net();
 free_parse();
}

void init_parse(int doall) {
/* Allocates and initializes the data structures needed for the parse. */

 int i, len;
 struct hash_nets *h_ptr;

 if (!doall) {  /* Initialization before first (counting) pass */
    num_nets = 0;  
    hash = (struct hash_nets **) my_calloc(sizeof(struct hash_nets *),
            HASHSIZE);
 }

/* Allocate memory for second (load) pass */ 

 else {   
    net = (struct s_net *) my_malloc(num_nets*sizeof(struct s_net));
    block = (struct s_block *) my_malloc(num_blocks*
        sizeof(struct s_block));   
    num_driver = (int *) my_malloc(num_nets * sizeof(int));
    temp_num_pins = (int *) my_malloc(num_nets*sizeof(int));

    for (i=0;i<num_nets;i++) {
       num_driver[i] = 0;
       net[i].num_pins = 0;
    }

    for (i=0;i<HASHSIZE;i++) {
       h_ptr = hash[i];   
       while (h_ptr != NULL) {
          net[h_ptr->index].pins = (int *) my_malloc(h_ptr->count*sizeof(int));

/* For avoiding assigning values beyond end of pins array. */
          temp_num_pins[h_ptr->index] = h_ptr->count;
          len = strlen (h_ptr->name);
          net[h_ptr->index].name = (char *) my_malloc ((len + 1)* sizeof(char));
          strcpy (net[h_ptr->index].name, h_ptr->name);
          h_ptr = h_ptr->next;
       }
    }
/*    printf("i\ttemp_num_pins\n\n");
    for (i=0;i<num_nets;i++) {
       printf("%d\t%d\n",i,temp_num_pins[i]);
    }  */
 }

/* Initializations for both passes. */

 ilines = 0;
 olines = 0;
 endlines = 0;
 num_p_inputs = 0;
 num_p_outputs = 0;
 num_luts = 0;
 num_latches = 0;
 num_blocks = 0;
}

void get_tok (char *buffer, int pass, int doall, int *done) {
/* Figures out which, if any token is at the start of this line and *
 * takes the appropriate action.                                    */

#define TOKENS " \t\n"
 void io_line (int in_or_out, int doall);
 void add_lut (int doall);
 void add_latch (int doall);
 void dum_parse (char *buf);
 char *ptr; 
 
 ptr = my_strtok(buffer,TOKENS,blif,buffer);
 if (ptr == NULL) return; 
 
 if (strcmp(ptr,".names") == 0) {
    if (pass == 3) {
       add_lut(doall);
    }
    else {
       dum_parse(buffer);
    }
    return;
 }

 if (strcmp(ptr,".latch") == 0) {
    if (pass == 3) {
       add_latch (doall);
    }
    else {
       dum_parse(buffer);
    }
    return;
 }

 if (strcmp(ptr,".model") == 0) {
    ptr = my_strtok(NULL,TOKENS,blif,buffer);
    if (ptr != NULL) strcpy(model,ptr);
    return;
 }
 if (strcmp(ptr,".inputs") == 0) {
    if (pass == 1) {
       io_line(DRIVER, doall);
       *done = 1;
    }
    else {
       dum_parse(buffer);
       if (pass == 3 && doall) ilines++;    /* Error checking only */
    }
    return;
 }
 if (strcmp(ptr,".outputs") == 0) {
    if (pass == 2) {
       io_line(RECEIVER, doall);
       *done = 1;
    }
    else {
       dum_parse(buffer);
       if (pass == 3 && doall) olines++;  /* Make sure only one .output line */
    }                            /* For error checking only */
    return;
 }
 if (strcmp(ptr,".end") == 0) {
    if (pass == 3 && doall) endlines++;   /* Error checking only */
    return;
 }

/* Could have numbers following a .names command, so not matching any *
 * of the tokens above is not an error.                               */

}

void dum_parse (char *buf) {
/* Continue parsing to the end of this (possibly continued) line. */

 while (my_strtok(NULL,TOKENS,blif,buf) != NULL)
       ;
}

void add_lut (int doall) {
/* Adds a LUT (.names) currently being parsed to the block array.  Adds *
 * its pins to the nets data structure by calling add_net.  If doall is *
 * zero this is a counting pass; if it is 1 this is the final (loading) *
 * pass.                                                                */

 char *ptr[MAXLUT+2], buf[BUFSIZE];
 int i, j, len;

 num_blocks++;

/* Count # nets connecting */
 i=0;
 while ((ptr[i] = my_strtok(NULL,TOKENS,blif,buf)) != NULL)  {
    if (i == MAXLUT+1) {
       fprintf(stderr,"Error:  LUT #%d has %d inputs.  Increase MAXLUT or"
            "check the netlist, line %d.\n",num_blocks-1,i-1,linenum);
       exit(1);
    }
    i++;
 }

 if (!doall) {          /* Counting pass only ... */
    for (j=0;j<i;j++) 
       add_net(ptr[j],RECEIVER,num_blocks-1,doall);
    return;
 }

 block[num_blocks-1].num_nets = i;
 block[num_blocks-1].type = LUT;
 for (i=0;i<block[num_blocks-1].num_nets-1;i++)   /* Do inputs */
    block[num_blocks-1].nets[i+1] = add_net(ptr[i],RECEIVER,num_blocks-1,doall); 
 block[num_blocks-1].nets[0] = add_net(ptr[block[num_blocks-1].num_nets-1],
       DRIVER,num_blocks-1,doall);

 for (i=block[num_blocks-1].num_nets; i<lut_size+2; i++)
    block[num_blocks-1].nets[i] = OPEN;

 len = strlen (ptr[block[num_blocks-1].num_nets-1]);
 block[num_blocks-1].name = (char *) my_malloc ((len+1) * sizeof (char));
 strcpy(block[num_blocks-1].name,ptr[block[num_blocks-1].num_nets-1]);
 num_luts++;
}


void add_latch (int doall) {
/* Adds the flipflop (.latch) currently being parsed to the block array. *
 * Adds its pins to the nets data structure by calling add_net.  If doall*
 * is zero this is a counting pass; if it is 1 this is the final         *
 * (loading) pass.  Blif format for a latch is:                          *
 * .latch <input> <output> <type (latch on)> <control (clock)> <init_val>*
 * The latch pins are in .nets 0 to 2 in the order: Q D CLOCK.           */

 char *ptr[6], buf[BUFSIZE];
 int i, len;

 num_blocks++;

/* Count # parameters, making sure we don't go over 6 (avoids memory corr.) */
 for (i=0;i<6;i++) {
    ptr[i] = my_strtok (NULL,TOKENS,blif,buf);
    if (ptr[i] == NULL) break;
 }

 if (i != 5) {
    fprintf(stderr,"Error:  .latch does not have 5 parameters.\n"
            "check the netlist, line %d.\n",linenum);
    exit(1);
 }

 if (!doall) {   /* If only a counting pass ... */
    add_net(ptr[0],RECEIVER,num_blocks-1,doall);  /* D */
    add_net(ptr[1],DRIVER,num_blocks-1,doall);    /* Q */
    add_net(ptr[3],RECEIVER,num_blocks-1,doall);  /* Clock */
    return;
 }

 block[num_blocks-1].num_nets = 3;
 block[num_blocks-1].type = LATCH;

 block[num_blocks-1].nets[0] = add_net(ptr[1],DRIVER,num_blocks-1,
     doall);                                                        /* Q */
 block[num_blocks-1].nets[1] = add_net(ptr[0],RECEIVER,num_blocks-1,
     doall);                                                        /* D */
 block[num_blocks-1].nets[lut_size+1] = add_net(ptr[3],RECEIVER,     
     num_blocks-1,doall);                                           /* Clock */

 for (i=2;i<lut_size+1;i++) 
    block[num_blocks-1].nets[i] = OPEN;
    
 len = strlen (ptr[1]);
 block[num_blocks-1].name = (char *) my_malloc ((len+1) * sizeof (char));
 strcpy(block[num_blocks-1].name,ptr[1]);
 num_latches++;
}


void io_line(int in_or_out, int doall) {  
/* Adds an input or output block to the block data structures.           *
 * in_or_out:  DRIVER for input, RECEIVER for output.                    *
 * doall:  1 for final pass when structures are loaded.  0 for           *
 * first pass when hash table is built and pins, nets, etc. are counted. */ 

 char *ptr;
 char buf2[BUFSIZE];
 int nindex, len;
  
 while (1) {
    ptr = my_strtok(NULL,TOKENS,blif,buf2);
    if (ptr == NULL) return;
    num_blocks++;
    
    nindex = add_net(ptr,in_or_out,num_blocks-1,doall); 
                 /* zero offset indexing */
    if (!doall) continue;   /* Just counting things when doall == 0 */

    len = strlen (ptr);
    if (in_or_out == RECEIVER) {  /* output pads need out: prefix 
                                     to make names unique from LUTs */
       block[num_blocks-1].name = (char *) my_malloc ((len+1+4) *
            sizeof (char));          /* Space for out: at start */
       strcpy(block[num_blocks-1].name,"out:");
       strcat(block[num_blocks-1].name,ptr);
    }
    else {
       block[num_blocks-1].name = (char *) my_malloc ((len+1) *
            sizeof (char));          /* Space for out: at start */
       strcpy(block[num_blocks-1].name,ptr);
    }

    block[num_blocks-1].num_nets = 1;
    block[num_blocks-1].nets[0] = nindex;  /* Put in driver position for */
   /*  OUTPAD, since it has only one pin (even though it's a receiver */

    if (in_or_out == DRIVER) {             /* processing .inputs line */
       num_p_inputs++;
       block[num_blocks-1].type = INPAD;
    }
    else {                                 /* processing .outputs line */
       num_p_outputs++;
       block[num_blocks-1].type = OUTPAD;
    }
 } 
}

int add_net (char *ptr, int type, int bnum, int doall) {   
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

void echo_input (void) {
 int i, j;	
 FILE *fp;

 printf("Input netlist file: %s  Model: %s\n",blif_file,model);
 printf("num_p_inputs: %d, num_p_outputs: %d, num_luts: %d,"
            " num_latches: %d\n",num_p_inputs,num_p_outputs,num_luts,
             num_latches);
 printf("num_blocks: %d, num_nets: %d\n",num_blocks,num_nets);
 
 fp = my_open ("input.echo","w",0); 
 fprintf(fp,"Input netlist file: %s  Model: %s\n",blif_file,model);
 fprintf(fp,"num_p_inputs: %d, num_p_outputs: %d, num_luts: %d,"
            " num_latches: %d\n",num_p_inputs,num_p_outputs,num_luts,
             num_latches);
 fprintf(fp,"num_blocks: %d, num_nets: %d\n",num_blocks,num_nets);
 fprintf(fp,"\nNet\tname\t#pins\tdriver\trecvs.\n");
 for (i=0;i<num_nets;i++) {
    fprintf(fp,"\n%d\t%s\t%d",i,net[i].name,net[i].num_pins);
    for (j=0;j<net[i].num_pins;j++) 
        fprintf(fp,"\t%d",net[i].pins[j]);
 }
fprintf(fp,"\n\nBlocks\nblock\tname\ttype\t#nets\toutput\tinputs\n\n");
 for (i=0;i<num_blocks;i++) { 
    fprintf(fp,"\n%d\t%s\t%d\t%d",i,block[i].name,
       block[i].type,block[i].num_nets);
    for (j=0;j<block[i].num_nets;j++)  
        fprintf(fp,"\t%d",block[i].nets[j]);
 }  
 fprintf(fp,"\n");
 fclose(fp);
}

void check_net (void) {
 int i, error, iblk;
 
 error = 0;

 if (ilines != 1) {
     printf("Warning:  found %d .inputs lines; expected 1.\n",
        ilines);
     error++;
 }
 if (olines != 1) {
     printf("Warning:  found %d .outputs lines; expected 1.\n",
        olines);
     error++;
 }
 if (endlines != 1) {
     printf("Warning:  found %d .end lines; expected 1.\n",
        endlines);
     error++;
 }

 for (i=0;i<num_nets;i++) {

    if (num_driver[i] != 1) {
       printf ("Warning:  net %s has"
         " %d signals driving it.\n",net[i].name,num_driver[i]);
       error++;
    }

    if ((net[i].num_pins - num_driver[i]) < 1) {

/* If this is an input pad, it is unused and I just remove it with  *
 * a warning message.  Lots of the mcnc circuits have this problem. */

       iblk = net[i].pins[0];
       if (block[iblk].type == INPAD) {
          printf("Warning:  Input %s is unused; removing it.\n",
             block[iblk].name);
          net[i].pins[0] = OPEN;
          block[iblk].type = EMPTY;
       }
          
       else {
          printf("Warning:  net %s has no fanout.\n",net[i].name);
          error++;
       }
    }

    if (strcmp(net[i].name, "open") == 0) {
       printf("Warning:  net #%d has the reserved name %s.\n",i,net[i].name);
       error++;
    }
 }

 for (i=0;i<num_blocks;i++) {
    if (block[i].type == LUT) {
       if (block[i].num_nets < 2) {
          printf("Warning:  logic block #%d with output %s has only %d pin.\n",
             i,block[i].name,block[i].num_nets);

/* LUTs with 1 pin (an output)  can be a constant generator.  Warn the   *
 * user, but don't exit.                                                 */

          if (block[i].num_nets != 1) {
             error++;
          }
          else {
             printf("\tPin is an output -- may be a constant generator.\n");
             printf("\tNon-fatal error.\n");
          }
       }

       if (block[i].num_nets > lut_size + 1) {
          printf("Warning:  logic block #%d with output %s has %d pins.\n",
             i,block[i].name,block[i].num_nets);
          error++;
       }
    }

    else if (block[i].type == LATCH) {
       if (block[i].num_nets != 3) {
          printf("Warning:  Latch #%d with output %s has %d pin(s).\n",
             i, block[i].name, block[i].num_nets);
          error++;
       }
    }
       
    else {
       if (block[i].num_nets != 1) {
          printf("Warning:  io block #%d with output %s of type %d"
             "has %d pins.\n", i, block[i].name, block[i].type,
             block[i].num_nets);
          error++;
       }
    }
 }
 
 if (error != 0) {
    printf("Found %d fatal errors in the input netlist.\n",error);
    exit(1);
 }
}

void free_parse (void) {  
 /* Release memory needed only during blif network parsing. */
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


