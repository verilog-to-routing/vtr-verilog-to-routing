#include "gtkcomboboxhelper.h"
#include <gtk/gtk.h>

gint get_item_count(gpointer combo_box) {
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(combo_box);

    // Get the model of the combo box
    GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));

    // Get the number of items (indexes) in the combo box
    gint count = gtk_tree_model_iter_n_children(model, NULL);
    return count;
}

gint get_item_index_by_text(gpointer combo_box, gchar* target_item) {
    gint result_index = -1;
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(combo_box);

    // Get the model of the combo box
    GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));

    gchar* current_item_text = nullptr;

    for (gint index=0; index<get_item_count(combo_box); ++index) {
        GtkTreeIter iter;

        // Check if the index is within bounds
        if (gtk_tree_model_iter_nth_child(model, &iter, NULL, index)) {
            gtk_tree_model_get(model, &iter, 0, &current_item_text, -1);
            if (g_strcmp0(target_item, current_item_text) == 0) {
                result_index = index;
                break;
            }
        }
    }

    g_free(current_item_text);
    return result_index;
}
