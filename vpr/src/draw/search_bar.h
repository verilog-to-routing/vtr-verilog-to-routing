#ifndef SEARCH_BAR_H
#define SEARCH_BAR_H

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"
#    include "draw_color.h"

void search_and_highlight(GtkWidget* /*widget*/, ezgl::application* app);
bool highlight_rr_nodes(int hit_node);
void auto_zoom_rr_node(int rr_node_id);
void highlight_cluster_block(ClusterBlockId clb_index);
void highlight_nets(ClusterNetId net_id);
void highlight_nets(std::string net_name);

void load_block_names(ezgl::application* app);
void load_net_names(ezgl::application* app);

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

void key_length_val_changed(GtkSpinButton* self, ezgl::application* app);

t_pb* find_atom_block_in_pb(std::string name, t_pb* pb);

bool highlight_atom_block(AtomBlockId atom_blk, ClusterBlockId cl_blk, ezgl::application* app);
#endif /* NO_GRAPHICS */

#endif /* SEARCH_BAR_H */
