#pragma once

#include <QApplication>
#include <QWidget>
#include <QString>

/**
 * Find a widget by objectName across ALL top-level widgets (including popover
 * QFrames that are not parented to the QMainWindow).
 *
 * This mirrors ezgl::application::get_object() which uses
 * QApplication::allWidgets() — necessary because QtGladeLoader creates
 * GtkPopover widgets as top-level Qt::Popup frames with no parent.
 */
template<typename T = QWidget>
T* findWidgetByName(const char* name) {
    for (QWidget* w : QApplication::allWidgets()) {
        if (w->objectName() == name) {
            return qobject_cast<T*>(w);
        }
    }
    return nullptr;
}
