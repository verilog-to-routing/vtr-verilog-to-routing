#include <stdio.h>
#include <string.h>
#include "util.h"
#include "pr.h"
#include "ext.h"
#include "graphics.h"
#include "read_netlist.h"
#include "read_arch.h"
#include "draw.h"
#include "place.h"
#include "stats.h"


/* Netlist to be mapped stuff */

int num_nets, num_blocks;
int num_p_inputs, num_p_outputs, num_clbs, num_globals;
struct s_net *net;
struct s_block *block;
int *is_global;         /* 0 if a net is normal, 1 if it is global.   *
                         * Global signals are not routed.             */

int **net_pin_class;    /* [0..num_nets-1][0..num_pins]               *
                         * First subscript is net number, second is   *
                         * pin number on that net.  Value stored is   *
                         * the class of clb pin that pin of the net   *
                         * must connect to.                           */

/* [0..num_blocks-1][0..num_subblocks_per_block[iblk]].  Contents of *
 * each logic block (cluster).  Not valid for IO blocks.             */
struct s_subblock **subblock_inf; 

/* [0..num_blocks-1].  Number of subblocks in each block.  0 for IOs, *
 * from 1 to max_subblocks_per_block for each logic block.            */
int *num_subblocks_per_block;


/* Physical architecture stuff */

int nx, ny, io_rat, pins_per_clb;
float chan_width_io;    
struct s_chan chan_x_dist, chan_y_dist;
int **pinloc;                /* Pinloc[0..3][0..pins_per_clb-1].             *
                              * For each pin pinloc[0..3][i] is 1 if     *
                              * pin[i] exists on that side of the clb.   *
                              * See pr.h for correspondence between the  *
                              * first index and the clb side.            */

int *clb_pin_class;  /* clb_pin_class[0..pins_per_clb-1].  Gives the class  *
                      * number of each pin on a clb.                    */

struct s_class *class_inf;   /* class_inf[0..num_class-1].  Provides   *
                              * information on all available classes.  */

int num_class;       /* Number of different classes.  */

int *chan_width_x, *chan_width_y;   /* [0..ny] and [0..nx] respectively  */

struct s_clb **clb;   /* Physical block list */

int max_subblocks_per_block;       /* Maximum cluster size */
int subblock_lut_size;             /* How many inputs in subblock LUTs? */


/* [0..num_nets-1] of linked list start pointers.  Define the routing. */
struct s_trace **trace_head, **trace_tail;

/* Structures below define the routing architecture of the FPGA. */
int num_rr_nodes;
struct s_rr_node *rr_node;                    /* [0..num_rr_nodes-1] */
struct s_rr_node_cost_inf *rr_node_cost_inf;  /* [0..num_rr_nodes-1] */
struct s_rr_node_draw_inf *rr_node_draw_inf;  /* [0..num_rr_nodes-1] */

/* Gives the rr_node indices of net terminals.    */
int **net_rr_terminals;                  /* [0..num_nets-1][0..num_pins-1]. */



static void get_input (char *net_file, char *arch_file, int place_cost_type,
     int num_regions, float aspect_ratio, boolean user_sized,
     enum e_route_type route_type, struct s_det_routing_arch 
     *det_routing_arch); 

static void parse_command (int argc, char *argv[], char *net_file, char
    *arch_file, char *place_file, char *route_file, int *operation,
    float *aspect_ratio,  boolean *full_stats, boolean *user_sized, 
    boolean *verify_binary_search, int *gr_automode, boolean *show_graphics, 
    struct s_annealing_sched *annealing_sched, struct s_placer_opts 
    *placer_opts, struct s_router_opts *router_opts);

static int read_int_option (int argc, char *argv[], int iarg);
static float read_float_option (int argc, char *argv[], int iarg);



int main (int argc, char *argv[]) {

 char title[] = "\n\nVPR FPGA Placement and Routing Program Version 3.99 "
                "by V. Betz.\n"
                "Source completed March 18, 1997; "
                "compiled " __DATE__ ".\n\n";
 char net_file[BUFSIZE], place_file[BUFSIZE], arch_file[BUFSIZE];
 char route_file[BUFSIZE];
 float aspect_ratio;
 boolean full_stats, user_sized; 
 char pad_loc_file[BUFSIZE];
 int operation;
 boolean verify_binary_search;
 boolean show_graphics;
 int gr_automode;
 struct s_annealing_sched annealing_sched; 
 struct s_placer_opts placer_opts;
 struct s_router_opts router_opts;
 struct s_det_routing_arch det_routing_arch;

 printf("%s",title);

 placer_opts.pad_loc_file = pad_loc_file;

/* Parse the command line. */

 parse_command(argc, argv, net_file, arch_file, place_file, route_file,
  &operation, &aspect_ratio,  &full_stats, &user_sized, &verify_binary_search,
  &gr_automode, &show_graphics, &annealing_sched, &placer_opts, &router_opts);

/* Parse input circuit and architecture */

 get_input(net_file, arch_file, placer_opts.place_cost_type, 
        placer_opts.num_regions, aspect_ratio, user_sized, 
        router_opts.route_type, &det_routing_arch);

 if (full_stats == TRUE) 
    print_lambda ();

#ifdef DEBUG 
    netlist_echo("net.echo", net_file);
    print_arch (arch_file, router_opts.route_type, det_routing_arch);
#endif

 set_graphics_state (show_graphics, gr_automode, router_opts.route_type); 
 if (show_graphics) {
    /* Get graphics going */
    init_graphics("VPR:  Versatile Place and Route for FPGAs");  
    alloc_draw_structs ();
 }
   
 fflush (stdout);

 place_and_route (operation, placer_opts, place_file, net_file, arch_file,
    route_file, full_stats, verify_binary_search, annealing_sched, router_opts,
    det_routing_arch);

 if (show_graphics) 
    close_graphics();  /* Close down X Display */

 exit (0);
}


static void parse_command (int argc, char *argv[], char *net_file, char
    *arch_file, char *place_file, char *route_file, int *operation,
    float *aspect_ratio,  boolean *full_stats, boolean *user_sized,
    boolean *verify_binary_search, int *gr_automode, boolean *show_graphics,
    struct s_annealing_sched *annealing_sched, struct s_placer_opts
    *placer_opts, struct s_router_opts *router_opts) {

/* Parse the command line to get the input and output files and options. */

   int i;
   int seed;
   boolean do_one_nonlinear_place, bend_cost_set;

/* Set the defaults.  If the user specified an option on the command *
 * line, the corresponding default is overwritten.                   */

   seed = 1;

/* Flag to check if place_chan_width has been specified. */

   do_one_nonlinear_place = FALSE;
   bend_cost_set = FALSE;

/* Allows me to see if nx and or ny have been set. */

   nx = 0;
   ny = 0;

   *operation = PLACE_AND_ROUTE;
   annealing_sched->type = AUTO_SCHED;
   annealing_sched->inner_num = 10.;
   annealing_sched->init_t = 100.;
   annealing_sched->alpha_t = 0.8;
   annealing_sched->exit_t = 0.01;

   placer_opts->place_cost_exp = 1.;
   placer_opts->place_cost_type = LINEAR_CONG;
   placer_opts->num_regions = 4;        /* Really 4 x 4 array */
   placer_opts->place_freq = PLACE_ONCE;
   placer_opts->place_chan_width = 100;   /* Reduces roundoff for lin. cong. */
   placer_opts->pad_loc_type = FREE;
   placer_opts->pad_loc_file[0] = '\0';

   router_opts->initial_pres_fac = 0.5;
   router_opts->pres_fac_mult = 1.5;
   router_opts->acc_fac = 0.2;
   router_opts->bend_cost = 1.;
   router_opts->max_router_iterations = 30;
   router_opts->bb_factor = 3;
   router_opts->route_type = DETAILED;

   *aspect_ratio = 1.;
   *full_stats = FALSE;
   *user_sized = FALSE;
   *verify_binary_search = FALSE;  
   *show_graphics = TRUE;
   *gr_automode = 1;     /* Wait for user input only after MAJOR updates. */

/* Start parsing the command line.  First four arguments are not   *
 * optional.                                                       */

   if (argc < 5) {
     printf("Usage:  vpr circuit.net fpga.arch placed.out routed.out "
             "[Options ...]\n\n");
     printf("General Options:  [-nodisp] [-auto <int>] [-route_only]\n");
     printf("\t[-place_only] [-aspect_ratio <float>]\n");
     printf("\t[-nx <int>] [-ny <int>] [-full_stats]\n");
     printf("\nPlacer Options:  [-init_t <float>] [-exit_t <float>]\n");
     printf("\t[-alpha_t <float>] [-inner_num <float>] [-seed <int>]\n");
     printf("\t[-place_cost_exp <float>] [-place_cost_type linear|nonlinear]"
                 "\n");
     printf("\t[-place_chan_width <int>] [-num_regions <int>] \n");
     printf("\t[-fix_pins random|<file.pads>]\n");
     printf("\nRouter Options:  [-max_router_iterations <int>] "
                 "[-bb_factor <int>]\n");
     printf("\t[-initial_pres_fac <float>] [-pres_fac_mult <float>] "
                  "[-acc_fac <float>]\n");
     printf("\t[-bend_cost <float>] [-route_type global|detailed]\n");
     printf("\t[-verify_binary_search]\n");
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
         *show_graphics = FALSE;
         i++;
         continue;
      }

      if (strcmp(argv[i],"-auto") == 0) {

         *gr_automode = read_int_option (argc, argv, i);

         if ((*gr_automode > 2) || (*gr_automode < 0)) {
           printf("Error:  -auto value must be between 0 and 2.\n");
           exit(1);
         }

         i += 2;
         continue;
      }

      if (strcmp(argv[i],"-fix_pins") == 0) {
 
        if (argc <= i+1) {
            printf("Error:  -fix_pins option requires a string parameter.\n");
            exit (1);
         }
 
         if (strcmp(argv[i+1], "random") == 0) {
            placer_opts->pad_loc_type = RANDOM;
         }
         else {
            placer_opts->pad_loc_type = USER;
            strncpy (placer_opts->pad_loc_file, argv[i+1], BUFSIZE);
         }
 
         i += 2;
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

         annealing_sched->init_t = read_float_option (argc, argv, i);

         if (annealing_sched->init_t <= 0) {
           printf("Error:  -init_t value must be greater than 0.\n");
           exit(1);
         }

         annealing_sched->type = USER_SCHED;
         i += 2;
         continue;
      }

      if (strcmp(argv[i],"-alpha_t") == 0) {

         annealing_sched->alpha_t = read_float_option (argc, argv, i);

         if ((annealing_sched->alpha_t <= 0) || 
                    (annealing_sched->alpha_t >= 1.)) {
           printf("Error:  -alpha_t value must be between 0. and 1.\n");
           exit(1);
         }

         annealing_sched->type = USER_SCHED;
         i += 2;
         continue;
      }

      if (strcmp(argv[i],"-exit_t") == 0) {

         annealing_sched->exit_t = read_float_option (argc, argv, i);

         if (annealing_sched->exit_t <= 0.) {
           printf("Error:  -exit_t value must be greater than 0.\n");
           exit(1);
         }

         annealing_sched->type = USER_SCHED;
         i += 2;
         continue;
      }

      if (strcmp(argv[i],"-inner_num") == 0) {

         annealing_sched->inner_num = read_float_option (argc, argv, i);

         if (annealing_sched->inner_num < 0) {
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
 
         placer_opts->place_cost_exp = read_float_option (argc, argv, i);
 
         if (placer_opts->place_cost_exp < 0.) {
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
            placer_opts->place_cost_type = LINEAR_CONG;
         }
         else if (strcmp(argv[i+1], "nonlinear") == 0) {
            placer_opts->place_cost_type = NONLINEAR_CONG;
         }
         else {
            printf("Error:  -place_cost_type must be linear or nonlinear.\n");
            exit (1);
         }

         i += 2;
         continue;

      }  

      if (strcmp(argv[i],"-num_regions") == 0) {

         placer_opts->num_regions = read_int_option (argc, argv, i);

         if (placer_opts->num_regions <= 0.) {
           printf("Error:  -num_regions value must be greater than 0.\n");
           exit(1);
         }

         i += 2;
         continue;
      }  


      if (strcmp(argv[i],"-place_chan_width") == 0) {

         placer_opts->place_chan_width = read_int_option (argc, argv, i);

         if (placer_opts->place_chan_width <= 0.) {
           printf("Error:  -place_chan_width value must be greater than 0.\n");
           exit(1);
         }

         i += 2;
         do_one_nonlinear_place = TRUE;
         continue;
      }  

      if (strcmp(argv[i],"-max_router_iterations") == 0) { 

         router_opts->max_router_iterations = read_int_option (argc, argv, i);

         if (router_opts->max_router_iterations < 0) {
            printf("Error:  -max_router_iterations value is less than 0.\n");
            exit(1);
         }

         i += 2;
         continue;
      }    

      if (strcmp(argv[i],"-bb_factor") == 0) {

         router_opts->bb_factor = read_int_option (argc, argv, i);

         if (router_opts->bb_factor < 0) {
            printf("Error:  -bb_factor is less than 0.\n");
            exit (1);
         }

         i += 2;
         continue;
      }

      if (strcmp(argv[i],"-initial_pres_fac") == 0) { 

         router_opts->initial_pres_fac = read_float_option (argc, argv, i);

         if (router_opts->initial_pres_fac <= 0.) { 
            printf("Error:  -initial_pres_fac must be greater than "
               "0.\n");
            exit (1); 
         } 

         i += 2; 
         continue; 
      }   

      if (strcmp(argv[i],"-pres_fac_mult") == 0) {

         router_opts->pres_fac_mult = read_float_option (argc, argv, i);

         if (router_opts->pres_fac_mult <= 0.) {
            printf("Error:  -pres_fac_mult must be greater than "
               "0.\n");
            exit (1);
         }  

         i += 2; 
         continue;
      }

      if (strcmp(argv[i],"-acc_fac") == 0) { 

         router_opts->acc_fac = read_float_option (argc, argv, i);

         if (router_opts->acc_fac < 0.) { 
            printf("Error:  -acc_fac must be nonnegative.\n");
            exit (1); 
         }    

         i += 2;  
         continue; 
      }   

      if (strcmp(argv[i],"-bend_cost") == 0) {
 
         router_opts->bend_cost = read_float_option (argc, argv, i);
 
         if (router_opts->bend_cost < 0.) {
            printf("Error:  -bend_cost cannot be less than 0.\n");
            exit (1);
         }
 
         bend_cost_set = TRUE;
         i += 2;
         continue;
      }  


      if (strcmp(argv[i],"-route_type") == 0) {

        if (argc <= i+1) {
            printf("Error:  -route_type option requires a "
               "string parameter.\n");
            exit (1);
         }

         if (strcmp(argv[i+1], "global") == 0) {
            router_opts->route_type = GLOBAL;
         }
         else if (strcmp(argv[i+1], "detailed") == 0) {
            router_opts->route_type = DETAILED;
         }
         else {
            printf("Error:  -route_type must be global or detailed.\n");
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
         placer_opts->place_freq = PLACE_NEVER;
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


      if (strcmp(argv[i],"-verify_binary_search") == 0) {
         *verify_binary_search = TRUE;
         i++;
         continue;
      }

      printf("Unrecognized flag: %s.  Aborting.\n",argv[i]);
      exit(1);

   }   /* End of giant while loop. */


/* Check for illegal options combinations. */

   if (placer_opts->place_cost_type == NONLINEAR_CONG && *operation !=
        PLACE_AND_ROUTE && do_one_nonlinear_place == FALSE) {
      printf("Error:  Replacing using the nonlinear congestion option\n");
      printf("        for each channel width makes sense only for full "
                      "place and route.\n");
      exit (1);
   }

   if (*operation == ROUTE_ONLY && placer_opts->pad_loc_type == USER) {
      printf ("Error:  You cannot specify both a full placement file and \n");
      printf ("        a pad location file.\n");
      exit (1);
   }

/* Now echo back the options the user has selected. */

   printf("\nGeneral Options:\n");

   if (*aspect_ratio != 1.) {
      printf("\tFPGA will have a width/length ratio of %g.\n", 
          *aspect_ratio);
   }

   if (*user_sized == TRUE) {
      printf("\tThe FPGA size has been specified by the user.\n");

   /* If one of nx or ny was unspecified, compute it from the other and  *
    * the aspect ratio.  If both are unspecified, wait till the netlist  *
    * is read and compute the smallest possible nx and ny in read_arch.  */

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
      if (annealing_sched->type == AUTO_SCHED) {
         printf("\tAutomatic annealing schedule selected.\n");
         printf("\tNumber of moves in the inner loop is (num_blocks)^4/3 "
            "* %g\n", annealing_sched->inner_num);
      }
      else {
         printf("\tUser annealing schedule selected with:\n");
         printf("\tInitial Temperature: %g\n",annealing_sched->init_t);
         printf("\tExit (Final) Temperature: %g\n",annealing_sched->exit_t);
         printf("\tTemperature Reduction factor (alpha_t): %g\n",
                annealing_sched->alpha_t);
         printf("\tNumber of moves in the inner loop is (num_blocks)^4/3 * "
            "%g\n", annealing_sched->inner_num);
      }
      
      if (placer_opts->place_cost_type == NONLINEAR_CONG) {
         printf("\tPlacement cost type is nonlinear congestion.\n");
         printf("\tCongestion will be determined on a %d x %d array.\n",
               placer_opts->num_regions, placer_opts->num_regions);
         if (do_one_nonlinear_place == TRUE) {
            placer_opts->place_freq = PLACE_ONCE;
            printf("\tPlacement will be performed once.\n");
            printf("\tPlacement channel width factor = %d.\n",
                 placer_opts->place_chan_width);
         }
         else {
            placer_opts->place_freq = PLACE_ALWAYS;
            printf("\tCircuit will be replaced for each channel width "
                  "attempted.\n");
         }
      }

      else if (placer_opts->place_cost_type == LINEAR_CONG) {
         placer_opts->place_freq = PLACE_ONCE;
         printf("\tPlacement cost type is linear congestion.\n");
         printf("\tPlacement will be performed once.\n");
         printf("\tPlacement channel width factor = %d.\n",
                placer_opts->place_chan_width);
         printf("\tExponent used in placement cost: %g\n",
                placer_opts->place_cost_exp);
      }
   
      if (placer_opts->pad_loc_type == RANDOM) {
         printf("\tPlacer will fix the IO pins in a random configuration.\n");
      }
      else if (placer_opts->pad_loc_type == USER) {
         printf ("\tPlacer will fix the IO pins as specified by file %s.\n",
                  placer_opts->pad_loc_file);
      }

      printf("\tInitial random seed: %d\n", seed);
      my_srandom(seed);
   }

   if (*operation != PLACE_ONLY) {
      printf("\nRouting Options:\n");

      if (router_opts->route_type == GLOBAL) 
         printf("\tOnly GLOBAL routing will be performed.\n");
      else 
         printf("\tCombined GLOBAL + DETAILED routing will be performed.\n");

      printf("\tThe router will try at most %d iterations.\n",
            router_opts->max_router_iterations);
      printf("\tRoutings can go at most %d channels outside their "
            "bounding box.\n", router_opts->bb_factor);

/* The default bend_cost for DETAILED routing is 0, while the default for *
 * GLOBAL routing is 1.                                                   */

      if (bend_cost_set == FALSE && router_opts->route_type == DETAILED) 
         router_opts->bend_cost = 0.;

      printf("\tCost of a bend (bend_cost) is %g.\n", router_opts->bend_cost);

      printf("\tInitial sharing penalty factor (initial_pres_fac): %g\n", 
            router_opts->initial_pres_fac);
      printf("\tSharing penalty growth factor (pres_fac_mult): %g\n",
            router_opts->pres_fac_mult);
      printf("\tAccumulated sharing penalty factor (acc_fac): %g\n", 
            router_opts->acc_fac);
       
      if (*verify_binary_search) {
/* Router will ensure routings with 1, 2, and 3 tracks fewer than the *
 * best found by the binary search will not succeed.  Normally only   *
 * verify that best - 1 tracks does not succeed.                      */
         printf("\tRouter will verify that binary search yields min. "
                  "channel width.\n");
      }
   }

   printf("\n");

}


static void get_input (char *net_file, char *arch_file, int place_cost_type,
     int num_regions, float aspect_ratio, boolean user_sized,
     enum e_route_type route_type, struct s_det_routing_arch 
     *det_routing_arch) {

/* This subroutine reads in the netlist and architecture files, initializes *
 * some data structures and does any error checks that require knowledge of *
 * both the algorithms to be used and the FPGA architecture.                */

 printf("Reading the FPGA architectural description from %s.\n", 
     arch_file); 
 read_arch (arch_file, route_type, det_routing_arch);
 printf("Successfully read %s.\n",arch_file);

 printf("Pins per clb: %d.  Pads per row/column: %d.\n", pins_per_clb, io_rat);
 printf("Subblocks per clb: %d.  Subblock LUT size: %d.\n", 
      max_subblocks_per_block, subblock_lut_size);

 if (route_type == DETAILED) {
    if (det_routing_arch->Fc_type == ABSOLUTE) 
       printf("Fc value is absolute number of tracks.\n"); 
    else 
       printf("Fc value is fraction of tracks in a channel.\n"); 
    
    printf("Fc_output: %g.  Fc_input: %g.  Fc_pad: %g.\n", 
           det_routing_arch->Fc_output, det_routing_arch->Fc_input,
           det_routing_arch->Fc_pad);

    if (det_routing_arch->switch_block_type == SUBSET)
       printf("Switch block type: Subset.\n");
    else if (det_routing_arch->switch_block_type == WILTON) 
       printf("Switch_block_type: WILTON.\n");
    else 
       printf ("Switch_block_type: UNIVERSAL.\n"); 
 }
 printf ("\n");


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

 printf("The circuit will be mapped into a %d x %d array of clbs.\n\n",
   nx, ny);

 if (place_cost_type == NONLINEAR_CONG && (num_regions > nx ||
         num_regions > ny)) {
    printf("Error:  Cannot use more regions than clbs in placement cost "
              "function.\n");
    exit(1);
 }
 
}


static int read_int_option (int argc, char *argv[], int iarg) {

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


static float read_float_option (int argc, char *argv[], int iarg) { 

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
