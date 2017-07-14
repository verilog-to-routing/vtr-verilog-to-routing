#include <cstdio>
#include <cstring>
#include "vtr_assert.h"
using namespace std;

#include "vtr_util.h"

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

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();

	/* Count number of global nets */
	for (i = 0; i < cluster_ctx.clbs_nlist.net.size(); i++) {
		if (cluster_ctx.clbs_nlist.net[i].is_global) {
			num_global_nets++;
		}
	}

	/* Count I/O input and output pads */
	for (i = 0; i < (unsigned int) cluster_ctx.clb_nlist.blocks().size(); i++) {
		if (cluster_ctx.blocks[i].type == device_ctx.IO_TYPE) {
			for (j = 0; j < (unsigned int) device_ctx.IO_TYPE->num_pins; j++) {
				if (cluster_ctx.blocks[i].nets[j] != OPEN) {
					if (device_ctx.IO_TYPE->class_inf[device_ctx.IO_TYPE->pin_class[j]].type
							== DRIVER) {
						L_num_p_inputs++;
					} else {
						VTR_ASSERT(
								device_ctx.IO_TYPE-> class_inf[device_ctx.IO_TYPE-> pin_class[j]]. type == RECEIVER);
						L_num_p_outputs++;
					}
				}
			}
		}
	}

	fp = vtr::fopen(foutput, "w");

	fprintf(fp, "Input netlist file: %s\n", net_file);
	fprintf(fp, "L_num_p_inputs: %d, L_num_p_outputs: %d, num_clbs: %d\n",
			L_num_p_inputs, L_num_p_outputs, (int) cluster_ctx.clb_nlist.blocks().size());
	fprintf(fp, "num_nets: %d, num_globals: %d\n",
			(int) cluster_ctx.clbs_nlist.net.size(), num_global_nets);
	fprintf(fp, "\nNet\tName\t\t#Pins\tDriver\t\tRecvs. (blocks, pin)\n");

	for (i = 0; i < cluster_ctx.clbs_nlist.net.size(); i++) {
		fprintf(fp, "\n%d\t%s\t", i, cluster_ctx.clbs_nlist.net[i].name);
		if (strlen(cluster_ctx.clbs_nlist.net[i].name) < 8)
			fprintf(fp, "\t"); /* Name field is 16 chars wide */
		fprintf(fp, "%d", (int) cluster_ctx.clbs_nlist.net[i].pins.size());
		for (j = 0; j < cluster_ctx.clbs_nlist.net[i].pins.size(); j++)
			fprintf(fp, "\t(%4d,%4d)", cluster_ctx.clbs_nlist.net[i].pins[j].block,
				cluster_ctx.clbs_nlist.net[i].pins[j].block_pin);
	}

	fprintf(fp, "\nBlock\tName\t\tType\tPin Connections\n\n");

	for (i = 0; i < (unsigned int) cluster_ctx.clb_nlist.blocks().size(); i++) {
		fprintf(fp, "\n%d\t%s\t", i, cluster_ctx.clb_nlist.block_name((BlockId) i).c_str());
		if (strlen(cluster_ctx.clb_nlist.block_name((BlockId) i).c_str()) < 8)
			fprintf(fp, "\t"); /* Name field is 16 chars wide */
		fprintf(fp, "%s", cluster_ctx.clb_nlist.block_type((BlockId) i)->name);

		max_pin = cluster_ctx.blocks[i].type->num_pins;

		for (j = 0; j < (unsigned int) max_pin; j++)
			print_pinnum(fp, cluster_ctx.blocks[i].nets[j]);
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
