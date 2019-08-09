#include <cstdio>
#include <sstream>
using namespace std;

#include "globals.h"
#include "draw.h"
#include "draw_global.h"
#include "save_graphics.h"
#include "vtr_path.h"




void save_graphics_from_command_option() {  
    t_draw_state* draw_state = get_draw_state_vars();
    
    if (draw_state->save_graphics == "none")
        return;
    
    std::array<std::string, 2> input_data = vtr::split_ext(draw_state->save_graphics);
    
    std::string file_name = input_data.front(); 
    std::string extension = input_data.back();
    
    // reset the status 
    draw_state->save_graphics = "none";
    
    save_graphics(extension, file_name);
    
}

void save_graphics_from_button(GtkWidget* /*widget*/, gint response_id, gpointer data) {  
    auto dialog = static_cast<GtkWidget *>(data);
    
    if(response_id == GTK_RESPONSE_ACCEPT) {
        //user clicked on the save button
        
        GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GList* list_of_widgets = gtk_container_get_children(GTK_CONTAINER(content_area));
        GtkWidget* combo_box = NULL;
        GtkWidget* text_entry = NULL;
        std::string file_name; 
        std::string extension;

        // loop through the list to find the combo box and the text entry
        GList* current = list_of_widgets;
        while (current != NULL) {
            GList* next = current->next;
            if(strcmp(gtk_widget_get_name(static_cast<GtkWidget *>(current->data)),"file_name_text_entry") == 0){
                // found text entry
                text_entry = static_cast<GtkWidget *>(current->data);
            }
            else if(strcmp(gtk_widget_get_name(static_cast<GtkWidget *>(current->data)),"file_name_combo_box") == 0){
                // found combo box
                combo_box = static_cast<GtkWidget *>(current->data);
            }
            current = next;
        }

        // get the data from the text entry and combo box
        file_name = gtk_entry_get_text(GTK_ENTRY(text_entry));
        extension = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_box));
        extension = "." + extension;
        
        //save the graphics
        save_graphics(extension, file_name);

        // free dynamically allocated list
        g_list_free(list_of_widgets);
    }
    // free widget
    gtk_widget_destroy(dialog);    
}

void save_graphics(std::string &extension, std::string &file_name){
    if (extension == ".pdf"){
        application.get_canvas(application.get_main_canvas_id())->print_pdf(file_name.c_str());
        return;
    }
    else if (extension == ".png") {
        application.get_canvas(application.get_main_canvas_id())->print_png(file_name.c_str());
        return;
    }
    else if (extension == ".svg") {
        application.get_canvas(application.get_main_canvas_id())->print_svg(file_name.c_str());
        return;
    }
    else {
        //invalid input format
        ;
    }
}


void save_graphics_dialog_box(GtkWidget* /*widget*/, ezgl::application* /*app*/) {
    GObject* main_window; 
    GtkWidget* content_area; 
    GtkWidget* text_entry; 
    GtkWidget* label;   
    GtkWidget* dialog;
    GtkWidget* combo_box;
    
    // get a pointer to the main window
    main_window = application.get_object(application.get_main_window_id().c_str());
    
    // create a dialog window modal
    dialog = gtk_dialog_new_with_buttons("Save Graphics Contents",
            GTK_WINDOW(main_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            ("_Save"),
            GTK_RESPONSE_ACCEPT,
            ("_Cancel"),
            GTK_RESPONSE_REJECT,
            NULL);
    
    // create elements
    label = gtk_label_new("Enter file name:");
    text_entry = gtk_entry_new();
    combo_box = gtk_combo_box_text_new();
    
    // set name for text entry and combo box for later data extraction
    gtk_widget_set_name (text_entry, "file_name_text_entry");
    gtk_widget_set_name (combo_box, "file_name_combo_box");
    
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "pdf");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "png");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "svg");
    
    g_signal_connect_swapped(GTK_DIALOG(dialog),
                             "response",
                             G_CALLBACK(save_graphics_from_button),
                             GTK_DIALOG(dialog));
    
    // attach elements to the content area of the dialog
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content_area), label);
    gtk_container_add(GTK_CONTAINER(content_area), text_entry);
    gtk_container_add(GTK_CONTAINER(content_area), combo_box);
    
    // show the label & child widget of the dialog
    gtk_widget_show_all(dialog);
    
    return;
}