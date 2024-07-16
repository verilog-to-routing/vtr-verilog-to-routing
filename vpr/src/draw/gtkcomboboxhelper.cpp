#ifndef NO_GRAPHICS

#include "gtkcomboboxhelper.h"
#include <gtk/gtk.h>

/**
 * @brief Get the number of items in the combo box.
 * 
 * This function returns the number of items currently present in the combo box.
 * 
 * @param combo_box A pointer to the combo box widget.
 * @return The number of items in the combo box.
 */
static gint get_items_count(gpointer combo_box) {
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(combo_box);

    // Get the model of the combo box
    GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));

    // Get the number of items (indexes) in the combo box
    gint count = gtk_tree_model_iter_n_children(model, nullptr);
    return count;
}

/**
 * @brief Get the index of an item in a combo box by its text.
 * 
 * This function searches for an item with the specified text in the combo box
 * and returns its index if found.
 * 
 * @param combo_box A pointer to the combo box widget.
 * @param target_item The text of the item to search for.
 * @return The index of the item if found, or -1 if not found.
 */
gint get_item_index_by_text(gpointer combo_box, const gchar* target_item) {
    gint result_index = -1;
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(combo_box);

    // Get the model of the combo box
    GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));

    gchar* current_item_text = nullptr;

    for (gint index = 0; index < get_items_count(combo_box); ++index) {
        GtkTreeIter iter;

        // Check if the index is within bounds
        if (gtk_tree_model_iter_nth_child(model, &iter, nullptr, index)) {
            gtk_tree_model_get(model, &iter, 0, &current_item_text, -1);
            if (g_ascii_strcasecmp(target_item, current_item_text) == 0) {
                result_index = index;
                break;
            }
        }
    }

    g_free(current_item_text);
    return result_index;
}

#endif // NO_GRAPHICS