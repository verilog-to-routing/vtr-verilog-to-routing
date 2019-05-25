/**
 * Author: Jason Luu
 * Date: May 2009
 *
 * Read a circuit netlist in XML format and populate the netlist data structures for VPR
 */

#include <cstdio>
#include <cstring>
#include <ctime>
#include <map>
using namespace std;

#include "pugixml.hpp"
#include "pugixml_loc.hpp"
#include "pugixml_util.hpp"

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_log.h"
#include "vtr_digest.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"

#include "hash.h"
#include "globals.h"
#include "atom_netlist.h"
#include "read_xml_util.h"
#include "read_netlist.h"
#include "pb_type_graph.h"
#include "token.h"

static const char* netlist_file_name = nullptr;

static int processPorts(pugi::xml_node Parent, t_pb* pb, t_pb_routes& pb_route, const pugiutil::loc_data& loc_data);

static void processPb(pugi::xml_node Parent, const ClusterBlockId index, t_pb* pb, t_pb_routes& pb_route, int* num_primitives, const pugiutil::loc_data& loc_data, ClusteredNetlist* clb_nlist);

static void processComplexBlock(pugi::xml_node Parent,
                                const ClusterBlockId index,
                                int* num_primitives,
                                const pugiutil::loc_data& loc_data,
                                ClusteredNetlist* clb_nlist);

static int add_net_to_hash(t_hash** nhash, const char* net_name, int* ncount);

static void load_external_nets_and_cb(ClusteredNetlist& clb_nlist);

static void load_internal_to_block_net_nums(const t_type_ptr type, t_pb_routes& pb_route);

static void load_atom_index_for_pb_pin(t_pb_routes& pb_route, int ipin);

static void mark_constant_generators(const ClusteredNetlist& clb_nlist, int verbosity);

static size_t mark_constant_generators_rec(const t_pb* pb, const t_pb_routes& pb_route, int verbosity);

static t_pb_routes alloc_pb_route(t_pb_graph_node* pb_graph_node);

static void load_atom_pin_mapping(const ClusteredNetlist& clb_nlist);
static void set_atom_pin_mapping(const ClusteredNetlist& clb_nlist, const AtomBlockId atom_blk, const AtomPortId atom_port, const t_pb_graph_pin* gpin);

/**
 * Initializes the clb_nlist with info from a netlist
 * net_file - Name of the netlist file to read
 */
ClusteredNetlist read_netlist(const char* net_file,
                              const t_arch* arch,
                              bool verify_file_digests,
                              int verbosity) {
    clock_t begin = clock();
    size_t bcount = 0;
    std::vector<std::string> circuit_inputs, circuit_outputs, circuit_clocks;

    auto& atom_ctx = g_vpr_ctx.mutable_atom();

    int num_primitives = 0;

    /* Parse the file */
    VTR_LOG("Begin loading packed FPGA netlist file.\n");

    //Save an identifier for the netlist based on it's contents
    auto clb_nlist = ClusteredNetlist(net_file, vtr::secure_digest_file(net_file));

    pugi::xml_document doc;
    pugiutil::loc_data loc_data;
    try {
        loc_data = pugiutil::load_xml(doc, net_file);
    } catch (pugiutil::XmlError& e) {
        vpr_throw(VPR_ERROR_NET_F, net_file, 0,
                  "Failed to load netlist file '%s' (%s).\n", net_file, e.what());
    }

    try {
        /* Save netlist file's name in file-scoped variable */
        netlist_file_name = net_file;

        /* Root node should be block */
        auto top = doc.child("block");
        if (!top) {
            vpr_throw(VPR_ERROR_NET_F, net_file, loc_data.line(top),
                      "Root element must be 'block'.\n");
        }

        /* Check top-level netlist attributes */
        auto top_name = top.attribute("name");
        if (!top_name) {
            vpr_throw(VPR_ERROR_NET_F, net_file, loc_data.line(top),
                      "Root element must have a 'name' attribute.\n");
        }

        VTR_LOG("Netlist generated from file '%s'.\n", top_name.value());

        //Verify top level attributes
        auto top_instance = pugiutil::get_attribute(top, "instance", loc_data);

        if (strcmp(top_instance.value(), "FPGA_packed_netlist[0]") != 0) {
            vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(top),
                      "Expected top instance to be \"FPGA_packed_netlist[0]\", found \"%s\".",
                      top_instance.value());
        }

        auto architecture_id = top.attribute("architecture_id");
        if (architecture_id) {
            //Netlist file has an architecture id, make sure it is
            //consistent with the loaded architecture file.
            //
            //Note that we currently don't require that the architecture_id exists,
            //to remain compatible with old .net files
            std::string arch_id = architecture_id.value();
            if (arch_id != arch->architecture_id) {
                auto msg = vtr::string_fmt(
                    "Netlist was generated from a different architecture file"
                    " (loaded architecture ID: %s, netlist file architecture ID: %s)",
                    arch->architecture_id, arch_id.c_str());
                if (verify_file_digests) {
                    vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(top), msg.c_str());
                } else {
                    VTR_LOGF_WARN(netlist_file_name, loc_data.line(top), "%s\n", msg.c_str());
                }
            }
        }

        auto atom_netlist_id = top.attribute("atom_netlist_id");
        if (atom_netlist_id) {
            //Netlist file has an_atom netlist_id, make sure it is
            //consistent with the loaded atom netlist.
            //
            //Note that we currently don't require that the atom_netlist_id exists,
            //to remain compatible with old .net files
            std::string atom_nl_id = atom_netlist_id.value();
            if (atom_nl_id != atom_ctx.nlist.netlist_id()) {
                auto msg = vtr::string_fmt(
                    "Netlist was generated from a different atom netlist file"
                    " (loaded atom netlist ID: %s, packed netlist atom netlist ID: %s)",
                    atom_nl_id.c_str(), atom_ctx.nlist.netlist_id().c_str());
                if (verify_file_digests) {
                    vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(top), msg.c_str());
                } else {
                    VTR_LOGF_WARN(netlist_file_name, loc_data.line(top), "%s\n", msg.c_str());
                }
            }
        }

        //Collect top level I/Os
        auto top_inputs = pugiutil::get_single_child(top, "inputs", loc_data);
        circuit_inputs = vtr::split(top_inputs.text().get());

        auto top_outputs = pugiutil::get_single_child(top, "outputs", loc_data);
        circuit_outputs = vtr::split(top_outputs.text().get());

        auto top_clocks = pugiutil::get_single_child(top, "clocks", loc_data);
        circuit_clocks = vtr::split(top_clocks.text().get());

        /* Parse all CLB blocks and all nets*/

        //Reset atom/pb mapping (it is reloaded from the packed netlist file)
        for (auto blk_id : atom_ctx.nlist.blocks())
            atom_ctx.lookup.set_atom_pb(blk_id, nullptr);

        //Count the number of blocks for allocation
        bcount = pugiutil::count_children(top, "block", loc_data, pugiutil::ReqOpt::OPTIONAL);
        if (bcount == 0)
            VTR_LOG_WARN("Packed netlist contains no clustered blocks\n");

        /* Process netlist */
        unsigned i = 0;
        for (auto curr_block = top.child("block"); curr_block; curr_block = curr_block.next_sibling("block")) {
            processComplexBlock(curr_block, ClusterBlockId(i), &num_primitives, loc_data, &clb_nlist);
            i++;
        }
        VTR_ASSERT(bcount == i);
        VTR_ASSERT(clb_nlist.blocks().size() == i);
        VTR_ASSERT(num_primitives >= 0);
        VTR_ASSERT(static_cast<size_t>(num_primitives) == atom_ctx.nlist.blocks().size());

        /* Error check */
        for (auto blk_id : atom_ctx.nlist.blocks()) {
            if (atom_ctx.lookup.atom_pb(blk_id) == nullptr) {
                vpr_throw(VPR_ERROR_NET_F, __FILE__, __LINE__,
                          ".blif file and .net file do not match, .net file missing atom %s.\n",
                          atom_ctx.nlist.block_name(blk_id).c_str());
            }
        }
        /* TODO: Add additional check to make sure net connections match */
        mark_constant_generators(clb_nlist, verbosity);

        load_external_nets_and_cb(clb_nlist);
    } catch (pugiutil::XmlError& e) {
        vpr_throw(VPR_ERROR_NET_F, e.filename_c_str(), e.line(),
                  "Error loading post-pack netlist (%s)", e.what());
    }

    /* TODO: create this function later
     * check_top_IO_matches_IO_blocks(circuit_inputs, circuit_outputs, circuit_clocks, blist, bcount); */

    /* load mapping between external nets and all nets */
    for (auto net_id : atom_ctx.nlist.nets()) {
        atom_ctx.lookup.set_atom_clb_net(net_id, ClusterNetId::INVALID());
    }

    //Save the mapping between clb and atom nets
    for (auto clb_net_id : clb_nlist.nets()) {
        AtomNetId net_id = atom_ctx.nlist.find_net(clb_nlist.net_name(clb_net_id));
        VTR_ASSERT(net_id);
        atom_ctx.lookup.set_atom_clb_net(net_id, clb_net_id);
    }

    /* load mapping between atom pins and pb_graph_pins */
    load_atom_pin_mapping(clb_nlist);

    clock_t end = clock();

    VTR_LOG("Finished loading packed FPGA netlist file (took %g seconds).\n", (float)(end - begin) / CLOCKS_PER_SEC);

    size_t num_pb_route_used = 0;
    size_t num_pb_route_alloc = 0;
    size_t num_pb_pins = 0;
    for (auto clb : clb_nlist.blocks()) {
        t_pb* pb = clb_nlist.block_pb(clb);

        for (int ipin = 0; ipin < pb->pb_graph_node->total_pb_pins; ++ipin) {
            if (pb->pb_route.count(ipin)) {
                ++num_pb_route_alloc;
                if (pb->pb_route[ipin].atom_net_id) {
                    ++num_pb_route_used;
                }
            }
            ++num_pb_pins;
        }
    }

    return clb_nlist;
}

/**
 * XML parser to populate CLB info and to update nets with the nets of this CLB
 * Parent - XML tag for this CLB
 * clb - Array of CLBs in the netlist
 * index - index of the CLB to allocate and load information into
 * loc_data - xml location info for error reporting
 */
static void processComplexBlock(pugi::xml_node clb_block,
                                const ClusterBlockId index,
                                int* num_primitives,
                                const pugiutil::loc_data& loc_data,
                                ClusteredNetlist* clb_nlist) {
    bool found;
    int i, num_tokens = 0;
    t_token* tokens;
    const t_pb_type* pb_type = nullptr;

    auto& device_ctx = g_vpr_ctx.device();
    auto& atom_ctx = g_vpr_ctx.mutable_atom();

    //Parse cb attributes
    auto block_name = pugiutil::get_attribute(clb_block, "name", loc_data);
    auto block_inst = pugiutil::get_attribute(clb_block, "instance", loc_data);
    tokens = GetTokensFromString(block_inst.value(), &num_tokens);
    if (num_tokens != 4 || tokens[0].type != TOKEN_STRING
        || tokens[1].type != TOKEN_OPEN_SQUARE_BRACKET
        || tokens[2].type != TOKEN_INT
        || tokens[3].type != TOKEN_CLOSE_SQUARE_BRACKET) {
        vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(clb_block),
                  "Unknown syntax for instance %s in %s. Expected pb_type[instance_number].\n",
                  block_inst.value(), clb_block.name());
    }
    VTR_ASSERT(ClusterBlockId(vtr::atoi(tokens[2].data)) == index);

    found = false;
    for (i = 0; i < device_ctx.num_block_types; i++) {
        if (strcmp(device_ctx.block_types[i].name, tokens[0].data) == 0) {
            t_pb* pb = new t_pb;
            pb->name = vtr::strdup(block_name.value());
            clb_nlist->create_block(block_name.value(), pb, &device_ctx.block_types[i]);
            pb_type = clb_nlist->block_type(index)->pb_type;
            found = true;
            break;
        }
    }
    if (!found) {
        vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(clb_block),
                  "Unknown cb type %s for cb %s #%lu.\n", block_inst.value(), clb_nlist->block_name(index).c_str(), size_t(index));
    }

    //Parse all pbs and CB internal nets
    atom_ctx.lookup.set_atom_pb(AtomBlockId::INVALID(), clb_nlist->block_pb(index));

    clb_nlist->block_pb(index)->pb_graph_node = clb_nlist->block_type(index)->pb_graph_head;
    clb_nlist->block_pb(index)->pb_route = alloc_pb_route(clb_nlist->block_pb(index)->pb_graph_node);

    auto clb_mode = pugiutil::get_attribute(clb_block, "mode", loc_data);

    found = false;
    for (i = 0; i < pb_type->num_modes; i++) {
        if (strcmp(clb_mode.value(), pb_type->modes[i].name) == 0) {
            clb_nlist->block_pb(index)->mode = i;
            found = true;
        }
    }
    if (!found) {
        vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(clb_block),
                  "Unknown mode %s for cb %s #%d.\n", clb_mode.value(), clb_nlist->block_name(index).c_str(), index);
    }

    processPb(clb_block, index, clb_nlist->block_pb(index), clb_nlist->block_pb(index)->pb_route, num_primitives, loc_data, clb_nlist);
    load_internal_to_block_net_nums(clb_nlist->block_type(index), clb_nlist->block_pb(index)->pb_route);

    //clb_nlist->block_pb(index)->pb_route.shrink_to_fit();

    freeTokens(tokens, num_tokens);
}

/**
 * This processes a set of key-value pairs in the XML e.g. block attributes or parameters,
 * which must be of the form <attributes><attribute name="attrName">attrValue</attribute> ... </attributes>
 */
template<typename T>
void processAttrsParams(pugi::xml_node Parent, const char* child_name, T& atom_net_range, const pugiutil::loc_data& loc_data) {
    std::map<std::string, std::string> kvs;
    if (Parent) {
        for (auto Cur = pugiutil::get_first_child(Parent, child_name, loc_data, pugiutil::OPTIONAL); Cur; Cur = Cur.next_sibling(child_name)) {
            std::string cname = pugiutil::get_attribute(Cur, "name", loc_data).value();
            std::string cval = Cur.text().get();
            bool found = false;
            // Look for corresponding key-value in range from AtomNetlist
            for (auto bitem : atom_net_range) {
                if (bitem.first == cname) {
                    if (bitem.second != cval) {
                        // Found in AtomNetlist range, but values don't match
                        vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(Cur),
                                  ".net file and .blif file do not match, %s %s set to \"%s\" in .net file but \"%s\" in .blif file.\n",
                                  child_name, cname.c_str(), cval.c_str(), bitem.second.c_str());
                    }
                    found = true;
                    break;
                }
            }
            if (!found) // Not found in AtomNetlist range
                vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(Cur),
                          ".net file and .blif file do not match, %s %s missing in .blif file.\n",
                          child_name, cname.c_str());
            kvs[cname] = cval;
        }
    }
    // Check for attrs/params in AtomNetlist but not in .net file
    for (auto bitem : atom_net_range) {
        if (kvs.find(bitem.first) == kvs.end())
            vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(Parent),
                      ".net file and .blif file do not match, %s %s missing in .net file.\n",
                      child_name, bitem.first.c_str());
    }
}

/**
 * XML parser to populate pb info and to update internal nets of the parent CLB
 * Parent - XML tag for this pb_type
 * pb - physical block to use
 * loc_data - xml location info for error reporting
 */
static void processPb(pugi::xml_node Parent, const ClusterBlockId index, t_pb* pb, t_pb_routes& pb_route, int* num_primitives, const pugiutil::loc_data& loc_data, ClusteredNetlist* clb_nlist) {
    int i, j, pb_index;
    bool found;
    const t_pb_type* pb_type;

    t_token* tokens;
    int num_tokens;

    auto& atom_ctx = g_vpr_ctx.mutable_atom();

    auto inputs = pugiutil::get_single_child(Parent, "inputs", loc_data);
    int num_in_ports = processPorts(inputs, pb, pb_route, loc_data);

    auto outputs = pugiutil::get_single_child(Parent, "outputs", loc_data);
    int num_out_ports = processPorts(outputs, pb, pb_route, loc_data);

    auto clocks = pugiutil::get_single_child(Parent, "clocks", loc_data);
    int num_clock_ports = processPorts(clocks, pb, pb_route, loc_data);

    auto attrs = pugiutil::get_single_child(Parent, "attributes", loc_data, pugiutil::OPTIONAL);
    auto params = pugiutil::get_single_child(Parent, "parameters", loc_data, pugiutil::OPTIONAL);

    pb_type = pb->pb_graph_node->pb_type;

    //Create the ports in the clb_nlist for the top-level pb
    if (pb->is_root()) {
        VTR_ASSERT(num_in_ports <= num_out_ports);

        for (i = 0; i < num_in_ports; i++) {
            clb_nlist->create_port(index, pb_type->ports[i].name, pb_type->ports[i].num_pins, PortType::INPUT);
        }
        for (i = num_in_ports; i < num_out_ports; i++) {
            clb_nlist->create_port(index, pb_type->ports[i].name, pb_type->ports[i].num_pins, PortType::OUTPUT);
        }
        for (i = num_out_ports; i < num_clock_ports; i++) {
            clb_nlist->create_port(index, pb_type->ports[i].name, pb_type->ports[i].num_pins, PortType::CLOCK);
        }

        VTR_ASSERT(clb_nlist->block_ports(index).size() == (unsigned)pb_type->num_ports);
    }

    if (pb_type->num_modes == 0) {
        /* A primitive type */
        AtomBlockId blk_id = atom_ctx.nlist.find_block(pb->name);
        if (!blk_id) {
            vpr_throw(VPR_ERROR_NET_F, __FILE__, __LINE__,
                      ".net file and .blif file do not match, encountered unknown primitive %s in .net file.\n",
                      pb->name);
        }

        //Update atom netlist mapping
        VTR_ASSERT(blk_id);
        atom_ctx.lookup.set_atom_pb(blk_id, pb);
        atom_ctx.lookup.set_atom_clb(blk_id, index);

        auto atom_attrs = atom_ctx.nlist.block_attrs(blk_id);
        auto atom_params = atom_ctx.nlist.block_params(blk_id);
        processAttrsParams(attrs, "attribute", atom_attrs, loc_data);
        processAttrsParams(params, "parameter", atom_params, loc_data);

        (*num_primitives)++;
    } else {
        /* process children of child if exists */

        pb->child_pbs = new t_pb*[pb_type->modes[pb->mode].num_pb_type_children];
        for (i = 0; i < pb_type->modes[pb->mode].num_pb_type_children; i++) {
            pb->child_pbs[i] = new t_pb[pb_type->modes[pb->mode].pb_type_children[i].num_pb];
        }

        /* Populate info for each physical block */
        for (auto child = Parent.child("block"); child; child = child.next_sibling("block")) {
            VTR_ASSERT(strcmp(child.name(), "block") == 0);

            auto instance_type = pugiutil::get_attribute(child, "instance", loc_data);
            tokens = GetTokensFromString(instance_type.value(), &num_tokens);
            if (num_tokens != 4 || tokens[0].type != TOKEN_STRING
                || tokens[1].type != TOKEN_OPEN_SQUARE_BRACKET
                || tokens[2].type != TOKEN_INT
                || tokens[3].type != TOKEN_CLOSE_SQUARE_BRACKET) {
                vpr_throw(VPR_ERROR_NET_F, loc_data.filename_c_str(), loc_data.line(child),
                          "Unknown syntax for instance %s in %s. Expected pb_type[instance_number].\n",
                          instance_type.value(), child.name());
            }

            found = false;
            pb_index = OPEN;
            for (i = 0; i < pb_type->modes[pb->mode].num_pb_type_children; i++) {
                if (strcmp(pb_type->modes[pb->mode].pb_type_children[i].name, tokens[0].data) == 0) {
                    pb_index = vtr::atoi(tokens[2].data);
                    if (pb_index >= pb_type->modes[pb->mode].pb_type_children[i].num_pb) {
                        vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(child),
                                  "Instance number exceeds # of pb available for instance %s in %s.\n",
                                  instance_type.value(), child.name());
                    }
                    if (pb->child_pbs[i][pb_index].pb_graph_node != nullptr) {
                        vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(child),
                                  "node is used by two different blocks %s and %s.\n",
                                  instance_type.value(),
                                  pb->child_pbs[i][pb_index].name);
                    }
                    pb->child_pbs[i][pb_index].pb_graph_node = &pb->pb_graph_node->child_pb_graph_nodes[pb->mode][i][pb_index];
                    found = true;
                    break;
                }
            }
            if (!found) {
                vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(child),
                          "Unknown pb type %s.\n", instance_type.value());
            }

            auto name = pugiutil::get_attribute(child, "name", loc_data);
            if (0 != strcmp(name.value(), "open")) {
                pb->child_pbs[i][pb_index].name = vtr::strdup(name.value());

                /* Parse all pbs and CB internal nets*/
                atom_ctx.lookup.set_atom_pb(AtomBlockId::INVALID(), &pb->child_pbs[i][pb_index]);

                auto mode = child.attribute("mode");
                pb->child_pbs[i][pb_index].mode = 0;
                found = false;
                for (j = 0; j < pb->child_pbs[i][pb_index].pb_graph_node->pb_type->num_modes; j++) {
                    if (strcmp(mode.value(), pb->child_pbs[i][pb_index].pb_graph_node->pb_type->modes[j].name) == 0) {
                        pb->child_pbs[i][pb_index].mode = j;
                        found = true;
                    }
                }
                if (!found && !pb->child_pbs[i][pb_index].is_primitive()) {
                    vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(child),
                              "Unknown mode %s for cb %s #%d.\n", mode.value(),
                              pb->child_pbs[i][pb_index].name, pb_index);
                }
                pb->child_pbs[i][pb_index].parent_pb = pb;

                processPb(child, index, &pb->child_pbs[i][pb_index], pb_route, num_primitives, loc_data, clb_nlist);
            } else {
                /* physical block has no used primitives but it may have used routing */
                pb->child_pbs[i][pb_index].name = nullptr;
                atom_ctx.lookup.set_atom_pb(AtomBlockId::INVALID(), &pb->child_pbs[i][pb_index]);

                auto lookahead1 = pugiutil::get_first_child(child, "outputs", loc_data, pugiutil::OPTIONAL);
                if (lookahead1) {
                    pugiutil::get_first_child(lookahead1, "port", loc_data); //Check that port child tag exists
                    auto mode = pugiutil::get_attribute(child, "mode", loc_data);

                    pb->child_pbs[i][pb_index].mode = 0;
                    found = false;
                    for (j = 0; j < pb->child_pbs[i][pb_index].pb_graph_node->pb_type->num_modes; j++) {
                        if (strcmp(mode.value(), pb->child_pbs[i][pb_index].pb_graph_node->pb_type->modes[j].name) == 0) {
                            pb->child_pbs[i][pb_index].mode = j;
                            found = true;
                        }
                    }
                    if (!found && !pb->child_pbs[i][pb_index].is_primitive()) {
                        vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(child),
                                  "Unknown mode %s for cb %s #%d.\n", mode.value(),
                                  pb->child_pbs[i][pb_index].name, pb_index);
                    }
                    pb->child_pbs[i][pb_index].parent_pb = pb;
                    processPb(child, index, &pb->child_pbs[i][pb_index], pb_route, num_primitives, loc_data, clb_nlist);
                }
            }
            freeTokens(tokens, num_tokens);
        }
    }
}

/**
 * Adds net to hashtable of nets.  If the net is "open", then this is a keyword so do not add it.
 * If the net already exists, increase the count on that net
 */
static int add_net_to_hash(t_hash** nhash, const char* net_name, int* ncount) {
    t_hash* hash_value;

    if (strcmp(net_name, "open") == 0) {
        return OPEN;
    }

    hash_value = insert_in_hash_table(nhash, net_name, *ncount);
    if (hash_value->count == 1) {
        VTR_ASSERT(*ncount == hash_value->index);
        (*ncount)++;
    }
    return hash_value->index;
}

static int processPorts(pugi::xml_node Parent, t_pb* pb, t_pb_routes& pb_route, const pugiutil::loc_data& loc_data) {
    int i, j, num_tokens;
    int in_port = 0, out_port = 0, clock_port = 0;
    std::vector<std::string> pins;
    t_pb_graph_pin*** pin_node;
    int *num_ptrs, num_sets;
    bool found;

    auto& atom_ctx = g_vpr_ctx.atom();

    for (auto Cur = pugiutil::get_first_child(Parent, "port", loc_data, pugiutil::OPTIONAL); Cur; Cur = Cur.next_sibling("port")) {
        auto port_name = pugiutil::get_attribute(Cur, "name", loc_data);

        //Determine the port index on the pb
        //
        //Traverse all the ports on the pb until we find the matching port name,
        //at that point in_port/clock_port/output_port will be the index associated
        //with that port
        in_port = out_port = clock_port = 0;
        found = false;
        for (i = 0; i < pb->pb_graph_node->pb_type->num_ports; i++) {
            if (0 == strcmp(pb->pb_graph_node->pb_type->ports[i].name, port_name.value())) {
                found = true;
                break;
            }
            if (pb->pb_graph_node->pb_type->ports[i].is_clock
                && pb->pb_graph_node->pb_type->ports[i].type == IN_PORT) {
                clock_port++;
            } else if (!pb->pb_graph_node->pb_type->ports[i].is_clock
                       && pb->pb_graph_node->pb_type->ports[i].type == IN_PORT) {
                in_port++;
            } else {
                VTR_ASSERT(pb->pb_graph_node->pb_type->ports[i].type == OUT_PORT);
                out_port++;
            }
        }
        if (!found) {
            vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(Cur),
                      "Unknown port %s for pb %s[%d].\n", port_name.value(),
                      pb->pb_graph_node->pb_type->name,
                      pb->pb_graph_node->placement_index);
        }

        //Extract all the pins for this port
        pins = vtr::split(Cur.text().get());
        num_tokens = pins.size();

        //Check that the number of pins from the netlist file matches the pb port's number of pins
        if (0 == strcmp(Parent.name(), "inputs")) {
            if (num_tokens != pb->pb_graph_node->num_input_pins[in_port]) {
                vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(Cur),
                          "Incorrect # pins %d found (expected %d) for input port %s for pb %s[%d].\n",
                          num_tokens, pb->pb_graph_node->num_input_pins[in_port], port_name.value(), pb->pb_graph_node->pb_type->name,
                          pb->pb_graph_node->placement_index);
            }
        } else if (0 == strcmp(Parent.name(), "outputs")) {
            if (num_tokens != pb->pb_graph_node->num_output_pins[out_port]) {
                vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(Cur),
                          "Incorrect # pins %d found (expected %d) for output port %s for pb %s[%d].\n",
                          num_tokens, pb->pb_graph_node->num_output_pins[out_port], port_name.value(), pb->pb_graph_node->pb_type->name,
                          pb->pb_graph_node->placement_index);
            }
        } else {
            VTR_ASSERT(0 == strcmp(Parent.name(), "clocks"));
            if (num_tokens != pb->pb_graph_node->num_clock_pins[clock_port]) {
                vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(Cur),
                          "Incorrect # pins %d found for clock port %s for pb %s[%d].\n",
                          num_tokens, pb->pb_graph_node->num_clock_pins[clock_port], port_name.value(), pb->pb_graph_node->pb_type->name,
                          pb->pb_graph_node->placement_index);
            }
        }

        //Process the input and clock ports
        if (0 == strcmp(Parent.name(), "inputs") || 0 == strcmp(Parent.name(), "clocks")) {
            if (pb->is_root()) {
                //We are processing a top-level pb, so these pins connect to inter-block nets
                for (i = 0; i < num_tokens; i++) {
                    //Set rr_node_index to the pb_route for the appropriate port
                    const t_pb_graph_pin* pb_gpin = nullptr;
                    if (0 == strcmp(Parent.name(), "inputs")) {
                        pb_gpin = &pb->pb_graph_node->input_pins[in_port][i];
                    } else {
                        pb_gpin = &pb->pb_graph_node->clock_pins[clock_port][i];
                    }

                    VTR_ASSERT(pb_gpin != nullptr);
                    int rr_node_index = pb_gpin->pin_count_in_cluster;

                    if (strcmp(pins[i].c_str(), "open") != 0) {
                        //For connected pins look-up the inter-block net index associated with it
                        AtomNetId net_id = atom_ctx.nlist.find_net(pins[i].c_str());
                        if (!net_id) {
                            vpr_throw(VPR_ERROR_NET_F, __FILE__, __LINE__,
                                      ".blif and .net do not match, unknown net %s found in .net file.\n.",
                                      pins[i].c_str());
                        }
                        //Mark the associated inter-block net
                        pb_route.insert(std::make_pair(rr_node_index, t_pb_route()));
                        pb_route[rr_node_index].atom_net_id = net_id;
                        pb_route[rr_node_index].pb_graph_pin = pb_gpin;
                    }
                }
            } else {
                //We are processing an internal pb
                for (i = 0; i < num_tokens; i++) {
                    if (0 == strcmp(pins[i].c_str(), "open")) {
                        continue;
                    }

                    //Extract the portion of the pin name after '->'
                    //
                    //e.g. 'memory.addr1[0]->address1' becomes 'address1'
                    size_t loc = pins[i].find("->");
                    VTR_ASSERT(loc != std::string::npos);

                    std::string pin_name = pins[i].substr(0, loc);

                    loc += 2; //Skip over the '->'
                    std::string interconnect_name = pins[i].substr(loc, std::string::npos);
                    // Interconnect name is the net name

                    pin_node = alloc_and_load_port_pin_ptrs_from_string(
                        pb->pb_graph_node->pb_type->parent_mode->interconnect[0].line_num,
                        pb->pb_graph_node->parent_pb_graph_node,
                        pb->pb_graph_node->parent_pb_graph_node->child_pb_graph_nodes[pb->parent_pb->mode],
                        pin_name.c_str(), &num_ptrs, &num_sets, true,
                        true);
                    VTR_ASSERT(num_sets == 1 && num_ptrs[0] == 1);

                    const t_pb_graph_pin* pb_gpin = nullptr;
                    if (0 == strcmp(Parent.name(), "inputs")) {
                        pb_gpin = &pb->pb_graph_node->input_pins[in_port][i];
                    } else {
                        pb_gpin = &pb->pb_graph_node->clock_pins[clock_port][i];
                    }
                    VTR_ASSERT(pb_gpin != nullptr);
                    int rr_node_index = pb_gpin->pin_count_in_cluster;

                    pb_route.insert(std::make_pair(rr_node_index, t_pb_route()));
                    pb_route[rr_node_index].driver_pb_pin_id = pin_node[0][0]->pin_count_in_cluster;
                    pb_route[rr_node_index].pb_graph_pin = pb_gpin;

                    found = false;
                    for (j = 0; j < pin_node[0][0]->num_output_edges; j++) {
                        if (0 == strcmp(interconnect_name.c_str(), pin_node[0][0]->output_edges[j]->interconnect->name)) {
                            found = true;
                            break;
                        }
                    }
                    for (j = 0; j < num_sets; j++) {
                        free(pin_node[j]);
                    }
                    free(pin_node);
                    free(num_ptrs);
                    if (!found) {
                        vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(Cur),
                                  "Unknown interconnect %s connecting to pin %s.\n",
                                  interconnect_name.c_str(), pin_name.c_str());
                    }
                }
            }
        }

        if (0 == strcmp(Parent.name(), "outputs")) {
            if (pb->pb_graph_node->is_primitive()) {
                /* primitives are drivers of nets */
                for (i = 0; i < num_tokens; i++) {
                    const t_pb_graph_pin* pb_gpin = &pb->pb_graph_node->output_pins[out_port][i];
                    int rr_node_index = pb_gpin->pin_count_in_cluster;
                    if (strcmp(pins[i].c_str(), "open") != 0) {
                        AtomNetId net_id = atom_ctx.nlist.find_net(pins[i].c_str());
                        if (!net_id) {
                            vpr_throw(VPR_ERROR_NET_F, __FILE__, __LINE__,
                                      ".blif and .net do not match, unknown net %s found in .net file.\n",
                                      pins[i].c_str());
                        }
                        pb_route.insert(std::make_pair(rr_node_index, t_pb_route()));
                        pb_route[rr_node_index].atom_net_id = net_id;
                        pb_route[rr_node_index].pb_graph_pin = pb_gpin;
                    }
                }
            } else {
                for (i = 0; i < num_tokens; i++) {
                    if (0 == strcmp(pins[i].c_str(), "open")) {
                        continue;
                    }
                    //Extract the portion of the pin name after '->'
                    //
                    //e.g. 'memory.addr1[0]->address1' becomes 'address1'
                    size_t loc = pins[i].find("->");
                    VTR_ASSERT(loc != std::string::npos);

                    std::string pin_name = pins[i].substr(0, loc);

                    loc += 2; //Skip over the '->'
                    std::string interconnect_name = pins[i].substr(loc, std::string::npos);

                    pin_node = alloc_and_load_port_pin_ptrs_from_string(
                        pb->pb_graph_node->pb_type->modes[pb->mode].interconnect->line_num,
                        pb->pb_graph_node,
                        pb->pb_graph_node->child_pb_graph_nodes[pb->mode],
                        pin_name.c_str(), &num_ptrs, &num_sets, true,
                        true);
                    VTR_ASSERT(num_sets == 1 && num_ptrs[0] == 1);
                    int rr_node_index = pb->pb_graph_node->output_pins[out_port][i].pin_count_in_cluster;

                    //Why does this not use the output pin used to deterimine the rr node index?
                    pb_route.insert(std::make_pair(rr_node_index, t_pb_route()));
                    pb_route[rr_node_index].driver_pb_pin_id = pin_node[0][0]->pin_count_in_cluster;
                    pb_route[rr_node_index].pb_graph_pin = pin_node[0][0];

                    found = false;
                    for (j = 0; j < pin_node[0][0]->num_output_edges; j++) {
                        if (0 == strcmp(interconnect_name.c_str(), pin_node[0][0]->output_edges[j]->interconnect->name)) {
                            found = true;
                            break;
                        }
                    }
                    for (j = 0; j < num_sets; j++) {
                        free(pin_node[j]);
                    }
                    free(pin_node);
                    free(num_ptrs);
                    if (!found) {
                        vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(Cur),
                                  "Unknown interconnect %s connecting to pin %s.\n",
                                  interconnect_name.c_str(), pin_name.c_str());
                    }
                }
            }
        }
    }

    //Record any port rotation mappings
    for (auto pin_rot_map = pugiutil::get_first_child(Parent, "port_rotation_map", loc_data, pugiutil::OPTIONAL);
         pin_rot_map;
         pin_rot_map = pin_rot_map.next_sibling("port_rotation_map")) {
        auto port_name = pugiutil::get_attribute(pin_rot_map, "name", loc_data).value();

        const t_port* pb_gport = find_pb_graph_port(pb->pb_graph_node, port_name);

        if (pb_gport == nullptr) {
            vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(pin_rot_map),
                      "Failed to find port with name '%s' on pb %s[%d]\n",
                      port_name,
                      pb->pb_graph_node->pb_type->name, pb->pb_graph_node->placement_index);
        }

        auto pin_mapping = vtr::split(pin_rot_map.text().get());

        if (size_t(pb_gport->num_pins) != pin_mapping.size()) {
            vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(pin_rot_map),
                      "Incorrect # pins %zu (expected %d) found for port %s rotation map in pb %s[%d].\n",
                      pin_mapping.size(), pb_gport->num_pins, port_name, pb->pb_graph_node->pb_type->name,
                      pb->pb_graph_node->placement_index);
        }

        for (int ipin = 0; ipin < (int)pins.size(); ++ipin) {
            if (pin_mapping[ipin] == "open") continue; //No mapping for this physical pin to atom pin

            int atom_pin_index = vtr::atoi(pin_mapping[ipin]);

            if (atom_pin_index < 0) {
                vpr_throw(VPR_ERROR_NET_F, netlist_file_name, loc_data.line(pin_rot_map),
                          "Invalid pin number %d in port rotation map (must be >= 0)\n", atom_pin_index);
            }

            VTR_ASSERT(atom_pin_index >= 0);

            const t_pb_graph_pin* pb_gpin = find_pb_graph_pin(pb->pb_graph_node, port_name, ipin);
            VTR_ASSERT(pb_gpin);

            //Set the rotation mapping
            pb->set_atom_pin_bit_index(pb_gpin, atom_pin_index);
        }
    }

    //Return the sum of the ports + 1, as we want to number of the ports
    //E.g. If there is 1 input port, the in_port index is 0, but num_in_ports = 1
    return in_port + out_port + clock_port + 1;
}

/**
 * This function updates the nets list and the connections between that list and the complex block
 */
static void load_external_nets_and_cb(ClusteredNetlist& clb_nlist) {
    int j, k, ipin;
    int ext_ncount = 0;
    t_hash** ext_nhash;
    t_pb_graph_pin* pb_graph_pin;
    ClusterNetId clb_net_id;

    auto& atom_ctx = g_vpr_ctx.atom();

    ext_nhash = alloc_hash_table();

    /* Assumes that complex block pins are ordered inputs, outputs, globals */

    /* Determine the external nets of complex block */
    for (auto blk_id : clb_nlist.blocks()) {
        const t_pb* pb = clb_nlist.block_pb(blk_id);

        ipin = 0;
        VTR_ASSERT(clb_nlist.block_type(blk_id)->pb_type->num_input_pins
                       + clb_nlist.block_type(blk_id)->pb_type->num_output_pins
                       + clb_nlist.block_type(blk_id)->pb_type->num_clock_pins
                   == clb_nlist.block_type(blk_id)->num_pins / clb_nlist.block_type(blk_id)->capacity);

        int num_input_ports = pb->pb_graph_node->num_input_ports;
        int num_output_ports = pb->pb_graph_node->num_output_ports;
        int num_clock_ports = pb->pb_graph_node->num_clock_ports;

        //Load the external nets connected to input ports
        for (j = 0; j < num_input_ports; j++) {
            ClusterPortId input_port_id = clb_nlist.find_port(blk_id, clb_nlist.block_type(blk_id)->pb_type->ports[j].name);
            for (k = 0; k < pb->pb_graph_node->num_input_pins[j]; k++) {
                pb_graph_pin = &pb->pb_graph_node->input_pins[j][k];
                VTR_ASSERT(pb_graph_pin->pin_count_in_cluster == ipin);

                if (pb->pb_route.count(pb_graph_pin->pin_count_in_cluster)) {
                    AtomNetId atom_net_id = pb->pb_route[pb_graph_pin->pin_count_in_cluster].atom_net_id;
                    if (atom_net_id) {
                        add_net_to_hash(ext_nhash, atom_ctx.nlist.net_name(atom_net_id).c_str(), &ext_ncount);
                        clb_net_id = clb_nlist.create_net(atom_ctx.nlist.net_name(atom_net_id));
                        clb_nlist.create_pin(input_port_id, (BitIndex)k, clb_net_id, PinType::SINK, ipin);
                    }
                }
                ipin++;
            }
        }

        //Load the external nets connected to output ports
        for (j = 0; j < num_output_ports; j++) {
            ClusterPortId output_port_id = clb_nlist.find_port(blk_id, clb_nlist.block_type(blk_id)->pb_type->ports[j + num_input_ports].name);
            for (k = 0; k < pb->pb_graph_node->num_output_pins[j]; k++) {
                pb_graph_pin = &pb->pb_graph_node->output_pins[j][k];
                VTR_ASSERT(pb_graph_pin->pin_count_in_cluster == ipin);

                if (pb->pb_route.count(pb_graph_pin->pin_count_in_cluster)) {
                    AtomNetId atom_net_id = pb->pb_route[pb_graph_pin->pin_count_in_cluster].atom_net_id;
                    if (atom_net_id) {
                        add_net_to_hash(ext_nhash, atom_ctx.nlist.net_name(atom_net_id).c_str(), &ext_ncount);
                        clb_net_id = clb_nlist.create_net(atom_ctx.nlist.net_name(atom_net_id));

                        AtomPinId atom_net_driver = atom_ctx.nlist.net_driver(atom_net_id);
                        bool driver_is_constant = atom_ctx.nlist.pin_is_constant(atom_net_driver);

                        clb_nlist.create_pin(output_port_id, (BitIndex)k, clb_net_id, PinType::DRIVER, ipin, driver_is_constant);

                        VTR_ASSERT(clb_nlist.net_is_constant(clb_net_id) == driver_is_constant);
                    }
                }
                ipin++;
            }
        }

        //Load the external nets connected to clock ports
        for (j = 0; j < num_clock_ports; j++) {
            ClusterPortId clock_port_id = clb_nlist.find_port(blk_id, clb_nlist.block_type(blk_id)->pb_type->ports[j + num_input_ports + num_output_ports].name);
            for (k = 0; k < pb->pb_graph_node->num_clock_pins[j]; k++) {
                pb_graph_pin = &pb->pb_graph_node->clock_pins[j][k];
                VTR_ASSERT(pb_graph_pin->pin_count_in_cluster == ipin);

                if (pb->pb_route.count(pb_graph_pin->pin_count_in_cluster)) {
                    AtomNetId atom_net_id = pb->pb_route[pb_graph_pin->pin_count_in_cluster].atom_net_id;
                    if (atom_net_id) {
                        add_net_to_hash(ext_nhash, atom_ctx.nlist.net_name(atom_net_id).c_str(), &ext_ncount);
                        clb_net_id = clb_nlist.create_net(atom_ctx.nlist.net_name(atom_net_id));
                        clb_nlist.create_pin(clock_port_id, (BitIndex)k, clb_net_id, PinType::SINK, ipin);
                    }
                }
                ipin++;
            }
        }
    }

    vtr::vector<ClusterNetId, int> count(ext_ncount);

    /* complete load of external nets so that each net points back to the blocks,
     * and blocks point back to net pins */
    for (auto blk_id : clb_nlist.blocks()) {
        for (j = 0; j < clb_nlist.block_type(blk_id)->num_pins; j++) {
            //Iterate through each pin of the block, and see if there is a net allocated/used for it
            clb_net_id = clb_nlist.block_net(blk_id, j);

            if (clb_net_id != ClusterNetId::INVALID()) {
                //Verify old and new CLB netlists have the same # of pins per net
                if (RECEIVER == clb_nlist.block_type(blk_id)->class_inf[clb_nlist.block_type(blk_id)->pin_class[j]].type) {
                    count[clb_net_id]++;

                    if (count[clb_net_id] > (int)clb_nlist.net_sinks(clb_net_id).size()) {
                        vpr_throw(VPR_ERROR_NET_F, __FILE__, __LINE__,
                                  "net %s #%d inconsistency, expected %d terminals but encountered %d terminals, it is likely net terminal is disconnected in netlist file.\n",
                                  clb_nlist.net_name(clb_net_id).c_str(), size_t(clb_net_id), count[clb_net_id],
                                  clb_nlist.net_sinks(clb_net_id).size());
                    }

                    //Asserts the ClusterBlockId is the same when ClusterNetId & pin BitIndex is provided
                    VTR_ASSERT(blk_id == clb_nlist.pin_block(*(clb_nlist.net_pins(clb_net_id).begin() + count[clb_net_id])));
                    //Asserts the block's pin index is the same
                    VTR_ASSERT(j == clb_nlist.pin_physical_index(*(clb_nlist.net_pins(clb_net_id).begin() + count[clb_net_id])));
                    VTR_ASSERT(j == clb_nlist.net_pin_physical_index(clb_net_id, count[clb_net_id]));

                    // nets connecting to global pins are marked as global nets
                    if (clb_nlist.block_type(blk_id)->is_pin_global[j]) {
                        clb_nlist.set_net_is_global(clb_net_id, true);
                    }

                    if (clb_nlist.block_type(blk_id)->is_ignored_pin[j]) {
                        clb_nlist.set_net_is_ignored(clb_net_id, true);
                    }
                    /* Error check performed later to ensure no mixing of ignored and non ignored signals */

                } else {
                    VTR_ASSERT(DRIVER == clb_nlist.block_type(blk_id)->class_inf[clb_nlist.block_type(blk_id)->pin_class[j]].type);
                    VTR_ASSERT(j == clb_nlist.pin_physical_index(*(clb_nlist.net_pins(clb_net_id).begin())));
                    VTR_ASSERT(j == clb_nlist.net_pin_physical_index(clb_net_id, 0));
                }
            }
        }
    }

    /* Error check global and non global signals */
    VTR_ASSERT(ext_ncount == static_cast<int>(clb_nlist.nets().size()));
    for (auto net_id : clb_nlist.nets()) {
        for (auto pin_id : clb_nlist.net_sinks(net_id)) {
            bool is_ignored_net = clb_nlist.net_is_ignored(net_id);
            if (clb_nlist.block_type(clb_nlist.pin_block(pin_id))->is_ignored_pin[clb_nlist.pin_physical_index(pin_id)] != is_ignored_net) {
                VTR_LOG_WARN(
                    "Netlist connects net %s to both global and non-global pins.\n",
                    clb_nlist.net_name(net_id).c_str());
            }
        }
    }

    free_hash_table(ext_nhash);
}

static void mark_constant_generators(const ClusteredNetlist& clb_nlist, int verbosity) {
    size_t const_gen_count = 0;
    for (auto blk_id : clb_nlist.blocks()) {
        const_gen_count += mark_constant_generators_rec(clb_nlist.block_pb(blk_id), clb_nlist.block_pb(blk_id)->pb_route, verbosity);
    }

    VTR_LOG("Detected %zu constant generators (to see names run with higher pack verbosity)\n", const_gen_count);
}

static size_t mark_constant_generators_rec(const t_pb* pb, const t_pb_routes& pb_route, int verbosity) {
    int i, j;
    t_pb_type* pb_type;
    bool const_gen;
    size_t const_gen_count = 0;

    auto& atom_ctx = g_vpr_ctx.atom();

    if (pb->pb_graph_node->pb_type->blif_model == nullptr) {
        // Check the model name to see if it exists, iterate through children if it does
        for (i = 0; i < pb->pb_graph_node->pb_type->modes[pb->mode].num_pb_type_children; i++) {
            pb_type = &(pb->pb_graph_node->pb_type->modes[pb->mode].pb_type_children[i]);
            for (j = 0; j < pb_type->num_pb; j++) {
                if (pb->child_pbs[i][j].name != nullptr) {
                    const_gen_count += mark_constant_generators_rec(&(pb->child_pbs[i][j]), pb_route, verbosity);
                }
            }
        }
    } else if (strcmp(pb->pb_graph_node->pb_type->blif_model, MODEL_INPUT) != 0) {
        const_gen = true;
        for (i = 0; i < pb->pb_graph_node->num_input_ports && const_gen == true; i++) {
            for (j = 0; j < pb->pb_graph_node->num_input_pins[i] && const_gen == true; j++) {
                int cluster_pin_idx = pb->pb_graph_node->input_pins[i][j].pin_count_in_cluster;
                if (!pb_route.count(cluster_pin_idx)) continue;
                if (pb_route[cluster_pin_idx].atom_net_id) {
                    const_gen = false;
                }
            }
        }
        for (i = 0; i < pb->pb_graph_node->num_clock_ports && const_gen == true; i++) {
            for (j = 0; j < pb->pb_graph_node->num_clock_pins[i] && const_gen == true; j++) {
                int cluster_pin_idx = pb->pb_graph_node->clock_pins[i][j].pin_count_in_cluster;
                if (!pb_route.count(cluster_pin_idx)) continue;
                if (pb_route[cluster_pin_idx].atom_net_id) {
                    const_gen = false;
                }
            }
        }
        if (const_gen == true) {
            VTR_LOGV(verbosity > 2, "%s is a constant generator.\n", pb->name);
            ++const_gen_count;
            for (i = 0; i < pb->pb_graph_node->num_output_ports; i++) {
                for (j = 0; j < pb->pb_graph_node->num_output_pins[i]; j++) {
                    int cluster_pin_idx = pb->pb_graph_node->output_pins[i][j].pin_count_in_cluster;
                    if (!pb_route.count(cluster_pin_idx)) continue;
                    if (pb_route[cluster_pin_idx].atom_net_id) {
                        AtomNetId net_id = pb_route[pb->pb_graph_node->output_pins[i][j].pin_count_in_cluster].atom_net_id;
                        AtomPinId driver_pin_id = atom_ctx.nlist.net_driver(net_id);
                        VTR_ASSERT(atom_ctx.nlist.pin_is_constant(driver_pin_id));
                    }
                }
            }
        }
    }
    return const_gen_count;
}

static t_pb_routes alloc_pb_route(t_pb_graph_node* /*pb_graph_node*/) {
    t_pb_routes pb_routes;
    //t_pb_route *pb_route;
    //int num_pins = pb_graph_node->total_pb_pins;

    //VTR_ASSERT(pb_graph_node->parent_pb_graph_node == nullptr); [> This function only operates on top-level pb_graph_node <]

    //pb_route = new t_pb_route[num_pins];

    return pb_routes;
}

static void load_internal_to_block_net_nums(const t_type_ptr type, t_pb_routes& pb_route) {
    int num_pins = type->pb_graph_head->total_pb_pins;

    for (int i = 0; i < num_pins; i++) {
        if (!pb_route.count(i)) continue;

        if (pb_route[i].driver_pb_pin_id != OPEN && !pb_route[i].atom_net_id) {
            load_atom_index_for_pb_pin(pb_route, i);
        }
    }
}

static void load_atom_index_for_pb_pin(t_pb_routes& pb_route, int ipin) {
    int driver = pb_route[ipin].driver_pb_pin_id;

    VTR_ASSERT(driver != OPEN);
    VTR_ASSERT(!pb_route[ipin].atom_net_id);

    if (!pb_route[driver].atom_net_id) {
        load_atom_index_for_pb_pin(pb_route, driver);
    }

    //Store the net coming from the driver
    pb_route[ipin].atom_net_id = pb_route[driver].atom_net_id;

    //Store ourselves with the driver
    pb_route[driver].sink_pb_pin_ids.push_back(ipin);
}

//Walk through the atom netlist looking up and storing the t_pb_graph_pin associated with
//each connected AtomPinId
static void load_atom_pin_mapping(const ClusteredNetlist& clb_nlist) {
    auto& atom_ctx = g_vpr_ctx.atom();

    for (const AtomBlockId blk : atom_ctx.nlist.blocks()) {
        const t_pb* pb = atom_ctx.lookup.atom_pb(blk);
        VTR_ASSERT_MSG(pb, "Atom block must have a matching PB");

        const t_pb_graph_node* gnode = pb->pb_graph_node;
        VTR_ASSERT_MSG(gnode->pb_type->model == atom_ctx.nlist.block_model(blk),
                       "Atom block PB must match BLIF model");

        for (int iport = 0; iport < gnode->num_input_ports; ++iport) {
            if (gnode->num_input_pins[iport] <= 0) continue;

            const AtomPortId port = atom_ctx.nlist.find_atom_port(blk, gnode->input_pins[iport][0].port->model_port);
            if (!port) continue;

            for (int ipin = 0; ipin < gnode->num_input_pins[iport]; ++ipin) {
                const t_pb_graph_pin* gpin = &gnode->input_pins[iport][ipin];
                VTR_ASSERT(gpin);

                set_atom_pin_mapping(clb_nlist, blk, port, gpin);
            }
        }

        for (int iport = 0; iport < gnode->num_output_ports; ++iport) {
            if (gnode->num_output_pins[iport] <= 0) continue;

            const AtomPortId port = atom_ctx.nlist.find_atom_port(blk, gnode->output_pins[iport][0].port->model_port);
            if (!port) continue;

            for (int ipin = 0; ipin < gnode->num_output_pins[iport]; ++ipin) {
                const t_pb_graph_pin* gpin = &gnode->output_pins[iport][ipin];
                VTR_ASSERT(gpin);

                set_atom_pin_mapping(clb_nlist, blk, port, gpin);
            }
        }

        for (int iport = 0; iport < gnode->num_clock_ports; ++iport) {
            if (gnode->num_clock_pins[iport] <= 0) continue;

            const AtomPortId port = atom_ctx.nlist.find_atom_port(blk, gnode->clock_pins[iport][0].port->model_port);
            if (!port) continue;

            for (int ipin = 0; ipin < gnode->num_clock_pins[iport]; ++ipin) {
                const t_pb_graph_pin* gpin = &gnode->clock_pins[iport][ipin];
                VTR_ASSERT(gpin);

                set_atom_pin_mapping(clb_nlist, blk, port, gpin);
            }
        }
    }
}

static void set_atom_pin_mapping(const ClusteredNetlist& clb_nlist, const AtomBlockId atom_blk, const AtomPortId atom_port, const t_pb_graph_pin* gpin) {
    auto& atom_ctx = g_vpr_ctx.mutable_atom();

    VTR_ASSERT(atom_ctx.nlist.port_block(atom_port) == atom_blk);

    ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(atom_blk);
    VTR_ASSERT(clb_index != ClusterBlockId::INVALID());

    const t_pb* clb_pb = clb_nlist.block_pb(clb_index);
    if (!clb_pb->pb_route.count(gpin->pin_count_in_cluster)) {
        return;
    }

    const t_pb_route* pb_route = &clb_pb->pb_route[gpin->pin_count_in_cluster];

    if (!pb_route->atom_net_id) {
        return;
    }

    const t_pb* atom_pb = atom_ctx.lookup.atom_pb(atom_blk);

    //This finds the index within the atom port to which the current gpin
    //is mapped. Note that this accounts for any applied pin rotations
    //(e.g. on LUT inputs)
    BitIndex atom_pin_bit_index = atom_pb->atom_pin_bit_index(gpin);

    AtomPinId atom_pin = atom_ctx.nlist.port_pin(atom_port, atom_pin_bit_index);

    VTR_ASSERT(pb_route->atom_net_id == atom_ctx.nlist.pin_net(atom_pin));

    //Save the mapping
    atom_ctx.lookup.set_atom_pin_pb_graph_pin(atom_pin, gpin);
}
