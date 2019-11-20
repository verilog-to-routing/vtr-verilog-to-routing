#include <stdio.h>
#include <string.h>
#include "blifmap.h"
#include "util.h"

char blif_file[BUFSIZE];
FILE *blif;
int lut_size;

char model[];

int num_nets, num_blocks;
int num_p_inputs, num_p_outputs, num_luts, num_latches;
struct s_net *net;
struct s_block *block;


int main (int argc, char *argv[]) {

 char title[] = "\nBlifmap Version 1.24 by V. Betz\n"
                "Netlist translator/mapper for use with VPR\n\n";

 int global_clocks = 0;
 int *is_clock;      /* [0..num_nets-1] 0 if not a clock, 1 otherwise. */

 char output_file[BUFSIZE];
 void echo_input (void);
 void map_netlist (int global_clocks, int *is_clock);
 void output_netlist (char *fname, int global_clocks, int *is_clock);
 void read_blif (FILE *blif);

 if (argc <= 4 || argc > 6) {
    printf("Usage:  blifmap input.blif output.net -lut_size num [-global]\n");
    exit(1);
 }

 printf("%s",title);

 strncpy (blif_file, argv[1], BUFSIZE);
 strncpy (output_file, argv[2], BUFSIZE);
    
 blif = my_open (blif_file,"r",0);

 if (strcmp (argv[3], "-lut_size") != 0) {
    printf("Error:  -lut_size parameter not set.\n");
    exit(1);
 }
 sscanf (argv[4], "%d", &lut_size);
 if (lut_size < 2 || lut_size > MAXLUT) {
    printf("Error:  lut_size must be between 2 and MAXLUT (%d).\n",MAXLUT);
    exit (1);
 }

 if (argc == 6) {
    if (strcmp (argv[5],"-global") != 0) {
       printf("Error:  Unrecognized option, %s.\n",argv[5]);
       exit(1);
    }
    global_clocks = 1;
 }

 read_blif (blif);
 echo_input ();
 is_clock = my_calloc (num_nets, sizeof(int));
 map_netlist (global_clocks, is_clock);
 output_netlist (output_file, global_clocks, is_clock);

 printf("\nNetlist conversion complete.\n\n");
 return (0);
}
