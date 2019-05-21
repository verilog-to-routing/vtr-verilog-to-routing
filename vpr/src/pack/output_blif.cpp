/*
 Jason Luu
 Date: February 11, 2013

 Print blif representation of circuit

 Limitations: [jluu] Due to ABC requiring that all blackbox inputs be named exactly the same in the netlist to be checked as in the input netlist,
 I am forced to skip all internal connectivity checking for inputs going into subckts.  If in the future, ABC no longer treats
 blackboxes as primariy I/Os, then we should revisit this and make it so that we can do formal equivalence on a more representative netlist.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace std;

#include "vtr_assert.h"
#include "vtr_util.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "atom_netlist.h"
#include "output_blif.h"
#include "vpr_utils.h"

#define LINELENGTH 1024
#define TABLENGTH 1

/****************** Subroutines local to this module ************************/
void print_atom_block(FILE *fpout, AtomBlockId atom_blk);
void print_routing_in_clusters(FILE *fpout, ClusterBlockId clb_index);
void print_models(FILE *fpout, t_model *user_models);

/**************** Subroutine definitions ************************************/

static void print_string(const char *str_ptr, int *column, FILE * fpout) {

	/* Prints string without making any lines longer than LINELENGTH.  Column  *
	 * points to the column in which the next character will go (both used and *
	 * updated), and fpout points to the output file.                          */

	int len;

	len = strlen(str_ptr);
	if (len + 3 > LINELENGTH) {
		vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
				"in print_string: String %s is too long for desired maximum line length.\n", str_ptr);
	}

	if (*column + len + 2 > LINELENGTH) {
		fprintf(fpout, "\\ \n");
		*column = TABLENGTH;
	}

	fprintf(fpout, "%s ", str_ptr);
	*column += len + 1;
}

static void print_net_name(AtomNetId net_id, int *column, FILE * fpout) {

	/* This routine prints out the atom_ctx.nlist net name (or open) and limits the    *
	 * length of a line to LINELENGTH characters by using \ to continue *
	 * lines.  net_id is the id of the atom_ctx.nlist net to be printed, while     *
	 * column points to the current printing column (column is both     *
	 * used and updated by this routine).  fpout is the output file     *
	 * pointer.                                                         */

	char *str_ptr;

	if (!net_id) {
		str_ptr = new char [6];
		sprintf(str_ptr, "open");
	} else {
        auto& atom_ctx = g_vpr_ctx.atom();
		str_ptr = new char[strlen(atom_ctx.nlist.net_name(net_id).c_str()) + 7];
		sprintf(str_ptr, "__|e%s", atom_ctx.nlist.net_name(net_id).c_str());
	}

	print_string(str_ptr, column, fpout);
	delete [] str_ptr;
}

/* Print netlist atom in blif format */
void print_atom_block(FILE *fpout, AtomBlockId atom_blk) {
	const t_pb_graph_node *pb_graph_node;
	t_pb_type *pb_type;

    auto& atom_ctx = g_vpr_ctx.atom();
	auto& cluster_ctx = g_vpr_ctx.clustering();

	ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(atom_blk);
	VTR_ASSERT(clb_index != ClusterBlockId::INVALID());

	const auto& pb_route = cluster_ctx.clb_nlist.block_pb(clb_index)->pb_route;
	pb_graph_node = atom_ctx.lookup.atom_pb_graph_node(atom_blk);
    VTR_ASSERT(pb_graph_node);

	pb_type = pb_graph_node->pb_type;


    if (atom_ctx.nlist.block_type(atom_blk) == AtomBlockType::INPAD) {
		int node_index = pb_graph_node->output_pins[0][0].pin_count_in_cluster;
		fprintf(fpout, ".names %s clb_%zu_rr_node_%d\n", atom_ctx.nlist.block_name(atom_blk).c_str(), size_t(clb_index), node_index);
		fprintf(fpout, "1 1\n\n");
    } else if (atom_ctx.nlist.block_type(atom_blk) == AtomBlockType::OUTPAD) {
		int node_index = pb_graph_node->input_pins[0][0].pin_count_in_cluster;

        //Don't add a buffer if the name is prefixed with :out, just remove the :out
        auto input_pins = atom_ctx.nlist.block_input_pins(atom_blk);
        VTR_ASSERT(input_pins.size() == 1);
        auto input_pin = *input_pins.begin();
        auto input_net = atom_ctx.nlist.pin_net(input_pin);

        const char* outpad_input_net = atom_ctx.nlist.net_name(input_net).c_str();
        const char* trimmed_outpad_name = atom_ctx.nlist.block_name(atom_blk).c_str() + 4;
		if (strcmp(outpad_input_net, trimmed_outpad_name) != 0) {
			fprintf(fpout, ".names %s clb_%zu_rr_node_%d\n", trimmed_outpad_name, size_t(clb_index), node_index);
			fprintf(fpout, "1 1\n\n");
		}
	}
	else if(strcmp(atom_ctx.nlist.block_model(atom_blk)->name, MODEL_NAMES) == 0) {
		/* Print a LUT, a LUT has K input pins and one output pin */
		fprintf(fpout, ".names ");

        VTR_ASSERT(pb_type->num_ports == 2);

        //Print the input port
		for (int i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == IN_PORT) {
				/* LUTs receive special handling because a LUT has logically equivalent inputs.
					The intra-logic block router may have taken advantage of logical equivalence so we need to unscramble the inputs when we output the LUT logic.
				*/
				VTR_ASSERT(pb_type->ports[i].is_clock == false);

                AtomPortId port_id = atom_ctx.nlist.find_port(atom_blk, pb_type->ports[i].name);
                if(port_id) {
                    for (int j = 0; j < pb_type->ports[i].num_pins; j++) {
                        AtomPinId pin_id = atom_ctx.nlist.port_pin(port_id, j);
                        if(pin_id) {
                            AtomNetId net_id = atom_ctx.nlist.pin_net(pin_id);
                            VTR_ASSERT(net_id);

                            int k;
                            for (k = 0; k < pb_type->ports[i].num_pins; k++) {
                                int node_index = pb_graph_node->input_pins[0][k].pin_count_in_cluster;
                                AtomNetId a_net_id = pb_route[node_index].atom_net_id;
                                if(a_net_id) {
                                    if(a_net_id == net_id) {
                                        fprintf(fpout, "clb_%zu_rr_node_%d ", size_t(clb_index), pb_route[node_index].driver_pb_pin_id);
                                        break;
                                    }
                                }
                            }
                            if(k == pb_type->ports[i].num_pins) {
                                /* Failed to find LUT input, a netlist error has occurred */
                                vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
                                    "LUT %s missing input %s post packing please file a bug report\n",
                                    atom_ctx.nlist.block_name(atom_blk).c_str(), atom_ctx.nlist.net_name(net_id).c_str());
                            }
                        }
                    }
                }
			}
		}

        //Print the output port
		for (int i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == OUT_PORT) {
				int node_index = pb_graph_node->output_pins[0][0].pin_count_in_cluster;
				VTR_ASSERT(pb_type->ports[i].num_pins == 1);

                auto port_id = atom_ctx.nlist.find_port(atom_blk, pb_type->ports[i].name);
                auto pin_id = atom_ctx.nlist.port_pin(port_id, 0);
                VTR_ASSERT(pin_id);
                VTR_ASSERT(atom_ctx.nlist.pin_net(pin_id) == pb_route[node_index].atom_net_id);

				fprintf(fpout, "%s\n", atom_ctx.nlist.block_name(atom_blk).c_str());
			}
		}

        //Print the truth table
        const auto& truth_table = atom_ctx.nlist.block_truth_table(atom_blk);
        for(auto row_iter = truth_table.rbegin(); row_iter != truth_table.rend(); ++row_iter) {
            const auto& row = *row_iter;
            for(auto iter = row.begin(); iter != row.end(); ++iter) {
                if(iter == --row.end()) {
                    fprintf(fpout, " ");
                }
                switch(*iter) {
                    case vtr::LogicValue::TRUE: fprintf(fpout, "1"); break;
                    case vtr::LogicValue::FALSE: fprintf(fpout, "0"); break;
                    case vtr::LogicValue::DONT_CARE: fprintf(fpout, "-"); break;
                    default: VTR_ASSERT(false);
                }
                if(iter == --row.end()) {
                    VTR_ASSERT(*iter != vtr::LogicValue::DONT_CARE);
                }
            }
            fprintf(fpout, "\n");
        }

        //Print the intermediate buffers for the output
		for (int i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == OUT_PORT) {
				int node_index = pb_graph_node->output_pins[0][0].pin_count_in_cluster;
				VTR_ASSERT(pb_type->ports[i].num_pins == 1);

                auto port_id = atom_ctx.nlist.find_port(atom_blk, pb_type->ports[i].name);
                auto pin_id = atom_ctx.nlist.port_pin(port_id, 0);
                VTR_ASSERT(pin_id);
                VTR_ASSERT(atom_ctx.nlist.pin_net(pin_id) == pb_route[node_index].atom_net_id);

				fprintf(fpout, "\n.names %s clb_%zu_rr_node_%d\n", atom_ctx.nlist.block_name(atom_blk).c_str(), size_t(clb_index), node_index);
				fprintf(fpout, "1 1\n");
			}
		}
	} else if (strcmp(atom_ctx.nlist.block_model(atom_blk)->name, MODEL_LATCH) == 0) {
		/* Print a flip-flop.  A flip-flop has a D input, a Q output, and a clock input */
		fprintf(fpout, ".latch ");
		VTR_ASSERT(pb_type->num_ports == 3);
		for (int i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == IN_PORT
					&& pb_type->ports[i].is_clock == false) {
				VTR_ASSERT(pb_type->ports[i].num_pins == 1);

                auto input_pins = atom_ctx.nlist.block_input_pins(atom_blk);
                VTR_ASSERT(input_pins.size() == 1);
                VTR_ASSERT_MSG(atom_ctx.nlist.pin_net(*input_pins.begin()), "Valid input net");

				int node_index = pb_graph_node->input_pins[0][0].pin_count_in_cluster;
				fprintf(fpout, "clb_%zu_rr_node_%d ", size_t(clb_index),	pb_route[node_index].driver_pb_pin_id);
			} else if (pb_type->ports[i].type == OUT_PORT) {
				VTR_ASSERT(pb_type->ports[i].num_pins == 1);

                auto output_pins = atom_ctx.nlist.block_output_pins(atom_blk);
                VTR_ASSERT(output_pins.size() == 1);
                auto output_net_id = atom_ctx.nlist.pin_net(*output_pins.begin());
                VTR_ASSERT_MSG(output_net_id, "Valid output net");


				fprintf(fpout, "%s re ", atom_ctx.nlist.net_name(output_net_id).c_str());
			} else if (pb_type->ports[i].type == IN_PORT
					&& pb_type->ports[i].is_clock == true) {
				VTR_ASSERT(pb_type->ports[i].num_pins == 1);

                auto clock_pins = atom_ctx.nlist.block_clock_pins(atom_blk);
                VTR_ASSERT(clock_pins.size() == 1);
                VTR_ASSERT_MSG(atom_ctx.nlist.pin_net(*clock_pins.begin()), "Valid clock net");

				int node_index = pb_graph_node->clock_pins[0][0].pin_count_in_cluster;
				fprintf(fpout, "clb_%zu_rr_node_%d 2", size_t(clb_index), pb_route[node_index].driver_pb_pin_id);
			} else {
				VTR_ASSERT(0);
			}
		}
		fprintf(fpout, "\n");
		for (int i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == OUT_PORT) {
				int node_index = pb_graph_node->output_pins[0][0].pin_count_in_cluster;
				VTR_ASSERT(pb_type->ports[i].num_pins == 1);
                AtomPortId port_id = atom_ctx.nlist.find_port(atom_blk, pb_type->ports[i].name);
                auto net_id = atom_ctx.nlist.port_net(port_id, 0);
				VTR_ASSERT(net_id == pb_route[node_index].atom_net_id);
				fprintf(fpout, "\n.names %s clb_%zu_rr_node_%d\n", atom_ctx.nlist.net_name(net_id).c_str(), size_t(clb_index), node_index);
				fprintf(fpout, "1 1\n");
			}
		}
	} else {
		const t_model *cur = atom_ctx.nlist.block_model(atom_blk);
		fprintf(fpout, ".subckt %s \\\n", cur->name);

		/* Print input ports */
		t_model_ports *port = cur->inputs;

		while (port != nullptr) {
            AtomPortId port_id = atom_ctx.nlist.find_port(atom_blk, port->name);
			for (int i = 0; i < port->size; i++) {
                fprintf(fpout, "%s[%d]=", port->name, i);

				if (port->is_clock == true) {
					VTR_ASSERT(port->index == 0);
					VTR_ASSERT(port->size == 1);

                    VTR_ASSERT(atom_ctx.nlist.port_type(port_id) == PortType::CLOCK);
                }

                AtomNetId net_id = atom_ctx.nlist.port_net(port_id, i);

                //Output the net
                if(net_id) {
                    fprintf(fpout, "%s", atom_ctx.nlist.net_name(net_id).c_str());
                } else {
                    fprintf(fpout, "unconn");
                }

                fprintf(fpout, " "); //Space after port assignment
				if (i % 4 == 3) {
					if (i + 1 < port->size) {
						fprintf(fpout, "\\\n");
					}
				}
			}
			port = port->next;
			fprintf(fpout, "\\\n");
		}

		/* Print output ports */
		port = cur->outputs;
		while (port != nullptr) {
            AtomPortId port_id = atom_ctx.nlist.find_port(atom_blk, port->name);
			for (int i = 0; i < port->size; i++) {
                AtomNetId net_id = atom_ctx.nlist.port_net(port_id, i);

                if(net_id) {
                    fprintf(fpout, "%s[%d]=%s ", port->name, i, atom_ctx.nlist.net_name(net_id).c_str());
                } else {
                    fprintf(fpout, "%s[%d]=unconn ", port->name, i);
                }

				if (i % 4 == 3) {
					if (i + 1 < port->size) {
						fprintf(fpout, "\\\n");
					}
				}
			}
			port = port->next;
			if (port != nullptr) {
				fprintf(fpout, "\\\n");
			}
		}
		fprintf(fpout, "\n");
		fprintf(fpout, "\n");

        for (auto param : atom_ctx.nlist.block_params(atom_blk)) {
            fprintf(fpout, ".param %s %s\n", param.first.c_str(), param.second.c_str());
        }
        for (auto attr : atom_ctx.nlist.block_attrs(atom_blk)) {
            fprintf(fpout, ".attr %s %s\n", attr.first.c_str(), attr.second.c_str());
        }

        /* Print output buffers */
        port = cur->outputs;
        while (port != nullptr) {
            AtomPortId port_id = atom_ctx.nlist.find_port(atom_blk, port->name);
            for (int ipin = 0; ipin < port->size; ipin++) {
                AtomNetId net_id = atom_ctx.nlist.port_net(port_id, ipin);
                if (net_id) {
                    t_pb_graph_pin *pb_graph_pin = get_pb_graph_node_pin_from_model_port_pin(port, ipin, pb_graph_node);
                    fprintf(fpout, ".names %s clb_%zu_rr_node_%d\n", atom_ctx.nlist.net_name(net_id).c_str(), size_t(clb_index), pb_graph_pin->pin_count_in_cluster);
                    fprintf(fpout, "1 1\n\n");
                }
            }
            port = port->next;
        }
	}
}

void print_routing_in_clusters(FILE *fpout, ClusterBlockId clb_index) {
	t_pb_graph_node *pb_graph_node;
	t_pb_graph_node *pb_graph_node_of_pin;
	int max_pb_graph_pin;
	t_pb_graph_pin** pb_graph_pin_lookup;

	auto& cluster_ctx = g_vpr_ctx.clustering();

	/* print routing of clusters */
	pb_graph_pin_lookup = alloc_and_load_pb_graph_pin_lookup_from_index(cluster_ctx.clb_nlist.block_type(clb_index));
	pb_graph_node = cluster_ctx.clb_nlist.block_pb(clb_index)->pb_graph_node;
	max_pb_graph_pin = pb_graph_node->total_pb_pins;
	const auto& pb_route = cluster_ctx.clb_nlist.block_pb(clb_index)->pb_route;

	for(int i = 0; i < max_pb_graph_pin; i++) {
		if(pb_route[i].atom_net_id) {
			int column = 0;
			pb_graph_node_of_pin = pb_graph_pin_lookup[i]->parent_node;

			/* Print interconnect */
			if(!pb_graph_node_of_pin->is_primitive() && pb_route[i].driver_pb_pin_id == OPEN) {
				/* Logic block input pin */
				VTR_ASSERT(pb_graph_node_of_pin->is_root());
				fprintf(fpout, ".names ");
				print_net_name(pb_route[i].atom_net_id, &column, fpout);
				fprintf(fpout, " clb_%zu_rr_node_%d\n", size_t(clb_index), i);
				fprintf(fpout, "1 1\n\n");
			} else if (!pb_graph_node_of_pin->is_primitive() && pb_graph_node_of_pin->is_root()) {
				/* Logic block output pin */
				fprintf(fpout, ".names clb_%zu_rr_node_%d ", size_t(clb_index), pb_route[i].driver_pb_pin_id);
				print_net_name(pb_route[i].atom_net_id, &column, fpout);
				fprintf(fpout, "\n");
				fprintf(fpout, "1 1\n\n");
			} else if (!pb_graph_node_of_pin->is_primitive() || pb_graph_pin_lookup[i]->port->type != OUT_PORT) {
				/* Logic block internal pin */
				fprintf(fpout, ".names clb_%zu_rr_node_%d clb_%zu_rr_node_%d\n", size_t(clb_index), pb_route[i].driver_pb_pin_id, size_t(clb_index), i);
				fprintf(fpout, "1 1\n\n");
			}
		}
	}

	free_pb_graph_pin_lookup_from_index(pb_graph_pin_lookup);
}

/* Print list of models to file */
void print_models(FILE *fpout, t_model *user_models) {

	t_model *cur;
	cur = user_models;

	while (cur != nullptr) {
		fprintf(fpout, "\n");
		fprintf(fpout, ".model %s\n", cur->name);

		/* Print input ports */
		t_model_ports *port = cur->inputs;
		if (port == nullptr) {
			fprintf(fpout, ".inputs \n");
		}
		else {
			fprintf(fpout, ".inputs \\\n");
		}
		while (port != nullptr) {
			for (int i = 0; i < port->size; i++) {
				fprintf(fpout, "%s[%d] ", port->name, i);
				if (i % 8 == 4) {
					if (i + 1 < port->size) {
						fprintf(fpout, "\\\n");
					}
				}
			}
			port = port->next;
			if (port != nullptr) {
				fprintf(fpout, "\\\n");
			}
			else {
				fprintf(fpout, "\n");
			}
		}
		/* Print input ports */
		port = cur->outputs;
		if (port == nullptr) {
			fprintf(fpout, ".outputs \n");
		}
		else {
			fprintf(fpout, ".outputs \\\n");
		}
		while (port != nullptr) {
			for (int i = 0; i < port->size; i++) {
				fprintf(fpout, "%s[%d] ", port->name, i);
				if (i % 8 == 4) {
					if (i + 1 < port->size) {
						fprintf(fpout, "\\\n");
					}
				}
			}
			port = port->next;
			if (port != nullptr) {
				fprintf(fpout, "\\\n");
			}
			else {
				fprintf(fpout, "\n");
			}
		}
		fprintf(fpout, ".blackbox\n");
		fprintf(fpout, ".end\n");
		cur = cur->next;
	}
}

/* This routine dumps out the output netlist in a format suitable for   *
* input to vpr.  This routine also dumps out the internal structure of  *
* the cluster, in essentially a graph based format.                     */
void output_blif(const t_arch *arch, const char *out_fname) {
	FILE *fpout;
	int column;

	auto& cluster_ctx = g_vpr_ctx.clustering();

	// Check that there's at least one block that exists
	if(cluster_ctx.clb_nlist.block_pb(ClusterBlockId(0)) == nullptr) {
		return;
	}

    auto& atom_ctx = g_vpr_ctx.atom();

	fpout = vtr::fopen(out_fname, "w");

	column = 0;
	fprintf(fpout, ".model %s\n", atom_ctx.nlist.netlist_name().c_str());

	/* Print out all input and output pads. */
	fprintf(fpout, "\n.inputs ");
	for (auto blk_id : atom_ctx.nlist.blocks()) {
		if (atom_ctx.nlist.block_type(blk_id) == AtomBlockType::INPAD) {
			print_string(atom_ctx.nlist.block_name(blk_id).c_str(), &column, fpout);
		}
	}

	column = 0;
	fprintf(fpout, "\n.outputs ");
	for (auto blk_id : atom_ctx.nlist.blocks()) {
		if (atom_ctx.nlist.block_type(blk_id) == AtomBlockType::OUTPAD) {
			/* remove output prefix "out:" */
			print_string(atom_ctx.nlist.block_name(blk_id).c_str() + 4, &column, fpout);
		}
	}

	column = 0;

	fprintf(fpout, "\n\n");
	fprintf(fpout, ".names unconn\n");
	fprintf(fpout, " 0\n\n");

	/* print out all circuit elements */
	for (auto blk_id : atom_ctx.nlist.blocks()) {
		print_atom_block(fpout, blk_id);
	}

	for(auto clb_index : cluster_ctx.clb_nlist.blocks()) {
		print_routing_in_clusters(fpout, clb_index);
	}

	fprintf(fpout, "\n.end\n");

	print_models(fpout, arch->models);

	fclose(fpout);
}
