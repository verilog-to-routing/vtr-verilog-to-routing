#ifndef NO_GRAPHICS

#include <cstdio>

#include "draw.h"
#include "save_graphics.h"
#include "search_bar.h"

#include <ezgl/qt/qtutils.hpp>

#include <QLineEdit>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>

extern ezgl::rectangle initial_world;

void save_graphics_from_button(QDialog* dialog) {
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

    auto canvas = application->get_canvas(application->get_main_canvas_id());

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

void save_graphics_dialog_box(QWidget* /*widget*/, ezgl::application* /*app*/) {
    QWidget* main_window = application->find_widget(application->get_main_window_id().c_str());
    QDialog* dialog = new QDialog(main_window);
    dialog->setWindowTitle("Save Graphics Contents");
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    auto* layout = new QVBoxLayout(dialog);

    // create elements
    auto* name_label = new QLabel("File name:");
    auto* text_entry = new QLineEdit;
    auto* type_label = new QLabel("File format:");
    auto* combo_box = new QComboBox;

    // set name for text entry and combo box for later data extraction
    text_entry->setObjectName("file_name_text_entry");
    combo_box->setObjectName("file_name_combo_box");

    combo_box->addItem("pdf"); // index 0
    combo_box->addItem("png"); // index 1
    combo_box->addItem("svg"); // index 2

    // set default values
    combo_box->setCurrentIndex(0);
    text_entry->setText("vpr_graphics");

    layout->addWidget(name_label);
    layout->addWidget(text_entry);
    layout->addWidget(type_label);
    layout->addWidget(combo_box);

    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, dialog);
    layout->addWidget(buttonBox);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, dialog, [dialog]() {
        save_graphics_from_button(dialog);
        dialog->accept();
    });
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    dialog->show();
}

#endif /* NO_GRAPHICS */
