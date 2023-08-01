/**
 * @file search_bar.h
 * 
 * This file essentially follows the whole search process, from searching, finding the match,
 * and finally highlighting the searched for item. Also includes auto-complete functionality/matching functions.
 * 
 * Author: Sebastian Lievano
 */

#ifndef SEARCH_BAR_H
#define SEARCH_BAR_H

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"
#    include "draw_color.h"

void search_and_highlight(GtkWidget* /*widget*/, ezgl::application* app);
bool highlight_rr_nodes(RRNodeId hit_node);
void auto_zoom_rr_node(RRNodeId rr_node_id);
void highlight_cluster_block(ClusterBlockId clb_index);
void highlight_nets(ClusterNetId net_id);
void highlight_nets(std::string net_name);

void highlight_atom_block(AtomBlockId block_id);

gboolean customMatchingFunction(
    GtkEntryCompletion* completer,
    const gchar* key,
    GtkTreeIter* iter,
    gpointer user_data);

//Function to manage entry completions when search type is changed
void search_type_changed(GtkComboBox* /*self*/, ezgl::application* app);

/*function below pops up a dialog box with no button, showing the input warning message*/
void warning_dialog_box(const char* message);

//Highlights atom block in cluster block
bool highlight_atom_block(AtomBlockId atom_blk, ClusterBlockId cl_blk, ezgl::application* app);

//Turns on autocomplete/suggestions
void enable_autocomplete(ezgl::application* app);

//Simulates key press event
GdkEvent simulate_keypress(char key, GdkWindow* window);

//Returns current search type
std::string get_search_type(ezgl::application* app);
#endif /* NO_GRAPHICS */

#endif /* SEARCH_BAR_H */