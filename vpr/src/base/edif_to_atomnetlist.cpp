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
         ,netlist_id_(netlist_id)
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
       // begin_model();
        read_ios();
        VTR_LOG("Reading blocks...\n");
       read_blocks();
        read_nets();
       // finish_parse();
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
    std::vector<std::tuple<std::string, std::string, std::string>> ports_vec;
    std::string top_cell_;
    struct SNode* node_;
    std::vector<AtomNetlist> edif_models_;
    std::vector<bool> edif_models_black_box_;
    bool ended_ = true; //Initially no active .model
    AtomBlockId curr_block_;

    void begin_model(){
           //Create a new model, and set it's name
    	std::string top_cell = (edif_.find_top_(node_));
           edif_models_.emplace_back(top_cell, netlist_id_);
           edif_models_.back().set_block_types(inpad_model_, outpad_model_);
           edif_models_black_box_.emplace_back(false);
           ended_ = false;
           set_curr_block(AtomBlockId::INVALID()); //This statement doesn't define a block, so mark invalid
       }
  /*  void read_ios() {
        printf("\n===================reading inputs==================\n");
         const t_model* input_model = find_model(MODEL_INPUT);
        const t_model* output_model = find_model(MODEL_OUTPUT);






        VTR_ASSERT_MSG(!input_model->inputs, "Inpad model has an input port");
                VTR_ASSERT_MSG(input_model->outputs, "Inpad model has no output port");
                VTR_ASSERT_MSG(input_model->outputs->size == 1, "Inpad model has non-single-bit output port");
                VTR_ASSERT_MSG(!input_model->outputs->next, "Inpad model has multiple output ports");



                VTR_ASSERT_MSG(!output_model->outputs, "Outpad model has an output port");
                       VTR_ASSERT_MSG(output_model->inputs, "Outpad model has no input port");
                       VTR_ASSERT_MSG(output_model->inputs->size == 1, "Outpad model has non-single-bit input port");
                       VTR_ASSERT_MSG(!output_model->inputs->next, "Outpad model has multiple input ports");



       // t_model* blk_model;
       // blk_model = new t_model;
        t_model_ports* inputs;
       t_model_ports* outputs;
       // inputs = new t_model_ports;
        //outputs = new t_model_ports;


      //  const t_model* outpad_model_;
      //
       outputs = input_model->outputs;
      inputs = output_model->inputs;
      //  inputs = new t_model_ports;
      //  outputs = new t_model_ports;
       // blk_model->inputs = inputs;
       // blk_model->outputs = outputs;
        //inpad_model_=blk_model;
       // outpad_model_=blk_model;



        std::string top_cell = (edif_.find_top_(node_));
        // edif_.find_cell_instances(node_, top_cell);
        edif_.map_cell_ports(node_);

        ports_vec = edif_.find_cell_ports(top_cell);
        std::string out_pre = "out:";
       // AtomBlockId blk_id_out = main_netlist_.create_block(top_cell, output_model);
              for (size_t i = 0; i < ports_vec.size(); i++) {
                   std::string port_name = std::get<0>(ports_vec[i]);
                   port_name = port_name + top_cell;
                   port_name = out_pre + port_name;
                   printf("\n Port name is %s", port_name.c_str());
                   std::string dir = std::get<1>(ports_vec[i]);
                   std::string size = std::get<2>(ports_vec[i]);
                   auto size_port = stoi(size);

                   printf(" direction is given as::%s\n", dir.c_str());
                   printf(" size in string is is given as::%s\n", size.c_str());
                   printf(" Size given as::%d\n", size_port);
                   std::string cmp_string_out = "OUTPUT";
                   std::string cmp_string_in = "INPUT";

                   if (dir == cmp_string_in) {

                	   outputs->name = const_cast<char*>(port_name.c_str());
                	   outputs->dir = OUT_PORT;
                	   outputs->size = size_port;
                	   outputs->min_size = 0;

                      // blk_model->outputs = outputs;
                	   AtomBlockId blk_id = curr_model().create_block(port_name, input_model);
                       AtomPortId port_id = curr_model().create_port(blk_id, input_model->outputs);
                      // AtomNetId net_id = curr_model().create_net(port_name);
                       //curr_model().create_pin(port_id, 0, net_id, PinType::DRIVER);
                       //AtomNetId net_id = main_netlist_.create_net(port_name);
                       // main_netlist_.create_pin(port_id, 0, net_id, PinType::DRIVER);
                       const std::string& in_blk = curr_model().block_name(blk_id);
                       const std::string& input_port = curr_model().port_name(port_id);
                       const std::string& netlistname = curr_model().netlist_name();

                       printf(" block created is given as::%s\n", in_blk.c_str());
                       printf(" port created is given as::%s\n", input_port.c_str());

                       printf("NETLIST name is given as::%s\n", netlistname.c_str());
                   }
                 if (dir == cmp_string_out) {
                	   inputs->name = const_cast<char*>(port_name.c_str());
                	   inputs->dir = IN_PORT;
                	   inputs->size = size_port;
                	   inputs->min_size = 0;
                      // blk_model->inputs = inputs;
                	   AtomBlockId blk_id = curr_model().create_block(port_name, output_model);
                       AtomPortId port_id = curr_model().create_port(blk_id, output_model->inputs);
                       //AtomNetId net_id = curr_model().create_net(port_name);
                         //          curr_model().create_pin(port_id, 0, net_id, PinType::SINK);

                       const std::string& out_blk = curr_model().block_name(blk_id);
                       // AtomNetId net_id = main_netlist_.create_net(port_name);
                       //main_netlist_.create_pin(port_id, 0, net_id, PinType::SINK);
                       const std::string& output_port = curr_model().port_name(port_id);
                       const std::string& netlistname = curr_model().netlist_name();

                       printf(" block created is given as::%s\n", out_blk.c_str());
                       printf(" port created is given as::%s\n", output_port.c_str());
                       printf("NETLIST name is given as::%s\n", netlistname.c_str());
                   }
                // set_curr_block(AtomBlockId::INVALID()); //This statement doesn't define a block, so mark invalid
               }

    }*/
    void read_ios() {
           printf("\n===================reading inputs==================\n");
            const t_model* input_model = find_model(MODEL_INPUT);
           const t_model* output_model = find_model(MODEL_OUTPUT);
          // t_model* blk_model;
          // blk_model = new t_model;
           t_model_ports* inputs;
          t_model_ports* outputs;
          // inputs = new t_model_ports;
           //outputs = new t_model_ports;


         //  const t_model* outpad_model_;
         //
          outputs = input_model->outputs;
         inputs = output_model->inputs;
         //  inputs = new t_model_ports;
         //  outputs = new t_model_ports;
          // blk_model->inputs = inputs;
          // blk_model->outputs = outputs;
           //inpad_model_=blk_model;
          // outpad_model_=blk_model;



           std::string top_cell = (edif_.find_top_(node_));
           // edif_.find_cell_instances(node_, top_cell);
           edif_.map_cell_ports(node_);

           ports_vec = edif_.find_cell_ports(top_cell);

          // AtomBlockId blk_id_out = main_netlist_.create_block(top_cell, output_model);
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

                      if (dir == cmp_string_in) {
                   	   outputs->name = const_cast<char*>(port_name.c_str());
                   	   outputs->dir = OUT_PORT;
                   	   outputs->size = size_port;
                   	   outputs->min_size = 0;

                         // blk_model->outputs = outputs;
                   	   AtomBlockId blk_id = main_netlist_.create_block(port_name, input_model);
                          AtomPortId port_id = main_netlist_.create_port(blk_id, input_model->outputs);
                          //AtomNetId net_id = main_netlist_.create_net(port_name);
                          // main_netlist_.create_pin(port_id, 0, net_id, PinType::DRIVER);
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
                   	   inputs->size = size_port;
                   	   inputs->min_size = 0;
                         // blk_model->inputs = inputs;
                   	   AtomBlockId blk_id = main_netlist_.create_block(port_name, output_model);
                          AtomPortId port_id = main_netlist_.create_port(blk_id, output_model->inputs);
                          const std::string& out_blk = main_netlist_.block_name(blk_id);
                          // AtomNetId net_id = main_netlist_.create_net(port_name);
                          //main_netlist_.create_pin(port_id, 0, net_id, PinType::SINK);
                          const std::string& output_port = main_netlist_.port_name(port_id);
                          const std::string& netlistname = main_netlist_.netlist_name();

                          printf(" block created is given as::%s\n", out_blk.c_str());
                          printf(" port created is given as::%s\n", output_port.c_str());
                          printf("NETLIST name is given as::%s\n", netlistname.c_str());
                      }
                  }

       }

    void read_blocks() {
        //t_model* ins_model;
        //ins_model = new t_model;
    	const t_model* blk_model = find_model(MODEL_NAMES);
    	// t_model* blk_model;
    	 //       blk_model=new t_model;
        t_model_ports* inputs;
        t_model_ports* outputs;
       // inputs = new t_model_ports;
       // outputs = new t_model_ports;
        inputs=blk_model->inputs  ;
        outputs= blk_model->outputs ;
        std::string top_cell = (edif_.find_top_(node_));
        edif_.find_cell_instances(node_, top_cell);
        edif_.find_cell_net(node_, top_cell);
        for (size_t k = 0; k < edif_.instance_vec.size(); k++) {
             std::string model_name = std::get<1>(edif_.instance_vec[k]);
            //std::string inst_name = std::get<0>(edif_.instance_vec[k]);
            std::string inst_name = (std::get<0>(edif_.instance_vec[k])).c_str();


            //const std::string& out_blk = main_netlist_.block_name(blk_ins);
            //printf(" block created is from read block section ::%s\n", out_blk.c_str());
            //  const t_model* ins_model = find_model(model_name);

            edif_.map_cell_ports(node_);
            ports_vec = edif_.find_cell_ports(model_name);

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
                	blk_model->inputs->name = const_cast<char*>(port_name.c_str());

                    blk_model->inputs->dir = IN_PORT;
                    blk_model->inputs->size = size_port;
                    blk_model->inputs->min_size = 0;
                    //blk_model->inputs = inputs;
                    AtomBlockId blk_ins = main_netlist_.create_block(port_name, blk_model);
                    AtomPortId port_id = main_netlist_.create_port(blk_ins, blk_model->inputs);
                    //net_id = main_netlist_.create_net(port_name);
                    // main_netlist_.create_pin(port_id, 0, net_id, PinType::DRIVER);
                    //const std::string& in_blk = main_netlist_.block_name(blk_ins);
                    const std::string& input_port = main_netlist_.port_name(port_id);
                    const std::string& netlistname = main_netlist_.netlist_name();

                    //  printf(" block created is given as::%s\n", in_blk.c_str());
                    printf(" port created is given as::%s\n", input_port.c_str());

                    printf("NETLIST name is given as::%s\n", netlistname.c_str());
                }
                if (dir == cmp_string_out) {
                	blk_model->outputs->name = const_cast<char*>(port_name.c_str());
                	blk_model->outputs->dir = OUT_PORT;
                	blk_model->outputs->size = size_port;
                	blk_model->outputs->min_size = 0;
                    //blk_model->outputs = outputs;
                	AtomBlockId blk_ins = main_netlist_.create_block(port_name, blk_model);
                    main_netlist_.create_port(blk_ins, blk_model->outputs);
                    //const std::string& out_blk = main_netlist_.block_name(blk_ins);

                    // main_netlist_.create_pin(port_id, 0, net_id, PinType::SINK);
                    //const std::string& output_port = main_netlist_.port_name(port_id);
                    //const std::string& netlistname = main_netlist_.netlist_name();

                    //  printf(" block created is given as::%s\n", out_blk.c_str());
                    //printf(" port created is given as::%s\n", output_port.c_str());
                    //printf("NETLIST name is given as::%s\n", netlistname.c_str());
                }
            }



        // 2 property lut
        // 3 property width

        //AtomNetlist::TruthTable truth_table;
        std::string property_lut = std::get<2>(edif_.instance_vec[k]);
       // int property_lut_i = stoi(property_lut);
        //truth_table.emplace_back();
        std::string property_width = std::get<3>(edif_.instance_vec[k]);
       // int property_width_i = stoi(property_width);
        //std::vector<bool>truth_table_single;
        vtr::LogicValue input_bool;
        input_bool = vtr::LogicValue::TRUE;
//truth_table_single.push_back(true);
//truth_table_single.push_back(true);
//truth_table_single.push_back(false);
        //truth_table[truth_table.size()].push_back(input_bool);
        //input_bool = vtr::LogicValue::TRUE;
        //truth_table[truth_table.size() - 1].push_back(input_bool);
        //input_bool = vtr::LogicValue::FALSE;
        //truth_table[truth_table.size() - 1].push_back(input_bool);
        //AtomBlockId blk_id_tt = curr_model().create_block( property_lut, blk_model, truth_table);
                //set_curr_block(blk_id_tt);

        std::vector<std::vector<vtr::LogicValue>> so_cover;
        std::vector<vtr::LogicValue>truth_table_single;

        truth_table_single.push_back (input_bool);
        truth_table_single.push_back (input_bool);
        truth_table_single.push_back (input_bool);
        so_cover.push_back(truth_table_single);
        AtomNetlist::TruthTable truth_table;
               for (const auto& row : so_cover) {
                   truth_table.emplace_back();
                   for (auto val : row) {
                       truth_table[truth_table.size() - 1].push_back(input_bool);
                   }
               }
              // AtomBlockId blk_id = curr_model().create_block(model_name, blk_model, truth_table);
        }
    }
    void read_nets() {
        printf("Entering the net connection vector");
        std::string net_name;
        std::vector<std::tuple<std::string, std::string, std::string>> temp_vec22;
        for (auto it = edif_.nets.begin(); it != edif_.nets.end(); it++) {
            net_name = it->first;
            printf("\n a net is created with a name of %s\n ", net_name.c_str());
            AtomNetId net_id = main_netlist_.create_net(net_name);
            temp_vec22 = (*it).second;
            size_t k;
            for (k = 0; k < (temp_vec22.size()); k++) {
                std::string port_ref = std::get<0>(temp_vec22[k]);
                printf("\n The port_ref is %s \n ", port_ref.c_str());
                std::string port_bit = std::get<1>(temp_vec22[k]);
                printf("\n The port_bit is %s \n ", port_bit.c_str());
                std::string inst_ref = std::get<2>(temp_vec22[k]);
                printf("\n The inst_ref is %s \n ", inst_ref.c_str());
                BitIndex port_bit_i = stoi(port_bit);
                // BitIndex port_bit_i = 0;
                printf(" \nport bit is  is given as::%d\n", port_bit_i);
                AtomBlockId blk_id = main_netlist_.find_block(port_ref);
                //const std::string& out_blk = main_netlist_.block_name(blk_id);
                //printf(" block created is from read block section============ ::%s\n", out_blk.c_str());
                AtomPortId port_id_ = main_netlist_.find_port(blk_id, port_ref);

                std::string top_cell = (edif_.find_top_(node_));

                if (main_netlist_.port_type(port_id_) == PortType::INPUT)
                // for (AtomPortId port_id_ : main_netlist_.block_input_ports(blk_id)) {
                {
                    printf("\nCondition 1 satisfied");
                    main_netlist_.create_pin(port_id_, port_bit_i, net_id, PinType::SINK);
                }
                if (main_netlist_.port_type(port_id_) == PortType::OUTPUT)
                // for (AtomPortId port_id_ : main_netlist_.block_input_ports(blk_id)) {
                {
                    printf("\nCondition 2 satisfied");
                    main_netlist_.create_pin(port_id_, port_bit_i, net_id, PinType::DRIVER);
                }

                //const std::string& output_port = main_netlist_.port_name(port_id_);
                //printf(" port found is  is given as::%s\n", output_port.c_str());
                //}
            }
        }
    }

    void get_truth_table (int property_lut, int property_width){
    	std::vector<std::vector<bool>> truth_table_values;




    }


    bool verify_blackbox_model(AtomNetlist& edif_model) {
           const t_model* arch_model = find_model(edif_model.netlist_name());

           //Verify each port on the model
           //
           // We parse each .model as it's own netlist so the IOs
           // get converted to blocks
           for (auto blk_id : edif_model.blocks()) {
               //Check that the port directions match
               if (edif_model.block_type(blk_id) == AtomBlockType::INPAD) {
                   const auto& input_name = edif_model.block_name(blk_id);

                   //Find model port already verifies the port widths
                   const t_model_ports* arch_model_port = find_model_port(arch_model, input_name);
                   VTR_ASSERT(arch_model_port);
                   VTR_ASSERT(arch_model_port->dir == IN_PORT);

               } else {
                   VTR_ASSERT(edif_model.block_type(blk_id) == AtomBlockType::OUTPAD);

                   auto raw_output_name = edif_model.block_name(blk_id);

                   std::string output_name = raw_output_name; //vtr::replace_first(raw_output_name, OUTPAD_NAME_PREFIX, "");

                   //Find model port already verifies the port widths
                   const t_model_ports* arch_model_port = find_model_port(arch_model, output_name);
                   VTR_ASSERT(arch_model_port);

                   VTR_ASSERT(arch_model_port->dir == OUT_PORT);
               }
           }
           return true;
       }


    //Retrieve the netlist
       size_t determine_main_netlist_index() {
           //Look through all the models loaded, to find the one which is non-blackbox (i.e. has real blocks
           //and is not a blackbox).  To check for errors we look at all models, even if we've already
           //found a non-blackbox model.
           int top_model_idx = -1; //Not valid

           for (int i = 0; i < static_cast<int>(edif_models_.size()); ++i) {
               if (!edif_models_black_box_[i]) {
                   //A non-blackbox model
                   if (top_model_idx == -1) {
                       //This is the top model
                       top_model_idx = i;
                   } //else {
                       //We already have a top model
                      // vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_,
                        //         "Found multiple models with primitives. "
                         //        "Only one model can contain primitives, the others must be blackboxes.");
                   //}
               } else {
                   //Verify blackbox models match the architecture
                   verify_blackbox_model(edif_models_[i]);
               }
           }

           if (top_model_idx == -1) {
            //   vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_,
              //           "No non-blackbox models found. The main model must not be a blackbox.");
           }

           //Return the main model
           VTR_ASSERT(top_model_idx >= 0);
           return static_cast<size_t>(top_model_idx);
       }



    void finish_parse() {
            //When parsing is finished we *move* the main netlist
            //into the user object. This ensures we never have two copies
            //(consuming twice the memory).
            size_t main_netlist_idx = determine_main_netlist_index();
            main_netlist_ = std::move(edif_models_[main_netlist_idx]);
            printf("\n the netlist index moved is %d",main_netlist_idx );
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

    /** @brief Sets the current block which is being processed
         *
         * Used to determine which block a .cname, .param, .attr apply to
         */
        void set_curr_block(AtomBlockId blk) {
            curr_block_ = blk;
        }

        ///@brief Gets the current block which is being processed
        AtomBlockId curr_block() const {
            return curr_block_;
        }
        ///@brief Retieves a reference to the currently active .model
           AtomNetlist& curr_model() {
               //if (edif_models_.empty() || ended_) {
                 //  vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "Expected .model");
               //}

               return edif_models_[edif_models_.size() - 1];
           }


};
    AtomNetlist read_edif(e_circuit_format circuit_format,
                          const char* edif_file,
                          const t_model* user_models,
                          const t_model* library_models) {
        AtomNetlist netlist;
        t_model* blk_model;
        blk_model=new t_model;
       // main_netlist_ = AtomNetlist(top_cell_, netlist_id);
        printf("reading inputs");
        FILE* fp = fopen("/home/users/raza.jafari/netlists/edif/and2_output_edif.edn", "r");
        struct SNode* node = snode_parse(fp);
        printf("reading inputs==========");

        std::string netlist_id = vtr::secure_digest_file(edif_file);
        usefull_data u1;
        std::string top_cell = (u1.find_top_(node));
     //  netlist = AtomNetlist(top_cell, netlist_id);
       // read_ios( netlist,u1, node, blk_model);
       // read_blocks( netlist,u1, node, blk_model);
       // read_nets( netlist,u1, node, blk_model);
        //void read_ios(netlist,u1, node, blk_model);
        printf("\n \n reading complete");
        EDIFReader reader(circuit_format, netlist, netlist_id, user_models, library_models, u1, node);
        fclose(fp);
        return netlist;
    }
