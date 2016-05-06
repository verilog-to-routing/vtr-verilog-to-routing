#include <iostream>
#include <sstream>
#include <set>
#include <map>
#include <vector>
#include <string>
#include <cassert>
#include "verilog_writer2.h"
#include "globals.h"

class PrintingVisitor : public NetlistVisitor {
    private:
        void visit_top_impl(const char* top_level_name) { 
            printf("Top: %s\n", top_level_name); 
        }

        void visit_clb_impl(const t_pb* clb) { 
            const t_pb_type* pb_type = clb->pb_graph_node->pb_type;
            printf("CLB: %s (%s)\n", clb->name, pb_type->name); 
        }

        void visit_atom_impl(const t_pb* atom) { 
            const t_pb_type* pb_type = atom->pb_graph_node->pb_type;
            const t_model* model = logical_block[atom->logical_block].model;
            printf("ATOM: %s (%s: %s)\n", atom->name, pb_type->name, model->name); 
        }
};



class VerilogWriterVisitor : public NetlistVisitor {
    public:
        VerilogWriterVisitor(std::ostream& os)
            : os_(os)
            {}

    private: //Internal types
        enum class PortDir {
            IN,
            OUT
        };

        struct Instance {
            Instance(std::string type_, std::string param_, std::string inst_name_, std::map<std::string,std::string> port_conns_)
                : type(type_)
                , param(param_)
                , inst_name(inst_name_)
                , port_connections(port_conns_)
                {}
            std::string type;
            std::string param;
            std::string inst_name;
            std::map<std::string,std::string> port_connections;
        };

        struct Assignment {
            Assignment(std::string lval_, std::string rval_)
                : lval(lval_)
                , rval(rval_)
                {}
            std::string lval;
            std::string rval;
        };
    private: //NetlistVisitor interface functions

        void visit_top_impl(const char* top_level_name) { 
            top_module_name_ = top_level_name;
        }

        void visit_atom_impl(const t_pb* atom) { 
            const t_pb_type* pb_type = atom->pb_graph_node->pb_type;
            const t_model* model = logical_block[atom->logical_block].model;

            if(model->name == std::string("input")) {
                inputs_.emplace_back(make_io(atom, PortDir::IN));
            } else if(model->name == std::string("output")) {
                outputs_.emplace_back(make_io(atom, PortDir::OUT));
            } else if(model->name == std::string("names")) {
                instances_.push_back(make_lut_instance(atom));
            }
            printf("ATOM: %s (%s: %s)\n", atom->name, pb_type->name, model->name); 
        }

        void finish_impl() {
            os_ << "module " << top_module_name_ << " (\n";
            for(auto iter = inputs_.begin(); iter != inputs_.end(); ++iter) {
                os_ << indent_ << "input " << *iter;
                if(iter + 1 != inputs_.end() || outputs_.size() > 0) {
                   os_ << ",";
                }
                os_ << "\n";
            }
            for(auto iter = outputs_.begin(); iter != outputs_.end(); ++iter) {
                os_ << indent_ << "output " << *iter;
                if(iter + 1 != outputs_.end()) {
                   os_ << ",";
                }
                os_ << "\n";
            }
            os_ << ");\n" ;

            os_ << "\n";
            os_ << indent_ << "//Wires\n";
            for(auto& kv : logical_net_drivers_) {
                os_ << indent_ << "wire " << kv.second << ";\n";
            }
            for(auto& kv : logical_net_sinks_) {
                for(auto& wire_name : kv.second) {
                    os_ << indent_ << "wire " << wire_name << ";\n";
                }
            }

            os_ << "\n";
            os_ << indent_ << "//IO assignments\n";
            for(auto& assign : assignments_) {
                os_ << indent_ << "assign " << assign.lval << " = " << assign.rval << ";\n";
            }

            os_ << "\n";
            os_ << indent_ << "//Interconnect\n";
            for(const auto& kv : logical_net_sinks_) {
                int atom_net_idx = kv.first;
                auto driver_iter = logical_net_drivers_.find(atom_net_idx);
                assert(driver_iter != logical_net_drivers_.end());
                const auto& driver_wire = driver_iter->second;

                for(auto& sink_wire : kv.second) {
                    std::string inst_name = driver_wire + "_to_" + sink_wire;
                    os_ << indent_ << "fpga_interconnect " << inst_name;
                    os_ << " (" << driver_wire << ", " << sink_wire << ");\n";
                }
            }

            os_ << "\n";
            os_ << indent_ << "//Cell instances\n";
            for(auto& inst : instances_) {
                //Instantiate the lut
                os_ << indent_ << inst.type << " #(" << inst.param << ") " << inst.inst_name << "(";

                //and all its named port connections
                const auto& port_conns = inst.port_connections;
                for(auto iter = port_conns.begin(); iter != port_conns.end(); ++iter) {
                    os_ << "." + iter->first << "(" << iter->second << ")";

                    if(iter != --inst.port_connections.end()) {
                        os_ << ", ";
                    }
                }
                os_ << ");\n";
            }

            os_ << "\n";
            os_ << "endmodule\n";
        }

    private: //Helper functions
        std::string escape_name(const char* name) {
            std::string escaped_name = name;
            for(size_t i = 0; i < escaped_name.size(); i++) {
                //Replace invalid verilog characters (e.g. '^' , '~' , '[' , etc. ) with '_'
                if(escaped_name[i] == '^' || 
                    (int) escaped_name[i] < 48 || 
                   ((int) escaped_name[i] > 57 && (int) escaped_name[i] < 65) || 
                   ((int) escaped_name[i] > 90 && (int) escaped_name[i] < 97) ||
                    (int) escaped_name[i] > 122) {
                    escaped_name[i]='_';
                }
            }
            return escaped_name;
        }

        int find_num_inputs(const t_pb* pb) {
            int count = 0;
            for(int i = 0; i < pb->pb_graph_node->num_input_ports; i++) {
                count += pb->pb_graph_node->num_input_pins[i];
            }
            return count;
        }

        std::string make_inst_wire(int atom_net_idx, std::string inst_name, PortDir dir, int port_idx, int pin_idx) {
            std::stringstream ss;
            ss << inst_name;
            if(dir == PortDir::IN) {
                ss << "_input";
            } else {
                assert(dir == PortDir::OUT);
                ss << "_output";
            }
            ss << "_" << port_idx;
            ss << "_" << pin_idx;

            std::string wire_name = ss.str();

            if(dir == PortDir::IN) {
                logical_net_sinks_[atom_net_idx].push_back(wire_name);
            } else {
                auto ret = logical_net_drivers_.insert(std::make_pair(atom_net_idx, wire_name));
                assert(ret.second); //Was inserted, drivers are unique
            }

            return wire_name;
        }

        std::string make_io(const t_pb* atom, PortDir dir) {
            std::string io_name = escape_name(atom->name);  
            

            const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;

            int cluster_pin_idx = -1;
            if(dir == PortDir::IN) {
                assert(pb_graph_node->num_output_ports == 1); //One output port
                assert(pb_graph_node->num_output_pins[0] == 1); //One output pin
                cluster_pin_idx = pb_graph_node->output_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster

            } else {
                assert(pb_graph_node->num_input_ports == 1); //One input port
                assert(pb_graph_node->num_input_pins[0] == 1); //One input pin
                cluster_pin_idx = pb_graph_node->input_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster

                //Trip off the starting 'out_' that vpr adds to uniqify outputs
                //this makes the port names match the input blif file
                io_name = std::string(io_name.begin() + 4, io_name.end());
            }

            const t_block* top_block = find_top_block(atom);

            int atom_net_idx = top_block->pb_route[cluster_pin_idx].atom_net_idx; //Connected net in atom netlist

            //Port direction is inverted (inputs drive internal nets, outputs sink internal nets)
            PortDir wire_dir = (dir == PortDir::IN) ? PortDir::OUT : PortDir::IN;

            auto wire_name = make_inst_wire(atom_net_idx, io_name, wire_dir, 0, 0);

            auto assignment = Assignment(io_name, wire_name);
            if(wire_dir == PortDir::IN) {
                assignments_.emplace_back(io_name, wire_name);
            } else {
                assignments_.emplace_back(wire_name, io_name);
            }
            
            return io_name;
        }

        Instance make_lut_instance(const t_pb* atom)  {
            //Determine what size LUT
            int lut_size = find_num_inputs(atom);

            //Determine the instance type
            std::stringstream ss;
            ss << "LUT_" << lut_size;
            auto inst_type = ss.str();

            //Determine the truth table
            std::stringstream lut_mask_param_ss;
            int lut_bits = (1 << lut_size);
            lut_mask_param_ss << lut_bits;
            lut_mask_param_ss << "'b";
            lut_mask_param_ss << load_lut_mask(lut_size, atom);
            std::string lut_mask_param = lut_mask_param_ss.str();

            //Determine the instance name
            auto inst_name = "lut_" + escape_name(atom->name);

            //Determine the port connections
            std::map<std::string,std::string> port_conns;

            const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;
            assert(pb_graph_node->num_input_ports == 1); //LUT has one input port

            const t_block* top_block = find_top_block(atom);

            //Add inputs adding connections
            for(int pin_idx = 0; pin_idx < pb_graph_node->num_input_pins[0]; pin_idx++) {
                int cluster_pin_idx = pb_graph_node->input_pins[0][pin_idx].pin_count_in_cluster; //Unique pin index in cluster
                int atom_net_idx = top_block->pb_route[cluster_pin_idx].atom_net_idx; //Connected net in atom netlist

                std::string port_name = "in_" + std::to_string(pin_idx);
                if(atom_net_idx == OPEN) {
                    //Disconnected

                    //We ground disconnected LUT input pins
                    auto ret = port_conns.insert(std::make_pair(port_name, "1'b0")); 
                    assert(ret.second); //Was inserted
                } else {
                    //Connected to a net

                    std::string input_net = make_inst_wire(atom_net_idx, inst_name, PortDir::IN, 0, pin_idx);
                    auto ret = port_conns.insert(std::make_pair(port_name, input_net)); 
                    assert(ret.second); //Was inserted
                }
            }

            //Add the single output connection
            {
                assert(pb_graph_node->num_output_ports == 1); //LUT has one output port
                assert(pb_graph_node->num_output_pins[0] == 1); //LUT has one output pin
                int cluster_pin_idx = pb_graph_node->output_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster
                int atom_net_idx = top_block->pb_route[cluster_pin_idx].atom_net_idx; //Connected net in atom netlist

                std::string port_name = "out_0";
                if(atom_net_idx == OPEN) {
                    //Disconnected

                    //We leave disconnected LUT output pins disconnected
                    auto ret = port_conns.insert(std::make_pair(port_name, "")); 
                    assert(ret.second); //Was inserted
                } else {
                    //Connected to a net

                    std::string input_net = make_inst_wire(atom_net_idx, inst_name, PortDir::OUT, 0, 0);
                    auto ret = port_conns.insert(std::make_pair(port_name, input_net)); 
                    assert(ret.second); //Was inserted
                }
            }

            return Instance(inst_type, lut_mask_param, inst_name, port_conns);
        }

        const t_block* find_top_block(const t_pb* curr) {
            //TODO: this is not very efficient...
            const t_pb* top_pb = find_top_clb(curr); 

            for(int i = 0; i < num_blocks; i++) {
                if(block[i].pb == top_pb) {
                    return &block[i];
                }
            }
            assert(false);
        }

        const t_pb* find_top_clb(const t_pb* curr) {
            //Walk up through the pb graph until curr
            //has no parent, at which point it will be the top pb
            const t_pb* parent = curr->parent_pb;
            while(parent != nullptr) {
                curr = parent;
                parent = curr->parent_pb;
            }
            return curr;
        }

        std::string load_lut_mask(int num_inputs, const t_pb* atom) {
            const t_model* model = logical_block[atom->logical_block].model;
            assert(model->name == std::string("names"));

            int lut_bits = (1 << num_inputs);

            //Initialize LUT mask to all false
            std::string lut_mask = std::string(lut_bits, '0'); 

            //VPR stores the truth table as in BLIF
            //Each row of the table (i.e. a c-string) is stored in a linked list 
            //
            //The c-string is the literal row from BLIF, e.g. "0 1" for an inverter, "11 1" for an AND2
            t_linked_vptr* truth_table_row_ptr = logical_block[atom->logical_block].truth_table;
            
            //Process each row
            while(truth_table_row_ptr != nullptr) {
                const std::string truth_table_row = (const char*) truth_table_row_ptr->data_vptr;

                std::cout << truth_table_row << std::endl;

                auto truth_val_iter = truth_table_row.end() - 1;
                auto space_iter = truth_table_row.end() - 2;

                assert(*space_iter == ' ');
                assert(*truth_val_iter == '1' || *truth_val_iter == '0');

                std::string input_values = std::string(truth_table_row.begin(), space_iter);
                assert(input_values.size() == truth_table_row.size() - 2);

                //Convert the set of input values to an index in the LUT mask
                //and set the corresponding entry to the truth value
                for(int lut_mask_idx : expand_input_values_to_lut_mask_index(lut_bits, input_values)) {
                    lut_mask[lut_mask_idx] = *truth_val_iter;
                }
                
                truth_table_row_ptr = truth_table_row_ptr->next;
            }

            return lut_mask;
        }

        std::vector<int> expand_input_values_to_lut_mask_index(int lut_bits, std::string input_values) {
            //Return a vector of indicies, since a single set of input values (if they have don't cares)
            //can expend to multiplie indicies.
            //However currently we do not support don't cares in the input values.
            std::vector<int> lut_mask_indicies;


            //We invert the index here to make the actual LUT mask parameter sensible,
            //by intializing it to the highest index in the lut_mask string, and then
            //subtracting the powers for each input that is specified as true.
            //
            //For example:
            //
            //  Consider an AND2 implemented in a 6-LUT
            //  The truth table in blif looks like:
            //  .names a b a_and_b
            //  11 1
            //
            //  The corresponding input_values is "11"
            //
            //  We initialize lut_mask_idx to 63 (since there are 2^6 == 64 bits in the lut mask)
            //
            //  This index corresponds to the right-most character in the std::string
            //  used to represent the lut mask:
            //
            //      0000000000000000000000000000000000000000000000000000000000000000
            //                                                                     ^
            //
            //  which corresponds to the LSB of the LUT mask when interpreted in verilog
            //
            //  We then subtract the corresponding power (i.e. 2^i) to get the string index
            //  corresponding to the appropriate case:
            //
            //      lut_mask_index = 63 - 2^0 - 2^1 = 60
            //
            //  Which is then used to set the appropriate lutmask bit
            //  
            //      0000000000000000000000000000000000000000000000000000000000001000
            //                                                                  ^   
            int lut_mask_idx = lut_bits-1;
            for(size_t i = 0; i < input_values.size(); i++) {
                int index_power = (1 << i); 

                if(input_values[i] == '1') {
                    lut_mask_idx -= index_power;
                } else {
                    assert(input_values[i] == '0'); //Currently no support for don't cares
                }
            }
            lut_mask_indicies.push_back(lut_mask_idx);

            return lut_mask_indicies;
        }

    private: //Data
        std::string top_module_name_;
        std::vector<std::string> inputs_;
        std::vector<std::string> outputs_;
        std::set<std::string> wires_;
        std::vector<Assignment> assignments_;
        std::vector<Instance> instances_;

        std::map<int, std::string> logical_net_drivers_;
        std::map<int, std::vector<std::string>> logical_net_sinks_;

        std::ostream& os_;
        std::string indent_ = "    ";
};

void verilog_writer2() {
    VerilogWriterVisitor visitor(std::cout);

    NetlistWalker nl_walker(visitor);

    nl_walker.walk();

}
