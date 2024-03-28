#ifndef GTKCOMBOBOXHELPER_H
#define GTKCOMBOBOXHELPER_H

#include <glib.h>

/**
 * @brief Helper function to retrieve the index of an item by its text.
 * Returns -1 if the item with the specified text is absent.
 */
gint get_item_index_by_text(gpointer combo_box, const gchar* target_item);

#endif // GTKCOMBOBOXHELPER_H
