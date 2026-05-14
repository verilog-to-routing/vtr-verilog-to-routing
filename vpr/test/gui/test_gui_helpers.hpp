#pragma once

#include <QApplication>
#include <QWidget>
#include <QString>
#include <QPoint>
#include <QtTest/QTest>

/**
 * Find a widget by objectName across QApplication::allWidgets().
 *
 * Mirrors ezgl::application::find_widget(), which uses the same lookup
 * so that production callers can find widgets without knowing the
 * QMainWindow pointer. Useful in tests that have access to the
 * application singleton but not the loaded window.
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

// ---------------------------------------------------------------------------
// Layer 4 — interactive event simulation helpers (S6 of the GUI test plan)
//
// Thin wrappers over QTest:: that give the Layer 4 tests a stable surface
// even if QTest's API shifts between Qt 6 minor versions. They post events
// synchronously to the target widget (no main-loop spin required when running
// under QT_QPA_PLATFORM=offscreen).
// ---------------------------------------------------------------------------

inline void simulate_mouse_move(QWidget* w, QPoint pos) {
    QTest::mouseMove(w, pos);
}

inline void simulate_mouse_click(QWidget* w, QPoint pos, Qt::MouseButton b = Qt::LeftButton, Qt::KeyboardModifiers m = Qt::NoModifier) {
    QTest::mouseClick(w, b, m, pos);
}

inline void simulate_key(QWidget* w, Qt::Key k, Qt::KeyboardModifiers m = Qt::NoModifier) {
    QTest::keyClick(w, k, m);
}
