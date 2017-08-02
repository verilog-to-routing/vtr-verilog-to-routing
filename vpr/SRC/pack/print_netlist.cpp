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

	unsigned int j;
	int max_pin, num_global_nets;
	int L_num_p_inputs, L_num_p_outputs;
	FILE *fp;

	num_global_nets = 0;
	L_num_p_inputs = 0;
	L_num_p_outputs = 0;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();

	/* Count number of global nets */
	for (auto net_id : cluster_ctx.clb_nlist.nets()) {
		if (cluster_ctx.clb_nlist.net_global(net_id)) {
			num_global_nets++;
		}
	}

	/* Count I/O input and output pads */
	for (auto block_id : cluster_ctx.clb_nlist.blocks()) {
		if (cluster_ctx.clb_nlist.block_type(block_id) == device_ctx.IO_TYPE) {
			for (auto pin_id : cluster_ctx.clb_nlist.block_pins(block_id)) {
				int pin_index = cluster_ctx.clb_nlist.pin_index(pin_id);
				if (device_ctx.IO_TYPE->class_inf[device_ctx.IO_TYPE->pin_class[pin_index]].type == DRIVER) {
					L_num_p_inputs++;
				}
				else {
					VTR_ASSERT(device_ctx.IO_TYPE->class_inf[device_ctx.IO_TYPE->pin_class[pin_index]].type == RECEIVER);
					L_num_p_outputs++;
				}
			}
		}
	}

	fp = vtr::fopen(foutput, "w");

	fprintf(fp, "Input netlist file: %s\n", net_file);
	fprintf(fp, "L_num_p_inputs: %d, L_num_p_outputs: %d, num_clbs: %lu\n",
			L_num_p_inputs, L_num_p_outputs, cluster_ctx.clb_nlist.blocks().size());
	fprintf(fp, "num_nets: %lu, num_globals: %d\n",
			cluster_ctx.clb_nlist.nets().size(), num_global_nets);
	fprintf(fp, "\nNet\tName\t\t#Pins\tDriver\t\tRecvs. (blocks, pin)\n");

	for (auto net_id : cluster_ctx.clb_nlist.nets()) {
		fprintf(fp, "\n%lu\t%s\t", (size_t)net_id, cluster_ctx.clb_nlist.net_name(net_id).c_str());
		if (cluster_ctx.clb_nlist.net_name(net_id).length() < 8)
			fprintf(fp, "\t"); /* Name field is 16 chars wide */
		fprintf(fp, "%lu", cluster_ctx.clb_nlist.net_pins(net_id).size());
		for (auto pin_id : cluster_ctx.clb_nlist.net_pins(net_id))
			fprintf(fp, "\t(%4lu, %4d)", (size_t)cluster_ctx.clb_nlist.pin_block(pin_id),
				cluster_ctx.clb_nlist.pin_index(pin_id));
	}


	fprintf(fp, "\nBlock\tName\t\tType\tPin Connections\n\n");

	for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
		fprintf(fp, "\n%lu\t%s\t", (size_t)blk_id, cluster_ctx.clb_nlist.block_name(blk_id).c_str());
		if (cluster_ctx.clb_nlist.block_name(blk_id).length() < 8)
			fprintf(fp, "\t"); /* Name field is 16 chars wide */
		fprintf(fp, "%s", cluster_ctx.clb_nlist.block_type(blk_id)->name);

		max_pin = cluster_ctx.clb_nlist.block_type(blk_id)->num_pins;

		for (j = 0; j < (unsigned int)max_pin; j++)
			print_pinnum(fp, (size_t)cluster_ctx.clb_nlist.block_net(blk_id,j));
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
