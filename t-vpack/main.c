#include <stdio.h>
#include <string.h>
#include "util.h"
#include "vpack.h"
#include "globals.h"
#include "read_blif.h"
#include "ff_pack.h"
#include "cluster.h"
#include "output_clustering.h"

#define BIG_INT 32000

int num_nets, num_blocks;
int num_p_inputs, num_p_outputs, num_luts, num_latches;
struct s_net *net;
struct s_block *block;

static void parse_command (int argc, char *argv[], char *blif_file, 
   char *output_file, boolean *global_clocks, int *cluster_size,
   int *inputs_per_cluster, int *clocks_per_cluster, int *lut_size,
   boolean *muxes_to_cluster_output_pins,
   boolean *hill_climbing_flag, boolean *timing_driven,
   enum e_cluster_seed *cluster_seed_type, float *alpha, 
   int *recompute_timing_percent, float *block_delay,
   float *intra_cluster_net_delay, float *inter_cluster_net_delay,
   boolean *skip_clustering, boolean *allow_unrelated_clustering,
   boolean *allow_early_exit, boolean *connection_driven);
static int read_int_option (int argc, char *argv[], int iarg);
static float read_float_option (int argc, char *argv[], int iarg);


/* Vpack was written by Vaughn Betz at the University of Toronto.   *
 * It is an extension / replacement for the older blifmap program   *
 * used with earlier versions of VPR.                               *
 * Contact vaughn@eecg.utoronto.ca if you wish to use vpack in your *
 * research or if you have questions about its use.                 *
 * All timing enhancements were written by Alexander (Sandy)        *
 * Marquardt at the University of Toronto. Contact                  *
 * arm@eecg.toronto.edu if you have any questions about these       *
 * enhancements                                                     */

static void unclustered_stats (int lut_size) {

/* Dumps out statistics on an unclustered netlist -- i.e. it is just *
 * packed into LUT + FF logic blocks, but not local routing from     *
 * output to input etc. is assumed.                                  */

 int iblk, num_logic_blocks;
 int min_inputs_used, min_clocks_used;
 int max_inputs_used, max_clocks_used;
 int summ_inputs_used, summ_clocks_used;
 int inputs_used, clocks_used;

 printf("\nUnclustered Netlist Statistics:\n\n");
 num_logic_blocks = num_blocks - num_p_inputs - num_p_outputs;
 printf("%d Logic Blocks.\n", num_logic_blocks);

 min_inputs_used = lut_size+1;
 min_clocks_used = 2;
 max_inputs_used = -1;
 max_clocks_used = -1;
 summ_inputs_used = 0;
 summ_clocks_used = 0;

 for (iblk=0;iblk<num_blocks;iblk++) {
    if (block[iblk].type == LUT || block[iblk].type == LATCH || 
           block[iblk].type == LUT_AND_LATCH) {
       inputs_used = num_input_pins(iblk);
       if (block[iblk].nets[lut_size+1] != OPEN)
          clocks_used = 1;
       else 
          clocks_used = 0;

       min_inputs_used = min (min_inputs_used, inputs_used);
       max_inputs_used = max (max_inputs_used, inputs_used);
       summ_inputs_used += inputs_used;

       min_clocks_used = min (min_clocks_used, clocks_used);
       max_clocks_used = max (max_clocks_used, clocks_used);
       summ_clocks_used += clocks_used;
    }
 }

 printf("\n\t\t\tAverage\t\tMin\tMax\n");
 printf("Logic Blocks / Cluster\t%f\t%d\t%d\n", 1., 1, 1);
 printf("Used Inputs / Cluster\t%f\t%d\t%d\n", (float) summ_inputs_used /
        (float) num_logic_blocks, min_inputs_used, max_inputs_used);
 printf("Used Clocks / Cluster\t%f\t%d\t%d\n", (float) summ_clocks_used /
        (float) num_logic_blocks, min_clocks_used, max_clocks_used);
 printf("\n");
}


int main (int argc, char *argv[]) {

 char title[] = "\nT-Vpack / Vpack Version 4.30\n"
                "Vaughn Betz and (modified by) Alexander(Sandy) Marquardt\n"
                "Netlist translator/logic block packer for use with VPR\n"
                "Source completed March 25, 2000; "
                "compiled " __DATE__ ".\n"
                "This code is licensed for non-commercial use only.\n\n";

 char blif_file[BUFSIZE];
 char output_file[BUFSIZE];
 boolean global_clocks;
 int cluster_size, inputs_per_cluster, clocks_per_cluster;
 int lut_size;
 boolean muxes_to_cluster_output_pins;
 boolean hill_climbing_flag, skip_clustering, allow_unrelated_clustering;
 boolean timing_driven;
 boolean allow_early_exit;
 enum e_cluster_seed cluster_seed_type;
 float alpha;
 int recompute_timing_after; 
 float block_delay;
 float intra_cluster_net_delay; 
 float inter_cluster_net_delay;
 boolean connection_driven;

 boolean *is_clock;      /* [0..num_nets-1] TRUE if a clock. */

 printf("%s",title);

 parse_command (argc, argv, blif_file, output_file, &global_clocks,
    &cluster_size, &inputs_per_cluster, &clocks_per_cluster, &lut_size,
    &muxes_to_cluster_output_pins, 
    &hill_climbing_flag, &timing_driven, &cluster_seed_type, &alpha, 
    &recompute_timing_after, &block_delay, &intra_cluster_net_delay,
    &inter_cluster_net_delay, &skip_clustering, &allow_unrelated_clustering,
    &allow_early_exit, &connection_driven);

 
 read_blif (blif_file, lut_size);
 echo_input (blif_file, lut_size, "input.echo");

 pack_luts_and_ffs (lut_size);
 compress_netlist (lut_size);

/* NB:  It's important to mark clocks and such *after* compressing the   *
 * netlist because the net numbers, etc. may be changed by removing      *
 * unused inputs and latch folding.                                      */

 is_clock = alloc_and_load_is_clock (global_clocks, lut_size);

 printf("\nAfter packing to LUT+FF Logic Blocks:\n");
 printf("LUT+FF Logic Blocks: %d.  Total Nets: %d.\n", num_blocks - 
       num_p_outputs - num_p_inputs, num_nets);


/* Uncomment line below if you want a dump of compressed netlist. */
/* echo_input (blif_file, lut_size, "packed.echo"); */

 if (skip_clustering == FALSE) {
    do_clustering (cluster_size, inputs_per_cluster, clocks_per_cluster,
        lut_size, global_clocks, muxes_to_cluster_output_pins,
        is_clock, hill_climbing_flag,
        output_file, timing_driven, cluster_seed_type, alpha, 
        recompute_timing_after, block_delay, 
        intra_cluster_net_delay, inter_cluster_net_delay, 
	allow_unrelated_clustering, allow_early_exit, connection_driven);
 }
 else {
    unclustered_stats (lut_size);
    output_clustering (NULL, NULL, cluster_size, inputs_per_cluster,
       clocks_per_cluster, 0, lut_size, global_clocks,
       muxes_to_cluster_output_pins, is_clock, output_file);
 }

 free (is_clock);

 printf("\nNetlist conversion complete.\n\n");
 return (0);
}




static void parse_command (int argc, char *argv[], char *blif_file, 
   char *output_file, boolean *global_clocks, int *cluster_size,
   int *inputs_per_cluster, int *clocks_per_cluster, int *lut_size,
   boolean *muxes_to_cluster_output_pins,
   boolean *hill_climbing_flag, boolean *timing_driven,
   enum e_cluster_seed *cluster_seed_type, float *alpha, 
   int *recompute_timing_after, float *block_delay,
   float *intra_cluster_net_delay, float *inter_cluster_net_delay,
   boolean *skip_clustering, boolean *allow_unrelated_clustering,
   boolean *allow_early_exit, boolean *connection_driven) {

/* Parse the command line to determine user options. */

 int i;
 boolean inputs_per_cluster_set;

 if (argc < 3) {
    printf("Usage:  t-vpack input.blif output.net [-lut_size <int>]\n");
    printf("\t[-cluster_size <int>] [-inputs_per_cluster <int>]\n");
    printf("\t[-clocks_per_cluster <int>] [-global_clocks on|off]\n");
    printf("\t[-muxes_to_cluster_output_pins on|off]\n");
    printf("\t[-hill_climbing on|off]\n");
    printf("\t[-timing_driven on|off]\n");
    printf("\t[-cluster_seed timing|max_inputs] [-alpha <float>]\n");
    printf("\t[-recompute_timing_after <int>] [-block_delay <float>]\n");
    printf("\t[-allow_unrelated_clustering on|off]\n");
    printf("\t[-allow_early_exit on|off]\n");
    printf("\t[-intra_cluster_net_delay <float>] \n");
    printf("\t[-inter_cluster_net_delay <float>] \n");
    printf("\t[-connection_driven on|off] \n");
    printf("\t[-no_clustering]\n\n");
    exit(1);
 }

/* Set defaults. */

 *lut_size = 4;
 *global_clocks = TRUE;
 *cluster_size = 1;
 *inputs_per_cluster = -1;  /* Dummy.  Need other params to set default. */
 inputs_per_cluster_set = FALSE;
 *clocks_per_cluster = 1;
 *muxes_to_cluster_output_pins = FALSE;

 *hill_climbing_flag = TRUE;
 *skip_clustering = FALSE;
 *allow_unrelated_clustering = TRUE;
 *allow_early_exit = FALSE;
 *connection_driven = FALSE;

 *timing_driven = TRUE;
 *cluster_seed_type = TIMING; /*is later set to MAX_INPUTS if we are not timing driven*/
 *alpha=0.75;
 *recompute_timing_after=BIG_INT; /*never recomputer timing*/
 *block_delay=0.1;
 *intra_cluster_net_delay=0.1;
 *inter_cluster_net_delay=1.0;


/* Start parsing the command line.  First two arguments are mandatory. */

 strncpy (blif_file, argv[1], BUFSIZE);
 strncpy (output_file, argv[2], BUFSIZE);
 i = 3;

/* Now get any optional arguments. */

 while (i < argc) {
    
    if (strcmp (argv[i], "-lut_size") == 0) {
    
       *lut_size = read_int_option (argc, argv, i);

       if (*lut_size < 2 || *lut_size > MAXLUT) {
          printf("Error:  lut_size must be between 2 and MAXLUT (%d).\n",
                   MAXLUT);
          exit (1);
       }
       i += 2;
       continue;
    }

    if (strcmp (argv[i],"-global_clocks") == 0) {
       if (argc <= i+1) {
          printf ("Error:  -global_clocks option requires a string parameter."
                  "\n"); 
          exit (1);    
       } 
       if (strcmp(argv[i+1], "on") == 0) {
          *global_clocks = TRUE;
       } 
       else if (strcmp(argv[i+1], "off") == 0) {
          *global_clocks = FALSE;
       } 
       else {
          printf("Error:  -global_clocks must be on or off.\n");
          exit (1);
       } 
 
       i += 2;
       continue;
    }

    if (strcmp (argv[i],"-no_clustering") == 0) {
       *skip_clustering = TRUE;
       i++;
       continue;
    }

    if (strcmp (argv[i],"-hill_climbing") == 0) {
       if (argc <= i+1) {
          printf ("Error:  -hill_climbing option requires a string parameter."
                  "\n");
          exit (1);
       }
       if (strcmp(argv[i+1], "on") == 0) {
          *hill_climbing_flag = TRUE;
       }
       else if (strcmp(argv[i+1], "off") == 0) {
          *hill_climbing_flag = FALSE;
       }
       else {
          printf("Error:  -hill_climbing must be on or off.\n");
          exit (1);
       }
 
       i += 2;
       continue;
    }

    if (strcmp (argv[i],"-allow_unrelated_clustering") == 0) {
       if (argc <= i+1) {
          printf ("Error:  -allow_unrelated_clustering option requires a "
		  "string parameter.\n");
          exit (1);
       }
       if (strcmp(argv[i+1], "on") == 0) {
          *allow_unrelated_clustering = TRUE;
       }
       else if (strcmp(argv[i+1], "off") == 0) {
          *allow_unrelated_clustering = FALSE;
       }
       else {
          printf("Error:  -allow_unrelated_clustering must be on or off.\n");
          exit (1);
       }
 
       i += 2;
       continue;
    }
    if (strcmp (argv[i],"-allow_early_exit") == 0) {
       if (argc <= i+1) {
          printf ("Error:  -allow_early_exit option requires a "
		  "string parameter.\n");
          exit (1);
       }
       if (strcmp(argv[i+1], "on") == 0) {
          *allow_early_exit = TRUE;
       }
       else if (strcmp(argv[i+1], "off") == 0) {
          *allow_early_exit = FALSE;
       }
       else {
          printf("Error:  -allow_early_exit must be on or off.\n");
          exit (1);
       }
 
       i += 2;
       continue;
    }

    if (strcmp (argv[i],"-timing_driven") == 0) {
       if (argc <= i+1) {
          printf ("Error:  -timing_driven option requires a string parameter."
                  "\n"); 
          exit (1);    
       } 
       if (strcmp(argv[i+1], "on") == 0) {
          *timing_driven = TRUE;
       } 
       else if (strcmp(argv[i+1], "off") == 0) {
          *timing_driven = FALSE;
       } 
       else {
          printf("Error:  -timing_driven must be on or off.\n");
          exit (1);
       } 
 
       i += 2;
       continue;
    }
    if (strcmp (argv[i],"-connection_driven") == 0) {
       if (argc <= i+1) {
          printf ("Error:  -connection_driven option requires a string parameter."
                  "\n"); 
          exit (1);    
       } 
       if (strcmp(argv[i+1], "on") == 0) {
          *connection_driven = TRUE;
       } 
       else if (strcmp(argv[i+1], "off") == 0) {
          *connection_driven = FALSE;
       } 
       else {
          printf("Error:  -connection_driven must be on or off.\n");
          exit (1);
       } 
 
       i += 2;
       continue;
    }

    if (strcmp(argv[i],"-cluster_seed") == 0) {
       if (argc <= i+1) {
           printf("Error:  -cluster_seed option requires a "
              "string parameter.\n");
           exit (1);
       }

       if (strcmp(argv[i+1], "timing") == 0) {
          *cluster_seed_type = TIMING;
       }
       else if (strcmp(argv[i+1], "max_inputs") == 0) {
          *cluster_seed_type = MAX_INPUTS;
       }
       else {
          printf("Error:  -cluster_seed must be timing or "
             "max_inputs.\n");
          exit (1);
       }

       i += 2;
       continue;
    }

    if (strcmp (argv[i],"-alpha") == 0) {
       *alpha = read_float_option (argc, argv, i);

       if (*alpha > 1.0 || *alpha < 0.0) {
          printf("Error:  alpha must be between 0 and 1.\n");
          exit (1);
       }

       i += 2;
       continue;
    }

    if (strcmp (argv[i],"-recompute_timing_after") == 0) {
       *recompute_timing_after = read_int_option (argc, argv, i);

       if (*recompute_timing_after < 1){
          printf("Error:  recompute_timing_after must be >= 1\n");
          exit (1);
       }

       i += 2;
       continue;
    }

    if (strcmp (argv[i],"-block_delay") == 0) {
       *block_delay = read_float_option (argc, argv, i);
       i += 2;
       continue;
    }


    if (strcmp (argv[i],"-intra_cluster_net_delay") == 0) {
       *intra_cluster_net_delay = read_float_option (argc, argv, i);
       i += 2;
       continue;
    }

    if (strcmp (argv[i],"-inter_cluster_net_delay") == 0) {
       *inter_cluster_net_delay = read_float_option (argc, argv, i);
       i += 2;
       continue;
    }

    if (strcmp (argv[i],"-cluster_size") == 0) {
       *cluster_size = read_int_option (argc, argv, i);

       if (*cluster_size < 1) {
          printf("Error:  cluster_size must be greater than 0.\n");
          exit (1);
       }

       i += 2;
       continue;
    }

    if (strcmp (argv[i],"-inputs_per_cluster") == 0) {
       *inputs_per_cluster = read_int_option (argc, argv, i);
       inputs_per_cluster_set = TRUE;

       /* Do sanity check after cluster_size and lut_size known. */

       i += 2;
       continue;
    }

    if (strcmp (argv[i],"-clocks_per_cluster") == 0) {
       *clocks_per_cluster = read_int_option (argc, argv, i);

       /* Do sanity check after cluster_size known. */

       i += 2;
       continue;
    }

   if (strcmp (argv[i],"-muxes_to_cluster_output_pins") == 0) {
       if (argc <= i+1) {
          printf ("Error:  -muxes_to_cluster_output_pins option requires a "
                     "string parameter.\n");
          exit (1);
       }
       if (strcmp(argv[i+1], "on") == 0) {
          *muxes_to_cluster_output_pins = TRUE;
       }
       else if (strcmp(argv[i+1], "off") == 0) {
          *muxes_to_cluster_output_pins = FALSE;
       }
       else {
          printf("Error:  -muxes_to_cluster_output_pins must be on or off.\n");
          exit (1);
       }

       i += 2;
       continue;
    }

    

    printf("Unrecognized option: %s.  Aborting.\n",argv[i]);
    exit(1);

 }   /* End of options while loop. */

/* Assign new default values if required. */

 if (!inputs_per_cluster_set) 
    *inputs_per_cluster = *lut_size * *cluster_size;

 if (!*timing_driven) {
   *cluster_seed_type=MAX_INPUTS;
   *allow_unrelated_clustering = TRUE;
 }
 
 if (*recompute_timing_after == -1) {
   *recompute_timing_after = *cluster_size;
 }

 /*additional santiy checks*/

 if (*inputs_per_cluster < *lut_size || *inputs_per_cluster > *cluster_size
            * *lut_size) {
    printf("Error:  Number of input pins must be between lut_size and "
       "cluster_size * lut_size.\n");
    exit (1);
 }

 if (*clocks_per_cluster < 1 || *clocks_per_cluster > *cluster_size) {
    printf("Error:  Number of clock pins must be between 1 and "
         "cluster_size.\n");
    exit(1);
 }

 if (*skip_clustering == TRUE) {
    if (*inputs_per_cluster != *lut_size || *cluster_size != 1 ||
           *clocks_per_cluster != 1 || *muxes_to_cluster_output_pins !=
            FALSE) {
       printf("Error:  Cluster attributes cannot be specified with \n");
       printf("-no_clustering enabled.\n");
       exit (1);
    }
 }

/* Echo back selected options. */

 printf("Selected options:\n\n");
 printf("\tLut Size: %d inputs\n", *lut_size);
 printf("\tCluster Size: %d (LUT+FF) logic block(s) / cluster\n", 
      *cluster_size);
 printf("\tDistinct Input Pins Per Cluster: %d\n", *inputs_per_cluster);
 printf("\tClock Pins Per Cluster: %d\n", *clocks_per_cluster);
 printf("\tClocks Routed Via Dedicated Resource: %s\n",
      (*global_clocks) ? "Yes" : "No");
 printf("\tEach BLE output connected directly to a cluster output: %s\n",
      (*muxes_to_cluster_output_pins) ? "No" : "Yes");

 if (*skip_clustering) {
    printf("\tClustering Step Will NOT Be Performed.\n");
 }
 else {
    printf("\tClustering Seed Found Via:  %s\n",
           (*cluster_seed_type == TIMING) ?
           "Most Critical Block" : "Maximum Inputs Used");
    printf("\tHill Climbing During Clustering: %s\n",
	   (*hill_climbing_flag) ? "Enabled" : "Disabled");
    printf("\tAllow Unrelated Blocks to be Clustered: %s\n",
	   (*allow_unrelated_clustering) ? "Yes" : " No "
	   "(Yes after critical path is fully minimized)");
    printf("\tConnection Driven Clustering: %s\n",
	   (*connection_driven) ? "yes" : " No ");
    if (*timing_driven){
      printf("\tTiming Driven Clustering On\n");
      printf("\tTiming Analysis Done Every %d blocks\n",*recompute_timing_after);
      printf("\tAllow Early Exit: %s\n",
           (*allow_early_exit) ? "Yes" : " No");
      printf("\tTradeoff Parameter Alpha:%7.2f\n",*alpha);
      printf("\tDelay Through Blocks:%7.2f\n",*block_delay);
      printf("\tIntra Cluster Net Delay:%7.2f\n", *intra_cluster_net_delay);
      printf("\tInter Cluster Net Delay:%7.2f\n", *inter_cluster_net_delay);
    }
    else
      printf("\tTiming Driven Clustering: Off\n");

 }

 printf("\n\n");
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
 * and be an integer, or an error message is printed and the program      *
 * exits.                                                                 */
 
 float value;
 int num_read;
 
 num_read = 0;
 
/* Does value exist for this option? */
 
 if (argc > iarg+1)
    num_read = sscanf(argv[iarg+1],"%f",&value);
 
/* Value exists and was a proper int? */
 
 if (num_read != 1) {
    printf("Error:  %s option requires an integer parameter.\n\n", argv[iarg]);
    exit(1);
 }
 
 return (value);
}
