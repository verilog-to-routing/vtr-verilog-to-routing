#ifndef VPR_PB_GRAPH_WALKER_H
#define VPR_PB_GRAPH_WALKER_H

//Utility class for walking through the PB graph
//
//The virtual methods on_node()/on_pin()/on_edge() are called 
//precisely once on every t_pb_graph_node/t_pb_graph_pin/
//t_pb_graph_edge in the PB //Graph. 
//
//Clients can override the on_*() methods to implement thier
//own operations, relying on PbGraphWalker to traversal/walk
//the PB graph.
class PbGraphWalker {
    public:
        virtual ~PbGraphWalker() = default;

        //Walks the tree from the specified node.
        // Note that his may walk pins/edges of the
        // parent of node
        void walk(t_pb_graph_node* node) {
            if (!node) return; //Nothing to do

            //Find the root pb_graph_node
            while (node->parent_pb_graph_node) {
                node = node->parent_pb_graph_node;
            }
            walk_node(node);
        }

        //Finds the root of the PB graph containing node,
        //and walks the entire tree from the root.
        //
        //This ensures the entire tree is walked.
        void walk_from_root(t_pb_graph_node* node) {
            if (!node) return; //Nothing to do

            //Find the root pb_graph_node
            while (node->parent_pb_graph_node) {
                node = node->parent_pb_graph_node;
            }
            walk_node(node);
        }

        virtual void on_node(t_pb_graph_node* /*node*/) {};
        virtual void on_pin(t_pb_graph_pin* /*pin*/) {};
        virtual void on_edge(t_pb_graph_edge* /*edge*/) {};
    private:
        //Walks a node, all its associated pins and all its child nodes
        void walk_node(t_pb_graph_node* node) {
            if (visited(node)) return;
            mark_visited(node);

            on_node(node);

            //Pins
            for (int iport = 0; iport < node->num_input_ports; ++iport) {
                for (int ipin = 0; ipin < node->num_input_pins[iport]; ++ipin) {
                    walk_pin(&node->input_pins[iport][ipin]); 
                }
            }

            for (int iport = 0; iport < node->num_output_ports; ++iport) {
                for (int ipin = 0; ipin < node->num_output_pins[iport]; ++ipin) {
                    walk_pin(&node->output_pins[iport][ipin]); 
                }
            }

            for (int iport = 0; iport < node->num_clock_ports; ++iport) {
                for (int ipin = 0; ipin < node->num_clock_pins[iport]; ++ipin) {
                    walk_pin(&node->clock_pins[iport][ipin]); 
                }
            }

            //Recurse on children
            for (int imode = 0; imode < node->pb_type->num_modes; ++imode) {
                t_mode* mode = &node->pb_type->modes[imode];
                for (int ichild = 0; ichild < mode->num_pb_type_children; ++ichild) {
                    for (int ipb = 0; ipb < mode->pb_type_children[ichild].num_pb; ++ipb) {
                        walk_node(&node->child_pb_graph_nodes[imode][ichild][ipb]);
                    }
                }
            }
        }

        //Walks a pin and all its associated edges
        void walk_pin(t_pb_graph_pin* pin) {
            if (visited(pin)) return;
            mark_visited(pin);

            on_pin(pin);

            //Edges
            for (int iedge = 0; iedge < pin->num_input_edges; ++iedge) {
                walk_edge(pin->input_edges[iedge]);
            }

            for (int iedge = 0; iedge < pin->num_output_edges; ++iedge) {
                walk_edge(pin->output_edges[iedge]);
            }
        }

        //Walks an edge and all its associated pins
        void walk_edge(t_pb_graph_edge* edge) {
            if (visited(edge)) return;
            mark_visited(edge);

            on_edge(edge);

            //Pins
            for (int ipin = 0; ipin < edge->num_input_pins; ++ipin) {
                walk_pin(edge->input_pins[ipin]);
            }

            for (int ipin = 0; ipin < edge->num_output_pins; ++ipin) {
                walk_pin(edge->output_pins[ipin]);
            }
        }

    private:

        //Utilites for clearing/marking/querying visited status
        void reset_visited() {
            visited_nodes_.clear();
            visited_pins_.clear();
            visited_edges_.clear();
        }

        void mark_visited(t_pb_graph_node* node) {
            visited_nodes_.insert(node);
        }

        void mark_visited(t_pb_graph_pin* pin) {
            visited_pins_.insert(pin);
        }

        void mark_visited(t_pb_graph_edge* edge) {
            visited_edges_.insert(edge);
        }

        bool visited(t_pb_graph_node* node) {
            return visited_nodes_.count(node);
        }

        bool visited(t_pb_graph_pin* pin) {
            return visited_pins_.count(pin);
        }

        bool visited(t_pb_graph_edge* edge) {
            return visited_edges_.count(edge);
        }

        //Visited state
        std::set<t_pb_graph_node*> visited_nodes_;
        std::set<t_pb_graph_pin*> visited_pins_;
        std::set<t_pb_graph_edge*> visited_edges_;
};

#endif
