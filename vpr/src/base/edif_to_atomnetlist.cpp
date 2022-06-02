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

vtr::LogicValue to_vtr_logic_value_(int val) {
    vtr::LogicValue new_val = vtr::LogicValue::UNKOWN;
    switch (val) {
        case 1:
            new_val = vtr::LogicValue::TRUE;
            break;
        case 0:
            new_val = vtr::LogicValue::FALSE;
            break;
        default:
            VTR_ASSERT_OPT_MSG(false, "Unkown logic value");
    }
    return new_val;
}
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
        read_nets();
        read_ios();
        VTR_LOG("Reading blocks...\n");
        read_blocks();
        read_nets();
        //find_corresponding_net (std::string port_name);
    }
    // Define top module

  private:
    e_circuit_format edif_format_ = e_circuit_format::EDIF;
    AtomNetlist& main_netlist_;
    const std::string netlist_id_;

    const t_model* user_arch_models_ = nullptr;
    const t_model* library_arch_models_ = nullptr;
    usefull_data& edif_;

    const t_model* inpad_model_;
    const t_model* outpad_model_;

    std::vector<std::tuple<std::string, std::string, std::string>> ports_vec;
    std::string top_cell_;
    struct SNode* node_;
    int num_ports;

    void read_ios() {
        printf("\n===================reading inputs==================\n");
        const t_model* input_model = find_model(MODEL_INPUT);
        const t_model* output_model = find_model(MODEL_OUTPUT);

        std::string top_cell = (edif_.find_top_(node_));
        // edif_.find_cell_instances(node_, top_cell);
        edif_.map_cell_ports(node_);

        ports_vec = edif_.find_cell_ports(top_cell);
        AtomBlockId blk_id;
        AtomPortId port_id;
        AtomNetId net_id;
        for (size_t i = 0; i < ports_vec.size(); i++) {
            std::string port_name = std::get<0>(ports_vec[i]);
            port_name = port_name + top_cell;
            printf("\n Port name is %s", port_name.c_str());
            std::string dir = std::get<1>(ports_vec[i]);
            std::string size = std::get<2>(ports_vec[i]);
            auto size_port = stoi(size);


            printf(" direction is given as::%s\n", dir.c_str());
            printf(" size in string is is given as::%s\n", size.c_str());
            printf(" Size given as::%d\n", size_port);
            std::string cmp_string_out = "OUTPUT";
            std::string cmp_string_in = "INPUT";
            AtomNetId netID;
            AtomPinId pinID;
            std::string temp_name;
            if (dir == cmp_string_in) {
            	for (num_ports=0;num_ports<size_port; num_ports++){
            	std::string num_ports_str= std::to_string(num_ports);
            	temp_name= port_name+num_ports_str;
            	printf("\n final port name for read ios input is %s" , temp_name.c_str());
                blk_id = main_netlist_.create_block(const_cast<char*>(temp_name.c_str()), input_model);
                port_id = main_netlist_.create_port(blk_id, input_model->outputs);

                 netID = main_netlist_.find_net( find_corresponding_net(temp_name));
                pinID = main_netlist_.create_pin(port_id, 0, netID, PinType::DRIVER);


                const std::string& in_blk = main_netlist_.block_name(blk_id);
                const std::string& input_port = main_netlist_.port_name(port_id);
                const std::string& netlistname = main_netlist_.netlist_name();

                printf(" block created is given as::%s\n", in_blk.c_str());
                printf(" port created is given as::%s\n", input_port.c_str());

                printf("NETLIST name is given as::%s\n", netlistname.c_str());
            	}
            }
            if (dir == cmp_string_out) {
            	for (num_ports=0;num_ports<size_port; num_ports++){
            	   std::string num_ports_str= std::to_string(num_ports);
            	  temp_name= port_name+num_ports_str;
                blk_id = main_netlist_.create_block(const_cast<char*>(temp_name.c_str()), output_model);
                port_id = main_netlist_.create_port(blk_id, output_model->inputs);
                 netID = main_netlist_.find_net( find_corresponding_net(temp_name));
                 pinID = main_netlist_.create_pin(port_id, 0, netID, PinType::SINK);
                const std::string& out_blk = main_netlist_.block_name(blk_id);
                const std::string& output_port = main_netlist_.port_name(port_id);
                const std::string& netlistname = main_netlist_.netlist_name();

                printf(" block created is given as::%s\n", out_blk.c_str());
                printf(" port created is given as::%s\n", output_port.c_str());
                printf("NETLIST name is given as::%s\n", netlistname.c_str());
            }
            }
        }
    }
    void read_blocks() {
        const t_model* blk_model = find_model(MODEL_NAMES);

        std::string top_cell = (edif_.find_top_(node_));
        edif_.find_cell_instances(node_, top_cell);
        AtomNetId netID;
                    AtomPinId pinID;
        for (size_t k = 0; k < edif_.instance_vec.size(); k++) {
            std::string model_name = std::get<1>(edif_.instance_vec[k]);

            std::string inst_name = (std::get<0>(edif_.instance_vec[k])).c_str();
            AtomNetlist::TruthTable truth_table;
            // 2 property lut
            // 3 property width
            std::vector<int> sc;
            std::vector<std::vector<int>> rows;
            std::string property_lut = std::get<2>(edif_.instance_vec[k]);
            printf(" property_lut is is given as::%s\n", property_lut.c_str());
            std::string property_width = std::get<3>(edif_.instance_vec[k]);

           int index= std::stoi(property_lut);
           int property_wid= std::stoi(property_width);
            while(index!=0){
                            sc.push_back(index%2);
                            index /= 2;
                        }



                        for(int xc=0; xc<=pow(2,property_wid); xc++){
                            std::vector<int> row_(property_wid, 0);
                            int x = xc;
                            printf("X is %i \n", x);
                           // int ix = log2(sc.size()) - 1;
                            int ix = property_wid;
                            while(x>0){
                                int topush = x%2;
                                printf (" \nto push value is %d", topush);
                                x = x/2;
                                row_.at(ix) = topush;
                                ix--;
                                printf (" \n ix value is %d", ix);
                            }
                           // row_.push_back(sc.at(xc));

                            rows.push_back(row_);

                            for(int vi=0; vi<row_.size(); vi++){
                                printf("%ith e of row is %i\n",vi,row_.at(vi));
                            }
                        }

                        for(int rs=0; rs<rows.size(); rs++){
                            truth_table.emplace_back();
                            printf("value of rs %i\n", rs);
                            std::vector<int> row_;
                            row_ = rows.at(rs);
                            for(int irs=0; irs<row_.size(); irs++){
                                truth_table[truth_table.size() - 1].push_back(to_vtr_logic_value_(row_.at(irs)));
                                printf("value at %i of row is %i\n", irs, row_.at(irs));
                            }
                        }

            edif_.map_cell_ports(node_);
            ports_vec = edif_.find_cell_ports(model_name);
            AtomBlockId blk_ins = main_netlist_.create_block(inst_name, blk_model,truth_table);
            for (size_t i = 0; i < ports_vec.size(); i++) {
                std::string port_name = std::get<0>(ports_vec[i]);
                std::string dir = std::get<1>(ports_vec[i]);
                std::string size = std::get<2>(ports_vec[i]);
                int size_port = stoi(size);

                port_name = port_name + inst_name;

                printf(" direction is given as::%s\n", dir.c_str());
                printf(" Size is given as::%s\n", size.c_str());
                std::string cmp_string_out = "OUTPUT";
                std::string cmp_string_in = "INPUT";

                if (dir == cmp_string_in) {

                     blk_model->inputs->size = size_port;
                    AtomPortId port_id = main_netlist_.create_port(blk_ins, blk_model->inputs);
                    for (num_ports=0;num_ports<size_port; num_ports++){
                     std::string num_ports_str= std::to_string(num_ports);
                     std::string temp_name= port_name+num_ports_str;
                      netID = main_netlist_.find_net( find_corresponding_net(temp_name));
                       pinID = main_netlist_.create_pin(port_id, num_ports, netID, PinType::SINK);

                    const std::string& input_port = main_netlist_.port_name(port_id);
                    const std::string& netlistname = main_netlist_.netlist_name();


                    printf(" port created is given as::%s\n", input_port.c_str());

                    printf("NETLIST name is given as::%s\n", netlistname.c_str());
                    }
                }
                if (dir == cmp_string_out) {
                     blk_model->outputs->size = size_port;
                     AtomPortId  port_id=  main_netlist_.create_port(blk_ins, blk_model->outputs);
                    for (num_ports=0;num_ports<size_port; num_ports++){
                       std::string num_ports_str= std::to_string(num_ports);
                       std::string temp_name= port_name+num_ports_str;
                       netID = main_netlist_.find_net( find_corresponding_net(temp_name));
                       pinID = main_netlist_.create_pin(port_id, num_ports, netID, PinType::DRIVER);
                }
                }
            }


         /*   vtr::LogicValue input_bool;
            input_bool = vtr::LogicValue::TRUE;
            std::vector<std::vector<vtr::LogicValue>> so_cover;
            std::vector<vtr::LogicValue> truth_table_single;
            truth_table_single.push_back(input_bool);
            truth_table_single.push_back(input_bool);
            so_cover.push_back(truth_table_single);
            AtomNetlist::TruthTable truth_table;
            for (const auto& row : so_cover) {
                truth_table.emplace_back();
                for (auto val : row) {
                    truth_table[truth_table.size() - 1].push_back(input_bool);
                }
            }*/
            //AtomBlockId blk_id = main_netlist_.create_block(model_name, blk_model, truth_table);
        }
    }
    void read_nets() {
        printf("Entering the net connection vector");
        std::string top_cell = (edif_.find_top_(node_));
        edif_.find_cell_net(node_, top_cell);
       // printf("the size of map is %d",edif_.nets.size() );
        std::string net_name;
        std::vector<std::tuple<std::string, std::string, std::string>> temp_vec22;
        for (auto it = edif_.nets.begin(); it != edif_.nets.end(); it++) {
            net_name = it->first;
            printf("\n a net is created with a name of %s\n ", net_name.c_str());
            AtomNetId net_id = main_netlist_.create_net(net_name);

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
    std::string find_corresponding_net (std::string port_name)
    {
    	//printf("\n\n\n\nEntering the find_coreesponding nets\n");
    //	printf("the size of map is %d",edif_.nets.size() );
    	 std::string net_name;
    	 std::vector<std::tuple<std::string, std::string, std::string>> temp_vec99;
    	        for (auto newit = edif_.nets.begin(); newit != edif_.nets.end(); newit++) {
    	             net_name = newit->first;
    	             printf("Entering the map");
    	            temp_vec99 = (*newit).second;
    	            size_t k;
    	            for (k = 0; k < temp_vec99.size(); k++) {
    	            	std::string port_ref = std::get<0>(temp_vec99[k]);
    	            	printf("\n net checking %s against %s\n ",port_ref.c_str(), port_name.c_str());

    	            	if (port_name== port_ref )
    	            	{
    	            		printf("\n port matched\n");
    	            		printf("\n a net is found of %s\n ", net_name.c_str());
    	            		return net_name;
    	            	}
    	            }
    	        }
    	return net_name;
    }

};

AtomNetlist read_edif(e_circuit_format circuit_format,
                      const char* edif_file,
                      const t_model* user_models,
                      const t_model* library_models) {
    AtomNetlist netlist;
    t_model* blk_model;
    //blk_model = new t_model;
    // main_netlist_ = AtomNetlist(top_cell_, netlist_id);
    printf("reading inputs");
    FILE* fp = fopen("/home/users/umar.iqbal/edif/and2_or2_output_edif.edn", "r");
    struct SNode* node = snode_parse(fp);
    printf("reading inputs==========");

    std::string netlist_id = vtr::secure_digest_file(edif_file);
    usefull_data u1;
    std::string top_cell = (u1.find_top_(node));

    printf("\n \n reading complete");
    EDIFReader reader(circuit_format, netlist, netlist_id, user_models, library_models, u1, node);
    fclose(fp);
    return netlist;
}
