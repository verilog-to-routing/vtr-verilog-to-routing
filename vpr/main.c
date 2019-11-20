#include <stdio.h>
#include <string.h>
#include "pr.h"
#include "util.h"
#include "graphics.h"
#include "ext.h"


/* Graphics stuff */

int showgraph = 1;  /* 1 => X graphics.  0 => no graphics */
int show_nets = 0;  /* Show nets of placement? */
int automode = 1;        /* Need user input after: 0: each t,   *
                          * 1: each place, 2: never             */
enum pic_type pic_on_screen;  /* What do I draw? */

/* Annealing schedule stuff */

int sched_type = AUTO_SCHED, inner_num = 10;
float init_t = 100., alpha_t = 0.8, exit_t = 0.01;

/* Netlist to be mapped stuff */

int num_nets, num_blocks;
int num_p_inputs, num_p_outputs, num_clbs, num_globals;
struct s_net *net;
struct s_block *block;
int *is_global;         /* 0 if a net is normal, 1 if it is global.   *
                         * Global signals are not routed.             */

int **net_pin_class;    /* [0..num_nets-1][0..num_pins]               *
                         * First subscript is net number, second is  *
                         * pin number on that net.  Value stored is   *
                         * the class of clb pin that pin of the net   *
                         * must connect to.                           */


/* Physical architecture stuff */

int nx, ny, io_rat, clb_size;
float chan_width_io;    
struct s_chan chan_x_dist, chan_y_dist;
int **pinloc;                /* Pinloc[0..3][0..clb_size-1].             *
                              * For each pin pinloc[0..3][i] is 1 if     *
                              * pin[i] exists on that side of the clb.   *
                              * See pr.h for correspondence between the  *
                              * first index and the clb side.            */

int *clb_pin_class;  /* clb_pin_class[0..clb_size-1].  Gives the class  *
                      * number of each pin on a clb.                    */

struct s_class *class_inf;   /* class_inf[0..num_class].  Provides     *
                              * information on all available classes.  */

int num_class;       /* Number of different classes.  */

int *chan_width_x, *chan_width_y;   /* [0..ny] and [0..nx] respectively  */

struct s_clb **clb;   /* Physical block list */

/* Store the bounding box and the number of blocks on each edge of the *
 * bounding box for efficient bounding box updates.                    */

struct s_bb *bb_coords, *bb_num_on_edges;  /* [0..inet-1] for both */

/* The arrays below are used to precompute the inverse of the average   *
 * number of tracks per channel between [subhigh] and [sublow].  Access *
 * them as chan?_place_cost_fac[subhigh][sublow].  They are used to     *
 * speed up the computation of the cost function that takes the length  *
 * of the net bounding box in each dimension, divided by the average    *
 * number of tracks in that direction; for other cost functions they    *
 * will never be used.                                                  */

float **chanx_place_cost_fac, **chany_place_cost_fac;



/* Structures used by router and graphics. */

struct s_phys_chan **chan_x, **chan_y;
   /* chan_x [1..nx][0..ny]  chan_y [0..nx][1..ny] */
struct s_trace **trace_head, **trace_tail;



/* Expected crossing counts for nets with different #'s of pins.  From *
 * DAC 94 pp. 690 - 695 (with linear interpolation applied by me).     */

const float cross_count[50] = {   /* [0..49] */
 1.0,    1.0,    1.0,    1.0828, 1.1536, 1.2206, 1.2823, 1.3385, 1.3991, 1.4493,
 1.4974, 1.5455, 1.5937, 1.6418, 1.6899, 1.7304, 1.7709, 1.8114, 1.8519, 1.8924,
 1.9288, 1.9652, 2.0015, 2.0379, 2.0743, 2.1061, 2.1379, 2.1698, 2.2016, 2.2334,
 2.2646, 2.2958, 2.3271, 2.3583, 2.3895, 2.4187, 2.4479, 2.4772, 2.5064, 2.5356,
 2.5610, 2.5864, 2.6117, 2.6371, 2.6625, 2.6887, 2.7148, 2.7410, 2.7671, 2.7933};

int main (int argc, char *argv[]) {

 char title[] = "\n\nVPR FPGA Placement and Routing Program Version 3.22 "
                "by V. Betz.\n"
                "Source completed June 10, 1996; "
                "compiled " __DATE__ ".\n\n";
 char net_file[BUFSIZE], place_file[BUFSIZE], arch_file[BUFSIZE];
 char route_file[BUFSIZE];
 float aspect_ratio;
 float place_cost_exp;
 int place_cost_type, num_regions, place_chan_width;
 boolean fixed_pins, full_stats, user_sized; 
 int operation, bb_factor, initial_cost_type, block_update_type;
 float initial_pres_fac, pres_fac_mult, acc_fac_mult, bend_cost;
 int max_block_update, max_immediate_update;
 enum pfreq place_freq;

 void get_input (char *net_file, char *arch_file, int place_cost_type,
    int num_regions, float aspect_ratio, boolean user_sized);
 void echo_input (char *foutput, char *net_file);
 void print_arch(char *arch_file);
 void parse_command (int argc, char *argv[], char *net_file, char
    *arch_file, char *place_file, char *route_file, int *operation,
    float *aspect_ratio, float *place_cost_exp, int *place_cost_type, 
    int *num_regions, int *bb_factor, int *initial_cost_type,
    float *initial_pres_fac, float *pres_fac_mult, float *acc_fac_mult,
    float *bend_cost, int *block_update_type, int *max_block_update,
    int *max_immediate_update, enum pfreq *place_freq, 
    int *place_chan_width, boolean *fixed_pins, boolean *full_stats,
    boolean *user_sized); 
 void alloc_draw_structs (void); 
 void place_and_route (int operation, float place_cost_exp, 
    int place_cost_type,  int num_regions, enum pfreq place_freq, 
    int place_chan_width, boolean fixed_pins, int bb_factor, 
    int initial_cost_type, float initial_pres_fac, float pres_fac_mult,
    float acc_fac_mult, float bend_cost, int block_update_type,
    int max_block_update, int max_immediate_update, char *place_file,
    char *net_file, char *arch_file, char *route_file, 
    boolean full_stats);
 void print_lambda (void);

 printf("%s",title);

/* Parse the command line. */

 parse_command(argc, argv, net_file, arch_file, place_file, route_file,
  &operation, &aspect_ratio, &place_cost_exp, &place_cost_type, 
  &num_regions, &bb_factor, &initial_cost_type, &initial_pres_fac,
  &pres_fac_mult, &acc_fac_mult, &bend_cost, &block_update_type,
  &max_block_update, &max_immediate_update, &place_freq,
  &place_chan_width, &fixed_pins, &full_stats, &user_sized);

/* Parse input circuit and architecture */

 get_input(net_file, arch_file, place_cost_type, num_regions, 
     aspect_ratio, user_sized);

 if (full_stats == TRUE) 
    print_lambda ();

#ifdef DEBUG 
    echo_input("net.echo", net_file);
    print_arch (arch_file);
#endif

 if (showgraph) {
    /* Get graphics going */
    init_graphics("VPR:  Versatile Place and Route for FPGAs");  
    alloc_draw_structs ();
 }
   
 fflush (stdout);

 place_and_route (operation, place_cost_exp, place_cost_type,
    num_regions, place_freq, place_chan_width, fixed_pins, bb_factor, 
    initial_cost_type, initial_pres_fac, pres_fac_mult, acc_fac_mult,
    bend_cost, block_update_type, max_block_update, max_immediate_update,
    place_file, net_file, arch_file, route_file, full_stats);

 if (showgraph) close_graphics();  /* Close down X Display */
 exit (0);
}


void parse_command (int argc, char *argv[], char *net_file, char 
   *arch_file, char *place_file, char *route_file, int *operation, 
   float *aspect_ratio, float *place_cost_exp, int *place_cost_type,
   int *num_regions, int *bb_factor, int *initial_cost_type,
   float *initial_pres_fac, float *pres_fac_mult, float *acc_fac_mult,
   float *bend_cost, int *block_update_type, int *max_block_update, 
   int *max_immediate_update, enum pfreq *place_freq, 
   int *place_chan_width, boolean *fixed_pins, boolean *full_stats,
   boolean *user_sized) {

/* Parse the command line to get the input and output files and options. */

   int i;
   int seed;
   boolean do_one_nonlinear_place;
   int read_int_option (int argc, char *argv[], int iarg);
   float read_float_option (int argc, char *argv[], int iarg);

/* Set the defaults.  If the user specified an option on the command *
 * line, the corresponding default is overwritten.                   */

   seed = 1;

/* Flag to check if place_chan_width has been specified. */

   do_one_nonlinear_place = FALSE;

/* Allows me to see if nx and or ny have been set. */

   nx = 0;
   ny = 0;

   *operation = PLACE_AND_ROUTE;
   *aspect_ratio = 1.;
   *place_cost_exp = 1.;
   *place_cost_type = LINEAR_CONG;
   *num_regions = 4;               /* Really 4 x 4 array */
   *max_block_update = 15;
   *max_immediate_update = 0;
   *bb_factor = 3;
   *initial_cost_type = NONE;
   *initial_pres_fac = 0.5;
   *pres_fac_mult = 1.5;
   *acc_fac_mult = 0.2;
   *bend_cost = 1.;
   *block_update_type = PATHFINDER;
   *place_freq = PLACE_ONCE;
   *place_chan_width = 100;        /* Reduces roundoff for linear cong. */
   *fixed_pins = FALSE;
   *full_stats = FALSE;
   *user_sized = FALSE;

/* Start parsing the command line.  First four arguments are not   *
 * optional.                                                       */

   if (argc < 5) {
     printf("Usage:  vpr circuit.net fpga.arch placed.out routed.out \n");
     printf("General Options:  [-nodisp] [-auto num] [-route_only]\n");
     printf("     [-place_only] [-route_only] [-aspect_ratio num]\n");
     printf("     [-nx num] [-ny num] [-full_stats]\n");
     printf("Placer Options:  [-init_t num] [-exit_t num]\n");
     printf("     [-alpha_t num] [-inner_num num] [-seed num]\n");
     printf("     [-place_cost_exp num]\n");
     printf("     [-place_cost_type linear|nonlinear]\n");
     printf("     [-place_chan_width num] [-num_regions num]\n");
     printf("     [-fixed_pins]\n");
     printf("Router Options:  [-max_block_update num]\n");
     printf("     [-max_immediate_update num] [-bb_factor num]\n");
     printf("     [-initial_cost_type div|sub|none] "
                    "[-initial_pres_fac num]\n");
     printf("     [-pres_fac_mult num] [-acc_fac_mult num]\n");
     printf("     [-bend_cost num] [-block_update_type block|mixed|"
                    "pathfinder]");
     printf("\n");
     exit(1);
   }

   strncpy(net_file,argv[1],BUFSIZE);
   strncpy(arch_file,argv[2],BUFSIZE);
   strncpy(place_file,argv[3],BUFSIZE);
   strncpy(route_file,argv[4],BUFSIZE);
   i = 5;

/* Now get any optional arguments.      */

   while (i < argc) {

      if (strcmp(argv[i],"-aspect_ratio") == 0) {
 
         *aspect_ratio = read_float_option (argc, argv, i);
 
         if (*aspect_ratio <= 0.) {
           printf("Error:  Aspect ratio must be >= 0.\n");
           exit(1);
         }
 
         i += 2;
         continue;
      }  

      if (strcmp(argv[i],"-nodisp") == 0) {
         showgraph = 0;
         i++;
         continue;
      }

      if (strcmp(argv[i],"-auto") == 0) {

         automode = read_int_option (argc, argv, i);

         if ((automode > 2) || (automode < 0)) {
           printf("Error:  -auto value must be between 0 and 2.\n");
           exit(1);
         }

         i += 2;
         continue;
      }

      if (strcmp(argv[i],"-fixed_pins") == 0) {
         *fixed_pins = TRUE;

         i += 1;
         continue;
      }  

      if (strcmp(argv[i],"-full_stats") == 0) {
         *full_stats = TRUE;
 
         i += 1;
         continue;
      }

      if (strcmp(argv[i],"-nx") == 0) {
 
         nx = read_int_option (argc, argv, i);
         *user_sized = TRUE;
 
         if (nx <= 0) {
           printf("Error:  -nx value must be greater than 0.\n");
           exit(1);
         }
 
         i += 2;
         continue;
      }  

      if (strcmp(argv[i],"-ny") == 0) { 
 
         ny = read_int_option (argc, argv, i); 
         *user_sized = TRUE;
 
         if (ny <= 0) { 
           printf("Error:  -ny value must be greater than 0.\n");
           exit(1); 
         } 
  
         i += 2; 
         continue;
      }   

      if (strcmp(argv[i],"-init_t") == 0) {

         init_t = read_float_option (argc, argv, i);

         if (init_t <= 0) {
           printf("Error:  -init_t value must be greater than 0.\n");
           exit(1);
         }

         sched_type = USER_SCHED;
         i += 2;
         continue;
      }

      if (strcmp(argv[i],"-alpha_t") == 0) {

         alpha_t = read_float_option (argc, argv, i);

         if ((alpha_t <= 0) || (alpha_t >= 1.)) {
           printf("Error:  -alpha_t value must be between 0. and 1.\n");
           exit(1);
         }

         sched_type = USER_SCHED;
         i += 2;
         continue;
      }

      if (strcmp(argv[i],"-exit_t") == 0) {

         exit_t = read_float_option (argc, argv, i);

         if (exit_t <= 0.) {
           printf("Error:  -exit_t value must be greater than 0.\n");
           exit(1);
         }

         sched_type = USER_SCHED;
         i += 2;
         continue;
      }

      if (strcmp(argv[i],"-inner_num") == 0) {

         inner_num = read_int_option (argc, argv, i);

         if (inner_num < 0) {
           printf("Error:  -inner_num value must be nonnegative.\n");
           exit(1);
         }

         i += 2;
         continue;
      }

      if (strcmp(argv[i],"-seed") == 0) {

         seed = read_int_option (argc, argv, i);

         i += 2;
         continue;
      }

      if (strcmp(argv[i],"-place_cost_exp") == 0) {
 
         *place_cost_exp = read_float_option (argc, argv, i);
 
         if (*place_cost_exp < 0.) {
           printf("Error:  -place_cost_exp value must be nonnegative.\n");
           exit(1);
         }
 
         i += 2;
         continue;
      }

      if (strcmp(argv[i],"-place_cost_type") == 0) {

        if (argc <= i+1) {
            printf("Error:  -place_cost_type option requires a "
               "string parameter.\n");
            exit (1);
         }
           
         if (strcmp(argv[i+1], "linear") == 0) {
            *place_cost_type = LINEAR_CONG;
         }
         else if (strcmp(argv[i+1], "nonlinear") == 0) {
            *place_cost_type = NONLINEAR_CONG;
         }
         else {
            printf("Error:  -place_cost_type must be linear or nonlinear."
               "\n");
            exit (1);
         }

         i += 2;
         continue;

      }  

      if (strcmp(argv[i],"-num_regions") == 0) {

         *num_regions = read_int_option (argc, argv, i);

         if (*num_regions <= 0.) {
           printf("Error:  -num_regions value must be greater than 0.\n");
           exit(1);
         }

         i += 2;
         continue;
      }  


      if (strcmp(argv[i],"-place_chan_width") == 0) {

         *place_chan_width = read_int_option (argc, argv, i);

         if (*place_chan_width <= 0.) {
           printf("Error:  -place_chan_width value must be greater than 0.\n");
           exit(1);
         }

         i += 2;
         do_one_nonlinear_place = TRUE;
         continue;
      }  


      if (strcmp(argv[i],"-max_block_update") == 0) {

         *max_block_update = read_int_option (argc, argv, i);

         if (*max_block_update < 0) {
            printf("Error:  -max_block_update value is less than 0.\n");
            exit(1);
         }

         i += 2;
         continue;
      }    

      if (strcmp(argv[i],"-max_immediate_update") == 0) { 

         *max_immediate_update = read_int_option (argc, argv, i);

         if (*max_immediate_update < 0) {
            printf("Error:  -max_immediate_update value is less than 0.\n");
            exit(1);
         }

         i += 2;
         continue;
      }    

      if (strcmp(argv[i],"-bb_factor") == 0) {

         *bb_factor = read_int_option (argc, argv, i);

         if (*bb_factor < 0) {
            printf("Error:  -bb_factor is less than 0.\n");
            exit (1);
         }

         i += 2;
         continue;
      }

      if (strcmp(argv[i],"-initial_cost_type") == 0) {
         if (argc <= i+1) {
            printf("Error:  -initial_cost_type option requires a "
               "string parameter.\n");
            exit (1);
         }

         if (strcmp(argv[i+1], "div") == 0) {
            *initial_cost_type = DIV;
         }
         else if (strcmp(argv[i+1], "sub") == 0) {
            *initial_cost_type = SUB;
         }
         else if (strcmp(argv[i+1], "none") == 0) {
            *initial_cost_type = NONE;
         }
         else {
            printf("Error:  -initial_cost_type must be div, sub or "
               "none.\n");
            exit (1);
         }

         i += 2;
         continue;
      }  

      if (strcmp(argv[i],"-initial_pres_fac") == 0) { 

         *initial_pres_fac = read_float_option (argc, argv, i);

         if (*initial_pres_fac <= 0.) { 
            printf("Error:  -initial_pres_fac must be greater than "
               "0.\n");
            exit (1); 
         } 

         i += 2; 
         continue; 
      }   

      if (strcmp(argv[i],"-pres_fac_mult") == 0) {

         *pres_fac_mult = read_float_option (argc, argv, i);

         if (*pres_fac_mult <= 0.) {
            printf("Error:  -pres_fac_mult must be greater than "
               "0.\n");
            exit (1);
         }  

         i += 2; 
         continue;
      }

      if (strcmp(argv[i],"-acc_fac_mult") == 0) { 

         *acc_fac_mult = read_float_option (argc, argv, i);

         if (*acc_fac_mult < 0.) { 
            printf("Error:  -acc_fac_mult must be nonnegative.\n");
            exit (1); 
         }    

         i += 2;  
         continue; 
      }   

      if (strcmp(argv[i],"-bend_cost") == 0) {
 
         *bend_cost = read_float_option (argc, argv, i);
 
         if (*bend_cost < 0.) {
            printf("Error:  -bend_cost cannot be less than 0.\n");
            exit (1);
         }
 
         i += 2;
         continue;
      }  

      if (strcmp(argv[i],"-block_update_type") == 0) {
         if (argc <= i+1) {
            printf("Error:  -block_update_type option requires a "
               "string parameter.\n");
            exit (1);
         }

         if (strcmp(argv[i+1], "block") == 0) {
            *block_update_type = BLOCK;
         }
         else if (strcmp(argv[i+1], "mixed") == 0) {
            *block_update_type = MIXED;
         }
         else if (strcmp(argv[i+1], "pathfinder") == 0) {
            *block_update_type = PATHFINDER;
         }
         else {
            printf("Error:  -block_update_type must be block, mixed or "
               "pathfinder.\n");
            exit (1);
         }

         i += 2;
         continue;
      }  


      if (strcmp(argv[i],"-route_only") == 0) {
         if (*operation == PLACE_ONLY) {
            printf("Error:  Both -route_only and -place_only specified.\n");
            exit (1);
         }
         *operation = ROUTE_ONLY;
         *place_freq = PLACE_NEVER;
         i++;
         continue;
      }

      if (strcmp(argv[i],"-place_only") == 0) {
         if (*operation == ROUTE_ONLY) {
            printf("Error:  Both -route_only and -place_only specified.\n");
            exit (1);
         }
         *operation = PLACE_ONLY;
         i++;
         continue;
      }

      printf("Unrecognized flag: %s.  Aborting.\n",argv[i]);
      exit(1);

   }   /* End of giant while loop. */


/* Check for illegal options combinations. */

   if (*place_cost_type == NONLINEAR_CONG && *operation !=
        PLACE_AND_ROUTE && do_one_nonlinear_place == FALSE) {
      printf("Error:  Replacing using the nonlinear congestion option\n");
      printf("        for each channel width makes sense only for full "
                      "place and route.\n");
      exit (1);
   }

/* Now echo back the options the user has selected. */

   printf("\nGeneral Options:\n");

   if (*aspect_ratio != 1.) {
      printf("\tFPGA will have a width/length ratio of %f.\n", 
          *aspect_ratio);
   }

   if (*user_sized == TRUE) {
      printf("\tThe FPGA size has been specified by the user.\n");

   /* If one of nx or ny was unspecified, compute it from the other and *
    * the aspect ratio.                                                 */

      if (ny == 0) {   
         ny = (float) nx / (float) *aspect_ratio;
      }
      else if (nx == 0) {
         nx = ny *  *aspect_ratio;
      }
      else if (*aspect_ratio != 1 && *aspect_ratio != nx / ny) {
            printf("\nError:  User-specified size and aspect ratio do\n");
            printf("not match.  Note that aspect ratio does not have to\n");
            printf("be specified if both nx and ny are specified.\n");
            exit (1);
      }
   }

   if (*full_stats == TRUE) {
      printf("\tA full statistical report will be printed.\n");
   }

   if (*operation == ROUTE_ONLY) {
      printf("\tRouting will be performed on an existing placement.\n");
   }

   else {
      if (*operation == PLACE_ONLY) {
         printf("\tThe circuit will be placed but not routed.\n");
      }
      printf("\nPlacer Options:\n");
      if (sched_type == AUTO_SCHED) {
         printf("\tAutomatic annealing schedule selected.\n");
         printf("\tNumber of moves in the inner loop is (num_blocks)^4/3 "
            "* %d\n", inner_num);
      }
      else {
         printf("\tUser annealing schedule selected with:\n");
         printf("\tInitial Temperature: %f\n",init_t);
         printf("\tExit (Final) Temperature: %f\n",exit_t);
         printf("\tTemperature Reduction factor (alpha_t): %f\n",alpha_t);
         printf("\tNumber of moves in the inner loop is (num_blocks)^4/3 * "
            "%d\n", inner_num);
      }
      
      if (*place_cost_type == NONLINEAR_CONG) {
         printf("\tPlacement cost type is nonlinear congestion.\n");
         printf("\tCongestion will be determined on a %d x %d array.\n",
           *num_regions, *num_regions);
         if (do_one_nonlinear_place == TRUE) {
            *place_freq = PLACE_ONCE;
            printf("\tPlacement will be performed once.\n");
            printf("\tPlacement channel width factor = %d.\n",
               *place_chan_width);
         }
         else {
            *place_freq = PLACE_ALWAYS;
            printf("\tCircuit will be replaced for each channel width "
                  "attempted.\n");
         }
      }

      else if (*place_cost_type == LINEAR_CONG) {
         *place_freq = PLACE_ONCE;
         printf("\tPlacement cost type is linear congestion.\n");
         printf("\tPlacement will be performed once.\n");
         printf("\tPlacement channel width factor = %d.\n",
             *place_chan_width);
         printf("\tExponent used in placement cost: %f\n",*place_cost_exp);
      }
   
      if (*fixed_pins == TRUE) {
         printf("\tPlacer will FIX the IO pins (no movement allowed)\n");
      }

      printf("\tInitial random seed: %d\n", seed);
      my_srandom(seed);
   }

   if (*operation != PLACE_ONLY) {
         printf("\nRouting Options:\n");
         printf("\tThe router will try %d block update iterations\n",
            *max_block_update);
         printf("\tand %d immediate update iterations.\n",
            *max_immediate_update);
         printf("\tRoutings can go at most %d channels outside their "
            "bounding box.\n", *bb_factor);
         printf("\tCost of a bend (bend_cost) is %f.\n",*bend_cost);

         if (*initial_cost_type == NONE) {
            printf("\tInitial routing based on expected congestion "
               "will NOT be performed.\n");
         }
         else if (*initial_cost_type == DIV) {
            printf("\tInitial routing will use the DIVIDE expected "
                  "congestion cost.\n");
         }
         else {
               printf("\tInitial routing will use the SUBTRACT expected "
                  "congestion cost.\n");
         }

         printf("\tInitial sharing penalty factor (initial_pres_fac): "
            "%f\n", *initial_pres_fac);
         printf("\tSharing penalty growth factor (pres_fac_mult): %f\n",
            *pres_fac_mult);
         printf("\tAccumulated sharing penalty factor (acc_fac_mult): "
            "%f\n", *acc_fac_mult);
       
         if (*max_block_update != 0) {  /* Block algorithm will be used */
            if (*block_update_type == BLOCK) {
               printf("\tBlock update iterations will update both channel\n"
                      "\t\tand pin costs in a block.\n\n");
            }
            else if (*block_update_type == MIXED) {
               printf("\tBlock update iterations will update channel costs "
                      "in a block\n"
                      "\t\t and pin costs immediately.\n\n");
            }
            else {
               printf("\tBlock update iterations will use true Pathfinder "
                      "algorithm.\n\n");
            }
         }
    }

}


void get_input (char *net_file, char *arch_file, int place_cost_type,
     int num_regions, float aspect_ratio, boolean user_sized) {

/* This subroutine reads in the netlist and architecture files, initializes *
 * some data structures and does any error checks that require knowledge of *
 * both the algorithms to be used and the FPGA architecture.                */

 void read_net (char *net_file);
 void read_arch (char *arch_file);
 void init_arch (float aspect_ratio, boolean user_sized);

 printf("Reading the FPGA architectural description from %s.\n", 
     arch_file); 
 read_arch (arch_file);
 printf("Successfully read %s.\n",arch_file);
 printf("FPGA consists of %d-pin CLBs and there are %d pads per CLB.\n\n",
    clb_size, io_rat);

 printf("Reading the circuit netlist from %s.\n",net_file);
 read_net (net_file);
 printf("Successfully read %s.\n", net_file);
 printf("%d blocks, %d nets, %d global nets.\n", num_blocks, num_nets, 
      num_globals);
 printf("%d clbs, %d inputs, %d outputs.\n", num_clbs, num_p_inputs,
      num_p_outputs);

/* Set up some physical FPGA data structures that need to   *
 *  know num_blocks.                                        */

 init_arch(aspect_ratio, user_sized);

 printf("The circuit will be mapped into a %d x %d array of LUTs.\n\n",
   nx, ny);

 if (place_cost_type == NONLINEAR_CONG && (num_regions > nx ||
         num_regions > ny)) {
    printf("Error:  Cannot use more regions than clbs in placement cost "
              "function.\n");
    exit(1);
 }
 
}

int read_int_option (int argc, char *argv[], int iarg) {
/* This routine returns the value in argv[iarg+1].  This value must exist *
 * and be an integer, or an error message is printed and the program      *
 * exits.                                                                 */

 int value, num_read;

 num_read = 0;

/* Does value exist for this option? */

 if (argc > iarg+1)           
    num_read = sscanf(argv[iarg+1],"%d",&value);

/* Value exists and was a proper int? */

 if (num_read != 1) {       
    printf("Error:  %s option requires an integer parameter.\n\n", argv[iarg]);
    exit(1);
 }

 return (value);
}


float read_float_option (int argc, char *argv[], int iarg) { 
/* This routine returns the value in argv[iarg+1].  This value must exist * 
 * and be a float, or an error message is printed and the program exits.  */ 
 
 int num_read; 
 float value;
 
 num_read = 0;   
 
/* Does value exist for this option? */
 
 if (argc > iarg+1)           
    num_read = sscanf(argv[iarg+1],"%f",&value);
 
/* Value exists and was a proper float? */
 
 if (num_read != 1) {       
    printf("Error:  %s option requires a float parameter.\n\n", argv[iarg]);
    exit(1);
 }
 
 return (value); 
} 
