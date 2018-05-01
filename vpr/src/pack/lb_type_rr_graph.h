/* 
  Functions to creates, manipulate, and free the lb_type_rr_node graph that represents interconnect within a logic block type.

  Author: Jason Luu
  Date: July 22, 2013
 */

#ifndef LB_TYPE_RR_GRAPH_H
#define LB_TYPE_RR_GRAPH_H

#include <algorithm>
#include "pack_types.h"

/**************************************************************************
* Intra-Logic Block Routing Data Structures (by type)
***************************************************************************/
/* Describes different types of intra-logic cluster routing resource nodes */
enum e_lb_rr_type {
	LB_SOURCE = 0, LB_SINK, LB_INTERMEDIATE, NUM_LB_RR_TYPES
};
const std::vector<const char*> lb_rr_type_str {
    "LB_SOURCE", "LB_SINK", "LB_INTERMEDIATE", "INVALID"
};


/* Output edges of a t_lb_type_rr_node */
struct t_lb_type_rr_node_edge {
    t_lb_type_rr_node_edge (int node, float cost)
        : node_index(node), intrinsic_cost(cost) {}

	int node_index;
	float intrinsic_cost;
};

/* Describes a routing resource node within a logic cluster type */
struct t_lb_type_rr_node {
	short capacity;			/* Number of nets that can simultaneously use this node */
	enum e_lb_rr_type type;	/* Type of logic cluster_ctx.blocks resource node */	

    std::vector<std::vector<t_lb_type_rr_node_edge>> outedges; /* [0..num_modes - 1][0..num_fanout-1] index and cost of out edges */

	t_pb_graph_pin *pb_graph_pin;	/* pb_graph_pin associated with this lb_rr_node if exists, NULL otherwise */
	float intrinsic_cost;					/* cost of this node */
	
	t_lb_type_rr_node() {
		capacity = 0;
		type = NUM_LB_RR_TYPES;
		pb_graph_pin = nullptr;
		intrinsic_cost = 0;
	}

    short num_modes() const { 
        return outedges.size();
    }

    short num_fanout(int mode) const { 
        VTR_ASSERT(mode < (int)outedges.size());
        return outedges[mode].size();
    }
};

struct t_lb_type_rr_external_info {
    int src_sink_node = OPEN;
    int routing_node = OPEN;
};

/*
 * The routing resource graph for a logic cluster type
 */
struct t_lb_type_rr_graph {
    std::vector<t_lb_type_rr_node> nodes;

    public:
        //Returns true if inode is a default external src/sink/routing node
        bool is_external_default_node(int inode) const {
            return is_external_node(inode, default_external_src_rr_info_idx_)
                || is_external_node(inode, default_external_sink_rr_info_idx_);
        }

        //Returns true if inode is a non-default external src/sink/routing node
        bool is_external_non_default_node(int inode) const {
            for (int iextern = 0; iextern != external_rr_info_.size(); ++iextern) {
                if (iextern == default_external_src_rr_info_idx_) continue;
                if (iextern == default_external_sink_rr_info_idx_) continue;

                if (is_external_node(inode, iextern)) {
                    return true;
                }
            }
            return false;
        }

        //Returns true if inode an external src/sink/routing node
        bool is_external_node(int inode) const {
            return is_external_default_node(inode) || is_external_non_default_node(inode);    
        }

        //Returns true if inode an external src/sink node
        bool is_external_src_sink_node(int inode) const {
            for (int iextern = 0; iextern != external_rr_info_.size(); ++iextern) {

                if (is_external_src_sink(inode, iextern)) {
                    return true;
                }
            }
            return false;
        }

        //Returns the set of pin classes assoicated with inode (only non-empty for external src/sink nodes)
        std::vector<int> node_classes(int inode) const {
            std::vector<int> classes;
            
            for (int iclass = 0; iclass < class_to_external_rr_info_idx_.size(); ++iclass) {
                int iextern = class_to_external_rr_info_idx_[iclass];

                if (is_external_node(inode, iextern)) {
                    classes.push_back(iclass);
                }
            }

            return classes;
        }

        //Returns information about the default external source + routing nodes
        const t_lb_type_rr_external_info& default_external_src_rr_info() const {
            return external_rr_info_[default_external_src_rr_info_idx_];
        }

        //Returns information about the default external sink + routing nodes
        const t_lb_type_rr_external_info& default_external_sink_rr_info() const {
            return external_rr_info_[default_external_sink_rr_info_idx_];
        }

        //Returns information about the external src/sink + routing nodes for the 
        //iclass'th pin class
        const t_lb_type_rr_external_info& external_rr_info(int iclass) const {
            return external_rr_info_[class_to_external_rr_info_idx_[iclass]];
        }

    public: //Mutators
        //Adds the specified external routing info, returning it's unique identifier
        int add_external_rr_info(t_lb_type_rr_external_info external_rr_info) {
            external_rr_info_.push_back(external_rr_info);
            return external_rr_info_.size() - 1;
        }

        //Sets the default external source routing info to iextern
        void set_default_external_src_rr_info(int iextern) {
            default_external_src_rr_info_idx_ = iextern;
        }

        //Sets the default external sink routing info to iextern
        void set_default_external_sink_rr_info(int iextern) {
            default_external_sink_rr_info_idx_ = iextern;
        }

        //Sets the external routing info for iclass to iextern
        void set_class_external_rr_info(int iclass, int iextern) {
            if (class_to_external_rr_info_idx_.size() == 0 || iclass > class_to_external_rr_info_idx_.size() - 1) {
                class_to_external_rr_info_idx_.resize(iclass + 1, OPEN);
            }
            VTR_ASSERT(iclass < class_to_external_rr_info_idx_.size());
            class_to_external_rr_info_idx_[iclass] = iextern;
        }

    private:
        //Returns true if inode is a source/sink node of the iextern'th
        //external node info
        bool is_external_src_sink(int inode, int iextern) const {
            return inode == external_rr_info_[iextern].src_sink_node;
        }

        //Returns true if inode is a routing node of the iextern'th
        //external node info
        bool is_external_routing(int inode, int iextern) const {
            return inode == external_rr_info_[iextern].routing_node;
        }

        //Returns true if inode is a src/sink/routing node of the iextern'th
        //external node info
        bool is_external_node(int inode, int iextern) const {
            return is_external_src_sink(inode, iextern)
                || is_external_routing(inode, iextern);
        }

    private: //Data
        std::vector<t_lb_type_rr_external_info> external_rr_info_;
        std::vector<int> class_to_external_rr_info_idx_; //[0..num_class-1]
        int default_external_sink_rr_info_idx_ = OPEN;
        int default_external_src_rr_info_idx_ = OPEN;

};

/* Constructors/Destructors */
std::vector <t_lb_type_rr_graph> alloc_and_load_all_lb_type_rr_graph();

/* Accessor functions */
int get_lb_type_rr_graph_ext_source_index(t_type_ptr lb_type, const t_lb_type_rr_graph& lb_rr_graph, const AtomPinId pin);
int get_lb_type_rr_graph_ext_sink_index(t_type_ptr lb_type, const t_lb_type_rr_graph& lb_rr_graph, const AtomPinId pin);
int get_num_modes_of_lb_type_rr_node(const t_lb_type_rr_node &lb_type_rr_node);

/* Debug functions */
void echo_lb_type_rr_graphs(char *filename, const std::vector<t_lb_type_rr_graph>& lb_type_rr_graphs);

#endif


