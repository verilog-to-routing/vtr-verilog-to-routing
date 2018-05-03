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
    //The set of nodes in the graph
    std::vector<t_lb_type_rr_node> nodes;

    public:
        //Returns true if inode an external src/sink/routing node
        bool is_external_node(int inode) const {
            return is_external_src_sink_node(inode) || is_external_routing_node(inode);    
        }

        //Returns true if inode an external src/sink node
        bool is_external_src_sink_node(int inode) const {
            for (int iextern = 0; iextern != (int) external_rr_info_.size(); ++iextern) {

                if (is_external_src_sink(inode, iextern)) {
                    return true;
                }
            }
            return false;
        }

        bool is_external_routing_node(int inode) const {
            for (int iextern = 0; iextern != (int) external_rr_info_.size(); ++iextern) {

                if (is_external_routing(inode, iextern)) {
                    return true;
                }
            }
            return false;
        }

        //Returns the set of pin classes assoicated with inode (only non-empty for external src/sink nodes)
        std::vector<int> node_classes(int inode) const {
            std::vector<int> classes;
            
            //NOTE: implemented as a linear search, can be made faster if required (currently used only for debug info)
            for (int iclass = 0; iclass < (int) class_to_external_rr_info_idx_.size(); ++iclass) {
                int iextern = class_to_external_rr_info_idx_[iclass];

                if (is_external_node(inode, iextern)) {
                    classes.push_back(iclass);
                }
            }

            return classes;
        }

        //Returns information about the external src/sink + routing nodes for the 
        //iclass'th pin class
        const t_lb_type_rr_external_info& class_external_rr_info(int iclass) const {
            return external_rr_info_[class_to_external_rr_info_idx_[iclass]];
        }

    public: //Mutators
        //Adds the specified external routing info, returning it's unique identifier
        int add_external_rr_info(t_lb_type_rr_external_info external_rr_info) {
            external_rr_info_.push_back(external_rr_info);
            return external_rr_info_.size() - 1;
        }

        //Sets the external routing info for iclass to iextern
        void set_class_external_rr_info(int iclass, int iextern) {
            if (class_to_external_rr_info_idx_.size() == 0 || iclass > (int) class_to_external_rr_info_idx_.size() - 1) {
                class_to_external_rr_info_idx_.resize(iclass + 1, OPEN);
            }
            VTR_ASSERT(iclass < (int) class_to_external_rr_info_idx_.size());
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
};

//Auxilary information about an RR graph
class t_lb_type_rr_graph_info {
    public:
        t_lb_type_rr_graph_info(size_t num_nodes)
            : reachable_nodes_(num_nodes)
            , external_sources_reaching_(num_nodes)
            , external_sinks_reachable_(num_nodes) {}


        //Returns the nodes reachable (downstream) from inode
        const std::vector<int>& reachable_nodes(int inode) const {
            return reachable_nodes_[inode];
        }

        bool is_reachable(int inode, int idest) const {
            return std::find(reachable_nodes_[inode].begin(), reachable_nodes_[inode].end(), idest) != reachable_nodes_[inode].end();
        }

        const std::vector<int>& external_sources_reaching(int inode) const {
            return external_sources_reaching_[inode];
        }

        const std::vector<int>& external_sinks_reachable(int inode) const {
            return external_sinks_reachable_[inode];
        }

    public:
        template<class Iterator>
        void add_reachable(int inode, Iterator first, Iterator last) {
            reachable_nodes_[inode].insert(reachable_nodes_[inode].end(), first, last);
        }

        template<class Iterator>
        void add_external_sources_reaching(int inode, Iterator first, Iterator last) {
            external_sources_reaching_[inode].insert(external_sources_reaching_[inode].begin(), first, last);
        }

        template<class Iterator>
        void add_external_sinks_reachable(int inode, Iterator first, Iterator last) {
            external_sinks_reachable_[inode].insert(external_sinks_reachable_[inode].end(), first, last);
        }
    private:
        std::vector<std::vector<int>> reachable_nodes_;
        std::vector<std::vector<int>> external_sources_reaching_;
        std::vector<std::vector<int>> external_sinks_reachable_;
};

/* Constructors/Destructors */
std::vector<t_lb_type_rr_graph> alloc_and_load_all_lb_type_rr_graph();

std::vector<t_lb_type_rr_graph_info> profile_lb_type_rr_graphs(const std::vector<t_lb_type_rr_graph>& lb_type_rr_graphs);

/* Debug functions */
void echo_lb_type_rr_graphs(char *filename, const std::vector<t_lb_type_rr_graph>& lb_type_rr_graphs);



#endif


