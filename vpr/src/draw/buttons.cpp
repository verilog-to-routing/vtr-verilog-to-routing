#ifndef NO_GRAPHICS

/*
 * This file defines all runtime created spin buttons, combo boxes, and labels. All 
 * button_for_toggle_X() functions are called in initial_setup_X functions in draw.cpp.
 * Each function creates a label and a combo box/spin button and connects the signal to 
 * the corresponding toggle_X callback functions, which are also defined in draw.cpp.
 * 
 * Authors: Dingyu (Tina) Yang
 * Last updated: Aug 2019
 */

#include "draw.h"
#include "buttons.h"

#include "ezgl/application.hpp"

//location of spin buttons, combo boxes, and labels on grid
int box_width = 1;
int box_height = 1;
int label_left_start_col = 0;
int box_left_start_col = 0;
int button_row = 2; // 2 is the row num of the window button in main.ui, add buttons starting from this row

void delete_button(const std::string& button_name) {
    QWidget* main_window_grid = application->find_widget("InnerGrid");
    QList<QWidget*> list_of_widgets = ezgl::widget_get_direct_children(main_window_grid);
    QWidget* target_button = nullptr;

    // loop through the list to find the button
    for (QWidget* widget : list_of_widgets) {
        if (widget->objectName().toStdString() == button_name) {
            // found text entry
            target_button = widget;
            break;
        }
    }

    if (target_button)
        target_button->deleteLater();
}

QWidget* find_button(const std::string& button_name) {
    QWidget* main_window_grid = application->find_widget("InnerGrid");
    QList<QWidget*> list_of_widgets = ezgl::widget_get_direct_children(main_window_grid);

    // loop through the list to find the button
    for (QWidget* widget : list_of_widgets) {
        if (widget->objectName().toStdString() == button_name) {
            return widget;
        }
    }

    return nullptr;
}

#endif /* NO_GRAPHICS */
