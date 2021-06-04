#include "manual_moves.h"
#include "move_utils.h"
#include "globals.h"
//#include "draw_debug.h"
#include "draw.h"
#include "move_generator.h"

#ifndef NO_GRAPHICS

//Global Variables
ManualMovesGlobals manual_moves_global;

void draw_manual_moves_window(std::string block_id) {

	std::cout << "Entered function in different file" << std::endl;

	if(!manual_moves_global.mm_window_is_open) {
		manual_moves_global.mm_window_is_open = true;

		//Window settings
		manual_moves_global.manual_move_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_position((GtkWindow*)manual_moves_global.manual_move_window, GTK_WIN_POS_CENTER);
		gtk_window_set_title((GtkWindow*)manual_moves_global.manual_move_window, "Manual Moves Generator");
		gtk_widget_set_name(manual_moves_global.manual_move_window, "manualMovesWindow");

		
		GtkWidget* grid = gtk_grid_new();
       	GtkWidget* block_entry = gtk_entry_new();
       	gtk_entry_set_text((GtkEntry*)block_entry, block_id.c_str());
       	GtkWidget* x_position_entry = gtk_entry_new();
       	GtkWidget* y_position_entry = gtk_entry_new();
		GtkWidget* subtile_position_entry = gtk_entry_new();
       	GtkWidget* block_label = gtk_label_new("Block ID/Block Name:");
     	GtkWidget* to_label = gtk_label_new("To Location:");
        GtkWidget* x = gtk_label_new("x:");
        GtkWidget* y = gtk_label_new("y:");
		GtkWidget* subtile = gtk_label_new("Subtile:");

        GtkWidget* calculate_cost_button = gtk_button_new_with_label("Calculate Costs");
		std::cout << "In draw_mm_window: " << block_id << std::endl;

        	//Add all to grid
        	gtk_grid_attach((GtkGrid*)grid, block_label, 0, 0, 1, 1);
        	gtk_grid_attach((GtkGrid*)grid, block_entry, 0, 1, 1, 1);
        	gtk_grid_attach((GtkGrid*)grid, to_label, 2, 0, 1, 1);
        	gtk_grid_attach((GtkGrid*)grid, x, 1, 1, 1, 1);
        	gtk_grid_attach((GtkGrid*)grid, x_position_entry, 2, 1, 1, 1);
        	gtk_grid_attach((GtkGrid*)grid, y, 1, 2, 1, 1);
        	gtk_grid_attach((GtkGrid*)grid, y_position_entry, 2, 2, 1, 1);
        	gtk_grid_attach((GtkGrid*)grid, subtile, 1, 3, 1, 1);
        	gtk_grid_attach((GtkGrid*)grid, subtile_position_entry, 2, 3, 1, 1);
        	gtk_grid_attach((GtkGrid*)grid, calculate_cost_button, 0, 4, 3, 1); //spans three columns

       		//Set margins
        	gtk_widget_set_margin_bottom(grid, 20);
        	gtk_widget_set_margin_top(grid, 20);
        	gtk_widget_set_margin_start(grid, 20);
        	gtk_widget_set_margin_end(grid, 20);
        	gtk_widget_set_margin_bottom(block_label, 5);
        	gtk_widget_set_margin_bottom(to_label, 5);
        	gtk_widget_set_margin_top(calculate_cost_button, 15);
        	gtk_widget_set_margin_start(x, 13);
        	gtk_widget_set_margin_start(y, 13);
        	gtk_widget_set_halign(calculate_cost_button, GTK_ALIGN_CENTER);

		g_signal_connect(G_OBJECT(manual_moves_global.manual_move_window), "destroy", G_CALLBACK(close_manual_moves_window), NULL);
        	
		//connect signals
        	g_signal_connect(calculate_cost_button, "clicked", G_CALLBACK(calculate_cost_callback), grid);


        	gtk_container_add(GTK_CONTAINER(manual_moves_global.manual_move_window), grid);
        	gtk_widget_show_all(manual_moves_global.manual_move_window);
    }			
}

void calculate_cost_callback(GtkWidget* /*widget*/, GtkWidget* grid) {
	
	int block_id;
	int x_location;
	int y_location;
	int subtile_location;
	bool valid_input = true;
	auto& cluster_ctx = g_vpr_ctx.clustering();
	
	//Getting entry values
	GtkWidget* block_entry = gtk_grid_get_child_at((GtkGrid*)grid, 0, 1);
	std::string block_id_string = gtk_entry_get_text((GtkEntry*)block_entry);
		
	if(string_is_a_number(block_id_string)) { //for block ID
		block_id = std::atoi(block_id_string.c_str());
	}
	else { //for block name
		block_id = size_t(cluster_ctx.clb_nlist.find_block(gtk_entry_get_text((GtkEntry*)block_entry)));
	}
	//if the block is not found 
	if((!cluster_ctx.clb_nlist.valid_block_id(ClusterBlockId(block_id))) || (cluster_ctx.clb_nlist.find_blockblock_id) == BlockId::INVALID)) {
		invalid_breakpoint_entry_window("Invalid block ID/Name");
		valid_input = false;
	}

	GtkWidget* x_position_entry = gtk_grid_get_child_at((GtkGrid*)grid, 2, 1);
	GtkWidget* y_position_entry = gtk_grid_get_child_at((GtkGrid*)grid, 2, 2);
	GtkWidget* subtile_position_entry = gtk_grid_get_child_at((GtkGrid*)grid, 2, 3);
	
	x_location = std::atoi(gtk_entry_get_text((GtkEntry*)x_position_entry));
	y_location = std::atoi(gtk_entry_get_text((GtkEntry*)y_position_entry));
	subtile_location = std::atoi(gtk_entry_get_text((GtkEntry*)subtile_position_entry));


	//Check validity of the location (i.e. within bounds)
	auto& device_ctx = g_vpr_ctx.device();
	if(x_location < 0 || x_location > int(device_ctx.grid.width())) {
		invalid_breakpoint_entry_window("x value is out of bounds");
		valid_input = false;
	}
	if(y_location < 0 || y_location > int(device_ctx.grid.height())) {
		invalid_breakpoint_entry_window("y value is out of bounds");
		valid_input = false;
	}

	//Checking the subtile validity 
	if(subtile_location < 0 || subtile_location > int(device_ctx.grid[x_location][y_location].type->capacity)) {
		invalid_breakpoint_entry_window("Invalid subtile value");
		valid_input = false;
	}

	//Checks if all fields from the user input window are complete.
	if(std::string(gtk_entry_get_text((GtkEntry*)block_entry)).empty() || std::string(gtk_entry_get_text((GtkEntry*)x_position_entry)).empty() || std::string(gtk_entry_get_text((GtkEntry*)y_position_entry)).empty() || std::string(gtk_entry_get_text((GtkEntry*)subtile_position_entry)).empty()) {
		invalid_breakpoint_entry_window("Not all fields are complete");
		valid_input = false;
	}	

	if(valid_input) {
				
		manual_moves_global.manual_move_info.valid_input = true;
		manual_moves_global.manual_move_info.blockID = block_id;
		manual_moves_global.manual_move_info.x_pos = x_location;
		manual_moves_global.manual_move_info.y_pos = y_location;
		manual_moves_global.manual_move_info.subtile = subtile_location;

		std::cout << "valid input: " << manual_moves_global.manual_move_info.valid_input << std::endl;
		std::cout << "block id: " << manual_moves_global.manual_move_info.blockID << std::endl;
		std::cout << "x location: " << manual_moves_global.manual_move_info.x_pos << std::endl;
		std::cout << "y location: " << manual_moves_global.manual_move_info.y_pos << std::endl;
		std::cout << "subtile: " << manual_moves_global.manual_move_info.subtile << std::endl;

		manual_moves_global.manual_move_info.to_location = t_pl_loc(manual_moves_global.manual_move_info.x_pos, manual_moves_global.manual_move_info.y_pos, manual_moves_global.manual_move_info.subtile);
		//std::cout << "To location x: " << manual_moves_global.manual_move_info.to_location.x << std::endl;

		//Highlighting the block
		ClusterBlockId clb_index = ClusterBlockId(block_id);

		//Unselects all blocks first
		deselect_all();

		//Highlight block requested by user
		draw_highlight_blocks_color(cluster_ctx.clb_nlist.block_type(clb_index), clb_index);
		application.refresh_drawing();

	}	
	else {
		manual_moves_global.manual_move_info.valid_input = false;
	}
	
}

bool string_is_a_number(std::string block_id) {
	for(size_t i = 0; i < block_id.size(); i++) {
		//Returns 0 if the string does not have characters from 0-9
		if(isdigit(block_id[i]) == 0) { 
			return false;
		}
	}
	return true;
}

ManualMovesInfo* get_manual_move_info() {
	//returning the address to manual move info
	return &manual_moves_global.manual_move_info;
}

ManualMovesGlobals* get_manual_moves_global() {
	return &manual_moves_global;
}

//Manual Move Generator function
e_create_move ManualMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, float /*rlim*/) {

	std::cout << "In propose move function" << std::endl;

	int block_id = manual_moves_global.manual_move_info.blockID;
	t_pl_loc to = manual_moves_global.manual_move_info.to_location;

	//std::cout << "In propose move function: " << to.x << ", " << to.y << ", " << to.sub_tile << std::endl;

	ClusterBlockId b_from = ClusterBlockId(block_id);

	//Checking if the block was found
	if(!b_from) {
		return e_create_move::ABORT; //No movable block was found
	}

	auto& place_ctx = g_vpr_ctx.placement();
	auto& cluster_ctx = g_vpr_ctx.clustering();
	auto& device_ctx = g_vpr_ctx.device();

	//Gets the current location of the block to move.
	t_pl_loc from = place_ctx.block_locs[b_from].loc;
	std::cout << "The block to move is at: " << from.x << ", " << from.y << std::endl;
	auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
	auto grid_from_type = device_ctx.grid[from.x][from.y].type;
	VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));


	//Retrieving the compressed block grid for this block type
	const auto& compressed_block_grid = place_ctx.compressed_block_grids[cluster_from_type->index];

	//Checking if the block has a compatible subtile.
	auto to_type = device_ctx.grid[to.x][to.y].type;
	auto& compatible_subtiles = compressed_block_grid.compatible_sub_tiles_for_tile.at(to_type->index);

	//No compatible subtile is found.
	if(std::find(compatible_subtiles.begin(), compatible_subtiles.end(), to.sub_tile) == compatible_subtiles.end()) {
		return e_create_move::ABORT;
	}

	e_create_move create_move = ::create_move(blocks_affected, b_from, to);
	return create_move;
}

//Checks if the are any compatible subtiles with the moving block type. 
bool find_to_loc_manual_move(t_logical_block_type_ptr type, t_pl_loc& to) {
	
	//Retrieve the comporessed block grid for this block type
	const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[type->index];
	auto to_type = g_vpr_ctx.device().grid[to.x][to.y].type;
	auto& compatible_subtiles = compressed_block_grid.compatible_sub_tiles_for_tile.at(to_type->index);
	//Checking for the to subtile in a vector. No compatible subtile was found
	if(std::find(compatible_subtiles.begin(), compatible_subtiles.end(), to.sub_tile) != compatible_subtiles.end()) {
		return false;
	}
	return true;
}

void cost_summary() {

	std::cout << "\n";
	std::cout << "In cost summary window" << std::endl;
	std::cout << "Delta cost: " << manual_moves_global.manual_move_info.delta_cost << std::endl;
	std::cout << "Delta timing: " << manual_moves_global.manual_move_info.delta_timing << std::endl;
	std::cout << "Delta bouding box: " << manual_moves_global.manual_move_info.delta_bounding_box << std::endl;

	//if(!manual_moves_global.cost_window_is_open) {

	//Window settings
	GtkWidget* cost_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position((GtkWindow*)cost_window, GTK_WIN_POS_CENTER);
	gtk_window_set_title((GtkWindow*)cost_window, "Move Costs");

	//Creating grid and labels
	GtkWidget* grid = gtk_grid_new();
	GtkWidget* title = gtk_label_new(NULL);
	gtk_label_set_markup((GtkLabel*)title, "<b>Move Costs and Outcomes</b>");
	std::string delta_cost_label = "Delta Cost: " + std::to_string(manual_moves_global.manual_move_info.delta_cost);
	GtkWidget* dc = gtk_label_new(delta_cost_label.c_str());
	std::string delta_bounding_box_label = "Delta Bounding Box Cost: " + std::to_string(manual_moves_global.manual_move_info.delta_bounding_box);
	GtkWidget* dbbc = gtk_label_new(delta_bounding_box_label.c_str());
	std::string delta_timing_label = "Delta Timing: " +std::to_string(manual_moves_global.manual_move_info.delta_timing);
	GtkWidget* dtc = gtk_label_new(delta_timing_label.c_str());
	std::string outcome = e_move_result_to_string(manual_moves_global.manual_move_info.placer_move_outcome);
	std::string move_outcome_label = "Move outcome: " + outcome;
	GtkWidget* mo = gtk_label_new(move_outcome_label.c_str());

	//Adding buttons 
	GtkWidget* accept = gtk_button_new_with_label("Accept");
	GtkWidget* reject = gtk_button_new_with_label("Reject");

	//Attach to grid	
	gtk_grid_attach((GtkGrid*)grid, title, 0, 0, 2, 1);
	gtk_grid_attach((GtkGrid*)grid, dc, 0, 1, 2, 1);
	gtk_grid_attach((GtkGrid*)grid, dbbc, 0, 2, 2, 1);
	gtk_grid_attach((GtkGrid*)grid, dtc, 0, 3, 2, 1);
	gtk_grid_attach((GtkGrid*)grid, mo, 0, 4, 2, 1);
	gtk_grid_attach((GtkGrid*)grid, accept, 0, 5, 1, 1);
	gtk_grid_attach((GtkGrid*)grid, reject, 1, 5, 1, 1);

	//Set margins
	gtk_widget_set_halign(accept, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(reject, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_bottom(grid, 20);
	gtk_widget_set_margin_top(grid, 20);
	gtk_widget_set_margin_end(grid, 20);
	gtk_widget_set_margin_start(grid, 20);
	gtk_widget_set_margin_bottom(title, 15);
	gtk_widget_set_margin_bottom(dc, 5);
	gtk_widget_set_margin_bottom(dbbc, 5);
	gtk_widget_set_margin_bottom(dtc, 5);
	gtk_widget_set_margin_bottom(mo, 15);
	
	//Callback functions for the buttons.
	g_signal_connect(accept, "clicked", G_CALLBACK(accept_manual_move_window), cost_window);
	g_signal_connect(reject, "clicked", G_CALLBACK(reject_manual_move_window), cost_window);

	gtk_container_add(GTK_CONTAINER(cost_window), grid);
	
	if(manual_moves_global.mm_window_is_open) {
		gtk_widget_show_all(cost_window);
	}
}

void accept_manual_move_window(GtkWidget* /*widget*/, GtkWidget* cost_window) {

	std::cout << "User accepted manual move" << std::endl;
	//User accepts the manual move, and windows are destroyed.
	manual_moves_global.manual_move_info.user_move_outcome = ACCEPTED;
	manual_moves_global.mm_window_is_open = false;
	gtk_widget_destroy((GtkWidget*)cost_window);
	gtk_widget_destroy(manual_moves_global.manual_move_window);
}

void reject_manual_move_window(GtkWidget* /*widget*/, GtkWidget* cost_window) {

	std::cout << "User rejected manual move" << std::endl;
	//User rejects the manual move, and windows are destroyed.
	manual_moves_global.manual_move_info.user_move_outcome = REJECTED;
	manual_moves_global.mm_window_is_open = false;
	gtk_widget_destroy((GtkWidget*)cost_window);
	gtk_widget_destroy(manual_moves_global.manual_move_window);
}

void close_manual_moves_window() {
	manual_moves_global.mm_window_is_open = false;
}









#endif
