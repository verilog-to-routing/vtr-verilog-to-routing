#ifndef NO_GRAPHICS

#include <cstdio>

#include "draw.h"
#include "save_graphics.h"
#include "search_bar.h"

extern ezgl::rectangle initial_world;

void save_graphics_from_button(GtkWidget* /*widget*/, [[maybe_unused]] gint response_id, gpointer data) {
    QDialog* dialog = Q_DIALOG(static_cast<QObject*>(data));
    QLineEdit* text_entry = dialog->findChild<QLineEdit*>("file_name_text_entry");
    QComboBox* combo_box = dialog->findChild<QComboBox*>("file_name_combo_box");
    if (text_entry && combo_box) {
        std::string file_name = text_entry->text().toStdString();
        std::string extension = combo_box->currentText().toStdString();
        save_graphics(extension, file_name);
    }
}

void save_graphics(std::string extension, std::string file_name) {
    //Trim any leading '.' from the extension
    if (vtr::starts_with(extension, ".")) {
        extension = std::string(extension.begin() + 1, extension.end());
    }

    auto canvas = application.get_canvas(application.get_main_canvas_id());

    bool result = true;

    file_name = file_name + "." + extension;
    if (extension == "pdf") {
        result = canvas->print_pdf(file_name.c_str(), initial_world.width(), initial_world.height());
    } else if (extension == "png") {
        constexpr int IMAGE_WIDTH_PIXELS = 2048;
        int image_height_pixels = IMAGE_WIDTH_PIXELS * float(initial_world.height()) / initial_world.width();
        result = canvas->print_png(file_name.c_str(), IMAGE_WIDTH_PIXELS, image_height_pixels);
    } else if (extension == "svg") {
        result = canvas->print_svg(file_name.c_str(), initial_world.width(), initial_world.height());
    } else {
        warning_dialog_box("Invalid file type");
    }

    VTR_ASSERT_MSG(result == true, "Failed to save graphics");
}

void save_graphics_dialog_box(GtkWidget* /*widget*/, ezgl::application* /*app*/) {
    GObject* main_window;
    GtkWidget* content_area;
    GtkWidget* text_entry;
    GtkWidget* name_label;
    GtkWidget* type_label;
    GtkWidget* dialog;
    GtkWidget* combo_box;

    // get a pointer to the main window
    main_window = application.get_object(application.get_main_window_id().c_str());
    dialog = new QDialog(Q_WIDGET(main_window));
    dialog->setWindowTitle("Save Graphics Contents");
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // create elements
    name_label = gtk_label_new("File name:");
    text_entry = gtk_entry_new();
    type_label = gtk_label_new("File format:");
    combo_box = gtk_combo_box_text_new();

    // set name for text entry and combo box for later data extraction
    gtk_widget_set_name(text_entry, "file_name_text_entry");
    gtk_widget_set_name(combo_box, "file_name_combo_box");

    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "pdf"); // index 0
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "png"); // index 1
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "svg"); // index 2

    // set default values
    gtk_combo_box_set_active((GtkComboBox*)combo_box, 0);      // default set to pdf which has an index 0
    gtk_entry_set_text((GtkEntry*)text_entry, "vpr_graphics"); // default text set to vpr_graphics

    // attach elements to the content area of the dialog
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content_area), name_label);
    gtk_container_add(GTK_CONTAINER(content_area), text_entry);
    gtk_container_add(GTK_CONTAINER(content_area), type_label);
    gtk_container_add(GTK_CONTAINER(content_area), combo_box);

    // show the label & child widget of the dialog
    gtk_widget_show_all(dialog);

    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, dialog);
    gtk_container_add(dialog, buttonBox);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, dialog, [dialog]() {
        save_graphics_from_button(dialog, 0, dialog);
        Q_DIALOG(dialog)->accept();
    });
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, Q_DIALOG(dialog), &QDialog::reject);
    return;
}

#endif /* NO_GRAPHICS */
