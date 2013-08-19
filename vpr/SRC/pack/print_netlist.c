#include <cstdio>
#include <cstring>
using namespace std;

#include <assert.h>

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "print_netlist.h"
#include "read_xml_arch_file.h"

/******************** Subroutines local to this module ***********************/

static void print_pinnum(FILE * fp, int pinnum);

/********************* Subroutine definitions ********************************/

void print_netlist(char *foutput, char *net_file) {

	/* Prints out the netlist related data structures into the file    *
	 * fname.                                                          */

	unsigned int i, j;
	int max_pin;
	int num_global_nets;
	int L_num_p_inputs, L_num_p_outputs;
	FILE *fp;

	num_global_nets = 0;
	L_num_p_inputs = 0;
	L_num_p_outputs = 0;

	/* Count number of global nets */
	for (i = 0; i < g_clbs_nlist.net.size(); i++) {
		if (g_clbs_nlist.net[i].is_global) {
			num_global_nets++;
		}
	}

	/* Count I/O input and output pads */
	for (i = 0; i < (unsigned int) num_blocks; i++) {
		if (block[i].type == IO_TYPE) {
			for (j = 0; j < (unsigned int) IO_TYPE->num_pins; j++) {
				if (block[i].nets[j] != OPEN) {
					if (IO_TYPE->class_inf[IO_TYPE->pin_class[j]].type
							== DRIVER) {
						L_num_p_inputs++;
					} else {
						assert(
								IO_TYPE-> class_inf[IO_TYPE-> pin_class[j]]. type == RECEIVER);
						L_num_p_outputs++;
					}
				}
			}
		}
	}

	fp = my_fopen(foutput, "w", 0);

	fprintf(fp, "Input netlist file: %s\n", net_file);
	fprintf(fp, "L_num_p_inputs: %d, L_num_p_outputs: %d, num_clbs: %d\n",
			L_num_p_inputs, L_num_p_outputs, num_blocks);
	fprintf(fp, "num_blocks: %d, num_nets: %d, num_globals: %d\n", num_blocks,
			(int) g_clbs_nlist.net.size(), num_global_nets);
	fprintf(fp, "\nNet\tName\t\t#Pins\tDriver\t\tRecvs. (block, pin)\n");

	for (i = 0; i < g_clbs_nlist.net.size(); i++) {
		fprintf(fp, "\n%d\t%s\t", i, g_clbs_nlist.net[i].name);
		if (strlen(g_clbs_nlist.net[i].name) < 8)
			fprintf(fp, "\t"); /* Name field is 16 chars wide */
		fprintf(fp, "%d", (int) g_clbs_nlist.net[i].pins.size());
		for (j = 0; j < g_clbs_nlist.net[i].pins.size(); j++)
			fprintf(fp, "\t(%4d,%4d)", g_clbs_nlist.net[i].pins[j].block,
				g_clbs_nlist.net[i].pins[j].block_pin);
	}

	fprintf(fp, "\nBlock\tName\t\tType\tPin Connections\n\n");

	for (i = 0; i < (unsigned int)num_blocks; i++) {
		fprintf(fp, "\n%d\t%s\t", i, block[i].name);
		if (strlen(block[i].name) < 8)
			fprintf(fp, "\t"); /* Name field is 16 chars wide */
		fprintf(fp, "%s", block[i].type->name);

		max_pin = block[i].type->num_pins;

		for (j = 0; j < (unsigned int) max_pin; j++)
			print_pinnum(fp, block[i].nets[j]);
	}

	fprintf(fp, "\n");

	/* TODO: Print out pb info */

	fclose(fp);
}

static void print_pinnum(FILE * fp, int pinnum) {

	/* This routine prints out either OPEN or the pin number, to file fp. */

	if (pinnum == OPEN)
		fprintf(fp, "\tOPEN");
	else
		fprintf(fp, "\t%d", pinnum);
}
