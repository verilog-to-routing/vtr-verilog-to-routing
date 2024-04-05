#ifndef GTKCOMBOBOXHELPER_H
#define GTKCOMBOBOXHELPER_H

#ifndef NO_GRAPHICS

#include <glib.h>

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
gint get_item_index_by_text(gpointer combo_box, const gchar* target_item);

#endif // NO_GRAPHICS

#endif // GTKCOMBOBOXHELPER_H
