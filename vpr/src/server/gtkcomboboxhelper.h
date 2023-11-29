#ifndef GTKCOMBOBOXHELPER_H
#define GTKCOMBOBOXHELPER_H

#include <glib.h>

gint get_item_count(gpointer combo_box);

gint get_item_index_by_text(gpointer combo_box, gchar* target_item);

#endif // GTKCOMBOBOXHELPER_H
