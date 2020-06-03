#ifndef PREPROCESS_H
#define PREPROCESS_H

//============================================================================================
//				INCLUDES
//============================================================================================

#include "vqm2blif_util.h"

//============================================================================================
//				GLOBALS
//============================================================================================
void preprocess_netlist(t_module* module, t_arch* arch, t_logical_block_type* arch_types, int num_types,
                        t_boolean fix_global_nets, t_boolean elaborate_ram_clocks, t_boolean single_clock_primitives,
                        t_boolean split_carry_chain_logic, t_boolean remove_const_nets);

//============================================================================================
//				STRUCTURES & TYPEDEFS
//============================================================================================
typedef vector<t_assign*> t_assign_vec;
typedef vector<t_node_port_association*> t_port_vec;

typedef struct s_split_inout_pin {
    t_pin_def* in;
    t_pin_def* out;
} t_split_inout_pin;


struct cmp_str {
    bool operator()(char const* a, char const* b) {
        return strcmp(a, b) < 0;
    }
};
typedef set<char*, cmp_str> t_str_ports; //Vector of port names
typedef map<char*, t_str_ports*, cmp_str> t_global_ports; //Key primitive type, value vector of port names
typedef set<t_pin_def*> t_global_nets;

typedef enum e_driver_type {ASSIGN, NODE_PORT} t_driver_type;
typedef struct s_net_driver {
    t_driver_type driver_type;
    union u_driver {
        t_assign* assign;
        union u_node_port {
            t_node_port_association* node_port;
            t_node* node;
        } node_port;
    } driver;
    bool is_global;
} t_net_driver;
typedef map<t_pin_def*, t_net_driver> t_net_driver_map;

typedef vector<t_node_port_association*> t_node_port_vec;
typedef struct s_node_port_vec_pair {
    t_node_port_vec global_to_local;
    t_node_port_vec local_to_global;
} t_node_port_vec_pair;

typedef struct s_assign_vec_pair {
    t_assign_vec global_to_local;
    t_assign_vec local_to_global;
} t_assign_vec_pair;

typedef enum e_buffer_type {G2L_BUFFER, L2G_BUFFER} t_buffer_type; //Global to Local, Local to Global

typedef queue<t_pb_type*> t_pb_type_queue;
//============================================================================================
//				PREPROCESS DEFINES
//============================================================================================
//Split RAM nodes
#define SPLIT_A_POSTFIX "__split_A__"
#define SPLIT_B_POSTFIX "__split_B__"
#define DUMMY_NET_NAME_FORMAT "__dummy_%d_A_B__"
#define DUMMY_A_PORT_NAME "__dummy_molecule_A__"
#define DUMMY_B_PORT_NAME "__dummy_molecule_B__"


//Global local prefixes
#define INOUT_INPUT_PREFIX "__in__"
#define INOUT_OUTPUT_PREFIX "__out__"

#define BLIF_NAME_PREFIX ".subckt "
#define FAKE_BUFFER_INST_FORMAT "__inst%d__"

#define FAKE_G2L_BUFFER_TYPE "fake_g2l_buf"
#define FAKE_G2L_BUFFER_INPUT_PORT_NAME "global_in"
#define FAKE_G2L_BUFFER_OUTPUT_PORT_NAME "local_out"
#define FAKE_G2L_BUFFER_NEW_NET_FORMAT "__localized%d__"

#define FAKE_L2G_BUFFER_TYPE "fake_l2g_buf"
#define FAKE_L2G_BUFFER_INPUT_PORT_NAME "local_in"
#define FAKE_L2G_BUFFER_OUTPUT_PORT_NAME "global_out"
#define FAKE_L2G_BUFFER_NEW_NET_FORMAT "__globalized%d__"
#define FAKE_L2G_BUFFER_NEW_NET_PREGLOBAL_FORMAT "__preglobal%d__"

//If an assignment's global sinks are more than this fraction of
// it's total sinks, then make the assignment's output a global
#define PROMOTE_NET_GLOBAL_SINK_FRAC 0.5

#endif
