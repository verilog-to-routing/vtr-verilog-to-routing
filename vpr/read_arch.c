#include "pr.h"
#include "util.h"
#include "ext.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* This source file reads in the architectural description of an FPGA. *
 * A # symbol anywhere in the input file denotes a comment to the end  *
 * of the line.  Put a \ at the end of a line if you want to continue  *
 * a command across multiple lines.   Non-comment lines are in the     *
 * format keyword value(s).  The entire file should be lower case.     *
 * The keywords and their arguments are:                               *
 *                                                                     *
 *   io_rat integer (sets the number of io pads which fit into the     *
 *                  space one CLB would use).                          *
 *   chan_width_io float   (Width of the channels between the pads and *
 *                          core relative to the widest core channel.) *
 *   chan_width_x [gaussian|uniform|pulse] peak <width> <xpeak> <dc>.  *
 *       (<> bracketed quantities needed only for pulse and gaussian.  *
 *       Width and xpeak values from 0 to 1.  Sets the distribution of *
 *       tracks for the x-directed channels.)                          *
 *       Other possibility:  delta peak xpeak dc.                      *
 *   chan_width_y [gaussian|uniform|pulse] peak <width> <xpeak> <dc>.  *
 *       (Sets the distribution of tracks for the y-directed channels.)*
 *   outpin class: integer [top|bottom|left|right] [top|bottom|left|   *
 *       right] ...                                                    *
 *       (Sets the class to which each pin belongs and the side(s) of  *
 *       CLBs on which the physical output pin connection(s) is (are). *
 *       All pins with the same class number are logically equivalent  *
 *       -- such as all the inputs of a LUT.  Class numbers must start *
 *       at zero and be consecutive.)                                  *
 *   inpin class: integer [top|bottom|left|right] [top|bottom|left|
 *       right] ...                                                    *
 *       (All parameters have the same meanings as their counterparts  *
 *       in the outpin statement)                                      *
 *                                                                     *
 *   NOTE:  The order in which your inpin and outpin statements appear *
 *      must be the same as the order in which your netlist (.net)     *
 *      file lists the connections to the clbs.  For example, if the   *
 *      first pin on each clb in the netlist file is the clock pin,    *
 *      your first pin statement in the architecture file must be      *
 *      an inpin statement defining the clock pin.                     */

#define NUMINP 6

static int isread[NUMINP];
static char *names[NUMINP] = {"io_rat", "chan_width_x", 
   "chan_width_y", "chan_width_io", "outpin", "inpin"};
float get_float (char *ptr, int inp_num, float llim, float ulim,
    FILE *fp_arch, char *buf); 


void read_arch (char *arch_file) {
/* Reads in the architecture description file for the FPGA. */

 int i, j, pinnum;
 char *ptr, buf[BUFSIZE];
 FILE *fp_arch;
 int get_int (char *ptr, int inp_num, FILE *fp_arch, char *buf);

 void check_arch(char *arch_file);
 void get_chan (char *ptr, struct s_chan *chan, int inp_num, 
     FILE *fp_arch, char *buf); 
 void get_pin (char *ptr, int pinnum, int type, FILE *fp_arch, char *buf);
 void countpass (FILE *fp_arch);

 fp_arch = my_open (arch_file, "r", 0);
 countpass (fp_arch);

 rewind (fp_arch);
 linenum = 0;
 pinnum = 0;

 for (i=0;i<NUMINP;i++) 
    isread[i] = 0;

 pinloc = (int **) alloc_matrix (0, 3, 0, clb_size-1, sizeof (int));

 for (i=0;i<=3;i++) 
    for (j=0;j<clb_size;j++) 
       pinloc[i][j] = 0; 

 while ((ptr = my_fgets(buf, BUFSIZE, fp_arch)) != NULL) { 
    ptr = my_strtok(ptr, TOKENS, fp_arch, buf);
    if (ptr == NULL) continue;
    if (strcmp(ptr,names[0]) == 0) {  /* io_rat */
       io_rat = get_int (ptr, 0, fp_arch, buf);
       continue;
    }
    if (strcmp(ptr,names[1]) == 0) { /*chan_width_x */
       get_chan(ptr, &chan_x_dist, 1, fp_arch, buf);
       continue;
    }
    if (strcmp(ptr,names[2]) == 0) { /* chan_width_y */
       get_chan(ptr, &chan_y_dist, 2, fp_arch, buf);
       continue;
    }
    if (strcmp(ptr,names[3]) == 0) { /* chan_width_io */
       chan_width_io = get_float (ptr, 3, 0. ,5000., fp_arch, buf);
       isread[3]++;
       continue;
    }
    if (strcmp(ptr,names[4]) == 0) { /* outpin */
       get_pin (ptr, pinnum, DRIVER, fp_arch, buf);
       pinnum++;
       isread[4]++;
       continue;   
    }
    if (strcmp(ptr,names[5]) == 0) { /* inpin */
       get_pin (ptr, pinnum, RECEIVER, fp_arch, buf);
       pinnum++;
       isread[5]++;
       continue;
    }
 }
 check_arch(arch_file);
 fclose (fp_arch);
}


void countpass (FILE *fp_arch) {
/* This routine parses the input architecture file in order to count *
 * the number of pinclasses so storage can be allocated for them     *
 * before the second (loading) pass begins.                          */

 char buf[BUFSIZE], *ptr;
 int *pins_per_class, class, i;
 int get_class (FILE *fp_arch, char *buf);

 linenum = 0;
 num_class = 1;   /* Must be at least 1 class  */
 
 pins_per_class = (int *) my_calloc (num_class, sizeof (int));

 while ((ptr = my_fgets(buf, BUFSIZE, fp_arch)) != NULL) { 
    ptr = my_strtok (ptr, TOKENS, fp_arch, buf);
    if (ptr == NULL) continue;

    if (strcmp (ptr, "inpin") == 0 || strcmp (ptr, "outpin") == 0) {
       class = get_class (fp_arch, buf);

       if (class >= num_class) {
          pins_per_class = (int *) my_realloc (pins_per_class, 
             (class + 1) * sizeof (int));

          for (i=num_class;i<=class;i++) 
             pins_per_class[i] = 0;

          num_class = class + 1;
       }

       pins_per_class[class]++;
    }

/* Go to end of line (possibly continued) */

    ptr = my_strtok (NULL, TOKENS, fp_arch, buf);
    while (ptr != NULL) {
       ptr = my_strtok (NULL, TOKENS, fp_arch, buf);
    } 
 }

/* Check for missing classes. */

 for (i=0;i<num_class;i++) {
    if (pins_per_class[i] == 0) {
       printf("\nError:  class index %d not used in architecture "
               "file.\n", i);
       printf("          Specified class indices are not consecutive.\n");
       exit (1);
    }
 }

/* I've now got a count of how many classes there are and how many    *
 * pins belong to each class.  Allocate the proper memory.            */

 class_inf = (struct s_class *) my_malloc (num_class * sizeof (struct s_class));

 clb_size = 0;
 for (i=0;i<num_class;i++) {
    class_inf[i].type = OPEN;                   /* Flag for not set yet. */
    class_inf[i].num_pins = 0; 
    class_inf[i].pinlist = (int *) my_malloc (pins_per_class[i] *
        sizeof (int));
    clb_size += pins_per_class[i];
 }

 free (pins_per_class);

 clb_pin_class = (int *) my_malloc (clb_size * sizeof (int));
}


int get_class (FILE *fp_arch, char *buf) {
/* This routine is called when strtok has moved the pointer to just before *
 * the class: keyword.  It advances the pointer to after the class         *
 * descriptor and returns the class number.                                */

 int class;
 char *ptr;

 ptr = my_strtok (NULL, TOKENS, fp_arch, buf);

 if (ptr == NULL) {
    printf("Error in get_class on line %d of architecture file.\n",linenum);
    printf("Expected class: keyword.\n");
    exit (1);
 }

 if (strcmp (ptr, "class:") != 0) {
    printf("Error in get_class on line %d of architecture file.\n",linenum);
    printf("Expected class: keyword.\n");
    exit (1);
 }

/* Now get class number. */

 ptr = my_strtok (NULL, TOKENS, fp_arch, buf);
 if (ptr == NULL) {
    printf("Error in get_class on line %d of architecture file.\n",linenum);
    printf("Expected class number.\n");
    exit (1);
 }

 class = atoi (ptr);
 if (class < 0) {
    printf("Error in get_class on line %d of architecture file.\n",linenum);
    printf("Expected class number >= 0, got %d.\n",class);
    exit (1);
 }

 return (class);
}


void get_pin (char *ptr, int pinnum, int type, FILE *fp_arch, char * buf) {
/* This routine parses an ipin or outpin line.  It should be called right *
 * after the inpin or outpin keyword has been parsed.                     */

 int i, valid, class, ipin;
 char *position[4] = {"top", "bottom", "left", "right"};
 int get_class (FILE *fp_arch, char *buf); 

 class = get_class (fp_arch, buf);

 if (class_inf[class].type == OPEN) {  /* First time through this class. */
    class_inf[class].type = type;
 }
 else {
    if (class_inf[class].type != type) {
       printf("Error in get_pin: architecture file, line %d.\n",
          linenum);
       printf("Class %d contains both input and output pins.\n",class);
       exit (1);
    }
 }

 ipin = class_inf[class].num_pins;
 class_inf[class].pinlist[ipin] = pinnum;
 class_inf[class].num_pins++;

 clb_pin_class[pinnum] = class;
  
 ptr = my_strtok(NULL,TOKENS,fp_arch,buf);
 if (ptr == NULL) {
    printf("Error:  pin statement specifies no locations, line %d.\n",
       linenum);
    exit(1);
 }

 do {
    valid = 0;
    for (i=0;i<=3;i++) {
       if (strcmp(ptr,position[i]) == 0) {
          pinloc[i][pinnum] = 1;
          valid = 1;
          break;
       }
    }
    if (valid != 1) {
       printf("Error:  bad pin location on line %d.\n", linenum);
       exit(1);
    }
 } while((ptr = my_strtok(NULL,TOKENS,fp_arch,buf)) != NULL);
}


int get_int (char *ptr, int inp_num, FILE *fp_arch, char *buf) {
/* This routine gets the next integer on the line.  It must be greater *
 * than zero or an error message is printed.                           */

 int val;

 ptr = my_strtok(NULL,TOKENS,fp_arch,buf);
 if (ptr == NULL) {
    printf("Error:  missing %s value on line %d.\n",
       names[inp_num],linenum);
    exit(1);
 }
 val = atoi(ptr);
 if (val <= 0) {
    printf("Bad value.  %s = %d on line %d.\n",
       names[inp_num],val,linenum);
    exit(1);
 }

 ptr = my_strtok (NULL, TOKENS, fp_arch, buf);
 if (ptr != NULL) {
    printf("Error:  extra characters at end of line %d.\n", linenum);
    exit (1);
 }

 isread[inp_num]++;
 return(val);
}

float get_float (char *ptr, int inp_num, float low_lim, 
   float upp_lim, FILE *fp_arch, char *buf) {
/* This routine gets the floating point number that is next on the line. *
 * low_lim and upp_lim specify the allowable range of numbers, while     *
 * inp_num gives the type of input line being parsed.                    */

 float val;
 
 ptr = my_strtok(NULL,TOKENS,fp_arch,buf);
 if (ptr == NULL) {
    printf("Error:  missing %s value on line %d.\n",
       names[inp_num],linenum);
    exit(1);
 }
 val = atof(ptr);
 if (val <= low_lim || val > upp_lim) {
    printf("Bad value parsing %s. %f on line %d.\n",
       names[inp_num],val,linenum);
    exit(1);
 }
 return(val);
}


/* Order:  chan_width_x [gaussian|uniform|pulse] peak <width>  *
 * <xpeak> <dc>.  (Bracketed quantities needed only for pulse  *
 * and gaussian).  All values from 0 to 1, except peak and dc, *
 * which can be anything.                                      *
 * Other possibility:  chan_width_x delta peak xpeak dc        */

void get_chan (char *ptr, struct s_chan *chan, int inp_num, 
   FILE *fp_arch, char *buf) {
/* This routine parses a channel functional description line.  chan  *
 * is the channel data structure to be loaded, while inp_num is the  *
 * type of input line being parsed.                                  */

 ptr = my_strtok(NULL,TOKENS,fp_arch,buf);
 if (ptr == NULL) {
    printf("Error:  missing %s value on line %d.\n",
       names[inp_num],linenum);
    exit(1);
 }

 if (strcmp(ptr,"uniform") == 0) {
    isread[inp_num]++;
    chan->type = UNIFORM;
    chan->peak = get_float(ptr,inp_num,0.,1., fp_arch, buf);
    chan->dc = 0.;
 }
 else if (strcmp(ptr,"delta") == 0) {
    isread[inp_num]++;
    chan->type = DELTA;
    chan->peak = get_float(ptr,inp_num,-1.e5,1.e5, fp_arch, buf); 
    chan->xpeak = get_float(ptr,inp_num,-1e-30,1., fp_arch, buf);
    chan->dc = get_float(ptr,inp_num,-1e-30,1., fp_arch, buf);
 }
 else {
    if (strcmp(ptr,"gaussian") == 0) 
       chan->type = GAUSSIAN; 
    if (strcmp(ptr,"pulse") == 0) 
       chan->type = PULSE;
    if (chan->type == GAUSSIAN || chan->type == PULSE) {
       isread[inp_num]++;
       chan->peak = get_float(ptr,inp_num,-1.,1., fp_arch, buf); 
       chan->width = get_float(ptr,inp_num,0.,1.e10, fp_arch, buf);
       chan->xpeak = get_float(ptr,inp_num,-1e-30,1., fp_arch, buf);
       chan->dc = get_float(ptr,inp_num,-1e-30,1., fp_arch, buf);
    } 
 }

 if (isread[inp_num] == 0) {
    printf("Error:  %s distribution keyword: %s unknown.\n",
       names[inp_num],ptr);
    exit(1);
 }

 if (my_strtok(NULL,TOKENS,fp_arch,buf) != NULL) {
    printf("Error:  extra value for %s at end of line %d.\n",
       names[inp_num],linenum);
    exit(1);
 }
}
    

void check_arch (char *arch_file) { 
/* This routine checks that the input architecture file makes sense and *
 * specifies all the needed parameters.  The parameters must also be    *
 * self-consistent and make sense.                                      */

 int i, fatal=0;

/* Expect all isread to be 1, except isread[4] (outpin) and isread[5]  *
 * (inpin)  which should both be greater than 0.                       */
 
 for (i=0;i<NUMINP-2;i++) {
    if (isread[i] == 0) {
       printf("Error:  %s not set in file %s.\n",names[i],
          arch_file);
       fatal=1;
    }
    if (isread[i] > 1) {
       printf("Error:  %s set %d times in file %s.\n",names[i],
           isread[i],arch_file);
       fatal = 1;
    }
 }

 for (i=NUMINP-2;i<NUMINP;i++) {
    if (isread[i] < 1) {
       printf("Error:  in file %s.  Clb has %d %s(s).\n",arch_file, 
          isread[i], names[i]);
       fatal = 1;
    }
 }

 if (fatal) exit(1);
}
       
void print_arch (char *arch_file) {
/* Prints out the architectural parameters for verification in the  *
 * file "arch.echo."  The name of the architecture file is passed   *
 * in and is printed out as well.                                   */

 int i, j;
 FILE *fp;

 fp = my_open ("arch.echo", "w", 0);

 fprintf(fp,"Input netlist file: %s\n\n",arch_file);

 fprintf(fp,"io_rat: %d.\n",io_rat);
 fprintf(fp,"chan_width_io: %f  clb_size: %d\n",chan_width_io,
     clb_size);

 fprintf(fp,"\n\nChannel Types:  UNIFORM = %d; GAUSSIAN = %d; PULSE = %d;"
    " DELTA = %d\n\n", UNIFORM, GAUSSIAN, PULSE, DELTA);

 fprintf(fp,"\nchan_width_x:\n");
 fprintf(fp,"type: %d  peak: %f  width: %f  xpeak: %f  dc: %f\n",
   chan_x_dist.type, chan_x_dist.peak, chan_x_dist.width, 
     chan_x_dist.xpeak, chan_x_dist.dc);

 fprintf(fp,"\nchan_width_y:\n");
 fprintf(fp,"type: %d  peak: %f  width: %f  xpeak: %f  dc: %f\n\n",
   chan_y_dist.type, chan_y_dist.peak, chan_y_dist.width, 
     chan_y_dist.xpeak, chan_y_dist.dc);

 fprintf(fp,"Pin #\tclass\ttop\tbottom\tleft\tright");
 for (i=0;i<clb_size;i++) {
    fprintf(fp,"\n%d\t%d\t", i, clb_pin_class[i]);
    for (j=0;j<=3;j++) 
       fprintf(fp,"%d\t",pinloc[j][i]);
 }

 fprintf(fp,"\n\nClass types:  DRIVER = %d; RECEIVER = %d\n\n", DRIVER, 
    RECEIVER);

 fprintf(fp,"Class\tType\tNumpins\tPins");
 for (i=0;i<num_class;i++) {
    fprintf(fp,"\n%d\t%d\t%d\t", i, class_inf[i].type, class_inf[i].num_pins);
    for (j=0;j<class_inf[i].num_pins;j++) 
       fprintf(fp,"%d\t",class_inf[i].pinlist[j]);
 }
    
 fprintf(fp,"\n\n");

 fclose (fp);
}


void init_arch (float aspect_ratio, boolean user_sized) {
/* Allocates various data structures that depend on the FPGA         *
 * architecture.  Aspect_ratio specifies how many columns there are  *
 * relative to the number of rows -- i.e. width/height.  Used-sized  *
 * is TRUE if the user specified nx and ny already; in that case     *
 * use the user's values and don't recompute them.                   */

 int i, io_lim;
 void fill_arch (void);


/* User specified the dimensions on the command line.  Check if they *
 * will fit the circuit.                                             */

 if (user_sized == TRUE) {
    if (num_clbs > nx * ny || num_p_inputs + num_p_outputs > 
           2 * io_rat * (nx + ny)) {
       printf ("Error:  User-specified size is too small for circuit.\n"); 
       exit (1);
    }
 }

/* Size the FPGA automatically to be smallest that will fit circuit */

 else {
/* Area = nx * ny = ny * ny * aspect_ratio                  *
 * Perimeter = 2 * (nx + ny) = 2 * ny * (1. + aspect_ratio)  */

    ny = (int) ceil (sqrt ((double) (num_clbs / aspect_ratio)));
    io_lim = (int) ceil ((num_p_inputs + num_p_outputs) / (2 * io_rat *
           (1. + aspect_ratio)));
    ny = max (ny, io_lim);

    nx = (int) ceil (ny * aspect_ratio);
 }

/* If both nx and ny are 1, we only have one valid location for a clb. *
 * That's a major problem, as won't be able to move the clb and the    *
 * find_to routine that tries moves in the placer will go into an      *
 * infinite loop trying to move it.  Exit with an error message        *
 * instead.                                                            */

 if (nx == 1  && ny == 1 && num_clbs != 0) {
    printf ("Sorry, can't place a circuit with only one valid location\n");
    printf ("for a logic block (clb).\n");
    printf ("Try me with a more realistic circuit!\n");
    exit (1);
 }

 clb = (struct s_clb **) alloc_matrix (0, nx+1, 0, ny+1, 
              sizeof(struct s_clb));

/* clb = (struct s_clb **) my_malloc (sizeof(struct s_clb *)*(nx+2));

 for (i=0;i<=nx+1;i++) 
    clb[i] = (struct s_clb *) my_malloc (sizeof(struct s_clb)*(ny+2)); */

 chan_width_x = (int *) my_malloc ((ny+1) * sizeof(int));
 chan_width_y = (int *) my_malloc ((nx+1) * sizeof(int)); 

 bb_coords = (struct s_bb *) my_malloc (num_nets * sizeof(struct s_bb));
 bb_num_on_edges = (struct s_bb *) my_malloc (num_nets * sizeof(struct s_bb));

/* The two arrays below are used to precompute the inverse of the average *
 * number of tracks per channel between [subhigh] and [sublow].  Access   *
 * them as chan?_place_cost_fac[subhigh][sublow].  Since subhigh must be  *
 * greater than or equal to sublow, we only need to allocate storage for  *
 * the lower half of a matrix.                                            */

 chanx_place_cost_fac = (float **) my_malloc ((ny + 1) * sizeof (float *));
 for (i=0;i<=ny;i++)
    chanx_place_cost_fac[i] = (float *) my_malloc ((i + 1) * sizeof (float));

 chany_place_cost_fac = (float **) my_malloc ((nx + 1) * sizeof (float *));
 for (i=0;i<=nx;i++)
    chany_place_cost_fac[i] = (float *) my_malloc ((i + 1) * sizeof (float));

 fill_arch();
}


void fill_arch (void) {
/* Fill some of the FPGA architecture data structures.         */

 int i, j, *index;

/* allocate io_blocks arrays. Done this way to save storage */

 i = 2*io_rat*(nx+ny);  
 index = (int *) my_malloc (i*sizeof(int));
 for (i=1;i<=nx;i++) {
    clb[i][0].u.io_blocks = index;
    index+=io_rat;
    clb[i][ny+1].u.io_blocks = index;
    index+=io_rat;
 }
 for (i=1;i<=ny;i++) {
    clb[0][i].u.io_blocks = index;
    index+=io_rat;
    clb[nx+1][i].u.io_blocks = index;
    index+=io_rat;
 }
 
 /* Initialize type, and occupancy. */

 for (i=1;i<=nx;i++) {  
    clb[i][0].type = IO;
    clb[i][ny+1].type = IO;  /* perimeter (IO) cells */
 }

 for (i=1;i<=ny;i++) {
    clb[0][i].type = IO;
    clb[nx+1][i].type = IO;
 }

 for (i=1;i<=nx;i++) {   /* interior (LUT) cells */
    for (j=1;j<=ny;j++) {
       clb[i][j].type = CLB;
    }
 }

/* Nothing goes in the corners.      */

 clb[0][0].type = clb[nx+1][0].type = ILLEGAL;  
 clb[0][ny+1].type = clb[nx+1][ny+1].type = ILLEGAL;
}
