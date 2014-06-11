/* Author: Long Yu Wang
 * Date: August 2013
 *
 * Author: Matthew J.P. Walker
 * Date: May 2014
 */

#ifndef INTRA_LOGIC_BLOCK_H
#define INTRA_LOGIC_BLOCK_H

#include "vpr_types.h"
#include "draw_types.h"
#include "netlist.h"
#include "route_tree_timing.h"
#include <unordered_set>

struct t_selected_sub_block_info {
private:
	struct clb_pin_tuple {
		int clb_index;
		int port_number;
		int pin_number;
		const t_pb_graph_node* pb_gnode;

		clb_pin_tuple(int clb_index, int port_number, int pin_number, const t_pb_graph_node* pb_gnode);
		clb_pin_tuple(const t_net_pin& pin, const t_pb_graph_node* pb_gnode);
		bool operator==(const clb_pin_tuple&) const;
	};

	struct sel_subblk_hasher {
		inline std::size_t operator()(const std::pair<const t_pb_graph_node*, const t_block*>& v) const {
			std::hash<const void*> ptr_hasher;
			return ptr_hasher((const void*)v.first) ^ ptr_hasher((const void*)v.second);
		}

		inline std::size_t operator()(const std::pair<clb_pin_tuple, clb_pin_tuple>& v) const {
			return (*this)(v.first) ^ (*this)(v.second);
		}

		inline std::size_t operator()(const clb_pin_tuple& v) const {
			std::hash<int> int_hasher;
			std::hash<const void*> ptr_hasher;
			return int_hasher(v.clb_index) ^ int_hasher(v.port_number)
				^ int_hasher(v.pin_number) ^ ptr_hasher((const void*)v.pb_gnode);
		}
	};

	t_pb* selected_pb;
	t_block* containing_block;
	t_pb_graph_node* selected_pb_gnode;
	std::unordered_set< std::pair<const t_pb_graph_node*, const t_block*>, sel_subblk_hasher> sinks;
	std::unordered_set< std::pair<const t_pb_graph_node*, const t_block*>, sel_subblk_hasher> sources;
	std::unordered_set< std::pair<clb_pin_tuple, clb_pin_tuple>, sel_subblk_hasher> on_critical_path;
	std::unordered_set< std::pair<const t_pb_graph_node*, const t_block*>, sel_subblk_hasher> in_selected_subtree;

	void add_sinks_and_sources_of(const t_pb_graph_node* pb_gnode, const t_block* clb);
public:

	t_selected_sub_block_info();

	void set(t_pb* new_selected_sub_block, t_block* containing_block);
	void add_to_critical_path(const t_tnode& src_tnode, const t_tnode& sink_tnode);

	t_pb_graph_node* get_selected_pb_gnode() const;
	t_block* get_containing_block() const;

	/*
	 * gets the t_pb that is currently selected. Please don't use this if
	 * you think you can get away with using is_selected(...) or get_selected_pb_gnode() and 
	 * get_containing_block(). May disappear in future. 
	 */
	t_pb* get_selected_pb() const;

	bool has_selection() const;
	void clear();
	void clear_critical_path();
	bool is_selected(const t_pb_graph_node* test, const t_block* test_block) const;
	bool is_child_of_selected(const t_pb_graph_node* test, const t_block* test_block) const;

	bool is_sink_of_selected(const t_pb_graph_node* test, const t_block* test_block) const;
	bool is_source_of_selected(const t_pb_graph_node* test, const t_block* test_block) const;
	bool is_on_critical_path(const t_net_pin& test_src, const t_pb_graph_node* test_src_pb_gnode,
	                         const t_net_pin& test_sink, const t_pb_graph_node* test_sink_pb_gnode) const;
	bool is_in_selected_subtree(const t_pb_graph_node* test, const t_block* test_block) const;
};

/* Enable/disable clb internals drawing. Internals drawing is enabled with a click of the
 * "Blk Internal" button. With each consecutive click of the button, a lower level in the 
 * pb_graph will be shown for every clb. When the number of clicks on the button exceeds 
 * the maximum level of sub-blocks that exists in the pb_graph, internals drawing
 * will be disabled.
 */
void toggle_blk_internal(void (*drawscreen_ptr)(void));

/* This function pre-allocates space to store bounding boxes for all sub-blocks. Each 
 * sub-block is identified by its descriptor_type and a unique pin ID in the type.
 */
void draw_internal_alloc_blk();

/* This function initializes bounding box values for all sub-blocks. It calls helper functions 
 * to traverse through the pb_graph for each descriptor_type and compute bounding box values.
 */
void draw_internal_init_blk();

/* Top-level drawing routine for internal sub-blocks. The function traverses through all 
 * grid tiles and calls helper function to draw inside each block.
 */
void draw_internal_draw_subblk();

/* Determines which part of a block to highlight, and stores it,
 * so that the other subblock drawing functions will obey it.
 * If the user missed all sub-parts, will return 1, else 0.
 */
int highlight_sub_block(const t_point& point_in_clb, t_block& clb);

/*
 * returns the struct with information about the sub-block selection
 */
t_selected_sub_block_info& get_selected_sub_block_info();

/*
 * Draws lines from the proper logical sources, to the proper logical sinks.
 * If the draw state says to show all logical connections, it will,
 * and if there is a selected sub-block, it will highlight it's conections
 */
void draw_logical_connections();

#endif /* INTRA_LOGIC_BLOCK_H */
