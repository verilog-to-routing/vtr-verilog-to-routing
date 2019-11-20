#include <stdio.h>
#include <string.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "graphics.h"
#include "read_netlist.h"
#include "print_netlist.h"
#include "read_arch.h"
#include "draw.h"
#include "place_and_route.h"
#include "stats.h"
#include "path_delay.h"


/******************** Global variables ************************************/

             /********** Netlist to be mapped stuff ****************/

int num_nets, num_blocks;
int num_p_inputs, num_p_outputs, num_clbs, num_globals;
struct s_net *net;
struct s_block *block;
boolean *is_global;         /* FALSE if a net is normal, TRUE if it is   */
                            /* global. Global signals are not routed.    */


             /********** Physical architecture stuff ****************/

int nx, ny, io_rat, pins_per_clb;

/* Pinloc[0..3][0..pins_per_clb-1].  For each pin pinloc[0..3][i] is 1 if    *
 * pin[i] exists on that side of the clb. See vpr_types.h for correspondence *
 * between the first index and the clb side.                                 */

int **pinloc;  

int *clb_pin_class;  /* clb_pin_class[0..pins_per_clb-1].  Gives the class  *
                      * number of each pin on a clb.                        */

/* TRUE if this is a global clb pin -- an input pin to which the netlist can *
 * connect global signals, but which does not connect into the normal        *
 * routing via muxes etc.  Marking pins like this (only clocks in my work)   *
 * stops them from screwing up the input switch pattern in the rr_graph      *
 * generator and from creating extra switches that the area model would      *
 * count.                                                                    */

boolean *is_global_clb_pin;     /* [0..pins_per_clb-1]. */

struct s_class *class_inf;   /* class_inf[0..num_class-1].  Provides   *
                              * information on all available classes.  */

int num_class;       /* Number of different classes.  */

int *chan_width_x, *chan_width_y;   /* [0..ny] and [0..nx] respectively  */

struct s_clb **clb;   /* Physical block list */


               /******** Structures defining the routing ***********/

/* [0..num_nets-1] of linked list start pointers.  Define the routing. */

struct s_trace **trace_head, **trace_tail;


           /**** Structures defining the FPGA routing architecture ****/

int num_rr_nodes;
t_rr_node *rr_node;                         /* [0..num_rr_nodes-1]  */

int num_rr_indexed_data;
t_rr_indexed_data *rr_indexed_data;         /* [0..num_rr_indexed_data-1] */

/* Gives the rr_node indices of net terminals.    */

int **net_rr_terminals;                  /* [0..num_nets-1][0..num_pins-1]. */

/* Gives information about all the switch types                      *
 * (part of routing architecture, but loaded in read_arch.c          */

struct s_switch_inf *switch_inf;      /* [0..det_routing_arch.num_switch-1] */

/* Stores the SOURCE and SINK nodes of all CLBs (not valid for pads).     */

int **rr_clb_source;          /* [0..num_blocks-1][0..num_class-1]*/


/********************** Subroutines local to this module ********************/


static void get_input (char *net_file, char *arch_file, int place_cost_type,
     int num_regions, float aspect_ratio, boolean user_sized,
     enum e_route_type route_type, struct s_det_routing_arch 
     *det_routing_arch, t_segment_inf **segment_inf_ptr, 
     t_timing_inf *timing_inf_ptr, t_subblock_data *subblock_data_ptr,
     t_chan_width_dist *chan_width_dist_ptr);

static void parse_command (int argc, char *argv[], char *net_file, char
    *arch_file, char *place_file, char *route_file, enum e_operation 
    *operation, float *aspect_ratio,  boolean *full_stats, boolean *user_sized, 
    boolean *verify_binary_search, int *gr_automode, boolean *show_graphics, 
    struct s_annealing_sched *annealing_sched, struct s_placer_opts 
    *placer_opts, struct s_router_opts *router_opts, boolean 
    *timing_analysis_enabled, float *constant_net_delay);

static int read_int_option (int argc, char *argv[], int iarg);
static float read_float_option (int argc, char *argv[], int iarg);


/************************* Subroutine definitions ***************************/


int main (int argc, char *argv[]) {

 char title[] = "\n\nVPR FPGA Placement and Routing Program Version 4.3\n"
                "Original VPR by V. Betz\n"
                "Timing-driven placement enhancements by A. Marquardt\n"
                "Source completed March 25, 2000; compiled " __DATE__ ".\n"
                "This code is licensed only for non-commercial use.\n\n";

 char net_file[BUFSIZE], place_file[BUFSIZE], arch_file[BUFSIZE];
 char route_file[BUFSIZE];
 float aspect_ratio;
 boolean full_stats, user_sized; 
 char pad_loc_file[BUFSIZE];
 enum e_operation operation;
 boolean verify_binary_search;
 boolean show_graphics;
 int gr_automode;
 struct s_annealing_sched annealing_sched; 
 struct s_placer_opts placer_opts;
 struct s_router_opts router_opts;
 struct s_det_routing_arch det_routing_arch;
 t_segment_inf *segment_inf;
 t_timing_inf timing_inf;
 t_subblock_data subblock_data;
 t_chan_width_dist chan_width_dist;
 float constant_net_delay;


 printf("%s",title);

 placer_opts.pad_loc_file = pad_loc_file;

/* Parse the command line. */

 parse_command (argc, argv, net_file, arch_file, place_file, route_file,
  &operation, &aspect_ratio,  &full_stats, &user_sized, &verify_binary_search,
  &gr_automode, &show_graphics, &annealing_sched, &placer_opts, &router_opts,
  &timing_inf.timing_analysis_enabled, &constant_net_delay);

/* Parse input circuit and architecture */

 get_input (net_file, arch_file, placer_opts.place_cost_type, 
        placer_opts.num_regions, aspect_ratio, user_sized, 
        router_opts.route_type, &det_routing_arch, &segment_inf,
        &timing_inf, &subblock_data, &chan_width_dist);

 if (full_stats == TRUE) 
    print_lambda ();

#ifdef DEBUG 
    print_netlist ("net.echo", net_file, subblock_data);
    print_arch (arch_file, router_opts.route_type, det_routing_arch,
         segment_inf, timing_inf, subblock_data, chan_width_dist);
#endif

 if (operation == TIMING_ANALYSIS_ONLY) {  /* Just run the timing analyzer. */
    do_constant_net_delay_timing_analysis (timing_inf, subblock_data,
                     constant_net_delay);
    free_subblock_data (&subblock_data);
    exit (0);
 }

 set_graphics_state (show_graphics, gr_automode, router_opts.route_type); 
 if (show_graphics) {
    /* Get graphics going */
    init_graphics("VPR:  Versatile Place and Route for FPGAs");  
    alloc_draw_structs ();
 }
   
 fflush (stdout);

 place_and_route (operation, placer_opts, place_file, net_file, arch_file,
    route_file, full_stats, verify_binary_search, annealing_sched, router_opts,
    det_routing_arch, segment_inf, timing_inf, &subblock_data, 
    chan_width_dist);

 if (show_graphics) 
    close_graphics();  /* Close down X Display */

 exit (0);
}


static void parse_command (int argc, char *argv[], char *net_file, char
			   *arch_file, char *place_file, char *route_file, enum e_operation 
			   *operation, float *aspect_ratio,  boolean *full_stats, boolean *user_sized,
			   boolean *verify_binary_search, int *gr_automode, boolean *show_graphics,
			   struct s_annealing_sched *annealing_sched, struct s_placer_opts
			   *placer_opts, struct s_router_opts *router_opts, boolean 
			   *timing_analysis_enabled, float *constant_net_delay) {

  /* Parse the command line to get the input and output files and options. */

  int i;
  int seed;
  boolean do_one_nonlinear_place, bend_cost_set, base_cost_type_set;

  /* Set the defaults.  If the user specified an option on the command *
 * line, the corresponding default is overwritten.                   */

  seed = 1;

  /* Flag to check if place_chan_width has been specified. */

  do_one_nonlinear_place = FALSE;
  bend_cost_set = FALSE;
  base_cost_type_set = FALSE;

  /* Allows me to see if nx and or ny have been set. */

  nx = 0;
  ny = 0;

  *operation = PLACE_AND_ROUTE;
  annealing_sched->type = AUTO_SCHED;
  annealing_sched->inner_num = 10.;
  annealing_sched->init_t = 100.;
  annealing_sched->alpha_t = 0.8;
  annealing_sched->exit_t = 0.01;

  placer_opts->place_algorithm = PATH_TIMING_DRIVEN_PLACE;
  placer_opts->timing_tradeoff = 0.5;
  placer_opts->block_dist = 1;
  placer_opts->place_cost_exp = 1.;
  placer_opts->place_cost_type = LINEAR_CONG;
  placer_opts->num_regions = 4;        /* Really 4 x 4 array */
  placer_opts->place_freq = PLACE_ONCE;
  placer_opts->place_chan_width = 100;   /* Reduces roundoff for lin. cong. */
  placer_opts->pad_loc_type = FREE;
  placer_opts->pad_loc_file[0] = '\0';
  placer_opts->recompute_crit_iter = 1; /*re-timing-analyze circuit at every temperature*/
  placer_opts->enable_timing_computations = FALSE;
  placer_opts->inner_loop_recompute_divider = 0; /*0 acts a a flag indicating that
						  *no inner loop recomputes be done*/
  placer_opts->td_place_exp_first = 1; /*exponentiation starts at 1 */
  placer_opts->td_place_exp_last = 8;   /*experimental results indicate 8 is good*/


/* Old values for breadth first router: first_iter_pres_fac = 0, *
 * pres_fac_mult = 2, acc_fac = 0.2.                             */

  router_opts->router_algorithm = TIMING_DRIVEN;
  router_opts->first_iter_pres_fac = 0.5;
  router_opts->initial_pres_fac = 0.5;
  router_opts->pres_fac_mult = 2;
  router_opts->acc_fac = 1;
  router_opts->base_cost_type = DEMAND_ONLY;
  router_opts->bend_cost = 1.;
  router_opts->max_router_iterations = 30;
  router_opts->bb_factor = 3;
  router_opts->route_type = DETAILED;
  router_opts->fixed_channel_width = NO_FIXED_CHANNEL_WIDTH;
  router_opts->astar_fac = 1.2;
  router_opts->max_criticality = 0.99;
  router_opts->criticality_exp = 1.;

  *timing_analysis_enabled = TRUE;
  *aspect_ratio = 1.;
  *full_stats = FALSE;
  *user_sized = FALSE;
  *verify_binary_search = FALSE;  
  *show_graphics = TRUE;
  *gr_automode = 1;     /* Wait for user input only after MAJOR updates. */
  *constant_net_delay = -1;

/* Start parsing the command line.  First four arguments are not   *
 * optional.                                                       */

  if (argc < 5) {
    printf("Usage:  vpr circuit.net fpga.arch placed.out routed.out "
	   "[Options ...]\n\n");
    printf("General Options:  [-nodisp] [-auto <int>] [-route_only]\n");
    printf("\t[-place_only] [-timing_analyze_only_with_net_delay <float>]\n");
    printf("\t[-aspect_ratio <float>] [-nx <int>] [-ny <int>] [-fast]\n");
    printf("\t[-full_stats] [-timing_analysis on | off]\n");

    printf("\nPlacer Options: \n");
    printf("\t[-place_algorithm bounding_box | net_timing_driven | path_timing_driven]\n");
    printf("\t[-init_t <float>] [-exit_t <float>]\n");
    printf("\t[-alpha_t <float>] [-inner_num <float>] [-seed <int>]\n");
    printf("\t[-place_cost_exp <float>] [-place_cost_type linear | "
	   "nonlinear]\n");
    printf("\t[-place_chan_width <int>] [-num_regions <int>] \n");
    printf("\t[-fix_pins random | <file.pads>]\n");
    printf("\t[-enable_timing_computations on | off]\n");
    printf("\t[-block_dist <int>]\n");

    printf("\nPlacement Options Valid Only for Timing-Driven Placement:\n");
    printf("\t[-timing_tradeoff <float>]\n");
    printf("\t[-recompute_crit_iter <int>]\n");
    printf("\t[-inner_loop_recompute_divider <int>]\n");
    printf("\t[-td_place_exp_first <float>]\n");
    printf("\t[-td_place_exp_last <float>]\n");

    printf("\nRouter Options:  [-max_router_iterations <int>] "
	   "[-bb_factor <int>]\n");
    printf("\t[-initial_pres_fac <float>] [-pres_fac_mult <float>]\n"
	   "\t[-acc_fac <float>] [-first_iter_pres_fac <float>]\n"
	   "\t[-bend_cost <float>] [-route_type global | detailed]\n"
	   "\t[-verify_binary_search] [-route_chan_width <int>]\n"
	   "\t[-router_algorithm breadth_first | timing_driven]\n"
	   "\t[-base_cost_type intrinsic_delay | delay_normalized | "
	   "demand_only]\n");

    printf ("\nRouting options valid only for timing-driven routing:\n"
	    "\t[-astar_fac <float>] [-max_criticality <float>]\n"
	    "\t[-criticality_exp <float>]\n\n");
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
	printf("Error:  Aspect ratio must be > 0.\n");
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

    if (strcmp(argv[i],"-recompute_crit_iter") == 0) {

      placer_opts->recompute_crit_iter = read_int_option (argc, argv, i);
      if (placer_opts->recompute_crit_iter < 1) {
	printf("Error: -recompute_crit_iter must be 1 or more \n");
	exit (1);
      }
      i += 2;
      continue;
    }

    if (strcmp(argv[i],"-inner_loop_recompute_divider") == 0) {

      placer_opts->inner_loop_recompute_divider = read_int_option (argc, argv, i);
      if (placer_opts->inner_loop_recompute_divider < 1) {
	printf("Error: -inner_loop_recompute_divider must be 1 or more \n");
	exit (1);
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


    if (strcmp(argv[i],"-fast") == 0) {
 
      /* Set all parameters for fast compile. */
 
      annealing_sched->inner_num = 1;
      router_opts->first_iter_pres_fac = 10000;
      router_opts->initial_pres_fac = 10000;
      router_opts->bb_factor = 0;
      router_opts->max_router_iterations = 10;
 
      i++;
      continue;
    }

    if (strcmp (argv[i],"-timing_analysis") == 0) {
      if (argc <= i+1) {
	printf ("Error:  -timing_analysis option requires a string "
		"parameter.\n");
	exit (1);
      } 
      if (strcmp(argv[i+1], "on") == 0) {
	*timing_analysis_enabled = TRUE;
      } 
      else if (strcmp(argv[i+1], "off") == 0) {
	*timing_analysis_enabled = FALSE;
      } 
      else {
	printf("Error:  -timing_analysis must be on or off.\n");
	exit (1);
      } 
 
      i += 2;
      continue;
    }

      
    if (strcmp(argv[i],"-timing_analyze_only_with_net_delay") == 0) {

      *operation = TIMING_ANALYSIS_ONLY;
      *constant_net_delay = read_float_option (argc, argv, i);

      if (*constant_net_delay < 0.) {
	printf("Error:  -timing_analyze_only_with_net_delay value must "
	       "be > 0.\n");
	exit(1);
      }

      i += 2;
      continue;
    }  


    if (strcmp(argv[i],"-init_t") == 0) {

      annealing_sched->init_t = read_float_option (argc, argv, i);

      if (annealing_sched->init_t < 0) {
	printf("Error:  -init_t value must be nonnegative 0.\n");
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

      if (annealing_sched->inner_num <= 0.) {
	printf("Error:  -inner_num value must be greater than 0.\n");
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



    if (strcmp(argv[i],"-td_place_exp_first") == 0) {
 
      placer_opts->td_place_exp_first = read_float_option (argc, argv, i);
 
      if (placer_opts->td_place_exp_first < 0.) {
	printf("Error:  -td_place_exp_first value must be nonnegative.\n");
	exit(1);
      }
 
      i += 2;
      continue;
    }



    if (strcmp(argv[i],"-td_place_exp_last") == 0) {
 
      placer_opts->td_place_exp_last = read_float_option (argc, argv, i);
 
      if (placer_opts->td_place_exp_last < 0.) {
	printf("Error:  -td_place_exp_last value must be nonnegative.\n");
	exit(1);
      }
 
      i += 2;
      continue;
    }



    if (strcmp(argv[i],"-place_algorithm") == 0) {

      if (argc <= i+1) {
	printf("Error:  -place_algorithm option requires a "
               "string parameter.\n");
	exit (1);
      }
           
      if (strcmp(argv[i+1], "bounding_box") == 0) {
	placer_opts->place_algorithm = BOUNDING_BOX_PLACE;
      }
      else if (strcmp(argv[i+1], "net_timing_driven") == 0) {
	placer_opts->place_algorithm = NET_TIMING_DRIVEN_PLACE;
      }
      else if (strcmp(argv[i+1], "path_timing_driven") == 0) {
	placer_opts->place_algorithm = PATH_TIMING_DRIVEN_PLACE;
      }
      else {
	printf("Error:  -place_algorithm must be bounding_box, "
	       "net_timing_driven, or path_timing_driven\n");
	exit (1);
      }

      i += 2;
      continue;

    }  

    if (strcmp(argv[i],"-timing_tradeoff") == 0) {
 
      placer_opts->timing_tradeoff = read_float_option (argc, argv, i);
 
      if (placer_opts->timing_tradeoff < 0.) {
	printf("Error:  -timing_tradeoff value must be nonnegative.\n");
	exit(1);
      }
 
      i += 2;
      continue;
    }



    if (strcmp(argv[i],"-enable_timing_computations") == 0) {
      if (argc <= i+1) {
	printf ("Error:  -enable_timing_computations option requires a string "
		"parameter.\n");
	exit (1);
      } 
      if (strcmp(argv[i+1], "on") == 0) {
	placer_opts->enable_timing_computations= TRUE;
      } 
      else if (strcmp(argv[i+1], "off") == 0) {
	placer_opts->enable_timing_computations = FALSE;
      } 
      else {
	printf("Error:  -enable_timing_computations must be on or off.\n");
	exit (1);
      } 
      i += 2;
      continue;
    }



    if (strcmp(argv[i],"-block_dist") == 0) {
 
      placer_opts->block_dist = read_int_option (argc, argv, i);
 
      if (placer_opts->block_dist < 0.) {
	printf("Error:  -block_dist value must be nonnegative.\n");
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


    if (strcmp(argv[i],"-router_algorithm") == 0) {

      if (argc <= i+1) {
	printf("Error:  -router_algorithm option requires a string "
	       "parameter.\n");
	exit (1);
      }
           
      if (strcmp(argv[i+1], "breadth_first") == 0) {
	router_opts->router_algorithm = BREADTH_FIRST;
      }
      else if (strcmp(argv[i+1], "timing_driven") == 0) {
	router_opts->router_algorithm = TIMING_DRIVEN;
      }
      else {
	printf("Error:  -router_algorithm must be breadth_first or "
	       "timing_driven.\n");
	exit (1);
      }

      i += 2;
      continue;
    }


    if (strcmp(argv[i],"-first_iter_pres_fac") == 0) {

      router_opts->first_iter_pres_fac = read_float_option (argc, argv, i);

      if (router_opts->first_iter_pres_fac < 0.) {
	printf("Error:  -first_iter_pres_fac must be nonnegative.\n");
	exit (1);
      }

      i += 2;
      continue;
    }  


    if (strcmp(argv[i],"-initial_pres_fac") == 0) { 

      router_opts->initial_pres_fac = read_float_option (argc, argv, i);

      if (router_opts->initial_pres_fac <= 0.) { 
	printf("Error:  -initial_pres_fac must be greater than 0.\n");
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


    if (strcmp(argv[i],"-astar_fac") == 0) { 

      router_opts->astar_fac = read_float_option (argc, argv, i);

      if (router_opts->astar_fac < 0.) { 
	printf("Error:  -astar_fac must be nonnegative.\n");
	exit (1); 
      }    

      i += 2;  
      continue; 
    }   


    if (strcmp(argv[i],"-max_criticality") == 0) {

      router_opts->max_criticality = read_float_option (argc, argv, i);

      if (router_opts->max_criticality < 0. || router_opts->max_criticality
	  >= 1.) {
	printf("Error:  -max_criticality must be greater than or equal\n"
	       "to 0 and less than 1.\n");
	exit (1);
      }

      i += 2;
      continue;
    }  


    if (strcmp(argv[i],"-criticality_exp") == 0) {

      router_opts->criticality_exp = read_float_option (argc, argv, i);

      if (router_opts->criticality_exp < 0.) {
	printf("Error:  -criticality_exp must be non-negative.\n");
	exit (1);
      }

      i += 2;
      continue;
    }  


    if (strcmp(argv[i],"-base_cost_type") == 0) {

      if (argc <= i+1) {
	printf("Error:  -base_cost_type option requires a string "
	       "parameter.\n");
	exit (1);
      }
           
      if (strcmp(argv[i+1], "intrinsic_delay") == 0) {
	router_opts->base_cost_type = INTRINSIC_DELAY;
      }
      else if (strcmp(argv[i+1], "delay_normalized") == 0) {
	router_opts->base_cost_type = DELAY_NORMALIZED;
      }
      else if (strcmp(argv[i+1], "demand_only") == 0) {
	router_opts->base_cost_type = DEMAND_ONLY;
      }
      else {
	printf("Error:  -base_cost_type must be intrinsic_delay, "
	       "delay_normalized or demand_only.\n");
	exit (1);
      }

      base_cost_type_set = TRUE;
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


    if (strcmp(argv[i],"-route_chan_width") == 0) {

      router_opts->fixed_channel_width = read_int_option (argc, argv, i);

      if (router_opts->fixed_channel_width <= 0) {
	printf("Error:  -route_chan_width value must be greater than 0.\n");
	exit(1);
      }
 
      if (router_opts->fixed_channel_width >= MAX_CHANNEL_WIDTH) {
	printf ("Error:  -router_chan_width value must be less than %d."
		"\n", MAX_CHANNEL_WIDTH);
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

    printf("Error:  Unrecognized flag: %s.  Aborting.\n",argv[i]);
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

  if (placer_opts->place_cost_type == NONLINEAR_CONG && 
      (placer_opts->place_algorithm == NET_TIMING_DRIVEN_PLACE ||
       placer_opts->place_algorithm == PATH_TIMING_DRIVEN_PLACE)) {

    /*note that this may work together, but I have not tested it */

    printf("Error: Cannot use nonlinear placement with \n"
	   "      timing driven placement\n");
    exit(1);
  }

  if (router_opts->route_type == GLOBAL &&
      (placer_opts->place_algorithm == NET_TIMING_DRIVEN_PLACE ||
       placer_opts->place_algorithm == PATH_TIMING_DRIVEN_PLACE)) {

    /* Works, but very weird.  Can't optimize timing well, since you're
     * not doing proper architecture delay modelling.
     */
    printf("Warning: Using global routing with timing-driven placement.\n");
    printf("\tThis is allowed, but strange, and circuit speed will suffer.\n");
  }
  
  if ( *timing_analysis_enabled == FALSE  &&
     (placer_opts->place_algorithm == NET_TIMING_DRIVEN_PLACE ||
      placer_opts->place_algorithm == PATH_TIMING_DRIVEN_PLACE)) { 

      /*may work, not tested*/
      printf("Error: timing analysis must be enabled for timing-driven placement\n");
      exit (1);
  }


  if (*operation == ROUTE_ONLY && placer_opts->pad_loc_type == USER) {
    printf ("Error:  You cannot specify both a full placement file and \n");
    printf ("        a pad location file.\n");
    exit (1);
  }

  if (router_opts->fixed_channel_width != NO_FIXED_CHANNEL_WIDTH &&
      *verify_binary_search) {
    printf ("Error:  Routing on a fixed channel width (%d tracks) "
	    "specified.\n", router_opts->fixed_channel_width);
    printf ("        There is no binary search to verify, so you \n"
	    "        cannot specify -verify_binary_search option.\n");
    exit (1);
  }

  if (*operation == ROUTE_ONLY || *operation == PLACE_AND_ROUTE) {
    if (router_opts->router_algorithm == TIMING_DRIVEN && 
	*timing_analysis_enabled == FALSE) {
      printf ("Error:  Cannot perform timing-driven routing when timing "
	      "analysis is disabled.\n");
      exit (1);
    }

    if (*timing_analysis_enabled == FALSE && router_opts->base_cost_type 
	!= DEMAND_ONLY) {
      printf ("Error:  base_cost_type must be demand_only when timing "
	      "analysis is disabled.\n");
      exit (1);
    }
  }

  if (*operation == TIMING_ANALYSIS_ONLY && *timing_analysis_enabled == 
      FALSE) {
    printf ("Error:  -timing_analyze_only_with_net_delay option requires\n"
	    "\tthat timing analysis not be disabled.\n");
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
    printf ("\tRouting will be performed on an existing placement.\n");
  } else if (*operation == PLACE_ONLY) {
    printf ("\tThe circuit will be placed but not routed.\n");
  } else if (*operation == PLACE_AND_ROUTE) {
    printf ("\tThe circuit will be placed and routed.\n");
  } else if (*operation == TIMING_ANALYSIS_ONLY) {
    printf ("\tOnly timing analysis (assuming a constant net delay) will "
	    "be performed.\n");
  }

  if (*operation == PLACE_ONLY || *operation == PLACE_AND_ROUTE) {
    printf("\nPlacer Options:\n");
    if (placer_opts->place_algorithm == NET_TIMING_DRIVEN_PLACE)
      printf("\tPlacement algorithm is net_timing_driven\n");
    if (placer_opts->place_algorithm == PATH_TIMING_DRIVEN_PLACE)
      printf("\tPlacement algorithm is path_timing_driven\n");
    if (placer_opts->place_algorithm == NET_TIMING_DRIVEN_PLACE ||
	placer_opts->place_algorithm == PATH_TIMING_DRIVEN_PLACE) {

      printf("\tTradeoff between bounding box and critical path: %g\n",placer_opts->timing_tradeoff);
      printf("\tLower bound assumes block distance: %d\n",placer_opts->block_dist);

      printf("\tRecomputing criticalities every %d temperature changes\n",
	     placer_opts->recompute_crit_iter);
      printf("\tInner loop computes criticalities every move_lim/%d moves\n",
	     placer_opts->inner_loop_recompute_divider);
      
      printf("\tExponent starting value used in timing-driven cost function is %g\n",
	     placer_opts->td_place_exp_first);
      printf("\tExponent final value used in timing-driven cost function is %g\n",
	     placer_opts->td_place_exp_last);
    }

    else if (placer_opts->place_algorithm == BOUNDING_BOX_PLACE) {
      printf("\tPlacement algorithm is bounding_box\n");
      if (placer_opts->enable_timing_computations) {
         printf("\tPlacement algorithm will generate timing estimates\n"
	    "\t(estimates do not affect the placement in bounding_box mode)\n");
      }
    }

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

    /* The default bend_cost for DETAILED routing is 0, while the default for *
     * GLOBAL routing is 1.                                                   */
    if (bend_cost_set == FALSE && router_opts->route_type == DETAILED) 
      router_opts->bend_cost = 0.; /*needed when computing  placement lookup matricies*/

    /* Default base_cost_type is DELAY_NORMALIZED for timing_driven routing */
    if (!base_cost_type_set && router_opts->router_algorithm == TIMING_DRIVEN)
      router_opts->base_cost_type = DELAY_NORMALIZED; /*needed when computing  placement lookup matricies*/

    printf("\tInitial random seed: %d\n", seed);
    my_srandom(seed);

  }   /* End of echo placement options. */


  if (*operation == ROUTE_ONLY || *operation == PLACE_AND_ROUTE) {
    printf("\nRouting Options:\n");

    if (router_opts->route_type == GLOBAL) 
      printf("\tOnly GLOBAL routing will be performed.\n");
    else 
      printf("\tCombined GLOBAL + DETAILED routing will be performed.\n");

    if (router_opts->router_algorithm == BREADTH_FIRST) 
      printf ("\tRouter algorithm:  breadth first.\n");
    else
      printf ("\tRouter algorithm:  timing driven.\n");

    printf("\tThe router will try at most %d iterations.\n",
	   router_opts->max_router_iterations);
    printf("\tRoutings can go at most %d channels outside their "
	   "bounding box.\n", router_opts->bb_factor);

    /* The default bend_cost for DETAILED routing is 0, while the default for *
     * GLOBAL routing is 1.                                                   */

    if (bend_cost_set == FALSE && router_opts->route_type == DETAILED) 
      router_opts->bend_cost = 0.;

    printf("\tCost of a bend (bend_cost) is %g.\n", router_opts->bend_cost);

    printf("\tFirst iteration sharing penalty factor (first_iter_pres_fac):"
	   " %g\n", router_opts->first_iter_pres_fac);
    printf("\tInitial (2nd iter.) sharing penalty factor (initial_pres_fac):"
	   " %g\n", router_opts->initial_pres_fac);
    printf("\tSharing penalty growth factor (pres_fac_mult): %g\n",
	   router_opts->pres_fac_mult);
    printf("\tAccumulated sharing penalty factor (acc_fac): %g\n", 
	   router_opts->acc_fac);

    /* Default base_cost_type is DELAY_NORMALIZED for timing_driven routing */

    if (!base_cost_type_set && router_opts->router_algorithm == TIMING_DRIVEN)
      router_opts->base_cost_type = DELAY_NORMALIZED;

    printf ("\tBase_cost_type:  ");
    if (router_opts->base_cost_type == INTRINSIC_DELAY) 
      printf ("intrinsic delay.\n");
    else if (router_opts->base_cost_type == DELAY_NORMALIZED) 
      printf ("delay normalized.\n");
    else if (router_opts->base_cost_type == DEMAND_ONLY) 
      printf ("demand only.\n");
      
    if (router_opts->router_algorithm == TIMING_DRIVEN) {
      printf ("\tSearch aggressiveness factor (astar_fac): %g\n", 
	      router_opts->astar_fac);
      printf ("\tMaximum sink criticality (max_criticality): %g\n",
	      router_opts->max_criticality);
      printf ("\tExponent for criticality computation (criticality_exp): "
	      "%g\n", router_opts->criticality_exp);
    }
       
    if (router_opts->fixed_channel_width == NO_FIXED_CHANNEL_WIDTH) {
      printf ("\tRouter will find the minimum number of tracks "
	      "required to route.\n");
    }  
    else {
      printf ("\tRouter will attempt routing only with a channel width"
	      " factor of %d.\n", router_opts->fixed_channel_width);
    }  

    if (*verify_binary_search) {

      /* Router will ensure routings with 1, 2, and 3 tracks fewer than the *
       * best found by the binary search will not succeed.  Normally only   *
       * verify that best - 1 tracks does not succeed.                      */

      printf("\tRouter will verify that binary search yields min. "
	     "channel width.\n");
    }
  }  /* End of echo router options. */

   
  if (*operation == TIMING_ANALYSIS_ONLY) {
    printf ("\tNet delay value for timing analysis: %g (s)\n", 
	    *constant_net_delay);
  }

  printf("\n");

}


static void get_input (char *net_file, char *arch_file, int place_cost_type,
		       int num_regions, float aspect_ratio, boolean user_sized,
		       enum e_route_type route_type, struct s_det_routing_arch 
		       *det_routing_arch, t_segment_inf **segment_inf_ptr,
		       t_timing_inf *timing_inf_ptr, t_subblock_data *subblock_data_ptr,
		       t_chan_width_dist *chan_width_dist_ptr) {

  /* This subroutine reads in the netlist and architecture files, initializes *
 * some data structures and does any error checks that require knowledge of *
 * both the algorithms to be used and the FPGA architecture.                */

  printf("Reading the FPGA architectural description from %s.\n", 
	 arch_file); 
  read_arch (arch_file, route_type, det_routing_arch, segment_inf_ptr,
	     timing_inf_ptr, subblock_data_ptr, chan_width_dist_ptr);
  printf("Successfully read %s.\n",arch_file);

  printf("Pins per clb: %d.  Pads per row/column: %d.\n", pins_per_clb, io_rat);
  printf("Subblocks per clb: %d.  Subblock LUT size: %d.\n", 
	 subblock_data_ptr->max_subblocks_per_block, 
	 subblock_data_ptr->subblock_lut_size);

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

    printf ("Distinct types of segments: %d.\n", 
            det_routing_arch->num_segment);
    printf ("Distinct types of user-specified switches: %d.\n", 
            det_routing_arch->num_switch - 2);
  }
  printf ("\n");


  printf("Reading the circuit netlist from %s.\n",net_file);
  read_net (net_file, subblock_data_ptr);
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
