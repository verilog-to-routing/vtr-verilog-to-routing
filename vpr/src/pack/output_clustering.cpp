/*
 * Jason Luu 2008
 * Print complex block information to a file
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_digest.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "pugixml.hpp"

#include "globals.h"
#include "atom_netlist.h"
#include "pack_types.h"
#include "cluster_router.h"
#include "pb_type_graph.h"
#include "output_clustering.h"
#include "read_xml_arch_file.h"
#include "vpr_utils.h"

#define LINELENGTH 1024
#define TAB_LENGTH 4

/****************** Static variables local to this module ************************/

static t_pb_graph_pin*** pb_graph_pin_lookup_from_index_by_type = nullptr; /* [0..device_ctx.num_block_types-1][0..num_pb_graph_pins-1] lookup pointer to pb_graph_pin from pb_graph_pin index */

/**************** Subroutine definitions ************************************/

/* Prints out one cluster (clb).  Both the external pins and the *
 * internal connections are printed out.                         */
static void print_stats() {
    int ipin, itype;
    int total_nets_absorbed;
    std::unordered_map<AtomNetId, bool> nets_absorbed;

    int *num_clb_types, *num_clb_inputs_used, *num_clb_outputs_used;

    auto& device_ctx = g_vpr_ctx.device();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    num_clb_types = num_clb_inputs_used = num_clb_outputs_used = nullptr;

    num_clb_types = (int*)vtr::calloc(device_ctx.num_block_types, sizeof(int));
    num_clb_inputs_used = (int*)vtr::calloc(device_ctx.num_block_types, sizeof(int));
    num_clb_outputs_used = (int*)vtr::calloc(device_ctx.num_block_types, sizeof(int));

    for (auto net_id : atom_ctx.nlist.nets()) {
        nets_absorbed[net_id] = true;
    }

    /* Counters used only for statistics purposes. */

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        for (ipin = 0; ipin < cluster_ctx.clb_nlist.block_type(blk_id)->num_pins; ipin++) {
            if (cluster_ctx.clb_nlist.block_pb(blk_id)->pb_route.empty()) {
                ClusterNetId clb_net_id = cluster_ctx.clb_nlist.block_net(blk_id, ipin);
                if (clb_net_id != ClusterNetId::INVALID()) {
                    auto net_id = atom_ctx.lookup.atom_net(clb_net_id);
                    VTR_ASSERT(net_id);
                    nets_absorbed[net_id] = false;
                    if (cluster_ctx.clb_nlist.block_type(blk_id)->class_inf[cluster_ctx.clb_nlist.block_type(blk_id)->pin_class[ipin]].type == RECEIVER) {
                        num_clb_inputs_used[cluster_ctx.clb_nlist.block_type(blk_id)->index]++;
                    } else if (cluster_ctx.clb_nlist.block_type(blk_id)->class_inf[cluster_ctx.clb_nlist.block_type(blk_id)->pin_class[ipin]].type == DRIVER) {
                        num_clb_outputs_used[cluster_ctx.clb_nlist.block_type(blk_id)->index]++;
                    }
                }
            } else {
                int pb_graph_pin_id = get_pb_graph_node_pin_from_block_pin(blk_id, ipin)->pin_count_in_cluster;

                const t_pb* pb = cluster_ctx.clb_nlist.block_pb(blk_id);
                if (pb->pb_route.count(pb_graph_pin_id)) {
                    //Pin used
                    auto atom_net_id = pb->pb_route[pb_graph_pin_id].atom_net_id;
                    if (atom_net_id) {
                        nets_absorbed[atom_net_id] = false;
                        if (cluster_ctx.clb_nlist.block_type(blk_id)->class_inf[cluster_ctx.clb_nlist.block_type(blk_id)->pin_class[ipin]].type == RECEIVER) {
                            num_clb_inputs_used[cluster_ctx.clb_nlist.block_type(blk_id)->index]++;
                        } else if (cluster_ctx.clb_nlist.block_type(blk_id)->class_inf[cluster_ctx.clb_nlist.block_type(blk_id)->pin_class[ipin]].type == DRIVER) {
                            num_clb_outputs_used[cluster_ctx.clb_nlist.block_type(blk_id)->index]++;
                        }
                    }
                }
            }
        }
        num_clb_types[cluster_ctx.clb_nlist.block_type(blk_id)->index]++;
    }

    for (itype = 0; itype < device_ctx.num_block_types; itype++) {
        if (num_clb_types[itype] == 0) {
            VTR_LOG("\t%s: # blocks: %d, average # input + clock pins used: %g, average # output pins used: %g\n",
                    device_ctx.block_types[itype].name, num_clb_types[itype], 0.0, 0.0);
        } else {
            VTR_LOG("\t%s: # blocks: %d, average # input + clock pins used: %g, average # output pins used: %g\n",
                    device_ctx.block_types[itype].name, num_clb_types[itype],
                    (float)num_clb_inputs_used[itype] / (float)num_clb_types[itype],
                    (float)num_clb_outputs_used[itype] / (float)num_clb_types[itype]);
        }
    }

    total_nets_absorbed = 0;
    for (auto net_id : atom_ctx.nlist.nets()) {
        if (nets_absorbed[net_id] == true) {
            total_nets_absorbed++;
        }
    }
    VTR_LOG("Absorbed logical nets %d out of %d nets, %d nets not absorbed.\n",
            total_nets_absorbed, (int)atom_ctx.nlist.nets().size(), (int)atom_ctx.nlist.nets().size() - total_nets_absorbed);
    free(num_clb_types);
    free(num_clb_inputs_used);
    free(num_clb_outputs_used);
    /* TODO: print more stats */
}

static const char* clustering_xml_net_text(AtomNetId net_id) {
    /* This routine prints out the atom_ctx.nlist net name (or open).
     * net_num is the index of the atom_ctx.nlist net to be printed
     */

    if (!net_id) {
        return "open";
    } else {
        auto& atom_ctx = g_vpr_ctx.atom();
        return atom_ctx.nlist.net_name(net_id).c_str();
    }
}

static std::string clustering_xml_interconnect_text(t_type_ptr type, int inode, const t_pb_routes& pb_route) {
    if (!pb_route.count(inode) || !pb_route[inode].atom_net_id) {
        return "open";
    }

    int prev_node = pb_route[inode].driver_pb_pin_id;
    int prev_edge;
    if (prev_node == OPEN) {
        /* No previous driver implies that this is either a top-level input pin or a primitive output pin */
        t_pb_graph_pin* cur_pin = pb_graph_pin_lookup_from_index_by_type[type->index][inode];
        VTR_ASSERT(cur_pin->parent_node->pb_type->parent_mode == nullptr || (cur_pin->is_primitive_pin() && cur_pin->port->type == OUT_PORT));
        return clustering_xml_net_text(pb_route[inode].atom_net_id);
    } else {
        t_pb_graph_pin* cur_pin = pb_graph_pin_lookup_from_index_by_type[type->index][inode];
        t_pb_graph_pin* prev_pin = pb_graph_pin_lookup_from_index_by_type[type->index][prev_node];

        for (prev_edge = 0; prev_edge < prev_pin->num_output_edges; prev_edge++) {
            VTR_ASSERT(prev_pin->output_edges[prev_edge]->num_output_pins == 1);
            if (prev_pin->output_edges[prev_edge]->output_pins[0]->pin_count_in_cluster == inode) {
                break;
            }
        }
        VTR_ASSERT(prev_edge < prev_pin->num_output_edges);

        char* name = prev_pin->output_edges[prev_edge]->interconnect->name;
        if (prev_pin->port->parent_pb_type->depth
            >= cur_pin->port->parent_pb_type->depth) {
            /* Connections from siblings or children should have an explicit index, connections from parent does not need an explicit index */
            return vtr::string_fmt("%s[%d].%s[%d]->%s",
                                   prev_pin->parent_node->pb_type->name,
                                   prev_pin->parent_node->placement_index,
                                   prev_pin->port->name,
                                   prev_pin->pin_number, name);
        } else {
            return vtr::string_fmt("%s.%s[%d]->%s",
                                   prev_pin->parent_node->pb_type->name,
                                   prev_pin->port->name,
                                   prev_pin->pin_number, name);
        }
    }
}

/* outputs a block that is open or unused.
 * In some cases, a block is unused for logic but is used for routing. When that happens, the block
 * cannot simply be marked open as that would lose the routing information. Instead, a block must be
 * output that reflects the routing resources used. This function handles both cases.
 */
static void clustering_xml_open_block(pugi::xml_node parent_node, t_type_ptr type, t_pb_graph_node* pb_graph_node, int pb_index, bool is_used, const t_pb_routes& pb_route) {
    int i, j, k, m;
    const t_pb_type *pb_type, *child_pb_type;
    t_mode* mode = nullptr;
    int prev_node;
    int mode_of_edge, port_index, node_index;

    mode_of_edge = UNDEFINED;

    pb_type = pb_graph_node->pb_type;

    pugi::xml_node block_node = parent_node.append_child("block");
    block_node.append_attribute("name") = "open";
    block_node.append_attribute("instance") = vtr::string_fmt("%s[%d]", pb_graph_node->pb_type->name, pb_index).c_str();
    std::vector<std::string> block_modes;

    if (is_used) {
        /* Determine mode if applicable */
        port_index = 0;
        for (i = 0; i < pb_type->num_ports; i++) {
            if (pb_type->ports[i].type == OUT_PORT) {
                VTR_ASSERT(!pb_type->ports[i].is_clock);
                for (j = 0; j < pb_type->ports[i].num_pins; j++) {
                    const t_pb_graph_pin* pin = &pb_graph_node->output_pins[port_index][j];
                    node_index = pin->pin_count_in_cluster;
                    if (pb_type->num_modes > 0 && pb_route.count(node_index) && pb_route[node_index].atom_net_id) {
                        prev_node = pb_route[node_index].driver_pb_pin_id;
                        const t_pb_graph_pin* prev_pin = pb_graph_pin_lookup_from_index_by_type[type->index][prev_node];
                        const t_pb_graph_edge* edge = get_edge_between_pins(prev_pin, pin);

                        VTR_ASSERT(edge != nullptr);
                        mode_of_edge = edge->interconnect->parent_mode_index;
                        if (mode != nullptr && &pb_type->modes[mode_of_edge] != mode) {
                            vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
                                      "Differing modes for block.  Got %s previously and %s for edge %d (interconnect %s).",
                                      mode->name, pb_type->modes[mode_of_edge].name,
                                      port_index,
                                      edge->interconnect->name);
                        }
                        VTR_ASSERT(mode == nullptr || &pb_type->modes[mode_of_edge] == mode);
                        mode = &pb_type->modes[mode_of_edge];
                    }
                }
                port_index++;
            }
        }

        VTR_ASSERT(mode != nullptr && mode_of_edge != UNDEFINED);

        block_node.append_attribute("mode") = mode->name;
        block_node.append_attribute("pb_type_num_modes") = pb_type->num_modes;

        pugi::xml_node inputs_node = block_node.append_child("inputs");

        port_index = 0;
        for (i = 0; i < pb_type->num_ports; i++) {
            if (!pb_type->ports[i].is_clock && pb_type->ports[i].type == IN_PORT) {
                pugi::xml_node port_node = inputs_node.append_child("port");
                port_node.append_attribute("name") = pb_graph_node->pb_type->ports[i].name;

                std::vector<std::string> pins;
                for (j = 0; j < pb_type->ports[i].num_pins; j++) {
                    node_index = pb_graph_node->input_pins[port_index][j].pin_count_in_cluster;

                    if (pb_type->parent_mode == nullptr) {
                        pins.push_back(clustering_xml_net_text(pb_route[node_index].atom_net_id));
                    } else {
                        pins.push_back(clustering_xml_interconnect_text(type, node_index, pb_route));
                    }
                }
                port_node.text().set(vtr::join(pins.begin(), pins.end(), " ").c_str());
                port_index++;
            }
        }

        pugi::xml_node outputs_node = block_node.append_child("outputs");

        port_index = 0;
        for (i = 0; i < pb_type->num_ports; i++) {
            if (pb_type->ports[i].type == OUT_PORT) {
                VTR_ASSERT(!pb_type->ports[i].is_clock);

                pugi::xml_node port_node = outputs_node.append_child("port");
                port_node.append_attribute("name") = pb_graph_node->pb_type->ports[i].name;
                std::vector<std::string> pins;
                for (j = 0; j < pb_type->ports[i].num_pins; j++) {
                    node_index = pb_graph_node->output_pins[port_index][j].pin_count_in_cluster;
                    pins.push_back(clustering_xml_interconnect_text(type, node_index, pb_route));
                }
                port_node.text().set(vtr::join(pins.begin(), pins.end(), " ").c_str());
                port_index++;
            }
        }

        pugi::xml_node clock_node = block_node.append_child("clocks");

        port_index = 0;
        for (i = 0; i < pb_type->num_ports; i++) {
            if (pb_type->ports[i].is_clock && pb_type->ports[i].type == IN_PORT) {
                pugi::xml_node port_node = clock_node.append_child("port");
                port_node.append_attribute("name") = pb_graph_node->pb_type->ports[i].name;

                std::vector<std::string> pins;
                for (j = 0; j < pb_type->ports[i].num_pins; j++) {
                    node_index = pb_graph_node->clock_pins[port_index][j].pin_count_in_cluster;
                    if (pb_type->parent_mode == nullptr) {
                        pins.push_back(clustering_xml_net_text(pb_route[node_index].atom_net_id));
                    } else {
                        pins.push_back(clustering_xml_interconnect_text(type, node_index, pb_route));
                    }
                }
                port_node.text().set(vtr::join(pins.begin(), pins.end(), " ").c_str());
                port_index++;
            }
        }

        if (pb_type->num_modes > 0) {
            for (i = 0; i < mode->num_pb_type_children; i++) {
                child_pb_type = &mode->pb_type_children[i];
                for (j = 0; j < mode->pb_type_children[i].num_pb; j++) {
                    port_index = 0;
                    is_used = false;
                    for (k = 0; k < child_pb_type->num_ports && !is_used; k++) {
                        if (child_pb_type->ports[k].type == OUT_PORT) {
                            for (m = 0; m < child_pb_type->ports[k].num_pins; m++) {
                                node_index = pb_graph_node->child_pb_graph_nodes[mode_of_edge][i][j].output_pins[port_index][m].pin_count_in_cluster;
                                if (pb_route.count(node_index) && pb_route[node_index].atom_net_id) {
                                    is_used = true;
                                    break;
                                }
                            }
                            port_index++;
                        }
                    }
                    clustering_xml_open_block(block_node, type,
                                              &pb_graph_node->child_pb_graph_nodes[mode_of_edge][i][j],
                                              j, is_used, pb_route);
                }
            }
        }
    }
}

/* outputs a block that is used (i.e. has configuration) and all of its child blocks */
static void clustering_xml_block(pugi::xml_node parent_node, t_type_ptr type, t_pb* pb, int pb_index, const t_pb_routes& pb_route) {
    int i, j, k, m;
    const t_pb_type *pb_type, *child_pb_type;
    t_pb_graph_node* pb_graph_node;
    t_mode* mode;
    int port_index, node_index;
    bool is_used;

    pb_type = pb->pb_graph_node->pb_type;
    pb_graph_node = pb->pb_graph_node;
    mode = &pb_type->modes[pb->mode];

    pugi::xml_node block_node = parent_node.append_child("block");
    block_node.append_attribute("name") = pb->name;
    block_node.append_attribute("instance") = vtr::string_fmt("%s[%d]", pb_type->name, pb_index).c_str();

    if (pb_type->num_modes > 0) {
        block_node.append_attribute("mode") = mode->name;
    } else {
        const auto& atom_ctx = g_vpr_ctx.atom();
        AtomBlockId atom_blk = atom_ctx.nlist.find_block(pb->name);
        VTR_ASSERT(atom_blk);

        pugi::xml_node attrs_node = block_node.append_child("attributes");
        for (const auto& attr : atom_ctx.nlist.block_attrs(atom_blk)) {
            pugi::xml_node attr_node = attrs_node.append_child("attribute");
            attr_node.append_attribute("name") = attr.first.c_str();
            attr_node.text().set(attr.second.c_str());
        }

        pugi::xml_node params_node = block_node.append_child("parameters");
        for (const auto& param : atom_ctx.nlist.block_params(atom_blk)) {
            pugi::xml_node param_node = params_node.append_child("parameter");
            param_node.append_attribute("name") = param.first.c_str();
            param_node.text().set(param.second.c_str());
        }
    }

    pugi::xml_node inputs_node = block_node.append_child("inputs");

    port_index = 0;
    for (i = 0; i < pb_type->num_ports; i++) {
        if (!pb_type->ports[i].is_clock && pb_type->ports[i].type == IN_PORT) {
            pugi::xml_node port_node = inputs_node.append_child("port");
            port_node.append_attribute("name") = pb_graph_node->pb_type->ports[i].name;

            std::vector<std::string> pins;
            for (j = 0; j < pb_type->ports[i].num_pins; j++) {
                node_index = pb->pb_graph_node->input_pins[port_index][j].pin_count_in_cluster;

                if (pb_type->parent_mode == nullptr) {
                    if (pb_route.count(node_index)) {
                        pins.push_back(clustering_xml_net_text(pb_route[node_index].atom_net_id));
                    } else {
                        pins.push_back(clustering_xml_net_text(AtomNetId::INVALID()));
                    }
                } else {
                    pins.push_back(clustering_xml_interconnect_text(type, node_index, pb_route));
                }
            }
            port_node.text().set(vtr::join(pins.begin(), pins.end(), " ").c_str());

            //The cluster router may have rotated equivalent pins (e.g. LUT inputs),
            //record the resulting rotation here so it can be unambigously mapped
            //back to the atom netlist
            if (pb_type->ports[i].equivalent != PortEquivalence::NONE && pb_type->parent_mode != nullptr && pb_type->num_modes == 0) {
                //This is a primitive with equivalent inputs

                auto& atom_ctx = g_vpr_ctx.atom();
                AtomBlockId atom_blk = atom_ctx.nlist.find_block(pb->name);
                VTR_ASSERT(atom_blk);

                AtomPortId atom_port = atom_ctx.nlist.find_atom_port(atom_blk, pb_type->ports[i].model_port);

                if (atom_port) { //Port exists (some LUTs may have no input and hence no port in the atom netlist)

                    pugi::xml_node port_rotation_node = inputs_node.append_child("port_rotation_map");
                    port_rotation_node.append_attribute("name") = pb_graph_node->pb_type->ports[i].name;

                    std::set<AtomPinId> recorded_pins;
                    std::vector<std::string> pin_map_list;

                    for (j = 0; j < pb_type->ports[i].num_pins; j++) {
                        node_index = pb->pb_graph_node->input_pins[port_index][j].pin_count_in_cluster;

                        if (pb_route.count(node_index)) {
                            AtomNetId atom_net = pb_route[node_index].atom_net_id;

                            VTR_ASSERT(atom_net);

                            //This physical pin is in use, find the original pin in the atom netlist
                            AtomPinId orig_pin;
                            for (AtomPinId atom_pin : atom_ctx.nlist.port_pins(atom_port)) {
                                if (recorded_pins.count(atom_pin)) continue; //Don't add pins twice

                                AtomNetId atom_pin_net = atom_ctx.nlist.pin_net(atom_pin);

                                if (atom_pin_net == atom_net) {
                                    recorded_pins.insert(atom_pin);
                                    orig_pin = atom_pin;
                                    break;
                                }
                            }

                            VTR_ASSERT(orig_pin);
                            //The physical pin j, maps to a pin in the atom netlist
                            pin_map_list.push_back(vtr::string_fmt("%d", atom_ctx.nlist.pin_port_bit(orig_pin)));
                        } else {
                            //The physical pin is disconnected
                            pin_map_list.push_back("open");
                        }
                    }
                    port_rotation_node.text().set(vtr::join(pin_map_list.begin(), pin_map_list.end(), " ").c_str());
                }
            }

            port_index++;
        }
    }

    pugi::xml_node outputs_node = block_node.append_child("outputs");

    port_index = 0;
    for (i = 0; i < pb_type->num_ports; i++) {
        if (pb_type->ports[i].type == OUT_PORT) {
            VTR_ASSERT(!pb_type->ports[i].is_clock);

            pugi::xml_node port_node = outputs_node.append_child("port");
            port_node.append_attribute("name") = pb_graph_node->pb_type->ports[i].name;
            std::vector<std::string> pins;
            for (j = 0; j < pb_type->ports[i].num_pins; j++) {
                node_index = pb->pb_graph_node->output_pins[port_index][j].pin_count_in_cluster;
                pins.push_back(clustering_xml_interconnect_text(type, node_index, pb_route));
            }
            port_node.text().set(vtr::join(pins.begin(), pins.end(), " ").c_str());
            port_index++;
        }
    }

    pugi::xml_node clock_node = block_node.append_child("clocks");

    port_index = 0;
    for (i = 0; i < pb_type->num_ports; i++) {
        if (pb_type->ports[i].is_clock && pb_type->ports[i].type == IN_PORT) {
            pugi::xml_node port_node = clock_node.append_child("port");
            port_node.append_attribute("name") = pb_graph_node->pb_type->ports[i].name;

            std::vector<std::string> pins;
            for (j = 0; j < pb_type->ports[i].num_pins; j++) {
                node_index = pb->pb_graph_node->clock_pins[port_index][j].pin_count_in_cluster;
                if (pb_type->parent_mode == nullptr) {
                    if (pb_route.count(node_index)) {
                        pins.push_back(clustering_xml_net_text(pb_route[node_index].atom_net_id));
                    } else {
                        pins.push_back(clustering_xml_net_text(AtomNetId::INVALID()));
                    }
                } else {
                    pins.push_back(clustering_xml_interconnect_text(type, node_index, pb_route));
                }
            }
            port_node.text().set(vtr::join(pins.begin(), pins.end(), " ").c_str());
            port_index++;
        }
    }

    if (pb_type->num_modes > 0) {
        for (i = 0; i < mode->num_pb_type_children; i++) {
            for (j = 0; j < mode->pb_type_children[i].num_pb; j++) {
                /* If child pb is not used but routing is used, I must print things differently */
                if ((pb->child_pbs[i] != nullptr) && (pb->child_pbs[i][j].name != nullptr)) {
                    clustering_xml_block(block_node, type, &pb->child_pbs[i][j], j, pb_route);
                } else {
                    is_used = false;
                    child_pb_type = &mode->pb_type_children[i];
                    port_index = 0;

                    for (k = 0; k < child_pb_type->num_ports && !is_used; k++) {
                        if (child_pb_type->ports[k].type == OUT_PORT) {
                            for (m = 0; m < child_pb_type->ports[k].num_pins; m++) {
                                node_index = pb_graph_node->child_pb_graph_nodes[pb->mode][i][j].output_pins[port_index][m].pin_count_in_cluster;
                                if (pb_route.count(node_index) && pb_route[node_index].atom_net_id) {
                                    is_used = true;
                                    break;
                                }
                            }
                            port_index++;
                        }
                    }
                    clustering_xml_open_block(block_node, type,
                                              &pb_graph_node->child_pb_graph_nodes[pb->mode][i][j],
                                              j, is_used, pb_route);
                }
            }
        }
    }
}

/* This routine dumps out the output netlist in a format suitable for  *
 * input to vpr. This routine also dumps out the internal structure of *
 * the cluster, in essentially a graph based format.                   */
void output_clustering(const vtr::vector<ClusterBlockId, std::vector<t_intra_lb_net>*>& intra_lb_routing, bool global_clocks, const std::unordered_set<AtomNetId>& is_clock, const std::string& architecture_id, const char* out_fname, bool skip_clustering) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    if (!intra_lb_routing.empty()) {
        VTR_ASSERT(intra_lb_routing.size() == cluster_ctx.clb_nlist.blocks().size());
        for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
            cluster_ctx.clb_nlist.block_pb(blk_id)->pb_route = alloc_and_load_pb_route(intra_lb_routing[blk_id], cluster_ctx.clb_nlist.block_pb(blk_id)->pb_graph_node);
        }
    }

    pb_graph_pin_lookup_from_index_by_type = new t_pb_graph_pin**[device_ctx.num_block_types];
    for (int itype = 0; itype < device_ctx.num_block_types; itype++) {
        pb_graph_pin_lookup_from_index_by_type[itype] = alloc_and_load_pb_graph_pin_lookup_from_index(&device_ctx.block_types[itype]);
    }

    pugi::xml_document out_xml;

    pugi::xml_node block_node = out_xml.append_child("block");
    block_node.append_attribute("name") = out_fname;
    block_node.append_attribute("instance") = "FPGA_packed_netlist[0]";
    block_node.append_attribute("architecture_id") = architecture_id.c_str();
    block_node.append_attribute("atom_netlist_id") = atom_ctx.nlist.netlist_id().c_str();

    std::vector<std::string> inputs;
    std::vector<std::string> outputs;

    for (auto blk_id : atom_ctx.nlist.blocks()) {
        auto type = atom_ctx.nlist.block_type(blk_id);
        switch (type) {
            case AtomBlockType::INPAD:
                if (skip_clustering) {
                    VTR_ASSERT(0);
                }
                inputs.push_back(atom_ctx.nlist.block_name(blk_id));
                break;

            case AtomBlockType::OUTPAD:
                if (skip_clustering) {
                    VTR_ASSERT(0);
                }
                outputs.push_back(atom_ctx.nlist.block_name(blk_id));
                break;

            case AtomBlockType::BLOCK:
                if (skip_clustering) {
                    VTR_ASSERT(0);
                }
                break;

            default:
                VTR_LOG_ERROR("in output_netlist: Unexpected type %d for atom block %s.\n",
                              type, atom_ctx.nlist.block_name(blk_id).c_str());
        }
    }

    block_node.append_child("inputs").text().set(vtr::join(inputs.begin(), inputs.end(), " ").c_str());
    block_node.append_child("outputs").text().set(vtr::join(outputs.begin(), outputs.end(), " ").c_str());

    if (global_clocks) {
        std::vector<std::string> clocks;
        for (auto net_id : atom_ctx.nlist.nets()) {
            if (is_clock.count(net_id)) {
                clocks.push_back(atom_ctx.nlist.net_name(net_id));
            }
        }

        block_node.append_child("clocks").text().set(vtr::join(clocks.begin(), clocks.end(), " ").c_str());
    }

    if (skip_clustering == false) {
        for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
            /* TODO: Must do check that total CLB pins match top-level pb pins, perhaps check this earlier? */
            clustering_xml_block(block_node, cluster_ctx.clb_nlist.block_type(blk_id), cluster_ctx.clb_nlist.block_pb(blk_id), size_t(blk_id), cluster_ctx.clb_nlist.block_pb(blk_id)->pb_route);
        }
    }

    out_xml.save_file(out_fname);

    print_stats();

    if (!intra_lb_routing.empty()) {
        for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
            cluster_ctx.clb_nlist.block_pb(blk_id)->pb_route.clear();
        }
    }

    for (int itype = 0; itype < device_ctx.num_block_types; itype++) {
        free_pb_graph_pin_lookup_from_index(pb_graph_pin_lookup_from_index_by_type[itype]);
    }
    delete[] pb_graph_pin_lookup_from_index_by_type;
}
