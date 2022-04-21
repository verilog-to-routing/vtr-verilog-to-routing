#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_log.h"
#include "vtr_logic.h"
#include "vtr_time.h"
#include "vtr_digest.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "globals.h"
#include "read_blif.h"
#include "arch_types.h"
#include "echo_files.h"
#include "edif_to_atomnetlist.h"
#include "netlist.h"
#include "vtr_logic.h"

#include <assert.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include "sort_edif.cpp"
struct EDIFReader {
  public:
    EDIFReader(e_circuit_format edif_format,
               AtomNetlist& main_netlist,
               const std::string netlist_id,
               const t_model* user_models,
               const t_model* library_models,
               usefull_data& edif,
               struct SNode* node)
        : main_netlist_(main_netlist)
        , netlist_id_(netlist_id)
        , user_arch_models_(user_models)
        , library_arch_models_(library_models)
        , edif_format_(edif_format)
        , edif_(edif)
        , node_(node) {
        printf("\n\n in constructor\n");
        top_cell_ = (edif.find_top_(node));

        // top_cell.assign(top_cell_);
        printf("\n TOP CELL NAME IS %s", top_cell_.c_str());
        main_netlist_ = AtomNetlist(top_cell_, netlist_id);
        printf("\n\n after main netlist");
        inpad_model_ = find_model(MODEL_INPUT);
        outpad_model_ = find_model(MODEL_OUTPUT);
        printf("\n\n after setting block types");
        main_netlist_.set_block_types(inpad_model_, outpad_model_);
        VTR_LOG("Reading IOs...\n");
        read_ios();
        VTR_LOG("Reading blocks...\n");
         read_blocks();
    }
    // Define top module

  private:
    AtomNetlist& main_netlist_;
    usefull_data& edif_;
    const std::string netlist_id_;
    const t_model* user_arch_models_ = nullptr;
    const t_model* library_arch_models_ = nullptr;
    const t_model* inpad_model_;
    const t_model* outpad_model_;
    e_circuit_format edif_format_ = e_circuit_format::EDIF;
    std::vector<std::pair<std::string, std::string>> ports_vec;
    std::string top_cell_;
    struct SNode* node_;
    std::unordered_map<size_t, std::unordered_map<std::pair<size_t, size_t>, std::string, vtr::hash_pair>> port_net_maps_;

    void read_ios() {
        printf("\n===================reading inputs==================\n");
        // const t_model* input_model = find_model(MODEL_INPUT);
        // const t_model* output_model = find_model(MODEL_OUTPUT);
        t_model* blk_model;
        blk_model = new t_model;
        t_model_ports* inputs;
        t_model_ports* outputs;
        inputs = new t_model_ports;
        outputs = new t_model_ports;

        std::string top_cell = (edif_.find_top_(node_));
        // edif_.find_cell_instances(node_, top_cell);
        edif_.map_cell_ports(node_);

        ports_vec = edif_.find_cell_ports(top_cell_);
        AtomBlockId blk_id = main_netlist_.create_block(top_cell, blk_model);
        for (size_t i = 0; i < ports_vec.size(); i++) {
            std::string port_name = ports_vec[i].first;
            std::string dir = ports_vec[i].second;
            printf(" direction is given as::%s\n", dir.c_str());
            std::string cmp_string_out = "OUTPUT";
            std::string cmp_string_in = "INPUT";

            if (dir == cmp_string_in) {
                outputs->name = const_cast<char*>(port_name.c_str());

                outputs->dir = OUT_PORT;
                outputs->size = 1;
                outputs->min_size = 0;
                blk_model->outputs = outputs;

                AtomPortId port_id = main_netlist_.create_port(blk_id, blk_model->outputs);
                AtomNetId net_id = main_netlist_.create_net(port_name);
                main_netlist_.create_pin(port_id, 0, net_id, PinType::DRIVER);
                const std::string& in_blk = main_netlist_.block_name(blk_id);
                const std::string& input_port = main_netlist_.port_name(port_id);
                const std::string& netlistname = main_netlist_.netlist_name();

                printf(" block created is given as::%s\n", in_blk.c_str());
                printf(" port created is given as::%s\n", input_port.c_str());

                printf("NETLIST name is given as::%s\n", netlistname.c_str());
            }
            if (dir == cmp_string_out) {
                inputs->name = const_cast<char*>(port_name.c_str());
                inputs->dir = IN_PORT;
                inputs->size = 1;
                inputs->min_size = 0;
                blk_model->inputs = inputs;
                AtomPortId port_id = main_netlist_.create_port(blk_id, blk_model->inputs);
                const std::string& out_blk = main_netlist_.block_name(blk_id);
                AtomNetId net_id = main_netlist_.create_net(port_name);
                main_netlist_.create_pin(port_id, 0, net_id, PinType::SINK);
                const std::string& output_port = main_netlist_.port_name(port_id);
                const std::string& netlistname = main_netlist_.netlist_name();

                printf(" block created is given as::%s\n", out_blk.c_str());
                printf(" port created is given as::%s\n", output_port.c_str());
                printf("NETLIST name is given as::%s\n", netlistname.c_str());
            }
        }
        for (AtomBlockId blk_id : main_netlist_.blocks()) {
            const std::string& block_name = main_netlist_.block_name(blk_id);
            printf("BLOCK : %s \n", block_name.c_str());
            for (AtomPortId port_id_ : main_netlist_.block_input_ports(blk_id)) {
                for (AtomPinId pin_id_ : main_netlist_.port_pins(port_id_)) {
                    AtomNetId net_id_ = main_netlist_.pin_net(pin_id_);
                    const std::string& net_name = main_netlist_.net_name(net_id_);
                    printf("ASSOCIATED input NET : %s \n", net_name.c_str());
                }
            }

            for (AtomPortId port_id_ : main_netlist_.block_output_ports(blk_id)) {
                for (AtomPinId pin_id_ : main_netlist_.port_pins(port_id_)) {
                    AtomNetId net_id_ = main_netlist_.pin_net(pin_id_);
                    const std::string& net_name = main_netlist_.net_name(net_id_);
                    printf("ASSOCIATED output NET : %s \n", net_name.c_str());
                }
            }
        }
    }
    void read_blocks() {
        t_model* ins_model;
        ins_model = new t_model;
        t_model_ports* inputs;
        t_model_ports* outputs;
        inputs = new t_model_ports;
        outputs = new t_model_ports;
        std::string top_cell = (edif_.find_top_(node_));
        edif_.find_cell_instances(node_, top_cell);
        for (size_t k = 0; k < edif_.instance_vec.size(); k++) {
            std::string model_name = std::get<1>(edif_.instance_vec[k]);
            //std::string inst_name = std::get<0>(edif_.instance_vec[k]);
            std::string inst_name = (std::get<0>(edif_.instance_vec[k])).c_str();
            AtomBlockId blk_ins = main_netlist_.create_block(inst_name, ins_model);

            const std::string& out_blk = main_netlist_.block_name(blk_ins);
            printf(" block created is from read block section ::%s\n", out_blk.c_str());
            //  const t_model* ins_model = find_model(model_name);
            edif_.find_cell_net(node_, top_cell);

            edif_.map_cell_ports(node_);
            ports_vec = edif_.find_cell_ports(model_name);

            for (size_t i = 0; i < ports_vec.size(); i++) {
                std::string port_name = ports_vec[i].first;
                port_name = inst_name + port_name;
                std::string dir = ports_vec[i].second;
                printf(" direction is given as::%s\n", dir.c_str());
                std::string cmp_string_out = "OUTPUT";
                std::string cmp_string_in = "INPUT";



                if (dir == cmp_string_in) {
                    outputs->name = const_cast<char*>(port_name.c_str());

                    outputs->dir = OUT_PORT;
                    outputs->size = 1;
                    outputs->min_size = 0;
                    ins_model->outputs = outputs;

                    AtomPortId port_id = main_netlist_.create_port(blk_ins, ins_model->outputs);
                    //net_id = main_netlist_.create_net(port_name);
                    // main_netlist_.create_pin(port_id, 0, net_id, PinType::DRIVER);
                    const std::string& in_blk = main_netlist_.block_name(blk_ins);
                    const std::string& input_port = main_netlist_.port_name(port_id);
                    const std::string& netlistname = main_netlist_.netlist_name();

                    //  printf(" block created is given as::%s\n", in_blk.c_str());
                    printf(" port created is given as::%s\n", input_port.c_str());

                    printf("NETLIST name is given as::%s\n", netlistname.c_str());
                }
                if (dir == cmp_string_out) {
                    inputs->name = const_cast<char*>(port_name.c_str());
                    inputs->dir = IN_PORT;
                    inputs->size = 1;
                    inputs->min_size = 0;
                    ins_model->inputs = inputs;
                    AtomPortId port_id = main_netlist_.create_port(blk_ins, ins_model->inputs);
                    const std::string& out_blk = main_netlist_.block_name(blk_ins);

                    // main_netlist_.create_pin(port_id, 0, net_id, PinType::SINK);
                    const std::string& output_port = main_netlist_.port_name(port_id);
                    const std::string& netlistname = main_netlist_.netlist_name();

                    //  printf(" block created is given as::%s\n", out_blk.c_str());
                    printf(" port created is given as::%s\n", output_port.c_str());
                    printf("NETLIST name is given as::%s\n", netlistname.c_str());
                }
                const t_model_ports* model_port = find_model_port(ins_model, port_name);
                VTR_ASSERT(model_port);

                //Determine the pin type
                PinType pin_type = PinType::SINK;
                if (model_port->dir == OUT_PORT) {
                    pin_type = PinType::DRIVER;
                } else {
                    VTR_ASSERT_MSG(model_port->dir == IN_PORT, "Unexpected port type");
                }

                AtomPortId port_id = main_netlist_.find_atom_port(blk_ins, model_port);
                const std::string& output_port = main_netlist_.port_name(port_id);
                printf(" port created is given as::%s\n", output_port.c_str());
                std::string net_name;
             //   for (size_t j = 0; j < edif_.con_vec.size(); j++) {
                  //  net_name = std::get<0>(edif_.con_vec[j]);
                   // std::string port_bit = std::get<2>(edif_.con_vec[j]);
                  //  auto port_bit_i = stoi(port_bit);
                    AtomNetId net_id = main_netlist_.create_net("a");


                    //Make the pin
                    main_netlist_.create_pin(port_id, 0, net_id, pin_type);
               // }
            }
        }
    }

    const t_model* find_model(std::string name) {
        const t_model* arch_model = nullptr;
        for (const t_model* arch_models : {user_arch_models_, library_arch_models_}) {
            arch_model = arch_models;
            while (arch_model) {
                if (name == arch_model->name) {
                    //Found it

                    break;
                }
                arch_model = arch_model->next;
            }
            if (arch_model) {
                break;
            }
        }
        return arch_model;
    }
    const t_model_ports* find_model_port(const t_model* blk_model, std::string name) {
        //We now look through all the ports on the model looking for the matching port
        for (const t_model_ports* ports : {blk_model->inputs, blk_model->outputs})
            for (const t_model_ports* port = ports; port != nullptr; port = port->next)
                if (name == std::string(port->name))
                    return port;

        //No match

        return nullptr;
    }
};
/*
 * std::vector<AtomNetlist> edif_models_;
 * const t_model_ports* find_model_port(const t_model* blk_model, std::string port_name) {
 * //We need to handle both single, and multi-bit port names
 * //
 * //By convention multi-bit port names have the bit index stored in square brackets
 * //at the end of the name. For example:
 * //
 * //   my_signal_name[2]
 * //
 * //indicates the 2nd bit of the port 'my_signal_name'.
 *
 * //We now look through all the ports on the model looking for the matching port
 * for (const t_model_ports* ports : {blk_model->inputs, blk_model->outputs}) {
 * const t_model_ports* curr_port = ports;
 * while (curr_port) {
 *
 * curr_port = curr_port->next;
 * }
 * }
 *
 * //No match
 * return nullptr;
 * }
 *
 *
 * const t_model* find_model(std::string name) {
 * const t_model* arch_model = nullptr;
 * for (const t_model* arch_models : {user_arch_models_, library_arch_models_}) {
 * arch_model = arch_models;
 * while (arch_model) {
 * if (name == arch_model->name) {
 * //Found it
 * break;
 * }
 * arch_model = arch_model->next;
 * }
 * if (arch_model) {
 * //Found it
 * break;
 * }
 * }
 * return arch_model;
 * }
 * AtomNetlist& curr_model() {
 *
 * return edif_models_[edif_models_.size() - 1];
 * }
 */

/*AtomNetlist names(std::vector<std::string> nets) {
 *
 * AtomNetlist atom_nl("my_netlist");
 * t_model* blk_model; //Initialize the block model appropriately
 * blk_model = new t_model;
 * std::vector<AtomBlockId> blkvect;
 * const t_model* inpad_model_;
 * const t_model* outpad_model_;
 * AtomNetId net_id;
 * printf ("Open the file stream");
 * FILE *fp = fopen("/home/users/raza.jafari/netlists/edif/adder_output_edif.edn", "r");
 * //FILE *fp = fopen("/home/users/raza.jafari/test_cases/vtr-verilog-to-routing/libs/EXTERNAL/libedifparse/test_cases/edif_files/test1.edif", "r");
 * // Read the file into a tree
 * struct SNode *node = snode_parse(fp);
 *
 * usefull_data u1;
 * char* top_cell = (u1.find_top_(node));
 *
 * struct Cell *cell_ = (u1.read_thelinklist (node, top_cell));
 * int i=0;
 * for (i=0; i<u1.ports_vec.size(); i++)
 * {
 * printf ("\nport is %s & Direction is %s ",u1.ports_vec[i].first, u1.ports_vec[i].second );
 * }
 *
 *
 * for ( i=0; i<u1.nets_vec.size(); i++)
 * {
 * printf (" \n the net is %s",u1.nets_vec[i].first);
 * }
 *
 * for(int k =0; k<u1.con_vec.size(); k++)
 * {
 * printf("\n\n\n NET NAME IS : %s \n PORT NAME IS : %s \n PIN NUMBER IS : %s \n INSTANCE NAME IS %s",std::get<0>(u1.con_vec[k]), std::get<1>(u1.con_vec[k]), std::get<2>(u1.con_vec[k]), std::get<3>(u1.con_vec[k]) );
 * }
 * int i=0;
 *
 *
 * for (i=0; i<u1.ports_vec.size(); i++)
 * {
 * printf ("\nport is %s & Direction is %s and iteration number is %d",u1.ports_vec[i].first, u1.ports_vec[i].second,i );
 * std::string in_out_string = u1.ports_vec[i].second;
 * std::string cmp_string = "INPUT";
 *
 * if (in_out_string==cmp_string)
 * {
 * char* out_port = u1.ports_vec[i].first;
 * t_model_ports* outputs;
 * outputs = new t_model_ports;
 * outputs->name = out_port;
 * outputs->dir = OUT_PORT;
 * outputs->size = 1;
 * outputs->min_size = 0;
 * blk_model->outputs = outputs;
 * inpad_model_ = blk_model;
 * blkvect.emplace_back(atom_nl.create_block(out_port, blk_model));
 * AtomPortId port_id = atom_nl.create_port(blkvect.back(), blk_model->outputs);
 * const std::string& out_blk = atom_nl.block_name(blkvect.back());
 * const std::string& output_port = atom_nl.port_name(port_id);
 * net_id = atom_nl.create_net(u1.nets_vec[i].first);
 * const std::string& net_out = atom_nl.net_name(net_id);
 * const std::string& netlistname = atom_nl.netlist_name();
 * printf(" block created is given as::%s\n", out_blk.c_str());
 * printf(" port created is given as::%s\n", output_port.c_str());
 * printf("NET is given as::%s\n", net_out.c_str());
 * printf("NETLIST name is given as::%s\n", netlistname.c_str());
 * atom_nl.create_pin(port_id, 0, net_id, PinType::DRIVER);
 * }
 * cmp_string = "OUTPUT";
 * if (in_out_string==cmp_string)
 * {
 * printf ("\n\n\n\n\n\n\n A block is created for the output");
 *
 * t_model_ports* inputs;
 * inputs = new t_model_ports;
 * char* in_port = u1.ports_vec[i].first;
 * inputs->name = in_port;
 * inputs->dir = IN_PORT;
 * inputs->size = 1;
 * inputs->min_size = 0;
 *
 * blk_model->inputs = inputs;
 * outpad_model_ = blk_model;
 *
 * blkvect.emplace_back(atom_nl.create_block(in_port, blk_model));
 * AtomPortId port_id = atom_nl.create_port(blkvect.back(), blk_model->inputs);
 * const std::string& in_blk = atom_nl.block_name(blkvect.back());
 * const std::string& input_port = atom_nl.port_name(port_id);
 * const std::string& net_in = atom_nl.net_name(net_id);
 * const std::string& netlistname = atom_nl.netlist_name();
 *
 * printf(" block created is given as::%s\n", in_blk.c_str());
 * printf(" port created is given as::%s\n", input_port.c_str());
 * printf("NET is given as::%s\n", net_in.c_str());
 * printf("NETLIST name is given as::%s\n", netlistname.c_str());
 * atom_nl.create_pin(port_id, 0, net_id, PinType::SINK);
 *
 * }
 * atom_nl.set_block_types(inpad_model_, outpad_model_);
 * }
 *
 * for(AtomBlockId blk_id : atom_nl.blocks()){
 * const std::string& block_name = atom_nl.block_name(blk_id);
 * printf("BLOCK : %s \n", block_name.c_str());
 * for(AtomPortId port_id_ : atom_nl.block_input_ports(blk_id)){
 * for(AtomPinId pin_id_ : atom_nl.port_pins(port_id_)) {
 * AtomNetId net_id_ = atom_nl.pin_net(pin_id_);
 * const std::string& net_name = atom_nl.net_name(net_id_);
 * printf("ASSOCIATED input NET : %s \n", net_name.c_str());
 *
 * }
 * }
 *
 *
 * for(AtomPortId port_id_ : atom_nl.block_output_ports(blk_id)){
 * for(AtomPinId pin_id_ : atom_nl.port_pins(port_id_)) {
 * AtomNetId net_id_ = atom_nl.pin_net(pin_id_);
 * const std::string& net_name = atom_nl.net_name(net_id_);
 * printf("ASSOCIATED output NET : %s \n", net_name.c_str());
 * }
 * }
 * }
 *
 * AtomNetlist::TruthTable truth_table(2);
 *
 * truth_table[0].push_back(vtr::LogicValue::TRUE);
 * truth_table[1].push_back(vtr::LogicValue::FALSE);
 * atom_nl.verify();
 * fclose(fp);
 *
 *
 * // Deallocate the memory used by the tree
 * snode_free(node);
 *
 * return atom_nl;
 * }*/
/*AtomNetlist names() {
 * AtomNetlist atom_nl("my_netlist");
 * printf ("Open the file stream");
 * FILE *fp = fopen("/home/users/umar.iqbal/Desktop/EDIF_WORK/gap_analysis/Gap-Analysis/misc/openfpga/param_config/d_run_4/and2_or2/MIN_ROUTE_CHAN_WIDTH/and2_or2_edif.edn", "r");
 *
 * struct SNode *node = snode_parse(fp);
 *
 * usefull_data u1;
 * char* top_cell = (u1.find_top_(node));
 *
 * struct Cell *cell_ = (u1.read_thelinklist (node, top_cell));
 *
 * //for(int k =0; k<u1.con_vec.size(); k++)
 * //{
 * // printf("\n\n\n NET NAME IS : %s \n PORT NAME IS : %s \n PIN NUMBER IS : %s \n INSTANCE NAME IS %s",std::get<0>(u1.con_vec[k]), std::get<1>(u1.con_vec[k]), std::get<2>(u1.con_vec[k]), std::get<3>(u1.con_vec[k]) );
 * std::string subckt_name=std::get<3>(u1.con_vec[0]);
 * //char* out_port = std::get<1>(u1.con_vec[0]);
 * const t_model* blk_model = find_model(subckt_name);
 * //AtomBlockId blk_id = atom_nl.find_block(subckt_name);
 * AtomBlockId blk_id = atom_nl.create_block("block1", blk_model);
 * const std::string& in_blk = atom_nl.block_name(blk_id);
 * printf(" block created is given as::%s\n", in_blk.c_str());
 * //AtomPortId port_id = atom_nl.create_port(blk_id, blk_model->outputs);
 * //}
 * //We name the subckt based on the net it's first output pin drives
   /*    std::string subckt_name;
    * for (size_t i = 0; i < ports.size(); ++i) {
    * const t_model_ports* model_port = find_model_port(blk_model, ports[i]);
    * VTR_ASSERT(model_port);
    *
    * if (model_port->dir == OUT_PORT) {
    * subckt_name = nets[i];
    * break;
    * }
    * }
    *
    * if (subckt_name.empty()) {
    * //We need to name the block uniquely ourselves since there is no connected output
    * subckt_name = unique_subckt_name();
    *
    * //Since this is unusual, warn the user
    * VTR_LOGF_WARN(filename_.c_str(), lineno_,
    * "Subckt of type '%s' at %s:%d has no output pins, and has been named '%s'\n",
    * blk_model->name, filename_.c_str(), lineno_, subckt_name.c_str());
    * }
    *
    * //The name for every block should be unique, check that there is no name conflict
    * AtomBlockId blk_id = curr_model().find_block(subckt_name);
    * if (blk_id) {
    * const t_model* conflicting_model = curr_model().block_model(blk_id);
    * vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_,
    * "Duplicate blocks named '%s' found in netlist."
    * " Existing block of type '%s' conflicts with subckt of type '%s'.",
    * subckt_name.c_str(), conflicting_model->name, subckt_model.c_str());
    * }
    *
    * //Create the block
    * blk_id = curr_model().create_block(subckt_name, blk_model);
    * set_curr_block(blk_id);
    *
    * for (size_t i = 0; i < ports.size(); ++i) {
    * //Check for consistency between model and ports
    * const t_model_ports* model_port = find_model_port(blk_model, ports[i]);
    * VTR_ASSERT(model_port);
    *
    * //Determine the pin type
    * PinType pin_type = PinType::SINK;
    * if (model_port->dir == OUT_PORT) {
    * pin_type = PinType::DRIVER;
    * } else {
    * VTR_ASSERT_MSG(model_port->dir == IN_PORT, "Unexpected port type");
    * }
    *
    * //Make the port
    * std::string port_base;
    * size_t port_bit;
    * std::tie(port_base, port_bit) = split_index(ports[i]);
    *
    * AtomPortId port_id = curr_model().create_port(blk_id, find_model_port(blk_model, port_base));
    *
    * //Make the net
    * AtomNetId net_id = curr_model().create_net(nets[i]);
    *
    * //Make the pin
    * curr_model().create_pin(port_id, port_bit, net_id, pin_type);
    * }
    * return atom_nl;
    * }
    */
AtomNetlist read_edif(e_circuit_format circuit_format,
                      const char* edif_file,
                      const t_model* user_models,
                      const t_model* library_models) {
    AtomNetlist netlist;
    printf("reading inputs");
    FILE* fp = fopen("/home/users/umar.iqbal/Desktop/EDIF_WORK/gap_analysis/Gap-Analysis/misc/openfpga/param_config/d_run_4/and2/MIN_ROUTE_CHAN_WIDTH/and2_edif.edn", "r");
    struct SNode* node = snode_parse(fp);
    printf("reading inputs==========");

    std::string netlist_id = vtr::secure_digest_file(edif_file);
    usefull_data u1;
    printf("ENTERING in constructor\n");
    EDIFReader reader(circuit_format, netlist, netlist_id, user_models, library_models, u1, node);

    return netlist;
}
