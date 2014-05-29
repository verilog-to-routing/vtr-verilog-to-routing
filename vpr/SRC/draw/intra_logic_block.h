/* Author: Long Yu Wang
 * Date: August 2013
 *
 * Author: Matthew J.P. Walker
 * Date: May 2014
 */

#include "vpr_types.h"
#include <unordered_set>

struct t_selected_sub_block_info {
private:
	struct pair_hash {
		inline std::size_t operator()(const std::pair<const t_pb_graph_node*, const t_block*>& v) const {
			std::hash<const void*> ptr_hasher;
			return ptr_hasher((const void*)v.first) ^ ptr_hasher((const void*)v.second);
		}
	};

	t_pb* selected_pb;
	t_block* containing_block;
	t_pb_graph_node* selected_pb_gnode;
	std::unordered_set< std::pair<const t_pb_graph_node*, const t_block*>, pair_hash> sinks;
	std::unordered_set< std::pair<const t_pb_graph_node*, const t_block*>, pair_hash> sources;

	void add_sinks_and_sources_of(const t_pb_graph_node* g_node, const t_block* clb);
public:

	t_selected_sub_block_info();

	void set(t_pb* new_selected_sub_block, t_block* containing_block);

	t_pb_graph_node* get_selected_pb_gnode() const;
	t_block* get_containing_block() const;

	/*
	 * gets the t_pb that is currently selected. Please don't use this if
	 * you think you can get away with using get_selected_pb_gnode() and 
	 * get_containing_block(). May disappear in future. 
	 */
	t_pb* get_selected_pb() const;

	bool has_selection() const;
	void clear();
	bool is_selected(const t_pb_graph_node* test, const t_block* test_block) const;

	bool is_sink_of_selected(const t_pb_graph_node* test, const t_block* test_block) const;
	bool is_source_of_selected(const t_pb_graph_node* test, const t_block* test_block) const;
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
int highlight_sub_block(int blocknum, float rel_x, float rel_y);

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
