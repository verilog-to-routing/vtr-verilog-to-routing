#include <iostream>
#include <cstring>
#include <vector>
#include <map>
#include <assert.h>
#include <math.h>

#include "lut_stats.h"
#include "vtr_assert.h"

using std::cout;
using std::endl;

using std::pair;
using std::vector;
using std::map;

bool node_is_lut(t_node* node);
map<t_node*, size_t> carry_chain_lengths(t_module* module, vector<t_node*> start_nodes);
size_t chain_length(t_node* node, map<t_pin_def*,t_node*>& net_to_node_map);

void print_lut_stats(t_module* module) {
    print_lut_carry_stats(module);
}

void print_lut_carry_stats(t_module* module) {
    size_t num_luts = 0;
    size_t num_carry_luts = 0;

    vector<t_node*> carry_chain_starts;

    for(int i = 0; i < module->number_of_nodes; i++) {
        t_node* node = module->array_of_nodes[i];

        if(node_is_lut(node)) {
            num_luts++;
            pair<bool,bool> lut_info = is_carry_chain_lut(node);

            if(lut_info.first) { //First is whether this is a carry cell
                //cout << "Found carry lut " << node->type << " " << node->name << endl;
                num_carry_luts++;
            }

            if(lut_info.second) { //Second is whether this is the start of a chian
                //cout << "Found chain start " << node->type << " " << node->name << endl;
                carry_chain_starts.push_back(node);
            }
        }
    }

    map<t_node*, size_t> chain_lengths = carry_chain_lengths(module, carry_chain_starts);

    double geomean = 1.0;
    double mean = 0.0;
    int min_len = -1;
    int max_len = -1;

    if(chain_lengths.size() > 0) {
        for(map<t_node*, size_t>::iterator iter = chain_lengths.begin(); iter != chain_lengths.end(); iter++) {
            pair<t_node*, size_t> chain_size = *iter;

            //cout << "Chain " << chain_size.first->type << " " << chain_size.first->name << " Length: " << chain_size.second << " bits" << endl;

            geomean *= chain_size.second;
            mean += chain_size.second;

            if(min_len == -1) {
                min_len = (int) chain_size.second;
            } else if((int) chain_size.second < min_len) {
                min_len = (int) chain_size.second;
            }

            if(max_len == -1) {
                max_len = (int) chain_size.second;
            } else if ((int) chain_size.second > max_len) {
                max_len = (int) chain_size.second;
            }
        }

        geomean = pow(geomean, 1.0/chain_lengths.size());
        mean /= (double) chain_lengths.size();
    } else {
        mean = 0.0;
        min_len = 0.0;
        max_len = 0.0;
    }
    
    cout << "\tLUTs: " << num_luts << ", Carry_LUTs: " << num_carry_luts << endl;
    cout << "\tCarry Chains: " << chain_lengths.size() << ", Mean Length (bits): " << mean << ", Min Length (bits): " << min_len << ", Max Length (bits): " << max_len << endl;
}


/*
 * Identify luts using carry chains
 */

pair<bool,bool> is_carry_chain_lut(t_node* node) {
    bool is_carry_chain_lut = false;
    bool is_carry_chain_start = false;

    bool found_cout = false;
    bool found_cin_or_sharein = false;

    for(int i = 0; i < node->number_of_ports; i++) {
        t_node_port_association* node_port = node->array_of_ports[i];
        //A port only appears in the VQM netlist if it is connected
        
        if(strcmp(node_port->port_name, "cout") == 0) {
            found_cout = true;
        }

        //Hardcoded for Stratix IV
        if(strcmp(node_port->port_name, "cin") == 0 ||
           found_cout ||
           strcmp(node_port->port_name, "sumout") == 0) {
            is_carry_chain_lut = true;
        }

        if(strcmp(node_port->port_name, "cin") == 0 ||
           strcmp(node_port->port_name, "sharein") == 0) {
            //According to stratixii_eda_fd.pdf, carry chains
            // always start with a constant input to one of these ports
            found_cin_or_sharein = true;
            
            //Constants are called 'gnd' or 'vcc'
            if(strcmp(node_port->associated_net->name, "gnd") == 0 ||
               strcmp(node_port->associated_net->name, "vcc") == 0 ) {
                is_carry_chain_start = true;
            }
        }
    }

    //We may have cleaned the netlist to remove gnd and vcc constants...
    //In that case we may have a start of chain if there is a cout, but no
    //cin.  I.E. found_cout && !found_cin_or_sharein
    if(!is_carry_chain_start && found_cout && !found_cin_or_sharein) {
        is_carry_chain_start = true;
    }

    //Must be a chain lut if we are the chain start
    if(is_carry_chain_start) 
        VTR_ASSERT(is_carry_chain_lut);

    return pair<bool,bool>(is_carry_chain_lut, is_carry_chain_start);
}

bool node_is_lut(t_node* node) {
    //Hardcoded for Stratix IV
    if(strcmp(node->type, "stratixiv_lcell_comb") == 0) {
        return true;
    }

    return false;
}

map<t_node*, size_t> carry_chain_lengths(t_module* module, vector<t_node*> start_nodes) {
    map<t_node*, size_t> chain_lengths;

    //Build a net to node map for the cin port
    //  This is required to avoid slow reverse look-ups from cout net to assoicated cin nodes
    map<t_pin_def*, t_node*> net_to_node_map;
    for(int i = 0; i < module->number_of_nodes; i++) {
        t_node* temp_node = module->array_of_nodes[i];
        for(int j = 0 ; j < temp_node->number_of_ports; j++) {
            t_node_port_association* temp_node_port = temp_node->array_of_ports[j];

            if(strcmp(temp_node_port->port_name, "cin") == 0) {
                t_pin_def* associated_net = temp_node_port->associated_net;

                net_to_node_map[associated_net] = temp_node;
            }
        }
    }

    //Determine the length of each carry chain
    for(vector<t_node*>::iterator iter = start_nodes.begin(); iter != start_nodes.end(); iter++) {
        t_node* node = *iter;

        chain_lengths[node] = chain_length(node, net_to_node_map);
    }

    return chain_lengths;

}

size_t chain_length(t_node* node, map<t_pin_def*,t_node*>& net_to_node_map) {
    //Follow the cout port

    //Find the cout port
    t_node* next_node = NULL;
    for(int i = 0; i < node->number_of_ports; i++) {
        t_node_port_association* node_port = node->array_of_ports[i];
        
        if(strcmp(node_port->port_name, "cout") == 0) {
            t_pin_def* associated_net = node_port->associated_net;

            //ensure the net is in the look-up
            VTR_ASSERT(net_to_node_map.count(associated_net));

            //Fast reverse lookup
            next_node = net_to_node_map[associated_net];
        }
    }

    if(next_node == NULL) {
        return 1; //Last node in the chain
    } else {
        return 1 + chain_length(next_node, net_to_node_map); //Recursively count the chain
    }
}
